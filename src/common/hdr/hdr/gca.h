/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

/*
** Name: GCA.H - Application interface to GCF
**
** Description:
**      This file contains the definitions necessary for GCA users (both client
**      and server) to access the GCA service interface.
**
**	The GCA service interface is the using application's access to the
**	underlying communication services provided by the General
**	Communication Facility.  The basic conceptual functions are the
**	transfer of data between cooperating users, and the support of
**	specific application protocols.
**
**	The operational structure is probably best understood by
**	considering a "top down" view, as seen by a GCA user. From this
**	point of view, the interface operates in the following way: the GCA
**	function call is issued, in order to invoke a service, in order to
**	send a message, which contains a specific data object.  
**
**	The various data objects are the form in which data meaningful to
**	the communicating users, such as queries and query results, are
**	transferred between them. The combination of a particular message
**	type and its associated data object compose a protocol element.
**	The application protocols are implemented by the specification of
**	both the allowable and the requisite sequencing of protocol elements.
**
** History: 
**      09-Apr-87 (berrow)
**          Created.
**      15-Apr-87 (jbowers)
**          Made cosmetic changes to pointer definitions.
**      06-Aug-87 (jbowers)
**          Added parm list for GCA_DISASSOC service.
**      27-Aug-87 (jbowers)
**          Added singleton select mask definition in GCA_Q_DATA.
**      01-Sept-87 (jbowers)
**          Modified parm lists for GCA_REGISTER and GCA_INITIATE.
**      21-Sept-87 (jbowers)
**          Added GCA_SERVER_ID_LNG definition.
**          Added structure names to all typedef's.
**      21-Dec-87 (jbowers)
**          Added CL_SYS_ER element to all GCA parm lists for returning OS-
**          specific error info.
**      11-Jan-88 (jbowers)
**          Added message type GCA_Q_STATUS to allow synchronous request for
**          error status during a copy operation.
**      03-May-88 (jbowers)
**          Added message types GCA_CREPROC, GCA_INVPRC, GCA_DRPPROC,
**	    GCA_RETPROC. Aded data objects GCA_NAME, GCA_P_PARAM, GCA_IP_DATA,
**	    GCA_RP_DATA.
**      06-Jun-88 (jbower)
**	    Modified parm lists for GCA_INITIATE, GCA_LISTEN, GCA_REQUEST,
**	    GCA_SEND, GCA_RECEIVE.  Added elements to the GCA_E_ELEMENT and
**	    GCA_RE_DATA data objects.
**      13-Jun-88 (jbowers)
**          Changed the use of DB_MAXNAME defining the length of certain
**	    character arrays to GCA_MAXNAME to resolve possible FE-BE
**	    inconsistencies and to accommodate I/STAR requirements.
**      05-aug-88 (thurston)
**	    Added the following typedef definitions:  GCA_TPL, GCA_ELEMENT,
**	    GCA_OBJECT_DESC, GCA_TUPOBJ_DESC, and GCA_TPLVEC.  Also added the
**	    flag GCA_TRIAD_VEC.
**      05-aug-88 (thurston)
**	    Added the `GC_*_ATOMIC' constants.
**      01-sep-88 (thurston)
**	    Added the constants GC_NAT_ATOMIC and GC_LONGNAT_ATOMIC.
**      01-Nov-88 (jbowers)
**          Updated GCA_PROTOCOL_LEVEL to 2.  Add message E_GC000A_INT_PROT_LVL.
**	13-dec-88 (neil)
**	    Added GCA_ERFORMATTED for GW error text.
**      12-Jan-89 (jbowers)
**          Added error status E_GC000D_ASSOCN_REFUSED.
**	    Added gca_lsncnfrm callback routine to GCA_INITIATE parms.
**      10-Feb-89 (jbowers)
**          Added parm list for GCA_DCONN service function.
**      12-Feb-89 (jbowers)
**          Added GCA_ASYNC_FLAG symbol for consistency.
**      9-Mar-89 (ac)
**          Added 2PC support.
**      10-Apr-89 (jbowers)
**          Added parm list for GCA_ATTRIBS service function.
**      12-Apr-89 (jbowers)
**          Added alternate exit indicators and alternate exit parameters to
**	    GCA_RQ_PARMS, GCA_SD_PARMS and GCA_RV_PARMS, to support driving
**	    alternate function completion exits.  This capability is used
**	    internally by GCA.
**	16-jul-89 (jorge:ralph)
**	    GRANT Enhancements, Phase 1:
**	    added new mod assoc msg types GCA_GRPID, GCA_APLID.
**      16-jul-89 (jorge:neil)
**          Added gca_association_id to GCA_SAVE.
**      18-Jul-89 (cmorris)
**          End-to-End bind change: Added modifier flag and api version to
**          GCA_INITIATE parameters; api version level define; alternate
**          completion information to GCA_REQUEST, GCA_LISTEN, GCA_SEND
**          and GCA_RECEIVE; new GCA_RESPONSE function.
**	31-Jul-89 (seiwald)
**	    New error GC000C for invalid API number.
**	31-Jul-89 (seiwald)
**	    New gca_reject_reason for GCA_RQRESP.
**	1-Aug-89 (seiwald)
** 	    Gca_reject_reason change to gca_response_status.
**	    GCA_RR_ACCEPT and GCA_RR_REJECT removed.
**      03-Aug-89 (cmorris)
**          Added tokens for api version 0 and gca version 2; changed
**          protocol level to 3.
**	02-oct-89 (pasker)
**	    Added new error message for login failure due to failure of
**	    GCusrpwd
**	011-oct-89 (ham)
**	    Added defines for byte alignment.
**      05-apr-1989 (jennifer)
**          Added security label to GCA_LS_PARMS.
**	05-sep-89 (jrb)
**	    Added GCA_DECFLOAT #def as new option index.  This flag is sent
**	    from the client to inform the server which kind of numeric literal
**	    semantics should be used, decimal or floating point.
**	25-Sep-89 (seiwald)
**	    New GCFFFE_INCOMPLETE status to indicate service must
**	    be reinvoked with GCA_RESUME indicator.  Becomes available
**	    at GCA_API_LEVEL_2.
**	02-oct-89 (pasker)
**	    Added new error message for login failure due to failure of
**	    GCusrpwd
**	03-Oct-89 (seiwald)
**	    Grouped all modifiers and flags at bottom of file.  Changed
**	    GCA_RQ_PARMS unused gca_flags to gca_modifiers for accepting
**	    GCA_RQ_DESCR modifier.
**	12-Oct-89 (seiwald)
**	   Ifdef'ed the B1 stuff until they get predator working.
**      06-Nov-89 (cmorris)
**         Removed byte alignment defines.
**	10-Nov-89 (seiwald)
**	   DCONN is dead.
**	21-Nov-89 (seiwald)
**	   Added GCA_RR_PARMS to GCA_PARMLIST.
**	24-Nov-89 (seiwald)
**	   New GCN error numbers.
**	26-Nov-89 (seiwald)
**	   New E_GC0104_GCN_USERS_FILE.
**	31-Dec-89 (seiwald)
**	   New E_GC0032_NO_PEER.
**	02-Jan-90 (seiwald)
**	   New GCA_MD_ASSOC index flag: GCA_RUPASS for frontend -P 
**	   (password) flag.
**	04-Jan-90 (seiwald)
**	   Updated to GCA_PROTOCOL_LEVEL_4, for name server change.
**	   At this level, FE's can accept GCA_ERROR message in response
**	   to GCN_RESOLVE message.
**	13-feb-90 (neil)
**	    Added new message GCA_EVENT and corresponding data GCA_EV_DATA.
**      07-may-1990 (fred)
**	    Added new mask, GCA_LO_MASK, to the gca_result_modifier field of the
**	    GCA_TD_DATA (tuple descriptor) typedef.  The presence of this mask
**	    value indicates that the descriptor in which it is contained
**	    describes at least one large object type tuple field.  The length of
**	    this field will not be contained in the gca_tsize field of the
**	    descriptor.
**
**	    Also added GCA_COMP_MASK, which is used to indicate that the values
**	    are being sent in compressed format.  This may be used in the future
**	    to send less data when large varchar/text fields are involved.
** 	1-jul-90 (seiwald)
**	    Added macros to build and decompose GCA messages.
**	6-Aug-90 (anton)
**	    gca_sec_label compiled in with xORANGE not GCF63
**      4-Sep-90 (seiwald/jorge/neil)
**         Blocked (bufferd) SQL Cursors, part 1. New GCA_READONLY_MASK defined
**         in GCA_TD_DATA (server side). In addition, gca_name[GCA_MAXNAME]
**         in GCA_ID structure is over-loaded for the GCA_FETCH message
**         (client side) in this release to contain optional max
**         number of blocked tuple rows server can send. gca_name over-loading
**         will be removed in next GCA protocol by introducing new
**         GCA_FT_DATA Object for GC_FETCH message.
**	06-sep-90 (jrb)
**	    Added GCA_TIMEZONE as a GCA_MD_ASSOC index so client can indicate
**	    its time zone to the server.
**	18-Sep-90 (jorge)
**	   Added automatic setup of GCF62 through GCF64 #defines based
**	   on the lowest symbolic value GCFxx that is defined. This
**	   is easier than setting the entire range. GC.H is also modified.
**	19-Sep-90 (jorge)
**	   Added 6.3/02 patch GCA_PROTOCOL_LEVEL_31, the new protocol for 
**	   support of additional TIMEZONE information in the GCA_MD_ASSOC 
**	   message. New Protocol Numbering: new delevelopment releases
**	   will assign new protocol numbers in increments of 10 (eg: 40,50, etc)
**	   so that maintenance and patch release can introduce incremental
**	   protocol changes. New Protocol 31 Semantics: users must be
**	   prepared to skip over un-recognized elements of the GCA_MD_ASSOC
**	   and still accept the association. An error message may optionaly
**	   be logged by the user if the GCA_MD_ASSOC contains un-recognized 
**	   elements.
**	19-Sep-90 (jorge)
**	   Made the current GCA_PROTOCOL_LEVEL "40". The GCF remote node
**	   "fail-over" GCA_PROTOCOL_LEVEL_4 is now GCA_PROTOCOL_LEVEL_40.
**	   Related GCF files (GCARQST.C andGCNSRSLV.C) have been changed
**	   accordingly.
**	11-Oct-90 (jorge)
**	   JRB's GCA_TIMEZONE MD_ASSOC change got droped so I put it back in.
**	06-Dec-90 (seiwald)
**	   Adjusted size of GCA_SERVER_ID_LNG to 64 and commented other
**	   limitations on server name length.
**      10-jan-91 (stevet)
**         New E_GC0105_GCN_CHAR_INIT.
**      12-feb-91 (mikem)
**	   Added support for returning logical key values as part of
**	   the response block (GCA_RE_DATA).  "inquire_ingres" statements will 
**	   be used by the application to query this data.
**	06-Mar-91 (seiwald)
**	   New GCA_MISC_PARM for GCA_MD_ASSOC message; this flag is used
**	   to pass miscellaneous flags as text to the DBMS server.
**	24-may-91 (andre)
**	    defined GCA_NEW_EFF_USER_MASK and GCA_FLUSH_QRY_IDS_MASK over
**	    GCA_RE_DATA.gca_rqstatus.
**	19-Jun-91 (seiwald)
**	   Moved typedef's forward; added explanatory comments above 
**	   GCA_TPL; rearranged the #defines in the GCA_TPL, GCA_ELEMENT,
**	   and GCA_OBJECT_DESC to improve readability.  No functional
**	   change.
**	19-Jun-91 (seiwald)
**	   New masks GCA_NEW_EFF_USER_MASK, GCA_FLUSH_QRY_IDS_MASK on
**	   gca_rqstatus in the response block (GCA_RE_DATA).
**	12-Jul-91 (gthorpe)
**	   Added server class for HP Allbase gateway.
**	17-Jul-91 (seiwald) bug #37564
**	   New messages GCA1_C_FROM, GCA1_C_INTO with corrected copy 
**	   row domain descriptor structures GCA1_CPRDD.  The old descriptor
**	   used a character array as the 'with null' nulldata, even though
**	   its type varied.  The null value is now a GCA_DATA_VALUE.
**	   Introduced at (new) GCA_PROTOCOL_LEVEL_50.
**	12-Aug-91 (seiwald)
**	   A few adjustments to GCA1_CPRDD requested by the front ends.
**	14-Aug-91 (seiwald)
**	   Buffer pointers are now given as char *, not PTR.  This is
**	   proper because their size is given as byte count and pointer
**	   arithmetic is used on them.
**	20-Nov-91 (alan)
**	   Defined new message GCA1_DELETE at (new) GCA_PROTOCOL_LEVEL_60.
**	   Supports table owner name, as per Andre's GCA_01DELETE change.
**	26-Dec-91 (seiwald)
**	   Added comments for API levels and touched up protocol level
**	   comments.
**	26-Dec-91 (seiwald)
**	   Support for installation passwords: added GCA_RQ_RSLVD to
**	   GCA_REQUEST's gca_modifiers: indicates that the partner name
**	   is a GCN_RESOLVED message, complete with rmt connection
**	   info.  Added GCA_LS_PASSWD to GCA_LISTEN's gca_flags: 
**	   indicates that GCA_LISTEN is passing up a user name/password
**	   to be validated.  New at new API_LEVEL_3.
**	23-Jan-92 (seiwald)
**	   Support for installation passwords: new GCA_PROTOCOL_LEVEL_55
**	   (with new messages GCN_NS_AUTHORIZE and GCN_NS_AUTHORIZED).
**	   Moved GCN error numbers out into gcn.h.
**	08-Jul-92 (brucek)
**	   Various changes to support GCA_FASTSELECT and GCM associations
**	   (all available at new GCA_API_LEVEL_4):
**	   new service GCA_FASTSELECT with parms GCA_FS_PARMS;
**	   new value GCA_RQ_GCM to gca_modifiers in GCA_RQ_PARMS; 
**	   new values GCA_RG_* to gca_modifiers in GCA_RG_PARMS;
**	   new value GCA_LS_GCM for peerinfo.gca_flags;
**	   new error code E_GC0033_GCM_NOSUPP if GCM not supported.
**	10-Jul-92 (brucek)
**	   New error code E_GC0034_GCM_PROTERR for GCM protocol error.
**	11-Aug-92 (seiwald)
**	   Added new array status "GCA_VARSEGAR" to GCA_ELEMENT.  This
**	   array status indicates that the element may be repeated 
**	   indefinitely, as controlled by the previous element.
**	11-Sep-92 (brucek)
**	   Added comments reserving range of GCM message types.
**      23-Sep-92 (stevet)
**	   Added GCA_TIMEZONE_NAME, which will be support for
**	   GCA_PROTOCOL_60, to replace GCA_TIMEZONE message.
**	29-Sep-92 (brucek)
**	   Added GCA_RG_MIB_MASK.
**	01-Oct-92 (brucek)
**	   Moved GCA_RG_MIB_* bits to high-order 2 bytes.
**	06-Oct-92 (brucek)
**	   Added #defines for GCA MIB classids.
**	07-Oct-92 (brucek)
**	   Added GCA_RG_TRAP to indicate trap support.
**	08-Oct-92 (brucek)
**	   Added comments for GCA_PROTOCOL_LEVEL_60.
**	14-Oct-92 (brucek)
**	   Added numerous GCM error codes (E_GCxxxx_GCM_*).
**	   Made the definitions of gca_assoc_id, gca_flags, and gca_modifiers
**	   in various parm lists consistent: assoc_id's are nats; flags and
**	   modifiers are unsigned i4s.
**	19-Oct-92 (brucek)
**	   Added GCA_MIB_LISTEN_ADDRESS.
**      7-Oct-92 (nandak)
**         Added support for GCA_XA_RESTART and GCA_XA_DIS_TRAN_ID
**      26-oct-92 (jhahn)
**	    added GCA_IS_BYREF and GCA_PARAM_NAMES_MASK
**	    for handling byref parameters in DB procs.
**      4-Nov-92 (brucek)
**         Added #defines for GCA privilege names.
**	19-Nov-92 (seiwald)
**	   Surrounded new XA stuff with ifndef GCF64.  Moved reverse
**	   compatibility defines up to enable this ifdef.
**	19-Nov-92 (seiwald)
**	   New gca_end_of_group indicator in GCA_IT_PARMS at API_LEVEL_4.
**      20-Nov-92 (gautam)
**         Password prompt support changes at GCA_PROTOCOL_LEVEL_60.
**         Bump up GCA_PROTOCOL_LEVEL to 60. Added in GCA_RQ_REMPW flag, 
**         and gca_rem_username, gca_rem_password fields to  GCA_RQ_PARMS. 
**         Added in new error message E_GC003C_PWPROMPT_NOTSUP.
**      30-Nov-92 (brucek)
**         Added GCA_LS_FASTSELECT flag value.
**      01-Dec-92 (brucek)
**         Added GCA_LS_REMOTE flag value.
**      03-Dec-92 (brucek)
**         Changed GCA MIB variable names to remove "common".
**	04-Dec-92 (seiwald)
**	    Added GCA_VARZERO arr_stat to GCA_ELEMENT.
**      07-Dec-92 (gautam)
**         Added in PM error messages for PM based GCA privileges
**	14-Dec-92 (seiwald)
**	   Dropped GCA_PROTOCOL_LEVEL back down to 40.  It should remain stuck
**	   there.
**      31-Dec-92 (brucek)
**         Changed GCA privilege names to remove "GCA_".
**      04-Jan-93 (brucek)
**         Added GCA_LS_TRUSTED flag value.
**      08-Jan-93 (brucek)
**         Added GCA_MIB_ASSOC_FLAGS.
**	08-Jan-93 (edg)
**	   Added FUNC_EXTERN for IIGCa_call.
**      11-Jan-93 (brucek)
**         Added new GCA services GCA_REG_TRAP and GCA_UNREG_TRAP.
**      12-Jan-93 (brucek)
**         Added gca_auth_user to GCA_IN_PARMS (API level 4);
**	   Removed GCA_LS_PASSWD flag -- this flag was only used by the
**	   Name Server, and became obsolete with recent API level 4
**	   changes.  Note that even at API level 3, this flag will no
**	   longer be returned -- it is truly gone!
**      25-Jan-93 (brucek)
**         Moved GCA_CS_P* gca_modifiers definitions here from gcaimsg.h.
**      29-Jan-93 (brucek)
**         Added E_GC0040_CS_OK;
**	02-feb-93 (fpang)
**         Added GCA_IP_DATA.gca_proc_mask flag GCA_IP_NO_XACT_STMT,
**	   GCA_Q_DATA.gca_query_modifier flag GCA_Q_NO_XACT_STMT,
**	   and GCA_RE_DATA.gca_rqstatus flag GCA_ILLEGAL_XACT_STMT.
**	   The first two tell the local server to not allow commit/aborts
**	   within database procedures or execute immediates. The last one is
**	   the status returned when a local database procedure or execute
**	   immediate attempted to commit/abort.
**	17-Feb-93 (seiwald)
**	    At API_LEVEL_4, GCA_RECEIVE also does GCA_INTERPRET's job.
**	10-Mar-93 (edg)
**	    Added MIB class def for number of associations.
**	17-Jun-93 (fpang)
**	    Added new messages GCA1_DELETE and GCA1_INVPROC, and new data object
**	    GCA1_IP_DATA, which support owner.procedure and owner.table. 
**      12-Aug-93 (stevet)
**          Added new message GCA_STRTRUNC which is used for string
**          truncation option (-string_truncation flag).  Also 
**          removed duplicate define for GCA_TIMEZONE_NAME.
**	20-Sep-93 (seiwald)
**	    Added GCA_VARPADAR arr_stat, explicitly for pad sizing.
**      22-Sep-93 (iyer)
**          Changed the GCA  message structure for the GCA_XA_PREPARE
**          message. The GCA_XA_DIS_TRAN_ID struct is now typedefed to
**          DB_XA_EXTD_DIS_TRAN_ID instead of DB_XA_DIS_TRAN_ID. This struct
**          includes two additional members in addition to the original 
**          DB_XA_DIS_TRAN_ID. The two members are branch_seqnum and 
**          branch_flag. In addition to the XA_XID these two members would also
**          be logged in the IMA table and used during a recovery operation.
**      22-sep-93 (johnst)
**	    Bug #56444
**          Changed sizeof(long) to sizeof(i4) in CGA_ADDLONG_MACRO and
**          CGA_GETLONG_MACRO: GCA "long's" are actually ingres i4 types
**	    (see cvgcc.h). This caused DEC alpha (axp_osf) problems since 
**	    ingres i4s are 32-bit, but native long's are 64-bit.
**	 3-Oct-93 (robf)
**	    Added association option GCA_SL_TYPE for security labels.
**	    Added new associated protocol level, GCA_PROTOCOL_LEVEL_61.
**	    Added gca_sec_label pointers to GCA_LISTEN, GCA_REQUEST,
**	    and GCA_FASTSELECT parms along with GCA_LS_SECLABEL and
**	    GCA_RQ_SECLABEL modifiers.  Added new associated api level,
**	    GCA_API_LEVEL_5.
**	13-Oct-93 (seiwald)
**	    Introduced regular parameter placement into GCA_PARMLIST
**	    so that GCA code doesn't have to hunt for status, syserr,
**	    etc.
**	 3-Nov-93 (robf)
**	    Added GCA_PROMPT and GCA_PRREPLY messages, GCA_PROMPT_DATA and
**	    GCA_PRREPLY_DATA data objects, and GCA_CAN_PROMPT association
**	    option to GCA_PROTOCOL_LEVEL_61.
**	14-Mar-94 (seiwald)
**	    New gca_formatted flag to GCA_SEND, GCA_RECEIVE at API_LEVEL_5.
**	29-Mar-94 (seiwald)
**	    New error messages for perform encoding/decoding.
**	11-Apr-94 (seiwald)
**	    More new error messages for perform encoding/decoding.
**      13-Apr-1994 (daveb)  62240
**          Add MO object names for more statistics ({data,msgs}_{in,out}).
**	12-May-94 (seiwald)
**	    Now that GCA_REGISTER is (potentially) asynchronous, it needs
**	    to expose it's assoc_id.
**	25-May-95 (gordy)
**	    Modified meaning of GCA_VARSEGAR: segment present indicator 
**	    now considered a part of the segment (cleans up the object 
**	    descriptors).  Added GCA_VARVSEGAR for segment similar to
**	    GCA_VARVCHAR for varchar.
**	14-Jun-95 (gordy)
**	    Added GCA_XA_TX_DATA data object for GCA_XA_SECURE to
**	    represent current practice.  There is certainly a cleaner
**	    way to do this than is currently done.
**	 1-Sep-95 (gordy) bug #67475
**	    Adding GCA_RQ_SERVER and GCA_LS_SERVER flags to at api level 5.
**	19-Oct-95 (gordy)
**	    Set GCA_CAN_PROMPT and GCA_SL_TYPE to match mainline prior to
**	    integration.  These values should be contiguous with the other
**	    values, but we are stuck with the values previously used.
**	14-nov-95 (nick)
**	    Add GCA_YEAR_CUTOFF ; move protocol level to 62.
**	14-Nov-95 (gordy)
**	    Added FASTSELECT data to listen parms at API level 5.  Removes
**	    current kludge of appending data to the aux data.
**	16-Nov-95 (gordy)
**	    Added GCA1_FETCH, GCA_FT_DATA at GCA_PROTOCOL_LEVEL_62.
**	20-Nov-95 (gordy)
**	    Removed GCF6* constants since no longer properly maintained.
**	    Removed PMFE things no longer applicable to this codeline.
**	    Obsoleted gca_db_name in GCA_LS_PARMS.
**	10-Jan-96 (emmag)
**	    Added OpenIngres Desktop server class.
**	10-Jan-96 (emmag)
**	    Integrate changes required for JASMINE.
**	06-Feb-96 (rajus01)
**	    Added GCA_BRIDGESVR_CLASS, GCA_RG_BRIDGESVR for Protocol Bridge.
**	22-Mar-96 (chech02)
**	    Added IIGCa_call() function prototypes for win3.1 port.
**	21-Jun-96 (gordy)
**	    Added GCA_VARMSGAR for message which are processed in bulk
**	    rather than as formatted elements.
**	 3-Sep-96 (gordy)
**	    New GCA entry point, gca_call(), to allow multiple GCA
**	    users in a single process without interference.  MIB
**	    changes to reflect reorgination of GCA global data.
**	 5-Feb-97 (gordy)
**	    Simplified the object descriptor array status definitions.
**	    Defined the rules for usage of the array status values.
**	21-Feb-97 (gordy)
**	    Made gca_formatted at API_LEVEL_5 into gca_modifiers to
**	    handle future extensions.
**	24-Feb-97 (gordy)
**	    Provide flags to override default end-of-group calculation.
**	19-Mar-97 (gordy)
**	    Added PROTOCOL_LEVEL_63 for Name Server fixes.
**	 2-may-97 (inkdo01)
**	    Added GCA_IS_GTTPARM flag for global temptab proc parms.
**	21-May-97 (gordy)
**	    New GCF security handling.  Passwords no longer available from
**	    GCA_LISTEN.  No user authentication function in GCA_INITIALIZE.
**	29-Sep-97 (mcgem01)
**	    Added FroNTier server class.
**	 4-Dec-97 (rajus01)
**	    Installation password handling as remote authentication. Added
**	    GCA_RQ_AUTH flag, gca_auth, gca_l_auth in GCA_REQUEST parms.
**	27-Jan-98 (marol01)
**	    Added ICE server class.
**	17-Feb-98 (gordy)
**	    Added MIB entry for trace log.
**	 5-May-98 (gordy)
**	    Made gca_account_name obsolete for GCA_REQUEST, GCA_FASTSELECT.
**	28-Jul-98 (gordy)
**	    Added MIB for installation ID for server bedchecks.
**	    Exposed GCA_AUX_DATA structure returned by GCA_LISTEN.
**	26-Aug-98 (gordy)
**	    Enhancements for delegation of user authentication.  Changed
**	    GCA_AUTH_SPECD (obsolete) to GCA_AUTH_DELEGATE and added aux
**	    data GCA_ID_DELEGATED.
**	15-Sep-98 (gordy)
**	    Added GCA_INITIATE flags to control GCM access.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitions to gcm.h.
**      13-Jan-1999 (fanra01)
**          Add GCA_RG_NMSVR for registered name servers.
**      08-Mar-99 (fanra01)
**          Changed GCA_ICE_CLASS from ICE to ICESVR class.
**	 2-Jul-99 (rajus01)
**	    Added GCA_LS_EMBEDGCC for embedded comm server support.
**	26-oct-1999 (thaju02)
**	    Added E_GC0057_NOTMOUT_DLOCK. (b76625)
**	19-nov-1999 (thaju02)
**	    Modified E_GC0057_NOTMOUT_DLOCK to E_GC0058_NOTMOUT_DLOCK.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 1-Mar-01 (gordy)
**	    Added GCA_PROTOCOL_LEVEL_64 for new Unicode datatypes.
**	16-mar-2001 (somsa01)
**	    Added E_GC000E_RMT_LOGIN_FAIL_INFO.
**      01-may-2001 (loera01)
**          Added E_GC0057_RMTACCESS_FAIL.
**	 5-Oct-01 (gordy)
**	    General cleanup as followup to removal of nat/longnat.
**	    Removed DBMS dependencies by providing GCA structures
**	    for those previously used from DBMS.  Added definition
**	    of supported data types.  Removed GCA internal info.
**	22-Mar-02 (gordy)
**	    Simplified typedefs for message object which are identical
**	    to other internal data objects.
**	15-Oct-03 (gordy)
**	    Added GCA_API_LEVEL_6 for RESUME of FASTSELECT.
**	15-Mar-04 (gordy)
**	    Added GCA_PROTOCOL_LEVEL_65 for eight-byte integers and
**	    MD_ASSOC param GCA_I8WIDTH.
**	28-May-04 (gordy)
**	    GCA_REGISTER may now be limited to just the Name Server
**	    to allow re-registration (requires prior normal register).
**	01-Mar-06 (gordy)
**	    Added E_GC001A_DUP_REGISTER for invalid additional GCA_REGISTER.
**	31-May-06 (gordy)
**	    Added GCA_PROTOCOL_LEVEL_66 for ANSI date/time datatypes.
**	22-june-06 (dougi)
**	    Added GCA_IS_OUT to declare OUT only proc params.
**	30-Jun-06 (gordy)
**	    Extend XA support.
**	28-Jul-06 (gordy)
**	    Added GCA_XA_ERROR_MASK to GCA_RE_DATA to indicate that
**	    an XA error code is in gca_errd5.
**      16-oct-2006 (stial01)
**          Added GCA_LOCATOR_MASK to request locator instead of blob.
**	24-Oct-06 (gordy)
**	    Added GCA_PROTOCOL_LEVEL_67 for Blob/Clob locators.
**	12-feb-2006 (dougi)
**	    Added GCA1_FT_DATA/GCA2_FETCH for scrollable cursors.
**	12-Mar-07 (gordy)
**	    Additional changes for scrollable cursors.
**      11-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          GCA_XA_DIS_TRAN_ID must match DB_XA_DIS_TRAN_ID to avoid
**          misalignment in MEcopy operations in IIsqXATPC() in libq.
**          On 64-bit platforms a long is 8 bytes.
**      12-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          Modify all XA xid fields to i4 (int for supplied files)
**          to ensure consistent use across platforms.
**	03-Aug-2007 (gupsh01)
**	    Added GCA_DATE_ALIAS to handle date_type_alias config
**	    parameter.
**	 1-Oct-09 (gordy)
**	    Added GCA2_INVPROC message, with GCA2_IP_DATA object,
**	    at protocol level GCA_PROTOCOL_LEVEL_68.
**	 9-Oct-09 (gordy)
**	    Added protocol level GCA_PROTOCOL_LEVEL_68 for boolean type.
**	5-feb-2010 (stephenb)
**	    Add GCA_REPEAT_MASK.
*/

