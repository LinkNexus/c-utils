CC = gcc
CFLAGS = -g -Wall -O0 -I. -MMD -MP

BUILD_DIR = build

TEST_SRCS = $(wildcard tests/*.c)
TEST_BINS = $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))
DEPS = $(TEST_BINS:.bin=)

all: compile_commands.json $(TEST_BINS)

$(BUILD_DIR)/%: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

-include $(TEST_BINS:=.d)

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
