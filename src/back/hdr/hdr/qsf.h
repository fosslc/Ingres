/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: QSF.H - All external definitions needed to use the 
**
** Description:
**      This file contains all of the typedefs and defines necessary
**      to use any of the Query Storage Facility routines.  It depends
**      on the DBMS.H header file already being #included.
**
**      This file defines the following typedefs:
**	    QSF_CB	- QSF session control block.
**	    QSO_OBID	- Object id.
**	    QSF_RCB	- QSF request block.
**	    QSF_AEO	- An Associated External Object.
**			    This is an object, managed externally to QSF,
**			    which is associated with a QSF object.  The
**			    associations primary purpose, at this level, is to
**			    assure related destruction.
**
** History: 
**      12-mar-86 (thurston)
**          Initial creation.
**      18-mar-86 (thurston)
**	    Got rid of the QSO_OBNAME typedef by incorporating it into
**	    the QSO_OBID typedef.  Also, got rid of the .qsf_oblks member
**	    from the QSF_RCB typedef.
**	19-mar-86 (thurston)
**	    Added the #defines for all E_QSxxx codes in the QSF_RCB typedef.
**	12-may-86 (thurston)
**	    Changed the definition of an QSO_OBID.
**      02-mar-87 (thurston)
**	    Added the .qsf_server member to the QSF_RCB typedef.
**      02-mar-87 (thurston)
**	    Added QSO_SQLDA_OBJ as another valid object type for QSF.
**      17-mar-87 (stec)
**	    Added QSO_CHTYPE define.
**	18-may-87 (thurston)
**	    Added the .qss_mem member to QSF session control block, QSF_CB.
**	26-may-87 (thurston)
**	    Added the .qsf_scb_sz, .qsf_1rsvd[], and .qsf_2rsvd[] members to
**	    the QSF_RCB typedef.
**	27-may-87 (thurston)
**	    Added the QSF op IDs, QSD_BO_DUMP and QSD_BOQ_DUMP.
**	04-jun-87 (thurston)
**	    Added the .qsf_pool_sz and .qsf_max_active_ses members to QSF_RCB.
**	14-jan-88 (thurston)
**	    Added QSO_ALIAS_OBJ as another valid object type for QSF, along with
**	    some notes about it.  Also added the QSO_TRANS_OR_DEFINE opcode, and
**	    the .qsf_feobj_id member to the request CB.
**	13-may-88 (thurston)
**	    Increased the size of .qso_name in the QSO_OBID structure, so that
**	    it can hold a db procedure name plus an owner name.
**	24-may-88 (eric)
**	    Added the QSO_JUST_TRANS opcode.
**	11-nov-88 (thurston)
**	    Increased the size of .qso_name in the QSO_OBID structure, so that
**	    it can hold a db procedure name plus an owner name, PLUS a unique
**	    database ID.
**      25-jan-89 (neil)
**	    Revised comment on meaning of QSO_ALIAS_OBJ.  Created QSO_NAME
**	    to be used instead of .qso_name.
**	07-jun-90 (andre)
**	    Defined QSO_CRTALIAS.
**	08-jan-92 (fred)
**	    Added QSF_AEO typedef & QSO_ASSOCIATE operation.
**	16-oct-92 (rog)
**	    Added the QSO_GETNEXT and the QSO_GET_OR_CREATE opcodes.
**	12-mar-1993 (rog)
**	    Added the QSO_DESTROYALL opcode.
**	07-jul-1993 (rog)
**	    Added prototypes for qsf_call() and qsf_trace().
**	11-dec-96 (stephenb)
**	    Add qsf_qsscb_offset to QSR_CB to qsf_session() re-write.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added CS_SID qsf_sid to QSF_RCB.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**	21-Feb-2005 (schka24)
**	    Get rid of unused entry points, and the unused external associated
**	    object stuff.  Update comments a bit.
**      18-Feb-2005 (hanal04) Bug 112376 INGSRV2837
**          Added QSF_SES_FLUSH_LRU for calls to qsf_clrsesobj().
**	17-Feb-2009 (wanfr01)
**	    Bug 121543
**	    Added qsf_flags to identify if QSF object is dbp or repeated query
*/


