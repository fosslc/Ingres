/*
**	Copyright (c) 2010 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	termdr.h -	Terminal Window Driver Module Definitions.
**
** Description:
**	This file contains definitions for the Terminal Window Driver Module.
**
** History:
**	5-13-83 (dkh) -- First written.
**	86/01/13  wong - Added screen size constants.
**	86/04/09  john - Mercury integration.
**	86/06/27  boba - Disable kfe for 3270 & VAX System V.
**	85/07/09  wong
**		Added masking definitions of 'BC', 'PC', 'UP' and 'ospeed' to
**		'II*'.  These are internal only.  (This gets around several
**		name conflicts with the MicroFocus Cobol interpreter.)
**	12/03/86(KY)  -- added _dx to WINDOW for Double Bytes attributes
**	03/04/87 (dkh) - Added support for ADTs.
**	03/19/97 (dkh) - Added VIGRAPH capabilities.
**	22-apr-87 (bab)
**		Added TDrmvright and TDrmvleft macros to support
**		movement in right-to-left fields.  Also changed the
**		existing TDmv... macros to support that change.
**	05/11/87 (dkh) - Deleted HZ as no one uses it.
**	08/25/87 (dkh) - Deleted include of "fkerr.h", it is no longer needed.
**	10-sep-87 (bab)
**		Zapped define (and use) of ERR.  Using FAIL now (for DG.)
**	15-sep-87 (bab)
**		Changed from extern to GLOBALREF.  Added the XF termcap
**		capability global.
**	24-sep-87 (bab)
**		Added the XL termcap capability global.
**	01-oct-87 (bab)
**		Added _starty and _startx members to the _win_st struct.
**		These members are used to hold the additional offsets from
**		the top and left side of the screen, respectively, of the
**		window, where that offset is due to reserving Y rows or
**		X columns of the terminal screen for use by other than the
**		forms system.  Initial use is for reserving top line of
**		screen for CEO status line under DG's AOS.  Also GLOBALREF
**		the IITDflu_firstlineused variable.
**	02-oct-87 (bab)
**		Parenthisize TDput macro; DG compiler bombs on the
**		= (assignment) operator in ?: statements unless in ()'s.
**	10/02/87 (dkh) - Changed TDmvcursor to TDmvcrsr.
**	04/08/88 (dkh) - Added WG and WR.
**	05/28/88 (dkh) - Integrated Tom Bishop's Venus code changes.
**	07/25/88 (dkh) - Added XC to support color on Tektronix.
**	09/26/89 (dkh) - Integrated changes to avoid name conflicts with
**			 curses on UNIX.  This is ifndef'ed VMS to reduce
**			 work for building on VMS.  This will hopefully
**			 be done more effectively in 8.0.
**	09/27/89 (dkh) - Eliminated DB->IIDB as it conflicted with lo.h
**	09/28/89 (dkh) - Added the _TDSPDRAW flag.  This allows a big
**			 form to just show up the first time (instead
**			 of scrolling into view) when the current field
**			 in the form is not normally visible.
**	10/19/89 (dkh) - Added a few more defines for Y* variables.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	05/01/90 (dkh) - Integrated PC changes.
**	08/22/90 (esd) - Ifdef'd out some stuff that doesn't apply
**			 to FT3270 (actually it was done earlier,
**			 but I don't know when or by whom).
**	10/02/90 (dkh) - Fixed bug 33605.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	09-jan-92 (leighb) DeskTop Porting Change: undef GT & LE
**	06-aug-92 (kirke)
**		Added OVERLAYSP.  When 1 cell of a 2 cell wide character is
**		overlaid, we substitute OVERLAYSP for the still visible cell.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**      03-Feb-96 (fanra01)
**          Added definitions for modules built for static linking and using
**          DLLs.
**	11-mar-96 (lawst01)
**	   Windows 3.1 port changes - added some function prototypes.
**	09-apr-1996 (chech02)
**	   Windows 3.1 port changes - add more function prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      03-May-2001 (fanra01)
**          Sir 104700
**          Add IIJS flag to list of GLOBALREFS.
**	28-Mar-2005 (lakvi01)
** 	    Corrected prototypes of some functions.
**	26-Aug-2009 (kschendel) b121804
**	    u_char prototypes to keep gcc 4.3 happy.
**	    Drop the old WIN16 section, misleading and out of date.  Better
**	    to just reconstruct as needed.
**      16-Feb-2009 (coomi01) b123294
**          Add prototyoe for TDsmvcur1()
**/

# undef		GT		 
# undef		LE		 

