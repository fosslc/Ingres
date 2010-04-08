/*
**	"@(#)mwintrnl.h	1.63"
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
** Name:	mwintrnl.h -- Defintions and declarations for MWS module files.
**
** Description:
**	This file declares and defines macros and functions for use
**	by the files in the MacWorkStation module.  This file is intended
**	for the internal use of MWS files.
**
** History:
**	25-sep-89 (nasser) - Initial definition.
**	07/10/90 (dkh) - Integrated changes into r63 code line.
**	24-aug-90 (nasser) - Added #define's for IIMWpxc() flags.
**	10/08/90 (nasser) - Removed the mwio functions as we can
**		use TE now.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* Flags to pass to IIMWpxc() */
# define mwMSG_FLUSH		0x01
# define mwMSG_SEQ_FORCE	0x02


/* Are we talking to MWS? */
GLOBALREF	i4	IIMWmws;

/* Function declarations */
# ifndef MAC_HOST

FUNC_EXTERN STATUS	IIMWaciAddColumnItem(/* i4  formAlias,
		i4 itemAlias, i4  column, FLDCOL * theColumn */);
FUNC_EXTERN STATUS	IIMWafAddForm(/* i4  formAlias,
		FRAME * theFrame */);
FUNC_EXTERN STATUS	IIMWafiAddFieldItem(/* i4  formAlias,
		i4 itemAlias, REGFLD * theField */);
FUNC_EXTERN STATUS	IIMWatcAddTableCell(/* i4  formAlias,
		i4 itemAlias, i4  column, i4  row, i4 flags,
		FLDVAL * theVal */);
FUNC_EXTERN STATUS	IIMWatgAddTblGroup(/* i4  formAlias,
		i4 itemAlias, i4  start_col, i4  start_row,
		i4 end_col, i4  end_row, i4 flags */);
FUNC_EXTERN STATUS	IIMWatiAddTableItem(/* i4  formAlias,
		i4 itemAlias, TBLFDL * theTable */);
FUNC_EXTERN STATUS	IIMWatrAddTRim(/* i4  formAlias, i4  itemAlias,
		TRIM * theTrim */);
FUNC_EXTERN STATUS	IIMWbfsBegFormSystem();
FUNC_EXTERN VOID	IIMWcfCloseForms();
FUNC_EXTERN STATUS	IIMWcmmCreateMacMenus();
FUNC_EXTERN STATUS	IIMWctgClearTblGroup(/* i4  formAlias,
		i4 itemAlias, i4  start_col, i4  start_row,
		i4 end_col, i4  end_row */);
FUNC_EXTERN STATUS	IIMWdccDoCompatCheck(/* i4  compat_info,
		char *ingres_ver, bool *proceed */);
FUNC_EXTERN STATUS	IIMWdfiDeleteFormItem(/* i4  formAlias,
		i4 itemAlias */);
FUNC_EXTERN STATUS	IIMWdrDoRun(/* i4  *res, i4  *flags,
		char *buf, KEYSTRUCT *kstruct */);
FUNC_EXTERN STATUS	IIMWdvvDViewVersion(/* TPMsg theMsg */);
FUNC_EXTERN STATUS	IIMWefsEndFormSystem();
FUNC_EXTERN STATUS	IIMWffFrontForm(/* i4  formAlias */);
FUNC_EXTERN STATUS	IIMWfftFreeFldsText(/* i4  formAlias */);
FUNC_EXTERN STATUS	IIMWfkcFKClear(/* char map_type */);
FUNC_EXTERN STATUS	IIMWfksFKSet(/* char map_type, i4  item,
		i2 val, i1 flag */);
FUNC_EXTERN STATUS	IIMWgciGetCurItem(/* i4  * formAlias,
		i4 * itemAlias, i4  * column, i4  * row */);
FUNC_EXTERN STATUS	IIMWgctGetCurTbcell(/* i4  * formAlias,
		i4 * itemAlias, i4  * column, i4  * row */);
