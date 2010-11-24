/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <cm.h>
#include    <gc.h>
#include    <er.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tr.h>
#include    <tm.h>
#include    <sp.h>
#include    <mo.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcm.h>
#include    <gcu.h>
#include    <cv.h>


/*
** Name: GCNSFILE.C - IIGCN database file manipulation routines
**
** Description:
**	This file contains the routines to load and dump IIGCN's
**	name queues.  Cacheing and file formatting is handled here.
**
**	gcn_nq_file		Generate disk file name for name queue.
**	gcn_nq_init		Create/open a new name queue.
**	gcn_nq_create		Build a new name queue (dynamic server class).
**	gcn_nq_drop		Remove an existing name queue.
**	gcn_nq_find		Find a name queue given its type.
**	gcn_nq_lock		Prepare a name queue for access.
**	gcn_nq_unlock		Finish a name queue update.
**	gcn_nq_add		Add a tuple to the name queue.
**	gcn_nq_del		Delete tuple(s) from a name queue.
**	gcn_nq_qdel		Delete name queue entries.
**	gcn_nq_ret		Find tuple(s) in a name queue.
**	gcn_nq_scan		Find tuple(s) in a name queue, repeated scan.
**	gcn_nq_assocs_upd	Update in cache value for tup's no_assocs.
**	gcn_fopen		Open a named file.
**
** History:
**	01-Mar-89 (seiwald)
**	    Created.
**	22-May-89 (seiwald)
**	    Use MEreqmem() instead of MEalloc().
**	17-Jul-89 (seiwald)
**	    Call gca_erlog(), rathern than TRdisplay(), to log error messages.
**	26-Jul-89 (seiwald)
**	    Don't reverse list on reading in.
**	25-Nov-89 (seiwald)
**	    Use IIGCn_static for references to global data.
**	    Added modified flag to gcn_dump_nq().  It may be used in
**	    the future to bypass rewriting the file if it hasn't changed.
**	28-Nov-89 (seiwald)
**	    Use the modified flag to gcn_dump_nq() to bypass rewriting the 
**	    file if it hasn't changed.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	23-Apr-91 (brucek)
**	    Modified to LOlast file before reading into queue.  If file
**	    has not been modified since last read, just return.
**	24-Apr-91 (brucek)
**	    Changed LOlast call to LOinfo (LOlast is TABOO)
**	16-May-91 (brucek)
**	    Added gcn_get_rec() to allow for SIread returning less than
**	    the total number of bytes requested.
**	20-Jun-91 (alan)
**	    Fix for TM addressing problem.  
**	01-Jul-91 (alan)
**	    Incremental file update support:  insert 'add'ed records in proper
**	    queue location, write only modified records unless file becomes
**	    too badly split (when adds/deletes > 10% of total records).
**	28-Jun-91 (seiwald)
**	    Rewritten to constrict access to name queues to this file;
**	    all access is through gcn_nq_xxx() routines now.
**	23-Jul-91 (seiwald)
**	    Open empty file in gcn_nq_init if one not already present - 
**	    function omitted during last rewrite.
**	11-nov-91 (seg)
**	    Can't dereference MEfree'd pointer in gcn_nq_unlock.  
**	    This causes core dumps under some conditions.
**	28-jan-92 (seiwald)
**	    Gcn_next_bedck is now a SYSTIME structure, cleared by the
**	    MEreqmem, so don't try to assign zero to it.
**	2-Mar-92 (seiwald)
**	    Fix insertion logic in gcn_nq_add.
**	29-Jun-92 (fredb)
**	    Integrate bryanp's 1989 changes for MPE (hp9_mpe):
**      	17-Sep-89 (bryanp)
**          		Fixed a bug in the hp9_mpe filename generation code 
**			introduced during 61b3ug integration.
**      	18-Sep-89 (bryanp)
**          		Fixed another bug in the hp9_mpe filename generation 
**			code which was introduced during 61b3ug integration.
**	10-Jul-92 (fredb)
**	    Fix another MPE problem.  The use of LOfaddpath is causing bad
**	    filenames to be generated.  Implemented the same sort of fix as
**	    byranp did in 9/89.
**	1-Sep-92 (seiwald)
**	    gca_erlog() renamed to gcu_erlog() and now takes ER_ARGUMENT's.
**	28-Sep-92 (brucek)
**	    Added MO calls for IMA support; new instance arg for gcn_nq_add;
**	    changed gcn_rec_no in GCN_DB_RECORD to gcn_tup_id;
**	    added gcn_tup_id and gcn_instance to GCN_TUP_QUE.
**	01-Oct-92 (brucek)
**	    Moved MIB class definitions here from gcnsinit.c.
**	06-Oct-92 (brucek)
**	    Ifdef out GCM/MO stuff.
**      15-oct-92 (rkumar)
**          changed parameter name 'shared' to 'common'. 'shared'
**          is a reserved word for Sequent's compiler.
**	05-Nov-92 (brucek)
**	    New function gcn_nq_create for dynamic server class support.
**	19-Nov-92 (brucek)
**	    Use gcn_getflag to output server flags in decimal format.
**	19-Nov-92 (seiwald)
**	    Surround whole gcn_getflag in ifndef GCF64.
**	15-Mar-93 (edg)
**	    Added gcn_nq_assocs_upd().
**      23-Mar-93 (smc)
**          Forward declared and added return types to gcn_get_rec()
**          and gcn_drop_nq().
**	06-Apr-93 (brucek)
**	    Fixed gcn_nq_create so as to open iiname.all for append, not rw.
**	28-Apr-93 (edg)
**	    Fixed 46332.  GCN was excessively accessing disk rather than
**	    cache on vms clusters causing intermittent nq init/fopen errors.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	16-jul-93 (edg)
**	    Integrate the 6.4/04 change to gcn_nq_add that was contributing
**	    to file contention problems on VAX clusters.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	28-sep-93 (eml)
**	    Revamped the queue update algorithm. Previous versions were
**	    writing NULL(empty) records to disk, causing unnecesary latency
**	    for rewrite operations. BUG #52995
**	09-Nov-93 (brucek)
**	    Corrected calls to MOulongout to set error status correctly.
**	17-Mar-94 (brucek)
**	    Don't count EOF record as part of gcn_records_in_file.
**      07-Dec-94 (liibi01)
**          Cross integration from 6.4(Bug 65236). On addition of new tuples,
**          where changes cause complete flush of queue to disk, remember to
**          reset tuple flag to GCN_REC_UNCHANGED.
**	30-Nov-95 (gordy)
**	    Move MIB stuff to iigcn.c and structure defs to gcnint.h
**	    Added gcn_nq_drop().  Added prototypes.
**	26-Feb-96 (gordy)
**	    Extracted disk filename formation to function to handle
**	    platform dependencies.
**	02-May-96 (rajus01)
**	    When adds/deletes > 10% of total records, set the active_in_cache
**	    counter to the number of records flushed from the queue to 
**	    the disk.
**	    (Test case: Start with empty cache. i.e no records in netutil. Add
**	                few entries in netutil. Note the values of 
**			gcn_records_in_file, gcn_active_in_cache counters for
**			the adds/deletes > 10% case.
**			active_in_cache counter never got changed, even though
**			according to the logic, it should be equal to the
**			number of records flushed to the disk.)
**	11-Jun-96 (rajus01)
**	    Bug# 76799 - gcn_nq_assocs_upd() was updating the 'no_assocs for
**	    the first matching tuple, whereas when we add more databases
**	    to the DBMS servers through config.dat (As in "dblist" case:
**	    (ii.<host>.dbms.<server>.database_list:) ), it is required that
**	    'no_assocs' be updated for all the matching tuples for a given
**	     server.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	27-May-97 (thoda04)
**	    Included gc.h. WIN16 fixes.
**	19-Feb-98 (gordy)
**	    Added gcn_nq_scan(), gcn_nq_qdel() for better dynamic access.
**	 5-Jun-98 (gordy)
**	    Dynamic server classes now maintained in svrclass.nam file.
**	10-Jun-98 (gordy)
**	    Reworked most file handling.  Added records may now reuse
**	    deleted records without rewriting the file.  Deleted record
**	    list maintained to facilitate reuse.  Bulk file rewriting
**	    moved to background processing.
**	16-Jun-98 (gordy)
**	    Added hash access to Name Queues.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitions to gcm.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Jul-04 (gordy)
**	    Allow fewer tuple value sub-fields than actual to be compared.
**	 3-Aug-09 (gordy)
**	    Implement new record format which allows variable length
**	    components.  Record length for a given file can also be
**	    extended if additional space is needed for an entry.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**	23-Apr-2010 (wanfr01) b123139
**	   Add cv.h for CV function definitions
**      20-Oct-2010 (Ralph Loen) Bug 122895
**          For gcn_nq_lock(), on VMS, open files as GCN_RACC_FILE for 
**          optimized file access.  See bug 102423.
**      22-Nov-2010 (Ralph Loen) Bug 122895
**          Cleaned up some cruft in the documentation header left over
**          from cross-integrating.  
*/

static VOID	gcn_rewrite( GCN_QUE_NAME * );
static STATUS	gcn_nq_append( GCN_QUE_NAME *, GCN_TUP *, bool, i4, i4  );
static VOID	gcn_nq_remove( GCN_QUE_NAME *, GCN_TUP_QUE * );
static VOID	gcn_drop_nq( GCN_QUE_NAME * );
static STATUS	gcn_rec_put( GCN_QUE_NAME *, i4 );
static i4	gcn_rec_get( GCN_QUE_NAME * );
static VOID	gcn_rec_flush( GCN_QUE_NAME * );
static STATUS	gcn_read_rec( GCN_QUE_NAME *, u_i1 *, i4  );
static VOID	gcn_read_rec0( GCN_DB_REC0 *, GCN_TUP * );
static STATUS	gcn_read_rec1( GCN_DB_REC1 *, GCN_TUP *, i4 * );
static STATUS	gcn_write_rec1( GCN_TUP *, i4, GCN_DB_REC1 * );
static i4	gcn_record_length( GCN_TUP *tup, i4 tup_id );


