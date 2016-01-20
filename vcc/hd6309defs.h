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
#define E 7
#define F 6
#define H 5
#define I 4
#define N 3
#define Z 2
#define V 1
#define C 0

//MD Flag masks
#define NATIVE6309	0
#define FIRQMODE	1
#define ILLEGAL		6
#define ZERODIV		7

// MC6309 Vector table
#define VTRAP	0xFFF0
#define VSWI3	0xFFF2
#define VSWI2	0xFFF4
#define VFIRQ	0xFFF6
#define VIRQ	0xFFF8
#define VSWI	0xFFFA
#define VNMI	0xFFFC
#define VRESET	0xFFFE




//Opcode Defs
//Last Char (D) Direct (I) Inherent (R) Relative (M) Immediate (X) Indexed (E) extened
#define NEG_D	0x00
#define OIM_D	0x01		//6309
#define AIM_D	0x02		//6309
#define COM_D	0x03
#define LSR_D	0x04
#define EIM_D	0x05		//6309
#define ROR_D	0x06
#define ASR_D	0x07
#define ASL_D	0x08
#define ROL_D	0x09
#define DEC_D	0x0A
#define	TIM_D	0x0B		//6309
#define INC_D	0x0C
#define TST_D	0x0D
#define JMP_D	0x0E
#define CLR_D	0x0F

#define Page2	0x10
#define	Page3	0x11
#define NOP_I	0x12
#define SYNC_I	0x13
#define SEXW_I	0x14		//6309	
#define LBRA_R	0x16
#define LBSR_R	0x17
#define DAA_I	0x19
#define ORCC_M	0x1A
#define ANDCC_M	0x1C
#define SEX_I	0x1D
#define EXG_M	0x1E		
#define TFR_M	0x1F

#define BRA_R	0x20
#define BRN_R	0x21
#define BHI_R	0x22
#define BLS_R	0x23
#define BHS_R	0x24
#define BLO_R	0x25
#define BNE_R	0x26
#define BEQ_R	0x27
#define BVC_R	0x28
#define BVS_R	0x29
#define BPL_R	0x2A
#define BMI_R	0x2B
#define BGE_R	0x2C
#define BLT_R	0x2D
#define BGT_R	0x2E
#define BLE_R	0x2F

#define LEAX_X	0x30
#define LEAY_X	0x31
#define LEAS_X	0x32
#define LEAU_X	0x33
#define PSHS_M	0x34
#define PULS_M	0x35
#define PSHU_M	0x36
#define PULU_M	0x37
#define RTS_I	0x39
#define ABX_I	0x3A
#define RTI_I	0x3B
#define CWAI_I	0x3C
#define MUL_I	0x3D
#define RESET	0x3E	//Undocumented instruction
#define SWI1_I	0x3F

#define NEGA_I	0x40	
#define COMA_I	0x43
#define LSRA_I	0x44
#define RORA_I	0x46
#define ASRA_I	0x47
#define ASLA_I	0x48
#define ROLA_I	0x49
#define DECA_I	0x4A
#define INCA_I	0x4C
#define TSTA_I	0x4D
#define CLRA_I	0x4F

#define NEGB_I	0x50	
#define COMB_I	0x53
#define LSRB_I	0x54
#define RORB_I	0x56
#define ASRB_I	0x57
#define ASLB_I	0x58
#define ROLB_I	0x59
#define DECB_I	0x5A
#define INCB_I	0x5C
#define TSTB_I	0x5D
#define CLRB_I	0x5F

#define NEG_X	0x60
#define OIM_X	0x61		//6309
#define AIM_X	0x62		//6309	
#define COM_X	0x63
#define LSR_X	0x64
#define EIM_X	0x65		//6309
#define ROR_X	0x66
#define ASR_X	0x67
#define ASL_X	0x68
#define ROL_X	0x69
#define DEC_X	0x6A
#define TIM_X	0x6B		//6309
#define INC_X	0x6C
#define TST_X	0x6D
#define	JMP_X	0x6E
#define CLR_X	0x6F

