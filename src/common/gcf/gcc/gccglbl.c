/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <qu.h>
#include    <tm.h>
#include    <gcccl.h>
#include    <gca.h>
#include    <gcc.h>
#include    <gcxdebug.h>

/**
**
**  Name: GCCGLBL.C - Global storage for GCC
**
**  Description:
**      This file declares all global storage required by GCC.
**
**  History:
**      07-Jan-1988 (jbowers)
**          Initial module creation.
**	13-Jun-1989 (jorge)
**	    Added include of descrip.h
**      16-Nov-89 (cmorris)
**          Cleaned out the junk.
**  	13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**      14-jul-93 (ed)
**          unnested dbms.h
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	20-Aug-97 (gordy)
**	    Added globals for bridge.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Moved GCX tracing definitions from gcudata.c to this file as
**          the references are seen only in GCC.
**	 4-Nov-05 (gordy)
**	    Added state SARAF and replaced input IAACEF with IAAAF.
**/

/*
**
**
*/

/*
** Pointer to GCC global data structure
*/

GLOBALDEF  GCC_GLOBAL        *IIGCc_global ZERO_FILL;

/*
** Protocol Bridge global data.
*/

GLOBALDEF N_ADDR    gcb_from_addr = {"", "", ""};
GLOBALDEF N_ADDR    gcb_to_addr = {"", "", ""};
GLOBALDEF GCC_BPMR  gcb_pm_reso = {0, NULL, NULL, NULL, FALSE};