#ifndef GCA_INCLUDED
#define GCA_INCLUDED

/*
** General definitions
**
** GCA_SERVER_ID_LNG limits the length of a server's local listen address.
** This is also affected by the Name Server's limit GCN_VAL_MAX_LEN (64).
*/

# define	GCA_SERVER_ID_LNG	64
# define	GCA_MAXNAME		64

/*
** GCA Privileges
*/

# define	GCA_PRIV_SERVER_CONTROL	"SERVER_CONTROL"
# define	GCA_PRIV_NET_ADMIN	"NET_ADMIN"
# define	GCA_PRIV_MONITOR	"MONITOR"
# define	GCA_PRIV_TRUSTED	"TRUSTED"


/*
** GCA protocol levels.
**
** GCA_PROTOCOL_LEVEL_2:
**	"Original."
**
** GCA_PROTOCOL_LEVEL_3:
**	Generic errors.
**
** GCA_PROTOCOL_LEVEL_31:
**	Time zone patch.
**
** GCA_PROTOCOL_LEVEL_40:
**	GCA_ERROR allowed as response to GCA_NS_RESOLVE.
**	GCA_RESOLVED changed to support remote VNODE rollover.
**
** GCA_PROTOCOL_LEVEL_50:	
**	Name Server messages may travel over INGRES/NET.
**	New GCA1_C_INTO, GCA1_C_FROM with fixed GCA1_CPRDD.
**
** GCA_PROTOCOL_LEVEL_55:
**	New Name Server messages GCN_NS_AUTHORIZE and GCN_AUTHORIZED.
**
** GCA_PROTOCOL_LEVEL_60:
**	GCM protocol support (new messages defined in gcm.h).
**      New timezone message GCA_TIMEZONE_NAME. New GCN_NS_2_RESOLVE message.
**	GCA1_DELETE for delete owner.table cursor
**	GCA1_INVPROC for invoke owner.procedure
**      GCA_STRTRUNC for -string_truncation flag.
**
** GCA_PROTOCOL_LEVEL_61
**	GCA_PROMPT, GCA_PRREPLY messages.
**	GCA_PROMPT_DATA, GCA_PRREPLY_DATA data objects.
**	GCA_SL_TYPE, GCA_CAN_PROMPT association options.
**
** GCA_PROTOCOL_LEVEL_62
**	GCA_YEAR_CUTOFF
**	GCA1_FETCH message and GCA1_FT_DATA data object.
**
** GCA_PROTOCOL_LEVEL_63
**	Name Server uses end-of-group for turn-around rathe than end-of-data.
**
** GCA_PROTOCOL_LEVEL_64
**	New datatypes GCA_TYPE_NCHR, GCA_TYPE_VNCHR, GCA_TYPE_LNCHR.
**
** GCA_PROTOCOL_LEVEL_65
**	New integer size: eight-bytes.  GCA_MD_ASSOC parameter GCA_I8WIDTH.
**
** GCA_PROTOCOL_LEVEL_66
**	New datatypes for ANSI date, time, timestamp and interval.
**	GCA_XA_{START,END,PREPARE,COMMIT,ROLLBACK} messages and
**	GCA_XA_DATA data object.
**	Parameter flag GCA_IS_OUTPUT.
**
** GCA_PROTOCOL_LEVEL_67
**	New datatypes for Blob/Clob locators.
**	Query modifier GCA_LOCATOR_MASK.
**	Invoke Procedure modifier GCA_IP_LOCATOR_MASK.
**	GCA2_FETCH and GCA2_FT_DATA for scrollable cursors.
**	Tuple Descriptor flag GCA_SCROLL_MASK.
**	Response cursor status/position in gca_errd0/gca_errd1.
**	Added GCA_DATE_ALIAS connection parameter for date_alias.
**
** GCA_PROTOCOL_LEVEL_68
**	New boolean datatype GCA_TYPE_BOOL.
**	GCA2_INVPROC for unrestricted procedure names.
**
** N.B. Since no GCA client can presume to speak a new protocol without
**      actual coding changes, the existence of the GCA_PROTOCOL_LEVEL 
**	define itself is a mistake.  Clients should explicitly use one
**	of the above listed protocol levels, and GCA_PROTOCOL_LEVEL should
**	remain stuck at 40. 
*/	

