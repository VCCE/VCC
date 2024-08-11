# VCC - Virtual Color Computer
# An emulation of The Tandy Color Computer 3 for Windows XP thru Windows 11 

VCC attempts to be an accurate emulation of a "stock" Tandy Color Computer 3 just as you would have bought in a Radio Shack store or Tandy Computer Center in 1986-1992. 

For VCC usage see the User Guide at <https://github.com/VCCE/VCC/wiki>  Also in the wiki are release notes and additional documantaion.

The Color Computer 3 (or the "Coco 3" as it was known by it's users) was the final iteration of a series of computers starting with the "TRS-80 Color Computer" (also known as the "Coco 1") in 1980 with it's 4k of RAM, Non-Extended Color BASIC, and expansion slot for Game & Software Cartridges. Programs could be run from cassette tape.  Eventually the memory was exapanded from 4k to 16k ( which allowed the use of "Extended Color BASIC"), then 32k, and finally 64k. Also relesed by Tandy was the Disk Drive unit for 5.25' Floppy disks, which along with 64k of memory, allowed it to run Microware's "OS-9 Level 1" for the 6809 processor. Several motherboard revisions were released with the Silver Coco 1, until Tandy upgraded to a new design with a smaller footprint and slightly more modern circuitry. This machine was the "Tandy Color Computer 2" or "Coco 2", which went through several revisions and came in 16k Color BASIC and 64k Extended Color BASIC models. Finally in 1986 Tandy release the final model, the "Tandy Color Computer 3" or "Coco 3" with 128k of memory (expandable to 512k), and "Super Extended Color BASIC" with expanded high resolution graphics with up to 16 colors from a palette of 64 colors.

VCC emulates a "stock" 128k Coco 3 that is expandable by products that were available by Tandy and 3rd party vendors. These include:

1. "Tandy MultiPak Interface" or "MPI" with 4 expansion slots.
2. "Tandy FD-502 Disk Drive Controller" with "Disk Extended BASIC" and 4 configurable virtual disk drives
3. A "Generic" Hard Drive Interface which allow VCC to use "Virtual Hard Disks" or "VHDs"
4. "SuperIDE Hard Drive Controller" - emulates dual IDE hard drives, the same model produced by Cloud 9 and will also use Compact Flash memory card images much like the real SuperIDE.
5. "Orchestra90cc" - a 5 voice music sequencer emulating the original prgram pack of the same name in cluding the stereo 8 bit DACs whitch play in stereo through your PC speakers.

We welcome all bug reports and suggestions. Post any bug reports and/or suggestions on the git hub "Issues" page. We check the issues every-so-often to see if there are fixes we can do while we are working on VCC. The VCC Developement Team is a small one and we work on this when we can as we all have lives and families, so VCC is NOT a priority but a hobby. Even being a hobby, it is also a work of love as we also use this software ourselves, so we try to make it as usable as possible. Sometimes progress is slow and it looks like nothing is going on (and it may not be), but usually, there's plenty going on behind the scenes and we have not committed our current work. Progress is slow, but progress is being made.

VCC version numbering has recently changed. The version number consists of "Vcc-" followed by 4 numbers seperated by dots, for example: "Vcc-2.1.8.2". The first number represents the "fork." Fork one is considered to be the original code from Joseph Forgeone. The current fork is "2". The second number represents a major version. It will be used if very significant changes are made in the way Vcc functions (not likely happen anytime soon). The third number represents normal releases, typically done one or two times per year. The fourth number represents bug fixes to the release. These are changes to VCC that correct errors and bugs in the intended functionality of the release.

# Building VCC

Either Microsoft Visual Studio 2022 or 2015 can be used to build VCC.  VCC is written in C and C++ but all sources are compiled as C++. Visual Studio 2015 is used to build a legacy VCC version that will run on XP and Windows 7.  Visual Studio is used to build for current Windows versions.

The quickest way to build VCC is to use the Build.bat or BuildClean.bat command file supplied with VCC sources. To use them first run the "Developer Command Prompt" supplied with Visual Studio 2022. This starts a command line window with the settings needed. From there use "cd" to change to the directory containing the VCC sources and type "build" or "buildclean". You can also build from the Visual Studio IDE if want to make changes or just to see how VCC works.

Within Visual Studio the "Release" and "Debug" configurations build VCC binaries that will run on Windows 10 and 11. The "Legacy" configuration builds binaries that will run on Windows XP through Windows 8.  "Release" and "Debug" use the v143 build tools from VS2022 and "Legacy" uses vs140_xp build tools from VS2015.

Since Windows versions before Windows 10 are considered out of service by Microsoft Visual Studio 2015 is no longer supplied as a web based installer.  To install VS2015 you have to download an ISO image and install it from that.  It is also possible to install VS2019 and to usethe vs141 build tools from that to build a VCC version that runs on Windows 7 but that will require retargeting the project files.

We welcome patches or code that contribute to the VCC project that is consistant with our goals. Comment your code well and be sure to add your name and the date in your comments if you want credit for your work. Also if you feel you would like to join the VCC project please contact us.  The code repository is on github and all changes to the code base are made by github "Pull Requests"

The default git branch is "main"  To submit changes for the VCC codebase fork the "main" branch to your own github account, then download branch your fork to your local development system.  Create a new branch for your changes. The branch name should suggest the purpose of your changes.  Make your changes and test them.  Once you done push your new branch back to your fork on github.  Then use github to create a "Pull Request" for your new branch.  We will review your changes and merge them if satisfactory.  Do not include changes to .vcxproj files in your pull request.  If you feel these need to be changed contact us with your thoughts and we will adjust the .vcxproj files by hand as is appropriate.

Thank You for using VCC

We will continue to try to make VCC the best Color Computer Emulator available.

The VCC Developement Team.
