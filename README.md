## VCC - Virtual Color Computer
#### An emulation of The Tandy Color Computer 3 for Microsoft Windows

VCC attempts to be an easy to use and accurate emulation Tandy Color Computer 3 as purchased from a Radio Shack store or Tandy Computer Center in 1986-1992.  For VCC usage see the User Guide at <https://github.com/VCCE/VCC/wiki>.  Also in the wiki are release notes and additional documents. Online documentation may reflect some features not available on earlier versions or that are pre-release.

In addition to emulating a stock 128k Coco 3 VCC is expandable to emulate additional products that were available from Tandy and 3rd party vendors. Some of these are:

1. "Tandy MultiPak Interface" or "MPI" with 4 expansion slots.
1. "Tandy FD-502 Disk Drive Controller" with "Disk Extended BASIC" and 4 configurable virtual disk drives
1. A "Generic" Hard Drive Interface which allow VCC to use "Virtual Hard Disks" or "VHDs"
1. "SuperIDE Hard Drive Controller" - emulates dual IDE hard drives, the same model produced by Cloud 9 and will also use Compact Flash memory card images much like the real SuperIDE.
1. "Orchestra90cc" - a 5 voice music sequencer emulating the original prgram pack of the same name in cluding the stereo 8 bit DACs whitch play in stereo through your PC speakers.
1. "Becker Port" interface that permits connection to a DriveWire server.
1. Memory expansions up to 8192k.
1. Replacement of the stock Motorolla 6809 cpu with an Hitachi 6309 with it's extended instruction set and additional cpu registers.

#### Obtaining VCC

Sources and binaries for VCC versions since v2.0.1 can be found at <https://github.com/VCCE/VCC/releases>. It is recommended that you use the "latest" release. 

VCC version numbering is somewhat irratic. Currently the version number consists of "Vcc-" followed by 4 numbers seperated by dots, for example: "Vcc-2.1.8.2". The first number represents a fork.  The current fork is "2".  The second number represents a major version. The third number represents releases that make additions or significant changes. The fourth number represents bug fixes or minor changes.

Please be aware that the binaries provided with VCC releases, including the installers, do not contain verification certificates. It is likely you will be presented with windows security warnings when you first run them. Alternately you can build the version of your choice from the sources available with the release.

#### Building VCC

VCC is written in C++ and C. Microsoft Visual Studio 2022 Community is used to build VCC and is available for free download from Microsoft.  Optionally components from Visual Studio 2015 can be used to build a "legacy" VCC version that will run on Windows XP.

To build VCC from the command line launch "Developer Command Prompt for VS 2022". From there change to the directory containing the VCC sources and type "Build" or "BuildClean". 

Within Visual Studio the "Release" and "Debug" configurations build VCC binaries that will run on current Windows versions. The "Legacy" configuration builds binaries that will run on Windows XP. "Release" and "Debug" use build tools from VS2022 and "Legacy" uses build tools from VS2015.  Note that Windows versions before Windows 10 are considered out of service by Microsoft and Visual Studio 2015 is no longer supplied using a web based installer. For legacy builds you need to find and install the v140_xp toolset from VS2015.

#### Contributing to VCC

Patches or code that contribute to the VCC project that are consistant with our goals is welcome. Comment your code well and add your name if you want credit for your work. Also if you feel you would like to join the VCC project please contact us.  The code repository is on github and all changes to the code base are made by github "Pull Requests"

The default git branch is "main."  To submit changes fork VCC to your own github account and download it to your development system.  Create a new branch with a name that suggests the purpose of the change.  After your changes are tested push the new branch to your fork on github.  Then use github to create a "Pull Request" for the branch.  We will review your changes and merge them if satisfactory.

Do not include changes to .sln or .vcxproj files in your pull request; any changes to these will result in your request being rejected.  If you feel these project files need to be changed contact us with your thoughts and we will adjust the .vcxproj files as is appropriate.

We also welcome bug reports and suggestions. Please post these on the github "Issues" tab. We check the issues every-so-often to see if there are fixes we can do while we are working on VCC. The VCC Developement Team is a small one and we work on this when we can as we all have lives and families, so VCC is NOT a priority but a hobby. It is a work of love as we also use this software ourselves, so we try to make it as usable as possible. Sometimes progress is slow and it looks like nothing is going on (and it may not be), but usually, there's plenty going on behind the scenes and we have not committed our current work.

Thank You for using VCC

We will continue to try to make VCC the best Color Computer 3 Emulator available.

The VCC Development Team.