/*
**  Defines of other constants.
*/

/*
**      These are the possible op-codes for using QSF:
*/
#define                 QSF_MIN_OPCODE	     1

#define                 QSR_STARTUP	     1
#define                 QSR_SHUTDOWN	     2
#define                 QSS_BGN_SESSION	     3
#define                 QSS_END_SESSION	     4
#define                 QSO_CREATE	     5
#define                 QSO_DESTROY	     6
#define                 QSO_LOCK	     7
#define                 QSO_UNLOCK	     8
#define                 QSO_SETROOT	     9
#define                 QSO_INFO	    10
#define                 QSO_PALLOC	    11
#define                 QSO_GETHANDLE	    12
/*	unused, was rename			13 */
#define                 QSD_OBJ_DUMP	    14
#define                 QSD_OBQ_DUMP	    15
#define                 QSO_CHTYPE	    16
#define                 QSD_BO_DUMP	    17
#define                 QSD_BOQ_DUMP	    18
#define                 QSO_TRANS_OR_DEFINE 19
#define			QSO_JUST_TRANS	    20
/*	unused, was crtalias			21 */
/*	unused, was associate			22 */
#define			QSO_GETNEXT	    23
/*	unused, was get_or_create		24 */
#define                 QSO_DESTROYALL	    25
#define			QSF_SES_FLUSH_LRU   26
#define                 QSF_MAX_OPCODE	    26


/*}
** Name: QSF_TRSTRUCT - Tracing structure for QSF.
**
** Description:
**
** History:
**     17-dec-87 (thurston)
**	    Initial creation.
**     30-nov-1992 (rog)
**	    Moved here from qsfint.h.  (This requires that ulf.h always be
**	    included before qsf.h.)
*/
typedef struct _QSF_TRSTRUCT
{
    i4				dummy;
    ULT_VECTOR_MACRO(128, 8)	trvect;
}   QSF_TRSTRUCT;


/*}
** Name: QSF_CB - QSF session control block.
**
** Description:
**      This structure defines the session control block for QSF.
**      One of these structures is maintained by SCF for each session
**      in the server.
**
** History:
**      12-mar-86 (thurston)
**          Initial creation.
**	18-may-87 (thurston)
**	    Added .qss_mem.
**	03-nov-1992 (rog)
**	    Replaced qss_mem with the contents of qss_mem so that it does not
**	    need to use QSF memory.  (Really just the qss_trstruct field.) 
**	    Added the qss_obj_list member to hold unnamed objects owned by this
**	    session.  Added qss_master to point to any master objects in the
**	    process of being created by this session.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef struct _QSF_CB
{
/********************   START OF STANDARD HEADER   ********************/

    struct _QSF_CB  *qsf_next;          /* Forward ptr for QSF_CB list. */
    struct _QSF_CB  *qsf_prev;          /* Backward ptr for QSF_CB list. */
    SIZE_TYPE       qsf_length;         /* Length of this structure: */
    i2              qsf_type;		/* Type of structure.  For now, 1114 */
#define                 QSFCB_CB        1114	/* for no particular reason */
    i2              qsf_s_reserved;	/* Possibly used by mem mgmt. */
    PTR             qsf_l_reserved;	/*    ''     ''  ''  ''  ''   */
    PTR             qsf_owner;          /* Owner for internal mem mgmt. */
    i4              qsf_ascii_id;       /* So as to see in a dump. */
					/*
					** Note that this is really a char[4]
					** but it is easer to deal with as an i4
					*/
#define                 QSFCB_ASCII_ID  CV_C_CONST_MACRO('Q', 'S', 'C', 'B')

