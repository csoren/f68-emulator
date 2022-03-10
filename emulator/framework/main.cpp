// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		main.c
//		Purpose:	Main program
//		Created:	22nd February 2022
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <includes.h>
#include <generated/buildnumber.h>

int main(int argc,char *argv[]) {
	char title[64];
	sprintf(title,"%s (Build %d : %s)",WIN_TITLE,BUILD_COUNT,BUILD_TIME);
	DEBUG_RESET();
	int runNow = DEBUG_ARGUMENTS(argc,argv);
	GFXOpenWindow(title,WIN_WIDTH,WIN_HEIGHT,WIN_BACKCOLOUR);
	GFXStart(runNow);
	MEMEndRun();
	GFXCloseWindow();
	return(0);
}


