#
#	GCC makefile for Vcc (Coco3 emulator)
#
# - builds under mingw64 (2016-02-16)
# - run 'conemu.bat' or 'buildtools.bat'
# - type 'make'
#

PLATFORM = win

# compiler, linker and utilities
AS = as
CC = gcc
LD = g++
CPP = g++
ECHO = @echo
RM = rm
MD = mkdir
OBJC = objcopy
MAKEFILE = makefile

VCC = vcc.exe
FD502_DIR = fd502
FD502_DLL = $(FD502_DIR).dll
HARDDISK_DIR = harddisk
HARDDISK_DLL = $(HARDDISK_DIR).dll
MPI_DIR = mpi
MPI_DLL = $(MPI_DIR).dll
ORCH90_DIR = orch90
ORCH90_DLL = $(ORCH90_DIR).dll
RAMDISK_DIR = ramdisk
RAMDISK_DLL = $(RAMDISK_DIR).dll
SUPERIDE_DIR = superide
SUPERIDE_DLL = $(SUPERIDE_DIR).dll

DLLS = $(FD502_DLL) $(HARDDISK_DLL) $(MPI_DLL) $(ORCH90_DLL) $(RAMDISK_DLL) $(SUPERIDE_DLL)
VCC_LIBS = $(DLLS:.dll=.a)

INCS = -I .

#LIBS = -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lole32 -ldinput -lddraw -ldxguid -lwinmm -ldsound
LIBS = -lkernel32 -lgdi32 -lcomdlg32 -lcomctl32 -lriched20 -ldinput -ldinput8 -lddraw -ldxguid -lwinmm -ldsound

CFLAGS = $(INCS) -std=gnu++0x
CPPFLAGS = $(INCS) -std=gnu++0x
LDFLAGS = $(LIBS)

OBJDIR = obj/$(PLATFORM)
FD502_OBJDIR = $(OBJDIR)/$(FD502_DIR)
HARDDISK_OBJDIR = $(OBJDIR)/$(HARDDISK_DIR)
MPI_OBJDIR = $(OBJDIR)/$(MPI_DIR)
ORCH90_OBJDIR = $(OBJDIR)/$(ORCH90_DIR)
RAMDISK_OBJDIR = $(OBJDIR)/$(RAMDISK_DIR)
SUPERIDE_OBJDIR = $(OBJDIR)/$(SUPERIDE_DIR)
OBJDIRS = \
	$(OBJDIR) \
	$(FD502_OBJDIR) \
	$(HARDDISK_OBJDIR) \
	$(MPI_OBJDIR) \
	$(ORCH90_OBJDIR) \
	$(RAMDISK_OBJDIR) \
	$(SUPERIDE_OBJDIR)

FD502_OBJS = \
	$(OBJDIR)/Fileops.o \
	$(FD502_OBJDIR)/distortc.o \
	$(FD502_OBJDIR)/wd1793.o \
	$(FD502_OBJDIR)/fd502_res.o \
	$(FD502_OBJDIR)/fd502.o

HARDDISK_OBJS = \
	$(OBJDIR)/Fileops.o \
	$(HARDDISK_OBJDIR)/cc3vhd.o \
	$(HARDDISK_OBJDIR)/cloud9.o \
	$(HARDDISK_OBJDIR)/harddisk_res.o \
	$(HARDDISK_OBJDIR)/harddisk.o

MPI_OBJS = \
	$(OBJDIR)/Fileops.o \
	$(MPI_OBJDIR)/mpi_res.o \
	$(MPI_OBJDIR)/mpi.o

ORCH90_OBJS = \
	$(OBJDIR)/Fileops.o \
	$(ORCH90_OBJDIR)/orch90_res.o \
	$(ORCH90_OBJDIR)/orch90.o

RAMDISK_OBJS = \
	$(RAMDISK_OBJDIR)/memboard.o \
	$(RAMDISK_OBJDIR)/ramdisk_res.o \
	$(RAMDISK_OBJDIR)/Ramdisk.o

SUPERIDE_OBJS = \
	$(OBJDIR)/Fileops.o \
	$(SUPERIDE_OBJDIR)/cloud9.o \
	$(SUPERIDE_OBJDIR)/IdeBus.o \
	$(SUPERIDE_OBJDIR)/logger.o \
	$(SUPERIDE_OBJDIR)/superide_res.o \
	$(SUPERIDE_OBJDIR)/SuperIDE.o

VCC_OBJS = \
	$(OBJDIR)/audio.o \
	$(OBJDIR)/Cassette.o \
	$(OBJDIR)/coco3.o \
	$(OBJDIR)/config.o \
	$(OBJDIR)/DirectDrawInterface.o \
	$(OBJDIR)/Fileops.o \
	$(OBJDIR)/hd6309.o \
	$(OBJDIR)/iobus.o \
	$(OBJDIR)/joystickinput.o \
	$(OBJDIR)/keyboard.o \
	$(OBJDIR)/keyboardLayout.o \
	$(OBJDIR)/logger.o \
	$(OBJDIR)/mc6809.o \
	$(OBJDIR)/mc6821.o \
	$(OBJDIR)/pakinterface.o \
	$(OBJDIR)/quickload.o \
	$(OBJDIR)/tcc1014graphics.o \
	$(OBJDIR)/tcc1014mmu.o \
	$(OBJDIR)/tcc1014registers.o \
	$(OBJDIR)/throttle.o \
	$(OBJDIR)/_xDebug.o \
	$(OBJDIR)/profiler.o \
	$(OBJDIR)/vcc_res.o \
	$(OBJDIR)/Vcc.o

all: $(OBJDIRS) $(DLLS) $(VCC) makefile
.PHONY: all