# define                    GCA_PROTOCOL_LEVEL_2    	2
# define		    GCA_PROTOCOL_LEVEL_3	3
# define                    GCA_PROTOCOL_LEVEL_31   	31
# define		    GCA_PROTOCOL_LEVEL_40	40
# define		    GCA_PROTOCOL_LEVEL_50	50
# define		    GCA_PROTOCOL_LEVEL_55	55
# define		    GCA_PROTOCOL_LEVEL_60	60
# define		    GCA_PROTOCOL_LEVEL_61       61
# define		    GCA_PROTOCOL_LEVEL_62       62
# define		    GCA_PROTOCOL_LEVEL_63       63
# define		    GCA_PROTOCOL_LEVEL_64	64
# define		    GCA_PROTOCOL_LEVEL_65	65
# define		    GCA_PROTOCOL_LEVEL_66	66
# define		    GCA_PROTOCOL_LEVEL_67	67
# define		    GCA_PROTOCOL_LEVEL_68	68
# define		    GCA_PROTOCOL_LEVEL	    	40


/*
** GCA API levels:
**
** GCA_API_LEVEL_0:
**	Original.
**
** GCA_API_LEVEL_1:
**	GCA_LISTEN split into GCA_LISTEN, GCA_RQRESP.
**	GCA_REQUEST may run async.
**	GCA_REQUEST takes protocol level parameter.
**
** GCA_API_LEVEL_2:
**	Timeouts supported.
**	All async completions driven.
**	GCA_DISASSOC may run async.
**	GCA_RESUME flag for async GCA_REQUEST, GCA_DISCONN.
**
** GCA_API_LEVEL_3:
**	GCA_REQUEST takes a GCA_RQ_RSLVD, indicating that the 
**	    partner_name is a GCN_RESOLVED message. 
**      GCA_LISTEN sets flag GCA_LS_PASSWD to indicate that the client
**	    must validate the gca_user_id/gca_password pair (now obsolete).
**
** GCA_API_LEVEL_4:
**	GCA_RESUME modifier for async GCA_LISTEN; GCM support.
**	GCA_INTERPRET sets gca_end_of_group in GCA_IT_PARMS.
**      GCA_REQUEST takes in a GCA_RQ_GCM modifier flag to indicate
**	    that a management association is desired.
**      GCA_REQUEST takes in a GCA_RQ_REMPW modifier flag for 
**          password prompt support.
**      GCA_LISTEN returns flags GCA_LS_GCM, GCA_LS_FASTSELECT,
**	    GCA_LS_REMOTE, and GCA_LS_TRUSTED.
**      GCA_INITIATE takes in a GCA_AUTH_SPECD modifier flag to 
**          indicate that the gca_auth_user callback has been specified.
**	GCA_RECEIVE does GCA_INTERPRET's job, placing the relevant info
**	    in the GCA_RV_PARMS.
**
** GCA_API_LEVEL_5:
**	GCA_LISTEN, GCA_REQUEST, GCA_FASTSELECT parms have gca_sec_label.
**	GCA_LISTEN returns GCA_LS_SECLABEL, GCA_LS_SERVER flags.
**	GCA_LISTEN returns FASTSELECT data.
**	GCA_REQUEST/FASTSELECT take GCA_RQ_SECLABEL, GCA_RQ_SERVER modifiers.
**	GCA_SEND, GCA_RECEIVE parms have gca_modifiers flags.
**	GCA_REQUEST parms have gca_l_auth, gca_auth, and GCA_RQ_AUTH flag.
** GCA_API_LEVEL_6:
**	GCA_RESUME modifier for async GCA_FASTSELECT.
*/

# define	GCA_API_LEVEL_0		0
# define	GCA_API_LEVEL_1		1
# define	GCA_API_LEVEL_2		2
# define	GCA_API_LEVEL_3		3
# define	GCA_API_LEVEL_4		4
# define	GCA_API_LEVEL_5		5
# define	GCA_API_LEVEL_6		6



/*
** Name:
**	gca_call
**	IIGCa_call
**	IIGCa_cb_call
**
** Description:
**	Function entry points for GCA.  IIGCa_call() is the original
**	entry point.  A Control block interface, IIGCa_cb_call(), was 
**	added so that multiple GCA users in a single process could 
**	co-exist without interference.  IIGCa_call() uses a default 
**	control block for applications still using the original 
**	interface.
**
**	An alias for the new entry point, gca_call(), is also provided 
**	which conforms to standard Ingres facility entry point naming
**	conventions.
**
** Input:
**	gca_cb	    GCA control block.
**	service	    GCA service requested.
**	params	    Service parameters.
**	flags	    Request flags
**			GCA_ASYNC_FLAG	    Execute asynchronously
**			GCA_SYNC_FLAG	    Execute synchronously
**			GCA_NO_ASYNC_EXIT   Don't call async exit
**			GCA_RESUME	    Resuming prior GCA request
**			GCA_ALT_EXIT	    Alternate exit provided
**	async	    Async request exit params.
**
** Output:
**	status	    Request status
**
** Returns:
**	STATUS	    Error code
*/

typedef union  _GCA_PARMLIST	GCA_PARMLIST;

# define gca_call( gca_cb, service, params, flags, async, time_out, status ) \
    IIGCa_cb_call( gca_cb, service, params, flags, async, time_out, status  )

FUNC_EXTERN STATUS
IIGCa_cb_call
( 
    PTR			*gca_cb, 
    i4			service, 
    GCA_PARMLIST 	*params,
    i4			flags, 
    PTR			async, 
    i4			time_out, 
    STATUS 		*status
);

FUNC_EXTERN STATUS 	
IIGCa_call
( 
    i4			service, 
    GCA_PARMLIST	*params, 
    i4			flags, 
    PTR			async,
    i4			time_out,
    STATUS		*status
);


/*
** GCA Service flags
*/

# define	GCA_ASYNC_FLAG		(i4)0x0000
# define	GCA_SYNC_FLAG		(i4)0x0001
# define	GCA_NO_ASYNC_EXIT	(i4)0x0002
# define	GCA_RESUME		(i4)0x0004
# define	GCA_ALT_EXIT		(i4)0x0008


/*
**  Parameter list STATUS values
**
**  There are reserved ranges of STATUS values
**
**	0x0000 ... 0x00FF	GCA STATUS values
**	0x0100 ... 0x01FF	GCN STATUS values
**	0x1000 ... 0x1FFF	GCS STATUS values
**	0x2000 ... 0x2FFF	GCC STATUS values
**	
**  The possible returned STATUS values for each GCA service are
**  listed in the GCA specification document.
*/

# define	E_GCF_MASK			(12 * 0x10000)
# define	E_GC0000_OK      		(STATUS) (0x0000)
# define	E_GC0001_ASSOC_FAIL		(STATUS) (E_GCF_MASK + 0x0001)
# define	E_GC0002_INV_PARM		(STATUS) (E_GCF_MASK + 0x0002)
# define	E_GC0003_INV_SVC_CODE		(STATUS) (E_GCF_MASK + 0x0003)
# define	E_GC0004_INV_PLIST_PTR		(STATUS) (E_GCF_MASK + 0x0004)
# define	E_GC0005_INV_ASSOC_ID		(STATUS) (E_GCF_MASK + 0x0005)
# define	E_GC0006_DUP_INIT		(STATUS) (E_GCF_MASK + 0x0006)
# define	E_GC0007_NO_PREV_INIT		(STATUS) (E_GCF_MASK + 0x0007)
# define	E_GC0008_INV_MSG_TYPE		(STATUS) (E_GCF_MASK + 0x0008)
# define	E_GC0009_INV_BUF_ADDR		(STATUS) (E_GCF_MASK + 0x0009)
# define	E_GC000A_INT_PROT_LVL		(STATUS) (E_GCF_MASK + 0x000A)
# define	E_GC000B_RMT_LOGIN_FAIL		(STATUS) (E_GCF_MASK + 0x000B)
# define	E_GC000C_API_VERSION_INVALID 	(STATUS) (E_GCF_MASK + 0x000C)
# define	E_GC000D_ASSOCN_REFUSED		(STATUS) (E_GCF_MASK + 0x000D)
# define	E_GC000E_RMT_LOGIN_FAIL_INFO	(STATUS) (E_GCF_MASK + 0x000E)
# define	E_GC000F_NO_REGISTER		(STATUS) (E_GCF_MASK + 0x000F)
# define	E_GC0010_BUF_TOO_SMALL		(STATUS) (E_GCF_MASK + 0x0010)
# define	E_GC0011_INV_CONTENTS		(STATUS) (E_GCF_MASK + 0x0011)
# define	E_GC0012_LISTEN_FAIL		(STATUS) (E_GCF_MASK + 0x0012)
# define	E_GC0013_ASSFL_MEM		(STATUS) (E_GCF_MASK + 0x0013)
# define	E_GC0014_SAVE_FAIL		(STATUS) (E_GCF_MASK + 0x0014)
# define	E_GC0015_BAD_SAVE_NAME		(STATUS) (E_GCF_MASK + 0x0015)
# define	E_GC0016_RESTORE_FAIL		(STATUS) (E_GCF_MASK + 0x0016)
# define	E_GC0017_RSTR_OPEN		(STATUS) (E_GCF_MASK + 0x0017)
# define	E_GC0018_RSTR_READ		(STATUS) (E_GCF_MASK + 0x0018)
# define	E_GC0019_RSTR_CLOSE		(STATUS) (E_GCF_MASK + 0x0019)
# define	E_GC001A_DUP_REGISTER		(STATUS) (E_GCF_MASK + 0x001A)
# define	E_GC0020_TIME_OUT		(STATUS) (E_GCF_MASK + 0x0020)
# define	E_GC0021_NO_PARTNER		(STATUS) (E_GCF_MASK + 0x0021)
# define	E_GC0022_NOT_IACK		(STATUS) (E_GCF_MASK + 0x0022)
# define	E_GC0023_ASSOC_RLSED		(STATUS) (E_GCF_MASK + 0x0023)
# define	E_GC0024_DUP_REQUEST		(STATUS) (E_GCF_MASK + 0x0024)
# define	E_GC0025_NM_SRVR_ID_ERR		(STATUS) (E_GCF_MASK + 0x0025)
# define	E_GC0026_NM_SRVR_ERR		(STATUS) (E_GCF_MASK + 0x0026)
# define	E_GC0027_RQST_PURGED		(STATUS) (E_GCF_MASK + 0x0027)
# define	E_GC0028_SRVR_RESOURCE		(STATUS) (E_GCF_MASK + 0x0028)
# define	E_GC0029_RQST_FAIL		(STATUS) (E_GCF_MASK + 0x0029)
# define	E_GC002A_RQST_RESOURCE		(STATUS) (E_GCF_MASK + 0x002A)
# define	E_GC002B_SND1_FAIL		(STATUS) (E_GCF_MASK + 0x002B)
# define	E_GC002C_SND2_FAIL		(STATUS) (E_GCF_MASK + 0x002C)
# define	E_GC002D_SND3_FAIL		(STATUS) (E_GCF_MASK + 0x002D)
# define	E_GC002E_RCV1_FAIL		(STATUS) (E_GCF_MASK + 0x002E)
# define	E_GC002F_RCV2_FAIL		(STATUS) (E_GCF_MASK + 0x002F)
# define	E_GC0030_RCV3_FAIL		(STATUS) (E_GCF_MASK + 0x0030)
# define    	E_GC0031_USERPWD_FAIL		(STATUS) (E_GCF_MASK + 0x0031)
# define	E_GC0032_NO_PEER		(STATUS) (E_GCF_MASK + 0x0032)
# define	E_GC0033_GCM_NOSUPP		(STATUS) (E_GCF_MASK + 0x0033)
# define	E_GC0034_GCM_PROTERR		(STATUS) (E_GCF_MASK + 0x0034)
# define	E_GC0035_GCM_INVLVL		(STATUS) (E_GCF_MASK + 0x0035)
# define	E_GC0036_GCM_NOEOD		(STATUS) (E_GCF_MASK + 0x0036)
# define	E_GC0037_GCM_INVTYPE		(STATUS) (E_GCF_MASK + 0x0037)
# define	E_GC0038_GCM_INVCOUNT		(STATUS) (E_GCF_MASK + 0x0038)
# define	E_GC0039_GCM_NOTRAP		(STATUS) (E_GCF_MASK + 0x0039)
# define	E_GC003A_GCM_INVMON		(STATUS) (E_GCF_MASK + 0x003A)
# define	E_GC003B_GCM_INVTRAP		(STATUS) (E_GCF_MASK + 0x003B)
# define	E_GC003C_PWPROMPT_NOTSUP	(STATUS) (E_GCF_MASK + 0x003C)
# define        E_GC003D_BAD_PMFILE             (STATUS) (E_GCF_MASK + 0x003D)
# define        E_GC003E_PMLOAD_ERR             (STATUS) (E_GCF_MASK + 0x003E)
# define        E_GC003F_PM_NOPERM              (STATUS) (E_GCF_MASK + 0x003F)
# define        E_GC0040_CS_OK              	(STATUS) (E_GCF_MASK + 0x0040)
# define	E_GC0041_INVALID_SEC_LABEL	(STATUS) (E_GCF_MASK + 0x0041)
# define	E_GC0042_AGENT_MISSING_REMOTE	(STATUS) (E_GCF_MASK + 0x0042)
# define	E_GC0043_AGENT_PARTNER_NOTSUP	(STATUS) (E_GCF_MASK + 0x0043)
# define	E_GC0050_PDD_ERROR		(STATUS) (E_GCF_MASK + 0x0050)
# define	E_GC0051_PDD_BADSIZE		(STATUS) (E_GCF_MASK + 0x0051)
# define	E_GC0055_PDE_ERROR		(STATUS) (E_GCF_MASK + 0x0055)
# define	E_GC0056_PDE_BADSIZE		(STATUS) (E_GCF_MASK + 0x0056)
# define        E_GC0057_RMTACCESS_FAIL         (STATUS) (E_GCF_MASK + 0x0057)
# define	E_GC0058_NOTMOUT_DLOCK          (STATUS) (E_GCF_MASK + 0x0058)

