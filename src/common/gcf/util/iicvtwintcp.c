/*
** Copyright (c) 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <cv.h>
#include    <ep.h>
#include    <er.h>
#include    <gc.h>
#include    <gv.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <pm.h>
#include    <si.h>
#include    <st.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <iiapi.h>

/*
** Name: iicvtwintcp.c
**
** Description:
**	iicvtwintcp converts "wintcp" to "tcp_ip" in the protocol field
**	of all vnode definitions in the name server node file.
**	It is intended to help customers on Windows migrate from 
**	"wintcp" to "tcp_ip" since "wintcp" is being deprecated.
**	The config.dat file is also modified to configure "tcp_ip"
**	on the default incoming port instead of "wintcp" or both.
**	A restore capability is also included in case it is
**	necessary to go back to using "wintcp".
**
**	This utility cannot be run while the Ingres name server is
**	running; this is to prevent losing concurrent updates.
**	Additionally, the utility cannot be run while another instance
**	of it is running against the same vnode files.
**	Though not recommended, this can be overriden with the -force 
**	parameter.  This utility may be run standalone or from the
**	Ingres installer.
**
**	NOTE: The following info is also displayed in usage and help:
**
**	Usage: iicvtwintcp [-help] [-verbose] [-noupdate] [-force]
**		           [-action convert_wintcp | convert_tcp_ip |
**		                    restore]
**		           [-bf <path>filename] [-nf <path>filename]
**		           [-scope all | vnodes | config]
**
**		-help	  Display command usage with details.
**		-verbose  Verbose mode (displays each individual record
**		          affected; otherwise only totals are displayed).
**		-noupdate Only report what the utility would have done,
**		          but don't update name server or backup files.
**		          Useful for determining impact prior to conversion.
**		          If name server is running, must also use -force
**		          parameter.  Also, with -verbose parameter, can
**		          be used to produce a report from which the vnode
**	                  updates could be done manually from one of the
**		          Ingres network config utilities.
**		-force	  Run even if the name server is up or another
**		          instance of this program is concurrently running.
**		          A warning message will be in either case.
**		          This parameter is useful with '-noupdate' parameter
**		          or running against a copy of the name server
**		          node file.  USE WITH CAUTION.
**		-action	  Action to be performed:
**		          *NOTE*: If not specified, display usage.
**		          Prevents unintentional conversion if someone
**		          just types in command name with no parameters.
**		            convert_wintcp: Convert wintcp to tcp_ip
**		            convert_tcp_ip: Convert tcp_ip to wintcp
**		            restore       : Restore converted protocol entries
**			                    back to prior values
**		-bf       Backoff/restore filename; default is
**		          <II_SYSTEM>\ingres\files\name\IINODE_hostname.BK
**		          May be specified with or without path name; if no
**		          path, defaults path to <II_SYSTEM>\ingres\files\name.
**		          NOTE:
**		          Written to as output if -action is convert_wintcp
**		                                          or convert_tcp_ip.
**		          Read from  as input  if -action is restore and
**		          is required for a successful restore operation.
**		-nf       Name server filename; default is
**		          <II_SYSTEM>\ingres\files\name\IINODE_hostname
**		          May be specified with or without path name; if no
**		          path, defaults path to <II_SYSTEM>\ingres\files\name.
**		-scope    Convert/restore following entities:
**		            all: vnode + config (default)
**		            vnodes: vnodes only
**		            config: config.dat file only
**
**	Returns:
**		>=0 if OK where return code is # of records that were
**		    or would be (for -noupdate) modified.
**	        -1  if FAIL
**
**
** Examples:
**	(All should be run while Ingres name server (iigcn) is down.)
**
**	1. Convert 'wintcp' to 'tcp_ip' in current Ingres installation.
**	     iicvtwintcp -action convert_wintcp
**
**	2. Restore converted records back to 'wintcp' after running example 1.
**	     iicvtwintcp -action restore
**
**	3. Convert 'tcp_ip' to 'wintcp' in current Ingres installation.
**	   Opposite of example 1, but similar to example 2 except that
**	   ALL 'tcp_ip' records will be set to 'wintcp', not just the
**	   ones that were originally converted by '-action convert_wintcp'.
**	   Likely usage would be if restore is desired but backup/restore
**	   file was lost.
**	     iicvtwintcp -action convert_tcp_ip
**
**	4. List connection (NODE) information for all vnodes.
**	     iicvtwintcp -action convert_wintcp -noupdate -verbose
**	
**
**
** History:
**	1-Oct-2008 (lunbr01)  Sir 119985
**	    Created to convert "wintcp" to "tcp_ip" in all vnodes
**	    as part of "wintcp" deprecation.
**	19-Nov-2008 (lunbr01)
**	    Changed BOOL to bool to fix compile errors on Linux/Unix.
**	    Changed some function parms from unsigned to signed to
**	    eliminate "signedness differs" compiler warning on Linux/Unix.
**	19-Dec-2008 (lunbr01)  Bug 121413
**	    Only 1st 3 chars of protocol were getting converted.  Changed
**	    "sizeof(pointer to protocol) -1" (=3) to actual prot data length.
**      21-Jan-2009 (lunbr01)  Sir 118847
**	    On Vista, this binary should only run in elevated mode if
**	    doing a conversion on the real Ingres name server NODE file
**	    and/or config.dat.  This change will ensure they get an error
**	    explaining the elevation requirement.
**	 4-Sep-09 (gordy)
**	    Implemented new DB record format.  As a temporary fixed,
**	    replaced GCN_DB_RECORD use with GCN_DB_REC0.  Check when
**	    reading records and fail if anything but the original
**	    record format is found.
**	25-Sep-09 (gordy)
**	    Fix support for new record format.  New format records are
**	    processed and converted to the original format.  Extended
**	    length records are not supported.  Length of data items
**	    are restricted to the original limits - processing fails
**	    if a longer item is found.
*/


/*
** Structure Definitions
*/

/*
** CVT_BK_RECORD - This is the record written to the backup file when
** doing a conversion.
**
** It is read as input for the restore option (-action restore).
** The unique KEY of this record is uid,obj,val fields combined, which
** is essentially the entire name server NODE record contents.
**
** The GCN constants for record fields have been increased.  For
** backward compatibility in the backup file, local constants with
** the original lengths are used.
*/

#define	CVT_UID_LEN	32
#define	CVT_VAL_LEN	64
#define	CVT_OBJ_LEN	256

typedef struct _CVT_BK_RECORD
{
    i4		cvtbk_rec_no;			/* Rec no of name server rec */

    /* Following fields are same as what is updated to name server file  */
    i4		cvtbk_l_uid;			/* - uid		 */
    char	cvtbk_uid[CVT_UID_LEN];
    i4		cvtbk_l_obj;			/* - obj (vnode name)    */
    char	cvtbk_obj[CVT_OBJ_LEN];
    i4		cvtbk_l_val;			/* - value (host,prot,port) */
    char	cvtbk_val[CVT_VAL_LEN];

    i4		cvtbk_l_val_old;		/* Len of value before cnvrsn */
    char	cvtbk_val_old[CVT_VAL_LEN]; /* Value before cnvrsn */
    SYSTIME	cvtbk_timestamp;		/* date/time converted */
}   CVT_BK_RECORD;


typedef struct _NQ_CACHE
{
    QUEUE	  nc_q;
    i4		  nc_nq_rec_no;
    GCN_DB_REC0   nc_rec;
}   NQ_CACHE;


/*
** Forward and/or External function references
*/

static	STATUS	convert_protocol( char *nq_filename, char *bk_filename, i4 *converted );
static	STATUS	restore_protocol( char *nq_filename, char *bk_filename, i4 *restored );
static	VOID	get_nq_filename( char *type, char *filename );
static	VOID	get_bk_filename( char *type, char *filename );
static	VOID	get_lk_filename( char *type, char *filename );
static	STATUS	open_file( char *name, char *mode, FILE **file, i4 reclen );
static	STATUS	read_rec( FILE *file, PTR record, u_i4 record_len, i4 rec_no );
static	STATUS	write_nq_rec( GCN_DB_REC0 *nq_record, FILE *nq_file, i4 rec_no );
static	STATUS	write_bk_rec( GCN_DB_REC0 *nq_record, FILE *bk_file, i4 rec_no, i4 nq_l_val_old, char *nq_val_old );
static	STATUS	load_nq_file( FILE *nq_file, QUEUE *nq_cache );
static	STATUS	convert_del_record( GCN_DB_RECORD *record );
static	STATUS	convert_v1_record( GCN_DB_RECORD *record );
static	STATUS	free_nq_cache( QUEUE *nq_cache );
static	VOID	find_nq_rec( CVT_BK_RECORD *bk_record, NQ_CACHE **nq_cache_start_p, GCN_DB_REC0 **nq_record_p, i4 *nq_rec_no_p );
static	VOID	get_vnode_protocol( char *buff, u_i4 buff_len, char **protocol, u_i4 *protocol_len );
static	STATUS	process_config_protocol( char *from_protocol, char *to_protocol, i4 *converted );
static	STATUS	get_cmdline_args( i4  argc, char **argv );
static	VOID	usage(bool detail);
static	bool	is_gcn_up();
static	bool	is_other_cvtwintcp_up();
static	STATUS	lk_unlock();
static	VOID	iiapi_checkError( char *api_fn_name, IIAPI_GENPARM *genParm );
static	VOID 	iiapi_term();
static	VOID	initialize_processing();
static	VOID	terminate_processing();

FUNC_EXTERN void        PMmLowerOn( PM_CONTEXT * );


/*
** Static and Global Variables
*/

static	char	*pgmname  = NULL;
static	char	installation_id[3];
static	char	*nq_filetype=ERx("NODE");
static	SYSTIME	pgm_run_timestamp;
static	bool	verbose  = FALSE;
static	bool	update   = TRUE;
static	bool	force    = FALSE;
#define ACTION_CONVERT_WINTCP	'W'
#define ACTION_CONVERT_TCP_IP	'T'
#define ACTION_RESTORE		'R'
static	char	action   = 0;
static	char	nq_filename[MAX_LOC]={EOS};  /* Name server queue file name */
static	char	bk_filename[MAX_LOC]={EOS};  /* Backup file name */
static	char	lk_filename[MAX_LOC]={EOS};  /* Lock/audit file name */
#define SCOPE_VNODES 0x00000001
#define SCOPE_CONFIG 0x00000002
static	int	scope = SCOPE_VNODES | SCOPE_CONFIG;
static  bool	iiapi_initialized = FALSE;
static  II_PTR	envHandle = (II_PTR)NULL;
static	QUEUE	nq_cache;
static	bool	debug  = TRUE;   /* must manually set/recompile */
#define UNLOCKED_C	"unlocked"
static	char	unlk_hdr[]=ERx("unlocked");
static	i4	unlk_hdr_len=sizeof(unlk_hdr);
#define	PROTOCOL_WINTCP		"wintcp"
#define	PROTOCOL_TCP_IP		"tcp_ip"



/*
** Name: main
**
** Description:
**	Initialize and process command line.
**	Call appropriate function to either convert "wintcp" to "tcp_ip"
**	or restore back to original values.
**	Terminate/cleanup.
**
** Returns:
**	>=0 if OK where return code is # of records that were 
**	    or would be (for -noupdate) modified.
**      -1  if FAIL
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

i4
main( i4  argc, char **argv )
{

    STATUS	status = OK;
    i4		num_recs_chgd = 0;	/* -1 indicates an error */
