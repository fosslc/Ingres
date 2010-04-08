/*
**	"@(#)mwproto.h	1.35"
**	Copyright (c) 2004 Ingres Corporation
**	All Rights reseved.
*/

/*
** Name:	mwproto.h -- Function declarations for MWS exported functions.
**
** Description:
**	This file contains the declarations of all functions that
**	are exported by the MacWorkStation modules and used by the
**	FT layer.
**
** History:
**	25-sep-89 (nasser)
**		Initial definition.
**	07/10/90 (dkh) - Integrated changes into r63 code line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* External function declarations */

FUNC_EXTERN STATUS	IIMWbfBldForm(/* FRAME *frm */);
FUNC_EXTERN STATUS	IIMWbqtBegQueryTbl();
FUNC_EXTERN STATUS	IIMWcClose(/* char *msg */);
FUNC_EXTERN STATUS	IIMWcmtClearMapTbl(/* char map_type */);
FUNC_EXTERN STATUS	IIMWdfDelForm(/* FRAME *frm */);
FUNC_EXTERN STATUS	IIMWdlkDoLastKey(/* bool ignore,
		bool *do_on_host */);
FUNC_EXTERN STATUS	IIMWdmDisplayMsg(/* char *msg,
		i4 bell, FRS_EVCB *evcb */);
FUNC_EXTERN STATUS	IIMWdsmDoSkipMove(/* FRAME *frm, i4  *fldno,
		i4 dispmode, i4  (*rowtrv)(), i4  (*valfld)() */);
FUNC_EXTERN STATUS	IIMWduiDoUsrInput(/* KEYSTRUCT **ks */);
FUNC_EXTERN VOID	IIMWemEnableMws(/* i4  enable */);
FUNC_EXTERN STATUS	IIMWeqtEndQueryTbl();
FUNC_EXTERN VOID	IIMWfcFldChanged(/* i4 *chgbit */);
FUNC_EXTERN STATUS	IIMWfiFlushInput();
FUNC_EXTERN STATUS	IIMWfkmFKMode(/* i4  mode */);
FUNC_EXTERN STATUS	IIMWfmFlashMsg(/* char *msg, i4  bell */);
FUNC_EXTERN STATUS	IIMWfrvFreeRngVals(/* FRAME *frm */);
FUNC_EXTERN STATUS	IIMWgrvGetRngVal(/* FRAME *frm, i4  fldno,
		TBLFLD *tbl, u_char *buf */);
FUNC_EXTERN STATUS	IIMWgscGetSkipCnt(/* FRAME *frm,
		i4 old_fldno, i4  *cnt */);
FUNC_EXTERN STATUS	IIMWguiGetUsrInp(/* char *prompt, i4  echo,
 		FRS_EVCB *evcb, char *result */);
FUNC_EXTERN bool	IIMWguvGetUsrVer(/* char *prompt */);
FUNC_EXTERN STATUS	IIMWgvGetVal(/* FRAME *frm, TBLFLD *tbl */);
FUNC_EXTERN STATUS	IIMWiInit(/* i4  MW */);
FUNC_EXTERN i4		IIMWimIsMws();
FUNC_EXTERN STATUS	IIMWkfnKeyFileName(/* char *name */);
FUNC_EXTERN STATUS	IIMWkmiKeyMapInit(/* struct mapping *map1,
		i4 map1_sz, struct frsop *map2, i4  frsmap_sz,
 		i2 *frop, i4  frop_sz, i1 *flg */);
FUNC_EXTERN VOID	IIMWmiMenuInit(/* char *(*flfrsfunc)(),
		char **map */);
FUNC_EXTERN STATUS	IIMWmwhMsgWithHelp(/* char *short_msg,
		char *long_msg, FRS_EVCB *evcb */);
FUNC_EXTERN STATUS	IIMWpmPutMenu(/* MENU mu */);
FUNC_EXTERN STATUS	IIMWrRefresh();
FUNC_EXTERN STATUS	IIMWrdcRngDnCp(/* FRAME *frm, i4  fldno,
		i4 count */);
FUNC_EXTERN STATUS	IIMWrfRstFld(/* FRAME *frm, i4  fldno,
		TBLFLD *tbl */);
FUNC_EXTERN VOID	IIMWrmeRstMapEntry(/* char map_type,
		i4 index */);
FUNC_EXTERN STATUS	IIMWrmwResumeMW();
FUNC_EXTERN STATUS	IIMWrrvRestoreRngVals(/* FRAME *frm */);
FUNC_EXTERN STATUS	IIMWrucRngUpCp(/* FRAME *frm, i4  fldno,
		i4 count */);
FUNC_EXTERN STATUS	IIMWsaSetAttr(/* FRAME *frm, i4  fldno,
 		i4 disponly, i4  col, i4  row, i4 attr */);
FUNC_EXTERN STATUS	IIMWsbSoundBell();
FUNC_EXTERN STATUS	IIMWscfSetCurFld(/* FRAME *frm, i4  fldno */);
FUNC_EXTERN STATUS	IIMWscmSetCurMode(/* i4  runMode, i4  curMode,
		bool kyint, bool keyfind, bool keyfinddo */);
FUNC_EXTERN STATUS	IIMWshfShowHideForm(/* FRAME *frm */);
FUNC_EXTERN STATUS	IIMWsmeSetMapEntry(/* char map_type,
		i4 index */);
FUNC_EXTERN STATUS	IIMWsmwSuspendMW();
FUNC_EXTERN STATUS	IIMWsrvSaveRngVals(/* FRAME *frm */);
FUNC_EXTERN STATUS	IIMWtvTblVis(/* FRAME *frm, i4  fldno,
		i4 disponly, i4  display */);
FUNC_EXTERN STATUS	IIMWufUpdFld(/* FRAME *frm, i4  fldno,
		i4 disponly, i4  col, i4  row */);

