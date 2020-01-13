# linuxulator-steam-utils

A set of workarounds for running the Linux Steam client under the FreeBSD Linux emulation layer.

## Usage

### Dependencies

At least *ca_root_nss*, *linux-c7-dbus-libs*, *linux-c7-devtools*, *linux-c7-nss* and *ruby*.
See [Makefile] for a more extensive list.

You can install these via `sudo make dependencies` from FreeBSD packages
repository.

### Setup

Roughly:

1. `git clone <this repo>`, run `make` and `sudo make install`. The files will be copied to */opt/steam-utils*.
1. Create a dedicated FreeBSD non-wheel user account for Steam. Switch to it.
1. Run `/opt/steam-utils/bin/steam-install` to download the Steam bootstrap executable, then `/opt/steam-utils/bin/steam` to download updates and start Steam.

## Limitations

1. Sandbox is disabled for the web browser component.
1. No controller input, no streaming, no VR.
1. Due to inherent difficulty of running games on Linuxulator as well as many (most?) native Linux ports being broken garbage in general, only the Source engine games are explicitly supported. I simply can't be bothered to test anything else on a regular basis.
1. Steam Play can't be implemented as long as the FreeBSD Ports Collection *lacks multilib support*, making it impossible to package Proton dependencies.

[Makefile]: https://github.com/shkhln/linuxulator-steam-utils/blob/master/Makefile