#define ERROR_RC -1
    pgmname = ( argc >= 1 ? argv[0] : "iicvtwintcp" );

    initialize_processing();

    if (get_cmdline_args(argc,argv) == FAIL)
	PCexit(ERROR_RC);

    get_nq_filename( nq_filetype, nq_filename ); /* Get name server file name */
    get_lk_filename( nq_filetype, lk_filename ); /* Get lock/audit  file name */

    /*
    ** Make sure there is no conflict with other processes that might 
    ** concurrently be running and updating the name server file...
    ** to avoid losing any updates.
    ** In particular, the Ingres name server (GCN) should be down.
    ** Can't depend on filelocking because GCN doesn't keep the name
    ** server files open all the time.
    ** Also, beware of other instances of ourselves.
    */
    if (is_gcn_up() || is_other_cvtwintcp_up())
	if (force)
	{
	    SIprintf("WARNING: '-force' override specified - processing continuing in spite of\n");
	    SIprintf("         potential concurrent activity on name server file.\n");
	}
	else
	{
	    SIprintf("ERROR: Name server file may be open to other processes...aborting processing!\n");
	    PCexit(ERROR_RC);
	}

    if ( action == ACTION_CONVERT_WINTCP || action == ACTION_CONVERT_TCP_IP )
	status = convert_protocol( nq_filename, bk_filename, &num_recs_chgd );
    else
    if ( action == ACTION_RESTORE )
	status = restore_protocol( nq_filename, bk_filename, &num_recs_chgd );
    else
    {
	SIprintf( "\nSpecify which '-action' option desired.\n" );
	usage(FALSE);  /* Display usage if -action not specified */
    }

    if ( status != OK )
	num_recs_chgd = ERROR_RC;

    terminate_processing();
    SIprintf( "\nProgram '%s' Ended: status=%d, return code=%d\n",
		pgmname, status, num_recs_chgd );
    PCexit(num_recs_chgd);
    return(num_recs_chgd);
}


/*
** Name: convert_protocol
**
** Description:
**	Invoked for "-action convert_wintcp" or "-action convert_tcp_ip"
**	options on command line.
**	Opens disk file associated with a Name Node Queue.  Reads records
**	in disk file and converts network protocol values of "wintcp"
**	to "tcp_ip" or vice versa.  Information required to restore the file, if needed,
**	is written to a backup file.  If "noupdate" option specified,
**	actual updates are not done...just reported.
**
** Input:
**	nq_filename	Name queue (NODE) filename from command line
**	                = NULL if not present
**	bk_filename	Backup/restore    filename from command line
**	                = NULL if not present
**
** Output:
**	converted	Number of records that were (or "would have been"
**		        in case of "noupdate" option) converted.
**
** Side Effects:
**	Report (stdout) of modifications that were made, where amount
**	of detail is dependent on verbose option.
**
** Returns:
**	OK   - if successful
**	FAIL - if any errors encountered
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
**	19-Nov-2008 (lunbr01)
**	    Changed "converted" parm from unsigned to signed
**	    to eliminate "signedness differs" compiler warning.
**	19-Dec-2008 (lunbr01)  Bug 121413
**	    Only 1st 3 chars of protocol were getting converted.  Changed
**	    "sizeof(pointer to protocol) -1" (=3) to actual prot data length.
**	 4-Sep-09 (gordy)
**	    Check for unsupported record format.  Abort processing for now.
**	25-Sep-09 (gordy)
**	    Add processing for new record format.
*/

static STATUS
convert_protocol( char *nq_filename, char *bk_filename, i4 *converted )
{
    STATUS		status;
    GCN_DB_REC0		nq_record;
    CVT_BK_RECORD	bk_record;
    FILE		*nq_file;
    FILE		*bk_file = NULL;
    char		*mode = (update ? ERx("rw") : ERx("r"));
    char		*protocol = NULL;
    u_i4		protocol_len = 0;
    i4			save_config_converted = 0;
    i4			records = 0;    /* in name queue file */
    i4			deleted = 0;
    i4			skipped = 0;
    i4			save_gcn_l_val;
    char		save_gcn_val[CVT_VAL_LEN];
    char		*from_protocol = NULL;
    char		*to_protocol = NULL;

    if ( action == ACTION_CONVERT_WINTCP )
    {
	from_protocol = PROTOCOL_WINTCP;
	to_protocol   = PROTOCOL_TCP_IP;
    }
    else  /* action == ACTION_CONVERT_TCP_IP */
    {
	from_protocol = PROTOCOL_TCP_IP;
	to_protocol   = PROTOCOL_WINTCP;
    }

    *converted = 0;

    if ( !update )
	SIprintf( "   ( Report Run Only: -noupdate specified )\n" );

    /*
    ** Verify to_protocol is supported in config.dat
    */
    if ( (status = process_config_protocol( from_protocol, to_protocol, converted )) != OK )
	return( FAIL );
    save_config_converted = *converted;  /* Save # config recs changed...for rptg summary below */

    if ( !(scope & SCOPE_VNODES) )  /* If not converting vnodes, we're done. */
	return( OK );

    SIprintf( "** Converting '%s' to '%s' in name server %s file for all vnodes. **\n",
	      from_protocol, to_protocol, nq_filetype);

    status = open_file( nq_filename, mode, &nq_file, sizeof(nq_record) );
    if ( status != OK )
    {
        SIprintf( "ERROR: Could not open name server file '%s' with mode '%s': 0x%x\n",
		  nq_filename, mode, status );
	return( FAIL );
    }

    if ( (!bk_filename) || !(*bk_filename) )
	get_bk_filename( nq_filetype, bk_filename );

    if ( update )
    {
	status = open_file( bk_filename, "a", &bk_file, sizeof(bk_record) );
	if ( status != OK )
	{
	    SIprintf( "ERROR: Could not open backup file '%s' for append mode: 0x%x\n",
			bk_filename, status );
	    return( FAIL );
	}
    }

    while( read_rec( nq_file, (PTR)&nq_record, sizeof(nq_record), 0 ) == OK  &&  nq_record.gcn_tup_id != GCN_DBREC_EOF )
    {
	records++;
	if ( verbose && (records == 1) )  SIprintf( "\nDetails (-verbose):\n");

	/*
	** V0 records start with a tuple identifier which
	** is always 0 (actually, may be marked as EOF, but
	** that was handled above).  V1 records start with
	** a record type which indicates EOF, V0, V1 and
	** DEL records.
	*/
	switch( nq_record.gcn_tup_id )
	{
	case GCN_DBREC_V0 :  /* Nothing to do */	break;

	case GCN_DBREC_V1 :
	    if ( convert_v1_record( (GCN_DB_RECORD *)&nq_record ) != OK )
	    {
		SIclose( nq_file );
		if ( bk_file ) SIclose( bk_file );
		return( FAIL );
	    }
	    break;

	case GCN_DBREC_DEL :
	    if ( convert_del_record( (GCN_DB_RECORD *)&nq_record ) != OK )
	    {
		SIclose( nq_file );
		if ( bk_file ) SIclose( bk_file );
	        return( FAIL );
	    }
	    break;

	default :
	    /*
	    ** This is an unknown format, abort processing.
	    */
	    SIprintf( "An unsupported record format has been found.\n" );
	    SIprintf( "Unable to perform conversion.  Processing aborted.\n" );
	    SIclose( nq_file );
	    if ( bk_file ) SIclose( bk_file );
	    return( FAIL );
	}

	if ( nq_record.gcn_invalid )  
	{
	    deleted++;
	    if ( verbose ) 
	    	SIprintf( "%s record %d: deleted\n", nq_filetype, records );
	    continue;
	}

	/*
	** Skip any record which does not contain one of the basic
	** components.  This is most likely due to a problem during
	** conversion into original format.
	*/
	if ( 
	     ! nq_record.gcn_l_uid  ||  
	     ! nq_record.gcn_l_obj  ||  
	     ! nq_record.gcn_l_val 
	   )
	{
	    skipped++;
	    if ( verbose )
	    	SIprintf( "%s record %d: skipped\n", nq_filetype, records );
	    continue;
	}

	if ( verbose )
	{
	    if ( nq_record.gcn_l_uid < CVT_UID_LEN )
		nq_record.gcn_uid[ nq_record.gcn_l_uid ] = '\0';
	    if ( nq_record.gcn_l_obj < CVT_OBJ_LEN )
		nq_record.gcn_obj[ nq_record.gcn_l_obj ] = '\0';
	    if ( nq_record.gcn_l_val < CVT_VAL_LEN )
		nq_record.gcn_val[ nq_record.gcn_l_val ] = '\0';

	    SIprintf( "\n%s record %d: %s(uid='%s') vnode='%s', . . .\n", 
		      nq_filetype, records,
		      nq_record.gcn_uid[0] == '*' ? "Global" : "Private", 
		      nq_record.gcn_uid, 
		      nq_record.gcn_obj );
	    SIprintf( " . . . ConnInfo='%s' (current)\n",
		      nq_record.gcn_val );
	}

	/*
	** Save off prior (old) value before converting.
	*/
	save_gcn_l_val = nq_record.gcn_l_val;
	MEcopy( nq_record.gcn_val, save_gcn_l_val, &save_gcn_val );

	/*
	** Check if record should be converted.
	*/
	get_vnode_protocol(nq_record.gcn_val, nq_record.gcn_l_val, &protocol, &protocol_len);
	if ( STbcompare(protocol, protocol_len, from_protocol, 0, TRUE) == 0 )
	{
	    (*converted)++;
	    if ( verbose )
	    {
		SIprintf( "       <converting '%s' to '%s'>\n",
			    from_protocol, to_protocol );
	    }
	    if ( !update )
		continue;
	    MEcopy(to_protocol, protocol_len, protocol); /* convert protocol */
	    if ( (status = write_bk_rec(&nq_record, bk_file, records, save_gcn_l_val, save_gcn_val)) != OK )
		return( FAIL );
	    if ( verbose )
	    {
		SIprintf( " . . . ConnInfo='%s' (new)\n",
			  nq_record.gcn_val );
	    }
	    if ( (status = write_nq_rec(&nq_record, nq_file, records)) != OK )
		return( FAIL );
	} /* endif match on from_protocol */
    } /* end while reading nq records */

    SIclose( nq_file );
    if ( bk_file )
	SIclose( bk_file );

    SIprintf( "\nSummary of %s file %s:\n", nq_filetype, nq_filename );
    SIprintf( "Records     : %d\n", records );
    SIprintf( " - Active   : %d\n", records - deleted );
    SIprintf( " - Inactive : %d (i.e., deleted)\n", deleted );
    SIprintf( " - Skipped  : %d (may need manual update)\n", skipped );
    SIprintf( " - Converted: %d %s\n", (*converted) - save_config_converted, update ? "" : "(not done because '-noupdate' specified)" );
    if ( update )
	SIprintf( "\nConversion backoff/restore file is %s\n", bk_filename );

    return( status );
}


