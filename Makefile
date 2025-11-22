# Compiler and flags
CC = gcc
LLVM_CONFIG = llvm-config-18
CFLAGS = -Wall -Wextra -std=c11 -O2 $(shell $(LLVM_CONFIG) --cflags)
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --libs core --system-libs)

# Target executable
TARGET = parser

# Source files
SRCS = main.c lexer.c syntax.c codegen.c symbol_table.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = lexer.h syntax.h symbol_table.h

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild from scratch
rebuild: clean all

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug build (no optimization, with debug symbols)
debug: CFLAGS = -Wall -Wextra -std=c11 -g -DDEBUG -O0 $(shell $(LLVM_CONFIG) --cflags)
debug: clean $(TARGET)

# Phony targets
.PHONY: all clean rebuild run debug
