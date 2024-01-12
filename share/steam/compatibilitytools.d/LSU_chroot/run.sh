#!/bin/sh
#TODO: sysctl -nq security.bsd.unprivileged_chroot
export LD_LIBRARY_PATH="`printenv LD_LIBRARY_PATH | sed -e 's/steam-runtime/nope/g'`"
export PATH="/bin"
shift # waitforexitandrun
exec env -u LD_PRELOAD /usr/local/../sbin/chroot -n $HOME/.steam/mnt\
 /bin/env LD_PRELOAD="$LD_PRELOAD" sh -c 'cd "$1" && shift && exec "$@"' - "$PWD" "$@"