/* from gcc.h */
GLOBALDEF GCXLIST
gcx_mde_service[] = {
	A_ASSOCIATE, "A_ASSOCIATE",
	A_CONNECTED, "A_CONNECTED",
	A_ABORT, "A_ABORT",
	A_COMPLETE, "A_COMPLETE",
	A_DATA, "A_DATA",
	A_EXP_DATA, "A_EXP_DATA",
	A_RELEASE, "A_RELEASE",
	A_LISTEN, "A_LISTEN",
	A_LSNCLNUP, "A_LSNCLNUP",
	A_PURGED, "A_PURGED",
	A_SHUT, "A_SHUT",
	A_QUIESCE, "A_QUIESCE",
	A_P_ABORT, "A_P_ABORT",
        A_I_REJECT, "A_I_REJECT",
        A_FINISH, "A_FINISH",
	A_R_REJECT, "A_R_REJECT",
	P_CONNECT, "P_CONNECT",
	P_RELEASE, "P_RELEASE",
	P_U_ABORT, "P_U_ABORT",
	P_P_ABORT, "P_P_ABORT",
	P_DATA, "P_DATA",
	P_EXP_DATA, "P_EXP_DATA",
	P_XOFF, "P_XOFF",
	P_XON, "P_XON",
	S_CONNECT, "S_CONNECT",
	S_RELEASE, "S_RELEASE",
	S_U_ABORT, "S_U_ABORT",
	S_P_ABORT, "S_P_ABORT",
	S_DATA, "S_DATA",
	S_EXP_DATA, "S_EXP_DATA",
	S_XOFF, "S_XOFF",
	S_XON, "S_XON",
	T_CONNECT, "T_CONNECT",
	T_DISCONNECT, "T_DISCONNECT",
	T_DATA, "T_DATA",
	T_OPEN, "T_OPEN",
	T_XOFF, "T_XOFF",
	T_XON, "T_XON",
	T_P_ABORT, "T_P_ABORT",
	N_CONNECT, "N_CONNECT",
	N_DISCONNECT, "N_DISCONNECT",
	N_DATA, "N_DATA",
	N_OPEN, "N_OPEN",
	N_RESET, "N_RESET",
	N_LSNCLUP, "N_LSNCLUP",
	N_ABORT, "N_ABORT",
	N_XOFF, "N_XOFF",
	N_XON, "N_XON",
	N_P_ABORT, "N_P_ABORT",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_mde_primitive[] = {
        RQS,    "RQS",
        IND,    "IND",
        RSP,    "RSP",
        CNF,    "CNF",
        0, 0
} ;       

/* from gccal.h */

GLOBALDEF GCXLIST
gcx_gcc_sal[] = {
	0,	"SACLD",
	1,	"SAIAL",
	2,	"SARAL",
	3,	"SADTI",
	4,	"SADSP",
	5,	"SACTA",
	6,	"SACTP",
	7,	"SASHT",
	8,	"SADTX",
	9,	"SADSX",
	10,	"SARJA",
	11,	"SARAA",
        12,     "SAQSC",
        13,     "SACLG",
	14,	"SARAB",
	15,	"SACAR",
    	16, 	"SAFSL",
	17,	"SARAF",
	18,	"SACMD",
	19,	"SAMAX",
	0, 0
} ;

/* from gccpb.h */

GLOBALDEF GCXLIST
gcx_gcb_sal[] = {
       0,      "SBACLD",
       1,      "SBASHT",
       2,      "SBAQSC",
       3,      "SBACLG",
       4,      "SBARJA",
       5,      "SBAMAX",
       0, 0
 } ;


GLOBALDEF GCXLIST
gcx_gcc_ial[] = {
	0,	"IALSN",
	1,	"IAAAQ",
	2,	"IAABQ",
	3,	"IAASA",
	4,	"IAASJ",
	5,	"IAPCI",
	6,	"IAPCA",
	7,	"IAPCJ",
	8,	"IAPAU",
	9,	"IAPAP",
	10,	"IAANQ",
	11,	"IAAEQ",
	12,	"IAACE",
	13,	"IAACQ",
	14,	"IAPDN",
	15,	"IAPDE",
	16,	"IAARQ",
	17,	"IAPRII",
	18,	"IAPRA",
	19,	"IAPRJ",
	20,	"IALSF",
	21,	"IAREJI",
	22,	"IAAPG",
	23,	"IAPDNH",
	24,	"IASHT",
	25,	"IAQSC",
	26,	"IAABT",
	27,	"IAXOF",
	28,	"IAXON",
	29,	"IAANQX",
	30,	"IAPDNQ",
	31,	"IAACEX",
	32,	"IAAPGX",
	33,	"IAABQX",
	34,	"IAABQR",
        35,     "IAAFN",
	36,	"IAREJR",
	37,	"IAPRIR",
	38,	"IAAAF",
    	39, 	"IAPAM",
	40,	"IACMD",
	41,	"IAMAX",
	0, 0
};

/* from gccpb.h */

GLOBALDEF GCXLIST
gcx_gcb_ial[] = {
       0,      "IBALSN",
       1,      "IBAABQ",
       2,      "IBAACE",
       3,      "IBAARQ",
       4,      "IBASHT",
       5,      "IBAQSC",
       6,      "IBAABT",
       7,      "IBAAFN",
       8,      "IBALSF",
       9,      "IBAREJI",
       10,     "IBAMAX",
       0, 0
};

GLOBALDEF GCXLIST
gcx_gcc_oal[] = {
	1,	"OAPCQ",
	2,	"OAPCA",
	3,	"OAPCJ",
	4,	"OAPAU",
	5,	"OAPDQ",
	6,	"OAPEQ",
	7,	"OAPRQ",
	8,	"OAPRS",
	9,	"OAXOF",
	10,	"OAXON",
        0, 0
	};


GLOBALDEF GCXLIST
gcx_gcc_aal[] = {
	1,	"AARQS",
	2,	"AARVN",
	3,	"AARVE",
	4,	"AADIS",
	5,	"AALSN",
	6,	"AASND",
	7,	"AARLS",
	8,	"AAENQ",
	9, 	"AADEQ",
	10,	"AAPGQ",
	11,	"AAMDE",
	12,	"AACCB",
	13,	"AASNDG",
	14,	"AASHT",
	15,	"AAQSC",
	16,	"AASXF",
	17,	"AARXF",
        18,     "AARSP",
        19,     "AAENQG",
    	20, 	"AAFSL",
	21,	"AACMD",
	0, 0
} ;

/* from gccpl.h */

GLOBALDEF GCXLIST
gcx_gcc_spl[] = {
	0, "SPCLD",
	1, "SPIPL",
	2, "SPRPL",
	3, "SPOPN",
	4, "SPCLG",
	5, "SPMAX",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_ipl[] = {
	0, "IPPCQ",
	1, "IPPCA",
	2, "IPPCJ",
	3, "IPPAU",
	4, "IPPDQ",
	5, "IPPEQ",
	6, "IPPRQ",
	7, "IPPRS",
	8, "IPSCI",
	9, "IPSCA",
	10, "IPSCJ",
	11, "IPSAP",
	12, "IPARU",
	13, "IPARP",
	14, "IPSDI",
	15, "IPSEI",
	16, "IPSRI",
	17, "IPSRA",
	18, "IPSRJ",
	19, "IPPDQH",
	20, "IPPRSC",
	21, "IPSRAC",
	22, "IPPEQH",
	23, "IPSDIH",
	25, "IPSEIH",
	26, "IPABT",
	27, "IPPXF",
	28, "IPPXN",
	29, "IPSXF",
	30, "IPSXN",
	31, "IPMAX",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_opl[] = {
	1, "OPPCI",
	2, "OPPCA",
	3, "OPPCJ",
	4, "OPPAU",
	5, "OPPAP",
	6, "OPPDI",
	7, "OPPEI",
	8, "OPPRI",
	9, "OPPRC",
	10, "OPSCQ",
	11, "OPSCA",
	12, "OPSCJ",
	13, "OPARU",
	14, "OPARP",
	15, "OPSDQ",
	16, "OPSEQ",
	17, "OPSRQ",
	18, "OPSRS",
	19, "OPSCR",
	20, "OPRCR",
	21, "OPMDE",
	22, "OPPDIH",
	23, "OPPEIH",
	24, "OPSDQH",
	25, "OPSEQH",
	26, "OPPXF",
	27, "OPPXN",
	28, "OPSXF",
	29, "OPSXN",
    	30, "OPTOD",
	0, 0
} ;

/* from gccsl.h */

GLOBALDEF GCXLIST
gcx_gcc_ssl[] = {
	0,	"SSCLD",
	1,	"SSISL",
	2,	"SSRSL",
	3,	"SSWAC",
	4,	"SSWCR",
	5,	"SSOPN",
	6,	"SSCDN",
	7,	"SSCRR",
	8,	"SSCTD",
	9,	"SSMAX",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_isl[] = {
	0,	"ISSCQ",
	1,	"ISTCI",
	2,	"ISTCC",
	3,	"ISSCA",
	4,	"ISSCJ",
	5,	"ISCN",
	6,	"ISAC",
	7,	"ISRF",
	8,	"ISSDQ",
	9,	"ISSEQ",
	10,	"ISDT",
	11,	"ISEX",
	12,	"ISSAU",
	13,	"ISAB",
	14,	"ISSRQ",
	15,	"ISSRS",
	16,	"ISFN",
	17,	"ISDN",
	18,	"ISTDI",
	19,	"ISNF",
	20,	"ISABT",
	21,	"ISSXF",
	22,	"ISSXN",
	23,	"ISTXF",
	24,	"ISTXN",
	25,	"ISDNCR",
	26,	"ISRSCI",
	27,	"ISMAX",
	0, 0
};

GLOBALDEF GCXLIST
gcx_gcc_osl[] = {
	1,	"OSSCI",
	2,	"OSSCA",
	3,	"OSSCJ",
	4,	"OSSAX",
	5,	"OSSAP",
	6,	"OSSDI",
	7,	"OSSEI",
	8,	"OSSRI",
	9,	"OSRCA",
	10,	"OSRCJ",
	11,	"OSTCQ",
	12,	"OSTCS",
	13,	"OSTDC",
	14,	"OSCN",
	15,	"OSAC",
	16,	"OSRF",
	17,	"OSDT",
	18,	"OSEX",
	19,	"OSAB",
	20,	"OSFN",
	21,	"OSDN",
	22,	"OSNF",
	23,	"OSSXF",
	24,	"OSSXN",
	25,	"OSTXF",
	26,	"OSTXN",
	27,	"OSCOL",
	28,	"OSDNR",
	29,	"OSMDE",
	30,	"OSPGQ",
    	31, 	"OSTFX",
	0, 0
} ;

/* from gcctl.h */

GLOBALDEF GCXLIST
gcx_gcc_stl[] = {
	0,	"STCLD",
	1,	"STWNC",
	2,	"STWCC",
	3,      "STWCR",
	4,	"STWRS",
	5,	"STOPN",
	6,	"STWDC",
	7,	"STSDP",
	8,	"STOPX",
	9,	"STSDX",
    	10,  	"STWND",
	11,	"STCLG",
	12,	"STMAX",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_itl[] = {
	0,	"ITOPN",
	1,	"ITNCI",
	2,	"ITNCC",
	3,	"ITTCQ",	
	4,	"ITCR",
	5,	"ITCC",
	6,	"ITTCS",
	7,	"ITTDR",
	8, 	"ITNDI",
	9, 	"ITNDC",
	10,	"ITTDA",
	11,	"ITDR",
	12,	"ITDC",
	13,	"ITDT",
	14,	"ITNC",
	15,	"ITNCQ",
	16,	"ITNOC",
	17,	"ITABT",
	18,	"ITNRS",
	19,	"ITXOF",
	20,	"ITXON",
	21,	"ITDTX",
	22,	"ITTDAQ",
	23,	"ITNCX",
	24,	"ITNLSF",	
    	25,	"ITMAX",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_otl[] = {
	1,	"OTTCI",
	2,	"OTTCC",
	3,	"OTTDR",
	4,	"OTTDI",
	5,	"OTTDA",
	6,	"OTCR",
	7,	"OTCC",
	8,	"OTDR",
	9,	"OTDC",
	10,	"OTDT",
	11,	"OTXOF",
	12,	"OTXON",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_atl[] = {
	1,	"ATCNC",
	2,	"ATDCN",
	3,	"ATMDE",
	4,	"ATCCB",
	5,	"ATLSN",
	6,	"ATRVN",
	7,	"ATSDN",
	8,	"ATOPN",
	9,	"ATSNQ",
	10,	"ATSDQ",
	11,	"ATPGQ",
	12,	"ATSXF",
	13,	"ATRXF",
	14,	"ATSDP",
	0, 0
} ;

/* from gccpb.h */

GLOBALDEF GCXLIST
gcx_gcc_spb[] = {
	0,	"SBCLD",
	1,	"SBWNC",
	2,	"SBOPN",
	3,	"SBSDP",
	4,	"SBOPX",
	5,	"SBSDX",
	6,	"SBCLG",
	7,	"SBMAX",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_ipb[] = {
	0,	"IBOPN",
	1,	"IBNOC",
	2,	"IBNRS",
	3,	"IBNCI",
	4,	"IBNCQ",
	5,	"IBNCC",
	6,	"IBNDR",
	7,	"IBNDI",
	8,	"IBNDC",
	9,	"IBSDN",
	10,	"IBDNI",
	11,	"IBDNC",
	12,	"IBDNQ",
	13,	"IBABT",
	14,	"IBXOF",
	15,	"IBXON",
	16,	"IBRDX",
	17,	"IBSDNQ",
	18,	"IBDNX",
	19,	"IBNCCQ",
    	20,     "IBNLSF",
    	21, 	"IBNCIP",
	22,	"IBMAX",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_opb[] = {
	1,	"OBNCQ",
	2,	"OBNDR",
	3,	"OBSDN",
	4,	"OBXOF",
	5,	"OBXON",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gcc_apb[] = {
	1,	"ABCNC",
	2,	"ABDCN",
	3,	"ABMDE",
	4,	"ABCCB",
	5,	"ABLSN",
	6,	"ABRVN",
	7,	"ABRVE",
	8,	"ABSDN",
	9,	"ABNCQ",
	10,	"ABOPN",
	11,	"ABSNQ",
	12,	"ABSDQ",
	13,	"ABSXF",
	14,	"ABRXF",
	15,     "ABNDR",
        16,     "ABALSN",
	17,     "ABAMDE",
	18,     "ABACCB",
	19,     "ABADIS",
	20,     "ABARSP",
	21,     "ABASHT",
	22,     "ABAQSC",
	0, 0
} ;

/* from gcccl.h */

GLOBALDEF GCXLIST
gcx_gcc_rq[] = {
	GCC_OPEN,	"GCC_OPEN",
	GCC_LISTEN,	"GCC_LISTEN",
        GCC_CONNECT,    "GCC_CONNECT",
	GCC_SEND,	"GCC_SEND",
	GCC_RECEIVE,	"GCC_RECEIVE",
	GCC_DISCONNECT,	"GCC_DISCONNECT",
	0, 0
} ;

GLOBALDEF GCXLIST
gcx_gca_rq[] = {
        0,      "GCC_A_LISTEN",
        1,      "GCC_A_REQUEST",
        2,      "GCC_A_RESPONSE",
        3,      "GCC_A_SEND",
        4,      "GCC_A_E_SEND",
        5,      "GCC_A_NRM_RCV",
        6,      "GCC_A_EXP_RCV",
        7,      "GCC_A_DISASSOC",
        8,      "GCC_N_GCA_RQ",
        0, 0
} ;
