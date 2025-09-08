#ifndef __HD6309DEFS_H__
#define __HD6309DEFS_H__
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

//MD Flag masks
constexpr auto NATIVE6309	= 0u;
constexpr auto FIRQMODE		= 1u;
constexpr auto ILLEGAL		= 6u;
constexpr auto ZERODIV		= 7u;

// MC6309 Vector table
constexpr auto VTRAP	= 0xFFF0u;
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
constexpr auto OIM_D	= 0x01u;		//6309
constexpr auto AIM_D	= 0x02u;		//6309
constexpr auto COM_D	= 0x03u;
constexpr auto LSR_D	= 0x04u;
constexpr auto EIM_D	= 0x05u;		//6309
constexpr auto ROR_D	= 0x06u;
constexpr auto ASR_D	= 0x07u;
constexpr auto ASL_D	= 0x08u;
constexpr auto ROL_D	= 0x09u;
constexpr auto DEC_D	= 0x0Au;
constexpr auto TIM_D	= 0x0Bu;		//6309
constexpr auto INC_D	= 0x0Cu;
constexpr auto TST_D	= 0x0Du;
constexpr auto JMP_D	= 0x0Eu;
constexpr auto CLR_D	= 0x0Fu;

constexpr auto Page2	= 0x10u;
constexpr auto Page3	= 0x11u;
constexpr auto NOP_I	= 0x12u;
constexpr auto SYNC_I	= 0x13u;
constexpr auto SEXW_I	= 0x14u;		//6309	
constexpr auto HALT		= 0x15u;		//Vcc Halt_0x15
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
constexpr auto OIM_X	= 0x61u;		//6309
constexpr auto AIM_X	= 0x62u;		//6309	
constexpr auto COM_X	= 0x63u;
constexpr auto LSR_X	= 0x64u;
constexpr auto EIM_X	= 0x65u;		//6309
constexpr auto ROR_X	= 0x66u;
constexpr auto ASR_X	= 0x67u;
constexpr auto ASL_X	= 0x68u;
constexpr auto ROL_X	= 0x69u;
constexpr auto DEC_X	= 0x6Au;
constexpr auto TIM_X	= 0x6Bu;		//6309
constexpr auto INC_X	= 0x6Cu;
constexpr auto TST_X	= 0x6Du;
constexpr auto JMP_X	= 0x6Eu;
constexpr auto CLR_X	= 0x6Fu;

