CC ?= clang
BUILD := build
SRC := src/dialogue_system.c
OBJ := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
CFLAGS := -std=c23 -Wall -Wextra -pedantic -Iinclude $(shell pkg-config --cflags lua)

.PHONY: all link clean lint format check test

$(BUILD):
	mkdir -p $(BUILD)

UNITY_DIR := vendor/Unity/src
UNITY_CFLAGS := -I$(UNITY_DIR)
TEST_CFLAGS := $(CFLAGS) $(UNITY_CFLAGS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/libdialogues.a: $(OBJ)
	ar rcs $@ $^
	ranlib $@

all: $(BUILD)/libdialogues.a

$(BUILD)/unity.o: $(UNITY_DIR)/unity.c | $(BUILD)
	$(CC) $(CFLAGS) $(UNITY_CFLAGS) -c -o $@ $<

$(BUILD)/test_dialogue_system.o: tests/test_dialogue_system.c | $(BUILD)
	$(CC) $(TEST_CFLAGS) -c -o $@ $<

$(BUILD)/test_runner: $(BUILD)/test_dialogue_system.o $(BUILD)/unity.o $(BUILD)/libdialogues.a
	$(CC) -o $@ $(BUILD)/test_dialogue_system.o $(BUILD)/unity.o $(BUILD)/libdialogues.a $(shell pkg-config --libs lua)

test: $(BUILD)/test_runner
	./$(BUILD)/test_runner

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
