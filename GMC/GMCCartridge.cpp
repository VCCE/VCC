#include "GMCCartridge.h"
#include <Windows.h>


GMCCartridge::GMCCartridge()
	: Cartridge("Game Master Catridge", "SN76489")
{}


void GMCCartridge::LoadConfiguration(const std::string& filename)
{
	Cartridge::LoadConfiguration(filename);
	m_Configuration = Configuration(filename);
}

void GMCCartridge::LoadMenuItems()
{
    AddMenuItem("",MID_BEGIN,MIT_Head);
    AddMenuItem("",MID_ENTRY,MIT_Slave);
    AddMenuItem("Select GMC ROM", MID_SDYNAMENU + MenuItems::SelectRom, MIT_StandAlone);
    AddMenuItem("",MID_FINISH,MIT_Head);
}

std::string GMCCartridge::GetStatusMessage() const
{
	std::string message("GMC Active");

	const auto activeRom(ExtractFilename(m_ROMImage.GetFilename()));
	if (m_ROMImage.HasImage())
	{
		message += " (" + activeRom + " Loaded)";
	}
	else
	{
		message += activeRom.empty() ? " (No ROM Selected)" : "(Unable to load `" + activeRom + "`)";
	}

	return message;
}


void GMCCartridge::OnMenuItemSelected(unsigned char menuId)
{
	if (menuId == MenuItems::SelectRom)
	{
		const auto selectedFile(SelectROMFile());
		if (selectedFile.empty())
		{
			return;
		}

		MessageBoxA(
			nullptr,
			"A new ROM file has been selected. Press F9 to reset\nyour Color Computer and activate the ROM.",
			"ROM Selected",
			MB_OK);

		m_Configuration.SetActiveRom(selectedFile.data());

		AssetCartridgeLine(false);
		AssetCartridgeLine(true);

	}
}

void GMCCartridge::OnReset()
{
	m_PSG.device_start();

	if (m_ROMImage.Load(m_Configuration.GetActiveRom()))
	{
		AssetCartridgeLine(false);
		AssetCartridgeLine(true);
	}
	else
	{
		m_ROMImage.Unload();
	}
}




unsigned short GMCCartridge::UpdateAudio()
{
	SN76489Device::stream_sample_t lbuffer, rbuffer;

	return m_PSG.sound_stream_update(lbuffer, rbuffer);
}




unsigned char GMCCartridge::OnReadMemory(unsigned short address) const
{
	return m_ROMImage.Read(address);
}


void GMCCartridge::OnWritePort(unsigned char port, unsigned char data)
{
	switch (port)
	{
	case Ports::BankSelect:
		m_ROMImage.SetBank(data);
		break;

	case Ports::PSG:
		m_PSG.write(data);
		break;
	}
}


unsigned char GMCCartridge::OnReadPort(unsigned char port) const
{
	if (port == Ports::BankSelect)
	{
		return m_ROMImage.GetBank();
	}

	return 0;
}


