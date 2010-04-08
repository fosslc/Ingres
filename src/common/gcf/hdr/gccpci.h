/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: GCCPCI.H
**
** Description:
**	Contains PCI definitions for AL, PL, SL, and TL.
**
** History:
**      5-Jan-88 (jbowers)
**          Initial module creation
**     09-Jan-89 (jbowers)
**          Added NL_PCI to provide pad area for protocol handlers.
**     14-Sep-89 (ham)
**          Defined more sizes in PCIs for byte alignment.
**     20-Sep-89 (ham)
**          Bring in byte alignment macros.
**	25-Jan-90 (seiwald)
**	     Support for message grouping:  track the peer
**	     presentation's protocol level in the CCB; allow for MDE's
**	     to be chained in the CCB for each direction and flow type;
**	     put a length indicator in the PL_TD_PCI; allow for new
**	     "end of group" indicator in the PL and AL srv_parms flags.
**      26-Jan-90 (cmorris)
**           Changed EX SPDU to EXD SPDU to avoid unfortunate clash with
**           EX services!!
**      13-Aug-91 (dchan)
**          Added an ifdef for decstations because the compiler
**          can't handle c statements like "a=x(b,c)".  We will use
**          use a=(x(b)?c:c) instead.  Hopefully dec will fix this
**          problem soon so the ifdef can be removed, but so far
**          they haven't given any sort of commitment yet on dates.
**  	11-May-92 (cmorris)
**  	    Added notes to indicate that "C" representations of PDUs
**  	    are local representations and not necessarily indicative of
**  	    the sequence of octets that goes out on the wire.
**  	14-Dec-92 (cmorris)
**  	    Got rid of the PL_(TD TE)_PCI structure definitions (no longer
**  	    used) and replaced them with a common description of Presentation
**  	    Data Value PCIs, and defines for its length.
**  	06-Jan-93 (cmorris)
**  	    Replaced remaining sizeofs with defines. Using sizeof is
**  	    simply a no-no as different architectures cause different
**  	    results to be created.
**  	07_Jan-93 (cmorris)
**  	    Moved PDU identifiers into definitions of PDUs.
**  	    Made flag words consistently single-octet quantities.
**  	08-Jan-93 (cmorris)
**  	    Straightened out defines for length of CR TPDU.
**  	05-Feb-93 (cmorris)
**  	    Removed all definitions for transport expedited data.
**  	06-Jun-97 (rajus01)
**  	    Added protect_parm structure to CR and CC TPDUs for
**	    handling encryption/decryption in the TL.
**	    Defined TL_PRTCT_PARMVAL_ID.
**	    Removed TCR_VN_DFLT, TCC_VN_DFLT to avoid confusion.
**	    Defined TCR_VN_1, TCR_VN_2, TCC_VN_1, TCC_VN_2.
**	20-Aug-97 (gordy)
**	    Cleanup TL PCI definitions.  Fixed invalid length definitions.
**	    Changed secondary reason in DR TPDU to an u_i4 to handle status
**	    codes.  Added options to CR TPDU protection parameters.
**	26-Aug-97 (gordy)
**	    Cleanup SL PCI definitions.  Added support for extended length
**	    indicators.
**	 4-Dec-97 (rajus01)
**	    Added SL_AUTH_XPI extended parm for remote authentication.
**	12-Jul-00 (gordy)
**	    Added SL_VRSN_PI for SL protocol version as a paramenter in
**	    the SL connection SPDU.
**	16-Jan-04 (gordy)
**	    Permit packet sizes upto 32K.
**	16-Jun-04 (gordy)
**	    Added session mask parameter to TL CR & CC messages.
*/

typedef struct _TL_FP_PCI TL_FP_PCI;
typedef struct _TL_CR_PCI TL_CR_PCI;
typedef struct _TL_CC_PCI TL_CC_PCI;
typedef struct _TL_DR_PCI TL_DR_PCI;
typedef struct _TL_DC_PCI TL_DC_PCI;
typedef struct _TL_DT_PCI TL_DT_PCI;
typedef struct _SL_ID SL_ID;
typedef struct _SL_XID SL_XID;
typedef struct _SL_PGIU SL_PGIU;
typedef struct _SL_XPGIU SL_XPGIU;
typedef struct _SL_PIU SL_PIU;
typedef struct _SL_XPIU SL_XPIU;
typedef struct _SL_SPDU SL_SPDU;
typedef struct _SL_XSPDU SL_XSPDU;
typedef struct _PL_CP_PCI PL_CP_PCI;
typedef struct _PL_CPA_PCI PL_CPA_PCI;
typedef struct _PL_CPR_PCI PL_CPR_PCI;
typedef struct _PL_ARU_PCI PL_ARU_PCI;
typedef struct _PL_ARP_PCI PL_ARP_PCI;
typedef struct _AL_AARQ_PCI AL_AARQ_PCI;
typedef struct _AL_AARE_PCI AL_AARE_PCI;
typedef struct _AL_RLRQ_PCI AL_RLRQ_PCI;
typedef struct _AL_RLRE_PCI AL_RLRE_PCI;
typedef struct _AL_ABRT_PCI AL_ABRT_PCI;