# define	E_GCFFFE_INCOMPLETE     	(STATUS) (E_GCF_MASK + 0xFFFE)
# define	E_GCFFFF_IN_PROCESS     	(STATUS) (E_GCF_MASK + 0xFFFF)


/* 
** GCA Service Codes and Parameter List structures.
**
** Each GCA service has a service code and an associated parameter list.
** The service code values and parameter list structures are as follows.
**
**	GCA_INITIATE:	Initialize GCA communication
**	GCA_TERMINATE:	End GCA Communication
**	GCA_REGISTER:	Register as a server for GCA communication
**	GCA_LISTEN:	Listen for Association Request
**	GCA_RQRESP:	Respond to a request for association
**	GCA_REQUEST:	Request Association
**	GCA_DISASSOC:	Clean up a released association
**	GCA_FORMAT:	Format a message buffer
**	GCA_SEND:	Send a Previously Initialized Buffer
**	GCA_INTERPRET:	Interpret a Received Message Buffer
**	GCA_RECEIVE:	Fill a Message Buffer
**	GCA_FASTSELECT:	"Connectionless" exchange of management data
**	GCA_ATTRIBS:	Obtain association attributes
**	GCA_SAVE:	Prepare for task switching
**	GCA_RESTORE:	Complete Task Switching 
**	GCA_REG_TRAP:	Register callback function for GCM trap indications
**	GCA_UNREG_TRAP:	Unregister trap callback function
**
** Note: Service code 0x000D (GCA_DCONN) is no longer used.
*/          

# define	GCA_INITIATE	(i4)0x0001	/* GCA_IN_PARMS */
# define	GCA_TERMINATE	(i4)0x0002	/* GCA_TE_PARMS */
# define	GCA_REQUEST	(i4)0x0003	/* GCA_RQ_PARMS */
# define	GCA_LISTEN	(i4)0x0004	/* GCA_LS_PARMS */
# define	GCA_SAVE	(i4)0x0005	/* GCA_SV_PARMS */
# define	GCA_RESTORE	(i4)0x0006	/* GCA_RS_PARMS */
# define	GCA_FORMAT	(i4)0x0007	/* GCA_FO_PARMS */
# define	GCA_SEND	(i4)0x0008	/* GCA_SD_PARMS */
# define	GCA_RECEIVE	(i4)0x0009	/* GCA_RV_PARMS */
# define	GCA_INTERPRET	(i4)0x000A	/* GCA_IT_PARMS */
# define	GCA_REGISTER	(i4)0x000B	/* GCA_RG_PARMS */
# define	GCA_DISASSOC	(i4)0x000C	/* GCA_DA_PARMS */
# define	GCA_ATTRIBS	(i4)0x000E	/* GCA_AT_PARMS */
# define	GCA_RQRESP	(i4)0x000F	/* GCA_RQ_PARMS */
# define	GCA_FASTSELECT	(i4)0x0010	/* GCA_FS_PARMS */
# define	GCA_REG_TRAP	(i4)0x0011	/* GCA_RT_PARMS */
# define	GCA_UNREG_TRAP	(i4)0x0012	/* GCA_UT_PARMS */


/*
** Name: GCA_AT_PARMS
**
** Service: GCA_ATTRIBS
*/
typedef struct _GCA_AT_PARMS GCA_AT_PARMS;

struct _GCA_AT_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4		gca_assoc_id;

    u_i4	gca_flags;

# define	GCA_HET		0x0001	/* Association is heterogeneous */
};


/*
** Name: GCA_DA_PARMS
**
** Service: GCA_DISASSOC
*/
typedef struct _GCA_DA_PARMS GCA_DA_PARMS;

struct _GCA_DA_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4      	gca_assoc_id;
};


/*
** Name: GCA_FO_PARMS
**
** Service: GCA_FORMAT
*/
typedef struct _GCA_FO_PARMS GCA_FO_PARMS;

struct _GCA_FO_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;

    char    	*gca_buffer;
    i4		gca_b_length;
    char    	*gca_data_area;
    i4		gca_d_length;
};


/*
** Name: GCA_FS_PARMS
**
** Service: GCA_FASTSELECT
**
** Note: 
**	The first part of this structure must match
**	the GCA_RQ_PARMS structure.  GCA_REQUEST and
**	GCA_FASTSELECT share common actions in the
**	GCA state machine and all GCA_REQUEST info
**	must be identically available for GCA_FASTSELECT.
*/
typedef struct _GCA_FS_PARMS GCA_FS_PARMS;

struct _GCA_FS_PARMS
{

/*
** GCA_REQUEST parameters
*/

    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4      	gca_assoc_id;

    u_i4	gca_modifiers;
    i4		gca_size_advise;
    i4		gca_peer_protocol;
    u_i4	gca_flags;

    char    	*gca_partner_name;
    char    	*gca_user_name;
    char    	*gca_password;
    char    	*gca_rem_username;
    char    	*gca_rem_password;
    PTR		gca_sec_label;
    i4		gca_l_auth;
    PTR		gca_auth;

/* 
** GCA_FASTSELECT parameters
*/
    char    	*gca_buffer;
    i4		gca_b_length;
    i4      	gca_message_type;
    i4		gca_msg_length;	

# define	GCA_FS_MAX_DATA		(i4)4096  /* max len of FS message */

    /* Obsolete */

    char    	*gca_account_name;
};


/*
** Name: GCA_IN_PARMS
**
** Service: GCA_INITIATE
*/
typedef struct _GCA_IN_PARMS GCA_IN_PARMS;

struct _GCA_IN_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;

    u_i4	gca_modifiers;

# define	GCA_SYN_SVC		0x0001	/* Obsolete */
# define	GCA_ASY_SVC		0x0002	/* Obsolete */
# define	GCA_API_VERSION_SPECD	0x0004	/* API version specified */
# define	GCA_AUTH_DELEGATE	0x0008	/* Delegate user auths */
# define	GCA_GCM_READ_NONE	0x0010	/* GCM_GET not permitted */
# define	GCA_GCM_READ_ANY	0x0020	/* Anyone may do GCM_GET */
# define	GCA_GCM_WRITE_NONE	0x0040	/* GCM_SET not permitted */
# define	GCA_GCM_WRITE_ANY	0x0080	/* Anyone may do GCM_SET */

    i4		gca_local_protocol;
    i4		gca_header_length;
    i4		gca_api_version;

    STATUS  	(*gca_alloc_mgr)();
    STATUS  	(*gca_dealloc_mgr)();

    VOID    	(*gca_expedited_completion_exit)();
    VOID    	(*gca_normal_completion_exit)();

    /* Deprecated */

    bool    	(*gca_lsncnfrm)();

    /* Obsolete */

    STATUS  	(*gca_auth_user)();
    STATUS  	(*gca_p_semaphore)();
    STATUS  	(*gca_v_semaphore)();
    STATUS  	(*gca_decompose)();
    PTR	    	gca_cb_decompose;
};


/*
** Name: GCA_IT_PARMS
**
** Service: GCA_INTERPRET
*/
typedef struct _GCA_IT_PARMS GCA_IT_PARMS;

struct _GCA_IT_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;

    char    	*gca_buffer;

    i4		gca_message_type;
    i4		gca_d_length;
    char    	*gca_data_area;
    bool     	gca_end_of_data;
    bool     	gca_end_of_group;		
};


/*
** Name: GCA_LS_PARMS
**
** Service: GCA_LISTEN
*/
typedef struct _GCA_LS_PARMS GCA_LS_PARMS;

struct _GCA_LS_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4      	gca_assoc_id;

    i4		gca_size_advise;
    i4		gca_partner_protocol;
    u_i4	gca_flags;

# define	GCA_DESCR_RQD		0x0001	/* Descriptor required */
# define	GCA_LS_TRUSTED		0x0002	/* Trusted GCA user */
# define	GCA_LS_GCM		0x0004	/* GCM connection */
# define	GCA_LS_FASTSELECT	0x0008	/* FASTSELECT connection */
# define	GCA_LS_REMOTE		0x0010	/* Remote connection desired */
# define	GCA_LS_SECLABEL		0x0020	/* Security label provided */
# define	GCA_LS_SERVER		0x0040	/* deprecated */
# define	GCA_LS_EMBEDGCC		0x0080	/* Client uses embedded GCC */

    i4	    	gca_l_aux_data;
    PTR	    	gca_aux_data;

    i4		gca_l_fs_data;	
    PTR		gca_fs_data;
    i4		gca_message_type;

    char    	*gca_user_name;
    char    	*gca_account_name;
    char    	*gca_access_point_identifier;
    PTR		gca_sec_label;

    /* Obsolete */

    char    	*gca_application_id;
    char    	*gca_db_name;
    char    	*gca_password;
};


/*
** Name: GCA_RG_PARMS
**
** Service: GCA_REGISTER
*/
typedef struct _GCA_RG_PARMS GCA_RG_PARMS;

struct _GCA_RG_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4      	gca_assoc_id;

    u_i4	gca_modifiers;

# define	GCA_RG_NO_NS		0x0001		/* No NS register */
# define	GCA_RG_SOLE_SVR		0x0002		/* Sole server */
# define	GCA_RG_NS_ONLY		0x0004		/* NS only register */

# define	GCA_RG_MIB_MASK		0x7fff0000	/* MIB support mask */
# define	GCA_RG_IINMSVR		0x40000000	/* Name Server MIB */
# define	GCA_RG_COMSVR		0x20000000	/* Comm Server MIB */
# define	GCA_RG_INGRES		0x10000000	/* Ingres DBMS MIB */
# define	GCA_RG_IIRCP		0x08000000	/* RCP MIB */
# define	GCA_RG_IIACP		0x04000000	/* ACP MIB */
# define	GCA_RG_INSTALL		0x02000000	/* Install-Wide MIB */
# define	GCA_RG_TRAP		0x01000000	/* Traps Supported */
# define	GCA_RG_BRIDGESVR	0x00200000	/* Bridge Server MIB */
# define	GCA_RG_NMSVR		0x00100000      /* Additional NS MIB */

    i4      	gca_l_so_vector;
    char    	**gca_served_object_vector;

    /*
    ** GCA does not restrict the identifiers that may
    ** be used for server classes (there are a few
    ** classes which must be 'known' by the Name Server,
    ** see gcn.h).  A registry of used and potential
    ** server classes is maintained separately.
    */
#   include	<gcaclass.h>
    char	*gca_srvr_class;

    char    	*gca_listen_id;

    /* Deprecated */

    i4	    	gca_no_connected;
    i4	    	gca_no_active;
    char    	*gca_installn_id;

    /* Obsolete */

    char    	*gca_process_id;
};


/*
** Name: GCA_RQ_PARMS
**
** Service: GCA_REQUEST
*/
typedef struct _GCA_RQ_PARMS GCA_RQ_PARMS;

struct _GCA_RQ_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4	    	gca_assoc_id;

    u_i4	gca_modifiers;

# define	GCA_NO_XLATE		0x0001	/* Partner resolved */
# define	GCA_RQ_DESCR		0x0002	/* Set descriptor required */
# define	GCA_RQ_RSLVD		0x0004	/* Partner GCN_RESOLVED */
# define	GCA_RQ_GCM		0x0008	/* GCM association requested */
# define	GCA_RQ_REMPW		0x0010	/* Use remote user, pwd */
# define	GCA_RQ_SECLABEL		0x0020	/* Security label provided */
# define	GCA_RQ_SERVER		0x0040	/* deprecated */
# define	GCA_RQ_AUTH		0x0080	/* Remote Auth provided */

# define	GCA_CS_SHUTDOWN		0x8000	/* Rqst for svr shutdownn */
# define	GCA_CS_QUIESCE		0x4000	/* Rqst for svr quiesce */
# define	GCA_CS_CMD_SESSN	0x2000	/* Rqst for command sessn */
# define	GCA_CS_BEDCHECK		0x1000	/* Probing request */

    i4		gca_size_advise;
    i4		gca_peer_protocol;
    u_i4	gca_flags;

	     /*	GCA_DESCR_RQD		0x0001 */

    char    	*gca_partner_name;
    char    	*gca_user_name;
    char    	*gca_password;
    char    	*gca_rem_username;
    char    	*gca_rem_password;
    PTR		gca_sec_label;
    i4		gca_l_auth;
    PTR		gca_auth;

    /* Obsolete */

    char    	*gca_account_name;
};