# define	AL	IIAL
# define	AM	IIAM
# define	BE	IIBE
# define	BL	IIBL
# define	BO	IIBO
# define	BS	IIBS
# define	BT	IIBT
# define	CA	IICA
# define	CAS	IICAS
# define	CD	IICD
# define	CE	IICE
# define	CL	IICL
# define	CM	IICM
# define	COLS	IICOLS
# define	DA	IIDA
# define	DC	IIDC
# define	DL	IIDL
# define	DM	IIDM
# define	DO	IIDO
# define	Def_term	IIDef_term
# define	EA	IIEA
# define	EB	IIEB
# define	ED	IIED
# define	EI	IIEI
# define	EO	IIEO
# define	ES	IIES
# define	GT	IIGT
# define	HO	IIHO
# define	IC	IIIC
# define	IM	IIIM
# define	IN	IIIN
# define	IP	IIIP
# define	IS	IIIS
# define	K0	IIK0
# define	K1	IIK1
# define	K2	IIK2
# define	K3	IIK3
# define	K4	IIK4
# define	K5	IIK5
# define	K6	IIK6
# define	K7	IIK7
# define	K8	IIK8
# define	K9	IIK9
# define	KAA	IIKAA
# define	KAB	IIKAB
# define	KAC	IIKAC
# define	KAD	IIKAD
# define	KAE	IIKAE
# define	KAF	IIKAF
# define	KAG	IIKAG
# define	KAH	IIKAH
# define	KAI	IIKAI
# define	KAJ	IIKAJ
# define	KAK	IIKAK
# define	KAL	IIKAL
# define	KAM	IIKAM
# define	KAN	IIKAN
# define	KAO	IIKAO
# define	KAP	IIKAP
# define	KAQ	IIKAQ
# define	KAR	IIKAR
# define	KAS	IIKAS
# define	KAT	IIKAT
# define	KAU	IIKAU
# define	KAV	IIKAV
# define	KAW	IIKAW
# define	KAX	IIKAX
# define	KAY	IIKAY
# define	KAZ	IIKAZ
# define	KD	IIKD
# define	KE	IIKE
# define	KEYNUMS	IIKEYNUMS
# define	KH	IIKH
# define	KKA	IIKKA
# define	KKB	IIKKB
# define	KKC	IIKKC
# define	KKD	IIKKD
# define	KL	IIKL
# define	KR	IIKR
# define	KS	IIKS
# define	KU	IIKU
# define	KY	IIKY
# define	LD	IILD
# define	LE	IILE
# define	LINES	IILINES
# define	LL	IILL
# define	LS	IILS
# define	LSP	IILSP
# define	M	IIM
# define	MA	IIMA
# define	MF	IIMF
# define	MS	IIMS
# define	My_term	IIMy_term
# define	NC	IINC
# define	ND	IIND
# define	NONL	IINONL
# define	OS	IIOS
# define	PF	IIPF
# define	PSP	IIPSP
# define	QA	IIQA
# define	QB	IIQB
# define	QC	IIQC
# define	QD	IIQD
# define	QE	IIQE
# define	QF	IIQF
# define	QG	IIQG
# define	QH	IIQH
# define	QI	IIQI
# define	QJ	IIQJ
# define	QK	IIQK
# define	RE	IIRE
# define	RSP	IIRSP
# define	RV	IIRV
# define	SE	IISE
# define	SF	IISF
# define	SO	IISO
# define	SR	IISR
# define	TA	IITA
# define	TDDC	IITDDC
# define	TDGA	IITDGA
# define	TDGC	IITDGC
# define	TDGE	IITDGE
# define	TDGH	IITDGH
# define	TDGI	IITDGI
# define	TDGL	IITDGL
# define	TDGO	IITDGO
# define	TDGP	IITDGP
# define	TDGR	IITDGR
# define	TDGS	IITDGS
# define	TDGW	IITDGW
# define	TDMI	IITDMI
# define	TD_GC	IITD_GC
# define	TD_GE	IITD_GE
# define	TD_GL	IITD_GL
# define	TD_GR	IITD_GR
# define	TDGX 	IITDGX
# define	TDGY 	IITDGY
# define	TE	IITE
# define	TI	IITI
# define	UC	IIUC
# define	UE	IIUE
# define	UL	IIUL
# define	US	IIUS
# define	VB	IIVB
# define	VE	IIVE
# define	VS	IIVS
# define	WG	IIWG
# define	WR	IIWR
# define	XC	IIXC
# define	XD	IIXD
# define	XF	IIXF
# define	XL	IIXL
# define	XN	IIXN
# define	YA	IIYA
# define	YB	IIYB
# define	YC	IIYC
# define	YD	IIYD
# define	YE	IIYE
# define	YF	IIYF
# define	YG	IIYG
# define	YH	IIYH
# define	ZA	IIZA
# define	ZB	IIZB
# define	ZC	IIZC
# define	ZD	IIZD
# define	ZE	IIZE
# define	ZF	IIZF
# define	ZG	IIZG
# define	ZH	IIZH
# define	ZI	IIZI
# define	ZJ	IIZJ
# define	ZK	IIZK
# define	_curwin	II_curwin
# define	_echoit	II_echoit
# define	_endwin	II_endwin
# define	_pfast	II_pfast
# define	_pwin	II_pwin
# define	_rawmode	II_rawmode
# define	_win	II_win
# define	curscr	IIcurscr
# define	depth	IIdepth
# define	inclrtop	IIinclrtop
# define	lastlnwin	IIlastlnwin
# define	lx	IIlx
# define	ly	IIly
# define	normtty	IInormtty
# define	pcolor	IIpcolor
# define	pda	IIpda
# define	pfonts	IIpfonts
# define	prev_sda	IIprev_sda
# define	outf	IIoutf
# define	stdmsg	IIstdmsg
# define	stdscr	IIstdscr
# define	tempwin	IItempwin
# define	ttytype	IIttytype
# define	vt100	IIvt100
# define	YN	IITDYN
# define	YO	IITDYO
# define	YP	IITDYP
# define	YQ	IITDYQ
# define	YR	IITDYR
# define	YS	IITDYS
# define	BC	IIBC
# define	PC	IIPC
# define	UP	IIUP
# define	ospeed	IIospeed


