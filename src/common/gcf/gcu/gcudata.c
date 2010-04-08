/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>
#include    <sl.h>
#include    <sp.h>
#include    <nm.h>
#include    <qu.h>
#include    <me.h>
#include    <mo.h>
#include    <st.h>
#include    <si.h>
#include    <cm.h>
#include    <cs.h>
#include    <cv.h>
#include    <mu.h>
#include    <tm.h>
#include    <gc.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcm.h>
#include    <gcaint.h>
#include    <gcaimsg.h>
#include    <gcocomp.h>
#include    <gcoint.h>
#include    <gcccl.h>
#include    <gcc.h>
#include    <gccer.h>
#include    <gcxdebug.h>

/*
** Name:	gcudata.c
**
** Description:	Global data for GCU facility.
**
** History:
**	23-sep-96 (canor01)
**	    Created.
**	 5-Feb-97 (gordy)
**	    Added gcx_gcd_type[] for new internal GCD datatypes.
**	    Additional changes made for compiling the new types.
**	27-Feb-97 (gordy)
**	    Filled out the GCA message info.
**	03-Jun-97 (rajus01)
**	    Added ATSDP in gcx_gcc_atl[].
**	20-Aug-97 (gordy)
**	    Added OTTDR in gcx_gcc_otl[].
**	17-Feb-98 (gordy)
**	    Add MIB definitions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 3-May-01 (gordy)
**	    Added new tracing definitions for Unicode datatype support.
**      25-June-2001(wansh01) 
**         Removed gcx_class. Moved gcxlevel declaration to
**         iigcc.c.
**	22-Mar-02 (gordy)
**	    Got rid of unneeded atomic types.  GCA now defines datatypes
**	    rather than using DBMS definitions.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	20-Apr-04 (gordy)
**	    Added support for 8 byte integers: OP_CV_I8, GCO_DT_INT8,
**	    GCO_DT_INT8_N.
**	20-Sept-04 (wansh01)
**	    Added support for GCC/ADMIN: SACMD, IACMD, AACMD.  
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Renamed gcx_gcc_ops to gcx_gco_ops, as this referenced outside
**          of GCC.  Moved other gcx_gcc global definitions to gccglbl.c,
**          as they are intended for the use of GCC only.
**	31-May-06 (gordy)
**	    Added ANSI date/time data types.
**	24-Oct-06 (gordy)
**	    Added Blob/Clob locator data types.
**	12-Mar-07 (gordy)
**	    Added GCA2_FETCH message.
**	 1-Oct-09 (gordy)
**	    Added GCA2_INVPROC message.
**	 9-Oct-09 (gordy)
**	    Added Boolean datatype.
**	22-Jan-10 (gordy)
**	    DOUBLEBYTE code generalized for multi-byte charset processing.
*/

/*
**
LIBRARY = IMPGCFLIBDATA
**
*/

/* gcudebug.c */
GLOBALDEF char tabs8[] = "\t\t\t\t\t\t\t\t";


/* from gca.h */

