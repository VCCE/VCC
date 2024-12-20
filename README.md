### VCC - Virtual Color Computer

An emulation of The Tandy Color Computer 3 for Microsoft Windows

VCC aims to provide an easy-to-use and accurate emulation of the Tandy Color Computer 3, as purchased from a Radio Shack store or Tandy Computer Center between 1986-1992. For VCC usage, see the User Guide at [VCC Wiki](https://github.com/VCCE/VCC/wiki). The wiki also contains release notes and additional documents. Online documentation may reflect some features not available in earlier versions or that are pre-release.

In addition to emulating a stock 128k CoCo 3, VCC is expandable to emulate additional products that were available from Tandy and third-party vendors. Some of these are:

- **Tandy MultiPak Interface (MPI)**: Four expansion slots.
- **Tandy FD-502 Disk Drive Controller**: Includes Disk Extended BASIC and four configurable virtual disk drives.
- **Generic Hard Drive Interface**: Allows VCC to use Virtual Hard Disks (VHDs).
- **SuperIDE Hard Drive Controller**: Emulates dual IDE hard drives, the same model produced by Cloud 9, and also uses Compact Flash memory card images like the real SuperIDE.
- **Orchestra90cc**: A five-voice music sequencer emulating the original program pack of the same name, including stereo 8-bit DACs that play in stereo through your PC speakers.
- **Becker Port**: Interface permitting connection to a DriveWire server.
- **Memory Expansions**: Up to 8192k.
- **CPU Replacement**: Swap the stock Motorola 6809 CPU with a Hitachi 6309, which has an extended instruction set and additional CPU registers.

### Obtaining VCC

Sources and binaries for VCC versions since v2.0.1 can be found at [VCC Releases](https://github.com/VCCE/VCC/releases). It is recommended to use the "latest" release.

VCC version numbering is somewhat erratic. Currently, the version number consists of "Vcc-" followed by four numbers separated by dots, for example: "Vcc-2.1.8.2". The first number represents a fork; the current fork is "2". The second number represents a major version, the third number represents releases that make additions or significant changes, and the fourth number represents bug fixes or minor changes.

Please be aware that the binaries provided with VCC releases, including the installers, do not contain verification certificates. It is likely you will be presented with Windows security warnings when you first run them. Alternatively, you can build the version of your choice from the sources available with the release.

### Building VCC

VCC is written in C++ and C. Microsoft Visual Studio 2022 Community is used to build VCC and is available for free download from Microsoft. Optionally, components from Visual Studio 2015 can be used to build a "legacy" VCC version that will run on Windows XP.

To build VCC from the command line, launch the "Developer Command Prompt for VS 2022". From there, change to the directory containing the VCC sources and type "Build" or "BuildClean".

Within Visual Studio, the "Release" and "Debug" configurations build VCC binaries that will run on current Windows versions. The "Legacy" configuration builds binaries that will run on Windows XP. "Release" and "Debug" use build tools from VS2022, and "Legacy" uses build tools from VS2015. Note that Windows versions before Windows 10 are considered out of service by Microsoft, and Visual Studio 2015 is no longer supplied using a web-based installer. For legacy builds, you need to find and install the v140_xp toolset from VS2015.

### Contributing to VCC

Patches or code contributions to the VCC project that are consistent with our goals are welcome. Comment your code well and add your name if you want credit for your work. If you would like to join the VCC project, please contact us. The code repository is on GitHub, and all changes to the code base are made by GitHub "Pull Requests".

The default git branch is "main". To submit changes, fork VCC to your own GitHub account and download it to your development system. Create a new branch with a name that suggests the purpose of the change. After testing your changes, push the new branch to your fork on GitHub. Then use GitHub to create a "Pull Request" for the branch. We will review your changes and merge them if satisfactory.

Do not include changes to `.sln` or `.vcxproj` files in your pull request; any changes to these will result in your request being rejected. If you feel these project files need to be changed, contact us with your thoughts, and we will adjust the `.vcxproj` files as appropriate.

We also welcome bug reports and suggestions. Please post these on the GitHub "Issues" tab. We check the issues periodically to see if there are fixes we can implement while working on VCC. The VCC Development Team is small, and we work on this project when we can, as we all have lives and families. VCC is a hobby, not a priority. It is a work of love, as we also use this software ourselves, so we try to make it as usable as possible. Sometimes progress is slow, and it looks like nothing is happening (and it may not be), but usually, there's plenty going on behind the scenes, and we have not committed our current work.

