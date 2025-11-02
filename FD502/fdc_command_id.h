////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#pragma once
/// @file
/// 
/// @brief Defines details about FDC command identifiers.


namespace vcc::cartridges::fd502::detail
{

	// TODO-CHET: place in ::vcc::devices::storage::wd1793::detail?? or is this 
	// something the derived classes need (probably not)?

	/// @brief Defines the commands of the WD179x FDC interface.
	/// 
	/// @todo add proper description.
	/// 
	/// Taken from https://hansotten.file-hunter.com/technical-info/wd1793/ and the WD179x
	/// datasheets.
	enum class fdc_command_id
	{
		/// @brief Restore the drive head to track 0.
		///
		/// Upon receipt of this command, the TR00 input is sampled. If TR00 is active (low)
		/// indicating the head is positioned over track 0, the Track Register is loaded with
		/// zeroes and an interrupt is generated. If TR00 is not active, stepping pulses at a
		/// rate specified by the r1 r0 field are issued until the TR00 input is activated.
		/// At this time, the Track Register is loaded with zeroes and an interrupt is
		/// generated.
		restore,
		/// @brief Moves the drive head to a specific track.
		///
		/// This command assumes that the Track Register contains the track number of the
		/// current position of the head and the Data Register contains the desired track
		/// number. The FD179X will update the Track Register and issue stepping pulses in
		/// the appropriate direction until the contents of the Track Register are equal to
		/// the contents of the Data Register. An interrupt is generated at the completion of
		/// the command. Note: when using multiple drives, the track register must be updated
		/// for the drive selected before seeks are issued.
		seek,
		/// @brief Steps the drive head to an adjacent track.
		///
		/// Upon receipt of this command, the FD179X issues one stepping pulse to the disk
		/// drive. The stepping direction motor direction is the same as in the previous step
		/// command. An interrupt is generated at the end of the command.
		step,
		/// @brief Steps the drive head to an adjacent track and update the track register
		/// with the new track.
		/// 
		/// Upon receipt of this command, the FD179X issues one stepping pulse to the disk
		/// drive. The stepping direction motor direction is the same as in the previous step
		/// command. After each step has complete the track register is updated with the new
		/// track number (TODO-CHET: VERIFY WHEN THE UPDATE OCCURS). An interrupt is
		/// generated at the end of the command.
		step_upd,
		/// @brief Steps the drive head one track in the direction of the highest track.
		/// 
		/// Upon receipt of this command, the FD179X issues one stepping pulse in the
		/// direction towards track 76. An interrupt is generated at the end of the command.
		step_in,
		/// @brief Steps the drive head one track in the direction of the highest track and
		/// update the track register with the new track.
		/// 
		/// Upon receipt of this command, the FD179X issues one stepping pulse in the
		/// direction towards track 76. After each step has complete the track register is
		/// updated with the new track number (TODO-CHET: VERIFY WHEN THE UPDATE OCCURS). An
		/// interrupt is generated at the end of the command.
		step_in_upd,
		/// @brief Steps the drive head one track in the direction of the lowest track and
		/// update the track register with the new track.
		/// 
		/// Upon receipt of this command, the FD179X issues one stepping pulse in the
		/// direction towards track 0. An interrupt is generated at the end of the command.
		step_out,
		/// @brief Steps the drive head one track in the direction of the lowest track and
		/// update the track register with the new track.
		/// 
		/// Upon receipt of this command, the FD179X issues one stepping pulse in the
		/// direction towards track 0. After each step has complete the track register is
		/// updated with the new track number (TODO-CHET: VERIFY WHEN THE UPDATE OCCURS). An
		/// interrupt is generated at the end of the command.
		step_out_upd,
		/// @brief Read sector from the disk.
		/// 
		/// Upon receipt of the command, the head is loaded, the busy status bit set and when
		/// an ID field is encountered that has the correct track number, correct sector
		/// number, correct side number, and correct CRC, the data field is presented to the
		/// computer. An DRQ is generated each time a byte is transferred to the DR. At the
		/// end of the Read operation, the type of Data Address Mark encountered in the data
		/// field is recorded in the Status Register (bit 5).
		read_sector,
		/// @brief Read multiple sectors from the disk.
		/// 
		/// @todo add details
		read_sector_m,
		/// @brief Write a sector to the disk.
		/// 
		/// Upon receipt of the command, the head is loaded, the busy status bit set and when
		/// an ID field is encountered that has the correct track number, correct sector
		/// number, correct side number, and correct CRC, a DRQ is generated. The FD179X
		/// counts off 22 bytes (in double density) from the CRC field and the Write Gate
		/// output is made active if the DRQ is serviced (ie. the DR has been loaded by the
		/// computer). If DRQ has not been serviced, the command is terminated and the Lost
		/// Data status bit is set. If the DRQ has been serviced, 12 bytes of zeroes (in
		/// double density) are written to the disk, then the Data Address Mark as determined
		/// by the a0 field of the command. The FD179X then writes the data field and
		/// generates DRQ’s to the computer. If the DRQ is not serviced in time for
		/// continuous writing the Lost Data Status bit is set and a byte of zeroes is written
		/// on the disk (the command is not terminated). After the last data byte has been
		/// written on the disk, the two-byte CRC is computed internally and written on the
		/// disk followed by one byte of logic ones.
		write_sector,
		/// @brief Write multiple sectors to the disk.
		/// 
		/// @todo add details
		write_sector_m,
		/// @brief Reads the sector record (address) of the next sector encountered.
		/// 
		/// Upon receipt of the Read Address command, the head is loaded and the Busy Status
		/// bit is set. The next encountered ID field is then read in from the disk, and the
		/// six data bytes of the ID field are assembled and transferred to the DR, and a DRQ
		/// is generated for each byte. The six bytes of the ID field are : Track address,
		/// Side number, Sector address, Sector Length, CRC1, CRC2. Although the CRC bytes
		/// are transferred to the computer, the FD179X checks for validity and the CRC error
		/// status bit is set if there is a CRC error. The track address of the ID field is
		/// written into the sector register so that a comparison can be made by the user. At
		/// the end of the operation, an interrupt is generated and the Busy status bit is
		/// reset.
		read_address,
		/// @brief Cancel the current command, if any, and generate an interrupt.
		/// 
		/// The Forced Interrupt command is generally used to terminate a multiple sector
		/// read or write command or insure Type I status register. This command can be
		/// loaded into the command register at any time. If there is a current command
		/// under execution (busy status bit set), the command will be terminated and the
		/// busy status bit reset.
		force_interupt,
		/// @brief Read a track from the disk.
		/// 
		/// Upon receipt of the Read Track command, the head is loaded, and the busy status
		/// bit is set. Reading starts with the leading edge of the first encountered index
		/// pulse and continues until the next index pulse. All gap, header, and data bytes
		/// are assembled and transferred to the data register and DRQ’s are generated for
		/// each byte. The accumulation of bytes is synchronized to each address mark
		/// encountered. An interrupt is generated at the completion of the command. The ID
		/// Address Mark, ID field, ID CRC bytes, DAM, Data and Data CRC bytes for each sector
		/// will be correct. The gap bytes may be read incorrectly during write-splice time
		/// because of synchronization.
		read_track,
		/// @brief Write a track to the disk.
		/// 
		/// Upon receipt of the Write Track command, the head is loaded and the Busy Status
		/// bit is set. Writing starts with the leading edge of the first encountered index
		/// pulse and continues until the next index pulse, at which time the interrupt is
		/// activated. The Data Request is activated immediately upon receiving the command,
		/// but writing will not start until after the first byte has been loaded into the
		/// DR. If the DR has not been loaded by the time the index pulse is encountered, the
		/// operation is terminated making the device Not Busy, the Lost Data status bit is
		/// set, and the interrupt is activated. If a byte is not present in the DR when
		/// needed, a byte of zeroes is substituted. This sequence continues from one index
		/// mark to the next index mark. Normally, whatever data pattern appears in the data
		/// register is written on the disk with a normal clock pattern. However, if the
		/// FD179X detects a data pattern of F5 thru FE in the data register, this is
		/// interpreted as data address marks with missing clocks or CRC generation. The CRC
		/// generator is initialized when an F5 data byte is about to be transferred (in MFM).
		/// An F7 pattern will generate two CRC bytes. As a consequence, the patterns F5 thru
		/// FE must not appear in the gaps, data fields, or ID fields. Tracks may be formatted
		/// with sector lengths of 128, 256, 512 or 1024 bytes.
		write_track
	};

}
