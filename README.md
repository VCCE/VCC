## VCC - Virtual Color Computer
VCC is an emulator for the Tandy Color Computer 3, designed to run on Microsoft Windows. It aims to provide an accurate and easy-to-use emulation of the original hardware, as purchased from a Radio Shack store or Tandy Computer Center between 1986-1992.  For VCC usage, see the User Guide at [VCC Wiki](https://github.com/VCCE/VCC/wiki). The wiki also contains release notes and additional documents. Online documentation may reflect some features not available in earlier versions or that are pre-release.

VCC is licensed under the GNU General Public License v3.0. See the [LICENSE](COPYING) file for more details.

## VCC-EDB Features the following patches:

    FD502 reset handling: corrected reset and lifecycle behavior.
    WD1793 floppy controller: reset handling for in-flight disk transfers.
    MPI cartridge window: refresh of window handles after cartridge changes.
    Cartridge DLL lifecycle: initialization and cleanup fixes.
    Plugin lifecycle: hard-disk and SDC plugin initialization/cleanup.

With a new extended storage solution:
Programmable disk-image playlist system...

    Up to 255 numbered DSK images
    Select disk using POKE &H7F00,N

Additional safeguards:

    Repeated identical CPU writes each detected as separate commands.
    Disk swaps are queued until the floppy controller is idle.
    Disk switching does not reset the CoCo.
    Never interrupts an active disk operation.

## DOWNLOAD THE CUSTOM COCO DECB ROMS!!

CoCo3 EXTENDED COLOR BASIC v2.01 and DISK EXTENDED COLOR BASIC v2.11
These ROMs can be burned and installed in your CoCo3, or used in emulators like VCC.
Changes for CoCo3 ROM:

    aesthetic improvements for boot up, no FoC (flash of color) corruption on both paths
    path 1 is standard ECB 2.01 boot up (normal), this is when there is NO DISK CONTROLLER PRESENT. This will exit normally to ECB but it will say v2.01 so you know it has EDB auto-boot capabilities.
    path 2 is all black, overclocked boot up, this is when there IS A DISK CONTROLLER PRESENT. hands it off to the disk controller for continued boot...

Changes to DISK ROM:

    aesthetic improvements for boot up, no FoC corruption on both paths.
    path 1 is standard DECB 2.11 boot up (normal), this is when there is NO EDB BOOT DISK in drive 0 (NOR any disk with *.BAS on it)
    path 2 is continued ALL BLACK OVERCLOCKED EBD boot, this is when there IS THE EDB BOOT DISK in drive 0 (OR any disk with *.BAS on it)

You can download these and select them in VCC under CPU, and under FD502 disk controller.

DOWNLOAD HERE! https://www.facebook.com/share/p/1MbmLtcLpH/

## Portable EDB first-run files

For the EDB portable profile, keep these companion files beside `Vcc.exe`:

- `coco3_201_edb.rom` — external CoCo 3 BASIC ROM.
- `disk_211_edb.rom` — external FD-502 disk-controller ROM.
- `custom.keymap` — portable custom keyboard map.

A new configuration defaults the keyboard layout to **Custom** and looks for
`custom.keymap` in the application directory. The initial hard reset is
performed after the MPI and its FD-502 slot are loaded, so the configured CoCo
and disk-controller ROMs are active before the first emulated instruction.
App-local keymap paths are stored relative to the executable directory so the
portable folder can be moved as a unit. If `custom.keymap` is absent or cannot
be read, VCC falls back to its built-in CoCo keyboard layout for that session.

## Features
VCC emulates a stock 128k CoCo 3 and additional products, including:
- **Tandy MultiPak Interface (MPI)**: Four expansion slots.
- **Tandy FD-502 Disk Drive Controller with Becker Port**: Includes Disk Extended BASIC,four configurable virtual disk drives, and a Becker Interface for connecting to a DriveWire Server
- **Virtual Hard Drive Interface**: Allows VCC to connect two Virtual Hard Disks (VHDs).
- **SuperIDE Hard Drive Controller**: Emulates dual IDE hard drives.
- **Orchestra90cc**: A five-voice music sequencer with stereo 8-bit DACs.
- **Becker Port**: Old (previous to V2.1.9.3) VCC Interface for connecting to a DriveWire server.
- **Memory Expansions**: Up to 8192k.
- **CPU Replacement**: Swap the Motorola 6809 CPU with a Hitachi 6309.
- **SDC Simulator**: Simulates a COCO SDC floppy emulator.

## Obtaining VCC
Sources and binaries for VCC versions since v2.0.1 can be found at [VCC Releases](https://github.com/VCCE/VCC/releases). It is recommended to use the "latest" release.

VCC version numbering is somewhat erratic. Currently, the version number consists of "Vcc-" followed by four numbers separated by dots, for example: "Vcc-2.1.8.3". The first number represents a fork; the current fork is "2". The second number represents a major version, the third number represents releases that make additions or significant changes, and the fourth number represents bug fixes or minor changes.

VCC binaries provided before v2.1.9.3 are not digitally signed and newer versions are privately signed by a VCC team developer.  It is likely you will be presented with Windows security warnings when you first run them.  Occasionally Virus detection software might flag VCC binaries due to false positives, even if you build from sources.  Every effort is taken to keep VCC safe to use but there is no warranty that it is.

## Building VCC
VCC is written in C++ and Microsoft Visual Studio 2022 Community is used to build VCC.  VS2022 is available for free download from Microsoft. The standard VCC build will run on Windows 10 and above. Optionally Visual Studio 2022 can be used to build a "legacy" VCC version using xp build tools that will run on Windows XP.  It is planned that VCC version 2.1.9.3 will be the last version to support XP.

To build VCC from the command line, launch the "Developer Command Prompt for VS 2022". From there, change to the directory containing the VCC sources and type "Build" or "BuildClean".

Within Visual Studio, the "Release" and "Debug" configurations build VCC binaries that will run on current Windows versions. The "Legacy" configuration builds binaries that will run on Windows XP. "Legacy" uses the v141_xp build tools from which you need to find and install.

Portions of VCC code have been modified to use features of the C++17 standard.

## Contributing to VCC
We welcome patches and code contributions that are consistent with our goals. Please comment your code well and add your name if you want credit for your work.

### How to Contribute
1. Fork the repository to your own GitHub account.
2. Clone the forked repository to your development system.
3. Create a new branch with a descriptive name.
4. Make your changes and commit them with clear messages.
5. Push your changes to your fork.
6. Submit a pull request to the `main` branch of the original repository.

Note: Do not include changes to `.sln` or `.vcxproj` files in your pull request. If you feel these project files need changes, contact the maintainers first.

We also welcome bug reports and suggestions. Please post these on the GitHub "Issues" tab. We check the issues periodically to see if there are fixes we can implement while working on VCC. The VCC Development Team is small, and we work on this project when we can, as we all have lives and families. VCC is a hobby, not a priority. It is a work of love, as we also use this software ourselves, so we try to make it as usable as possible. Sometimes progress is slow, and it looks like nothing is happening (and it may not be), but usually, there's plenty going on behind the scenes, and we have not committed our current work.

