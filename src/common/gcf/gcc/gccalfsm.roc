/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gccal.h>
 
/**
** Name: GCCALFSM.ROC - State machine for Application Layer
**
** History:
**      25-Jan-88 (jbowers)
**          Initial module creation
**      18-Jan-89 (jbowers)
**          Added valid intersections to accommodate certain obscure error
**	    conditions. 
**      10-Feb-89 (jbowers)
**          Added AADCN to support forcible abort of a GCA association 
**	    using GCA_DCONN instead of GCA_DISASSOC
**	    conditions. 
**      16-Feb-89 (jbowers)
**          Add local flow control: expand FSM to handle XON and XOFF
**	    primitives.  Separate a GCA_RELEASE message from the P_RELEASE SDU.
**      07-Mar-89 (jbowers)
**          Correct a FSM error.
**      13-Mar-89 (jbowers)
**          Fix for Bug 4985, failure to clean up aborted sessions: added input
**	    event IAPAPS to force disconnect when local session is with a
**	    server. 
**      16-Mar-89 (jbowers)
**	    Fix for failure to release an MDE on receipt of P_U_ABORT and
**	    P_P_ABORT.
**      17-Mar-89 (jbowers)
**	    Fix for race condition resulting in occasional failure to clean up
**	    connection w/ server.
**      17-APR-89 (jbowers)
**	    Fix for a termination race condition (FSM entry (13, 0)), and for
**	    failure to release MDE's on heterogeneous connections.
**      20-Jul_89 (cmorris)
**          change protocol version to GCC_AAS_DATA from u_i4 to u_char[4] for
**          portability reasons; create #define's for all versions of the
**          application protocol that we support. (6.1, 6.2, 7.0)
**      01-Aug-89 (cmorris)
**          changes to protocol machine for end to end bind.
**      02-Aug-89 (cmorris)
**          reversed (incorrect) transitions in SARAA and SARJA states on
**          IAACE input event.
**      08-Sep-89 (cmorris)
**          states, events and state transitions to fix fast selects from
**          name server.
**	26-Sep-89 (seiwald)
**	    Removed AADIS, AACCB actions from IAPRI event: they are driven
**	    by the completion of the AADCN action.
**      05-Oct-89 (cmorris)
**          Made changes to AL datastructures for byte alignment/network
**          encoding.
**      12-Oct-89 (cmorris)
**          State transition table changes to fix memory leak on GOTO_HET
**          message.
**      02-Nov-89 (cmorris)
**          Removed IAPCIQ and IAPCIM input events and OAIMX output event.
**      08-Nov-89 (cmorris)
**          Added define for disassociate timeout.
**      09-Nov-89 (cmorris)
**          State, event, action and state transition changes for disassociate
**          fixing.
**      10-Nov-89 (cmorris)
**          State transition changes to make quiesce work.
**      13-Nov-89 (cmorris)
**          Fixed state transition for IAABT in SACLG state.
**      14-Nov-89 (cmorris)
**          State transition for initialization now deletes CCB and MDE.
**      14-Dec-89 (cmorris)
**          State transition to inhibit AADIS action until send of
**          GCA_RELEASE message completes.
**	12-Jan-90 (seiwald)
**	     Ironed out ownership of MDE's: an MDE now belongs to exactly 
**	     one layer at a time.  See change comments in gccal.c.
**	01-Mar-90 (cmorris)
**           Renamed IAREJ to IAREJI (IAL reject), added IAREJR (RAL reject).
**           Added state transition for RAL rejection of incoming 
**	     association request.
**	02-Mar-90 (cmorris)
**	     Fixed two abort transitions.
**      05-Apr-90 (seiwald)
**          Enqueue expedited data if a send is in progress, that is,
**          on IAPDE in SADSP do an AAENQ not an AASND.  This fixes the
**          state table; the documentation was already correct.
**      3-Aug-90 (seiwald)
**          Introduced a new state SARAB (GCA_REQUEST completion
**          pending, P_ABORT received) to handle aborts during during
**          SARAL (waiting for GCA_REQUEST to complete).  Since it
**          currently is not possible to disconnect during GCA_REQUEST,
**          we must hang onto the abort and disconnect when GCA_REQUEST
**          completes.  (N.B. Version 6.3 handles this differently.)
**	    Also reshuffled state table to make it readable, albeit only
**	    on wide terminals.
**	8-Aug-90 (seiwald)
**	    Introduced two new input events, IAREJM (A_I_REJECT &&
**	    OB_MAX exceeded) and IAAFNM (A_FINISH request && OB_MAX
**	    exceeded), to defer posting a GCA_LISTEN when OB_MAX has
**	    been exceeded.
**  	28-Dec-90 (cmorris)
**  	    Added new state transitions in state SACLG to handle 
**  	    possible presentation indications that are legal but
**  	    which can be discarded.
**  	03-Jan-91 (cmorris)
**  	    Fixed several state transitions to purge pending
**  	    GCA_SEND message queue on receipt of aborts.
**  	03-Jan-91 (cmorris)
**  	    Fixed SARAB handling.
**  	04-Jan-91 (cmorris)
**  	    Handle internal aborts in state SACLD.
**  	10-Jan-91 (cmorris)
**  	    Handle internal aborts on command session.
**	1-Mar-91 (seiwald)
**	    Extracted from GCCAL.H.
**	02-May-91 (cmorris)
**	    Chuck away received GCA messages and XON requests in
**	    state SACTA; XOF requests in state SACTP.
**	09-May-91 (cmorris)
**	    Bizarrely, we can get XON/XOF requests in state SACLD. This is
**	    because the underlying transport connection is not released
**	    simultaneously with the session connection et al.
**	10-May-91 (cmorris)
**	    Chuck away XOFF requests in state SACTA, XON requests in 
**	    state SACTP, and both XON/XOFF in state SACLG.
**	14-May-91 (cmorris)
**	    If we're waiting for completion of send of GCA_RELEASE message,
**	    repost receives to avoid potential for hanging servers who have
**	    no normal receive posted.
**	20-May-91 (cmorris)
**	    Purge receive queue when we disassociate in state SACTA.
**	23-May-91 (cmorris)
**	    Added support for release collisions.
**  	14-Aug-91 (cmorris)
**  	    State transition fix:- a GCA send can complete after a P_RELEASE
**  	    has been requested.
**  	15-Aug-91 (cmorris)
**  	    We can no longer receive XON/XOFF in SACLD due to lower layer fix.
**  	06-Sep-91 (cmorris)
**  	    Don't free CCB on abort when we're already in SACLD; the CCB has
**  	    already been freed to get to this state.
**  	06-Oct-92 (cmorris)
**  	    Got rid of states, events and actions associated with
**          command session.
**	19-Nov-92 (seiwald)
**	    Changed nat's to char's in state table for compactness.
**  	27-Nov-92 (cmorris)
**  	    No longer execute AALSN from the state machine of another
**  	    connection. Also, changed IAINT to IALSN for readability.
**      08-Dec-92 (cmorris)
**  	    Added transitions for fast select.
**      24-Feb-94 (brucek)
**  	    For FASTSELECT processing, execute AARVE on completion of
**	    GCA_RQRESP, rather than of GCA_SEND, so as to prevent multiple
**	    receives being issued.
**	20-Nov-95 (gordy)
**	    Fix entry 92 to have output in correct list (not in action).
**	    Drop use of entry 82 since same as 22.
**	23-Aug-96 (gordy)
**	    Fast Select now sends a GCA_RELEASE message to end exchange.
**	    Added GCA normal channel receive to Fast Select action list
**	    (IAACEF, SARAA).  Added handling of A_RELEASE during fast
**	    select (IAARQ, SAFSL).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-Sept-2004 (wansh01)
**	    Added input (IACMD) state (SACMD) action(96, 97) to support
**	    GCADM/GCC interface. 
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Removed LIBRARY jam hint.  This file is now included in the
**          GCC local library only.
**	14-Dec-04 (gordy)
**	    GCA send completions can, on occasion, be reported following
**	    events which would seem to come after, such as a receive
**	    completion of the message sent in response to the outstanding
**	    send request.  This is unexpected and requires adding entries
**	    for these events in states where it is not readily apparent
**	    that it is needed. Example: IAACEF in SAFSL would stay in
**	    SAFSL and then IAARQ would move to SACTP.  If IAARQ occurs
**	    first, then IAACEF in SACTP must be handled.
**       4-Nov-05 (gordy)
**          Originally, FASTSELECT processing was handled by a single
**          data transfer state (SAFSL).  This didn't allow for message 
**	    queuing (SADSP) and send collisions could occur when the 
**	    FASTSELECT data was just large enough to overflow an internal 
**	    buffer.  This change switches the FASTSELECT state to listen 
**	    completion (instead of SAIAL) and adds a companion state 
**	    (SARAF instead of SARAA) for the response.  A new input
**	    (IAAAF) drives the transition on the listen completion, but 
**	    then standard inputs (IAPCA, IAACE, IAANQ, etc) drive the 
**	    proper actions (AAFSL, AARVN) from the new states.  The 
**	    normal data transfer states (SADTI, SADSP) are now used, 
**	    after the initial FASTSELECT data is used to 'prime the pump'
**	    (AAFSL).  The original special input (IAACEF) which drove 
**	    the transition to the FASTSELECT state after the listen 
**	    response is no longer used.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	19-dec-2008 (joea)
**	    Replace GLOBALDEF const by GLOBALCONSTDEF.
**/