#define NEG_E	0x70
#define OIM_E	0x71		//6309
#define AIM_E	0x72		//6309
#define COM_E	0x73
#define LSR_E	0x74
#define EIM_E	0x75		//6309
#define ROR_E	0x76
#define ASR_E	0x77
#define ASL_E	0x78
#define ROL_E	0x79
#define DEC_E	0x7A
#define TIM_E	0x7B		//6309
#define INC_E	0x7C
#define TST_E	0x7D
#define	JMP_E	0x7E
#define CLR_E	0x7F

#define SUBA_M	0x80
#define CMPA_M	0x81
#define SBCA_M	0x82
#define SUBD_M	0x83
#define ANDA_M	0x84
#define BITA_M	0x85
#define LDA_M	0x86
#define EORA_M	0x88
#define ADCA_M	0x89
#define ORA_M	0x8A
#define	ADDA_M	0x8B
#define CMPX_M	0x8C
#define BSR_R	0x8D
#define LDX_M	0x8E

#define SUBA_D	0x90
#define CMPA_D	0x91
#define SBCA_D	0x92
#define SUBD_D	0x93
#define ANDA_D	0x94
#define BITA_D	0x95
#define LDA_D	0x96
#define STA_D	0x97
#define EORA_D	0x98
#define ADCA_D	0x99
#define ORA_D	0x9A
#define	ADDA_D	0x9B
#define CMPX_D	0x9C
#define BSR_D	0x9D
#define LDX_D	0x9E
#define STX_D	0x9F

#define SUBA_X	0xA0
#define CMPA_X	0xA1
#define SBCA_X	0xA2
#define SUBD_X	0xA3
#define ANDA_X	0xA4
#define BITA_X	0xA5
#define LDA_X	0xA6
#define STA_X	0xA7
#define EORA_X	0xA8
#define ADCA_X	0xA9
#define ORA_X	0xAA
#define	ADDA_X	0xAB
#define CMPX_X	0xAC
#define BSR_X	0xAD
#define LDX_X	0xAE
#define STX_X	0xAF

#define SUBA_E	0xB0
#define CMPA_E	0xB1
#define SBCA_E	0xB2
#define SUBD_E	0xB3
#define ANDA_E	0xB4
#define BITA_E	0xB5
#define LDA_E	0xB6
#define STA_E	0xB7
#define EORA_E	0xB8
#define ADCA_E	0xB9
#define ORA_E	0xBA
#define	ADDA_E	0xBB
#define CMPX_E	0xBC
#define BSR_E	0xBD
#define LDX_E	0xBE
#define STX_E	0xBF

#define SUBB_M	0xC0
#define CMPB_M	0xC1
#define SBCB_M	0xC2
#define ADDD_M	0xC3
#define ANDB_M	0xC4
#define BITB_M	0xC5
#define LDB_M	0xC6
#define EORB_M	0xC8
#define ADCB_M	0xC9
#define ORB_M	0xCA
#define	ADDB_M	0xCB
#define LDD_M	0xCC
#define LDQ_M	0xCD		//6309
#define LDU_M	0xCE

#define SUBB_D	0xD0
#define CMPB_D	0xD1
#define SBCB_D	0xD2
#define ADDD_D	0xD3
#define ANDB_D	0xD4
#define BITB_D	0xD5
#define LDB_D	0xD6
#define STB_D	0XD7
#define EORB_D	0xD8
#define ADCB_D	0xD9
#define ORB_D	0xDA
#define	ADDB_D	0xDB
#define LDD_D	0xDC
#define STD_D	0xDD
#define LDU_D	0xDE
#define STU_D	0xDF

#define SUBB_X	0xE0
#define CMPB_X	0xE1
#define SBCB_X	0xE2
#define ADDD_X	0xE3
#define ANDB_X	0xE4
#define BITB_X	0xE5
#define LDB_X	0xE6
#define	STB_X	0xE7
#define EORB_X	0xE8
#define ADCB_X	0xE9
#define ORB_X	0xEA
#define	ADDB_X	0xEB
#define LDD_X	0xEC
#define STD_X	0xED
#define LDU_X	0xEE
#define STU_X	0xEF

#define SUBB_E	0xF0
#define CMPB_E	0xF1
#define SBCB_E	0xF2
#define ADDD_E	0xF3
#define ANDB_E	0xF4
#define BITB_E	0xF5
#define LDB_E	0xF6
#define	STB_E	0xF7
#define EORB_E	0xF8
#define ADCB_E	0xF9
#define ORB_E	0xFA
#define	ADDB_E	0xFB
#define LDD_E	0xFC
#define STD_E	0xFD
#define LDU_E	0xFE
#define STU_E	0xFF

