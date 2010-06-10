 /*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iilibq.h - Global definition for LIBQ (the routines that interact
**		     with the DBMS).
**
**  Description:
**	This file describes the global structures that has all the information
**	needed by LIBQ to maintain a consistent state with the DBMS.  It also
**	supports old global names that may have been [ab]used outside of LIBQ
**	by other front ends and the form system.
**
**  Notes:
** 	1. If used properly (with GCA) this file should be included after
**	   gca.h and iicgc.h.
** 	2. If used properly (with Row Descriptors) this file should be
**	   included after iirowdesc.h.
**
**  History:
**	05-oct-1986	- Written from a collection of 5.0 globals that were
**			  defined in iiglobals.h. (ncg)
**	10-aug-1987	- Modified to start using GCA. (ncg)
**	21-oct-1987 (peter)
**		Take out sid, and add stime to II_ERR_DESC.
**	25-feb-1988	- Added ii_lq_abflags to IIlbqcb structure
**	04-may-1988	- Modified and extended some structures for the
**			  processing of database procedures. (neil)
**	29-sep-1988	- Modified some cursor structures to allow
**			  dynamically named cursors. (neil = ncg)
**	13-mar-1989	- Added ii_lq_parm to keep track of with
**			  clause parameters on connect statement.
**	19-may-1989	- Created II_GLB_CB data structure by extracting
**			  elements from II_LBQ_CB and II_ERR_DESC.  This is
**			  to keep session-specific information separate for
**			  multiple database connections in an application. (bjb)
**	25-jun-1989	- Moved SQLCA and dml back to II_LBQ_CB from II_GLB_CB
**			  so that outer query still has access to sqlca and
**			  knows qry lang is SQL even after nested query has
**			  released SQLCA and reset dml. 
**	28-sep-1989	- Converted FE cursor to full cursor id with original
**			  FE name to allow variable cursor names in GW's (neil)
**	18-nov-1989	- Updated for Phoenix/Alerters (bjb)
**	26-mar-1990	- Added more state for error tracking of session
**			  switching within program loops. Bug 9159. (barbara)
**	05-apr-1990	(barbara)
**	    Added to II_ER_DESC field ii_er_submsg for saving "subsidiary"
**	    error messages.
**	07-may-1990	- Added II_LBQ_CB ii_lq_curqry flag (II_Q_DISCON) so
**			  that EVENTS are not processed when disconnecting.(bjb)
**	27-apr-1990	(neil)
**	    Cursor performance project.  Added some pre-fetch fields to the
**	    cursor states, cursor trace flag and new query flag to pick up
**	    alternate response block.
**	14-jun-1990	(barbara)
**	    Added flag to IIglbcb to inform whether or not program should
**	    quit on association failures; also IIlbqcb flag to inform
**	    whether association failure has occurred. (bug 21832)
**	27-july-1990 (barbara)
**	    Added field to cursor state for inquire_ingres.
**      28-aug-1990 (Joe)
**          Added ii_lq_cdata and ii_lq_cid to support multiple session
**          in UI.  The interface is provided by the routines
**          IILQpsdPutSessionData and IILQgsdGetSessionData.  The interface
**          is somewhat generalized to support several clients, but at
**          this point only one client can store data with a session.
**      13-sep-1990 (joe)
**	    Added II_L_NAMES so that 4GL cursors can get result column names.
**	21-sep-1990	(barbara)
**	    Added IIEMB_SET structure field to II_GLB_CB.  It contains
**	    information about II_EMBED_SET settings.
**	15-oct-1990	(barbara)
**	    Flag warnings in ii_lq_curqry so that select/retrieve loops can
**	    continue on errors that are actually warnings.  (Bug 9398 and
**	    bug 20010)
**	03-dec-1990	(barbara)
**	    To fix bug 9160 (INQUIRE_INGRES sees errors belonging to other
**	    session) moved II_ERR_DESC into the II_LBQ_CB.  Some fields
**	    that were in the II_ERR_DESC, e.g. ii_er_prt and ii_er_trace
**	    remain in the II_GLB_CB (but their names changed to ii_gl_seterr
**	    and ii_gl_trace)
**	13-mar-1991	(teresal)
**	    Add support for saving the last query text. 
**	13-mar-1991	(barbara)
**	    To trap trace messages received from the server, added ii_gl_svrtrc
**	    field and II_G_SVRTRC mask to II_GLB_CB; also added PRTTRC mask
**	    to represent II_EMBED_SET 'printtrace' setting.
**      20-mar-1991     (teresal)
**          Add logical key structure (IILQ_LOGKEY) for support of
**          INQUIRE_INGRES with TABLE_KEY and OBJECT_KEY objects.
**          Logical key values are read from the response block after
**          an insert, stored in the local LIBQ control block (II_LBQ_CB),
**          and the user can then inquire about this value.
**	22-mar-1991	(barbara)
**	    Added pointer field for COPYHANDLER.
**	28-mar-1991	(teresal)
**	    Fixed typo - added a ';' after ii_lq_logkey.
**	16-apr-1991 	(teresal)
**	    Add flag to event structure to tell if the event has been
**	    consumed.
**	25-apr-1991	(teresal)
**	    Clean up file integration mess - file didn't get merged
**	    correctly.
**	25-jul-1991	(teresal)
**	    Update comments to reflect new DBEVENT syntax.
**	01-nov-1991	(kathryn)
**	    Add structure IILQ_TRACE - Replace individual trace info variables
**	    in global LIBQ control block (II_GLB_CB) with three IILQ_TRACE
**	    structures for PRINTQRY, PRINTGCA and SERVER trace info handling.
**	24-mar-92 (leighb) DeskTop Porting Change:
**	    added function prototype for IIqid_find().
**	6-apr-92 (fraser)
**	    Enclosed function prototype in ifdef CL_HAS_PROTOS
**	3-nov-92 (larrym)
**	    Added SQLCODE and SQLSTATE support.  Added pointer to ANSI user's
**	    SQLCODE to IIlbqcb.  Added pointer to ANSI user's SQLSTATE to
**	    IIgblcb, as there is only one per process.  Created the ANSI
**	    Diagnostic Area, although for now it only has RETURNED_SQLSTATE.
**	09-nov-1992 (kathryn)
**	    Add function prototypes for new BLOB handling functions and
**	    define IIDB_LONGTYPE_MACRO.
**	09-nov-1992 (kathryn)
**	    Correct prototype for IILQlvg_LoValueGet.
**      14-nov-1992 (nandak, mani)
**         Added XA transaction id support
**	09-dec-1992 (teresal)
**	   Moved ADF control block from the global structure to the
**	   session specific structure because of decimal changes.
**	   The protocol level is now stored in the ADF control block
**	   (to determine decimal support) and that can vary with each session.
**	15-jan-1993 (kathryn)
**	   Added IIDB_VARTYPE_MACRO and IIDB_CHARTYPE_MACRO.
**	26-jan-1993 (teresal)
**	   Add prototype for new function that creates multiple ADF control 
**	   blocks.
**	24-mar-93 (fraser)
**	    Added tags to structure definitions so that Microsoft prototyping
**	    tool will work properly.
**	26-apr-1993 (kathryn)
**	    Add prototype for IILQlcc_LoCbClear. Changed prototypes.
**	15-jun-1993 (larrym)
**	    renamed IILQssSessionSwitch to IILQfsSessionHandle to more 
**	    accurately reflect what it does (and because I created a new
**	    function to actually do the switching and I needed the name).
**	18-jun-1993 (mgw)
**	    Add IIDT_LONGTYPE_MACRO(), analogous to IIDB_LONGTYPE_MACRO(),
**	    but for when you don't have a DB_DATA_VALUE handy, just the
**	    II_DT_ID.
**	26-jul-1993 (kathryn)
**	    Added IIDB_BYTETYPE_MACRO macro. Added DB_VBYTE_TYPE as DB_VARTYPE
**	    and DB_LBYTE_TYPE as DB_LONGTYPE.
**	23-aug-1993 (larrym)
**	    Added owner.procedure functionality; added types IIDB_PROC_ID
**	    and II_PROC_IDS; and added element to the session control block
**	    II_LBQ_CB.
**	20-feb-1995 (shero03)
**	    Bug #65565
**	    Support long UserDataTypes 
**      12-Dec-95 (fanra01)
**          Added definitions for referencing data in DLLs on windows NT from
**          code built for static libraries.
**      06-Feb-96 (fanra01)
**          Changed __USE_LIB__ to IMPORT_DLL_DATA.
**      23-apr-96 (chech02)
**          Added function prototypes for windows 3.1 port.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	09-Jan-97 (mcgem01)
**	    The use of DLL's on NT requires that IIgca_cb be GLOBALDLLREF'ed.
**	24-mar-1999 (somsa01)
**	    Removed "#ifdef WIN16" and cleaned up some function declarations.
**	01-apr-1999 (somsa01)
**	    Removed definition of IIexQRead(), as it is a static function in
**	    iiexec.c .
**	01-apr-1999 (somsa01)
**	    Removed function prototypes that were causing headaches when
**	    building on UNIX platforms, and readjusted some other
**	    prototypes.
**	14-apr-1999 (somsa01)
**	    Replaced nat with i4, caused by previous cross-integration.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-oct-2000 (stial01)
**          Added II_Q_LO_PUT_QTXT
**      07-nov-2000 (stial01)
**          Removed II_Q_LO_PUT_QTXT
**	20-feb-2001 (gupsh01)
**	    Added support for long nvarchar, added IIDB_UNICODETYPE_MACRO.
**      12-jun-2001
**          Added {} to II_DB_SCAL_MACRO, added DB_NVCHR_TYPE to 
**          IIDB_VARTYPE_MACRO
**      28-mar-2003 (hanch04/somsa01)
**          Changed return type of IILQdbl() and IILQint() to "void *".
**	    Included prototypes for iierror.c
**      24-jan-2004 (stial01)
**          Changed prototypes for IILQlrs_*, IILQlws_* (b113792)
**      16-jun-2005 (gupsh01)
**	    Added IILQucolinit and IItm_adfucolset.
**	08-Mar-2007 (kiria01) b117892
**	    Allow for DB_INT_TYPE to be defined as an enum now
**	10-Jun-2008 (lunbr01)  bug 120432
**	    Added II_G_NO_COMP value for ii_gl_flags in II_GLB_CB to
**	    suppress varchar compression if variable II_VCH_COMPRESS_ON=N.
**  16-Jun-2009 (thich01)
**      Treat GEOM type the same as LBYTE.
**  20-Aug-2009 (thich01)
**      Treat all spatial types the same as LBYTE.
**	20-Aug-2009 (drivi01)
**	    Add a prototype for IIputdomio in iisetdom.c.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

# include	<er.h>
# include	<mu.h>

/*
** The file iirowdesc.h must be included, though since descriptors are
** used only by RETRIEVE and CURSOR managers (and the Terminal Monitor) we
** may have to include for the general users of iilibq.h in order to define
** the ROW_DESC descriptor (it is not a pointer).
*/
#ifndef RD_DEFAULT
#	include <iirowdsc.h>
#endif  /* RD_DEFAULT */

/*
** For the same reasons as the above, redefine the IICGC_DESC structure.
** (Note that the first field in the IICGC_DESC is a i4.)  IICGC_VQRY
** is one of the constants defined by the header file.  Also IICGC_ALTRD.
*/
#ifndef IICGC_VQRY
#	define  IICGC_DESC	i4
#	define  IICGC_ALTRD	i4
#endif  /* IICGC_VQRY */

/*
** For the same reasons as the above, redefine the DB_EMBEDDED_DATA structure
** to its first field's type (ed_type is an i2).
** Redefine the GCA_ID structure to a i4 (its first field type).
*/
#if !defined(DB_INT_TYPE) && !defined(DB_DT_ID_MACRO)
# 	define	DB_EMBEDDED_DATA	i2
#endif  /* DB_INT_TYPE */

