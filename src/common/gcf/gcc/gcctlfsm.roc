/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<qu.h>
# include   	<tm.h>
# include	<gcccl.h>
# include	<gcctl.h>

/**
** Name: GCCTLFSM.ROC - State machine for Transport Layer
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
**	    Extracted from GCCPL.H.
**      28-May-91 (cmorris)
**          Purge send queue on network disconnections and internal aborts.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to multiplex.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to implement peer
**	    Transport layer flow control.
**      09-Jul-91 (cmorris)
**	    Split STCLD state into STCLD, STWCR abd STCLG. Fixes a number
**          of disconnect problems.
**	29-Jul-91 (cmorris)
**	    In state STCLG, we still have to handle and chuck away a number
**	    of send/receive related input events until such time as all
**	    protocol drivers are updated.
**  	13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**  	15-Aug-91 (cmorris)
**  	    Don't send XOFF/XON up after a disconnect indication/request.
**  	06-Sep-91 (cmorris)
**  	    Don't free CCB on abort when we're already in STCLD; the CCB has
**  	    already been freed to get to this state.
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
**  	08-Jan-93 (cmorris)
**  	    Removed unused input events.
**  	05-Feb-93 (cmorris)
**  	    Removed events relating to expedited data.
**	16-Feb-93 (seiwald)
**	    Don't bother including gcc.h since we don't need it.
**	20-Aug-97 (gordy)
**	    Added OTTDR to process DR TPDU prior to calling SL.
**	    Added new entry to repost receive in STWDC.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Removed LIBRARY jam hint.  This file is now included in the
**          GCC local library only.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	19-dec-2008 (joea)
**	    Replace GLOBALDEF const by GLOBALCONSTDEF.
**/


/*}
** Name: TLFSM_TBL - FSM matrix
**
**
** Description:
**      There are two static data structures which together
**	define the Finite State Machine (FSM) for the TL.  They are
**	statically initialized as read only structures.  They drive the
**	operation of the TL FSM.  They are both documented here, even though
**	they are separate structures.
**
**	The FSM is a sparse matrix.  The two table mechanism is a way to deal
**	with this by not wasting too much memory on empty matrix elements, but
**	at the same time provide a simple and reasonably quick lookup scheme.
**	The first table is a matrix of integers, of the  same configuration as
**	the FSM.  Each matrix element which represents a valid FSM state/input
**	combination contains an index into the second table.  The second table
**	is a linear array whose size is exactly equal to the number of valid
**	intersections in the FSM.  Each index entry in the first table
**	references a corresponding positional entry in the second table.  This
**	latter entry specifies the new state, output events and actions which
**	are associated with the state/input intersection.  The FSM matrix is
**	specified in the preamble to the module GCCTL.C. The description is
**	actually more complicated than the mechanism.
**	
**      the following structure TLFSM_TBL is the representation of the FSM
**	matrix.
**	
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
**	20-Aug-97 (gordy)
**	    Added OTTDR to process DR TPDU prior to calling SL.
**	    Added new entry to repost receive in STWDC.
*/

