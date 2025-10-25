#pragma once
#include "CartridgeTrampolines.h"
#include "detail/default_handlers.h"
#include <string>

class Cartridge
{
protected:

	Cartridge();


public:

	virtual ~Cartridge();


	virtual void SetHostKey(void* key);
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


protected:

	void AssetCartridgeLine(bool state) const
	{
		AssetCartridgeLinePtr(m_HostKey, state);
	}

	void AddMenuItem(const char * name, int id, MenuItemType type)
	{
		AddMenuItemPtr(m_HostKey, name, id, type);
	}

	virtual void LoadConfiguration(const std::string& filename);
	virtual void LoadMenuItems();

private:

	friend const char* PakGetName();
	friend const char* PakCatalogName();
	friend void PakInitialize(
		void* const host_key,
		const char* const configuration_path,
		const pak_initialization_parameters* const parameters);
	friend void PakGetStatus(char* text_buffer, size_t buffer_size);
	friend void PakMenuItemClicked(unsigned char menuId);
	friend void PakReset();
	friend unsigned char PakReadMemoryByte(unsigned short address);
	friend void PakWritePort(unsigned char port, unsigned char data);
	friend unsigned char PakReadPort(unsigned char port);
	friend unsigned short PakSampleAudio();

private:

	static Cartridge*	m_Singleton;

	void*				m_HostKey = nullptr;
	std::string			m_Name;
	std::string			m_CatalogId;
	std::string			m_ConfigurationPath;
	PakAssertCartridgeLineHostCallback	AssetCartridgeLinePtr = detail::NullAssetCartridgeLine;
	PakAppendCartridgeMenuHostCallback 	AddMenuItemPtr = detail::NullAddMenuItem;
};