#ifndef GCA_MAXNAME
# 	define	GCA_ID			i4
#       define  GCA_MAXNAME             0
#endif  /* GCA_MAXNAME */

/* And for the same reasons, define ADF_CB as in adf.h */
#ifndef ADF_USER_ERROR
#	define	ADF_CB			i4
#endif  /* ADF_USER_ERROR */

/* And for the same reasons, define ADD_LOW_CLSOBJ as in add.h */
#ifndef ADD_LOW_CLSOBJ
#define ADD_LOW_CLSOBJ			8192
#endif /* ADD_LOW_CLSOBJ */

/*
** And for users who do not include dbms.h...
*/
#ifndef	DB_SQLSTATE_STRING_LEN
#define         DB_SQLSTATE_STRING_LEN     5
typedef  char   DB_SQLSTATE[DB_SQLSTATE_STRING_LEN];
#endif	/* DB_SQLSTATE_STRING_LEN */

/*
** Globals
*/

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF PTR IIgca_cb;		/* GCA control block */
# else
GLOBALREF PTR	IIgca_cb;		/* GCA control block */
# endif


/*
** Structures that make up the LIBQ control block.
*/

/*}
+*  Name: II_RET_DESC - RETRIEVE statement descriptor.
**
**  Description:
**	This structure maintains all the information needed to process a 
**	RETRIEVE statement.  
**	  ii_rd_desc	- A descriptor saved for the current RETRIEVE.
**	  ii_rd_colnum	- Current column being retrieved (is used as index
**			  into the above descriptor and the PARAM one).
**	  ii_rd_param	- A descriptor describing host variables used with a 
**			  PARAM RETRIEVE.  Because of the way IIparret and
**			  IIgettup are called the descriptor (with the variable
**			  addresses) must be saved between calls.  This
**			  descriptor includes all variables used with the
**			  PARAM target list including null indicators.
**	  ii_rd_pnum	- Number of DB_EMBEDDED_DATA vals allocated (as
**			  an array).  If ii_rd_pnum < number of result cols,
**			  current vals are freed and new space is allocated.
**	  ii_rd_patts	- Number of result variables in the above descriptor
**			  that are for result columns.  ii_rd_patts is always
**			  <= ii_rd_pnum.
**	  ii_rd_ptrans	- Translation function for PARAM host variables that
**			  must be converted on the way out to the program.
**	  ii_rd_flags	- Mask for certain states during a RETRIEVE.
-*
**  History:
**	05-oct-1986	- Written (ncg)
**	08-oct-1989	- Moved ii_rd_dml to IILBQCB struct.
*/
typedef struct s_II_RET_DESC {
    ROW_DESC	ii_rd_desc;		/* Row descriptor */
    i4		ii_rd_colnum;		/* Current column number */
# define	  II_R_NOCOLS	-1	  /* Before retrieving any columns
					  ** the current column number is
					  ** set to NOCOLS so that IInextget
					  ** can trap "mismatching column"
					  ** errors */
    DB_EMBEDDED_DATA
		*ii_rd_param;		/* PARAM variable descriptor */
    i4		ii_rd_pnum;		/* PARAM variable count */
    i4		ii_rd_patts;		/* PARAM result variable count (does
					** not include null indicators) */
    i4		(*ii_rd_ptrans)(); 	/* PARAM translation function */
    i4		ii_rd_flags;		/* Flags for RETRIEVE state */
# define	  II_R_DEFAULT	0x000	  /* Default state */
# define	  II_R_NESTERR	0x001	  /* Error occurred during RETRIEVE
					  ** particular to the RETRIEVE */
# define	  II_R_INITERR	0x002	  /* Error during initialization
					  ** allows the call to IIerrtest
					  ** to be dropped.  IIflush checks
					  ** this flag.  See optional codegen
					  ** in IIretinit for ESQL. */
} II_RET_DESC;

/*}
+*  Name: IIDB_PROC_ID - Procedure id.
**
**  Description:
**      This typedef is modeled after the new GCA1_INVPROC message type. It
**      contains a GCA_ID (two nats and a name) followed by an owner
**      name (also a name).  When a procedure is executed for the first
**      time, the server generates a query plan, does all of its validation
**      and then executes the procedure.  The server saves the query plan
**      and tags it with an ID which it returns it to us in a GCA_RETPROC
**      message.  Subsequent invokations use only the ID which saves all
**      this overhead.
**
**  History:
**      15-jun-1993 (larrym)
**          written.
*/

typedef struct s_IIDB_PROC_ID {
    GCA_ID                      gca_id_proc;    /* two nat's and a name */
    char                        gca_proc_own[GCA_MAXNAME+1];
    struct s_IIDB_PROC_ID       *ii_pi_next;
}       IIDB_PROC_ID;

typedef struct s_II_PROC_IDS {
    IIDB_PROC_ID        *ii_pi_defined;         /* List of defined proc ids */
    IIDB_PROC_ID        *ii_pi_pool;            /* Pool of proc id nodes */
    u_i4               ii_pi_tag;              /* Allocation tag */
} II_PROC_IDS;

/*}
+*  Name: IIDB_QRY_ID - Query/Cursor/Procedure id.
**
**  Description:
** 	A query id has 2 i4's followed by a blank padded GCA_MAXNAME name.
** 	In LIBQ, in order to generalize and to use it for query id's too, we
** 	use the name IIDB_QRY_ID.
-*
**  History:
**	27-oct-1986	- Written (ncg)
**	22-jun-1988	- Modified for GCA_ID which is possibly large than 
**			  an INGRES cursor id. (ncg)
*/
typedef	GCA_ID	IIDB_QRY_ID;





/*}
+*  Name: II_QRY_ELM - Element in the list of query id's maintained by LIBQ.
**
**  Description:
**	When defining a REPEAT query the BE returns a unique DB-chosen
**	id.  This id must be saved together with the FE-preprocessor-generated
**	id in order for subsequent executions to send the correct DB-id.
**	On each execution, LIBQ takes the statically defined FE-id and searches
**	for matching DB-id.  It is this id that it sends.
**
**	Note that sometimes a REPEAT query may be undefined by the BE without
**	the FE knowing it.  Consequently the FE should make sure that when
**	it enters a DB-id for an FE-id it first searches the FE-ids to see
**	if it is already there.
**	  ii_qi_next	- Next element int the "defined" or "pool" list.
**	  ii_qi_rep	- TRUE/FALSE - Repeat query, procedure id.
**	  ii_qi_1id	- First id generated by preprocessor.
**	  ii_qi_2id	- Second id generated by preprocessor.
**	  ii_qi_dbid	- DB-id returned from DBMS (2 i4's and a name)
-*
**  History:
**	23-oct-1986	- Written (ncg)
**	03-may-1988	- Modified to store procedure ids too (ncg)
*/

typedef struct _iiqid {
    struct _iiqid	*ii_qi_next;	/* Next query id in the list */
    bool		ii_qi_rep;	/* Repeat or procedure id */
    i4			ii_qi_1id;	/* FE id1 - note a "nat" as generated */
    i4			ii_qi_2id;	/* FE id2 -   by preprocessors        */
    IIDB_QRY_ID		ii_qi_dbid;	/* DB id (i4, i4, name) */
} II_QRY_ELM;




/*}
+*  Name: II_QRY_IDS - List (and pool) of query ids.
**
**  Description:
**	This list is made up of a pool (free list) of yet unused query ids
**	and a list of defined query ids.  As mentioned above, there should
**	be at most one node in the defined list represented by any FE-id; if
**	there are more then we do not know which one defines the most recent
**	DB-defined id and we much assume either a stack or a queue.
**	  ii_qi_defined	- List of already defined query ids.
**	  ii_qi_pool	- Free list of query id nodes.
-*
**  History:
**	23-oct-1986	- Written (ncg)
**	4/91 (Mike S)	- Add ME tag
*/

typedef struct s_II_QRY_IDS {
    II_QRY_ELM		*ii_qi_defined;		/* List of defined query ids */
    II_QRY_ELM		*ii_qi_pool;		/* Pool of query id nodes */
    u_i4		ii_qi_tag;		/* Allocation tag */
} II_QRY_IDS;




/*{
+*  Name: IICS_STATE - Runtime cursor state.
**
**  Description:
**	One of these structures is created for each open cursor.  It 
**	maintains information about the cursor needed by the FE cursor
**	support routines.  Cursor states are looked up in a list by their 
**	"FE" id.  These id's are generated by the preprocessor as
**	arguments to most LIBQ cursor routines.  The cursor state also
**	has a "DBMS" id, which is used at runtime to send cursor queries 
**	to the DBMS.
**	  css_next	- Points to the next cursor on the FE's list
**			  of open cursors.
**	  css_fename	- Front end cursor id (two i4's and a name)
**	  css_name	- DBMS cursor id (two i4's and a name)
**			  The name is not necessarily the same cursor name
**			  used by the program.
**	  css_flags	- Identifies different types/states of cursors.
**	  css_colcount	- The number of columns fetched by LIBQ so far 
**			  within current RETRIEVE CURSOR statement.
**	  css_cdesc	- Row description of type and length of each column 
**			  to be processed by this cursor.  This is needed 
**			  to set up for each cursor retrieve.
**	  css_pfstream	- Pre-fetch buffer stream.
-*	
**  History:
**	31-oct-1986 - written (bjb)
**	29-sep-1988 - added css_flags for dynamically named cursors (ncg)
**	28-sep-1989 - converted FE cursor to DBMS full cursor id (ncg)
**	27-apr-1990 - add css_pfstream for cursor pre-fetching (ncg)
*/

typedef struct IICS_{
    struct IICS_	*css_next;	/* Linked list of cursor states */
    IIDB_QRY_ID		css_fename;	/* FE preproc-generated cursor id */
    IIDB_QRY_ID		css_name;	/* DBMS cursor id (name and 2 long's) */
    i4			css_flags;	/* Identifies type of cursor */
# define	  II_CS_DYNAMIC	0x001	/* Dynamically named cursor.  This
					** cursor requires look-up by name
					** rather than integers */
    i4			css_colcount;	/* Current column processed in FETCH */
    ROW_DESC		css_cdesc;	/* Row descriptor of col descs */
    IICGC_ALTRD		*css_pfstream;	/* Cursor pre-fetch alternate read
					** stream.  Used around cursor
					** FETCH statements.
					*/
} IICS_STATE;



/*{
+*  Name: IICSR_STAT - List pointer and current cursor pointer
**
**  Description:
**	The list is of all the cursors that the FE thinks are (is?) open.
**	The current cursor is the one being used in the current query (may 
**	be NULL).
-*
**  History:
**	31-oct-1986 - written (bjb)
**	29-sep-1988 - added field to keep track of initial front-end id for
**		      dynamically named cursors (ncg)
**	27-apr-1990 - add css_pfrows for cursor pre-fetching (ncg)
**	27-jul-1990 (barbara)
**	    Add css_pfcgrows to keep track of how many rows are actually
**	    being fetched.
**	03-dec-1990 (barbara)
*/

typedef struct s_IICSR_STAT {
    IICS_STATE		*css_cur;	/* Current cursor */
    IICS_STATE		*css_first;	/* First cursor on list */
    i4			css_dynmax;	/* Highwater number for dynamic ids */
    i4			css_pfrows;	/* # rows to pre-fetch for the next
					** cursor - bound to cursor when it is
					** opened.  If 0 then the requested
					** number of rows is determined
					** dynamically.  If user sets to > 0
					** (IIeqstio) then may override default.
					*/
    i4			css_pfcgrows;	/* # rows actually being fetched by
					** cgc routines.  Useful for
					** INQUIRE_INGRES PREFETCHROWS.
					*/
} IICSR_STAT;