GLOBALCONSTDEF char
tlfsm_tbl[ITMAX][STMAX]
=
{
/*   0	   1     2     3     4     5	 6     7     8     9    10    11    	        */
/* STCLD STWNC STWCC STWCR STWRS STOPN STWDC STSDP STOPX STSDX STWND STCLG    	        */
   { 1,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00}, /* 00 ITOPN  */
   { 2,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00}, /* 01 ITNCI  */
   {00,    3,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00}, /* 02 ITNCC  */
   { 4,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00}, /* 03 ITTCQ  */
   {00,   00,   00,    7,   00,   00,   00,   00,   00,   00,   00,   00}, /* 04 ITCR   */
   {00,   00,    8,   00,   00,   00,   63,   00,   00,   00,   00,   00}, /* 05 ITCC   */
   {00,   00,   00,   00,    9,   00,   00,   00,   00,   00,   00,   00}, /* 06 ITTCS  */
   {00,   10,   13,   00,   13,   13,   00,   11,    5,   14,   00,   00}, /* 07 ITTDR  */
   {00,   17,   17,   10,   17,   17,   10,   62,   17,   62,   10,   36}, /* 08 ITNDI  */
   {00,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00,   19}, /* 09 ITNDC  */
   {00,   00,   00,   00,   00,   20,   00,   24,   40,   41,   00,   00}, /* 10 ITTDA  */
   {00,   00,   23,   00,   23,   23,   10,   44,   00,   00,   00,   36}, /* 11 ITDR   */
   {00,   00,   00,   00,   00,   00,   10,   00,   00,   00,   00,   36}, /* 12 ITDC   */
   {00,   00,   00,   00,   00,   27,   63,   18,   00,   00,   00,   36}, /* 13 ITDT   */
   {00,   00,   34,   00,   00,   00,   35,   30,   00,   45,   25,   36}, /* 14 ITNC   */
   {00,   00,   00,   00,   00,   00,   37,   15,   00,   46,   39,   36}, /* 15 ITNCQ  */
   {16,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00}, /* 16 ITNOC  */
   { 6,   17,   17,   10,   17,   17,   10,   62,   17,   62,   10,   19}, /* 17 ITABT  */
   {19,   33,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00}, /* 18 ITNRS  */
   {00,   00,   00,   00,   00,   47,   00,   48,   49,   50,   00,   00}, /* 19 ITXOF  */
   {00,   00,   00,   00,   00,   51,   00,   52,   53,   54,   00,   00}, /* 20 ITXON  */
   {00,   00,   00,   00,   00,   55,   00,   56,   00,   00,   00,   36}, /* 21 ITDTX  */
   {00,   00,   00,   00,   00,   00,   00,   60,   00,   61,   00,   00}, /* 22 ITTDAQ */
   {00,   00,   00,   00,   00,   00,   35,   58,   00,   59,   25,   36}, /* 23 ITNCX  */
   {12,	  00,   00,   00,   00,   00,   00,   00,   00,   00,   00,   00}  /* 24 ITNLSF */
};

/*}
** Name: TLFSM_ENTRY - FSM entry table
**
** Description:
**      This is the linear array containing the entries for the FSM matrix.  The
**	entries in the preceeding table TLFSM_TBL are index values
**	corresponding to entries in the current array.  The array size limits on
**	the individual elements of each array entry, TL_OUT_MAX and TL_ACTN_MAX
**	are set to values which are currently adequate.  They are confined in
**	scope to the TL.  If future expansion requires these to be increased,
**	this can be done without affecting anything outside TL.  If the values
**	are changed in GCCTL.H and the TL recompiled, everything will work.
**
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
**	06-Jun-97 (rajus01)
**	    Changed ATSDN to ATSDP for encrypted data sending in 
**	    the indexes 3, 9 of the tlfsm_entry.
**	20-Aug-97 (gordy)
**	    Added OTTDR to process DR TPDU prior to calling SL.
**	    Added new entry to repost receive in STWDC.
*/


