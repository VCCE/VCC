/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

 //CC Flag masks
constexpr auto E = 7u;
constexpr auto F = 6u;
constexpr auto H = 5u;
constexpr auto I = 4u;
constexpr auto N = 3u;
constexpr auto Z = 2u;
constexpr auto V = 1u;
constexpr auto C = 0u;

// MC6809 Vector table
constexpr auto VSWI3	= 0xFFF2u;
constexpr auto VSWI2	= 0xFFF4u;
constexpr auto VFIRQ	= 0xFFF6u;
constexpr auto VIRQ		= 0xFFF8u;
constexpr auto VSWI		= 0xFFFAu;
constexpr auto VNMI		= 0xFFFCu;
constexpr auto VRESET	= 0xFFFEu;

//Opcode Defs
//Last Char (D) Direct (I) Inherent (R) Relative (M) Immediate (X) Indexed (E) extened
constexpr auto NEG_D	= 0x00u;
constexpr auto COM_D	= 0x03u;
constexpr auto LSR_D	= 0x04u;
constexpr auto ROR_D	= 0x06u;
constexpr auto ASR_D	= 0x07u;
constexpr auto ASL_D	= 0x08u;
constexpr auto ROL_D	= 0x09u;
constexpr auto DEC_D	= 0x0Au;
constexpr auto INC_D	= 0x0Cu;
constexpr auto TST_D	= 0x0Du;
constexpr auto JMP_D	= 0x0Eu;
constexpr auto CLR_D	= 0x0Fu;

constexpr auto Page2	= 0x10u;
constexpr auto Page3	= 0x11u;
constexpr auto NOP_I	= 0x12u;
constexpr auto SYNC_I	= 0x13u;
constexpr auto HALT		= 0x15u;	// Vcc Halt_15
constexpr auto LBRA_R	= 0x16u;
constexpr auto LBSR_R	= 0x17u;
constexpr auto DAA_I	= 0x19u;
constexpr auto ORCC_M	= 0x1Au;
constexpr auto ANDCC_M	= 0x1Cu;
constexpr auto SEX_I	= 0x1Du;
constexpr auto EXG_M	= 0x1Eu;		
constexpr auto TFR_M	= 0x1Fu;

constexpr auto BRA_R	= 0x20u;
constexpr auto BRN_R	= 0x21u;
constexpr auto BHI_R	= 0x22u;
constexpr auto BLS_R	= 0x23u;
constexpr auto BHS_R	= 0x24u;
constexpr auto BLO_R	= 0x25u;
constexpr auto BNE_R	= 0x26u;
constexpr auto BEQ_R	= 0x27u;
constexpr auto BVC_R	= 0x28u;
constexpr auto BVS_R	= 0x29u;
constexpr auto BPL_R	= 0x2Au;
constexpr auto BMI_R	= 0x2Bu;
constexpr auto BGE_R	= 0x2Cu;
constexpr auto BLT_R	= 0x2Du;
constexpr auto BGT_R	= 0x2Eu;
constexpr auto BLE_R	= 0x2Fu;

constexpr auto LEAX_X	= 0x30u;
constexpr auto LEAY_X	= 0x31u;
constexpr auto LEAS_X	= 0x32u;
constexpr auto LEAU_X	= 0x33u;
constexpr auto PSHS_M	= 0x34u;
constexpr auto PULS_M	= 0x35u;
constexpr auto PSHU_M	= 0x36u;
constexpr auto PULU_M	= 0x37u;
constexpr auto RTS_I	= 0x39u;
constexpr auto ABX_I	= 0x3Au;
constexpr auto RTI_I	= 0x3Bu;
constexpr auto CWAI_I	= 0x3Cu;
constexpr auto MUL_I	= 0x3Du;
constexpr auto RESET	= 0x3Eu;	//Undocumented instruction
constexpr auto SWI1_I	= 0x3Fu;