# define KFE

# ifdef FT3270
# undef KFE
# endif

# ifndef WINDOW

# ifndef reg
# define	reg	register
# endif

/*
** Screen Size Constants
*/

# define DEF_COLS	80		/* Default no. of columns */
# define DEF_LINES	24		/* Default no. of lines */
# define MIN_COLS	4		/* Minimum no. of columns */
# define MIN_LINES	5		/* Minimum no. of lines */

/*
** WINDOW._flags definition
*/
# define	_SUBWIN		01
# define	_ENDLINE	02
# define	_FULLWIN	04
# define	_SCROLLWIN	010
# define	_FLUSH		020
# define	_MOVE		040
# define	_BOXFIX		0100	/* flag: we must fixup boxchars */
# define	_STANDOUT	0200
# define	_NOCHANGE	-1
# define	_TDSPDRAW	0400	/* special handling for big forms */
# define	_DONTCTR	01000	/* Turn off centering in refresh */

/*
** defs for display attributes.
*/

# define	_CHGINT		01
# define	_UNLN		02
# define	_BLINK		04
# define	_RVVID		010
# define	_CIULBLRV	017
# define	_CI_UL		03
# define	_CI_BL		05
# define	_CI_RV		011
# define	_UL_BL		06
# define	_UL_RV		012
# define	_BL_RV		014
# define	_CI_UL_BL	07
# define	_UL_BL_RV	016
# define	_BL_RV_CI	015
# define	_RV_CI_UL	013

# ifdef	MSDOS
GLOBALREF	u_char	TDdefattr;
# endif

/*
** defs for line drawing graphics, etc.
*/

# define	_LINEDRAW	0200

/*
** defs for display 7 different colors
*/

# define	_COLOR1		020
# define	_COLOR2		040
# define	_COLOR3		060
# define	_COLOR4		0100
# define	_COLOR5		0120
# define	_COLOR6		0140
# define	_COLOR7		0160

/*
** Display Attribute Masks
*/

# define	_DAMASK		017
# define	_COLORMASK	0160
# define	_LINEMASK	0200

/*KJ
** defs for Double bytes attributes (_dx)
*/

# define	_PS		01  /* Phantom Space    */
# define	_DBL2ND		02  /* 2nd byte of a Double Bytes character*/

# define	_DBLMASK	03  /* a bit pattern for masking a Double bytes attribute
					and use this insted of "(_PS | _DBL2ND)".
				    */

# ifndef FT3270
/*
** defs to fake an enumeration type for display attributes.
*/

# define	NOATTR		-1
# define	BOLD		0
# define	UNLN		1
# define	BLINK		2
# define	REVERSE		3
# define	ALLATTR		4
# define	BO_UL		5
# define	BO_BL		6
# define	BO_RV		7
# define	UL_BL		8
# define	UL_RV		9
# define	BL_RV		10
# define	BO_UL_BL	11
# define	UL_BL_RV	12
# define	BL_RV_BO	13
# define	RV_BO_UL	14
# define	LINEDR		15
# endif /* FT3270 */


