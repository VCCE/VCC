# VCC - Virtual Color Computer
# An emulation of tThe Tandy Color Computer 3 for Windows XP, Windows 7, Windows 8, & Windows 10

VCC is an accurate emulation of a "stock" Tandy Color Computer 3 just as you would have bought in a Radio Shack store or Tandy Computer Center in 1986-1992

The Color Computer 3 (or the "Coco 3" as it was known by it's users) was the final iteration in a series of computers starting with the "TRS-80 Color Computer" (also known as the "Coco 1") in 1980 with it's 4k of RAM, Non-Extended Color BASIC, and expansion slot for Game & Software Cartridges. Programs could be run from cassette tape. Eventually the memory was exapnded from 4k to 16k which allowed the use of "Extended Color BASIC", then 32k, then finally 64k. Also relesed by Tandy was the Disk Drive unti for 5.25' Floppy disks, which along with 64k of memory, allowed it to run Microware's "OS-9 Level 1" for the 6809 processor. Several motherboard revisions were released with the Silver Coco 1, until Tandy upgraded to a completely new design with a smaller footprint and slightly more modern circuitry (for the time). This machine was the "Tandy Color Computer 2" or "Coco 2", which went through several revisions and came in 16k Color BASIC & 64k Extended Color BASIC models. Finally in 1986 Tandy release the final model, the "Tandy Color Computer 3" or "Coco 3" with 128k of memory (expandable to 512k), and "Super Extended Color BASIC" with expanded high resolution graphics with up to 16 colors from a palette of 64 colors.

VCC directly models the "stock" 128k Coco 3 and is expandable to many levels that were available by Tandy and 3rd party vendors. These include:
1. "Tandy MultiPak Interface" or "MPI" with 4 expansion slots.
2. "Tandy FD-502 Disk Drive Controller" with "Disk Extended BASIC" and 4 configurable virtual disk drives
3. A "Generic" Hard Drive Interface which allow VCC to use "Virtual Hard Disks" or "VHDs"
4. "SuperIDE Hard Drive Controller" - emulates dual IDE hard drives, the same model produced by Cloud 9 and will also use Compact Flash memory card images much like the real SuperIDE.
5 "Orchestra90cc" - a 5 voice music sequencer emulating the original prgram pack of the same name in cluding the stereo 8 bit DACs whitch play in stereo through your PC speakers.

We welcome all bug reports and suggestions. Post any bug reports and/or suggestions on the "Issues" page and we will be promptly notified. We do a "check list" of the issues page every-so-often to see if there's any "quick fixes" we can add while we are working on current changes. The VCC Developement Team is a small one and we work on this when we can as we all have lives and families, so VCC is NOT a priority but a hobby. Even being a hobby, it is also a work of love as we also use this software ourselves, so we try to make it as usable as possible. Sometimes progress is slow and it looks like nothing is going on (and it may not be), but usually, there's plenty going on behind the scenes and we have not committed our current work. Progress is slow, but progress is being made.


# Compiling the VCC Sources
Build Environment - Currently, VCC is compiled in C/C++ using "Microsoft Visual Studio 2015 Community", which is a free download from the MSDN downloads.
Support Packages - Include "Win x86", & "XP" support packages ("DirectX 9" should be in one of these packages, if not, you must include that as well)
Later versions of Visual Studio cannot be used due to the later versions using newer libraries. Later versions will load and compile the VS2015 code fine, but the resulting code will render the sources uncompilable on VS2015 which is what the VCC Development Team is using.

VS2015 requires Windows 7 or greater to install. Check the VS2015 "System Requirements" before trying to install on your system.

As of the current build, the only "extras" needed to compile VCC are the Win32 and WinXP compatibility packages to ensure build compatibility with older versions of Windows. This may change at any time as we are trying to eventually move the code to cross-compile for cross-platform use, making VCC available for Mac and Linux users and not just the Windows users.

Compiling any VCC sources other than the "Release" source set is not guaranteed. Any branch other than the release may at any time, be in a state of developement in which it is not compilable as we are constantly updating the sources with changes. We will try to maintain the default branch as the "Release" branch ensuring a "running" emulator. For example; the "Original Masters" branch is a copy of Joseph Forgeone's (VCC's original author) original "VS C++ 6" code that when using that platform, will compile into the equivelent of VCC 1.42. It WILL NOT compile under VS2015. These sources are stored here more for historical purposes than anything else.

If you think you have patches or code that could contribute to the VCC project, please contact the developement team and we will see if it fits the current direction of the project, and if so, add it to our code. Be sure to add your name and the date in your comments (and please comment your code well) if you want credit for your work. Also, if your programming skills are up to "snuff" and you feel you would like to join the VCC project, contact us and we will see if you are worthy :-)

If you are submitting a "Pull Request", make sure your local repository code is up to date with the current release code. Sometimes VS2015 can not make changes if the code bases are several patches ahead or behind.

# NOTES

Ok guys, I've found some discrepancies in our build settings and we all need to be on the same page.

1. Before doing a pull request make sure the "Project" build properties (right click "Solution 'VCC' (8 projects)"),
in "Configuration Properties" is set to "Configuration: Release", "Platform: Win32", "Build: Checked", and "Deploy: Unchecked"

2. In each of the 8 projects, right click and select "Properties" then "General" then "Project Defaults", use the pulldown and select "Visual Studio 2015 - Windows XP (v140_xp).
This must be done on each project file.
These setting are the only way it will build with XP compatibility.
I will put this in the "readme.md" in the repo so we'll remember it. I just found my solution had been changed by the 2 pulls I've done from the edits we made on the last release (v2.1.0a).

If these settings are not found, then your support packages have not been included.
You must make sure to get the Win32 and WinXP compatibility packages to ensure build compatibility with older versions of Windows.

# Contributing to the VCC Project

To contribute to our project, you must first create a GitHub account, then clone the VCC project to your account. Create your edits in your local copy of the project and then "Sync" your project back to your GitHub repository. Once your GitHub repository is up to date, you can create a "Pull Request" from within Visual Studio 2015 (or on GitHub). The "Pull Request" will be seen and reviewed by the VCC Project Team and if accepted, will be updated to the main project. Please be sure to refresh your repository with the original project often, as changes are made without notice and could affect you changes as well.

We will continue to try to make VCC the best Color Computer Emulator available.

Thank You for using VCC

The VCC Developement Team.
