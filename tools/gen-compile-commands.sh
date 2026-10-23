#!/bin/sh
# Generate compile_commands.json for clangd / editors.
#
# Environment (set by the Makefile):
#   ROOT    absolute repository root
#   CC      compiler command
#   CFLAGS  preprocessor and compiler flags
#   SOURCES whitespace separated source files relative to ROOT
#
# Entries use the "arguments" form so paths containing spaces do not need
# shell-like quoting.
set -eu

root=${ROOT:-$(pwd)}
cc=${CC:-cc}
cflags=${CFLAGS:-}
sources=${SOURCES:-}

json_escape() {
    printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g'
}

printf '[\n'
first=1
for src in $sources; do
    obj="build/$(basename "$src" .c).o"

    if [ "$first" -eq 0 ]; then
        printf ',\n'
    fi
    first=0

    printf '  {\n'
    printf '    "directory": "%s",\n' "$(json_escape "$root")"
    printf '    "file": "%s",\n' "$(json_escape "$src")"
    printf '    "arguments": ["%s"' "$(json_escape "$cc")"
    for flag in $cflags; do
        printf ', "%s"' "$(json_escape "$flag")"
    done
    printf ', "-c", "%s", "-o", "%s"]\n' "$(json_escape "$src")" \
        "$(json_escape "$obj")"
    printf '  }'
done
printf '\n]\n'
