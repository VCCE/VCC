#pragma once
#include "CartridgeTrampolines.h"
#include <string>


class Cartridge
{
protected:

	explicit Cartridge(std::string name, std::string catalogId = std::string());


public:

	virtual ~Cartridge();

	virtual std::string GetName() const;
	virtual std::string GetCatalogId() const;

	virtual void SetCartLineAssertCallback(SETCART callback);
	virtual void SetMenuBuilderCallback(DYNAMICMENUCALLBACK addMenuCallback);
	virtual void SetConfigurationPath(std::string path);

	virtual std::string GetStatusMessage() const;
	virtual void OnMenuItemSelected(unsigned char menuId);

	virtual void OnReset();
	virtual unsigned short UpdateAudio();
	virtual unsigned char OnReadMemory(unsigned short address) const;
	virtual void OnWritePort(unsigned char port, unsigned char data);
	virtual unsigned char OnReadPort(unsigned char port) const;


protected:

	enum class ItemType
	{
		Head,
		Slave,
		StandAlone
	};

	void AssetCartridgeLine(bool state)
	{
		AssetCartridgeLinePtr(state);
	}

	void AddMenuSeparator()
	{
		AddMenuItem("", 6000, ItemType::Head);
	}

	void AddMenuItem(const std::string& name, int id, ItemType type)
	{
		AddMenuItemPtr(name.c_str(), id, static_cast<int>(type));
	}

	virtual void LoadConfiguration(const std::string& filename);
	virtual void LoadMenu();



private:

	friend void ModuleName(char *ModName, char *CatNumber, DYNAMICMENUCALLBACK addMenuCallback);
	friend void ModuleStatus(char *statusBuffer);
	friend void ModuleConfig(unsigned char menuId);
	friend void SetIniPath(const char *iniFilePath);
	friend void ModuleReset(void);
	friend void SetCart(SETCART Pointer);
	friend unsigned char PakMemRead8(unsigned short address);
	friend void PackPortWrite(unsigned char port, unsigned char data);
	friend unsigned char PackPortRead(unsigned char port);
	friend unsigned short ModuleAudioSample(void);

private:

	static Cartridge*	m_Singleton;

	std::string			m_Name;
	std::string			m_CatalogId;
	std::string			m_ConfigurationPath;
	SETCART				AssetCartridgeLinePtr;
	DYNAMICMENUCALLBACK AddMenuItemPtr;
};