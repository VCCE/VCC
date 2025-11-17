#pragma once
#include "vcc/devices/psg/sn76496.h"
#include "vcc/devices/rom/banked_rom_image.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_bus.h"
#include <memory>
#include <Windows.h>


class superide_cartridge : public ::vcc::bus::cartridge
{
public:

	using expansion_bus_type = ::vcc::bus::expansion_bus;
	using path_type = std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;


public:

	superide_cartridge(
		std::unique_ptr<expansion_bus_type> bus,
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

	const std::unique_ptr<expansion_bus_type> bus_;
	const HINSTANCE module_instance_;
};