/*
********************************************************************************
********************************************************************************
**                                                                            **
**			   APPLICATION LAYER				      **
**                                                                            **
**			    APDU Structures				      **
**                                                                            **
********************************************************************************
********************************************************************************
**
** Following are the definitions of the structures used to map the Protocol
** Control Information (PCI) portions of the Application Layer APDU's.
** Note: As these are "C" language representations, they do not necessarily
** represent the sequence of octets that go out "on the wire". The
** structures are merely a local representation of the APDU's.
*/

/*}
** Name: AL_AARQ_PCI - Application Layer A_ASSOCIATE_REQUEST PCI layout
**
** Description:
**      This structure maps the PCI header area for the Application Layer 
**      A_ASSOCIATE_REQUEST (AARQ) PDU.
*/

struct _AL_AARQ_PCI		/* A_ASSOCIATE_REQUEST (AL_AARQ) */
{
    u_i2	prot_vrsn;			/* Protocol version */
    u_i2	l_ae_title;			/* Length of App entity title */
    char	ae_title[GCC_L_AE_TITLE];	/* App entity title */

# define SZ_AARQ_PCI		(2+2+GCC_L_AE_TITLE)
    
}; /* AL_AARQ_PCI */

/*}
** Name: AL_AARE_PCI - Application Layer A_ASSOCIATE_RESPONSE PCI layout
**
** Description:
**      This structure maps the PCI header area for the Application Layer 
**      A_ASSOCIATE_REQUEST (AL_AARE) PDU.
*/

struct _AL_AARE_PCI		/* A_ASSOCIATE_RESPONSE (AL_AARE) */
{
    u_i2	prot_vrsn;		/* Protocol version */
    u_i2	result;			/* Result of assocn rqst */

# define SZ_AARE_PCI		(2+2)

}; /* AL_AARE_PCI */

/*}
** Name: AL_RLRQ_PCI - Application Layer A_RELEASE_REQUEST PCI layout
**
** Description:
**      This structure maps the PCI header area for the Application Layer 
**      A_RELEASE_REQUEST (RLRQ) PDU.
*/

struct _AL_RLRQ_PCI		/* A_RELEASE_REQUEST (RLRQ) */
{
    u_i2	reason;			/* Reason for release */

# define RLRQ_NORMAL		0
# define RLRQ_URGENT		1
# define RLRQ_UNDEFINED		2

# define SZ_RLRQ_PCI		2

}; /* AL_RLRQ _PCI */

/*}
** Name: AL_RLRE_PCI - Application Layer A_RELEASE_RESPONSE PCI layout
**
** Description:
**      This structure maps the PCI header area for the Application Layer 
**      A_RELEASE_RESPONSE (RLRE) PDU.
*/

struct _AL_RLRE_PCI		/* A_RELEASE_RESPONSE (RLRE) */
{
    u_i2	reason;			/* Reason for release */

# define RLRE_NORMAL		0
# define RLRE_NOT_FNSHD		1
# define RLRE_UNDEFINED		2

# define SZ_RLRE_PCI		2

}; /* AL_RLRE_PCI */

/*}
** Name: AL_ABRT_PCI - Application Layer A_ABORT PCI layout
**
** Description:
**      This structure maps the PCI header area for the Application Layer 
**      A_ABORT (ABRT) PDU.
*/

struct _AL_ABRT_PCI		/* A_ABORT (ABRT) */
{
    u_i2	abort_source;		/* Source: user or provider */

# define ABRT_REQUESTOR		1    
# define ABRT_PROVIDER		2

# define SZ_ABRT_PCI		2

}; /* AL_ABRT_PCI */


/*
********************************************************************************
********************************************************************************
**                                                                            **
**			   PRESENTATION LAYER				      **
**                                                                            **
**			    PPDU Structures				      **
**                                                                            **
********************************************************************************
********************************************************************************
**
** Following are the definitions of the structures used to map the Protocol
** Control Information (PCI) portions of the Presentation Layer PPDU's.
** Note: As these are "C" language representations, they do not necessarily
** represent the sequence of octets that go out "on the wire". The
** structures are merely a local representation of the PPDU's.
*/

/*}
** Name: PL_CP_PCI - Presentation Layer Connect Presentation PCI layout
**
** Description:
**      This structure maps the PCI header area for the Presentation Layer 
**      Connect Presentation (CP) PDU.
*/

struct _PL_CP_PCI		/* Connect Presentation (CP) PPDU PCI */
{
    u_i2	cg_psel;		/* Calling Presentation Selector */
    u_i2	cd_psel;		/* Called Presentation Selector */
    u_i2	prot_vrsn;		/* Protocol version */
    PL_CONTEXT	dflt_cntxt;		/* Default Context */
    u_i2	prsntn_rqmnts;		/* Presentation requirements */
    u_i2	session_rqmnts;		/* Session requirements */

# define SZ_CP_PCI		(2+2+2+SZ_CONTEXT+2+2)

}; /* PL_CP_PCI */