GLOBALDEF GCXLIST
gcx_gca_msg[] = {
	GCA_MD_ASSOC, "GCA_MD_ASSOC",
	GCA_ACCEPT, "GCA_ACCEPT",
	GCA_REJECT, "GCA_REJECT",
	GCA_RELEASE, "GCA_RELEASE",
	GCA_Q_BTRAN, "GCA_Q_BTRAN",
	GCA_S_BTRAN, "GCA_S_BTRAN",
	GCA_ABORT, "GCA_ABORT",
	GCA_SECURE, "GCA_SECURE",
	GCA_DONE, "GCA_DONE",
	GCA_REFUSE, "GCA_REFUSE",
	GCA_COMMIT, "GCA_COMMIT",
	GCA_QUERY, "GCA_QUERY",
	GCA_DEFINE, "GCA_DEFINE",
	GCA_INVOKE, "GCA_INVOKE",
	GCA_FETCH, "GCA_FETCH",
	GCA_DELETE, "GCA_DELETE",
	GCA_CLOSE, "GCA_CLOSE",
	GCA_ATTENTION, "GCA_ATTENTION",
	GCA_QC_ID, "GCA_QC_ID",
	GCA_TDESCR, "GCA_TDESCR",
	GCA_TUPLES, "GCA_TUPLES",
	GCA_C_INTO, "GCA_C_INTO",
	GCA_C_FROM, "GCA_C_FROM",
	GCA_CDATA, "GCA_CDATA",
	GCA_ACK, "GCA_ACK",
	GCA_RESPONSE, "GCA_RESPONSE",
	GCA_ERROR, "GCA_ERROR",
	GCA_TRACE, "GCA_TRACE",
	GCA_Q_ETRAN, "GCA_Q_ETRAN",
	GCA_S_ETRAN, "GCA_S_ETRAN",
	GCA_IACK, "GCA_IACK",
	GCA_NP_INTERRUPT, "GCA_NP_INTERRUPT",
	GCA_ROLLBACK, "GCA_ROLLBACK",
	GCA_Q_STATUS, "GCA_Q_STATUS",
	GCA_CREPROC, "GCA_CREPROC",
	GCA_DRPPROC, "GCA_DRPPROC",
	GCA_INVPROC, "GCA_INVPROC",
	GCA_RETPROC, "GCA_RETPROC",
	GCA_EVENT, "GCA_EVENT",
	GCA1_C_INTO, "GCA1_C_INTO",
	GCA1_C_FROM, "GCA1_C_FROM",
	GCA1_DELETE, "GCA1_DELETE",
	GCA_XA_SECURE, "GCA_XA_SECURE",
	GCA1_INVPROC, "GCA1_INVPROC",
	GCA_PROMPT, "GCA_PROMPT",
	GCA_PRREPLY, "GCA_PRREPLY",
	GCA1_FETCH, "GCA1_FETCH",
	GCA2_FETCH, "GCA2_FETCH",
	GCA2_INVPROC, "GCA2_INVPROC",

	GCN_NS_RESOLVE, "GCN_NS_RESOLVE",
	GCN_RESOLVED, "GCN_RESOLVED",
	GCN_NS_OPER, "GCN_NS_OPER",
	GCN_RESULT, "GCN_RESULT",
	GCN_NS_AUTHORIZE, "GCN_NS_AUTHORIZE",
	GCN_AUTHORIZED, "GCN_AUTHORIZED",
	GCN_NS_2_RESOLVE, "GCN_NS_2_RESOLVE",
	GCN2_RESOLVED, "GCN2_RESOLVED",

	GCA_IT_DESCR, "GCA_IT_DESCR",
	GCA_TO_DESCR, "GCA_TO_DESCR",
	GCA_GOTOHET, "GCA_GOTOHET",

	GCA_XA_START, "GCA_XA_START",
	GCA_XA_END, "GCA_XA_END",
	GCA_XA_PREPARE, "GCA_XA_PREPARE",
	GCA_XA_COMMIT, "GCA_XA_COMMIT",
	GCA_XA_ROLLBACK, "GCA_XA_ROLLBACK",

	GCM_GET, "GCM_GET",
	GCM_GETNEXT, "GCM_GETNEXT",
	GCM_SET, "GCM_SET",
	GCM_RESPONSE, "GCM_RESPONSE",
	GCM_SET_TRAP, "GCM_SET_TRAP",
	GCM_TRAP_IND, "GCM_TRAP_IND",
	GCM_UNSET_TRAP, "GCM_UNSET_TRAP",

	0, 0
};



GLOBALDEF GCXLIST
gcx_atoms[] = 
{
	GCO_ATOM_CHAR,		"GCO_ATOM_CHAR",
	GCO_ATOM_BYTE,		"GCO_ATOM_BYTE",
	GCO_ATOM_INT,		"GCO_ATOM_INT",
	GCO_ATOM_FLOAT,		"GCO_ATOM_FLOAT",
	0, 0
} ; 