$(OBJDIRS):
	$(MD) -p $@
  
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
$(FD502_DLL): $(FD502_OBJS) $(FD502_RES)
	$(LD) -shared  $(FD502_OBJS) $(LDFLAGS) -o $@ -Wl,--out-implib,fd502.a

$(FD502_OBJDIR)/%.o: $(FD502_DIR)/%.c
	$(CPP) $(CFLAGS) -Dfd502_EXPORTS -c $< -o $@
	
$(FD502_OBJDIR)/%.o: $(FD502_DIR)/%.cpp
	$(CPP) -Dfd502_EXPORTS $(CPPFLAGS) -c $< -o $@

$(FD502_OBJDIR)/fd502_res.o: $(FD502_DIR)/fd502.rc
	windres --use-temp-file -i $< -o $@ --include-dir=resources

$(HARDDISK_DLL): $(HARDDISK_OBJS)
	$(LD) -shared  $(HARDDISK_OBJS) $(LDFLAGS) -o $@ -Wl,--out-implib,harddisk.a

$(HARDDISK_OBJDIR)/%.o: $(HARDDISK_DIR)/%.c
	$(CPP) $(CFLAGS) -Dharddisk_EXPORTS -c $< -o $@
	
$(HARDDISK_OBJDIR)/%.o: $(HARDDISK_DIR)/%.cpp
	$(CPP) -Dharddisk_EXPORTS $(CPPFLAGS) -c $< -o $@

$(HARDDISK_OBJDIR)/harddisk_res.o: $(HARDDISK_DIR)/harddisk.rc
	windres --use-temp-file -i $< -o $@ --include-dir=resources

$(MPI_DLL): $(MPI_OBJS)
	$(LD) -shared  $(MPI_OBJS) $(LDFLAGS) -o $@ -Wl,--out-implib,mpi.a

$(MPI_OBJDIR)/%.o: $(MPI_DIR)/%.c
	$(CPP) $(CFLAGS) -DMPI_EXPORTS -c $< -o $@
	
$(MPI_OBJDIR)/%.o: $(MPI_DIR)/%.cpp
	$(CPP) -DMPI_EXPORTS $(CPPFLAGS) -c $< -o $@

$(MPI_OBJDIR)/mpi_res.o: $(MPI_DIR)/mpi.rc
	windres --use-temp-file -i $< -o $@ --include-dir=resources

$(ORCH90_DLL): $(ORCH90_OBJS)
	$(LD) -shared  $(ORCH90_OBJS) $(LDFLAGS) -o $@ -Wl,--out-implib,orch90.a

$(ORCH90_OBJDIR)/%.o: $(ORCH90_DIR)/%.c
	$(CPP) $(CFLAGS) -DORCH90_EXPORTS -c $< -o $@
	
$(ORCH90_OBJDIR)/%.o: $(ORCH90_DIR)/%.cpp
	$(CPP) -DORCH90_EXPORTS $(CPPFLAGS) -c $< -o $@

$(ORCH90_OBJDIR)/orch90_res.o: $(ORCH90_DIR)/orch90.rc
	windres --use-temp-file -i $< -o $@ --include-dir=resources

$(RAMDISK_DLL): $(RAMDISK_OBJS)
	$(LD) -shared  $(RAMDISK_OBJS) $(LDFLAGS) -o $@ -Wl,--out-implib,ramdisk.a

$(RAMDISK_OBJDIR)/%.o: $(RAMDISK_DIR)/%.c
	$(CPP) $(CFLAGS) -DRAMDISK_EXPORTS -c $< -o $@
	
$(RAMDISK_OBJDIR)/%.o: $(RAMDISK_DIR)/%.cpp
	$(CPP) -DRAMDISK_EXPORTS $(CPPFLAGS) -c $< -o $@

$(RAMDISK_OBJDIR)/ramdisk_res.o: $(RAMDISK_DIR)/ramdisk.rc
	windres --use-temp-file -i $< -o $@ --include-dir=resources

$(SUPERIDE_DLL): $(SUPERIDE_OBJS)
	$(LD) -shared  $(SUPERIDE_OBJS) $(LDFLAGS) -o $@ -Wl,--out-implib,superide.a

$(SUPERIDE_OBJDIR)/%.o: $(SUPERIDE_DIR)/%.c
	$(CPP) $(CPPFLAGS) -SUPERIDE_EXPORTS -c $< -o $@
	
$(SUPERIDE_OBJDIR)/%.o: $(SUPERIDE_DIR)/%.cpp
	$(CPP) -DSUPERIDE_EXPORTS $(CPPFLAGS) -c $< -o $@

$(SUPERIDE_OBJDIR)/superide_res.o: $(SUPERIDE_DIR)/superide.rc
	windres --use-temp-file -i $< -o $@ --include-dir=resources

$(VCC): $(VCC_OBJS)
	$(LD) $(VCC_OBJS) $(VCC_LIBS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: %.c
	$(CPP) $(CFLAGS) -c $< -o $@
	
$(OBJDIR)/%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/vcc_res.o: vcc.rc
	windres --use-temp-file -i $< -o $@ --include-dir=resources

clean:
	$(ECHO) Deleting object files...
	$(RM) -f $(FD502_OBJDIR)/*.o
	$(RM) -f $(HARDDISK_OBJDIR)/*.o
	$(RM) -f $(MPI_OBJDIR)/*.o
	$(RM) -f $(ORCH90_OBJDIR)/*.o
	$(RM) -f $(RAMDISK_OBJDIR)/*.o
	$(RM) -f $(SUPERIDE_OBJDIR)/*.o
	$(RM) -f $(OBJDIR)/*.o
	$(ECHO) Deleting dlls and executables...
	$(RM) -f $(DLLS)
	$(RM) -f $(VCC)
