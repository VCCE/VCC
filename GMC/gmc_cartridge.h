#pragma once
#include "sn76496.h"
#include <vcc/devices/rom/banked_rom_image.h>
#include <vcc/bus/cartridge.h>
#include <vcc/bus/cartridge_context.h>
#include <memory>
#include <Windows.h>


class gmc_cartridge : public ::vcc::bus::cartridge
{
public:

	using context_type = ::vcc::bus::cartridge_context;
	using path_type = std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;


public:

	gmc_cartridge(std::unique_ptr<context_type> context, HINSTANCE module_instance);

	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;

	void start() override;
	void stop() override;
	void reset() override;

	unsigned char read_memory_byte(size_type memory_address) override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;

	void process_horizontal_sync() override;

	unsigned short sample_audio() override;

	void status(char* status_buffer, size_t buffer_size) override;
	void menu_item_clicked(unsigned char menu_item_id) override;


protected:

	void load_rom(const path_type& filename, bool reset_on_load);
	void build_menu();


private:

	struct menu_item_ids
	{
		static const unsigned int select_rom = 3;
	};

	struct mmio_registers
	{
		static const unsigned char select_bank = 0x40;
		static const unsigned char psg_io = 0x41;
	};

	static const path_type configuration_section_id_;
	static const path_type configuration_rom_key_id_;


	const std::unique_ptr<context_type> context_;
	const HINSTANCE module_instance_;
	rom_image_type rom_image_;
	SN76489Device psg_;
};