/*
** Name: restore_protocol
**
** Description:
**	Invoked for "-action restore" option on command line.
**	Opens and reads a backup/restore file created from one or more
**	prior runs of this program with "-action convert_wintcp".
**	For each backup record, find the corresponding record (if it
**	still exists) in the name queue (NODE) file.  Then restore
**	the network protocol value from "tcp_ip" back to "wintcp".
**	The goal is to enable the user to go back to "wintcp" if needed.
**
**	Another goal is that the restore will be successful (and
**	complete) no matter how many times this utility is run
**	to convert "wintcp" to "tcp_ip" and no matter how many
**	manual updates to the vnodes in the interim.
**
**	If "noupdate" option specified, actual updates are not done...
**	just reported.
**
**	Some points/assumptions to keep in mind are:
**	1. The backup file is treated as a transaction file against
**	   the master name queue NODE file.  Direct access to the 
**	   name queue file is used to update it, as opposed to a
**	   sequential read/matchup approach.
**	2. The sequence of records in the backup file is the order that
**	   the records were updated/converted originally.  Each "convert"
**	   run appends to the end of the previous backup file.  Normally,
**	   convert will have only been run once, but may indeed be run
**	   any number of times.  Hence, the order might be something like:
**	        name queue node record 3, run1 timestamp, etc
**	        name queue node record 4, run1 timestamp, etc
**	        name queue node record 7, run1 timestamp, etc
**	        name queue node record 2, run2 timestamp, etc
**	3. Records in the name queue file are NOT kept in keyed order.
**	   That is, even though the Ingres network utilities display the
**	   vnodes in vnode name order, this is not the order they are
**	   maintained in the name queue file.  Instead, they are kept
**	   at the same relative record # in the file until such time
**	   as the name server decides to collapse the file due to a
**	   large number of deletes.  Hence, the record # is not
**	   sufficient for identifying or accessing the name server
**	   record, since the record number "may" change.
**	   However, name server records will often remain at the same
**	   location...it's just not guaranteed.  The records WILL tend
**	   to be in the same relative order within the name server file
**	   with respect to each other.  The exception is when a vnode
**	   is deleted, another one added (which grabs the deleted record),
**	   and then the vnode that was deleted is readded, putting it at
**	   the end of the file (or next available deleted slot).
**	4. The original name queue relative record # is stored into the
**	   backup file by the original conversion(s).  This is for
**	   information purposes only as the key (relative record #) can
**	   change when there are a lot of vnode updates.
**	5. NOTE: All of the following MUST MATCH (ie, not have changed)
**	   for the restore to occur on a particular record:
**	   a. The uid + obj(aka,vnode) + val(aka,host+protocol+port).
**	      NOTE that uid is not to be confused with the login
**            value, which is maintained in the LOGIN name queue
**	      file, not the NODE file; the uid is '*' for global
**	      entries and is the id of the user who created the
**	      vnode for private entries.
**	   WHAT THIS MEANS...is that vnodes that are deleted and later
**	   readded and end up in a different location in the name queue
**	   file will be restored as long as they are identical to what
**	   was converted (ie, nothing changed).
**
**	The following are some examples that all begin with the action:
**	   - convert_wintcp run changes vnodeA protocol from "wintcp" to 
**	     "tcp_ip"
**	Then... perform actions in one of the network utilities followed
**      by a rerun of this utility, but with '-action restore':
**	   Example 1:
**	   - user modifies vnodeA in any field except protocol or the
**	     vnode is not modified at all
**	   - 'restore' run changes vnodeA protocol back to "wintcp"
**	   Example 2:
**	   - user modifies vnodeA protocol field to something other than
**	     "tcp_ip"
**	   - 'restore' run will NOT restore vnodeA protocol to "wintcp"
**	     (because the protocol is no longer what it was converted to).
**	   Example 3:
**	   - user deletes vnodeA
**	   - user re-adds vnodeA with same private/global setting
**	     (so that vnodeA location is the same...grabs deleted entry)
**	     -- if protocol set to "tcp_ip"
**	        'restore' run will restore vnodeA protocol to "wintcp"
**	     -- else (protocol set to something other than "tcp_ip")
**	        'restore' run will NOT restore vnodeA protocol to "wintcp"
**	   Example 4:
**	   - user deletes vnodeB
**	   - user deletes vnodeA
**	   - user re-adds vnodeA with same private/global setting
**	     (so that vnodeA location is different...grabs vnodeB's
**	      deleted entry)
**	   - 'restore' run will NOT restore vnodeA protocol to "wintcp"
**	     (because the location in the name queue file is not the same).
**
** Input:
**	nq_filename	Name queue (NODE) filename from command line
**	                = NULL if not present
**	bk_filename	Backup/restore    filename from command line
**	                = NULL if not present
**
** Output:
**	restored 	Number of records that were (or "would have been"
**		        in case of "noupdate" option) restored.
**
** Side Effects:
**	Report (stdout) of modifications that were made, where amount
**	of detail is dependent on verbose option.
**
** Returns:
**	OK   - if successful
**	FAIL - if any errors encountered
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
**	19-Nov-2008 (lunbr01)
**	    Changed "restored" parm and "restored_config" from unsigned 
**	    to signed to eliminate "signedness differs" compiler warning.
*/
static STATUS
restore_protocol( char *nq_filename, char *bk_filename, i4 *restored )
{
    STATUS		status;
    GCN_DB_REC0		*nq_record;
    CVT_BK_RECORD	bk_record;
    FILE		*nq_file;
    FILE		*bk_file = NULL;
    char		*mode = ( update ? ERx("rw") : ERx("r") );
    char		*protocol = NULL;
    char		from_protocol[CVT_VAL_LEN] = {EOS};
    char		to_protocol[CVT_VAL_LEN] = {EOS};
    u_i4		protocol_len = 0;
    i4			nq_rec_no;
    i4			records = 0;  /* in backup file */
    i4			notfound = 0;
    i4			nq_write_err = 0;
    SYSTIME		last_bk_timestamp = {0};
    char		time_buff[ TM_SIZE_STAMP + 1 ];
    NQ_CACHE		*nq_cache_start = NULL;

    *restored = 0;

    if ( !update )
	SIprintf( "   ( Report Run Only: -noupdate specified )\n" );

    SIprintf( "** Restoring protocols in vnodes to prior values in name server %s file. **\n", nq_filename);

    status = open_file( nq_filename, mode, &nq_file, sizeof(*nq_record) );
    if ( status != OK )
    {
        SIprintf( "ERROR: Could not open name server file '%s' with mode '%s': 0x%x\n",
		  nq_filename, mode, status );
	return( FAIL );
    }
	
    if ( verbose )  SIprintf( "\nDetails (-verbose):\n");

    /* Load nq recs into memory cache */
    if ( (status = load_nq_file( nq_file, &nq_cache )) != OK )
    {
	SIclose( nq_file );
    	return( status );
    }

    if ( (!bk_filename) || !(*bk_filename) )
	get_bk_filename( nq_filename, bk_filename );

    status = open_file( bk_filename, "r", &bk_file, sizeof(bk_record) );
    if ( status != OK )
    {
        SIprintf( "ERROR: Could not open backup file '%s' for read access: 0x%x\n",
		  bk_filename, status );
	return( FAIL );
    }

    while( read_rec( bk_file, (PTR)&bk_record, sizeof(bk_record), 0 ) == OK )
    {
	records++;

	/*
	**  If there is a change in the backup timestamp values, then
	**  we are processing backup records from a subsequent conversion
	**  run (remember: backup file is appended to with each conversion
	**  run).  Just report this fact for auditing purposes.
	*/
	if ( MEcmp( &(bk_record.cvtbk_timestamp), &last_bk_timestamp, sizeof(last_bk_timestamp) ) != 0 )
	{
	    MEcopy( &(bk_record.cvtbk_timestamp), sizeof(last_bk_timestamp), &last_bk_timestamp );
	    TMstr( &last_bk_timestamp, time_buff );
	    SIprintf( "\n[Processing backup records from conversion run on %s]\n", time_buff);
	}

	if ( verbose )
	{
	    SIprintf( "\nBackup record %d: %s(uid='%s') vnode='%s', for %s rec#=%d\n",
		      records,
		      bk_record.cvtbk_uid[0] == '*' ? "Global" : "Private",
		      bk_record.cvtbk_uid,
		      bk_record.cvtbk_obj,
		      nq_filetype,
		      bk_record.cvtbk_rec_no );
	    SIprintf( " . . . ConnInfo='%s' (current)\n",
		      bk_record.cvtbk_val );
	    SIprintf( " . . . ConnInfo='%s' (prior)\n",
		      bk_record.cvtbk_val_old );
	}

	/*
	**  Find the corresponding name queue record that this backup
	**  record was created for (if it still exists and/or hasn't
	**  been modified).
	*/
	find_nq_rec( &bk_record, &nq_cache_start, &nq_record, &nq_rec_no);
	if ( !nq_rec_no )
	{
	    /*
	    **  No name queue record found!  Can't restore...report it 
	    **  and continue to next backup record.
	    */
	    notfound++;
	    if ( verbose )
		SIprintf( "   --> WARNING: %s record for backup record %d not found\n", nq_filetype, records );
	    continue;
	}

	/*
	**  Determine from/to values for config restore (if applicable) later.
	**  To handle special case where successive runs of this utility
	**  have been made with different convert actions, the "from_protocol"
	**  is only taken from the first backup record.  The "to_protocol"
	**  is taken from the last backup record.
	*/
	if ( scope & SCOPE_CONFIG )
	{
	    if ( from_protocol[0] != EOS )  /* Only update from 1st backup rec*/
	    {
		get_vnode_protocol(nq_record->gcn_val, nq_record->gcn_l_val, &protocol, &protocol_len);
		STlcopy( protocol, from_protocol, protocol_len );
	    }
	    get_vnode_protocol(bk_record.cvtbk_val_old, bk_record.cvtbk_l_val_old, &protocol, &protocol_len);
	    STlcopy( protocol, to_protocol, protocol_len );
	}

	/*
	**  Restore the old values
	*/
	nq_record->gcn_l_val = bk_record.cvtbk_l_val_old;
	MEcopy( bk_record.cvtbk_val_old, nq_record->gcn_l_val, nq_record->gcn_val );
	(*restored)++;

	if ( verbose )
	    SIprintf( "   --> restoring prior values\n");
	if ( !update )
	    continue;

	if ( (status = write_nq_rec(nq_record, nq_file, nq_rec_no)) != OK )
	{
	    nq_write_err++;
	    (*restored)--;  /* Adjust back down due to error */
	    SIprintf( "   --> ERROR: %s record %d: Error writing to file\n", nq_filetype, nq_rec_no );
	    continue;
	}

    } /* end while reading backup records */

    SIclose( nq_file );
    SIclose( bk_file );
    free_nq_cache( &nq_cache );  /* Free nq memory cache */

    SIprintf( "\nSummary of Backup/restore file %s:\n", bk_filename );
    SIprintf( "Records        : %d\n", records );
    SIprintf( " - Restored    : %d\n", *restored );
    /*
    **  If any records were NOT restored, print stats for unrestored items.
    */
    if ( (*restored) != records )
    {
	SIprintf( " - Not restored: %d  ...for the following reasons:\n",
	      records - (*restored) );
	SIprintf( "   -- Not Found in %s file: %d (eg, deleted/updated vnodes or dup backup recs)\n", nq_filetype, notfound );
	SIprintf( "   -- Write Err to %s file: %d\n", nq_filetype, nq_write_err );
    }
    SIprintf( "\nRestored %s file is %s\n", nq_filetype, nq_filename );

    /*
    **  If config.dat is to be updated, restore its values as well.
    */
    if ( scope & SCOPE_CONFIG )
	/*
	**  Verify from/to are different...normally will be, but with
	**  special cases such as multiple runs with different convert
	**  actions, they might not be...or in case where no vnode restores
	**  were found/processed.
	*/
	if ( STbcompare(from_protocol, 0, to_protocol, 0, TRUE) != 0 )
	{
	    i4	restored_config = 0;
	    CVlower( from_protocol );
	    CVlower( to_protocol );
	    status = process_config_protocol( from_protocol, to_protocol, &restored_config );
	    (*restored) += restored_config;
	    if ( status != OK )
		return( FAIL );
	}

    return( status );
}


/*
** Name: get_nq_filename
**
** Description:
**	Form a disk filename for a name queue.
**
** Input:
**	type		Name of queue.
**	filename	Disk filename...if already defined (via cmdline), exit.
**
** Output:
**	filename	Disk filename.
**
** Returns:
**	VOID
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created. (modeled after gcn_nq_file() in gcnsfile.c)
*/

