/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/**
** Name: GCCPL.H - Data structures and constants for Presentation Layer
**
** Description:
**      GCCPL.H contains definitions of constants and data structures which
**      are specific to the Presentation Layer.
**
** History:
**      25-Jan-88 (jbowers)
**          Initial module creation
**	06-Jul-89 (jorge/cmorris)
**	    introduce unique PL_PROTOCOL_VRSN
**      06-Sep-89 (cmorris)
**          Updated PL_PROTOCOL_VRSN for 7.0.
**	12-Feb-90 (seiwald)
**	    Moved het support into gccod.h.
**      01-Aug-90 (bxk)
**          Bumped PL_PROTOCOL_VRSN for 8-bit ASCII NCS
**	1-Mar-91 (seiwald)
**	    Moved state machine into gccplfsm.roc.
**	20-May-91 (cmorris)
**	    Added events for release collision support.
**	2-Mar-91 (seiwald)
**	    New PL_PROTOCOL_VRSN_CSET for character set negotiation.
**	20-May-91 (seiwald)
**	    New GCC_ARCH_VEC, GCC_CSET_NAME, GCC_CSET_XLT for character set
**	    negotiation support.
**  	12-Aug-91 (cmorris)
**  	    Added OPTOD output event.
**	14-Aug-91 (seiwald)
**	    Renamed CONTEXT structure to PL_CONTEXT to avoid clash on OS/2.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**	6-Apr-92 (seiwald)
**	    Support for doublebyte character translation.
**	7-Apr-92 (seiwald)
**	    Use u_i2 instead of non-portable u_short.
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
**  	08-Dec-92 (cmorris)
**  	    Bumped PL Protocol version for GCA message concatentation.
**      22-Mar-93 (huffman)
**          Change "extern READONLY" to "GLOBALCONSTREF"
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Jan-10 (gordy)
**	    Move character translation specific items to GCO.
**/

#ifndef GCCPL_H_INCLUDED
#define GCCPL_H_INCLUDED

#include <gcocomp.h>


/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _GCC_ARCH_VEC GCC_ARCH_VEC;
typedef struct _GCC_CSET_NAME GCC_CSET_NAME;
typedef struct _GCC_CSET_XLT GCC_CSET_XLT;

/*
**  Defines of other constants.
*/


/*
**      Presentation Layer Finite State Machine constant definitions
*/

/*
**      States for FSM
*/

#define                 SPCLD		 0
#define                 SPIPL		 1
#define                 SPRPL		 2
#define                 SPOPN		 3
#define                 SPCLG		 4
#define                 SPMAX		 5

/*
**      Input events for Presentation Layer FSM
*/

#define                 IPPCQ            0
#define                 IPPCA            1
#define                 IPPCJ            2
#define                 IPPAU            3
#define                 IPPDQ            4
#define                 IPPEQ		 5
#define                 IPPRQ            6
#define                 IPPRS            7
#define                 IPSCI            8
#define                 IPSCA            9
#define                 IPSCJ           10
#define                 IPSAP           11
#define                 IPARU           12
#define                 IPARP           13
#define                 IPSDI           14
#define                 IPSEI           15
#define                 IPSRI           16
#define                 IPSRA           17
#define                 IPSRJ           18
#define                 IPPDQH          19
#define			IPPRSC		20
#define			IPSRAC		21
#define                 IPPEQH          22
#define                 IPSDIH          23
/*					24 */
#define                 IPSEIH          25
#define                 IPABT           26
#define                 IPPXF		27
#define                 IPPXN		28
#define                 IPSXF		29
#define                 IPSXN		30
#define                 IPMAX		31

/*
**      Output events for Presentation Layer FSM
*/

#define                 OPPCI            1
#define                 OPPCA            2
#define                 OPPCJ            3
#define                 OPPAU            4
#define                 OPPAP            5
#define                 OPPDI		 6
#define                 OPPEI            7
#define                 OPPRI            8
#define                 OPPRC            9
#define                 OPSCQ           10
#define                 OPSCA           11
#define                 OPSCJ           12
#define                 OPARU		13
#define                 OPARP		14
#define                 OPSDQ           15
#define                 OPSEQ           16
#define                 OPSRQ           17
#define                 OPSRS           18
#define			OPSCR		19
#define			OPRCR		20
#define                 OPMDE		21
#define                 OPPDIH		22
#define                 OPPEIH          23
#define                 OPSDQH          24
#define                 OPSEQH          25
#define                 OPPXF		26
#define                 OPPXN		27
#define                 OPSXF		28
#define                 OPSXN		29
#define	    	    	OPTOD	    	30

/*
**      Definitions for constructing Presentation Layer FSM
*/

#define                 PL_OUT_MAX	4   /* max # output events/entry */

/*
**	Current PL protocol level.
*/