/*
** Name: gcn_nq_file
**
** Description:
**	Form a disk filename for a name queue.  The host name should
**	be provided for non-global queues.
**
** Input:
**	type		Name of queue.
**	host		Name of host (NULL if global queue).
**
** Output:
**	filename	Disk filename.
**
** Returns:
**	VOID
**
** History:
**	26-Feb-96 (gordy)
**	    Created.
*/

VOID
gcn_nq_file( char *type, char *host, char *filename )
{
    i4		len, plen, slen;

    /*
    ** Filename template: II<type>[_<host>]
    */
    plen = 2;
    len = STlength( type );
    slen = (host && *host) ? STlength( host ) + 1 : 0;

    /*
    ** Adjust filename for platform limitations:
    **   drop prefix if type and suffix OK; 
    **   drop suffix if prefix and type OK;
    **   otherwise, drop both prefix and suffix.
    */
    if ( plen + len + slen > LO_NM_LEN )
	if ( len + slen <= LO_NM_LEN )
	    plen = 0;
	else  if ( plen + len <= LO_NM_LEN )
	    slen = 0;
	else
	    plen = slen = 0;

    STprintf( filename, "%s%s%s%s", 
	      plen ? "II" : "", type, slen ? "_" : "", slen ? host : "" );

    /*
    ** Finally, truncate the filename if it is too long
    ** (hopefully this will never happen).
    */
    if ( STlength( filename ) > LO_NM_LEN )
	filename[ LO_NM_LEN ] = '\0';
    
    return;
}


/*
** Name: gcn_nq_init - create/open a new name queue
**
** Description:
**	Given a name queue type, file name, and other bits, returns a
**	name queue structure pointer for use with other gcn_nq routines.
**	Sorry, no analogous close operation.
**
** Inputs:
**	type			string name of queue
**
**	filename		final path component of file to open
**				if null, no file will back up the queue
**
**	common			boolean; true if must check file for updates
**				from other processes
**
**	transient		server registry subject to bedchecking
**
**	subfields		number of , separated subfields in the
**				tup.val - used by gcn_tup_compare
**
** Returns:
**	GCN_QUE_NAME *		if successful
**	0			if failed
**
** Side effects:
**	may log error message
**
** History:
**	24-Jun-91 (seiwald)
**	    Written.
**	23-Jul-91 (seiwald)
**	    Open empty file if one not already present - function omitted
**	    during last rewrite.
**	10-Jun-98 (gordy)
**	    Reworked most file handling.  Added records may now reuse
**	    deleted records without rewriting the file.  Deleted record
**	    list maintained to facilitate reuse.  Bulk file rewriting
**	    moved to background processing.
**	 3-Aug-09 (gordy)
**	    Set default record length and check existing files for the
**	    actual record length.  Dynamically allocate queue name.
*/

GCN_QUE_NAME *
gcn_nq_init( char *type, char *filename, 
	     bool common, bool transient, i4  subfields )
{
    GCN_QUE_NAME	*nq;
    ER_ARGUMENT		earg[1];
    STATUS 		status;

    if ( ! (nq = (GCN_QUE_NAME *)MEreqmem( 0, sizeof( *nq ), TRUE, NULL )) )
    {
	earg->er_value = ERx("name queue");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );
	return( NULL );
    }

    if ( ! (nq->gcn_type = STalloc( type )) )
    {
	earg->er_value = ERx("name queue ID");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );
	return( NULL );
    }

    nq->gcn_filename = filename ? STalloc( filename ) : NULL;
    nq->gcn_file = NULL;
    STRUCT_ASSIGN_MACRO( IIGCn_static.now, nq->gcn_next_cleanup );
    nq->gcn_transient = transient;
    nq->gcn_shared = common;
    nq->gcn_incache = FALSE;
    nq->gcn_rec_len = sizeof( GCN_DB_RECORD );	/* Default record length */
    nq->gcn_logical_eof = 0;
    nq->gcn_active = 0;
    nq->gcn_records_added = 0;
    nq->gcn_records_deleted = 0;
    nq->gcn_subfields = subfields;
    nq->gcn_hash = NULL;
    nq->gcn_rec_list = NULL;
    QUinit( &nq->gcn_deleted );
    QUinit( &nq->gcn_tuples );

    /*
    ** Initialization of temp queues is complete.
    */
    if ( ! nq->gcn_filename )
    {
	QUinsert( &nq->q, &IIGCn_static.name_queues );
    	return( nq );
    }

    /*
    ** Open file for reading to see if it exists and to check
    ** the record length.  If it can't be opened, create the
    ** file.
    */
    if ( (status = gcn_fopen( nq->gcn_filename, "r", 
    			      &nq->gcn_file, nq->gcn_rec_len )) != OK )
	status = gcn_fopen( nq->gcn_filename, "w", 
    				    &nq->gcn_file, nq->gcn_rec_len );
    else
    {
    	GCN_DB_RECORD	rec;

	SIfseek( nq->gcn_file, (i4) 0, SI_P_START );

	/*
	** Read first record, or part of first record.  We just 
	** need enough to determine the record type and length.  
	** Assume default length if an error occurs.
	*/
	if ( gcn_read_rec( nq, (u_i1 *)&rec, sizeof( rec ) ) == OK )
	    switch( rec.rec1.gcn_rec_type )
	    {
	    case GCN_DBREC_V0 :
		/* Default record length */
		break;

	    case GCN_DBREC_V1 :
	    case GCN_DBREC_DEL :
		nq->gcn_rec_len = rec.rec1.gcn_rec_len;
		break;

	    case GCN_DBREC_EOF :
	    	/* No records in file, use default record length */
		break;

	    default :
		/*
		** V0 records start with a tuple ID which is either 
		** GCN_DBREC_V0 or has bits set in reserved portions 
		** of the record type, in which case the default 
		** record length is used.  Anything else is an 
		** invalid format.
		*/
		if ( ! (rec.rec1.gcn_rec_type & GCN_DBREC_V0_MASK) )
		{
		    status = E_GC011F_GCN_BAD_RECFMT;
		    SIclose( nq->gcn_file );
		    nq->gcn_file = NULL;
		}
	        break;
    	    }
    }

    if ( status != OK )
    {
	earg->er_value = nq->gcn_filename;
	earg->er_size = STlength( nq->gcn_filename );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0101_GCN_FILE_OPEN, NULL, 1, (PTR)earg );
	gcu_erlog( 0, IIGCn_static.language, status, NULL, 0, NULL );

	MEfree( (PTR)nq );
	return( NULL );
    }

    QUinsert( &nq->q, &IIGCn_static.name_queues );
    SIclose( nq->gcn_file );
    nq->gcn_file = NULL;

    return( nq );
}


/*
** Name: gcn_nq_create - create a new name queue
**
** Description:
**	Creates a name queue for dynamically defined server class
**	and adds a line into the class file so the new class will 
**	be remembered.
**
**	The input queue name will be upper-cased.
**
** Inputs:
**	type			string name of queue
**
** Returns:
**	GCN_QUE_NAME *		NULL if error.
**
** History:
**	05-Nov-92 (brucek)
**	    Written.
**	26-Feb-96 (gordy)
**	    Extracted disk filename formation to function to handle
**	    platform dependencies.
**	17-Feb-98 (gordy)
**	    Hostname now available in global info.
**	 5-Jun-98 (gordy)
**	    Dynamic server classes now maintained in svrclass.nam file.
**	 3-Aug-09 (gordy)
*8	    It is OK to uppercase the input parameter, don't need to copy.
*/

GCN_QUE_NAME *
gcn_nq_create( char *type )
{
    STATUS		status;
    FILE		*file;
    GCN_QUE_NAME	*nq;
    ER_ARGUMENT		earg[1];
    char		filename[MAX_LOC];

    CVupper( type );
    gcn_nq_file( type, IIGCn_static.hostname, filename );

    if ( ! (nq = gcn_nq_init( type, filename, FALSE, TRUE, 1 )) )
	return( NULL );

    /* 
    ** update class file 
    */
    if ( (status = gcn_fopen( GCN_SVR_FNAME, "a", &file, 0 )) != OK )
    {
	earg->er_value = GCN_SVR_FNAME;
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0101_GCN_FILE_OPEN, NULL, 1, (PTR)earg );
	gcu_erlog( 0, IIGCn_static.language, status, NULL, 0, (PTR)earg );
	return( NULL );
    }

    SIfprintf( file, "%s\t%d\t%s\t%s\n", type, 1, "local", "transient" );
    SIclose( file );

    return( nq );
}


/*
** Name: gcn_nq_drop
**
** Description:
**	Frees the resources associated with a Name Queue and removes
**	it from the Name Database.
**
** Input:
**	nq		Name Queue.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 4-Dec-95 (gordy)
**	    Created.
**	16-Jun-98 (gordy)
**	    Added hash access to Name Queues.
*/

VOID
gcn_nq_drop( GCN_QUE_NAME *nq )
{
    /*
    ** Free resources associated with name queue.
    */
    gcn_drop_nq( nq );
    if ( nq->gcn_hash )  gcn_nq_unhash( nq, FALSE );

    if ( nq->gcn_filename )
    {
	MEfree( (PTR)nq->gcn_filename );
	nq->gcn_filename = NULL;
    }

    /*
    ** Free the name queue.
    */
    QUremove( &nq->q );
    MEfree( (PTR)nq );

    return;
}


/*
** Name: gcn_nq_find - find a name queue given its type
**
** Description:
**	Scans the list of known name queues, returning a pointer to
**	the name queue structure for the requested queue.
**
** Inputs:
**	type			string name of queue
**
** Returns:
**	GCN_QUE_NAME *		if found
**	0			if unknown
**
** History:
**	24-Jun-91 (seiwald)
**	    Written.
*/

GCN_QUE_NAME *
gcn_nq_find( char *type )
{
    QUEUE *q;

    for( 
	 q = IIGCn_static.name_queues.q_next;
	 q != &IIGCn_static.name_queues;
	 q = q->q_next 
       )
    {
	GCN_QUE_NAME *nq = (GCN_QUE_NAME *)q;

	if ( ! STbcompare( nq->gcn_type, 0, type, 0, TRUE ) )
	    return( nq );
    }

    return( NULL );
}