static VOID
get_nq_filename( char *type, char *filename )
{
    i4		len, plen, slen;
    char	host[50];

    if ( filename && *filename )  /* Exit if already defined */
	return;

    GChostname( host, sizeof( host ) );

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
** Name: get_bk_filename
**
** Description:
**	Form a backup disk filename for a name queue.
**
** Input:
**	type		Name of queue.
**
** Output:
**	filename	Backup Disk filename.
**
** Returns:
**	VOID
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
get_bk_filename( char *type, char *filename )
{
    if ( filename && *filename )  /* Exit if already defined */
	return;

    /*
    ** If the backup file name was not specified on the command line,
    ** then default to the same name as the name server queue file
    ** name except append suffix ".BK" to the end of the filename.
    */
    if ( (!nq_filename) || !(*nq_filename) )
	get_nq_filename( nq_filetype, nq_filename );  /* Get name server file name */

    STpolycat( 2, nq_filename, ".BK", filename);
    return;
}


/*
** Name: get_lk_filename
**
** Description:
**	Form a lock/audit disk filename for a name queue.
**	This file is used to protect against multiple instances of this
**	program from running concurrently in an installation, which
**	is needed to avoid corruption of the name server or backup file.
**	The file is transient in that it is created at startup and
**	removed when the program finishes.  It also, while it exists,
**	provides some information about the run.
**
** Input:
**	type		Name of queue.
**
** Output:
**	filename	Lock/audit Disk filename.
**
** Returns:
**	VOID
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
get_lk_filename( char *type, char *filename )
{
    /*
    ** Set to the same name as the name server queue file name
    ** except append suffix ".LK" to the end of the filename.
    */
    if ( (!nq_filename) || !(*nq_filename) )
	get_nq_filename( nq_filetype, nq_filename );  /* Get name server file name */

    STpolycat( 2, nq_filename, ".LK", filename);
    return;
}


/*
** Name: open_file 	- Open a named file.
**
** Description:
**	This routine opens a named file which contains the type of database 
**	identified by its name. The file descriptor is returned.
**
** Inputs:
**	name	- The file name.
**	mode	- read/write mode.
**	reclen	- record length for those with fixed record lengths
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
**	    The file is left open
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created. (modeled after gcn_fopen() in gcnsfile.c)
**      21-Jan-2009 (lunbr01)  Sir 118847
**	    On Vista, this binary should only run in elevated mode if
**	    doing a conversion on the real Ingres name server NODE file.
**	    This change will ensure they get an error explaining the
**	    elevation requirement.
**
*/

static STATUS 
open_file( char *name, char *mode, FILE **file, i4  reclen )
{
    LOCATION	loc;
    STATUS	status;
    char	dev[MAX_LOC + 1] = {EOS};
    char	path[MAX_LOC + 1] = {EOS};
    char	null_str[MAX_LOC + 1];

    /*
    ** Convert input string for path/filename to LOCATION format.
    ** and extract to components (device, path, etc).
    ** If fails, must be invalid format on input parm.
    */
    if ( ( status = LOfroms( PATH & FILENAME, name, &loc ) ) != OK )
	goto error;
    if ( ( status = LOdetail( &loc, dev, path, null_str, null_str, null_str ) ) != OK ) 
	goto error;

    if ( ( dev[0] == EOS ) && ( path[0] == EOS) )
    {
	/*
	** Device and path were not specified in input parm, so
	** use <II_SYSTEM>\ingres\files\name as the path (aka directory).
	*/
	if ( ( status = NMloc( FILES, PATH, (char *)NULL, &loc ) ) != OK )
	    goto error;

	LOfaddpath( &loc, "name", &loc );
	LOfstfile( name, &loc );
    }

    if ( ! STcompare( mode, "r" )  &&  (status = LOexist( &loc )) != OK )
	goto error;
		
    if ( reclen )
	status = SIfopen( &loc, mode, SI_RACC, reclen, file );
    else
	status = SIopen( &loc, mode, file );

    if ( status == OK )  return( OK );

error:
    /*
    ** Check if this is Vista, and if user has sufficient privileges
    ** to execute.
    */
    ELEVATION_REQUIRED_WARNING();

    *file = NULL;

    return( status );
}


/*
** Name: read_rec
**
** Description:
**	Read a record from a Name Queue disk file.
**	If a record number is passed in, it will do a direct read
**	of that record.  For normal sequential reads, record
**	number should be passed in as zero.
**
** Input:
**	file		Open file handle.
**	record		Ptr to buffer to read record into
**	record_len	Length of record
**	rec_no		If 0, read next record.
**			Else, read requested record.
**
** Output:
**	record		Contents of next record in file.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created. (modeled after gcn_read_rec() in gcnsfile.c)
*/

static STATUS
read_rec( FILE *file, PTR record, u_i4 record_len, i4 rec_no )
{
    STATUS      status = OK;
    char        *rec_ptr = (char *)record;
    i4     	bytes_read = 0;
    i4		rec_len = record_len;
    i4		offset;

    if ( rec_no > 0 )
    {
	offset = (rec_no - 1) * record_len;
	SIfseek( file, offset, SI_P_START );
    }

    while( rec_len )
    {
	status = SIread( file, rec_len, &bytes_read, rec_ptr );
	if ( status != OK )  break;

	rec_ptr += bytes_read;
	rec_len -= bytes_read;
    }

    return( status );
}

/*
** Name: write_nq_rec
**
** Description:
**	Rewrite the name queue record (after it has been modified).
**
** Input:
**	nq_record	Ptr to name queue record
**	nq_file		Name queue file handle
**	rec_no		Record number (starting at 1) to rewrite the record to
**
** Output:
**	None
**
** Side Effects:
**	Record is rewritten to name queue file.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static STATUS
write_nq_rec( GCN_DB_REC0 *nq_record, FILE *nq_file, i4 rec_no )
{
    STATUS		status = OK;
    i4			n, offset;

    offset = (rec_no - 1) * sizeof( GCN_DB_REC0 );
    SIfseek( nq_file, offset, SI_P_START );
    status = SIwrite( sizeof(*nq_record), (PTR) nq_record, &n, nq_file ); 
    if ( status != OK )
    {
	SIprintf( "ERROR: Could not update name queue file for...\n");
	SIprintf( "  ...record %d: %s(uid='%s') vnode='%s', ConnInfo='%s'\n", 
		  rec_no,
		  nq_record->gcn_uid[0] == '*' ? "Global" : "Private", 
		  nq_record->gcn_uid, 
		  nq_record->gcn_obj, nq_record->gcn_val );
    }
    offset = (rec_no) * sizeof( GCN_DB_REC0 );
    SIfseek( nq_file, offset, SI_P_START );

    return( status );
}


/*
** Name: write_bk_rec
**
** Description:
**	Write the backup record for the "about to be" converted name
**	queue record.  NOTE that this is not a full backup of the
**	original record, but just enough info to restore what was
**	converted back to its original contents.  The backup record
**	is timestamped in case they convert the name file multiple times;
**	we need to know what order to reapply the backup if a recover
**	action is requested.
**
** Input:
**	nq_record	Ptr to name queue record
**	bk_file		Backup file handle
**
** Output:
**	None
**
** Side Effects:
**	Record is written(appended) to backup/restore file.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static STATUS
write_bk_rec( GCN_DB_REC0 *nq_record, FILE *bk_file, i4 rec_no, i4 nq_l_val_old, char *nq_val_old )
{
    STATUS		status = OK;
    i4			n;
    CVT_BK_RECORD	bk_record;

    MEfill( sizeof(bk_record), 0, &bk_record );
    bk_record.cvtbk_rec_no = rec_no;
    MEcopy( &pgm_run_timestamp, sizeof(pgm_run_timestamp), &bk_record.cvtbk_timestamp );
    bk_record.cvtbk_l_uid = nq_record->gcn_l_uid;
    MEcopy( nq_record->gcn_uid, bk_record.cvtbk_l_uid, bk_record.cvtbk_uid );
    bk_record.cvtbk_l_obj = nq_record->gcn_l_obj;
    MEcopy( nq_record->gcn_obj, bk_record.cvtbk_l_obj, bk_record.cvtbk_obj );
    bk_record.cvtbk_l_val = nq_record->gcn_l_val;
    MEcopy( nq_record->gcn_val, bk_record.cvtbk_l_val, bk_record.cvtbk_val );
    bk_record.cvtbk_l_val_old = nq_l_val_old;
    MEcopy( nq_val_old, nq_l_val_old, bk_record.cvtbk_val_old );

    status = SIwrite( sizeof(bk_record), (PTR) &bk_record, &n, bk_file ); 
    if ( status != OK )
    {
	SIprintf( "ERROR: Could not write backup record for ... \n");
	SIprintf( "  ...name queue record %d: %s(uid='%s') vnode='%s', ConnInfo='%s'\n", 
		  rec_no,
		  nq_record->gcn_uid[0] == '*' ? "Global" : "Private", 
		  nq_record->gcn_uid, 
		  nq_record->gcn_obj,
		  nq_record->gcn_val );
    }

    return( status );
}


/*
** Name: load_nq_file
**
** Description:
**	Read all records of the name server file and save them into
**	a linked list.
**
** Input:
**	nq_file		Name server queue file handle
**	nq_cache 	Ptr to linked list anchor for nq recs
**
** Output:
**	nq_cache 	Linked list of nq recs
**
** Side Effects:
**	None
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
**	 4-Sep-09 (gordy)
**	    Check for unsupported record formats.  Abort processing for now.
**	25-Sep-09 (gordy)
**	    Add processing for new record format.
*/

static STATUS
load_nq_file( FILE *nq_file, QUEUE *nq_cache )
{
    STATUS		status = OK;
    QUEUE		*nc_q_last = nq_cache;
    NQ_CACHE     	*nc_entry;
    GCN_DB_REC0		*nq_record;
    i4			nc_rec_no = 1;
    i4			nq_rec_no = 0;

    if ( verbose )    /* if verbose on, display all non-deleted nmsrvr recs */
	SIprintf( "\n[List of non-deleted name server %s records.]\n", nq_filetype);

    QUinit( nq_cache );
    /*
    ** Allocate memory for the first cache entry so the name server
    ** record can be read directly into it.
    */
    if ( (nc_entry = (NQ_CACHE *)MEreqmem( 0, sizeof(NQ_CACHE), TRUE, NULL )) == 0 )
    {
	SIprintf( "ERROR: Unable to allocate memory for name queue cache entry %d\n", nc_rec_no );
	return( FAIL );
    }

    while( read_rec( nq_file, (PTR)&(nc_entry->nc_rec), sizeof(GCN_DB_REC0), 0 ) == OK &&
	   nc_entry->nc_rec.gcn_tup_id != GCN_DBREC_EOF )
    {
	nq_rec_no++;
	nq_record = &(nc_entry->nc_rec);

	/*
	** V0 records start with a tuple identifier which
	** is always 0 (actually, may be marked as EOF, but
	** that was handled above).  V1 records start with
	** a record type which indicates EOF, V0, V1 and
	** DEL records.
	*/
	switch( nq_record->gcn_tup_id )
	{
	case GCN_DBREC_V0 :  /* Nothing to do */	break;

	case GCN_DBREC_V1 :
	    if ( convert_v1_record( (GCN_DB_RECORD *)nq_record ) != OK )  
	    	return( FAIL );
	    break;

	case GCN_DBREC_DEL :
	    if ( convert_del_record( (GCN_DB_RECORD *)nq_record ) != OK )
	    	return( FAIL );
	    break;

	default :
	    /*
	    ** This is an unknown format, abort processing.
	    */
	    SIprintf( "An unsupported record format has been found.\n" );
	    SIprintf( "Unable to perform conversion.  Processing aborted.\n" );
	    return( FAIL );
	}

	/*
	** Continue to next record if current
	** record is invalid (deleted);
	*/
	if ( nq_record->gcn_invalid == TRUE )
	    continue;

	/*
	** Skip any record which does not contain one of the basic
	** components.  This is most likely due to a problem during
	** conversion into original format.
	*/
	if ( 
	     ! nq_record->gcn_l_uid  ||  
	     ! nq_record->gcn_l_obj  ||  
	     ! nq_record->gcn_l_val 
	   )
	    continue;

	nc_rec_no++;
	nc_entry->nc_nq_rec_no = nq_rec_no;

	if ( verbose )    /* if verbose on, display the name server record */
	{
	    if ( nq_record->gcn_l_uid < CVT_UID_LEN )
		nq_record->gcn_uid[ nq_record->gcn_l_uid ] = '\0';
	    if ( nq_record->gcn_l_obj < CVT_OBJ_LEN )
		nq_record->gcn_obj[ nq_record->gcn_l_obj ] = '\0';
	    if ( nq_record->gcn_l_val < CVT_VAL_LEN )
		nq_record->gcn_val[ nq_record->gcn_l_val ] = '\0';

	    SIprintf( "\n%s record %d: %s(uid='%s') vnode='%s', . . .\n", 
		      nq_filetype, nq_rec_no,
		      nq_record->gcn_uid[0] == '*' ? "Global" : "Private", 
		      nq_record->gcn_uid, 
		      nq_record->gcn_obj );
	    SIprintf( " . . . ConnInfo='%s' (current)\n",
		      nq_record->gcn_val );
	}

	/*
	** Insert nq record into linked list.
	*/
	QUinsert( &(nc_entry->nc_q), nc_q_last );
	nc_q_last = &(nc_entry->nc_q);

	/*
	** Prep for next nq record...get memory to read into
	*/
	if ( (nc_entry = (NQ_CACHE *)MEreqmem( 0, sizeof( NQ_CACHE ), TRUE, NULL )) == 0 )
	{
	    SIprintf( "ERROR: Unable to allocate memory for name queue cache entry %d\n", nc_rec_no );
	    return( FAIL );
	}
    } /* End while reading all nq recs */

    MEfree( (PTR)nc_entry );  /* Last memalloc can be freed...not used */

    return( OK );
}


