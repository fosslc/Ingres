/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include       <compat.h>
# include       <cv.h>
# include       <st.h>
# include       <gl.h>
# include       <sl.h>
# include       <er.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <ft.h>
# include       <fmt.h>
# include       <adf.h>
# include       <frame.h>
# include       <menu.h>
# include       <runtime.h>
# include       "setinq.h"
# include       <fsicnsts.h>
# include       <frserrno.h>
# include       <lqgdata.h>
# include       <rtvars.h>
# include       <compat.h>
# include       <me.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <afe.h>
# include       <fsicnsts.h>
# include       "iigpdef.h"
# include       "erfr.h"

/*
** Name:        runtdata.c
**
** Description: Global data for runtime facility.
**
** History:
**
**      24-sep-96 (hanch04)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/
 
/*
**	fetest.c
*/

GLOBALDEF       char    *FEtstin = NULL;
GLOBALDEF       char    *FEtstout = NULL;

/*
**	fetfile.c
*/

GLOBALDEF bool  multp = FALSE;

/*
**	ii40stiq.c
**
**	defines are duplicated in ii40stiq.c
*/

# define        FSIBUFLEN       63
 
# define        FSI_SET         01
# define        FSI_INQ         02
# define        FSI_FRS         04
# define        FSI_FORM        010
# define        FSI_FIELD       020
# define        FSI_TABLE       040
# define        FSI_COL         0100
# define        FSI_ROW         0200
# define        FSI_RC          0400
# define        FSI_MU          01000
 
# define        FSI_SI          (FSI_SET | FSI_INQ)
# define        FSI_OBJS        (FSI_FIELD | FSI_COL | FSI_ROW | FSI_RC)
# define        FSI_OOBJ        (FSI_FIELD | FSI_COL | FSI_FORM | FSI_TABLE)
# define        FSI_FC          (FSI_FIELD | FSI_COL)
# define        FSI_FFRC        (FSI_FORM | FSI_FIELD | FSI_RC)
# define        FSI_RFF         (FSI_ROW | FSI_FORM | FSI_FIELD)
# define        FSI_HTWID       (FSI_FIELD | FSI_COL | FSI_FRS | FSI_FORM)
# define        FSI_FFC         (FSI_FORM | FSI_FIELD | FSI_COL)
 
 
/*
**  Changed "flags" from i2 to i1; "dtype" split into "sdtype" and "idtype".
**  Fix for BUG 7576. (dkh)
*/
 
typedef struct fsistruct
{
        ER_MSGID        name;
        i2              flags;
        i2              sdtype;
        i2              idtype;
        i2              rtcode;
} FSIINFO;

