# linuxulator-steam-utils

A set of workarounds for running the Linux Steam client under the FreeBSD Linux emulation layer.

For the list of tested Linux games see the [compatibility](https://github.com/shkhln/linuxulator-steam-utils/wiki/Compatibility) page in the wiki.

## Limitations

1. Sandbox is disabled for the web browser component.
1. No controller input, no streaming, no VR.
1. Valve Anti-Cheat doesn't work with FreeBSD < 13, other than that it's largely untested.
1. No Linux Proton at the moment.

## Setup

### Dependencies

At least *ca_root_nss*, *linux-c7-dbus-libs*, *linux-c7-devtools*, *linux-c7-nss* and *ruby*.
See [Makefile](Makefile) for a more extensive list.

You can install these via `sudo make dependencies` from FreeBSD packages
repository.

### Steam

Roughly:
1. `git clone <this repo>`, run `make` and `sudo make install`. The files will be copied to */opt/steam-utils*.
1. Create a dedicated FreeBSD non-wheel user account for Steam. Switch to it.
1. Run `/opt/steam-utils/bin/steam-install` to download the Steam bootstrap executable, then `/opt/steam-utils/bin/steam` to download updates and start Steam.

### Proton

There is semi-experimental support for [emulators/wine-proton](https://www.freshports.org/emulators/wine-proton/) (native Wine with Proton's patchset).
Note that this port is quite different from both official Linux Proton builds and vanilla Wine,
thus any issues encountered with it can *not* be directly reported to either project's bug tracker.

1. Run `sudo pkg install wine-proton libc6-shim python3` and `lsu-pkg32 install wine-proton mesa-dri`.
1. In Steam install Proton 6.3 (appid 1580130).
1. Run `lsu-register-proton` to copy files from the Proton 5.13 distribution and register emulators/wine-proton as a compatibility tool.
1. Restart Steam.
1. Select emulators/wine-proton in `Properties` -> `Compatibility` (per game) or `Settings` -> `Steam Play` (globally).