/*}
** Name: PL_CPA_PCI - Presentation Layer Connect Presentation Accept PCI layout
**
** Description:
**      This structure maps the PCI header area for the Presentation Layer 
**      Connect Presentation Accept (CPA) PDU.
*/

struct _PL_CPA_PCI		/* Connect Presentation Accept (CPA) PPDU PCI */
{
    u_i2	cd_psel;		/* Called Presentation Selector */
    PL_CONTEXT	dflt_cntxt;	 	/* Default Context */
    u_i2	prot_vrsn;		/* Protocol version */
    u_i2	prsntn_rqmnts;		/* Presentation requirements */
    u_i2	session_rqmnts;		/* Session requirements */

# define SZ_CPA_PCI		(2+SZ_CONTEXT+2+2+2)

}; /* PL_CPA_PCI */

/*}
** Name: PL_CPR_PCI - Presentation Layer Connect Presentation Reject PCI layout
**
** Description:
**      This structure maps the PCI header area for the Presentation Layer 
**      Connect Presentation Reject (CPR) PDU.
*/

struct _PL_CPR_PCI		/* Connect Presentation Reject (CPR) PPDU PCI */
{
    u_i2	cd_psel;		/* Called Presentation Selector */
    PL_CONTEXT	dflt_cntxt;		/* Default Context */
    u_i2	prot_vrsn;		/* Protocol version */
    u_i2	provider_reason;	/* Refusal reason */

# define SZ_CPR_PCI		(2+SZ_CONTEXT+2+2)

}; /* PL_CPR_PCI */

/*}
** Name: PL_ARU_PCI - Presentation Layer User Abort PCI layout
**
** Description:
**      This structure maps the PCI header area for the Presentation Layer 
**      User Abort (ARU) PDU.
*/

struct _PL_ARU_PCI		/* User Abort (ARU) PPDU PCI */
{
    u_char	ppdu_id;		/* ID: ARU or ARP */

# define ARU			1	/* Abnormal Release: user   */

# define SZ_ARU_UDATA		9

    u_char	user_data[SZ_ARU_UDATA];	/* User data */

# define SZ_ARU_PCI		(1+SZ_ARU_UDATA)

}; /* PL_ARU_PCI */

/*}
** Name: PL_ARP_PCI - Presentation Layer Provider Abort PCI layout
**
** Description:
**      This structure maps the PCI header area for the Presentation Layer 
**      Provider Abort (ARP) PDU.
*/

struct _PL_ARP_PCI		/* Provider Abort (ARP) PPDU PCI */
{
    u_char	ppdu_id;		/* ID: ARU or ARP */

# define ARP			2	/* Abnrml Rlse: provider */

    u_i2	abort_reason;		/* Provider abort reason */

# define PL_ARP_UNSPECIFIED	0

# define SZ_ARP_PCI		(1+2)

}; /* PL_ARP_PCI */

/*
** Name: PDV - Presentation Data Value layout
**
** Description:
**      This is a description of the PCIs for Presentation Data Values.
**
**  +-------+----------+--------+
**  | flags | msg type | length |
**  +-------+----------+--------+
**  0       1          2        4
**
**  flags - GCA message attributes 
**  msg type - GCA message type 
**  length - Length of ensuing GCA message
**
**  Note: SZ_PDV is present for compatability reasons:- for PL versions prior
**  to PL_VRSN_CONCAT, length was not present.
*/

# define SZ_PDV			(1+1)
# define SZ_PDVN		(1+1+2)


/*
********************************************************************************
********************************************************************************
**                                                                            **
**			    SESSION LAYER				      **
**                                                                            **
**			    SPDU Structures				      **
**                                                                            **
********************************************************************************
********************************************************************************
**
** Following are the definitions of the structures used to map the 
** Protocol Control Information (PCI) portions of the Session Layer 
** SPDU's.
**
** Note: As these are "C" language representations, they do not 
** necessarily represent the sequence of octets that go out "on 
** the wire". The structures are merely a local representation 
** of the TPDU's.
*/

/*
** Name: SL_ID - Session Layer PCI identifier layout
**
** Description:
**      This structure maps the initial portion of the SL
**	PCI header area, to allow identification of the SPDU.
*/

struct _SL_ID			/* Identifier part of SPDU PCI */
{
    u_char	si;			/* SPDU type indicator */

# define SL_DT			1	/* Normal Data */
# define SL_EX			5	/* Expedited Data */
# define SL_NF			8 	/* Not Finished */
# define SL_FN			9	/* Finish */
# define SL_DN			10	/* Disconnect */
# define SL_RF			12	/* Refuse */
# define SL_CN			13	/* Connect */
# define SL_AC			14	/* Accept */
# define SL_AB			25	/* Abort */

    u_char	li;			/* length indicator */

# define SZ_SL_ID		(1+1)

}; /* SL_ID */

/*
** Name: SL_XID - Session Layer Extended PCI identifier layout
**
** Description:
**      This structure maps the initial portion of the SL
**	extended PCI header area, to allow identification 
**	of the SPDU.  Length indicators are extended to
**	support values of 255 and greater.
*/

