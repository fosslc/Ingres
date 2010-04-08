/*
** Header file for Terminal I/O Compatibility library for
** 4.2 Berkeley UNIX.
**
** Created 2-15-83 (dkh)
**      06-Feb-96 (fanra01)
**          Added IMPORT_DLL_DATA section for references by statically built
**          code to data in DLL.
**      11-Jun-96 (fanra01)
**          Added hTEconsoleInput.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**
*/

# include       <compat.h>              /* include compatiblity header file */
# include       <st.h>
# include       <si.h>                  /* to get stdio.h stuff */

/*
** Terminal characteristics you can set
*/

# define        TECRMOD         1

/*
** The following are defines for the delays that a terminal
** uses when running at a specific baud rate.
*/

# define        DE_ILLEGAL      -1
# define        DE_B50          2000
# define        DE_B75          1333
# define        DE_B110         909
# define        DE_B134         743
# define        DE_B150         666
# define        DE_B200         500
# define        DE_B300         333
# define        DE_B600         166
# define        DE_B1200        83
# define        DE_B1800        55
# define        DE_B2000        50
# define        DE_B2400        41
# define        DE_B3600        20
# define        DE_B4800        10
# define        DE_B9600        5
# define        DE_B19200       2

/*
** Prototype for local functions.
*/

bool    TEisa(FILE *);
VOID    TEposcur(i4 line, i4 col);
STATUS  TEsave(void);


/*
** if TEok2write != OK -> do not write non ^G chars to stdout.
*/

GLOBALREF  i2  TEok2write;

/*
** Update Entire Screen Flag.
*/

GLOBALREF  i2   TEwriteall;

/*
** defs for display attributes from termdr.h
*/

# define        _CHGINT         0x01    /* CHanGe INTensity (Bold)       */
# define        _UNLN           0x02    /* UNderLiNe                     */
# define        _BLINK          0x04    /* BLINKing                      */
# define        _RVVID          0x08    /* ReVerse VIDeo                 */

/*
** Last mode set by TErestore(), either TE_NORMAL or TE_FORMS
*/
GLOBALREF      i2  TEmode;

/*
** Ptr and Handle of Video Display Address
*/

GLOBALREF LOCALHANDLE   TEhDispAddr;              /* RBF */
GLOBALREF char *        TEpDispAddr;              /* RBF */

/*
** Video Color and Attribute Translate Table
*/

GLOBALREF       u_char    TExlate[];

/*
** Mouse Flag and Speeds.
*/

GLOBALREF       i2      TEmouse;        /* 1 -> Mouse Driver is Installed */
GLOBALREF       i2      TEmenuSpeed;    /* Mouse speed when on ring menus */
GLOBALREF       i2      TEhorzSpeed;    /* Horizontal spd when not @ring menu*/
GLOBALREF       i2      TEvertSpeed;    /* Vertical spd when not on ring menu*/
GLOBALREF       i2      TEdbleSpeed;    /* Threshold for dbl spd when ! @ring*/
GLOBALREF       char    TE2buttons[];   /* Keys str for both bottons pressed */

/*
** Max WIdth & Height of Main Window
*/

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF	i2	TExScreen;
GLOBALDLLREF	i2	TEyScreen;
# else
GLOBALREF	i2	TExScreen;	/* Max Width of Main Window */
GLOBALREF	i2	TEyScreen;	/* Max Height of Main Window */
# endif
/*
** NT console buffer varibales.
*/

GLOBALREF	HANDLE						hTEconsole;
GLOBALREF       HANDLE                          hTEconsoleInput;
GLOBALREF	BOOL						bNewTEconsole;
GLOBALREF	SECURITY_ATTRIBUTES			TEsa;
GLOBALREF	CONSOLE_SCREEN_BUFFER_INFO	TEcsbi;
GLOBALREF	CONSOLE_CURSOR_INFO			TEcci;
GLOBALREF	HANDLE					hTEprevconsole;
GLOBALREF	HANDLE					hTEprevconsoleInput;