//******************Page 2 Codes*****************************
#define LBRN_R	0x21
#define LBHI_R	0x22
#define LBLS_R	0x23
#define LBHS_R	0x24
#define LBCS_R	0x25
#define LBNE_R	0x26
#define LBEQ_R	0x27
#define LBVC_R	0x28
#define LBVS_R	0x29
#define LBPL_R	0x2A
#define LBMI_R	0x2B
#define LBGE_R	0x2C
#define LBLT_R	0x2D
#define LBGT_R	0x2E
#define LBLE_R	0x2F
#define ADDR	0x30	//6309
#define	ADCR	0x31	//6309
#define SUBR	0x32	//6309
#define	SBCR	0x33	//6309
#define ANDR	0x34	//6309
#define	ORR		0x35	//6309
#define EORR	0x36	//6309
#define CMPR	0x37	//6309
#define PSHSW	0x38	//6309
#define PULSW	0x39	//6309
#define PSHUW	0x3A	//6309
#define PULUW	0x3B	//6309
#define SWI2_I	0x3F
#define NEGD_I	0x40	//6309
#define COMD_I	0x43	//6309
#define LSRD_I	0x44	//6309
#define RORD_I	0x46	//6309
#define ASRD_I	0x47	//6309
#define ASLD_I	0x48	//6309
#define ROLD_I	0x49	//6309
#define DECD_I	0x4A	//6309
#define INCD_I	0x4C	//6309
#define TSTD_I	0x4D	//6309
#define CLRD_I	0x4F	//6309
#define COMW_I	0x53	//6309
#define LSRW_I	0x54	//6309
#define RORW_I	0x56	//6309
#define ROLW_I	0x59	//6309
#define DECW_I	0x5A	//6309
#define INCW_I	0x5C	//6309
#define TSTW_I	0x5D	//6309
#define CLRW_I	0x5F	//6309
#define SUBW_M	0x80	//6309
#define CMPW_M	0x81	//6309
#define SBCD_M	0x82	//6309
#define CMPD_M	0x83
#define ANDD_M	0x84	//6309
#define BITD_M	0x85	//6309
#define LDW_M	0x86	//6309
#define EORD_M	0x88	//6309
#define ADCD_M	0x89	//6309
#define ORD_M	0x8A	//6309
#define ADDW_M	0x8B	//6309
#define CMPY_M	0x8C
#define LDY_M	0x8E
#define SUBW_D	0x90	//6309
#define CMPW_D	0x91	//6309
#define SBCD_D	0x92	//6309
#define CMPD_D	0x93
#define ANDD_D	0x94	//6309
#define BITD_D	0x95	//6309
#define LDW_D	0x96	//6309
#define STW_D	0x97	//6309
#define EORD_D	0x98	//6309
#define ADCD_D	0x99	//6309
#define ORD_D	0x9A	//6309
#define ADDW_D	0x9B	//6309
#define CMPY_D	0x9C
#define LDY_D	0x9E
#define STY_D	0x9F
#define SUBW_X	0xA0	//6309
#define CMPW_X	0xA1	//6309
#define SBCD_X	0xA2	//6309
#define CMPD_X	0xA3
#define ANDD_X	0xA4	//6309
#define BITD_X	0xA5	//6309
#define LDW_X	0xA6	//6309
#define STW_X	0xA7	//6309
#define EORD_X	0xA8	//6309
#define ADCD_X	0xA9	//6309
#define ORD_X	0xAA	//6309
#define ADDW_X	0xAB	//6309
#define CMPY_X	0xAC
#define LDY_X	0xAE
#define STY_X	0xAF
#define SUBW_E	0xB0	//6309
#define CMPW_E	0xB1	//6309
#define SBCD_E	0xB2	//6309
#define CMPD_E	0xB3
#define ANDD_E	0xB4	//6309
#define BITD_E	0xB5	//6309
#define LDW_E	0xB6	//6309
#define STW_E	0xB7	//6309
#define EORD_E	0xB8	//6309
#define ADCD_E	0xB9	//6309
#define ORD_E	0xBA	//6309
#define ADDW_E	0xBB	//6309
#define CMPY_E	0xBC
#define LDY_E	0xBE
#define STY_E	0xBF
#define LDS_I	0xCE
#define LDQ_D	0xDC	//6309
#define STQ_D	0xDD	//6309
#define LDS_D	0xDE
#define STS_D	0xDF
#define LDQ_X	0xEC
#define STQ_X	0xED
#define LDS_X	0xEE
#define STS_X	0xEF
#define LDQ_E	0xFC	//6309 
#define STQ_E	0xFD	//6309 
#define LDS_E	0xFE
#define STS_E	0xFF

