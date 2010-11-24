/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/
 
/**
** Name: GCCAL.H - Data structures and constants for Application Layer
**
** Description:
**      GCCAL.H contains definitions of constants and data structures which
**      are specific to the Application Layer.
**
** History:
**      25-Jan-88 (jbowers)
**          Initial module creation
**	06-Jul-89 (jorge/cmorris)
**	    introduce unique AL_PROTOCOL_VRSN
**	28-Dec-90 (seiwald)
**	    New defines for versions 6.3 and 6.4.
**  	02-Jan-91 (cmorris)
**  	    Bumped protocol version to 6.4.
**	1-Mar-91 (seiwald)
**	    Moved state machine into gccalfsm.roc.
**	21-May-91 (cmorris)
**	    Added state/events for release collision handling.
**	17-Jul-91 (seiwald)
**	    New define GCC_GCA_PROTOCOL_LEVEL, the protocol level at
**	    which the Comm Server is running.  It may be distinct from
**	    the current GCA_PROTOCOL_LEVEL.
**	28-Jan-92 (seiwald)
**	    Support for installation passwords: GCC_GCA_PROTOCOL_LEVEL
**	    moved up to GCA_PROTOCOL_LEVEL_55.
**  	06-Oct-92 (cmorris)
**  	    Got rid of events and actions associated with command session.
**	9-Nov-92 (seiwald)
**	    Support 6.5 protocol with GCA_PROTOCOL_LEVEL_60.
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
**  	27-Nov-92 (cmorris)
**  	    Changed IAINT to IALSN for readability, removed IAREJM and IAAFNM.
**  	30-Nov-92 (cmorris)
**  	    Bumped protocol version to 6.5.
**  	08-Dec-92 (cmorris)
**  	    Added SAFSL, IAACEF, IAPAM and AAFSL for fast select support.
**	22-Mar-93 (huffman)
**	    Change "extern READONLY" to "GLOBALCONSTREF"
**	05-Apr-93 (robf)
**	    Secure 2.0: Added handling for security labels in connections,
**	14-nov-95 (nick)
**	    Increase GCC_GCA_PROTOCOL_LEVEL to GCA_PROTOCOL_LEVEL_62
**	20-Mar-97 (gordy)
**	    Raised GCA protocol level to 63 to support Name Server
**	    communications accros the net.
**	 3-Jun-97 (gordy)
**	    Moved initial connection info to gcc.h
**	 5-Sep-97 (gordy)
**	    Added new protocol level for SL processing of connection info.
**	 1-Mar-01 (gordy)
**	    New GCA protocol level for Unicode datatypes.
**	15-Mar-04 (gordy)
**	    New GCA protocol level  for eight-byte integers.
**	15-Sept-04 (wansh01)
**	    Added new state SACMD input IACMD action AACMD to support gcc/admin.
**	 4-Nov-05 (gordy)
**	    Originally, FASTSELECT processing was handled by a single
**	    data transfer state.  This didn't allow for message queuing
**	    and send collisions could occur when the FASTSELECT data 
**	    was just large enough to overflow an internal buffer.  This
**	    change switches the FASTSELECT state to listen completion
**	    and adds a companion state for the response.  A new input
**	    drives the transition on the listen completion, but then
**	    standard inputs drive the proper actions from the new states.
**	    The normal data transfer states are now used, after the
**	    initial FASTSELECT data is used to 'prime the pump'.  The
**	    original special input which drove the transition to the
**	    FASTSELECT state after the listen response is no longer
**	    needed.
**	16-Jun-06 (gordy)
**	    Bumped GCA protocol level for ANSI date/time.
**	24-Oct-06 (gordy)
**	    Bumped GCA protocol level for Blob/Clob locators.
**	 1-Oct-09 (gordy)
**	    Bumped GCA protocol level for long procedure names.
**	 30-Aug-10 (thich01)
**	    Bumped GCA protocol level for spatial types.
*/

# define GCC_GCA_PROTOCOL_LEVEL	GCA_PROTOCOL_LEVEL_69


/*
**  Defines of other constants.
*/
 
/*
**	Current AL protocol level.
**	The most significant byte is the major release number and the
**	the least significant byte is the minor release number. Note
**		Release 6.1 is 0x0.
*/
 
#define	AL_PROTOCOL_VRSN_61     0x0000
#define	AL_PROTOCOL_VRSN_62	0x0602
#define	AL_PROTOCOL_VRSN_63	0x0603	/* GCA protocol in connect data */
#define	AL_PROTOCOL_VRSN_64	0x0604
#define	AL_PROTOCOL_VRSN_65 	0x0605	/* GCA flags in connect user data */
#define	AL_PROTOCOL_VRSN_66	0x0606	/* SL handles connect user data */