/*
** Name: gcn_nq_lock - prepare a name queue for access
**
** Description:
**	Prepares a name queue for access via gcn_nq_add, gcn_nq_del,
**	and gcn_nq_ret.  If only gcn_nq_ret is to be called, the rw
**	flag may be false.  Otherwise, it must be true.
**
** Inputs:
**	nq	name queue for updating
**	rw	open for read/write
**
** Returns:
**	STATUS
**
** Side effects:
**	Opens (and possibly closes) a file.
**	May log error status.
**
** History:
**	24-Jun-91 (seiwald)
**	    Written.
**	23-Jul-91 (alan)
**	    Support for incremental name file update.
**	10-Jul-92 (fredb)
**	    Fix another MPE problem.  The use of LOfaddpath is causing bad
**	    filenames to be generated.  Implemented the same sort of fix as
**	    byranp did in 9/89.
**	10-Jun-98 (gordy)
**	    Reworked most file handling.  Added records may now reuse
**	    deleted records without rewriting the file.  Deleted record
**	    list maintained to facilitate reuse.  Bulk file rewriting
**	    moved to background processing.
**	16-Jun-98 (gordy)
**	    Added hash access to Name Queues.
**	10-Jul-98 (gordy)
**	    Handle NULL filename in trace statement.
**	 3-Aug-09 (gordy)
**	    Added new variable length record format.  Declare default
**	    record buffer.  Use dynamic storage if actual record length
**	    exceeds default size.  Return status codes.
**	8-Sep-09 (bonro01)
**	    Fix syntax error introduced by previous change which fails
**	    on Solaris.
**      20-Oct-2010 (Ralph Loen) Bug 122895
**          On VMS, open files as GCN_RACC_FILE for optimized file access.
*/

STATUS
gcn_nq_lock( GCN_QUE_NAME *nq, bool rw )
{
    GCN_DB_RECORD	dflt_rec;
    GCN_DB_RECORD	*record = &dflt_rec;
    LOCATION		loc;
    LOINFORMATION	loc_info;
    STATUS		status;
    char		*mode = rw ? "rw" : "r";
    i4			flag = LO_I_LAST;
    i4			record_length;
    ER_ARGUMENT		earg[1];

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_lock [%s]: file '%s'%s mode %s\n", nq->gcn_type, 
		   nq->gcn_filename ? nq->gcn_filename : "<none>", 
		   nq->gcn_shared ? " SHARED" : "", rw ? "UPDATE" : "READ" );

    if ( ! nq->gcn_filename  ||  (nq->gcn_incache  &&  ! nq->gcn_shared) )
	return( OK );

    /* 
    ** test queue timestamp against last-modified date on file 
    */
# ifndef hp9_mpe
    if ( ( status = NMloc( ADMIN, PATH, (char *)NULL, &loc ) ) != OK )
    {
	earg->er_value = nq->gcn_type;
	earg->er_size = STlength( nq->gcn_type );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0101_GCN_FILE_OPEN, NULL, 1, (PTR)earg );
	gcu_erlog( 0, IIGCn_static.language, status, NULL, 0, (PTR)earg );
	return( status );
    }

    LOfaddpath( &loc, "name", &loc );
    LOfstfile( nq->gcn_filename, &loc );
# else
    if ( (status = NMloc( FILES, FILENAME, nq->gcn_filename, &loc )) != OK )
    {
	err[0] = nq->gcn_type;
	gca_erlog( 0, 0, E_GC0101_GCN_FILE_OPEN, (CL_ERR_DESC *)0, 1, err );
	gca_erlog( 0, 0, status, (CL_ERR_DESC *)0, 0, err );
	return( status );
    }
# endif

    LOinfo( &loc, &flag, &loc_info );

    if ( nq->gcn_incache  &&  ! TMcmp( &loc_info.li_last, &nq->gcn_last_mod ) )
	return( OK );

    /*
    ** Clear the cache, associated counters,
    ** and hash table.  Load the name queue file.
    */
    gcn_drop_nq( nq );
    if ( nq->gcn_hash )  gcn_nq_unhash( nq, TRUE );
    nq->gcn_incache = TRUE;
    STRUCT_ASSIGN_MACRO( loc_info.li_last, nq->gcn_last_mod );
    record_length = nq->gcn_rec_len;

    status = gcn_fopen( nq->gcn_filename, mode, &nq->gcn_file, record_length );

    if ( status != OK )
    {
	earg->er_value = nq->gcn_filename;
	earg->er_size = STlength( nq->gcn_filename );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0101_GCN_FILE_OPEN, NULL, 1, (PTR)earg );
	gcu_erlog( 0, IIGCn_static.language, status, NULL, 0, (PTR)earg );
	return( status );
    }

    if ( record_length > sizeof( GCN_DB_RECORD )  &&
    	 ! (record = (GCN_DB_RECORD *)MEreqmem(0, record_length, FALSE, NULL)) )
    {
	earg->er_value = ERx("DB record buffer");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );
	goto fail_exit;
    }

    SIfseek( nq->gcn_file, (i4) 0, SI_P_START );

    /*
    ** Read records until EOF or record marking logical EOF.
    */
    while( gcn_read_rec( nq, (u_i1 *)record, record_length ) == OK  &&
	   record->rec1.gcn_rec_type != GCN_DBREC_EOF )
    {
	GCN_TUP	tup;
	i4	tuple_id = 0;
	bool	deleted = FALSE;

	/*
	** The record number of the logical EOF is
	** also the count of the number of records
	** in the file.  Bump for current record.
	*/
	nq->gcn_logical_eof++;

	/*
	** V0 records overload the record type with a tuple ID.
	** These records are identified by the use of reserved
	** bits.  The tuple ID is saved and type forced to V0.
	*/
	if ( record->rec1.gcn_rec_type & GCN_DBREC_V0_MASK )
	{
	    tuple_id = record->rec0.gcn_tup_id;
	    record->rec1.gcn_rec_type = GCN_DBREC_V0;
	}

	switch( record->rec1.gcn_rec_type )
	{
	case GCN_DBREC_V0 :
	    /*
	    ** V0 records have a separate deleted flag.
	    */
	    if ( record->rec0.gcn_invalid )
	    {
	    	deleted = TRUE;
		break;
	    }
	    
	    gcn_read_rec0( &record->rec0, &tup );
	    break;

	case GCN_DBREC_V1 :
	    if ( record->rec1.gcn_rec_len != record_length )
	    {
		if ( IIGCn_static.trace >= 1 )  
		    TRdisplay( "Invalid DB record length: %d\n", 
			       record->rec1.gcn_rec_len );

	    	status = E_GC011F_GCN_BAD_RECFMT;
		goto error_exit;
	    }

	    status = gcn_read_rec1( &record->rec1, &tup, &tuple_id );
	    if ( status != OK )  goto error_exit;
	    break;

	case GCN_DBREC_DEL :
	    if ( record->rec1.gcn_rec_len != record_length )
	    {
		if ( IIGCn_static.trace >= 1 )  
		    TRdisplay( "Invalid DB record length: %d\n", 
			       record->rec1.gcn_rec_len );

	    	status = E_GC011F_GCN_BAD_RECFMT;
		goto error_exit;
	    }

	    deleted = TRUE;
	    break;

	default :
	    if ( IIGCn_static.trace >= 1 )  
		TRdisplay( "Invalid DB record type: %d\n", 
			   record->rec1.gcn_rec_type );

	    status = E_GC011F_GCN_BAD_RECFMT;
	    goto error_exit;
	}

	if ( deleted )
	    status = gcn_rec_put( nq, nq->gcn_logical_eof - 1 );
	else  if ( (status = gcn_nq_append( nq, &tup, FALSE, 
					    nq->gcn_logical_eof - 1, 
					    tuple_id )) == OK )
	    nq->gcn_active++;

	if ( status != OK )  goto fail_exit;
    }

    /*
    ** Files opened for writing are left open
    ** for gcn_nq_unlock() to update.
    */
    if ( ! rw )
    {
	SIclose( nq->gcn_file );
	nq->gcn_file = NULL;
    }

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_lock: records = %d active = %d\n",
		   nq->gcn_logical_eof, nq->gcn_active );

    status = OK;
    goto done;

  error_exit:

    earg->er_value = nq->gcn_filename;
    earg->er_size = STlength( nq->gcn_filename );
    gcu_erlog( 0, IIGCn_static.language, 
	       E_GC0101_GCN_FILE_OPEN, NULL, 1, (PTR)earg );
    gcu_erlog( 0, IIGCn_static.language, status, NULL, 0, (PTR)earg );

  fail_exit :

    SIclose( nq->gcn_file );
    nq->gcn_file = NULL;

  done :

    if ( record  &&  record != &dflt_rec )  MEfree( (PTR)record );
    return( status );
}


/*
** Name: gcn_nq_unlock - finish a name queue update
**
** Description:
**	Finish accessing a name queue.
**
** Inputs:
**	nq		name queue you're done with
**
** Returns:
**	void
**
** Side effects:
**	May write and close a file.
**
** History:
**	24-Jun-91 (seiwald)
**	    Written.
**	24-Jul-91 (alan)
**	    Support for incremental name file update.
**      07-Dec-94 (liibi01)
**          Cross integration from 6.4(Bug 65236). On addition of new tuples,
**          where changes cause complete flush of queue to disk, remember to 
**          reset tuple flag to GCN_REC_UNCHANGED.
**	02-May-96 (rajus01)
**	    When adds/deletes > 10% of total records, set the active_in_cache
**	    counter to the number of records flushed from the queue to 
**	    the disk.
**	    (Test case: Start with empty cache. i.e no records in netutil. Add
**	                few entries in netutil. Note the values of 
**			gcn_records_in_file, gcn_active_in_cache counters for
**			the adds/deletes > 10% case.
**			active_in_cache counter never got changed, eventhough
**			according to the logic, it should be equal to the
**			number of records flushed to the disk.)
**	10-Jun-98 (gordy)
**	    Reworked most file handling.  Added records may now reuse
**	    deleted records without rewriting the file.  Deleted record
**	    list maintained to facilitate reuse.  Bulk file rewriting
**	    moved to background processing.
**	16-Jun-98 (gordy)
**	    Added hash access to Name Queues.
**	 3-Aug-09 (gordy)
**	    Added new variable length record format.  Declare default
**	    record buffer.  Use dynamic storage if actual record length
**	    exceeds default size.  Check for file re-write condition
**	    caused by need to extend record length.
*/

