/*
**  Define global variables
**
**  TERMDR.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	03/19/87 (dkh) - Added VIGRAPH capabilities.
**	08/14/87 (dkh) - ER changes.
**	15-sep-87 (bab)
**		Added def of XF termcap capability.
**	24-sep-87 (bab)
**		Added def of XL termcap capability.
**	01-oct-87 (bab)
**		Added def of IITDflu_firstlineused, for use
**		with _starty WINDOW struct member.
**	04/08/88 (dkh) - Added support for window sizing on startup.
**	07/25/88 (dkh) - Added XC to support color on Tektronix.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	09/27/89 (dkh) - Removed declaration of DB as no one uses it.
**	01/04/90 (dkh) - Added WX capability for X113 xterm.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	06-aug-92 (kirke)
**		Added OVERLAYSP.  Used when only 1 cell of a 2 cell
**		wide character is visible, the other cell having been
**		overlaid by some other character.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      03-May-2001 (fanra01)
**          Sir 104700
**          Add IIJS termcap flag for Java servlet wrapper.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

/* Jam hints
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<er.h>
# include	<termdr.h>

GLOBALDEF bool	_echoit		= TRUE, /* set if stty indicates ECHO	*/
	_rawmode	= FALSE,/* set if stty indicates RAW mode	*/
	My_term		= FALSE,/* set if user specifies terminal type	*/
	_endwin		= FALSE;/* set if endwin has been called	*/

GLOBALDEF char	ttytype[80] = {0},		/* long name of tty	*/
	*Def_term	= ERx("unknown");	/* default terminal type*/

GLOBALDEF i4	LINES		= 0,	/* number of lines allowed on screen*/
	COLS		= 0,	/* number of columns allowed on screen	*/
	YN		= 0,	/* narrow width of terminal */
	YO		= 0,	/* wide width of terminal */
	TDGC		= 0,	/* GC capabilitye for VIGRAPH */
	TDGR		= 0,	/* GR capabilitye for VIGRAPH */
	TDGA		= 0,	/* Ga capabilitye for VIGRAPH */
	TD_GC		= 0,	/* Gc capabilitye for VIGRAPH */
	TD_GR		= 0,	/* Gr capabilitye for VIGRAPH */
	TDGX		= 0,	/* Gx capabilitye for VIGRAPH */
	TDGY		= 0,	/* Gy capabilitye for VIGRAPH */
	KEYNUMS		= 0;	/* number of function keys	*/

GLOBALDEF WINDOW	*stdscr		= NULL,
	*curscr		= NULL,
	*stdmsg		= NULL,
	*tempwin	= NULL,
	*lastlnwin	= NULL;

/* indicate which line on the terminal is topmost to be used (indexed from 0) */
GLOBALDEF i4	IITDflu_firstlineused	= 0;

GLOBALDEF FILE	*outf = NULL;		/* debug output file */

GLOBALDEF bool	AM = 0, BS = 0, CA = 0, DA = 0, PF = 0, XD = 0,
	EO = FALSE, GT = FALSE, IN = FALSE, XF = FALSE,
	TDMI = FALSE, MS = 0, NC = 0, OS = FALSE, UL = FALSE, XN = FALSE,
	TDGH = FALSE, TDGI = FALSE, TD_GL = FALSE, TDGO = FALSE,
	TDGS = FALSE, TDGW = FALSE, XC = FALSE, IITDWX = FALSE,
	XS = FALSE, IIMW = FALSE,
	IITDWS = FALSE,		/* window view flag */
	IITDDT = FALSE,		/* decterm locator capability */
	IITDXT = FALSE,		/* mit xterm locator capability */
	KY = 0, NONL = 0, normtty = 0, _pfast = 0;

GLOBALDEF bool IIJS = FALSE;    /* add flag for Java servlet implementation */
GLOBALDEF bool IITDlsLocSupport = FALSE;	/* locator support flag */

