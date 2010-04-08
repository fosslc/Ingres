/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cs.h>
#include    <mu.h>
#include    <cv.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <me.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcu.h>
#include    <gcc.h>
#include    <gcaint.h>
#include    <gcaimsg.h>
#include    <gcatrace.h>
#include    <gcocomp.h>
#include    <gccpci.h>
#include    <gccer.h>
#include    <gccgref.h>
#include    <gccpl.h>
#include    <gcxdebug.h>

/**
**
**  Name: GCCPL.C - GCF communication server Presentation Layer module
**
**  Description:
**      GCCPL.C contains all routines which constitute the Presentation Layer 
**      of the GCF communication server. 
**
**          gcc_pl - Main entry point for the Presentation Layer.
**	    gcc_pl_event - FSM processor
**          gcc_plinit - PL initialization routine
**          gcc_plterm - PL termination routine
**          gcc_pl_evinp - FSM input event evaluator
**          gcc_plout_exec - FSM output event execution processor
**	    gcc_pl_abort - PL internal abort routine
**	    gcc_pdc_down - PL outbound data conversion
**	    gcc_pdc_up - PL inbound data conversion
**          gcc_dup_mde - Duplicate MDE for data conversion
**
**
**  History:
**      22-Mar-88 (jbowers)
**          Initial module creation
**      13-Sep-88 (berrow)
**          Added code for heterogeneous support
**      16-Jan-89 (jbowers)
**          Documentation change to FSM to document fix for an invalid
**	    intersection. 
**      16-Feb-89 (jbowers)
**          Add local flow control: expand FSM to handle XON and XOFF
**	    primitives. 
**      01-Mar-89 (berrow)
**          First live Heterogenous link debugged version code added.
**	    Full code commenting, cleanup and optimization still to come ...
**      03-Mar-89 (ham)
**          Added call to gcc_init_conv.
**      16-Mar-89 (jbowers)
**	    Fix for bug # 5075:
**          Fix for failure to clean up sessions when ARU PPDU is received on a
**	    session currently in the process of closing.  Actual fix is in the
**	    FSM in GCCPL.H.  FSM documentation is updated here.
**      16-Apr-89 (jbowers)
**	    Update documentation of PL FSM to add events for heterogeneous
**	    connections.
**      17-Apr-89 (jbowers)
**	    Fix for PL failure to release an MDE obtained when processing in
**	    incoming internal tuple descriptor (input event IPSDIO).  Since the
**	    MDE created by perform_conversion() remains in PL, it must be
**	    released there as part of OPLODI.
**      20-Apr-89 (jbowers)
**	    Fix for bug in out_exec_chain causing enqueued MDE's to be lost when
**	    doing heterogeneous processing of a chain of MDE's has a counter
**	    flowing event such as XOFF.  The queue of MDE's is only checked
**	    when the output event is a normal or expedited data send to SL or
**	    AL.
**      27-Apr-89 (jbowers)
**	    Fix for bug in perform_conversion associated with a conjunction of a
**	    set of boundary conditions which cause a pad not to be expanded, and
**	    a subsequent failure due to leftover context.  The fix forces an
**	    additional pass through a 'while' loop.
**      11-may-89 (pasker) v1.001
**	    Fix bug in fix of 27-apr-89 by (jbowers).  The variable 
**	    one_mo_tahm must be initialized to FALSE so that zero length
**	    messages will work properly.
**	12-may-89 (pasker) v1.002
**	    Make each GC2402 error distinct to assist in further problem
**	    determination.  While all these errors are "SHOULD NEVER HAPPEN",
**	    they do, so we must be able to tell what failed.
**	15-May-89 (seiwald)
**	    Casted some pointers and initialized some variables to please
**	    lint.  Introduced provisional debugging code, triggered by the
**	    # define GCXDEBUG.  Debugging code requires the new gcxdebug.c
**	    file to be linked in.
**      30-May-89 (cmorris)
**          Added check to gcc_pl_pod to make sure constructed message
**          doesn't run past end of MDE.
**	13-June-89 (jorge)
**	    added descrip.h include in front of gcccl.h include
**	16-jun-89 (seiwald)
**	    Added a little more tracing for the presentation layer.
**	19-jun-89 (seiwald)
**	    changed MEalloc to gcc_alloc.
**	19-jun-89 (pasker)
**	    ... and some GCATRACing.
**	22-jun-89 (seiwald)
**	    Straightened out gcc[apst]l.c tracing.
**      21-aug-89 (seiwald)
**          Replaced VARPADAR and VAR1PADAR, special lengths for PAD's,
**          with VARVCHAR and VAR1VCHAR, special lengths for the paddable
**          data.  VARVCHAR and VAR1VCHAR are like VAREXPAR, in that the
**          array length is given by the last_integer, but they subtract that
**          amount from the parent_length, so PAD's are just VARTPLAR (the
**          remaining parent_length).  VAR1VCHAR, the special length for
**          nullable, paddable data, also subtracts the extra byte for the
**          null flag, and does a sanity check on the last_integer, since
**          if the data is nulled, the length given by last_integer may be
**          unset.  All because ADF is insane.
**       07-Sep-89 (cmorris)
**          Spruced up tracing.
**	 13-Sep-89 (ham)
**	    Byte Alignment fixes.  We no longer use a structure pointer to
**	    address the pci in the mde.  We use macros the build the pci
**	    in the mde byte by byte and strip the pci out byte by byte.
**	05-Oct-89 (seiwald)
**	    Moved misplaced defines into gca.h.
**	06-Oct-89 (seiwald)
**	    Perform sanity checking on all the lengths of all PADable.
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**	02-Nov-1989 (cmorris)
**	    Reapplied bug fix that somehow got lost (see 06-Sep-89 (cmorris))
**        07-Nov-89 (cmorris)
**          Corrected obviously wrong and/or misleading comments.
**	07-Dec-89 (seiwald)
**	     GCC_MDE reorganised: mde_hdr no longer a substructure;
**	     some unused pieces removed; pci_offset, l_user_data,
**	     l_ad_buffer, and other variables replaced with p1, p2, 
**	     papp, and pend: pointers to the beginning and end of PDU 
**	     data, beginning of application data in the PDU, and end
**	     of PDU buffer.
**	09-Dec-89 (seiwald)
**	     Allow for variously sized MDE's: specify size when allocating
**	     one.  Keep track of original data offset into pre-conversion 
**	     MDE so we can have the same amount of space in the post-conversion
**	     one.  Zero out the obj_ptr for atomic types.
**	02-Jan-90 (seiwald)
**	    Made common two separate definitions of a PL_TD_PCI: they
**	    need to reference the same structure.
**      09-Jan-90 (cmorris)
**          Tidied up GCA real-time tracing.
**	12-Jan-90 (seiwald)
**	     Ironed out ownership of MDE's: an MDE now belongs to exactly 
**	     one layer at a time.  See change comments in gccal.c.
**	     Reworked gcc_pl_pdc to free the incoming MDE if it generates a
**	     new output one.
**	25-Jan-90 (seiwald)
**	     Support for as yet unimplemented message grouping.  
**	     See comments in history in gccpl.h.
**	     Perform conversion split apart into three sections:
**	     gcc_pdc_up (perform conversion on inbound messages),
**	     gcc_pdc_down (perform conversion on outbound messages),
**	     and perform_conversion (copy input to output, doing
**	     conversion).
**	25-Jan-90 (cmorris)
**           Tracing changes.
**	12-Feb-90 (seiwald)
**	    Moved het support into gccpdc.c.
**	16-Feb-90 (seiwald)
**	    Reworked architecture vector support.
**      06-Mar-90 (cmorris)
**	    Reworked internal aborts.
**	27-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**      02-May-91 (cmorris)
**	    Allow user abort after connection release has been requested.
**	15-May-91 (seiwald)
**	    Changed mout->p1 and mout->p2 to have the same offset as
**	    the orginal MDE, to accomodate the jumbo 1K MDEs coming
**	    in from 6.3 Comm Servers.  Without this, MDE's over 1012
**	    bytes would get truncated in perform_conversion() (bug
**	    #37568).
**	17-May-91 (seiwald)
**	    Fixed previous change by reversing it and fixing the real
**	    problems:  a) gcc_pdc_up was fragmenting messages but not
**	    not clearing the EOD (end-of-data) on the initial fragments;
**	    b) gcc_pdc_down was taking the two bytes for the TD_PCI out of 
**	    the data area, rather than tacking it on before the data area.
**	    These have been corrected.  The previous change was trying to
**	    circumvent problem (a) but tickled problem (b) which caused
**	    the gcc_pdc_down to generate infinite zero sized fragments!
**	    Haste makes waste!
**	20-May-91 (cmorris)
**	    Added support for release collisions.
**	20-May-91 (seiwald)
**	     Support for character set negotiation.
**	     Initialize new IIGCc_global->gcc_cset_nam_q.
**	     Initialize new IIGCc_global->gcc_cset_xlt_q.
**	     Make calls into gcc_xxx_av() during connection
**	     establishment.
**  	12-August-91 (cmorris)
**  	    Added support to delete any tuple object descriptor when
**  	    connection is released or aborted.
**      13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**	14-Aug-91 (seiwald)
**	    GCaddn2() takes a i4, so use 0 instead of NULL.  NULL is
**	    declared as a pointer on OS/2.
**	14-Aug-91 (seiwald) bug #39172.
**	    Gcc_pl_lod can now return a failure status.
**	14-Aug-91 (seiwald)
**	    Linted.
**  	15-Aug-91 (cmorris)
**  	    After we have issued/received an abort of the session
**  	    connection, no further interaction with the SL occurs.
**  	15-Aug-91 (cmorris)
**  	    Ditto for interactions with AL.
**  	16-Aug-91 (cmorris)
**  	    Fix aborts in closed state.
**  	08-May-92 (cmorris)
**  	    Comment cleanup .
**  	14-May-92 (cmorris)
**  	    Fix some fairly subtle memory leaks in gcc_pdc_(down up):
**  	    If we abort the connection after allocating an output MDE
**  	    for data conversion, we need to free up said MDE.
**	6-aug-92 (seiwald)
**	    Moved gcc_pl_pod(), gcc_pl_lod() in from gccpdc.c.
**	    Support for compiled message descriptors: gcc_pl_lod() now
**	    compiles the the tuple descriptor after it is assembled
**	    and manages the tod_prog (tuple object descriptor program)
**	    hanging off of the GCC_CCB.
**	26-sep-92 (seiwald)
**	    Moved fragment handling from perform_conversion to gcc_pdc_down.
**  	11-Dec-92 (cmorris)
**  	    Added concatenation support for homogeneous data.
**  	16-Dec-92 (cmorris)
**  	    Got rid of gcc_dup_mde:- it had become completely trivial.
**	5-Feb-93 (seiwald)
**	    Gcc_pdc_down() and gcc_pdc_up() now allocate and free the GCC_DOC
**	    for perform_conversion(), and they now call perform_conversion()
**	    with just the buffers and other relevant data - the MDE's themselves
**	    are not passed.
**	10-Feb-93 (seiwald)
**	    Removed generic_status and system_status from gcc_xl(),
**	    gcc_xlfsm(), and gcc_xlxxx_exec() calls.
**	11-Feb-93 (seiwald)
**	    Merge the gcc_xlfsm()'s into the gcc_xl()'s.
**	11-Feb-93 (seiwald)
**	    Use QU routines rather than the roll-your-own gca_qxxx ones.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      13-sep-93 (johnst)
**          Integrated from 6.4 codeline the Asiatech (kchin) change for
**          axp_osf 64-bit platform. This is the (seiwald) fix to gca_pl_lod()
**          to properly reconstruct the received object descriptor from the
**	    GCA message, for platforms that use 64-bit pointers. This matches
**	    the coresponding message formating change in gcf/gca/gcasend.c.
**	    Use new PTR_BITS_64 symbol to conditionally compile this code.
**	20-sep-93 (swm)
**	    removed dbdbms.h for ed.
**	04-jan-94 (seiwald) bug #55992
**	    When converting incoming network data from a pre-6.4 jumbo 1k mde,
**	    make sure the destination mde is sized accordingly.  This prevents
**	    incoming messages from being fragmented and tickling the noxious
**	    problem of fragmented varchar pads.
**	22-jan-94 (seiwald) bug #58809, #57550, #57545
**	    Fix fragment handling in gcc_pdc_down(): we mistakenly treated
**	    any input unused by perform_conversion() as a fragment if it was
**	    small enough to be a fragment (less than 8 bytes): we saved it
**	    so that it could be prepended to the next input message, which
**	    presumably would contain the rest of the fragment.  But perform
**	    conversion() can return with a small amount of input unused when
**	    there is no more input data coming: when it returns because the 
**	    output buffer is full.  This change makes sure that the input
**	    fragment will be followed by another input message.
**	14-mar-94 (seiwald)
**	    Renamed gccod.h to gcdcomp.h.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	20-Dec-95 (gordy)
**	    Add incremental initialization support to gcd_init().
**	 8-Mar-96 (gordy)
**	    Simplified the gcd_convert() interface.
**	24-par-96 (chech02, lawst01)
**	    cast %d arguments to i4  in TRdisplay for windows 3.1 port.
**	13-May-96 (gordy)
**	    Fix in gcc_pdc_up() related to BUG #75718.
**	19-Jun-96 (gordy)
**	    Recursive requests are now queued and processed by the first
**	    instance of gcc_pl().  This ensures that all outputs of an 
**	    input event are executed prior to processing subsequent requests.
**	16-Sep-96 (gordy)
**	    Limit PL protocol level when message concatenation disabled.
**	 1-Oct-96 (gordy)
**	    Previous change made it necessary to explicitly negotiate the
**	    protocol level used on an association rather than implicitly
**	    as had previously been done.
**	11-feb-97 (loera01) Bug 88874
**	    In gcc_pl_lod(), make sure we have a full TOD header before
**	    determining the element count.
**	11-Mar-97 (gordy)
**	    Added gcu.h for utility function declarations.
**	27-May-97 (thoda04)
**	    Included function prototype for gcu_get_tracesym().
**      22-Oct-97 (rajus01)
**          Replaced gcc_get_obj(), gcc_del_obj() with
**          gcc_add_mde(), gcc_del_mde() calls respectively.
**	12-Jan-98 (gordy)
**	    Flush normal channel buffered concatenation data when prior to
**	    sending expedited channel messages.
**	28-jul-98 (loera01 & gordy) Bugs 88214 and 91775:
**	    For gcc_pdc_up() and gcc_pdc_down(): before performing the 
**	    conversion, change the state to DOC_DONE if the GCC is in the 
**	    midst of processing a message, and the existing message type is 
**	    superseded by a new incoming message type. Otherwise, GCA purge 
**	    operations won't work.
**     23-dec-98 (loera01) Bug 94673
**	    For gcc_pdc_up() and gcc_pdc_down(): the above change covers 
**	    most cases, but sometimes there is a fragment left over--the
**	    field doc->frag_bytes is non-zero.  The field is flushed to
**	    prevent the GCC from attempting to compile a message as if it
**	    had fragments, because the new message has none. 
**	 4-Feb-99 (gordy)
**	    Set/clear PS_EOG for lower protocols based on message type.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	12-Jul-00 (gordy)
**	    The CSET protocol level defined EOG, but never actually used
**	    it.  Need to set EOG if protocol is CSET or below.
**      25-Jun-2001 (wansh01) 
**          Replace gcxlevel with IIGCc_global->gcc_trace_level
**	21-Dec-01 (gordy)
**	    Replaced PTR in GCA_COL_ATT and GCA_ELEMENT with i4.
**	22-Mar-02 (gordy)
**	    Removed unused GCA_UO_DESCR.  Cleanup processing of tuple
**	    and object descriptor messages.
**	20-Aug-02 (gordy)
**	    Check the status of het/net negotiation.  Reject connection
**	    if failure.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	12-May-03 (gordy)
**	    Rewrote gcc_pdc_down() and gcc_pdc_up() to control processing
**	    based on output state of gco_convert().  Bulk of gcc_pdc_up()
**	    was extracted to gcc_pdc_lcl().  Make sure HET/NET resources
**	    are freed from CCB and global.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Use gcx_getGCAMsgMap() to reference the gcx_gca_msg global ref,
**          otherwise the reference cannot be resolved in Windows.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	22-Jan-10 (gordy)
**	    Generalized double-byte processing code for mulit-byte charsets.
*/

/*
**  Forward and/or External function references.
*/

static STATUS	gcc_pl_event( GCC_MDE *mde );
static u_i4	gcc_pl_evinp( GCC_MDE *mde, STATUS *generic_status );
static STATUS	gcc_plout_exec( i4  output_event, GCC_MDE *mde, GCC_CCB *ccb );
static VOID	gcc_pl_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *status, 
			      CL_ERR_DESC *system_status, GCC_ERLIST *erlist );
static STATUS	gcc_pdc_down( GCC_MDE *mde, u_i1  nrmexp );
static STATUS	gcc_pdc_up( GCC_MDE *mde, i4  nrmexp );
static STATUS	gcc_pdc_lcl( GCC_MDE *mde, i4  nrmexp );
static STATUS	gcc_pl_pod( GCC_MDE *mde );
static STATUS	gcc_pl_lod( GCC_MDE *mde );
static STATUS	gcc_pl_flush( GCC_CCB *ccb );


