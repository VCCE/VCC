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

// MC6809 Vector table
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
#define COM_D	0x03
#define LSR_D	0x04
#define ROR_D	0x06
#define ASR_D	0x07
#define ASL_D	0x08
#define ROL_D	0x09
#define DEC_D	0x0A
#define INC_D	0x0C
#define TST_D	0x0D
#define JMP_D	0x0E
#define CLR_D	0x0F

#define Page2	0x10
#define	Page3	0x11
#define NOP_I	0x12
#define SYNC_I	0x13
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
#define COM_X	0x63
#define LSR_X	0x64
#define ROR_X	0x66
#define ASR_X	0x67
#define ASL_X	0x68
#define ROL_X	0x69
#define DEC_X	0x6A
#define INC_X	0x6C
#define TST_X	0x6D
#define	JMP_X	0x6E
#define CLR_X	0x6F

#define NEG_E	0x70
#define COM_E	0x73
#define LSR_E	0x74
#define ROR_E	0x76
#define ASR_E	0x77
#define ASL_E	0x78
#define ROL_E	0x79
#define DEC_E	0x7A
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
#define SWI2_I	0x3F
#define CMPD_M	0x83
#define CMPY_M	0x8C
#define LDY_M	0x8E
#define CMPD_D	0x93
#define CMPY_D	0x9C
#define LDY_D	0x9E
#define STY_D	0x9F
#define CMPD_X	0xA3
#define CMPY_X	0xAC
#define LDY_X	0xAE
#define STY_X	0xAF
#define CMPD_E	0xB3
#define CMPY_E	0xBC
#define LDY_E	0xBE
#define STY_E	0xBF
#define LDS_I	0xCE
#define LDS_D	0xDE
#define STS_D	0xDF
#define LDQ_X	0xEC
#define STQ_X	0xED
#define LDS_X	0xEE
#define STS_X	0xEF
#define LDS_E	0xFE
#define STS_E	0xFF

//*******************Page 3 Codes*********************************
#define SWI3_I	0x3F
#define CMPU_M	0x83
#define CMPS_M	0x8C
#define CMPU_D	0x93
#define CMPS_D	0x9C
#define CMPU_X	0xA3
#define CMPS_X	0xAC
#define CMPU_E	0xB3
#define CMPS_E	0xBC
