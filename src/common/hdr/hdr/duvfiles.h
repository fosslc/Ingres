/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DUVFILES.H -	structs and definitions used in the status/audit and
**			log files.
**
** Description:
**        This file contains struct and constant definitions used in the
**	status/audit and log files used by CONVTO60.  These structs should
**	be used by the 5.0, and 6.0 DBMS and FE phases.
**
** History: $Log-for RCS$
**      09-Jul-87 (ericj)
**          Initial creation.
**      25-Mar-88 (ericj)
**          Removed all references to the global log file.
[@history_template@]...
**/
/*
[@forward_type_references@]
*/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DU_STATUS duv_0statf_open(); /* Opens the CONVTO60 status/audit
					 ** file.
					 */
FUNC_EXTERN DU_STATUS duv_finit();      /* Initializes the file location for
					** one of the CONVTO60 utility files.
					*/
FUNC_EXTERN DU_STATUS duv_2statf_read();    /* Reads the CONVTO60 status/audit
					    ** file into a global buffer.
					    */
FUNC_EXTERN DU_STATUS duv_3statf_write();   /* Writes a global buffer into the
					    ** CONVTO60 status/audit file.
					    */
FUNC_EXTERN DU_STATUS duv_determine_status();	/* Determines the status of
						** a database's conversion.
						*/
FUNC_EXTERN DU_STATUS duv_f_close();	/* Closes the log or status files. */
FUNC_EXTERN DU_STATUS duv_dbf_open();	/* Initializes and opens the database
					** specific log file.
					*/
FUNC_EXTERN VOID duv_prnt_msg();	/* Prints a message to specified files.
					*/
FUNC_EXTERN DU_STATUS duv_iw_msg();	/* Formats and then prints a warning or
					** an information message to the
					** specified files.
					*/
/*
[@function_reference@]...
*/

/*
**  Defines of other constants.
*/

/*
**      Constants used to specify to duv_prnt_msg() what files to print the
**	message to.
*/
#define			DUV_STDOUT	    01
/* Note, the global log file used to be defined as 02 */
#define			DUV_DBF		    04

#define			DUV_ALLF	    (DUV_STDOUT | DUV_DBF)
#define			DUV_SDF		    (DUV_STDOUT | DUV_DBF)


/*
**      A constant to specify the maximum length of a path and filename
**	in both 5.0 and 6.0 versions of Ingres.  This number was derived
**	by taking the values of MAXPATH and MAXFILENAME found in the
**	5.0 header, ingconst.h, and adding a little padding.
*/
#define                 DUV_FMAXPATH    272

/*
**	Define the maximum filename length that can be used in both
**	5.0 and 6.0.  Currently, this is a figure taken from the
**	5.0 header, ingconst.h, with some padding added.
*/
#define			DUV_FMAXNAME		16

/*
**	Define the names of the CONVTO60 utility copy files used to
**	get data from 5.0 to 6.0.
*/
#define			DUV_REL50DAT		"rel50.dat"
#define			DUV_ATT50DAT		"att50.dat"
#define			DUV_LOC50DAT		"loc50.dat"
#define			DUV_CODE50DAT		"code50.dat"
#define			DUV_IND50DAT		"ind50.dat"
#define			DUV_EXT50DAT		"ext50.dat"


/*
[@group_of_defined_constants@]...
*/

/*}
** Name: DUV_CONV_STAT - defines the contents of the CONVTO60 status/audit file.
**
** Description:
**        This struct defines the contensts of the CONVTO60 status/audit file.
**	This file and its internal structure must be referenced in the 5.0
**	phase, and the 6.0 DBMS and FE phases.
**
** History:
**      07-Jul-87 (ericj)
**          Initial creation.
**      30-Dec-88 (teg)
**          Added program status DUV_FECRIT_OK and audit status
**	    DUV_45FECRIT_DONE.  Also added DUV_FE_CRITICAL and DUV_FE_FINAL.
[@history_template@]...
*/