constexpr auto NEG_E	= 0x70u;
constexpr auto OIM_E	= 0x71u;		//6309
constexpr auto AIM_E	= 0x72u;		//6309
constexpr auto COM_E	= 0x73u;
constexpr auto LSR_E	= 0x74u;
constexpr auto EIM_E	= 0x75u;		//6309
constexpr auto ROR_E	= 0x76u;
constexpr auto ASR_E	= 0x77u;
constexpr auto ASL_E	= 0x78u;
constexpr auto ROL_E	= 0x79u;
constexpr auto DEC_E	= 0x7Au;
constexpr auto TIM_E	= 0x7Bu;		//6309
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
constexpr auto LDQ_M	= 0xCDu;		//6309
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
constexpr auto ADDR		= 0x30u;	//6309
constexpr auto ADCR		= 0x31u;	//6309
constexpr auto SUBR		= 0x32u;	//6309
constexpr auto SBCR		= 0x33u;	//6309
constexpr auto ANDR		= 0x34u;	//6309
constexpr auto ORR		= 0x35u;	//6309
constexpr auto EORR		= 0x36u;	//6309
constexpr auto CMPR		= 0x37u;	//6309
constexpr auto PSHSW	= 0x38u;	//6309
constexpr auto PULSW	= 0x39u;	//6309
constexpr auto PSHUW	= 0x3Au;	//6309
constexpr auto PULUW	= 0x3Bu;	//6309
constexpr auto SWI2_I	= 0x3Fu;
constexpr auto NEGD_I	= 0x40u;	//6309
constexpr auto COMD_I	= 0x43u;	//6309
constexpr auto LSRD_I	= 0x44u;	//6309
constexpr auto RORD_I	= 0x46u;	//6309
constexpr auto ASRD_I	= 0x47u;	//6309
constexpr auto ASLD_I	= 0x48u;	//6309
constexpr auto ROLD_I	= 0x49u;	//6309
constexpr auto DECD_I	= 0x4Au;	//6309
constexpr auto INCD_I	= 0x4Cu;	//6309
constexpr auto TSTD_I	= 0x4Du;	//6309
constexpr auto CLRD_I	= 0x4Fu;	//6309
constexpr auto COMW_I	= 0x53u;	//6309
constexpr auto LSRW_I	= 0x54u;	//6309
constexpr auto RORW_I	= 0x56u;	//6309
constexpr auto ROLW_I	= 0x59u;	//6309
constexpr auto DECW_I	= 0x5Au;	//6309
constexpr auto INCW_I	= 0x5Cu;	//6309
constexpr auto TSTW_I	= 0x5Du;	//6309
constexpr auto CLRW_I	= 0x5Fu;	//6309
constexpr auto SUBW_M	= 0x80u;	//6309
constexpr auto CMPW_M	= 0x81u;	//6309
constexpr auto SBCD_M	= 0x82u;	//6309
constexpr auto CMPD_M	= 0x83u;
constexpr auto ANDD_M	= 0x84u;	//6309
constexpr auto BITD_M	= 0x85u;	//6309
constexpr auto LDW_M	= 0x86u;	//6309
constexpr auto EORD_M	= 0x88u;	//6309
constexpr auto ADCD_M	= 0x89u;	//6309
constexpr auto ORD_M	= 0x8Au;	//6309
constexpr auto ADDW_M	= 0x8Bu;	//6309
constexpr auto CMPY_M	= 0x8Cu;
constexpr auto LDY_M	= 0x8Eu;
constexpr auto SUBW_D	= 0x90u;	//6309
constexpr auto CMPW_D	= 0x91u;	//6309
constexpr auto SBCD_D	= 0x92u;	//6309
constexpr auto CMPD_D	= 0x93u;
constexpr auto ANDD_D	= 0x94u;	//6309
constexpr auto BITD_D	= 0x95u;	//6309
constexpr auto LDW_D	= 0x96u;	//6309
constexpr auto STW_D	= 0x97u;	//6309
constexpr auto EORD_D	= 0x98u;	//6309
constexpr auto ADCD_D	= 0x99u;	//6309
constexpr auto ORD_D	= 0x9Au;	//6309
constexpr auto ADDW_D	= 0x9Bu;	//6309
constexpr auto CMPY_D	= 0x9Cu;
constexpr auto LDY_D	= 0x9Eu;
constexpr auto STY_D	= 0x9Fu;
constexpr auto SUBW_X	= 0xA0u;	//6309
constexpr auto CMPW_X	= 0xA1u;	//6309
constexpr auto SBCD_X	= 0xA2u;	//6309
constexpr auto CMPD_X	= 0xA3u;
constexpr auto ANDD_X	= 0xA4u;	//6309
constexpr auto BITD_X	= 0xA5u;	//6309
constexpr auto LDW_X	= 0xA6u;	//6309
constexpr auto STW_X	= 0xA7u;	//6309
constexpr auto EORD_X	= 0xA8u;	//6309
constexpr auto ADCD_X	= 0xA9u;	//6309
constexpr auto ORD_X	= 0xAAu;	//6309
constexpr auto ADDW_X	= 0xABu;	//6309
constexpr auto CMPY_X	= 0xACu;
constexpr auto LDY_X	= 0xAEu;
constexpr auto STY_X	= 0xAFu;
constexpr auto SUBW_E	= 0xB0u;	//6309
constexpr auto CMPW_E	= 0xB1u;	//6309
constexpr auto SBCD_E	= 0xB2u;	//6309
constexpr auto CMPD_E	= 0xB3u;
constexpr auto ANDD_E	= 0xB4u;	//6309
constexpr auto BITD_E	= 0xB5u;	//6309
constexpr auto LDW_E	= 0xB6u;	//6309
constexpr auto STW_E	= 0xB7u;	//6309
constexpr auto EORD_E	= 0xB8u;	//6309
constexpr auto ADCD_E	= 0xB9u;	//6309
constexpr auto ORD_E	= 0xBAu;	//6309
constexpr auto ADDW_E	= 0xBBu;	//6309
constexpr auto CMPY_E	= 0xBCu;
constexpr auto LDY_E	= 0xBEu;
constexpr auto STY_E	= 0xBFu;
constexpr auto LDS_I	= 0xCEu;
constexpr auto LDQ_D	= 0xDCu;	//6309
constexpr auto STQ_D	= 0xDDu;	//6309
constexpr auto LDS_D	= 0xDEu;
constexpr auto STS_D	= 0xDFu;
constexpr auto LDQ_X	= 0xECu;
constexpr auto STQ_X	= 0xEDu;
constexpr auto LDS_X	= 0xEEu;
constexpr auto STS_X	= 0xEFu;
constexpr auto LDQ_E	= 0xFCu;	//6309 
constexpr auto STQ_E	= 0xFDu;	//6309 
constexpr auto LDS_E	= 0xFEu;
constexpr auto STS_E	= 0xFFu;

