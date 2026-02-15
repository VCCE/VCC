#pragma once
#include "CartridgeTrampolines.h"
#include <string>

namespace detail
{
	void NullAssetCartridgeLine(slot_id_type, bool);
	void NullAddMenuItem(slot_id_type, const char*, int, MenuItemType);
}

class Cartridge
{
protected:

	Cartridge();


public:

	virtual ~Cartridge();


	virtual void SetSlotId(slot_id_type SlotId);
	virtual void SetCartLineAssertCallback(PakAssertCartridgeLineHostCallback callback);
	virtual void SetMenuBuilderCallback(PakAppendCartridgeMenuHostCallback addMenuCallback);
	virtual void SetConfigurationPath(std::string path);

	virtual std::string GetStatusMessage() const;
	virtual void OnMenuItemSelected(unsigned char menuId);

	virtual void OnReset();
	virtual unsigned short UpdateAudio();
	virtual unsigned char OnReadMemory(unsigned short address) const;
	virtual void OnWritePort(unsigned char port, unsigned char data);
	virtual unsigned char OnReadPort(unsigned char port) const;
	virtual bool GetMenuItem(menu_item_entry* item, size_t index);

protected:

	void AssetCartridgeLine(bool state) const
	{
		AssetCartridgeLinePtr(m_SlotId, state);
	}

	void AddMenuItem(const char * name, int id, MenuItemType type)
	{
		AddMenuItemPtr(m_SlotId, name, id, type);
	}

	virtual void LoadConfiguration(const std::string& filename);
	virtual void LoadMenuItems();

private:

	friend const char* PakGetName();
	friend const char* PakGetCatalogId();
	friend void PakInitialize(
		slot_id_type SlotId,
		const char* const configuration_path,
		HWND hVccWnd,
		const cpak_callbacks* const callbacks);
	friend void PakGetStatus(char* text_buffer, size_t buffer_size);
	friend void PakMenuItemClicked(unsigned char menuId);
	friend void PakReset();
	friend unsigned char PakReadMemoryByte(unsigned short address);
	friend void PakWritePort(unsigned char port, unsigned char data);
	friend unsigned char PakReadPort(unsigned char port);
	friend unsigned short PakSampleAudio();
	friend bool PakGetMenuItem(menu_item_entry* item, size_t index);

private:

	static Cartridge*	m_Singleton;

	slot_id_type		m_SlotId {};
	std::string			m_Name;
	std::string			m_CatalogId;
	std::string			m_ConfigurationPath;
	PakAssertCartridgeLineHostCallback	AssetCartridgeLinePtr = detail::NullAssetCartridgeLine;
	PakAppendCartridgeMenuHostCallback 	AddMenuItemPtr = detail::NullAddMenuItem;
};