/* constants used to specify which FE Program/Audit status to set via
** DUV_ONELAST_THING().
*/
#define			DUV_FE_CRITICAL	 0  /* FE critical Phase complete */
#define			DUV_FE_FINAL	 1  /* all FE conversion complete */
typedef struct _DUV_CONV_STAT
{
    i4              du_prog_stat;       /* Program status passed from one
					** phase of CONVTO60 to another.
					*/
#define                 E_DU_PROG_OK    0
#define			DUV_DBMS60_OK	    1  /* 6.0 BE Conversion */
#define			DUV_FE60_OK	    2  /* noncritical FE conversion */
#define			DUV_FECRIT_OK	    3  /* critical FE conversion */
#define			DUV_DBMSBAD_PROG    -2
#define			DUV_NOPROG_CALL	    -1
    i4		    du_audit_stat;	/* Last action to be successfully
					** completed by one of the phases
					** of CONVTO60.
					*/
/*
**      The following defines are used by the 5.0 phase of CONVTO60 to
**	insure idempotence and increase efficientcy.  They are defined
**	so as to never conflict with valid values in access field
**	of the database catalog for existing 5.0 databases.  Currently, the
**	only bits being used by 5.0 are the 0th and 1st.
*/
#define	DUV_1STATUS_FILE_EXISTS		1	/* The temporary CONVTO60 status
						** table has been created.
						*/
#define	DUV_2CONVTO60_TABS		2	/* The temporary CONVTO60 util-
						** ity files have been created.
						** These include a user 
						** name-code map table for all
						** dbs and a database audit
						** table for the 5.0 dbdb.
						*/
#define	DUV_3DBDB_ENTRIES_MADE		3	/* The 6.0 database database
						** entries have been made in the
						** 5.0 dbdb.  This is only app-
						** licable when converting dbdb.
						**
						*/
#define	DUV_4DBDB_COPIED		4	/* The 5.0 database database,
						** dbdb, has been copied to its
						** 6.0 directory, iidbdb.  Only
						** applicable when converting
						** dbdb.
						*/
#define	DUV_5BTREES_CONVERTED		5	/* The database's Btrees have
						** been successfully converted.
						*/
#define	DUV_6QRYMOD_DEFS		6	/* The database's qrymod
						** definitions have been written
						** out to a text file.
						*/
#define	DUV_7ADMIN_RENAMED		7	/* The database's 5.0 admin
						** file has been renamed.  This
						** is done to prevent other users
						** from accessing it while it's
						** being converted.
						*/
#define DUV_30CORE_CATS_MADE		30	/* The core system catalogs have
						** been made.  This implies
						** that the database can be
						** opened using 6.0 with 
						** special priveledge flags.
						*/
#define	DUV_31DBCATS_MADE		31	/* The rest of the database
						** catalogs have been
						** successfully booted.
						*/
#define	DUV_310_UTILTAB_MADE		310	/* The utility tables have
						** been created and data from
						** v5 copy files has been read
						** in.
						*/
#define	DUV_311_RELTAB_PATCH		311	/* the iirelation utility table
						** has been patched to resemble
						** v6 values */
#define	DUV_312_ATTTAB_PATCH		312	/* the iiattribute utility table
						** has been patched to contain
						** table ids. */
#define	DUV_313_INDTAB_PATCH		313	/* the iiindex utility table
						** has been patched to contain
						** table ids. */
#define	DUV_314_RELTAB_POPULATE		314	/* the iirelation table has been
						** populated from its hybrid
						** conversion table. */
#define	DUV_315_ATTTAB_POPULATE		315	/* the iiattribute table has been
						** populated from its hybrid
						** conversion table. */
#define	DUV_316_INDTAB_POPULATE		316	/* the iiindex table has been
						** populated from its hybrid
						** conversion table. */
#define	DUV_32UTILTABS_MADE		32	/* The CONVTO60 utility tables
						** have been made, populated,
						** patched and entries
						** appended to the 6.0 system
						** catalogs.
						*/
#define	DUV_33TABS_RENAMED		33	/* The 5.0 user tables and
						** system catalogs have been
						** renamed to their appropriate
						** 6.0 filename.
						*/
#define	DUV_330_DBDB_CATS_MADE		330	/* The iidbdb specific v6
						** catalogs have been created */
#define DUV_331_IIDATABASE_PATCH	331	/* The iidatabase catalog has
						** been patched to have it v6
						** values */
#define	DUV_332_IIUSER_CONVERT		332	/* The iiuser catalog has been
						** updated with v5 user info */
#define	DUV_333_IIDBPRIV_CVT		333	/* The iidbaccess catalog has 
						** been updated with v5 dbaccess
						** info and also updated with
						** private db info */
#define	DUV_334_LOCATION_CVT		334	/* The iilocations catalog has 
						** been updated with v5 location
						** info */
#define	DUV_335_EXTEND_CONVERT		335	/* The iiextend catalog has 
						** been updated with v5 extended
						** location info */
#define	DUV_336_STATISTICS_CVT		336	/* The iistatistics catalog has
						** been updated with info from 
						** the v5 zopt1stat catalog. */
#define	DUV_337_HISTOGRAM_CVT		337	/* The iihistogram catalog has
						** been updated with info from 
						** the v5 zopt2stat catalog. */
#define	DUV_34SYSCATS_CONVERTED		34	/* The 5.0 DBMS system catalogs
						** have been converted.  
						*/
#define	DUV_35EXTLOCS_MADE		35	/* The locations that this
						** database has been extended to
						** have been made known to the
						** DBMS.
						*/
#define	DUV_37LAST_TABID_UPD		37	/* The last table id field in
						** the database's config file
						** has been updated.
						*/
#define	DUV_36QRYMOD_DEFS		36	/* The query mod definitions
						** have been redefined in the
						** database.
						*/
#define DUV_45FECRIT_DONE	        45      /* FECVT60 Critical Phase
						** completed.
						*/

/* {@fix_me@} */
/* Later, this entry and many others should be made greater than
** DUV_NOBACK.  For right now, we'll leave it this way for ease
** of testing the BACKTO50 function.
*/
#define	DUV_50DB60_OPERATIVE		50	/* The database being converted
						** has been marked operative
						** in the 6.0 database database.
						*/
#define	DUV_51NO50_DBDB_ENTRY		51	/* The database's entries have
						** been removed from the
						** database database's catalogs.
						** This marks that the database
						** has been sucessfully converted.
						*/


#define		    DUV_NOBACK		    DUV_7ADMIN_RENAMED
#define		    DUV_50PASSOFF	    DUV_7ADMIN_RENAMED
#define		    DUV_LAST_DBMS_ACTION    DUV_36QRYMOD_DEFS
/*
[@defined_constants@]...
*/
}	DUV_CONV_STAT;