constexpr auto NEGA_I	= 0x40u;
constexpr auto COMA_I	= 0x43u;
constexpr auto LSRA_I	= 0x44u;
constexpr auto RORA_I	= 0x46u;
constexpr auto ASRA_I	= 0x47u;
constexpr auto ASLA_I	= 0x48u;
constexpr auto ROLA_I	= 0x49u;
constexpr auto DECA_I	= 0x4Au;
constexpr auto INCA_I	= 0x4Cu;
constexpr auto TSTA_I	= 0x4Du;
constexpr auto CLRA_I	= 0x4Fu;

constexpr auto NEGB_I	= 0x50u;	
constexpr auto COMB_I	= 0x53u;
constexpr auto LSRB_I	= 0x54u;
constexpr auto RORB_I	= 0x56u;
constexpr auto ASRB_I	= 0x57u;
constexpr auto ASLB_I	= 0x58u;
constexpr auto ROLB_I	= 0x59u;
constexpr auto DECB_I	= 0x5Au;
constexpr auto INCB_I	= 0x5Cu;
constexpr auto TSTB_I	= 0x5Du;
constexpr auto CLRB_I	= 0x5Fu;

constexpr auto NEG_X	= 0x60u;
constexpr auto COM_X	= 0x63u;
constexpr auto LSR_X	= 0x64u;
constexpr auto ROR_X	= 0x66u;
constexpr auto ASR_X	= 0x67u;
constexpr auto ASL_X	= 0x68u;
constexpr auto ROL_X	= 0x69u;
constexpr auto DEC_X	= 0x6Au;
constexpr auto INC_X	= 0x6Cu;
constexpr auto TST_X	= 0x6Du;
constexpr auto JMP_X	= 0x6Eu;
constexpr auto CLR_X	= 0x6Fu;

constexpr auto NEG_E	= 0x70u;
constexpr auto COM_E	= 0x73u;
constexpr auto LSR_E	= 0x74u;
constexpr auto ROR_E	= 0x76u;
constexpr auto ASR_E	= 0x77u;
constexpr auto ASL_E	= 0x78u;
constexpr auto ROL_E	= 0x79u;
constexpr auto DEC_E	= 0x7Au;
constexpr auto INC_E	= 0x7Cu;
constexpr auto TST_E	= 0x7Du;
constexpr auto JMP_E	= 0x7Eu;
constexpr auto CLR_E	= 0x7Fu;

constexpr auto SUBA_M	= 0x80u;
constexpr auto CMPA_M	= 0x81u;
constexpr auto SBCA_M	= 0x82u;
constexpr auto SUBD_M	= 0x83u;
constexpr auto ANDA_M	= 0x84u;
constexpr auto BITA_M	= 0x85u;
constexpr auto LDA_M	= 0x86u;
constexpr auto EORA_M	= 0x88u;
constexpr auto ADCA_M	= 0x89u;
constexpr auto ORA_M	= 0x8Au;
constexpr auto ADDA_M	= 0x8Bu;
constexpr auto CMPX_M	= 0x8Cu;
constexpr auto BSR_R	= 0x8Du;
constexpr auto LDX_M	= 0x8Eu;

constexpr auto SUBA_D	= 0x90u;
constexpr auto CMPA_D	= 0x91u;
constexpr auto SBCA_D	= 0x92u;
constexpr auto SUBD_D	= 0x93u;
constexpr auto ANDA_D	= 0x94u;
constexpr auto BITA_D	= 0x95u;
constexpr auto LDA_D	= 0x96u;
constexpr auto STA_D	= 0x97u;
constexpr auto EORA_D	= 0x98u;
constexpr auto ADCA_D	= 0x99u;
constexpr auto ORA_D	= 0x9Au;
constexpr auto ADDA_D	= 0x9Bu;
constexpr auto CMPX_D	= 0x9Cu;
constexpr auto BSR_D	= 0x9Du;
constexpr auto LDX_D	= 0x9Eu;
constexpr auto STX_D	= 0x9Fu;

