/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cv.h>
#include    <gcccl.h>
#include    <me.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcatrace.h>
#include    <gcc.h>
#include    <gccpci.h>
#include    <gccer.h>
#include    <gccgref.h>
#include    <gccsl.h>
#include    <gcxdebug.h>

/* 
**  INSERT_AUTOMATED_FUNCTION_PROTOTYPES_HERE
*/

/**
**
**  Name: GCCSL.C - GCF communication server Session Layer module
**
**  Description:
**      GCCSL.C contains all routines which constitute the Session Layer 
**      of the GCF communication server. 
**
**          gcc_sl - Main entry point for the Session Layer.
**	    gcc_sl_event - FSM processor
**          gcc_slinit - SL initialization routine
**          gcc_slterm - SL termination routine
**          gcc_sl_evinp - FSM input event evaluator
**          gcc_slout_exec - FSM output event execution processor
**	    gcc_sl_abort - SL internal abort routine
**
**
**  History:
**      22-Mar-88 (jbowers)
**          Initial module creation
**      16-Jan-89 (jbowers)
**          Documentation change to FSM to document fix for an invalid
**	    intersection. 
**      16-Feb-89 (jbowers)
**          Add local flow control: expand FSM to handle XON and XOFF
**	    primitives. 
**      18-APR-89 (jbowers)
**          Added handling of (10, 6) intersection to FSM (documentation only:
**	    actual code fix is in GCCSL.H).
**	12-jun-89 (jorge)
**	    added include descrip.h
**	19-jun-89 (seiwald)
**	    Added GCXDEBUGING
**	19-jun-89 (pasker)
**	    Added GCCSL GCATRACing.
**	22-jun-89 (seiwald)
**	    Straightened out gcc[apst]l.c tracing.
**      06-Sep-89 (cmorris)
**          Spruced up gcx tracing.
**      14-Sep-89 (ham)
**	    Byte alignment.  Instead of using structures pointers to
**	    the mde we now build the pci ine the mde byte by byte and
**	    strip it out byte by byte.
**      10-Nov-89 (cmorris)
**          Cleaned up the more obviously inaccurate comments.
**	07-Dec-89 (seiwald)
**	     GCC_MDE reorganised: mde_hdr no longer a substructure;
**	     some unused pieces removed; pci_offset, l_user_data,
**	     l_ad_buffer, and other variables replaced with p1, p2, 
**	     papp, and pend: pointers to the beginning and end of PDU 
**	     data, beginning of application data in the PDU, and end
**	     of PDU buffer.
**	09-Dec-89 (seiwald)
**	     Allow for variously sized MDE's: specify size when allocating
**	     one. 
**      09-Jan-90 (cmorris)
**           Tidied up real-time GCA tracing.
**	12-Jan-90 (seiwald)
**	     Ironed out ownership of MDE's: an MDE now belongs to exactly 
**	     one layer at a time.  See change comments in gccal.c.
**      23-Jan-90 (cmorris)
**           More tracing stuff.
**      03-Mar-90 (cmorris)
**	     Don't dereference array more often than we need too...
**	06-Mar-90 (cmorris)
**	     Reworked internal aborts.
**	27-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	13-May-91 (cmorris)
**	    Fixed a number of potential memory leaks. Documentation
**	    only:- fix is in GCCSLFSM.ROC.
**	14-May-91 (cmorris)
**	    Actions are no more:- incorporate only remaining action as event.
**	    Added support for Session connection release collisions.
**	28-May-91 (cmorris)
**	    Separate out freeing of saved Connect Request MDE into
**	    an output event facilitate removing memory leak in abort cases.
**      13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**	14-Aug-91 (seiwald)
**	    GCaddn2() takes a i4, so use 0 instead of NULL.  NULL is
**	    declared as a pointer on OS/2.
**  	15-Aug-91 (cmorris)
**  	    After we have issued/received a transport disconnect, no
**          further interaction with the TL occurs. Similarly don't
**  	    send XON/XOFF up after session (as opposed to underlying
**  	    transport) connection has been released.
**  	15-Aug-91 (cmorris)
**  	    After we have issued/received a session abort, no further 
**  	    interaction with PL occurs.
**  	16-Aug-91 (cmorris)
**  	    Force XON down to TL when we want to issue an abort. This
**  	    must be done to make sure that TL has a receive posted to
**  	    pick up the response to the abort: fixes problems where
**  	    upper layer internal aborts could hang when TL was XOFF'd.
**  	16-Aug-91 (cmorris)
**  	    Fix aborts in closed state.
**  	12-Mar-92 (cmorris)
**  	    If we get a transport disconnect indication or an internal abort
**          when we're waiting for a Connect SPDU, don't issue an abort up to
**  	    Presentation:- there's no presentation connection at this stage.
**  	08-May-92 (cmorris)
**  	    Comment tidyup.
**	10-Feb-93 (seiwald)
**	    Removed generic_status and system_status from gcc_xl(),
**	    gcc_xlfsm(), and gcc_xlxxx_exec() calls.
**	11-Feb-93 (seiwald)
**	    Merge the gcc_xlfsm()'s into the gcc_xl()'s.
**	16-Feb-93 (seiwald)
**	    Including gcc.h now requires including gca.h.
**	16-mar-1993 (lauraw)
**	    Left-shifted the # ifdef that was erroneously indented.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-Sep-93 (brucek)
**	    Removed include of dbdbms.h.
**  	28-Sep-93 (cmorris)
**  	    Added support to hand 'flush data' flag down on T_DATA requests.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	24-par-96 (chech02, lawst01)
**	    cast %d arguments to i4  in TRdisplay for windows 3.1 port.
**	19-Jun-96 (gordy)
**	    Recursive requests are now queued and processed by the first
**	    instance of gcc_sl().  This ensures that all outputs of an 
**	    input event are executed prior to processing subsequent requests.
**	 3-Jun-97 (gordy)
**	    The initial connection info, passed as user data, is now
**	    built here rather than in the AL.  This will allow future
**	    extensions based on negotiated protocol level after the
**	    T_CONNECT request is processed (which can't be done in
**	    the AL).
**	26-Aug-97 (gordy)
**	    Connection info now associated with CN SPDU now built
**	    built by SL at protocol level 2.  Added support for
**	    extended length indicators.  General cleanup of PCI's.
**	10-Oct-97 (rajus01)
**	    Added password encryption capability.
**      22-Oct-97 (rajus01)
**          Replaced gcc_get_obj(), gcc_del_obj() with
**          gcc_add_mde(), gcc_del_mde() calls respectively.
**	13-may-1998 (canor01)
**	    Add include of me.h.
**	20-May-98 (gordy)
**	    Authentications may be longer then 254 bytes,
**	    so use extended length indicator format.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	10-Jul-00 (gordy)
**	    Kludged ABORT handling to ignore length indicators which
**	    are being sent incorrectly (see OSAB and OSSAX).
**	12-Jul-00 (gordy)
**	    Fixed all SL PCI length indicators to handle normal and
**	    extended formats (cleans up the kludge of the previous fix).
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      25-Jun-2001 (wansh01)
**          replace gcxlevel with IIGCc_global->gcc_trace_level 
**	16-Jun-04 (gordy)
**	    Added additional mask to password encryption key.
**	31-Mar-06 (gordy)
**	    Sanity check length read from IO buffers.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**	17-Dec-09 (Bruce Lunsford)  Sir 122536
**	    In support of long names (user, password), modify send
**	    of SL user/password connect info to use extended format
**	    which has a 2-byte length and can handle the longer strings.
*/

/*
**  Forward and/or External function references.
*/

static STATUS	gcc_sl_event( GCC_MDE *mde );
static u_i4	gcc_sl_evinp( GCC_MDE *mde, STATUS *generic_status);
static STATUS	gcc_slout_exec( i4  output_event, GCC_MDE *mde, GCC_CCB *ccb );
static VOID	gcc_sl_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *status, 
			      CL_ERR_DESC *system_status, GCC_ERLIST *erlist );


