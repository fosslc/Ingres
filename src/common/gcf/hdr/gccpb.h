/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: GCCPB.H - Data structures and constants for Protocol Bridge
**
** Description:
**      GCCPB.H contains definitions of constants and data structures which
**      are specific to the Protocol Bridge.
**
** History: $Log-for RCS$
**      30-Apr-90 (cmorris)
**          Initial module creation
**      10-Jan-92 (cmorris)
**          Added IBNLSF event
**  	11-Mar-92 (cmorris)
**  	    Updated terminology in several comments
**  	10-Apr-92 (cmorris)
**  	    Added IBNCIP event
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
**      22-Mar-93 (huffman)
**          Change "extern READONLY" to "GLOBALCONSTREF"
**	06-Feb-96 (rajus01)
**	    Added states, actions, input events for Bridge Application Layer.
**	    Added _pbfsm_entry, _pbalfsm_entry structures.
**/

/*
**  Defines of other constants.
*/

# define			GCBAL_LSNRES 		0x200
# define			PBAL_PROTOCOL_VRSN_12	0x0102
# define			NULLSTR			""

/*
**      Protocol Bridge Finite State Machine constant definitions
*/

/*
**      States for Protocol Bridge FSM
*/

#define                 SBCLD		 0
#define			SBWNC		 1
#define			SBOPN		 2
#define			SBSDP		 3
#define			SBOPX		 4
#define			SBSDX		 5
#define			SBCLG		 6
#define                 SBMAX            7

/*
**      States for Protocol Bridge Application Layer FSM
*/

#define                 SBACLD		 0
#define			SBASHT		 1
#define			SBAQSC		 2
#define                 SBACLG		 3
#define                 SBARJA		 4
#define                 SBAMAX           5

/*
**      Input events for Protocol Bridge FSM
*/
#define                 IBOPN		 0
#define                 IBNOC            1
#define                 IBNRS            2
#define                 IBNCI            3
#define                 IBNCQ            4
#define                 IBNCC            5
#define                 IBNDR            6
#define                 IBNDI            7
#define                 IBNDC		 8
#define                 IBSDN            9
#define			IBDNI		10
#define                 IBDNC           11
#define	    	    	IBDNQ	    	12
#define                 IBABT           13
#define                 IBXOF           14
#define                 IBXON		15
#define                 IBRDX		16
#define                 IBSDNQ		17
#define                 IBDNX           18
#define			IBNCCQ		19
#define	    	    	IBNLSF	    	20
#define	    	    	IBNCIP	    	21
#define	    	    	IBNCLUP	    	22
#define                 IBMAX	 	23	

/*
**      Input events for  Protocol Bridge Application Layer FSM
*/

#define	    	    	IBALSN	    	0   /* Bridge Application (BA) LSN */
#define	    	    	IBAABQ	    	1
#define	    	    	IBAACE	    	2
#define	    	    	IBAARQ	    	3
#define	    	    	IBASHT	    	4
#define	    	    	IBAQSC	    	5
#define	    	    	IBAABT	    	6
#define	    	    	IBAAFN	        7	
#define	    	    	IBALSF	        8	
#define	    	    	IBAREJI	        9	
#define                 IBAMAX		10	

/*
**      Output events for Protocol Bridge FSM
*/

#define                 OBNCQ		 1
#define                 OBNDR		 2
#define                 OBSDN		 3
#define                 OBXOF		 4
#define                 OBXON		 5

/*
**      Actions for Protocol Bridge FSM
*/

#define                 ABCNC            1
#define                 ABDCN            2
#define                 ABMDE            3
#define                 ABCCB            4
#define                 ABLSN            5
#define                 ABRVN            6
#define                 ABRVE            7
#define                 ABSDN            8
#define                 ABNCQ		 9
#define                 ABOPN		10
#define                 ABSNQ		11
#define                 ABSDQ		12
#define                 ABSXF		13
#define                 ABRXF		14
#define                 ABNDR		15


#define                 ABALSN		16 /* Bridge Application (BA)*/
#define                 ABAMDE		17
#define                 ABACCB		18
#define                 ABADIS		19
#define                 ABARSP	  	20	
#define                 ABASHT	  	21	
#define                 ABAQSC	  	22	
/*
**      Definitions for constructing Protocol Bridge FSM
*/

#define                 PB_OUT_MAX	2   /* max # output events/entry */
#define                 PB_ACTN_MAX	4   /* max # actions/entry */

/*
**      Miscellaneous constants
*/

GLOBALCONSTREF    char	    pbfsm_tbl[IBMAX][SBMAX];

GLOBALCONSTREF struct _pbfsm_entry
{
    char		    state;
    char		    output[PB_OUT_MAX];
    char		    action[PB_ACTN_MAX];
} pbfsm_entry[];

GLOBALCONSTREF    char	    pbalfsm_tbl[IBAMAX][SBAMAX];

GLOBALCONSTREF struct _pbalfsm_entry
{
    char		    state;
    char		    output[PB_OUT_MAX];
    char		    action[PB_ACTN_MAX];
} pbalfsm_entry[];

