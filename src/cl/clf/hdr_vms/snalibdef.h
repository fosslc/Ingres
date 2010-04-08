/********************************************************************************************************************************/
/* Created 11-DEC-1987 16:58:14 by VAX-11 SDL V3.0-2      Source: 22-OCT-1987 14:36:52 SNA$ENVIR:[AI.AI.SRC]SNALIBDEF.SDL;1 */
/********************************************************************************************************************************/
 
/*** MODULE SNALIBDF IDENT V01.10 ***/
/* SNA Library Definitions                                                  */
/*                                                                          */
/***************************************************************************** */
/**									     * */
/**  Copyright (C) 1982, 1983, 1984, 1985, 1986, 1987			     * */
/**  by Digital Equipment Corporation, Maynard, Mass.			     * */
/** 									     * */
/**  This software is furnished under a license and may be used and  copied  * */
/**  only  in  accordance  with  the  terms  of  such  license and with the  * */
/**  inclusion of the above copyright notice.  This software or  any  other  * */
/**  copies  thereof may not be provided or otherwise made available to any  * */
/**  other person.  No title to and ownership of  the  software  is  hereby  * */
/**  transferred.							     * */
/** 									     * */
/**  The information in this software is subject to change  without  notice  * */
/**  and  should  not  be  construed  as  a commitment by digital equipment  * */
/**  corporation.							     * */
/** 									     * */
/**  Digital assumes no responsibility for the use or  reliability  of  its  * */
/**  software on equipment which is not supplied by digital.		     * */
/**									     * */
/***************************************************************************** */
/**++                                                                       */
/** FACILITY:	SNA Gateway Access Routines                                 */
/**                                                                         */
/** ABSTRACT:	SNA Library Definitions                                     */
/**                                                                         */
/** ENVIRONMENT:                                                            */
/**                                                                         */
/**	VAX/VMS Operating System                                            */
/**                                                                         */
/** AUTHOR:  Distributed Systems Software Engineering                       */
/**                                                                         */
/** CREATION DATE:  14-July-1982                                            */
/**                                                                         */
/** Modified by:                                                            */
/**                                                                         */
/** 1.01  19-Oct-1983  Dave Garrod                                          */
/**	  Add ASYNCEFN and NOTPAR parameters to the CONNECT and LISTEN      */
/**	  functions.                                                        */
/**                                                                         */
/** 1.02  21-Mar-1984  Dave Garrod                                          */
/**	  Define fields in BIND that are used by LU6.2. Add Conditional     */
/**	  End Bracket to RH definitions.                                    */
/**                                                                         */
/** 1.03  23-Aug-1984  Dave Garrod                                          */
/**	  Define crytography fields in BIND.                                */
/**                                                                         */
/** 1.04  18-Sep-1984  Dave Garrod                                          */
/**	  Add SNAEVT$K_PLURESET from EAI component.                         */
/**                                                                         */
/** 1.05  16-Nov-1984  Dave Garrod                                          */
/**	  Add routine SNA$REPORT_ERROR.                                     */
/**                                                                         */
/** 1.06  22-May-1985  Dave Garrod                                          */
/**	  Add support for VMS/SNA.                                          */
/**                                                                         */
/** 1.07  20-Aug-1985  Jeff Waltz                                           */
/**	  Removed routine SNA$REPORT_ERROR which                            */
/**	  which is no longer needed.                                        */
/**                                                                         */
/** 1.08  09-May-1986  Dave Garrod                                          */
/**	  Add definition for routines SNA$LINK_ACCEPT_W and SNA$LINK_ACCEPT. */
/**	  Add LUPASSWORD and PID parameters to SNA$CONNECT[_W] and          */
/**	  SNA$LISTEN[_W].                                                   */
/**                                                                         */
/** 1.09  24-Sep-1986  Jonathan D. Belanger                                 */
/**	  Changed bytes 23 and 25 in the structure SNABND to reflect similar */
/**	  IBM changes.                                                      */
/**                                                                         */
/** 1.10  03-Aug-1987  Jonathan D. Belanger                                 */
/**	  Add the constants SNABUF$K_SSCP_LU_BUFSIZ and SNABUF$K_LU_LU_BUFSIZ */
/**	  to be used by SNA$$GETRECBUF to figure out which size of buffer to */
/**	  allocate.                                                         */
/**                                                                         */
/**      HISTORY:                                                           */
/**      25-sep-95 (whitfield)                                              */
/**              Turn off member_alignment.                                 */
/*      19-jul-2007 (stegr01)                                               */
/*          use compiler supplied macro __alpha and __ia64 instead of ALPHA */
/**--                                                                       */

#if (defined(__alpha) || defined(__ia64))
#pragma member_alignment __save
#pragma nomember_alignment
#endif /* ALPHA */

