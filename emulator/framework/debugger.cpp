 // *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		debugger.c
//		Purpose:	Debugger Code (System Independent)
//		Created:	22nd February 2022
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <includes.h>
	
static int isInitialised = 0; 														// Flag to initialise first time
static int addressSettings[] = { 0,0,0,0 }; 										// Adjustable values : Code, Data, Other, Break.
static int keyMapping[16];															// Mapping for control keys to key values
static int inRunMode = 0;															// Non zero when free Running
static int lastKey,currentKey;														// Last and Current key state
static int stepBreakPoint;															// Extra breakpoint used for step over.
static Uint32 nextFrame = 0;														// Time of next frame.

// *******************************************************************************************************************************
//								Handle one frame of rendering etc. for the debugger.
// *******************************************************************************************************************************

void GFXXRender(SDL_Surface *surface,int autoStart,int scale) {

	if (isInitialised == 0) {														// Check if first time
		isInitialised = 1;															// Now initialised
		inRunMode = autoStart;														// Now running
		addressSettings[0] = DEBUG_HOMEPC();										// Set default locations
		addressSettings[1] = DEBUG_RAMSTART;
		addressSettings[3] = 0xFFFFFFFE;
		stepBreakPoint = 0xFFFFFFFE;

		//addressSettings[3] = 0xFFC10724;
	
		DBGDefineKey(DBGKEY_RESET,GFXKEY_F1);										// Assign default keys
		DBGDefineKey(DBGKEY_SHOW,GFXKEY_TAB);
		DBGDefineKey(DBGKEY_STEP,GFXKEY_F7);		
		DBGDefineKey(DBGKEY_STEPOVER,GFXKEY_F8);
		DBGDefineKey(DBGKEY_RUN,GFXKEY_F5);	
		DBGDefineKey(DBGKEY_BREAK,GFXKEY_F6);	
		DBGDefineKey(DBGKEY_HOME,GFXKEY_F2);		
		DBGDefineKey(DBGKEY_SETBREAK,GFXKEY_F9);		
		lastKey = currentKey = -1;
	}

	if (inRunMode != 0 || GFXIsKeyPressed(keyMapping[DBGKEY_SHOW]))					// Display system screen if Run or Sjhow
		DEBUG_VDURENDER(addressSettings,scale);
	else 																			// Otherwise show Debugger screen
		DEBUG_CPURENDER(addressSettings,scale);

	currentKey = -1;																// Identify which key is pressed.
	for (int i = 0;i < 128;i++) {
		if (!GFXISMODIFIERKEY(i)) {													// Don't return SHIFT and CTRL as keypresses.
			if (GFXIsKeyPressed(i)) {
				currentKey = i;
			}
		}
	}
	if (currentKey != lastKey) {													// Key changed
		lastKey = currentKey;														// Update current key.
		currentKey = DEBUG_KEYMAP(currentKey,inRunMode != 0);						// Pass keypress to called.
		if (currentKey >= 0) {														// Key depressed ?
			currentKey = toupper(currentKey);										// Make it capital.

			#define CMDKEY(n) GFXIsKeyPressed(keyMapping[n])						// Support Macro.

			if (CMDKEY(DBGKEY_RESET)) {												// Reset processor (F1)
				DEBUG_RESET();					
				addressSettings[0] = DEBUG_HOMEPC();
				GFXSetFrequency(0);
			}

			if (inRunMode == 0) {
				GFXSetFrequency(0);													// Will drive us mental otherwise.
				if (isxdigit(currentKey)) {											// Is it a hex digit 0-9 A-F.
					int digit = isdigit(currentKey)?currentKey:(currentKey-'A'+10);	// Convert to a number.
					int setting = 0;												// Which value is being changed ?
					if (GFXIsKeyPressed(GFXKEY_SHIFT)) setting = 1;
					if (GFXIsKeyPressed(GFXKEY_CONTROL)) setting = 2;
					addressSettings[setting] = 										// Shift it using this macro (so we could use Octal, say)
									DEBUG_SHIFT(addressSettings[setting],(digit & 0x0F));
				}

				if (CMDKEY(DBGKEY_HOME)) {											// Home pointer (F2)
					addressSettings[0] = DEBUG_HOMEPC();
				}
				if (CMDKEY(DBGKEY_RUN)) {											// Run program (F5)
					inRunMode = 1;											
					stepBreakPoint = -2;
				}
				if (CMDKEY(DBGKEY_STEP)) {											// Execute a single instruction (F7)
					DEBUG_SINGLESTEP();
					addressSettings[0] = DEBUG_HOMEPC();
				}
				if (CMDKEY(DBGKEY_STEPOVER)) {										// Step over program calls (F8)
					stepBreakPoint = DEBUG_GETOVERBREAK();							// Where if anywhere should we break ?
					if (stepBreakPoint == 0) {										// No step over, just single step.
						DEBUG_SINGLESTEP();
						addressSettings[0] = DEBUG_HOMEPC();
					} else {
						inRunMode = 1;												// Run until step break or normal break.
					}
				}
				if (CMDKEY(DBGKEY_SETBREAK)) {										// Set Breakpoint (F9)
						addressSettings[3] = addressSettings[0];
				}
			} else {																// In Run mode.
				if (CMDKEY(DBGKEY_BREAK)) {
					inRunMode = 0;
					addressSettings[0] = DEBUG_HOMEPC();
				}
			}
		} 
	}
	if (inRunMode != 0) {															// Running a program.
		int frameRate = DEBUG_RUN(addressSettings[3],stepBreakPoint);				// Run a frame, or try to.
		if (frameRate == 0) {														// Run code with step breakpoint, maybe.
			inRunMode = 0;															// Break has occurred.
		} else {
			/*
			int exceeded = SDL_GetTicks() - nextFrame;
			if (exceeded > 0)
				printf("Frame time exceeded by %d ms\n", exceeded);
				*/

			while (SDL_GetTicks() < nextFrame) { SDL_Delay(1); };									// Wait for frame timer to elapse.
			nextFrame = SDL_GetTicks() + 1000 / frameRate;							// And calculate the next sync time.

		}
		addressSettings[0] = DEBUG_HOMEPC();
	}	
}

// *******************************************************************************************************************************
//													Redefine a key
// *******************************************************************************************************************************

void DBGDefineKey(int keyID,int gfxKey) {
	keyMapping[keyID] = gfxKey;
}

// *******************************************************************************************************************************
//											Draw a vertical set of labels (helper)
// *******************************************************************************************************************************

void DBGVerticalLabel(int x,int y,const char *labels[],int fgr,int bgr) {
	int n = 0;
	while (labels[n] != NULL) {
		GFXString(GRID(x,y),labels[n],GRIDSIZE,fgr,bgr);
		y++;n++;
	}
}

// *******************************************************************************************************************************
//												  Get if running or not.
// *******************************************************************************************************************************

int DBGGetRunMode(void) {
	return inRunMode;
}

// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Changes made
//	
//		Date 			Changes
//		---- 			-------
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************
