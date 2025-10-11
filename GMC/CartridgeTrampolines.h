#pragma once
#include "GMC.h"


GMC_EXPORT void ModuleName(char *ModName, char *CatNumber, AppendCartridgeMenuModuleCallback addMenuCallback);
GMC_EXPORT void ModuleStatus(char *statusBuffer);
GMC_EXPORT void ModuleConfig(unsigned char /*menuId*/);
GMC_EXPORT void SetIniPath(const char *iniFilePath);
GMC_EXPORT void ModuleReset();
GMC_EXPORT void SetCart(AssertCartridgeLineModuleCallback Pointer);
GMC_EXPORT unsigned char PakMemRead8(unsigned short address);
GMC_EXPORT void PackPortWrite(unsigned char port, unsigned char data);
GMC_EXPORT unsigned char PackPortRead(unsigned char port);
GMC_EXPORT unsigned short ModuleAudioSample();
