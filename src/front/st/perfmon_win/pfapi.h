/*
** Copyright (c) 1998, 1999 Ingres Corporation  
*/
# ifndef PFAPI_H
# define PFAPI_H 

/*{
** Name: pfapi.h - Communications API include for the performance DLL
**
** Description:
**	Include file for functions to interface with the Ingres API.
** 
** 	This interface provides a synchronous interface to an Ingres DBMS
** 	Server via the Synchronous API Wrapper (SAW).
**
** History:
** 	15-oct-1998 (wonst02)
** 	    Created.
** 	19-dec-1998 (wonst02)
** 	    Added pfApiSelectSampler() declaration.
** 	07-jan-1999 (wonst02)
** 	    Rename pfApiSelectSampler to pfApiSelectThreads; add pfApiSelectEvents.
** 	16-apr-1999 (wonst02)
** 	    Add connection block parm to pfApiInit & pfApiTerm.
**	22-oct-1999 (somsa01
**	    Added pfApiExecSelect() for executing a given singleton select.
**	    Also added another parameter to pfApiCOnnect() and
**	    pfApiDisconnect().
*/

# include <iiapidep.h>

/*
** 	A row of the ima_dmf_cache_stats table
*/
typedef struct _IMA_DMF_CACHE_STATS
{	
	i4	page_size;
	i4	force_count;
	i4	io_wait_count;           
	i4	group_buffer_read_count;
	i4	group_buffer_write_count;
	i4	fix_count;
	i4	unfix_count;
	i4	read_count;
	i4	write_count;
	i4	hit_count;
	i4	dirty_unfix_count;
	i4	free_buffer_count;
	i4	free_buffer_waiters;
	i4	fixed_buffer_count;
	i4	modified_buffer_count;
	i4	free_group_buffer_count;
	i4	fixed_group_buffer_count;
	i4	modified_group_buffer_count;
	i4	flimit;
	i4	mlimit;
	i4	wbstart;
	i4	wbend;
} IMA_DMF_CACHE_STATS;

/*
** 	A row containing the lock counts
*/
typedef struct _IMA_LOCKS
{
	i4	threadcount;		/* Number of waiting threads */
	i4	resourcecount;		/* Num. resources waited upon */
} IMA_LOCKS;

/*
** 	A row of the ima_cssampler_threads table
*/
typedef struct _IMA_CSSAMPLER_THREADS
{
	i4	 numthreadsamples;
	i4	 stateFREE;
	i4	 stateCOMPUTABLE;
	i4	 stateEVENT_WAIT;
	i4	 stateMUTEX;
	i4	 facilityCLF;
	i4	 facilityADF;
	i4	 facilityDMF;
	i4	 facilityOPF;
	i4	 facilityPSF;
	i4	 facilityQEF;
	i4	 facilityQSF;
	i4	 facilityRDF;
	i4	 facilitySCF;
	i4	 facilityULF;
	i4	 facilityDUF;
	i4	 facilityGCF;
	i4	 facilityRQF;
	i4	 facilityTPF;
	i4	 facilityGWF;
	i4	 facilitySXF;
	i4	 eventDISKIO;
	i4	 eventLOGIO;
	i4	 eventMESSAGEIO;
	i4	 eventLOG;
	i4	 eventLOCK;
	i4	 eventLOGEVENT;
	i4	 eventLOCKEVENT;
} IMA_CSSAMPLER_THREADS;

/*
** 	A row of the ima_cssampler_stats table
*/
typedef struct _IMA_CSSAMPLER
{
	i4 numsamples_count;
	i4 numtlssamples_count;
	i4 numtlsslots_count;
	i4 numtlsprobes_count;
	i4 numtlsreads_count;
	i4 numtlsdirty_count;
	i4 numtlswrites_count;
} IMA_CSSAMPLER;

# define	MAXSTMTS		4	/* Maximum no. of SQL stmts */

typedef struct _PFAPI_CONN
{
	i2	db_no;				/* database number */
	int	state;				/* connection state... */
# define	PFCTRS_STAT_INVALID	0	/*   Uninitialized */
# define	PFCTRS_STAT_INITING	2	/*   Initializing */
# define	PFCTRS_STAT_TERMING	3	/*   Terminating */
# define	PFCTRS_STAT_INITED	4	/*   Initialized */
# define	PFCTRS_STAT_CONNECTING	6	/*   Connecting session */
# define	PFCTRS_STAT_DISCONNECTING 7     /*   Disconnecting session */
# define	PFCTRS_STAT_CONNECTED	8	/*   Connected to server */
	char	vnode_name[DB_MAXNAME+1];	/* vnode name */
	char	db_name[DB_MAXNAME+1];		/* database name */
	char	owner_name[DB_MAXNAME+1];	/* owner name */
	char	dbms_type[9];			/* dbms type */
	int	target_type;			/* to indicate service level */
	int	open_retry;			/* open retry counter */
	IMA_DMF_CACHE_STATS	*cache_stats;	/* Ptr to row of table*/
	II_PTR	connHandle;			/* OpenAPI connection handle */
	II_PTR	tranHandle;			/* OpenAPI transaction handle */
	II_PTR	stmtHandle;			/* OpenAPI statement handle */
	II_PTR  qryHandle[MAXSTMTS];		/* OpenAPI query handles */
	II_PTR	envHandle;			/* OpenAPI environment handle */
	STATUS	status;				/* Status of latest PFapi call*/
} PFAPI_CONN;

/*
** Function signatures
*/
II_EXTERN STATUS II_EXPORT
pfApiInit (PFAPI_CONN *conn);

II_EXTERN STATUS II_EXPORT
pfApiTerm (PFAPI_CONN *conn);

II_EXTERN STATUS II_EXPORT
pfApiConnect (char *dbName, char *userid, char *password, PFAPI_CONN *conn, bool IsImaDb);

II_EXTERN void II_EXPORT
pfApiDisconnect (PFAPI_CONN *conn, bool IsImaDb);

II_EXTERN STATUS II_EXPORT
pfApiSelectCache (PFAPI_CONN *conn, IMA_DMF_CACHE_STATS c_rows[], DWORD *num_rows);

STATUS II_EXPORT
pfApiSelectLocks (PFAPI_CONN *conn, IMA_LOCKS l_rows[], DWORD *num_rows);

STATUS II_EXPORT
pfApiSelectThreads (PFAPI_CONN *conn, IMA_CSSAMPLER_THREADS s_rows[], DWORD *num_rows);

STATUS II_EXPORT
pfApiSelectSampler (PFAPI_CONN *conn, IMA_CSSAMPLER l_rows[], DWORD *num_rows);

STATUS II_EXPORT
pfApiExecSelect (PFAPI_CONN *conn, char *qryid, char *stmt, DWORD *value);

# endif /* PFAPI_H */