/*{
** Name: gcc_sl	- Entry point for the Session Layer
**
** Description:
**	gcc_sl is the SL entry point.  
**	It is invoked with a Message Data Element (MDE) as its input
**	parameter.  The MDE may come from PL and is outbound for the network, or
**	from TL and is inbound for a local client or server, sent from an
**	external node. For a detailed discussion of the
**	operation of an induvidual layer, see the header of GCCAL.C.
**

**	States, input events and output events are symbolized
**	respectively as follows:
**
**	SSxxx, ISxxx and OSxxx, where xxx is a unique mnemonic designator.  In
**	the case of input and output events, the designator indicates the
**	associated service primitive or SPDU.
**
**	Output events and actions are marked with a '*' if they pass 
**	the incoming MDE to another layer, or if they delete the MDE.
**	There should be exactly one such output event or action per
**	state cell.
 
**	Connection initiation is indicated in one of two ways: by receipt of an
**	S_CONNECT request sent by PL or by recipt of a T_CONNECT indication
**	sent by TL as a result of a GCA_REQUEST by a remote client.
**
**	In the first case, we are the Initiating Session Layer (ISL), driven by
**	a client.  A T_CONNECT request is constructed and sent to TL.  A
**	T_CONNECT confirm is awaited before proceeding.  When received, the
**	Transport Connection is open, and a CN SPDU is sent to the peer.
**	Either an AC or an RF SPDU will be returned.  In the former case, an
**	S_CONNECT confirm/accept is sent to PL and the data transfer phase is
**	entered.  In the latter case, an S_CONNECT confirm/reject is sent to
**	PL, and a T_DISCONNECT request is sent to TL.  If a T_DISCONNECT
**	indication is received instead, an S_P_ABORT indication is sent to PL.
**
**	In the second case, we are the Responding Session Layer (RSL), in a
**	remote server node.  A T_CONNECT response/accept is sent to TL and a CN
**	SPDU is awaited.  If received, a P_CONNECT indication is sent to PL,
**	and  a P_CONNECT response  is awaited.  When received, if it is ACCEPT,
**	an AC SPDU is sent to the peer SL. If it is REJECT, an RF SPDU is sent
**	to the peer SL.
**	
**	In the FSM definition matrices which follows, the parenthesized integers
**	are the designators for the valid entries in the matrix, and are used in
**	the representation of the FSM in GCCSLFSM.ROC as the entries in SLFSM_TLB
**	which are the indices into the entry array SLFSM_ENTRY.  See GCCSLFSM.ROC
**	for further details.
**
**	The SL FSM recognizes the flow control primitives XON and XOFF.  These
**	may be in the form of S_XON and S_XOFF requests from PL, or T_XON and
**	T_XOFF indications from TL.  These are simply passed through SL to the
**	appropriate adjacent layer.  In the first case, they flow to TL as
**	T_XON and T_XOFF requests respectively.  In the second case, they flow
**	to PL as S_XON and S_XOFF indications respectively.  No real flow
**	control operations are performed in SL.

** 
**	For the Session Layer, the following states, andevents
**	are defined, with the state characterizing an individual 
**	connection:
**
**	States:
**
**	    SSCLD   - Closed, connection does not exist.
**	    SSISL   - ISL: T_CONNECT confirm pending
**	    SSRSL   - RSL: Awaiting CN SPDU
**	    SSWAC   - ISL: Awaiting AC SPDU
**	    SSWCR   - RSL: Awaiting S_CONNECT response
**	    SSOPN   - Connected
**	    SSCDN   - Closing: initiator awaiting DN SPDU
**	    SSCRR   - Closing: responder awaiting S_RELEASE response
**	    SSCTD   - Closing: responder awaiting T_DISCONNECT indication
**
**	Input Events:
**
**	    ISSCQ   -  S_CONNECT request
**	    ISTCI   -  T_CONNECT indication
**	    ISTCC   -  T_CONNECT confirm
**	    ISSCA   -  S_CONNECT response/accept
**	    ISSCJ   -  S_CONNECT response/reject
**	    ISCN    -  CN (Connect) SPDU (in T_DATA indication)
**	    ISAC    -  AC (Accept Connection) SPDU (in T_DATA indication)
**	    ISRF    -  RF (Refuse Connection) SPDU (in T_DATA indication)
**	    ISSDQ   -  S_DATA request 
**	    ISSEQ   -  S_EXP_DATA request
**	    ISDT    -  DT (Data) SPDU (in T_DATA indication)
**	    ISEX    -  EX (Expedited Data) SPDU (in T_DATA indication)
**	    ISSAU   -  S_U_ABORT request
**	    ISAB    -  AB (Abort) SPDU (in T_DATA indication)
**	    ISSRQ   -  S_RELEASE request
**	    ISSRS   -  S_RELEASE response (accept)
**	    ISFN    -  FN (Finished) SPDU (in T_DATA indication)
**	    ISDN    -  DN (Disconnect) SPDU (in T_DATA indication)
**	    ISTDI   -  T_DISCONNECT indication
**	    ISNF    -  NF (Not Finished) SPDU (in T_DATA indication)
**	    ISABT   -  S_P_ABORT: SL internal event execution failure
**	    ISSXF   -  S_XOFF request
**	    ISSXN   -  S_XON request
**	    ISTXF   -  T_XOFF indication
**	    ISTXN   -  T_XON indication
**	    ISDNCR  -  DN SPDU, release collision, responder
**	    ISRSCI  -  S_RELEASE response (accept), release collision, initiator
**
**	Output Events:
**
**	    OSSCI*  -  S_CONNECT indication 
**	    OSSCA*  -  S_CONNECT confirm/accept 
**	    OSSCJ*  -  S_CONNECT confirm/reject 
**	    OSSAX*  -  S_U_ABORT or S_P_ABORT indication
**	    OSSAP*  -  S_P_ABORT indication
**	    OSSDI*  -  S_DATA indication
**	    OSSEI*  -  S_EXP_DATA indication
**	    OSSRI*  -  S_RELEASE indication
**	    OSRCA*  -  S_RELEASE confirm/accept
**	    OSRCJ*  -  S_RELEASE confirm/reject
**	    OSTCQ*  -  T_CONNECT request
**	    OSTCS*  -  T_CONNECT response
**	    OSTDC   -  T_DISCONNECT request
**	    OSCN    -  CN SPDU (in T_DATA request)
**	    OSAC*   -  AC SPDU (in T_DATA request)
**	    OSRF*   -  RF SPDU (in T_DATA request)
**	    OSDT*   -  DT SPDU (in T_DATA request)
**	    OSEX*   -  EX SPDU (in T_DATA request)
**	    OSAB*   -  AB SPDU (in T_DATA request)
**	    OSDN*   -  DN SPDU (in T_DATA request)
**	    OSFN*   -  FN SPDU (in T_DATA request)
**	    OSSXF*  -  S_XOFF indication
**	    OSSXN*  -  S_XON indication
**	    OSTXF*  -  T_XOFF request
**	    OSTXN*  -  T_XON request
**	    OSCOL   -  Set Release Collision
**	    OSDNR   -  Set DN SPDU received during release collision
**          OSMDE*  -  Discard an MDE
**	    OSPGQ   -  Purge queued S_CONNECT MDE
**  	    OSTFX   -  Generate forced T_XON request

**
**	Following is the Finite State Machine (FSM) of the Session Layer:
**
**
**	State || SSCLD| SSISL| SSRSL| SSWAC| SSWCR| SSOPN| SSCDN| SSCRR| SSCTD|
**	      ||  00  |  01  |  02  |  03  |  04  |  05  |  06  |  07  |  08  |
**	=======================================================================
**	Event ||
**	=======================================================================
**	ISSCQ || SSISL|      |      |      |      |      |      |      |      |
**	(00)  || OSTCQ*      |      |      |      |      |      |      |      |
**	      ||  (1) |      |      |      |      |      |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISTCI || SSRSL|      |      |      |      |      |      |      |      |
**	(01)  || OSTCS*      |      |      |      |      |      |      |      |
**	      ||  (2) |      |      |      |      |      |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISTCC ||      | SSWAC|      |      |      |      |      |      |      |
**	(02)  ||      | OSCN |      |      |      |      |      |      |      |
**	      ||      | OSMDE*      |      |      |      |      |      |      |
**	      ||      |  (3) |      |      |      |      |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISSCA ||      |      |      |      | SSOPN|      |      |      |      |
**	(03)  ||      |      |      |      | OSAC*|      |      |      |      |
**	      ||      |      |      |      |  (4) |      |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISSCJ ||      |      |      |      | SSCTD|      |      |      |      |
**	(04)  ||      |      |      |      | OSRF*|      |      |      |      |
**	      ||      |      |      |      |  (5) |      |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISCN  ||      |      | SSWCR|      |      |      |      |      | SSCLD|
**	(05)  ||      |      | OSSCI*      |      |      |      |      | OSTDC|
**	      ||      |      |      |      |      |      |      |      | OSMDE*
**	      ||      |      |  (6) |      |      |      |      |      |  (7) |
**	------++------+------+------+------+------+------+------+------+------+
**	ISAC  ||      |      | SSCLD| SSOPN|      |      |      |      |      |
**	(06)  ||      |      | OSTDC| OSSCA*      |      |      |      |      |
**	      ||      |      | OSMDE*      |      |      |      |      |      |
**	      ||      |      |  (7) |  (9) |      |      |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISRF  ||      |      | SSCLD| SSCLD|      |      |      |      |      |
**	(07)  ||      |      | OSTDC| OSSCJ*      |      |      |      |      |
**	      ||      |      | OSMDE* OSTDC|      |      |      |      |      |
**	      ||      |      |  (10)|  (11)|      |      |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISSDQ ||      |      |      |      |      | SSOPN|      |      |      |
**	(08)  ||      |      |      |      |      | OSDT*|      |      |      |
**	      ||      |      |      |      |      | (12) |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISSEQ ||      |      |      |      |      | SSOPN|      |      |      |
**	(09)  ||      |      |      |      |      | OSEX*|      |      |      |
**	      ||      |      |      |      |      | (13) |      |      |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISDT  ||      |      |      |      |      | SSOPN| SSCDN|      | SSCTD|
**	(10)  ||      |      |      |      |      | OSSDI* OSMDE*      | OSMDE*
**	      ||      |      |      |      |      | (14) |  (51)|      |  (8) |
**	------++------+------+------+------+------+------+------+------+------+

**	State || SSCLD| SSISL| SSRSL| SSWAC| SSWCR| SSOPN| SSCDN| SSCRR| SSCTD|
**	      ||  00  |  01  |  02  |  03  |  04  |  05  |  06  |  07  |  08  |
**	=======================================================================
**	Event ||
**	=======================================================================
**	ISEX  ||      |      |      |      |      | SSOPN|      |      | SSCTD|
**	(11)  ||      |      |      |      |      | OSSEI*      |      | OSMDE*
**	      ||      |      |      |      |      | (15) |      |      |  (8) |
**	------++------+------+------+------+------+------+------+------+------+
**	ISSAU ||      | SSCLD|      | SSCTD| SSCTD| SSCTD| SSCTD| SSCTD|      |
**	(12)  ||      | OSPGQ|      | OSTFX| OSTFX| OSTFX| OSTFX| OSTFX|      |
**	      ||      | OSTDC|      | OSAB*| OSAB*| OSAB*| OSAB*| OSAB*|      |
**	      ||      | OSMDE*      |      |      |      |      |      |      |
**	      ||      |  (52)|      | (17) | (17) | (17) | (17) | (17) |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISAB  ||      |      | SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD|
**	(13)  ||      |      | OSTDC| OSTDC| OSTDC| OSTDC| OSTDC| OSTDC| OSTDC|
**	      ||      |      | OSMDE* OSSAX* OSSAX* OSSAX* OSSAX* OSSAX* OSMDE*
**	      ||      |      |  (16)| (18) | (18) | (18) | (18) | (18) |  (16)|
**	------++------+------+------+------+------+------+------+------+------+
**	ISSRQ ||      |      |      |      |      | SSCDN|      | SSCRR|      |
**	(14)  ||      |      |      |      |      | OSFN*|      | OSCOL|      |
**	      ||      |      |      |      |      |      |      | OSFN*|      |
**	      ||      |      |      |      |      |  (19)|      | (20) |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISSRS ||      |      |      |      |      |      |      | SSCTD|      |
**	(15)  ||      |      |      |      |      |      |      | OSDN*|      |
**	      ||      |      |      |      |      |      |      | (21) |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISFN  ||      |      | SSCLD|      |      | SSCRR| SSCRR|      | SSCTD|
**	(16)  ||      |      | OSTDC|      |      | OSSRI* OSCOL|      | OSMDE*
**	      ||      |      | OSMDE*      |      |      | OSSRI*      |      |
**	      ||      |      |  (16)|      |      | (22) | (23) |      |  (8) |
**	------++------+------+------+------+------+------+------+------+------+
**	ISDN  ||      |      | SSCLD|      |      |      | SSCLD|      | SSCTD|
**	(17)  ||      |      | OSTDC|      |      |      | OSRCA*      | OSMDE*
**	      ||      |      | OSMDE*      |      |      | OSTDC|      |      |
**	      ||      |      | (16) |      |      |      | (24) |      |  (8) |
**	------++------+------+------+------+------+------+------+------+------+
**	ISTDI || SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD|
**	(18)  || OSMDE* OSPGQ| OSMDE* OSSAP* OSSAP* OSSAP* OSSAP* OSSAP* OSMDE*
**            ||      | OSSAP*      |      |      |      |      |      |      |
**	      || (27) | (53) | (27) | (26) | (26) | (26) | (26) | (26) | (27) |
**	------++------+------+------+------+------+------+------+------+------+
**	ISNF  ||      |      | SSCLD|      |      |      | SSOPN|      | SSCTD|
**	(19)  ||      |      | OSTDC|      |      |      | OSRCJ*      | OSMDE*
**	      ||      |      | OSMDE*      |      |      |      |      |      |
**	      ||      |      |  (16)|      |      |      | (28) |      |  (8) |
**	------++------+------+------+------+------+------+------+------+------+
**	ISABT || SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD| SSCLD|
**	(20)  || OSMDE* OSPGQ| OSTDC| OSTDC| OSTDC| OSTDC| OSTDC| OSTDC| OSTDC|
**	      ||      | OSTDC| OSMDE* OSSAP* OSSAP* OSSAP* OSSAP* OSSAP* OSSAP*
**            ||      | OSSAP*      |      |      |      |      |      |      |
**	      ||  (27)|  (54)|  (16)|  (50)|  (50)|  (50)|  (50)|  (50)|  (50)|
**	------++------+------+------+------+------+------+------+------+------+

**	State || SSCLD| SSISL| SSRSL| SSWAC| SSWCR| SSOPN| SSCDN| SSCRR| SSCTD|
**	      ||  00  |  01  |  02  |  03  |  04  |  05  |  06  |  07  |  08  |
**	=======================================================================
**	Event ||
**	=======================================================================
**	ISSXF ||      |      |      |      |      | SSOPN| SSCDN| SSCRR|      |
**	(21)  ||      |      |      |      |      | OSTXF* OSTXF* OSTXF*      |
**	      ||      |      |      |      |      |  (30)|  (31)|  (32)|      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISSXN ||      |      |      |      |      | SSOPN| SSCDN| SSCRR|      |
**	(22)  ||      |      |      |      |      | OSTXN* OSTXN* OSTXN*      |
**	      ||      |      |      |      |      |  (35)|  (36)|  (37)|      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISTXF ||      |      |      |      |      | SSOPN| SSCDN| SSCRR| SSCTD|
**	(23)  ||      |      |      |      |      | OSSXF* OSSXF* OSSXF* OSMDE*
**	      ||      |      |      |      |      |  (40)|  (41)|  (42)|  (43)|
**	------++------+------+------+------+------+------+------+------+------+
**	ISTXN ||      |      |      |      |      | SSOPN| SSCDN| SSCRR| SSCTD|
**	(24)  ||      |      |      |      |      | OSSXN* OSSXN* OSSXN* OSMDE*
**	      ||      |      |      |      |      |  (45)|  (46)|  (47)|  (48)|
**	------++------+------+------+------+------+------+------+------+------+
**	ISDNCR||      |      |      |      |      |      |      | SSCRR|      |
**	(25)  ||      |      |      |      |      |      |      | OSDNR|      |
**	      ||      |      |      |      |      |      |      | OSRCA*      |
**	      ||      |      |      |      |      |      |      | (49) |      |
**	------++------+------+------+------+------+------+------+------+------+
**	ISRSCI||      |      |      |      |      |      |      | SSCDN|      |
**	(26)  ||      |      |      |      |      |      |      | OSDN*|      |
**	      ||      |      |      |      |      |      |      | (25) |      |
**	------++------+------+------+------+------+------+------+------+------+

**
** Inputs:
**      mde			Pointer to dequeued Message Data Element
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-Jun-96 (gordy)
**	    Renamed gcc_sl() to gcc_sl_event() and created gcc_sl()
**	    to disallow nested processing of events.  The first
**	    call to this routine sets a flag which causes recursive
**	    calls to queue the MDE for processing sequentially.
*/