FUNC_EXTERN TPMsg	IIMWgeGetEvent(/* i4  * class, i4  * id */);
FUNC_EXTERN STATUS	IIMWgitGetItemText(/* i4  formAlias,
		i4 itemAlias, u_char * text */);
FUNC_EXTERN PTR		IIMWgpGetPassword(/* PTR msg, bool bell */);
FUNC_EXTERN TPMsg	IIMWgseGetSpecificEvent(/* i4  class, i4  id */);
FUNC_EXTERN STATUS	IIMWgsiGetSysInfo();
FUNC_EXTERN STATUS	IIMWgttGetTbcellText(/* i4  formAlias,
		i4 itemAlias, i4  *column, i4  *row,
		i4 row_nbr_abs, u_char *text */);
FUNC_EXTERN STATUS	IIMWhfHideForm(/* i4  formAlias */);
FUNC_EXTERN STATUS	IIMWhiHideItem(/* i4  formAlias,
		i4 itemAlias, i4  col */);
FUNC_EXTERN STATUS	IIMWhvrHostVarResp(/* i4  variable, PTR text */);
FUNC_EXTERN i4		IIMWimIsMws();
FUNC_EXTERN STATUS	IIMWmgdMsgGetDec(/* TPMsg msg, i4  *num,
		char mark */);
FUNC_EXTERN STATUS	IIMWmgiMsgGetId(/* TPMsg msg, char *ch,
		i4 *id */);
FUNC_EXTERN STATUS	IIMWmgsMsgGetStr(/* TPMsg msg, char *str,
		char mark */);
FUNC_EXTERN STATUS	IIMWmvMwsVersion(/* TPMsg theMsg */);
FUNC_EXTERN STATUS	IIMWofOpenFile(/* char *file_env, char *mode,
 		FILE **fp, i4 log_ctrl */);
FUNC_EXTERN STATUS	IIMWpcPutCmd(/* u_char * class_id,
		u_char * params */);
FUNC_EXTERN STATUS	IIMWpdmPDisplayMsg(/* PTR msg, bool bell */);
FUNC_EXTERN VOID	IIMWpelPrintErrLog(/* i4 type, char *msg */);
FUNC_EXTERN STATUS	IIMWpfmPFlashMsg(/* PTR msg, bool bell,
		i4 time */);
FUNC_EXTERN VOID	IIMWplPrintLog(/* i4 type, char *msg */);
FUNC_EXTERN STATUS	IIMWplkProcLastKey(/* bool ignore */);
FUNC_EXTERN STATUS	IIMWpmhPMsgHelp(/* char *short_msg,
		char *long_msg, bool bell */);
FUNC_EXTERN STATUS	IIMWprdPRngDncp(/* i4  frm_alias,
		i4 item_alias, i4  count */);
FUNC_EXTERN STATUS	IIMWprfPRstFld(/* i4  frm_alias, i4  item_alias,
		i4 col, i4  row */);
FUNC_EXTERN STATUS	IIMWpruPRngUpcp(/* i4  frm_alias,
		i4 item_alias, i4  count */);
FUNC_EXTERN PTR		IIMWpuPromptUser(/* PTR msg, bool bell */);
FUNC_EXTERN VOID	IIMWpvPutVal(/* i4  changed, char *text */);
FUNC_EXTERN STATUS	IIMWpxcPutXCmd(/* u_char *class_id, u_char *params,
		i4 flags */);
FUNC_EXTERN STATUS	IIMWqhvQueryHostVar(/* TPMsg theMsg */);
FUNC_EXTERN STATUS	IIMWrfsResumeFormSystem();
FUNC_EXTERN STATUS	IIMWrftRestoreFldsText(/* i4  formAlias */);
FUNC_EXTERN STATUS	IIMWsciSetCurItem(/* i4  FormAlias,
		i4 ItemAlias, FRAME *frm, REGFLD *fld */);
FUNC_EXTERN STATUS	IIMWsctSetCurTbcell(/* i4  FormAlias,
		i4 ItemAlias, FRAME *frm, TBLFLD *tbl */);
