// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		gfx.c
//		Purpose:	Support library for SDL.
//		Created:	22nd February 2022
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <includes.h>

static SDL_Window *mainWindow = NULL;
static SDL_Surface *mainSurface = NULL;
static int background;
static int displaySelectCount = 0;

#define RED(x) (((x) >> 16) & 0xFF)
#define GREEN(x) (((x) >> 8) & 0xFF)
#define BLUE(x) ((x) & 0xFF)

static void _GFXInitialiseKeyRecord(void);
static void _GFXUpdateKeyRecord(int scancode,int isDown);

static Beeper beeper;

// *******************************************************************************************************************************
//
//								Open window of specified size, set title and background.
//
// *******************************************************************************************************************************

void GFXOpenWindow(const char *title,int width,int height,int colour) {
	if (SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)	{							// Try to initialise SDL Video and Audio
		exit(printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError()));
	}

	mainWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, 					// Try to create a window
							SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
	if (mainWindow == NULL) {
		exit(printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() ));
	}

	mainSurface = SDL_GetWindowSurface(mainWindow);									// Get a surface to draw on.

	background = colour;															// Remember required backgrounds.
	_GFXInitialiseKeyRecord();														// Set up key system.
}

// *******************************************************************************************************************************
//
//												Start the main rendering loop
//
// *******************************************************************************************************************************

static int isRunning = -1;																// Is app running

void GFXStart(int autoStart,int scale) {

	SDL_Event event;

	while(isRunning) {																// While still running.
		while (SDL_PollEvent(&event)) {												// While events in event queue.
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) 	// Exit if ESC pressed.
																			isRunning = 0;
			if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {				// Handle other keys.
				if (event.type == SDL_KEYDOWN) { 	
					if (event.key.keysym.sym == SDLK_F3) {							// F3 toggles display.
						displaySelectCount++;
					}
				}				
				HWScanCodeHandler(event.key.keysym.scancode,event.type == SDL_KEYDOWN);
				_GFXUpdateKeyRecord(event.key.keysym.sym,event.type == SDL_KEYDOWN);
			}
		}
		SDL_FillRect(mainSurface, NULL, 											// Draw the background.
							SDL_MapRGB(mainSurface->format, RED(background),GREEN(background),BLUE(background)));
		GFXXRender(mainSurface,autoStart,scale);											// Ask app to render state.
		SDL_UpdateWindowSurface(mainWindow);										// And update the main window.
	}
	SDL_CloseAudio();
}

// *******************************************************************************************************************************
//
//											Exit Program
//
// *******************************************************************************************************************************

void GFXExit(void) {
	isRunning = 0;
}

// *******************************************************************************************************************************
//
//												Force exit.
//
// *******************************************************************************************************************************

void GFXCloseOnDebug(void) {
	isRunning = 0;
}
// *******************************************************************************************************************************
//
//													Close the gfx system
//
// *******************************************************************************************************************************

void GFXCloseWindow(void) {
	SDL_DestroyWindow(mainWindow);													// Destroy working window
	SDL_Quit();																		// Exit SDL.
}

// *******************************************************************************************************************************
//
//											Support Routine - Draw solid rectangle
//
// *******************************************************************************************************************************

void GFXRectangle(SDL_Rect *rc,int colour) {
	SDL_FillRect(mainSurface,rc,SDL_MapRGB(mainSurface->format,RED(colour),GREEN(colour),BLUE(colour)));
}

// *******************************************************************************************************************************
//
//									Support Routine - Draw 5 x 7 bitmap font character
//
// *******************************************************************************************************************************

#include "font.h"

void GFXCharacter(int xc,int yc,int character,int size,int colour,int back) {

	Uint32 col = SDL_MapRGB(mainSurface->format,RED(colour),GREEN(colour),BLUE(colour));				
	SDL_Rect rc;
	if (back >= 0) {
		Uint32 col2 = SDL_MapRGB(mainSurface->format,RED(back),GREEN(back),BLUE(back));				
		rc.x = xc-size/2;rc.y = yc-size/2;rc.w = 6 * size;rc.h = 8*size;
		SDL_FillRect(mainSurface,&rc,col2);
	}
	if (character < 32 || character >= 128) character = '?';						// Unknown character
	character = character - 32;														// First font item is $20 (Space)
	rc.w = rc.h = size;																// Width and Height of pixel.
	for (int x = 0;x < 5;x++) {														// 5 Across
		rc.x = xc + x * size;
		for (int y = 0;y < 7;y++) {													// 7 Down
			rc.y = yc + y * size;
			if (fontdata[character*5+x] & (0x01 << y)) {							// Is bit set ? (note inversion)
				SDL_FillRect(mainSurface,&rc,col);									// If so, draw the pixel
			}
		}
	}
}

// *******************************************************************************************************************************
//
//													Redefine a character
//
// *******************************************************************************************************************************

void GFXDefineCharacter(int nChar,int b1,int b2,int b3,int b4,int b5) {
	if (nChar >= 32 && nChar < 128) {
		nChar = (nChar-32) * 5;
		fontdata[nChar++] = b1;
		fontdata[nChar++] = b2;
		fontdata[nChar++] = b3;
		fontdata[nChar++] = b4;
		fontdata[nChar++] = b5;
	}
}

