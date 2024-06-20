# VCC - Virtual Color Computer
# An emulation of The Tandy Color Computer 3 for Windows XP thru Windows 11 

VCC attempts to be an accurate emulation of a "stock" Tandy Color Computer 3 just 
as you would have bought in a Radio Shack store or Tandy Computer Center in 1986-1992. 

For VCC usage see the User Guide at <https://github.com/VCCE/VCC/wiki>  Also on the
wiki are release notes and a guide for using legacy Wimgtools.

The Color Computer 3 (or the "Coco 3" as it was known by it's users) was the final 
iteration of a series of computers starting with the "TRS-80 Color Computer" (also 
known as the "Coco 1") in 1980 with it's 4k of RAM, Non-Extended Color BASIC, and 
expansion slot for Game & Software Cartridges. Programs could be run from cassette tape.
Eventually the memory was exapanded from 4k to 16k ( which allowed the use of 
"Extended Color BASIC"), then 32k, and finally 64k. Also relesed by Tandy was the Disk 
Drive unit for 5.25' Floppy disks, which along with 64k of memory, allowed it to run 
Microware's "OS-9 Level 1" for the 6809 processor. Several motherboard revisions were 
released with the Silver Coco 1, until Tandy upgraded to a new design with a smaller 
footprint and slightly more modern circuitry. This machine was the "Tandy Color Computer 2"
or "Coco 2", which went through several revisions and came in 16k Color BASIC and 
64k Extended Color BASIC models. Finally in 1986 Tandy release the final model, the 
"Tandy Color Computer 3" or "Coco 3" with 128k of memory (expandable to 512k), 
and "Super Extended Color BASIC" with expanded high resolution graphics with up to 16 
colors from a palette of 64 colors.

VCC directly models the "stock" 128k Coco 3 and is expandable to many levels that were 
available by Tandy and 3rd party vendors. These include:

1. "Tandy MultiPak Interface" or "MPI" with 4 expansion slots.
2. "Tandy FD-502 Disk Drive Controller" with "Disk Extended BASIC" and 4 configurable virtual disk drives
3. A "Generic" Hard Drive Interface which allow VCC to use "Virtual Hard Disks" or "VHDs"
4. "SuperIDE Hard Drive Controller" - emulates dual IDE hard drives, the same model produced by Cloud 9 and will also use Compact Flash memory card images much like the real SuperIDE.
5 "Orchestra90cc" - a 5 voice music sequencer emulating the original prgram pack of the same name in cluding the stereo 8 bit DACs whitch play in stereo through your PC speakers.

We welcome all bug reports and suggestions. Post any bug reports and/or suggestions on the 
"Issues" page and we will be promptly notified. We do a "check list" of the issues page 
every-so-often to see if there's any "quick fixes" we can add while we are working on 
current changes. The VCC Developement Team is a small one and we work on this when we can 
as we all have lives and families, so VCC is NOT a priority but a hobby. Even being a hobby,
it is also a work of love as we also use this software ourselves, so we try to make it as 
usable as possible. Sometimes progress is slow and it looks like nothing is going on 
(and it may not be), but usually, there's plenty going on behind the scenes and we have 
not committed our current work. Progress is slow, but progress is being made.

VCC version numbering has recently changed. The version number consists of "Vcc-" followed
by 4 numbers seperated by dots, for example: "Vcc-2.1.8.2". The first number represents the 
"fork." Fork one is considered to be the original code from Joseph Forgeone. The current
fork is "2". It promises to function on Windows XP and Windows 7 operating systems. We 
might want to add capabilitues that will not work on these older systems. If we do that we
will create a third "fork." This will allow us to backport changes from fork 3 code back
to fork 2. The second number represents a major version. It will be used if very significant
changes are made in the way Vcc functions (not likely happen anytime soon). The third
number represents normal releases, typically done one or two times per year. The fourth
number represents bug fixes to the release. These are changes to VCC that correct errors
and bugs in the intended functionality of the release. Binaries containing these changes
will be updated in the affected release files.

# Compiling the VCC Sources
Currently, VCC is compiled in C/C++ using "Microsoft Visual Studio 2015 Community".  This
older Visual Studio version is used to maintain compatibility with Windows XP.  You must
include "Win x86", & "XP" support packages ("DirectX 9" should be in one of these packages,
if not, you must include that as well).  Later versions of Visual Studio will load and 
compile the VS2015 code fine, but the resulting code will may not be compatible with what
the VCC Development Team is using.  

VS2015 requires Windows 7 or greater to install. Check the VS2015 "System Requirements" 
before trying to install on your system. Since Microsoft is no longer supplying a web based
installer for VS2015 you will have to download an ISO image and install it from that.

The only extra packages needed to compile VCC are the Win32 and WinXP compatibility packages
to ensure build compatibility with older versions of Windows. This may change if we move
the code to cross-compile for Mac and Linux users and not just the Windows users.

Error free compiling any VCC sources other than the "Release" source set is not guaranteed.
The source code that was used for official releases is available under "Assets" for each release. 

If you think you have patches or code that could contribute to the VCC project, please 
contact the developement team and to see if it fits the current direction of the project, 
and if so, add it to our code. Be sure to add your name and the date in your comments 
(and please comment your code well) if you want credit for your work. Also, if your 
programming skills are up to "snuff" and you feel you would like to join the VCC project, 
please contact us.

The default git branch is "main"  If you want to make changes to the VCC codebase please
fork the "main" branch to your own github account, then use git to download branch main
from your fork to your local development system.  From main create a new branch for 
your changes. The branch name should suggest the purpose of your changes.  Make your
changes and test them.  Once you done push your new branch back to your fork on github.  
Then use github to create a "Pull Request" for VCCE/VCC.  We will review your changes and 
merge them if satisfactory.  Changes to .vcxproj files caused by using the wrong release
or version of Visual Studio will generally be rejected and may result in your entire 
request being rejected if we can not build the merge with VS2015. 

Thank You for using VCC

We will continue to try to make VCC the best Color Computer Emulator available.

The VCC Developement Team.
