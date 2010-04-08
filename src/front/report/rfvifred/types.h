/*
**  types.h
**  contains the types of external procedures
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	02-jun-89 (bruceb)
**		Removed editcom()--not used.
**	09/23/89 (dkh) - Porting changes integration.
**	10-oct-89 (mgw)
**		backed out 9/23 change by changing vfcreate(), vfformattr(),
**		and vfWhersCurs() back to i4  from VOID. Changing them to VOID
**		creates illegal pointer combination warnings in menudef.c.
**		If you change them back to VOID, be sure to change the ct_func
**		member of the com_tab struct in front!hdr!hdr!menu.h to VOID
**		and also all funcs pointed at in com_tab.ct_func
**		initializations in menudef.c and any FUNC_EXTERN references
**		to them. There are about 13 such functions and the rFroptions()
**		function may pose a problem here since it returns a value
**		(is it used?).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-Jul-2009 (kschendel) SIR 122138
**	    SPARC solaris compiled 64-bit flunks undo test, fix by properly
**	    declaring exChange (for pointers) and exi4Change (for i4's).
**	25-Aug-2009 (kschendel) 121804
**	    Kill f-setfmt declaration, was wrong and it's declared in format.h
*/

u_char *vfgets();
FRAME	*vfgetRelFrame(), *vfgetFrame(), *blankFrame();
FIELD	*fdAlloc();
TRIM	*trAlloc();
FMT	*vfchkFmt();
i4	vfcuUp(), vfcuDown(), vfcuLeft(), vfcuRight();
i4	vfslUp(), vfslDown();
POS	*nextFeat(), *prevFeat(), *horOverLap(), *onPos(),
		*overLap(), *palloc();
VFNODE	*ndalloc(), **buildLines(),
		**vflnAdrNext(), *ndonPos();


FLDHDR	*FDgethdr();
FLDTYPE	*FDgettype();
FLDVAL	*FDgetval();
FLDCOL	*FDnewcol();
char	*saveStr();
VFNODE	*vflnNext();
POS	*onPos();
bool	putEdData();
u_char	*vfEdtStr();
char	*vfsaveFmt();
struct	vfqftbl *vfinittbl();

/*
**  vfqbftbl not called by anyone. (dkh)
**
FIELD	*vfqbftbl();
*/

FRAME	*vfqbffrm();
FRAME	*FDwfrcreate();
i4	blankcom();
i4	createcom();
i4	catcall();
i4	delCom();
i4	edit2com();
i4	enterAttr();
i4	enterData();
i4	enterField();
i4	enterTitle();
i4	enterTrim();
VOID	fdcats();
i4	heading();
i4	openCom();
i4	rFroptions();
i4	rFstructure();
i4	rFwrite();
i4	selectcom();
i4	undoCom();
i4	tblfldcreate();
i4	vfcreate();
i4	vfedit();
i4	vfseqchg();
i4	vfsequence();
i4	vfformattr();
i4	vfWhersCurs();
i4	vfnewbox();
i4	vfseqdflt();
bool	testSide();
bool	vfVertReg();
bool	inRegion();
VOID	vfDispAll();
VOID	vfDispBoxes();
VOID	reDisplay();
VOID	vfPostUpd();
VOID	vfUpdate();
VFNODE	*vfgrfbuild();
LIST	*lalloc();
POS	*maxColumn();
bool	placecmd();
bool	rightcom();
bool	leftcom();
bool	centercom();
bool	placeField();
bool	placeTable();
bool	placeTrim();
bool	placeBox();
bool	placeTitle();
bool	placeData();
bool	place_Column();
bool	titleDataOver();
FRAME	*FDfrcreate();
i4	qdefcom();
POS	*insBox();

/* Proper ansi-style definitions can start here... */
FUNC_EXTERN void compOver(POS *ps, POS *over);
FUNC_EXTERN void exChange(char **thing1, char **thing2);
FUNC_EXTERN void exi4Change(i4 *thing1, i4 *thing2);
