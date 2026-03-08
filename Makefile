CC ?= clang
BUILD := build
SRC := src/dialogue_system.c
OBJ := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
CFLAGS := -std=c23 -Wall -Wextra -pedantic -Iinclude $(shell pkg-config --cflags lua)

.PHONY: all link clean lint format check

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/libdialogues.a: $(OBJ)
	ar rcs $@ $^
	ranlib $@

all: $(BUILD)/libdialogues.a

clean:
	rm -rf $(BUILD)

# Lint with clang-tidy (install: brew install llvm)
lint:
	clang-tidy $(SRC) -- $(CFLAGS)

# Format with clang-format
format:
	clang-format -i $(SRC)

# Check formatting without modifying (CI)
check:
	clang-format --dry-run --Werror $(SRC)