VOID
gcn_nq_unlock( GCN_QUE_NAME *nq )
{
    GCN_TUP_QUE		*tq, *dq, *ntq;
    GCN_DB_RECORD	dflt_rec;
    GCN_DB_RECORD	*record = &dflt_rec;
    ER_ARGUMENT		earg[1];
    STATUS		status = OK;
    char		*mode = "rw";
    i4			n;
    bool		new_eof = FALSE;

    if ( IIGCn_static.trace >= 3 )  
	TRdisplay( "gcn_nq_unlock [%s]\n", nq->gcn_type );

    /* 
    ** Shortcut if no modification 
    */
    if ( ! nq->gcn_filename )  return;

    if ( ! nq->gcn_records_added  &&  ! nq->gcn_records_deleted )
    {
	if ( nq->gcn_file )		/* close file if open */
	{
	    SIclose( nq->gcn_file );
	    nq->gcn_file = NULL;
	}

	return;
    }

    if ( nq->gcn_transient )  TMnow( &IIGCn_static.cache_modified );

    /*
    ** If queue is flagged for forced rewrite, 
    ** the disk file should be overwritten.
    */
    if ( nq->gcn_rewrite )
    {
	if ( nq->gcn_file )		/* close file if open */
	{
	    SIclose( nq->gcn_file );
	    nq->gcn_file = NULL;
	}

    	mode = "w";
    }

    if ( (status = gcn_fopen( nq->gcn_filename, mode, 
			      &nq->gcn_file, nq->gcn_rec_len )) != OK )
    {
	earg->er_value = nq->gcn_filename;
	earg->er_size = STlength( nq->gcn_filename );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0101_GCN_FILE_OPEN, NULL, 1, (PTR)earg );
	gcu_erlog( 0, IIGCn_static.language, status, NULL, 0, (PTR)earg );
	return;
    }

    /* 
    ** We want to do the least amount of work, so rewrite
    ** the file if the number active records is less than
    ** the number of records to be updated.
    */
    if ( nq->gcn_rewrite  ||
         (nq->gcn_active + nq->gcn_records_added - nq->gcn_records_deleted) 
		    <= max( nq->gcn_records_added, nq->gcn_records_deleted ) )
    {
	if ( IIGCn_static.trace >= 4 )
	    TRdisplay( "gcn_nq_unlock: rewriting disk file\n" );

	gcn_rewrite( nq );
    }
    else  if ( nq->gcn_rec_len > sizeof( GCN_DB_RECORD )  &&
	       ! (record = (GCN_DB_RECORD *)MEreqmem( 0, nq->gcn_rec_len, 
						      FALSE, NULL )) )
    {
	earg->er_value = ERx("DB record buffer");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );
	goto done;
    }
    else
    {
	/*
	** New tuples are at the end of the tuple queue
	** and have a negative record number.
	*/
	for( 
	     tq = (GCN_TUP_QUE *) nq->gcn_tuples.q_prev;
	     tq != (GCN_TUP_QUE *)&nq->gcn_tuples  &&  tq->gcn_rec_no < 0; 
	     tq = (GCN_TUP_QUE *)tq->gcn_q.q_prev 
	   )
	{
	    /*
	    ** Find a record for the new tuple.  First
	    ** search the deleted tuple queue for a
	    ** record to reuse (watch out for tuples
	    ** which have not been assigned to a record).
	    */
	    for( 
		 dq = (GCN_TUP_QUE *)nq->gcn_deleted.q_next;
		 dq != (GCN_TUP_QUE *)&nq->gcn_deleted; 
		 dq = ntq
	       )
	    {
		ntq = (GCN_TUP_QUE *)dq->gcn_q.q_next;

		/*
		** Save the record number and free the tuple.
		*/
		tq->gcn_rec_no = dq->gcn_rec_no;
		MEfree( (PTR)QUremove( &dq->gcn_q ) );

		/*
		** We are done if the deleted tuple
		** was associated with a record.
		*/
		if ( tq->gcn_rec_no >= 0 )  
		{
		    /*
		    ** We decrement the active count here
		    ** for the record just deleted.  It
		    ** will be incremented below for the
		    ** new tuple.
		    */
		    nq->gcn_active--;
		    break;
		}
	    }

	    /*
	    ** Use an inactive record if no deleted records available.
	    */
	    if ( tq->gcn_rec_no < 0 )  tq->gcn_rec_no = gcn_rec_get( nq );

	    /*
	    ** Finally, extend the file if no deleted records available.
	    */
	    if ( tq->gcn_rec_no < 0 )
	    {
		tq->gcn_rec_no = nq->gcn_logical_eof++;
		new_eof = TRUE;
	    }

	    MEfill( nq->gcn_rec_len, 0, (PTR)record );
	    record->rec1.gcn_rec_type = GCN_DBREC_V1;
	    record->rec1.gcn_rec_len = nq->gcn_rec_len;

	    if ( gcn_write_rec1( &tq->gcn_tup, 
	    			 tq->gcn_tup_id, &record->rec1 ) != OK )
	    {
		earg->er_value = nq->gcn_filename;
		earg->er_size = STlength( earg->er_value );
		gcu_erlog( 0, IIGCn_static.language, 
			   E_GC0120_GCN_BAD_RECLEN, NULL, 1, (PTR)earg );
		goto done;
	    }

	    if ( IIGCn_static.trace >= 4 )
		TRdisplay( "gcn_nq_unlock: writing record %d\n",
			   tq->gcn_rec_no );

	    SIfseek( nq->gcn_file, 
	    	     tq->gcn_rec_no * nq->gcn_rec_len, SI_P_START);
	    SIwrite( nq->gcn_rec_len, (PTR)record, &n, nq->gcn_file );

	    nq->gcn_active++;		/* Update active count */
	}

	/*
	** Process the deleted tuple queue.
	*/
	for( 
	     tq = (GCN_TUP_QUE *)nq->gcn_deleted.q_next;
	     tq != (GCN_TUP_QUE *)&nq->gcn_deleted; 
	     tq = ntq
	   )
	{
	    ntq = (GCN_TUP_QUE *)tq->gcn_q.q_next;
	    QUremove( &tq->gcn_q );

	    /*
	    ** Mark disk record as invalid (deleted).
	    ** Added records are not assigned a record 
	    ** number until flushed to disk and do not 
	    ** require updating if deleted prior to 
	    ** being flushed.
	    */
	    if ( tq->gcn_rec_no >= 0 )
	    {
		MEfill( nq->gcn_rec_len, 0, (PTR)record );
		record->rec1.gcn_rec_type = GCN_DBREC_DEL;
		record->rec1.gcn_rec_len = nq->gcn_rec_len;

		if ( IIGCn_static.trace >= 4 )
		    TRdisplay( "gcn_nq_unlock: marking record %d deleted\n",
			       tq->gcn_rec_no );

		SIfseek( nq->gcn_file, 
			 tq->gcn_rec_no * nq->gcn_rec_len, SI_P_START );
		SIwrite( nq->gcn_rec_len, (PTR)record, &n, nq->gcn_file );

		nq->gcn_active--;		/* Update active count */
		gcn_rec_put( nq, tq->gcn_rec_no );
	    }

	    MEfree( (PTR)tq );
	}

	if ( new_eof )
	{
	    /*
	    ** Update logical end-of-file marker
	    */
	    MEfill( nq->gcn_rec_len, 0, (PTR)record );
	    record->rec1.gcn_rec_type = GCN_DBREC_EOF;

	    if ( IIGCn_static.trace >= 4 )
		TRdisplay( "gcn_nq_unlock: new logical EOF record %d\n",
			   nq->gcn_logical_eof );

	    SIfseek( nq->gcn_file, 
	    	     nq->gcn_logical_eof * nq->gcn_rec_len, SI_P_START );
	    SIwrite( nq->gcn_rec_len, (PTR)record, &n, nq->gcn_file );
	}
    }

    nq->gcn_records_added = 0;
    nq->gcn_records_deleted = 0;

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_unlock: records = %d active = %d\n",
		   nq->gcn_logical_eof, nq->gcn_active );

  done:

    if ( record  &&  record != &dflt_rec )  MEfree( (PTR)record );
    SIclose( nq->gcn_file );
    nq->gcn_file = NULL;

    return;
}


/*
** Name: gcn_rewrite
**
** Description:
**	Rewrite name queue file removing invalid (deleted) records.
**	All active records are written sequentially from start of 
**	file and the logical EOF marker is updated.  The deleted 
**	tuple queue and record list are flushed.
**
**	The name queue file must be opened prior to calling this routine.
**	The name queue file is not closed by this routine.
**
** Input:
**	nq		Name queue.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	10-Jun-98 (gordy)
**	    Created.
**	16-Jun-98 (gordy)
**	    Added hash access to Name Queues.
**	 3-Aug-09 (gordy)
**	    New record format.  Declare default sized record buffer.
**	    Use dynamic storage if record length exceeds default size.
*/

