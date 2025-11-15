#include "symbol_table.h"

unsigned int hash(const char *key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + *key++;
    }
    return hash % TABLE_SIZE;
}

HashMap* create_map() {
    HashMap *map = malloc(sizeof(HashMap));
    for (int i = 0; i < TABLE_SIZE; i++) {
        map->buckets[i] = NULL;
    }
    return map;
}

void map_put(HashMap *map, const char *key, LLVMValueRef value) {
    unsigned int index = hash(key);
    Value *entry = map->buckets[index];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    Value *new_entry = malloc(sizeof(Value));
    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
}

LLVMValueRef map_get(HashMap *map, const char *key){
    unsigned int index = hash(key);
    Value *entry = map->buckets[index];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}