struct _SL_XID			/* Identifier part of SPDU PCI */
{
    u_char	si;			/* SPDU type indicator */
    u_char	xi;			/* extended length indicator */

# define SL_XLI			0xff

    u_i2	li;			/* length indicator */

# define SZ_SL_XID		(1+1+2)

}; /* SL_XID */

/*
** Name: SL_PGIU - Session Layer Parameter Group Identifier Unit (PGIU)
**
** Description:
**      This structure maps the Parameter Group Identifier (PGI) area of the
**	PCI header area, to allow identification of the parameter group which
**	follows.
*/

struct _SL_PGIU			/* PGI part of SPDU PCI */
{
    u_char	pgi;			/* Parameter Group Identifier */

# define SL_UDATA_PGI		193	/* User Data */
# define SL_XUDATA_PGI		194	/* Extended User Data */

    u_char	li;			/* length indicator */

# define SZ_SL_PGIU		(1+1)

}; /* SL_PGIU */

/*
** Name: SL_XPGIU - Session Layer Parameter Group Identifier Unit (PGIU)
**
** Description:
**      This structure maps the Parameter Group Identifier (PGI) area of the
**	PCI header area, to allow identification of the parameter group which
**	follows.
*/

struct _SL_XPGIU			/* PGI part of SPDU PCI */
{
    u_char	pgi;			/* Parameter Group Identifier */
    u_char	xi;			/* Extended length indicator */
    u_i2	li;			/* length indicator */

# define SZ_SL_XPGIU		(1+1+2)

}; /* SL_XPGIU */

/*
** Name: SL_PIU - Session Layer Parameter Identifier Unit (PIU)
**
** Description:
**	This structure maps the Parameter Identifier Unit (PIU) area of the 
**	PCI header area, to allow identification of the parameter (within a
**	parameter group) which follows.
*/

struct _SL_PIU			/* PIU part of SPDU PCI */
{
    u_char	pi;			/* Parameter Identifier */

# define SL_TDISC_PI		17	/* Transport disconnect */
# define SL_VRSN_PI		22	/* SL protocol version */
# define SL_RSN_PI		50	/* Reason */

/*
** The following parameter indicators are only
** used in the SL_XUDATA_PGI.  They use values
** similar to existing SL parameter indicators,
** so they are designated as extended parameter
** indicators.
*/
# define SL_USER_XPI		10	/* Username */
# define SL_PASS_XPI		11	/* Password */
# define SL_SECT_XPI		12	/* Security label type */
# define SL_SECL_XPI		13	/* Security label */
# define SL_AUTH_XPI		16	/* Remote authentication */
# define SL_FLAG_XPI		19	/* GCA flags */
# define SL_PROT_XPI		22	/* GCA protocol */

    u_char	li;			/* length indicator */

# define SZ_SL_PIU		(1+1)

}; /* SL_PIU */

struct _SL_XPIU			/* PIU part of SPDU PCI */
{
    u_char	pi;			/* Parameter Identifier */
    u_char	xi;			/* Extended length indicator */
    u_i2	li;			/* length indicator */

# define SZ_SL_XPIU		(1+1+2)

}; /* SL_XPIU */

/*
** Name SL_SPDU
**
** Description:
**	Defines the common SL SPDU format.  Most SL SPDU's 
**	consist of a single parameter for the user data 
**	(containing the AL/PL PCI's).
*/
struct _SL_SPDU			/* Common SL SPDU */
{
    SL_ID	id;			/* SPDU Identifier */
    SL_PGIU	pgiu;			/* User Data parm group */

# define SZ_SL_SPDU		(SZ_SL_ID+SZ_SL_PGIU)

}; /* SL_SPDU */

/*
** Name SL_XSPDU
**
** Description:
**	Defines the common extended SL SPDU format.  Most SL SPDU's 
**	consist of a single parameter for the user data (containing 
**	the AL/PL PCI's).  This structure uses the extended length
**	indicator format to support lengths of 255 and greater.
*/
struct _SL_XSPDU		/* Common extended SL SPDU */
{
    SL_XID	id;			/* SPDU Identifier */

    SL_XPGIU	pgiu;			/* User Data parm group */

# define SZ_SL_XSPDU		(SZ_SL_XID+SZ_SL_XPGIU)

}; /* SL_XSPDU */

/*
** Some SPDU sizes needed to figure offsets of pci/user data.
*/
# define SZ_CN_PCI		(SZ_SL_XID + SZ_SL_PIU + 1 + SZ_SL_XPGIU)
# define SZ_DT_PCI		SZ_SL_ID

/*
** Definitions for SL SPDU PGIU and PIU parameter values.
*/

/*
** RF SPDU Reason Code
*/
# define SS_RJ_USER		2	/* Rejection by SS user */

/*
** AB SPDU Transport Disconnect
*/
# define SL_TCREL		0x01	/* Trnsprt Conn released */
# define SL_USER		0x02	/* User Abort */
# define SL_PROT_ER		0x04	/* Protocol Error */
# define SL_UNDEF		0x08	/* Undefined */