static VOID
gcn_rewrite( GCN_QUE_NAME *nq )
{
    GCN_TUP_QUE		*tq, *ntq;
    GCN_DB_RECORD	dflt_rec;
    GCN_DB_RECORD	*record = &dflt_rec;
    ER_ARGUMENT		earg[1];
    i4			n;

    if ( nq->gcn_rec_len > sizeof( GCN_DB_RECORD )  &&
	 ! (record = (GCN_DB_RECORD *)MEreqmem( 0, nq->gcn_rec_len, 
						FALSE, NULL )) )
    {
	earg->er_value = ERx("DB record buffer");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );
        return;
    }

    SIfseek( nq->gcn_file, (i4)0, SI_P_START );

    /*
    ** Flush the deleted record list and tuple queue.
    */
    gcn_rec_flush( nq );

    for( 
	 tq = (GCN_TUP_QUE *)nq->gcn_deleted.q_next;
	 tq != (GCN_TUP_QUE *)&nq->gcn_deleted; 
	 tq = ntq
       )
    {
	ntq = (GCN_TUP_QUE *)tq->gcn_q.q_next;
	MEfree( (PTR)QUremove( &tq->gcn_q ) );
    }

    /*
    ** Write active tuples.
    */
    nq->gcn_active = 0;
    nq->gcn_records_added = 0;
    nq->gcn_records_deleted = 0;

    for( 
	 tq = (GCN_TUP_QUE *)nq->gcn_tuples.q_next;
	 tq != (GCN_TUP_QUE *)&nq->gcn_tuples; 
	 tq = (GCN_TUP_QUE *)tq->gcn_q.q_next
       )
    {
	MEfill( nq->gcn_rec_len, 0, (PTR)record );
	record->rec1.gcn_rec_type = GCN_DBREC_V1;
	record->rec1.gcn_rec_len = nq->gcn_rec_len;

	if ( gcn_write_rec1( &tq->gcn_tup, 
			     tq->gcn_tup_id, &record->rec1 ) != OK )
	{
	    earg->er_value = nq->gcn_filename;
	    earg->er_size = STlength( earg->er_value );
	    gcu_erlog( 0, IIGCn_static.language, 
		       E_GC0120_GCN_BAD_RECLEN, NULL, 1, (PTR)earg );
	    goto done;
	}

	SIwrite( nq->gcn_rec_len, (PTR)record, &n, nq->gcn_file );
	tq->gcn_rec_no = nq->gcn_active++;
    }

    /*
    ** Write logical EOF marker.
    */
    nq->gcn_logical_eof = nq->gcn_active;
    MEfill( nq->gcn_rec_len, 0, (PTR)record );
    record->rec1.gcn_rec_type = GCN_DBREC_EOF;
    SIwrite( nq->gcn_rec_len, (PTR)record, &n, nq->gcn_file );
    nq->gcn_rewrite = FALSE;

    /*
    ** If the queue is empty, we might as well
    ** flush the hash table as well.
    */
    if ( nq->gcn_hash  &&  ! nq->gcn_active )
	gcn_nq_unhash( nq, TRUE );

  done:

    if ( record != &dflt_rec )  MEfree( (PTR)record );
    return;
}


/*
** Name: gcn_nq_compress
**
** Description:
**	Check name queue file and compress (remove unused records)
**	when percentage of file used drops below compression point.
**
**	Do not call this routine when queue is locked (gcn_nq_lock()).
**
** Input:
**	nq		Name queue.
**
** Output:
**	None.
**
** Returns:
**	bool		TRUE if name queue file was compressed.
**
** History:
**	10-Jun-98 (gordy)
**	    Created.
**	15-Jul-04 (gordy)
**	    If file can't be opened, still make sure to unlock.
**	 3-Aug-09 (gordy)
**	    Support extended record lengths.
*/

bool
gcn_nq_compress( GCN_QUE_NAME *nq )
{
    ER_ARGUMENT	earg[1];
    STATUS	status;
    bool	compressed = FALSE;
    i4		file_usage = 100;
    
    /*
    ** We don't compress transient files since 
    ** they are usually pretty small.
    */
    if ( nq->gcn_transient  ||  gcn_nq_lock( nq, TRUE ) != OK )
	return( FALSE );

    if ( nq->gcn_logical_eof )
	file_usage = (nq->gcn_active * 100) / nq->gcn_logical_eof;

    if ( file_usage < IIGCn_static.compress_point )
    {
	/*
	** Open the file if not opened by gcn_nq_lock().
	*/
	if ( ! nq->gcn_file  &&
	     (status = gcn_fopen( nq->gcn_filename, "w", 
	 			  &nq->gcn_file, nq->gcn_rec_len )) != OK )
	{
	    earg->er_value = nq->gcn_filename;
	    earg->er_size = STlength( nq->gcn_filename );
	    gcu_erlog( 0, IIGCn_static.language, 
		       E_GC0101_GCN_FILE_OPEN, NULL, 1, (PTR)earg );
	    gcu_erlog( 0, IIGCn_static.language, status, NULL, 0, (PTR)earg );
	}
	else
	{
	    if ( IIGCn_static.trace >= 3 )
		TRdisplay( "gcn_nq_compress [%s]: rewriting disk file\n",
			   nq->gcn_type );

	    gcn_rewrite( nq );
	    compressed = TRUE;
	}
    }

    /*
    ** The only thing left to do is close the file.  Since 
    ** we locked the name queue above, we now unlock the 
    ** name queue which will close the file for us.
    */
    gcn_nq_unlock( nq );

    return( compressed );
}


/*
** Name: gcn_nq_add
**
** Description:
**	Given a name queue and a new tuple, appends the tuple onto the
**	name queue tuple queue.
**
**	Can only be called between bracketing gcn_nq_lock and gcn_nq_unlock
**	calls.
**
** Inputs:
**	nq		name queue
**	tup		tuple to add
**
** Returns:
**	i4		Count added (0 or 1)
**
** History:
**	 10-Jun-98 (gordy)
**	    Made a cover for local function gcn_nq_append().
**	 3-Aug-09 (gordy)
**	    Status now returned by gcn_nq_append().
*/

i4
gcn_nq_add( GCN_QUE_NAME *nq, GCN_TUP *tup )
{
    i4  count = (gcn_nq_append( nq, tup, TRUE, 0, 0 ) == OK) ? 1 : 0;

    nq->gcn_records_added += count;

    return( count );
}


/*
** Name: gcn_nq_del - delete tuple(s) from a name queue
**
** Description:
**	Given a name queue and a tuple mask (a tuple potentially with 
**	wildcards (*'s) for values), deletes all matching tuples from
**	the name queue.
**
**	Can only be called between bracketing gcn_nq_lock and gcn_nq_unlock
**	calls.
**
** Inputs:
**	nq		name queue
**	tupmask		tuple mask to delete; *'s are wildcards
**
** Returns:
**	Count deleted (may be zero)
**
** Side effects:
**	May write the file.
**
** History:
**	24-Jun-91 (seiwald)
**	    Written.
**	24-Jul-91 (alan)
**	    Defer QUremove/MEfree till gcn_tq_unlock (for incremental update).
**	10-Jun-98 (gordy)
**	    Reworked most file handling.  Added records may now reuse
**	    deleted records without rewriting the file.  Deleted record
**	    list maintained to facilitate reuse.  Bulk file rewriting
**	    moved to background processing.
**	16-Jun-98 (gordy)
**	    Added hash access to Name Queues.
*/

i4
gcn_nq_del( GCN_QUE_NAME *nq, GCN_TUP *tupmask )
{
    GCN_TUP_QUE	*tq, *ntq;
    i4		deleted = 0;

    for( 
	 tq = (GCN_TUP_QUE *)nq->gcn_tuples.q_next;
	 tq != (GCN_TUP_QUE *)&nq->gcn_tuples; 
	 tq = ntq
       )
    {
	ntq = (GCN_TUP_QUE *)tq->gcn_q.q_next;

	if ( gcn_tup_compare( &tq->gcn_tup, tupmask, nq->gcn_subfields ) )
	{
	    gcn_nq_remove( nq, tq );
	    deleted++;
	}
    }

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_del [%s]: %s,%s,%s = %d\n", nq->gcn_type, 
		   tupmask->uid, tupmask->obj, tupmask->val, deleted );

    return( deleted );
}


/*
** Name: gcn_nq_qdel
**
** Description:
**	Delete a list of queue entries.  The entry list may be obtained
**	by calling gcn_nq_scan().
**
** Inputs:
**	nq		Name queue.
**	count		Number of queue entries in vector.
**	qvec		Array of queue pointers.
**
** Returns:
**	i4		Number of entries deleted.
**
** History:
**	19-Feb-98 (gordy)
**	    Created.
**	10-Jun-98 (gordy)
**	    Reworked most file handling.  Added records may now reuse
**	    deleted records without rewriting the file.  Deleted record
**	    list maintained to facilitate reuse.  Bulk file rewriting
**	    moved to background processing.
**	16-Jun-98 (gordy)
**	    Added hash access to Name Queues.
*/

i4
gcn_nq_qdel( GCN_QUE_NAME *nq, i4  count, PTR *qvec )
{
    i4	i, deleted = 0;

    for( i = 0; i < count; i++ )
    {
	GCN_TUP_QUE *tq = (GCN_TUP_QUE *)qvec[ i ];

	gcn_nq_remove( nq, tq );
	deleted++;
    }

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_qdel [%s]: %d deleted\n", nq->gcn_type, deleted );

    return( deleted );
}


/*
** Name: gcn_nq_ret - find tuple(s) in a name queue
**
** Description:
**	Given a name queue and a tuple mask (a tuple potentially with 
**	wildcards (*'s) for values), find all matching tuples from
**	the name queue.  
**
**	Returns at most 'tupmax' rows; to get all rows, call repeatedly
**	increasing 'tupskip' to skip initial rows.
**
**	Can only be called between bracketing gcn_nq_lock and gcn_nq_unlock
**	calls.
**
** Inputs:
**	nq		name queue
**	tupmask		tuple mask to retrieve; *'s are wildcards
**	tupskip		number of tuples to skip before starting
**	tupmax		max count to retrieve
**	tupvec		ptr to array of GCN_TUP's, count long
**
** Returns:
**	Count retrieved (may be zero)
**
** History:
**	24-Jun-91 (seiwald)
**	    Written.
**	24-Jul-91 (alan)
**	    Ignore tuples marked as "deleted".
*/

i4
gcn_nq_ret( GCN_QUE_NAME *nq, 
	    GCN_TUP *tupmask, i4  tupskip, i4  tupmax, GCN_TUP *tupvec )
{
    GCN_TUP_QUE	*tq;
    i4		count = 0;
    GCN_TUP	*tup = tupvec;

    for( 
	 tq = (GCN_TUP_QUE *)nq->gcn_tuples.q_next;
	 tq != (GCN_TUP_QUE *)&nq->gcn_tuples  &&  tupmax; 
	 tq = (GCN_TUP_QUE *)tq->gcn_q.q_next
       )
	if ( gcn_tup_compare( &tq->gcn_tup, tupmask, nq->gcn_subfields ) )
	    if ( tupskip )
		tupskip--;
	    else 
	    {
		STRUCT_ASSIGN_MACRO( tq->gcn_tup, *tup );
		tup++, tupmax--, count++;
	    }

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_ret [%s]: %s,%s,%s = %d\n", nq->gcn_type, 
		   tupmask->uid, tupmask->obj, tupmask->val, count );

    return( count );
}