#define AL_PROTOCOL_VRSN	AL_PROTOCOL_VRSN_66
/*
**      Application Layer Finite State Machine constant definitions
*/
 
/*
**      States definitions
*/
 
#define                 SACLD             0
#define                 SAIAL             1
#define                 SARAL             2
#define                 SADTI             3
#define                 SADSP             4
#define                 SACTA             5
#define                 SACTP             6
#define                 SASHT             7
#define                 SADTX		  8
#define                 SADSX             9
#define                 SARJA             10
#define                 SARAA             11
#define                 SAQSC             12
#define                 SACLG             13
#define                 SARAB		  14
#define			SACAR		  15
#define	    	    	SAFSL	    	  16
#define                 SARAF		  17
#define                 SACMD		  18
#define                 SAMAX		  19
 
/*
**      Input events for Application Layer FSM
*/
#define                 IALSN           0
#define                 IAAAQ           1
#define                 IAABQ           2
#define                 IAASA           3
#define                 IAASJ           4
#define                 IAPCI		5
#define                 IAPCA           6
#define                 IAPCJ           7
#define                 IAPAU           8
#define                 IAPAP           9
#define                 IAANQ           10
#define                 IAAEQ           11
#define                 IAACE           12
#define                 IAACQ           13
#define                 IAPDN           14
#define                 IAPDE           15
#define                 IAARQ           16
#define                 IAPRII          17
#define                 IAPRA           18
#define                 IAPRJ           19
#define                 IALSF		20
#define                 IAREJI		21
#define                 IAAPG		22
#define                 IAPDNH          23
#define			IASHT		24
#define			IAQSC		25
#define                 IAABT		26
#define                 IAXOF		27
#define                 IAXON		28
#define                 IAANQX		29
#define                 IAPDNQ          30
#define                 IAACEX		31
#define                 IAAPGX          32
#define                 IAABQX          33
#define                 IAABQR		34
#define                 IAAFN           35
#define			IAREJR		36
#define			IAPRIR		37
#define                 IAAAF		38
#define	    	    	IAPAM	    	39
#define                 IACMD           40
#define                 IAMAX           41
 
/*
**      Output events for Application Layer FSM
*/
 
#define                 OAPCQ		1
#define                 OAPCA           2
#define                 OAPCJ           3
#define                 OAPAU           4
#define                 OAPDQ           5
#define                 OAPEQ           6
#define                 OAPRQ           7
#define                 OAPRS           8
#define                 OAXOF		9
#define                 OAXON		10
 
/*
**      Actions for Application Layer FSM
*/
 
#define                 AARQS           1
#define                 AARVN           2
#define                 AARVE           3
#define                 AADIS           4
#define                 AALSN           5
#define                 AASND           6
#define                 AARLS           7
#define                 AAENQ		8
#define                 AADEQ           9
#define                 AAPGQ           10
#define                 AAMDE           11
#define                 AACCB           12
#define                 AASNDG          13
#define                 AASHT		14
#define                 AAQSC		15
#define                 AASXF		16
#define                 AARXF		17
#define                 AARSP           18
#define                 AAENQG          19
#define	    	    	AAFSL	    	20
#define	    	    	AACMD	    	21
 
/*
**      Definitions for constructing Application Layer FSM
*/
 
#define                 AL_OUT_MAX	2   /* max # output events/entry */
#define                 AL_ACTN_MAX	4   /* max # actions/entry */
 
/*
**      Miscellaneous constants
*/
 
# define      GCC_TIMEOUT             3000    /* timeout (ms) */


/*}
** Name: GCC_AASRE_DATA - A_ASSOCIATE RESPONSE user data structure
**
** Description:
**	This element specifies the structure of the user data contained in an
**	A_ASSOCIATE RESPONSE service primitive. 
**
** History:
**      01-Aug-89 (cmorris)
**          Initial structure implementation.
**      31-Oct-89 (cmorris)
**          Removed SZ_STATUS define.
*/
typedef struct _GCC_AASRE_DATA
{
    u_i2	    gcc_gca_protocol;
    STATUS          gcc_generic_status;     /* Generic status */
}   GCC_AASRE_DATA;


 
GLOBALCONSTREF    char	    alfsm_tbl[IAMAX][SAMAX];
 
GLOBALCONSTREF    struct   _alfsm_entry
{
    char		    state;
    char		    output[AL_OUT_MAX];
    char		    action[AL_ACTN_MAX];
} alfsm_entry[];