/*}
+*  Name: II_ERR_DESC - Error descriptor.
**
**  Description:
**	This structure maintains the error information local to LIBQ.
**	Most errors (at least their numbers) go through LIBQ and are processed
**	here.
**	  ii_er_num	- Current error number - reset with each query.  In
**			  SQL this is the generic error number, while in QUEL
**			  this is the DBMS-specific error number.
**	  ii_er_other	- Associated error number.  If SQL this is the local
**			  DBMS-specific error number, if QUEL this is the
**			  generic error number:
**			      		SQL		QUEL
**					--------------------
**			  num:		generic		INGRES-specific
**			  other:	DBMS-specific	generic
**	  ii_er_estat	- Current error status - only valid if er_num is set.
**			  Warning or Error.
**	  ii_er_snum	- Saved error number.
**	  ii_er_sbuflen - Size of currently allocated error buffer space.
**	  ii_er_smsg	  The case where the error message is not displayed
**			  (such as COBOL or SQL cases) then this is where the
**			  error data is buffered.  This is the DBMS-specific
**			  (or LIBQ) error text already formatted.
**	  ii_er_submsg	- Saved "subsidiary" error message.  Used to save low
**			  level GCA err messages.  After high level LIBQ error
**			  occurs, low level message is concatenated.
**	  ii_er_mnum	- User message number (database procedures).
**	  ii_er_mbuflen - Size of currently allocated user message space.
**	  ii_er_msg
**        ii_er_flags   - Flags state of error processing.  User may require
**                        that the error be saved (SQL) to be printed
**                        later.
-*
**  History:
**	05-oct-1986	- Written (ncg)
**	21-dec-1987	- Added trace function. (ncg)
**	11-may-1988	- Added fields for saving user messages. (ncg)
**	09-dec-1988	- Added "other" error and removed "stime" field
**			  as text is preformatted. (ncg)
**	14-apr-1989	- Added constants for ii_er_flags (bjb)
**	05-apr-1990	(barbara)
**	    New field, ii_er_submsg, to save error text for "subsidiary"
**	    errors.  Certain low-level GCA errors were getting automatically
**	    printed.  Now we save them and append their text to the "higher
**	    level" error mesasge.
**	03-dec-1990 (barbara)
**	    This structure is moving back to be part of the II_LBQ_CB to
**	    fix bug 9160.  A couple of its former fields (seterr and trace
**	    function pointers) now reside in the II_GLB_CB.
**	15-mar-1991 (kathryn)
**	    Add II_E_FRSERR mask for ii_er_flags.  
**	21-mar-1991 (kathryn)
**	    Removed II_E_FRSERR (Added for user defined handlers) - not needed.
*/

