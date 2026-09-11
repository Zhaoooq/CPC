#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(dirname -- "$script_dir")
build_dir=${CPC_BUILD_DIR:-"$project_dir/build"}
build_jobs=${CPC_BUILD_JOBS:-2}

mkdir -p "$build_dir"
cd "$build_dir"

qmake "$project_dir/CPC_1.pro"
make -j"$build_jobs"

# Keep the path used by start-cpc-1.sh stable. Rename only after a complete build.
install -m 0755 "$build_dir/CPC_1" "$project_dir/CPC_1.next"
mv -f "$project_dir/CPC_1.next" "$project_dir/CPC_1"

printf 'CPC build complete: %s\n' "$project_dir/CPC_1"
