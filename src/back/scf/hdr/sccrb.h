/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: SCCRB.H - Connection Request Block and associated entities
**
** Description:
**      This file contains definitions and types for processing
**      requests for connections to the server.
**
** History: $Log-for RCS$
**      19-Jun-1986 (fred)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name: SCD_CRB - Connection Request Block
**
** Description:
**      This structure is used to request the creation and connection
**      to a session within the Ingres DBMS.  It contains information
**      supplied by the client program which describes the method used
**      to connect to the server, the database to which the user wishes
**      to be attached, the query language to be used (QUEL or SQL0),
**      datatype information (such as date/money format, decimal separator
**      character ($ 3.00 vs. $DM 3,00, etc.), and any other session specific
**      information which happens to be created.
**
**      This structure will have to be very closely tied with the IN and CS
**      portions of the CL.  Perhaps even in them...film at 11 (1986).
**
** History:
**     19-Jun-1986 (fred)
**          Created.
**	7-jul-1993 (shailaja)
**	    Fixed compier warning.	
*/
typedef struct _SCD_CRB
{
    i4		    crb_version;	/* version id of requestor */
    DB_LANG	    crb_qlang;		/* (primary) language of use */
    DB_DECIMAL	    crb_decimal;	/* decimal character (. or ,) */
    DB_DATE_FMT	    crb_date_format;	/* date format requested */
    DB_MONY_FMT	    crb_money_format;	/* money format requested */
    DB_DB_NAME	    crb_dbname;		/* database name */
    DB_OWN_NAME	    crb_username;	/* user name requesting conn. */

	/*
	** Here, things may get machine specific.  Have to wait and see
	*/
    PID		    crb_pid;            /* process id of requester */
/*[@member_definition@]...*/
}	SCD_CRB   ;