typedef struct s_II_ERR_DESC {
    i4	ii_er_num;	/* Current error number */
    i4	ii_er_other;	/* Local DBMS-specific error number */
    i4		ii_er_estat;	/* Current error status */
    i4	ii_er_snum;	/* Saved error number (COBOL or SQL) */
    i4		ii_er_sbuflen;	/* Size of allocated error text buffer */
    char	*ii_er_smsg;	/* Saved error message */
    char	*ii_er_submsg;	/* Saved "subsidiary" error message */
    i4	ii_er_mnum;	/* User message number */
    i4		ii_er_mbuflen;	/* Size of allocated user message buffer */
    char	*ii_er_msg;	/* Saved user message */
    i4          ii_er_flags;    /* Flag state of error */
# define          II_E_DEFAULT  0x000   /* Deafult state */
# define          II_E_PRT      0x001   /* Inside user's error handler. This
					** flag avoids calling the error handler                                        ** recursively from with LIBQ */
# define          II_E_SQLPRT   0x002   /* Print ESQL errors to stdout
					** (useful for debugging */
# define          II_E_DBMS     0x004   /* Make local DBMS errors the default */
# define          II_E_GENERIC  0x008   /* Leave generic errors as default */
# define	  II_E_FRSERR   0x010	/* FRS err - dont call user handler */
} II_ERR_DESC;


/*{
+*  Name: IISQ_PARMS - Parameters from WITH clause on CONNECT statement.
**
**  Description:
**	WITH clause parameters are buffered and sent to the dbms/gateway
**	as a single message parameter.
-*
**  History:
**	11-mar-1989 - written (bjb)
**      14-nov-1992 (nandak, mani)
**         Added XA transaction id support
*/

typedef struct s_IISQ_PARMS {
    char	*ii_sp_buf;	/* Saved WITH clause parameters */
    i4		ii_sp_flags;	/* State of saved parameters */
# define	  II_PC_DEFAULT	0x000	/* Default state */
# define	  II_PC_XID	0x001	/* Processing transaction ids */
# define          II_PC_XA_XID  0x002   /* Processing XA transaction ids */
} IISQ_PARMS;

/*{
** Name: II_EVENT
**
** Description:
**	The event structure contains information about an event received
**	from the DBMS.  II_EVENT structures are kept on a list in LIBQ's
**	session control block  until another event is requested by the 
**	user with a GET DBEVENT statement or the event is consumed by an 
**	event handler (i.e., WHENEVER DBEVENT, an FRS ACTIVATE DBEVENT 
**	block or a user-defined event handler).  The user can request 
**	event information with the INQUIRE_SQL statement.
**
**	Note that the date element must be aligned.
**
** History:
**	18-nov-1989	(barbara)
**	    Written for Phoenix/Alerters.
*/

typedef struct IIEV_{
    struct IIEV_	*iie_next;		/* Next event on list */
    char		iie_name[DB_MAXNAME+1];	/* Event name */
    char		iie_owner[DB_MAXNAME+1];/* Event owner */
    char		iie_db[DB_MAXNAME+2];	/* Database(+2 for alignment) */
    DB_DATE		iie_time;		/* Timestamp */
    char		*iie_text;		/* Optional text */
} II_EVENT;

/*{
** Name: IILQ_EVENT - Information on event handling.
**
** Description:
**	This structure contains information pertaining to event handling
**	in LIBQ.  It also contains a list of events received from the dbms
**	but as yet unprocessed by the application.
**
** History:
**	18-nov-1989	(barbara)
**	    Written for Phoenix/Alerters.
**	16-apr-1991	(teresal)
**	    Add II_EV_CONSUMED flag to indicate the first event on the
**	    stack has been consumed by the system.  The event will be
**	    freed next time a "get event" is done, but meanwhile the event
**	    will "hang out" on the stack so the user can INQUIRE_INGRES
**	    on the event.
*/

typedef struct s_IILQ_EVENT {
    i4		ii_ev_flags;
# define	  II_EV_ASYNC	0x01	/* Async supported */
# define	  II_EV_DISP	0x02	/* Print events */
# define	  II_EV_CONSUMED 0x04	/* Event has been consumed */
# define	  II_EV_GETERR	0x08	/* Error in reading event */
    II_EVENT	*ii_ev_list;	/* Pointer to first event on list */
} IILQ_EVENT;

/*{
** Name: IILQ_LOGKEY - Information on logical keys.
**
** Description:
**      This structure contains information pertaining to logical keys.
**      Specifically, it contains the value of the logical key as
**      returned in the reponse block by the dbms.  The logical
**      key is stored as 16 bytes where the first 8 bytes are the
**      table key (if one exists) and the entire 16 byte is the
**      object key (if one exists).  One or both types of keys
**      can be stored here - the flag indicates what logical key
**      types were returned in the response block.
**
** History:
**      20-mar-1991     (teresal)
**          Written to support INQUIRE_INGRES with TABLE_KEY and
**          OBJECT_KEY feature.
*/

typedef struct s_IILQ_LOGKEY {
    i4          ii_lg_flags;            /* Type of logical key stored */
# define          II_LG_INIT    0x00    /* No logical Key */
# define          II_LG_OBJKEY  0x01    /* Object Key type */
# define          II_LG_TABKEY  0x02    /* Table Key type */

    char        ii_lg_key[DB_LEN_OBJ_LOGKEY]; /* Logical key value */
} IILQ_LOGKEY;

/*{
**  Name:  IILQ_LODATA -- Large Object Information structure.
**
**  Description:
**	This structure maintains the information about the large object
**	currently being processed.
**
**  ii_lo_flags   - Current status of large object transmission.
**		    II_LO_START - Processing new large object. Set when begin
**		    processing new LO - Header of ADP_PERIPHERAL
**		    structure has not been transmitted.
**		    II_LO_END - The entire Large Object has been processed
**		    Used to set DATAEND parameter of GET/PUT DATA stmts.
**		    II_LO_ISNULL - Large Object is null do not call handler.
**		    II_LO_PUTHDLR - In PUT datahandler.
**		    II_LO_GETHDLR - In GET datahandler.
**  ii_lo_seglen  - User specified length of current segment of LO.
**		    (PUT/GET DATA "SEGMENTLENGTH" parameter).
**  ii_lo_type    - Datatype of Large Object currently being processed.
**		    (INQUIRE_SQL columntype) valid only within DATAHANDLER.
**  ii_lo_name    - Column Name of Large Object currently being processed.
**		    (INQUIRE_SQL columnname) valid only within DATAHANDLER.
**  ii_lo_msgtype - Message type of large object (GCA_CDATA or GCA_TUPLES...).
**		    This specifies the type of GCA message the large object
**		    is being read/written to.
**
**  History:
**      01-nov-1992  (kathryn)  Written.
**  	08-nov-1992  (kathryn)
**	    Removed flag II_LO_MORE -- no longer needed.
**	01-mar-1993  (kathryn)
**	    Added ii_lo_message field. Contains the type identifier of the 
**	    GCA message (GCA_CDATA, GCA_TUPLES...) in which the large object 
**	    is being transmitted.
*/

typedef struct {
    i4	ii_lo_flags;
# define	II_LO_ISNULL   0x001    /* Is Large Object value NULL */
# define	II_LO_START    0x002    /* Start of large object processing */
# define	II_LO_END      0x004    /* We have processed the entire LO */
# define	II_LO_PUTHDLR  0x010    /* In PUT datahandler */
# define	II_LO_GETHDLR  0x020    /* In GET datahandler */
    i4		ii_lo_maxseglen;        /* User specified max segment length */
    i4		ii_lo_msgtype;		/* GCA message type containing LO */
    DB_DT_ID    ii_lo_datatype;         /* Datatype of large object */
    char	ii_lo_name[DB_MAXNAME + 1];
} IILQ_LODATA;

/*}
+*  Name: II_LBQ_CB - LIBQ control block for query processing.
**
**  Description:
**	This structure maintains information used for query processing.  It
**	should not be used for other purposes.  All query processing information
**	is maintained here.
**	  ii_lq_next	- Pointer to next II_LBQ_CB on list.  When a program
**			  has multiple database connections, a list of the
**			  sessions' state information is maintained.
**	  ii_lq_sid	- Session identifier.   If this is user specified 
**			  it must be non-zero.  A zero session id is reserved
**			  for a single-session connection.
**	  ii_lq_con_target
**			- full FE dbname (e.g. hostname::dbname/GW)
**	  ii_lq_con_name- connection name (used by SQL only).
**	  ii_lq_sqlca 	- Pointer to embedded program's SQLCA structure.
**			  If this field is a null pointer, the program
**			  did not include the SQLCA.  The SQLCA and the DML
**			  (next item) are items that are global to a
**			  program.  However, they are in the II_LBQ_CB so
**			  that the outer query of a nested query doesn't
**			  have the SQLCA and the DML "released" from under it.
**	  ii_lq_error	- Error descriptor contains information about current
**			  error message.
**	  ii_lq_dml	- DML language (QUEL/SQL).
**	  ii_lq_svdml	- Save query language of outer scope, eg SELECT loop
**                        database procedure loop, scope around calling user
**                        handlers.  We save this because a nested query might
**                        cause ii_lq_dml field to be reset.
**	  ii_lq_adf	- Control block to be used for all calls to ADF
**	  ii_lq_gca	- Control block to be used for all calls to GCA.
**	  ii_lq_retdesc	- Descriptor that maintains the state of the current
**			  RETRIEVE query loop.
**	  ii_lq_rowcount- The number of rows affected by the last query.  This
**			  value is returned with each query from the DBMS.  In
**			  the case of a RETRIEVE, the row count is maintained
**			  during the loop, so that the count is valid while
**			  the RETRIEVE is being processed.
**	  ii_lq_qrystat	- The status of the last query.  At the end of the
**			  query this status is returned from the DBMS.
**	  ii_lq_rproc	- Return status from procedure.  Default is zero.
**	  ii_lq_stamp[] - Commit Stamp returned from last query.  This is
**			  returned by the DBMSat the end of a commital.
**	  ii_lq_curqry	- The phases of the current query maintained by LIBQ.
**			  During the query this field is used to maintain the
**			  "current" state of the query.  This is useful in cases
**			  that track whether the query is valid (ie, no current
**			  DBMS session) or whether the query was broken in the
**			  middle (ie, bad variable type in the middle of an
**			  APPEND).  Note that this flag does not span more than
**			  one query - it's set at the start, used in the
**			  middle, and reset at the end.
**	  ii_lq_qryids	- List of query ids used by REPEAT query processing.
**	  ii_lq_csr     - List of open cursors.
**	  ii_lq_dbg	- Debug structure for query echoing on error.
**	  ii_lq_host	- Host programming language (at least at startup).
**	  ii_lq_flags	- Miscellaneous masks to flag certain events, 
**			  whether we are in a RETRIEVE loop.  Note that this
**			  flag may span more than one query (unlike
**			  ii_lq_curqry), and is not reset.
**	  ii_lq_prflags	- Masks to handle database procedure processing.
**	  ii_lq_parm	- WITH clause parameters for CONNECT statement.
**	  ii_lq_event	- Event descriptor containing list of events.
**	  ii_lq_cdata	- A PTR to a single client's data structure. The
**                        structure is private to the client, but is maintained
**                        with this session.
**	  ii_lq_cid	- The id of the client.  This id is an integer that
**                        the client provided on a call to
**                        IILQpsdPutSessionData.  At this point, the
**                        UI module is the only known client that uses
**                        these features.  In the future, if more than one
**                        client wants to maintain data with a session, a
**                        list of client data could be maintained, and the
**                        interface would not have to change.
**                        It is assumed that clients will use something
**                        like the class codes for the 2 letter directory
**                        code of the client (e.g. _UI_CLASS)
**        ii_lq_logkey	- Structure that stores logical key information.
**        ii_lq_sqlcdata- Pointer to an ANSI user's SQLCODE variable.  Set
**			  when we set the sqlca->sqlcode.
**	  ii_lq_lodata  - Structure that stores large object information.
-*
**  History:
**	05-oct-1986	- Created from a collection of 5.0 globals. (ncg)
**	04-may-1988	- Added some flags for processing procedures. (ncg)
**	30-aug-1988	- Added transaction state flag. (ncg)
**	09-sep-1988	- Added GCA trace structure. (ncg)
**	13-mar-1989	- Added ii_lq_parm to replace static in IIsqparm (bjb)
**	19-may-1989	- Moved ii_lq_adf, ii_lq_sqlca to II_GLB_CB, and
**			  added ii_lq_next and ii_lq_sid fields. (bjb)
**	25-may-1989	- Added flag for Dynamic SQL procedures. (ncg)
**	15-jun-1989	- Put SQLCA and DML back into this structure to
**			  because releasing it from II_GLB_CB at end of
**			  query caused strange error handling in nested
**			  queries (bjb)
**	07-sep-1989	- Added flag for dynamic SELECT statement. (bjb)
**	18-nov-1989	- Added ii_lq_event for event handling. (bjb)
**	08-dec-1989	- Save dml in here instead of in II_RET_DESC. (bjb)
**	30-apr-1990	- New flag (II_Q_PFRESP) to allow IIqry_end to
**			  process a pre-fetched response block. (ncg)
**	14-jun-1990	- New flag (II_L_ASSOCFAIL) to allow IIqry_start to
**			  give errors if connection has already failed.
**      28-aug-1990     - Added ii_lq_cdata and ii_lq_cid to maintain
**                        information a single client would like to maintain
**                        for a session.  These are used by
**                        IILQpsdPutSessionData and IILQgsdGetSessionData. (Joe)
**      13-sep-1990     - Added the II_L_NAMES constant as a new value
**                        for ii_lq_flags. (joe)
**	15-oct-1990	(barbara)
**	    Flag warnings in ii_lq_curqry so that select/retrieve loops can
**	    continue on errors that are actually warnings.  (Bug 9398 and
**	    bug 20010)
**	03-dec-1990 (barbara)
**	    Moved II_ERR_DESC (ii_lq_error) from II_GLB_CB to here.  This
**	    fixes bug 9160 (INQUIRE_INGRES errorstuff should be on a per
**	    session basis).
**	20-mar-1991 (teresal)
**          Added IILQ_LOGKEY structure to store logical key values.
**	25-oct-1992 (larrm)
**	    Added ii_lq_st_list to maintain list of SQLSTATES.
**      01-nov-1992 (kathryn)
**          Added IILQ_LODATA structure for Large Obeject information, and
**          defined II_L_DATAHDLR for ii_lq_flags indicating user defined
**          datahandler is in effect.
**	03-nov-1992 (larrym)
**	    removed ii_lq_st_list - decided not to use it.
**	    Added ii_lq_sqlcdata, address of ANIS user's SQLCODE.
**	16-nov-1992 (larrym)
**	    Added ii_lq_fe_dbname to maintain dbname as it's not maintained
**	    in the UI client data.
**	07-dec-1992 (teresal)
**	    Moved ADF control block from being global to being session
**	    specific.
**	21-dec-1992 (larrym)
**	    Changed ii_lq_fe_dbname to ii_lq_con_target to conform with 
**	    SQL syntax.
**	03-mar-1993 (larrym)
**	    Added ii_lq_con_name (connection name)
**	01-mar-1993 (kathryn)
**	    Define II_L_COPY to valid ii_lq_flags to specify that a COPY
**	    statement is being processed.
**	10-jun-1993 (kathryn)
**	    FIPS - Support of ANSI STD ESQL/C retrieval assigment rules.
**	    Define II_L_CHRPAD to specify blank-padding of fixed length
**	    DB_CHR_TYPE host variables.  
**	23-Aug-1993 (larrym)
**	    Added ii_lq_procids to support owner.procedure functionality.
**	18-nov-1993 (teresal)
**	    Add support for looking for EOS for ANSI SQL2.
**	25-apr-00 (inkdo01)
**	    Add II_P_RESROWS flag for row producing procs.
**       9-may-2000 (wanfr01)
**	    Bug 101250, INGCBT272.  Added II_L_ENDPROC to indicate that
**	    we are relooping due to a message received during ENDPROC
**          processing of a database procedure.
**       7-Sep-2004 (hanal04) Bug 48179 INGSRV2543
**          Added II_L_NO_ING_SET to flag CONNECT calls that have the -Z
**          flag specified. This is used to prevent ING_SET being applied to
**          a connection. sysmod makes a connection to iidbdb before
**          performing its work on the target DB. The connection to iidbdb
**          will use -Z to prevent ING_SET values generating unexpected
**          though legitimate errors.
**      27-oct-2004 (huazh01)
**          Add II_L_HELP. This bit is turned ON if users runs a 'help'
**          statement within an ISQL session.
**          b64679, INGCBT545.
**	11-oct-2003 (gupsh01)
**	    Added new parameters to set up unicode collation table.
*/

typedef struct IILQ_ {
			    /* Session information */
    struct IILQ_*ii_lq_next;	/* Next session state on list */
    i4		ii_lq_sid;	/* Session identifier */
    char	*ii_lq_con_target; /* full FE dbname */
    char	*ii_lq_con_name;/* connection name (can point to con target) */
    IISQLCA	*ii_lq_sqlca;	/* Pointer to user's SQLCA */
    II_ERR_DESC ii_lq_error;	/* Error descriptor */
    i4		ii_lq_dml;	/* DML language (QUEL/SQL) */
    i4		ii_lq_svdml;	/* Save DML */
    ADF_CB	*ii_lq_adf;	/* Control block for ADF */
			    /* Communication information */
    IICGC_DESC	*ii_lq_gca;	/* Control block for communications */
			    /* Query information */
    II_RET_DESC	ii_lq_retdesc;	/* Retrieve tuple/column descriptor */
    i4		ii_lq_rowcount;	/* Rows affected by last query (from DBMS) */
    i4		ii_lq_qrystat;	/* Status of last query (only from DBMS) */
    i4		ii_lq_rproc;	/* Return status of last procedure */
    i4		ii_lq_stamp[2];	/* Commit stamp from DBMS */
    i4		ii_lq_curqry;	/* Status of current query (from LIBQ) 
				** if grows > 16 bits then make an i4 */
# define	  II_Q_DEFAULT	0x000	/* Default state */
# define	  II_Q_INQRY	0x001	/* Currently writing a query (to check
					** if GCA initialization and GCA result
					** processing are required). */
# define	  II_Q_PARAM	0x002	/* PARAM query is being executed */
# define	  II_Q_QERR	0x004	/* Some error occurred in LIBQ (not the
					** DBMS). For example, bad variable
					** type,  query in RETRIEVE, no DBMS
					** session.  This flag marks that an 
					** error message has been sent and the
					** query should be ignored till the 
					** end. */
# define	  II_Q_DONE	0x008	/* Query has completed */
# define	  II_Q_NULIND	0x010	/* If a null indicator is used, and its
					** value is not DB_EMB_NUL, then do NOT
					** zero out the indicator pointer. This
					** is used by REPEAT query definition
					** and execution, where nullability must
					** be compiled into the type regardless
					** whether it implies the NULL value */
# define	  II_Q_1COMMA	0x020	/* These are used to determine if we  */
# define	  II_Q_2COMMA	0x040	/*       need to generate a comma before
					** the next put of a data value.  The
					** execution phase of dynamic SQL does
					** not include commas between the 
					** variables (so as to allow for future
					** extensions with blocks), so we need
					** to separate the marked binary values
					** with commas.  Logic is (before
					** put data):
					**  if (2comma) then	  ! later ones
					**	put comma;
					**  else if (1comma) then ! first one
					**	2comma;
					** This was done specifically for the
					**  EXECUTE s USING :v1, :v2, :v3;
					** case which we didn't want to show
					** through to the user             */
# define	  II_Q_DISCON	0x080	/* DISCONNECT/EXIT in process.  Don't
					** bother to process any events that
					** might come in.		   */
# define	  II_Q_PFRESP  0x0100	/* A response block has been pre-
					** fetched and only now is being
					** processed.  Even if not in a query
					** IIqry_end should still pick up the
					** response values.  */
# define	  II_Q_WARN    0x0200	/* Current error is a warning.  Used
					** in looping statements (SELECT,
					** EXEC PROC, to continue. */
    II_QRY_IDS	ii_lq_qryids;	/* List of query ids used by REPEAT query
				** processing */
    II_PROC_IDS  ii_lq_procids;         /* List of procedure ids */
    IIDB_PROC_ID *ii_lq_curproc;        /* Pointer to current procedre */
    IICSR_STAT   ii_lq_csr;      /* List of open cursors and current cursor */

			    /* Error information */
    PTR		ii_lq_dbg;	/* Error debug structure pointer */

			    /* Language information */
    i4		ii_lq_host;	/* Host language */

			    /* Miscellaneous status masks */
    i4	ii_lq_flags;	/* Status mask local to LIBQ query processing */
# define	II_L_CURRENT	0x0001	/* Current session */
# define	  II_L_RETRIEVE	0x0002	/* Within RETRIEVE body (to prevent
					** nested RETRIEVE loops) */