STATUS
gcc_sl( GCC_MDE *mde )
{
    GCC_CCB	*ccb = mde->ccb;
    STATUS	status = OK;
    
    /*
    ** If the event queue active flag is set, this
    ** is a recursive call and SL events are being
    ** processed by a previous instance of this 
    ** routine.  Queue the current event, it will
    ** be processed once we return to the active
    ** processing level.
    */
    if ( ccb->ccb_sce.sl_evq.active )
    {
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
	{
            TRdisplay( "gcc_sl: queueing event %s %s\n",
                   gcx_look( gcx_mde_service, mde->service_primitive ),
                   gcx_look( gcx_mde_primitive, mde->primitive_type ));
	}
# endif /* GCXDEBUG */

	QUinsert( (QUEUE *)mde, ccb->ccb_sce.sl_evq.evq.q_prev );
	return( OK );
    }

    /*
    ** Flag active processing so any subsequent 
    ** recursive calls will be queued.  We will 
    ** process all queued events once the stack
    ** unwinds to us.
    */
    ccb->ccb_sce.sl_evq.active = TRUE;

    /*
    ** Process the current input event.  The queue
    ** should be empty since no other instance of
    ** this routine is active, and the initial call
    ** always clears the queue before exiting.
    */
    status = gcc_sl_event( mde );

    /*
    ** Process any queued input events.  These
    ** events are the result of recursive calls
    ** to this routine while we are processing
    ** events.
    */
    while( ccb->ccb_sce.sl_evq.evq.q_next != &ccb->ccb_sce.sl_evq.evq )
    {
	mde = (GCC_MDE *)QUremove( ccb->ccb_sce.sl_evq.evq.q_next );

# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
	{
            TRdisplay( "gcc_sl: processing queued event %s %s\n",
                   gcx_look( gcx_mde_service, mde->service_primitive ),
                   gcx_look( gcx_mde_primitive, mde->primitive_type ));
	}
# endif /* GCXDEBUG */

	status = gcc_sl_event( mde );
    }

    ccb->ccb_sce.sl_evq.active = FALSE;		/* Processing done */

    return( status );
} /* end gcc_sl */


/*
** Name: gcc_sl_event
**
** Description:
**	Session layer event processing.  An input MDE is evaluated
**	to a SL event.  The input event causes a state transition and
**	the associated outputs and actions are executed.
**	
** Input:
**	mde		Current MDE event.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**      18-Jan-88 (jbowers)
**          Initial routine creation
**      05-Mar-90 (cmorris)
**	    Make error handling consistent with other layers'
**	19-Jun-96 (gordy)
**	    Renamed from gcc_sl() to handle recursive requests.
**	    This routine will no longer be called recursively.
*/

