CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes \
          -Wmissing-prototypes -Wformat=2
CPPFLAGS += -Isrc
LDLIBS += -lm

BIN := simpleisp
LIB_SRCS := $(wildcard src/core/*.c) $(wildcard src/io/*.c) \
            $(wildcard src/isp/*.c)
SRCS := $(LIB_SRCS) src/main.c
HEADERS := $(wildcard src/core/*.h) $(wildcard src/io/*.h) \
           $(wildcard src/isp/*.h)
TEST_BIN := build/test_isp
SAN_BIN := build/test_isp_san
COMPILE_DB := compile_commands.json

.PHONY: all test sanitize compiledb clean

all: $(BIN)

$(BIN): $(SRCS) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SRCS) $(LDLIBS)

$(TEST_BIN): tests/test_isp.c $(LIB_SRCS) $(HEADERS)
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_isp.c $(LIB_SRCS) $(LDLIBS)

test: $(BIN) $(TEST_BIN)
	./$(TEST_BIN)

# Generate a compilation database for clangd and other language servers.
# Uses only the shell tooling; no bear/compiledb package required.
$(COMPILE_DB): tools/gen-compile-commands.sh $(SRCS) $(HEADERS)
	@ROOT="$(CURDIR)" CC="$(CC)" \
	  CFLAGS="$(CPPFLAGS) $(CFLAGS)" SOURCES="$(SRCS)" \
	  sh tools/gen-compile-commands.sh > $@.tmp
	@mv $@.tmp $@
	@printf 'wrote %s\n' '$@'

compiledb: $(COMPILE_DB)

sanitize:
	@mkdir -p build
	$(CC) $(CPPFLAGS) -O1 -g -std=c11 -Wall -Wextra \
	      -fsanitize=address,undefined -fno-omit-frame-pointer \
	      -o $(SAN_BIN) tests/test_isp.c $(LIB_SRCS) $(LDLIBS)
	./$(SAN_BIN)

clean:
	rm -rf build $(BIN)
