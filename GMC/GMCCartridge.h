#pragma once
#include "Cartridge.h"
#include "ROM.h"
#include "sn76496.h"
#include "Configuration.h"


class GMCCartridge : public Cartridge
{
public:

	GMCCartridge();

	void LoadConfiguration(const std::string& filename) override;
	void LoadMenu() override;

	std::string GetStatusMessage() const override;
	void OnMenuItemSelected(unsigned char menuId) override;
	void OnReset() override;
	unsigned short UpdateAudio() override;
	unsigned char OnReadMemory(unsigned short address) const override;
	void OnWritePort(unsigned char port, unsigned char data) override;
	unsigned char OnReadPort(unsigned char port) const override;

private:

	struct MenuItems
	{
		static const unsigned int SelectRom = 3;
	};

	struct Ports
	{
		static const unsigned char BankSelect = 0x40;
		static const unsigned char PSG = 0x41;
	};

	ROM				m_ROMImage;
	SN76489Device	m_PSG;
	Configuration	m_Configuration;
};


