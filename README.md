# VCC - Virtual Color Computer
# An emulation of the Tandy Color Computer 3 for Windows XP, Windows 7, Windows 8, & Windows 10

VCC is an accurate emulation of a stock Tandy Color Computer 3 just as you would have bought in a Radio Shack store or Tandy Computer Center in 1986-1992.

The Color Computer 3 (or the _CoCo 3_ as it was known by its users) was the final iteration in a series of computers starting with the TRS-80 Color Computer (also known as the _CoCo 1_) in 1980 with its 4k of RAM, Non-Extended Color BASIC, and expansion slot for game & software cartridges. 

Programs could be run from cassette tape. Eventually the memory was expanded from 4k to 16k which allowed the use of Extended Color BASIC, then 32k, then finally 64k. Also released by Tandy was the Disk Drive unit for 5.25" floppy disks, which along with 64k of memory, allowed it to run Microware's OS-9 Level 1 operating system for the 6809 processor. 

Several motherboard revisions were released with the silver CoCo 1, until Tandy upgraded to a completely new design with a smaller footprint and slightly more modern circuitry (for the time). This machine was the Tandy Color Computer 2 or _Coco 2_, which went through several revisions and came in 16k Color BASIC and 64k Extended Color BASIC models. Finally in 1986 Tandy released the final model, the Tandy Color Computer 3 or _Coco 3_ with 128k of memory (expandable to 512k) and Super Extended Color BASIC, with expanded high resolution graphics with up to 16 colors from a palette of 64 colors.

VCC directly models the stock 128k CoCo 3 and is expandable to many levels that were available by Tandy and 3rd party vendors. These include:
#. **Tandy MultiPak Interface** or _MPI_ with 4 expansion slots
#. **Tandy FD-502 Disk Drive Controller** with Disk Extended BASIC and 4 configurable virtual disk drives
#. **A generic hard drive interface** which allows VCC to use "Virtual Hard Disks" or "VHDs"
#. **SuperIDE Hard Drive Controller** - emulates dual IDE hard drives, the same model produced by Cloud 9 and will also use Compact Flash memory card images much like the real SuperIDE
#. **Orchestra90cc** - a 5 voice music sequencer emulating the original prgram pack of the same name in cluding the stereo 8 bit DACs whitch play in stereo through your PC speakers.

## Compiling the VCC Sources

### Build Environment

Currently, VCC is compiled in C/C++ using [Microsoft Visual Studio Community 2015](https://visualstudio.microsoft.com/vs/older-downloads/), which is a free download for anyone.

**Support Packages** - Include **Win x86**, & **XP** support packages (**DirectX 9** should be in one of these packages, if not, you must include that as well)

VS2015 requires Windows 7 or greater to install. Check the VS2015 System Requirements before trying to install on your system.

As of the current build, there are no extras needed to compile VCC. This may change at any time as we are trying to eventually move the code to cross-compile for cross-platform use, making VCC available for Mac and GNU/Linux users and not just the Windows users.

Compiling any VCC sources other than the _Release_ source set is not guaranteed. Any branch other than the release may at any time, be in a state of developement in which it is not compilable as we are constantly updating the sources with changes. We will try to maintain the default branch as the _Release_ branch ensuring a running emulator. For example; the _Original Masters_ branch is a copy of Joseph Forgeone's (VCC's original author) original VS C++ 6 code that when using that platform, will compile into the equivelent of VCC 1.42. **It WILL NOT compile under VS2015**. These sources are stored here more for historical purposes than anything else.

**We welcome all bug reports and suggestions.** 

Post any bug reports and/or suggestions [on the _Issues_ page](https://github.com/VCCE/VCC/issues) and we will be promptly notified. 

We do a check list of the issues page every-so-often to see if there are any quick fixes we can add while we are working on current changes. The VCC Developement Team is a small one and we work on this when we can as we all have lives and families, so VCC is NOT a priority but a hobby. Even being a hobby, it is also a work of love as we also use this software ourselves, so we try to make it as usable as possible. Sometimes progress is slow and it looks like nothing is going on (and it may not be), but usually, there's plenty going on behind the scenes and we have not committed our current work.  **Progress is slow, but progress is being made.**

**Code contributions are also welcome.**

If you think you have patches or code that could contribute to the VCC project, please contact the developement team and we will see if it fits the current direction of the project, and if so, add it to our code. Be sure to add your name and the date in your comments (and please comment your code well) if you want credit for your work. Also, if your programming skills are up to "snuff" and you feel you would like to join the VCC project, contact us and we will see if you are worthy :-)

If you are submitting a _Pull Request_, make sure your local repository code is up to date with the current release code. Sometimes VS2015 can not make changes if the code bases are several patches ahead or behind.

Ok guys, I've found some discrepancies in our build settings and we all need to be on the same page.

#. Before doing a pull request make sure the **Project** build properties (right click **Solution 'VCC' (8 projects)**) in **Configuration Properties** is set to **Configuration: Release**, **Platform: Win32**, **Build: Checked**, and **Deploy: Unchecked**
#. In each of the eight projects, right click and select **Properties** then **General** then **Project Defaults**, use the pulldown and select **Visual Studio 2015 - Windows XP (v140_xp)**

This must be done on each project file.

These setting are the only way it will build with XP compatibility.

If these settings are not found, then your support packages have not been included.

You must make sure to get the Win32 and WinXP compatibility packages to ensure build compatibility with older versions of Windows.

We will continue to try to make VCC the best Color Computer Emulator available.

Thank You for using VCC,

The VCC Developement Team.