/*{
** Name: gcc_pl	- Entry point for the Presentation Layer
**
** Description:
**	gcc_pl is the entry point for the presentation layer.
**	It is invoked with a Message Data Element (MDE) as its input
**	parameter.  The MDE may come from AL and is outbound for the network, or
**	from SL and is inbound for a local client or server, sent from an
**	external node.  For a detailed discussion of the
**	operation of an individual layer, see the header of GCCAL.C.
**
**	States, input events and output events are symbolized
**	respectively as SPxxx, IPxxx and OPxxx.
**
**	Output events and actions are marked with a '*' if they pass 
**	the incoming MDE to another layer, or if they delete the MDE.
**	There should be exactly one such output event or action per
**	state cell.
**
**	In the FSM definition matrices which follows, the parenthesized 
**  	integers are the designators for the valid entries in the matrix, 
**  	and are used in	the representation of the FSM in GCCPLFSM.ROC as the
**      entries in PLFSM_TBL which are the indices into the entry array 
**  	PLFSM_ENTRY.  See GCCPLFSM.ROC for further details.

** 
**      For the Presentation Layer, the following states, and events
**      are defined, with the state characterizing an individual 
**      connection:
**
**      States:
**
**          SPCLD   - Idle, connection does not exist.
**          SPIPL   - IPL: S_CONNECT confirm pending
**          SPRPL   - RPL: P_CONNECT response pending
**          SPOPN   - Connected
**          SPCLG   - Closing
**
**      Input Events:
**
**          IPPCQ   -  P_CONNECT request
**          IPPCA   -  P_CONNECT response/accept
**          IPPCJ   -  P_CONNECT response/reject
**          IPPAU   -  P_U_ABORT request
**          IPPDQ   -  P_DATA request 
**          IPPEQ   -  P_EXPEDITED_DATA request
**          IPPRQ   -  P_RELEASE request
**          IPPRS   -  P_RELEASE response
**          IPSCI   -  S_CONNECT Indication
**          IPSCA   -  S_CONNECT Confirm/Accept
**          IPSCJ   -  S_CONNECT Confirm/Reject
**          IPSAP   -  S_P_ABORT
**          IPARP   -  S_U_ABORT (ARP : Abort - Provider)
**          IPARU   -  S_U_ABORT (ARU : Abort - User)
**          IPSDI   -  S_DATA Indication
**          IPSEI   -  S_EXP_DATA Indication
**          IPSRI   -  S_RELEASE indication
**          IPSRA   -  S_RELEASE confirm accept
**          IPSRJ   -  S_RELEASE confirm reject
**          IPPDQH  -  P_DATA request, heterogeneous
**          IPPEQH  -  P_EXPEDITED_DATA request, heterogeneous
**          IPSDIH  -  S_DATA Indication, heterogeneous
**          IPSEIH  -  S_EXP_DATA Indication, heterogeneous
**          IPABT   -  P_P_ABORT request: PL internal error
**          IPPXF   -  P_XOFF request
**          IPPXN   -  P_XON request
**          IPSXF   -  S_XOFF indication
**          IPSXN   -  S_XON indication
**          IPPRSC  -  P_RELEASE response, release collision
**          IPSRAC  -  S_RELEASE confirm accept, release collision
**
**      Output Events:
**
**          OPPCI*  -  P_CONNECT indication 
**          OPPCA*  -  P_CONNECT confirm/accept 
**          OPPCJ*  -  P_CONNECT confirm/reject 
**          OPPAU*  -  P_U_ABORT indication
**          OPPAP*  -  P_P_ABORT indication
**          OPPDI*  -  P_DATA indication
**          OPPEI*  -  P_EXP_DATA indication
**          OPPDIH  -  P_DATA indication, heterogeneous
**          OPPEIH  -  P_EXP_DATA indication, heterogeneous
**          OPPRI*  -  P_RELEASE indication
**          OPPRC*  -  P_RELEASE confirm
**          OPSCQ*  -  S_CONNECT request
**          OPSCA*  -  S_CONNECT response/accept 
**          OPSCJ*  -  S_CONNECT response/reject 
**          OPARU*  -  S_U_ABORT request w/ ARU PPDU
**          OPARP   -  S_U_ABORT request w/ ARP PPDU
**          OPSDQ*  -  S_DATA request
**          OPSEQ*  -  S_EXP_DATA request
**          OPSDQH  -  S_DATA request, heterogeneous
**          OPSEQH  -  S_EXP_DATA request, heterogeneous
**          OPSRQ*  -  S_RELEASE request
**          OPSRS*  -  S_RELEASE response
**          OPPXF*  -  P_XOFF indication
**          OPPXN*  -  P_XON indication
**          OPSXF*  -  S_XOFF request
**          OPSXN*  -  S_XON request
**	    OPSCR   -  Set Release Collision flag
**	    OPRCR   -  Reset Release Collision flag
**	    OPMDE*  -  Discard an MDE
**  	    OPTOD   -  Delete any tuple object descriptor
**
**      Following is the Finite State Machine (FSM) representation of the 
**      Presentation Layer:

**      State || SPCLD| SPIPL| SPRPL| SPOPN| SPCLG|
**            ||  00  |  01  |  02  |  03  |  04  |
**      ===========================================
**      Event ||
**      ===========================================
**      IPPCQ || SPIPL|      |      |      |      |
**       (00) || OPSCQ*      |      |      |      |
**            ||  (1) |      |      |      |      |
**      ------++------+------+------+------+------+
**      IPPCA ||      |      | SPOPN|      |      |
**       (01) ||      |      | OPSCA*      |      |
**            ||      |      |  (2) |      |      |
**      ------++------+------+------+------+------+
**      IPPCJ ||      |      | SPCLD|      |      |
**       (02) ||      |      | OPSCJ*      |      |
**            ||      |      |  (3) |      |      |
**      ------++------+------+------+------+------+
**      IPPAU ||      | SPCLD| SPCLD| SPCLD| SPCLD|
**       (03) ||      | OPARU* OPARU* OPTOD| OPTOD|
**  	      ||      |      |      | OPARU* OPARU*
**            ||      |  (4) |  (5) |  (6) |  (6) |
**      ------++------+------+------+------+------+
**      IPPDQ ||      |      |      | SPOPN|      |
**       (04) ||      |      |      | OPSDQ*      |
**            ||      |      |      |  (7) |      |
**      ------++------+------+------+------+------+
**      IPPEQ ||      |      |      | SPOPN|      |
**       (05) ||      |      |      | OPSEQ*      |
**            ||      |      |      |  (8) |      |
**      ------++------+------+------+------+------+
**      IPPRQ ||      |      |      | SPCLG| SPCLG|
**       (06) ||      |      |      | OPSRQ* OPSCR|
**	      ||      |      |      |      | OPSRQ*
**            ||      |      |      |  (11)|  (30)|
**      ------++------+------+------+------+------+
**      IPPRS ||      |      |      |      | SPCLD|
**       (07) ||      |      |      |      | OPTOD|
**            ||      |      |      |      | OPSRS*
**            ||      |      |      |      |  (12)|
**      ------++------+------+------+------+------+
**      IPSCI || SPRPL|      |      |      |      |
**       (08) || OPPCI*      |      |      |      |
**            ||  (13)|      |      |      |      |
**      ------++------+------+------+------+------+
**      IPSCA ||      | SPOPN|      |      |      |
**       (09) ||      | OPPCA*      |      |      |
**            ||      |  (14)|      |      |      |
**      ------++------+------+------+------+------+
**      IPSCJ ||      | SPCLD|      |      |      |
**       (10) ||      | OPPCJ*      |      |      |
**            ||      |  (15)|      |      |      |
**      ------++------+------+------+------+------+
**      IPSAP ||      | SPCLD| SPCLD| SPCLD| SPCLD|
**       (11) ||      | OPPAP* OPPAP* OPTOD| OPTOD|
**  	      ||      |      |      | OPPAP* OPPAP*
**            ||      |  (16)|  (16)|  (9) |  (9) |
**      ------++------+------+------+------+------+

        State || SPCLD| SPIPL| SPRPL| SPOPN| SPCLG|
**            ||  00  |  01  |  02  |  03  |  04  |
**      ===========================================
**      Event ||
**      ===========================================
**      IPARU ||      | SPCLD| SPCLD| SPCLD| SPCLD|
**       (12) ||      | OPPAU* OPPAU* OPTOD| OPTOD|
**            ||      |      |      | OPPAU* OPPAU*
**            ||      |  (19)|  (19)|  (10)|  (10)|
**      ------++------+------+------+------+------+
**      IPARP ||      | SPCLD| SPCLD| SPCLD| SPCLD|
**       (13) ||      | OPPAP* OPPAP* OPTOD| OPTOD|
**            ||      |      |      | OPPAP* OPPAP*
**            ||      |  (16)|  (16)|  (9) |  (9) |
**      ------++------+------+------+------+------+
**      IPSDI ||      |      |      | SPOPN|      |
**       (14) ||      |      |      | OPPDI*      |
**            ||      |      |      |  (22)|      |
**      ------++------+------+------+------+------+
**      IPSEI ||      |      |      | SPOPN|      |
**       (15) ||      |      |      | OPPEI*      |
**            ||      |      |      |  (23)|      |
**      ------++------+------+------+------+------+
**      IPSRI ||      |      |      | SPCLG| SPCLG|
**       (16) ||      |      |      | OPPRI* OPSCR|
**	      ||      |      |      |      | OPPRI*
**            ||      |      |      |  (26)|  (31)|
**      ------++------+------+------+------+------+
**      IPSRA ||      |      |      |      | SPCLD|
**       (17) ||      |      |      |      | OPTOD|
**            ||      |      |      |      | OPPRC*
**            ||      |      |      |      |  (27)|
**      ------++------+------+------+------+------+
**      IPSRJ ||      |      |      |      | SPCLD|
**       (18) ||      |      |      |      | OPTOD|
**            ||      |      |      |      | OPPRC*
**            ||      |      |      |      |  (28)|
**      ------++------+------+------+------+------+
**      IPPDQH||      |      |      | SPOPN|      |
**       (19) ||      |      |      |OPSDQH|      |
**            ||      |      |      |OPMDE*|      |
**            ||      |      |      |  (29)|      |
**      ------++------+------+------+------+------+
**      IPPRSC||      |      |      |      | SPCLG|
**       (20) ||      |      |      |      | OPRCR|
**            ||      |      |      |      | OPSRS*
**            ||      |      |      |      |  (34)|
**      ------++------+------+------+------+------+
**      IPSRAC||      |      |      |      | SPCLG|
**       (21) ||      |      |      |      | OPRCR|
**            ||      |      |      |      | OPPRC*
**            ||      |      |      |      |  (49)|
**      ------++------+------+------+------+------+
**      IPPEQH||      |      |      | SPOPN|      |
**       (22) ||      |      |      |OPSEQH|      |
**            ||      |      |      |OPMDE*|      |
**            ||      |      |      |  (32)|      |
**      ------++------+------+------+------+------+

        State || SPCLD| SPIPL| SPRPL| SPOPN| SPCLG|
**            ||  00  |  01  |  02  |  03  |  04  |
**      ===========================================
**      Event ||
**      ===========================================
**      IPSDIH||      |      |      | SPOPN|      |
**       (23) ||      |      |      |OPPDIH|      |
**            ||      |      |      |OPMDE*|      |
**            ||      |      |      |  (33)|      |
**      ------++------+------+------+------+------+
**      IPSEIH||      |      |      | SPOPN|      |
**       (25) ||      |      |      |OPPEIH|      |
**            ||      |      |      |OPMDE*|      |
**            ||      |      |      |  (35)|      |
**      ------++------+------+------+------+------+
**      IPABT || SPCLD| SPCLD| SPCLD| SPCLD| SPCLD|
**       (26) || OPMDE* OPPAP* OPPAP* OPTOD| OPTOD|
**            ||      | OPARP| OPARP| OPPAP* OPPAP*
**            ||      |      |      | OPARP| OPARP|
**            ||  (25)|  (36)|  (36)|  (24)|  (24)|
**      ------++------+------+------+------+------+
**      IPPXF ||      |      |      | SPOPN| SPCLG|
**       (27) ||      |      |      | OPSXF* OPSXF*
**            ||      |      |      |  (37)|  (38)|
**      ------++------+------+------+------+------+
**      IPPXN ||      |      |      | SPOPN| SPCLG|
**       (28) ||      |      |      | OPSXN* OPSXN*
**            ||      |      |      |  (39)|  (40)|
**      ------++------+------+------+------+------+
**      IPSXF ||      |      |      | SPOPN| SPCLG|
**       (29) ||      |      |      | OPPXF* OPPXF*
**            ||      |      |      |  (41)|  (42)|
**      ------++------+------+------+------+------+
**      IPSXN ||      |      |      | SPOPN| SPCLG|
**       (30) ||      |      |      | OPPXN* OPPXN*
**            ||      |      |      |  (43)|  (44)|
**      ------++------+------+------+------+------+

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
**	    Renamed gcc_pl() to gcc_pl_event() and created gcc_pl()
**	    to disallow nested processing of events.  The first
**	    call to this routine sets a flag which causes recursive
**	    calls to queue the MDE for processing sequentially.
*/

STATUS
gcc_pl( GCC_MDE *mde )
{
    GCC_CCB	*ccb = mde->ccb;
    STATUS	status = OK;
    
    /*
    ** If the event queue active flag is set, this
    ** is a recursive call and PL events are being
    ** processed by a previous instance of this 
    ** routine.  Queue the current event, it will
    ** be processed once we return to the active
    ** processing level.
    */
    if ( ccb->ccb_pce.pl_evq.active )
    {
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 3 )
	{
            TRdisplay( "gcc_pl: queueing event %s %s\n",
                   gcx_look( gcx_mde_service, mde->service_primitive ),
                   gcx_look( gcx_mde_primitive, mde->primitive_type ));
	}
# endif /* GCXDEBUG */

	QUinsert( (QUEUE *)mde, ccb->ccb_pce.pl_evq.evq.q_prev );
	return( OK );
    }

    /*
    ** Flag active processing so any subsequent 
    ** recursive calls will be queued.  We will 
    ** process all queued events once the stack
    ** unwinds to us.
    */
    ccb->ccb_pce.pl_evq.active = TRUE;

    /*
    ** Process the current input event.  The queue
    ** should be empty since no other instance of
    ** this routine is active, and the initial call
    ** always clears the queue before exiting.
    */
    status = gcc_pl_event( mde );

    /*
    ** Process any queued input events.  These
    ** events are the result of recursive calls
    ** to this routine while we are processing
    ** events.
    */
    while( ccb->ccb_pce.pl_evq.evq.q_next != &ccb->ccb_pce.pl_evq.evq )
    {
	mde = (GCC_MDE *)QUremove( ccb->ccb_pce.pl_evq.evq.q_next );

# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 3 )
	{
            TRdisplay( "gcc_pl: processing queued event %s %s\n",
                   gcx_look( gcx_mde_service, mde->service_primitive ),
                   gcx_look( gcx_mde_primitive, mde->primitive_type ));
	}
# endif /* GCXDEBUG */

	status = gcc_pl_event( mde );
    }

    ccb->ccb_pce.pl_evq.active = FALSE;		/* Processing done */

    return( status );
} /* end gcc_pl */


/*
** Name: gcc_pl_event
**
** Description:
**	Presentation layer event processing.  An input MDE is evaluated
**	to a PL event.  The input event causes a state transition and
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
**          Make error handling consistent with other layers'
**	19-Jun-96 (gordy)
**	    Renamed from gcc_pl() to handle recursive requests.
**	    This routine will no longer be called recursively.
*/