/*
** Name: convert_del_record
**
** Description:
**	Converts new format deleted record back into V0 deleted record format.
**
** Input:
**	record	New format deleted record.
**
** Output:
**	record	V0 format deleted record.
**
** Returns:
**	STATUS	OK or FAIL.
**
** History:
**	25-Sep-09 (gordy)
**	    Created.
*/

static STATUS
convert_del_record( GCN_DB_RECORD *record )
{
    if ( record->del.gcn_rec_len != sizeof( GCN_DB_RECORD ) )
    {
	SIprintf( "An unsupported record length has been found.\n" );
	SIprintf( "Unable to perform conversion.  Processing aborted.\n" );
    	return( FAIL );
    }

    record->rec0.gcn_invalid = TRUE;
    return( OK );
}


/*
** Name: convert_v1_record
**
** Description:
**	Converts a V1 format record back into V0 format.
**
** Input:
**	record	V1 format record.
**
** Output:
**	record	V0 format record.
**
** Returns:
**	STATUS	OK or FAIL.
**
** History:
**	25-Sep-09 (gordy)
**	    Created.
*/

static STATUS
convert_v1_record( GCN_DB_RECORD *record )
{
    GCN_DB_REC0	rec0;
    u_i1	*ptr = (u_i1 *)&record->rec1 + sizeof( GCN_DB_REC1 );
    u_i1	*end = (u_i1 *)&record->rec1 + record->rec1.gcn_rec_len;

    if ( record->rec1.gcn_rec_len != sizeof( GCN_DB_RECORD ) )
    {
	SIprintf( "An unsupported record length has been found.\n" );
	SIprintf( "Unable to perform conversion.  Processing aborted.\n" );
    	return( FAIL );
    }

    MEfill( sizeof( rec0 ), 0, (PTR)&rec0 );

    while( (ptr + sizeof( GCN_DB_PARAM )) <= end )
    {
    	GCN_DB_PARAM	param;

	MEcopy( (PTR)ptr, sizeof( GCN_DB_PARAM ), (PTR)&param );
	if ( ! param.gcn_p_type )  break;
	ptr += sizeof( GCN_DB_PARAM );

	if ( (ptr + param.gcn_p_len) > end )
	{
	    SIprintf("A corrupted record has been found - bad item length.\n");
	    SIprintf( "Unable to perform conversion.  Processing aborted.\n" );
	    return( FAIL );
	}

	switch( param.gcn_p_type )
	{
	case GCN_P_UID :
	    if ( param.gcn_p_len <= sizeof( rec0.gcn_uid ) )
	    {
		rec0.gcn_l_uid = param.gcn_p_len;
		MEcopy( (PTR)ptr, param.gcn_p_len, (PTR)rec0.gcn_uid );
	    }
	    else  if ( verbose )
	    {
	    	SIprintf( "Length of UID (%d) exceeds max supported (%d)\n",
			  param.gcn_p_len, sizeof( rec0.gcn_uid ) );
	    }
	    break;

	case GCN_P_OBJ :
	    if ( param.gcn_p_len <= sizeof( rec0.gcn_obj ) )
	    {
		rec0.gcn_l_obj = param.gcn_p_len;
		MEcopy( (PTR)ptr, param.gcn_p_len, (PTR)rec0.gcn_obj );
	    }
	    else  if ( verbose )
	    {
	    	SIprintf( "Length of VNODE (%d) exceeds max supported (%d)\n",
			  param.gcn_p_len, sizeof( rec0.gcn_obj ) );
	    }
	    break;

	case GCN_P_VAL :
	    if ( param.gcn_p_len <= sizeof( rec0.gcn_val ) )
	    {
		rec0.gcn_l_val = param.gcn_p_len;
		MEcopy( (PTR)ptr, param.gcn_p_len, (PTR)rec0.gcn_val );
	    }
	    else  if ( verbose )
	    {
	    	SIprintf( "Length of value (%d) exceeds max supported (%d)\n",
			  param.gcn_p_len, sizeof( rec0.gcn_val ) );
	    }
	    break;

	case GCN_P_TID :
	    /*
	    ** NODE records are not expected to have a tuple ID
	    ** (handling included for completeness).
	    */
	    if ( param.gcn_p_len != sizeof( i4 ) )
	    {
		SIprintf("A corrupted record has been found - bad data length.\n");
		SIprintf( "Unable to perform conversion.  Processing aborted.\n" );
	    	return( FAIL );
	    }

	    MEcopy( (PTR)ptr, param.gcn_p_len, (PTR)&rec0.gcn_tup_id );
	    break;

	default :
	    SIprintf( "A corrupted record has been found - bad item type.\n" );
	    SIprintf( "Unable to perform conversion.  Processing aborted.\n" );
	    return( FAIL );
	}

    	ptr += param.gcn_p_len;
    }

    MEcopy( (PTR)&rec0, sizeof( rec0 ), (PTR)&record->rec0 );
    return( OK );
}


/*
** Name: find_nq_rec
**
** Description:
**	Find a match for the backup record in the name server file
**	memory cache.
**	This routine does a sequential search through the memory
**	cache of name server records beginning immediately after
**	the entry pointed to by input parm nq_cache_start_p.
**	This value is then updated on return with the cache pointer
**	for the matching name server record or unchanged if no
**	match was found.  In general, the caller would return the
**	pointer on his next call so that we start the next search
**	at the point we left off at.  This should be optimal because
**	the backup/restore file will generally be in the same order
**	as the name server file/cache, though there will be holes
**	and additions due to post-backup activity on the name server file.
**	In spite of any subsequent activity and/or records not converted
**	and hence not backed up, the general sequence should be the same
**	and the matching name server record found quickly, if it still
**	exists.
**	The search will wrap around the memory cache back to the input
**	starting point before determining no match is found.  This will
**	allow for the case where a vnode was deleted and then re-added
**	but ended up in a totally different relative location in the
**	name server file.
**	Other approaches could have been implemented:
**	 (1) always start from the beginning of the cache
**	     - simpler, but less optimal for performance (though
**	       performance is not a big concern)
**	 (2) Use hashing algorithms in name server code
**	     - efficient, but more complex
**	     - unnecessary since access is more sequential than random
**	
**
** Input:
**	bk_record	  Backup/restore record
**	nq_cache_start_p  Ptr to ptr to cache entry at which to start search
**			  If = NULL or pts to NULL, then start at beginning
**			  of memory cache.
**	nq_record_p	  Ptr to field to store matching name server rec into
**	nq_rec_no_p	  Ptr to field to store name server file record number
**
** Output:
**	*nq_cache_start_p Ptr to cache entry at which to start search
**	*nq_record_p	  Ptr to matching name server record
**			  =NULL if no match found
**	*nq_rec_no_p	  Record # of name server record in file
**			  =0 if no match found
**
** Side Effects:
**	None
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
find_nq_rec( CVT_BK_RECORD *bk_record, NQ_CACHE **nq_cache_start_p, GCN_DB_REC0 **nq_record_p, i4 *nq_rec_no_p)
{
    QUEUE		*nc_q_next, *nc_q_end;
    NQ_CACHE     	*nc_entry;
    GCN_DB_REC0		*nq_record;

    /*
    ** Init return values to NULL/zero.
    */
    *nq_rec_no_p = 0;
    *nq_record_p = NULL;

    /*
    ** If no starting point for the search was provided in the input parm,
    ** start at base of name server records memory cache.
    ** Else, use the pointer provided.
    */
    if ( !nq_cache_start_p || !(*nq_cache_start_p) )
	nc_entry = (NQ_CACHE *)&nq_cache;
    else
	nc_entry = *nq_cache_start_p;

    /*
    ** Start and end the search at the first cache entry past the starting
    ** point.  This is because the starting point is really (in general)
    ** the last returned entry, so is not likely to be a valid match again.
    */
    nc_q_next = nc_q_end = nc_entry->nc_q.q_next;
    do
    {
	/*
	** Set up ptrs to current name server cache entry and
	** bump next pointer for do/while loop.
	*/
	nc_entry  = (NQ_CACHE *)nc_q_next;
	nc_q_next = nc_q_next->q_next;

	/*
	** Skip the anchor queue entry because it is not a real NQ_CACHE
	** entry...it's just the anchor QUEUE element.
	*/
	if ( (QUEUE *)nc_entry == &nq_cache )
	    continue;

	nq_record = &(nc_entry->nc_rec);

	/*
	**  Compare the backup record with the name server record.
	**  Is it at least a partial match??  That is, do the uid and
	**  vnode match?
	**  If not, continue to next name server record in cache.
	*/
	if ( ( bk_record->cvtbk_l_uid != nq_record->gcn_l_uid )
	||   ( STbcompare( bk_record->cvtbk_uid, bk_record->cvtbk_l_uid,
			nq_record->gcn_uid, nq_record->gcn_l_uid, TRUE ) != 0 )
	||   ( bk_record->cvtbk_l_obj != nq_record->gcn_l_obj )
	||   ( STbcompare( bk_record->cvtbk_obj, bk_record->cvtbk_l_obj,
			nq_record->gcn_obj, nq_record->gcn_l_obj, TRUE ) != 0))
	{
	    continue;
	}

	/*
	**  Is it a full match??  That is, does the value also match?
	**  Note that the value consists of "host,protocol,port".
	**  If it doesn't match, then there may be multiple connection
	**  entries for the vnode or the connection info has been manually
	**  modified after the initial conversion.  While some effort could
	**  be made to identify the latter scenario, it is safer not to
	**  to restore if anything has been changed.
	*/
	if ( ( bk_record->cvtbk_l_val != nq_record->gcn_l_val )
	||   ( STbcompare( bk_record->cvtbk_val, bk_record->cvtbk_l_val,
			nq_record->gcn_val, nq_record->gcn_l_val, TRUE ) != 0 ))
	{
	    continue;
	}

	/*
	**  It's a match!!
	**  Return the corresponding name queue file record number that 
	**  this backup record matches.
	*/
	*nq_rec_no_p = nc_entry->nc_nq_rec_no;
	break;
    }
    while( nc_q_next != nc_q_end );  /* End do/while nc_q_next... */

    if ( !(*nq_rec_no_p) )   /* No matching name queue record found	*/
    {
	*nq_record_p = NULL;
	/*...leave starting cache point "as is" */
    }
    else		/* Match was found			*/
    {
	*nq_record_p = nq_record;
	if ( nq_cache_start_p )
	    *nq_cache_start_p = nc_entry;	/* Reset next starting point */
    }
    return;
}