GLOBALCONSTDEF struct _tlfsm_entry 
tlfsm_entry[]
=
{
 /* ( 0) */ {STCLD, {OTTDI, 0}, {ATDCN, 0, 0, 0}},/* Illegal FSM intersection */
 /* ( 1) */ {STCLD, {0, 0}, {ATOPN, 0, 0, 0}},
 /* ( 2) */ {STWCR, {0, 0}, {ATRVN, ATLSN, ATMDE, 0}},
 /* ( 3) */ {STWCC, {OTCR, 0}, {ATSDP, 0, 0, 0}},
 /* ( 4) */ {STWNC, {0, 0}, {ATCNC, 0, 0, 0}},
 /* ( 5) */ {STWDC, {OTDR, 0}, {ATSDN, ATRVN, 0, 0}},
 /* ( 6) */ {STCLD, {0, 0}, {ATMDE, 0, 0, 0}},
 /* ( 7) */ {STWRS, {OTTCI, 0}, {0, 0, 0, 0}},
 /* ( 8) */ {STOPN, {OTTCC, 0}, {ATRVN, 0, 0, 0}},
 /* ( 9) */ {STSDP, {OTCC, 0}, {ATRVN, ATSDP, 0, 0}},
 /* (10) */ {STCLG, {0, 0}, {ATDCN, 0, 0, 0}},
 /* (11) */ {STWDC, {OTDR, 0}, {ATSNQ, 0, 0, 0}},
 /* (12) */ {STCLG, {0, 0}, {ATLSN, ATDCN, 0, 0}},
 /* (13) */ {STWDC, {OTDR, 0}, {ATSDN, 0, 0, 0}},
 /* (14) */ {STWDC, {OTDR, 0}, {ATSNQ, ATRVN, 0, 0}},
 /* (15) */ {STSDP, {0, 0}, {ATSDQ, ATMDE, 0, 0}},
 /* (16) */ {STCLD, {0, 0}, {ATLSN, ATCCB, ATMDE, 0}},
 /* (17) */ {STCLG, {OTTDI, 0}, {ATDCN, 0, 0, 0}},
 /* (18) */ {STSDP, {OTTDA, 0}, {ATRVN, 0, 0, 0}},
 /* (19) */ {STCLD, {0, 0}, {ATCCB, ATMDE, 0, 0}},
 /* (20) */ {STSDP, {OTDT, 0}, {ATSDN, 0, 0, 0}},
 /* (21) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (22) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (23) */ {STWND, {OTTDR, OTDC}, {ATSDN, 0, 0, 0}},
 /* (24) */ {STSDP, {OTDT, 0}, {ATSNQ, 0, 0, 0}},
 /* (25) */ {STWND, {0, 0}, {ATRVN, ATMDE, 0, 0}},
 /* (26) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (27) */ {STOPN, {OTTDA, 0}, {ATRVN, 0, 0, 0}},
 /* (28) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (29) */ {STCLD,  {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (30) */ {STOPN, {0, 0}, {ATMDE, 0, 0, 0}},
 /* (31) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (32) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (33) */ {STCLD, {OTTDI, 0}, {ATCCB, ATMDE, 0, 0}},
 /* (34) */ {STWCC, {0, 0}, {ATRVN, ATMDE, 0, 0}},
 /* (35) */ {STWDC, {0, 0}, {ATMDE, 0, 0, 0}},
 /* (36) */ {STCLG, {0, 0}, {ATMDE, 0, 0, 0}},
 /* (37) */ {STWDC, {0, 0}, {ATSDQ, ATMDE, 0, 0}},
 /* (38) */ {STCLD, {0, 0}, {0, 0, 0, 0}},
 /* (39) */ {STWND, {0, 0}, {ATSDQ, ATMDE, 0, 0}},
 /* (40) */ {STSDX, {OTDT, 0}, {ATSDN, 0, 0, 0}},
 /* (41) */ {STSDX, {OTDT, 0}, {ATSNQ, 0, 0, 0}},
 /* (42) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (43) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (44) */ {STWND, {OTTDR, OTDC}, {ATPGQ, ATSNQ, 0, 0}},
 /* (45) */ {STOPX, {0, 0}, {ATMDE, 0, 0, 0}},
 /* (46) */ {STSDX, {0, 0}, {ATSDQ, ATMDE, 0, 0}},
 /* (47) */ {STOPN, {0, 0}, {ATSXF, ATMDE, 0, 0}},
 /* (48) */ {STSDP, {0, 0}, {ATSXF, ATMDE, 0, 0}},
 /* (49) */ {STOPX, {0, 0}, {ATSXF, ATMDE, 0, 0}},
 /* (50) */ {STSDX, {0, 0}, {ATSXF, ATMDE, 0, 0}},
 /* (51) */ {STOPN, {0, 0}, {ATRXF, ATMDE, 0, 0}},
 /* (52) */ {STSDP, {0, 0}, {ATRXF, ATMDE, 0, 0}},
 /* (53) */ {STOPN, {0, 0}, {ATRXF, ATRVN, ATMDE, 0}},
 /* (54) */ {STSDP, {0, 0}, {ATRXF, ATRVN, ATMDE, 0}},
 /* (55) */ {STOPX, {OTTDA, 0}, {0, 0, 0, 0}},
 /* (56) */ {STSDX, {OTTDA, 0}, {0, 0, 0, 0}},
 /* (57) */ {STCLD, {0, 0}, {0, 0, 0, 0}}, /* NOT USED */
 /* (58) */ {STOPN, {OTXON, 0}, {ATMDE, 0, 0, 0}},
 /* (59) */ {STOPX, {OTXON, 0}, {ATMDE, 0, 0, 0}},
 /* (60) */ {STSDP, {OTXOF, OTDT}, {ATSNQ, 0, 0, 0}},
 /* (61) */ {STSDX, {OTXOF, OTDT}, {ATSNQ, 0, 0, 0}},
 /* (62) */ {STCLG, {OTTDI, 0}, {ATPGQ, ATDCN, 0, 0}},
 /* (63) */ {STWDC, {0, 0}, {ATRVN, ATMDE, 0, 0}}
};
