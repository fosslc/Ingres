/*
** MENUS.H
**	Menu declaration header file for VIFRED.
**
**	Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - Eliminated old ifdefs.
**	01-sep-89 (cmr)	added define for LAYOUT and COLOPTS
**	5/22/90 (martym) Changed "rbfedFldMu" to "rbfedFldMu1" and added 
**			 "rbfedFldMu2". Also added "ediBREAK".
**	16-jul-90 (bruceb)
**		Added extern of new VIFRED menu (for when linked with ABF).
**	19-sep-90 (sylviap)
**		Took out reference to muHeading - no longer used.
**	06/27/92 (dkh) - Added support for naming a component in the
**			 detail section of rbf layout.
*/

# include  <menu.h> 

# ifndef NULLRET
# define	NULLRET	1
# define	INPUT	2
# endif /* NULLRET */

# define	HELP	1
# define	BLANK	2
# define	vfmuCAT	3
# define	EDIT	4
# define	CREATE	5
# define	QUIT	6

# define	wrCONT	7
# define	wrSTART	8
# define	wrWRITE	9


# define	DELETE	10
# define	EDIT2	11
# define	FIELDE	12
# define	OPEN	13
# define	SELECT	14
# define	TRIME	15
# define	CREATETF	35
# define	COLOPTS	38
# define 	LAYOUT	39
# define	ORDER	40
# define	PROFILE 41
# define	EXIT	42
# define	UNDO	16
# define	MOVE	04
# define	NAMING	47	/* base table column name tag */
# define	GRPMOVE	48	/* group move tag */
# define	ALIGN	49	/* alingment options tag */


# ifndef FORRBF

/* form attribute.. PopUp options (are the menu item def's supposed to
   be unique program wide? or just on the menu?? ) */
# define 	FORMATR 44

/* box resize definition */ 
# define	RESIZE	45

/* whereis cursor menuitem */
# define	WHERCUR	46

extern MENU cursor;
extern MENU IIVFcmCursorMenu;
# endif


# define	ATTR	17
# define	DATA	18
# define	TITLE	19
# define	DONE	20


# ifndef FORRBF
extern MENU entField;
# endif


# define	CENTER	21
# define	LEFT	22
# define	PLACE	23
# define	RIGHT	24
# define	SHIFT	25

extern MENU select_m;
extern MENU selbox_m;

# define	NAME	26




# define	selDATA	27
# define	selTITLE	28


# ifndef FORRBF
extern MENU selFldMu;
# endif


# ifdef FORRBF
extern MENU rSFldMu;
# endif


# ifndef FORRBF
extern MENU selComMu;
# endif


# define	ediATTR		29
# define	ediDATA		30
# define	ediTITLE	31
# define	ediBREAK 	32


# ifndef FORRBF
extern MENU edFldMu;
# endif


# ifdef FORRBF
extern MENU rbfedFldMu1;
extern MENU rbfedFldMu2;
# endif


# define	HEADING	32


# ifdef FORRBF
extern MENU rbfCursor;
# endif


# define	RCOLUMN	33

extern MENU rbfSelect;


# ifdef FORRBF
extern MENU rbfFldMu;

extern MENU rbfColSel;

# endif


# define	NAMES	34
# define	SEQUENCE	36




# define	SEQCHANGE	37
# define	SEQDFLT	38


# ifndef FORRBF
extern	MENU	muSequence;
# endif

# define	QDEF		39


# define	EXPANDIT	43

extern	MENU	boundMu;