/*********************   END OF STANDARD HEADER   *********************/

    QSF_TRSTRUCT	 qss_trstruct;	/* QSF's session trace vector.  */
    struct _QSO_OBJ_HDR	*qss_obj_list;	/* List of unshared objects owned by
					** this session.
					*/
    struct _QSO_MASTER_HDR *qss_master;	/* Master object of QP being created by
					** this session.
					*/
}   QSF_CB;


/*}
** Name: QSO_OBID - Object id for a QSF object.
**
** Description:
**      This structure defines a QSF object id, which uniquely identifies an
**      object stored in QSF's memory.  It consists of an object name (optional)
**      and an object type.  Since the object id uniquely identifies an object
**      stored in QSF's memory, and the id consists of an object name and an
**      object type, there can be one object of each type with the same name.
**      This means that the query text, query tree, and query plan for a given
**      query can all share the same name but still be different QSF objects. 
**
**	Objects can be (and indeed very often are) un-named, in which
**	case the object "name" is just its handle, i.e. address.  Unnamed
**	objects are private to a session, and the session is responsible
**	for tracking and freeing unnamed objects.
**
**	When an object has a name, the name is of the form:
**	i4, i4			 |
**	DB_MAXNAME characters	-| actually part of a DB_CURSOR_ID
**	DB_MAXNAME more characters
**	another i4
**
**	Not all of this will be used (only the first lname chars).
**	There is a feeble attempt to structure-ize this name (see
**	later on), but in actuality it's hardwired in several places.
**
**	The QSF hash lookup algorithm depends on the fact that the
**	first DB_MAXNAME string will be the same in an ID's alias as
**	it is in the "real" object ID.  (I.e. the object ID and the
**	object's alias, if any, will hash to the same place.)
**
** History:
**	12-mar-86 (thurston)
**          Initial creation.
**	18-mar-86 (thurston)
**	    Incorporated the QSO_OBNAME typedef into this structure.  There
**	    was no real need for it, and by just using a char array here,
**	    it will be simpler to use.
**	18-mar-86 (thurston)
**	    Added the .qso_handle member.
**	12-may-86 (thurston)
**	    Added the .qso_lname member.
**	02-mar-87 (thurston)
**	    Added QSO_SQLDA_OBJ as another valid object type for QSF.
**	14-jan-88 (thurston)
**	    Added QSO_ALIAS_OBJ as another valid object type for QSF, along with
**	    some notes about it.
**	13-may-88 (thurston)
**	    Increased the size of .qso_name so that it can hold a db procedure
**	    name plus an owner name.
**	11-nov-88 (thurston)
**	    Increased the size of .qso_name so that it can hold a db procedure
**	    name plus an owner name, PLUS a unique database ID.
**	25-jan-89 (neil)
**	    Revised comment of QSO_ALIAS and .qso_name.  See QSO_NAME for a
**	    for new usage.
**	30-nov-1992 (rog)
**	    Added QSO_MASTER_OBJ object #define.
**	22-Feb-2005 (schka24)
**	    Remove same, updated comments.
*/
typedef struct _QSO_OBID
{
    PTR		    qso_handle;         /* When created, QSF assigns this
                                        ** "handle" to the object.  This
                                        ** must be used to do all other
                                        ** QSF work with that object, such
                                        ** as QSO_PALLOC, QSO_ACCESS, etc.
                                        ** This is actually a pointer to the
                                        ** object header, but should only be
                                        ** used as an abstraction by the calling
                                        ** routines.
					*/
    i4              qso_type;           /* Type of object this is.  This must
					** be one of the following:
					*/
#define                 QSO_QTEXT_OBJ   1   /* Object is query text */
#define                 QSO_QTREE_OBJ   2   /* Object is query tree */
#define                 QSO_QP_OBJ      3   /* Object is query plan */
#define                 QSO_SQLDA_OBJ   4   /* Object is SQLDA for `describe' */
#define                 QSO_ALIAS_OBJ   5   /* Object ID is QP alias.
					** No object actually has ALIAS as
					** its type.  A QP alias is used only						** as part of the FE alternate name of
					** a shared QP (repeat qry or DBP).
					** The "fe" ID is tagged with ALIAS
					** to distinguish it from the BE id,
					** which is tagged QP.
					*/
#define			QSO_MAX_OBJ	5

    i4              qso_lname;          /* Number of characters in name;
					** zero implies an unnamed object.
					*/
    char            qso_name[sizeof(DB_CURSOR_ID) + DB_MAXNAME + sizeof(i4)]; 
					/* Name this object is known by.
                                        ** Only the first qso_lname characters
					** will be relevant to the object name.
                                        ** This is used for QSO_CREATE (where it
                                        ** is optional; that is, if you are
                                        ** creating a named object), QSO_RENAME
                                        ** (where it is the new name for the
                                        ** object), QSO_GETHANDLE (to retrieve
                                        ** the object's "handle"), and QSO_INFO
                                        ** (where it is returned if the object
                                        ** has a name).  Should be big enough
					** to hold largest object name,
					** currently db procedure name plus
					** owner name, plus unique database ID.
					** *Incredibly dangerous*: because
					** DB_MAXNAME just happens to be a
					** multiple of 4, this qso_name is
					** identical to the QSO_NAME struct.
					** The i4 will be aligned, although
					** many users don't assume that.
					*/
}   QSO_OBID;