GLOBALDEF	FSIINFO IIfsiinfo[FsiATTR_MAX] =
{
S_FR0004_TERM,	FSI_INQ | FSI_FRS,	fdSTRING,	fdSTRING,   frsTERM,
S_FR0005_ERRN,	FSI_INQ | FSI_FRS,	fdINT,		fdINT,	    frsERRNO,
S_FR0006_VNXT,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsVALNXT,
S_FR0007_VPRV,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsVALPRV,
S_FR0008_VMKY,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsVMUKY,
S_FR0009_VKYS,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsVKEYS,
S_FR000A_VMNU,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsVMENU,
S_FR000B_MUMP,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsMENUMAP,
S_FR000C_MAP,	FSI_SI | FSI_FRS,	fdINT,		fdSTRING,   frsMAP,
S_FR000D_LABL,	FSI_SI | FSI_FRS,	fdSTRING,	fdSTRING,   frsLABEL,
S_FR000E_MPFL,	FSI_SI | FSI_FRS,	fdSTRING,	fdSTRING,   frsMAPFILE,
S_FR000F_PCST,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsPCSTYLE,
S_FR0010_STLN,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsSTATLN,
S_FR0011_VFRK,	FSI_SI | FSI_FRS,	fdINT,		fdINT,	    frsVFRSKY,
S_FR0012_NORM,	FSI_SI | FSI_OBJS,	fdINT,		fdINT,	    frsNORMAL,
S_FR0013_RVVI,	FSI_SI | FSI_OBJS,	fdINT,		fdINT,	    frsRVVID,
S_FR0014_BLNK,	FSI_SI | FSI_OBJS,	fdINT,		fdINT,	    frsBLINK,
S_FR0015_UNLN,	FSI_SI | FSI_OBJS,	fdINT,		fdINT,	    frsUNLN,
S_FR0016_INTS,	FSI_SI | FSI_OBJS,	fdINT,		fdINT,	    frsINTENS,
S_FR0017_NAME,	FSI_INQ | FSI_OOBJ,	fdSTRING,	fdSTRING,   frsFLDNM,
S_FR0018_CHNG,	FSI_SI | FSI_RFF,	fdINT,		fdINT,	    frsFRMCHG,
S_FR0019_MODE,	FSI_SI | FSI_OOBJ,	fdSTRING,	fdSTRING,   frsFLDMODE,
S_FR001A_FILD,	FSI_INQ | FSI_FORM,	fdSTRING,	fdSTRING,   frsFLD,
S_FR001B_NUMB,	FSI_INQ | FSI_FC,	fdINT,		fdINT,	    frsFLDNO,
S_FR001C_LEN,	FSI_INQ | FSI_FC,	fdINT,		fdINT,	    frsFLDLEN,
S_FR001D_TYPE,	FSI_INQ | FSI_FC,	fdINT,		fdINT,	    frsFLDTYPE,
S_FR001E_FMT,	FSI_SI  | FSI_FC,	fdSTRING,	fdSTRING,   frsFLDFMT,
S_FR001F_VALD,	FSI_INQ | FSI_FC,	fdSTRING,	fdSTRING,   frsFLDVAL,
S_FR0020_TBL,	FSI_INQ | FSI_FIELD,	fdINT,		fdINT,	    frsFLDTBL,
S_FR0021_RWNO,	FSI_INQ | FSI_TABLE,	fdINT,		fdINT,	    frsROWNO,
S_FR0022_MXRW,	FSI_SI  | FSI_TABLE,	fdINT,		fdINT,	    frsROWMAX,
S_FR0023_LROW,	FSI_INQ | FSI_TABLE,	fdINT,		fdINT,	    frsLASTROW,
S_FR0024_DROW,	FSI_INQ | FSI_TABLE,	fdINT,		fdINT,	    frsDATAROWS,
S_FR0025_MCOL,	FSI_INQ | FSI_TABLE,	fdINT,		fdINT,	    frsCOLMAX,
S_FR0026_COLN,	FSI_INQ | FSI_TABLE,	fdSTRING,	fdSTRING,   frsCOLUMN,
S_FR0027_COLR,	FSI_SI	| FSI_OBJS,	fdINT,		fdINT,	    frsCOLOR,
S_FR0028_ANXT,	FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsACTNXT,
S_FR0029_APRV,	FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsACTPRV,
S_FR002A_LCMD,	FSI_INQ | FSI_FRS,	fdINT,		fdINT,	    frsLCMD,
S_FR002B_AMKY,	FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsAMUKY,
S_FR002C_AMUI,	FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsAMUI,
S_FR002D_AKYS,	FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsAKEYS,
S_FR002E_DTYP,	FSI_INQ | FSI_FC,	fdINT,		fdINT,	    frsDATYPE,
S_FR004C_ETXT,	FSI_INQ | FSI_FRS,	fdSTRING,	fdSTRING,   frsERRTXT,
S_FR005A_HGHT,  FSI_INQ | FSI_HTWID,	fdINT,		fdINT,	    frsHEIGHT,
S_FR005B_WIDTH, FSI_INQ | FSI_HTWID,	fdINT,		fdINT,	    frsWIDTH,
S_FR005C_SROW,  FSI_INQ | FSI_RFF,	fdINT,		fdINT,	    frsSROW,
S_FR005D_SCOL,  FSI_INQ | FSI_RFF,	fdINT,		fdINT,	    frsSCOL,
S_FR005E_CROW,  FSI_INQ | FSI_FRS,	fdINT,		fdINT,	    frsCROW,
S_FR005F_CCOL,  FSI_INQ | FSI_FRS,	fdINT,		fdINT,	    frsCCOL,
S_FR0060_TMOUT, FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsTMOUT,
S_FR0061_EACT,  FSI_SI  | FSI_FRS,	fdINT,		fdINT,	    frsENTACT,
S_FR0062_DROW,  0,			fdINT,		fdINT,	    -1,
S_FR0159_RDONLY,FSI_SI	| FSI_FC,	fdINT,		fdINT,	    frsRDONLY,
S_FR015A_INVIS,	FSI_SI	| FSI_FC,	fdINT,		fdINT,	    frsINVIS,
S_FR015B_SHELL, FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsSHELL,
S_FR015F_LFRSK, FSI_INQ | FSI_FRS,	fdINT,		fdINT,	    frsLFRSK,
S_FR0160_EDIT,  FSI_SI  | FSI_FRS,	fdINT,		fdINT,	    frsEDITOR,
S_FR0161_DRVED,	FSI_INQ | FSI_FC,	fdINT,		fdINT,	    frsDRVED,
S_FR0162_DRVST,	FSI_INQ | FSI_FC,	fdSTRING,	fdSTRING,   frsDRVSTR,
S_FR016C_DPREC,	FSI_INQ	| FSI_FC,	fdINT,		fdINT,	    frsDPREC,
S_FR016D_DSCL,	FSI_INQ	| FSI_FC,	fdINT,		fdINT,	    frsDSCL,
S_FR016E_DSZE,	FSI_INQ	| FSI_FC,	fdINT,		fdINT,	    frsDSZE,
S_FR016F_GMSGS,	FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsGMSGS,
S_FR0170_KEYFND,FSI_SI	| FSI_FRS,	fdINT,		fdINT,	    frsKEYFIND,
S_FR0171_EXISTS,FSI_INQ | FSI_FFC,	fdINT,		fdINT,	    frsEXISTS,
S_FR0172_MURNME,FSI_SET | FSI_MU,	fdSTRING,	fdSTRING,   frsMURNME,
S_FR0173_MUONOF,FSI_SI  | FSI_MU,	fdINT,		fdINT,	    frsMUONOF,
S_FR0174_ODMSG, FSI_SI  | FSI_FRS,	fdINT,		fdINT,	    frsODMSG,
S_FR0175_EMASK,	FSI_SI  | FSI_FC,	fdINT,		fdINT,	    frsEMASK
};