// *******************************************************************************************************************************
//
//									Support Routine - Draw 5 x 7 bitmap font string
//
// *******************************************************************************************************************************

void GFXString(int xc,int yc,const char *text,int size,int colour,int back) {
	while (*text != '\0') {															// While more text
		GFXCharacter(xc,yc,*text,size,colour,back);									// Draw text
		text++;																		// Next character
		xc = xc + 6 * size;															// Advance horizontally.
	}
}

// *******************************************************************************************************************************
//
//												Display Number in base
//
// *******************************************************************************************************************************

void GFXNumber(int xc,int yc,unsigned int number,int base,int width,int size,int colour,int back) {
	if (width > 0) {
		GFXNumber(xc,yc,number/base,base,width-1,size,colour,back);
		GFXCharacter(xc + size*6*(width-1),yc,"0123456789ABCDEF"[number % base],size,colour,back);
	}
}

// *******************************************************************************************************************************
//
//													Set Grid Size.
//	
// *******************************************************************************************************************************

static SDL_Rect gridInfo;

void GFXSetCharacterSize(int xSize,int ySize) {
	gridInfo.w = mainSurface->w / xSize / 6;
	gridInfo.h = mainSurface->h / ySize / 8;
	gridInfo.w = (gridInfo.w < gridInfo.h) ? gridInfo.w : gridInfo.h;
	gridInfo.h = gridInfo.w;
	gridInfo.x = mainSurface->w / 2 - gridInfo.w * xSize * 6 / 2;
	gridInfo.y = mainSurface->h / 2 - gridInfo.h * ySize * 8 / 2;
}

int _GFXX(int x) { return gridInfo.x + gridInfo.w * 6 * x; }
int _GFXY(int y) { return gridInfo.y + gridInfo.h * 8 * y; }
int _GFXS(void)  { return gridInfo.w; }

// *******************************************************************************************************************************
//
//				This table is a mapping of GFXKEY_ values (0-127) to SDLK_Values (can be anything.)
//
// *******************************************************************************************************************************

static int keyTable[] = {
	GFXKEY_UP,SDLK_UP, GFXKEY_DOWN,SDLK_DOWN, GFXKEY_LEFT,SDLK_LEFT, GFXKEY_RIGHT,SDLK_RIGHT,
	GFXKEY_RETURN,SDLK_RETURN,GFXKEY_BACKSPACE,SDLK_BACKSPACE,GFXKEY_TAB,SDLK_TAB,GFXKEY_DELETE,SDLK_DELETE,
	GFXKEY_LSHIFT,SDLK_LSHIFT,GFXKEY_RSHIFT,SDLK_RSHIFT,GFXKEY_SHIFT,-1,GFXKEY_CONTROL,SDLK_LCTRL,	

	GFXKEY_F1,SDLK_F1, GFXKEY_F2,SDLK_F2, GFXKEY_F3,SDLK_F3, GFXKEY_F4,SDLK_F4, GFXKEY_F5,SDLK_F5, 
	GFXKEY_F6,SDLK_F6, GFXKEY_F7,SDLK_F7, GFXKEY_F8,SDLK_F8, GFXKEY_F9,SDLK_F9, 
	GFXKEY_F10,SDLK_F10, GFXKEY_F11,SDLK_F11, GFXKEY_F12,SDLK_F12,GFXKEY_INSERT,SDLK_INSERT,

	'A',SDLK_a, 'B',SDLK_b, 'C',SDLK_c, 'D',SDLK_d, 'E',SDLK_e, 'F',SDLK_f, 'G',SDLK_g, 'H',SDLK_h, 'I',SDLK_i,
	'J',SDLK_j, 'K',SDLK_k, 'L',SDLK_l, 'M',SDLK_m, 'N',SDLK_n, 'O',SDLK_o, 'P',SDLK_p, 'Q',SDLK_q, 'R',SDLK_r,
	'S',SDLK_s, 'T',SDLK_t, 'U',SDLK_u, 'V',SDLK_v, 'W',SDLK_w, 'X',SDLK_x, 'Y',SDLK_y, 'Z',SDLK_z,

	'0',SDLK_0, '1',SDLK_1, '2',SDLK_2, '3',SDLK_3, '4',SDLK_4, '5',SDLK_5, '6',SDLK_6, '7',SDLK_7, '8',SDLK_8, '9',SDLK_9,

	'-',SDLK_MINUS,'\\',SDLK_BACKSLASH,'@',SDLK_QUOTE,'[',SDLK_LEFTBRACKET,']',SDLK_RIGHTBRACKET,';',SDLK_SEMICOLON,':',SDLK_COLON,
	'.',SDLK_PERIOD,',',SDLK_COMMA,'/',SDLK_SLASH,'#',SDLK_HASH,'=',SDLK_EQUALS,' ',SDLK_SPACE,

-1 };

// *******************************************************************************************************************************
//
//													Handles Key State
//
// *******************************************************************************************************************************

