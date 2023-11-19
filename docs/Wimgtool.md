<h2 align="center">Using wimgtool</h2>

 The *"wimgtool-os9.exe" & "wimgtool-rsdos.exe"* virtual disk utilities
 were derived from the old M.E.S.S. utility "wimgtool.exe". Each of
 these utilities have been modified to serve a specific purpose to work
 with OS-9 formatted disks and RSDOS formatted disks respectively. We 
 are unable credit to the individual who modified these utilities because
 we don't know who it was.

 Both utilities have the same usage with only minor differences, and
 those differences will be explained here. These utilities differ from
 their parent utility in that they do not work with virtual disks for
 non-coco systems, even though you can create and format a new disk for
 that system, the image functions are geared specifically for OS-9 and
 RSDOS. The utilities were created to fix a few bugs for these formats
 that existed in the original utility.

**Wimgtool-os9.exe** - Move OS-9 files to and from your PC and an
    OS-9 formatted disk image

  - **File [ALT-F]** - File related functions
    - **New [CNTRL-N]** - Create a *new* formatted OS-9 disk
      image. Navigate to the desired directory in which you want
      your disk created.
    - **Filename** - Type in the disk name without the  extension.
    - **Save as** type - Select the desired system type from
      the pull down menu. To create an OS-9 formatted disk for
      Mame, select "CoCo OS-9 disk image (OS-9 format)(\*.os9)"
      with the extension "os9" (required by Mame) or "CoCo JVC
      disk image (OS-9 format) (\*.dsk)" used by most other emulators
      (including VCC).
    - **More** - Expands the disk type menu to allow you to
      select more advanced options for your new disk image.
      Clicking "**Less**" collapses this advanced menu.
      - **Heads** - Number of disk sides (Usually 1 or 2 for OS-9)
      - **Tracks** - Number of tracks on the disk image
        (Usually 35, 40, or 80 for OS-9)
      - **Sectors** - Number of sectors per track (Usually 18 for OS-9)
      - **Sector Bytes** - Number of bytes per sector (Usually 256 for OS-9)
      - **Interleave** - Not used for these disk types
      - **First Sector** - Select "0" or "1" for the
        starting sector \# (usually 1)
    - **Save** - Saves the new disk image to your PC
    - **Cancel** - Cancels the new disk creation
  - **Open [CNTRL-O]** - Open an existing OS-9 formatted
      disk image. Navigate to the desired directory containing the
      disk image you want to load, select the image and click "Open".
  - **Close [CNTRL-W]** - Close the current open disk
      image, saving the new image over the original disk image.
      Clicking the "X" to close the program will produce the same
      results and the modified disk image will be saved.
  - **Image [ALT-I]** - File to disk and disk to file functions
    - **Insert File [CNTRL-I]** - Inserts an OS-9 file (or
      any ASCII text file) onto the OS-9 formatted disk image. Use
      the "Open" dialog to navigate to the desired file and click
      "Open" to insert the file or "Cancel" to cancel the operation.
    - **Extract [CNTRL-E]** - Extracts a selected file or
      files to the PC hard drive. Use the "Save As" dialog to
      navigate to the desire location you want to save the file,
      then select the desired format from the "Save as type" (OS-9
      file will have only one option available) pull down menu and
      click "Save" to save the file or "Cancel" to cancel the operation.
    - **New Directory [CNTRL-Y]** - Creates a new OS-9
      directory on the current disk image. Each new directory can
      be clicked and opened to create multiple sub-directory levels.
    - **Delete [DEL]** - Deletes the selected file(s) from the
      OS-9 formatted disk image. CAUTION: This operation cannot be undone.
    - **View Sector Data [CNTRL-V]** - Opens the "Sector
      Viewer" utility. From this utility, you can view the raw
      data contained on your disk image, sector by sector.
      - **Track** - Sets the track number from which you are viewing.
      - **Side** - Selects the disk "side" in which you are viewing.
      - **Sector** - Selects the actual sector shown in the viewer.
      - **OK** - Exits the viewer
  - **View [ALT-V]** - Sets the interface viewing style and file associations.
    - **Icons** - Sets the viewing style to "Icons", for a more
      "Windows" file list view.
    - **List** - Sets the viewing style to a simple filename list
      with less information.
    - **Details** - (Default) Sets the viewing style to a more detailed
      list showing the "Filename", :Size", "Attributes", and "Notes".
  - **File Associations** - Select the disk file extensions you
    want associated with the utility, then when this type of
    file is double clicked in Windows, it will open with this utility.
  - **Status Bar** - The Status Bar at the bottom of the window
    shows the file(s) selected, the name of the current disk image
    and the remaining free bytes on the disk image.

