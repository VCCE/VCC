# Microsoft Developer Studio Project File - Name="Vcc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Vcc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Vcc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Vcc.mak" CFG="Vcc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Vcc - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Vcc - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Vcc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fr /YX /FD /c /Tp
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ddraw.lib comctl32.lib Winmm.lib dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib dinput8.lib dxguid.lib /nologo /subsystem:windows /machine:I386 /IGNORE:4089
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Vcc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c /Tp
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ddraw.lib dxguid.lib comctl32.lib Winmm.lib dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib dinput8.lib shlwapi.lib /nologo /subsystem:windows /map /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Vcc - Win32 Release"
# Name "Vcc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Audio.c
# End Source File
# Begin Source File

SOURCE=.\Cassette.cpp
# End Source File
# Begin Source File

SOURCE=.\coco3.cpp
# End Source File
# Begin Source File

SOURCE=.\config.c
# End Source File
# Begin Source File

SOURCE=.\DirectDrawInterface.cpp
# End Source File
# Begin Source File

SOURCE=.\Fileops.c
# End Source File
# Begin Source File

SOURCE=.\hd6309.c
# End Source File
# Begin Source File

SOURCE=.\iobus.c
# End Source File
# Begin Source File

SOURCE=.\joystickinput.c
# End Source File
# Begin Source File

SOURCE=.\keyboard.c
# End Source File
# Begin Source File

SOURCE=.\license.txt
# End Source File
# Begin Source File

SOURCE=.\logger.c
# End Source File
# Begin Source File

SOURCE=.\mc6809.c
# End Source File
# Begin Source File

SOURCE=.\mc6821.c
# End Source File
# Begin Source File

SOURCE=.\pakinterface.c
# End Source File
# Begin Source File

SOURCE=.\quickload.c
# End Source File
# Begin Source File

SOURCE=.\tcc1014graphics.c
# End Source File
# Begin Source File

SOURCE=.\tcc1014mmu.c
# End Source File
# Begin Source File

SOURCE=.\tcc1014registers.c
# End Source File
# Begin Source File

SOURCE=.\throttle.c
# End Source File
# Begin Source File

SOURCE=.\Vcc.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\audio.h
# End Source File
# Begin Source File

SOURCE=.\Cassette.h
# End Source File
# Begin Source File

SOURCE=.\cc2font.h
# End Source File
# Begin Source File

SOURCE=.\cc3font.h
# End Source File
# Begin Source File

SOURCE=.\coco3.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\defines.h
# End Source File
# Begin Source File

SOURCE=.\DirectDrawInterface.h
# End Source File
# Begin Source File

SOURCE=.\fileops.h
# End Source File
# Begin Source File

SOURCE=.\hd6309.h
# End Source File
# Begin Source File

SOURCE=.\hd6309defs.h
# End Source File
# Begin Source File

SOURCE=.\iobus.h
# End Source File
# Begin Source File

SOURCE=.\joystickinput.h
# End Source File
# Begin Source File

SOURCE=.\keyboard.h
# End Source File
# Begin Source File

SOURCE=.\logger.h
# End Source File
# Begin Source File

SOURCE=.\mc6809.h
# End Source File
# Begin Source File

SOURCE=.\mc6809defs.h
# End Source File
# Begin Source File

SOURCE=.\mc6821.h
# End Source File
# Begin Source File

SOURCE=.\pakinterface.h
# End Source File
# Begin Source File

SOURCE=.\quickload.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\tcc1014graphics.h
# End Source File
# Begin Source File

SOURCE=.\tcc1014mmu.h
# End Source File
# Begin Source File

SOURCE=.\tcc1014registers.h
# End Source File
# Begin Source File

SOURCE=.\throttle.h
# End Source File
# Begin Source File

SOURCE=.\Vcc.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\3guys.bmp
# End Source File
# Begin Source File

SOURCE=.\bender.bmp
# End Source File
# Begin Source File

SOURCE=.\binfile.ico
# End Source File
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\bjork.bmp
# End Source File
# Begin Source File

SOURCE=.\bjork.ico
# End Source File
# Begin Source File

SOURCE=.\composite.ico
# End Source File
# Begin Source File

SOURCE=.\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\hitachi.ico
# End Source File
# Begin Source File

SOURCE=.\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\ico00004.ico
# End Source File
# Begin Source File

SOURCE=.\ico00005.ico
# End Source File
# Begin Source File

SOURCE=.\ico00006.ico
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\joystick.ico
# End Source File
# Begin Source File

SOURCE=.\Keyboard.ico
# End Source File
# Begin Source File

SOURCE=.\moto.ico
# End Source File
# Begin Source File

SOURCE=.\mouse.ico
# End Source File
# Begin Source File

SOURCE=.\nodrop.cur
# End Source File
# Begin Source File

SOURCE=.\progpack.ico
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\rgb.ico
# End Source File
# Begin Source File

SOURCE=.\speaker.ico
# End Source File
# Begin Source File

SOURCE=.\Speaker1.bmp
# End Source File
# Begin Source File

SOURCE=.\Speaker2.bmp
# End Source File
# Begin Source File

SOURCE=.\vcc.bmp
# End Source File
# Begin Source File

SOURCE=.\Vcc.rc
# End Source File
# End Group
# End Target
# End Project