/*
** Name: gcn_nq_scan
**
** Description:
**	Retrieve a list of tuples matching a tuple mask.  A scan
**	control block maintains current scan info for subsequent
**	calls providing better performance than gcn_nq_ret() when
**	the result set is likely to be greater than the result
**	vector being used.  The control block (a PTR) should be
**	set to NULL prior to the first call and should not be
**	changed between subsequent calls.
**
**	This routine is also able to return a list of name queue
**	entry pointers corresponding to the tuple list returned,
**	which may be used as input to gcn_nq_qdel() to delete the
**	matched tuples directly rather than using a matching scan
**	by calling gcn_nq_del().
**
** Inputs:
**	nq		Name queue.
**	tupmask		Tuple mask with matching criteria.
**	scan_cb		Scanning position control block, set initially NULL.
**	tupmax		Number of entries to be returned in following vectors.
**	tupvec		Matching tuples, may be NULL.
**	qvec		Matching queue pointers, may be NULL.
**
** Returns:
**	i4		Number of matching entries.
**
** History:
**	19-Feb-98 (gordy)
**	    Created.
*/

i4
gcn_nq_scan( GCN_QUE_NAME *nq, GCN_TUP *tupmask, PTR *scan_cb, 
	     i4  tupmax, GCN_TUP **tupvec, PTR *qvec )
{
    GCN_TUP_QUE	*tq;
    i4		count = 0;

    if ( ! *scan_cb )
	tq = (GCN_TUP_QUE *)nq->gcn_tuples.q_next;
    else
	tq = (GCN_TUP_QUE *)*scan_cb;

    for( ; tq != (GCN_TUP_QUE *)&nq->gcn_tuples  &&  count < tupmax; 
	   tq = (GCN_TUP_QUE *)tq->gcn_q.q_next )
    {
	if ( gcn_tup_compare( &tq->gcn_tup, tupmask, nq->gcn_subfields ) )
	{
	    if ( tupvec )  tupvec[ count ] = &tq->gcn_tup;
	    if ( qvec )  qvec[ count ] = (PTR)tq;
	    count++;
	}
    }

    *scan_cb = (PTR)tq;

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_scan [%s]: %s,%s,%s = %d\n", nq->gcn_type, 
		   tupmask->uid, tupmask->obj, tupmask->val, count );

    return( count );
}


/*
** Name: gcn_nq_assocs_upd - Update no_assocs for a tuple
**
** Description:
**	Given a name queue and a value for listen address, find and update
**	the no_assocs value for a tuple in the cache.  
** Inputs:
**	nq		name queue
**	val		value for the typle = server listen address
**	no_assocs	value to use for updating the tuple's no_assocs
**
** Returns:
**	Nothing
**
** History:
**	15-Mar-93 (edg)
**	    Written.
**      11-Jun-96 (rajus01)
**          Bug# 76799 - gcn_nq_assocs_upd() was updating the 'no_assocs for
**          the first matching tuple, whereas when we add more databases 
**          to the DBMS servers through config.dat (As in "dblist" case:
**          (ii.<host>.dbms.<server>.database_list:) ), it is required that
**          'no_assocs' be updated for all the matching tuples for a given
**           server.
*/

VOID
gcn_nq_assocs_upd( GCN_QUE_NAME *nq, char *val, i4  no_assocs )
{
    GCN_TUP_QUE	*tq;

    if ( nq->gcn_incache )
	for( 
	     tq = (GCN_TUP_QUE *)nq->gcn_tuples.q_next;
	     tq != (GCN_TUP_QUE *)&nq->gcn_tuples; 
	     tq = (GCN_TUP_QUE *)tq->gcn_q.q_next
	   )
	    if ( ! STbcompare( tq->gcn_tup.val, 0, val, 0, TRUE ) )
		tq->gcn_tup.no_assocs = no_assocs;

    return;
}



/*
** Name: gcn_tup_compare() - compare two tuples
**
** Description:
**	Compares two tuples, piece (uid,obj,val) by piece.
**	If subfields is set to 0 (or less), the tup->val 
**	field is compared as a single entity.  If subfields 
**	is greater than 0, tup->val sub-fields are compared 
**	individually upto the minimum of subfields and the
**	number of sub-fields in tup2 (which must not exceed 
**	number of sub-fields in tup1).  There is currently
**	a limit of at most 4 sub-fields.
**
**	Any field or sub-field in tup2 which is empty (EOS) 
**	will not be compared.
**
** Inputs:
**	tup1		Tuple to compare
**	tup2		Tuple to compare, may have wildcards (EOS).
**	subfields	Number of subfields to compare.
**
** Returns:
**	bool		TRUE if they match, FALSE if they don't
**	
** History:
**	21-Mar-89 (seiwald)
**	    Written.
**	16-Jun-98 (gordy)
**	    Made global for use by hash access.
**	15-Jul-04 (gordy)
**	    Rather than compare sub-fields directly, allow for a smaller
**	    (and not more than actual) number of sub-fields to be compared.
**	13-Aug-09 (gordy)
**	    Declare default sized temp buffer.  Use dynamic storage
**	    if actual lengths exceed default size.
*/	

bool
gcn_tup_compare( GCN_TUP *tup1, GCN_TUP *tup2, i4 subfields )
{
    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_tup_compare: %s,%s,%s = %s,%s,%s (%d)\n",
		   tup1->uid, tup1->obj, tup1->val,
		   tup2->uid, tup2->obj, tup2->val, subfields );

    /*
    ** compare the uid value
    */
    if ( tup2->uid[0] && STbcompare( tup1->uid, 0, tup2->uid, 0, TRUE ) )
	return( FALSE );

    if ( tup2->obj[0] && STbcompare( tup1->obj, 0, tup2->obj, 0, TRUE ) )
	return( FALSE );

    if ( tup2->val[0] )
	if ( subfields <= 0 )
	{
	    if ( STbcompare( tup1->val, 0, tup2->val, 0, TRUE ) )
		return( FALSE );
	}
	else 
	{
	    char	tbuff[ 256 ];
	    char	*tmp, *pv1[ 4 ], *pv2[ 4 ];
	    i4		len1, len2, v1, v2;

	    len1 = STlength( tup1->val ) + 1;
	    len2 = STlength( tup2->val ) + 1;

	    tmp = ((len1 + len2) <= sizeof( tbuff ))
		  ? tbuff : (char *)MEreqmem( 0, len1 + len2, FALSE, NULL );

	    if ( ! tmp )  return( FALSE );
	
	    /*
	    ** Extract sub-fields.
	    */
	    v1 = gcu_words( tup1->val, tmp, pv1, ',', 4 );
	    v2 = gcu_words( tup2->val, tmp + len1, pv2, ',', 4 );

	    /*
	    ** Limit comparison to requested number of sub-fields
	    ** Make sure there are enough sub-fields for comparison.
	    */
	    if ( v2 > subfields )  v2 = subfields;
	    if ( v1 < v2 )
	    {
		if ( tmp != tbuff )  MEfree( (PTR)tmp );
		return( FALSE );
	    }
		
	    /*
	    ** Compare the sub-fields.
	    */
	    while( v2-- )
		if ( *pv2[v2]  &&  STbcompare(pv1[v2], 0, pv2[v2], 0, TRUE) )
		{
		    if ( tmp != tbuff )  MEfree( (PTR)tmp );
		    return( FALSE );	
		}

	    if ( tmp != tbuff )  MEfree( (PTR)tmp );
	} 

    return( TRUE );
}


/*
** Name: gcn_nq_append
**
** Description:
**	Given a name queue and a new tuple, appends the tuple onto the
**	name queue tuple queue.
**
**	When loading tuples from a file, the file record number and 
**	tuple ID must be provided.  When adding new tuples, record 
**	number and tuple ID will be ignored.
**
**	Can only be called between bracketing gcn_nq_lock and gcn_nq_unlock
**	calls.
**
** Inputs:
**	nq		name queue
**	tup		tuple to add
**	new_tup		TRUE for new tuples, FALSE when loading from file.
**	rec_no		Record number of tuple in file.
**	tup_id		Tuple ID for MOattach().
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	24-Jun-91 (seiwald)
**	    Written.
**	23-Jul-91 (alan)
**	    Added two args for incremental file update.
**	2-Mar-92 (seiwald)
**	    Fix insertion logic so that items stay in order.
**	29-Sep-92 (brucek)
**	    Add instance arg.
**      28-Apr-93 (edg)
**          Fixed bug 46332.  Don't change gcn_modified to TRUE if flag
**          is GCN_REC_UNCHANGED.
**	10-Jun-98 (gordy)
**	    Reworked most file handling.  Added records may now reuse
**	    deleted records without rewriting the file.  Deleted record
**	    list maintained to facilitate reuse.  Bulk file rewriting
**	    moved to background processing.
**	16-Jun-98 (gordy)
**	    Added hash access to Name Queues.
**	23-Sep-98 (gordy)
**	    Make sure the instance attached is the same as the tuple ID.
**	 3-Aug-09 (gordy)
**	    Return status rather than count.  Reference queue name rather
**	    than copy.
*/