/*}
** Name: ALFSM_TBL - FSM matrix
**
**
** Description:
**      There are two static data structures which together
**	define the Finite State Machine (FSM) for the AL.  They are
**	statically initialized as read only structures.  They drive the
**	operation of the AL FSM.  They are both documented here, even though
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
**	specified in the preamble to the module GCCAL.C. The description is
**	actually more complicated than the mechanism.
**	
**      The following structure AL_FSM_TBL is the representation of the FSM
**	matrix.  
**
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
**	23-Aug-96 (gordy)
**	    Fast Select now sends a GCA_RELEASE message to end exchange.
**	    Added handling of A_RELEASE during fast select (IAARQ, SAFSL).
**	 4-Nov-05 (gordy)
**	    Added state SARAF and changed SAFSL from a data transfer
**	    state to listen completion.  Input IAACEF replaced with
**	    IAAAF
*/
 
 
GLOBALCONSTDEF char 
alfsm_tbl[IAMAX][SAMAX] =
{
/*  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18 */
/* CLD IAL RAL DTI DSP CTA CTP SHT DTX DSX RJA RAA QSC CLG RAB CAR FSL RAF CMD*/
  { 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (00) IALSN */
  { 2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (01) IAAAQ */
  { 0,  3,  0,  3, 13, 32, 33, 36,  3, 13,  4, 13,  9,  8,  0, 45,  3, 13,  4 }, /* (02) IAABQ */
  { 0,  0,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0 }, /* (03) IAASA */
  { 0,  0,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0 }, /* (04) IAASJ */
  { 7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (05) IAPCI */
  { 0, 93,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 41,  0,  0 }, /* (06) IAPCA */
  { 0, 94,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 94,  0,  0 }, /* (07) IAPCJ */
  { 0, 94, 95, 14, 15, 35,  4,  0, 75, 79,  0, 15,  0,  8,  0,  4,  0,  0,  0 }, /* (08) IAPAU */
  { 0, 94, 95, 14, 15, 35,  4,  0, 75, 79,  0, 15,  0,  8,  0,  4,  0,  0,  0 }, /* (09) IAPAP */
  { 0,  0,  0, 18, 19, 49,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (10) IAANQ */
  { 0,  0,  0, 20, 21, 73,  0,  0, 53, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (11) IAAEQ */
  { 0,  0,  0, 22, 22,  4, 33, 36,  0, 55,  4, 88,  9,  8,  0,  0,  0, 12, 82 }, /* (12) IAACE */
  { 0,  0,  0,  0, 23, 37,  0,  0,  0, 56,  0, 89,  0,  0,  0,  0,  0, 43,  0 }, /* (13) IAACQ */
  { 0,  0,  0, 24, 25,  0,  0,  0, 51, 52,  0, 90,  0,  8,  0,  0,  0, 85,  0 }, /* (14) IAPDN */
  { 0,  0,  0, 24, 25,  0,  0,  0, 51, 52,  0, 90,  0,  8,  0,  0,  0, 85,  0 }, /* (15) IAPDE */
  { 0,  0,  0, 28, 29, 32,  0, 36,  0,  0,  0,  0,  9,  8,  0,  0,  0,  0,  0 }, /* (16) IAARQ */
  { 0,  0,  0, 30, 31,  0, 39,  0, 30, 80,  0, 31,  0,  8,  0,  0,  0, 31,  0 }, /* (17) IAPRII*/
  { 0,  0,  0,  0,  0,  0, 34,  0,  0,  0,  0,  0,  0,  0,  0, 30,  0,  0,  0 }, /* (18) IAPRA */
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (19) IAPRJ */
  {10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (20) IALSF */
  {11,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (21) IAREJI*/
  { 0,  0,  0, 16, 17, 32,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (22) IAAPG */
  { 0,  0,  0, 42,  0,  0,  0,  0,  0,  0,  0, 91,  0,  8,  0,  0,  0, 86,  0 }, /* (23) IAPDNH*/
  {26,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (24) IASHT */
  {27,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (25) IAQSC */
  {48, 50, 46, 50, 13, 13, 50,  4, 50, 13,  4, 13,  4, 44,  4, 50, 50, 13,  4 }, /* (26) IAABT */
  { 0,  0,  0, 57, 58, 35, 33,  0, 59, 60,  0,  0,  0,  8,  0, 45,  0,  0,  0 }, /* (27) IAXOF */
  { 0,  0,  0, 61, 62, 35, 33,  0, 63, 64,  0,  0,  0,  8,  0, 45,  0,  0,  0 }, /* (28) IAXON */
  { 0,  0,  0, 65, 66, 49,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (29) IAANQX*/
  { 0,  0,  0,  0, 67,  0,  0,  0,  0, 68,  0, 92,  0,  8,  0,  0,  0, 87,  0 }, /* (30) IAPDNQ*/
  { 0,  0,  0,  0, 69, 70,  0,  0,  0, 74,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (31) IAACEX*/
  { 0,  0,  0, 71, 72, 32,  0,  0,  0,  0,  0,  0,  0,  8,  0,  0,  0,  0,  0 }, /* (32) IAAPGX*/
  { 0,  0,  0, 76, 77,  0,  0,  0, 78, 77,  0,  0,  0,  8,  0,  0,  0,  0,  0 }, /* (33) IAABQX*/
  { 0,  0,  0, 22, 83, 32, 33,  0, 71, 84,  0,  0,  0,  8,  0, 45,  0,  0,  0 }, /* (34) IAABQR*/
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 44,  0,  0,  0,  0, 44 }, /* (35) IAAFN */
  { 0,  0, 47,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 44,  0,  0,  0,  0 }, /* (36) IAREJR*/
  { 0,  0,  0, 30, 31,  0, 45,  0, 30, 80,  0,  0,  0,  8,  0,  0,  0,  0,  0 }, /* (37) IAPRIR */
  {40,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (38) IAAAF */
  { 0, 94, 95,  4, 32,  0,  0,  0,  4, 32,  0, 15,  0,  8,  0,  4, 94, 15,  0 }, /* (39) IAPAM */
  {81,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /* (40) IACMD */
};


/*}
** Name: ALFSM_ENTRY - FSM matrix
**
** Description:
**	This is the linear array containing the entries for the FSM matrix.
**	The entries in the preceeding table AL_FSM_TBL are index values
**	corresponding to entries in the current array.  The array size limits
**	on the individual elements of each array entry, TL_OUT_MAX and
**	TL_ACTN_MAX are set to values which are currently adequate.  They are
**	confined in scope to the AL.  If future expansion requires these to be
**	increased, this can be done without affecting anything outside AL.  If
**	the values are changed in GCCAL.H and the AL recompiled, everything
**	will work.
**
** History:
**      10-Jun-88 (jbowers)
**          Initial structure implementation
**	23-Aug-96 (gordy)
**	    Fast Select now sends a GCA_RELEASE message to end exchange.
**	    Added GCA normal channel receive to Fast Select action list
**	    (IAACEF, SARAA).  
**	 4-Nov-05 (gordy)
**	    Changed actions associated with state transitions for SAFSL
**	    and new state SARAF.  In particular, a normal channel receive
**	    no longer occurs at the same time as AAFSL ('priming the
**	    pump' with the FASTSELECT data).  The normal channel receive
**	    now occurs later as part of the standard data receive actions
**	    (SADTI and IAANQ).  Originally, a special action was required
**	    to not post the receive when the psuedo FASTSELECT receive
**	    occured.
*/
 
 
GLOBALCONSTDEF struct _alfsm_entry 
alfsm_entry[]
=
{
 /* ( 0)  */ {SACLD, {0, 0}, {0, 0, 0, 0}},	/* Illegal instructionn  */
 /* ( 1)  */ {SACLD, {0, 0},{AALSN, 0, 0, 0}},
 /* ( 2)  */ {SAIAL, {OAPCQ, 0},{0, 0, 0, 0}},
 /* ( 3)  */ {SACLG, {OAPAU, 0},{AADIS, 0, 0, 0}},
 /* ( 4)  */ {SACLG, {0, 0},{AADIS, AAMDE, 0, 0}},
 /* ( 5)  */ {SADTI, {OAPCA, 0},{AARVN, AARVE, 0, 0}},
 /* ( 6)  */ {SACLG, {OAPCJ, 0},{AADIS, 0, 0, 0}},
 /* ( 7)  */ {SARAL, {0, 0},{AARQS, 0, 0, 0}},
 /* ( 8)  */ {SACLG, {0, 0},{AAMDE, 0, 0, 0}},
 /* ( 9)  */ {SACLG, {0, 0},{AAQSC, AADIS, AAMDE, 0}},
 /* (10)  */ {SACLG, {0, 0},{AADIS, AAMDE, 0}},
 /* (11)  */ {SARJA, {0, 0},{AARSP, 0, 0, 0}},
 /* (12)  */ {SADTI, {0, 0},{AAFSL, AARVE, AAMDE, 0}},
 /* (13)  */ {SACLG, {OAPAU, 0},{AAPGQ, AADIS, 0, 0}},
 /* (14)  */ {SACTA, {0, 0},{AARLS, AASND, 0, 0}},
 /* (15)  */ {SACTA, {0, 0},{AAPGQ, AARLS, AAENQ, 0}},
 /* (16)  */ {SADTI, {0, 0},{AARVN, AAMDE, 0, 0}},
 /* (17)  */ {SADSP, {0, 0},{AARVN, AAMDE, 0, 0}},
 /* (18)  */ {SADTI, {OAPDQ, 0},{AARVN, 0, 0, 0}},
 /* (19)  */ {SADSP, {OAPDQ, 0},{AARVN, 0, 0, 0}},
 /* (20)  */ {SADTI, {OAPEQ, 0},{AARVE, 0, 0, 0}},
 /* (21)  */ {SADSP, {OAPEQ, 0},{AARVE, 0, 0, 0}},
 /* (22)  */ {SADTI, {0, 0},{AAMDE, 0, 0, 0}},
 /* (23)  */ {SADSP, {0, 0},{AADEQ, AAMDE, 0, 0}},
 /* (24)  */ {SADSP, {0, 0},{AASND, 0, 0, 0}},
 /* (25)  */ {SADSP, {0, 0},{AAENQ, 0, 0, 0}},
 /* (26)  */ {SASHT, {0, 0},{AARSP, 0, 0, 0}},
 /* (27)  */ {SAQSC, {0, 0},{AARSP, 0, 0, 0}},
 /* (28)  */ {SACTP, {OAPDQ, OAPRQ},{0, 0, 0, 0}},
 /* (29)  */ {SACTP, {OAPDQ, OAPRQ},{AAPGQ, 0, 0, 0}},
 /* (30)  */ {SACLG, {OAPRS, 0},{AADIS, 0, 0, 0}},
 /* (31)  */ {SACTA, {OAPRS, 0},{0, 0, 0, 0}},
 /* (32)  */ {SACLG, {0, 0},{AAPGQ, AADIS, AAMDE, 0}},
 /* (33)  */ {SACTP, {0, 0},{AAMDE, 0, 0, 0}},
 /* (34)  */ {SACLG, {0, 0},{AADIS, AAMDE, 0, 0}},
 /* (35)  */ {SACTA, {0, 0},{AAMDE, 0, 0, 0}},
 /* (36)  */ {SACLG, {0, 0},{AASHT, AADIS, AAMDE, 0}},
 /* (37)  */ {SACTA, {0, 0},{AADEQ, AAMDE, 0, 0}},
 /* (38)  */ {SACLD, {0, 0},{0, 0, 0, 0}},	/* NOT USED */
 /* (39)  */ {SACTP, {OAPRS, 0},{0, 0, 0, 0}},
 /* (40)  */ {SAFSL, {OAPCQ, 0},{0, 0, 0, 0}},
 /* (41)  */ {SARAF, {0, 0},{AARSP, 0, 0, 0}},
 /* (42)  */ {SADSP, {0, 0},{AAENQ, AASNDG, 0, 0}},
 /* (43)  */ {SADSP, {0, 0},{AAFSL, AARVE, AADEQ, AAMDE}},
 /* (44)  */ {SACLD, {0, 0},{AACCB, AAMDE, 0, 0}},
 /* (45)  */ {SACAR, {0, 0},{AAMDE, 0, 0, 0}},
 /* (46)  */ {SARAB, {OAPAU, 0},{0, 0, 0, 0}}, 
 /* (47)  */ {SACLD, {OAPCJ, 0},{AACCB, 0, 0, 0}},
 /* (48)  */ {SACLD, {0, 0},{AAMDE, 0, 0, 0}},
 /* (49)  */ {SACTA, {0, 0},{AARVN, AAMDE, 0, 0}},
 /* (50)  */ {SACLG, {OAPAU, 0}, {AADIS, 0, 0, 0}},
 /* (51)  */ {SADSX, {0, 0},{AASND, 0, 0, 0}},
 /* (52)  */ {SADSX, {0, 0},{AAENQ, 0, 0, 0}},
 /* (53)  */ {SADTX, {OAPEQ, 0},{AARVE, 0, 0, 0}},
 /* (54)  */ {SADSX, {OAPEQ, 0},{AARVE, 0, 0, 0}},
 /* (55)  */ {SADTX, {0, 0},{AAMDE, 0, 0, 0}},
 /* (56)  */ {SADSX, {0, 0},{AADEQ, AAMDE, 0, 0}},
 /* (57)  */ {SADTI, {0, 0},{AASXF, AAMDE, 0, 0}},
 /* (58)  */ {SADSP, {0, 0},{AASXF, AAMDE, 0, 0}},
 /* (59)  */ {SADTX, {0, 0},{AASXF, AAMDE, 0, 0}},
 /* (60)  */ {SADSX, {0, 0},{AASXF, AAMDE, 0, 0}},
 /* (61)  */ {SADTI, {0, 0},{AARXF, AAMDE, 0, 0}},
 /* (62)  */ {SADSP, {0, 0},{AARXF, AAMDE, 0, 0}},
 /* (63)  */ {SADTI, {0, 0},{AARXF, AARVN, AAMDE, 0}},
 /* (64)  */ {SADSP, {0, 0},{AARXF, AARVN, AAMDE, 0}},
 /* (65)  */ {SADTX, {OAPDQ, 0},{0, 0, 0, 0}},
 /* (66)  */ {SADSX, {OAPDQ, 0},{0, 0, 0, 0}},
 /* (67)  */ {SADSP, {OAXOF, 0},{AAENQ, 0, 0, 0}},
 /* (68)  */ {SADSX, {OAXOF, 0},{AAENQ, 0, 0, 0}},
 /* (69)  */ {SADTI, {OAXON, 0},{AAMDE, 0, 0, 0}},
 /* (70)  */ {SACLG, {OAXON, 0},{AAPGQ, AADIS, AAMDE, 0}},
 /* (71)  */ {SADTX, {0, 0},{AAMDE, 0, 0, 0}},
 /* (72)  */ {SADSX, {0, 0},{AAMDE, 0, 0, 0}},
 /* (73)  */ {SACTA, {0, 0},{AARVE, AAMDE, 0, 0}},
 /* (74)  */ {SADTX, {OAXON, 0},{AAMDE, 0, 0, 0}},
 /* (75)  */ {SACTA, {0, 0},{AARVN, AARLS, AASND, 0}},
 /* (76)  */ {SACLG, {OAXON, OAPAU},{AADIS, 0, 0, 0}},
 /* (77)  */ {SACLG, {OAXON, OAPAU},{AAPGQ, AADIS, 0, 0}},
 /* (78)  */ {SACLG, {OAXON, OAPAU},{AADIS, 0, 0, 0}},
 /* (79)  */ {SACTA, {0, 0},{AARVN, AAPGQ, AARLS, AAENQ}},
 /* (80)  */ {SACTA, {OAPRS, 0},{AARVN, 0, 0, 0}},
 /* (81)  */ {SACMD, {0, 0},{AARSP, 0, 0, 0}},
 /* (82)  */ {SACMD, {0, 0},{AACMD, 0, 0, 0}},
 /* (83)  */ {SADTI, {0, 0},{AAPGQ, AAMDE, 0, 0}},
 /* (84)  */ {SADTX, {0, 0},{AAPGQ, AAMDE, 0, 0}},
 /* (85)  */ {SARAF, {0, 0},{AAENQ, 0, 0, 0}},
 /* (86)  */ {SARAF, {0, 0},{AAENQ, AAENQG, 0, 0}},
 /* (87)  */ {SARAF, {OAXOF, 0},{AAENQ, 0, 0, 0}},
 /* (88)  */ {SADTI, {0, 0},{AARVN, AARVE, AAMDE, 0}},
 /* (89)  */ {SADSP, {0, 0},{AARVN, AARVE, AADEQ, AAMDE}},
 /* (90)  */ {SARAA, {0, 0},{AAENQ, 0, 0, 0}},
 /* (91)  */ {SARAA, {0, 0},{AAENQ, AAENQG, 0, 0}},
 /* (92)  */ {SARAA, {OAXOF, 0},{AAENQ, 0, 0, 0}},
 /* (93)  */ {SARAA, {0, 0},{AARSP, 0, 0, 0}},
 /* (94)  */ {SARJA, {0, 0},{AARSP, 0, 0, 0}}, 
 /* (95)  */ {SARAB, {0, 0},{AAMDE, 0, 0, 0}},
}; 
