# linuxulator-steam-utils

A set of workarounds for the Linux Steam client targeting FreeBSD 13.2+.

Nvidia GPU is highly recommended.
For the list of tested Linux games see the [compatibility](https://github.com/shkhln/linuxulator-steam-utils/wiki/Compatibility) page in the wiki.

## Limitations

1. Sandbox is disabled for the web browser component.
1. No controller input, no streaming, no VR.
1. Valve Anti-Cheat is untested.
1. Steam's own container runtime (pressure-vessel) doesn't work.

## Setup

LSU uses unprivileged chroot and mounts to run the Steam's embedded web browser and (optionally) games.
Set *security.bsd.unprivileged_chroot* and *vfs.usermount* sysctls to 1 to enable those. Add *nullfs* to kld_list.

It's generally expected that you use the linux-* packages as they are and leave compat.linux.emul_path sysctl alone.
Any attempts to run LSU with a custom Ubuntu base will be met with unsympathetic "WTF?" and "I told you" replies.

### Dependencies

At least *ca_root_nss*, *linux-rl9-dbus-libs*, *linux-rl9-devtools*, *linux-rl9-nss* and *ruby*.
See [Makefile](Makefile) for a more extensive list. You can install these via `sudo make dependencies`
from FreeBSD packages repository.

### Steam

Roughly:
1. `git clone <this repo>`, run `make` and `sudo make install`. The files will be copied to */opt/steam-utils*.
1. Create a dedicated FreeBSD non-wheel user account for Steam. Switch to it.
1. Run `/opt/steam-utils/bin/lsu-bootstrap` to download the Steam bootstrap executable, then `/opt/steam-utils/bin/steam` to download updates and start Steam.

## Chroots

As an alternative to the Steam's container runtime (which we cannot use) LSU registers a few [custom compatibility tools](https://gitlab.steamos.cloud/steamrt/steam-runtime-tools/-/blob/main/docs/steam-compat-tool-interface.md)
launching applications in FreeBSD's [unprivileged chroot](https://cgit.freebsd.org/src/commit/?id=a40cf4175c90142442d0c6515f6c83956336699b).

At the moment there are 3 such tools: LSU chroot with legacy Steam Runtime, LSU chroot with Steam Runtime 3 (Sniper), LSU chroot with Proton 8.
They roughly match the corresponding container environments. Note that tools need to be explicitly selected in the compatibility tab in the game properties.

LSU Proton chroot is generally not recommended over the native FreeBSD Wine integration.
Only 64-bit Windows games are expected to work with it. Advanced Proton features like fsync or the seccomp-bpf syscall filter don't work.

All chroots actively reuse Steam Runtime libs, so they all depend on [Steam Linux Runtime 3.0 (sniper)](https://steamdb.info/app/1628350/).
In addition to that the Proton 8 chroot depends on [Proton 8](https://steamdb.info/app/2348590/). Steam doesn't allow us to request those dependencies,
so the task of installing them is left to the user. Since Steam doesn't actually provide up-to-date Mesa libs,
they will be copied into chroot from Linux base (/compat/linux).

If you need chroot to access directories outside of $HOME (which is aways mounted) use the STEAM_COMPAT_MOUNTS environment variable, set it before starting Steam.

Another useful environment variable is LSU_DEBUG: setting the launch options to `LSU_DEBUG=ktrace %command%` will tell LSU to run the application though `ktrace`.
ktrace.out will be placed in the game directory.

## Native Wine integration

LSU's "FreeBSD Wine" compatibility tool works by substituting Proton's Linux Wine binaries with FreeBSD Wine binaries
(provided by the [emulators/wine-proton](https://www.freshports.org/emulators/wine-proton/) port), while other Proton parts remain unchanged.
There are, in fact, a few non-Wine Linux binaries there, they are not being replaced but rather run through [emulators/libc6-shim](https://www.freshports.org/emulators/libc6-shim/).
(Yes, we have another, entirely independent from Linuxulator, emulation thing.)

This kind of hybrid Proton setup is quite different from both official Linux Proton builds and vanilla Wine,
thus any issues encountered with it can *not* be directly reported to either project's bug tracker.

Known issues include incompatibility with the Steam overlay.

This tool is generally recommended over Linux Proton (running under Linuxulator) due to working WoW64 and more reliable 3D acceleration on Intel/AMD GPUs.

To install dependencies:
1. Run `sudo pkg install wine-proton libc6-shim python3`.
1. Run `/usr/local/wine-proton/bin/pkg32.sh install wine-proton mesa-dri`.
1. In Steam install the Proton version corresponding to the wine-proton's port version (only major.minor numbers have to match).

To enable the tool right click a game title in Steam, click Properties, click Compatibility, select "FreeBSD Wine (emulators/wine-proton)".