# define	  II_L_REPEAT	0x0004	/* Flag that REPEAT queries can be
					** executed or defined while in the
					** generated loop (see iiexec.c) */
# define	  II_L_DASHE	0x0008	/* User traps ^C with the "-e" flag */
# define	  II_L_EXITING	0x0010	/* The program is trying to exit -
					** prevent recursive exit recovery */
# define	  II_L_SINGLE	0x0020	/* The next query to be processed is
					** a singleton SELECT.  This flag is
					** turned on in IIsqMods and turned
					** off in IIqry_start */
# define	  II_L_TRACEQRY 0x0040  /* DBMS trace data is being sent to
					** a file */
# define	  II_L_DBPROC  0x00080	/* Within PROCEDURE body (to prevent
					** nested queries) and to maintain
					** state of ProcStatus.  Must also be
					** turned off by IIbreak.
					*/
# define	  II_L_XACT    0x00100	/* Within a logical transaction.  This
					** flag is set whenever we enter any
					** query (IIqry_start) and turned off
					** after terminating a transaction.
					** Flag maintained to supress the last
					** COMMIT of SQL and is not meant to 
					** describe the state of the DBMS.
					** If NOT set then this flag guarantees
					** that you are NOT in a transaction. 
					** If set you MAY be in a transaction.
					*/
# define	II_L_DYNSEL    0x00200	/* Processing a dynamic select stmt.
					** Turned on in IIsqMods() and off in
					** IIqry_end().  IIsqExImmed uses this
					** flag to distinguish from regular
					** exec immed statements; IIcsDaGet
					** also uses it to distinguish dynamic
					** selects into sqlda from cursor
					** fetches into sqlda.
					*/
# define	II_L_CONNECT	0x00400	/* Association is active. */
# define        II_L_NAMES      0x00800 /* Column names are to be returned in
                                        ** all descriptors. */
# define        II_L_DATAHDLR   0x01000 /* Currently within a User Datahandler
                                        ** Set in IILQldh_LoDataHandler(). */
# define	II_L_COPY	0x02000 /* Processing COPY statement */
# define	II_L_CHRPAD	0x04000 /* blank-pad fixed CHR_TYPE hostvars */
# define	II_L_CHREOS	0x08000L /* Check CHR_TYPE hostvars for EOS */
# define        II_L_NO_ING_SET 0x10000L /* Don't pass ING_SET */
# define        II_L_HELP       0x20000L /* ON if user is running a detaled 
                                         ** help statement, i.e., help 
                                         ** table/view .. */
# define	II_L_ENDPROC    0x40000L /* Processing DBProcedure ENDPROC */


			    /* Database procedure status mask */
    i4	ii_lq_prflags; 	/* Maintained as a separate flag to
				** recover from nested queries like
				** a RETRIEVE.  This flag need not be
				** reset except within the procedure code
				** as it is not checked by anyone.
				*/
# define	 II_P_DEFAULT 	0x0000  /* No state to handle */
# define	 II_P_NESTERR 	0x0001  /* Error by nesting query in 
					** procedure loop.
					*/
# define	 II_P_SENT 	0x0002 	/* GCA request sent to DBMS */
# define	 II_P_EXEC 	0x0004 	/* Started execution phase of procedure
				        ** and ready for parameters.  Allows
					** Dynamic SQL to call "put DBP param"
					** when processing an SQLDA. Turned
					** off after all parameters are read.
					*/
# define	 II_P_RESROWS	0x0008	/* Procedure returns result rows */

    IISQ_PARMS	ii_lq_parm;	/* Connect with-clause parameters */
    IILQ_EVENT  ii_lq_event;	/* Event descriptor */
    PTR		ii_lq_cdata;    /* Clients data area. */
    i4		ii_lq_cid;      /* id of the client. */
    IILQ_LOGKEY ii_lq_logkey;   /* Logical Key information */	
    i4  	*ii_lq_sqlcdata;	/* pointer to app's SQLCODE */
    IILQ_LODATA ii_lq_lodata;   /* Large Object Information */

    i4          ii_lq_ucode_init; /* unicode collation table initialized */
    PTR         ii_lq_ucode_ctbl; /* unicode collation information */
    PTR         ii_lq_ucode_cvtbl;
} II_LBQ_CB;

/* Forward declarations of functions returning other than 'int' */
FUNC_EXTERN II_LBQ_CB *IILQfsSessionHandle();

/* --------------------------------------------------------------- */

/*
** Name: IILOOP_ID - LIBQ stack of nested loops.
**
** Description:
**	Loop-structured statements may be nested across different
**	sessions.  For example, the following can occur:
**
**	main program		subroutine_a		subroutine_b
**
**	In session1		In session2		In session3
**	SELECT			SELECT			SELECT
**	{			{			{
**	  switch session2	  switch session3	
**	  subroutine_a()	  subroutine_b()
**	}			}			}
**				switch session1		switch session2
**
**	Program-wise (but not query-wise) the SELECT loops are nested.
**	Problems arise if program control pops back into a different
**	program-loop without changing sessions.  For example, if
**	subroutine_a omits to switch back to session1, the program will
**	return to the SELECT loop in the main program and attempt to get
**	more data.  Even more of a problem, subroutine_a could mistakenly
**	do the following:
**
**				subroutine_a
**
**				In session2
**				SELECT
**				{
**				    switch session1
**				}
**	The select loop opened in session2 would then end up getting
**	data from session1.
**
**	Keeping a stack of a session ids for nested loop-structured
**	statements helps to catch this type of error.  The current session id
**	is pushed when the loop opens and popped when the loop closes.
**	At loop iteration time, the code checks that the session id matches
**	the top value of the stack.
**
** History:
**	26-mar-1990	(barbara)
**	    Written.  Fixes bug 9159.
*/

typedef struct IILP_ 
{

    struct IILP_	*lp_next;		/* Next element up on stack */
    i4	 		lp_sid;			/* Session id */

} IILOOP_ID;

/* --------------------------------------------------------------- */

/*
** Name: IILQ_DA - SQL Diagnostic Area. 
**
** Description:
**      This structure maintains Information necessary to support ANSI SQL's
**	GET DIAGNOSTIC STATEMENT.
**
**	For this first round, it will support only the SQLSTATE variable
**	in the users code, which corresponds to RETURNED_SQLSTATE.
**	Please feel free to add to it as necessary.
**
** History:
**      03-nov-1991 (larrym)  
**	    written.  
*/

typedef struct 
{
    char	ii_da_ret_sqlst[DB_SQLSTATE_STRING_LEN]; /* RETURNED_SQLSTATE */
    char	*ii_da_usr_sqlstdata;	/* Address of SQLSTATE variable */	
    i2		ii_da_usr_sqlsttype;	/* Datatype of SQLSTATE variable */
    i4	ii_da_infostate;	/* state flags about the info in DA */

# define	II_DA_INIT	0x00L	/* 
					** Initialized, no valid data yet. 
					** Well, actually, the RETURNED_SQLSTATE
					** is initialized to SUCCESS, so that
					** if there are no errors, it has the
					** the correct value.
					*/
# define	II_DA_SET	0x01L	/* 
					** One or more Information Items
					** have been set.
					*/
# define	II_DA_RS_SET	0x02L	/* The RETURNED SQLSTATE has been set */

} IILQ_DA;

/* 
** The following are the Information Items (da_itemtype) that can be set 
** by the function IILQdaSIsetItem();
*/
# define	IIDA_RETURNED_SQLSTATE	0x01	/* RETURNED_SQLSTATE */

/*
** Name: IIUT_ARGS
**
** Description:
**
** History:
*/

typedef struct
{

# define IIutNMLEN		30
# define IIutMAXARGS		10
# define IIutBUFLEN		512

    char	ut_program[ IIutNMLEN + 1 ];
    i4		ut_argcnt;
    char	*ut_args[ IIutMAXARGS + 1 ];
    char	ut_buf[ IIutBUFLEN + 1 ];

} IIUT_ARGS;


/*
** Name: II_THR_CB
**
** Description:
**	Global LIBQ data which must be maintained on a per-thread
**	basis in multi-threaded applications.  Contains globals
**	and statics which pertain to a thread of execution.  Each
**	thread has its own 'current' session and a session can only
**	be 'current' to a single thread at any point in time.  This
**	restriction has the benefit of making II_LBQ_CB info thread
**	safe.  Each thread will have its own instance of this data,
**	managed through the ME thread-local-storage routines.
**	
**	Single threaded applications there will have only one instance 
**	of this data, similar to how things were prior to adding multi-
**	threading support.
**
**	ii_th_loop	A stack of session ids corresponding to open loop
**			structured statements.  Note that the first
**			stack element is static; subsequent elements are
**			dynamically allocated and freed.  
**	ii_th_da	ANSI SQL Diagnostic Area.
**
** History:
*/

typedef struct s_II_THR_CB
{

    II_LBQ_CB	*ii_th_session;		/* Current session */
    II_LBQ_CB	ii_th_defsess;		/* Default (no) session CB */

# define LQ_MAXUSER		32
# define LQ_MAXCONNAME		128

    char	ii_th_user[ LQ_MAXUSER+3 ];	/* Connection user ID */
    char	ii_th_cname[ LQ_MAXCONNAME+1 ];	/* User spedificed conn name */
    i4	 	ii_th_sessid;		/* User specified session ID */
    i4	ii_th_flags;		/* Connection state flags */

# define II_T_WITHLIST		0x0001	/* CONNECT statement has WITH clause */
# define II_T_SESSZERO		0x0002	/* User tried to use session 0 */
# define II_T_CONNAME		0x0004	/* Connection name has been set */

    i4          ii_hd_flags;		/* Handler flags */

# define II_HD_MSHDLR		0x0001	/* Executing user Message Handler */
# define II_HD_ERHDLR		0x0002	/* Executing user Error Handler */
# define II_HD_EVHDLR		0x0004	/* Executing user Event Handler */
# define II_HD_DBPRMPTHDLR	0x0008	/* Executing db prompt handler */

    IILOOP_ID	ii_th_loop;		/* Stack of ids for open loop stmts */
    PTR		ii_th_copy;		/* COPY control block */
    IISQLCA	*ii_th_sqlca_ptr;	/* Thread's SQLCA */
    IISQLCA	ii_th_sqlca;
    IILQ_DA	ii_th_da;		/* Diagnostic Area */
    IIUT_ARGS	ii_th_ut;		/* UTexe arguments */
    i4		ii_th_iparm;		/* Temp storage for integer parms */
    double	ii_th_fparm;		/* Temp storage for float parms */
    i4		ii_th_readonly;		/* Cursor FOR READONLY processing */

} II_THR_CB;

/* --------------------------------------------------------------- */

/*
+* Name: IISES_STATE - LIBQ list pointer for database connection states
**
** Description:
**	The list pointed at by ss_first is a list of items containing
**	LIBQ state information for a database connection.
**	  ss_first	- Points to the first II_LBQ_CB state on the list
**			  (may be null).
**	  ss_tot	- The number of connected sessions.
**	  ss_gchdrlen	- Information needed by LIBQGCA on all multiple connects
**			  subsequent to the first one.
-*
** History:
**	19-may-1989 - written (bjb)
*/

typedef struct s_IISES_STATE 
{
    MU_SEMAPHORE	ss_sem;
    II_LBQ_CB		*ss_first;
    i4			ss_tot;
    i4			ss_gchdrlen;

} IISES_STATE;