GLOBALDEF GCXLIST
gcx_datatype[] = 
{
	GCA_TYPE_CHAR,		"CHAR",
	GCA_TYPE_BYTE,		"BYTE",
	GCA_TYPE_NCHR,		"UCS2",
	GCA_TYPE_C,		"'C'",
	GCA_TYPE_QTXT,		"QUERY TEXT",
	GCA_TYPE_NQTXT,		"UCS2 QUERY TEXT",
	GCA_TYPE_VCHAR,		"VARCHAR",
	GCA_TYPE_VBYTE,		"VARBYTE",
	GCA_TYPE_VNCHR,		"VARUCS2",
	GCA_TYPE_TXT,		"TEXT",
	GCA_TYPE_LTXT,		"LONG TEXT",
	GCA_TYPE_LCHAR,		"LONG CHAR",
	GCA_TYPE_LBYTE,		"LONG BYTE",
	GCA_TYPE_LNCHR,		"LONG UCS2",
	GCA_TYPE_LCLOC,		"LONG CHAR Locator",
	GCA_TYPE_LBLOC,		"LONG BYTE Locator",
	GCA_TYPE_LNLOC,		"LONG UCS2 Locator",
	GCA_TYPE_INT,		"INTEGER",
	GCA_TYPE_FLT,		"FLOAT",
	GCA_TYPE_DATE,		"IDATE",
	GCA_TYPE_ADATE,		"ADATE",
	GCA_TYPE_TIME,		"TIME LCL",
	GCA_TYPE_TMWO,		"TIME W/O",
	GCA_TYPE_TMTZ,		"TIME TZ",
	GCA_TYPE_TS,		"TIMESTAMP LCL",
	GCA_TYPE_TSWO,		"TIMESTAMP W/O",
	GCA_TYPE_TSTZ,		"TIMESTAMP TZ",
	GCA_TYPE_INTYM,		"INTERVAL Y/M",
	GCA_TYPE_INTDS,		"INTERVAL D/S",
	GCA_TYPE_MNY,		"MONEY",
	GCA_TYPE_DEC,		"DECIMAL",
	GCA_TYPE_BOOL,		"BOOLEAN",
	GCA_TYPE_LKEY,		"LOGICAL KEY",
	GCA_TYPE_TKEY,		"TABLE KEY",
	GCA_TYPE_SECL,		"SECURITY LABEL",

	GCA_TYPE_CHAR_N,	"Nullable CHAR",
	GCA_TYPE_BYTE_N,	"Nullable BYTE",
	GCA_TYPE_NCHR_N,	"Nullable UCS2",
	GCA_TYPE_C_N,		"Nullable 'C'",
	GCA_TYPE_QTXT_N,	"Nullable QUERY TEXT",
	GCA_TYPE_NQTXT_N,	"Nullable UCS2 QUERY TEXT",
	GCA_TYPE_VCHAR_N,	"Nullable VARCHAR",
	GCA_TYPE_VBYTE_N,	"Nullable VARBYTE",
	GCA_TYPE_VNCHR_N,	"Nullable VARUCS2",
	GCA_TYPE_TXT_N,		"Nullable TEXT",
	GCA_TYPE_LTXT_N,	"Nullable LONG TEXT",
	GCA_TYPE_LCHAR_N,	"Nullable LONG CHAR",
	GCA_TYPE_LBYTE_N,	"Nullable LONG BYTE",
	GCA_TYPE_LNCHR_N,	"Nullable LONG UCS2",
	GCA_TYPE_LCLOC_N,	"Nullable LONG CHAR Locator",
	GCA_TYPE_LBLOC_N,	"Nullable LONG BYTE Locator",
	GCA_TYPE_LNLOC_N,	"Nullable LONG UCS2 Locator",
	GCA_TYPE_INT_N,		"Nullable INTEGER",
	GCA_TYPE_FLT_N,		"Nullable FLOAT",
	GCA_TYPE_DATE_N,	"Nullable IDATE",
	GCA_TYPE_ADATE_N,	"Nullable ADATE",
	GCA_TYPE_TIME_N,	"Nullable TIME LCL",
	GCA_TYPE_TMWO_N,	"Nullable TIME W/O",
	GCA_TYPE_TMTZ_N,	"Nullable TIME TZ",
	GCA_TYPE_TS_N,		"Nullable TIMESTAMP LCL",
	GCA_TYPE_TSWO_N,	"Nullable TIMESTAMP W/O",
	GCA_TYPE_TSTZ_N,	"Nullable TIMESTAMP TZ",
	GCA_TYPE_INTYM_N,	"Nullable INTERVAL Y/M",
	GCA_TYPE_INTDS_N,	"Nullable INTERVAL D/S",
	GCA_TYPE_MNY_N,		"Nullable MONEY",
	GCA_TYPE_DEC_N,		"Nullable DECIMAL",
	GCA_TYPE_BOOL_N,	"Nullable BOOLEAN",
	GCA_TYPE_LKEY_N,	"Nullable LOGICAL KEY",
	GCA_TYPE_TKEY_N,	"Nullable TABLE KEY",
	GCA_TYPE_SECL_N,	"Nullable SECURITY LABEL",
	0, 0
} ; 