static STATUS
gcc_sl_event( GCC_MDE *mde )
{
    u_i4	input_event;
    STATUS	status;
    
    /*
    ** Invoke  the FSM executor, passing it the current state and the 
    ** input event, to execute the output event(s), the action(s)
    ** and return the new state.  The input event is determined by the
    ** output of gcc_sl_evinp().
    */
    input_event = gcc_sl_evinp(mde, &status);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
        TRdisplay( "gcc_sl: primitive %s %s event %s size %d\n",
                gcx_look( gcx_mde_service, mde->service_primitive ),
                gcx_look(gcx_mde_primitive, mde->primitive_type ),
                gcx_look( gcx_gcc_isl, input_event ),
                (i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */

    if ( status == OK)
    {
	i4		output_event;
	i4		i;
	i4		j;
	GCC_CCB		*ccb = mde->ccb;
	u_i4		old_state;

	/*
	** Determine the state transition for this state and input event and
	** return the new state.
	*/

	old_state = ccb->ccb_sce.s_state;
	j = slfsm_tbl[input_event][old_state];

	/* Make sure we have a legal transition */
	if (j != 0)
	{
	    ccb->ccb_sce.s_state = slfsm_entry[j].state;

	    /* do tracing... */
	    GCX_TRACER_3_MACRO("gcc_slfsm>", mde, "event = %d, state = %d->%d",
			    input_event, old_state, (u_i4)ccb->ccb_sce.s_state);

# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >=1 )
		TRdisplay( "gcc_slfsm: new state=%s->%s\n", 
			   gcx_look(gcx_gcc_ssl, old_state), 
			   gcx_look(gcx_gcc_ssl, (u_i4)ccb->ccb_sce.s_state) );
# endif /* GCXDEBUG */

	    /*
	    ** For each output event in the list for this state and input event,
	    ** call the output executor.
	    */

	    for (i = 0;
		 i < SL_OUT_MAX
		 &&
		 (output_event =
		 slfsm_entry[j].output[i])
		 > 0;
		 i++)
	    {
		if ((status = gcc_slout_exec(output_event, mde, ccb )) != OK)
		{
		    /*
		    ** Execution of the output event has failed. Aborting the 
		    ** connection will already have been handled so we just get
		    ** out of here.
		    */
		    return (status);
		} /* end if */
	    } /* end for */
	} /* end if */
	else 
	{
	    GCC_ERLIST		erlist;

	    /*
	    ** This is an illegal transition.  This is a program bug or a
	    ** protocol violation.
	    ** Log the error.  The message contains 2 parms: the input event and
	    ** the current state.
	    */

	    erlist.gcc_parm_cnt = 2;
	    erlist.gcc_erparm[0].size = sizeof(input_event);
	    erlist.gcc_erparm[0].value = (PTR) &input_event;
	    erlist.gcc_erparm[1].size = sizeof(old_state);
	    erlist.gcc_erparm[1].value = (PTR) &old_state;
	    status = E_GC2604_SL_FSM_STATE;

	    /* Abort the connection */
	    gcc_sl_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL, &erlist);
	    status = FAIL;
	}
    }
    else
    {
	/* Log the error and abort the connection. */

	gcc_sl_abort(mde, mde->ccb, &status, (CL_ERR_DESC *) NULL,
			(GCC_ERLIST *) NULL);
	status = FAIL;
    } /* end else */
    return(status);
}  /* end gcc_sl_event */

/*{
** Name: gcc_slinit	- Session Layer initialization routine
**
** Description:
**      gcc_slinit is invoked by ILC on comm server startup.  It initializes
**      the PL and returns.  There are currently no PL initialization functions.
** 
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code for initialization error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-Jan-88 (jbowers)
**          Initial routine creation
*/
STATUS
gcc_slinit( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    STATUS              return_code = OK;

    *generic_status = OK;
    return(return_code);
}  /* end gcc_slinit */

/*{
** Name: gcc_slterm	- Session Layer termination routine
**
** Description:
**      gcc_slterm is invoked by ILC on comm server termination.  It terminates
**      the SL and returns.  There are currently no SL termination functions.
** 
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code for termination error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-Jan-88 (jbowers)
**          Initial routine creation
*/
STATUS
gcc_slterm( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    STATUS              return_code = OK;

    *generic_status = OK;
    return(return_code);
}  /* end gcc_slterm */

/*{
** Name: gcc_sl_evinp	- PL FSM input event id evaluation routine
**
** Description:
**      gcc_sl_evinp accepts as input an MDE received by the SL.  It  
**      evaluates the input event based on the MDE type and possibly on  
**      some other status information.  It returns as its function value the  
**      ID of the input event to be used by the FSM engine. 
** 
** Inputs:
**      mde			Pointer to dequeued Message Data Element
**
** Outputs:
**      generic_status		Mainline status code.
**
**	Returns:
**	    u_i4	input_event
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      25-Jan-88 (jbowers)
**          Initial function implementation
**      01-Jan-90 (cmorris)
**          Fixed tracer macro call
**	07-Mar-90 (cmorris)
**	    Added processing of S_P_ABORT
**      19-Jan-93 (cmorris)
**  	    Got rid of system_status argument:- never used.
**	31-Mar-06 (gordy)
**	    Sanity check length read from IO buffers.
*/
static u_i4
gcc_sl_evinp( GCC_MDE *mde, STATUS *generic_status)
{
    u_i4               input_event;

    *generic_status = OK;

    GCX_TRACER_3_MACRO("gcc_sl_evinp>", mde,
	"primitive = %d/%d, size = %d",
	    mde->service_primitive,
	    mde->primitive_type,
	    (i4)(mde->p2 - mde->p1));

    switch (mde->service_primitive)
    {
    case S_CONNECT:
    {
	switch (mde->primitive_type)
	{
	case RQS:
	{
	    input_event = ISSCQ;
	    break;
	}  /* end case RQS */
	case RSP:
	{
	    switch (mde->mde_srv_parms.sl_parms.ss_result)
	    {
	    case SS_ACCEPT:
	    {
		input_event = ISSCA;
		break;
	    }
	    case SS_USER_REJECT:
	    {
		input_event = ISSCJ;
		break;
	    }
	    default:
		{
		    *generic_status = E_GC2602_SL_FSM_INP;
		    break;
		}
	    } /* end switch */
	    break;
	} /* end case RSP */
	default:
	    {
		*generic_status = E_GC2602_SL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    } /* end case S_CONNECT */
    case S_U_ABORT:
    {
	input_event = ISSAU;
	break;
    } /* end case S_U_ABORT */
    case S_P_ABORT:
    {
	input_event = ISABT;
	break;
    } /* end case S_P_ABORT */
    case S_DATA:
    {
	input_event = ISSDQ;
	break;
    } /* end case S_DATA */
    case S_EXP_DATA:
    {
	input_event = ISSEQ;
	break;
    } /* end case S_EXP_DATA */
    case S_RELEASE:
    {
	switch (mde->primitive_type)
	{
	case RQS:
	{
	    input_event = ISSRQ;
	    break;
	}  /* end case RQS */
	case RSP:
	{
	    /*
	    ** Check to see if there's no collision or the collision has been
	    ** "cleared" by receipt of a DN SPDU
	    */
	    if ((!(mde->ccb->ccb_sce.s_flags & GCCSL_COLL)) ||
		((mde->ccb->ccb_sce.s_flags & GCCSL_COLL) && 
                 (mde->ccb->ccb_sce.s_flags & GCCSL_DNR)))
	    	input_event = ISSRS;
	    else

		/* See if there's a collision and we're the initiator */
		if ((mde->ccb->ccb_sce.s_flags & GCCSL_COLL) &
		    (mde->ccb->ccb_hdr.flags & CCB_OB_CONN))
		    input_event = ISRSCI;
		else
		    *generic_status = E_GC2602_SL_FSM_INP;
	    break;
	} /* end case RSP */
	default:
	    {
		*generic_status = E_GC2602_SL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    } /* end case S_RELEASE */
    case T_CONNECT:
    {
	switch (mde->primitive_type)
	{
	case IND:
	{
	    input_event = ISTCI;
	    break;
	}  /* end case IND */
	case CNF:
	{
	    input_event = ISTCC;
	    break;
	} /* end case CNF */
	default:
	    {
		*generic_status = E_GC2602_SL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    }  /* end case T_CONNECT */
    case T_DISCONNECT:
    {
	switch (mde->primitive_type)
	{
	case IND:
	{
	    input_event = ISTDI;
	    break;
	}  /* end case IND */
	default:
	    {
		*generic_status = E_GC2602_SL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    }  /* end case T_DISCONNECT */
    case T_DATA:
    {
	SL_ID		sl_id;
    	char		*pci;

	/*
	** Set a pointer to the SL PCI area in the MDE.  The pci_offset element
	** of the MDE is currently the offset to the start of the SL PCI area.
	*/
	sl_id.si = 0;
    	pci = mde->p1;
	if ( pci < mde->p2 )  pci += GCgeti1(pci, &sl_id.si);

	/* Identify the SPDU type from the identifier in the SPCI.  */
	switch (sl_id.si)
	{
	case SL_CN:
	{
	    input_event = ISCN;
	    break;
	} /* end case CN */
	case SL_AC:
	{
	    input_event = ISAC;
	    break;
	} /* end case AC */
	case SL_RF:
	{
	    input_event = ISRF;
	    break;
	} /* end case RF */
	case SL_DT:
	{
	    input_event = ISDT;
	    break;
	} /* end case DT */
	case SL_EX:
	{
	    input_event = ISEX;
	    break;
	} /* end case EX */
	case SL_AB:
	{
	    input_event = ISAB;
	    break;
	} /* end case AB */
	case SL_FN:
	{
	    input_event = ISFN;
	    break;
	} /* end case FN */
	case SL_DN:
	{
	    /* See if there's a release collision and we're the responder */
	    if ((mde->ccb->ccb_sce.s_flags & GCCSL_COLL) &&
		(mde->ccb->ccb_hdr.flags & CCB_IB_CONN))
		input_event = ISDNCR;
	    else
	        input_event = ISDN;
	    break;
	} /* end case DN */
	case SL_NF:
	{
	    input_event = ISNF;
	    break;
	} /* end case NF */
	default:
	    {
		*generic_status = E_GC2602_SL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    }  /* end case T_DATA */
    case S_XOFF:
    {
	/* S_XOFF has only a request form */

	input_event = ISSXF;
	break;
    }  /* end case S_XOFF */
    case S_XON:
    {
	/* S_XON has only a request form */

	input_event = ISSXN;
	break;
    }  /* end case S_XON */
    case T_XOFF:
    {

	input_event = ISTXF;
	break;
	/* T_XOFF has only an indication form */
    }  /* end case T_XOFF */
    case T_XON:
    {
	/* T_XON has only an indication form */

	input_event = ISTXN;
	break;
    }  /* end case T_XON */
    default:
	{
	    *generic_status = E_GC2602_SL_FSM_INP;
	    break;
	}
    }  /* end switch */
    return(input_event);
} /* end gcc_sl_evinp */

/*{
** Name: gcc_slout_exec	- SL FSM output event execution routine
**
** Description:
**      gcc_slout_exec accepts as input an output event identifier.
**	It executes this output event and returns.
 **
**	It also accepts as input a pointer to an 
**	MDE or NULL.  If the pointer is non-null, the MDE pointed to is
**	used for the output event.  Otherwise, a new one is obtained.
** 
**
** Inputs:
**	output_event		Identifier of the output event
**      mde			Pointer to dequeued Message Data Element
**
**	Returns:
**	    OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      25-Jul-88 (jbowers)
**          Initial function implementation
**	19-jun-89 (seiwald)
**	    Adjust sizeof( SL_TDISC ) -- contains only 3 bytes, though
**	    sizeof( SL_TDISC ) is 4.  When C structures are no longer
**	    used to describe the protocols this size shuffling should
**	    go away.
**      06-Sep-89 (cmorris)
**          More C structure nonsense:- don't use sizeof on odd byte
**          structure. Ditto comments above...
**      05-Mar-90 (cmorris)
**	    Pass back status from calling another layer; this prevents
**	    us ploughing on with events and actions oblvious to the fact
**	    that we've already hit a fatal error.
**  	09-Jan-91 (cmorris)
**  	    Check status returned by gcc_get_obj.
**  	28-Dec-92 (cmorris)
**  	    Removed tl_parms that are unused.
**  	04-Jan-92 (cmorris)
**  	    Removed copy of remote network address:- it's now held in
**  	    the CCB.
**	 3-Jun-97 (gordy)
**	    The initial connection info, passed as user data, is now
**	    built here rather than in the AL.  This will allow future
**	    extensions based on negotiated protocol level after the
**	    T_CONNECT request is processed (which can't be done in
**	    the AL).
**	26-Aug-97 (gordy)
**	    Connection info now associated with CN SPDU now built
**	    built by SL at protocol level 2.  Added support for
**	    extended length indicators.  General cleanup of PCI's.
**	10-Oct-97 (rajus01)
**	    Added password encryption capability.
**	 4-Dec-97 (rajus01)
**	    Added remote authentication to connection info.
**	20-Mar-98 (gordy)
**	    Cleaned up defaults for security labels.
**	20-May-98 (gordy)
**	    Authentications may be longer then 254 bytes,
**	    so use extended length indicator format.
**	26-Jun-98 (gordy)
**	    Use separate pci packet in OSSCI for extended user data
**	    so as to not step on values used by the outer loop.
**	10-Jul-00 (gordy)
**	    Kludged ABORT handling to ignore length indicators which
**	    are being sent incorrectly (see OSAB and OSSAX).
**	12-Jul-00 (gordy)
**	    Fixed all SL PCI length indicators to handle normal and
**	    extended formats (cleans up the kludge of the previous fix).
**	16-Jun-04 (gordy)
**	    Added additional mask to password encryption key.
**	15-Jul-04 (gordy)
**	    Changed password buffer type to match encode/decode types.
**	31-Mar-06 (gordy)
**	    Sanity check length read from IO buffers.
**	25-Apr-06 (gordy)
**	    Fix subscript error in prior submission.
**	21-Jul-09 (gordy)
**	    Username and password dynamically allocated.  Remove fixed 
**	    length string/buffer restrictions.  Remove security label
**	    stuff.  Declare standard sized password buffer.  Use
**	    dynamic storage if size exceeds default.
**	17-Dec-09 (Bruce Lunsford)  Sir 122536
**	    In support of long names (user, password), modify send
**	    of SL user/password connect info to use extended format
**	    which has a 2-byte length and can handle the longer strings.
**	    Remove unref'd variable e_passwd.
*/

static STATUS
gcc_slout_exec( i4  output_event, GCC_MDE *mde, GCC_CCB *ccb )
{
    STATUS	call_status = OK;
    STATUS	status;
    i4	layer_id;
    char	*pci;
    char	*endpci;
    char	*user_data;
    i4		l_user_data;
    u_i2	li;

    status = OK;
# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >= 2 )
		TRdisplay( "gcc_slout_exec: output event %s\n",
			gcx_look( gcx_gcc_osl, output_event ) );
# endif /* GCXDEBUG */

    switch (output_event)
    {
    case OSSCI:
    {
	/*
	** Send an S_CONNECT indication to PL.  The  SPDU is CN.
	*/
	if ( ccb->ccb_hdr.flags & CCB_PWD_ENCRYPT )
	{
	    SL_XSPDU	cni_pci;
	    char	*parm, *end_parm;
	    char	*user_data;
	    u_i2	tmpi2;
	    char 	*user_key = NULL;
	    u_char	*passwd_ptr = NULL;
	    i4		passwd_len = 0;
	    STATUS	status;

	    /*
	    ** The TL negotiated protocol level is represented
	    ** by VRSN_1.  Higher protocol levels are negotiated
	    ** with the CN SPDU.
	    */
	    ccb->ccb_sce.s_version = SL_PROTOCOL_VRSN_1;

	    /*
	    ** Provide defaults for SL CN parameters.
	    */
	    user_data = mde->papp;
	    l_user_data = 0;
	    ccb->ccb_hdr.gca_flags = 0;
	    ccb->ccb_hdr.gca_protocol = 0;
	    ccb->ccb_hdr.authlen = 0;

	    /*
	    ** Read the SL CN header.
	    */
	    pci = mde->p1;
	    pci += GCgeti1( pci, &cni_pci.id.si );
	    SL_GET_LI( pci, cni_pci.id.li );
	    cni_pci.id.li = (pci > mde->p2) 
	    		    ? 0 : min( cni_pci.id.li, mde->p2 - pci );
	    endpci = pci + cni_pci.id.li;

	    /*
	    ** Read the SL CN parameter group units.
	    */
	    while( pci < endpci )
	    {
		pci += GCgeti1( pci, &cni_pci.pgiu.pgi );
		SL_GET_LI( pci, cni_pci.pgiu.li );
		cni_pci.pgiu.li = (pci > endpci) 
				  ? 0 : min( cni_pci.pgiu.li, endpci - pci );

		switch( cni_pci.pgiu.pgi )
		{
		    case SL_VRSN_PI :
			if ( cni_pci.pgiu.li == 1 )
			{
			    /* Negotiate SL protocol level */
			    u_i1 tmpi1;
			    GCgeti1( pci, &tmpi1 );
			    ccb->ccb_sce.s_version = 
				min( tmpi1, SL_PROTOCOL_VRSN );
			}
			break;

		    case SL_UDATA_PGI :
			/*
			** When we are done with the SL PCI,
			** we want to send the AL/PL PCI's
			** up to be processed.  Save the
			** PCI's and continue processing.
			*/
			user_data = pci;
			l_user_data = cni_pci.pgiu.li;
			break;
		    
		    case SL_XUDATA_PGI :
			/*
			** Read the parameters contained in the
			** extended user data parameter group.
			*/
			parm = pci;
			end_parm = pci + cni_pci.pgiu.li;

			while( parm < end_parm )
			{
			    SL_XSPDU	xud_pci;

			    parm += GCgeti1( parm, &xud_pci.pgiu.pgi );
			    SL_GET_LI( parm, xud_pci.pgiu.li );
			    xud_pci.pgiu.li = (parm > end_parm) ? 0 : 
			    		min(xud_pci.pgiu.li, end_parm - parm);

			    switch( xud_pci.pgiu.pgi )
			    {
				case SL_FLAG_XPI :
				    if ( xud_pci.pgiu.li >= 2 )
				    {
					GCgeti2( parm, &tmpi2 );
					ccb->ccb_hdr.gca_flags = tmpi2;
				    }
				    break;

				case SL_PROT_XPI :
				    if ( xud_pci.pgiu.li >= 2 )
				    {
					GCgeti2( parm, &tmpi2 );
					ccb->ccb_hdr.gca_protocol = tmpi2;
				    }
				    break;

				case SL_USER_XPI :
				    if ( xud_pci.pgiu.li )
				    {
					/*
					** Save NTF form of username as
					** decryption key for password.
					** Translate username to local
					** charset.
					*/
					user_key = parm;
					ccb->ccb_hdr.username = 
					    (char *)gcc_alloc(xud_pci.pgiu.li);
					GCngets( parm, xud_pci.pgiu.li,
						 ccb->ccb_hdr.username );
				    	ccb->ccb_hdr.flags |= CCB_USERNAME;
				    }
				    break;

				case SL_PASS_XPI :
				    if ( xud_pci.pgiu.li )
				    {
					passwd_ptr = (u_char *)parm;
					passwd_len = xud_pci.pgiu.li;
				    }
				    break;

				case SL_SECT_XPI :
				    /* Deprecated */
				    break;

				case SL_SECL_XPI :
				    /* Deprecated */
				    break;

				case SL_AUTH_XPI :
				    if ( xud_pci.pgiu.li )
				    {
					ccb->ccb_hdr.auth = 
					    MEreqmem( 0, (u_i4)xud_pci.pgiu.li,
						      FALSE, &status);

					if ( ccb->ccb_hdr.auth )
					{
					    MEcopy( (PTR)parm, xud_pci.pgiu.li,
						    ccb->ccb_hdr.auth );
					    ccb->ccb_hdr.authlen = 
					        xud_pci.pgiu.li;
				        }
				    }
				    break;

				default :	/* Ignored */
				    break;
			    }

			    parm += xud_pci.pgiu.li;
			}
			break;

		    default :	 /* Ignored. */
			break;
		}

		pci += cni_pci.pgiu.li;
	    }

	    /*
	    ** Decrypt password if username/password pair received.
	    ** Password must be converted from NTS form to local
	    ** after decryption.
	    */
	    if ( user_key  &&  passwd_len )
	    {
		char 	buff[ 128 ];
		char	*pwd = (passwd_len >= sizeof( buff )) 
			       ? (char *)gcc_alloc( passwd_len + 1 ) : buff;

		if ( pwd )
		{
		    gcc_decode( passwd_ptr, passwd_len, user_key, 
				(ccb->ccb_hdr.mask_len > 0) 
				? ccb->ccb_hdr.mask : NULL, pwd );

		    passwd_len = STlength( pwd );
		    ccb->ccb_hdr.password = (char *)gcc_alloc(passwd_len + 1);

		    if ( ccb->ccb_hdr.password )
		    {
			GCngets( pwd, passwd_len, ccb->ccb_hdr.password );
			ccb->ccb_hdr.password[ passwd_len ] = EOS;
		    }

		    if ( pwd != buff )  gcc_free( (PTR)pwd );
	        }
	    }

	    /*
	    ** Reset pointers for the PL/AL PCIs.
	    */
	    mde->p1 = user_data;
	    mde->p2 = user_data + l_user_data;
	}
	else
	{
	    SL_SPDU	cni_pci;
	
	    /*
	    ** This is the original protocol level, and
	    ** no further protocol negotiation is done.
	    */
	    ccb->ccb_sce.s_version = SL_PROTOCOL_VRSN_0;

	    /*
	    ** The connection user data and AL/PC PCI's
	    ** are encapsulated in the SL user data PGI,
	    ** which is required to be the last SL parm.
	    */
	    pci = mde->p1;
	    pci += GCgeti1( pci, &cni_pci.id.si );
	    pci += GCgeti1( pci, &cni_pci.id.li );
	    cni_pci.id.li = (pci > mde->p2) 
	    		    ? 0 : min( cni_pci.id.li, mde->p2 - pci );
	    endpci = pci + cni_pci.id.li;

	    while( pci < endpci )
	    {
		pci += GCgeti1(pci, &cni_pci.pgiu.pgi);
		pci += GCgeti1(pci, &cni_pci.pgiu.li);
		cni_pci.pgiu.li = (pci > endpci) 
				  ? 0 : min( cni_pci.pgiu.li, endpci - pci );

		if ( cni_pci.pgiu.pgi == SL_UDATA_PGI )  break;

	        pci += cni_pci.pgiu.li;
	    } 

	    mde->p1 = pci;
	}

	mde->service_primitive = S_CONNECT;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OSSCI */
    case OSSCA:
    {
	SL_SPDU	aci_pci;

	/*
	** Send an S_CONNECT confirm/accept to PL.  The SPDU is AC.
	*/
	pci = mde->p1;
	pci += GCgeti1(pci, &aci_pci.id.si);
	SL_GET_LI( pci, li );
	li = (pci > mde->p2) ? 0 : min( li, mde->p2 - pci );
	endpci = pci + li;

	/*
	** Provide defaults for AC parameters.
	*/
	user_data = mde->papp;
	l_user_data = 0;

	while( pci < endpci )
	{
	    pci += GCgeti1( pci, &aci_pci.pgiu.pgi );
	    SL_GET_LI( pci, li );
	    li = (pci > endpci) ? 0 : min( li, endpci - pci );

	    switch( aci_pci.pgiu.pgi )
	    {
		case SL_VRSN_PI :
		    if ( li == 1 )
		    {
			/* Negotiate SL protocol level */
			u_i1 tmpi1;
			GCgeti1( pci, &tmpi1 );
			ccb->ccb_sce.s_version = min( tmpi1, SL_PROTOCOL_VRSN );
		    }
		    break;

		case SL_UDATA_PGI :
		    /*
		    ** Save the AL/PL PCI's.
		    */
		    if ( li )
		    {
			user_data = pci;
			l_user_data = li;
		    }
		    break;
		
		default :	/* Ignored! */
		    break;
	    }

	    pci += li;
	} 

	/*
	** Pass AL/PL PCI's up for processing.
	*/
	mde->p1 = user_data;
	mde->p2 = mde->p1 + l_user_data;

	mde->service_primitive = S_CONNECT;
	mde->primitive_type = CNF;
	mde->mde_srv_parms.sl_parms.ss_result = SS_ACCEPT;
	layer_id = UP;
	break;
    } /* end case OSSCA */
    case OSSCJ:
    {
	SL_SPDU	rfi_pci;
	char	*parm;
	u_char	reason;

	/*
	** Send an S_CONNECT confirm/reject to PL.  The SPDU is RF.
	*/
	pci = mde->p1;
	pci += GCgeti1( pci, &rfi_pci.id.si );
	SL_GET_LI( pci, li );
	li = (pci > mde->p2) ? 0 : min( li, mde->p2 - pci );
	endpci = pci + li;

	/*
	** Provide defaults for RF parameters.
	*/
	reason = SS_RJ_USER;
	user_data = mde->papp;
	l_user_data = 0;

	while( pci < endpci )
	{
	    pci += GCgeti1( pci, &rfi_pci.pgiu.pgi );
	    SL_GET_LI( pci, li );
	    li = (pci > endpci) ? 0 : min( li, endpci - pci );
	    parm = pci;

	    switch( rfi_pci.pgiu.pgi )
	    {
		case SL_RSN_PI :
		    /*
		    ** Save the reason code and AL/PL PCI's.
		    */
		    if ( li )
		    {
			parm += GCgeti1( parm, &reason );
			user_data = parm;
			l_user_data = li - 1;
		    }
		    break;
		
		default :	/* Ignore! */
		    break;
	    }

	    pci += li;
	}

	/*
	** Pass AL/PL PCI's and reason code up for processing.
	*/
	mde->p1 = user_data;
	mde->p2 = mde->p1 + l_user_data;
	mde->mde_srv_parms.sl_parms.ss_result = reason;

	mde->service_primitive = S_CONNECT;
	mde->primitive_type = CNF;
	layer_id = UP;
	break;
    } /* end case OSSCJ */
    case OSSAX:
    {
	/*
	** Send an S_U_ABORT or an S_P_ABORT indication to PL.
	*/

	/*
	** The original gcc code did not handle the length
	** indicator correctly, but basically worked because
	** the length was ignored for the most part.  We now
	** generate extended lengths when needed.
	*/
	if ( ccb->ccb_sce.s_version >= SL_PROTOCOL_VRSN_1 )
	{
	    SL_SPDU	abi_pci;
	    u_char	tdisc;

	    pci = mde->p1;
	    pci += GCgeti1( pci, &abi_pci.id.si );
	    SL_GET_LI( pci, li );
	    li = (pci > mde->p2) ? 0 : min( li, mde->p2 - pci );
	    endpci = pci + li;

	    /*
	    ** Provide defaults for RF parameters.
	    */
	    mde->service_primitive = S_U_ABORT;
	    user_data = mde->papp;
	    l_user_data = 0;

	    while( pci < endpci )
	    {
		pci += GCgeti1(pci, &abi_pci.pgiu.pgi);
		SL_GET_LI( pci, li );
		li = (pci > endpci) ? 0 : min( li, endpci - pci );

		switch( abi_pci.pgiu.pgi )
		{
		    case SL_TDISC_PI :
			if ( li )
			{
			    GCgeti1( pci, &tdisc );
			    if ( tdisc != SL_USER )  
				mde->service_primitive = S_P_ABORT;
			}
			break;

		    case SL_UDATA_PGI :
			/*
			** Save the AL/PL PCI's.
			*/
			if ( li )
			{
			    user_data = pci;
			    l_user_data = li;
			}
			break;

		    default :	/* Ignored! */
			break;
		}

		pci += li;
	    }

	    /*
	    ** Pass AL/PL PCI's up for processing.
	    */
	    mde->p1 = user_data;
	    mde->p2 = mde->p1 + l_user_data;
	}
	else
	{
	    SL_SPDU	abi_pci;
	    u_char	tdisc;
	
	    pci = mde->p1;
	    pci += GCgeti1(pci, &abi_pci.id.si);

	    /*
	    ** Original GCC code only used a single byte
	    ** length indicator, even though messages can
	    ** easily be greater then 255 bytes.  Ignore
	    ** the indicator and use the full message.
	    */
	    pci += GCgeti1(pci, &abi_pci.id.li);
	    endpci = mde->p2;

	    /*
	    ** First find the Transport Disconnect parameter 
	    ** to determine if this is a user or provider abort.
	    */
	    while( pci < endpci )
	    {
		pci += GCgeti1(pci, &abi_pci.pgiu.pgi);
		pci += GCgeti1(pci, &abi_pci.pgiu.li);
		abi_pci.pgiu.li = (pci > endpci) 
				  ? 0 : min( abi_pci.pgiu.li, endpci - pci );

	        if ( abi_pci.pgiu.pgi == SL_TDISC_PI )
		{
		    if ( abi_pci.pgiu.li )
		    {
			pci += GCgeti1(pci, &tdisc);
			mde->service_primitive = (tdisc == SL_USER) 
						 ? S_U_ABORT : S_P_ABORT;
		    }

		    break;
		}

		pci += abi_pci.pgiu.li;
	    }


	    /*
	    ** Now find the user data parameter.
	    */
	    while( pci < endpci )
	    {
		pci += GCgeti1(pci, &abi_pci.pgiu.pgi);
		pci += GCgeti1(pci, &abi_pci.pgiu.li);
		abi_pci.pgiu.li = (pci > endpci) 
				  ? 0 : min( abi_pci.pgiu.li, endpci - pci );

		if ( abi_pci.pgiu.pgi == SL_UDATA_PGI )  break;
	        pci += abi_pci.pgiu.li;
	    }

	    mde->p1 = pci;
	}

	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OSSAX */
    case OSSAP:
    {
	/*
	** Send an S_P_ABORT indication to PL.  There was no SPDU from the peer;
	** this is an internal error.  Probably the transport connection broke.
	** Alternatively, this is an SL internal error.
	*/

	switch (mde->service_primitive)
	{
	case T_DISCONNECT:
	{
	    mde->mde_srv_parms.sl_parms.ss_result = SL_TCREL;
	    break;
	}
	default:    /* SL internal error */
	    {
		mde->mde_srv_parms.sl_parms.ss_result = SL_UNDEF;
		break;
	    }
	} /* end switch */

	mde->service_primitive = S_P_ABORT;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OSSAP */
    case OSSDI:
    {
	SL_ID	dti_pci;

	/*
	** Send an S_DATA indication to PL.  The SPDU was DT.
	** Data follows the DT SPDU (parameters are ignored).
	*/
	pci = mde->p1;
	pci += GCgeti1( pci, &dti_pci.si );
	SL_GET_LI( pci, li );
	li = (pci > mde->p2) ? 0 : min( li, mde->p2 - pci );
	pci += li;
	mde->p1 = pci;

	mde->service_primitive = S_DATA;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OSSDI */
    case OSSEI:
    {
	SL_ID	exi_pci;

	/*
	** Send an S_EXP_DATA indication to PL.  The SPDU was EX.
	** Data follows the EX SPDU (parameters are ignored).
	*/
	pci = mde->p1;
	pci += GCgeti1( pci, &exi_pci.si );
	SL_GET_LI( pci, li );
	li = (pci > mde->p2) ? 0 : min( li, mde->p2 - pci );
	pci += li;
	mde->p1 = pci;

	mde->service_primitive = S_EXP_DATA;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OSSEI */
    case OSSRI:
    {
	SL_SPDU	fni_pci;

	/*
	** Send an S_RELEASE indication to PL.  The SPDU is FN.
	*/
	pci = mde->p1;
	pci += GCgeti1(pci, &fni_pci.id.si);
	SL_GET_LI( pci, li );
	li = (pci > mde->p2) ? 0 : min( li, mde->p2 - pci );
	endpci = pci + li;

	/*
	** Provide defaults for FN parameters.
	*/
	user_data = mde->papp;
	l_user_data = 0;

	while( pci < endpci )
	{
	    pci += GCgeti1( pci, &fni_pci.pgiu.pgi );
	    SL_GET_LI( pci, li );
	    li = (pci > endpci) ? 0 : min( li, endpci - pci );

	    switch( fni_pci.pgiu.pgi )
	    {
		case SL_UDATA_PGI :
		    /*
		    ** Save the AL/PL PCI's.
		    */
		    if ( li )
		    {
			user_data = pci;
			l_user_data = li;
		    }
		    break;
		
		default :	/* Ignored! */
		    break;
	    }

	    pci += li;
	}

	/*
	** Pass AL/PL PCI's up for processing.
	*/
	mde->p1 = user_data;
	mde->p2 = mde->p1 + l_user_data;

	mde->service_primitive = S_RELEASE;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OSSRI */
    case OSRCA:
    {
	SL_SPDU	dni_pci;

	/*
	** Send an S_RELEASE confirm/accept to PL.  The SPDU is DN.
	*/
	pci = mde->p1;
	pci += GCgeti1( pci, &dni_pci.id.si );
	SL_GET_LI( pci, li );
	li = (pci > mde->p2) ? 0 : min( li, mde->p2 - pci );
	endpci = pci + li;

	/*
	** Provide defaults for DN parameters.
	*/
	user_data = mde->papp;
	l_user_data = 0;

	while( pci < endpci )
	{
	    pci += GCgeti1( pci, &dni_pci.pgiu.pgi );
	    SL_GET_LI( pci, li );
	    li = (pci > endpci) ? 0 : min( li, endpci - pci );

	    switch( dni_pci.pgiu.pgi )
	    {
		case SL_UDATA_PGI :
		    /*
		    ** Save the AL/PL PCI's.
		    */
		    if ( li )
		    {
			user_data = pci;
			l_user_data = li;
		    }
		    break;
		
		default :	/* Ignored! */
		    break;
	    }

	    pci += li;
	}

	/*
	** Pass AL/PL PCI's up for processing.
	*/
	mde->p1 = user_data;
	mde->p2 = mde->p1 + l_user_data;

	mde->service_primitive = S_RELEASE;
	mde->primitive_type = CNF;
	mde->mde_srv_parms.sl_parms.ss_result = SS_AFFIRMATIVE;
	layer_id = UP;
	break;
    } /* end case OSRCA */
    case OSRCJ:
    {
	SL_SPDU	nfi_pci;

	/*
	** Send an S_RELEASE confirm/reject to PL.  The SPDU is NF.
	*/
	pci = mde->p1;
	pci += GCgeti1(pci, &nfi_pci.id.si);
	SL_GET_LI( pci, li );
	li = (pci > mde->p2) ? 0 : min( li, mde->p2 - pci );
	endpci = pci + li;
	
	/*
	** Provide defaults for NF parameters.
	*/
	user_data = mde->papp;
	l_user_data = 0;

	while( pci < endpci )
	{
	    pci += GCgeti1( pci, &nfi_pci.pgiu.pgi );
	    SL_GET_LI( pci, li );
	    li = (pci > endpci) ? 0 : min( li, endpci - pci );

	    switch( nfi_pci.pgiu.pgi )
	    {
		case SL_UDATA_PGI :
		    /*
		    ** Save the AL/PL PCI's.
		    */
		    if ( li )
		    {
			user_data = pci;
			l_user_data = li;
		    }
		    break;
		
		default :	/* Ignored! */
		    break;
	    }

	    pci += li;
	}

	/*
	** Pass AL/PL PCI's up for processing.
	*/
	mde->p1 = user_data;
	mde->p2 = mde->p1 + l_user_data;

	mde->service_primitive = S_RELEASE;
	mde->primitive_type = IND;
	mde->mde_srv_parms.sl_parms.ss_result = SS_NEGATIVE;
	layer_id = UP;
	break;
    } /* end case OSRCJ */
    case OSTCQ:
    {
	GCC_MDE		*tcq_mde;
	
	/*
	** Send a T_CONNECT request to TL.
	**
	** This is the result of an S_CONNECT request.  The MDE for the
	** S_CONNECT request must be saved in the CCB, since it contains
    	** user data from the PL. It will be subsequently used to construct
    	** a CN SPDU when the Transport Connection has been established.  
    	** To generate the T_CONNECT, get a new MDE, construct the  T_CONNECT
    	** primitive and copy the called address from the S_CONNECT MDE.
	*/

	tcq_mde = gcc_add_mde( ccb, 0 );
  	if (tcq_mde == NULL)
	{
	    /* Log an error and abort the connection */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_sl_abort( tcq_mde, ccb, &status, NULL, NULL );
	    return(FAIL);
	} /* end if */

     	tcq_mde->service_primitive = T_CONNECT;
	tcq_mde->primitive_type = RQS;
	ccb->ccb_sce.s_con_mde = mde;
	tcq_mde->ccb = 	ccb;
	mde = tcq_mde;

	/*
	** Send the SDU to TL 
	*/

	layer_id = DOWN;
	break;
    } /* end case OSTCQ */
    case OSTCS:
    {
	/*
	** Send a T_CONNECT response to TL.
	*/

    	mde->p1 = mde->p2 = mde->papp;
	mde->service_primitive = T_CONNECT;
	mde->primitive_type = RSP;

	/*
	** Send the SDU to TL 
	*/

	layer_id = DOWN;
	break;
    }/* end case OSTCS */
    case OSTDC:
    {
	GCC_MDE		*tdc_mde;

	/*
	** Send a T_DISCONNECT request.
	*/

	tdc_mde = gcc_add_mde( ccb, 0 );
  	if (tdc_mde == NULL)
	{
	    /* Log an error and abort the connection */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_sl_abort(tdc_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	} /* end if */

	tdc_mde->service_primitive = T_DISCONNECT;
	tdc_mde->primitive_type = RQS;
	tdc_mde->ccb = ccb;
	mde = tdc_mde;

	/*
	** Send the SDU to TL
	*/

	layer_id = DOWN;
	break;
    }/* end case OSTDC */
    case OSCN:
    {
	/*
	** Send a CN SPDU.  This flows on a T_DATA request.  
	** It carries a User Data parm unit.
	** 
	** The current CCB points to a previously saved MDE 
	** containing an S_CONNECT primitive.  This is the 
	** MDE in which the CN is constructed.  It carries 
	** pci's from upper layers.
	*/
	mde = ccb->ccb_sce.s_con_mde;
	ccb->ccb_sce.s_con_mde = NULL;

	/*
	** Originally, the connection user data was processed 
	** by the AL and sent in the SL user data PGI along 
	** with the AL and PL PCI's.  The connection data is 
	** now processed by the SL and sent in the SL extended 
	** user data PGI (the AL/PC PCI's are still sent in the 
	** SL user data PGI).  The TL negotiates the initial
	** protocol level and signals the new capability with
	** the password encryption flag.
	*/
	if ( ccb->ccb_hdr.flags & CCB_PWD_ENCRYPT )
	{
	    char	*xud_start, *xud_len_ptr;
	    char	*tmp, *user_key;
	    i4		len;

	    /*
	    ** The TL negotiated protocol level is represented
	    ** by VRSN_1.  Higher protocol levels are negotiated
	    ** with the CN SPDU.
	    */
	    ccb->ccb_sce.s_version = SL_PROTOCOL_VRSN_1;

	    /*
	    ** Build the CN SPDU.
	    **
	    **	1. Encapsulate AL/PL PCI's as SL user data.
	    **	2. Append session info as SL extended user data.
	    **	3. Build SPDU header with ID and parameters.
	    */

	    /*
	    ** 1. Encapsulate the AL/PL PCI's in the SL user data.
	    */
	    l_user_data = mde->p2 - mde->p1;
	    pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XPGIU : SZ_SL_PGIU);
	    pci += GCaddn1( SL_UDATA_PGI, pci );
	    SL_ADD_LI( l_user_data, pci );

	    /*
	    ** 2. Append session info as SL extended user data.
	    ** We use extended length indicator since we won't 
	    ** know the actual length until complete.
	    */
	    pci = mde->p2;
	    pci += GCaddn1( SL_XUDATA_PGI, pci );
	    pci += GCaddn1( SL_XLI, pci );
	    xud_len_ptr = pci;			/* Save for later update */
	    pci += GCaddn2( 0, pci );
	    xud_start = pci;

	    /*
	    ** Now build the individual parameters for the
	    ** extended user data PGI.
	    */
	    pci += GCaddn1( SL_FLAG_XPI, pci );
	    pci += GCaddn1( 2, pci );
	    pci += GCaddn2( ccb->ccb_hdr.gca_flags, pci );

	    pci += GCaddn1( SL_PROT_XPI, pci );
	    pci += GCaddn1( 2, pci );
	    pci += GCaddn2( ccb->ccb_hdr.gca_protocol, pci );

	    /* 
	    ** Username must be converted to NTS form.  Translation
	    ** occurs in output buffer.  Buffer length not known
	    ** until translation complete, so length indicator is
	    ** temporarily initialized and updated afterwards.
	    ** Save reference to translated username to use as
	    ** password encryption key.
	    */
	    pci += GCaddn1( SL_USER_XPI, pci );

	    /*
	    ** To allow for worst-case scenario where the username
	    ** length is 255 (including EOS) or longer, force "extended"
	    ** format which has a 1-byte indicator followed by a 2-byte
	    ** length field (rather than just a 1 byte length field). 
	    */
	    tmp = pci + 1;			/* Length place holder */
	    SL_ADD_LI( 255, pci );		/* 255 forces extended fmt */

	    /* Translate to NTS form */
	    len = STlength( ccb->ccb_hdr.username );
	    len = min( len, mde->pend - pci - 1 );
	    len  = GCnadds( ccb->ccb_hdr.username, len, pci );
	    pci[ len++ ] = EOS;

	    user_key = pci;			/* Save for password key  */
	    pci += len;
	    GCaddn2( len, tmp );		/* Update length */

	    if ( ccb->ccb_hdr.password  &&  *ccb->ccb_hdr.password )
	    {
		char	buff[ 128 ];
		char	*pwd;
		
		len = STlength( ccb->ccb_hdr.password ) + 1;
		pwd = (len > sizeof( buff )) ? (char *)gcc_alloc( len ) : buff;

		if ( pwd  &&  (mde->pend - pci) >= GCC_ENCODE_LEN( len ) )
		{
		    /* 
		    ** Password is encrypted using username (in NTS form)
		    ** as the key.  Password must also be translated to
		    ** NTS form.
		    */
		    GCnadds( ccb->ccb_hdr.password, len, pwd );

		    pci += GCaddn1( SL_PASS_XPI, pci );

		    /*
		    ** To allow for worst-case scenario where the password
		    ** length is 255 (including EOS) or longer, force "extended"
		    ** format which has a 1-byte indicator followed by a 2-byte
		    ** length field (rather than just a 1 byte length field). 
		    */
		    tmp = pci + 1;		/* Length place holder */
		    SL_ADD_LI( 255, pci );	/* 255 forces extended fmt */

		    len = gcc_encode( pwd, user_key, 
				      (ccb->ccb_hdr.mask_len > 0) 
				      ? ccb->ccb_hdr.mask : NULL, pci );

		    pci += len;
		    GCaddn2( len, tmp );	/* Update length */
		    if ( pwd != buff )  gcc_free( (PTR)pwd );
	        }
	    }

	    if ( ccb->ccb_hdr.authlen )
	    {
		pci += GCaddn1( SL_AUTH_XPI, pci );
		pci += GCaddn1( SL_XLI, pci );
		pci += GCaddn2( ccb->ccb_hdr.authlen, pci );
	        MEcopy((PTR)ccb->ccb_hdr.auth, ccb->ccb_hdr.authlen, (PTR)pci);
	        pci += ccb->ccb_hdr.authlen;

		MEfree( ccb->ccb_hdr.auth );
		ccb->ccb_hdr.auth = NULL;
		ccb->ccb_hdr.authlen = 0;
	    }

	    /*
	    ** Fix up length which can now be determined.
	    */
	    GCaddn2( pci - xud_start, xud_len_ptr );
	    mde->p2 = pci;

	    /*
	    ** 3. Build SPDU header with ID and parameters.
	    **
	    ** Send our protocol version for negotiation.
	    */
	    pci = (mde->p1 -= SZ_SL_PIU + 1);
	    pci += GCaddn1( SL_VRSN_PI, pci );
	    pci += GCaddn1( 1, pci );
	    pci += GCaddn1( SL_PROTOCOL_VRSN, pci );

	    /*
	    ** Add the SPDU ID.
	    */
	    l_user_data = mde->p2 - mde->p1;
	    pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XID : SZ_SL_ID);
	    pci += GCaddn1( SL_CN, pci );
	    SL_ADD_LI( l_user_data, pci );
	}
	else
	{
	    /*
	    ** This is the original protocol level, and
	    ** no further protocol negotiation is done.
	    */
	    ccb->ccb_sce.s_version = SL_PROTOCOL_VRSN_0;

	    /*
	    ** Build the connection user data.  This was 
	    ** originally done in the AL but must now be
	    ** done here after the GCC capabilities have 
	    ** been negotiated by the TL.
	    */
	    gcc_set_conn_info( ccb, mde );
	    l_user_data = mde->p2 - mde->p1;

	    /*
	    ** Build SL PCI.  AL PCI, PL PCI and connection 
	    ** user data included in the SL user data PGI.
	    ** NOTE: the following code is broken!  The user
	    ** data length has grown larger than what can be
	    ** represented by a single octect length.  Since
	    ** the receiving code was pretty loose, things
	    ** still were able to work.  
	    ** 
	    */
	    pci = (mde->p1 -= SZ_SL_SPDU);
	    pci += GCaddn1( SL_CN, pci );
	    pci += GCaddn1( SZ_SL_PGIU + l_user_data, pci );
	    pci += GCaddn1( SL_UDATA_PGI, pci );
	    pci += GCaddn1( l_user_data, pci );
	}

	/*
	** Send the SDU to TL
	*/
	mde->service_primitive = T_DATA;
	mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    }/* end case OSCN */
    case OSAC:
    {
	/*
	** Send an AC SPDU.  This flows on a T_DATA request.
	**
	** 1. Encapsulate AL/PL PCI's as SL user data.
	** 2. Build SPDU header with ID and parameters.
	*/

	/*
	** 1. Encapsulate the AL/PL PCI's in the SL user data.
	*/
	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XPGIU : SZ_SL_PGIU);
	pci += GCaddn1( SL_UDATA_PGI, pci );
	SL_ADD_LI( l_user_data, pci );

	/*
	** 2. Build SPDU header with ID and parameters.
	*/
	if ( ccb->ccb_sce.s_version >= SL_PROTOCOL_VRSN_1 )
	{
	    /*
	    ** Send negotiated protocol version.
	    */
	    pci = (mde->p1 -= SZ_SL_PIU + 1);
	    pci += GCaddn1( SL_VRSN_PI, pci );
	    pci += GCaddn1( 1, pci );
	    pci += GCaddn1( ccb->ccb_sce.s_version, pci );
	}

	/*
	** Add the SPDU ID.
	*/
	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XID : SZ_SL_ID);
	pci += GCaddn1( SL_AC, pci );
	SL_ADD_LI( l_user_data, pci );

	mde->service_primitive = T_DATA;
	mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    }/* end case OSAC */
    case OSRF:
    {
	/*
	** Send an RF SPDU.  This flows on a T_DATA request.
	** The reason code parameter value consists of a
	** single octet reason code followed by the user
	** data.
	*/
	l_user_data = mde->p2 - mde->p1 + 1;
	pci = (mde->p1 -= ((l_user_data >= 255) ? SZ_SL_XPIU : SZ_SL_PIU) + 1);
	pci += GCaddn1( SL_RSN_PI, pci );
	SL_ADD_LI( l_user_data, pci );
	pci += GCaddn1( SS_RJ_USER, pci );

	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XID : SZ_SL_ID);
	pci += GCaddn1( SL_RF, pci );
	SL_ADD_LI( l_user_data, pci );

	mde->service_primitive = T_DATA;
	mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    }/* end case OSRF */
    case OSDT:
    {
	/*
	** Send an DT SPDU.  This flows on a T_DATA request.
	** There are no parameters sent with this SPDU.
	*/
	pci = (mde->p1 -= SZ_SL_ID);
	pci += GCaddn1( SL_DT, pci );
	pci += GCaddn1( 0, pci );

	mde->service_primitive = T_DATA;
	mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags =
            mde->mde_srv_parms.sl_parms.ss_flags;
	layer_id = DOWN;
	break;
    }/* end case OSDT */
    case OSEX:
    {
	/*
	** Send an EX SPDU.  This flows on a T_DATA request.
	** There are no parameters sent with this SPDU.
	*/
	pci = (mde->p1 -= SZ_SL_ID );
	pci += GCaddn1( SL_EX, pci );
	pci += GCaddn1( 0, pci );

	mde->service_primitive = T_DATA;
	mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags =
            mde->mde_srv_parms.sl_parms.ss_flags;
	layer_id = DOWN;
	break;
    }/* end case OSEX */
    case OSAB:
    {
	/*
	** Send an AB SPDU.  This flows on a T_DATA 
	** request.  The SPDU contains two parameters: 
	** transport disconnect (value is 1 octet) and 
	** user data.
	**
	** Old GCC code only sent single byte length
	** indicators.  A scan was made through the
	** SPDU length looking for the TDISC and user
	** data parameters, which were actually the
	** only things provided.  The user data length
	** was completely ignored.  The best thing we 
	** can do is make the user data indicator as 
	** large as possible (not 255 since this would 
	** signal a three byte length indicator), and 
	** only include the SL parameter headers in the 
	** SPDU length (which is much less than 255 and
	** will limit the scan to just relevant data).
	*/

	/*
	** First, encapsulate the AL/PL PCI's as user data.
	*/
	l_user_data = mde->p2 - mde->p1;
	if ( ccb->ccb_sce.s_version < SL_PROTOCOL_VRSN_1 )
	    l_user_data = min( 254, l_user_data );

	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XPGIU : SZ_SL_PGIU);
	pci += GCaddn1( SL_UDATA_PGI, pci );
	SL_ADD_LI( l_user_data, pci );

	/*
	** Now prepend the transport disconnect value.
	*/
	pci = (mde->p1 -= SZ_SL_PIU + 1); 
	pci += GCaddn1( SL_TDISC_PI, pci );
	pci += GCaddn1( 1, pci );
	pci += GCaddn1( SL_USER, pci );

	/*
	** Finally, add the SPDU ID.
	*/
	if ( ccb->ccb_sce.s_version >= SL_PROTOCOL_VRSN_1 )
	    l_user_data = mde->p2 - mde->p1;
	else
	    l_user_data = SZ_SL_PGIU + SZ_SL_PIU + 1;

	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XID : SZ_SL_ID);
	pci += GCaddn1( SL_AB, pci );
	SL_ADD_LI( l_user_data, pci );

	mde->service_primitive = T_DATA;
	mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    }/* end case OSAB */
    case OSFN:
    {
	/*
	** Send an FN SPDU.  This flows on a T_DATA request.
	*/
	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XPGIU : SZ_SL_PGIU);
	pci += GCaddn1( SL_UDATA_PGI, pci );
	SL_ADD_LI( l_user_data, pci );

	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XID : SZ_SL_ID);
	pci += GCaddn1( SL_FN, pci );
	SL_ADD_LI( l_user_data, pci );

	mde->service_primitive = T_DATA;
        mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    }/* end case OSFN */
    case OSDN:
    {
	/*
	** Send an DN SPDU.  This flows on a T_DATA request.
	*/
	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XPGIU : SZ_SL_PGIU);
	pci += GCaddn1( SL_UDATA_PGI, pci );
	SL_ADD_LI( l_user_data, pci );

	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XID : SZ_SL_ID);
	pci += GCaddn1( SL_DN, pci );
	SL_ADD_LI( l_user_data, pci );

	mde->service_primitive = T_DATA;
	mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    }/* end case OSDN */
    case OSNF:
    {
	/*
	** Send an NF SPDU.  This flows on a T_DATA request.
	*/
	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XPGIU : SZ_SL_PGIU);
	pci += GCaddn1( SL_UDATA_PGI, pci );
	SL_ADD_LI( l_user_data, pci );

	l_user_data = mde->p2 - mde->p1;
	pci = (mde->p1 -= (l_user_data >= 255) ? SZ_SL_XID : SZ_SL_ID);
	pci += GCaddn1( SL_NF, pci );
	SL_ADD_LI( l_user_data, pci );

	mde->service_primitive = T_DATA;
	mde->primitive_type = RQS;
    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    }/* end case OSNF */
    case OSSXF:
    {
	/*
	** Send an S_XOFF indication to PL.  
	*/

	mde->service_primitive = S_XOFF;
	mde->primitive_type = IND;

	/*
	** Send the SDU to PL
	*/

	layer_id = UP;
	break;
    } /* end case OSSXF */
    case OSSXN:
    {
	/*
	** Send an S_XON indication to PL.  
	*/

	mde->service_primitive = S_XON;
	mde->primitive_type = IND;

	/*
	** Send the SDU to PL
	*/

	layer_id = UP;
	break;
    } /* end case OSSXN */
    case OSTXF:
    {
	/*
	** Send a T_XOFF request to TL.  
	*/

	mde->service_primitive = T_XOFF;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to TL
	*/

	layer_id = DOWN;
	break;
    } /* end case OSTXF */
    case OSTXN:
    {
	/*
	** Send a T_XON request to TL.  
	*/

	mde->service_primitive = T_XON;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to TL
	*/

	layer_id = DOWN;
	break;
    } /* end case OSTXN */
    case OSMDE:	/* Delete the MDE */
    {
	gcc_del_mde( mde );
	layer_id = US;
	break;
    } /* end case OSMDE */
    case OSCOL:	/* Set Release Collision */
    {
	ccb->ccb_sce.s_flags |= GCCSL_COLL;
	layer_id = US;
	break;
    } /* end case OSCOL */
    case OSDNR:	/* Set DN SPDU received in release collision */
    {
	ccb->ccb_sce.s_flags |= GCCSL_DNR;
	layer_id = US;
	break;
    } /* end case OSDNR */
    case OSPGQ: /* Purge Connect Request MDE */
    {
	/* Free leftover MDE saved by OSTCQ */

        gcc_del_mde( ccb->ccb_sce.s_con_mde );
	layer_id = US;
        break;
    } /* end case OSPGQ */
    case OSTFX:
    {
	GCC_MDE		*fx_mde;

	/*
	** Generate and send a force T_XON request to TL. Forcing
    	** XON at the TL is ok in the case where the TL is already
    	** XON'd because all the TL will do in that case is set a
    	** flag (ie it won't try to post another receive).
	*/

	fx_mde = gcc_add_mde( ccb, 0 );
  	if (fx_mde == NULL)
	{
	    /* Log an error and abort the connection */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_sl_abort(fx_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	} /* end if */

    	fx_mde->ccb = ccb;
	fx_mde->service_primitive = T_XON;
	fx_mde->primitive_type = RQS;
    	mde = fx_mde;

	/*
	** Send the SDU to TL
	*/

	layer_id = DOWN;
	break;
    } /* end case OSTFX */
    default:
	{
	    /*
	    ** Unknown event ID.  This is a program bug. Log an error
	    ** and abort the connection.
	    */
	    status = E_GC2605_SL_INTERNAL_ERROR;
	    gcc_sl_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	}
    } /* end switch */

    /*
    ** Determine whether this output event goes to layer N+1 or	N-1 and send it.
    */

    switch (layer_id)
    {
    case UP:	/* Send to layer N+1 */
    {
    	call_status = gcc_pl(mde);
	break;
    }
    case DOWN:	/* Send to layer N-1 */
    {
    	call_status = gcc_tl(mde);
	break;
    }
    default:
        {
            break;
        }
    } /* end switch */
    return(call_status);
} /* end gcc_slout_exec */

/*{
** Name: gcc_sl_abort	- Session Layer internal abort
**
** Description:
**	gcc_sl_abort handles the processing of internally generated aborts.
** 
** Inputs:
**      mde		    Pointer to mde
**      ccb		    Pointer to ccb
**	generic_status      Mainline status code
**      system_status       System specfic status code
**      erlist		    Error parameter list
**
** Outputs:
**      none
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-Mar-90 (cmorris)
**          Initial routine creation
**      16-Jul-91 (cmorris)
**	    Fixed system status argument to gcc_sl call
**  	15-Jan-93 (cmorris)
**  	    Always reset p1, p2 before processing internal abort.
**  	    The pointers could be set to anything when the abort
**  	    occurred.
**	17-Dec-09 (Bruce Lunsford)  Sir 122536
**	    Remove unref'd variable temp_system_status.
*/
static VOID
gcc_sl_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *generic_status, 
	      CL_ERR_DESC *system_status, GCC_ERLIST *erlist )
{
    STATUS 	temp_generic_status;

    if (!(ccb->ccb_sce.s_flags & GCCSL_ABT))
    {
	/* Log an error */
	gcc_er_log(generic_status, system_status, mde, erlist);

	/* Set abort flag so we don't get in an endless aborting loop */
        ccb->ccb_sce.s_flags |= GCCSL_ABT;    

        /* Issue the abort */
    	if (mde != NULL)
	{
    	    mde->service_primitive = S_P_ABORT;
	    mde->primitive_type = RQS;
    	    mde->p1 = mde->p2 = mde->papp;
	    mde->mde_generic_status[0] = *generic_status;
            temp_generic_status = OK;
            (VOID) gcc_sl(mde);
	}
    }

}  /* end gcc_sl_abort */