/*}
** Name: DU_FILE - describes one of the utility files used by CONVTO60.
**
** Description:
**        This struct describes one of the utility files used by CONVTO60.
**	These files include the audit file (used to inform the user as to
**	what actions have occurred in CONVTO60), the log file (used internally
**	by CONVTO60 to guarantee idempotence), the status file (used to
**	communicate status between the various phases of CONVTO60).
**
** History:
**      05-Jun-87 (ericj)
**          Initial creation.
[@history_template@]...
*/
typedef struct _DU_FILE
{
    LOCATION        du_floc;            /* The files location. */
    FILE	    *du_file;		/* If this is non-null, then the file
					** is open.
					*/
    char	    du_flocbuf[DUV_FMAXPATH];	/* Buffer to
						** associate with the
						** location field.
						*/
}   DU_FILE;

#ifdef	FIX_ME

/*}
** Name: DU_INTERNAL_STATE - describes the internal state of the 5.0 phase
**			      of the CONVTO60 program.
**
** Description:
**        This structure is used primarily for cleaning up in the exit
**	handler.
**
** History:
**      05-Jun-87 (ericj)
**          Initial creation.
**	19-nov-90 (teresa)
**	    comment out FIX_ME on #endif line
[@history_template@]...
*/
typedef struct _DU_INTERNAL_STATE
{
    DU_FILE        du_audit_file;	/* Audit file description. */
    DU_FILE        du_log_file;		/* Log file description. */
    DU_FILE        du_status_file;	/* Status file description. */
}   DU_INTERNAL_STATE;
#endif	/* FIX_ME */
/*
[@type_definitions@]
*/

/*
** References to globals variables declared in other C files.
*/


GLOBALREF DU_FILE             Duv_statf;	    /* CONVTO60 file descriptor
						    ** used for the audit/status
						    ** file.
						    */
GLOBALREF DU_FILE             Duv_dbf;		    /* CONVTO60 file descriptor
						    ** used for the db specific
						    ** log file.
						    */
GLOBALREF DUV_CONV_STAT	      Duv_conv_status;		/* Buffer for the
							** conversion status
							** and log file.
							*/
/*
[@global_variable_references@]
*/