GLOBALDEF	i4	IIsiobj = 0;
GLOBALDEF	RUNFRM	*IIsirfrm = NULL;
GLOBALDEF	FRAME	*IIsifrm = NULL;
GLOBALDEF	TBLFLD	*IIsitbl = NULL;
GLOBALDEF	i4	IIsirow = 0;
GLOBALDEF	TBSTRUCT	*IIsitbs = NULL;

/*
** IIsifld is set for tablefield operations only, and is the FIELD
** structure to use in calculating display area sizes
*/
GLOBALDEF	FIELD	*IIsifld = NULL;

/*
**	iiendfrm.c
*/

GLOBALDEF       bool    IIclrqbf = TRUE;
GLOBALDEF       bool    IIFRpfwp = FALSE;
GLOBALDEF       char    IIFRpfnPrevFormName[FE_MAXNAME + 1] = ERx("");

/*
**	iigp.c
*/

GLOBALDEF GP_PARM **IIFR_gparm;         /* parameter index array */
 
GLOBALDEF i4  IIFR_maxpid = -1;         /* Maximum id, for error checks */

/*
**	iimapfle.c
*/

GLOBALDEF       char    *IIsimapfile = ERx("");

/*
**	iimenu.c
*/

GLOBALDEF i4    form_mu = TRUE;         /* true if menu on a form       */
/* Stores the timeout return value for the current ##submenu.  */
GLOBALDEF       i4      IIFRtovTmOutVal = 0;  
GLOBALDEF       bool    IIexted_submu = FALSE;
 
/*
**	iiqbfutl.c
*/

GLOBALDEF       FRAME   *IIqbffrm = NULL;

/*
**      rtsisys.c
*/

GLOBALDEF RTATTLIST     IIattfrs[] =
{
        ERx("errorno"), FALSE,  DB_INT_TYPE,    frsERRNO,       RTinqsys,
        ERx("error"),   FALSE,  DB_INT_TYPE,    frsERRNO,       RTinqsys,
        ERx("terminal"),FALSE,  DB_CHR_TYPE,    frsTERM,        RTinqsys,
        NULL,           FALSE,  0,      0,              NULL
};

/*
**	rtsifrm.c
*/

GLOBALDEF RTATTLIST     IIattform[] =
{
        ERx("change"),  FALSE,  DB_INT_TYPE,    frsFRMCHG,      RTinqfrm,
        ERx("mode"),    FALSE,  DB_CHR_TYPE,    frsFLDMODE,     RTinqfrm,
        ERx("field"),   FALSE,  DB_CHR_TYPE,    frsFLD,         RTinqfrm,
        ERx("name"),    FALSE,  DB_CHR_TYPE,    frsFLDNM,       RTinqfrm,
        ERx("change"),  TRUE,   DB_INT_TYPE,    frsFRMCHG,      RTsetfrm,
        ERx("mode"),    TRUE,   DB_CHR_TYPE,    frsFLDMODE,     RTsetfrm,
        NULL,           FALSE,  0,      0,              NULL
};

/*
**	rtsitbl.c
*/

