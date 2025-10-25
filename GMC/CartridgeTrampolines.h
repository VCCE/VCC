#pragma once
#include "GMC.h"

GMC_EXPORT const char* PakGetName();
GMC_EXPORT const char* PakGetCatalogId();
GMC_EXPORT void PakInitialize(
	void* const host_key,
	const char* const configuration_path,
	const cpak_cartridge_context* const context);

GMC_EXPORT void PakGetStatus(char* text_buffer, size_t buffer_size);
GMC_EXPORT void PakMenuItemClicked(unsigned char /*menuId*/);
GMC_EXPORT void PakReset();
GMC_EXPORT unsigned char PakReadMemoryByte(unsigned short address);
GMC_EXPORT void PakWritePort(unsigned char port, unsigned char data);
GMC_EXPORT unsigned char PakReadPort(unsigned char port);
GMC_EXPORT unsigned short PakSampleAudio();
