#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define MAX_HEIGHT 1000
int lprofile[MAX_HEIGHT];
int rprofile[MAX_HEIGHT];
#define INFINITY (1<<20)

typedef int ELEMENT;

typedef struct BSTNode_struct BSTNode;

struct BSTNode_struct {
    BSTNode *ptLeft, *ptRight;

    //length of the edge from this node to its children
    int edge_length;

    int height;

    ELEMENT element;

    //-1=I am left, 0=I am root, 1=right
    int parent_dir;

    //max supported unit32 in dec, 10 digits max
    char label[11];
};

typedef struct Tree Tree;

struct Tree {
    Tree *left, *right;
    int element;
};

Tree *find_min(Tree *t) {
    if (t == NULL) {
        return NULL;
    }
    else if (t->left == NULL) {
        return t;
    }
    else {
        return find_min(t->left);
    }
}

Tree *find_max(Tree *t) {
    if (t == NULL) {
        return NULL;
    }
    else if (t->right == NULL) {
        return t;
    }
    else {
        return find_max(t->right);
    }
}

Tree *find(int elem, Tree *t) {
    if (t == NULL) {
        return NULL;
    }

    if (elem < t->element) {
        return find(elem, t->left);
    }
    else if (elem > t->element) {
        return find(elem, t->right);
    }
    else {
        return t;
    }
}

//Insert i into the tree t, duplicate will be discarded
//Return a pointer to the resulting tree.
Tree *insert(int value, Tree *t) {
    Tree *new_node;

    if (t == NULL) {
        new_node = (Tree *) malloc(sizeof(Tree));
        if (new_node == NULL) {
            return t;
        }

        new_node->element = value;

        new_node->left = new_node->right = NULL;
        return new_node;
    }

    if (value < t->element) {
        t->left = insert(value, t->left);
    }
    else if (value > t->element) {
        t->right = insert(value, t->right);
    }
    else {
        //duplicate, ignore it
        return t;
    }
    return t;
}


//adjust gap between left and right nodes
int gap = 3;

//used for printing next node in the same level,
//this is the x coordinate of the next char printed
int print_next;

int MIN(int X, int Y) {
    return ((X) < (Y)) ? (X) : (Y);
}

int MAX(int X, int Y) {
    return ((X) > (Y)) ? (X) : (Y);
}

BSTNode *build_ascii_tree_recursive(Tree *t) {
    BSTNode *node;

    if (t == NULL) return NULL;

    node = malloc(sizeof(BSTNode));
    node->ptLeft = build_ascii_tree_recursive(t->left);
    node->ptRight = build_ascii_tree_recursive(t->right);

    if (node->ptLeft != NULL) {
        node->ptLeft->parent_dir = -1;
    }

    if (node->ptRight != NULL) {
        node->ptRight->parent_dir = 1;
    }

    sprintf(node->label, "%d", t->element);
    node->element = strlen(node->label);

    return node;
}


//Copy the tree into the ascii node structre
BSTNode *build_ascii_tree(Tree *t) {
    BSTNode *node;
    if (t == NULL) return NULL;
    node = build_ascii_tree_recursive(t);
    node->parent_dir = 0;
    return node;
}

//Free all the nodes of the given tree
void free_ascii_tree(BSTNode *node) {
    if (node == NULL) return;
    free_ascii_tree(node->ptLeft);
    free_ascii_tree(node->ptRight);
    free(node);
}

//The following function fills in the lprofile array for the given tree.
//It assumes that the center of the label of the root of this tree
//is located at a position (x,y).  It assumes that the edge_length
//fields have been computed for this tree.
void compute_lprofile(BSTNode *node, int x, int y) {
    int i, isleft;
    if (node == NULL) return;
    isleft = (node->parent_dir == -1);
    lprofile[y] = MIN(lprofile[y], x - ((node->element - isleft) / 2));
    if (node->ptLeft != NULL) {
        for (i = 1; i <= node->edge_length && y + i < MAX_HEIGHT; i++) {
            lprofile[y + i] = MIN(lprofile[y + i], x - i);
        }
    }
    compute_lprofile(node->ptLeft, x - node->edge_length - 1, y + node->edge_length + 1);
    compute_lprofile(node->ptRight, x + node->edge_length + 1, y + node->edge_length + 1);
}

void compute_rprofile(BSTNode *node, int x, int y) {
    int i, notleft;
    if (node == NULL) return;
    notleft = (node->parent_dir != -1);
    rprofile[y] = MAX(rprofile[y], x + ((node->element - notleft) / 2));
    if (node->ptRight != NULL) {
        for (i = 1; i <= node->edge_length && y + i < MAX_HEIGHT; i++) {
            rprofile[y + i] = MAX(rprofile[y + i], x + i);
        }
    }
    compute_rprofile(node->ptLeft, x - node->edge_length - 1, y + node->edge_length + 1);
    compute_rprofile(node->ptRight, x + node->edge_length + 1, y + node->edge_length + 1);
}