/*
********************************************************************************
********************************************************************************
**                                                                            **
**			    TRANSPORT LAYER				      **
**                                                                            **
**			    TPDU Structures				      **
**                                                                            **
********************************************************************************
********************************************************************************
**
** Following are the definitions of the structures used to map the Protocol
** Control Information (PCI) portions of the Transport Layer TPDU's. 
** Note: As these are "C" language representations, they do not necessarily
** represent the sequence of octets that go out "on the wire". The
** structures are merely a local representation of the TPDU's.
*/

/*
** Length of the li parameter.  All TL PCIs begin with the li parameter
** (the length of that particular PCI - the length of the li (1)).  
** This define is for expediency in calculating the PCI length.
*/
# define SZ_TL_li		1   	   

/*
** TL user data area associated with CR, CC TPDU's.
*/
# define TL_PRTCT_PARMVAL_ID	0xE1	/* Protection parameter value ID */

/*}
** Name: TL_FP_PCI - Transport Layer PCI fixed part header layout
**
** Description:
**      This structure maps the initial portion of the fixed part of the TL
**	PCI header area, to allow identification of the TPDU.
*/

struct _TL_FP_PCI		/* Fixed part of TPDU PCI */
{
    u_char	li;			/* length indicator */

    /* Following is the fixed part of the TPDU */

    u_char	tpdu;			/* TPDU code */

# define TPDU_CODE_MASK		0xF0	/* TPDU code mask */

    u_i2	dst_ref;		/* Destination reference */

# define SZ_TL_FP_PCI		(1+1+2)

}; /* TL_FP_PCI */

/*}
** Name: TL_CR_PCI - Transport Layer Connection Request PCI layout
**
** Description:
**      This structure maps the PCI header area for the Transport Layer 
**      Connection Request (CR) PDU.
**
** History:
**	16-Jan-04 (gordy)
**	    Permit packet sizes upto 32K.
**	16-Jun-04 (gordy)
**	    Added session mask.
*/

struct _TL_CR_PCI		/* Connection Request (CR) TPDU PCI */
{
    u_char	li;			/* length indicator */

    /* Following is the fixed part of the CR TPDU */

    u_char	tpdu_cdt;		/* TPDU code/initial credit alloc */

# define TCR_TPDU		0xE0	/* CR TPDU code */

    u_i2	dst_ref;		/* Destination reference */
    u_i2	src_ref;		/* Source reference */
    u_char	cls_option;		/* Class-option */

# define TCR_CLS_MASK		0xF0	/* Mask for class portion */
# define TCR_OPTN_MASK		0x0F	/* Mask for option portion */
# define TCR_CLASS		2	/* TL class */
# define TCR_CLS_OPT		((TCR_CLASS << 4) & TCR_CLS_MASK)

# define SZ_CR_FP_PCI	    	(1+1+2+2+1)

    /* Following is the variable part of the CR TPDU */

    struct			/* Calling TSAP identifier */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCR_CG_TSAP		0xC1	/* Calling TSAP parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCR_L_CG_TSAP		2	/* Calling TSAP parm length */

	u_i2	tsap_id;		/* TSAP identifier */

# define SZ_CG_TSAP		(1+1+2)

    } cg_tsap_id;

    struct			/* Called TSAP identifier */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCR_CD_TSAP		0xC2	/* Called TSAP parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCR_L_CD_TSAP		2	/* Called TSAP parm length */

	u_i2	tsap_id;		/* TSAP identifier */

# define SZ_CD_TSAP		(1+1+2)

    } cd_tsap_id;

    struct			/* TPDU size */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCR_SZ_TPDU		0xC0	/* TPDU size parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCR_L_SZ_TPDU		1	/* TPDU size parm length */

	u_char	size;			/* Encoded TPDU size */

# define TPDU_64K_SZ		0x10	/* 65536 TPDU size */
# define TPDU_32K_SZ		0x0F	/* 32768 TPDU size */
# define TPDU_16K_SZ		0x0E	/* 16384 TPDU size */
# define TPDU_8K_SZ		0x0D	/* 8192 TPDU size */
# define TPDU_4K_SZ		0x0C	/* 4096 TPDU size */
# define TPDU_2K_SZ		0x0B	/* 2048 TPDU size */
# define TPDU_1K_SZ		0x0A	/* 1024 TPDU size */
# define TPDU_512_SZ		0x09	/* 512 TPDU size */
# define TPDU_256_SZ		0x08	/* 256 TPDU size */
# define TPDU_128_SZ		0x07	/* 128 TPDU size */

# define SZ_TPDU_SZ		(1+1+1)

    } tpdu_size;

    struct			/* Version number */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCR_VERSION_NO		0xC4	/* Version number parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCR_L_VRSN_NO		1	/* TPDU size parm length */

	u_char	version_no;		/* Version number */

# define TCR_VN_1		0x01	 /* First version  */
# define TCR_VN_2		0x02     /* Second version */

