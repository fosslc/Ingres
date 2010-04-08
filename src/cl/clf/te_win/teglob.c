# include   <compat.h>
# include   <te.h>
/*
** Copyright (c) 1985 Ingres Corporation
**
**      Name:
**              teglob.c
**
**
**      Description:
**
**              Initialize TE global variables.  These variables used to be
**              in the termdr.
**
**      Side Effects:
**              None
**
LIBRARY = IMPCOMPATLIBDATA
**
**      History:
**          10/83 - (mmm)
**              written
**          1-apr-91 (fraser)
**              Removed definition of UPCSTERM, as well as the definitions
**              conditional on ALLTERM.
**      12-Dec-95 (fanra01)
**          Changed bNewTEconsole from GLOBALREF to GLOBALDEF as not defined
**          anywhere.
**      11-Jun-96 (fanra01)
**          Added definition of hTEconsoleInput.
**
*/



/*
**  The following definitions are for PMFE special modifications.
*/

/*
** Last mode set by TErestore().
*/

GLOBALDEF	i2  TEmode = TE_NORMAL;

/*
** Flag to indicate whether TE is to actually write or not.
*/

GLOBALDEF	i2      TEok2write = OK;     /* != OK -> do not write non ^G chars to stdout */

/*
** Table to translate from Ingres Display Colors and Attributes
** to IBM/PC Colors and Attributes.
*/

GLOBALDEF	u_char    TExlate[256] = {  0x1F, 0x1E, 0x1D, 0x1C, 0x9F, 0x9E, 0x9D, 0x9C,
                                        0x71, 0x70, 0x30, 0x74, 0xF1, 0xF0, 0xB0, 0xF4 };

/*
** Number of rows and columns as defined in the termcap file.
*/

GLOBALDEF	i2      TErows = 25;
GLOBALDEF	i2      TEcols = 80;

/*
** Mouse Flag and Speeds.
*/

GLOBALDEF	i2      TEmouse = 1;            /* 1 -> Mouse Driver is Installed and Ready */
GLOBALDEF	i2      TEmenuSpeed = 13;       /* Mouse speed when on ring menus */
GLOBALDEF	i2      TEhorzSpeed = 20;       /* Horizontal Mouse spd when not on ring menu*/
GLOBALDEF	i2      TEvertSpeed = 35;       /* Vertical Mouse speed when not on ring menu*/
GLOBALDEF	i2      TEdbleSpeed = 72;       /* Threshold for double spd when not on ring */
GLOBALDEF	char    TE2buttons[] = "\033[P";/* Keystroke string for both bottons pressed */

/*
** Max WIdth & Height of Main Window
*/

GLOBALDEF	i2	TExScreen = 0;		/* Max Width of Main Window */
GLOBALDEF	i2	TEyScreen = 0;		/* Max Height of Main Window */

/*
** NT console buffer variables.
*/

GLOBALDEF	HANDLE						hTEconsole = 0;
GLOBALDEF       HANDLE                      hTEconsoleInput = 0;
GLOBALDEF	BOOL                        bNewTEconsole = TRUE;
GLOBALDEF	SECURITY_ATTRIBUTES			TEsa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
GLOBALDEF	CONSOLE_SCREEN_BUFFER_INFO	TEcsbi = {0};
GLOBALDEF	CONSOLE_CURSOR_INFO			TEcci = {10, TRUE};
GLOBALDEF	HANDLE				hTEprevconsole = 0;
GLOBALDEF	HANDLE				hTEprevconsoleInput =0;