/*                                                                          */
/* Access Routine Events                                                    */
/*                                                                          */
#define SNAEVT$K_MIN 1
#define SNAEVT$K_RCVEXP 1               /* Data received on expedited flow  */
#define SNAEVT$K_UNBHLD 2               /* UNBIND HOLD event                */
#define SNAEVT$K_TERM 3                 /* Session terminated by IBM host or gateway */
#define SNAEVT$K_COMERR 4               /* Gateway Communication Error      */
#define SNAEVT$K_PLURESET 5             /* Half session state machines reset */
#define SNAEVT$K_MAX 5
struct SNAEVT {
    unsigned long int SNAEVT$L_DUMMY;   /* Dummy entry to keep SDL happy    */
/*                                                                          */
/* The Basic AI returns RCVEXP, UNBHLD, TERM, and COMERR. The EAI returns   */
/* these plus PLURESET                                                      */
/*                                                                          */
    } ;
/*                                                                          */
/* Gateway Access Routine Buffer Definitions                                */
/*                                                                          */
#define SNABUF$K_FHDLEN 2               /* Function header length           */
#define SNABUF$K_HDLEN 7                /* Standard buffer header size      */
#define SNABUF$K_LENGTH 263             /* Buffer Length                    */
#define SNABUF$K_SSCP_LENGTH 2055       /* SSCP buffer length (2048 + 7)    */
#define SNABUF$K_SSCP_LU_BUFSIZ 1       /* Allocate an SSCP-LU buffer       */
#define SNABUF$K_LU_LU_BUFSIZ 2         /* Allocate an LU-LU buffer         */
struct SNABUF {
    unsigned char SNABUF$B_FCT;         /* Function code                    */
    unsigned char SNABUF$B_FCTMOD;      /* Function modifier                */
    unsigned short int SNABUF$W_SEQ;    /* Sequence number                  */
    char SNABUF$T_RH [3];
    char SNABUF$T_DATA [256];           /* Data area                        */
    } ;
/*                                                                          */
/* Gateway Access Routine Termination Buffer Definitions                    */
/*                                                                          */
#define SNATRM$K_LENGTH 11              /* Length of termination buffer     */
struct SNATRM {
    unsigned long int SNATRM$L_REASON;  /* VMS reason code                  */
    unsigned short int SNATRM$W_SENSE0; /* Sense code word 0                */
    unsigned short int SNATRM$W_SENSE1; /* Sense code word 1                */
    unsigned char SNATRM$B_UNBCOD;      /* Unbind code                      */
    unsigned char SNATRM$B_GWYCOD;      /* Gateway abort code               */
    unsigned char SNATRM$B_GWYQUA;      /* Gateway abort code qualifier     */
    } ;
/*                                                                          */
/* Gateway Access Routine IOSB definitions                                  */
/*                                                                          */
#define SNAIOS$K_LENGTH 8               /* Length of IOSB                   */
struct SNAIOS {
    unsigned long int SNAIOS$L_STATUS;  /* Status field                     */
    unsigned long int SNAIOS$L_QUAL;    /* Qualifier field                  */
    } ;