static STATUS
gcn_nq_append( GCN_QUE_NAME *nq, GCN_TUP *tup, 
	       bool new_tup, i4  rec_no, i4 tup_id )
{
    GCN_TUP_QUE	*nt;
    char	*p;
    i4		l_uid = STlength( tup->uid ) + 1;
    i4		l_obj = STlength( tup->obj ) + 1;
    i4		l_val = STlength( tup->val ) + 1;
    i4		reclen;
    STATUS	status;
    ER_ARGUMENT	earg[1];

    /*
    ** Tuple ID only applicable under certain conditions.
    ** Need to establish tuple ID to determine associated
    ** record length.
    */
    if ( ! (IIGCn_static.flags & GCN_MIB_INIT)  ||  ! nq->gcn_transient )
	tup_id = 0;
    else  if (  new_tup )
    {
	SYSTIME	timestamp;
	TMnow( &timestamp );
	tup_id = timestamp.TM_secs;
    }

    /*
    ** Check record length needed for current data
    ** and flag queue for rewrite if record expansion
    ** is needed.
    */
    if ( (reclen = gcn_record_length( tup, tup_id )) > nq->gcn_rec_len )
    {
	nq->gcn_rewrite = TRUE;
    	nq->gcn_rec_len = reclen;
    }

    /*
    ** Memory for UID, object, and value is allocated
    ** in a variable size array at end of the tuple.
    */
    nt = (GCN_TUP_QUE *)MEreqmem( 0, sizeof( *nt ) + l_uid + l_obj + l_val, 
				  TRUE, NULL );
    if ( ! nt )
    {
	earg->er_value = ERx("name queue tuple");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );
	return( E_GC0121_GCN_NOMEM );
    }

    nt->gcn_type = nq->gcn_type;
    nt->gcn_rec_no = new_tup ? -1 : rec_no;

    p = nt->gcn_data;
    nt->gcn_tup.uid = p;
    MEcopy( tup->uid, l_uid, p );
    p += l_uid;
    nt->gcn_tup.obj = p;
    MEcopy( tup->obj, l_obj, p );
    p += l_obj;
    nt->gcn_tup.val = p;
    MEcopy( tup->val, l_val, p );

    if ( nq->gcn_hash  &&  (status = gcn_nq_hadd( nq, nt )) != OK )
    {
	if ( status == FAIL  &&  IIGCn_static.trace >= 1 )
	    TRdisplay( "gcn_nq_hadd: internal processing error!\n" );

	MEfree( (PTR)nt );
	return( status);
    }

    /*
    ** Append to end of queue.
    */
    QUinsert( &nt->gcn_q, nq->gcn_tuples.q_prev );

    /* 
    ** Update Name Server MIB.
    */
    if ( IIGCn_static.flags & GCN_MIB_INIT  &&  nq->gcn_transient )
    {
	nt->gcn_tup_id = tup_id;

	if ( new_tup )
	{
	    do
	    {
	        nt->gcn_tup_id++;
		MOulongout( MO_VALUE_TRUNCATED, (u_i8)nt->gcn_tup_id, 
			    sizeof(nt->gcn_instance), nt->gcn_instance );
		status = MOattach( 0, GCN_MIB_SERVER_ENTRY,
				   nt->gcn_instance, (PTR) nt );
	    } while( status == MO_ALREADY_ATTACHED );
	}
	else
	{
	    MOulongout( MO_VALUE_TRUNCATED, (u_i8)nt->gcn_tup_id, 
			sizeof( nt->gcn_instance ), nt->gcn_instance );
	    status = MOattach( 0, GCN_MIB_SERVER_ENTRY,
			       nt->gcn_instance, (PTR)nt );
	}
    }

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_add [%s]: %s,%s,%s\n",
		   nq->gcn_type, tup->uid, tup->obj, tup->val );

    return( OK );
}


/*
** Name: gcn_nq_remove
**
** Description:
**	Moves tuples from active queue to deleted queue.
**
** Input:
**	nq		Name Queue.
**	tq		Tuple.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	16-Feb-98 (gordy)
**	    Extracted from gcn_nq_del().
*/

static VOID
gcn_nq_remove( GCN_QUE_NAME *nq, GCN_TUP_QUE *tq )
{
    nq->gcn_records_deleted++;
    QUinsert( QUremove( &tq->gcn_q ), &nq->gcn_deleted );
    if ( nq->gcn_hash ) gcn_nq_hdel( nq, tq );

    /* Update Name Server MIB */
    if ( IIGCn_static.flags & GCN_MIB_INIT  &&  nq->gcn_transient )
	MOdetach( GCN_MIB_SERVER_ENTRY, tq->gcn_instance );

    return;
}

 
/* 
** Name: gcn_drop_nq
**
** Description:
**	Empty the name queue tuple and deleted lists.
**
** Input:
**	nq		Name queue.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	10-Jun-98 (gordy)
**	    Added queue for deleted tuples.
*/ 

static VOID
gcn_drop_nq( GCN_QUE_NAME *nq ) 
{
    GCN_TUP_QUE	*tq, *ntq;

    /*
    ** Flush active tuple queue.
    */
    for( 
	 tq = (GCN_TUP_QUE *)nq->gcn_tuples.q_next;
	 tq != (GCN_TUP_QUE *)&nq->gcn_tuples; 
	 tq = ntq
       )
    {
	ntq = (GCN_TUP_QUE *)tq->gcn_q.q_next;
	MEfree( (PTR)QUremove( (QUEUE *)tq ) );
    }

    /*
    ** Flush deleted tuple queue.
    */
    for( 
	 tq = (GCN_TUP_QUE *)nq->gcn_deleted.q_next;
	 tq != (GCN_TUP_QUE *)&nq->gcn_deleted; 
	 tq = ntq
       )
    {
	ntq = (GCN_TUP_QUE *)tq->gcn_q.q_next;
	MEfree( (PTR)QUremove( (QUEUE *)tq ) );
    }

    /*
    ** Flush deleted record list.
    */
    gcn_rec_flush( nq );

    /*
    ** Reset associated counters.
    */
    nq->gcn_logical_eof = 0;
    nq->gcn_active = 0;
    nq->gcn_records_added = 0;
    nq->gcn_records_deleted = 0;

    return;
}


/*
** Name: gcn_rec_put
**
** Description:
**	Save a record number for later retrieval with gcn_rec_get().
**
** Input:
**	nq		Name queue.
**	rec		Record number.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or FAIL if memory allocation error.
**
** History:
**	10-Jun-98 (gordy)
**	    Created.
*/

static STATUS
gcn_rec_put( GCN_QUE_NAME *nq, i4 rec )
{
    /*
    ** Add new sub-list if current sub-list is full.
    */
    if ( ! nq->gcn_rec_list  ||  
	 nq->gcn_rec_list->count >= nq->gcn_rec_list->size )
    {
	GCN_REC_LST	*lst;
	i2		size = nq->gcn_transient ? GCN_LIST_SMALL 
						 : GCN_LIST_LARGE;

	lst = (GCN_REC_LST *)MEreqmem( 0, sizeof( GCN_REC_LST ) + 
					  ((size - 1) * sizeof( i4 )), 
				       TRUE, NULL );
	if ( ! lst )  
	{
	    ER_ARGUMENT	earg[1];

	    earg->er_value = ERx("deleted record list");
	    earg->er_size = STlength( earg->er_value );
	    gcu_erlog( 0, IIGCn_static.language, 
		       E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );
	    return( FAIL );
	}

	lst->next = nq->gcn_rec_list;
	lst->size = size;
	lst->count = 0;
	nq->gcn_rec_list = lst;
    }

    /*
    ** Save available record in next slot.
    */
    nq->gcn_rec_list->rec[ nq->gcn_rec_list->count++ ] = rec;

    return( OK );
}


/*
** Name: gcn_rec_get
**
** Description:
**	Returns the next available record number as saved by
**	gcn_rec_put().  A -1 is returned if no records available.
**
** Input:
**	nq		Name Queue.
**
** Output:
**	None.
**
** Returns:
**	i4		Record number, -1 if no record available.
**
** History:
**	10-Jun-98 (gordy)
**	    Created.
*/

static i4
gcn_rec_get( GCN_QUE_NAME *nq )
{
    i4	rec = -1;

    /*
    ** Free the current sub-list if empty.
    */
    while( nq->gcn_rec_list  &&  ! nq->gcn_rec_list->count )
    {
	GCN_REC_LST	*lst = nq->gcn_rec_list;

	nq->gcn_rec_list = lst->next;
	MEfree( (PTR)lst );
    }

    /*
    ** Get next available record.
    */
    if ( nq->gcn_rec_list )
	rec = nq->gcn_rec_list->rec[ --nq->gcn_rec_list->count ];

    return( rec );
}


/*
** Name: gcn_rec_flush
**
** Description:
**	Frees resources associated with the record list.
**
** Input:
**	nq		Name queue.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	10-Jun-98 (gordy)
**	    Created.
*/

static VOID
gcn_rec_flush( GCN_QUE_NAME *nq )
{
    while( nq->gcn_rec_list )
    {
	GCN_REC_LST	*lst = nq->gcn_rec_list;

	nq->gcn_rec_list = lst->next;
	MEfree( (PTR)lst );
    }

    return;
}


/*
** Name: gcn_fopen 	- Open a named file.
**
** Description:
**	This routine opens a named file which contains the type of database 
**	identified by its name. The file descriptor is returned.
**
** Inputs:
**	name	- The file name.
**	mode  - read/write mode.
**	reclen - record length for those with fixed record lengths
**
** Outputs:
**	file
**
** Returns:
**	OK  - succeed	
**	status - Error code
**
** Exceptions:
**	    none
**
** Side Effects:
**	    The file is leaved opened until gcn_closefile is called.
**
** History:
**
**      23-Mar-88 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Revamped.  Added reclen.
**	29-Jun-92 (fredb)
**	    Integrate bryanp's 1989 changes for MPE (hp9_mpe):
**      	17-Sep-89 (bryanp)
**          		Fixed a bug in the hp9_mpe filename generation code 
**			introduced during 61b3ug integration.
**      	18-Sep-89 (bryanp)
**          		Fixed another bug in the hp9_mpe filename generation 
**			code which was introduced during 61b3ug integration.
**
*/