/*
** Name: GCA_RR_PARMS
**
** Service: GCA_RQRESP
*/
typedef struct _GCA_RR_PARMS GCA_RR_PARMS;

struct _GCA_RR_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4      	gca_assoc_id;

    u_i4	gca_modifiers;

	     /*	GCA_RQ_DESCR		0x0002	   Set descriptor required */

    i4		gca_local_protocol;
    STATUS  	gca_request_status;
};


/*
** Name: GCA_RS_PARMS
**
** Service: GCA_RESTORE
*/
typedef struct _GCA_RS_PARMS GCA_RS_PARMS;

struct _GCA_RS_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;

    char   	*gca_save_name;
    i4		gca_length_user_data;
    PTR     	gca_ptr_user_data;
};


/*
** Name: GCA_RT_PARMS
**
** Service: GCA_REG_TRAP
*/
typedef struct _GCA_RT_PARMS GCA_RT_PARMS;

struct _GCA_RT_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;

    i4      	gca_trap_handle;
    VOID    	(*gca_trap_callback)();
};


/*
** Name: GCA_RV_PARMS
**
** Service: GCA_RECEIVE
*/
typedef struct _GCA_RV_PARMS GCA_RV_PARMS;

struct _GCA_RV_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4      	gca_assoc_id;

    u_i4	gca_modifiers;

	     /*	GCA_FORMATTED	0x0001	   GCD structured buffer */

    i4		gca_flow_type_indicator;

# define	GCA_NORMAL	0x0000
# define	GCA_EXPEDITED	0x0001

    i4		gca_b_length;
    char    	*gca_buffer;

    i4	    	gca_message_type;
    i4		gca_d_length;
    char    	*gca_data_area;
    bool	gca_end_of_data;
    bool	gca_end_of_group;

    /* Obsolete */

    PTR	    	gca_descriptor;
};


/*
** Name: GCA_SD_PARMS
**
** Service: GCA_SEND
*/
typedef struct _GCA_SD_PARMS GCA_SD_PARMS;

struct _GCA_SD_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4      	gca_assoc_id;

    u_i4	gca_modifiers;

# define	GCA_FORMATTED	0x0001	/* GCD structured buffer */
# define	GCA_EOG		0x0002	/* Force end-of-group */
# define	GCA_NOT_EOG	0x0004	/* Force ! end-of-group */

    char    	*gca_buffer;
    i4		gca_message_type;
    i4		gca_msg_length;
    bool     	gca_end_of_data;

    PTR	    	gca_descriptor;
};


/*
** Name: GCA_SV_PARMS
**
** Service: GCA_SAVE
*/
typedef struct _GCA_SV_PARMS GCA_SV_PARMS;

struct _GCA_SV_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4		gca_assoc_id;

    char   	*gca_save_name;
    i4		gca_length_user_data;
    PTR     	gca_ptr_user_data;
};


/*
** Name: GCA_TE_PARMS
**
** Service: GCA_TERMINATE
*/
typedef struct _GCA_TE_PARMS GCA_TE_PARMS;

struct _GCA_TE_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
};


/*
** Name: GCA_UT_PARMS
**
** Service: GCA_UNREG_TRAP
*/
typedef struct _GCA_UT_PARMS GCA_UT_PARMS;

struct _GCA_UT_PARMS
{
    i4		gca_service;
    STATUS  	gca_status;
    CL_SYS_ERR 	gca_os_status;

    i4      	gca_trap_handle;
};


/*
** Name: GCA_AUX_DATA
**
** Description:
**      Information to be surfaced as a returned parameter at the 
**	GCA interface as part of a GCA_LISTEN service.
*/
typedef struct _GCA_AUX_DATA GCA_AUX_DATA;

struct _GCA_AUX_DATA
{
    i4		len_aux_data;
    i4		type_aux_data;

# define	GCA_ID_RMT_ADDR		1   /* Old style remote conn info */ 
# define	GCA_ID_CMDSESS		2   /* Start a local command session */
# define	GCA_ID_SHUTDOWN		3   /* Shutdown now */
# define	GCA_ID_QUIESCE		4   /* Quiesce, then shutdown */
# define	GCA_ID_BEDCHECK		5   /* Respond and disconnect */
# define	GCA_ID_AUTH		6   /* GCS authentication */
# define	GCA_ID_SRV_KEY		7   /* GCS server key */
# define	GCA_ID_RMT_INFO		8   /* New remote connection info */
# define	GCA_ID_SECLAB		9   /* Security label */
# define	GCA_ID_DELEGATED	10  /* Delegated authentication */
};


/*
** Name: GCA_ALL_PARMS
**
** Description: 
**	Parameters common to all operations
*/
typedef struct _GCA_ALL_PARMS GCA_ALL_PARMS;

struct _GCA_ALL_PARMS
{
    i4		gca_service;
    STATUS	gca_status;
    CL_SYS_ERR	gca_os_status;
    VOID    	(*gca_completion)();
    PTR	    	gca_closure;
    i4		gca_assoc_id;

/*
** Earlier versions used both gca_assoc_id and
** gca_association_id.  The following allows
** backward compatibility while standardizing
** on gca_assoc_id.
*/
# define	gca_association_id	gca_assoc_id 
};


/*
** Name: GCA_PARMLIST - Union of all parameter lists
**
** Description:
**      A parameter list is a data structure containing the parameters
**      required by the requested GCA service.  All GCA services are 
**	accessed via the "gca_call" function call.  Each GCA service 
**	has a service code and an associated parameter list.
*/
union _GCA_PARMLIST
{                                                                            
     GCA_ALL_PARMS	gca_all_parms;	/* common subparms	*/
     GCA_AT_PARMS	gca_at_parms;	/* GCA_ATTRIBS		*/
     GCA_DA_PARMS	gca_da_parms;	/* GCA_DISASSOC		*/
     GCA_FO_PARMS	gca_fo_parms;	/* GCA_FORMAT		*/
     GCA_FS_PARMS	gca_fs_parms;	/* GCA_FASTSELECT	*/
     GCA_IN_PARMS	gca_in_parms;	/* GCA_INITIATE		*/
     GCA_IT_PARMS	gca_it_parms;	/* GCA_INTERPRET	*/
     GCA_LS_PARMS	gca_ls_parms;	/* GCA_LISTEN		*/
     GCA_RG_PARMS	gca_rg_parms;	/* GCA_REGISTER		*/
     GCA_RQ_PARMS	gca_rq_parms;	/* GCA_REQUEST		*/
     GCA_RR_PARMS	gca_rr_parms;	/* GCA_RQRESP 		*/
     GCA_RS_PARMS	gca_rs_parms;	/* GCA_RESTORE		*/
     GCA_RT_PARMS	gca_rt_parms;	/* GCA_REG_TRAP		*/
     GCA_RV_PARMS	gca_rv_parms;	/* GCA_RECEIVE		*/
     GCA_SD_PARMS	gca_sd_parms;	/* GCA_SEND		*/
     GCA_SV_PARMS	gca_sv_parms;	/* GCA_SAVE		*/
     GCA_TE_PARMS	gca_te_parms;	/* GCA_TERMINATE	*/
     GCA_UT_PARMS	gca_ut_parms;	/* GCA_UNREG_TRAP	*/
};



/*
** GCA message types
**
** The essential service provided by GCA is the transfer of messages
** between cooperating application programs (GCA users). A message 
** is characterized by a message type designation and (usually) a 
** data object. The collection of GCA message types and data objects 
** is known by and is semantically meaningful to GCA users. The GCA 
** service interface provides the mechanism for transferring message 
** type indications and message data across the GCA/user interface.
**
** There are reserved ranges of message type numbers
**
**	0x00 ... 0x01	Internal
**	0x02 ... 0x3F	GCA messages
**	0x40 ... 0x4F	GCN messages
**	0x50 ... 0x5F	GCC messages
**	0x60 ... 0x6F	GCM messages
**	0x70 ... 0xFF	Unassigned
**
** The following list defines the GCA message types and associated 
** data objects.  Only GCA messages are of concern to the GCA user.
**
**	GCA_ABORT	Abort transaction
**	GCA_ACCEPT	Accept association/modification
**	GCA_ACK		Acknowledge
**	GCA_ATTENTION	Interrupt
**	GCA_C_FROM	Copy from map
**	GCA1_C_FROM	Copy from map (GCA_PROTOCOL_LEVEL_50)
**	GCA_C_INTO	Copy into map
**	GCA1_C_INTO	Copy into map (GCA_PROTOCOL_LEVEL_50)
**	GCA_CDATA	Copy data
**	GCA_CLOSE	Cursor close
**	GCA_COMMIT	Commit transaction
**	GCA_CREPROC	Create database procedure
**	GCA_DEFINE	Define repeat query
**	GCA_DELETE	Cursor delete
**	GCA1_DELETE	Cursor delete (GCA_PROTOCOL_LEVEL_60)
**	GCA_DONE	Transaction secured/committed
**	GCA_DRPPROC	Drop database procedure
**	GCA_ERROR	Error/User message
**	GCA_EVENT	Event notification
**	GCA_FETCH	Cursor fetch
**	GCA1_FETCH	Cursor fetch (GCA_PROTCOL_LEVEL_62)
**	GCA2_FETCH	Scrollable cursor fetch (GCA_PROTCOL_LEVEL_67)
**	GCA_IACK	Interrupt acknowledge
**	GCA_INVOKE	Invoke repeat query
**	GCA_INVPROC	Invoke database procedure
**	GCA1_INVPROC	Invoke database procedure (GCA_PROTOCOL_LEVEL_60)
**	GCA2_INVPROC	Invoke database procedure (GCA_PROTOCOL_LEVEL_68)
**	GCA_MD_ASSOC	Modify association
**	GCA_NP_INTERRUPT Interrupt (non-purging)
**	GCA_PROMPT	Prompt user
**	GCA_PRREPLY	Prompt reply
**	GCA_Q_BTRAN	Begin transaction
**	GCA_Q_ETRAN	End transaction
**	GCA_Q_STATUS	Request status
**	GCA_QC_ID	Query/Cursor ID
**	GCA_QUERY	Execute query
**	GCA_REFUSE	Transaction commit refused
**	GCA_REJECT	Reject association/modification
**	GCA_RELEASE	Terminate association
**	GCA_RESPONSE	Query results
**	GCA_RETPROC	Database procedure results
**	GCA_ROLLBACK	Rollback transaction
**	GCA_S_BTRAN	Transaction begin results
**	GCA_S_ETRAN	Transaction end results
**	GCA_SECURE	Secure transaction
**	GCA_TDESCR	Tuple descriptor
**	GCA_TRACE	Trace message
**	GCA_TUPLES	Tuple data
**	GCA_XA_COMMIT	Commit XA transaction
**	GCA_XA_END	End association with XA transaction
**	GCA_XA_PREPARE	Prepare XA transaction
**	GCA_XA_ROLLBACK	Rollback XA transaction
**	GCA_XA_SECURE	Secure XA transaction
**	GCA_XA_START	Begin association with XA transaction
*/

# define	GCA_MD_ASSOC		0x02	/* GCA_SESSION_PARMS */
# define	GCA_ACCEPT		0x03	/* */
# define	GCA_REJECT		0x04	/* GCA_ER_DATA */
# define	GCA_RELEASE		0x05	/* GCA_ER_DATA */
# define	GCA_Q_BTRAN		0x06	/* GCA_TX_DATA */
# define	GCA_S_BTRAN		0x07	/* GCA_RE_DATA */
# define	GCA_ABORT		0x08	/* */
# define	GCA_SECURE		0x09	/* GCA_TX_DATA */
# define	GCA_DONE		0x0A	/* GCA_RE_DATA */
# define	GCA_REFUSE		0x0B	/* GCA_RE_DATA */
# define	GCA_COMMIT		0x0C	/* */
# define	GCA_QUERY		0x0D	/* GCA_Q_DATA */
# define	GCA_DEFINE		0x0E	/* GCA_Q_DATA */
# define	GCA_INVOKE		0x0F	/* GCA_IV_DATA */
# define	GCA_FETCH		0x10	/* GCA_ID_DATA */
# define	GCA_DELETE		0x11	/* GCA_DL_DATA */
# define	GCA_CLOSE		0x12	/* GCA_ID_DATA */
# define	GCA_ATTENTION		0x13	/* GCA_AT_DATA */
# define	GCA_QC_ID		0x14	/* GCA_ID_DATA */
# define	GCA_TDESCR		0x15	/* GCA_TD_DATA */
# define	GCA_TUPLES		0x16	/* GCA_TU_DATA */
# define	GCA_C_INTO		0x17	/* GCA_CP_MAP */
# define	GCA_C_FROM		0x18	/* GCA_CP_MAP */
# define	GCA_CDATA		0x19	/* GCA_TU_DATA */
# define	GCA_ACK			0x1A	/* GCA_AK_DATA */
# define	GCA_RESPONSE		0x1B	/* GCA_RE_DATA */
# define	GCA_ERROR		0x1C	/* GCA_ER_DATA */
# define	GCA_TRACE		0x1D	/* GCA_TR_DATA */
# define	GCA_Q_ETRAN		0x1E	/* */
# define	GCA_S_ETRAN		0x1F	/* GCA_RE_DATA */
# define	GCA_IACK		0x20	/* GCA_AK_DATA */
# define	GCA_NP_INTERRUPT	0x21	/* GCA_AT_DATA */
# define	GCA_ROLLBACK		0x22	/* */
# define	GCA_Q_STATUS		0x23	/* */
# define	GCA_CREPROC		0x24	/* GCA_Q_DATA */
# define	GCA_DRPPROC		0x25	/* GCA_Q_DATA */
# define	GCA_INVPROC		0x26	/* GCA_IP_DATA */
# define	GCA_RETPROC		0x27	/* GCA_RP_DATA */
# define	GCA_EVENT		0x28	/* GCA_EV_DATA */
# define	GCA1_C_INTO		0x29	/* GCA1_CP_MAP */
# define	GCA1_C_FROM		0x2A	/* GCA1_CP_MAP */
# define	GCA1_DELETE		0x2B	/* GCA1_DL_DATA */
# define	GCA_XA_SECURE		0x2C	/* GCA_XA_TX_DATA */
# define	GCA1_INVPROC		0x2D	/* GCA1_IP_DATA */
# define	GCA_PROMPT		0x2E	/* GCA_PROMPT_DATA */
# define	GCA_PRREPLY		0x2F	/* GCA_PRREPLY_DATA */
# define	GCA1_FETCH		0x30	/* GCA1_FT_DATA */
# define	GCA2_FETCH		0x31	/* GCA2_FT_DATA */
# define	GCA2_INVPROC		0x32	/* GCA2_IP_DATA */

# define	GCA_MAXMSGTYPE		0x32

/*
** Extended messages.
*/