struct _KeyRecord {
	int 	sdlKey;																	// Key representation in SDL.
	int 	gfxKey;																	// Key representation in GFX. (same as array index)
	int 	isPressed;																// Non zero if is pressed.
};

static struct _KeyRecord keyState[128];												// Array of key state records.

static void _GFXInitialiseKeyRecord(void) {
	for (int i = 0;i < 128;i++) {													// Erase the whole structure.
		keyState[i].sdlKey = keyState[i].gfxKey = keyState[i].isPressed = 0;
	}
	int n = 0;
	while (keyTable[n] != -1) {														// Scan the list of known keys.
		keyState[keyTable[n]].gfxKey = keyTable[n];									// Save gfx number of the key.
		keyState[keyTable[n]].sdlKey = keyTable[n+1];								// Save the corresponding SDL key.
		n = n + 2;
	}
}

static void _GFXUpdateKeyRecord(int scancode,int isDown) {
	for (int i = 0;i < 128;i++)														// Find key with corresponding scan code
		if (keyState[i].sdlKey == scancode)
			keyState[i].isPressed = (isDown != 0);									// Copy state into it.
	keyState[GFXKEY_SHIFT].isPressed = 												// Either shift key operates SHIFT.
					keyState[GFXKEY_LSHIFT].isPressed || keyState[GFXKEY_RSHIFT].isPressed;
}

// *******************************************************************************************************************************
//
//											Check to see if key is pressed.
//
// *******************************************************************************************************************************

int  GFXIsKeyPressed(int character) {
	if (character >= 'a' && character <= 'z') character = character - 'a' + 'A';	// Make lower case upper case
	return keyState[character].isPressed;							
}

// *******************************************************************************************************************************
//
//												Convert character to ASCII
//
//	UK Keyboard layout, will probably behave bizarrely elsewhere.
// *******************************************************************************************************************************

int  GFXToASCII(int ch,int applyModifiers) {
	if (ch >= ' ' && ch < 127) {													// Legitimate key.
		ch = tolower(ch);
		if (ch == '@') ch = '\'';													// @ is actually '
		if (applyModifiers != 0) {
			if (GFXIsKeyPressed(GFXKEY_SHIFT)) {
				switch(ch) {
					case '\'':	ch = '@';break;
					case '-':	ch = '_';break;
					case '#':	ch = '~';break;
					case '=':	ch = '+';break;
					case ';':	ch = ':';break;
					case '6':	ch = '^';break;
					case '7':	ch = '&';break;
					case '8':	ch = '*';break;
					case '9':	ch = '(';break;
					case '0':	ch = ')';break;
					default:	ch = ch ^ ((ch < 64) ? 0x10:0x20);break;
				}
			}
			if (GFXIsKeyPressed(GFXKEY_CONTROL)) ch = ch & 31;						// Handle control
		}

	} else {
		switch (ch) {																// Control characters
			case GFXKEY_TAB:		ch = 0x09;break;								// Handle TAB, Backspace and CR.
			case GFXKEY_RETURN:		ch = 0x0D;break;
			case GFXKEY_BACKSPACE:	ch = 0x08;break;
			default:				ch = 0x00;break;
		}
	}
	return ch;
}

// *******************************************************************************************************************************
//
//												Get elapsed time in ms.
//
// *******************************************************************************************************************************

int  GFXTimer(void) {
	return SDL_GetTicks();
}

// *******************************************************************************************************************************
//
//													Audio 
//
// *******************************************************************************************************************************

const int AMPLITUDE = 28000;
const int FREQUENCY = 44100;

void audio_callback(void*, Uint8*, int);

Beeper::Beeper()
{
    SDL_AudioSpec desiredSpec;

    desiredSpec.freq = FREQUENCY;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = 2048;
    desiredSpec.callback = audio_callback;
    desiredSpec.userdata = this;

    SDL_AudioSpec obtainedSpec;
    SDL_OpenAudio(&desiredSpec, &obtainedSpec);
    SDL_PauseAudio(0);
    setFrequency(0);
}

Beeper::~Beeper()
{
    SDL_CloseAudio();
}

void Beeper::generateSamples(Sint16 *stream, int length)
{
    int i = 0;
    while (i < length) {

        if (freq == 0) {
            while (i < length) {
                stream[i] = 0;
                i++;
            }
            return;
       	}
       	int samplesToDo = length;
        while (i < samplesToDo) {
            stream[i] = AMPLITUDE * ((((int)v*2/FREQUENCY) % 2) ? -1 : 1);
            i++;
            v += freq;
        }
    }
}

void Beeper::setFrequency(double f) {
	freq = f;
}

void audio_callback(void *_beeper, Uint8 *_stream, int _length)
{
    Sint16 *stream = (Sint16*) _stream;
    int length = _length / 2;
    Beeper* beeper = (Beeper*) _beeper;
    beeper->generateSamples(stream, length);
}

void GFXSetFrequency(int freq) {
	beeper.setFrequency(freq);
}

// *******************************************************************************************************************************
//											Get Debug Display Counter
// *******************************************************************************************************************************

int GFXGetDisplayToggle(void) {
	return displaySelectCount;
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