#define                 PL_PROTOCOL_VRSN_63	0x0700
#define                 PL_PROTOCOL_VRSN_8BIT   0x0701
#define			PL_PROTOCOL_VRSN_CSET	0x0702
#define	    	    	PL_PROTOCOL_VRSN_CONCAT 0x0703
#define                 PL_PROTOCOL_VRSN	0x0703

/*
** External references.
*/

GLOBALCONSTREF char	    plfsm_tbl[IPMAX][SPMAX];

GLOBALCONSTREF struct   _plfsm_entry
{
    char		    state;
    char		    output[PL_OUT_MAX];
} plfsm_entry[];

/*
**	Datatype designation octet positions within the
**	architecture characteristic vector.  (Historical).
*/

#define                 DDOC_INT         0	/* Integer type */
#define                 DDOC_FLT         1	/* Float type   */
#define                 DDOC_NCSJ        2	/* National Char. Set (Major)*/
#define                 DDOC_NCSN        3	/* National Char. Set (Minor)*/

/* Heterogenous data masks (for use with "het_mask" in "ccb_pce") */

# define                 GCC_HOMOGENEOUS                0x0000
# define                 GCC_INTEGER_HET                0x0001
# define                 GCC_FLOAT_HET                  0x0002
# define                 GCC_CHARACTER_HET              0x0004


/*
** Name: GCC_ARCH_VEC - layout of architecture vector (PL_CONTEXT)
**
** Description:
**	This structure redefines the transfer syntax area of the
**	PL_CONTEXT structure exchanged between PL peers in connection
**	initiation.
**	
**	If any of the first three elements (int_type, flt_type,
**	char_type) differ between the peer PL's, subsequent messages
**	will be converted to NTS (network transfer syntax) by the PL
**	before transmission and converted back to the local syntax upon
**	receipt.
**
**	This first three elements are compatible with the use of the
**	PL_CONTEXT structure prior to PL_PROTOCOL_VRSN_CSET.  The int_type
**	and flt_type remain unchanged; char_type was consolidated from
**	two u_char's into a single u_i2, while maintaining the same
**	representation on the network.
**
**	Char_vec is ignored by by PL's below PL_PROTOCOL_VRSN_CSET, and
**	enumerates the negotiable network character sets on connection
**	request and the negotiated network character set on connection
**	confirmation.
**
**	This structure must remain <= 16 (GCC_L_SYNTAX_NM) bytes.
**
** History:
**	7-Mar-91 (seiwald)
**	    Created.
**	30-Aug-1995 (sweeney)
**	    Fixed a typo -- this stuff is confusing enough as it is.
*/

# define GCC_L_CHAR_VEC 6

struct _GCC_ARCH_VEC
{
       u_char	int_type;	/* integer type (see cvgcc.h CV_*_INT) */
       u_char	flt_type;	/* float type (see cvgcc.h CV_*_FLT) */
       u_i2	char_type;	/* char type */
       u_i2	char_vec[ GCC_L_CHAR_VEC ];	/* char type vec */
};


/*
** Name: GCC_CSET_NAME - character set name <-> id mapping
**
** Description:
**	This struction contains a single mapping between a character set
**	name and its u_i2 id.  These structures are built by gcc_read_cname()
**	and QUEUE'd together onto IIGCc_global->gcc_cset_nam_q for access.
**
** History:
**	7-Mar-91 (seiwald)
**	    Created.
*/

struct _GCC_CSET_NAME
{
    QUEUE	q;		/* extant mappings chain */
    u_i2	id;		/* character set id */
    char	name[ 32 ];	/* character set name */
};

/* Default network standard character set id; others defined in gcccset.nam */

# define	GCC_ID_NSCS		(u_i2)0x0000


/*
** Name: GCC_CSET_XLT - character transliteration table
**
** Description:
**	This structure contains a mapping between two character sets.
**
**	These transliteration tables are built by gcc_read_cxlt() and 
**	QUEUE'd together onto IIGCc_global->gcc_cset_xlt_q.
**
** History:
**	7-Mar-91 (seiwald)
**	    Created.
**	6-Apr-92 (seiwald)
**	    Support for doublebyte character translation: pointers
**	    to doublebyte translation tables.
**	22-Jan-10 (gordy)
**	    Moved translation specific data to GCO.
*/

struct _GCC_CSET_XLT
{
	QUEUE	q;		/* extant tables chain */
	u_i2	lcl_id;		/* local char set - per installation */
	u_i2	net_id;		/* transfer char set - per table */
	i4	class;		/* preference of mapping */

# define ZERO_CLASS	0 	/* identity mappings (local to local) */
# define FIRST_CLASS	1	/* oddball special mappings */
# define SECOND_CLASS	2	/* mappings to group representative */
# define THIRD_CLASS	3	/* mappings to NSCS */

	GCO_CSET_XLT	xlt;	/* Character mapping */
};

#endif

