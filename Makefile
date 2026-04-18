BUILD_DIR ?= build
CTEST_ARGS ?= --output-on-failure

.PHONY: all configure build test test-verbose test-one clean reconfigure

all: test

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) -j

test: build
	ctest --test-dir $(BUILD_DIR) $(CTEST_ARGS)

test-verbose: build
	ctest --test-dir $(BUILD_DIR) -V

test-one: build
	@if [ -z "$(NAME)" ]; then \
		echo "Usage: make test-one NAME=test_dstr"; \
		exit 1; \
	fi
	ctest --test-dir $(BUILD_DIR) -R "^$(NAME)$$" --output-on-failure

reconfigure:
	cmake -S . -B $(BUILD_DIR) --fresh

clean:
	rm -rf $(BUILD_DIR)