/*}
** Name: QSO_NAME - Name/alias representing a QSF object.
**
** Description:
**	This structure the name of a QSF object (previously referenced through
**	.qso_name).  This struct is currently used to identify procedures
**	invoked during the processing of database rules.
**	It's in fact meant to be the same as the "qso_name" member
**	of a QSO_OBID object id.
**
** History:
**	23-jan-89 (neil)
**          Initial creation for Terminator.
**	09-mar-89 (neil)
**	    Modified qso_n_own to DB_OWN_NAME.
*/
typedef struct _QSO_NAME
{
    DB_CURSOR_ID    qso_n_id;		    /* Query id of named object.
					    ** A "raw" alias consists of
					    ** zero integer ids, and a blank
					    ** padded object name.
					    */
    DB_OWN_NAME	    qso_n_own;  	    /* User/owner of above object */
    i4		    qso_n_dbid;		    /* Unique database id */
}   QSO_NAME;

/*}
** Name: QSF_RCB - Request block for all calls to QSF via qsf_call().
**
** Description:
**      This structure defines the request block which is used for passing
**      and retrieving all information to and from all calls to QSF.  This
**      is done via the qsf_call() routine, where a QSF_RCB is the second
**      argument (the opcode is the first).
**
** History:
**      12-mar-86 (thurston)
**          Initial creation.
**      18-mar-86 (thurston)
**          Removed the .qsf_oblks member since QSF will now use a generalized
**	    memory management scheme, and will not know this information.
**      19-mar-86 (thurston)
**	    Added the #defines for all E_QSxxx codes.
**      02-mar-87 (thurston)
**	    Added the .qsf_server member.
**	26-mar-87 (thurston)
**	    Added the .qsf_scb_sz, .qsf_1rsvd[], and .qsf_2rsvd[] members.
**	04-jun-87 (thurston)
**	    Added the .qsf_pool_sz and .qsf_max_active_ses members.
**	14-jan-88 (thurston)
**	    Added the .qsf_feobj_id member for handling repeat query plans.
**	01-sep-88 (puree)
**	    Change type for qsf_next and qsf_prev from _QSF_CB to _QSF_RCB.
**	08-jan-1992 (fred)
**	    Added qsf_aeo field.
**	02-dec-1992 (rog)
**	    Added QSO_WASTHERE and QSO_WASCREATED #define's.  Also added errors
**	    1C, 1D, and 1E.
**	07-jan-1993 (rog)
**	    Added error E_QS0020_QS_INIT_MO_ERROR.
**	14-jan-1993 (rog)
**	    Added error E_QS001F_TOO_MANY_ORPHANS.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	11-dec-96 (stephenb)
**	    Add qsf_qsscb_offset for qsf_session() re-write.
**	10-Jan-2001 (jenjo02)
**	    Added CS_SID qsf_sid to QSF_RCB.
*/
typedef struct _QSF_RCB
{
/********************   START OF STANDARD HEADER   ********************/

    struct _QSF_RCB *qsf_next;          /* Forward ptr for QSF_RCB list. */
    struct _QSF_RCB *qsf_prev;          /* Backward ptr for QSF_RCB list. */
    SIZE_TYPE       qsf_length;         /* Length of this structure: */
    i2              qsf_type;		/* Type of structure.  For now, 2197 */
#define                 QSFRB_CB        2197	/* for no particular reason */
    i2              qsf_s_reserved;	/* Possibly used by mem mgmt. */
    PTR             qsf_l_reserved;	/*    ''     ''  ''  ''  ''   */
    PTR             qsf_owner;          /* Owner for internal mem mgmt. */
    i4              qsf_ascii_id;       /* So as to see in a dump. */
					/*
					** Note that this is really a char[4]
					** but it is easer to deal with as an i4
					*/
#define                 QSFRB_ASCII_ID  CV_C_CONST_MACRO('Q', 'S', 'R', 'B')

/*********************   END OF STANDARD HEADER   *********************/

    PTR             qsf_server;         /* Ptr to memory for QSF's server
                                        ** control block.  This is returned
                                        ** for QSR_STARTUP, and must be supplied
                                        ** for QSS_BGN_SESSION.
                                        */
    QSF_CB          *qsf_scb;           /* Ptr to QSF session control block.
					** This is supplied for QSS_BGN_SESSION
					** and QSS_END_SESSION.
					*/
    CS_SID	    qsf_sid;		/* Caller-supplied session id */
    QSO_OBID        qsf_obj_id;         /* QSF object id.  For most opcodes,
					** it's the object ID to operate on.
					** For trans-or-define, or just-trans,
					** it's the returned BE object ID that
					** matches the supplied FE alias.
					** And, for trans-or-define, if the
					** alias is not found, it's the BE
					** object ID to use for the created
					** QP object.
					*/
    QSO_OBID        qsf_feobj_id;       /* This is the Front End object ID to be
					** used as the source for translation
					** into a DBMS object ID.  This will
					** only be used for repeat QP's or DBP
					** QP's that are shareable, and is
					** only used by the trans-or-define,
					** just-trans, and gethandle opcodes.
					*/
    i4		    qsf_t_or_d;		/* This tells the caller of the
					** QSO_TRANS_OR_DEFINE opcode whether
					** a translation or definition happened.
					** Only touched by TRANS_OR_DEFINE,
					** or by JUST_TRANS -- some psf code
					** depends on that (improperly)
					*/
#define	    QSO_WASTRANSLATED	    100
#define	    QSO_WASDEFINED	    101
#define	    QSO_WASTHERE	    200
#define	    QSO_WASCREATED	    201

    PTR             qsf_piece;          /* Ptr to memory piece allocated for
					** an object.  This is always
                                        ** guaranteed to be aligned on a
                                        ** sizeof(ALIGN_RESTRICT) byte boundary.
                                        ** This is returned by QSO_PALLOC. 
					*/
    PTR             qsf_root;           /* Root address of an object.  This
					** is supplied for QSO_SET_ROOT and
					** returned by QSO_ACCESS and QSO_INFO.
					*/
    i4		    qsf_sz_piece;       /* Requested size of memory piece to
					** allocate for an object.  This is
					** supplied for QSO_PALLOC, and can
					** conceivably be larger than the block
					** size for the object being created.
					*/
    i4              qsf_lk_state;       /* Current lock state of an object.
                                        ** This is returned by QSO_INFO,
                                        ** supplied to QSO_LOCK, and will always
                                        ** be one of the following: 
					*/
#define                 QSO_FREE      0	    /* Object is not locked. */
#define                 QSO_SHLOCK    1	    /* Object is locked for sharing. */
#define                 QSO_EXLOCK    2	    /* Object is exclusively locked. */

    i4              qsf_lk_cur;         /* # locks currently on an object.
					** This is returned by QSO_INFO.
					*/
    i4              qsf_lk_id;          /* When an object is exclusively locked
                                        ** (via QSO_CREATE or QSO_LOCK), QSF
                                        ** hands out a lock id.  This lock id
                                        ** will be required by QSO_UNLOCK before
                                        ** the object will be unlocked, as well
                                        ** as QSO_PALLOC, QSO_SETROOT,
                                        ** QSO_RENAME, and QSO_DESTROY.  THIS
                                        ** DOES NOT APPLY TO SHARED LOCKING.
                                        ** This is returned by QSO_CREATE and
                                        ** QSO_LOCK (when locking exclusively),
                                        ** and supplied to QSO_UNLOCK (when
                                        ** unlocking an exclusive lock),
                                        ** QSO_PALLOC, QSO_SETROOT, QSO_RENAME,
                                        ** and QSO_DESTROY. 
					*/
    i4              qsf_force;          /* Only used at QSR_SHUTDOWN time.  If
					** this is supplied as non-zero, then
					** QSF will attempt a forced shutdown
					** by ignoring the fact that there might
					** still be existing objects and/or
					** active QSF sessions.
					*/
    DB_ERROR        qsf_error;          /* Standard error structure returned
					** by all opcodes.
					*/
#define                 E_QS_MASK  			( DB_QSF_ID << 16 )
#define                 E_QS0000_OK			            (0x0000L)
#define                 E_QS0001_NOMEM			(E_QS_MASK + 0x0001L)
#define                 E_QS0002_SEMINIT		(E_QS_MASK + 0x0002L)
#define                 E_QS0003_OP_CODE		(E_QS_MASK + 0x0003L)
#define                 E_QS0004_SEMRELEASE		(E_QS_MASK + 0x0004L)
#define                 E_QS0005_ACTIVE_OBJECTS		(E_QS_MASK + 0x0005L)
#define                 E_QS0006_ACTIVE_SESSIONS	(E_QS_MASK + 0x0006L)
#define                 E_QS0007_MEMPOOL_RELEASE	(E_QS_MASK + 0x0007L)
#define                 E_QS0008_SEMWAIT		(E_QS_MASK + 0x0008L)
#define			E_QS0009_BAD_OBJTYPE		(E_QS_MASK + 0x0009L)
#define			E_QS000A_OBJ_ALREADY_EXISTS	(E_QS_MASK + 0x000AL)
#define			E_QS000B_BAD_OBJNAME		(E_QS_MASK + 0x000BL)
#define			E_QS000C_ULM_ERROR  		(E_QS_MASK + 0x000CL)
#define			E_QS000D_CORRUPT    		(E_QS_MASK + 0x000DL)
#define                 E_QS000E_SEMFREE		(E_QS_MASK + 0x000EL)
#define                 E_QS000F_BAD_HANDLE             (E_QS_MASK + 0x000FL)
#define                 E_QS0010_NO_EXLOCK              (E_QS_MASK + 0x0010L)
#define                 E_QS0011_ULM_STREAM_NOT_CLOSED  (E_QS_MASK + 0x0011L)
#define                 E_QS0012_OBJ_NOT_LOCKED		(E_QS_MASK + 0x0012L)
#define                 E_QS0013_BAD_LOCKID             (E_QS_MASK + 0x0013L)
#define                 E_QS0014_EXLOCK                 (E_QS_MASK + 0x0014L)
#define                 E_QS0015_SHLOCK                 (E_QS_MASK + 0x0015L)
#define                 E_QS0016_BAD_LOCKSTATE          (E_QS_MASK + 0x0016L)
#define                 E_QS0017_BAD_REQUEST_BLOCK      (E_QS_MASK + 0x0017L)
#define			E_QS0018_BAD_PIECE_SIZE		(E_QS_MASK + 0x0018L)
#define			E_QS0019_UNKNOWN_OBJ		(E_QS_MASK + 0x0019L)
#define			E_QS001A_NOT_STARTED		(E_QS_MASK + 0x001AL)
#define			E_QS001B_NO_SESSION_CB		(E_QS_MASK + 0x001BL)
#define			E_QS001C_EXTRA_OBJECT		(E_QS_MASK + 0x001CL)
#define			E_QS001D_NOT_OWNER		(E_QS_MASK + 0x001DL)
#define			E_QS001E_ORPHANED_OBJ		(E_QS_MASK + 0x001EL)
#define			E_QS001F_TOO_MANY_ORPHANS	(E_QS_MASK + 0x001FL)
#define			E_QS0020_QS_INIT_MO_ERROR	(E_QS_MASK + 0x0020L)

#define                 E_QS0500_INFO_REQ          	(E_QS_MASK + 0x0500L)

#define                 E_QS9001_NOT_YET_AVAILABLE 	(E_QS_MASK + 0x9001L)
#define                 E_QS9002_UNEXPECTED_EXCEPTION	(E_QS_MASK + 0x9002L)
#define                 E_QS9999_INTERNAL_ERROR    	(E_QS_MASK + 0x9999L)

    i4              qsf_scb_sz;         /* This is only used by QSR_STARTUP
                                        ** who returns it.  It represents the
                                        ** number of bytes required for a QSF
                                        ** session control block.
                                        */
    i4              qsf_max_active_ses; /* Max number of active sessions allowed
					** for the server.  This is only used by
					** QSR_STARTUP, where it is supplied.
					** Even here, however, it is only looked
                                        ** at if .qsf_pool_sz is zero or
                                        ** negative.  If .qsf_max_active_ses is
                                        ** a positive number, then QSR_STARTUP
                                        ** will calculate the memory pool size
                                        ** based on this number.  If, however,
                                        ** .qsf_max_active_ses is NOT a positive
                                        ** number, QSR_STARTUP will use an
                                        ** internal constant as the size of the
                                        ** memory pool.  Basically, then, we
                                        ** have three ways of setting the memory
                                        ** pool size for QSF; in order of
					** importance:
					**	1 - If .qsf_pool_sz is positive,
					**	    use that as the size.
					**	2 - If .qsf_max_active_ses is
					**	    positive, calculate the size
					**	    based on that.
					**	3 - Use an internal constant as
					**	    the size.
					*/
    i4		    qsf_qsscb_offset;	/* offset from SCB of qsscb (used
					** to get session's QSF_CB in 
					** qsf_session().
					*/
    SIZE_TYPE       qsf_pool_sz;        /* Number of bytes to allocate for QSF's
					** memory pool.  This is only used by
					** QSR_STARTUP, where it is supplied.
					** If this number is zero or negative,
					** it will be ignored, and
					** .qsf_max_active_ses will be looked
					** at.
					*/
    i4		    qsf_flags;		
#define                 QSO_REPEATED_OBJ      0x001	/* repeat query */
#define                 QSO_DBP_OBJ           0x002	/* database procedure */
}   QSF_RCB;

/*
** Function prototypes for externally visible functions
*/

FUNC_EXTERN DB_STATUS	qsf_call( i4 qsf_operation, QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qsf_trace( DB_DEBUG_CB *qsf_debug_cb );