# define	GCA_XA_START		0x5A	/* GCA_XA_DATA */
# define	GCA_XA_END		0x5B	/* GCA_XA_DATA */
# define	GCA_XA_PREPARE		0x5C	/* GCA_XA_DATA */
# define	GCA_XA_COMMIT		0x5D	/* GCA_XA_DATA */
# define	GCA_XA_ROLLBACK		0x5E	/* GCA_XA_DATA */


/*
**  GCA data objects
**
**  GCA is aware of and deals with a specific set of complex data objects
**  which pass across the GCA service interface as data associated with
**  various message types.  This is the set of objects which are meaningful
**  to the application users of GCA.  Following is the complete set.
*/


/*  GCA_NAME: Generalized name element */
typedef struct _GCA_NAME GCA_NAME;

struct _GCA_NAME
{
    i4		gca_l_name;	    /* Length of following array */
    char	gca_name[1];	    /* Variable length */
};


/*  GCA_ID: DBMS object ID */
typedef struct _GCA_ID GCA_ID;

struct _GCA_ID
{
    i4		gca_index[2];
    char	gca_name[ GCA_MAXNAME ];
};


/*  GCA_DATA_VALUE: Generalized Data Representation  */
typedef struct _GCA_DATA_VALUE GCA_DATA_VALUE;

struct _GCA_DATA_VALUE
{
    i4		gca_type;	/* GCA Datatype */

    /*
    ** Precision and scale are combined in the low order
    ** two bytes: precision in high byte, scale in low.
    */
    i4		gca_precision;
    i4		gca_l_value;	/* Length of following */
    char	gca_value[1];	/* Variable length (see GCA Datatypes) */
};


/*  
** GCA_DBMS_VALUE: DBMS Data Representation
**
** This structure corresponds to the DB_DATA_VALUE
** structure defined by the DBMS which, for some 
** unknown reason, was originally used in the GCA 
** column attribute structure.  Since attributes 
** are not accompanied by data, the data pointer 
** (here defined as a 4 byte integer) should be
** ignored (set to NULL).
*/
typedef struct _GCA_DBMS_VALUE GCA_DBMS_VALUE;

struct _GCA_DBMS_VALUE
{
    i4		db_data;		/* --- unused --- */
    i4		db_length;		/* Length of data in bytes */
    i2		db_datatype;		/* GCA Data Type */

    /*
    ** Hi byte: precision
    ** Low byte: scale
    */
    i2		db_prec;
};


/*  GCA_COL_ATT: Column Attribute  */
typedef struct _GCA_COL_ATT GCA_COL_ATT;

struct _GCA_COL_ATT
{
    GCA_DBMS_VALUE  gca_attdbv;
    i4		    gca_l_attname;	/* Length of following array */
    char	    gca_attname[1];	/* Variable length */
};


/*  GCA_CPRDD: Copy Row Domain Descriptor  */
typedef struct _GCA_CPRDD GCA_CPRDD;

struct _GCA_CPRDD
{
    char	gca_domname_cp[ GCA_MAXNAME ];
    i4		gca_type_cp;
    i4		gca_length_cp;
    i4		gca_user_delim_cp;
    i4		gca_l_delim_cp;
    char	gca_delim_cp[ 2 ];
    i4		gca_tupmap_cp;
    i4		gca_cvid_cp;
    i4		gca_cvlen_cp;
    i4		gca_withnull_cp;
    i4		gca_nullen_cp;
};	


/*  GCA1_CPRDD: Copy Row Domain Descriptor (GCA_PROTOCOL_LEVEL_50)  */
typedef struct _GCA1_CPRDD GCA1_CPRDD;

struct _GCA1_CPRDD
{
    char		gca_domname_cp[ GCA_MAXNAME ];
    i4			gca_type_cp;
    i4			gca_precision_cp;
    i4			gca_length_cp;
    i4			gca_user_delim_cp;
    i4			gca_l_delim_cp;
    char		gca_delim_cp[ 2 ];
    i4			gca_tupmap_cp;
    i4			gca_cvid_cp;
    i4			gca_cvprec_cp;
    i4			gca_cvlen_cp;
    i4			gca_withnull_cp;
    i4			gca_l_nullvalue_cp;	/* Length of following array */
    GCA_DATA_VALUE	gca_nullvalue_cp[1];	/* Variable length */
};	


/*  GCA_E_ELEMENT: Error data element  */
typedef struct _GCA_E_ELEMENT GCA_E_ELEMENT;

struct _GCA_E_ELEMENT
{                                                         
    i4			gca_id_error;
    i4			gca_id_server;  
    i4			gca_server_type;
    i4			gca_severity;

# define	GCA_ERDEFAULT	0x0000	/* This is an error: default	*/
# define	GCA_ERMESSAGE	0x0001	/* This is a user message	*/
# define	GCA_ERWARNING	0x0002	/* This is a warning message	*/
# define	GCA_ERFORMATTED	0x0010	/* Text already formatted by GW	*/

    i4			gca_local_error;
    i4			gca_l_error_parm;	/* Length of following array */
    GCA_DATA_VALUE	gca_error_parm[1];	/* Variable length */
};


/*  GCA_P_PARAM: Database procedure parameter */
typedef struct _GCA_P_PARAM GCA_P_PARAM;

struct _GCA_P_PARAM
{
    GCA_NAME		gca_parname;
    i4			gca_parm_mask;

# define	GCA_IS_BYREF	0x0001	/* Parameter is passed byref */
# define	GCA_IS_GTTPARM	0x0002	/* Param is global temporary table */
# define	GCA_IS_OUTPUT	0x0004	/* Parameter is delcared OUT in caller */

    GCA_DATA_VALUE	gca_parvalue;
};


/*  GCA_TUP_DESCR: Tuple descriptor */
typedef struct _GCA_TUP_DESCR GCA_TUP_DESCR;

struct _GCA_TUP_DESCR
{
    i4		gca_tsize;		/* Size of the tuple */
    i4		gca_result_modifier;
  
# define	GCA_NAMES_MASK		0x0001	/* Column names present */
# define	GCA_SCROLL_MASK		0x0002	/* Cursor is scrollable */
# define	GCA_READONLY_MASK	0x0004	/* Cursor is READONLY */

/*
**  The presence of the GCA_LO_MASK value indicates that the descriptor
**  may describe at least one field which contains a large object.
**  Large objects are of unknown, but potentially quite large, length;
**  consequently, their length is determined by the value itself, and is
**  not known when the query is planned.  The length of the large object
**  field(s) is not included in the gca_tsize field above.
*/
# define	GCA_LO_MASK		0x0008

/*
**  The presence of the GCA_COMP_MASK value indicates that the data
**  described by this descriptor is compressed.  Compressed indicates
**  that variable length datatypes (such as TEXT or VARCHAR) are having
**  their actual lengths sent, rather than their maximum lengths.
**
**  That is, if the string "COMPRESSED" is being sent as a compressed
**  varchar(20) element, the bytes sent will be
**	00 0A 'C' 'O' 'M' 'P' 'R' 'E' 'S' 'S' 'E' 'D'
**  The same string, sent uncompressed, would be followed by 10 more
**  bytes of undefined value (padding).
**
**  When this value is present, the gca_tsize value indicates that
**  maximum possible length of the described tuple.
*/
# define	GCA_COMP_MASK		0x0010

    u_i4	gca_id_tdscr;		/* ID of this descriptor */
    i4		gca_l_col_desc;		/* Number of columns */
    GCA_COL_ATT	gca_col_desc[1];	/* One for each column */
};


/*
** GCA_XA_DIS_TRAN_ID: XA Distributed Transaction ID
**
** This structure corresponds to the DB_XA_EXTD_DIS_TRAN_ID
** structure defined by the DBMS.
*/
typedef struct _GCA_XA_DIS_TRAN_ID GCA_XA_DIS_TRAN_ID;

struct _GCA_XA_DIS_TRAN_ID
{
    i4		formatID;
    i4		gtrid_length;
    i4		bqual_length;

# define	GCA_XA_GTRID_MAX	64	/* Max global xact ID len */
# define	GCA_XA_BQUAL_MAX	64	/* Max branch qualifier len */
# define	GCA_XA_XID_MAX		(GCA_XA_GTRID_MAX + GCA_XA_BQUAL_MAX)

    char	data[ GCA_XA_XID_MAX ];

    i4		branch_seqnum;
    i4		branch_flag;

# define	GCA_XA_BRANCH_FLG_FIRST	0x0001	/* FIRST TUX "AS" prepared */
# define	GCA_XA_BRANCH_FLG_LAST	0x0002	/* LAST TUX "AS" prepared */
# define	GCA_XA_BRANCH_FLG_2PC	0x0004	/* TUX 2PC transaction */
# define	GCA_XA_BRANCH_FLG_1PC	0x0008	/* TUX 1PC transaction */
};


/*  GCA_USER_DATA: User Data  */
typedef struct _GCA_USER_DATA GCA_USER_DATA;

struct _GCA_USER_DATA
{    
    i4			gca_p_index;    
    GCA_DATA_VALUE	gca_p_value;

/*
** These define the index values sent back and forth as GCA user data
** indices for the purposes of establishing associations.  Each index
** includes the description of the data value which will be associated
** with it.  In places where no data value is necessary, it is expected
** that a data value for the integer value 0 will be sent.  In cases
** where an on/off value is required, an integer data value of 0 indicates
** off, and a value of 1 indicates on.  All integers are expected to
** be 4 byte integers (i4's).
*/

/*
**  These options take no argument -- a zero integer should be sent
*/

    /* (-l) Whether to lock the db exclusively -- integer 0 value */
# define	GCA_EXCLUSIVE		(i4)0x0001


/*
**  These options take an integer value -- value depends upon
**  the option.
*/

    /*
    ** Version of the user protocol to work with -- Integer value 
    ** From API level 1 on, this parameter is redunant and
    ** should be avoided.
    */
# define	GCA_VERSION		(i4)0x0101

# define	GCA_V_60	0x0
# define	GCA_V_70	0x1

    /* (-A) Application code -- integer value */
# define	GCA_APPLICATION		(i4)0x0102

    /* Database language of choice -- integer value (DB_{QUEL,SQL}) */
# define	GCA_QLANGUAGE		(i4)0x0103

    /* (-c) Width of c columns */
# define	GCA_CWIDTH		(i4)0x0104

    /* (-t) Width of text columns */
# define	GCA_TWIDTH		(i4)0x0105

    /* (-i{1,2,4}nnn) Width of integer columns */
# define	GCA_I1WIDTH		(i4)0x0106
# define	GCA_I2WIDTH		(i4)0x0107
# define	GCA_I4WIDTH		(i4)0x0108
# define	GCA_I8WIDTH		(i4)0x0115

    /* (-f{4,8}[{e,f,g,n}[w[.p]]]) Float width & precision */
# define	GCA_F4WIDTH		(i4)0x0109
# define	GCA_F4PRECISION		(i4)0x010A
# define	GCA_F8WIDTH		(i4)0x010B
# define	GCA_F8PRECISION		(i4)0x010C

    /* Type of server function desired */
# define	GCA_SVTYPE		(i4)0x010D

# define	GCA_SVNORMAL	0x0000
# define	GCA_MONITOR	0x0001

    /* Native language for errors, etc */
# define	GCA_NLANGUAGE		(i4)0x010E

    /* Money precision */
# define	GCA_MPREC		(i4)0x010F

    /* Leading or trailing money sign */
# define	GCA_MLORT		(i4)0x0110

    /* Date format  -- values in DBMS.h */
# define	GCA_DATE_FORMAT		(i4)0x0111

    /* Time zone of client */
# define	GCA_TIMEZONE		(i4)0x0112
   
    /* Time zone name of client */
# define	GCA_TIMEZONE_NAME	(i4)0x0113

    /* II_DATE_CENTURY_BOUNDARY */
# define	GCA_YEAR_CUTOFF		(i4)0x0114

/*		GCA_I8WIDTH		(i4)0x0115	- See above */

    /* Security label support type of client */
# define	GCA_SL_TYPE		(i4)0x0120

/*
**  These options all expect a CHAR type string, non-padded
*/

    /* Database name with which to work -- comes with char type */
# define	GCA_DBNAME		(i4)0x0201

    /* (-n) Default secondary index structure */
# define	GCA_IDX_STRUCT		(i4)0x0202

    /* (-r) Default structure for result tables (ret. into's). */
# define	GCA_RES_STRUCT		(i4)0x0203

    /*
    ** for these two, the value strings are valid ingres table
    ** structures: [c]{isam,heap,btree,hash} (no heaps for
    ** idx_struct.
    */

    /* (-u) Effective user name */
# define	GCA_EUSER		(i4)0x0204

    /* (-x) Treatment of math exceptions -- char 'w' or 'f' or 'i' */
# define	GCA_MATHEX		(i4)0x0205

    /* (-f4x or -f8x) Floating point format "style"  -- one of [efgn] */ 
# define	GCA_F4STYLE		(i4)0x0206
# define	GCA_F8STYLE		(i4)0x0207

    /* Money sign */
# define	GCA_MSIGN		(i4)0x0208

    /* Decimal character */
# define	GCA_DECIMAL		(i4)0x0209

    /* Application id/password */
# define	GCA_APLID		(i4)0x020A

    /* Group id */
# define	GCA_GRPID		(i4)0x020B
	
/* (-k) Treatment of numeric literals -- char 'f' or 'd' */
# define	GCA_DECFLOAT		(i4)0x020C	

/* (-P) Real user password */
# define	GCA_RUPASS		(i4)0x020D

/* Miscellaneous flags passed as text to the DBMS */
# define	GCA_MISC_PARM		(i4)0x020E

/* (-string_truncation) String truncation option */
# define	GCA_STRTRUNC		(i4)0x020F

/* date_alias option value values are 'ingresdate' and 'ansidate' */
# define        GCA_DATE_ALIAS          (i4)0x0210

/* 
**  These options are on/off type options -- the value should be a
**  4 byte integer specifying either GCA_ON or GCA_OFF
*/

# define	GCA_ON	    0x1
# define	GCA_OFF	    0x0

    /* (+/- U) Update system catalog, exclusive lock */
# define	GCA_XUPSYS		(i4)0x0301

    /* (+/- Y) Update system catalog, shared lock */
    /*
    **	    This value is also used in modification requests
    **	to allow FE's to update their own catalogs
    */
# define	GCA_SUPSYS		(i4)0x0302

    /* (+/- w) Wait on DB */
# define	GCA_WAIT_LOCK		(i4)0x0303

    /* Frontend can handle prompts */
# define	GCA_CAN_PROMPT		(i4)0x0310

    /*
    **  To handle miscellaneous gateway parameters, the following
    **  constant allow the sending of miscellaneous text strings
    **  which can be parsed by the gateway.
    */
# define	GCA_GW_PARM		(i4)0x0400

    /*
    **  To handle re-association event after a broken connection
    **  between the coordinator and the local DBMS.
    */
# define	GCA_RESTART		(i4)0x0500

    /*
    **  To handle re-association event after a broken connection
    **  between the coordinator and the local DBMS using XA protocols.
    */
# define	GCA_XA_RESTART		(i4)0x0600

};