//*******************Page 3 Codes*********************************
constexpr auto BAND		= 0x30u;	//6309
constexpr auto BIAND	= 0x31u;	//6309
constexpr auto BOR		= 0x32u;	//6309
constexpr auto BIOR		= 0x33u;	//6309
constexpr auto BEOR		= 0x34u;	//6309
constexpr auto BIEOR	= 0x35u;	//6309
constexpr auto LDBT		= 0x36u;	//6309 
constexpr auto STBT		= 0x37u;	//6309
constexpr auto TFM1		= 0x38u;	//6309
constexpr auto TFM2		= 0x39u;	//6309
constexpr auto TFM3		= 0x3Au;	//6309
constexpr auto TFM4		= 0x3Bu;	//6309
constexpr auto BITMD_M	= 0x3Cu;	//6309
constexpr auto LDMD_M	= 0x3Du;	//6309
constexpr auto BREAK	= 0x3Eu;    //VCC Halt_0x113e
constexpr auto SWI3_I	= 0x3Fu;
constexpr auto COME_I	= 0x43u;	//6309
constexpr auto DECE_I	= 0x4Au;	//6309
constexpr auto INCE_I	= 0x4Cu;	//6309
constexpr auto TSTE_I	= 0x4Du;	//6309
constexpr auto CLRE_I	= 0x4Fu;	//6309
constexpr auto COMF_I	= 0x53u;	//6309
constexpr auto DECF_I	= 0x5Au;	//6309
constexpr auto INCF_I	= 0x5Cu;	//6309
constexpr auto TSTF_I	= 0x5Du;	//6309
constexpr auto CLRF_I	= 0x5Fu;	//6309
constexpr auto SUBE_M	= 0x80u;	//6309
constexpr auto CMPE_M	= 0x81u;	//6309
constexpr auto CMPU_M	= 0x83u;
constexpr auto LDE_M	= 0x86u;	//6309
constexpr auto ADDE_M	= 0x8Bu;	//6309
constexpr auto CMPS_M	= 0x8Cu;
constexpr auto DIVD_M	= 0x8Du;	//6309
constexpr auto DIVQ_M	= 0x8Eu;	//6309
constexpr auto MULD_M	= 0x8Fu;	//6309
constexpr auto SUBE_D	= 0x90u;	//6309
constexpr auto CMPE_D	= 0x91u;	//6309
constexpr auto CMPU_D	= 0x93u;
constexpr auto LDE_D	= 0x96u;	//6309
constexpr auto STE_D	= 0x97u;	//6309
constexpr auto ADDE_D	= 0x9Bu;	//6309
constexpr auto CMPS_D	= 0x9Cu;
constexpr auto DIVD_D	= 0x9Du;	//6309
constexpr auto DIVQ_D	= 0x9Eu;	//6309
constexpr auto MULD_D	= 0x9Fu;	//6309
constexpr auto SUBE_X	= 0xA0u;	//6309
constexpr auto CMPE_X	= 0xA1u;	//6309
constexpr auto CMPU_X	= 0xA3u;
constexpr auto LDE_X	= 0xA6u;	//6309
constexpr auto STE_X	= 0xA7u;	//6309
constexpr auto ADDE_X	= 0xABu;	//6309
constexpr auto CMPS_X	= 0xACu;
constexpr auto DIVD_X	= 0xADu;	//6309 
constexpr auto DIVQ_X	= 0xAEu;	//6309
constexpr auto MULD_X	= 0xAFu;	//6309
constexpr auto SUBE_E	= 0xB0u;	//6309
constexpr auto CMPE_E	= 0xB1u;	//6309
constexpr auto CMPU_E	= 0xB3u;
constexpr auto LDE_E	= 0xB6u;	//6309
constexpr auto STE_E	= 0xB7u;	//6309
constexpr auto ADDE_E	= 0xBBu;	//6309
constexpr auto CMPS_E	= 0xBCu;
constexpr auto DIVD_E	= 0xBDu;	//6309
constexpr auto DIVQ_E	= 0xBEu;	//6309
constexpr auto MULD_E	= 0xBFu;	//6309
constexpr auto SUBF_M	= 0xC0u;	//6309
constexpr auto CMPF_M	= 0xC1u;	//6309
constexpr auto LDF_M	= 0xC6u;	//6309
constexpr auto ADDF_M	= 0xCBu;	//6309
constexpr auto SUBF_D	= 0xD0u;	//6309
constexpr auto CMPF_D	= 0xD1u;	//6309
constexpr auto LDF_D	= 0xD6u;	//6309
constexpr auto STF_D	= 0xD7u;	//6309
constexpr auto ADDF_D	= 0xDBu;	//6309
constexpr auto SUBF_X	= 0xE0u;	//6309
constexpr auto CMPF_X	= 0xE1u;	//6309
constexpr auto LDF_X	= 0xE6u;	//6309
constexpr auto STF_X	= 0xE7u;	//6309
constexpr auto ADDF_X	= 0xEBu;	//6309
constexpr auto SUBF_E	= 0xF0u;	//6309
constexpr auto CMPF_E	= 0xF1u;	//6309
constexpr auto LDF_E	= 0xF6u;	//6309
constexpr auto STF_E	= 0xF7u;	//6309
constexpr auto ADDF_E	= 0xFBu;	//6309

#endif
