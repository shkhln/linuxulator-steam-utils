#!/bin/sh
. lsu-linux-to-freebsd-env.sh
__dir__="$(dirname "$(realpath "$0")")"
exec "$__dir__/run.rb" "$@"