STATUS 
gcn_fopen( char *name, char *mode, FILE **file, i4  reclen )
{
    LOCATION	loc;
    STATUS	status;

#ifndef hp9_mpe
    if ( (status = NMloc( ADMIN, FILENAME, name, &loc )) != OK )
	goto error;

    LOfaddpath( &loc, "name", &loc );
    LOfstfile( name, &loc );
#else
    if ( (status = NMloc( FILES, FILENAME, name, &loc )) != OK )
	goto error;
#endif

    if ( ! STcompare( mode, "r" )  &&  (status = LOexist( &loc )) != OK )
	goto error;
		
    if ( reclen )
#ifdef VMS
	status = SIfopen( &loc, mode, GCN_RACC_FILE, reclen, file );
#else
	status = SIfopen( &loc, mode, SI_RACC, reclen, file );
#endif
    else
#ifndef hp9_mpe
	status = SIopen( &loc, mode, file );
#else
	status = SIfopen( &loc, mode, SI_TXT, 256, file );
#endif

    if ( status == OK )  return( OK );

error:

    *file = NULL;

    return( status );
}


/*
** Name: gcn_read_rec - get a single complete gcn record
**
** History:
**	 3-Aug-09 (gordy)
**	    New record format.
*/

static STATUS
gcn_read_rec( GCN_QUE_NAME *nq, u_i1 *record, i4  rec_len )
{
    STATUS	status = OK;
    char	*rec_ptr = (char *)record;
    i4	bytes_read = 0;

    while( rec_len ) 
    {
	status = SIread( nq->gcn_file, rec_len, &bytes_read, rec_ptr );
	if ( status != OK )  break;

	rec_ptr += bytes_read;
	rec_len -= bytes_read;
    }

    return( status );
}


/*
** Name: gcn_read_rec0
**
** Description:
**	Extract components of a V0 DB record.  Input record
**	may be corrupted as a result of processing.
**
** Input:
**	record	V0 DB record buffer.
**
** Output
**	tup	Extracted components.
**
** Returns:
**	VOID
**
** Side-Effects:
**	Input buffer may be corrupted.
**
** History:
**	 3-Aug-09 (gordy)
*8	    Created.
*/

static VOID
gcn_read_rec0( GCN_DB_REC0 *record, GCN_TUP *tup )
{
    /* 
    ** Null terminate strings for backwards compatibility.
    ** Old IIGCN may not have had null terminated strings
    ** in file. The following may result in adding an extra
    ** EOS at the end but will not change the string value.
    ** The field following the string may be overwritten,
    ** so save members needed prior to termination.
    */
    i4 uidlen = min( record->gcn_l_uid, sizeof( record->gcn_uid ) );
    i4 objlen = min( record->gcn_l_obj, sizeof( record->gcn_obj ) );
    i4 vallen = min( record->gcn_l_val, sizeof( record->gcn_val ) );

    record->gcn_uid[ uidlen ] = EOS;
    record->gcn_obj[ objlen ] = EOS;
    record->gcn_val[ vallen ] = EOS;

    tup->uid = record->gcn_uid;
    tup->obj = record->gcn_obj;
    tup->val = record->gcn_val;

    return;
}


/*
** Name: gcn_read_rec1
**
** Description:
**	Extract components of a V1 DB record.  Input record
**	may be corrupted as a result of processing.
**
** Input:
**	record	V1 DB record buffer.
**
** Output:
**	tup	Extracted components.
**	tup_id	Tuple ID.
**
** Returns:
**	status	OK or error code.
**
** Side-Effects:
**	Input buffer may be corrupted.
**
** History:
**	 3-Aug-09 (gordy)
**	    Created.
*/

static STATUS
gcn_read_rec1( GCN_DB_REC1 *record, GCN_TUP *tup, i4 *tup_id )
{
    static char *empty = "";

    u_i1	*ptr = (u_i1 *)record + sizeof( GCN_DB_REC1 );
    u_i1	*end = (u_i1 *)record + record->gcn_rec_len;
    i4		uidlen = 0;
    i4		objlen = 0;
    i4		vallen = 0;

    tup->uid = empty;
    tup->obj = empty;
    tup->val = empty;
    *tup_id = 0;

    while( (ptr + sizeof( GCN_DB_PARAM )) <= end )
    {
    	GCN_DB_PARAM	param;

	MEcopy( (PTR)ptr, sizeof( GCN_DB_PARAM ), (PTR)&param );
	if ( ! param.gcn_p_type )  break;
	ptr += sizeof( GCN_DB_PARAM );

	if ( (ptr + param.gcn_p_len) > end )
	{
	    if ( IIGCn_static.trace >= 1 )  
		TRdisplay( "Invalid DB record param length: %d\n", 
			   param.gcn_p_len );
	    return( E_GC011F_GCN_BAD_RECFMT );
	}

	switch( param.gcn_p_type )
	{
	case GCN_P_UID :
	    tup->uid = (char *)ptr;
	    uidlen = param.gcn_p_len;
	    break;

	case GCN_P_OBJ :
	    tup->obj = (char *)ptr;
	    objlen = param.gcn_p_len;
	    break;

	case GCN_P_VAL :
	    tup->val = (char *)ptr;
	    vallen = param.gcn_p_len;
	    break;

	case GCN_P_TID :
	    if ( param.gcn_p_len != sizeof( i4 ) )
	    {
		if ( IIGCn_static.trace >= 1 )  
		    TRdisplay( "Invalid DB record tuple ID length: %d\n", 
			       param.gcn_p_len );
		return( E_GC011F_GCN_BAD_RECFMT );
	    }

	    MEcopy( (PTR)ptr, param.gcn_p_len, (PTR)tup_id );
	    break;
    
	default :
	    if ( IIGCn_static.trace >= 1 )  
		TRdisplay( "Invalid DB record param type: %d\n", 
			   param.gcn_p_type );
	    return( E_GC011F_GCN_BAD_RECFMT );
	}

	ptr += param.gcn_p_len;
    }

    /*
    ** Strings should already be NULL terminated, but
    ** append an EOS just to be sure.  This overwrites
    ** the parameter which follows, but all needed info
    ** has been extracted at this point.
    */
    tup->uid[ uidlen ] = EOS;
    tup->obj[ objlen ] = EOS;
    tup->val[ vallen ] = EOS;

    return( OK );
}


/*
** Name: gcn_write_rec1
**
** Description:
**	Builds a V1 DB record.  Adds parameters for the record
**	components.  If tuple ID is not 0, also adds a parameter
**	for the tuple ID.
**
**	The formatted record size is checked against the record length.
**
** Input:
**	tup	Record components.
**	tup_id	Tuple ID.
**	record	Record buffer.
**
** Output:
**	record	Formatted record.
**	
** Returns:
**	STATUS	FAIL if formatted components exceed record length.
**
** History:
**	 3-Aug-09 (gordy)
**	    Created.
*/

static STATUS
gcn_write_rec1( GCN_TUP *tup, i4 tup_id, GCN_DB_REC1 *record )
{
    GCN_DB_PARAM	param;
    u_i1		*ptr = (u_i1 *)record + sizeof( GCN_DB_REC1 );
    i4			uidlen = STlength( tup->uid ) + 1;
    i4			objlen = STlength( tup->obj ) + 1;
    i4			vallen = STlength( tup->val ) + 1;
    i4			size = sizeof( GCN_DB_REC1 ) + 
    			       (sizeof( GCN_DB_PARAM ) * 3) + 
			       uidlen + objlen + vallen;

    if ( tup_id )  size += sizeof( GCN_DB_PARAM ) + sizeof( i4 );

    if ( size > record->gcn_rec_len )
    {
	if ( IIGCn_static.trace >= 1 )  
	    TRdisplay( "Formatted reoord size %d exceeds record length %d\n", 
		       size, record->gcn_rec_len );
    	return( FAIL );
    }

    if ( tup_id )
    {
	param.gcn_p_type = GCN_P_TID;
	param.gcn_p_len = sizeof( i4 );
	MEcopy( (PTR)&param, sizeof( param ), (PTR)ptr );
	ptr += sizeof( param );
	MEcopy( (PTR)&tup_id, sizeof( i4 ), (PTR)ptr );
	ptr += sizeof( i4 );
    }

    param.gcn_p_type = GCN_P_UID;
    param.gcn_p_len = uidlen;
    MEcopy( (PTR)&param, sizeof( param ), (PTR)ptr );
    ptr += sizeof( param );
    MEcopy( (PTR)tup->uid, uidlen, (PTR)ptr );
    ptr += uidlen;

    param.gcn_p_type = GCN_P_OBJ;
    param.gcn_p_len = objlen;
    MEcopy( (PTR)&param, sizeof( param ), (PTR)ptr );
    ptr += sizeof( param );
    MEcopy( (PTR)tup->obj, objlen, (PTR)ptr );
    ptr += objlen;

    param.gcn_p_type = GCN_P_VAL;
    param.gcn_p_len = vallen;
    MEcopy( (PTR)&param, sizeof( param ), (PTR)ptr );
    ptr += sizeof( param );
    MEcopy( (PTR)tup->val, vallen, (PTR)ptr );
    ptr += vallen;

    return( OK );
}


/*
** Name: gcn_record_length
**
** Description:
**	Determine record length needed to hold tuple data.
**
** Input:
**	tup	Tuple strings.
**	tup_id	Tuple ID.
**
** Output:
**	None.
**
** Returns:
**	i4	Record length.
**
** History:
**	 3-Aug-09 (gordy)
**	    Created.
*/

static i4
gcn_record_length( GCN_TUP *tup, i4 tup_id )
{
    i4	record_length = sizeof( GCN_DB_RECORD );  /* Default record length */
    i4	uidlen = STlength( tup->uid ) + 1;
    i4	objlen = STlength( tup->obj ) + 1;
    i4	vallen = STlength( tup->val ) + 1;
    
    /*
    ** The minimum size required is the base record structure
    ** plus three parameters for UID, OBJ and VAL plus an
    ** optional parameter if TID is not zero.
    */
    i4	size = sizeof( GCN_DB_REC1 ) + (sizeof( GCN_DB_PARAM ) * 3) + 
					uidlen + objlen + vallen;

    if ( tup_id )  size += sizeof( GCN_DB_PARAM ) + sizeof( i4 );

    /*
    ** If size needed exceeds default record length,
    ** start with base size and increment in steps
    ** until record length is sufficient.
    */
    if ( size > record_length )
	for( 
	     record_length = GCN_RECSIZE_BASE; 
	     size > record_length; 
	     record_length += GCN_RECSIZE_INC 
	   );

    return( record_length );
}

