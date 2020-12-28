// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
#pragma once
#include <cstdint>


class SN76489Device
{
public:

	using stream_sample_t = unsigned short;

	SN76489Device();

	virtual void device_start();
	virtual void write(uint8_t data);
	virtual stream_sample_t sound_stream_update(stream_sample_t &lbuffer, stream_sample_t &rbuffer);


protected:



private:

	inline bool     in_noise_mode();
	void            countdown_cycles();


	const int32_t	m_feedback_mask;    // mask for feedback
	const int32_t	m_whitenoise_tap1;  // mask for white noise tap 1 (higher one, usually bit 14)
	const int32_t	m_whitenoise_tap2;  // mask for white noise tap 2 (lower one, usually bit 13)
	const bool		m_negate;           // output negate flag
	const bool		m_stereo;           // whether we're dealing with stereo or not
	const float		m_period_divider;   // period divider
	const bool		m_ncr_style_psg;    // flag to ignore writes to regs 1,3,5,6,7 with bit 7 low
	const bool		m_sega_style_psg;   // flag to make frequency zero acts as if it is one more than max (0x3ff+1) or if it acts like 0; the initial register is pointing to 0x3 instead of 0x0; the volume reg is preloaded with 0xF instead of 0x0

	int32_t         m_vol_table[16];    // volume table (for 4-bit to db conversion)

	bool            m_ready_state;
	int32_t         m_cycles_to_ready;  // number of cycles until the READY line goes active
	int32_t         m_register[8];      // registers
	int32_t         m_last_register;    // last register written
	uint32_t        m_RNG;              // noise generator LFSR
	int32_t         m_stereo_mask;      // the stereo output mask
	int32_t         m_period[4];        // Length of 1/2 of waveform
	int32_t         m_volume[4];        // db volume of voice 0-2 and noise
	int32_t         m_count[4];         // Position within the waveform
	int32_t         m_output[4];        // 1-bit output of each channel, pre-volume
};