/*                                                                          */
/* RH Definitions :-                                                        */
/* The definitions are in reverse order to IBM because of                   */
/* the differing bit labeling conventions between DEC and IBM               */
/*                                                                          */
#define SNARH$K_LENGTH 3                /* Length of RH                     */
#define SNARH$M_ECI 1
#define SNARH$M_BCI 2
#define SNARH$M_SDI 4
#define SNARH$M_FI 8
#define SNARH$M_RE0 16
#define SNARH$M_RUC 96
#define SNARH$M_RRI 128
#define SNARH$M_PI 256
#define SNARH$M_QRI 512
#define SNARH$M_RE1 3072
#define SNARH$M_ERI 4096
#define SNARH$M_DR2I 8192
#define SNARH$M_RE2 16384
#define SNARH$M_DR1I 32768
#define SNARH$M_CEB 65536
#define SNARH$M_PDI 131072
#define SNARH$M_EDI 262144
#define SNARH$M_CSI 524288
#define SNARH$M_RE4 1048576
#define SNARH$M_CDI 2097152
#define SNARH$M_EBI 4194304
#define SNARH$M_BBI 8388608
#define SNARH$M_RE5 255
#define SNARH$M_RE6 3840
#define SNARH$M_RTI 4096
#define SNARH$M_RE8 16711680
#define SNARH$K_RRI_RQ 0                /* Request                          */
#define SNARH$K_RRI_RSP 1               /* Response                         */
/*                                                                          */
#define SNARH$K_RUC_MIN 0
#define SNARH$K_RUC_FMD 0               /* Function management data         */
#define SNARH$K_RUC_NC 1                /* Network Control data             */
#define SNARH$K_RUC_DFC 2               /* Data Flow Control data           */
#define SNARH$K_RUC_SC 3                /* Session Control data             */
#define SNARH$K_RUC_MAX 3
/*                                                                          */
#define DFC$K_RQ_LUSTAT 4               /* Logical Unit Status              */
#define DFC$K_RQ_RTR 5                  /* Ready to Receive brackets        */
#define DFC$K_RQ_BIS 112                /* Bracket Initiation Stopped       */
#define DFC$K_RQ_SBI 113                /* Stop Bracket Initiation          */
#define DFC$K_RQ_QEC 128                /* Quiesce at End of Chain          */
#define DFC$K_RQ_QC 129                 /* Quiesce Complete                 */
#define DFC$K_RQ_RELQ 130               /* Release Quiesce                  */
#define DFC$K_RQ_CANCEL 131             /* Cancel                           */
#define DFC$K_RQ_CHASE 132              /* Chase                            */
#define DFC$K_RQ_SHUTD 192              /* Shutdown                         */
#define DFC$K_RQ_SHUTC 193              /* Shutdown Complete                */
#define DFC$K_RQ_RSHUTD 194             /* Request Shutdown                 */
#define DFC$K_RQ_BID 200                /* Bid to begin bracket             */
#define DFC$K_RQ_SIG 201                /* Signal                           */
#define SC$K_RQ_DACTLU 14               /* Deactivate logical unit          */
#define SC$K_RQ_BIND 49                 /* Bind session                     */
#define SC$K_RQ_UNBIND 50               /* Unbind session                   */
#define SC$K_RQ_SDT 160                 /* Start Data Traffic               */
#define SC$K_RQ_CLEAR 161               /* Clear session to Data-Traffic-Reset */
#define SC$K_RQ_STSN 162                /* Set and Test Sequence Numbers    */
#define SC$K_RQ_RQR 163                 /* Request Recovery                 */
#define SC$K_RQ_CRV 192                 /* Crypto-verify                    */
#define FMD$K_RQ_NSPE 67076             /* NS Procedure Error               */
union SNARH {
    struct  {                           /* RH bytes                         */
        unsigned char SNARH$B_B0;       /* Byte 0                           */
        unsigned char SNARH$B_B1;       /* Byte 1                           */
        unsigned char SNARH$B_B2;       /* Byte 2                           */
        } SNARH$R_RHBYT;
    union  {                            /* Whole of the RH                  */
        struct  {                       /* Request definitions              */
/*                                                                          */
/* Byte 0                                                                   */
/*                                                                          */
            unsigned SNARH$V_ECI : 1;   /* End chain indicator              */
            unsigned SNARH$V_BCI : 1;   /* Begin Chain indicator            */
            unsigned SNARH$V_SDI : 1;   /* Sense Data Included indicator    */
            unsigned SNARH$V_FI : 1;    /* Format indicator                 */
            unsigned SNARH$V_RE0 : 1;   /* Reserved                         */
            unsigned SNARH$V_RUC : 2;   /* Request-Response unit category   */
            unsigned SNARH$V_RRI : 1;   /* Request-Response indicator       */
/*                                                                          */
/* Byte 1                                                                   */
/*                                                                          */
            unsigned SNARH$V_PI : 1;    /* Pacing indicator                 */
            unsigned SNARH$V_QRI : 1;   /* Queued Response indicator        */
            unsigned SNARH$V_RE1 : 2;   /* Reserved                         */
            unsigned SNARH$V_ERI : 1;   /* Exception Response indicator     */
            unsigned SNARH$V_DR2I : 1;  /* Definite Response 2 indicator    */
            unsigned SNARH$V_RE2 : 1;   /* Reserved                         */
            unsigned SNARH$V_DR1I : 1;  /* Definite Response 1 indicator    */
/*                                                                          */
/* Byte 2                                                                   */
/*                                                                          */
            unsigned SNARH$V_CEB : 1;   /* Conditional End Bracket          */
            unsigned SNARH$V_PDI : 1;   /* Padded Data indicator            */
            unsigned SNARH$V_EDI : 1;   /* Enciphered Data indicator        */
            unsigned SNARH$V_CSI : 1;   /* Code Selection indicator         */
            unsigned SNARH$V_RE4 : 1;   /* Reserved                         */
            unsigned SNARH$V_CDI : 1;   /* Change Direction indicator       */
            unsigned SNARH$V_EBI : 1;   /* End Bracket indicator            */
            unsigned SNARH$V_BBI : 1;   /* Begin Bracket indicator          */
            } SNARH$R_RQDEF;
        struct  {                       /* RH response definitions          */
/*                                                                          */
/* Byte 0                                                                   */
/*                                                                          */
            unsigned SNARH$V_RE5 : 8;
/*                                                                          */
/* Byte 1                                                                   */
/*                                                                          */
            unsigned SNARH$V_RE6 : 4;   /* Reserved                         */
            unsigned SNARH$V_RTI : 1;   /* Response Type indicator          */
            unsigned SNARH$V_RE7 : 3;   /* Reserved                         */
/*                                                                          */
/* Byte 2                                                                   */
/*                                                                          */
            unsigned SNARH$V_RE8 : 8;   /* Reserved                         */
            } SNARH$R_RSPDEF;
        } SNARH$V_FULL_RH;
/*                                                                          */
/* RH field values                                                          */
/*                                                                          */
/* RU Categories                                                            */
/*                                                                          */
/* Data Flow Control request definitions.                                   */
/*                                                                          */
    struct  {
        unsigned char DFC$B_DUMMY;      /* Dummy field, keeps SDL happy     */
        } SNARH$R_DFCDEF;
/*                                                                          */
/* Session Control request definitions.                                     */
/*                                                                          */
    struct  {
        unsigned char SC$B_DUMMY;       /* Dummy field, keeps SDL happy     */
        } SNARH$R_SCDEF;
/*                                                                          */
/* FMD NS Header Definitions.                                               */
/*                                                                          */
    struct  {
        unsigned char FMD$B_DUMMY;      /* Dummy field, keeps SDL happy     */
        } SNARH$R_FMDDEF;
    } ;
