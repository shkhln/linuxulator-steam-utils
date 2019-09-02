# linuxulator-steam-utils

A set of workarounds for running the Linux Steam client under the FreeBSD Linux emulation layer.

## Usage

### Dependencies

At least *ruby* and *linux-c7-devtools*.

### Setup

Roughly:

1. `git clone <this repo>`, run `make` && `sudo make install`. The files will be copied to */opt/steam-utils*.
1. Create a dedicated FreeBSD non-wheel user account for Steam. Switch to it.
1. Run `/opt/steam-utils/bin/steam-install` to download the Steam bootstrap executable, then `/opt/steam-utils/bin/steam` to download updates.
1. Run `/opt/steam-utils/bin/steam` again to actually start Steam.

## Limitations

1. The replacement launcher does not replicate the ad hoc steam.sh restart logic. That means you occasionally have to run it twice in a row.
1. No in-client browser, no controller input, no streaming, no VR.
1. Due to inherent difficulty of running games on Linuxulator as well as many (most?) native Linux ports being broken garbage in general, only the Source engine games are explicitly supported. I simply can't be bothered to test anything else on a regular basis.
1. Steam Play can't be implemented as long as the FreeBSD Ports Collection *lacks multilib support*, making it impossible to package Proton dependencies.
