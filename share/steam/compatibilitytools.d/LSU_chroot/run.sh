#!/bin/sh
. lsu-linux-to-freebsd-env
__dir__="$(dirname "$(realpath "$0")")"
exec "$__dir__/run.rb" "$@"
