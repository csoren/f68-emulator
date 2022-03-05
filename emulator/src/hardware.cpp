// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		hardware.c
//		Purpose:	Hardware Emulation
//		Created:	22nd February 2022
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <includes.h>

// *******************************************************************************************************************************
//												Reset Hardware
// *******************************************************************************************************************************

void HWReset(void) {	
}

// *******************************************************************************************************************************
//												  Reset CPU
// *******************************************************************************************************************************

void HWSync(void) {
}

// *******************************************************************************************************************************
//											   Handle Keystrokes
// *******************************************************************************************************************************

#include <scan_lookup.h>

void  HWScanCodeHandler(int scancode,int keydown) {

	if (keydown == 0) return;
	
	if (scancode == SDL_SCANCODE_F1 || scancode == SDL_SCANCODE_F2 || scancode == SDL_SCANCODE_F3 || scancode == SDL_SCANCODE_F4 ||
		scancode == SDL_SCANCODE_F5 || scancode == SDL_SCANCODE_F6 || scancode == SDL_SCANCODE_F7 || scancode == SDL_SCANCODE_F8 ||
		scancode == SDL_SCANCODE_F9 || scancode == SDL_SCANCODE_F10) return;

	int n = 0;
	while (mau_table[n] != -1 && mau_table[n] != scancode) {
		n++;
	}
	if (mau_table[n] == scancode && mau_table[n] != 0 && mau_table[n] != 0x80) {		
		int mau = n | (keydown ? 0x00:0x80);
		//printf("Keyboard %x\n",mau);
		GAVIN_InsertMauFIFO(mau);
		GAVIN_FlagInterrupt(3,2); 								 						// Bit 0 of ICR 1 (Gavin SuperIO)		
		m68k_set_irq(IRQ_GAVIN_SUPERIO); 												// Interrupt level 4 (Gavin SuperIO)
	}
}
