#pragma once
#include "vcc/devices/psg/sn76496.h"
#include "vcc/devices/rom/banked_rom_image.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/cartridge_context.h"
#include <memory>
#include <Windows.h>


class superide_cartridge : public ::vcc::bus::cartridge
{
public:

	using context_type = ::vcc::bus::cartridge_context;
	using path_type = std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;


public:

	superide_cartridge(
		std::unique_ptr<context_type> context,
		HINSTANCE module_instance);

	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;

	void start() override;
	void stop() override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;

	void status(char* status_buffer, size_t buffer_size) override;

	void menu_item_clicked(unsigned char menu_item_id) override;


private:

	void LoadConfig();
	void BuildCartridgeMenu();


private:

	const std::unique_ptr<context_type> context_;
	const HINSTANCE module_instance_;
};
