#!/usr/bin/env sh

ROOT_PATH="$(dirname $0)/.."

echo $(pkg-config --variable pc_path pkg-config):$(realpath ./libs/include/):$(realpath ./libs/include/include/):$(realpath ./libs/include/lib/pkgconfig/):$(realpath ./libs/pcre2-*/build/):$(realpath ./libs/libyang-*/build/):$(realpath ./libs/doctest-*/build/)