# ifdef	MSDOS
GLOBALREF	i4	sc_cury, sc_curx;
GLOBALREF	i4	sc_rely, sc_relx;
GLOBALREF	i4	sc_clear;
GLOBALREF	i4	sc_curpg;
GLOBALREF	i4	sc_oldpg;
GLOBALREF	i4	sc_color;
GLOBALREF	i4	sc_reqmode;
GLOBALREF	i4	sc_curmode;
GLOBALREF	i4	sc_oldmode;
GLOBALREF	i2	sc_image[25][80];
# endif	/* MSDOS */

/*
** Capabilities from termcap
**
** Masking definitions.
**
**	To avoid naming conflicts, esp. with the MicroFocus Cobol Interpreter.
*/


# ifndef FT3270
GLOBALREF bool  AM, BS, CA, DA, EO, GT, IN, TDMI, MS, NC, OS, UL, XF,
		XN, KY, PF, XD, TDGH, TDGI, TD_GL, TDGO, TDGS, TDGW, XC,
		XS, IIMW, IIJS;
GLOBALREF char  *AL, *BC, *BT, *CD, *CE, *CL, *CM, *DC, *DL, *DM, *DO,
		*ED, *EI, *HO, *IC, *IM, *IP, *IS, *LL, *MA, *ND, *SE,
		*SF, *SO, *SR, *TA, *TE, *TI, *UC, *UE, *UP, *US, *VB,
		*TDGE, *TDGL, *TD_GE, *TDGP, *TDDC,
		*LD, *LE, *LS, *QA, *QB, *QC, *QD, *QE, *QF, *QG, *QH,
		*QI, *QJ, *QK, *BL, *BE, *RV, *RE, *EA, *ZA, *ZB, *ZC,
		*ZD, *ZE, *ZF, *ZG, *ZH, *ZI, *ZJ, *ZK, *BO, *EB,
		*K0, *K1, *K2, *K3, *K4, *K5, *K6, *K7, *K8, *K9, *KAA,
		*KAB, *KAC, *KAD, *KAE, *KAF, *KAG, *KAH, *KS, *KE, *CAS,
		*KAI, *KAJ, *KAK, *KAL, *KAM, *KAN, *KAO, *KAP, *KAQ, *KAR,
		*KAS, *KAT, *KAU, *KAV, *KAW, *KAX, *KAY, *KAZ, *KKA, *KKB,
		*KKC, *KKD, *MF, *ES, *WG, *WR,
		*KU, *KD, *KR, *KL, *KH,
		*YA, *YB, *YC, *YD, *YE, *YF, *YG, *YH,
		*VE, *VS, PC, XL;
# endif /* FT3270 */

/*
** From the tty modes...
*/

GLOBALREF bool	NONL,  normtty, _pfast;
GLOBALREF bool	vt100;

struct _win_st
{
	i4	_cury, _curx;		/* screen cursor y,x coordinates */
	i4	_maxy, _maxx;		/* window lines and columns */
	i4	_begy, _begx;		/* beginning y,x coords of window */
	i4	_starty, _startx;	/* y,x coord of curscr on terminal */
	i4	_flags;			/* window attributes */
	bool	_clear;			/* clear window before redrawing */
	bool	lvcursor;		/* move cursor back after refresh */
	bool	_scroll;		/* can curscr scroll */
	bool	_relative;		/* are coords screen relative */
	char	**_y;			/* 2D array of chars in window */
	i4	*_firstch;		/* array of first change for each line*/
	i4	*_lastch;		/* array of last change for each line */
	struct _win_st	*_parent;	/* parent window if a sub-window */
	char	**_da;			/* 2D array of display attribute flags*/
	char	**_dx;			/* 2D array of Double Bytes attributes*/
	char	**_freeptr;		/* pointer for freeing alloc'ed memory*/
};

# define	WINDOW	struct _win_st

GLOBALREF bool	My_term, _echoit, _rawmode, _endwin;

GLOBALREF char	*Def_term, ttytype[];

GLOBALREF i4	LINES, COLS, KEYNUMS, TDGC, TDGR, TDGA, TD_GC, TD_GR,TDGX,TDGY;

GLOBALREF WINDOW	*stdscr, *stdmsg, *lastlnwin, *tempwin;

# ifndef PMFE_CURSCR
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF WINDOW	*curscr;
# else              /* NT_GENRIC && IMPORT_DLL_DATA */
GLOBALREF WINDOW	*curscr;
# endif             /* NT_GENRIC && IMPORT_DLL_DATA */
# else
FUNC_EXTERN	WINDOW	*get_curscr(void);
# define	curscr	get_curscr();
# endif /* PMFE_CURSCR */