constexpr auto SUBA_X	= 0xA0u;
constexpr auto CMPA_X	= 0xA1u;
constexpr auto SBCA_X	= 0xA2u;
constexpr auto SUBD_X	= 0xA3u;
constexpr auto ANDA_X	= 0xA4u;
constexpr auto BITA_X	= 0xA5u;
constexpr auto LDA_X	= 0xA6u;
constexpr auto STA_X	= 0xA7u;
constexpr auto EORA_X	= 0xA8u;
constexpr auto ADCA_X	= 0xA9u;
constexpr auto ORA_X	= 0xAAu;
constexpr auto ADDA_X	= 0xABu;
constexpr auto CMPX_X	= 0xACu;
constexpr auto BSR_X	= 0xADu;
constexpr auto LDX_X	= 0xAEu;
constexpr auto STX_X	= 0xAFu;

constexpr auto SUBA_E	= 0xB0u;
constexpr auto CMPA_E	= 0xB1u;
constexpr auto SBCA_E	= 0xB2u;
constexpr auto SUBD_E	= 0xB3u;
constexpr auto ANDA_E	= 0xB4u;
constexpr auto BITA_E	= 0xB5u;
constexpr auto LDA_E	= 0xB6u;
constexpr auto STA_E	= 0xB7u;
constexpr auto EORA_E	= 0xB8u;
constexpr auto ADCA_E	= 0xB9u;
constexpr auto ORA_E	= 0xBAu;
constexpr auto ADDA_E	= 0xBBu;
constexpr auto CMPX_E	= 0xBCu;
constexpr auto BSR_E	= 0xBDu;
constexpr auto LDX_E	= 0xBEu;
constexpr auto STX_E	= 0xBFu;

constexpr auto SUBB_M	= 0xC0u;
constexpr auto CMPB_M	= 0xC1u;
constexpr auto SBCB_M	= 0xC2u;
constexpr auto ADDD_M	= 0xC3u;
constexpr auto ANDB_M	= 0xC4u;
constexpr auto BITB_M	= 0xC5u;
constexpr auto LDB_M	= 0xC6u;
constexpr auto EORB_M	= 0xC8u;
constexpr auto ADCB_M	= 0xC9u;
constexpr auto ORB_M	= 0xCAu;
constexpr auto ADDB_M	= 0xCBu;
constexpr auto LDD_M	= 0xCCu;
constexpr auto LDU_M	= 0xCEu;

constexpr auto SUBB_D	= 0xD0u;
constexpr auto CMPB_D	= 0xD1u;
constexpr auto SBCB_D	= 0xD2u;
constexpr auto ADDD_D	= 0xD3u;
constexpr auto ANDB_D	= 0xD4u;
constexpr auto BITB_D	= 0xD5u;
constexpr auto LDB_D	= 0xD6u;
constexpr auto STB_D	= 0XD7u;
constexpr auto EORB_D	= 0xD8u;
constexpr auto ADCB_D	= 0xD9u;
constexpr auto ORB_D	= 0xDAu;
constexpr auto ADDB_D	= 0xDBu;
constexpr auto LDD_D	= 0xDCu;
constexpr auto STD_D	= 0xDDu;
constexpr auto LDU_D	= 0xDEu;
constexpr auto STU_D	= 0xDFu;

constexpr auto SUBB_X	= 0xE0u;
constexpr auto CMPB_X	= 0xE1u;
constexpr auto SBCB_X	= 0xE2u;
constexpr auto ADDD_X	= 0xE3u;
constexpr auto ANDB_X	= 0xE4u;
constexpr auto BITB_X	= 0xE5u;
constexpr auto LDB_X	= 0xE6u;
constexpr auto STB_X	= 0xE7u;
constexpr auto EORB_X	= 0xE8u;
constexpr auto ADCB_X	= 0xE9u;
constexpr auto ORB_X	= 0xEAu;
constexpr auto ADDB_X	= 0xEBu;
constexpr auto LDD_X	= 0xECu;
constexpr auto STD_X	= 0xEDu;
constexpr auto LDU_X	= 0xEEu;
constexpr auto STU_X	= 0xEFu;

