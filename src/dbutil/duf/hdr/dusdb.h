/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DUSDB.H -  defines variables, constants, and typedefs specific
**		    to the system utility, sysmod.
**
** Description:
**        This header file defines variables, constants and typedefs
**	that are specific to the database utility, sysmod.
**
** History: $Log-for RCS$
**      06-nov-86 (ericj)
**          Initial creation.
**	07-may-90 (pete)
**	    Add support for -f flag (similar work done in createdb's "ducdb.h").
**      28-may-98 (stial01)
**          Support VPS system catalogs.
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
*/



/*}
** Name: DUS_MODE_VECT -    vector which describes all of the command
**			    line mode options for the database utility,
**			    sysmod.
**
** Description:
**        This struct is a vector which describes all of the command
**	line mode options for the database utility, sysmod.
**
** History:
**      06-nov-86 (ericj)
**          Initial creation.
**	08-may-1990 (pete)
**	    Add "dus_client_flag" and "dus_client_name" and "MAX_CLIENT_NAME".
**	10-july-1990 (pete)
**	    Remove "dus_client_flag" & change MAX_CLIENT_NAME to DU_MAX..
**      22-jun-99 (chash01)
**          Sysmod now supports RMS Gateway:  (mimicking CREATEDB and DUCDB.H)
**          add "dus_gw_flag" here; so, use GW_NONE and GW_RMS_FLAG definitions
**          in commonhdr!dudbms.h to discriminate database being sysmoded.
[@history_template@]...
*/
typedef struct _DUS_MODE_VECT
{
    i4              dus_dbdb_flag;      /* Is the dbdb being sysmoded? */
    i4		    dus_wait_flag;	/* Wait until the database is
					** is available?
					*/
    i4		    dus_superusr_flag;	/* Was the super user flag used on
					** the command line?
					*/

    i4		    dus_all_catalogs;	/* Sysmod all of the catalogs? */
    /* Gateways modification */
    i4             dus_gw_flag;        /* Sysmod'ing a gateway database */
    i4         dus_page_size;      /* Page size for modify statements */
    char	    dus_progname[256];
    char	    dus_client_name[DU_MAX_CLIENT_NAME+1]; /*dict client names*/
}   DUS_MODE_VECT;

/*
[@type_definitions@]
*/