/*
** Name: free_nq_cache
**
** Description:
**	Free memory associated with nq memory cache.
**
** Input:
**	nq_cache 	Ptr to linked list anchor for nq recs
**
** Output:
**	nq_cache 	Cleared Linked list of nq recs
**
** Side Effects:
**	None
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static STATUS
free_nq_cache( QUEUE *nq_cache )
{
    STATUS		status = OK;
    QUEUE		*nc_q_next = nq_cache->q_next;
    NQ_CACHE     	*nc_entry;

    while( nc_q_next != nq_cache )
    {
	nc_entry = (NQ_CACHE *)nc_q_next;
	nc_q_next = nc_entry->nc_q.q_next;
	QUremove( (QUEUE *)nc_entry );
	MEfree( (PTR)nc_entry );
    } /* End while processing all nq rec entries in memory cache */

    QUinit( nq_cache );

    return( OK );
}


/*
** Name: get_vnode_protocol
**
** Description:
**	Get ptr to protocol value from NODE record value field.
**	Just in case an invalid name file is passed in, the code
**	tries to assume it may find most anything (ie, no delimiters
**	or protocol).
**
** Input:
**	buff		Ptr to NODE record value field 
**		          (format is "host,protocol,port")
**	protocol	Ptr to store ptr to input buff's protocol value.
**	protocol_len	Ptr to field to hold length of protocol value found.
**
** Output:
**	protocol	Ptr to network protocol value in input buff.
**	protocol_len	Length of protocol value found.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
get_vnode_protocol( char *buff, u_i4 buff_len, char **protocol, u_i4 *protocol_len )
{
    char	*protocol_end;
    char	*delim = ERx(",");     /* comma delimiter */
    *protocol = NULL;
    *protocol_len = 0;

    if ( ! buff  ||  ! *buff  ||  !buff_len )  
	return;

    if ( (*protocol = STindex( buff, delim, buff_len)) != NULL )
    {
	(*protocol)++;
	if ( (protocol_end = STindex(*protocol, delim, 0))  != NULL )
	    *protocol_len = protocol_end - *protocol;
	else
	    *protocol_len = (buff + buff_len + 1) - *protocol;
    }
    return;
}



/*
** Name: process_config_protocol
**
** Description:
**	Verify that the "to_protocol" is supported in the config.dat.
**	Also, if scope command line argument indicates config.dat
**	is to be updated, make sure the "to_protocol" is the default
**	protocol in the config.dat.
**	NOTE that this routine is also called for -action=restore and
**	does a restore of config.dat values by the same code that
**	converts (just receives reversed from_protocol and to_protocol
**	values).  Since there are exceptions (such as for GCD) on how
**	config.dat values are modified, the restore is not a perfect
**	restore in that values "may" not be exactly the same as they
**	were originally.  This is because we do not save the original
**	config.dat values in a backup file as we do for vnode records.
**	However, the code handles the known "normal" cases so only
**	when the config.dat has been modified from its default values
**	will there be the possibility exist that a value may not get
**	restored; even in that situation, the impact is likely to be
**	nil and the user can easily manually update the config as desired.
**
** Input:
**	from_protocol	Protocol being converted "from"...normally set to
**		        "wintcp" but could be "tcp_ip"
**	to_protocol	Protocol being converted "to"  ...normally set to
**		        "tcp_ip" but could be "wintcp"
** Output:
**	none
**
** Side Effect:
**	config.dat network protocols may get updated to make "to_protocol"
**	the default.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
**	    Changed "converted" parm from unsigned to signed
**	    to eliminate "signedness differs" compiler warning.
**      21-Jan-2009 (lunbr01)  Sir 118847
**	    On Vista, this binary should only run in elevated mode if
**	    doing a conversion on the real Ingres name server config.dat.
**	    This change will ensure they get an error explaining the
**	    elevation requirement. Also, don't do an update if no entries
**	    were found to convert.
*/

static STATUS
process_config_protocol( char *from_protocol, char *to_protocol, i4 *converted )
{
    STATUS	status = OK;
    char	*instance_name = ERx("*");
    PM_CONTEXT	*config;	/* configuration data PM context */
    char	pmsym[128];
    char	from_pmsym[128];
    char	*pm_val;
    char	*from_pm_val;
    char	gcx_server[][4] = { ERx("gcb"), ERx("gcc"), ERx("gcd"), 0 };
    char	default_port[4];
    char	default_port_gcd[4];
    i4  	i;

    STpolycat( 2, installation_id, "7", default_port_gcd );
    (void) PMmInit( &config );
    PMmLowerOn( config );
    switch( PMmLoad( config, NULL, NULL ) )
    {
	case OK : break;

	case PM_FILE_BAD :
	    SIprintf( "ERROR: Bad config.dat file (status=PM_FILE_BAD)\n" );
	    return( FAIL );
	
	default :
	    SIprintf( "ERROR: Unable to load config.dat entries\n" );
	    return( FAIL );
    }

    PMmSetDefault( config, 0, SystemCfgPrefix );
    PMmSetDefault( config, 1, PMhost() );

    /*
    ** Verify that GCC has a config.dat entry for the target protocol.
    ** If not, don't do the conversion since the protocol may not be
    ** supported in this version of Ingres.  Exit with FAILure.
    */
    PMmSetDefault( config, 2, ERx("gcc") );
    PMmSetDefault( config, 3, instance_name );
    STprintf( pmsym, "!.%s.status", to_protocol );
    if ( PMmGet( config, pmsym, &pm_val ) != OK )
    {
	SIprintf( "ERROR: '%s' protocol not defined in config.dat for 'gcc' server!\n\tConversion aborted since protocol may not be supported.\n",
		to_protocol );
	return( FAIL );
    }

    /*
    ** If scope not set to update the config.dat, then we're done here.
    */
    if ( !(scope & SCOPE_CONFIG) )  return( OK );

    /*
    ** Update protocol status and port as follows:
    ** 1. Set to_protocol to the default (status=ON and 
    **    port=installationID).
    ** 2. If 'wintcp' is the from_protocol, turn it off (status=OFF).
    ** Don't try to update customized config.dat entries.
    */
    for ( i = 0; gcx_server[i][0] != 0; i++ )
    {
	PMmSetDefault( config, 2, gcx_server[i] );
	if ( verbose )
	    SIprintf( "\nconfig: !=%s.%s.%s.%s\n",
		      PMmGetDefault(config, 0),
		      PMmGetDefault(config, 1),
		      PMmGetDefault(config, 2),
		      PMmGetDefault(config, 3) );

	if ( STbcompare( gcx_server[i], 0, "gcd", 0, FALSE ) == 0 )
	    STcopy( default_port_gcd, default_port );
	else
	    STcopy( installation_id, default_port );

        /*
        ** Check "to" protocol status.  Skip if not defined.
	** An alternative would be to insert .status=ON, but reluctant
	** to do so since it should have already been defined unless user
	** has specifically removed it or the release of Ingres doesn't
	** support it.
        */
        STprintf( pmsym, "!.%s.status", to_protocol );
        if ( PMmGet( config, pmsym, &pm_val ) != OK )
	    continue;

	if ( verbose ) SIprintf( "config:   %s=%s", pmsym, pm_val );

	if ( STcasecmp( pm_val, "OFF" ) == 0 )
	{
	    /*
	    ** status is OFF...modify to ON
	    ** ...unless: server=GCD, to_protocol='wintcp', action=restore;
	    ** this special case is skipped because default config (before
	    ** conversion) has gcd.*.status=OFF for wintcp...we don't want
	    ** to set it to ON here, which is not the original value.
	    */
	    if ( ( action == ACTION_RESTORE ) &&
		 ( STbcompare( to_protocol, 0, PROTOCOL_WINTCP, 0, TRUE ) == 0 ) &&
		 ( STbcompare( gcx_server[i], 0, "gcd", 0, FALSE ) == 0 ) )
	    {
		if ( verbose ) SIprintf( "\n" );
	    }
	    else
	    {
		PMmDelete( config, pmsym );
		PMmInsert( config, pmsym, "ON" );
		(*converted)++;
		if ( verbose ) SIprintf( " <-- changed to ON\n" );
	    }
	}
	else if ( verbose ) SIprintf( "\n" );

	if ( STbcompare( from_protocol, 0, PROTOCOL_WINTCP, 0, TRUE ) == 0 )
	{
            STprintf( pmsym, "!.%s.status", from_protocol );
            if ( PMmGet( config, pmsym, &pm_val ) == OK )
	    {
		if ( verbose ) SIprintf( "config:   %s=%s", pmsym, pm_val );
	        if ( STcasecmp( pm_val, "ON" ) == OK )
		{
		    /*
		    ** "From" protocol is "wintcp" so change status to OFF.
		    */
		    PMmDelete( config, pmsym );
		    PMmInsert( config, pmsym, "OFF" );
		    (*converted)++;
		    if ( verbose ) SIprintf( " <-- changed to OFF\n" );
		}
		else if ( verbose ) SIprintf( "\n" );
	    }
	}

	/*
	** Swap from and to protocol port values unless
	** to_protocol already set to default: installation id
	** for GCC, installation id followed by a 7 for GCD.
	*/
        STprintf( pmsym, "!.%s.port", to_protocol );
        if ( PMmGet( config, pmsym, &pm_val ) != OK )
	{
	    /*
	    ** Port for to_protocol not defined...add it (use default port).
	    */
	    PMmInsert( config, pmsym, default_port );
	    (*converted)++;
	    if ( verbose ) SIprintf( "config: %s=%s <-- added\n", pmsym, default_port );
	    continue;
	}
	if ( verbose ) SIprintf( "config:   %s=%s\n", pmsym, pm_val );

        STprintf( from_pmsym, "!.%s.port", from_protocol );
	/*
	** If from_protocol port not defined (unusual but OK),
	** then leave to_protocol port "as-is".
	*/
        if ( PMmGet( config, from_pmsym, &from_pm_val ) != OK )
	    continue;
	if ( verbose ) SIprintf( "config:   %s=%s", from_pmsym, from_pm_val );

	/*
	** If the port values are the same for "from" and "to"
	** or if the "to" protocol is already set to the default port,
	** then leave to_protocol port "as-is".
	*/
	if ( ( STbcompare( pm_val, 0, from_pm_val, 0, FALSE ) == 0 ) ||
	     ( STcasecmp( pm_val, default_port ) == 0 ) )
	{
	    if ( verbose ) SIprintf( "\n" );
	    continue;  /* already same...leave "as-is". */
	}

	/*
	** Swap "from" and "to" protocol port values.
	*/
	PMmDelete( config, pmsym );
	PMmInsert( config, pmsym, from_pm_val );
	PMmDelete( config, from_pmsym );
	PMmInsert( config, from_pmsym, pm_val );
	(*converted)+=2;
	if ( verbose )
	    SIprintf( " <-- swapped port values...%s=%s and %s=%s\n",
		      from_protocol, pm_val,
		      to_protocol, from_pm_val );

    } /* end foreach GCx server */

    if ( update && ((*converted) > 0) )
    {
	status = PMmWrite( config, NULL );  /* Write updates to config.dat */
	if ( status != OK)
	{
	    ELEVATION_REQUIRED_WARNING();
	    SIprintf( "ERROR: Unable to update config.dat entries\n" );
	    return( FAIL );
	}
    }

    SIprintf( "\nSummary of config.dat file updates:\n" );
    SIprintf( " - Modified  : %d %s\n", *converted, update ? "" : "(not done because '-noupdate' specified)" );

    if ( verbose ) SIprintf( "\n** End of config updates. **\n" );
    SIprintf( "\n" );
    return( OK );
}