//*******************Page 3 Codes*********************************
#define BAND	0x30	//6309
#define BIAND	0x31	//6309
#define BOR		0x32	//6309
#define BIOR	0x33	//6309
#define BEOR	0x34	//6309
#define BIEOR	0x35	//6309
#define LDBT	0x36	//6309 
#define STBT	0x37	//6309
#define TFM1	0x38	//6309
#define TFM2	0x39	//6309
#define TFM3	0x3A	//6309
#define TFM4	0x3B	//6309
#define	BITMD_M	0x3C	//6309
#define LDMD_M	0x3D	//6309
#define SWI3_I	0x3F
#define COME_I	0x43	//6309
#define DECE_I	0x4A	//6309
#define INCE_I	0x4C	//6309
#define TSTE_I	0x4D	//6309
#define CLRE_I	0x4F	//6309
#define COMF_I	0x53	//6309
#define DECF_I	0x5A	//6309
#define INCF_I	0x5C	//6309
#define TSTF_I	0x5D	//6309
#define CLRF_I	0x5F	//6309
#define SUBE_M	0x80	//6309
#define CMPE_M	0x81	//6309
#define CMPU_M	0x83
#define LDE_M	0x86	//6309
#define ADDE_M	0x8B	//6309
#define CMPS_M	0x8C
#define DIVD_M	0x8D	//6309
#define DIVQ_M	0x8E	//6309
#define MULD_M	0x8F	//6309
#define SUBE_D	0x90	//6309
#define CMPE_D	0x91	//6309
#define CMPU_D	0x93
#define LDE_D	0x96	//6309
#define STE_D	0x97	//6309
#define ADDE_D	0x9B	//6309
#define CMPS_D	0x9C
#define DIVD_D	0x9D	//6309
#define DIVQ_D	0x9E	//6309
#define MULD_D	0x9F	//6309
#define SUBE_X	0xA0	//6309
#define CMPE_X	0xA1	//6309
#define CMPU_X	0xA3
#define LDE_X	0xA6	//6309
#define STE_X	0xA7	//6309
#define ADDE_X	0xAB	//6309
#define CMPS_X	0xAC
#define DIVD_X	0xAD	//6309 
#define DIVQ_X	0xAE	//6309
#define MULD_X	0xAF	//6309
#define SUBE_E	0xB0	//6309
#define CMPE_E	0xB1	//6309
#define CMPU_E	0xB3
#define LDE_E	0xB6	//6309
#define STE_E	0xB7	//6309
#define ADDE_E	0xBB	//6309
#define CMPS_E	0xBC
#define DIVD_E	0xBD	//6309
#define DIVQ_E	0xBE	//6309
#define MULD_E	0xBF	//6309
#define SUBF_M	0xC0	//6309
#define CMPF_M	0xC1	//6309
#define LDF_M	0xC6	//6309
#define ADDF_M	0xCB	//6309
#define SUBF_D	0xD0	//6309
#define CMPF_D	0xD1	//6309
#define LDF_D	0xD6	//6309
#define STF_D	0xD7	//6309
#define ADDF_D	0xDB	//6309
#define SUBF_X	0xE0	//6309
#define CMPF_X	0xE1	//6309
#define LDF_X	0xE6	//6309
#define STF_X	0xE7	//6309
#define ADDF_X	0xEB	//6309
#define SUBF_E	0xF0	//6309
#define CMPF_E	0xF1	//6309
#define LDF_E	0xF6	//6309
#define STF_E	0xF7	//6309
#define ADDF_E	0xFB	//6309

#endif
