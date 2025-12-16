## DREAM-VCC - The Tandy Color Computer 3 Emulator
DREAM-VCC, or simply DREAM, is an emulator for the Tandy Color Computer 3, designed to run on Microsoft Windows. It aims to provide an accurate and easy-to-use emulation of the original hardware, as purchased from a Radio Shack store or Tandy Computer Center between 1986-1992. For DREAM usage, see the User Guide at [DREAM Wiki](https://github.com/ChetSimpson/DREAM-VCC/wiki). The wiki also contains release notes and additional documents. Online documentation may reflect some features not available in earlier versions or that are pre-release.

DREAM is licensed under the GNU General Public License v3.0. See the [LICENSE](COPYING) file for more details.

## Features
DREAM emulates a stock 128k CoCo 3 and additional products, including:
- **Tandy Multi-Pak Interface (MPI)**: Four expansion slots.
- **Tandy FD-502 Floppy Disk Drive Controller with inegrated Becker Port**: Includes Disk Extended BASIC, four configurable virtual disk drives, and a Becker Interface for connecting to a DriveWire Server
- **Orchestra90cc**: A five-voice music sequencer with stereo 8-bit DACs.
- **Becker Port**: Simple serial device for connecting to a DriveWire server.
- **Memory Expansions**: Up to 8192k.
- **CPU Replacement**: Swap the Motorola 6809 CPU with a Hitachi 6309.

## Obtaining DREAM

Sources and binaries for DREAM can be found at [DREAM Releases](https://github.com/ChetSimpson/DREAM-VCC/releases). It is recommended to use the "latest" release.

Please be aware that the binaries provided with DREAM releases, including the installers, are not digitally signed. It is likely you will be presented with Windows security warnings when you first run them. Alternatively, you can build the version of your choice from the sources available with the release.

Occasionally Virus detection software might flag DREAM binaries due to false positives, even if you build from sources. If you encounter these issues you may be able to add an exception for DREAM in the protection software you are using. Every effort is taken to keep DREAM safe to use but be aware there is no warranty that the instance of DREAM you are running actually is.

## Building DREAM
DREAM is written in C++ and Microsoft Visual Studio 2022 Community is used to build DREAM. VS2022 is available for free download from Microsoft. The standard DREAM build will run on Windows 10 and above. Optionally Visual Studio 2022 can be used to build a "legacy" DREAM version using xp build tools that will run on Windows XP.

To build DREAM from the command line, launch the "Developer Command Prompt for VS 2022". From there, change to the directory containing the DREAM sources and type "Build" or "BuildClean".

## Contributing to DREAM
We welcome patches and code contributions that are consistent with our goals. Please comment your code well and add your name if you want credit for your work.

### How to Contribute

1. Fork the repository to your own GitHub account.
2. Clone the forked repository to your development system.
3. Create a new branch with a descriptive name.
4. Make your changes and commit them with clear messages.
5. Push your changes to your fork.
6. Submit a pull request to the `develop` branch of the DREAM-VCC repository.

We also welcome bug reports and suggestions. Please post these on the GitHub "Issues" tab. We check the issues periodically to see if there are fixes we can implement while working on DREAM. The DREAM Development Team is small, and we work on this project when we can, as we all have lives and families. DREAM is a hobby, not a priority. It is a work of love, as we also use this software ourselves, so we try to make it as usable as possible. Sometimes progress is slow, and it looks like nothing is happening (and it may not be), but usually, there's plenty going on behind the scenes, and we have not committed our current work.