/*                                                                          */
/* Bind definitions.                                                        */
/* The definitions are in reverse order to IBM because of                   */
/* the differing bit labeling conventions between DEC and IBM               */
/*                                                                          */
#define SNABND$M_P_SEB 1
#define SNABND$M_P_CMP 2
#define SNABND$M_R0 4
#define SNABND$M_PH_COM 8
#define SNABND$M_P_CHR 48
#define SNABND$M_P_RMS 64
#define SNABND$M_P_CHU 128
#define SNABND$M_S_SEB 1
#define SNABND$M_S_CMP 2
#define SNABND$M_R1 4
#define SNABND$M_S_TPC 8
#define SNABND$M_S_CHR 48
#define SNABND$M_S_RMS 64
#define SNABND$M_S_CHU 128
#define SNABND$M_BRQ 1
#define SNABND$M_BIS 2
#define SNABND$M_SNA 4
#define SNABND$M_ACS 8
#define SNABND$M_BTP 16
#define SNABND$M_BRU 32
#define SNABND$M_FMU 64
#define SNABND$M_SEG 128
#define SNABND$M_CNR 1
#define SNABND$M_CVI 2
#define SNABND$M_ALPI 12
#define SNABND$M_BFS 16
#define SNABND$M_RCR 32
#define SNABND$M_NFM 192
#define SNABND$M_SSW 63
#define SNABND$M_R2 64
#define SNABND$M_SSI 128
#define SNABND$M_SRW 63
#define SNABND$M_R3 192
#define SNABND$M_S_EXP 15
#define SNABND$M_S_MAN 240
#define SNABND$M_P_EXP 15
#define SNABND$M_P_MAN 240
#define SNABND$M_PSW 63
#define SNABND$M_R4 64
#define SNABND$M_PSI 128
#define SNABND$M_PRW 63
#define SNABND$M_R5 192
#define SNABND$M_LUT 127
#define SNABND$M_PUF 128
#define SNABND$M_R12 1
#define SNABND$M_AVI 2
#define SNABND$M_R13 12
#define SNABND$M_ASI 16
#define SNABND$M_R14 224
#define SNABND$M_GDS 1
#define SNABND$M_PAS 2
#define SNABND$M_RER 12
#define SNABND$M_R15 16
#define SNABND$M_SYL 96
#define SNABND$M_R16 128
#define SNABND$M_CRL 15
#define SNABND$M_CRS 48
#define SNABND$M_CRP 192
#define SNABND$M_CCM 7
#define SNABND$M_R17 56
#define SNABND$M_SCKSLU 192
#define SNABND$K_LENGTH 29              /* Length of defined part of BIND image */
/* (the BIND image can be longer)                                           */
struct SNABND {
    struct  {                           /* Byte 0                           */
        unsigned char SNABND$B_CMD;     /* Bind command                     */
        } SNABND$B_B0;
    struct  {                           /* Byte 1                           */
        unsigned char SNABND$B_BFMT;    /* Bind format                      */
        } SNABND$B_B1;
    struct  {                           /* Byte 2                           */
        unsigned char SNABND$B_FMP;     /* FM profile                       */
        } SNABND$B_B2;
    struct  {                           /* Byte 3                           */
        unsigned char SNABND$B_TSP;     /* TS profile                       */
        } SNABND$B_B3;
    struct  {                           /* Byte 4 - Primary LU protocols    */
        unsigned SNABND$V_P_SEB : 1;    /* Send End Bracket indicator	    */
        unsigned SNABND$V_P_CMP : 1;    /* Compresion indicator             */
        unsigned SNABND$V_R0 : 1;       /* Reserved                         */
        unsigned SNABND$V_PH_COM : 1;   /* 2-phase commit for sync point    */
        unsigned SNABND$V_P_CHR : 2;    /* Chaining Responses               */
        unsigned SNABND$V_P_RMS : 1;    /* Request mode selection           */
        unsigned SNABND$V_P_CHU : 1;    /* Chaining use                     */
        } SNABND$B_B4;
    struct  {                           /* Byte 5 - Secondary LU protocols  */
        unsigned SNABND$V_S_SEB : 1;    /* Send End Bracket indicator       */
        unsigned SNABND$V_S_CMP : 1;    /* Compresion indicator             */
        unsigned SNABND$V_R1 : 1;       /* Reserved                         */
        unsigned SNABND$V_S_TPC : 1;    /* Two Phase Commit Supported       */
        unsigned SNABND$V_S_CHR : 2;    /* Chaining Responses               */
        unsigned SNABND$V_S_RMS : 1;    /* Request mode selection           */
        unsigned SNABND$V_S_CHU : 1;    /* Chaining use                     */
        } SNABND$B_B5;
    struct  {                           /* Byte 6 - Common Protocols        */
        unsigned SNABND$V_BRQ : 1;      /* BIND response queue capability   */
        unsigned SNABND$V_BIS : 1;      /* BIS sent                         */
        unsigned SNABND$V_SNA : 1;      /* Sequence number availability     */
        unsigned SNABND$V_ACS : 1;      /* Alternate Code Selection         */
        unsigned SNABND$V_BTP : 1;      /* Bracket Termination Protocol     */
        unsigned SNABND$V_BRU : 1;      /* Bracket usage                    */
        unsigned SNABND$V_FMU : 1;      /* FM Header usage                  */
        unsigned SNABND$V_SEG : 1;      /* Session segmenting               */
        } SNABND$B_B6;
    struct  {                           /* Byte 7 - Common Protocols        */
        unsigned SNABND$V_CNR : 1;      /* Contention Resolution            */
        unsigned SNABND$V_CVI : 1;      /* Control vectors included         */
        unsigned SNABND$V_ALPI : 2;     /* Alternat code processing ASCII-7 or 8 */
        unsigned SNABND$V_BFS : 1;      /* Bracket First Speaker            */
        unsigned SNABND$V_RCR : 1;      /* Recovery Responsibilty           */
        unsigned SNABND$V_NFM : 2;      /* Normal Flow Mode                 */
        } SNABND$B_B7;
    struct  {                           /* Byte 8                           */
        unsigned SNABND$V_SSW : 6;      /* Secondary CPMGRs send Window Size */
        unsigned SNABND$V_R2 : 1;       /* Reserved                         */
        unsigned SNABND$V_SSI : 1;      /* Secondary CPMGRs Staging Indicator */
        } SNABND$B_B8;
    struct  {                           /* Byte 9                           */
        unsigned SNABND$V_SRW : 6;      /* Secondary CPMGRs receive Window Size */
        unsigned SNABND$V_R3 : 2;       /* Reserved                         */
        } SNABND$B_B9;
    struct  {                           /* Byte 10                          */
        struct  {                       /* Maximum RU size sent by SLU      */
            unsigned SNABND$V_S_EXP : 4;
/* Exponent                                                                 */
            unsigned SNABND$V_S_MAN : 4;
/* Mantissa                                                                 */
            } SNABND$B_S_MRU;
        } SNABND$B_B10;
    struct  {                           /* Byte 11                          */
        struct  {                       /* Maximum RU size sent by PLU      */
            unsigned SNABND$V_P_EXP : 4;
/* Exponent                                                                 */
            unsigned SNABND$V_P_MAN : 4;
/* Mantissa                                                                 */
            } SNABND$B_P_MRU;
        } SNABND$B_B11;
    struct  {                           /* Byte 12                          */
        unsigned SNABND$V_PSW : 6;      /* Primary CPMGRs send window size  */
        unsigned SNABND$V_R4 : 1;       /* Reserved                         */
        unsigned SNABND$V_PSI : 1;      /* Primary CPMGRs staging indicator */
        } SNABND$B_B12;
    struct  {                           /* Byte 13                          */
        unsigned SNABND$V_PRW : 6;      /* Primary CPMGRs receive window size */
        unsigned SNABND$V_R5 : 2;       /* Reserved                         */
        } SNABND$B_B13;
    struct  {                           /* Byte 14 - PS Profile             */
        unsigned SNABND$V_LUT : 7;      /* LU type                          */
        unsigned SNABND$V_PUF : 1;      /* PS Usage Field                   */
        } SNABND$B_B14;
    union  {                            /* Bytes 15-25 - PS Characteristics */
        struct  {                       /* Straight bytes                   */
            unsigned char SNABND$B_B15; /* Byte 15                          */
            unsigned char SNABND$B_B16; /* Byte 16                          */
            unsigned char SNABND$B_B17; /* Byte 17                          */
            unsigned char SNABND$B_B18; /* Byte 18                          */
            unsigned char SNABND$B_B19; /* Byte 19                          */
            unsigned char SNABND$B_B20; /* Byte 20                          */
            unsigned char SNABND$B_B21; /* Byte 21                          */
            unsigned char SNABND$B_B22; /* Byte 22                          */
            unsigned char SNABND$B_B23; /* Byte 23                          */
            unsigned char SNABND$B_B24; /* Byte 24                          */
            unsigned char SNABND$B_B25; /* Byte 25                          */
            } SNABND$R_BYTES;
        struct  {                       /* LU2 PS usage                     */
            char SNABND$B_R9 [5];       /* Bytes 15-19 - Reserved           */
            unsigned char SNABND$B_DFR; /* Byte 20 - Default Number of Rows */
            unsigned char SNABND$B_DFC; /* Byte 21 - Default Number of Columns */
            unsigned char SNABND$B_ANR; /* Byte 22 - Alternate Number of Rows */
            unsigned char SNABND$B_ANC; /* Byte 23 - Alternate Number of Columns */
            unsigned char SNABND$B_SSS; /* Byte 24 - Session Screen Size    */
            unsigned char SNABND$B_R10; /* Byte 25 - Reserved               */
            } SNABND$R_LU2;
        struct  {                       /* LU6.2 PS usage                   */
            unsigned char SNABND$B_LUL; /* Byte 15 - LU level               */
            char SNABND$B_R11 [7];      /* Bytes 16-22 - Reserved           */
            struct  {                   /* Byte 23                          */
                unsigned SNABND$V_R12 : 1; /* Reserved                      */
                unsigned SNABND$V_AVI : 1; /* Already Verified Indicator accepted */
                unsigned SNABND$V_R13 : 2; /* Reserved                      */
                unsigned SNABND$V_ASI : 1; /* Access Security Information accepted */
                unsigned SNABND$V_R14 : 3; /* Retired                       */
                } SNABND$R_LU62_B23;
            struct  {                   /* Byte 24                          */
                unsigned SNABND$V_GDS : 1; /* CNOS GDS variable flow support */
                unsigned SNABND$V_PAS : 1; /* Parallel session support      */
                unsigned SNABND$V_RER : 2; /* Reinitiation responsibility   */
                unsigned SNABND$V_R15 : 1; /* Reserved                      */
                unsigned SNABND$V_SYL : 2; /* Synchronization Level         */
                unsigned SNABND$V_R16 : 1; /* Reserved                      */
                } SNABND$R_LU62_B24;
            unsigned char SNABND$B_LU62_B25; /* Byte 25 - Reserved          */
            } SNABND$R_LU62;
        } SNABND$R_PS_CHAR;
    struct  {                           /* Byte 26                          */
        unsigned SNABND$V_CRL : 4;      /* Length of cryptography options field */
        unsigned SNABND$V_CRS : 2;      /* Session-level cryptography options */
        unsigned SNABND$V_CRP : 2;      /* Private-cryptography options     */
        } SNABND$B_B26;
    struct  {                           /* Byte 27                          */
        unsigned SNABND$V_CCM : 3;      /* Cryptography Cipher Method       */
        unsigned SNABND$V_R17 : 3;      /* Reserved                         */
        unsigned SNABND$V_SCKSLU : 2;   /* Session cryptography key from SLU */
        } SNABND$B_B27;
    unsigned char SNABND$B_B28;         /* Byte 28                          */
    } ;
