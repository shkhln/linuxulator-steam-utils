# linuxulator-steam-utils

A set of workarounds for the Linux Steam client targeting FreeBSD 13.2+.

Nvidia GPU is highly recommended, Intel might work and *don't even bother with AMD*.
For the list of tested Linux games see the [compatibility](https://github.com/shkhln/linuxulator-steam-utils/wiki/Compatibility) page in the wiki.

## Limitations

1. Sandbox is disabled for the web browser component.
1. No controller input, no streaming, no VR.
1. Valve Anti-Cheat is untested.
1. Steam's own container runtime (pressure-vessel) doesn't work.

## Setup

LSU uses unprivileged chroot and mounts to run the Steam's embedded web browser and (optionally) games.
Set *security.bsd.unprivileged_chroot* and *vfs.usermount* sysctls to 1 to enable those. Add *nullfs* to kld_list.

It's generally expected that you use the linux-c7 packages as they are and leave compat.linux.emul_path sysctl alone.
Any attempts to run LSU with a custom Ubuntu base will be met with unsympathetic "WTF?" and "I told you" replies.

### Dependencies

At least *ca_root_nss*, *linux-c7-dbus-libs*, *linux-c7-devtools*, *linux-c7-nss* and *ruby*.
See [Makefile](Makefile) for a more extensive list. You can install these via `sudo make dependencies`
from FreeBSD packages repository.

### Steam

Roughly:
1. `git clone <this repo>`, run `make` and `sudo make install`. The files will be copied to */opt/steam-utils*.
1. Create a dedicated FreeBSD non-wheel user account for Steam. Switch to it.
1. Run `/opt/steam-utils/bin/lsu-bootstrap` to download the Steam bootstrap executable, then `/opt/steam-utils/bin/steam` to download updates and start Steam.

## Chroots

As an alternative to the Steam's container runtime (which we cannot use) LSU registers a few [custom compatibility tools](https://gitlab.steamos.cloud/steamrt/steam-runtime-tools/-/blob/main/docs/steam-compat-tool-interface.md) allowing us to launch applications in FreeBSD's [unprivileged chroot](https://cgit.freebsd.org/src/commit/?id=a40cf4175c90142442d0c6515f6c83956336699b).
Note that they have to be explicitly selected in the compatibility tab in the game properties.

At the moment there are 3 such tools: the legacy Steam Runtime chroot, Steam Runtime v3 (aka Sniper) chroot and Proton 8 chroot.
Legacy is supposed to provide the experience that is similar to running games without chroot, but with up-to-date glibc and Mesa libs. Use this if you own an Intel/AMD GPU.
Sniper is obviously meant to be used with the games that are built against the corresponding Steam Runtime version.
Linux Proton is generally not recommended over the native FreeBSD Wine integration, but it might be useful here and there.
Only 64-bit Windows games are expected to work. Advanced Proton features like fsync or the seccomp-bpf syscall filter don't work.

Be warned that the extent of 3D acceleration support for Intel/AMD under Linuxulator is unclear.
If you want to test it before committing to a full Steam installation, you can use this script: https://gist.github.com/shkhln/99ed7076d981a8eee39801bb6634caed.

All chroots actively reuse Steam Runtime libs, so they all depend on [Steam Linux Runtime 3.0 (sniper)](https://steamdb.info/app/1628350/).
In addition to that the Proton 8 chroot depends on [Proton 8](https://steamdb.info/app/2348590/). Steam doesn't allow us to request those dependencies,
so the task of installing them is left to the user. Since Steam doesn't actually provide up-to-date Mesa libs,
a few Ubuntu packages are ~hastily slapped~ carefully placed on top of the runtime libs, they will be downloaded automatically.
The tmpfs mount holding all these libs is expected to consume around 1.2 GiB of RAM.

If you need chroot to access directories outside of $HOME (which is aways mounted) use the STEAM_COMPAT_MOUNTS environment variable, set it before starting Steam.

The other useful environment variable is LSU_DEBUG: setting the launch options to `LSU_DEBUG=ktrace %command%` will tell LSU to run the application though `ktrace`.
ktrace.out will be placed in the game directory.

## Native Wine integration

LSU's "FreeBSD Wine" compatibility tool works by substituting Proton's Linux Wine binaries with FreeBSD Wine binaries
(provided by the [emulators/wine-proton](https://www.freshports.org/emulators/wine-proton/) port), while other Proton parts remain unchanged.
There are, in fact, a few non-Wine Linux binaries there, they are not being replaced but rather run through [emulators/libc6-shim](https://www.freshports.org/emulators/libc6-shim/).
(Yes, we have another, entirely independent from Linuxulator, emulation thing.)

This kind of hybrid Proton setup is quite different from both official Linux Proton builds and vanilla Wine,
thus any issues encountered with it can *not* be directly reported to either project's bug tracker.

Known issues include incompatibility with the Steam's overlay and Windows versions of Half-Life and Half-Life 2 engine games.

This tool is generally recommended over Linux Proton (running under Linuxulator) due to working WoW64 and more reliable 3D acceleration on Intel/AMD GPUs.

To install dependencies:
1. Run `sudo pkg install wine-proton libc6-shim python3`.
1. Run `/usr/local/wine-proton/bin/pkg32.sh install wine-proton mesa-dri`.
1. In Steam install the Proton version corresponding to the wine-proton's port version (only major.minor numbers have to match).

To enable the tool right click a game title in Steam, click Properties, click Compatibility, select "FreeBSD Wine (emulators/wine-proton)".