# define SZ_VRSN_NO		(1+1+1)

    } version_no;

    struct			/* Session Mask */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCR_SESS_MASK		0xC7	/* Session mask parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCR_L_SESS_MASK	GCC_SESS_MASK_MAX  /* TPDU size parm length */

	u_char	mask[GCC_SESS_MASK_MAX];/* Session mask */

# define SZ_SESS_MASK		(1+1+GCC_SESS_MASK_MAX)

    } session_mask;

    struct			/* Protection Parameter */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCR_PROTECT_PARM	0xC5	/* Protection parm id */

	u_char	parm_lng;		/* Parameter length */

	u_char	options;		/* Protection options */

# define TCR_ENCRYPT_REQUEST	0x01	/* Encryption permited */

	u_char	mech_id[1];		/* Variable length list of Mech IDs */

# define SZ_CR_PROT_PARM	(1+1+1)	

    } protection_parm;

# define SZ_TL_CR_PCI		(SZ_CR_FP_PCI + SZ_CG_TSAP + \
				 SZ_CD_TSAP + SZ_TPDU_SZ + \
				 SZ_VRSN_NO + SZ_SESS_MASK)

}; /* TL_CR_PCI */

/*}
** Name: TL_CC_PCI - Transport Layer Connection Confirm PCI layout
**
** Description:
**      This structure maps the PCI header area for the Transport Layer 
**      Connection Confirm (CC) PDU.
*/

struct _TL_CC_PCI		/* Connection Confirm (CC) TPDU PCI */
{
    u_char	li;			/* length indicator */

    /* Following is the fixed part of the CC TPDU */

    u_char	tpdu_cdt;		/* TPDU code/initial credit alloc */

# define TCC_TPDU		0xD0	/* CC TPDU code */

    u_i2	dst_ref;		/* Destination reference */
    u_i2	src_ref;		/* Source reference */
    u_char	cls_option;		/* Class option */

# define TCC_CLS_MASK		0xF0	/* Mask for class portion */
# define TCC_OPTN_MASK		0x0F	/* Mask for option portion */
# define TCC_CLASS		2
# define TCC_CLS_OPT		((TCC_CLASS << 4) & TCC_CLS_MASK)

# define SZ_CC_FP_PCI		(1+1+2+2+1)

    /* Following is the variable part of the CC TPDU */

    struct			/* Calling TSAP identifier */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCC_CG_TSAP		0xC1	/* Calling TSAP parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCC_L_CG_TSAP		2	/* Calling TSAP parm length */

	u_i2	tsap_id;		/* TSAP identifier */

    } cg_tsap_id;

    struct			/* Called TSAP identifier */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCC_CD_TSAP		0xC2	/* Called TSAP parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCC_L_CD_TSAP		2	/* Calling TSAP parm length */

	u_i2	tsap_id;		/* TSAP identifier */

    } cd_tsap_id;

    struct			/* TPDU size */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCC_SZ_TPDU		0xC0	/* TPDU size parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCC_L_SZ_TPDU		1	/* TPDU size parm length */

	u_char	size;			/* Encoded TPDU size */

    } tpdu_size;

    struct			/* Version number */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCC_VERSION_NO		0xC4	/* Version number parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCC_L_VRSN_NO		1	/* TPDU size parm length */

	u_char	version_no;		/* Version number */

# define TCC_VN_1		0x01	/* First version  */
# define TCC_VN_2    		0x02	/* Second version */

    } version_no;

    struct			/* Session mask */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCC_SESS_MASK		0xC7	/* Session mask parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCC_L_SESS_MASK	GCC_SESS_MASK_MAX  /* TPDU size parm length */

	u_char	mask[GCC_SESS_MASK_MAX];/* Session mask */

    } session_mask;

    struct			/* Protection Parameter */
    {
	u_char	parm_code;		/* Parameter identifier code */

# define TCC_PROTECT_PARM	0xC5	/* Protection parm id */

	u_char	parm_lng;		/* Parameter length */

# define TCC_L_PROTECT		1	/* Protection parm length */

	u_char	mech_id;		/* list of Mechanism IDs */

# define SZ_CC_PROT_PARM	(1+1+1)

    } protection_parm;

# define SZ_TL_CC_PCI		(SZ_CC_FP_PCI + SZ_CG_TSAP + \
				 SZ_CD_TSAP + SZ_TPDU_SZ + SZ_VRSN_NO)

# define SZ_TL_CC_INVALID	(SZ_TL_CC_PCI + 1 - SZ_TL_li)

}; /* TL_CC_PCI */

/*}
** Name: TL_DR_PCI - Transport Layer Disconnect Request PCI layout
**
** Description:
**      This structure maps the PCI header area for the Transport Layer 
**      Disconnect Request (DR) PDU.
*/

struct _TL_DR_PCI		/* Disconnect Request (DR) TPDU PCI */
{
    u_char	li;			/* length indicator */

    /* Following is the fixed part of the DR TPDU */

    u_char	tpdu;			/* TPDU code */

# define TDR_TPDU		0x80	/* DR TPDU code */

