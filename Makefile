CC = gcc
CFLAGS = -g -Wall -O0 -I.

BUILD_DIR = build

TEST_SRCS = $(wildcard tests/*.c)
TEST_BINS = $(patsubst tests/%.c,$(BUILD_DIR)/test_%,$(TEST_SRCS))

all: compile_commands.json test

$(BUILD_DIR)/test_%: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

test: $(TEST_BINS)
	@for bin in $(TEST_BINS); do \
		echo "--- Running $$bin ---"; \
		./$$bin; \
	done

compile_commands.json:
	@echo "Updating compilation database..."
	@compiledb -n make test

clean:
	rm -rf $(BUILD_DIR) compile_commands.json

.PHONY: all test clean