FUNC_EXTERN STATUS	IIMWsfShowForm(/* i4  formAlias */);
FUNC_EXTERN STATUS	IIMWsfsSuspendFormSystem();
FUNC_EXTERN STATUS	IIMWsftSaveFldsText(/* i4  formAlias */);
FUNC_EXTERN STATUS	IIMWshvSetHostVar(/* TPMsg theMsg */);
FUNC_EXTERN STATUS	IIMWsiShowItem(/* i4  formAlias,
		i4 itemAlias, i4  col */);
FUNC_EXTERN STATUS	IIMWsiaSetItemAttr(/* i4  formAlias,
		i4 itemAlias, i4 attributes */);
FUNC_EXTERN STATUS	IIMWsimSetIngresMenu(/* i4  count,
		u_char * theMenu */);
FUNC_EXTERN STATUS	IIMWsitSetItemText(/* i4  frm_alias,
		i4 item_alias, u_i4 cnt, u_char *text */);
FUNC_EXTERN STATUS	IIMWsrmSetRunMode(/* i4  runMode,
		i4 curMode, i4  flags */);
FUNC_EXTERN STATUS	IIMWstfScrollTblFld(/* i4  formAlias,
		i4 itemAlias, i4  count */);
FUNC_EXTERN STATUS	IIMWsttSetTbfldText(/* i4  frm_alias,
		i4 item_alias, i4  col, i4  row, u_i4 cnt,
		u_char *text */);
FUNC_EXTERN STATUS	IIMWsvSysVersion(/* TPMsg theMsg */);
FUNC_EXTERN STATUS	IIMWtfTossForm(/* i4  alias */);
FUNC_EXTERN VOID	IIMWuaUnusedArg();
FUNC_EXTERN STATUS	IIMWynqYNQues(/* PTR prompt,
		bool bell, bool *response */);

# else /* MAC_HOST */

/* include the header files needed by the following declarations */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <adf.h>
# include <fmt.h>
# include <ft.h>
# include <frame.h>
# include <frsctblk.h>
# include <kst.h>

STATUS	IIMWaciAddColumnItem(i4 formAlias, i4  ItemAlias,
		i4 column, FLDCOL *theColumn);
STATUS	IIMWafAddForm(i4 formAlias,FRAME *theFrame);
STATUS	IIMWafiAddFieldItem(i4 formAlias, i4  ItemAlias,
		REGFLD *theField);
STATUS	IIMWatcAddTableCell(i4 FormAlias,nat ItemAlias,
		i4 column,nat row,i4 flags,FLDVAL *theVal);
STATUS	IIMWatgAddTblGroup(i4 formAlias, i4  itemAlias,
		i4 start_col, i4  start_row, i4  end_col, i4  end_row,
		i4 flags);
STATUS	IIMWatiAddTableItem(i4 FormAlias, i4  ItemAlias,
		TBLFLD *theTable);
STATUS	IIMWatrAddTRim(i4 FormAlias, i4  ItemAlias, TRIM *theTrim);
STATUS	IIMWbfBldForm(FRAME *frm);
STATUS	IIMWbfsBegFormSystem();
STATUS	IIMWbqtBegQueryTbl();
STATUS	IIMWcClose(char *msg);
VOID	IIMWcfCloseForms();
STATUS	IIMWcmmCreateMacMenus();
STATUS	IIMWcmtClearMapTbl(char map_type);
STATUS	IIMWctgClearTblGroup(i4 formAlias, i4  itemAlias,
		i4 start_col, i4  start_row, i4  end_col, i4  end_row);
STATUS	IIMWdccDoCompatCheck(i4 compat_info, char *ingres_ver,
		bool *proceed);
STATUS	IIMWdfDelForm(FRAME *frm);
STATUS	IIMWdfiDeleteFormItem(i4 FormAlias,nat ItemAlias);
STATUS	IIMWdlkDoLastKey(bool ignore, bool *do_on_host);
STATUS	IIMWdmDisplayMsg(char *msg, i4  bell, FRS_EVCB *evcb);
STATUS	IIMWdrDoRun(i4 *res, i4  *flags, char *buf, KEYSTRUCT *kstruct);
STATUS	IIMWdsmDoSkipMove(FRAME *frm, i4  *fldno, i4  dispmode,
		i4 (*rowtrv)(), i4  (*valfld)());