/*
** Name: get_cmdline_args
**
** Description:
**	Get and verify input command line parameters.  Store
**	the values in appropriate static variables.
**
** Input:
**	argc		# of command line arguments.
**	argv		list of ptrs to command line arguments.
**
** Output:
**	Displays error message and usage if error encountered.
**	Else, move parameter values to static variables.
**
** Returns:
**	OK   - if all input parameters are valid
**	FAIL - if any input parameters are invalid
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static STATUS
get_cmdline_args( i4  argc, char **argv )
{
    i4  argnum;
    char *argname;
    char *argvalue;

    if ( argc < 2 )    /* if no parms, show usage */
    {
	usage(FALSE);
	return(FAIL);
    }

    for (argnum = 1; argnum < argc; argnum++)
    {
	argname = argv[argnum];
	argvalue = "";
	if ( STbcompare(argname, 0, "-help", 0, TRUE) == 0 )
	{
	    usage(TRUE);
	    return(FAIL);
	}
	else if ( STbcompare(argname, 0, "-verbose", 0, TRUE) == 0 )
	    verbose = TRUE;
	else if ( STbcompare(argname, 0, "-noupdate", 0, TRUE) == 0 )
	    update = FALSE;
	else if ( STbcompare(argname, 0, "-force", 0, TRUE) == 0 )
	    force = TRUE;
	else if ( STbcompare(argname, 0, "-action", 0, TRUE) == 0 )
	{
	    /* Is there a value specified for -action parm? */
	    if ( ((argnum + 1) < argc) && (argv[argnum + 1][0] != '-') )
	    {
		argnum++;
		argvalue = argv[argnum];
		if ( STbcompare(argvalue, 0, "convert_wintcp", 0, TRUE) == 0 )
		    action = ACTION_CONVERT_WINTCP;
		else 
		if ( STbcompare(argvalue, 0, "convert_tcp_ip", 0, TRUE) == 0 )
		    action = ACTION_CONVERT_TCP_IP;
		else 
		if ( STbcompare(argvalue, 0, "restore", 0, TRUE) == 0 )
		    action = ACTION_RESTORE;
		else
		{
		    SIprintf( "ERROR: Invalid -action parm value='%s' -> must be 'convert_wintcp', 'convert_tcp_ip' or 'restore'\n", argvalue);
		    usage(FALSE);
		    return(FAIL);
		}
	    } /* end if value for -action parm */
	    else  /* No value for -action parm */
	    {
		SIprintf( "ERROR: Missing -action parm value -> must be 'convert_wintcp', 'convert_tcp_ip' or 'restore'\n");
		usage(FALSE);
		return(FAIL);
	    }
	}
	else if ( STbcompare(argname, 0, "-bf", 0, TRUE) == 0 )
	{
	    /* Is there a value specified for -bf parm? */
	    if ( ((argnum + 1) < argc) && (argv[argnum + 1][0] != '-') )
	    {
		argnum++;
		STlcopy( argv[argnum], bk_filename, sizeof(bk_filename) );
	    }
	    else  /* No value for -bf parm */
	    {
		SIprintf( "ERROR: Missing -bf parm value -> must be backup file name\n");
		usage(FALSE);
		return(FAIL);
	    }
	}
	else if ( STbcompare(argname, 0, "-nf", 0, TRUE) == 0 )
	{
	    /* Is there a value specified for -nf parm? */
	    if ( ((argnum + 1) < argc) && (argv[argnum + 1][0] != '-') )
	    {
		argnum++;
		STlcopy( argv[argnum], nq_filename, sizeof(nq_filename) );
	    }
	    else  /* No value for -nf parm */
	    {
		SIprintf( "ERROR: Missing -nf parm value -> must be name server file name\n");
		usage(FALSE);
		return(FAIL);
	    }
	}
	else if ( STbcompare(argname, 0, "-scope", 0, TRUE) == 0 )
	{
	    /* Is there a value specified for -scope parm? */
	    if ( ((argnum + 1) < argc) && (argv[argnum + 1][0] != '-') )
	    {
		argnum++;
		argvalue = argv[argnum];
		if ( STbcompare(argvalue, 0, "all", 0, TRUE) == 0 )
		    scope = SCOPE_VNODES | SCOPE_CONFIG;
		else 
		if ( STbcompare(argvalue, 0, "vnodes", 0, TRUE) == 0 )
		    scope = SCOPE_VNODES;
		else 
		if ( STbcompare(argvalue, 0, "config", 0, TRUE) == 0 )
		    scope = SCOPE_CONFIG;
		else
		{
		    SIprintf( "ERROR: Invalid -scope parm value='%s' -> must be 'all', 'vnodes' or 'config'\n", argvalue);
		    usage(FALSE);
		    return(FAIL);
		}
	    } /* end if value for -scope parm */
	    else  /* No value for -scope parm */
	    {
		SIprintf( "ERROR: Missing -scope parm value -> must be 'all', 'vnodes' or 'config'\n");
		usage(FALSE);
		return(FAIL);
	    }
	}
	else
	{
	    SIprintf( "ERROR: Invalid parm '%s'\n", argname);
	    usage(FALSE);
	    return(FAIL);
	}
    }

    return(OK);
}


/*
** Name: usage
**
** Description:
**	Display command line usage info.
**
** Input:
**	detail		Flag indicating if detail description wanted
**			=TRUE  for max details
**			=FALSE for format only
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
usage(bool detail)
{
    SIprintf( "usage: %s [-help] [-verbose] [-noupdate] [-force]\n", 
	      pgmname );
    SIprintf( "	           [-action convert_wintcp | convert_tcp_ip | restore]\n");
    SIprintf( "	           [-bf filename] [-nf filename]\n");
    SIprintf( "	           [-scope all | vnodes | config]\n");
    if (!detail) return;

    {
	int	line;
	char	usage_detail[][80] =
{
"	-help	  Display command usage with details.",
"	-verbose  Verbose mode (displays each individual record",
"	          affected; otherwise only totals are displayed).",
"	-noupdate Only report what the utility would have done,",
"	          but don't update name server or backup files.",
"	          Useful for determining impact prior to conversion.",
"	          If name server is running, must also use -force",
"	          parameter.  Also, with -verbose parameter, can",
"	          be used to produce a report from which the vnode",
"                  updates could be done manually from one of the",
"	          Ingres network config utilities.",
"	-force	  Run even if the name server is up or another",
"	          instance of this program is concurrently running.",
"	          A warning message will be in either case.",
"	          This parameter is useful with '-noupdate' parameter",
"	          or running against a copy of the name server",
"	          node file.  USE WITH CAUTION.",
"	-action	  Action to be performed:",
"	          *NOTE*: If not specified, display usage.",
"	          Prevents unintentional conversion if someone",
"	          just types in command name with no parameters.",
"	            convert_wintcp: Convert wintcp to tcp_ip",
"	            convert_tcp_ip: Convert tcp_ip to wintcp",
"	            restore       : Restore converted protocol entries",
"		                    back to prior values",
"	-bf       Backoff/restore filename; default is",
"	          <II_SYSTEM>\\ingres\\files\\name\\IINODE_hostname.BK",
"	          May be specified with or without path name; if no",
"	          path, defaults path to <II_SYSTEM>\\ingres\\files\\name.",
"	          NOTE:",
"	          Written to as output if -action is convert_wintcp",
"	                                          or convert_tcp_ip.",
"	          Read from  as input  if -action is restore and",
"	          is required for a successful restore operation.",
"	-nf       Name server filename; default is",
"	          <II_SYSTEM>\\ingres\\files\\name\\IINODE_hostname",
"	          May be specified with or without path name; if no",
"	          path, defaults path to <II_SYSTEM>\\ingres\\files\\name.",
"	-scope    Convert/restore following entities:",
"	            all: vnode + config (default)",
"	            vnodes: vnodes only",
"	            config: config.dat file only",
" ",
"  Returns:",
"	>=0 if OK where return code is # of records that were",
"	    or would be (for -noupdate) modified.",
"        -1  if FAIL",
" ",
" ",
"  Examples:",
" 	(All should be run while Ingres name server (iigcn) is down.)",
" ",
" 	1. Convert 'wintcp' to 'tcp_ip' in current Ingres installation.",
" 	     iicvtwintcp -action convert_wintcp",
" ",
" 	2. Restore converted records back to 'wintcp' after running example 1.",
" 	     iicvtwintcp -action restore",
" ",
" 	3. Convert 'tcp_ip' to 'wintcp' in current Ingres installation.",
" 	   Opposite of example 1, but similar to example 2 except that",
" 	   ALL 'tcp_ip' records will be set to 'wintcp', not just the",
" 	   ones that were originally converted by '-action convert_wintcp'.",
" 	   Likely usage would be if restore is desired but backup/restore",
" 	   file was lost.",
" 	     iicvtwintcp -action convert_tcp_ip",
" ",
" 	4. List connection (NODE) information for all vnodes.",
" 	     iicvtwintcp -action convert_wintcp -noupdate -verbose",
{ 0 }   /* The end */
};
	for( line = 0; usage_detail[line][0] != 0; line++ )
	    SIprintf( "%s\n", usage_detail[line] );
    }
    return;
}


/*
** Name: is_gcn_up
**
** Description:
**	Check to see if the Ingres Name Server (GCN) is up (running).
**	Any errors encountered trying to determine if it is up
**	will be treated AS IF IT IS!! (since we can't tell) 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	TRUE  if GCN up (or can't tell due to error)
**	FALSE if GCN down
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
**	19-Nov-2008 (lunbr01)
**	    Changed function return type and gcn_up_flag from BOOL to bool
**	    to fix compile error on Linux/Unix.
*/

