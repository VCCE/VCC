#pragma once
#include "sn76496.h"
#include <vcc/core/cartridge.h>
#include <vcc/core/cartridge_context.h>
#include <vcc/core/banked_rom_image.h>
#include <memory>
#include <Windows.h>


class gmc_cartridge : public ::vcc::core::cartridge
{
public:

	using context_type = ::vcc::core::cartridge_context;
	using path_type = ::std::string;
	using rom_image_type = ::vcc::core::banked_rom_image;


public:

	gmc_cartridge(std::unique_ptr<context_type> context, HINSTANCE module_instance);

	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;

	void start() override;
	void stop() override;
	void reset() override;
	void process_horizontal_sync() override;
	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;
	unsigned char read_memory_byte(unsigned short memory_address) override;
	void status(char* status_buffer, size_t buffer_size) override;
	unsigned short sample_audio() override;
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