/*
** GCA objects directly associated with messages
*/

/*
** GCA_AK_DATA: Acknowledgement Data
**
** Messages: GCA_ACK, GCA_IACK
*/
typedef struct _GCA_AK_DATA GCA_AK_DATA;

struct _GCA_AK_DATA
{
    i4		gca_ak_data;
};


/*
** GCA_AT_DATA: Interrupt Data
**
** Messages: GCA_ATTENTION, GCA_NP_INTERRUPT
*/
typedef struct _GCA_AT_DATA GCA_AT_DATA;

struct _GCA_AT_DATA
{
    i4		gca_itype;

# define	GCA_INTALL	0x0000  /* Cancel everything */
# define	GCA_ENDRET      0x0001	/* Cancel retrieve */
# define	GCA_COPY_ERROR	0x0002	/* Copy error */
};


/*
** GCA_CP_MAP: Copy Map
**
** Messages: GCA_C_FROM, GCA_C_INTO
*/
typedef struct _GCA_CP_MAP GCA_CP_MAP;

struct _GCA_CP_MAP
{
    i4			gca_status_cp;
    i4			gca_direction_cp;

# define	GCA_CPY_INTO	0
# define	GCA_CPY_FROM	1

    i4			gca_maxerrs_cp;
    i4          	gca_l_fname_cp;		/* Length of following array */
    char        	gca_fname_cp[1];	/* Variable length */
    i4          	gca_l_logname_cp;	/* Length of following array */
    char        	gca_logname_cp[1];	/* Variable length */
    GCA_TUP_DESCR	gca_tup_desc_cp;	/* Variable length */
    i4			gca_l_row_desc_cp;	/* Length of following array */
    GCA_CPRDD   	gca_row_desc_cp[1];	/* Variable length */
};	


/*
** GCA1_CP_MAP: Copy Map
**
** Message: GCA1_C_FROM, GCA1_C_INTO
*/
typedef struct _GCA1_CP_MAP GCA1_CP_MAP;

struct _GCA1_CP_MAP
{
    i4			gca_status_cp;
    i4			gca_direction_cp;

	    /*	GCA_CPY_INTO	0 */
	    /*	GCA_CPY_FROM	1 */

    i4			gca_maxerrs_cp;
    i4          	gca_l_fname_cp;		/* Length of following array */
    char        	gca_fname_cp[1];	/* Variable length */
    i4          	gca_l_logname_cp;	/* Length of following array */
    char        	gca_logname_cp[1];	/* Variable length */
    GCA_TUP_DESCR	gca_tup_desc_cp;	/* Variable length */
    i4			gca_l_row_desc_cp;	/* Length of following array */
    GCA1_CPRDD  	gca_row_desc_cp[1];	/* Variable length */
};	


/*
** GCA_DL_DATA: Cursor delete
**
** Messages: GCA_DELETE
*/
typedef struct _GCA_DL_DATA GCA_DL_DATA;

struct _GCA_DL_DATA
{
    GCA_ID	gca_cursor_id;
    GCA_NAME	gca_table_name;
};


/* 
** GCA1_DL_DATA: Cursor delete
**
** Messages: GCA1_DELETE
*/
typedef struct _GCA1_DL_DATA GCA1_DL_DATA;

struct _GCA1_DL_DATA
{
    GCA_ID	gca_cursor_id;
    GCA_NAME    gca_owner_name;
    GCA_NAME	gca_table_name;
};


/*
** GCA_ER_DATA: Standard Error Representation
**
** Messages: GCA_ERROR, GCA_REJECT, GCA_RELEASE
*/
typedef struct _GCA_ER_DATA GCA_ER_DATA;

struct _GCA_ER_DATA
{        
    i4			gca_l_e_element;	/* Length of following array */
    GCA_E_ELEMENT	gca_e_element[1];	/* Variable length */
};


/*
** GCA_EV_DATA: Event Notification Data
**
** Messages: GCA_EVENT
*/
typedef struct _GCA_EV_DATA GCA_EV_DATA;

struct _GCA_EV_DATA
{
    GCA_NAME		gca_evname;
    GCA_NAME		gca_evowner;
    GCA_NAME		gca_evdb;
    GCA_DATA_VALUE	gca_evtime;
    i4			gca_l_evvalue;	/* Length of following array */
    GCA_DATA_VALUE	gca_evvalue[1];	/* Variable length */
};


/*
** GCA1_FT_DATA: Cursor fetch
**
** Messages: GCA1_FETCH
*/
typedef struct _GCA1_FT_DATA GCA1_FT_DATA;

struct _GCA1_FT_DATA
{
    GCA_ID	gca_cursor_id;
    i4		gca_rowcount;
};


/*
** GCA2_FT_DATA: Scrollable cursor fetch
**
** Messages: GCA2_FETCH
*/
typedef struct _GCA2_FT_DATA GCA2_FT_DATA;

struct _GCA2_FT_DATA
{
    GCA_ID	gca_cursor_id;
    i4		gca_rowcount;
    u_i4	gca_anchor;

# define GCA_ANCHOR_BEGIN	0x01	/* Start of result-set */
# define GCA_ANCHOR_END		0x02	/* End of result-set */
# define GCA_ANCHOR_CURRENT	0x03	/* Current position in result-set */

    i4		gca_offset;
};


/*
** GCA_ID_DATA: Query and Cursor Identifier
**
** Messages: GCA_CLOSE, GCA_FETCH, GCA_QC_ID
*/
typedef GCA_ID GCA_ID_DATA;


/*
** GCA_IP_DATA: Stored procedure invocation data
**
** Messages: GCA_INVPROC
*/
typedef struct _GCA_IP_DATA GCA_IP_DATA;

struct _GCA_IP_DATA
{
    GCA_ID	gca_id_proc;
    i4		gca_proc_mask;

# define	GCA_PARAM_NAMES_MASK	0x0001	/* GCA_NAMES_MASK */
# define	GCA_IP_NO_XACT_STMT	0x0002	/* No xact stmts */
# define	GCA_IP_LOCATOR_MASK	0x0008	/* GCA_LOCATOR_MASK */
# define	GCA_IP_COMP_MASK	0x0010	/* GCA_COMP_MASK */

    GCA_P_PARAM gca_param[1];		/* Variable length */
};


/*
** GCA1_IP_DATA: Stored procedure invocation data
**
** Messages: GCA1_INVPROC
*/
typedef struct _GCA1_IP_DATA GCA1_IP_DATA;

struct _GCA1_IP_DATA
{
    GCA_ID	gca_id_proc;
    GCA_NAME	gca_proc_own;
    i4		gca_proc_mask;

	     /*	GCA_PARAM_NAMES_MASK	0x0001	   GCA_NAMES_MASK */
	     /*	GCA_IP_NO_XACT_STMT	0x0002	   No xact stmts */
	     /*	GCA_IP_LOCATOR_MASK	0x0008	   GCA_LOCATOR_MASK */
	     /*	GCA_IP_COMP_MASK	0x0010	   GCA_COMP_MASK */

    GCA_P_PARAM gca_param[1];		/* Variable length */
};


/*
** GCA2_IP_DATA: Stored procedure invocation data
**
** Messages: GCA2_INVPROC
*/
typedef struct _GCA2_IP_DATA GCA2_IP_DATA;

struct _GCA2_IP_DATA
{
    GCA_NAME	gca_proc_name;
    GCA_NAME	gca_proc_own;
    i4		gca_proc_mask;

	     /*	GCA_PARAM_NAMES_MASK	0x0001	   GCA_NAMES_MASK */
	     /*	GCA_IP_NO_XACT_STMT	0x0002	   No xact stmts */
	     /*	GCA_IP_LOCATOR_MASK	0x0008	   GCA_LOCATOR_MASK */
	     /*	GCA_IP_COMP_MASK	0x0010	   GCA_COMP_MASK */

    GCA_P_PARAM gca_param[1];		/* Variable length */
};


/*
** GCA_IV_DATA: Repeat Query Invocation Data
**
** Messages: GCA_INVOKE
*/
typedef struct _GCA_IV_DATA GCA_IV_DATA;

struct _GCA_IV_DATA
{
    GCA_ID		gca_qid;
    GCA_DATA_VALUE	gca_qparm[1];	/* Variable length */
};


/*
** GCA_PROMPT_DATA: Prompt Data
**
** Messages: GCA_PROMPT
*/
typedef struct _GCA_PROMPT_DATA GCA_PROMPT_DATA;

struct _GCA_PROMPT_DATA
{
    i4			gca_prflags;

# define	GCA_PR_NOECHO	0x01	/* Prompt no echo */
# define	GCA_PR_TIMEOUT	0x02	/* Timeout provided */
# define	GCA_PR_PASSWORD	0x04	/* Requesting Password */

    i4			gca_prtimeout;
    i4			gca_prmaxlen;
    i4			gca_l_prmesg;	/* Length of following array */
    GCA_DATA_VALUE	gca_prmesg[1];	/* Variable Length */
};


/*
** GCA_PRREPLY_DATA: Reply to Prompt
**
** Messages: GCA_PRREPLY
*/
typedef struct _GCA_PRREPLY_DATA GCA_PRREPLY_DATA;

struct _GCA_PRREPLY_DATA
{
    i4			gca_prflags;

# define	GCA_PRREPLY_TIMEDOUT	0x01	/* Prompt timed out */
# define	GCA_PRREPLY_NOECHO	0x02	/* No echo set */

    i4			gca_l_prvalue;	/* Length of following array */
    GCA_DATA_VALUE	gca_prvalue[1];	/* Variable length */
};


/*  
** GCA_Q_DATA: Query Data
**
** Messages: GCA_CREPROC, GCA_DEFINE, GCA_DRP_PROC, GCA_QUERY
*/
typedef struct _GCA_Q_DATA GCA_Q_DATA;

struct _GCA_Q_DATA
{                   
    i4			gca_language_id;
    i4			gca_query_modifier;

		     /*	GCA_NAMES_MASK		0x0001     Column names */
# define		GCA_SINGLE_MASK		0x0002  /* Singleton query */
		     /*				0x0004 */
# define		GCA_LOCATOR_MASK	0x0008	/* Blob/Clob locators */
		     /*	GCA_COMP_MASK		0x0010	   Compress VARCHAR */
# define		GCA_Q_NO_XACT_STMT	0x0020	/* No xact stmts */
# define 		GCA_REPEAT_MASK 	0x0040 /* repeat previous query
						       ** (no query text required) */

    GCA_DATA_VALUE	gca_qdata[1];	/* Variable length */
};


/*
** GCA_RE_DATA: Response Data
**
** Messages: GCA_DONE, GCA_REFUSE, GCA_RESPONSE, GCA_S_BTRAN, GCA_S_ETRAN
*/
typedef struct _GCA_RE_DATA GCA_RE_DATA;

struct _GCA_RE_DATA
{           
    i4		gca_rid;		/* Response ID (unused) */
    i4		gca_rqstatus;                       

# define        GCA_OK_MASK		0x0000
# define        GCA_FAIL_MASK		0x0001  
# define        GCA_ALLUPD_MASK		0x0002  
# define        GCA_REMNULS_MASK	0x0004  
# define        GCA_RPQ_UNK_MASK	0x0008  
# define        GCA_END_QUERY_MASK	0x0010
# define	GCA_CONTINUE_MASK	0x0020
# define	GCA_INVSTMT_MASK	0x0040
# define	GCA_LOG_TERM_MASK	0x0080
# define	GCA_OBJKEY_MASK		0x0100
# define	GCA_TABKEY_MASK		0x0200
# define	GCA_NEW_EFF_USER_MASK	0x0400
# define        GCA_FLUSH_QRY_IDS_MASK  0x0800
# define	GCA_ILLEGAL_XACT_STMT	0x1000
# define	GCA_XA_ERROR_MASK	0x2000

    /*
    ** LIBQ mapping of response information is slightly
    ** different than as indicated by the following names:
    **
    ** sqlerrd[0]	Error code from GCA_ERROR message.
    ** sqlerrd[1]	gca_errd1
    ** sqlerrd[2]	gca_rowcount
    ** sqlerrd[3]	gca_cost
    ** sqlerrd[4]	gca_errd4
    ** sqlerrd[5]	gca_errd5
    */
    i4		gca_rowcount;		/* Number of rows */

# define	GCA_NO_ROW_COUNT	-1

    i4		gca_rhistamp;
    i4		gca_rlostamp;
    i4		gca_cost;		/* Unused */
    i4		gca_errd0;		/* Cursor status  */

# define	GCA_ROW_BEFORE		0x0001	/* Before first row */
# define	GCA_ROW_FIRST		0x0002	/* First row */
# define	GCA_ROW_LAST		0x0004	/* Last row */
# define	GCA_ROW_AFTER		0x0008	/* After last row */
# define	GCA_ROW_INSERTED	0x0010	/* Row inserted */
# define	GCA_ROW_UPDATED		0x0020	/* Row updated */
# define	GCA_ROW_DELETED		0x0040	/* Row deleted */

    i4		gca_errd1;		/* Row position */
    i4		gca_errd4;		/* Unused */
    i4		gca_errd5;		/* XA error if GCA_XA_ERROR_MASK */

    /* 
    ** If either GCA_OBJKEY_MASK or or GCA_TABKEY_MASK is 
    ** asserted in the gca_rqstatus field then gca_logkey 
    ** will contain the logical key value just assigned 
    ** as part of an insert into a table with a system 
    ** maintained logical key.  The table_key is in the 
    ** first 8 bytes, the object key is in the entire 16 
    ** bytes.
    */
    char	gca_logkey[16];	    
};


/*
** GCA_RP_DATA: Stored procedure response completion
**
** Messages: GCA_RETPROC
*/
typedef struct _GCA_RP_DATA GCA_RP_DATA;

struct _GCA_RP_DATA
{
    GCA_ID	gca_id_proc;
    i4		gca_retstat;
};


/*
** GCA_SESSION_PARMS: Association Parameters
**
** Messages: GCA_MD_ASSOC
*/
typedef struct _GCA_SESSION_PARMS GCA_SESSION_PARMS;

struct _GCA_SESSION_PARMS
{ 
    i4			gca_l_user_data;	/* Length of following array */
    GCA_USER_DATA	gca_user_data[1];	/* Variable length */
};


/*
** GCA_TD_DATA: Tuple Descriptor
**
** Messages: GCA_TDESCR
*/
typedef GCA_TUP_DESCR GCA_TD_DATA;


/*
** GCA_TR_DATA: DBMS Trace Information
**
** Messages: GCA_TRACE
*/
typedef struct _GCA_TR_DATA GCA_TR_DATA;

struct _GCA_TR_DATA
{
    char	gca_tr_data[1];		/* Variable length */
};


/*
** GCA_TU_DATA: Tuple Data
**
** Messages: GCA_CDATA, GCA_TUPLES
*/
typedef struct _GCA_TU_DATA GCA_TU_DATA;

struct _GCA_TU_DATA
{
    char    gca_tu_data[1];
};


/*
** GCA_TX_DATA: Transaction Data
**
** Messages: GCA_Q_BTRAN, GCA_SECURE
*/
typedef struct _GCA_TX_DATA GCA_TX_DATA;