GLOBALDEF char	*AL = ERx(""), *BC = ERx(""),	*BT = ERx(""),
	*CD = ERx(""),	*CE = ERx(""),	*CL = ERx(""),	*CM = ERx(""),
	*DC = ERx(""),	*DL = ERx(""),	*DM = ERx(""),	*DO = ERx(""),
	*ED = ERx(""),	*EI = ERx(""),	*HO = ERx(""),	*IC = ERx(""),
	*IM = ERx(""),	*IP = ERx(""),	*IS = ERx(""),	*LL = ERx(""),
	*MA = ERx(""),	*ND = ERx(""),	*SE = ERx(""),	*SF = ERx(""),
	*SO = ERx(""),	*MF = ERx(""),	*ES = ERx(""),
	*SR = ERx(""),	*TA = ERx(""),	*TE = ERx(""),	*TI = ERx(""),
	*UC = ERx(""),	*UE = ERx(""),	*UP = ERx(""),	*US = ERx(""),
	*LD = ERx(""),	*LE = ERx(""),	*LS = ERx(""),	*QA = ERx(""),
	*QB = ERx(""),	*QC = ERx(""),	*QD = ERx(""),	*QE = ERx(""),
	*QF = ERx(""),	*QG = ERx(""),	*QH = ERx(""),	*QI = ERx(""),
	*QJ = ERx(""),	*QK = ERx(""),	*BL = ERx(""),	*BE = ERx(""),
	*RV = ERx(""),	*RE = ERx(""),	*EA = ERx(""),	*VB = ERx(""),
	*TDGE = ERx(""),*TDGL = ERx(""),*TD_GE = ERx(""),*TDGP = ERx(""),
	*TDDC = ERx(""),*WG = ERx(""),  *WR = ERx(""),
	*ZA = ERx(""),
	*YP = ERx(""),	*YQ = ERx(""),	*YR = ERx(""),	*YS = ERx(""),
	*ZB = ERx(""),	*ZC = ERx(""),	*ZD = ERx(""),	*ZE = ERx(""),
	*ZF = ERx(""),	*ZG = ERx(""),	*ZH = ERx(""),	*ZI = ERx(""),
	*ZJ = ERx(""),	*ZK = ERx(""),	*BO = ERx(""),	*EB = ERx(""),
	*K0 = ERx(""),	*K1 = ERx(""),	*K2 = ERx(""),	*K3 = ERx(""),
	*K4 = ERx(""),	*K5 = ERx(""),	*K6 = ERx(""),	*K7 = ERx(""),
	*K8 = ERx(""),	*K9 = ERx(""),	*KAA = ERx(""),	*KAB = ERx(""),
	*KAC = ERx(""),	*KAD = ERx(""),	*KAE = ERx(""),	*KAF = ERx(""),
	*KAG = ERx(""),	*KAH = ERx(""),	*KS = ERx(""),	*KE = ERx(""),
	*KU = ERx(""),	*KD = ERx(""),	*KR = ERx(""),	*KL = ERx(""),
	*KH = ERx(""),	*CAS = ERx(""),	*YA = ERx(""),	*YB = ERx(""),
	*YC = ERx(""),	*YD = ERx(""),	*YE = ERx(""),	*YF = ERx(""),
	*YG = ERx(""),	*YH = ERx(""),	*KAI = ERx(""),	*KAJ = ERx(""),
	*KAK = ERx(""),	*KAL = ERx(""),	*KAM = ERx(""),	*KAN = ERx(""),
	*KAO = ERx(""),	*KAP = ERx(""),	*KAQ = ERx(""),	*KAR = ERx(""),
	*KAS = ERx(""),	*KAT = ERx(""),	*KAU = ERx(""),	*KAV = ERx(""),
	*KAW = ERx(""),	*KAX = ERx(""),	*KAY = ERx(""),	*KAZ = ERx(""),
	*KKA = ERx(""),	*KKB = ERx(""),	*KKC = ERx(""),	*KKD = ERx(""),
	*VE = ERx(""),	*VS = ERx(""),	PC  = '\0',	XL = '\0';

GLOBALDEF i4	TDoutcount = {0};
GLOBALDEF char	*TDobptr = {0};
GLOBALDEF char	*TDoutbuf = {0};
/*
** for Double Bytes characters
*/
GLOBALDEF   char    PSP = '-';	/* for a phantom space character */
GLOBALDEF   char    LSP = '+';	/* for a left edge character instead of
				** a 2nd byte of a Double Bytes character
				** (it uses only in REFRESH.C)
				*/
GLOBALDEF   char    RSP = '=';	/* for a right edge character instead of
				** a 1st byte of a Double Bytes character
				** (it uses only in REFRESH.C)
				*/
GLOBALDEF   char    OVERLAYSP = ' ';	/* Used when only 1 cell of a 2 cell
					** wide character is visible, the
					** other cell having been overlaid by
					** some other character.  e.g. popups
					*/
GLOBALDEF       bool    *sflags[] =
                {
                        &AM, &BS, &MS, &NC, &KY, &PF, &XD, &XF, &XN,
                        &TDGH, &TDGI, &TD_GL, &TDGO, &TDGS, &TDGW,
                        &IITDWS,        /* brett, windex termcap entry */
                        &IIMW,  /* termcap entry for MWS */
                        &XC, &IITDWX,
                        &XS,
                        &IITDDT,        /* DECterm termcap entry */
                        &IITDXT,         /* Xterm termcap entry */
                        &IIJS           /* Java servlet entry */

                };
GLOBALDEF char  *xPC = ERx("");
GLOBALDEF char  *xXL = ERx("");
GLOBALDEF char  **sstrs[] =
                {
                        &BC, &BT, &CE, &CL, &CM,
                        &HO, &IS,
                        &LL, &ND, &xPC, &SE, &SO, &SR, &TA,
                        &TE, &TI, &UC, &UE, &UP, &US, &VS, &VE, &xXL, &CAS,
                        &TDGE, &TDGL, &TD_GE, &TDGP, &TDDC,
                        &YP, &YQ, &YR, &YS,
                        &LD, &LE, &LS, &QA, &QB, &QC, &QD, &QE, &QF,
                        &QG, &QH, &QI, &QJ, &QK,
                        &BE, &BL, &RE, &RV, &EA, &ZA, &ZB, &ZC, &ZD,
                        &ZE, &ZF, &ZG, &ZH, &ZI, &ZJ, &ZK, &BO, &EB, &KS, &KE,
                        &YA, &YB, &YC, &YD, &YE, &YF, &YG, &YH, &MF, &ES, &WG,
                        &WR,
                        &KU, &KD, &KR, &KL,
                        &K0, &K1, &K2, &K3, &K4, &K5, &K6, &K7, &K8,
                        &K9, &KAA, &KAB, &KAC, &KAD, &KAE, &KAF, &KAG,
                        &KAH, &KAI, &KAJ, &KAK, &KAL, &KAM, &KAN, &KAO,
                        &KAP, &KAQ, &KAR, &KAS, &KAT, &KAU,
                        &KAV, &KAW, &KAX, &KAY, &KAZ, &KKA, &KKB, &KKC, &KKD
                };


# ifdef PMFE
/*
**  Dave - This routine from the PC group does not conform to coding stanadars.
*/
WINDOW *
get_curscr()
{
	return(curscr);
}
# endif /* PMFE */