GLOBALDEF RTATTLIST     IIatttbl[] =
{
        /* in order of frequency */
        ERx("name"),    FALSE,  DB_CHR_TYPE,    frsTABLE,       RTinqtbl,
        ERx("rowno"),   FALSE,  DB_INT_TYPE,    frsROWNO,       RTinqtbl,
        ERx("column"),  FALSE,  DB_CHR_TYPE,    frsCOLUMN,      RTinqtbl,
        ERx("lastrow"), FALSE,  DB_INT_TYPE,    frsLASTROW,     RTinqtbl,
        ERx("mode"),    FALSE,  DB_CHR_TYPE,    frsFLDMODE,     RTinqtbl,
        ERx("datarows"),FALSE,  DB_INT_TYPE,    frsDATAROWS,    RTinqtbl,
        ERx("maxrow"),  FALSE,  DB_INT_TYPE,    frsROWMAX,      RTinqtbl,
        ERx("maxcol"),  FALSE,  DB_INT_TYPE,    frsCOLMAX,      RTinqtbl,
        NULL,           FALSE,  0,      0,              NULL
};

/*
**	rtsifld.c
*/

GLOBALDEF RTATTLIST     IIattfld[] =
{
        ERx("name"),    FALSE,  DB_CHR_TYPE,    frsFLDNM,       RTinqfld,
        ERx("number"),  FALSE,  DB_INT_TYPE,    frsFLDNO,       RTinqfld,
        ERx("length"),  FALSE,  DB_INT_TYPE,    frsFLDLEN,      RTinqfld,
        ERx("type"),    FALSE,  DB_INT_TYPE,    frsFLDTYPE,     RTinqfld,
        ERx("format"),  FALSE,  DB_CHR_TYPE,    frsFLDFMT,      RTinqfld,
        ERx("valid"),   FALSE,  DB_CHR_TYPE,    frsFLDVAL,      RTinqfld,
        ERx("table"),   FALSE,  DB_INT_TYPE,    frsFLDTBL,      RTinqfld,
        ERx("mode"),    FALSE,  DB_CHR_TYPE,    frsFLDMODE,     RTinqfld,
        ERx("mode"),    TRUE,   DB_CHR_TYPE,    frsFLDMODE,     RTsetfld,
        NULL,           FALSE,  0,      0,              NULL
};

/*
**	rtsicol.c
*/

GLOBALDEF RTATTLIST     IIattcol[] =
{
        ERx("name"),    FALSE,  DB_CHR_TYPE,    frsFLDNM,       RTinqcol,
        ERx("number"),  FALSE,  DB_INT_TYPE,    frsFLDNO,       RTinqcol,
        ERx("length"),  FALSE,  DB_INT_TYPE,    frsFLDLEN,      RTinqcol,
        ERx("type"),    FALSE,  DB_INT_TYPE,    frsFLDTYPE,     RTinqcol,
        ERx("format"),  FALSE,  DB_CHR_TYPE,    frsFLDFMT,      RTinqcol,
        ERx("valid"),   FALSE,  DB_CHR_TYPE,    frsFLDVAL,      RTinqcol,
        ERx("mode"),    FALSE,  DB_CHR_TYPE,    frsFLDMODE,     RTinqcol,
        ERx("mode"),    TRUE,   DB_CHR_TYPE,    frsFLDMODE,     RTsetcol,
        NULL,           FALSE,  0,      0,              NULL
};

/*
**	iisetinq.c
*/

# define        fdsiFRAME       1
# define        fdsiFIELD       2
# define        fdsiTABLE       3
# define        fdsiCOL         4
# define        fdsiFRS         5

GLOBALDEF RTOBJLIST     IIfrsobjs[] =
{
/*      OBJECT          PTRTYPE     ATTRIBUTE TABLE     FILE    */
        ERx("frs"),     fdsiFRS,        IIattfrs,    /* RTSISYS.c */
        ERx("form"),    fdsiFRAME,      IIattform,    /* RTSIFRM.c */
        ERx("frame"),   fdsiFRAME,      IIattform,    /* RTSIFRM.c */
        ERx("table"),   fdsiTABLE,      IIatttbl,    /* RTSITBL.c */ 
        ERx("field"),   fdsiFIELD,      IIattfld,    /* RTSIFLD.c */ 
        ERx("column"),  fdsiCOL,        IIattcol,    /* RTSICOL.c */ 
        NULL,           0,              NULL,
};
  
/*
**      IIinqset set this pointer to the appropriate object
*/
GLOBALDEF RTOBJLIST     *IIcurobj = NULL;


GLOBALDEF i4      IIUFlfiLocateFormInit ZERO_FILL;