/*                                                                          */
/* Define all the SNA Gateway Access Interface Routines                     */
/*                                                                          */
/*                                                                          */
/* Accept                                                                   */
/*                                                                          */
long int SNA$ACCEPT() ;
long int SNA$ACCEPT_W() ;
/*                                                                          */
/* Connect                                                                  */
/*                                                                          */
long int SNA$CONNECT() ;
long int SNA$CONNECT_W() ;
/*                                                                          */
/* Link_Accept                                                              */
/*                                                                          */
long int SNA$LINK_ACCEPT() ;
long int SNA$LINK_ACCEPT_W() ;
/*                                                                          */
/* Listen                                                                   */
/*                                                                          */
long int SNA$LISTEN() ;
long int SNA$LISTEN_W() ;
/*                                                                          */
/* Read Event                                                               */
/*                                                                          */
long int SNA$READEVENT() ;
long int SNA$READEVENT_W() ;
/*                                                                          */
/* Receive                                                                  */
/*                                                                          */
long int SNA$RECEIVE() ;
long int SNA$RECEIVE_W() ;
/*                                                                          */
/* Receive from SSCP                                                        */
/*                                                                          */
long int SNA$RECEIVE_SSCP() ;
long int SNA$RECEIVE_SSCP_W() ;
/*                                                                          */
/* Reconnect                                                                */
/*                                                                          */
long int SNA$RECONNECT() ;
long int SNA$RECONNECT_W() ;
/*                                                                          */
/* Reject                                                                   */
/*                                                                          */
long int SNA$REJECT() ;
long int SNA$REJECT_W() ;
/*                                                                          */
/* Terminate session                                                        */
/*                                                                          */
long int SNA$TERMINATE() ;
long int SNA$TERMINATE_W() ;
/*                                                                          */
/* Transmit                                                                 */
/*                                                                          */
long int SNA$TRANSMIT() ;
long int SNA$TRANSMIT_W() ;
/*                                                                          */
/* Transmit to SSCP                                                         */
/*                                                                          */
long int SNA$TRANSMIT_SSCP() ;
long int SNA$TRANSMIT_SSCP_W() ;
 
