/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	rbfcons.h - constants for rbf
**
** Description:
**
** History:
**	30-sep-1987 (rdesmond)
**		consolidated with vifrbf.c and move to src directory.
**	24-nov-1987 (rdesmond)
**		changed rf_iidetail to rfiidetail and rf_iisave to rfiisave.
**	07/27/88 (dkh) - Shortened names to conform to coding standards.
**  07/27/89 (martym) - Added an origin type for JoinDefs.
**	08-aug-89 (cmr)
**		added defines for the forms and helpfiles for the new
**		RBF frames (F_BREAK, F_EDITLAYOUT). 
**	22-nov-89 (cmr)
**		added defines for the new forms F_COLUMNS and F_AGGREGATES.
**	05-dec-89 (cmr)
**		changed/added defines for help files.
**	12/10/89 (elein)
**		Added formname getname and its fields, defined new help files
**		Add listpick choices for data source, style and destination
**		Added OT_Duplicate, new "origin type"
**	12/27/89 (elein)
**		Rearranged order of style lists
**	1/2/90	(martym)
**		Added F_INDTOPTS.
**	04-jan-90 (sylviap)
**		Added batch pop-up frames.
**	1/9/90 (elein)
**		Remove Default as option for report styles, wrap is same thing.
**	1/9/90 (martym)
**		Added actions taken by r_JDMaintAttrList().
**	12-jan-90 (sylviap)
**		Added help files for Printer, File and Declare Variables frames.
**	1/23/90 (martym)
**		Added H_INDOPT.
**	2/15/90 (martym)
**		Changed all JD_ATR_* to JD_TAB_* and added JD_TAB_TAB_ADV_
**		INS_CNT, JD_TAB_INIT_INS_CNT, and JD_TAB_RVAR_INS_CNT.
**	16-feb-90 (sylviap)
**		Added field names for coptions.frm, rfindop.frm  and 
**		  rfoptions.frm.
**		Added H_SRTDIR, H_UNDOPT, H_INDOPT and H_SELVAL.
**	2/20/90 (martym)
**		Added RF_STAT_CHK().
**	3/2/90 (martym)
**		Changed JD_ATT_GET_FIELD to JD_ATT_GET_FIELD_CNT, and 
**		JD_ATT_GET_RVAR to JD_ATT_GET_RVAR_NAME. Also added 
**		JD_ATT_GET_FIELD_NAME.
**	02-apr-90 (sylviap)
**		Added H_WIDREP.
**	19-apr-90 (sylviap)
**		Added archive pop-up constants.
**	19-apr-90 (cmr)
**		Added H_UNIQAGGS.
**	22-apr-90 (cmr)
**		Added H_CUMAGGS.
**	5/23/90 (martym)
**		Added F_BRKOPT, F_BKCOLN, F_PRNTAL, F_PRNTBRK, F_PRNTPAGE,
**		F_NEWPAGE, F_PAGOPT.
**	6/6/90 (martym) 
**		Added H_BRKOPTS & H_PAGOPTS.
**	19-sep-90 (sylviap)
**		Took out F_BRKCOL.  No longer needed.
**	21-nov-90 (sylviap)
**		Added the folding formats, F_CF and F_CFE. (#34522, #34523)
**	15-mar-91 (sylviap)
**		Added F_BATCH1 & F_INSTR.
**	9-mar-1993 (rdrane)
**		Added RBF_DEF_VAR_NAME and RBF_MAX_VAR_SEQ.
**	30-jun-93 (sylviap)
**		Added F_FIRSTFF, F_RCOMP, F_INCL & F_FMT for roptions.frm.
**	1-jul-1993 (rdrane)
**		Added RBF_DEF_DATE_FMT, RBF_DEF_TIME_FMT, RBF_DEF_PAGENO_FMT,
**		RBF_DATE_LIT, RBF_TIME_LIT, and RBF_PAGENO_LIT to support user
**		override of these formats.
**	13-jul-1993 (rdrane)
**		Lowercase date/time  and pageno FMT variable name strings.
**		This is consistent w/ RBF casing of ColumnOption variables,
**		and eliminates DIFF's between RBF archive and COPYREP/SREPORT
**		results.
**	23-jul-93 (sylviap)
**		Added H_INCLCOMP, H_FORMFEED, H_PAGEHDR, H_FIRSTFF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# include <vifrbfc.h>

/*
**	Codes for underlining style.
*/

# define	ULS_NONE	'n'		/* no underlining */
# define	ULS_LAST	'l'		/* last line only */
# define	ULS_ALL		'a'		/* all lines */

/*
**	Prefixes for fields on Fstructure.
*/

# define	SEQ_PRE		ERx("seq")
# define	ORD_PRE		ERx("ord")
# define	BRK_PRE		ERx("brk")

/*
**	Code returned by rFmlayout()
*/

# define	LAYOUTERR	0
# define	LAYOUTOK	1
# define	LAYOUTWIDE	2



/*
**	Codes for Rbf_ortype (origin type)
**	These loosely correspond to the values of Rbf_SrcType
**	Since rfvifred uses these values to decide the save
**	method, and vifred uses these for other (?) things,
**	this value is set according to Rbf_SrcType.
*/

# define	OT_EDIT		0
# define	OT_TABLE	1
# define    	OT_JOINDEF	2
# define    	OT_DUPLICATE	3


/*
**	Error number for report in RBF being too wide for terminal.
**	Number is really 7711.
*/

# define	REPTOOWIDE	711

/*
**	Names of forms.
*/

# define	F_ROPTIONS	ERx("roptions")
# define	F_BREAK		ERx("break")
# define	F_COPTIONS	ERx("coptions")
# define	F_EDITLAYOUT	ERx("rep_layout")
# define	F_RSAVE		ERx("rfiisave")
# define	F_RDETAILS	ERx("rfiidetail")
# define	F_AGGREGATES	ERx("rfaggs")
# define	F_GETNAME	ERx("rfgetname")
# define	F_INDTOPTS	ERx("rfindop") 
# define	F_RBFABF	ERx("rbfabf")
# define	F_PRINT		ERx("rfprint")
# define	F_VARIABLE	ERx("rfvar")
# define	F_FPROMPT	ERx("rfprompt")
# define	F_FILE		ERx("rffile")
# define	F_ARCHIVE	ERx("rfarchive")
# define	F_BRKOPT	ERx("rfbrkopt") 
# define	F_PAGOPT	ERx("rfnwpag") 

/*
** Fields on Forms
*/
#define		F_GTNTITLE	ERx("name_title")
#define		F_GTNPROMPT	ERx("name_prompt")
#define		F_GTNOBJECT	ERx("objname")
#define		F_RFAFTYPE	ERx("rep_src")
#define		F_RFAFNAME	ERx("rep_name")

#define		F_STRTBL	ERx("rfstrtbl")		/* rfindopt.frm */
#define		F_COLNM		ERx("str_col")

#define		F_OPTTBL	ERx("ropttbl") 		/* roptions.frm */
#define		F_UNDER		ERx("ropt_under")
#define		F_PGLEN		ERx("rpl")
#define		F_UNDCHR	ERx("rulchar")
#define		F_FRMFD		ERx("rffs")
#define		F_PGHDR		ERx("rphdr_first")
#define		F_NULL		ERx("rnullstring")
#define		F_FIRSTFF	ERx("rfirstff")
#define		F_RCOMP		ERx("rcomp")
#define		F_INCL		ERx("include")
#define		F_FMT		ERx("format")

#define		F_SRTTBL	ERx("rfsrttbl") 	/* coptions.frm */
#define		F_SRTSEQ	ERx("srt_seq") 	
#define		F_SRTDIR	ERx("srt_dir") 	
#define		F_SELVAL	ERx("srt_selval") 	
#define		F_COLNAM	ERx("srt_colname") 	

	/* Field names on batch pop-ups */
#define		F_LOGNAME	ERx("log_name") 	/* rfprint.frm */
#define		F_PRINTER	ERx("print_name")
#define		F_COPIES	ERx("copies")
#define		F_BATCH		ERx("batch")
#define		F_BATCH1	ERx("batch1")
#define		F_INSTR		ERx("instructions")

#define		T_VAR		ERx("var") 		/* rfvar.frm */
#define		F_PROMPT	ERx("prompt_col")
#define		F_VALUE		ERx("value")

#define		F_COLUMN	ERx("colname")		/* rfprompt.frm */
#define		F_FULLPR	ERx("full_prompt")

#define		F_FNAME		ERx("fname")		/* rffile.frm 
							** and rarchive.frm */

#define		F_BKCOLN	ERx("brkcol")		/* rfbrkcol.frm */
#define		F_PRNTAL	ERx("prntalways")
#define		F_PRNTBRK	ERx("prntonbrk")
#define		F_PRNTPAGE	ERx("prntonpage")
#define		F_NEWPAGE	ERx("newpage")


/*
**	Help file names.
*/

# define	H_ROPTIONS	ERx("rbfropt.hlp")
# define	H_COPTIONS	ERx("rbfcopt.hlp")
# define	H_SECTIONS	ERx("rfsect.hlp")
# define	H_CREATSECT	ERx("rfcrsect.hlp")
# define	H_AGGREGATE	ERx("rfagg.hlp")
# define	H_STRUCTURE	ERx("rfstruct.hlp")
# define	H_RCATALOG	ERx("rbfcat.hlp")
# define	H_UTILS		ERx("rbfutils.hlp")
# define	H_SOURCE	ERx("rfsource.hlp")
# define	H_GTNDUP	ERx("rfgtndup.hlp")
# define	H_GTNTBL	ERx("rfgtntbl.hlp")
# define	H_GTNJD		ERx("rfgtnjd.hlp")
# define	H_LPREPORTS	ERx("rflprpt.hlp")
# define	H_LPJOINDEFS	ERx("rflpjd.hlp")
# define	H_STYLES	ERx("rfstyles.hlp")
# define	H_DEST		ERx("rfdest.hlp")
# define	H_DETAIL	ERx("rfdetail.hlp")
# define	H_COLUMNS	ERx("rfcols.hlp")
# define	H_PRINT		ERx("rbfprint.hlp")
# define	H_FILE		ERx("rbffile.hlp")
# define	H_DECLVAR	ERx("rbfvar.hlp")
# define	H_PROMPT	ERx("rbfprmt.hlp")
# define	H_INDOPT	ERx("rfindop.hlp")
# define	H_SRTDIR	ERx("rfsrtdr.hlp")
# define	H_SELVAL	ERx("rfselvl.hlp")
# define	H_UNDOPT	ERx("rfund.hlp")
# define	H_INDBRK	ERx("rfindbrk.hlp")
# define	H_WIDREP	ERx("rfwidrep.hlp")
# define	H_ARCHIVE	ERx("rfarchv.hlp")
# define	H_UNIQAGGS	ERx("rfaggu.hlp")
# define	H_CUMAGGS	ERx("rfaggc.hlp")
# define	H_BRKOPTS	ERx("rfbrkopt.hlp")
# define	H_PAGOPTS	ERx("rfnwpag.hlp")
# define	H_INCLCOMP	ERx("rfincl.hlp")
# define	H_FORMFEED	ERx("rffrmfd.hlp")
# define	H_PAGEHDR	ERx("rfpghdr.hlp")
# define	H_FIRSTFF	ERx("rfrstff.hlp")

/*
** List Pick Selections--These must correspond
** to the order in which the items are displayed
** in the listpick list.  The selections from
** the listpick lists are in errf.msg. 
**/
/* ListPick List for Data Source Selection
** is in rcons.h 
*/

/* ListPick List for Style Selection*/
#define         NoStyle         -1      /* No Style */
#define         TabStyle        0       /* Tabular */
#define         WrpStyle        1       /* Wrap */
#define         BlkStyle        2       /* Block */
#define         IndStyle        3       /* Indented */
#define         LblStyle        4       /* Label */
#define         MDStyle         5       /* Master/Detail */

/* ListPick List for Report Destination */
#define         ToNowhere       -1      /* Output to screen or file */
#define         ToDefault       0       /* Output to screen or file */
#define         ToPrinter       1       /* Output to printer */
#define         ToFile          2       /* Output to file */

/*
** Actions supported by the "r_JDMaintAttrList()" module:
*/
#define JD_ATT_ADD_TO_LIST       1	/* Add to list                        */
#define JD_ATT_GET_FIELD_CNT     2	/* Get the field cnt                  */
#define JD_ATT_DELETE_FROM_LIST  3 	/* Delete from list                   */
#define JD_ATT_LIST_EMPTY        4	/* Is the list empty?                 */
#define JD_ATT_LIST_FLUSH        5	/* Flush the list                     */
#define JD_ATT_INIT_USE_FLAGS    6	/* Reset all of the usage flags       */
#define JD_ATT_SET_USE_FLAG      7	/* Mark attribute as having been used */
#define JD_ATT_GET_OCCUR_CNT     8	/* Get number of occurrenece of attr  */
#define JD_ATT_GET_RVAR_NAME     9	/* Get the range variable of attr     */
#define JD_ATT_GET_ATTR_NAME     10	/* Get the internal name of attr      */
#define JD_ATT_GET_FIELD_NAME    11	/* Get the name of attr               */

/*
** Actions supported by the "r_JDMaintTabList()" module:
*/
#define JD_TAB_INIT		 1	/* Empty the list                     */
#define JD_TAB_ADD		 2	/* Add to the list                    */
#define JD_TAB_INIT_INS_CNT	 3	/* Reset all of the instance counts   */
#define JD_TAB_TAB_ADV_INS_CNT	 4	/* Advance the instance counts by rvar*/
#define JD_TAB_RVAR_ADV_INS_CNT	 5	/* Advance the instance counts by tab */


/*
** Macro to check the return status of calls to functions returning STATUS. 
** It will return if the status is not OK:
*/
#define RF_STAT_CHK(rf_op) 	if ((stat = (rf_op)) != OK) return(stat)


/* FORMATS - Needed when creating Indented and Master/Detail reports */

# define        F_CF            21      /* folding character type */
# define        F_CFE           22      /* same as 'cf' but preserve spaces */

/*
** Default base name for an RW variable generated from a 
** "degenerate" (IIUGfnFeName() returns EOS) delim id.
** Allow for a full i4  (i4) sequence number (13 digits).
*/
#define	RBF_DEF_VAR_NAME	ERx("rbfvar")
#define	RBF_MAX_VAR_SEQ		(13)

/*
** Establish default format strings and variable names for header
** date/time stamp and footer page number report variables.
*/
# define RBF_DEF_DATE_FMT	ERx("d'03-feb-1901'")
# define RBF_DEF_W_DATE_FMT	11
# define RBF_NAM_DATE_FMT	ERx("ii_rbf_date_fmt")
# define RBF_DEF_TIME_FMT	ERx("d'16:05:06'")
# define RBF_DEF_W_TIME_FMT	8
# define RBF_NAM_TIME_FMT	ERx("ii_rbf_time_fmt")
# define RBF_DEF_PAGENO_FMT	ERx("'\\-zzzn \\-'")
# define RBF_DEF_W_PAGENO_FMT	7
# define RBF_NAM_PAGENO_FMT	ERx("ii_rbf_pageno_fmt")

/*
** Establish display literals for header date/time stamp
** and footer page number form titles.
*/
# define	RBF_DATE_LIT	ERx("Date")
# define	RBF_TIME_LIT	ERx("Time")
# define	RBF_PAGE_LIT	ERx("Page")

