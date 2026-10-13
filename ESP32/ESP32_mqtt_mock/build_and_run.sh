#!/usr/bin/env sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Reconfigura e compila antes de repassar os argumentos ao executavel.
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_PREFIX_PATH="$project_dir/.deps/install"
cmake --build "$project_dir/build"
exec "$project_dir/build/ESP32Mock" "$@"
