//------------------------------------------------------------------
// Copyright E J Jaquay 2022
//
// This file is part of VCC (Virtual Color Computer).
//
// VCC (Virtual Color Computer) is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.  You should have
// received a copy of the GNU General Public License along with VCC 
// (Virtual Color Computer). If not see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------
// Defines
//------------------------------------------------------------------
// Status Register
// b0 Par Rx parity error
// b1 Frm Rx framing error
// B2 Ovr Rx data over run
// b3 RxF Rx Data register full
// b4 TxE Tx Data register empty
// b5 DCD Data carrier (clear=active)
// b6 DSR Data set ready (clear=active)
// b7 IRQ Interrupt read status
constexpr auto StatPar  = 0x01u;
constexpr auto StatFrm  = 0x02u;
constexpr auto StatOvr  = 0x04u;
constexpr auto StatRxF  = 0x08u;
constexpr auto StatTxE  = 0x10u;
constexpr auto StatDCD  = 0x20u;
constexpr auto StatDSR  = 0x40u;
constexpr auto StatIRQ  = 0x80u;

// Command register
// b0   DTR Enable receive and interupts  (set=enabled)
// b1   RxI Receiver IRQ control by StatRxF (set=disabled)
// b2-3 TIRB Transmit IRQ control
//      00 trans disabled,01 enabled,10 disabled,11 break
// b4   Echo Set=Activated
// b5-7 Par  Parity control
//      xx0 none, 001 Odd, 011 Even, 101 mark, 111 space
constexpr auto CmdDTR		= 0x01u;
constexpr auto CmdRxI		= 0x02u;
constexpr auto CmdTIRB		= 0x0Cu;
constexpr auto TIRB_Off		= 0x00u;
constexpr auto TIRB_On		= 0x04u;
constexpr auto TIRB_RTS		= 0x08u;
constexpr auto TIRB_Brk		= 0x0Cu;

// Control Register
// b0-3 Baud rate
//		{ X,60,75,110,
//		 135,150,300,600,
//		 1200,1800,2400,3600,
//		 4800,7200,9600,19200 }
// b4   Rcv Clock source 0=X 1=intenal
// b5-6 Data len 00=8 01=7 10=6 11=5
// b7   Stop bits 0=1, 1-2
// Parity is index to [odd,even,mark,space]
// BaudRate is index to [19200,50,75,110,135,150,300,600,1200,
//                       1800,2400,3600,4800 7200,9600,19200]
