#pragma once
#include "Cartridge.h"
#include "ROM.h"
#include "sn76496.h"
#include "Configuration.h"


class GMCCartridge : public Cartridge
{
public:

	void LoadConfiguration(const std::string& filename) override;

	std::string GetStatusMessage() const override;
	void OnMenuItemSelected(unsigned char menuId) override;
	void OnReset() override;
	unsigned short UpdateAudio() override;
	unsigned char OnReadMemory(unsigned short address) const override;
	void OnWritePort(unsigned char port, unsigned char data) override;
	unsigned char OnReadPort(unsigned char port) const override;
	bool GetMenuItem(menu_item_entry* item, size_t index) override;

private:

	struct MenuItems
	{
		static const unsigned int SelectRom = 10;
	};

	struct Ports
	{
		static const unsigned char BankSelect = 0;
		static const unsigned char PSG = 1;
	};

	ROM				m_ROMImage;
	SN76489Device	m_PSG;
	Configuration	m_Configuration;
};