/* --------------------------------------------------------------- */
/*
+* Name: IIEMB_SET - Encoding of values set by II_EMBED_SET
**
** Description:
**	The various II_EMBED_SET values are read early in connection code.
**	Some values are set immediately; others must wait until the
**	an II_LBQ_CB structure has been allocated.  This structure keeps
**	track of the values.  It also saves reading the logical on every
*-	subsequent connection.
**
** History:
**	21-sep-1990 (barbara)
**	    Written.  Motivated by the need to read NOTIMEZONE before
**	    connection.  Before now all II_EMBED_SET values were processed
**	    after connection.
**	01-nov-1990 (barbara)
**	    Defined prefetchrows.  It was omitted when I created this struct.
**	16-mar-1991 (teresal)
**	    Add savequery option.
**	13-mar-1991 (barbara)
**	    Defined value for PRINTRACE.
**	10-apr-1991 (kathryn)
**	    Added defines and arguments for gcafile,qryfile and trcfile.
**	14-dec-1992 (larrym)
**	    Added temp flag II_ES_NOBYREF to work around FE/BE integeration
**	    issues.  This should be removed before product goes to testing.
**	18-may-1993 (larrym)
**	    Removed II_ES_NOBYREF.  See above.
*/
typedef struct s_IIEMB_SET {
    i4	iies_values;
#define	II_ES_SQLPRT	0x0001	/* sqlprint */
#define	II_ES_DBMSER	0x0002	/* dbmserror */
#define	II_ES_EVTDSP	0x0004	/* dbeventdisplay */
#define	II_ES_NOZONE	0x0008	/* nozone */
#define II_ES_PFETCH	0x0010	/* prefetchrows */
		/* Arguments for individual settings follow */
    i4	iies_prfarg;		/* Number of rows to prefetch */
} IIEMB_SET;

/*{
** Name: IILQ_HDLRS - Information on user defined handlers.
**
** Description:
**      This structure contains information pertaining to user defined
**	handlers. It contains pointers to each of the handlers:
**		ERRORHANDLER, MESSGEHANDLER, DBEVENTHANDLER and COPYHANDLER.
**
** History:
**      15-mar-1991     (kathryn)
**          Written.
**	22-mar-1991	(barbara)
**	    Added COPYHANDLER pointer.
**	01-apr-1991 	(kathryn)
**	    Added II_HD_MSHDLR and II_HD_EVHDLR in place of II_HD_INHDLR.
**	4-nov-93 (robf)
**          Add II_HD_DBPRMPTHDR
*/

typedef struct s_IILQ_HDLRS {
    i4          (*ii_err_hdl)();
    i4		(*ii_evt_hdl)();
    i4          (*ii_msg_hdl)();
    i4		(*ii_cpy_hdl)();
    i4		(*ii_dbprmpt_hdl)();
} IILQ_HDLRS;

/*{
** Name: IILQ_TRC_HDL - Information for LIBQ tracing handlers.
**
** Description:
**      This structure maintains a list of callback functions for 
**	trace output handling.  This is to facilitate having more than
**	one handler for each kind of tracing.
**
** History:
**      30-dec-1992     (larrym) written.  
*/

typedef struct IILTH_{
    VOID	 (*ii_tr_hdl)();	/* Trace Handler */
    PTR 	   ii_tr_cb;		/* Control Block passed to handler */
} IILQ_TRC_HDL;


/*{
** Name: IILQ_TRACE - Information for LIBQ tracing.
**
** Description:
**      This structure maintains information used for trace output handling.
**	One structure is needed for each type of LIBQ trace output
**	(PRINTGCA, PRINTQRY and SERVER tracing).
**
** History:
**      01-nov-1991     (kathryn)  written.  
**	     For the addition of trace handlers with handler control blocks,
**	     and needed trace flags.  File ptr and file name were removed from
**	     global LIBQ control block (II_GLB_CB) and included here.
**	30-dec-1992	(larrym)
**	    Changed trace hndlr function pointer to IILQ_TRC_HDL structure
**	    which is a list of handlers.
*/

typedef struct s_IILQ_TRACE 
{
    MU_SEMAPHORE	ii_tr_sem;
    i4  		ii_tr_flags;

# define II_TR_FILE		0x0001  /* Print trace output to file */
# define II_TR_HDLR		0x0002  /* Trace handler is registered */
# define II_TR_NOTERM		0x0004  /* Do not print trace to stdout */
# define II_TR_APPEND		0x0008	/* Append to output file */

    char		*ii_tr_fname;	/* Filename for trace output */
    PTR 		ii_tr_file;	/* File Ptr of trace output file */
    i4			ii_tr_sid;	/* Last session traced in file */

# define II_TR_MAX_HDLR		3	/* Currently: W4GL XA [none] */

    i4			ii_num_hdlrs;	/* number of active handlers */
    IILQ_TRC_HDL	ii_tr_hdl_l[ II_TR_MAX_HDLR ];	/* Handler list */

} IILQ_TRACE;


/* --------------------------------------------------------------- */
/*{
+*  Name: II_GLB_CB - LIBQ global control block for query processing.
**
**  Description:
**	This structure maintains information used for query processing.
**	The information is global across all multi-connected sessions
**	as opposed to the information in the II_LBQ_CB structure which
**	pertains to LIBQ information for a particular database session.
**	  ii_gl_sstate -  Session state descriptor which maintains
**			  a pointer to a list of session states associated
**			  with multiple database connections.
**        ii_gl_seterr -  Error printing function. Set-able by an embedded
**                        program, the caller can decide whether or not to
**                        return the error number (print it or not).
**        ii_gl_trace   - Trace function pointer.  If this is set, then trace
**                        data is passed to function.
**	  ii_gl_xa_hdlr	- function pointer into libqxa handler.  Used when 
**			  libq needs to call libqxa.  If this is NULL, then
**			  we assume that the application is non-XA.
**	  ii_gl_flags	- Flags for to indicate state tracing.
**	  ii_gl_hdlrs	- Information about user-settable handlers.
**	  iigl_embset	- Information about II_EMBED_SET settings.
** 	  iigl_msgtrc   - PRINTGCA trace handling information.
**	  iigl_qrytrc	- PRINTQRY trace handling information.
**	  iigl_svrtrc   - SERVER trace handling information.
-*
**  History:
**	19-may-1989	- Extracted from II_LBQ_CB for mult connections (bjb)
**	27-apr-1990 	- New flag for cursor tracing (neil)
**	14-jun-1990	(barbara)
**	    Added flag II_G_PRQUIT to inform whether or not program should
**	    quit on association failures (bug 21832)
**	21-sep-1990	(barbara)
**	    Added iigl_embset structure field containing information about
**	    II_EMBED_SET settings.  This allows versatility in the code as to
**	    where we apply those settings.  Note also that from this point
**	    on I am substituting a "iigl_" prefix for the "ii_gl_" prefix
**	    because it's getting too hard to keep names unique to 7 chars.
**	03-dec-1990 (barbara)
**	    Moved ii_gl_error to the II_LBQ_CB as part of fix to bug 9160.
**	    Extracted the seterr and the trace function pointers out of
**	    II_ERR_DESC and gave them new names in the II_GLB_CB.
**	15-mar-1991 (kathryn)
**	    Added IILQ_HDLRS struct for user defined handlers.
**	13-mar-1991 (teresal)
**	    Add flag II_G_SAVQRY which indicates if the last query text 
**	    should be saved.
**	13-mar-1991 (barbara)
**	    To trap trace messages received from the server, added ii_gl_svrtrc
**	    field and II_G_SVRTRC mask.
**	10-apr-1991 (kathryn)
**	    Added ii_gl_msgfile (file name for "printgca" output )
**		  ii_gl_qryfile (file name for "printqry" output)
**		  ii_gl_svrfile (file name for "printtrace" output)
**	15-apr-1991 (kathryn)
**	    Added II_G_APPTRC - tracefiles should be re-opened in append mode.
**	    This occurs when a child process has been spawned so file was
**	    closed - Want to re-open append so previous output is not 
**	    over-written.
**	01-nov-1991 (kathryn)
**	    Move trace filename and file pointer variables from this structure
**	    into the new IILQ_TRACE structure added for support of internal
**	    trace handlers, handler context blocks and needed trace flags.
**	    Added IILQ_TRACE structures:
**		iigl_msgtrc - for PRINTGCA trace handling information.
**		iigl_qrytrc - for PRINTQRY trace handling information.
**	        iigl_svrtrc - for SERVER trace handling information.
**	03-nov-1992 (larrym)
**	    Added IILQ_DA, the ANSI SQL Diagnostic Area.  
**	19-nov-1992 (larrym)
**	    Added ii_gl_xa_hdlr, to allow libq to call into libqxa.
**	07-dec-1992 (teresal)
**	    Removed ADF control block - there is now a ADF control block
**	    per session (ii_gl_adf has been changed to ii_lq_adf).
**	14-dec-1992 (larrym)
**	    added temp flag II_G_NOBYREF to workaround FE/BE issues.  It should
**	    be removed before product goes to testing.
**	18-may-1993 (larrym)
**	    removed II_G_NOBYREF.  See above.
**	21-jun-2001 (gupsh01)
**	    Added DB_NCHR_TYPE and DB_NVCHR_TYPE to the definition of 
**	    IIDB_CHARTYPE_MACRO.
**	10-jun-2008 (lunbr01)  bug xxxxx
**	    Added II_G_NO_COMP value for ii_gl_flags to suppress varchar
**	    compression if variable II_VCH_COMPRESS_ON=N.
*/

typedef struct s_II_GLB_CB {
    MU_SEMAPHORE ii_gl_sem;		/* General semaphore */
    IISES_STATE ii_gl_sstate;		/* Descriptor of II_LBQ_CB states */
    i4	ii_gl_flags;		/* Global state flags */
# define	    II_G_TM	 0x0001	/* The terminal monitor is running */
# define	    II_G_DEBUG	 0x0002	/* The program is using the "-d"
					** feature to debug queries */
# define	    II_G_SAVQRY	 0x0004	/* The SET_INGRES "savequery" feature 
					** has been set */
# define	    II_G_CSRTRC	 0x0008	/* II_EMBED_SET "printcsr" was set.
					** Trace output for this flag goes
					** to the screen (internal only).
					*/
# define	    II_G_PRQUIT	 0x0010	/* II_EMBED_SET/SET_INGRES "programquit"
					** was set. */
# define	    II_G_SAVPWD  0x0020 /* SET_INGRES "savepassword" set*/
# define	    II_G_ABORT   0x0040	/* Indicates cleanup is in progress  */
# define	    II_G_NO_COMP 0x0080	/* Do not compress varchars */
    IIEMB_SET	iigl_embset;		/* Info about II_EMBED_SET values */
    IILQ_TRACE  iigl_msgtrc;            /* PRINTGCA trace information */
    IILQ_TRACE  iigl_svrtrc;            /* Server trace information */
    IILQ_TRACE  iigl_qrytrc;		/* PRINTQRY trace information */
    IILQ_HDLRS	ii_gl_hdlrs;		/* user handlers */
    i4	(*ii_gl_seterr)();	/* Error printing routine */
    i4		(*ii_gl_trace)();	/* Trace printing routine */
    i4		(*ii_gl_xa_hdlr)();	/* handler for libq to call libqxa */
    char	*ii_gl_password;	/* Saved password for prompting */

} II_GLB_CB;

/*
** A pointer to LIBQ's global state information 
*/
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF	II_GLB_CB	*IIglbcb;
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF	II_GLB_CB	*IIglbcb;
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

/* --------------------------------------------------------------- */

/* Error severity flags */
# define	II_ERR	0	/* Corresponds to UG_ERR_ERROR */
# define	II_FAT	3	/* Corresponds to UG_ERR_FATAL */

/*
** Error constants coming from DBMS
*/
# define IIerOK		(i4)0		/* No error -- MUST BE 0! */

/*
** DBMS group will let us know when following two errors become one
*/
# define CSCLOSED	(i4)4601	/* DBMS: Cursor closed (FETCH/DELETE) */
# define CSRCLOSED	(i4)2205	/* DBMS: Cursor closed (REPLACE) */