static STATUS
gcc_pl_event( GCC_MDE *mde )
{
    u_i4	input_event;
    STATUS	status;

    /*
    ** Invoke  the FSM executor, passing it the current state and the 
    ** input event, to execute the output event(s), the action(s)
    ** and return the new state.  The input event is determined by the
    ** output of gcc_pl_evinp().
    */

    status = OK;
    input_event = gcc_pl_evinp(mde, &status);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
        TRdisplay( "gcc_pl: primitive %s %s event %s size %d\n",
                gcx_look( gcx_mde_service, mde->service_primitive ),
                gcx_look( gcx_mde_primitive, mde->primitive_type ),
                gcx_look( gcx_gcc_ipl, input_event ),
		(i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */

    if (status == OK)
    {
	i4		output_event;
	i4		i,j;
	GCC_CCB		*ccb = mde->ccb;
	u_i4		old_state;

	/*
	** Determine the state transition for this state and input event and
	** return the new state.
	*/

	old_state = ccb->ccb_pce.p_state;
	j = plfsm_tbl[input_event][old_state];

	/* Make sure we have a legal transition */
	if (j != 0)
	{
	    ccb->ccb_pce.p_state = plfsm_entry[j].state;

	    /* do tracing... */
	    GCX_TRACER_3_MACRO("gcc_plfsm>", mde, "event=%d, state=%d->%d", 
			    input_event, old_state, (u_i4)ccb->ccb_pce.p_state);

# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >= 2 )
		TRdisplay( "gcc_plfsm: new state=%s->%s\n", 
			   gcx_look(gcx_gcc_spl, old_state), 
			   gcx_look(gcx_gcc_spl, (u_i4)ccb->ccb_pce.p_state) );
# endif /* GCXDEBUG */

	    /*
	    ** For each output event in the list for this state and input event, 
	    ** call the output executor.
	    */

	    for (i = 0;
		 i < PL_OUT_MAX
		 &&
		 (output_event =
		 plfsm_entry[j].output[i])
		 > 0;
		 i++)
	    {
		if ((status = gcc_plout_exec(output_event, mde, ccb)) != OK)
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
	    status = E_GC2404_PL_FSM_STATE;

	    /* Abort the connection */

	    gcc_pl_abort(mde, ccb, &status, (CL_ERR_DESC *)0, &erlist);
	    status = FAIL;
	}
    }
    else
    {
	/* Log the error and abort the connection. */

	gcc_pl_abort(mde, mde->ccb, &status, (CL_ERR_DESC *) NULL,
			(GCC_ERLIST *) NULL);
	status = FAIL;
    } /* end else */
    return(status);
}  /* end gcc_pl_event */

/*{
** Name: gcc_plinit	- Presentation Layer initialization routine
**
** Description:
**      gcc_plinit is invoked by ILC on comm server startup.  It initializes
**      the PL and returns.  The following functions are performed:
**
**      - Set up ADF
**      - Set up conversion tables for heterogeneous support
**      - Load architecture characteristic vector
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
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-Jan-88 (jbowers)
**          Initial routine creation
**	06-Sep-90 (cmorris)
**	    Check return code from gcc_tbl_init
**	24-mar-94 (forky01)
**	    Check return code from gcc_adf_init
**	13-Sep-95 (gordy)
**	    Renamed gcc_adf_init() to gcd_init().
**	20-Dec-95 (gordy)
**	    Add incremental initialization support to gcd_init().
**	16-Sep-96 (gordy)
**	    Limit PL protocol level when message concatenation disabled.
*/
STATUS
gcc_plinit( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    char	*sym = NULL;
    STATUS	status = OK;

    QUinit(&IIGCc_global->gcc_doc_q);

    /* Initialize character translation table queue */

    QUinit( &IIGCc_global->gcc_cset_nam_q );
    QUinit( &IIGCc_global->gcc_cset_xlt_q );
    IIGCc_global->gcc_cset_xlt = 0;

    /*
    ** Set our protocol level.  There is a bug in 6.4 LIBQ which does
    ** not support multi-segment GCA_RESPONSE messages.  By disabling
    ** PL message concatenation, we significantly reduce the probability
    ** that such an event will occur.
    */
    gcu_get_tracesym( "II_GCC_MESSAGE_CONCAT", "!.message_concat", &sym );

    if ( 
	 sym  &&  *sym  && 
	 ( ! STcasecmp( sym, "FALSE" )  ||
	   ! STcasecmp( sym, "OFF" ) )
       )
	IIGCc_global->gcc_pl_protocol_vrsn = 
				min( PL_PROTOCOL_VRSN, PL_PROTOCOL_VRSN_CSET );
    else
	IIGCc_global->gcc_pl_protocol_vrsn = PL_PROTOCOL_VRSN;

    /*
    ** Initialize the heterogenous support systems:
    ** GCA message processing and character set conversion.
    ** The standalone Comm Server does full initialization,
    ** while the embedded Comm Server configuration does
    ** incremental initialization.
    */
    status = gco_init(IIGCc_global->gcc_flags & GCC_STANDALONE ? TRUE : FALSE);
    if ( status == OK )  status = gcc_init_cset();
    
    *generic_status = status;

    return(status);
}  /* end gcc_plinit */

/*{
** Name: gcc_plterm	- Presentation Layer initialization routine
**
** Description:
**      gcc_plterm is invoked by ILC on comm server termination.  It terminates
**      the PL and returns.  There are currently no PL termination functions.
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
**	12-May-03 (gordy)
**	    Free DOC in free queue.
*/
STATUS
gcc_plterm( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    QUEUE	*doc_ptr;
    STATUS	return_code = OK;

    while( (doc_ptr = QUremove( IIGCc_global->gcc_doc_q.q_next )) )
	gcc_free( (PTR)doc_ptr );

    *generic_status = OK;
    return(return_code);
}  /* end gcc_plterm */

/*{
** Name: gcc_pl_evinp	- PL FSM input event id evaluation routine
**
** Description:
**      gcc_pl_evinp accepts as input an MDE received by the PL.  It  
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
**	07-Mar-90 (cmorris)
**	    Added handling of P_P_ABORT
**      19-Jan-93 (cmorris)
**  	    Got rid of system_status argument:- never used.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
*/
static u_i4
gcc_pl_evinp( GCC_MDE *mde, STATUS *generic_status )
{
    u_i4	input_event;

    GCX_TRACER_3_MACRO("gcc_pl_evinp>", mde,
	"primitive = %d/%d, size = %d",
	    mde->service_primitive,
	    mde->primitive_type,
	    (i4)(mde->p2 - mde->p1));

    *generic_status = OK;
    switch (mde->service_primitive)
    {
    case P_CONNECT:
    {
	switch (mde->primitive_type)
	{
	case RQS:
	{
	    input_event = IPPCQ;
	    break;
	}  /* end case RQS */
	case RSP:
	{
	    switch (mde->mde_srv_parms.pl_parms.ps_result)
	    {
	    case PS_ACCEPT:
	    {
		input_event = IPPCA;
		break;
	    }
	    case PS_USER_REJECT:
	    {
		input_event = IPPCJ;
		break;
	    }
	    default:
		{
		    *generic_status = E_GC2406_IE_BAD_PCON_RSP_RSLT;
		    break;
		}
	    } /* end switch */
	    break;
	} /* end case RSP */
	default:
	    {
		*generic_status = E_GC2407_IE_BAD_PCON_TYPE;
		break;
	    }
	} /* end switch */
	break;
    } /* end case P_CONNECT */
    case P_U_ABORT:
    {
	input_event = IPPAU;
	break;
    } /* end case P_U_ABORT */
    case P_P_ABORT:
    {
	input_event = IPABT;
	break;
    } /* end case P_P_ABORT */
    case P_DATA:
    {
	if (mde->ccb->ccb_pce.het_mask == GCC_HOMOGENEOUS)
	{
	    input_event = IPPDQ;
	}
	else
	{
	    input_event = IPPDQH;
	}
	break;
    } /* end case P_DATA */
    case P_EXP_DATA:
    {
	if (mde->ccb->ccb_pce.het_mask == GCC_HOMOGENEOUS)
	{
	    input_event = IPPEQ;
	}
	else
	{
	    input_event = IPPEQH;
	}
	break;
    } /* end case P_EXP_DATA */
    case P_RELEASE:
    {
	switch (mde->primitive_type)
	{
	case RQS:
	{
	    input_event = IPPRQ;
	    break;
	}  /* end case RQS */
	case RSP:
	{
	    /* See if we've had a release collision */
            if (mde->ccb->ccb_pce.p_flags & GCCPL_CR)
		input_event = IPPRSC;
	    else
	        input_event = IPPRS;
	    break;
	} /* end case RSP */
	default:
	    {
		*generic_status = E_GC2408_IE_BAD_PRSL_TYPE;
		break;
	    }
	} /* end switch */
	break;
    } /* end case P_RELEASE */
    case S_CONNECT:
    {
	switch (mde->primitive_type)
	{
	case IND:
	{
	    /* S_CONNECT indication */
	    input_event = IPSCI;
	    break;
	}  /* end case IND */
	case CNF:
	{
	    /* S_CONNECT confirm */
	    switch (mde->mde_srv_parms.sl_parms.ss_result)
	    {
	    case SS_ACCEPT:
	    {
		input_event = IPSCA;
		break;
	    }
	    case SS_RJ_USER:
	    {
		input_event = IPSCJ;
		break;
	    }
	    default:
		{
		    *generic_status = E_GC2409_IE_BAD_SCON_RSP_RSLT;
		    break;
		}
	    } /* end switch */
	    break;
	} /* end case CNF */
	default:
	    {
		*generic_status = E_GC240A_IE_BAD_SCON_TYPE;
		break;
	    }
	} /* end switch */
	break;
    }  /* end case S_CONNECT */
    case S_RELEASE:
    {
	switch (mde->primitive_type)
	{
	case IND:
	{
	    input_event = IPSRI;
	    break;
	}  /* end case IND */
	case CNF:
	{
	    switch (mde->mde_srv_parms.sl_parms.ss_result)
	    {
	    case SS_AFFIRMATIVE:
	    {
		/* See if we've had release collision */
		if (mde->ccb->ccb_pce.p_flags & GCCPL_CR)
		    input_event = IPSRAC;
		else
		    input_event = IPSRA;
		break;
	    }
	    case SS_NEGATIVE:
	    {
		input_event = IPSRJ;
		break;
	    }
	    default:
		{
		    *generic_status = E_GC240B_IE_BAD_SREL_CNF_RSLT;
		    break;
		}
	    } /* end switch */
	    break;
	} /* end case CNF */
	default:
	    {
		*generic_status = E_GC240C_IE_BAD_SREL_TYPE;
		break;
	    }
	} /* end switch */
	break;
    }  /* end case S_RELEASE */
    case S_DATA:
    {
	if (mde->ccb->ccb_pce.het_mask == GCC_HOMOGENEOUS)
	{
	    input_event = IPSDI;
	}
	else
	{
	    input_event = IPSDIH;
	}
	break;
    }  /* end case S_DATA */
    case S_EXP_DATA:
    {
	if (mde->ccb->ccb_pce.het_mask == GCC_HOMOGENEOUS)
	{
	    input_event = IPSEI;
	}
	else
	{
	    input_event = IPSEIH;
	}
	break;
    }  /* end case S_EXP_DATA */
    case S_U_ABORT:
    {
    	char	    	*pci;
	PL_ARP_PCI	arpi_pci;

	arpi_pci.ppdu_id = 0;
	pci = mde->p1;
	if ( pci < mde->p2 )  pci += GCgeti1(pci, &arpi_pci.ppdu_id);

	switch (arpi_pci.ppdu_id)
	{
	case ARP:
	{
	    input_event = IPARP;
	    break;
	}
	case ARU:
	{
	    input_event = IPARU;
	    break;
	}
	default:
	    {
		*generic_status = E_GC240D_IE_BAD_SUAB_PDU;
		break;
	    }
	} /* end switch */
	break;
    }  /* end case S_U_ABORT */
    case S_P_ABORT:
    {
	input_event = IPSAP;
	break;
    }  /* end case S_P_ABORT */
    case P_XOFF:
    {
	/* P_XOFF has only a request form */

	input_event = IPPXF;
	break;
    }  /* end case P_XOFF */
    case P_XON:
    {
	/* P_XON has only a request form */

	input_event = IPPXN;
	break;
    }  /* end case P_XON */
    case S_XOFF:
    {

	input_event = IPSXF;
	break;
	/* S_XOFF has only an indication form */
    }  /* end case S_XOFF */
    case S_XON:
    {
	/* S_XON has only an indication form */

	input_event = IPSXN;
	break;
    }  /* end case S_XON */
    default:
	{
	    *generic_status = E_GC240E_IE_BAD;
	    break;
	}
    }  /* end switch */
    return(input_event);
} /* end gcc_pl_evinp */

/*{
** Name: gcc_plout_exec	- PL FSM output event execution routine
**
** Description:
**      gcc_plout_exec accepts as input an output event identifier.
**	It executes this output event and returns.
**
**
** Inputs:
**	output_event		Identifier of the output event
**      mde			Pointer to Pointer to dequeued 
**				Message Data Element
**
**	Returns:
**	    OK
**          FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      25-Jan-88 (jbowers)
**          Initial function implementation
**      06-Sep-89 (cmorris)
**          Encode the protocol version number correctly
**      06-Sep-89 (cmorris)
**          Don't look in the provider reason field for connect rejects.
**          This field wasn't being set by the responding PL and anyway
**          was supposed to refer to a specific rejection reason, not a yes
**          or no answer.
**  	09-Jan-91 (cmorris)
**  	    Check status of gcc_get_obj.
**	10-May-91 (cmorris)
**	    Check for legal result on P_RELEASE response.
**  	11-Dec-92 (cmorris)
**  	    Events OPPDI and OPSDQ now support concatentation.
**  	04-Jan-93 (cmorris)
**  	    Removed copy of remote network address:- it's now help in
**  	    the CCB.
**	16-Sep-96 (gordy)
**	    PL protocol level now a runtime value.
**	 1-Oct-96 (gordy)
**	    Previous change made it necessary to explicitly negotiate the
**	    protocol level used on an association rather than implicitly
**	    as had previously been done.
**	12-Jan-98 (gordy)
**	    Flush normal channel buffered concatenation data when prior to
**	    sending expedited channel messages.
**	 4-Feb-99 (gordy)
**	    Set/clear PS_EOG for lower protocols based on message type.
**	12-Jul-00 (gordy)
**	    The CSET protocol level defined EOG, but never actually used
**	    it.  Need to set EOG if protocol is CSET or below.
**	20-Aug-02 (gordy)
**	    Check the status of het/net negotiation.  Reject connection
**	    if failure.
**	12-May-03 (gordy)
**	    Free all HET/NET CCB resources in OPTOD.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
*/

static STATUS
gcc_plout_exec( i4  output_event, GCC_MDE *mde, GCC_CCB *ccb )
{
    STATUS	    call_status = OK;
    i4	    layer_id;
    char	    *pci;
    STATUS	    status;

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
	TRdisplay( "gcc_plout_exec: output %s\n",
		gcx_look( gcx_gcc_opl, output_event ) );
# endif /* GCXDEBUG */

    status = OK;

    switch (output_event)
    {
    case OPPCI:
    {
	PL_CP_PCI	cpi_pci;

	/*
	** Send a P_CONNECT indication to AL.
	*/
	if ( (mde->p2 - mde->p1) < SZ_CP_PCI )
	{
	    STATUS status = E_GC2407_IE_BAD_PCON_TYPE;
	    gcc_er_log( &status, NULL, mde, NULL );
	    mde->mde_generic_status[0] = status;
	    mde->mde_srv_parms.pl_parms.ps_result = PS_NEGATIVE;
	    mde->service_primitive = P_CONNECT;
	    mde->primitive_type = IND;
	    layer_id = UP;
	    break;
	}

	/*
	**  Get the pci, piece by piece
	*/
	pci = mde->p1;
	pci += GCgeti2(pci, &cpi_pci.cg_psel);
	pci += GCgeti2(pci, &cpi_pci.cd_psel);
	pci += GCgeti2(pci, &cpi_pci.prot_vrsn);
	pci += GCgets(pci, SZ_CONTEXT, (u_char *)&cpi_pci.dflt_cntxt);
	pci += GCgeti2(pci, &cpi_pci.prsntn_rqmnts);
	pci += GCgeti2(pci, &cpi_pci.session_rqmnts);
	mde->p1 = pci;

	/* 
	** Determine the exchange protocol version.  This
	** used to be done by simply using the peer protocol.
	** If we had a higher protocol level, we would limit
	** ourselves to what the peer could accept and the
	** peer would work at its max level.  If the peer had
	** a higher protocol level, we would work at our max
	** and the peer would limit itself to our level.  This
	** all resulted in an implicit agreement to set the
	** protocol version to min( local_max, remote_max ).
	**
	** Now that config settings can lower our default
	** protocol version to less than our maximum, we
	** must explicitly set the protocol version to
	** min( local_vrsn, remote_vrsn ).
	*/
	ccb->ccb_pce.p_version = 
		min( IIGCc_global->gcc_pl_protocol_vrsn, cpi_pci.prot_vrsn );

	/* Compare the architecture characteristic (a constant octet array
	** set up at PL init. time.) with the one sent by our peer as part of
	** the Default Context area. According to match/mismatch of each
	** datatype designator, set the "het_mask" for this connection.
	*/
	if ( (status = gcc_ind_av( ccb, &cpi_pci.dflt_cntxt )) == OK )
	    mde->mde_srv_parms.pl_parms.ps_result = PS_AFFIRMATIVE;
	else
	{
	    /*
	    ** The two systems are architecturally incompatible.
	    ** Log the error and provide indicator to AL that
	    ** connection should be rejected.
	    */
	    gcc_er_log( &status, NULL, mde, NULL );
	    mde->mde_generic_status[0] = status;
	    mde->mde_srv_parms.pl_parms.ps_result = PS_NEGATIVE;
	}

	mde->service_primitive = P_CONNECT;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OPPCI */
    case OPPCA:
    {
	PL_CPA_PCI	cpai_pci;

	/*
	** Send a P_CONNECT confirm/accept to AL.
	*/
	if ( (mde->p2 - mde->p1) < SZ_CPA_PCI )
	{
	    STATUS status = E_GC2406_IE_BAD_PCON_RSP_RSLT;
	    mde->mde_generic_status[0] = status;
	    gcc_pl_abort( mde, ccb, &status, NULL, NULL );
	    call_status = status;
	    layer_id = US;
	    break;
	}

	/*
	**  Strip out the pci.
	*/
	pci = mde->p1;
	pci += GCgeti2(pci, &cpai_pci.cd_psel);
	pci += GCgets(pci, SZ_CONTEXT, (u_char *)&cpai_pci.dflt_cntxt);
	pci += GCgeti2(pci, &cpai_pci.prot_vrsn);
	pci += GCgeti2(pci, &cpai_pci.prsntn_rqmnts);
	pci += GCgeti2(pci, &cpai_pci.session_rqmnts);	
	mde->p1 = pci;

	/* 
	** Determine the exchange protocol version.  This
	** used to be done by simply using the peer protocol.
	** If we had a higher protocol level, we would limit
	** ourselves to what the peer could accept and the
	** peer would work at its max level.  If the peer had
	** a higher protocol level, we would work at our max
	** and the peer would limit itself to our level.  This
	** all resulted in an implicit agreement to set the
	** protocol version to min( local_max, remote_max ).
	**
	** Now that config settings can lower our default
	** protocol version to less than our maximum, we
	** must explicitly set the protocol version to
	** min( local_vrsn, remote_vrsn ).
	*/
	ccb->ccb_pce.p_version = 
		min( IIGCc_global->gcc_pl_protocol_vrsn, cpai_pci.prot_vrsn );


	if ( (status = gcc_cnf_av( ccb, &cpai_pci.dflt_cntxt )) != OK )
	{
	    mde->mde_generic_status[0] = status;
	    gcc_pl_abort( mde, ccb, &status, NULL, NULL );
	    call_status = status;
	    layer_id = US;
	}
	else
	{
	    mde->service_primitive = P_CONNECT;
	    mde->primitive_type = CNF;
	    mde->mde_srv_parms.pl_parms.ps_result = PS_ACCEPT;
	    layer_id = UP;
	}
	break;
    } /* end case OPPCA */
    case OPPCJ:
    {
	PL_CPR_PCI	cpri_pci;

	/*
	** Send a P_CONNECT confirm/reject to AL.
	*/
	if ( (mde->p2 - mde->p1) < SZ_CPR_PCI )
	{
	    STATUS status = E_GC2406_IE_BAD_PCON_RSP_RSLT;
	    mde->mde_generic_status[0] = status;
	    gcc_pl_abort( mde, ccb, &status, NULL, NULL );
	    call_status = status;
	    layer_id = US;
	    break;
	}

	pci = mde->p1;
	pci += GCgeti2(pci, &cpri_pci.cd_psel);
	pci += GCgets(pci, SZ_CONTEXT, (u_char *)&cpri_pci.dflt_cntxt);
	pci += GCgeti2(pci, &cpri_pci.prot_vrsn);
	pci += GCgeti2(pci, &cpri_pci.provider_reason);
	mde->p1 = pci;
	
	mde->service_primitive = P_CONNECT;
	mde->primitive_type = CNF;
	mde->mde_srv_parms.pl_parms.ps_result = PS_USER_REJECT;
	layer_id = UP;
	break;
    } /* end case OPPCJ */
    case OPPAU:
    {
	/*
	** Send a P_U_ABORT indication to AL. The PCI is currently unused.
	*/

	mde->p1 += SZ_ARU_PCI;
	mde->service_primitive = P_U_ABORT;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OPPAU */
    case OPPAP:
    {
	/*
	** Send a P_P_ABORT indication to AL. The PCI is currently unused.
	*/

	mde->service_primitive = P_P_ABORT;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OPPAP */
    case OPPDI:
    {
    	GCC_MDE	    	*mde_out;
    	u_i2	    	length;

	/*
	** Send a P_DATA indication to AL.
	** 
    	** See what version our peer is running at. If it's pre-concatentation
    	** just handle this like we've always done.
	*/
    	if (ccb->ccb_pce.p_version < PL_PROTOCOL_VRSN_CONCAT)
    	{
    	    /*
	    ** No OSI architected PCI is associated with a TD PPDU.  
	    ** However, GCF messages require a PCI structure in order 
	    ** that they may be identified, so it is extracted here.
	    */
	    if ( (mde->p2 - mde->p1) < SZ_PDV )
	    {
		STATUS status = E_GC240E_IE_BAD;
		gcc_pl_abort( mde, ccb, &status,
			     (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
		return( FAIL );
	    }

	    mde->service_primitive = P_DATA;
	    mde->primitive_type = IND;

	    /*
	    **  strip out the pci.
	    */
	    pci = mde->p1;
	    pci += GCgeti1(pci, &mde->mde_srv_parms.pl_parms.ps_flags);
	    pci += GCgeti1(pci, &mde->mde_srv_parms.pl_parms.ps_msg_type);

	    if ( ccb->ccb_pce.p_version <= PL_PROTOCOL_VRSN_CSET )
	    {
		/*
		** The End-Of-Group flag is not sent at lower protocols.
		** Set or clear EOG based on the message type.  Note
		** that the messages tested are a subset of what GCA
		** actually uses, but they are sufficient for the
		** protocol level involved.
		*/
		if ( 
		     mde->mde_srv_parms.pl_parms.ps_msg_type != GCA_QC_ID  &&
		     mde->mde_srv_parms.pl_parms.ps_msg_type != GCA_TDESCR &&
		     mde->mde_srv_parms.pl_parms.ps_msg_type != GCA_TUPLES &&
		     mde->mde_srv_parms.pl_parms.ps_msg_type != GCA_CDATA
		   )
		    mde->mde_srv_parms.pl_parms.ps_flags |= PS_EOG;
		else
		    mde->mde_srv_parms.pl_parms.ps_flags &= ~PS_EOG;
	    }

	    mde->p1 = pci;
	    layer_id = UP;
	}
	else
	{
    	    /* 
    	    ** Decomposition of potentially concatenated messages is attempted
	    ** here. We copy each message within the concatenated set into a 
    	    ** new MDE which we then pass up to the AL. For the last (or only)
    	    ** message in the MDE, we just pass up the input MDE with the 
    	    ** pointer to the start of the PDU data set accordingly.
	    */
    	    while (1)
	    {
	    	/*
	    	**  strip out the PCI. Note: for concatenation, the PCI also
    	    	**  contains the length of the subsequent message.
	    	*/
		if ( (mde->p2 - mde->p1) < SZ_PDVN )
		{
		    STATUS status = E_GC240E_IE_BAD;
		    gcc_pl_abort( mde, ccb, &status,
				 (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
		    return( FAIL );
		}

	    	pci = mde->p1;
	    	pci += GCgeti1(pci, &mde->mde_srv_parms.pl_parms.ps_flags);
	    	pci += GCgeti1(pci, &mde->mde_srv_parms.pl_parms.ps_msg_type);
    	    	pci += GCgeti2(pci, &length);
		length = min( length, mde->p2 - mde->p1 );
    		mde->p1 = pci;

    	    	if (mde->p1 + length < mde->p2)
		{
    	    	    /* Duplicate MDE and copy message into it */
	    	    if(!(mde_out = gcc_add_mde( ccb, ccb->ccb_tce.tpdu_size )))
	    	    {
	    	    	/* 
    	    	    	** Fatal error. Log an error and abort the
    	    	    	** connection.
    	    	    	*/
	    	    	status = E_GC2004_ALLOCN_FAIL;
	    	    	gcc_pl_abort(mde, ccb, &status,
    	    	            	    (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
	    	    	return FAIL;
		    }

    	    	    mde_out->ccb = ccb;
    	    	    mde_out->p1 = mde_out->p2 = mde_out->pdata;
	    	    mde_out->mde_srv_parms.pl_parms.ps_msg_type =
	    	    	mde->mde_srv_parms.pl_parms.ps_msg_type;
	    	    mde_out->mde_srv_parms.pl_parms.ps_flags =
    		    	mde->mde_srv_parms.pl_parms.ps_flags;
    	    	    MEcopy((PTR) mde->p1, length, (PTR) mde_out->p1);
    	    	    mde_out->p2 += length;
    	    	    mde->p1 += length;

    	    	    /* 
	    	    ** Send P_DATA to the AL.
	    	    */
    		    mde_out->service_primitive = P_DATA;
	    	    mde_out->primitive_type = IND;

	            if(call_status = gcc_al(mde_out))
		    {
    	    	    	/* Free the input MDE */
		    	gcc_del_mde( mde );
		    	return (call_status);
		    }

		}
    	    	else
		    /* We're at the last message */
		    break;
    	    } 

    	    /* Last (or only) message in the MDE. Just pass it up */
	    mde->service_primitive = P_DATA;
	    mde->primitive_type = IND;
	    layer_id = UP;
	}

    	break;
    } /* end case OPPDI */
    case OPPEI:
    {
    	u_i2	length;

	/*
	** Send a P_EXP_DATA indication to AL.
	** 
	** No OSI architected PCI is associated with a TD PPDU.  However, GCF
	** messages require a PCI structure in order that they may be
	** identified, so it is extracted here.
    	**
	** Note: Concatentation of expedited data is NOT supported as this
	** would potentially lead to the rule for expedited data (that it be
	** delivered before subsequently sent normal data) being broken.
    	** However, for consistency with normal data, the PCI contains
    	** the length of the expedited data in PL versions that support 
    	** concatentation.
	*/
	if ( (mde->p2 - mde->p1) < 
	     ((ccb->ccb_pce.p_version < PL_PROTOCOL_VRSN_CONCAT) ? SZ_PDV 
	     							 : SZ_PDVN) )
	{
	    STATUS status = E_GC240E_IE_BAD;
	    gcc_pl_abort( mde, ccb, &status,
			 (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
	    return( FAIL );
	}

	mde->service_primitive = P_EXP_DATA;
	mde->primitive_type = IND;

	/*
	**  Rip out the pci.
	*/
	pci = mde->p1;
	pci += GCgeti1(pci, &mde->mde_srv_parms.pl_parms.ps_flags);
	pci += GCgeti1(pci, &mde->mde_srv_parms.pl_parms.ps_msg_type);

    	if (ccb->ccb_pce.p_version >= PL_PROTOCOL_VRSN_CONCAT)
    	    pci += GCgeti2(pci, &length);
	else  if ( ccb->ccb_pce.p_version <= PL_PROTOCOL_VRSN_CSET )
	{
	    /*
	    ** The End-Of-Group flag is not sent at lower protocols.
	    ** Always set the end-of-group flag for expedited data.
	    */
	    mde->mde_srv_parms.pl_parms.ps_flags |= PS_EOG;
	}

	mde->p1 = pci;
	layer_id = UP;
	break;
    } /* end case OPPEI */
    case OPPDIH:
    {
	/*
	** Perform data conversion and send P_DATA to AL.
	** This calls the AL directly, so layer_id is left with US.
	*/

	call_status = gcc_pdc_up( mde, NORMAL );
	layer_id = US;
	break;
    }/* end case OPPDIH */
    case OPPEIH:
    {
	/*
	** Perform data conversion and send P_EXP_DATA to AL.
	** This calls the AL directly, so layer_id is left with US.
	*/

	call_status = gcc_pdc_up( mde, EXPEDITED );
	layer_id = US;
	break;
    }/* end case OPPEIH */
    case OPPRI:
    {
	/*
	** Send a P_RELEASE indication to AL.
	** 
	** There is no PPDU and hence no PCI associated with this PL service
	** primitive.  
	*/

	mde->service_primitive = P_RELEASE;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OPPRI */
    case OPPRC:
    {
	/*
	** Send a P_RELEASE confirm to AL.
	** 
	** There is no PPDU and hence no PCI associated with this PL service
	** primitive.  The result code in the service parms is taken from
	** the S_RELEASE confirm.
	*/

	mde->service_primitive = P_RELEASE;
	mde->primitive_type = CNF;
	mde->mde_srv_parms.pl_parms.ps_result =
	    mde->mde_srv_parms.sl_parms.ss_result;
	layer_id = UP;
	break;
    } /* end case OPPRC */
    case OPSCQ:
    {
	PL_CP_PCI	cpo_pci;

	/*
	** Send an S_CONNECT request to SL.
	*/

	gcc_rqst_av( &cpo_pci.dflt_cntxt );

	/*
	** Construct the CP PPDU.
	** 
	** Set local pointer to start of PL PCI area, and set the offset in the
	** MDE.
	*/
	pci = ( mde->p1 -= SZ_CP_PCI );
	pci += GCaddn2(0, pci);	    /* Calling psel (unused) */
	pci += GCaddn2(0, pci);	    /* Called psel (unused) */

        /* Insert the PL protocol version in the CP PPDU. NOTE: At
        ** 6.1 and 6.2 there was a bizarre mistake in the encoding of
        ** the version number. The code used is shown below:
        **
        ** cpo_pci->prot_vrsn[0] = PL_PROTOCOL_VRSN & 0xFF;
        ** cpo_pci->prot_vrsn[1] = (PL_PROTOCOL_VRSN >> sizeof(u_char)) & 0xFF;
        **
        ** This code had the mistaken belief that sizeof returned a bit
        ** as opposed to a byte count. The net result was an encoding of
        ** 06 03 at 6.2!!! (6.1 encoding was 00 00)
        */
	pci += GCaddn2(IIGCc_global->gcc_pl_protocol_vrsn, pci);

 	/* Insert the architecture characteristic (a constant octet array
	** set up at PL init. time.) into the Default Context area.
	*/
	pci += GCadds((u_char *)&cpo_pci.dflt_cntxt, SZ_CONTEXT, pci);
	pci += GCaddn2(0, pci);	    /* Presentation reqs (unused) */
	pci += GCaddn2(0, pci);	    /* Session reqs (unused) */

	/* Fill in the S_CONNECT request SDU */

     	mde->service_primitive = S_CONNECT;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to SL 
	*/

	layer_id = DOWN;
	break;
    } /* end case OPSCQ */
    case OPSCA:
    {
	PL_CPA_PCI	cpao_pci;

	/*
	** Create a CPA (Connect Presentation Accept) PPDU and send an
	** S_CONNECT response/accept to SL.
	** 
    	*/

	gcc_rsp_av( ccb, &cpao_pci.dflt_cntxt );

	pci = ( mde->p1 -= SZ_CPA_PCI );
	pci += GCaddn2(0,pci);	    /* called psel (unused) */

 	/* Insert the architecture characteristic (a constant octet array
	** set up at PL init. time.) into the Default Context area.
	*/
	pci += GCadds((u_char *)&cpao_pci.dflt_cntxt, SZ_CONTEXT, pci);	

    	/*
	** Insert the PL protocol version in the CP PPDU. NOTE: the PL
    	** protocol version is _declared_ not negotiated.
    	*/
	pci += GCaddn2(IIGCc_global->gcc_pl_protocol_vrsn, pci);
	pci += GCaddn2(0,pci);	    /* presentation reqs (unused) */
	pci += GCaddn2(0,pci);	    /* session reqs (unused) */

	/* Fill in the S_CONNECT response SDU */

	mde->service_primitive = S_CONNECT;
	mde->primitive_type = RSP;
	mde->mde_srv_parms.sl_parms.ss_result = SS_ACCEPT;

	/*
	** Send the SDU to SL 
	*/

	layer_id = DOWN;
	break;
    }/* end case OPSCA */
    case OPSCJ:
    {
	/*
	** Create a CPR (Connect Presentation Reject ) PPDU and send an
	** S_CONNECT response/reject to SL.
	** 
	*/
	pci = ( mde->p1 -= SZ_CPR_PCI );
	pci += GCaddn2(0,pci);
	pci += GCaddns(SZ_CONTEXT, pci);	

    	/*
	** Insert the PL protocol version in the CP PPDU. NOTE: the PL
    	** protocol version is _declared_ not negotiated.
    	*/
	pci += GCaddn2(IIGCc_global->gcc_pl_protocol_vrsn, pci);
	pci += GCaddn2(PS_USER_REJECT, pci);

	/* Fill in the S_CONNECT response SDU */

	mde->service_primitive = S_CONNECT;
	mde->primitive_type = RSP;
	mde->mde_srv_parms.sl_parms.ss_result = SS_USER_REJECT;

	/*
	** Send the SDU to SL
	*/

	layer_id = DOWN;
	break;
    }/* end case OPSCJ */
    case OPARU:
    {

	/*
	** Send an S_U_ABORT request to SL, with an ARU PPDU.
	**
	** The PL ARU PCI is moved in, and the S_U_ABORT service primitive
	** is constructed.
	*/

	/*
	** Construct the ARU PPDU.
	** 
	** Set local pointer to start of PL PCI area, and set the offset in the
	** MDE.  There is currently no PCI inserted, although there is provision
	** for user data.
	*/

	pci = ( mde->p1 -= SZ_ARU_PCI );
	pci += GCaddn1(ARU, pci);

	/*	
	** Fill in the S_U_ABORT request SDU.  In the future, the reason code
	** in the parm list could be used.  Currently, it is superfluous.
	*/

	mde->service_primitive = S_U_ABORT;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to SL
	*/

	layer_id = DOWN;
	break;
    }/* end case OPARU */
    case OPARP:
    {
	GCC_MDE		*arp_mde;

	/*
	** Send an S_U_ABORT request to SL, with an ARP PPDU.
	**
	** The PL ARP PCI is moved in, and the S_U_ABORT service primitive
	** is constructed.
	*/

	arp_mde = gcc_add_mde( ccb, 0 );
  	if (arp_mde == NULL)
	{
	    /* Log an error and abort the connection */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_pl_abort(arp_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	} /* end if */

	arp_mde->ccb = ccb;

	pci = ( arp_mde->p1 -= SZ_ARP_PCI );
	pci += GCaddn1(ARP, pci);

	/*	
	** Fill in the S_U_ABORT request SDU.  In the future, the reason code
	** in the parm list could be used.  Currently, it is superfluous.
	*/

	arp_mde->service_primitive = S_U_ABORT;
	arp_mde->primitive_type = RQS;

	/*
	** Send the SDU to SL
	*/

	mde = arp_mde;
	layer_id = DOWN;
	break;
    }/* end case OPARP */
    case OPSDQ:
    {
    	u_i2    length;
    	GCC_MDE *mde_out;

	/*
	** Send S_DATA request(s) to SL.
	**
    	** See what version our peer is running at. If it's pre-concatenation
    	** just handle this like we've always done.
	*/
    	if (ccb->ccb_pce.p_version < PL_PROTOCOL_VRSN_CONCAT)
    	{

    	    /*
	    ** No OSI architected PCI is associated with a TD PPDU.  
    	    ** However, GCF messages require a PCI structure in order
    	    ** that they may be identified, so it is inserted here.
	    */

	    pci = ( mde->p1 -= SZ_PDV );
	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_flags, pci);
	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_msg_type, pci);
	
	    mde->service_primitive = S_DATA;
	    mde->primitive_type = RQS;

 	    /*
	    ** Send the SDU to TL
	    */

	    layer_id = DOWN;
    	}
    	else
	{
	    /*
            ** Concatentation of GCA messages is attempted here. Look
    	    ** to see whether there's a previously saved MDE.
    	    */
    	    if (mde_out = ccb->ccb_pce.data_mde[OUTFLOW])
	    {
		/*
		** See if there's room to concatenate message in new MDE 
    	    	** to message(s) in saved MDE. Don't forget to take account
    	    	** of PCI that will be tacked on to front of message.
		*/
    	    	length = mde->p2 - mde->p1;

    	    	if ((u_i2) (mde_out->pend - mde_out->p2) >= 
    	    	    (u_i2) (SZ_PDVN + length))
		{
		    /* There is room. Set up PCI for new MDE's message */
	    	    pci = mde_out->p2;
	            pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_flags, pci);
	    	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_msg_type,
    	    	    	    	   pci);
	    	    pci += GCaddi2(&length, pci);
    	    	    mde_out->p2 = pci;

		    /* Now copy in the actual message */
    	    	    MEcopy((PTR) mde->p1, length, (PTR) mde_out->p2);
		    mde_out->p2 += length;

    	    	    /* 
    	    	    ** See whether saved MDE now needs flushing down to the
		    ** SL. This is done if it's full _or_ if the message
		    ** we just copied into it is the last segment of the
		    ** last message in a group. To test "fullness", once
		    ** again remember that any subsequently concatenated
		    ** message would have to have PCI tacked on the front
		    ** of it.
		    */
    	    	    if (((mde_out->p2 + SZ_PDVN) >= mde_out->pend) ||
			(mde->mde_srv_parms.pl_parms.ps_flags & PS_EOG))
		    {
    	    	    	ccb->ccb_pce.data_mde[OUTFLOW] = (GCC_MDE *) NULL;

    	    		/* 
	    	    	** Send S_DATA to the SL. 
	    	    	*/
	                if(call_status = gcc_sl(mde_out))
			{
    	    	    	    /* Free the input MDE */
		    	    gcc_del_mde( mde );
		    	    return (call_status);
			}
    	    	    }

    	    	    /* Free the new MDE now we've finished with it */
		    gcc_del_mde( mde );

    	    	    /* 
    	    	    ** As we managed to concatenate to saved MDE, inhibit
    	    	    ** the subsequent calling of the SL for the new MDE we
		    ** were handed.
    	    	    */
    	    	    layer_id = US;
    	    	    break;
		}
		else
		{
		    /*
    	    	    ** There's not room for the new message in the saved MDE. 
    	    	    ** Hence, we have to pass down the saved mde, in its 
    	    	    ** current state, to the SL.
		    */
    	    	    ccb->ccb_pce.data_mde[OUTFLOW] = (GCC_MDE *) NULL;

    	    	    /* 
	    	    ** Send S_DATA to the SL. 
	    	    */
	            if(call_status = gcc_sl(mde_out))
		    {
    	    	    	/* Free the input MDE */
		    	gcc_del_mde( mde );
		    	return (call_status);
		    }

		} /* end of else */

	    } /* end of if */

	    /* Set up PCI for new MDE's message */
    	    length = mde->p2 - mde->p1;
	    pci = ( mde->p1 -= SZ_PDVN );
	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_flags, pci);
	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_msg_type,
    	      	    	   pci);
	    pci += GCaddi2(&length, pci);
    	    
	    mde->service_primitive = S_DATA;
	    mde->primitive_type = RQS;

    	    /* 
    	    ** Test whether we need to pass down new MDE to SL, or whether 
    	    ** we need to save it for potential concatenation. This is done
    	    ** if there's room for concatentation (ie it's not full) or if
    	    ** the message is not the last segment of the last message in
    	    ** a group.
	    */
    	    if (((mde->p2 + SZ_PDVN) < mde->pend) &&
    	    	(!(mde->mde_srv_parms.pl_parms.ps_flags & PS_EOG)))
    	    {
	    	/* Save the MDE for future concatenation */
    	        ccb->ccb_pce.data_mde[OUTFLOW] = mde;
    	    	layer_id = US;
	    }
    	    else
		layer_id = DOWN;
	} /* end of else */

    	break;
    }/* end case OPSDQ */
    case OPSEQ:
    {
    	u_i2	length;

	/*
	** Send an S_EXP_DATA request to SL.
	**
	** No OSI architected PCI is associated with a  PPDU.  However, GCF
	** messages require a PCI structure in order that they may be
	** identified, so it is inserted here.
    	**
	** Note: Concatentation of expedited data is NOT supported as this
	** would potentially lead to the rule for expedited data (that it be
	** delivered before subsequently sent normal data) being broken.
    	** However, for consistency with normal data, the PCI contains
    	** the length of the expedited data in PL versions that support 
    	** concatentation.  Also, any normal data waiting for concatenation
	** is sent ahead of the expedited data.
	*/

    	if (ccb->ccb_pce.p_version < PL_PROTOCOL_VRSN_CONCAT)
    	{
	    pci = ( mde->p1 -= SZ_PDV );
	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_flags, pci);
	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_msg_type, pci);
	}
	else
	{
	    if ( (call_status = gcc_pl_flush( mde->ccb )) )
		return( call_status );

	    pci = ( mde->p1 -= SZ_PDVN );
	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_flags, pci);
	    pci += GCaddi1(&mde->mde_srv_parms.pl_parms.ps_msg_type, pci);
    	    length = mde->p2 - mde->p1;
    	    pci += GCaddi2(&length, pci);
	}

	mde->service_primitive = S_EXP_DATA;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to TL
	*/

	layer_id = DOWN;
	break;
    }/* end case OPSEQ */
    case OPSDQH:
    {
	/*
	** Perform data conversion and send S_DATA to SL.
	** This calls the SL directly, so layer_id is left with US.
	*/

	call_status = gcc_pdc_down( mde, NORMAL );
	layer_id = US;
	break;
    }/* end case OPSDQH */
    case OPSEQH:
    {
	/*
	** Perform data conversion and send S_EXP_DATA to SL.
	** This calls the SL directly, so layer_id is left with US.
	**
	** Flush any normal data (waiting for concatenation) to SL first.
	*/
	if ( (call_status = gcc_pl_flush( mde->ccb )) )
	    return( call_status );

	call_status = gcc_pdc_down( mde, EXPEDITED );
	layer_id = US;
	break;
    }/* end case OPSEQH */
    case OPSRQ:
    {
	/*
	** Send an S_RELEASE request to SL.
	**
	** There is no PPDU corresponding to an S_RELEASE service primitive, so
	** there is no PL PCI.
	*/

	mde->service_primitive = S_RELEASE;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to TL
	*/

	layer_id = DOWN;
	break;
    }/* end case OPSRQ */
    case OPSRS:
    {
	/*
	** Send an S_RELEASE response to SL.  This may indicate AFFIRMATIVE or
	** NEGATIVE.
	**
	** There is no PPDU corresponding to a P_RELEASE service primitive, so
    	** there is no PL PCI
	*/

	mde->service_primitive = S_RELEASE;
	mde->primitive_type = RSP;
	switch (mde->mde_srv_parms.pl_parms.ps_result)
	{
	case PS_AFFIRMATIVE:
	{
	    mde->mde_srv_parms.sl_parms.ss_result = SS_AFFIRMATIVE;
	    break;
	}
	case PS_NEGATIVE:
	{
	    mde->mde_srv_parms.sl_parms.ss_result = SS_NEGATIVE;
	    break;
	}
	default:
	    {

	        /*
	        ** Unknown result.  This is a program bug. Log an error
	        ** and abort the connection.
	        */
	        status = E_GC2405_PL_INTERNAL_ERROR;
	        gcc_pl_abort(mde, ccb, &status, (CL_ERR_DESC *)0, 
			     (GCC_ERLIST *) NULL);
	        return(FAIL);
	    }
	} /* end switch */
	layer_id = DOWN;
	break;
    } /* end case OPSRS */
    case OPPXF:
    {
	/*
	** Send a P_XOFF indication to AL.  
	*/

	mde->service_primitive = P_XOFF;
	mde->primitive_type = IND;

	/*
	** Send the SDU to AL
	*/

	layer_id = UP;
	break;
    } /* end case OPPXF */
    case OPPXN:
    {
	/*
	** Send a P_XON indication to AL.  
	*/

	mde->service_primitive = P_XON;
	mde->primitive_type = IND;

	/*
	** Send the SDU to AL
	*/

	layer_id = UP;
	break;
    } /* end case OPPXN */
    case OPSXF:
    {
	/*
	** Send an S_XOFF request to  SL.  
	*/

	mde->service_primitive = S_XOFF;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to SL
	*/

	layer_id = DOWN;
	break;
    } /* end case OPSXF */
    case OPSXN:
    {
	/*
	** Send an S_XON request to  SL.  
	*/

	mde->service_primitive = S_XON;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to SL
	*/

	layer_id = DOWN;
	break;
    } /* end case OPSXN */
    case OPMDE: /* Delete MDE */
    {
	gcc_del_mde( mde );
	layer_id = US;
	break;
    } /* end case OPMDE */
    case OPSCR:	/* Set Release Collision */
    {
	ccb->ccb_pce.p_flags |= GCCPL_CR;
	layer_id = US;
	break;
    } /* end case OPSCR */
    case OPRCR:	/* Reset release collision */
    {
	ccb->ccb_pce.p_flags &= ~GCCPL_CR;
	layer_id = US;
	break;
    } /* end case OPRCR */
    case OPTOD: /* Delete any Tuple Object Descriptor + saved MDEs */
    	    
    {
    	if (ccb->ccb_pce.tod_ptr != NULL)
	{
    	    gcc_free(ccb->ccb_pce.tod_ptr);
	    ccb->ccb_pce.tod_ptr = NULL;
	}
	if (ccb->ccb_pce.tod_prog_ptr != NULL)
	{
    	    gcc_free(ccb->ccb_pce.tod_prog_ptr);
	    ccb->ccb_pce.tod_prog_ptr = NULL;
	}
	if ( ccb->ccb_pce.doc_stacks[OUTFLOW][NORMAL] )
	{
	    GCO_DOC *doc = (GCO_DOC *)ccb->ccb_pce.doc_stacks[OUTFLOW][NORMAL];
	    ccb->ccb_pce.doc_stacks[OUTFLOW][NORMAL] = NULL;
	    QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	}
	if ( ccb->ccb_pce.doc_stacks[OUTFLOW][EXPEDITED] )
	{
	    GCO_DOC *doc = 
	    	(GCO_DOC *)ccb->ccb_pce.doc_stacks[OUTFLOW][EXPEDITED];
	    ccb->ccb_pce.doc_stacks[OUTFLOW][EXPEDITED] = NULL;
	    QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	}
	if ( ccb->ccb_pce.doc_stacks[INFLOW][NORMAL] )
	{
	    GCO_DOC *doc = (GCO_DOC *)ccb->ccb_pce.doc_stacks[INFLOW][NORMAL];
	    ccb->ccb_pce.doc_stacks[INFLOW][NORMAL] = NULL;
	    QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	}
	if ( ccb->ccb_pce.doc_stacks[INFLOW][EXPEDITED] )
	{
	    GCO_DOC *doc = 
	    	(GCO_DOC *)ccb->ccb_pce.doc_stacks[INFLOW][EXPEDITED];
	    ccb->ccb_pce.doc_stacks[INFLOW][EXPEDITED] = NULL;
	    QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	}
    	if (ccb->ccb_pce.data_mde[OUTFLOW] != NULL)
	{
	    gcc_del_mde( ccb->ccb_pce.data_mde[OUTFLOW] );
	    ccb->ccb_pce.data_mde[OUTFLOW] = NULL;
	}
    	if (ccb->ccb_pce.data_mde[INFLOW] != NULL)
	{
	    gcc_del_mde( ccb->ccb_pce.data_mde[INFLOW] );
	    ccb->ccb_pce.data_mde[INFLOW] = NULL;
	}

	{
	    QUEUE *mde_ptr;

	    while( (mde_ptr = QUremove( &ccb->ccb_pce.mde_q[ OUTFLOW ] )) )
		gcc_del_mde( (GCC_MDE *)mde_ptr );
		     
	    while( (mde_ptr = QUremove( &ccb->ccb_pce.mde_q[ INFLOW ] )) )
		gcc_del_mde( (GCC_MDE *)mde_ptr );
	}

	layer_id = US;
	break;
    } /* end case OPTOD */
    default:
	{
	    /*
	    ** Unknown event ID.  This is a program bug. Log an error
	    ** and abort the connection.
	    */
	    status = E_GC2405_PL_INTERNAL_ERROR;
	    gcc_pl_abort(mde, ccb, &status, (CL_ERR_DESC *)0, 
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	}
    } /* end switch */

    switch (layer_id)
    {
    case UP:	/* Send to layer N+1 */
    {
	call_status = gcc_al(mde);
	break;
    }
    case DOWN:	/* Send to layer N-1 */
    {
	call_status = gcc_sl(mde);
	break;
    }
    case US:	/* Not time to send yet, layer N keeps it */
    default:
	{
	    break;
	}
    } /* end switch */

    return(call_status);
} /* end gcc_plout_exec */

/*{
** Name: gcc_pl_abort	- Presentation Layer internal abort
**
** Description:
**	gcc_pl_abort handles the processing of internally generated aborts.
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
**  	15-Jan-93 (cmorris)
**  	    Always reset p1, p2 before processing internal abort.
**  	    The pointers could be set to anything when the abort
**  	    occurred.
*/
static VOID
gcc_pl_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *generic_status, 
	      CL_ERR_DESC *system_status, GCC_ERLIST *erlist )
{
    if (!(ccb->ccb_pce.p_flags & GCCPL_ABT))
    {
	/* Log an error */
	gcc_er_log(generic_status, system_status, mde, erlist);

	/* Set abort flag so we don't get in an endless aborting loop */
        ccb->ccb_pce.p_flags |= GCCPL_ABT;    

        /* Issue the abort */
    	if (mde != NULL)
	{
	    mde->service_primitive = P_P_ABORT;
	    mde->primitive_type = RQS;
    	    mde->p1 = mde->p2 = mde->papp;
	    mde->mde_generic_status[0] = *generic_status;
            (VOID) gcc_pl(mde);
	}
    }

}  /* end gcc_pl_abort */


/* ---------------------- HETEROGENOUS SUPPORT BEGINS ---------------------- */

/*
** Name: gcc_pdc_down
**
** Description:
**	Heterogenous data conversion.  Convert message data from local 
**	platform format to network standard (platform indepedent) format.
** 
**	Initialization:
**
**	    Process internal HET/NET messages.
**	    Check DOC and continuity of message segments.
**	    Check saved output MDE and determine initial actions.
**
**	Processing Loop:
**
**	    Perform action list.
**	    Perform HET/NET conversion.
**	    Determine actions from conversion status.
**
**	Actions:
**
**	    LOAD_OUTPUT		Load output MDE to append segments/messages.
**	    SAVE_OUTPUT		Save output MDE to append segments/messages.
**	    ALLOC_OUTPUT	Allocate output MDE (includes INIT_SEGMENT).
**	    INIT_SEGMENT	Initialize PCI.
**	    END_SEGMENT		Save segment PCI info.
**	    ALLOC_DOC		Allocate DOC info.
**	    FREE_DOC		Free DOC info.
**	    SAVE_DOC		Save DOC info for continued message.
**	    SEND_OUTPUT		Send output MDE to SL.
**	    QUEUE_OUTPUT	Save output MDE on queue until flushed.
**	    FLUSH_QUEUE		Flush saved output MDE queue to SL.
**	    EXIT		Exit processing loop.
**
** Inputs:
**	mde		Input MDE containing message to be converted.
**	nrmexp		Normal or Expedited message.
**
** Returns:
**	STATUS		OK:	Input MDE data consumed, not freed or used.
**			FAIL:	Input MDE freed or reused.
**
** History:
**      06-Mar-90 (cmorris)
**          Initial template creation
**  	14-May-92 (cmorris)
**  	    Fixed memory leaks.
**  	11-Dec-92 (cmorris)
**  	    Abort connection if we get a GCA_UO_DESCR
**  	14-Dec-92 (cmorris)
**  	    Don't use intermediary structure for PCI: just copy
**  	    straight from PL parms into PCI.
**  	14-Dec-92 (cmorris)
**  	    Added support for message concatenation.
**	27-mar-95 (peeje01)
**	    Cross integration from doublebyte label 6500db_su4_us42
**	 8-Mar-96 (gordy)
**	    Simplified the gcd_convert() interface.
**	26-Jul-98 (gordy)
**	    Reset doc->state if message type changes.  This handles the
**	    cases where GCA_ATTENTION/GCA_IACK interrupt the msg stream.
**	28-jul-98 (loera01 & gordy) Bug 88214 and 91775:
**	    Before performing the conversion, change the state to DOC_DONE
**          if the GCC is in the midst of processing a message, and the 
**	    existing message type is superseded by a new incoming message 
**	    type. Otherwise, GCA purge operations won't work.
**     23-dec-98 (loera01) Bug 94673
**	    The above change covers most cases, but sometimes there is a 
**	    fragment left over--the field doc->frag_bytes is non-zero.  
**	    The field is flushed to prevent the GCC from attempting to 
**	    compile a message as if it had fragments, because the new 
**	    message has none. 
**	22-Mar-02 (gordy)
**	    Removed unused GCA_UO_DESCR.
**	12-May-03 (gordy)
**	    Rewrote to use output state of gco_convert() to processing
**	    control.
**	22-Jan-10 (gordy)
**	    Cleaned up multi-byte charset processing.  Added actions to
**	    queue and flush output MDEs based on processing status.
**	    Fragment processing now done by gco_convert() - removed
**	    associated actions.  Charset translation info moved to gco.
*/

static STATUS
gcc_pdc_down( GCC_MDE *mde, u_i1 nrmexp )
{
    GCC_CCB	*ccb = mde->ccb;
    u_i1	msg_type = mde->mde_srv_parms.pl_parms.ps_msg_type;
    u_i1	pci_size = (ccb->ccb_pce.p_version < PL_PROTOCOL_VRSN_CONCAT)
			   ? SZ_PDV : SZ_PDVN;
    GCC_MDE	*mde_out = NULL;	/* Output MDE */
    GCO_DOC	*doc = NULL;		/* Data Object Context */
    bool	eod = FALSE;		/* End-of-data */
    bool	eog = FALSE;		/* End-of-group */
    char	*pci;			/* Message segment PCI */
    u_i1	action[ 5 ];		/* Processing action list */
    u_i1	act, actions = 0;	/* Action counts */
    i2		length;
    STATUS	status;

#define	GCC_DWN_LOAD_OUTPUT		0
#define	GCC_DWN_SAVE_OUTPUT		1
#define	GCC_DWN_ALLOC_OUTPUT		2
#define	GCC_DWN_INIT_SEGMENT		3
#define	GCC_DWN_END_SEGMENT		4
#define	GCC_DWN_ALLOC_DOC		5
#define	GCC_DWN_FREE_DOC		6
#define	GCC_DWN_SAVE_DOC		7
#define	GCC_DWN_SEND_OUTPUT		8
#define	GCC_DWN_QUEUE_OUTPUT		9
#define	GCC_DWN_FLUSH_QUEUE		10
#define	GCC_DWN_EXIT			11

    static GCXLIST	gcx_down_actions[] =
    {
	{ GCC_DWN_LOAD_OUTPUT,	"LOAD_OUTPUT" },
	{ GCC_DWN_SAVE_OUTPUT,	"SAVE_OUTPUT" },
	{ GCC_DWN_ALLOC_OUTPUT,	"ALLOC_OUTPUT" },
	{ GCC_DWN_INIT_SEGMENT,	"INIT_SEGMENT" },
	{ GCC_DWN_END_SEGMENT,	"END_SEGMENT" },
	{ GCC_DWN_ALLOC_DOC,	"ALLOC_DOC" },
	{ GCC_DWN_FREE_DOC,	"FREE_DOC" },
	{ GCC_DWN_SAVE_DOC,	"SAVE_DOC" },
	{ GCC_DWN_SEND_OUTPUT,	"SEND_OUTPUT" },
	{ GCC_DWN_QUEUE_OUTPUT,	"QUEUE_OUTPUT" },
	{ GCC_DWN_FLUSH_QUEUE,	"FLUSH_QUEUE" },
	{ GCC_DWN_EXIT,		"EXIT" },
	{ 0, NULL }
    };

    /*
    ** Phase 1: Determine initial processing actions.
    */

    /* Message status flags: end-of-group only significant if end-of-data */
    if ( (eod = ((mde->mde_srv_parms.pl_parms.ps_flags & PS_EOD) != 0)) )
    	eog = ((mde->mde_srv_parms.pl_parms.ps_flags & PS_EOG) != 0);

# ifdef GCXDEBUG
    if ( IIGCc_global->gcc_trace_level >= 2 )
	TRdisplay( "cnvrt2net: convert msg %s length %d flags %x %s\n", 
		   gcx_look( gcx_getGCAMsgMap(), (i4)msg_type ), 
		   (i4)(mde->p2 - mde->p1), 
		   mde->mde_srv_parms.pl_parms.ps_flags,
		   eod ? (eog ? "(EOD,EOG)" : "(EOD)") : "" );
# endif /* GCXDEBUG */

    /*
    ** Process internal HET/NET messages.  These messages flow
    ** from client or server, through local GCC and across net
    ** to remote GCC where they are consumed.  They provide a
    ** description of TUPLE data which will follow.
    */
    switch( msg_type )
    {
    case GCA_IT_DESCR:		/* First convert to GCA_TO_DESCR */
	if ( (status = gcc_pl_pod( mde ) != OK) )
	{
	    gcc_pl_abort( mde, ccb, &status, NULL, NULL );
	    return( FAIL );
	}

	/* fall thru */
	
    case GCA_TO_DESCR:		/* Load the GCA_OBJECT_DESC. */
	if ( (status = gcc_pl_lod( mde ) != OK) )
	{
	    gcc_pl_abort( mde, ccb, &status, NULL, NULL );
	    return( FAIL );
	}
	break;
    }

    /*
    ** Was DOC saved because current message not complete?
    ** If so, Check the continuity of the message segments.
    */
    doc = (GCO_DOC *)ccb->ccb_pce.doc_stacks[ OUTFLOW ][ nrmexp ];
    ccb->ccb_pce.doc_stacks[ OUTFLOW ][ nrmexp ] = NULL;

    if ( ! doc )
	action[ actions++ ] = GCC_DWN_ALLOC_DOC;
    else  if ( msg_type != doc->message_type )  
    {
	/*
	** Current message is not a continuation of previous message.
	*/
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 3 )
	    if ( doc->state == DOC_DONE )
		TRdisplay( "cnvrt2net: missing EOD for prev msg %s\n", 
			   gcx_look(gcx_getGCAMsgMap(), (i4)doc->message_type ) );
	    else
		TRdisplay( "cnvrt2net: previous message %s truncated\n", 
			   gcx_look( gcx_getGCAMsgMap(), (i4)doc->message_type ));
# endif /* GCXDEBUG */

	DOC_CLEAR_MACRO( doc );
    }
    else  if ( doc->state == DOC_DONE  &&  ! eod  &&  mde->p1 < mde->p2 )
    {
	/*
	** Completed processing of message, waiting for EOD.
	** Can't convert any additional message data, so ignore.
	*/
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
	    TRdisplay("cnvrt2net: want EOD, ignored %d bytes\n",
		      (i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */
		
	mde->p1 = mde->p2;
    }

    /*
    ** There may be a saved output MDE from a previous iteration.
    ** If not, an output MDE needs to be allocated.  If an MDE
    ** exists, then we should only need to initialize a new data
    ** segment.  An MDE is only saved for NORMAL messages.  If
    ** this is an EXPEDITED message then any saved output MDE
    ** must be flushed prior to processing this message.
    */ 
    if ( ! ccb->ccb_pce.data_mde[ OUTFLOW ] )
	action[ actions++ ] = GCC_DWN_ALLOC_OUTPUT;
    else  if ( nrmexp == NORMAL )
    {
	action[ actions++ ] = GCC_DWN_LOAD_OUTPUT;
	action[ actions++ ] = GCC_DWN_INIT_SEGMENT;
    }
    else
    {
	/*
	** Any saved output MDEs need to be flushed prior
	** to sending the current output MDE.
	*/
	if ( ccb->ccb_pce.mde_q[ OUTFLOW ].q_next != 
		&ccb->ccb_pce.mde_q[ OUTFLOW ] )
	    action[ actions++ ] = GCC_DWN_FLUSH_QUEUE;

	action[ actions++ ] = GCC_DWN_LOAD_OUTPUT;
	action[ actions++ ] = GCC_DWN_SEND_OUTPUT;
	action[ actions++ ] = GCC_DWN_ALLOC_OUTPUT;
    }

    /*
    ** Phase 2: Perform processing actions.
    */
  process_loop:

    for( act = 0; act < actions; act++ )
    {
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 3 )
	    TRdisplay( "cnvrt2net: action %s\n",
		       gcx_look( gcx_down_actions, (i4)action[ act ] ) );
# endif /* GCXDEBUG */

	switch( action[ act ] )
	{
	case GCC_DWN_LOAD_OUTPUT :	/* Load saved output MDE */
	    mde_out = ccb->ccb_pce.data_mde[ OUTFLOW ];
	    ccb->ccb_pce.data_mde[ OUTFLOW ] = NULL;
	    break;

	case GCC_DWN_SAVE_OUTPUT :	/* Save output MDE */
    	    ccb->ccb_pce.data_mde[ OUTFLOW ] = mde_out;
	    mde_out = NULL;
	    break;

	case GCC_DWN_ALLOC_OUTPUT :	/* Allocate output MDE */
    	    if ( ! (mde_out = gcc_add_mde( ccb, ccb->ccb_tce.tpdu_size )) )
	    {
		if ( doc )  QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	    	status = E_GC2004_ALLOCN_FAIL;
	    	gcc_pl_abort( mde, ccb, &status, NULL, NULL );
	    	return( FAIL );
	    }

    	    mde_out->ccb = ccb;
    	    mde_out->p1 = mde_out->p2 = mde_out->pdata - pci_size;
    	    mde_out->service_primitive = (nrmexp == NORMAL) 
					 ? S_DATA : S_EXP_DATA;
	    mde_out->primitive_type = RQS;

	    /* Fall through to initialize PCI */

	case GCC_DWN_INIT_SEGMENT :	/* Save room for segment PCI */
	    pci = mde_out->p2;		
	    mde_out->p2 += pci_size;
	    mde_out->mde_srv_parms.pl_parms.ps_msg_type = msg_type;
	    mde_out->mde_srv_parms.pl_parms.ps_flags = 0;
	    break;

	case GCC_DWN_END_SEGMENT :	/* Save segment PCI info */
	    length = (mde_out->p2 - pci) - pci_size;
	    pci += GCaddi1( &mde_out->mde_srv_parms.pl_parms.ps_flags, pci );
	    pci += GCaddi1( &mde_out->mde_srv_parms.pl_parms.ps_msg_type, pci );
	    if ( ccb->ccb_pce.p_version >= PL_PROTOCOL_VRSN_CONCAT )
		pci += GCaddi2( &length, pci );
	    break;
	
	case GCC_DWN_ALLOC_DOC :	/* Allocate DOC */
	    /* Get new DOC from free queue or allocate */
	    if ( ! (doc = (GCO_DOC *)QUremove(IIGCc_global->gcc_doc_q.q_next)) 
		 &&  ! (doc = (GCO_DOC *)gcc_alloc( sizeof( GCO_DOC ) )) )
	    {
		if ( mde_out )  gcc_del_mde( mde_out );
		status = E_GC2004_ALLOCN_FAIL;
		gcc_pl_abort( mde, ccb, &status, NULL, NULL );
		return( FAIL );
	    }

	    DOC_CLEAR_MACRO( doc );
	    break;

	case GCC_DWN_FREE_DOC :		/* Free DOC */
	    QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	    doc = NULL;
	    break;

	case GCC_DWN_SAVE_DOC :		/* Save DOC for next message segment */
	    ccb->ccb_pce.doc_stacks[ OUTFLOW ][ nrmexp ] = (PTR)doc;
	    doc = NULL;
	    break;

	case GCC_DWN_SEND_OUTPUT :	/* Send output MDE to SL */
	    if ( (status = gcc_sl( mde_out )) == OK )
		mde_out = NULL;
	    else
	    {
		if ( doc )  QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
		return( status );
	    }
	    break;

	case GCC_DWN_QUEUE_OUTPUT :	/* Save output MDE on queue */
	    QUinsert(&mde_out->mde_q_link, ccb->ccb_pce.mde_q[OUTFLOW].q_prev);
	    mde_out = NULL;
	    break;

	case GCC_DWN_FLUSH_QUEUE :	/* Flush output MDE queue to SL */
	{
	    QUEUE *mde_ptr;

	    for( 
		 mde_ptr = ccb->ccb_pce.mde_q[ OUTFLOW ].q_next;
		 mde_ptr != &ccb->ccb_pce.mde_q[ OUTFLOW ];
		 mde_ptr = ccb->ccb_pce.mde_q[ OUTFLOW ].q_next
	       )
	    {
		QUremove( mde_ptr );

		if ( (status = gcc_sl( (GCC_MDE *)mde_ptr )) != OK )
		{
		    if ( doc ) QUinsert(&doc->q, &IIGCc_global->gcc_doc_q);
		    return( status );
		}
	    }
	    break;
	}

	case GCC_DWN_EXIT :		/* Exit processing loop */
	    /*
	    ** The DOC and output MDE should have been
	    ** taken care of already, but just in case...
	    */
	    if ( doc )  QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	    if ( mde_out )  gcc_del_mde( mde_out );
	    return( OK );
	}
    }

    /*
    ** Phase 3: Call GCO to perform HET/NET conversions on message data.
    */
    doc->src1 = mde->p1;
    doc->src2 = mde->p2;
    doc->dst1 = mde_out->p2;
    doc->dst2 = mde_out->pend;
    doc->message_type = msg_type;
    doc->eod = eod;
    doc->tod_prog = ccb->ccb_pce.tod_prog_ptr;

    status = gco_convert( doc, OUTFLOW, 
    			  &((GCC_CSET_XLT *)ccb->ccb_pce.p_csetxlt)->xlt );

    mde->p1 = doc->src1;	/* Save output positions */
    mde_out->p2 = doc->dst1;

    if ( status != OK )		/* Fatal HET/NET processing error */
    {
	QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	gcc_del_mde( mde_out );
	gcc_pl_abort( mde, ccb, &status, NULL, NULL);
	return( FAIL );
    }

    /*
    ** Phase 4: Determine processing actions for current state.
    */
    actions = 0;

    switch( doc->state )
    {
    case DOC_MOREOUT :		/* Need output buffer space */
	/*
	** If output has been queued, due to a MOREQOUT
	** result, and hasn't been flushed, then the
	** current output needs to be handled the same
	** as MOREQOUT until it can be flushed along
	** with the preceding data.
	*/
	if ( ccb->ccb_pce.mde_q[ OUTFLOW ].q_next == 
		&ccb->ccb_pce.mde_q[ OUTFLOW ] )
	{
	    action[ actions++ ] = GCC_DWN_END_SEGMENT;
	    action[ actions++ ] = GCC_DWN_SEND_OUTPUT;
	    action[ actions++ ] = GCC_DWN_ALLOC_OUTPUT;
	    break;
   	}

	/*
	** Fall through for MOREQOUT condition...
	*/

    case DOC_MOREQOUT :		/* Need output buffer space, save current */
    	action[ actions++ ] = GCC_DWN_END_SEGMENT;
	action[ actions++ ] = GCC_DWN_QUEUE_OUTPUT;
	action[ actions++ ] = GCC_DWN_ALLOC_OUTPUT;
	break;

    case DOC_FLUSHQOUT :	/* Flush saved output */
    	action[ actions++ ] = GCC_DWN_FLUSH_QUEUE;
	break;

    /*
    ** Processing of MOREIN and DONE is almost identical
    ** except for the conditions where warnings are produced 
    ** and the saving of atomic data fragments.
    **
    ** DOC_MOREIN : EOD is not expected.  If EOD issue a
    ** message truncation warning.  Otherwise, save any
    ** unprocessed atomic fragment for later processing.
    **
    ** DOC_DONE : EOD is expected, but it may come in a
    ** subsequent empty message segment.  Irregardless,
    ** any message data remaining is truncated (with 
    ** suitable warning).
    */
    case DOC_DONE :		/* Message processing complete */
	/*
	** There should not be any message data remaining.  
	** If there is data, we can't convert to network 
	** format so we must ignore it (with suitable trace).
	*/
	if ( mde->p1 < mde->p2 )
	{
# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >= 2 )
		TRdisplay( "cnvrt2net: extra message data (ignored %d bytes)\n",
			   (i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */
	
	    mde->p1 = mde->p2;
	}

    	/*
	** Fall through for common processing...
	*/

    case DOC_MOREIN :		/* Need more input message data */
	action[ actions++ ] = GCC_DWN_END_SEGMENT;

	/* 
	** Input message state determines DOC handling:
	** if end-of-data, DOC is freed and message flags set;
	** otherwise, DOC is saved for next message segment.
	*/
	if ( ! eod )
	    action[ actions++ ] = GCC_DWN_SAVE_DOC;
	else
	{
# ifdef GCXDEBUG
	    /*
	    ** Give warning when input is expected but not available.
	    */
	    if (doc->state == DOC_MOREIN && IIGCc_global->gcc_trace_level >= 2)
		TRdisplay("cnvrt2net: message truncated: %d byte fragment\n",
			  (i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */

	    mde->p1 = mde->p2;
    	    mde_out->mde_srv_parms.pl_parms.ps_flags |= PS_EOD;
    	    if ( eog )  mde_out->mde_srv_parms.pl_parms.ps_flags |= PS_EOG;
	    action[ actions++ ] = GCC_DWN_FREE_DOC;
	}

	/*
	** The output MDE is saved (to append additional message
	** segments) under the following conditions:
	**
	**   The PL protocol level supports message concatenation.
	**   There is space for another PCI segment.
	**   The message flow type is NORMAL.
	**   The current message is not complete (concat next data segment)
	**	or is not end-of-group (concat next message).
	*/
	if (
	     ccb->ccb_pce.p_version >= PL_PROTOCOL_VRSN_CONCAT  &&
    	     (mde_out->p2 + pci_size) < mde_out->pend  &&
	     nrmexp == NORMAL  &&  (! eod  ||  ! eog)
	   )
	    action[ actions++ ] = GCC_DWN_SAVE_OUTPUT;
	else  if ( ccb->ccb_pce.mde_q[ OUTFLOW ].q_next != 
		   &ccb->ccb_pce.mde_q[ OUTFLOW ] )
	    action[ actions++ ] = GCC_DWN_QUEUE_OUTPUT;
	else
	    action[ actions++ ] = GCC_DWN_SEND_OUTPUT;

	/*
	** We be done for now.
	*/
	action[ actions++ ] = GCC_DWN_EXIT;
	break;
    
    default :
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay("cnvrt2net: invalid DOC state: %d\n", (i4)doc->state);
# endif /* GCXDEBUG */
	QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	gcc_del_mde( mde_out );
	status = E_GC2405_PL_INTERNAL_ERROR;
	gcc_pl_abort( mde, ccb, &status, NULL, NULL);
	return( FAIL );
    }

    goto process_loop;
} /* gcc_pdc_down() */



/*
** Name: gcc_pdc_up
**
** Description:
**	Heterogenous data conversion.  Convert message data from network
**	standard (platform indepedent) format to local platform format.
** 
**	Handles message concatenation by setting the MDE pointers for
**	each message segment and calling gcc_pdc_lcl() to process the
**	segment.
**
** Inputs:
**	mde		Input MDE containing message to be converted.
**	nrmexp		Normal or Expedited message.
**
** Returns:
**	STATUS		OK:	Input MDE data consumed, not freed or used.
**			FAIL:	Input MDE freed or reused.
** History:
**      06-Mar-90 (cmorris)
**          Initial template creation
**  	16-Dec-92 (cmorris)
**  	    Added support for message concatenation.
**	 8-Mar-96 (gordy)
**	    Simplified the gcd_convert() interface.
**	13-May-96 (gordy)
**	    Fix related to BUG #75718.  Referencing mde->p2 rather than
**	    mde_p2 would cause the server to hang when mde_p2 was only
**	    a few bytes less than mde->p2.  As a result, some bytes
**	    would be moved to the frag_buffer and mde->p1 would end up
**	    greater than mde_p2 and the inner loop would not be exited.
**	26-Jul-98 (gordy)
**	    Reset doc->state if message type changes.  This handles the
**	    cases where GCA_ATTENTION/GCA_IACK interrupt the msg stream.
**	28-jul-98 (loera01 & gordy) bug 88214 and 91775:
**	    Before performing the conversion, change the state to DOC_DONE
**          if the GCC is in the midst of processing a message, and the 
**	    existing message type is superseded by a new incoming message 
**	    type. Otherwise, GCA purge operations won't work.
**     23-dec-98 (loera01) Bug 94673
**	    The above change covers most cases, but sometimes there is a 
**	    fragment left over--the field doc->frag_bytes is non-zero.  
**	    The field is flushed to prevent the GCC from attempting to 
**	    compile a message as if it had fragments, because the new 
**	    message has none. 
**	 4-Feb-99 (gordy)
**	    Set/clear PS_EOG for lower protocols based on message type.
**	22-Mar-02 (gordy)
**	    Removed unused GCA_UO_DESCR.
**	12-May-03 (gordy)
**	    Extracted bulk of processing to gcc_pdc_lcl().
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
*/

static STATUS
gcc_pdc_up( GCC_MDE *mde, i4  nrmexp )
{
    GCC_CCB	*ccb = mde->ccb;
    char	*mde_p2 = mde->p2; /* End-of-data, mde->p2 is end-of-segment */
    u_i1	pci_size = (ccb->ccb_pce.p_version < PL_PROTOCOL_VRSN_CONCAT)
			   ? SZ_PDV : SZ_PDVN;
    STATUS	status = OK;

    /*
    ** Each call to gcc_pdc_lcl() processes exactly one 
    ** message segment in the MDE.  We extract the next
    ** message segment and call gcc_pdc_lcl() until no 
    ** message segments remain.
    **
    ** Note: for pre-concatentation versions, the MDE 
    ** will contain precisely one message.
    */
    while( (mde->p1 + pci_size) <= mde_p2 )
    {
    	/*
    	** Get info from PCI
    	*/
        mde->p1 += GCgeti1( mde->p1, &mde->mde_srv_parms.pl_parms.ps_flags );
    	mde->p1 += GCgeti1( mde->p1, &mde->mde_srv_parms.pl_parms.ps_msg_type );

	if ( ccb->ccb_pce.p_version <= PL_PROTOCOL_VRSN_CSET )
	{
	    /*
	    ** The End-Of-Group flag is not set at lower protocols.
	    ** Set or clear EOG based on the message type.  Note
	    ** that the messages tested are a subset of what GCA
	    ** actually uses, but they are sufficient for the
	    ** protocol level involved.
	    */
	    if ( 
		 mde->mde_srv_parms.pl_parms.ps_msg_type != GCA_QC_ID  &&
		 mde->mde_srv_parms.pl_parms.ps_msg_type != GCA_TDESCR &&
		 mde->mde_srv_parms.pl_parms.ps_msg_type != GCA_TUPLES &&
		 mde->mde_srv_parms.pl_parms.ps_msg_type != GCA_CDATA
	       )
		mde->mde_srv_parms.pl_parms.ps_flags |= PS_EOG;
	    else
		mde->mde_srv_parms.pl_parms.ps_flags &= ~PS_EOG;
	}
    	else  if ( ccb->ccb_pce.p_version >= PL_PROTOCOL_VRSN_CONCAT )
	{
	    /*
	    ** Setup MDE for next message segment.
	    */
	    u_i2 length;
    	    mde->p1 += GCgeti2( mde->p1, &length );
	    length = min( length, mde_p2 - mde->p1 );
    	    mde->p2 = mde->p1 + length;		/* End of message segment */ 
	}

	/*
	** Now process the message segment.
	*/
	if ( (status = gcc_pdc_lcl( mde, nrmexp )) != OK )  break;
    }

# ifdef GCXDEBUG
    if ( status == OK  &&  mde->p1 < mde_p2 )
	if ( IIGCc_global->gcc_trace_level >= 2 )
	    TRdisplay( "cnvrt2lcl: invalid PCI size: %d bytes\n", 
			(i4)(mde_p2 - mde->p1) );
# endif /* GCXDEBUG */

    return( status );
} /* gcc_pdc_up() */


/*
** Name: gcc_pdc_lcl
**
** Description:
**	Heterogenous data conversion.  Convert message data from network
**	standard (platform indepedent) format to local platform format.
** 
**	Initialization:
**
**	    Check DOC and continuity of message segments.
**	    Check saved output MDE and determine initial actions.
**
**	Processing Loop:
**
**	    Perform action list.
**	    Perform HET/NET conversion.
**	    Determine actions from conversion status.
**
**	Actions:
**
**	    LOAD_OUTPUT		Load output MDE to append messages data.
**	    SAVE_OUTPUT		Save output MDE to append messages data.
**	    ALLOC_OUTPUT	Allocate output MDE.
**	    ALLOC_DOC		Allocate DOC info.
**	    FREE_DOC		Free DOC info.
**	    SAVE_DOC		Save DOC info for continued message.
**	    SEND_OUTPUT		Send output MDE to SL.
**	    QUEUE_OUTPUT	Save output MDE on queue until flushed.
**	    FLUSH_QUEUE		Flushe saved output MDE queue to SL.
**	    EXIT		Exit processing loop.
**
** Inputs:
**	mde		Input MDE containing message to be converted.
**	nrmexp		Normal or Expedited message.
**
** Returns:
**	STATUS		OK:	Input MDE data consumed, not freed or used.
**			FAIL:	Input MDE freed or reused.
**
** History:
**	12-May-03 (gordy)
**	    Extracted from gcc_pdc_up() and rewrote to use output state
**	    of gco_convert() for processing control.
**	24-Nov-03 (gordy)
**	    When processing GCA_TO_DESCR, set mde_out to NULL when freed.
**	22-Jan-10 (gordy)
**	    Cleaned up multi-byte charset processing.  Added actions to
**	    queue and flush output MDEs based on processing status.
**	    Fragment processing now done by gco_convert() - remvoed
**	    associated actions.  Charset translation info moved to gco.
*/

static STATUS
gcc_pdc_lcl( GCC_MDE *mde, i4  nrmexp )
{
    GCC_CCB	*ccb = mde->ccb;
    u_i1	msg_type = mde->mde_srv_parms.pl_parms.ps_msg_type;
    GCO_DOC	*doc = NULL;			/* Data Object Context */
    GCC_MDE	*mde_out = NULL;		/* Output MDE */
    bool	eod = FALSE;			/* End-of-data */
    bool	eog = FALSE;			/* End-of-group */
    u_i1	action[ 5 ];			/* Processing action list */
    u_i1	act, actions = 0;		/* Action counts */
    STATUS	status;

#define	GCC_UP_LOAD_OUTPUT		0
#define	GCC_UP_SAVE_OUTPUT		1
#define	GCC_UP_ALLOC_OUTPUT		2
#define	GCC_UP_ALLOC_DOC		3
#define	GCC_UP_FREE_DOC			4
#define	GCC_UP_SAVE_DOC			5
#define	GCC_UP_SEND_OUTPUT		6
#define	GCC_UP_QUEUE_OUTPUT		7
#define	GCC_UP_FLUSH_QUEUE		8
#define	GCC_UP_EXIT			9

    static GCXLIST	gcx_up_actions[] =
    {
	{ GCC_UP_ALLOC_OUTPUT,	"ALLOC_OUTPUT" },
	{ GCC_UP_LOAD_OUTPUT,	"LOAD_OUTPUT" },
	{ GCC_UP_SAVE_OUTPUT,	"SAVE_OUTPUT" },
	{ GCC_UP_ALLOC_DOC,	"ALLOC_DOC" },
	{ GCC_UP_FREE_DOC,	"FREE_DOC" },
	{ GCC_UP_SAVE_DOC,	"SAVE_DOC" },
	{ GCC_UP_SEND_OUTPUT,	"SEND_OUTPUT" },
	{ GCC_UP_QUEUE_OUTPUT,	"QUEUE_OUTPUT" },
	{ GCC_UP_FLUSH_QUEUE,	"FLUSH_QUEUE" },
	{ GCC_UP_EXIT,		"EXIT" },
	{ 0, NULL }
    };

    /*
    ** Phase 1: Determine initial processing actions.
    */

    /* Message status flags: end-of-group only significant if end-of-data */
    if ( (eod = ((mde->mde_srv_parms.pl_parms.ps_flags & PS_EOD) != 0)) )
	eog = ((mde->mde_srv_parms.pl_parms.ps_flags & PS_EOG) != 0);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
	TRdisplay( "cnvrt2lcl: convert msg %s length %d flags %x %s\n", 
		    gcx_look( gcx_getGCAMsgMap(), (i4)msg_type ),
		    (i4)(mde->p2 - mde->p1), 
		    mde->mde_srv_parms.pl_parms.ps_flags,
		    eod ? (eog ? "(EOD,EOG)" : "(EOD)") : "" );
# endif /* GCXDEBUG */

    /*
    ** Was DOC saved because current message not complete?
    ** If so, check the continuity of the message segments.
    */
    doc = (GCO_DOC *)ccb->ccb_pce.doc_stacks[ INFLOW ][ nrmexp ];
    ccb->ccb_pce.doc_stacks[ INFLOW ][ nrmexp ] = NULL;

    if ( ! doc )
	action[ actions++ ] = GCC_UP_ALLOC_DOC;
    else  if ( msg_type != doc->message_type )
    {
	/*
	** Current message is not a continuation of previous message.
	*/
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 3 )
	    if ( doc->state == DOC_DONE )
		TRdisplay( "cnvrt2lcl: missing EOD for prev msg %s\n", 
			   gcx_look(gcx_getGCAMsgMap(), (i4)doc->message_type ) );
	    else
		TRdisplay( "cnvrt2lcl: previous message %s truncated\n", 
			   gcx_look( gcx_getGCAMsgMap(), (i4)doc->message_type ));
# endif /* GCXDEBUG */

	DOC_CLEAR_MACRO( doc );
    }
    else  if ( doc->state == DOC_DONE  &&  ! eod  &&  mde->p1 < mde->p2 )
    {
	/*
	** Completed processing of message, waiting for EOD.
	** Can't convert any additional message data, so ignore.
	*/
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
	    TRdisplay( "cnvrt2lcl: want EOD, ignored %d bytes\n",
		       (i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */

	mde->p1 = mde->p2;
    }

    /*
    ** There may be a saved output MDE from a previous iteration.
    ** If not, an output MDE needs to be allocated.  An MDE is 
    ** only saved for NORMAL messages.  If this is an EXPEDITED 
    ** message then any saved output MDE must be flushed prior to 
    ** processing this message.  The same is true if the current
    ** message is not a continuation of the prior message (DOC
    ** does not exist or DOC is idle).
    */ 
    if ( ! ccb->ccb_pce.data_mde[ INFLOW ] )
	action[ actions++ ] = GCC_UP_ALLOC_OUTPUT;
    else  if ( nrmexp == NORMAL  &&  doc != NULL  &&  doc->state != DOC_IDLE )
	action[ actions++ ] = GCC_UP_LOAD_OUTPUT;
    else
    {
	/*
	** Any saved output MDEs need to be flushed prior
	** to sending the current output MDE.
	*/
	if ( ccb->ccb_pce.mde_q[ INFLOW ].q_next !=
		&ccb->ccb_pce.mde_q[ INFLOW ] )
	    action[ actions++ ] = GCC_UP_FLUSH_QUEUE;

	action[ actions++ ] = GCC_UP_LOAD_OUTPUT;
	action[ actions++ ] = GCC_UP_SEND_OUTPUT;
	action[ actions++ ] = GCC_UP_ALLOC_OUTPUT;
    }

    /*
    ** Phase 2: Perform processing actions.
    */
  process_loop:

    for( act = 0; act < actions; act++ )
    {
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 3 )
	    TRdisplay( "cnvrt2lcl: action %s\n",
		       gcx_look( gcx_up_actions, (i4)action[ act ] ) );
# endif /* GCXDEBUG */

	switch( action[ act ] )
	{
	case GCC_UP_LOAD_OUTPUT :	/* Load saved output MDE */
	    mde_out = ccb->ccb_pce.data_mde[ INFLOW ];
	    ccb->ccb_pce.data_mde[ INFLOW ] = NULL;
	    break;

	case GCC_UP_SAVE_OUTPUT :	/* Save output MDE */
    	    ccb->ccb_pce.data_mde[ INFLOW ] = mde_out;
	    mde_out = NULL;
	    break;

	case GCC_UP_ALLOC_OUTPUT :	/* Allocate output MDE */
	    if ( ! (mde_out = gcc_add_mde( ccb, ccb->ccb_tce.tpdu_size )) )
	    {
		if ( doc )  QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
		status = E_GC2004_ALLOCN_FAIL;
		gcc_pl_abort( mde, ccb, &status, NULL, NULL );
		return( FAIL );
	    }

	    /* 
	    ** Set offset in mde_out to match that in mde, 
	    ** in case mde is one of those pre-6.4 jumbo 1k ones. 
	    */
	    mde_out->ccb = ccb;
	    mde_out->p1 = mde_out->p2 = (char *)(mde_out + 1) + GCC_OFF_DATA;
	    mde_out->mde_srv_parms.pl_parms.ps_msg_type = msg_type;
	    mde_out->mde_srv_parms.pl_parms.ps_flags = 0;
	    mde_out->service_primitive = (nrmexp == NORMAL) 
					 ? P_DATA : P_EXP_DATA;
	    mde_out->primitive_type = IND;
	    break;

	case GCC_UP_ALLOC_DOC :	/* Allocate DOC */
	    if ( ! (doc = (GCO_DOC *)QUremove(IIGCc_global->gcc_doc_q.q_next))
		 &&  ! (doc = (GCO_DOC *)gcc_alloc( sizeof( GCO_DOC ) )) )
	    {
		if ( mde_out )  gcc_del_mde( mde_out );
		status = E_GC2004_ALLOCN_FAIL;
		gcc_pl_abort( mde, ccb, &status, NULL, NULL );
		return( FAIL );
	    }

	    DOC_CLEAR_MACRO( doc );
	    break;

	case GCC_UP_FREE_DOC :		/* Free DOC */
	    QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	    doc = NULL;
	    break;

	case GCC_UP_SAVE_DOC :		/* Save DOC for next message segment */
	    ccb->ccb_pce.doc_stacks[ INFLOW ][ nrmexp ] = (PTR)doc;
	    doc = NULL;
	    break;

	case GCC_UP_SEND_OUTPUT :	/* Flush output MDE to AL */
	    /*
	    ** Tuple object descriptors flow from remote GCA to
	    ** remote GCC to local GCC and are consumed.  They
	    ** are NOT passed to local GCA.
	    */
	    if ( mde_out->mde_srv_parms.pl_parms.ps_msg_type == GCA_TO_DESCR )
	    {
	    	if ( (status = gcc_pl_lod( mde_out )) != OK )
	    	{
	    	    gcc_pl_abort( mde_out, ccb, &status, NULL, NULL );
	    	    return( FAIL );
	    	}

	    	gcc_del_mde( mde_out );
		mde_out = NULL;
	    	break;
	    }

	    if ( (status = gcc_al( mde_out )) == OK )
		mde_out = NULL;
	    else
	    {
		if ( doc )  QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
		return( status );
	    }
	    break;

	case GCC_UP_QUEUE_OUTPUT :	/* Save output MDE on queue */
	    QUinsert(&mde_out->mde_q_link, ccb->ccb_pce.mde_q[INFLOW].q_prev);
	    mde_out = NULL;
	    break;

	case GCC_UP_FLUSH_QUEUE :	/* Flush output MDE queue to SL */
	{
	    QUEUE *mde_ptr;

	    for( 
		 mde_ptr = ccb->ccb_pce.mde_q[ INFLOW ].q_next;
		 mde_ptr != &ccb->ccb_pce.mde_q[ INFLOW ];
		 mde_ptr = ccb->ccb_pce.mde_q[ INFLOW ].q_next
	       )
	    {
		QUremove( mde_ptr );

		if ( (status = gcc_al( (GCC_MDE *)mde_ptr )) != OK )
		{
		    if ( doc ) QUinsert(&doc->q, &IIGCc_global->gcc_doc_q);
		    return( status );
		}
	    }
	    break;
	}

	case GCC_UP_EXIT :		/* Exit processing loop */
	    /*
	    ** The DOC and output MDE should have been
	    ** taken care of already, but just in case...
	    */
	    if ( doc )  QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	    if ( mde_out )  gcc_del_mde( mde_out );
	    return( OK );
	}
    }

    /*
    ** Phase 3: Call GCO to perform HET/NET conversions on message data.
    */
    doc->src1 = mde->p1;
    doc->src2 = mde->p2;
    doc->dst1 = mde_out->p2;
    doc->dst2 = mde_out->pend;
    doc->message_type = msg_type;
    doc->eod = eod;
    doc->tod_prog = ccb->ccb_pce.tod_prog_ptr;

    status = gco_convert( doc, INFLOW, 
    			  &((GCC_CSET_XLT *)ccb->ccb_pce.p_csetxlt)->xlt );

    mde->p1 = doc->src1;	/* Save output positions */
    mde_out->p2 = doc->dst1;

    if ( status != OK )		/* Fatal HET/NET processing error */
    {
	QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	gcc_del_mde( mde_out );
	gcc_pl_abort( mde, ccb, &status, NULL, NULL );
	return( FAIL );
    }

    /*
    ** Phase 4: Determine processing actions for current state.
    */
    actions = 0;

    switch( doc->state )
    {
    case DOC_MOREOUT :		/* Need output buffer space */
	/*
	** If output has been queued, due to a MOREQOUT
	** result, and hasn't been flushed, then the 
	** current output needs to be handled the same 
	** as MOREQUOUT until it can be flushed along
	** with the preceding data.
	*/
	if ( ccb->ccb_pce.mde_q[ INFLOW ].q_next == 
		&ccb->ccb_pce.mde_q[ INFLOW ] )
	{
	    action[ actions++ ] = GCC_UP_SEND_OUTPUT;
	    action[ actions++ ] = GCC_UP_ALLOC_OUTPUT;
	    break;
   	}

	/*
	** Fall through for MOREQOUT condition...
	*/

    case DOC_MOREQOUT :		/* Need output buffer space, save current */
        action[ actions++ ] = GCC_UP_QUEUE_OUTPUT;
	action[ actions++ ] = GCC_UP_ALLOC_OUTPUT;
	break;

    case DOC_FLUSHQOUT :	/* Flush queued output */
        action[ actions++ ] = GCC_UP_FLUSH_QUEUE;
	break;

    /*
    ** Processing for MOREIN and DONE is almost identical
    ** except for the conditions where warnings are produced 
    ** and the saving of atomic data fragments.
    **
    ** DOC_MOREIN : EOD is not expected.  If EOD issue a
    ** message truncation warning.  Otherwise, save any
    ** unprocessed atomic fragment for later processing.
    **
    ** DOC_DONE : EOD is expected, but it may come in a
    ** subsequent empty message segment.  Irregardless,
    ** any message data remaining is truncated (with 
    ** suitable warning).
    */
    case DOC_DONE :		/* Message processing complete */
	/*
	** There should not be any message data remaining.  
	** If there is data, we can't convert to network 
	** format so we must ignore it (with suitable trace).
	*/
	if ( mde->p1 < mde->p2 )
	{
# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >= 2 )
		TRdisplay("cnvrt2lcl: extra message data (ignored %d bytes)\n",
			  (i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */

	    mde->p1 = mde->p2;
	}

    	/*
	** Fall through for common processing...
	*/

    case DOC_MOREIN :		/* Need more input message data */
	/*
	** Input message state determines DOC handling:
	** if end-of-data, DOC is freed and message flags set;
	** otherwise, DOC is saved for next message segment.
	*/ 
	if ( ! eod )
	    action[ actions++ ] = GCC_UP_SAVE_DOC;
	else
	{
# ifdef GCXDEBUG
	    /*
	    ** Give warning when input is expected but not available.
	    */
	    if (doc->state == DOC_MOREIN && IIGCc_global->gcc_trace_level >= 2)
		TRdisplay("cnvrt2lcl: message truncated: %d byte fragment\n", 
			    (i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */

	    mde->p1 = mde->p2;
    	    mde_out->mde_srv_parms.pl_parms.ps_flags |= PS_EOD;
    	    if ( eog )  mde_out->mde_srv_parms.pl_parms.ps_flags |= PS_EOG;
	    action[ actions++ ] = GCC_UP_FREE_DOC;
	}

	/*
	** The output MDE is saved (to append additional message data) 
	** under the following conditions:
	**
	**   The message flow type is NORMAL.
	**   The current message is not complete.
	*/
	if ( nrmexp == NORMAL  &&  ! eod )
	    action[ actions++ ] = GCC_UP_SAVE_OUTPUT;
	else  if ( ccb->ccb_pce.mde_q[ INFLOW ].q_next !=
		   &ccb->ccb_pce.mde_q[ INFLOW ] )
	    action[ actions++ ] = GCC_UP_QUEUE_OUTPUT;
	else
	    action[ actions++ ] = GCC_UP_SEND_OUTPUT;

	/*
	** We be done for now.
	*/
	action[ actions++ ] = GCC_UP_EXIT;
	break;
    
    default :
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay("cnvrt2lcl: invalid DOC state: %d\n", (i4)doc->state);
# endif /* GCXDEBUG */

	QUinsert( &doc->q, &IIGCc_global->gcc_doc_q );
	gcc_del_mde( mde_out );
	status = E_GC2405_PL_INTERNAL_ERROR;
	gcc_pl_abort( mde, ccb, &status, NULL, NULL);
	return( FAIL );
    }

    goto process_loop;
} /* gcc_pdc_lcl() */


/*
** Name: gcc_pl_pod - turn GCA_TD_DATA into GCA_OBJECT_DESC
**
** Description:
**	In former times, when a DBMS handed a GCA_TD_DATA pointer to
**	GCA_SEND describe tuple data, GCA_SEND sent the GCA_TD_DATA to
**	the local GCC as a GCA_IT_DESCR message, and gcc_pl_pod turned
**	that into a (more) useful GCA_OBJECT_DESC.  Now that GCA_SEND
**	turns the GCA_TD_DATA into a GCA_OBJECT_DESC and sends it to
**	the local GCC as a GCA_TO_DESC, gcc_pl_pod is no longer
**	called.  This routine is maintained for compatibility.  
**
** Inputs:
**  	mde 	    	    Pointer to mde that contains GCA_TD_DATA.
**  	    	    	    
** Outputs:
**  	none
**  
**  	Returns:
**  	    STATUS
**  
** Side Effects:
**  	none
**
** History:
**  	16-Dec-92 (cmorris)
**  	    Finally gave function a complete template :-)
**	21-Dec-01 (gordy)
**	    GCA_COL_ATT no longer uses DB_DATA_VALUE.
**	22-Mar-02 (gordy)
**	    Decompose TDESCR object and build TOD using 'safe' macros.
*/
static STATUS
gcc_pl_pod( GCC_MDE *mde )
{
    char	*td = mde->p1;
    char	*tod, *buffer;
    i4		count, tod_size;
    i4		i, tmpi4;

    /*
    ** Read GCA_TD_DATA header
    */
    td += GCA_GETI4_MACRO( td, &tmpi4 );	/* gca_tsize */
    td += GCA_GETI4_MACRO( td, &tmpi4 );	/* gca_result_modifier */
    td += GCA_GETI4_MACRO( td, &tmpi4 );	/* gca_id_tdscr */
    td += GCA_GETI4_MACRO( td, &count );	/* gca_l_col_desc */
    tod_size = GCA_OBJ_SIZE( count );

    /* 
    ** Make sure that the size of the TOD that we are going to build does
    ** not exceed the available user-data area in the MDE. This is something
    ** of a hack introduced as a stop-gap to at least prevent the situation
    ** whereby the construction of the TOD runs past the end of the MDE. This
    ** can happen because the TOD message is *slightly* larger than the
    ** message from which it's built. 
    */
    if ( tod_size > (mde->pend - mde->p1) ) 
	return( E_GC2405_PL_INTERNAL_ERROR );

    if ( ! (tod = buffer = (char *)gcc_alloc( tod_size )) )
	return(  E_GC2004_ALLOCN_FAIL );

    /*
    ** Build GCA_OBJECT_DESC header
    */
    MEfill( GCA_OBJ_NAME_LEN, 0, (PTR)tod );
    tod += GCA_OBJ_NAME_LEN;				/* data_object_name */
    tmpi4 = GCA_IGNPRCLEN;
    tod += GCA_PUTI4_MACRO( &tmpi4, tod );		/* flags */
    tod += GCA_PUTI4_MACRO( &count, tod );		/* el_count */

    /* 
    ** Walk through TD building TOD 
    */
    for( i = 0; i < count; i++ )
    {
	i2	type, prec;
	i4	length;

	td += GCA_GETI4_MACRO( td, &tmpi4 );		/* db_data */
	td += GCA_GETI4_MACRO( td, &length );		/* db_length */
	td += GCA_GETI2_MACRO( td, &type );		/* db_datatype */
	td += GCA_GETI2_MACRO( td, &prec );		/* db_prec */
	td += GCA_GETI4_MACRO( td, &tmpi4 );		/* gca_l_attname */
	td += tmpi4;					/* gca_attname */

	tod += GCA_PUTI2_MACRO( &type, tod );		/* type */
	tod += GCA_PUTI2_MACRO( &prec, tod );		/* precision */
	tod += GCA_PUTI4_MACRO( &length, tod );		/* length */
	tmpi4 = 0;
	tod += GCA_PUTI4_MACRO( &tmpi4, tod );		/* od_ptr */
	tmpi4 = GCA_NOTARRAY;
	tod += GCA_PUTI4_MACRO( &tmpi4, tod );		/* arr_stat */
    }

    /* 
    ** Copy TOD into user-data area of mde 
    */
    MEcopy( (PTR)buffer, (tod - buffer), (PTR)mde->p1 );
    mde->p2 = mde->p1 + (tod - buffer);
    mde->mde_srv_parms.pl_parms.ps_msg_type = GCA_TO_DESCR;
    gcc_free( (PTR)buffer );

    return( OK );
}


/*
** Name: gcc_pl_lod - Load and compile object descriptor
**
** Description:
**      Takes a GCA_TO_DESCR message containing a GCA_OBJECT_DESC,
**  	loads it into a buffer hanging off the CCB (allocating a new
**  	buffer if required) and finally compiling it.
**
** Inputs:
**  	mde 	    	    Pointer to mde that contains GCA_TD_DESCR
**  	    	    	    
** Outputs:
**  	none
**  
**  	Returns:
**  	    STATUS
**  
** Side Effects:
**  	none
**
** History:
**  	16-Dec-92 (cmorris)
**  	    Finally gave function a template :-)
**  	16-Dec-92 (cmorris)
**  	    Return errors from gcc_alloc calls.
**      13-sep-93 (johnst)
**          Integrated from 6.4 codeline the Asiatech (kchin) change for
**          axp_osf 64-bit platform. This is the (seiwald) fix to gca_pl_lod()
**          to properly reconstruct the received object descriptor from the
**	    GCA message, for platforms that use 64-bit pointers. This matches
**	    the coresponding message formating change in gcf/gca/gcasend.c.
**	11-feb-97 (loera01) Bug 88874
**	    Make sure we have a full TOD header before determining the 
**	    element count.
**	21-Dec-01 (gordy)
**	    GCA byte stream format no longer uses PTR, so must convert
**	    GCA_ELEMENT values.
**	22-Mar-02 (gordy)
**	    Cleanup of GCA_OBJECT_DESC allows removal of nasty ifdef.
**	    New macros for TOD processing hide the details of ptr/i4.
**	22-Jan-10 (gordy)
**	    Macro added for calculating tuple program length.
*/

static STATUS
gcc_pl_lod( GCC_MDE *mde )
{
    GCC_CCB     *ccb = mde->ccb;
    bool        last_seg = (mde->mde_srv_parms.pl_parms.ps_flags & PS_EOD) != 0;
    u_i2        data_len = mde->p2 - mde->p1;

    /*
    ** To allocate space for the TOD, we need the TOD element 
    ** count which is contained in the fixed portion of the 
    ** GCA_OBJECT_DESC.  Once we have obtained the element 
    ** count, additional pieces of the TOD are simply appended 
    ** to what we have already received.
    */
    if ( ccb->ccb_pce.tod_data_len < GCA_OBJ_HDR_LEN )
    {
	i4     tod_size;

	if ( (ccb->ccb_pce.tod_data_len + data_len) < GCA_OBJ_HDR_LEN )
	    tod_size = GCA_OBJ_HDR_LEN;	/* Element count not yet available */
	else
        {
	    char *tod;
	    i4	 count, tmpi4;

	    /* extract element count */
	    if ( ! ccb->ccb_pce.tod_data_len )
	        tod = mde->p1;
	    else
            {
		data_len = GCA_OBJ_HDR_LEN - ccb->ccb_pce.tod_data_len;
		MEcopy( mde->p1, data_len, (char *)ccb->ccb_pce.tod_ptr +
		    ccb->ccb_pce.tod_data_len ); 
                ccb->ccb_pce.tod_data_len += data_len; 
		mde->p1 += data_len; 
		data_len = mde->p2 - mde->p1;
		tod = ccb->ccb_pce.tod_ptr;
            }

	    /* Compute size of TOD */
	    tod += GCA_OBJ_NAME_LEN;			/* data_object_name */
	    tod += GCA_GETI4_MACRO( tod, &tmpi4 );	/* flags */
	    tod += GCA_GETI4_MACRO( tod, &count );	/* el_count */
	    tod_size = GCA_OBJ_SIZE( count );
        }

	/* expand TOD buffer if needed */
	if ( tod_size > ccb->ccb_pce.tod_len )
        {
	    PTR tod_ptr;

	    if ( ! (tod_ptr = gcc_alloc( tod_size )) )
	        return(  E_GC2004_ALLOCN_FAIL );

	    if ( ccb->ccb_pce.tod_data_len )
	        MEcopy( ccb->ccb_pce.tod_ptr, 
			ccb->ccb_pce.tod_data_len, tod_ptr );

	    if ( ccb->ccb_pce.tod_ptr )  gcc_free( ccb->ccb_pce.tod_ptr ); 
            ccb->ccb_pce.tod_ptr = tod_ptr; 
            ccb->ccb_pce.tod_len = tod_size;
        }
    }

    /* 
    ** Append new TOD data to that hanging off the CCB. 
    */
    MEcopy( mde->p1, data_len, 
	    (char *)ccb->ccb_pce.tod_ptr + ccb->ccb_pce.tod_data_len ); 
    ccb->ccb_pce.tod_data_len += data_len;

    /* 
    ** See whether this was the last fragment of the TOD 
    */
    if ( last_seg )
    {
	char	*od = (char *)ccb->ccb_pce.tod_ptr;
	i4	tod_prog_len, count;

        /* 
	** Reset the TOD data length so that next time we get a 
	** TOD message, we'll know that it's the first fragment.
        */
        ccb->ccb_pce.tod_data_len = 0;

	od += GCA_OBJ_NAME_LEN;				/* data_object_name */
	od += GCA_GETI4_MACRO( od, &count );		/* flags */
	od += GCA_GETI4_MACRO( od, &count );		/* el_count */

	/* 
	** Make sure we have enough room to compile tod. 
	*/
	tod_prog_len = GCO_TUP_OP_MAX(count) * sizeof( GCO_OP );

	if( ccb->ccb_pce.tod_prog_len < tod_prog_len )
	{
	    if( ccb->ccb_pce.tod_prog_ptr )
		gcc_free( ccb->ccb_pce.tod_prog_ptr );
	    if(!(ccb->ccb_pce.tod_prog_ptr = gcc_alloc( tod_prog_len )))
		return E_GC2004_ALLOCN_FAIL;
	    ccb->ccb_pce.tod_prog_len = tod_prog_len;
	}

	/* 
	** Compile tuple descriptor into program for gco_convert.
	*/
	gco_comp_tod( (PTR)ccb->ccb_pce.tod_ptr, 
		      (GCO_OP *)ccb->ccb_pce.tod_prog_ptr );
    }

    return( OK );
}

/*
** Name: gcc_pl_flush
**
** Description:
**	Flushes saved MDE (waiting for concatenation) to SL.
**
** Input:
**	ccb		Connection control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	12-Jan-98 (gordy)
**	    Created.
*/

static STATUS
gcc_pl_flush( GCC_CCB *ccb )
{
    GCC_MDE	*mde_out;
    STATUS	status = OK;

    if ( (mde_out = ccb->ccb_pce.data_mde[OUTFLOW]) )
    {
	ccb->ccb_pce.data_mde[OUTFLOW] = NULL;
	status = gcc_sl( mde_out );
    }

    return( status );
}