**Wimgtool-RSDOS.exe** - Move RSDOS formatted files to and from
    your PC hard drive and RSDOS formatted disk image.

  - **File [ALT-F]** - File related functions
    - **New [CNTRL-N]** - Create a *new* formatted RSDOS disk image.
      Navigate to the desired directory in which you want your disk created.
      - **Filename** - Type in the disk name without the extension.
      - **Save as** type - Select the desired system type from
        the pulldown menu. To create an OS-9 formatted disk,
        select "CoCo JVC disk image (RS-DOS format) (\*.dsk)"
        used by most emulators (including VCC).
      - **More** - Expands the disk type menu to allow you to
        select more advanced options for your new disk image.
        Clicking "**Less**" collapses this advanced menu.
      - **Heads** - Number of disk sides (usually 1 for RSDOS)
      - **Tracks** - Number of tracks on the disk image (Usually 35 for RSDOS)
      - **Sectors** - Number of sectors per track (Usually 18 for RSDOS)
      - **Sector Bytes** - Number of bytes per sector (Usually 256 for RSDOS)
      - **Interleave** - Not used for these disk types
      - **First Sector** - Select "0" or "1" for the
        starting sector \# (usually 1)
      - **Save** - Saves the new disk image to your PC
      - **Cancel** - Cancels the new disk creation
    - **Open [CNTRL-O]** - Open an existing RSDOS formatted disk image.
      Navigate to the desired directory containing the disk image you
      want to load, select the image and click "Open".
    - **Close [CNTRL-W]** - Close the current open disk
      image, saving the new image over the original disk image.
      Clicking the "X" to close the program will produce the same
      results and the modified disk image will be saved.
  - **Image [ALT-I]** - File to disk and disk to file functions
    - **Insert File [CNTRL-I]** - Inserts an RSDOS file (or
      any ASCII text file) onto the RSDOS formatted disk image.
      Use the "Open" dialog to navigate to the desired file and
      click "Open" to insert the file or "Cancel" to cancel the
      operation. Once you click "open" the following dialog appears.
      - **File Options** - Select the desired file attributes
        to use on the RSDOS file
        - **Mode** - Select the file mode for the selected file..
         - **Raw** - saves the file to disk in raw binary
           mode. Use for tokenised BASIC and machine language file types.
        - **ASCII** - Saves the file in ASCII text
          format.
      - **File Type** - Selects the file type in which the
        selected file is saved to disk.
        - **Basic** - Use for CB, ECB, DECB, and SECB standard Basic files.
        - **Data** - Used by certain text editors and database programs
        - **Binary** - For raw binary data formatted files
        - **Assembler Source** - Used by EDTASM ans
          various other RSDOS assemblers.
      - **ASCII Flag** - Flags the file as ASCII or binary
        - **ASCII** - Flags the file as ASCII text
        - **Binary** - Flags the file as raw binary database
      - **OK** - Completes the file transfer and returns to
        the disk image display. Click "**Cancel**" to cancel
        the file transfer and return to the disk image display
   - **Extract [CNTRL-E]** - Extracts a selected file or
     files to the PC hard drive. Use the "Save As" dialog to
     navigate to the desire location you want to save the file,
     then select the desired format from the "Save as type"
     (RSDOS will have only one option available) pull down menu
     and click "**Save**" to save the file or "**Cancel**" to
     cancel the operation.
    - **Mode** - Select the mode in which the file is transferred
      - **ASCII** - Save the file in ASCII text mode.
      - **Raw** - Saves the file in raw binary mode.
  - **New Directory [CNTRL-Y]** - Not available for RSDOS disk images.
  - **Delete [DEL]** - Deletes the selected file(s) from the-
    RSDOS formatted disk image. CAUTION: This operation cannot be undone.
  - **View Sector Data [CNTRL-V]** - Opens the "Sector Viewer" utility.
    From this utility, you can view the raw data contained on your disk
    image, sector by sector.
      - **Track** - Sets the track number from which you are viewing.
      - **Side** - Selects the disk "side" in which you are viewing.
      - **Sector** - Selects the actual sector shown in the viewer.
      - **OK** - Exits the viewer
  - **View [ALT-V]** - Sets the interface viewing style and file associations.
    - **Icons** - Sets the viewing style to "Icons", for a more
      "Windows" file list view.
    - **List** - Sets the viewing style to a simple filename list
      with less information.
    - **Details** - (Default) Sets the viewing style to a more
      detailed list showing the "Filename", :Size", "Attributes", and "Notes".
    - **File Associations** - Select the disk file extensions you
      want associated with the utility, then when this type of
      file is double clicked in Windows, it will open with this utility.
  - **Status Bar** - The Status Bar at the bottom of the window
    shows the file(s) selected, the name of the current disk image
    and the remaining free bytes on the disk image.