/*
** This macro reflects INGRES errors related to the DBMS.  Although LIBQ
** errors will also cause ii_er_num to be set, this macro assumes we've
** already checked for those errors by looking at the II_Q_QERR bit in
** ii_lq_curqry.  Therefore ii_er_num will indicate a DBMS error.
*/
# define II_DBERR_MACRO(lcb)					\
		(lcb->ii_lq_error.ii_er_num != IIerOK)

/* --------------------------------------------------------------- */

/*
** Useful DB_DATA_VALUE macros
*/

/* Fill a DB_DATA_VALUE with a scalar type */
# define	II_DB_SCAL_MACRO( db, typ, len, data )			\
{								  	\
			(db).db_datatype = (typ);			\
			(db).db_length = (len);				\
			(db).db_data = (PTR)(data);			\
			(db).db_prec = 0;				\
}

/* Given a pointer to a DB_DATA_VALUE determine whether datatype fits
** a particular category ( LONG, VARYING, CHARACTER, ...).
*/

# define	IIDB_LONGTYPE_MACRO(dbv) 			\
		(  abs((dbv)->db_datatype) == DB_LVCH_TYPE 	\
		|| abs((dbv)->db_datatype) == DB_LBYTE_TYPE	\
		|| abs((dbv)->db_datatype) == DB_GEOM_TYPE	\
		|| abs((dbv)->db_datatype) == DB_POINT_TYPE	\
		|| abs((dbv)->db_datatype) == DB_MPOINT_TYPE	\
		|| abs((dbv)->db_datatype) == DB_LINE_TYPE	\
		|| abs((dbv)->db_datatype) == DB_MLINE_TYPE	\
		|| abs((dbv)->db_datatype) == DB_POLY_TYPE	\
		|| abs((dbv)->db_datatype) == DB_MPOLY_TYPE	\
		|| abs((dbv)->db_datatype) == DB_LBIT_TYPE      \
	        || abs((dbv)->db_datatype) == DB_LNVCHR_TYPE)

		

# define	IIDB_VARTYPE_MACRO(dbv)				\
		(  abs((dbv)->db_datatype) == DB_VCH_TYPE	\
		|| abs((dbv)->db_datatype) == DB_VBYTE_TYPE	\
		|| abs((dbv)->db_datatype) == DB_NVCHR_TYPE	\
		|| abs((dbv)->db_datatype) == DB_LTXT_TYPE)

# define	IIDB_CHARTYPE_MACRO(dbv)			\
		(  abs((dbv)->db_datatype) == DB_CHA_TYPE	\
		|| abs((dbv)->db_datatype) == DB_CHR_TYPE	\
		|| abs((dbv)->db_datatype) == DB_VCH_TYPE	\
		|| abs((dbv)->db_datatype) == DB_NCHR_TYPE	\
		|| abs((dbv)->db_datatype) == DB_NVCHR_TYPE	\
		|| abs((dbv)->db_datatype) == DB_LTXT_TYPE)

# define        IIDB_BYTETYPE_MACRO(dbv)                        \
                (  abs((dbv)->db_datatype) == DB_BYTE_TYPE      \
                || abs((dbv)->db_datatype) == DB_VBYTE_TYPE )


/* Given a DB_DT_ID (the db_datatype member of the DB_DATA_VALUE struct)
** determine whether the datatype is a LONGTYPE.
*/
# define        IIDT_LONGTYPE_MACRO(dt)        \
	        (  abs(dt) == DB_LVCH_TYPE     \
		|| abs(dt) == DB_LBYTE_TYPE    \
		|| abs(dt) == DB_GEOM_TYPE    \
		|| abs(dt) == DB_POINT_TYPE    \
		|| abs(dt) == DB_MPOINT_TYPE    \
		|| abs(dt) == DB_LINE_TYPE    \
		|| abs(dt) == DB_MLINE_TYPE    \
		|| abs(dt) == DB_POLY_TYPE    \
		|| abs(dt) == DB_MPOLY_TYPE    \
	        || abs(dt) == DB_LBIT_TYPE     \
		|| abs(dt) == DB_LNVCHR_TYPE)

/* B65565 Given a DB_DT_ID ( the db_datatype member of the DB_DATA_VALUE struct)
** determine whether the datatype is a UDT.
*/
# define	IIDT_UDT_MACRO(dt) 		\
		( abs(dt) >= ADD_LOW_CLSOBJ)

/* Given a DB_DT_ID determine if it is a unicode data type */
# define        IIDB_UNICODETYPE_MACRO(dbv)                     \
                (  abs((dbv)->db_datatype) == DB_NCHR_TYPE       \
                || abs((dbv)->db_datatype) == DB_NVCHR_TYPE     \
                || abs((dbv)->db_datatype) == DB_LNVCHR_TYPE)


/* --------------------------------------------------------------- */
/* PROTOTYPES */

/* iiadfcb.c */
FUNC_EXTERN  STATUS	IILQasAdfcbSetup( II_THR_CB * );

/* iicsrun.c */
FUNC_EXTERN  VOID	IIcsAllFree( II_LBQ_CB * );

/* iida.c */
FUNC_EXTERN  VOID	IILQdaSIsetItem( II_THR_CB *, u_i4, char * );

/* iidathdl.c */
FUNC_EXTERN  STATUS 	IILQlcd_LoColDesc(II_THR_CB *,i4 **,DB_DATA_VALUE **);
FUNC_EXTERN  void	IILQlcc_LoCbClear( II_LBQ_CB * );

/* iidbg.c */
FUNC_EXTERN  VOID	IIdbg_info( II_LBQ_CB *, char *, i4  );
FUNC_EXTERN  VOID	IIdbg_gca( i4, DB_DATA_VALUE * );
FUNC_EXTERN  VOID	IIdbg_dump( II_LBQ_CB * );
FUNC_EXTERN  STATUS	IIdbg_getquery( II_LBQ_CB *, DB_DATA_VALUE * );
FUNC_EXTERN  VOID	IIdbg_response( II_LBQ_CB *, bool );
FUNC_EXTERN  VOID	IIdbg_toggle( II_LBQ_CB *, i4  flags );
FUNC_EXTERN  STATUS	IILQathAdminTraceHdlr( IILQ_TRACE *, 
					       VOID (*)(), PTR, bool );
FUNC_EXTERN  VOID	IILQcthCallTraceHdlrs( IILQ_TRC_HDL *, char *, i4  );

/* iidbprmt.c */
FUNC_EXTERN  VOID	IILQdbprompt( II_THR_CB *thr_cb );

/* iierror.c */
FUNC_EXTERN  VOID	IIdberr( II_THR_CB *, 
				 i4, i4, i4, i4, char *, bool );
FUNC_EXTERN  VOID 	IIsdberr( II_THR_CB *, DB_SQLSTATE *, 
				  i4, i4, i4, i4, char *, bool );
FUNC_EXTERN  VOID	IIdbermsg( II_THR_CB *, i4, i4, char * );
FUNC_EXTERN  VOID	IIer_save( II_LBQ_CB *, char * );
FUNC_EXTERN  VOID	IIdbmsg_disp( II_LBQ_CB * );
FUNC_EXTERN  i4	IIret_err();

/* iiexit.c */
FUNC_EXTERN  VOID	IIlqExit( VOID );

/* iigcahdl.c */
FUNC_EXTERN  i4	IIhdl_gca();

/* iigcatrc.c */
FUNC_EXTERN  VOID	IILQgstGcaSetTrace( II_LBQ_CB *, i4  );

/* iigetdat.c */
FUNC_EXTERN  i4	IIgetdata( II_THR_CB *, DB_DATA_VALUE *, 
				   DB_EMBEDDED_DATA *, i4, i4  );

/* iiingres.c */
FUNC_EXTERN  i4	IILQaiAssocID( VOID );

/* iiloget.c */
FUNC_EXTERN  STATUS	IILQlmg_LoMoreGet( II_LBQ_CB * );
FUNC_EXTERN  STATUS	IILQlag_LoAdpGet( II_LBQ_CB *, PTR );
FUNC_EXTERN  STATUS	IILQlrs_LoReadSegment( i4 flags,
					    II_LBQ_CB *, DB_DATA_VALUE *, 
					    DB_DATA_VALUE *, i4,
					    i4 *, i4  * );
FUNC_EXTERN  STATUS	IILQlvg_LoValueGet( II_THR_CB *, i2 *, i4, 
					    i4, i4, PTR, 
					    DB_DATA_VALUE *, i4  );

/* iiloput.c */
FUNC_EXTERN  VOID	IILQlap_LoAdpPut( II_LBQ_CB * );
FUNC_EXTERN  VOID	IILQlmp_LoMorePut( II_LBQ_CB *, i4  );
FUNC_EXTERN  VOID	IILQlws_LoWriteSegment( i4 flags,
					    II_LBQ_CB *, DB_DATA_VALUE * );

/* iilqevt.c */
FUNC_EXTERN  VOID	IILQeaEvAdd( II_THR_CB * );
FUNC_EXTERN  VOID	IILQedEvDisplay( II_LBQ_CB *, II_EVENT * );
FUNC_EXTERN  VOID	IILQefEvFree( II_LBQ_CB * );

/* iilqsess.c */
FUNC_EXTERN  II_LBQ_CB	*IILQsnSessionNew( i4  * );
FUNC_EXTERN  VOID	IILQsdSessionDrop( II_THR_CB *, i4  );
FUNC_EXTERN  STATUS	IILQssSwitchSession( II_THR_CB *, II_LBQ_CB * );
FUNC_EXTERN  II_LBQ_CB	*IILQsbnSessionByName( char *, II_LBQ_CB * );
FUNC_EXTERN  II_LBQ_CB	*IILQscConnNameHandle( char * );
FUNC_EXTERN  VOID	IILQsepSessionPush( II_THR_CB *thr_cb );
FUNC_EXTERN  VOID	IILQspSessionPop( II_THR_CB *thr_cb );
FUNC_EXTERN  i4	IILQscSessionCompare( II_THR_CB *thr_cb );
FUNC_EXTERN  STATUS	IILQucolinit(i4 );

/* iilqstrc.c */
FUNC_EXTERN  i4	IILQstpSvrTracePrint( bool,DB_DATA_VALUE * );

/* iilqthr.c */
FUNC_EXTERN  II_THR_CB	*IILQthThread( VOID );

/* iiqid.c */
FUNC_EXTERN  IIDB_QRY_ID *IIqid_find( II_LBQ_CB *, bool, char *, i4, i4  ); 
FUNC_EXTERN  VOID	IIqid_send( II_LBQ_CB *, IIDB_QRY_ID * );
FUNC_EXTERN  STATUS	IIqid_read( II_LBQ_CB *, IIDB_QRY_ID * );
FUNC_EXTERN  STATUS	IIqid_add( II_LBQ_CB *, bool, i4, i4, IIDB_QRY_ID * );
FUNC_EXTERN  VOID	IIqid_free( II_LBQ_CB * );

/* iiqry.c */
FUNC_EXTERN  STATUS	IIqry_read( II_LBQ_CB *, i4  );

/* iisndlog.c */
FUNC_EXTERN  VOID	IILQpeProcEmbed( II_LBQ_CB * );

/* iisqcrun.c */
FUNC_EXTERN  VOID	IIsqSetSqlCode( II_LBQ_CB *, i4  );
FUNC_EXTERN  VOID	IIsqAbort( II_LBQ_CB * );
FUNC_EXTERN  VOID	IIsqEnd( II_THR_CB * );
FUNC_EXTERN  VOID	IIsqWarn( II_THR_CB *, i4  );
FUNC_EXTERN  i4	IIsqSaveErr(II_LBQ_CB *,bool,i4,i4,char *);
FUNC_EXTERN  VOID	IIsqEvSetCode( II_LBQ_CB * );