constexpr auto SUBB_E	= 0xF0u;
constexpr auto CMPB_E	= 0xF1u;
constexpr auto SBCB_E	= 0xF2u;
constexpr auto ADDD_E	= 0xF3u;
constexpr auto ANDB_E	= 0xF4u;
constexpr auto BITB_E	= 0xF5u;
constexpr auto LDB_E	= 0xF6u;
constexpr auto STB_E	= 0xF7u;
constexpr auto EORB_E	= 0xF8u;
constexpr auto ADCB_E	= 0xF9u;
constexpr auto ORB_E	= 0xFAu;
constexpr auto ADDB_E	= 0xFBu;
constexpr auto LDD_E	= 0xFCu;
constexpr auto STD_E	= 0xFDu;
constexpr auto LDU_E	= 0xFEu;
constexpr auto STU_E	= 0xFFu;

//******************Page 2 Codes*****************************
constexpr auto LBRN_R	= 0x21u;
constexpr auto LBHI_R	= 0x22u;
constexpr auto LBLS_R	= 0x23u;
constexpr auto LBHS_R	= 0x24u;
constexpr auto LBCS_R	= 0x25u;
constexpr auto LBNE_R	= 0x26u;
constexpr auto LBEQ_R	= 0x27u;
constexpr auto LBVC_R	= 0x28u;
constexpr auto LBVS_R	= 0x29u;
constexpr auto LBPL_R	= 0x2Au;
constexpr auto LBMI_R	= 0x2Bu;
constexpr auto LBGE_R	= 0x2Cu;
constexpr auto LBLT_R	= 0x2Du;
constexpr auto LBGT_R	= 0x2Eu;
constexpr auto LBLE_R	= 0x2Fu;
constexpr auto SWI2_I	= 0x3Fu;
constexpr auto CMPD_M	= 0x83u;
constexpr auto CMPY_M	= 0x8Cu;
constexpr auto LDY_M	= 0x8Eu;
constexpr auto CMPD_D	= 0x93u;
constexpr auto CMPY_D	= 0x9Cu;
constexpr auto LDY_D	= 0x9Eu;
constexpr auto STY_D	= 0x9Fu;
constexpr auto CMPD_X	= 0xA3u;
constexpr auto CMPY_X	= 0xACu;
constexpr auto LDY_X	= 0xAEu;
constexpr auto STY_X	= 0xAFu;
constexpr auto CMPD_E	= 0xB3u;
constexpr auto CMPY_E	= 0xBCu;
constexpr auto LDY_E	= 0xBEu;
constexpr auto STY_E	= 0xBFu;
constexpr auto LDS_I	= 0xCEu;
constexpr auto LDS_D	= 0xDEu;
constexpr auto STS_D	= 0xDFu;
constexpr auto LDQ_X	= 0xECu;
constexpr auto STQ_X	= 0xEDu;
constexpr auto LDS_X	= 0xEEu;
constexpr auto STS_X	= 0xEFu;
constexpr auto LDS_E	= 0xFEu;
constexpr auto STS_E	= 0xFFu;

//*******************Page 3 Codes*********************************
constexpr auto BREAK	= 0x3Eu;	// Vcc Halt_113E
constexpr auto SWI3_I	= 0x3Fu;
constexpr auto CMPU_M	= 0x83u;
constexpr auto CMPS_M	= 0x8Cu;
constexpr auto CMPU_D	= 0x93u;
constexpr auto CMPS_D	= 0x9Cu;
constexpr auto CMPU_X	= 0xA3u;
constexpr auto CMPS_X	= 0xACu;
constexpr auto CMPU_E	= 0xB3u;
constexpr auto CMPS_E	= 0xBCu;
