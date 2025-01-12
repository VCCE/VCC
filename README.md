## VCC - Virtual Color Computer
VCC is an emulator for the Tandy Color Computer 3, designed to run on Microsoft Windows. It aims to provide an accurate and easy-to-use emulation of the original hardware, as purchased from a Radio Shack store or Tandy Computer Center between 1986-1992.  For VCC usage, see the User Guide at [VCC Wiki](https://github.com/VCCE/VCC/wiki). The wiki also contains release notes and additional documents. Online documentation may reflect some features not available in earlier versions or that are pre-release.

VCC is licensed under the GNU General Public License v3.0. See the [LICENSE](COPYING) file for more details.

## Features
VCC emulates a stock 128k CoCo 3 and additional products, including:
- **Tandy MultiPak Interface (MPI)**: Four expansion slots.
- **Tandy FD-502 Disk Drive Controller**: Includes Disk Extended BASIC and four configurable virtual disk drives.
- **Generic Hard Drive Interface**: Allows VCC to use Virtual Hard Disks (VHDs).
- **SuperIDE Hard Drive Controller**: Emulates dual IDE hard drives.
- **Orchestra90cc**: A five-voice music sequencer with stereo 8-bit DACs.
- **Becker Port**: Interface for connecting to a DriveWire server.
- **Memory Expansions**: Up to 8192k.
- **CPU Replacement**: Swap the Motorola 6809 CPU with a Hitachi 6309.

## Obtaining VCC

Sources and binaries for VCC versions since v2.0.1 can be found at [VCC Releases](https://github.com/VCCE/VCC/releases). It is recommended to use the "latest" release.

VCC version numbering is somewhat erratic. Currently, the version number consists of "Vcc-" followed by four numbers separated by dots, for example: "Vcc-2.1.8.2". The first number represents a fork; the current fork is "2". The second number represents a major version, the third number represents releases that make additions or significant changes, and the fourth number represents bug fixes or minor changes.

Please be aware that the binaries provided with VCC releases, including the installers, do not contain verification certificates. It is likely you will be presented with Windows security warnings when you first run them. Alternatively, you can build the version of your choice from the sources available with the release.

## Building VCC
VCC is written in C++ and C. Microsoft Visual Studio 2022 Community is used to build VCC and is available for free download from Microsoft. Optionally, components from Visual Studio 2015 can be used to build a "legacy" VCC version that will run on Windows XP.

To build VCC from the command line, launch the "Developer Command Prompt for VS 2022". From there, change to the directory containing the VCC sources and type "Build" or "BuildClean".

Within Visual Studio, the "Release" and "Debug" configurations build VCC binaries that will run on current Windows versions. The "Legacy" configuration builds binaries that will run on Windows XP. "Release" and "Debug" use build tools from VS2022, and "Legacy" uses build tools from VS2015. Note that Windows versions before Windows 10 are considered out of service by Microsoft, and Visual Studio 2015 is no longer supplied using a web-based installer. For legacy builds, you need to find and install the v140_xp toolset from VS2015.

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