struct _GCA_TX_DATA
{
    GCA_ID	gca_tx_id;
    i4		gca_tx_type;

# define	GCA_DTX		1
};


/* 
** GCA_XA_TX_DATA: XA Transaction Data
**
** Messages: GCA_XA_SECURE
*/
typedef struct _GCA_XA_TX_DATA GCA_XA_TX_DATA;

struct _GCA_XA_TX_DATA
{
    i4			gca_xa_type;
    i4			gca_xa_precision;
    i4			gca_xa_l_value;
    GCA_XA_DIS_TRAN_ID	gca_xa_id;
};


/*
** GCA_XA_DATA: XA Transaction Data
**
** Messages: GCA_XA_START, GCA_XA_END, GCA_XA_PREPARE, 
**	     GCA_XA_COMMIT, GCA_XA_ROLLBACK
*/
typedef struct _GCA_XA_DATA GCA_XA_DATA;

struct _GCA_XA_DATA
{
    u_i4		gca_xa_flags;

# define	GCA_XA_FLG_JOIN		0x00200000
# define	GCA_XA_FLG_FAIL		0x20000000
# define	GCA_XA_FLG_1PC		0x40000000

    GCA_XA_DIS_TRAN_ID	gca_xa_xid;
};



/*
** GCA Data Types
**
** The following data types are supported by GCA in parameters,
** tuples, and tuple descriptors.  A data value is defined by a 
** descriptor triad: type, precision, length (TPL).  Tuple data
** is separated into data values (GCA_TU_DATA) and TPL descriptors
** (GCA_TUP_DESCR) while parameters combine values and TPL into
** a single GCA object (GCA_DATA_VALUE).
**
** The basic internal formats of the data values are defined here,
** but details beyond what is needed for Het-Net processing are
** not included.  In most cases, the actual internal formats are
** controlled by the ADF facility.  
**
** Nullable types are followed by a single byte containing a NULL 
** indicator bit (GCA_NULL_BIT).  Generally, a NULL data value 
** should be filled with null bytes.  However, when a data type 
** contains internal structural info (variable and long types)
** in addition to the data value, the internal data should be
** correctly set even when NULL.
**
** The TPL type identifiers correspond to the data types declared 
** by the DBMS.  Nullable types have a '_N' suffix and and a type 
** identifier which is the negative of the corresponding non-
** nullable type.
**
** The TPL length represents the length of the entire data value 
** in bytes (including optional NULL byte).  Thus a nullable 
** varchar(32) will have a TPL length of 35 (32 max characters, 
** 2 byte internal length, 1 byte null indicator).
**
** For compressed data values, the TPL length indicates the max
** uncompressed length.  For the long (segmented) data types,
** the TPL indicates the minimum fixed length.
**
** The TPL precision actually contains the precision and scale
** (where applicable).  These two values are combined into a two
** byte integer with the high byte holding the precision and the
** low byte holding the scale.
*/

# define	GCA_NULL_BIT	0x01

/*
** Fixed array types
**
** The following types are formed as arrays of atomic
** elements.  The length of the array, in bytes (not
** number of elements), is defined by the associated
** TPL.  
**
** These types are padded to their full TPL length 
** with values appropriate to their element type.  
** Three types of atomic elements are supported:
**
**   Binary:	Elements are unsigned single byte
**		integers (u_i1).
**
**	struct
**	{
**	    u_i1 data[ <TPL> ];
**	} gca_byte;
**
**   Character:	Elements are single byte characters
**		which are encoded according to the
**		single or double byte character set
**		associated with the GCA installation.
**
**	struct
**	{
**	    char data[ <TPL> ];
**	} gca_char;
**
**   UCS2:	Elements are two byte characters 
**		which are encoded according to the 
**		UCS2 standard.  The elements are 
**		actually treated as unsigned two 
**		byte integers (u_i2).
**
**	struct
**	{
**	    u_i2 data[ <TPL / 2> ];
**	} gca_ucs2;
*/

# define	GCA_TYPE_CHAR		20	/* Char		DB_CHA_TYPE */
# define	GCA_TYPE_CHAR_N		(-20)
# define	GCA_TYPE_BYTE		23	/* Byte		DB_BYTE_TYPE */
# define	GCA_TYPE_BYTE_N		(-23)
# define	GCA_TYPE_NCHR		26	/* UCS2		DB_NCHR_TYPE */
# define	GCA_TYPE_NCHR_N		(-26)
# define	GCA_TYPE_C		32	/* 'C'		DB_CHR_TYPE */
# define	GCA_TYPE_C_N		(-32)
# define	GCA_TYPE_QTXT		51	/* Query Text	DB_QTXT_TYPE */
# define	GCA_TYPE_QTXT_N		(-51)
# define	GCA_TYPE_NQTXT		54	/* Query UCS2	DB_NQTXT_TYPE */
# define	GCA_TYPE_NQTXT_N	(-54)

/*
** Variable array types
**
** The following types are formed as arrays of atomic
** elements.  The maximum length of the array, in bytes 
** (not number of elements), is defined by the associated
** TPL.  A leading internal length indicator defines the 
** valid number of elements (not bytes) in the array.  
**
** Generally, these types are padded to their full TPL 
** length with null bytes which are not considered a part 
** of the actual data.  Array elements may be the same
** atomic types as are supported for fixed array types.
**
**	struct
**	{
**	    u_i2 len;
**	    u_i1 data[ <len> ];
**	    u_i1 pad[ <TPL - len> ];
**	} gca_vbyte;
**
**	struct
**	{
**	    u_i2 len;
**	    char data[ <len> ];
**	    u_i1 pad[ <TPL - len> ];
**	} gca_vchar;
**
**	struct
**	{
**	    u_i2 len;
**	    u_i2 data[ <len> ];
**	    u_i1 pad[ <TPL - (len * 2)> ];
**	} gca_vucs2;
**
** Compressed values do not contain padding.  Compression 
** is optional for values contained in tuples as indicated 
** separately from the TPL and data value (see GCA_COMP_MASK).  
**
**	struct
**	{
**	    u_i2 len;
**	    u_i1 data[ <len> ];
**	} gca_cvbyte;
**
**	struct
**	{
**	    u_i2 len;
**	    char data[ <len> ];
**	} gca_cvchar;
**
**	struct
**	{
**	    u_i2 len;
**	    u_i2 data[ <len> ];
**	} gca_cvucs2;
**
** A NULL value which is also compressed will contain an 
** internal length indicator of 0 followed immediatly by 
** the null indicator byte.
*/

# define	GCA_TYPE_VCHAR		21	/* Var Char	DB_VCH_TYPE */
# define	GCA_TYPE_VCHAR_N	(-21)
# define	GCA_TYPE_VBYTE		24	/* Var Byte	DB_VBYTE_TYPE */
# define	GCA_TYPE_VBYTE_N	(-24)
# define	GCA_TYPE_VNCHR		27	/* Var UCS2	DB_NVCHR_TYPE */
# define	GCA_TYPE_VNCHR_N	(-27)
# define	GCA_TYPE_TXT		37	/* Text		DB_TXT_TYPE */
# define	GCA_TYPE_TXT_N		(-37)
# define	GCA_TYPE_LTXT		41	/* Long Text	DB_LTXT_TYPE */
# define	GCA_TYPE_LTXT_N		(-41)

/*
** Long (segmented) types
**
** The following types represent variable length arrays 
** with no pre-defined maximum length.  They are formed
** with a fixed header and end indicator and a variable
** (including 0) number of data segments.  The associated
** TPL only describes the fixed portions.
**
** Array elements may be the same atomic types as are 
** supported for fixed array types.
**
**   Header:
**
**	struct
**	{
**	    i4	 tag;
**	    u_i4 len0;
**	    u_i4 len1;
**	} gca_long_hdr;
**
**   Segment:
**
**	struct
**	{
**	    u_i4 not_zero; // = 1
**	    u_i2 len;
**	    u_i1 data[ <len> ];
**	} gca_long_byte;
**
**	struct
**	{
**	    u_i4 not_zero; // = 1
**	    u_i2 len;
**	    char data[ <len> ];
**	} gca_long_char;
**
**	struct
**	{
**	    u_i4 not_zero; // = 1
**	    u_i2 len;
**	    u_i2 data[ <len> ];
**	} gca_long_ucs2;
**
**   End-of-segments:
**
**	struct
**	{
**	    u_i4 zero; // = 0
**	} gca_long_end;
**
** A NULL value contains only the header and end indicator.
*/

# define	GCA_TYPE_LCHAR		22	/* Long Char	DB_LVCH_TYPE */
# define	GCA_TYPE_LCHAR_N	(-22)
# define	GCA_TYPE_LBYTE		25	/* Long Byte	DB_LBYTE_TYPE */
# define	GCA_TYPE_LBYTE_N	(-25)
# define	GCA_TYPE_LNCHR		28	/* Long UCS2	DB_LNVCHR_TYPE*/
# define	GCA_TYPE_LNCHR_N	(-28)

/*
** Long locator types
**
** The following types represent references to long variable
** length array values stored in the DBMS.  They share the 
** same header used with the segmented types but replace the 
** data segments with a single fixed locator value.
**
**   Header:
**
**	struct
**	{
**	    i4	 tag;
**	    u_i4 len0;
**	    u_i4 len1;
**	} gca_long_hdr;
**
**   Locator:
**
**	struct
**	{
**	    u_i4 data;
**	} gca_locator;
**
*/

# define	GCA_TYPE_LCLOC		36	/* Char Locator DB_LCLOC_TYPE */
# define	GCA_TYPE_LCLOC_N	(-36)
# define	GCA_TYPE_LBLOC		35	/* Byte Locator DB_LBLOC_TYPE */
# define	GCA_TYPE_LBLOC_N	(-36)
# define	GCA_TYPE_LNLOC		29	/* UCS2 Locator DB_LNLOC_TYPE */
# define	GCA_TYPE_LNLOC_N	(-29)

/*
** Numeric types
**
** These types represent signed integers of
** one, two, four and eight bytes (i1, i2, 
** i4, i8) and floating point values of four 
** and eight bytes (f4, f8).  They are stored 
** using the hardware format of the platform.
**
**	struct { i1 data; } gca_int1;
**	struct { i2 data; } gca_int2;
**	struct { i4 data; } gca_int4;
**	struct { i8 data; } gca_int8;
**	struct { f4 data; } gca_float4;
**	struct { f8 data; } gca_float8;
*/

# define	GCA_TYPE_INT		30	/* Integer	DB_INT_TYPE */
# define	GCA_TYPE_INT_N		(-30)
# define	GCA_TYPE_FLT		31	/* Float	DB_FLT_TYPE */
# define	GCA_TYPE_FLT_N		(-31)

/*
** Abstract types
**
** The following types are defined by the Abstract
** Datatype Facility (ADF).  Only formatting info
** sufficient for Het-Net conversions is provided
** here.
**
**   Boolean:
**
**	struct
**	{
**	    u_i1 data;
**	} gca_bool;
**
**   Decimal:
**
**	struct
**	{
**	    u_i1 data[ <TPL> ];
**	} gca_dec;
**
**   Money:
**
**	struct
**	{
**	    f8	data;
**	} gca_mny;
**
**   Date:
**
**	struct
**	{
**	    u_i1 data1;
**	    i1   data2;
**	    i2   data3;
**	    i2   data4;
**	    u_i2 data5;
**	    i4   data6;
**	} gca_idate;
**
**	struct
**	{
**	    i2	data1;
**	    i1	data2;
**	    i1	data3;
**	} gca_adate;
**
**   Time:
**
**	struct
**	{
**	    i4	data1;
**	    i4	data2;
**	    i1	data3;
**	    i1	data4;
**	} gca_time;
**
**   Timestamp:
**
**	struct
**	{
**	    i2	data1;
**	    i1	data2;
**	    i1	data3;
**	    i4	data4;
**	    i4	data5;
**	    i1	data6;
**	    i1	data7;
**	} gca_ts;
**
*   Interval:
**
**	struct
**	{
**	    i2	data1;
**	    i1	data2;
**	} gca_intym;
**
**	struct
**	{
**	    i4	data1;
**	    i4	data2;
**	    i4	data3;
**	} gca_intds;
**
**   Logical Key:
**
**	struct
**	{
**	    i1	data[ 16 ];
**	} gca_lkey;
**
**   Table Key:
**
**	struct
**	{
**	    i1	data[ 8 ];
**	} gca_tkey;
**
**   Security Label:
**
**	struct
**	{
**	    i1	data[ <TPL> ];
**	} gca_secl;
*/

# define	GCA_TYPE_DATE		3	/* Date		DB_DTE_TYPE */
# define	GCA_TYPE_DATE_N		(-3)
# define	GCA_TYPE_ADATE		4	/* ANSI Date	DB_ADTE_TYPE */
# define	GCA_TYPE_ADATE_N	(-4)
# define	GCA_TYPE_MNY		5	/* Money	DB_MNY_TYPE */
# define	GCA_TYPE_MNY_N		(-5)
# define	GCA_TYPE_TMWO		6	/* Time w/o TZ	DB_TMWO_TYPE */
# define	GCA_TYPE_TMWO_N		(-6)
# define	GCA_TYPE_TMTZ		7	/* Time with TZ	DB_TMW_TYPE */
# define	GCA_TYPE_TMTZ_N		(-7)
# define	GCA_TYPE_TIME		8	/* Time		DB_TME_TYPE */
# define	GCA_TYPE_TIME_N		(-8)
# define	GCA_TYPE_TSWO		9	/* TS w/o TZ	DB_TSWO_TYPE */
# define	GCA_TYPE_TSWO_N		(-9)
# define	GCA_TYPE_DEC		10	/* Decimal	DB_DEC_TYPE */
# define	GCA_TYPE_DEC_N		(-10)
# define	GCA_TYPE_LKEY		11	/* Logical Key	DB_LOGKEY_TYPE*/
# define	GCA_TYPE_LKEY_N		(-11)
# define	GCA_TYPE_TKEY		12	/* Tab Key	DB_TABKEY_TYPE*/
# define	GCA_TYPE_TKEY_N		(-12)
# define	GCA_TYPE_TSTZ		18	/* TS with TZ	DB_TSW_TYPE */
# define	GCA_TYPE_TSTZ_N		(-18)
# define	GCA_TYPE_TS		19	/* Timestamp	DB_TSTMP_TYPE */
# define	GCA_TYPE_TS_N		(-19)
# define	GCA_TYPE_INTYM		33	/* Y/M Interval	DB_INYM_TYPE */
# define	GCA_TYPE_INTYM_N	(-33)
# define	GCA_TYPE_INTDS		34	/* D/S Interval	DB_INDS_TYPE */
# define	GCA_TYPE_INTDS_N	(-34)
# define	GCA_TYPE_BOOL		38	/* Boolean	DB_BOO_TYPE */
# define	GCA_TYPE_BOOL_N		(-38)
# define	GCA_TYPE_SECL		60	/* Sec Label	DB_SEC_TYPE */
# define	GCA_TYPE_SECL_N		(-60)

#endif
/************************** End of gca.h *************************************/