//This function fills in the edge_length and
//height fields of the specified tree
void filledge(BSTNode *node) {
    int h, hmin, i, delta;
    if (node == NULL) return;
    filledge(node->ptLeft);
    filledge(node->ptRight);

    /* first fill in the edge_length of node */
    if (node->ptRight == NULL && node->ptLeft == NULL) {
        node->edge_length = 0;
    }
    else {
        if (node->ptLeft != NULL) {
            for (i = 0; i < node->ptLeft->height && i < MAX_HEIGHT; i++) {
                rprofile[i] = -INFINITY;
            }
            compute_rprofile(node->ptLeft, 0, 0);
            hmin = node->ptLeft->height;
        }
        else {
            hmin = 0;
        }
        if (node->ptRight != NULL) {
            for (i = 0; i < node->ptRight->height && i < MAX_HEIGHT; i++) {
                lprofile[i] = INFINITY;
            }
            compute_lprofile(node->ptRight, 0, 0);
            hmin = MIN(node->ptRight->height, hmin);
        }
        else {
            hmin = 0;
        }
        delta = 4;
        for (i = 0; i < hmin; i++) {
            delta = MAX(delta, gap + 1 + rprofile[i] - lprofile[i]);
        }

        //If the node has two children of height 1, then we allow the
        //two leaves to be within 1, instead of 2
        if (((node->ptLeft != NULL && node->ptLeft->height == 1) ||
             (node->ptRight != NULL && node->ptRight->height == 1)) && delta > 4) {
            delta--;
        }

        node->edge_length = ((delta + 1) / 2) - 1;
    }

    //now fill in the height of node
    h = 1;
    if (node->ptLeft != NULL) {
        h = MAX(node->ptLeft->height + node->edge_length + 1, h);
    }
    if (node->ptRight != NULL) {
        h = MAX(node->ptRight->height + node->edge_length + 1, h);
    }
    node->height = h;
}

//This function prints the given level of the given tree, assuming
//that the node has the given x cordinate.
void printLevel(BSTNode *node, int x, int level) {
    int i, isleft;
    if (node == NULL) return;
    isleft = (node->parent_dir == -1);
    if (level == 0) {
        for (i = 0; i < (x - print_next - ((node->element - isleft) / 2)); i++) {
            printf(" ");
        }
        print_next += i;
        printf("%s", node->label);
        print_next += node->element;
    }
    else if (node->edge_length >= level) {
        if (node->ptLeft != NULL) {
            for (i = 0; i < (x - print_next - (level)); i++) {
                printf(" ");
            }
            print_next += i;
            printf("/");
            print_next++;
        }
        if (node->ptRight != NULL) {
            for (i = 0; i < (x - print_next + (level)); i++) {
                printf(" ");
            }
            print_next += i;
            printf("\\");
            print_next++;
        }
    }
    else {
        printLevel(node->ptLeft,
                   x - node->edge_length - 1,
                   level - node->edge_length - 1);
        printLevel(node->ptRight,
                   x + node->edge_length + 1,
                   level - node->edge_length - 1);
    }
}

//prints ascii tree for given Tree structure
void printElements(Tree *t) {
    BSTNode *proot;
    int xmin, i;
    if (t == NULL) return;
    proot = build_ascii_tree(t);
    filledge(proot);
    for (i = 0; i < proot->height && i < MAX_HEIGHT; i++) {
        lprofile[i] = INFINITY;
    }
    compute_lprofile(proot, 0, 0);
    xmin = 0;
    for (i = 0; i < proot->height && i < MAX_HEIGHT; i++) {
        xmin = MIN(xmin, lprofile[i]);
    }
    for (i = 0; i < proot->height; i++) {
        print_next = 0;
        printLevel(proot, -xmin, i);
        printf("\n");
    }
    if (proot->height >= MAX_HEIGHT) {
        printf("(This tree is taller than %d, and may be drawn incorrectly.)\n", MAX_HEIGHT);
    }
    free_ascii_tree(proot);
}


//driver
int main() {
    //A sample use of these functions.  Start with the empty tree
    //insert some stuff into it, and then delete it
    Tree *root;
    int i;
    root = NULL;

//    make_empty(root);

    printf("\nAfter inserting element 10..\n");
    root = insert(10, root);
    printElements(root);

    printf("\nAfter inserting element 5..\n");
    root = insert(5, root);
    printElements(root);

    printf("\nAfter inserting element 20..\n");
    root = insert(20, root);
    printElements(root);

    printf("\nAfter inserting elements 7, 14, 28..\n");
    root = insert(7, root);
    root = insert(14, root);
    root = insert(28, root);
    printElements(root);

    printf("\nAfter inserting element 3.\n");
    root = insert(3, root);

 ;
    printElements(root);
    return 0;

}