    u_i2	dst_ref;		/* Destination reference */
    u_i2	src_ref;		/* Source reference */
    u_char	reason;			/* Reason for disconnect */

# define TDR_NORMAL		(128+0)	/* Normal disconnect by SL */

# define SZ_TL_DR_PCI		(1+1+2+2+1)

    /* Following is the variable part of the DR TPDU */

    struct			/* Secondary reason code */
    {
	u_char	parmTPDU;		/* Parameter identifier code */

# define TDR_SEC_RSN		0xE0	/* Secondary reason parm code */

	u_char	parm_lng;		/* Parameter length */

# define TDR_L_SEC_RSN		2	/* Secondary reason parm lng */

	u_i4	sec_rsn;		/* Secondary reason */

# define SZ_TL_DR_PARM		(1+1+4)

    } sec_rsn;

# define SZ_TL_DR_INVALID	(SZ_TL_DR_PCI + SZ_TL_DR_PARM - 1 - SZ_TL_li)

}; /* TL_DR_PCI */

/*}
** Name: TL_DC_PCI - Transport Layer Disconnect Confirm PCI layout
**
** Description:
**      This structure maps the PCI header area for the Transport Layer 
**      Disconnect Confirm (DC) PDU.
*/

struct _TL_DC_PCI		/* Disconnect Confirm (DC) TPDU PCI */
{
    u_char	li;			/* length indicator */

    /* Following is the fixed part of the DC TPDU */

    u_char	tpdu;			/* TPDU code */

# define TDC_TPDU		0xC0	/* DC TPDU code */

    u_i2	dst_ref;		/* Destination reference */
    u_i2	src_ref;		/* Source reference */

# define SZ_TL_DC_PCI		(1+1+2+2)

}; /* TL_DC_PCI */

/*}
** Name: TL_DT_PCI - Transport Layer Data PCI layout
**
** Description:
**      This structure maps the PCI header area for the Transport Layer 
**      Data (DT) PDU.
*/

struct _TL_DT_PCI		/* Data (DT) TPDU PCI */
{
    u_char	li;			/* length indicator */

    /* Following is the fixed part of the DT TPDU */

    u_char	tpdu;			/* TPDU code */

# define TDT_TPDU		0xF0	/* DT TPDU code */

    u_i2	dst_ref;		/* Destination reference */
    u_char	nr_eot;			/* Send seq. no. and EOT indicator */

# define TDT_EOT_MASK		0x80	/* Mask for EOT portion */

# define SZ_TL_DT_PCI		(1+1+2+1)

}; /* TL_DT_PCI */


/*}
** Name: NL PCI - Network Layer Protocol Control Information layout
**
** Description:
**      This is a description of the Network Layer PCI.
**
**  +---------------------------+
**  |         pad area          |
**  +---------------------------+
**  0                          16
**
**  pad area - for possible use by a protocol driver as a
**  hidden header area if needed by idiosynchrasies of the particular
**  protocol.  An example is TCP/IP, which provides a stream interface, and
**  may split up packets in arbitrary ways.  It is necessary for the sender
**  to prepend a packet size count so the receiver can fetch the complete
**  TPDU, possibly in several pieces.  There may be other uses by other
**  protocols.  The use is hidden from mainline view.
*/

# define SZ_NL_PCI		16


/*
** Some layers need to know certain offsets into the PDU;  they are:
**
**	GCC_OFF_TPDU	The beginning of the TPDU: because of the irregular
**			design of the NL, the TL doesn't issue NL reads
**			from the start of the NPDU, but rather from the 
** 			start of the TPDU.
**
*/

# define GCC_OFF_TPDU		SZ_NL_PCI

/*
**	GCC_OFF_APP	The offset end of the largest potential combined
**	    	        NL, TL, SL, PL and AL PCI's. This combination
**	    	    	occurs on an A-Associate request. For most PDU's, AL 
**			data is built forward from this offset and PCI's
**	    	        are built backward from this offset. 
*/

# define GCC_OFF_APP		(SZ_NL_PCI + SZ_TL_DT_PCI + \
				SZ_CN_PCI + SZ_CP_PCI + SZ_AARQ_PCI)

/*
**	GCC_OFF_DATA	The offset of the end of the user data carrying NL,
**			TL, SL and PL PCI's (AL has no PCI for user data).
**			For user data carrying PDU's, user data is built
**			forward from this offset and PCI's are built backward
**			from this offset.
*/ 

# define GCC_OFF_DATA		(SZ_NL_PCI + SZ_TL_DT_PCI + \
				SZ_DT_PCI + SZ_PDVN)


/**
**
** Description:
**      The GCC protcol control information (PCI) for the AL, PL, SL
**	and TL layers used to be contained in C structures.  This created
**	porting problems on certain environments.  These macros have been
**	wriiten to move the PCI into the mde so as not to cause alignment
**	problems.
**
*/

# define GCaddr(src,dst,off,roll) (((unsigned char *)(dst))[off]=(src)>>(roll))
# define GCgetr(src,off,roll)	  (((unsigned char *)(src))[off]<<roll)


#ifndef ds3_ulx

