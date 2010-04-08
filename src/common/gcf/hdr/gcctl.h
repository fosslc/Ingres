/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: GCCTL.H - Data structures and constants for Transport Layer
**
** Description:
**      GCCTL.H contains definitions of constants and data structures which
**      are specific to the Transport Layer.
**
** History: $Log-for RCS$
**      25-Jan-88 (jbowers)
**          Initial module creation
**      07-Dec-88 (jbowers)
**          Added ITABT input event.
**      22-Dec-88 (jbowers)
**	    Fixed bug wherein TL issues a LISTEN request even when a network
**	    OPEN has failed.  Added ITNRS input event.
**      04-Jan-89 (jbowers)
**          Added ITCSD and ITCRV input event.
**      14-Jan-89 (jbowers)
**          Added intersections (16,0) and (19,0)
**      18-Jan-89 (jbowers)
**          Added valid intersections to accommodate certain obscure error
**	    conditions. 
**      16-Feb-89 (jbowers)
**          Add local flow control: expand FSM to handle XON and XOFF
**	    primitives. 
**      18-APR-89 (jbowers)
**          Added handling of (16, 5) intersection to FSM.
**      13-Dec-89 (cmorris)
**          Fix fsm entry 26 to remove memory leak.
**	12-Jan-90 (seiwald)
**	     Ironed out ownership of MDE's: an MDE now belongs to exactly 
**	     one layer at a time.  See change comments in gccal.c.
**	07-Feb-90 (cmorris)
**	     FSM changes for network connect holes.
**	08-Feb-90 (cmorris)
**	     Fixed FSM to do disconnect after listen failure.
**  	17-Dec-90 (cmorris)
**  	    Added STWND state and associated state transitions to fix
**  	    release on underlying network connection when after we have
**  	    received a DC TPDU from our peer.
**  	17-Dec-90 (cmorris)
**  	    Renamed STCLG to STWDC and cleaned up a handful of crappy state
**  	    transitions in this area.
**	1-Mar-91 (seiwald)
**	    Moved state machine into gcctlfsm.roc.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to multiplex.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to implement peer
**	    Transport layer flow control.
**      09-Jul-91 (cmorris)
**	    Split STCLD state into STCLD, STWCR abd STCLG. Fixes a number
**          of disconnect problems.
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
**  	08-Jan-93 (cmorris)
**  	    Removed unused input events.
**  	04-Feb-93 (cmorris)
**  	    Removed unused output events/actions.
**      22-Mar-93 (huffman)
**          Change "extern READONLY" to "GLOBALCONSTREF"
**  	03-Jun-97 (rajus01)
**  	    Added ATSDP action for encrypted send in TL.
**	20-Aug-97 (gordy)
**	    Added OTTDR (superset of OTTDI) to read TDR TPDU.
**	    Moved some definitions to gccpci.h.
**/

/*
**      Transport Layer Finite State Machine constant definitions
*/

/*
**      States for Transport Layer FSM
*/

#define                 STCLD		 0
#define                 STWNC            1
#define                 STWCC            2
#define			STWCR		 3
#define                 STWRS            4
#define                 STOPN            5
#define                 STWDC            6
#define                 STSDP		 7
#define                 STOPX            8
#define                 STSDX            9
#define                 STWND            10
#define			STCLG		 11
#define                 STMAX            12

/*
**      Input events for Transport Layer FSM
*/
#define                 ITOPN		 0
#define                 ITNCI            1
#define                 ITNCC            2
#define                 ITTCQ		 3
#define                 ITCR             4
#define                 ITCC             5
#define                 ITTCS            6
#define                 ITTDR		 7
#define                 ITNDI            8
#define                 ITNDC            9
#define                 ITTDA           10
#define                 ITDR		11
#define                 ITDC		12
#define                 ITDT		13
#define                 ITNC		14
#define                 ITNCQ		15
#define                 ITNOC		16
#define                 ITABT		17
#define                 ITNRS		18
#define                 ITXOF		19
#define                 ITXON		20
#define                 ITDTX		21
#define                 ITTDAQ          22
#define                 ITNCX		23
#define			ITNLSF		24
#define                 ITMAX		25

/*
**      Output events for Transport Layer FSM
*/

#define                 OTTCI		 1
#define                 OTTCC		 2
#define			OTTDR		 3
#define                 OTTDI		 4
#define                 OTTDA		 5
#define                 OTCR		 6
#define                 OTCC		 7
#define                 OTDR		 8
#define                 OTDC		 9
#define                 OTDT		10
#define                 OTXOF		11
#define                 OTXON		12

/*
**      Actions for Transport Layer FSM
*/

#define                 ATCNC            1
#define                 ATDCN            2
#define                 ATMDE            3
#define                 ATCCB            4
#define                 ATLSN            5
#define                 ATRVN            6
#define                 ATSDN            7
#define                 ATOPN		 8
#define                 ATSNQ		 9
#define                 ATSDQ		10
#define                 ATPGQ		11
#define                 ATSXF		12
#define                 ATRXF		13
#define                 ATSDP           14 

/*
**      Definitions for constructing Transport Layer FSM
*/

#define                 TL_OUT_MAX	2   /* max # output events/entry */
#define                 TL_ACTN_MAX	4   /* max # actions/entry */

/*
**      Miscellaneous constants
*/

GLOBALCONSTREF	char	    tlfsm_tbl[ITMAX][STMAX];

GLOBALCONSTREF	struct _tlfsm_entry
{
    char		    state;
    char		    output[TL_OUT_MAX];
    char		    action[TL_ACTN_MAX];
} tlfsm_entry[];
