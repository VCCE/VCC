// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
#pragma once
#include "vcc/detail/exports.h"
#include <cstdint>


namespace vcc::devices::psg
{

	class LIBCOMMON_EXPORT sn76489_device
	{
	public:

		using sample_type = unsigned short;

		sn76489_device() = default;
		virtual ~sn76489_device() = default;

		virtual void start();
		virtual void reset();
		virtual void write(uint8_t data);
		virtual sample_type sound_stream_update(sample_type& lbuffer, sample_type& rbuffer);


	private:

		virtual void initialize();
		bool in_noise_mode() const;
		void countdown_cycles();


	private:

		const int32_t	feedback_mask_ = 0x4000;	// mask for feedback
		const int32_t	whitenoise_tap1_ = 0x01;	// mask for white noise tap 1 (higher one, usually bit 14)
		const int32_t	whitenoise_tap2_ = 0x02;	// mask for white noise tap 2 (lower one, usually bit 13)
		const bool		negate_ = false;			// output negate flag
		const bool		stereo_ = false;			// whether we're dealing with stereo or not
		const float		period_divider_ = 6.11f;	// period divider
		const bool		ncr_style_psg_ = false;		// flag to ignore writes to regs 1,3,5,6,7 with bit 7 low
		const bool		sega_style_psg_ = true;		// flag to make frequency zero acts as if it is one more than max (0x3ff+1) or if it acts like 0; the initial register is pointing to 0x3 instead of 0x0; the volume reg is preloaded with 0xF instead of 0x0

		int32_t         volume_table_[16] = { 0 };	// volume table (for 4-bit to db conversion)

		bool            ready_state_ = false;
		int32_t         cycles_to_ready_ = 0;		// number of cycles until the READY line goes active
		int32_t         registers_[8] = { 0 };		// registers
		int32_t         last_register_ = 0;			// last register written
		uint32_t        noise_lfsr_ = 0;			// noise generator LFSR
		int32_t         stereo_mask_ = 0;			// the stereo output mask
		int32_t         channel_period_[4] = { 0 };	// Length of 1/2 of waveform
		int32_t         channel_volume_[4] = { 0 };	// db volume of voice 0-2 and noise
		int32_t         channel_count_[4] = { 0 };	// Position within the waveform
		int32_t         channel_output_[4] = { 0 };	// 1-bit output of each channel, pre-volume
	};

}