# define GCgeti1(src, dst)  ((*(dst) = GCgetr(src,0,0)), 1)
# define GCgeti2(src, dst)  ((*(dst) = GCgetr(src,0,8)+GCgetr(src,1,0)), 2)
# define GCgeti4(src, dst)  ((*(dst) = GCgetr(src,0,24) + GCgetr(src,1,16) +\
				       GCgetr(src,2,8) + GCgetr(src,3,0)), 4)

# define GCaddi1(src, dst)  (GCaddr(*src,dst,0,0), 1)
# define GCaddi2(src, dst)  (GCaddr(*src,dst,0,8), GCaddr(*src,dst,1,0), 2)
# define GCaddi4(src, dst)  (GCaddr(*src,dst,0,24), GCaddr(*src,dst,1,16), \
			     GCaddr(*src,dst,2,8), GCaddr(*src,dst,3,0), 4)

# define GCaddn1(src, dst)  (GCaddr(src,dst,0,0), 1)
# define GCaddn2(src, dst)  (GCaddr(src,dst,0,8), GCaddr(src,dst,1,0), 2)
# define GCaddn4(src, dst)  (GCaddr(src,dst,0,24), GCaddr(src,dst,1,16), \
			     GCaddr(src,dst,2,8), GCaddr(src,dst,3,0), 4)

/*
**  Put a string to the PCI
*/
# define GCadds(src,len,dest) \
		(MEcopy((PTR)(src),(len),(PTR)(dest)),  len) 

/*
**  Put a string to the PCI
*/
# define GCgets(src,len,dest) \
		(MEcopy((PTR)(src),(len),(PTR)(dest)),  len) 

/*
**  Put a null string to the PCI
*/
# define GCaddns(len,dest) \
		(MEfill((len), (u_char) 0,(PTR)(dest)),  len) 

#else

/*
 * Turned the following 6 defines into a?b:c statements because the dec
 * compiler couldn't handle x+=(a,b) correctly. This is a bug in the
 * compiler and dec hasn't fixed it yet.
 *
 */

# define GCgeti1(src, dst)  ((*(dst) = GCgetr(src,0,0)) ? 1 : 1)
# define GCgeti2(src, dst)  ((*(dst) = GCgetr(src,0,8)+GCgetr(src,1,0)) ? 2 : 2)
# define GCgeti4(src, dst)  ((*(dst) = GCgetr(src,0,24)+GCgetr(src,1,16))+\
				       GCgetr(src,2,8)+GCgetr(src,3,0) ? 4 : 4)

# define GCaddi1(src, dst)  (GCaddr(*src,dst,0,0) ? 1 : 1)
# define GCaddi2(src, dst)  ((GCaddr(*src,dst,0,8) & GCaddr(*src,dst,1,0)) ? 2 : 2)
# define GCaddi2(src, dst)  ((GCaddr(*src,dst,0,24) & GCaddr(*src,dst,1,16)) &\
			    GCaddr(*src,dst,2,8) & GCaddr(*src,dst,3,0) ? 4 : 4)

# define GCaddn1(src, dst)  (GCaddr(src,dst,0,0) ? 1 : 1)
# define GCaddn2(src, dst)  ((GCaddr(src,dst,0,8) & GCaddr(src,dst,1,0)) ? 2 : 2)
# define GCaddn2(src, dst)  ((GCaddr(src,dst,0,24) & GCaddr(src,dst,1,16)) &\
			    GCaddr(src,dst,2,8) & GCaddr(src,dst,3,0) ? 4 : 4)


/*
 * because of a dec compiler bug, had to substitute the "real" memcpy for
 * MEcopy().  This is because MEcopy is declared void and the a?b:c
 * statement has to be able to evaluate 'a' before deciding on b or c.
 * A void MEcopy returns nothing and hence can't be evaluated...
 * 
 */
/*
**  Put a string to the PCI
*/
# define GCadds(src,len,dest) \
	(memcpy(dest, src, (int) len) ? len : len) 

/*
**  Put a string to the PCI
*/
# define GCgets(src,len,dest) \
 	(memcpy(dest, src, (int) len) ? len : len) 

/*
**  Put a null string to the PCI
*/
# define GCaddns(len,dest) \
	(memset((PTR)dest ,(u_char)0, len) ? len : len)

#endif /* ds3_ulx */

/*
** Macros to read/write a SL length indicator 
** which may be in one of two forms: a standard 
** one octet indicator or an extended indicator 
** of 2 octets preceded by an octect containing 
** an extension indicator, SL_XLI.
*/
# define SL_ADD_LI( src, dst ) \
    if ( (src) < 255 ) \
	(dst) += GCaddn1( (src), (dst) ); \
    else \
    { \
	(dst) += GCaddn1( SL_XLI, (dst) ); \
	(dst) += GCaddn2( (src), (dst) ); \
    }

# define SL_GET_LI( src, dst ) \
{ \
    u_char _li; \
    (src) += GCgeti1( (src), &_li ); \
    if ( _li != SL_XLI ) \
	(dst) = _li; \
    else \
    { \
	u_i2 _xi; \
	(src) += GCgeti2( (src), &_xi ); \
	(dst) = _xi; \
    } \
}