GLOBALDEF GCXLIST
gcx_gco_type[] = {
	GCO_DT_INT1, "GCO_DT_INT1",
	GCO_DT_INT1_N, "GCO_DT_INT1_N",
	GCO_DT_INT2, "GCO_DT_INT2",
	GCO_DT_INT2_N, "GCO_DT_INT2_N",
	GCO_DT_INT4, "GCO_DT_INT4",
	GCO_DT_INT4_N, "GCO_DT_INT4_N",
	GCO_DT_INT8, "GCO_DT_INT8",
	GCO_DT_INT8_N, "GCO_DT_INT8_N",
	GCO_DT_FLT4, "GCO_DT_FLT4",
	GCO_DT_FLT4_N, "GCO_DT_FLT4_N",
	GCO_DT_FLT8, "GCO_DT_FLT8",
	GCO_DT_FLT8_N, "GCO_DT_FLT8_N",
	GCO_DT_DEC, "GCO_DT_DEC",
	GCO_DT_DEC_N, "GCO_DT_DEC_N",
	GCO_DT_MONEY, "GCO_DT_MONEY",
	GCO_DT_MONEY_N, "GCO_DT_MONEY_N",
	GCO_DT_C, "GCO_DT_C",
	GCO_DT_C_N, "GCO_DT_C_N",
	GCO_DT_CHAR, "GCO_DT_CHAR",
	GCO_DT_CHAR_N, "GCO_DT_CHAR_N",
	GCO_DT_VCHR, "GCO_DT_VCHR",
	GCO_DT_VCHR_N, "GCO_DT_VCHR_N",
	GCO_DT_VCHR_C, "GCO_DT_VCHR_C",
	GCO_DT_VCHR_CN, "GCO_DT_VCHR_CN",
	GCO_DT_LCHR, "GCO_DT_LCHR",
	GCO_DT_LCHR_N, "GCO_DT_LCHR_N",
	GCO_DT_LCLOC, "GCO_DT_LCLOC",
	GCO_DT_LCLOC_N, "GCO_DT_LCLOC_N",
	GCO_DT_BYTE, "GCO_DT_BYTE",
	GCO_DT_BYTE_N, "GCO_DT_BYTE_N",
	GCO_DT_VBYT, "GCO_DT_VBYT",
	GCO_DT_VBYT_N, "GCO_DT_VBYT_N",
	GCO_DT_VBYT_C, "GCO_DT_VBYT_C",
	GCO_DT_VBYT_CN, "GCO_DT_VBYT_CN",
	GCO_DT_LBYT, "GCO_DT_LBYT",
	GCO_DT_LBYT_N, "GCO_DT_LBYT_N",
	GCO_DT_LBLOC, "GCO_DT_LBLOC",
	GCO_DT_LBLOC_N, "GCO_DT_LBLOC_N",
	GCO_DT_TEXT, "GCO_DT_TEXT",
	GCO_DT_TEXT_N, "GCO_DT_TEXT_N",
	GCO_DT_TEXT_C, "GCO_DT_TEXT_C",
	GCO_DT_TEXT_CN, "GCO_DT_TEXT_CN",
	GCO_DT_LTXT, "GCO_DT_LTXT",
	GCO_DT_LTXT_N, "GCO_DT_LTXT_N",
	GCO_DT_LTXT_C, "GCO_DT_LTXT_C",
	GCO_DT_LTXT_CN, "GCO_DT_LTXT_CN",
	GCO_DT_QTXT, "GCO_DT_QTXT",
	GCO_DT_QTXT_N, "GCO_DT_QTXT_N",
	GCO_DT_NCHAR, "GCO_DT_NCHAR",
	GCO_DT_NCHAR_N, "GCO_DT_NCHAR_N",
	GCO_DT_NVCHR, "GCO_DT_NVCHR",
	GCO_DT_NVCHR_N, "GCO_DT_NVCHR_N",
	GCO_DT_NVCHR_C, "GCO_DT_NVCHR_C",
	GCO_DT_NVCHR_CN, "GCO_DT_NVCHR_CN",
	GCO_DT_NLCHR, "GCO_DT_NLCHR",
	GCO_DT_NLCHR_N, "GCO_DT_NLCHR_N",
	GCO_DT_LNLOC, "GCO_DT_LNLOC",
	GCO_DT_LNLOC_N, "GCO_DT_LNLOC_N",
	GCO_DT_NQTXT, "GCO_DT_NQTXT",
	GCO_DT_NQTXT_N, "GCO_DT_NQTXT_N",
	GCO_DT_IDATE, "GCO_DT_IDATE",
	GCO_DT_IDATE_N, "GCO_DT_IDATE_N",
	GCO_DT_ADATE, "GCO_DT_ADATE",
	GCO_DT_ADATE_N, "GCO_DT_ADATE_N",
	GCO_DT_TIME, "GCO_DT_TIME",
	GCO_DT_TIME_N, "GCO_DT_TIME_N",
	GCO_DT_TMWO, "GCO_DT_TMWO",
	GCO_DT_TMWO_N, "GCO_DT_TMWO_N",
	GCO_DT_TMTZ, "GCO_DT_TMTZ",
	GCO_DT_TMTZ_N, "GCO_DT_TMTZ_N",
	GCO_DT_TS, "GCO_DT_TS",
	GCO_DT_TS_N, "GCO_DT_TS_N",
	GCO_DT_TSWO, "GCO_DT_TSWO",
	GCO_DT_TSWO_N, "GCO_DT_TSWO_N",
	GCO_DT_TSTZ, "GCO_DT_TSTZ",
	GCO_DT_TSTZ_N, "GCO_DT_TSTZ_N",
	GCO_DT_INTYM, "GCO_DT_INTYM",
	GCO_DT_INTYM_N, "GCO_DT_INTYM_N",
	GCO_DT_INTDS, "GCO_DT_INTDS",
	GCO_DT_INTDS_N, "GCO_DT_INTDS_N",
	GCO_DT_BOOL, "GCO_DT_BOOL",
	GCO_DT_BOOL_N, "GCO_DT_BOOL_N",
	GCO_DT_LKEY, "GCO_DT_LKEY",
	GCO_DT_LKEY_N, "GCO_DT_LKEY_N",
	GCO_DT_TKEY, "GCO_DT_TKEY",
	GCO_DT_TKEY_N, "GCO_DT_TKEY_N",
	GCO_DT_SECL, "GCO_DT_SECL",
	GCO_DT_SECL_N, "GCO_DT_SECL_N",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gca_elestat[] = {
	GCA_VARZERO, "GCA_VARZERO",
	GCA_NOTARRAY, "GCA_NOTARRAY",
	GCA_VARIMPAR, "GCA_VARIMPAR",
	GCA_VAREXPAR, "GCA_VAREXPAR",
	GCA_VARLSTAR, "GCA_VARLSTAR",
	GCA_VARSEGAR, "GCA_VARSEGAR",
	GCA_VARTPLAR, "GCA_VARTPLAR",
	GCA_VARVCHAR, "GCA_VARVCHAR",
	0, 0
} ;

/* from gcoint.h */

GLOBALDEF GCXLIST
gcx_gcc_eop[] = {
	EOP_DEPLETED, "more message and no more descriptor",
	EOP_NOMSGOD, "no object descriptor for message",
	EOP_NOTPLOD, "no object descriptor for TPL",
	EOP_BADDVOD, "GCA_DATA_VALUE elements incorrect",
	EOP_VARLSTAR, "List elements incorrect",
	EOP_ARR_STAT, "invalid array status for a type",
	EOP_NOT_INT, "invalid TPL for explicit array length",
	0, 0
} ;

/* from gcoint.h */

GLOBALDEF GCXLIST
gcx_gco_ops[] = {
	OP_ERROR, "OP_ERROR",
	OP_DONE, "OP_DONE",
	OP_DOWN, "OP_DOWN",
	OP_UP, "OP_UP",
	OP_DEBUG_TYPE, "OP_DEBUG_TYPE",
	OP_DEBUG_BUFS, "OP_DEBUG_BUFS",
	OP_JZ, "OP_JZ",
	OP_DJNZ, "OP_DJNZ",
	OP_JNINP, "OP_JNINP",
	OP_GOTO, "OP_GOTO",
	OP_CALL_DDT, "OP_CALL_DDT",
	OP_CALL_DV, "OP_CALL_DV",
	OP_CALL_TUP, "OP_CALL_TUP",
	OP_END_TUP, "OP_END_TUP",
	OP_RETURN, "OP_RETURN",
	OP_PUSH_LIT, "OP_PUSH_LIT",
	OP_PUSH_I2, "OP_PUSH_I2",
	OP_PUSH_I4, "OP_PUSH_I4",
	OP_PUSH_REFI2, "OP_PUSH_REFI2",
	OP_UPD_REFI2, "OP_UPD_REFI2",
	OP_PUSH_REFI4, "OP_PUSH_REFI4",
	OP_UPD_REFI4, "OP_UPD_REFI4",
	OP_PUSH_VCH, "OP_PUSH_VCH",
	OP_CHAR_LIMIT, "OP_CHAR_LIMIT",
	OP_ADJUST_PAD, "OP_ADJUST_PAD",
	OP_POP, "OP_POP",
	OP_CV_I1, "OP_CV_I1",
	OP_CV_I2, "OP_CV_I2",
	OP_CV_I4, "OP_CV_I4",
	OP_CV_I8, "OP_CV_I8",
	OP_CV_F4, "OP_CV_F4",
	OP_CV_F8, "OP_CV_F8",
	OP_CV_BYTE, "OP_CV_BYTE",
	OP_CV_CHAR, "OP_CV_CHAR",
	OP_CV_PAD, "OP_CV_PAD",
	OP_PAD, "OP_PAD",
	OP_SKIP_INPUT, "OP_SKIP_INPUT",
	OP_SAVE_CHAR, "OP_SAVE_CHAR",
	OP_RSTR_CHAR, "OP_RSTR_CHAR",
	OP_MSG_BEGIN, "OP_MSG_BEGIN",
	OP_IMP_BEGIN, "OP_IMP_BEGIN",
	OP_COPY_MSG, "OP_COPY_MSG",
	OP_CHAR_MSG, "OP_CHAR_MSG",
	OP_SAVE_MSG, "OP_SAVE_MSG",
	OP_RSTR_MSG, "OP_RSTR_MSG",
	OP_VAR_BEGIN, "OP_VAR_BEGIN",
	OP_VAR_END, "OP_VAR_END",
	OP_DV_BEGIN, "OP_DV_BEGIN",
	OP_DV_END, "OP_DV_END",
	OP_DV_ADJUST, "OP_DV_ADJUST",
	OP_DV_REF, "OP_DV_REF",
	OP_SEG_BEGIN, "OP_SEG_BEGIN",
	OP_SEG_END, "OP_SEG_END",
	OP_SEG_CHECK, "OP_SEG_CHECK",
	OP_MARK, "OP_MARK",
	OP_REWIND, "OP_REWIND",
	OP_PAD_ALIGN, "OP_PAD_ALIGN",
	OP_DIVIDE, "OP_DIVIDE",
	OP_BREAK, "OP_BREAK",
	0, 0
} ;