static bool
is_gcn_up()
{
    bool		gcn_up_flag=TRUE;
    IIAPI_INITPARM	initParm;
    IIAPI_CONNPARM      connParm;
    IIAPI_DISCONNPARM   disconnParm;
    IIAPI_WAITPARM      waitParm = { -1 };

    if ( !iiapi_initialized )
    {
	/*
	**  Initialize OpenAPI
	*/
	initParm.in_version = IIAPI_VERSION_2;
	initParm.in_timeout = -1;
	IIapi_initialize( &initParm );
	if ( initParm.in_status != IIAPI_ST_SUCCESS )
	{
	    SIprintf( "ERROR: IIapi_initialize()\tin_status = %s\n",
               (initParm.in_status == IIAPI_ST_FAILURE) ? 
	    		             "IIAPI_ST_FAILURE" :
               (initParm.in_status == IIAPI_ST_OUT_OF_MEMORY) ?
			             "IIAPI_ST_OUT_OF_MEMORY"   :
                                     "(unknown status)" );
	    return( TRUE );
	}
	envHandle = initParm.in_envHandle;
	iiapi_initialized = TRUE;
    }

    /*
    **  Connect to Ingres Name Server (GCN)
    */
    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target =  NULL;       /* for Name Server (/IINMSVR) */
    connParm.co_connHandle = envHandle;
    connParm.co_tranHandle = NULL;
    connParm.co_type = IIAPI_CT_NS;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = -1;

    IIapi_connect( &connParm );
    while( connParm.co_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    if ( connParm.co_genParm.gp_status == IIAPI_ST_SUCCESS )
    {
	gcn_up_flag = TRUE;
	SIprintf("WARNING: Ingres Name Server(iigcn) is running...should be down!\n");
    }
    else
    {
	if ( connParm.co_genParm.gp_status == IIAPI_ST_FAILURE )
	    gcn_up_flag = FALSE;
	else
	{
	    gcn_up_flag = TRUE;
	    iiapi_checkError("IIapi_connect", &connParm.co_genParm);
	    SIprintf("WARNING: Unable to determine if Ingres Name Server(iigcn) is running due to errors!\n");
	}
    }

    if ( !connParm.co_connHandle )
	return( gcn_up_flag );

    /*
    **  Disconnect
    */
    disconnParm.dc_genParm.gp_callback = NULL;
    disconnParm.dc_genParm.gp_closure = NULL;
    disconnParm.dc_connHandle = connParm.co_connHandle;

    IIapi_disconnect( &disconnParm );
    while( disconnParm.dc_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );
    return( gcn_up_flag );
}


/*
** Name: is_other_cvtwintcp_up
**
** Description:
**	Check to see if another instance of this program is up (running)
**	and is processing the same name server file.
**	The basic idea here is to see if a lock file already exists
**	and, if so, is the process id (PID) in the record still running?
**	If yes, then exit with error that other instance is running.
**	Else, write our information (PID primarily) out to the file.
**	A cross-process named mutex that we could check without going
**	into a wait would be easier and more foolproof but didn't exist
**	in the Ingres CL; CS semaphores almost the ticket, but they
**	always wait until the mutex is released and bring a lot of
**	Ingres DBMS server stuff along.
**	There is a small window while we're processing the lock file
**	that another instance "could" get in, but the risk is minimal.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	TRUE  if found another instance of ourselves or undetermined (ERROR)
**	FALSE if not and updates lock file
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
**	19-Nov-2008 (lunbr01)
**	    Changed function return type from BOOL to bool to fix 
**	    compile error on Linux/Unix.
*/

static bool
is_other_cvtwintcp_up()
{
    STATUS	status;
    FILE	*lk_file;
    i4		lk_records = 0;  	/* #recs in lock/audit file */
    i4		bytes_read = 0;  	/* #bytes read from lock/audit file */
    i4		n;
    char	DELIM=';';
    i4		lk_hdr_len;
    char	lk_hdr[80];
    char	lk_record[1024];
    char	*lk_record_end;		/* pointer to end of lk_record read */
    char	*lk_audit_info;		/* misc audit info */
    char	time_buff[ TM_SIZE_STAMP + 1 ];
    PID		pid;
    char	*pid_a_p;		/* pointer to pid in ascii string */
    char	*pid_delim = ERx(":");	/* pid delimiter */

    /*
    ** Set up lock header for use below.
    */
    STprintf( lk_hdr, "lock %s file for program/pid=%s/", nq_filetype, pgmname);
    lk_hdr_len = STlength( lk_hdr );

    status = open_file( lk_filename, "r", &lk_file, 0 );
    if ( status == OK )
    {
	/*
	**  The file exists so another instance may be running.
	**  Check the PID to see if it is still running.
	**  If yes, then exit with error message.
	**  If not running, then continue and put our own PID in.
	*/
	status = SIread( lk_file, sizeof(lk_record), &bytes_read, lk_record );
	SIclose( lk_file ); /* Only first record is used. */
	if ( (status != OK) && (status != ENDFILE) )	/* Read error */
	{
	    SIprintf( "ERROR: Read error in lock file='%s'!  Status=%d\n",
		   lk_filename, status );
	    return( TRUE );   /* Error treated as undetermined state */
	}

        /*
	** Lock file exists and contains a record.
	*/
	lk_record_end = lk_record + bytes_read;
	lk_record[bytes_read] = EOS;
        /*
	** First see if it has been "unlocked".
	*/
	if ( ( bytes_read >= (sizeof(UNLOCKED_C) - 1) ) &&
	     ( MEcmp( lk_record, UNLOCKED_C, (sizeof(UNLOCKED_C) - 1) ) == 0 ) )
	{
	    goto fmt_and_lock;
	}
        /*
	** May be another instance of this program running!
	**
	** Is it a valid lock record format?  Check first few fields
	** in record, which should look like:
	**       "lock <nq_filetype> file for program/pid=<pgmname>/<pid>:"
	** E.g., "lock NODE file for program/pid=iicvtwintcp/12345:"
	*/
	if ( ( bytes_read < lk_hdr_len ) ||
	     ( MEcmp( lk_record, lk_hdr, lk_hdr_len ) != 0 ) )
	{
	    SIprintf( "ERROR: Invalid record in lock file='%s'!  Should start with:\n%s\n  ...but instead found:\n%s\n",
			lk_filename, lk_hdr, lk_record );
	    return( TRUE );   /* Error treated as undetermined state */
	}

	/*
	** Find PID value (follows header) and then determine if 
	** the process is still running.  If so, exit with error...
	** other instance of self running.  Display entire lock record 
	** contents and lock file name.
	*/
	pid_a_p = lk_record + lk_hdr_len;
	if ( ( (lk_audit_info = STindex( pid_a_p, pid_delim, (lk_record_end - pid_a_p) )) == NULL ) ||
	     ( (lk_audit_info - pid_a_p) == 0 ) )
	{
	    SIprintf( "ERROR: Invalid record in lock file='%s'!  PID missing after program name:\n%s",
			lk_filename, lk_record );
	    return( TRUE );   /* Error treated as undetermined state */
	}
	*lk_audit_info = EOS;
	if ( CVal( pid_a_p, &pid ) != OK )
	{
	    SIprintf( "ERROR: Invalid record in lock file='%s'!  PID='%s' not numeric, record=...\n",
			lk_filename, pid_a_p );
	    *lk_audit_info = pid_delim[0]; /* Restore delim for lk_record display*/
	    SIprintf( "%s", lk_record );
	    return( TRUE );   /* Error treated as undetermined state */
	}
	*lk_audit_info = pid_delim[0];  /* Restore delim for lk_record displays */
	if ( PCis_alive( pid ) )
	{
	    SIprintf( "ERROR: Other instance of this program currently running!\n -> See pid=%d and lock file='%s', record=...\n%s\n",
			pid, lk_filename, lk_record );
	    return( TRUE );   /* Error treated as undetermined state */
	}

	/*
	** Although a lock file record existed, the process is no longer
	** running so OK to proceed (fall through to format/write).
	*/

    } /* End if lock file exists */
    
fmt_and_lock:
    /*
    ** Format and write out lock file record.
    */
    PCpid( &pid );			/* Get current process id */
    TMstr( &pgm_run_timestamp, time_buff );   /* Format timestamp */
    STprintf( lk_record, "%s%d: %s, name server filename='%s'",
	      lk_hdr, pid, time_buff,
	      nq_filename );

    if ( (status = open_file( lk_filename, "w", &lk_file, 0 )) != OK )
    {
	SIprintf( "ERROR: Unable to open lock file='%s' for write!  Status=%d\n%s\n",
		   lk_filename, status );
	return( TRUE );   /* Error */
    }

    if ( (status = SIwrite( STlength(lk_record), lk_record, &n, lk_file )) != OK ) 
    {
	SIprintf( "ERROR: Unable to write record to lock file='%s'!  Status=%d\n%s\n",
		   lk_filename, status );
	return( TRUE );   /* Error */
    }

    SIclose( lk_file ); /* We now own the lock for this program. */
    return( FALSE );	/* Indicates no other instance currently running. */
}


/*
** Name: lk_unlock
**
** Description:
**	Update the lock file as "unlocked" (called at termination).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	OK	if update successful
**	status	if error
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static STATUS
lk_unlock()
{
    STATUS	status;
    FILE	*lk_file;
    char	lk_record[]=UNLOCKED_C;
    i4		n;

    if ( (status = open_file( lk_filename, "w", &lk_file, 0 )) != OK )
    {
	SIprintf( "ERROR: Unable to open lock file='%s' for write to unlock!  Status=%d\n%s\n",
		   lk_filename, status );
	return( status );   /* Error */
    }

    if ( (status = SIwrite( STlength(lk_record), lk_record, &n, lk_file )) != OK ) 
    {
	SIprintf( "ERROR: Unable to write unlock record to lock file='%s'!  Status=%d\n%s\n",
		   lk_filename, status );
	return( status );   /* Error */
    }

    SIclose( lk_file );
    return( OK );	/* Successfully unlocked */
}


/*
** Name: iiapi_checkError
**
** Description:
**	Check the status of an API function call 
**	and process error information.
**
** Input:
**	api_fn_name	API function name.
**	genParm		API generic parameters.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
iiapi_checkError( char *api_fn_name, IIAPI_GENPARM *genParm )
{
    IIAPI_GETEINFOPARM	getErrParm; 
    char		type[33];

    /*
    ** Check API call status.
    */
    SIprintf( "%s()\tgp_status = %s\n", api_fn_name,
               (genParm->gp_status == IIAPI_ST_SUCCESS) ?  
			"IIAPI_ST_SUCCESS" :
               (genParm->gp_status == IIAPI_ST_MESSAGE) ?  
			"IIAPI_ST_MESSAGE" :
               (genParm->gp_status == IIAPI_ST_WARNING) ?  
			"IIAPI_ST_WARNING" :
               (genParm->gp_status == IIAPI_ST_NO_DATA) ?  
			"IIAPI_ST_NO_DATA" :
               (genParm->gp_status == IIAPI_ST_ERROR)   ?  
			"IIAPI_ST_ERROR"   :
               (genParm->gp_status == IIAPI_ST_FAILURE) ? 
			"IIAPI_ST_FAILURE" :
               (genParm->gp_status == IIAPI_ST_NOT_INITIALIZED) ?
			"IIAPI_ST_NOT_INITIALIZED" :
               (genParm->gp_status == IIAPI_ST_INVALID_HANDLE) ?
			"IIAPI_ST_INVALID_HANDLE"  :
               (genParm->gp_status == IIAPI_ST_OUT_OF_MEMORY) ?
			"IIAPI_ST_OUT_OF_MEMORY"   :
                        "(unknown status)" );

    /*
    ** Check for error information.
    */
    if ( ! genParm->gp_errorHandle )  return;
    getErrParm.ge_errorHandle = genParm->gp_errorHandle;

    do
    { 
	/*
	** Invoke API function call.
 	*/
    	IIapi_getErrorInfo( &getErrParm );

 	/*
	** Break out of the loop if no data or failed.
	*/
    	if ( getErrParm.ge_status != IIAPI_ST_SUCCESS )
	    break;

	/*
	** Process result.
	*/
	switch( getErrParm.ge_type )
	{
	    case IIAPI_GE_ERROR	 : 
		STcopy( "ERROR", type ); 	break;

	    case IIAPI_GE_WARNING :
		STcopy( "WARNING", type ); 	break;

	    case IIAPI_GE_MESSAGE :
		STcopy("USER MESSAGE", type);	break;

	    default:
		STprintf( type, "unknown error type: %d", getErrParm.ge_type);
		break;
	}

	SIprintf( "\tError Info: %s '%s' 0x%x: %s\n",
		   type, getErrParm.ge_SQLSTATE, getErrParm.ge_errorCode,
		   getErrParm.ge_message ? getErrParm.ge_message : "NULL" );

    } while( 1 );

    return;
}


/*
** Name: iiapi_term
**
** Description:
**	Terminate OpenAPI
**
** Input:
**	None (though API environment handle taken from static/global).
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
iiapi_term()
{
    IIAPI_RELENVPARM    relEnvParm;
    IIAPI_TERMPARM      termParm;
    /*
    **  Terminate OpenAPI
    */
    if ( envHandle )
    {
	relEnvParm.re_envHandle = envHandle;
	IIapi_releaseEnv(&relEnvParm);
    }

    IIapi_terminate( &termParm );

    return;
}
/*
** Name: initialize_processing
**
** Description:
**	Miscellaneous initialization functions
**
** Input:
**	None
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
initialize_processing()
{
    char	*env;
    char	time_buff[ TM_SIZE_STAMP + 1 ];

    MEadvise(ME_INGRES_ALLOC);
    TMnow( &pgm_run_timestamp );
    TMstr( &pgm_run_timestamp, time_buff );
    SIeqinit();

    NMgtAt( "II_INSTALLATION", &env );
    STncpy( installation_id, env ? env : "II", sizeof( installation_id ) - 1 );
    installation_id[ sizeof( installation_id ) - 1 ] = EOS;

    SIprintf( "\nProgram '%s' for installation='%s' started on %s\n\n",
	      pgmname, installation_id, time_buff );
    return;
}


/*
** Name: terminate_processing
**
** Description:
**	Terminate/cleanup 
**
** Input:
**	None
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	1-Oct-2008 (lunbr01)
**	    Created.
*/

static VOID
terminate_processing()
{
    lk_unlock();	/* Mark lock file as "unlocked" */
    iiapi_term();	/* Terminate OpenAPI */
    return;
}