STATUS	IIMWduiDoUsrInput(KEYSTRUCT **ks);
STATUS	IIMWdvvDViewVersion(TPMsg theMsg);
STATUS	IIMWefsEndFormSystem();
VOID	IIMWemEnableMws(i4 enable);
STATUS	IIMWeqtEndQueryTbl();
VOID	IIMWfcFldChanged(i4 *chgbit);
STATUS	IIMWffFrontForm(i4 FormAlias);
STATUS	IIMWfftFreeFldsText(i4 formAlias);
STATUS	IIMWfiFlushInput();
STATUS	IIMWfkcFKClear(char map_type);
STATUS	IIMWfkmFKMode(i4 mode);
STATUS	IIMWfksFKSet(char map_type,nat item,i2 val,i1 flag);
STATUS	IIMWfmFlashMsg(char *msg, i4  bell);
STATUS	IIMWfrvFreeRngVals(FRAME *frm);
STATUS	IIMWgciGetCurItem(i4 *FormAlias,nat *ItemAlias,
		i4 *column,nat *row);
STATUS	IIMWgctGetCurTbcell(i4 *FormAlias, i4  *ItemAlias,
		i4 *column, i4  *row);
TPMsg	IIMWgeGetEvent(i4 *class, i4  *id);
i4	IIMWgfkGetFRSKeyevent(i4 theKey);
STATUS	IIMWgitGetItemText(i4 FormAlias,nat ItemAlias,u_char *text);
PTR	IIMWgpGetPassword(PTR msg,bool bell);
STATUS	IIMWgrvGetRngVal(FRAME *frm, i4  fldno, TBLFLD	*tbl,
		u_char *buf);
STATUS	IIMWgscGetSkipCnt(FRAME *frm, i4  old_fldno, i4  *cnt);
TPMsg	IIMWgseGetSpecificEvent(i4 class, i4  id);
STATUS	IIMWgsiGetSysInfo();
STATUS	IIMWgttGetTbcellText(i4 FormAlias,nat ItemAlias,
		i4 *column, i4  *row, i4  row_nbr_abs, u_char *text);
STATUS	IIMWguiGetUsrInp(char *prompt, i4  echo,
		FRS_EVCB *evcb, char *result);
bool	IIMWguvGetUsrVer(char *prompt);
STATUS	IIMWgvGetVal(FRAME *frm, TBLFLD *tbl);
STATUS	IIMWhfHideForm(i4 FormAlias);
STATUS	IIMWhiHideItem(i4 FormAlias, i4  ItemAlias, i4  col);
STATUS	IIMWhvrHostVarResp(i4 variable, PTR text);
STATUS	IIMWiInit(i4 MW);
i4	IIMWimIsMws();
STATUS	IIMWkfnKeyFileName(char *name);
STATUS	IIMWkmiKeyMapInit(struct mapping *map1, i4  map1_sz,
		struct frsop *map2, i4  frsmap_sz,
		i2 *frop, i4  frop_sz, i1 *flg);
STATUS	IIMWmgdMsgGetDec(TPMsg msg, i4  *num, char mark);
STATUS	IIMWmgiMsgGetId(TPMsg msg, char *ch, i4  *id);
STATUS	IIMWmgsMsgGetStr(TPMsg msg, char *str, char mark);
VOID	IIMWmiMenuInit(char *(*flfrsfunc)(), char **map);
STATUS	IIMWmvMwsVersion(TPMsg theMsg);
STATUS	IIMWmwhMsgWithHelp(char *short_msg, char *long_msg,
		FRS_EVCB *evcb);
STATUS	IIMWofOpenFile(char *file_env, char *mode,
		FILE **fp, i4 log_ctrl);
