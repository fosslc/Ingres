/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: SCSERVER.H - Overall internal server definitions
**
** Description:
**      This file contains the definitions for types and constants
**      which are used throughout SCF on a server level.  That is,
**      these items control the overall execution of the system,
**      but are visible at all levels.
**
** History: $Log-for RCS$
**      25-Jun-1986 (fred)
**          Created on Jupiter
**      10-dec-1986 (fred)
**          Added dbdb stuff
**      12-Apr-2004 (stial01)
**          Define db_length as SIZE_TYPE.
**	17-Nov-2010 (jonj) SIR 124738
**	    Add SCV_NODBMO_MASK to prevent making MO objects for
**	    database, typically when fetching iidbdb information.
[@history_template@]...
**/

/*
**  Defines of other constants.
*/

/*
**      Function codes for dbopen()
*/
#define                 SCV_NOOPEN_MASK 0x0001
#define                 SCV_NODBMO_MASK 0x0002

/*}
** Name: SCV_LOC - DMF locations for a database
**
** Description:
**      This structure holds an array of DMC_LOC_ENTRIES for use by
**      SCF in adding and opening databases.  These entries are filled
**      in by the dbdb session on the first attempt to open (and therefore
**      add) the database.
**
** History:
**     2-Jul-1986 (fred)
**          Created on Jupiter.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
[@history_template@]...
*/
struct _SCV_LOC
{
    i4		     loc_l_loc_entry;
	/* All entries which are always present should be placed above here */ 
    DMC_LOC_ENTRY    loc_entry;         /*
					** an array of variable size
					** [1] is just a placeholder
					*/
};
/*
** The following constant is used to compute the size of these
** structures in practice.  Care must be taken to assure its
** correctness.
*/

/*}
** Name: SCV_DBCB - Database control block
**
** Description:
**      This structure contains information about databases which
**      are "known" to SCF.  A known database is one which has been added
**      to DMF.  This structure is used to keep track of resource utilization
**      within the server.
**
**      A known database CB contains the database name, the locations at which
**      the database resides, the mode with which the database was opened,
**      and the count of the number of users accessing the database.
**
** History:
**     25-Jun-1986 (fred)
**          Created on Jupiter.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
[@history_template@]...
*/
struct _SCV_DBCB
{
    SCV_DBCB        *db_next;           /* standard que entries */
    SCV_DBCB	    *db_prev;
    SIZE_TYPE	    db_length;
    i2		    db_type;
#define			DBCB_TYPE   0x103
    i2		    db_s_reserved;
    PTR		    db_l_reserved;
    PTR		    db_owner;
    i4		    db_tag;
#define			DBCB_TAG    CV_C_CONST_MACRO('D', 'B', 'C', 'B')
    i4		    db_ucount;		/* number of users using the database */
    i4		    db_state;
#define			SCV_IN_PROGRESS	0x1
#define			SCV_ADDED	0x2
    i4	    db_flags_mask;
    i4	    db_access_mode;
    PTR		    db_addid;

    DB_OWN_NAME	    db_dbowner;
    DB_DB_NAME	    db_dbname;
    SCV_LOC	    db_location;
    DU_DATABASE	    db_dbtuple;
    CS_SEMAPHORE    db_semaphore;
    /* Star extensions */
    DD_DDB_DESC	    db_ddb_desc;	/* DDB description */
};
/*
[@type_definitions@]*/