FUNC_EXTERN  VOID	IILQdtDbvTrim (DB_DATA_VALUE *);
FUNC_EXTERN  void	IIwritio (i4, i2*, i4, i4, i4, char*);
FUNC_EXTERN  void	IIcsGetio (i2*, i4, i4, i4, char*);
FUNC_EXTERN  i4	IIconvert (i4*, i4*, i4, i4, i4, i4);
FUNC_EXTERN  VOID	IIUGsber(DB_SQLSTATE*,char*,i4,ER_MSGID,i4,i4,PTR,
				 PTR,PTR,PTR,PTR,PTR,PTR,PTR,PTR,PTR);

/* iidml.c */
FUNC_EXTERN  i4  IIdml(i4 lang);

/* iierror.c */
FUNC_EXTERN  i4 IIret_err(i4 *err);
FUNC_EXTERN  i4 IIno_err(i4 *err);
FUNC_EXTERN  i4 IItest_err(void);
FUNC_EXTERN  void IImsgutil(char *bufr);

/* iiexec.c */
FUNC_EXTERN  void IIsexec(void);
FUNC_EXTERN  i4   IInexec(void);
FUNC_EXTERN  void IIexDefine(i4 what,char *name,i4 num1,i4 num2);
FUNC_EXTERN  i4  IIexExec(i4 what,char *name,i4 num1,i4 num2);

/* iiexit.c */
FUNC_EXTERN  void IIexit(void);
FUNC_EXTERN  void IIabort(void);

/* iife.c */
FUNC_EXTERN  void IIlq_Protect(bool set_flag);
FUNC_EXTERN  i4   IIfeddb(void);
FUNC_EXTERN  i4   IIfePrepare(void);
FUNC_EXTERN  i4   IIfeDescribe(void);
FUNC_EXTERN  struct _LIBQGDATA *IILQgdata(void);

/* iiflush.c */
FUNC_EXTERN	void IIflush(char *file_nm,i4 line_no);

/* iigcatrc.c */
FUNC_EXTERN	void IILQgwtGcaWriteTrace(i4 action,i4 len,char *buf);

/* iigettup.c */
FUNC_EXTERN	i4 IItupget(void);

/* iiingres.c */
FUNC_EXTERN	void IIingopen(i4 lang,char *p1,char *p2,char *p3,char *p4,
		char *p5,char *p6,char *p7,char *p8,char *p9,char *p10,
		char *p11,char *p12,char *p13,char *p14,char *p15);
FUNC_EXTERN	void IIdbconnect(i4 dml,i4 lang,char *p1,char *p2,char *p3,
		char *p4,char *p5,char *p6,char *p7,char *p8,char *p9,
		char *p10,char *p11,char *p12,char *p13,char *p14,char *p15,
		char *p16);
FUNC_EXTERN	STATUS IIx_flag(char *dbname,char *minus_x);

/* iiinqset.c */
FUNC_EXTERN	void IILQisInqSqlio(i2 *indaddr,i4 isvar,i4 type,
                 i4  len,char *addr,i4 attrib);
FUNC_EXTERN	void IIeqiqio(i2 *indaddr,i4 isvar,i4 type,i4 len,
                 char *addr,char *name);
FUNC_EXTERN	void IIeqinq(i4 isvar,i4 type,i4 len,char *addr,char *name);
FUNC_EXTERN	void IILQssSetSqlio(i4 attrib,i2 *indaddr,i4 isvar,i4 type,
                 i4  len,PTR data);
FUNC_EXTERN	void IIeqstio(char *name,i2 *indaddr,i4 isvar,i4 type,i4 len,
                 PTR  data);
FUNC_EXTERN	void IIeqset(char *name,i4 isvar,i4 type,i4 len,PTR  data);
FUNC_EXTERN	i4 IItuples(void);
FUNC_EXTERN	i4 IIerrnum(i4 setting,i4 errval);
FUNC_EXTERN	void IILQshSetHandler(i4 hdlr,i4 (*funcptr)(void));


/* iilang.c */
FUNC_EXTERN	i4 IIlang(i4 newlang);

/* iiloget.c */
FUNC_EXTERN	void IILQlgd_LoGetData(i4 flags,i4 hosttype,i4 hostlen,
            char *addr,i4 maxlen,i4 *seglen,i4 *dataend);

/* iiloput.c */
FUNC_EXTERN	void IILQlpd_LoPutData(i4 flags,i4 hosttype,i4 hostlen,
            char *addr,i4 seglen,i4 dataend);

/* iilqadr.c */
FUNC_EXTERN	void * IILQdbl(double dblval);
FUNC_EXTERN	void * IILQint(i4 intval);

/* iilqevt.c */
FUNC_EXTERN	void IILQesEvStat(i4 escall,i4 eswait);

/* iilqsess.c */
FUNC_EXTERN	void IILQsidSessID(i4 session_id);
FUNC_EXTERN	void IILQcnConName(char *con_name);
FUNC_EXTERN	struct IILQ_ *IILQfsSessionHandle(i4 session_id);
FUNC_EXTERN	void IILQncsNoCurSession(void);
FUNC_EXTERN	STATUS IILQpsdPutSessionData(i4 client_id,PTR client_data);
FUNC_EXTERN	PTR  IILQgsdGetSessionData(i4 client_id);
FUNC_EXTERN	void IILQcseCopySessionError(struct s_II_ERR_DESC *sedesc,
                struct s_II_ERR_DESC *dedesc);

/* iimain.c */
FUNC_EXTERN	void IImain(void);

/* iinext.c */
FUNC_EXTERN	i4 IInextget(void);
FUNC_EXTERN	i4  IIerrtest(void);

/* iiparam.c */
FUNC_EXTERN	i4 IIParProc(char *target,i4 *htype,i4 *hlen,i4 *is_ind,
                char *errbuf,i4 flag);

/* iiparret.c */
FUNC_EXTERN	i4 IIparret(char *tlist,char **argv ,i4 (*xfunc)());

/* iiqry.c */
FUNC_EXTERN	STATUS IIqry_start(i4 gcatype,i4 gcamask,i4 qrylen,char *qryerr);
FUNC_EXTERN	void IIqry_end(void);

/* iiretdom.c */
FUNC_EXTERN	i4 IIgetdomio(i2 *indaddr,i4 isvar,i4 hosttype,i4 hostlen,
                char *addr);
FUNC_EXTERN	i4 IIretdom(i4 isvar,i4 hosttype,i4 hostlen,char *addr);


/* iiretini.c */
FUNC_EXTERN	void IIretinit(char *file_nm,i4 line_no);

/* iirowdsc.c */
FUNC_EXTERN	STATUS IIrdDescribe(i4 desc_type,struct s_ROW_DESC *row_desc);

/* iisetdom.c */
FUNC_EXTERN	void IIputdomio(i2 *indaddr,i4 isvar,i4	type,i4	length,char *addr);
FUNC_EXTERN	void IIsetdom(i4 isvar,i4 type,i4 length,char *addr);
FUNC_EXTERN	void IILQdtDbvTrim(struct _DB_DATA_VALUE *dbv);

/* iiseterr.c */
FUNC_EXTERN	i4 (*IIseterr(i4 (*proc)(void)))(void);

/* iisndlog.c */
FUNC_EXTERN	void IIsendlog(char *db,i4 child);
FUNC_EXTERN	void IILQesaEmbSetArg(char *srcptr,char *destptr,i4 length);
FUNC_EXTERN	void IILQreReadEmbed(void);


/* iisqcrun.c */
FUNC_EXTERN	void IIsqInit(register IISQLCA *sqc);
FUNC_EXTERN	void IIsqlcdInit(register IISQLCA *sqc,i4 *sqlcd);
FUNC_EXTERN	void IIsqUser(char *usename);
FUNC_EXTERN	void IIsqConnect(i4 lang,char *db,char *a1,char *a2,char *a3,
          char *a4,char *a5,char *a6,char *a7,char *a8,char *a9,
          char *a10,char *a11, char *a12, char *a13);
FUNC_EXTERN	void IIsqMods(i4 mod_flag);
FUNC_EXTERN	void IIsqFlush(char *filename,i4 linenum);
FUNC_EXTERN	void IIsqDisconnect(void);
FUNC_EXTERN	void IIsqStop(register IISQLCA *sqc);
FUNC_EXTERN	void IIsqPrint(register IISQLCA *sqlca);


/* iisqdrun.c */
FUNC_EXTERN	void IIsqExImmed(char *query);
FUNC_EXTERN	void IIsqExStmt(char *stmt_name,i4 using_vars);

/* iisqparm.c */
FUNC_EXTERN	void IIsqParms(i4 flag,char *name_spec,i4 val_type,
                  char *str_val,i4 int_val);

/* iistrutl.c */
FUNC_EXTERN	char *IIstrconv(i4 flag,const char *orig,char *buf,i4 len);

/* iisync.c */
FUNC_EXTERN	void IIsyncup(char *file_nm,i4 line_no);

/* iisyserr.c */
FUNC_EXTERN	void syserr(char *p0,char *p1,char *p2,char *p3,char *p4,char *p5);


/* iitm.c */
FUNC_EXTERN	void IItm(bool turn_on);
FUNC_EXTERN	void IItm_dml(i4 dml);
FUNC_EXTERN	void IItm_retdesc(struct s_ROW_DESC **desc );
FUNC_EXTERN	void IItm_status(i4 *rows,i4 *error,i4 *status,bool *libq_connect_chgd);
FUNC_EXTERN	void IItm_trace(i4 (*trace_func)());
FUNC_EXTERN	void IItm_adfucolset (ADF_CB *scb);

/* iiutsys.c */
FUNC_EXTERN	void IIutsys(i4 uflag,char *uname,char *uval);

/* iivarstr.c */
FUNC_EXTERN	void IIvarstrio(i2 *indnull,i4 isvar,i4 type,i4 length,char *addr);

/* iiwrite.c */
FUNC_EXTERN	void IIwritio(i4 trim,i2 *ind_unused,i4 isref_unused,i4 type,
                 i4  length,char *qry);
FUNC_EXTERN	void IIwritedb(char *qry);
FUNC_EXTERN	void IIputctrl(i4 ctrl);


/* iixact.c */
FUNC_EXTERN	void IIxact(i4 what);
FUNC_EXTERN	void IIsqTPC(i4 highdxid,i4 lowdxid);

/* iierror.c */
FUNC_EXTERN	void IIlocerr(i4 genericno, i4 errnum, i4 severity, i4 parcount,
			... );
FUNC_EXTERN	void IIdberr( II_THR_CB   *thr_cb, i4  dbgeneric, i4  dblocal,
    			i4 dbstatus, i4 dberrlen, char *dberrtext,
		         bool dbtimestmp);
FUNC_EXTERN	void IIsdberr( II_THR_CB   *thr_cb, DB_SQLSTATE *db_sqlst,
    			i4 dbgeneric, i4 dblocal, i4 dbstatus,
    			i4 dberrlen, char *dberrtext, bool dbtimestmp);
FUNC_EXTERN	void IIdbermsg(II_THR_CB *thr_cb, i4 msgno, i4 msglen, char *msgtext );
FUNC_EXTERN	void IIer_save( II_LBQ_CB *IIlbqcb, char * msg );
FUNC_EXTERN	i4 IIret_err( i4 *err);
FUNC_EXTERN	i4 IIno_err( i4 *err);
FUNC_EXTERN	i4 IItest_err();
FUNC_EXTERN	ER_MSGID IILQfrserror( ER_MSGID        errnum);
FUNC_EXTERN	VOID IImsgutil( char    *bufr);
FUNC_EXTERN	VOID IIdbmsg_disp( II_LBQ_CB *IIlbqcb );
FUNC_EXTERN	VOID IIfakeerr( i4 dml, i4 genericno, i4 errnum,
			i4 severity, i4 parcount,
			... );