/* File:  SNAMSGDEF.H    Created:  11-DEC-1987 17:00:09.26 */
 
#define SNA$_FACILITY	0X00000203
#define SNA$_NORMAL	0X02038009
#define SNA$_TOOFEWPAR	0X02038012
#define SNA$_TOOMANPAR	0X0203801A
#define SNA$_INVACCPAR	0X02038022
#define SNA$_INVAPPPAR	0X0203802A
#define SNA$_INVASTADR	0X02038032
#define SNA$_INVASTPAR	0X0203803A
#define SNA$_INVBINBUF	0X02038042
#define SNA$_INVBINLEN	0X0203804A
#define SNA$_INVBUFPAR	0X02038052
#define SNA$_INVCIRPAR	0X0203805A
#define SNA$_INVDATPAR	0X02038062
#define SNA$_INVEVFNUM	0X0203806A
#define SNA$_INVEVTCOD	0X02038072
#define SNA$_INVNOTADR	0X0203807A
#define SNA$_INVGWYNOD	0X02038082
#define SNA$_INVIOSB	0X0203808A
#define SNA$_INVLENADR	0X02038092
#define SNA$_INVLMTPAR	0X0203809A
#define SNA$_INVPASPAR	0X020380A2
#define SNA$_INVPORTID	0X020380AA
#define SNA$_INVRECCOU	0X020380B2
#define SNA$_INVRHPAR	0X020380BA
#define SNA$_INVSENCOD	0X020380C2
#define SNA$_INVSEQNUM	0X020380CA
#define SNA$_INVSESPAR	0X020380D2
#define SNA$_INVTRAFLG	0X020380DA
#define SNA$_INVUSEPAR	0X020380E2
#define SNA$_ACCTOOLON	0X020380EA
#define SNA$_APPTOOLON	0X020380F2
#define SNA$_BUFTOOSHO	0X020380FA
#define SNA$_CIRTOOLON	0X02038102
#define SNA$_DATTOOLON	0X0203810A
#define SNA$_EVFOUTRAN	0X02038112
#define SNA$_LENTOOLON	0X0203811A
#define SNA$_LMTTOOLON	0X02038122
#define SNA$_PASTOOLON	0X0203812A
#define SNA$_RECTOOLAR	0X02038132
#define SNA$_USETOOLON	0X0203813A
#define SNA$_FAIBLDNCB	0X02038142
#define SNA$_FAIALLBUF	0X0203814A
#define SNA$_FAIALLCTX	0X02038152
#define SNA$_FAIASSCHA	0X0203815A
#define SNA$_FAICOPBIN	0X02038162
#define SNA$_FAICOPBUF	0X0203816A
#define SNA$_FAICREMBX	0X02038172
#define SNA$_FAIESTLIN	0X0203817A
#define SNA$_FUNCABORT	0X02038182
#define SNA$_FUNNOTVAL	0X0203818A
#define SNA$_GATCOMERR	0X02038192
#define SNA$_ILLASTSTA	0X0203819A
#define SNA$_INSRESOUR	0X020381A2
#define SNA$_INVRECLOG	0X020381AA
#define SNA$_MAXSESACT	0X020381B2
#define SNA$_NOEVEPEN	0X020381BA
#define SNA$_NO_GWYNOD	0X020381C2
#define SNA$_TERMPEND	0X020381CA
#define SNA$_TEXT	0X020381D2
#define SNA$_UNBINDREC	0X020381DB
#define SNA$_ABNSESTER	0X020381E2
#define SNA$_ACCINTERR	0X020381EA
#define SNA$_APPNOTSPE	0X020381F2
#define SNA$_BINSPEUNA	0X020381FA
#define SNA$_CIRNOTAVA	0X02038202
#define SNA$_CIRNOTSPE	0X0203820A
#define SNA$_CONREQREJ	0X02038212
#define SNA$_GATINTERR	0X0203821A
#define SNA$_INSGATRES	0X02038222
#define SNA$_INCVERNUM	0X0203822A
#define SNA$_LOGUNIDEA	0X02038232
#define SNA$_NO_SUCACC	0X0203823A
#define SNA$_NO_SUCCIR	0X02038242
#define SNA$_NO_SUCSES	0X0203824A
#define SNA$_PROUNBREC	0X02038252
#define SNA$_SESIN_USE	0X0203825A
#define SNA$_SESNOTAVA	0X02038262
#define SNA$_SESINUNAC	0X0203826A
#define SNA$_UNUUNBREC	0X02038272
#define SNA$_FATINTERR	0X0203827C
#define SNA$_ABOCTXPRE	0X02038284
#define SNA$_ABOWAIACC	0X0203828C
#define SNA$_ASTBLKZER	0X02038294
#define SNA$_BINDATREC	0X0203829C
#define SNA$_CANCELFAI	0X020382A4
#define SNA$_CTXBLKINU	0X020382AC
#define SNA$_DCLASTFAI	0X020382B4
#define SNA$_FAICONMBX	0X020382BC
#define SNA$_FAICOPMBX	0X020382C4
#define SNA$_FAICOPRH	0X020382CC
#define SNA$_FAIDEAMBX	0X020382D4
#define SNA$_FAIFREBUF	0X020382DC
#define SNA$_FAIFRENCB	0X020382E4
#define SNA$_FAIGETCHA	0X020382EC
#define SNA$_FAIGETMBX	0X020382F4
#define SNA$_FAITRIBLA	0X020382FC
#define SNA$_FLUBUFREC	0X02038304
#define SNA$_GATTRAFAI	0X0203830C
#define SNA$_GETDVIFAI	0X02038314
#define SNA$_ILLMBXMSG	0X0203831C
#define SNA$_INVRECCHK	0X02038324
#define SNA$_LIBFREFAI	0X0203832C
#define SNA$_LIBGETFAI	0X02038334
#define SNA$_MBXIOSERR	0X0203833C
#define SNA$_MBXREAFAI	0X02038344
#define SNA$_NOTNORDAT	0X0203834C
#define SNA$_OBJTRAFAI	0X02038354
#define SNA$_PORUNKSTA	0X0203835C
#define SNA$_PORREFNON	0X02038364
#define SNA$_PORREFOUT	0X0203836C
#define SNA$_PROERRBIN	0X02038374
#define SNA$_RECBUFINU	0X0203837C
#define SNA$_RECFREFAI	0X02038384
#define SNA$_RECPENMSG	0X0203838C
#define SNA$_STANOTRUN	0X02038394
#define SNA$_UNINUMUNK	0X0203839C
#define SNA$_UNKDATMSG	0X020383A4
#define SNA$_UNKMSGREC	0X020383AC
#define SNA$_UNKPMRMSG	0X020383B4
#define SNA$_UNKUNBREC	0X020383BC
#define SNA$_UNSUSEREC	0X020383C4
#define SNA$_INVAEFPAR	0X020383CA
#define SNA$_INVNOTPAR	0X020383D2
#define SNA$_AEFOUTRAN	0X020383DA
#define SNA$_FAIFREEF	0X020383E2
#define SNA$_INVARGLEN	0X020383EA
#define SNA$_COMERR	0X020383F2
#define SNA$_FUNNOTIMP	0X020383FA
#define SNA$_RUSIZEXC	0X02038402
#define SNA$_GETJPIFAI	0X0203840C
#define SNA$_PACFSM	0X02038414
#define SNA$_PENUSETRA	0X0203841C
#define SNA$_DFCFATAL	0X02038964
#define SNA$_INMEMCT	0X0203896C
#define SNA$_UNFREECT	0X02038974
#define SNA$_NOMEMRSP	0X0203897C
#define SNA$_PROBFREEMEM	0X02038984
#define SNA$_UNFREELUCB	0X0203898C
#define SNA$_UNFREESCB	0X02038994
#define SNA$_UNFREEMUCB	0X0203899C
#define SNA$_UNFREEEVT	0X020389A4
#define SNA$_UNABLEEVT	0X020389AC
#define SNA$_PROBINREVT	0X020389B4
#define SNA$_INCOPYERR	0X020389BC
#define SNA$_NOTOCCURST	0X020389C4
#define SNA$_JUNKSTATE	0X020389CC
#define SNA$_NOINTMUCB	0X020389D4
#define SNA$_UNDEFSENDCHK	0X020389DC
#define SNA$_INSENDERR	0X020389E4
#define SNA$_UNQAST	0X020389EC
#define SNA$_UNDQAST	0X020389F4
#define SNA$_UNABLELUCB	0X020389FA
#define SNA$_UNABLESCB	0X02038A02
#define SNA$_UNABLEMUCB	0X02038A0A
#define SNA$_MUTOSENDCHK	0X02038A12
#define SNA$_MUTORCVCHK	0X02038A1A
#define SNA$_PLUPROVIO	0X02038A22
#define SNA$_MUTOEXR	0X02038A2B
#define SNA$_INVFLOPAR	0X02038A32
#define SNA$_INVGWYNAM	0X02038A3A
#define SNA$_INVINBSIZ	0X02038A42
#define SNA$_INVOUTSIZ	0X02038A4A
#define SNA$_SNAASSFAI	0X02038A52
#define SNA$_RECFAIL	0X02038A5C
#define SNA$_LINSETOFF	0X0203E402
#define SNA$_SESNOTSPE	0X0203E40A
#define SNA$_INVSESTYP	0X0203E412
#define SNA$_TRANOTSPE	0X0203E41A
#define SNA$_NOTRANTAB	0X0203E422
#define SNA$_USEOUTRAN	0X0203E42A
#define SNA$_APPOUTRAN	0X0203E432
#define SNA$_ACCTRAERR	0X0203E43A
#define SNA$_INTSENERR	0X0203E442
#define SNA$_PPINOTSPE	0X0203E44A
#define SNA$_PORNOTSPE	0X0203E452
#define SNA$_TCSNDCHK	0X0203E45A
#define SNA$_NOSNRMREC	0X0203E462
#define SNA$_NOACTPREC	0X0203E46A
#define SNA$_NOACTLREC	0X0203E472
#define SNA$_ACTLOGREC	0X0203E47A
#define SNA$_FAIGETUSE	0X0203E722
#define SNA$_INVLUPPAR	0X0203E72A
#define SNA$_INVPIDPAR	0X0203E732
#define SNA$_LUPTOOLON	0X0203E73A
#define SNA$_SESACCDEN	0X0203E742
#define SNA$_CVTERR	0X0203E74A
#define SNA$_EXPBUF	0X0203E752
#define SNA$_FILOPN	0X0203E75A
#define SNA$_GOTBUF	0X0203E762
#define SNA$_INVNCBLEN	0X0203E76A
#define SNA$_INVNCBPAR	0X0203E772
#define SNA$_NOCHAN	0X0203E77A
#define SNA$_READFIL	0X0203E782
#define SNA$_UNKFUNC	0X0203E78A
#define SNA$_UNXWRIT	0X0203E792
#define SNA$_WRITBAD	0X0203E79A
#define SNA$_WRITFLO	0X0203E7A2
#define SNA$_WRITLEN	0X0203E7AA
#define SNA$_INVOPC	0X0203E7B4
#define SNA$_SESOUTRAN	0X0203E7BA
#define SNA$_INVPACWIN	0X0203E7C2
#define SNA$_INVRUSIZE	0X0203E7CA

#if (defined(__alpha) || defined(__ia64))
#pragma member_alignment __restore
#endif /* ALPHA */
