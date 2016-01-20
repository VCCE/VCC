SET OUTDIR=.\_bin
SET OBJDIR=.\__obj\Win32\Release

if not exist %OUTDIR% (
md %OUTDIR%
)
copy %OBJDIR%\vcc\out\vcc.exe %OUTDIR%
copy %OBJDIR%\mpi\out\*.dll %OUTDIR%
copy %OBJDIR%\fd502\out\*.dll %OUTDIR%
copy %OBJDIR%\harddisk\out\*.dll %OUTDIR%
copy %OBJDIR%\superide\out\*.dll %OUTDIR%
copy %OBJDIR%\orch90\out\*.dll %OUTDIR%
copy %OBJDIR%\ramdisk\out\*.dll %OUTDIR%
copy %OBJDIR%\becker\out\*.dll %OUTDIR%
