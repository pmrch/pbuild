# Compiler choice
CC = gcc

# Strict Flag Collection
# -Werror: Turn warnings into errors
# -Wpedantic: Reject everything that isn't ISO C
# -Wextra: The "extra" warnings often missed by -Wall
STRICT_FLAGS_GCC = -Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wmaybe-uninitialized \
	-Wconversion -Wsign-conversion -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -Wpointer-arith \
	-Warray-bounds -Wnull-dereference -Wmissing-prototypes -Wstrict-prototypes \
	-Wold-style-definition -Wredundant-decls -Wshadow -Wundef -Wformat=2 -Wformat-security \
	-Wwrite-strings -Wvla -Wdouble-promotion -Wfloat-equal -Wswitch-enum -Wswitch-default \
	-Wunused -Wunused-function -Wunused-variable -Wunused-parameter -Wduplicated-cond \
	-Wduplicated-branches -Wlogical-op -Wno-padded -Wno-declaration-after-statement 

STRICT_FLAGS_CLANG = -Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wold-style-definition          \
	-Wsign-conversion -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -Wpointer-arith -Warray-bounds      \
	-Wnull-dereference -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wredundant-decls -Wvla   \
	-Wshadow -Wundef -Wformat=2 -Wformat-security -Wwrite-strings -Wdouble-promotion -Wfloat-equal     \
	-Wswitch-enum -Wswitch-default -Wunused -Wunused-function -Wunused-variable -Wunused-parameter     \
	-Wno-padded -Wno-declaration-after-statement -Weverything -Wno-jump-misses-init -Wno-unsafe-buffer-usage \
	-Wno-disabled-macro-expansion -Wno-unknown-warning-option

# Combine with standard flags and optimization
CFLAGS = -std=c23 $(STRICT_FLAGS_GCC) -Iinclude -MMD -MP -O3 -march=native -flto -ffast-math

# Linker flags (for libraries)
LDFLAGS = -flto

# Target definition
SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
TARGET = pbuild

# Target tests definition
# Test definition (Automatically finds all test files in tests/)
TEST_SRC := $(shell find tests -name '*.c')
TEST_BIN := $(patsubst tests/%.c,build/tests/%,$(TEST_SRC))

# Main application target
$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

print-version:
	@$(CC) --version

# Compile core source files into build/src/
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Automatically compile and link each test file with necessary core objects
# (Excludes main.o from SRC objects if your main function is in src/main.c)
build/tests/%: tests/%.c $(filter-out build/src/main.o, $(OBJ))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	@echo "Runnin all automated tests..."
	@for t in $(TEST_BIN); do \
		echo "==============================================="; \
		./$$t || exit 1; \
	done
	
clean:
	rm -f $(TARGET)
	rm -rf compile_commands.json
	rm -rf build

all: compile_commands $(TARGET)

.PHONY: clean test print-version all compile_commands
compile_commands:
	bear -- make