STATUS	IIMWpcPutCmd(PTR class_id, PTR pString);
STATUS	IIMWpdmPDisplayMsg(msg,bell);
VOID	IIMWpelPrintErrLog(i4 type, char *msg);
STATUS	IIMWpfmPFlashMsg(msg,bell,time);
VOID	IIMWplPrintLog(i4 type, char *msg);
STATUS	IIMWplkProcLastKey(bool ignore);
STATUS	IIMWpmhPMsgHelp(short_msg,long_msg,bell);
STATUS	IIMWprfPRstFld(i4 FormAlias,nat ItemAlias,nat column,nat row);
PTR	IIMWpuPromptUser(PTR msg,bool bell);
VOID	IIMWpvPutVal(i4 changed, char *text);
STATUS	IIMWpxcPutXCmd(PTR class_id, PTR params, i4  flags);
STATUS	IIMWqhvQueryHostVar(TPMsg theMsg);
STATUS	IIMWrRefresh();
STATUS	IIMWrdcRngDnCp(FRAME *frm, i4  fldno, i4  count);
STATUS	IIMWrfRstFld(FRAME *frm, i4  fldno, TBLFLD *tbl);
STATUS	IIMWrfsResumeFormSystem();
STATUS	IIMWrftRestoreFldsText(i4 formAlias);
VOID	IIMWrmeRstMapEntry(char map_type, i4  index);
STATUS	IIMWrmwResumeMW();
STATUS	IIMWrrvRestoreRngVals(FRAME *frm);
STATUS	IIMWrucRngUpCp(FRAME *frm, i4  fldno, i4  count);
STATUS	IIMWsaSetAttr(FRAME *frm, i4  fldno, i4  disponly,
		i4 col, i4  row, i4 attr);
STATUS	IIMWsbSoundBell();
STATUS	IIMWscfSetCurFld(FRAME *frm, i4  fldno);
STATUS	IIMWsciSetCurItem(i4 FormAlias, i4  ItemAlias, FRAME *frm,
		REGFLD *fld);
STATUS	IIMWscmSetCurMode(i4 runMode, i4  curMode, bool kyint,
		bool keyfind, bool keyfinddo);
STATUS	IIMWsctSetCurTbcell(i4 FormAlias, i4  ItemAlias, FRAME *frm,
		TBLFLD *tbl);
STATUS	IIMWsfShowForm(i4 FormAlias);
STATUS	IIMWsfsSuspendFormSystem();
STATUS	IIMWsftSaveFldsText(i4 formAlias);
STATUS	IIMWshfShowHideForm(FRAME *frm);
STATUS	IIMWshvSetHostVar(TPMsg theMsg);
STATUS	IIMWsiShowItem(i4 FormAlias, i4  ItemAlias, i4  col);
STATUS	IIMWsiaSetItemAttr(i4 FormAlias,nat ItemAlias,
		i4 column,nat row,i4 flags);
STATUS	IIMWsimSetIngresMenu(i4 count,u_char *theMenu);
STATUS	IIMWsitSetItemText(i4 FormAlias, i4  ItemAlias,
		u_i4 count, u_char *text);
STATUS	IIMWsmeSetMapEntry(char map_type, i4  index);
STATUS	IIMWsmwSuspendMW();
STATUS	IIMWsrmSetRunMode(i4 runMode, i4  curMode, i4  flags);
STATUS	IIMWsrvSaveRngVals(FRAME *frm);
STATUS	IIMWstfScrollTblFld(i4 formAlias, i4  itemAlias, i4  count);
STATUS	IIMWsttSetTbcellText(i4 FormAlias, i4  ItemAlias,
		i4 column, i4  row, u_i4 count, u_char *text);
STATUS	IIMWsvSysVersion(TPMsg theMsg);
STATUS	IIMWtfTossForm(i4 FormAlias);
STATUS	IIMWtvTblVis(FRAME *frm, i4  fldno, i4  disponly, i4  display);
VOID	IIMWuaUnusedArg();
STATUS	IIMWufUpdFld(FRAME *frm, i4  fldno, i4  disponly,
		i4 col, i4  row);
STATUS	IIMWynqYNQues(PTR prompt, bool bell, bool *response);

# endif /* MAC_HOST */