GLOBALREF i4	IITDflu_firstlineused;

/*
**	Define VOID to stop lint from generating "null effect"
**	comments.
**
**  Ifdef'd out since our code does not use __void__ any where
**  (dkh) 7-25-84
*/



i4	_TDputchar();
# define	_puts(s)	TDtputs(s, (i4) 0, _TDputchar);


# define	TDCHHDLR	(i4) 0
# define	TDSTRHDLR	(i4) 1


# define	TD_TOPT		(i4) 0
# define	TD_BOTTOMT	(i4) 1
# define	TD_LEFTT	(i4) 2
# define	TD_RIGHTT	(i4) 3
# define	TD_XLINES	(i4) 4

# define	TD_FK_SET	(i4) 0
# define	TD_FK_UNSET	(i4) 1

# define	TDtopt(win, x, y)	TDboxhdlr(win, x, y, TD_TOPT)
# define	TDbottomt(win, x, y)	TDboxhdlr(win, x, y, TD_BOTTOMT)
# define	TDleftt(win, x, y)	TDboxhdlr(win, x, y, TD_LEFTT)
# define	TDrightt(win, x, y)	TDboxhdlr(win, x, y, TD_RIGHTT)
# define	TDxlines(win, x, y)	TDboxhdlr(win, x, y, TD_XLINES)

# define	TDBUFSIZE	1024

GLOBALREF	i4	TDoutcount;
GLOBALREF	char	*TDobptr;
GLOBALREF	char	*TDoutbuf;

#define	TDput(c)	(--TDoutcount > 0 ? (*TDobptr++ = c) : TDflush(c))

/*
** pseudo functions for standard screen
**
** macros for standard screen deleted since we don't use standard screen.
** (dkh) 1-4-84.
*/

/*
** pseudo mv functions for standard screen
**
** also eliminated since we don't use standard screen. (dkh) 1-4-85.
*/

/*
** psuedo functions
*/

# define	clearok(win,bf)	 (win->_clear = bf)
# define	leaveok(win,bf)	 (win->lvcursor = bf)
# define	scrollok(win,bf) (win->_scroll = bf)
# define	flushok(win,bf)	 (bf ? (win->_flags |= _FLUSH):(win->_flags &= ~_FLUSH))
# define	getyx(win,y,x)	y = win->_cury, x = win->_curx
# define	TDinch(win)	(win->_y[win->_cury][win->_curx])

/*
** function prototypes
*/
FUNC_EXTERN WINDOW	*TDnewwin(i4, i4, i4, i4);
FUNC_EXTERN WINDOW	*TDsubwin(WINDOW *, i4, i4, i4, i4, WINDOW *);
FUNC_EXTERN WINDOW	*TDextend(WINDOW *, i4);
FUNC_EXTERN char	*TDlongname(reg char *, reg char *);
FUNC_EXTERN char	*TDtgoto(char *, i4, i4);
FUNC_EXTERN VOID	TDoverlay(WINDOW *, i4, i4, WINDOW *, i4, i4);
FUNC_EXTERN VOID        TDsmvcur1 (i4, i4, i4, i4);

/*
**  Left and right brackets for comments in recovery files (azad)
**  Updated by dkh.
*/

# define	LEFT_COMMENT	'\021'
# define	RIGHT_COMMENT	'\023'

/*
** cousor movement functions for Double Bytes handling
*/
# define	TDmvUP		(i4) 1
# define	TDmvDOWN	(i4) 2
# define	TDmvLEFT	(i4) 3
# define	TDmvRIGHT	(i4) 4

# define	TDmvup(win, repts)	TDmvcrsr(TDmvUP, win, repts, FALSE)
# define	TDmvdown(win, repts)	TDmvcrsr(TDmvDOWN, win, repts, FALSE)
# define	TDmvleft(win, repts)	TDmvcrsr(TDmvLEFT, win, repts, FALSE)
# define	TDmvright(win, repts)	TDmvcrsr(TDmvRIGHT, win, repts, FALSE)
# define	TDrmvleft(win, repts)	TDmvcrsr(TDmvLEFT, win, repts, TRUE)
# define	TDrmvright(win, repts)	TDmvcrsr(TDmvRIGHT, win, repts, TRUE)

/*
** for Double Bytes characters. (see TERMDR.C for detail.)
*/
GLOBALREF	char	PSP, LSP, RSP, OVERLAYSP;

# endif	    /* WINDOW */


/* Function prototypes */

FUNC_EXTERN	u_char	TDsaveLast(char *);
FUNC_EXTERN	u_char	IITDwin_attr(struct _win_st *);
