/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DUCDB.H -   defines variables, constants and typedefs specific to
**		    the system utility, createdb.
**
** Description:
**        This header file defines variables, constants and typedefs that
**	are specific to the database utility, createdb.
**
** History: 
**      20-Sep-86 (ericj)
**          Initial creation.
**	27-Jan-88 (ericj)
**	    Updated to support sort locations.
**	13-jun-1989 (linda)
**	    Modification for Gateways.
**	4-may-1990  (pete)
**	    Add support for FE dictionary client names on Createdb command line.
** 	22-apr-1997 (wonst02)
** 	    Added duc_rdonly_flag to support readonly databases.
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@global_variable_references@]
*/


/*}
** Name: DUC_MODE_VECT -    vector which describes all of the command line
**			    options.
**
** Description:
**        This struct is a vector which describes all of the command line
**	mode options for the database utility, createdb.
**
** History:
**      20-Sep-86 (ericj)
**          Initial creation.
**      27-dec-1988 (alexc)
**          Modification for TITAN.
**	08-feb-1989 (EdHsu)
**	    updated to support online backup
**	4-may-1990  (pete)
**	    Add support for FE dictionary client names on Createdb command line:
**	    add "duc_client_name" & DU_MAX_CLIENT_NAME (remove structure
**	    element: duc_client_flag, which was present in early versions).
**	    (note that DU_MAX_CLIENT_NAME is defined in dudbms.qh)
**	22-may-90 (linda)
**	    Added GW_NONE definition for duc_gw_flag.
**	6-jul-90 (linda)
**	    Moved GW_NONE and GW_RMS_FLAG definitions to commonhdr!dudbms.h, so
**	    they can be accessed by FECVT60.
** 	22-apr-1997 (wonst02)
** 	    Added duc_rdonly_flag to support readonly databases.
**	19-mar-2001 (stephenb)
**	    added duc_national_flag
**      21-Apr-2004 (nansa02)
**         Added duc_page_size and duc_all_catalogs for modifying system catalogs.  
**	04-Feb-2005 (gupsh01)
**	    Added duc_unicode_nfc flag to indicate if Normal form C 
**	    normalization is required for Unicode.
**
[@history_template@]...
*/
typedef struct _DUC_MODE_VECT
{
    i4              duc_dbdb_flag;      /* Was the "-S" flag used? */
    i4		    duc_private_flag;	/* Was the "-p" flag used? */
    i4		    duc_dbloc_flag;	/* Was the "-d" flag used? */
    i4		    duc_jnlloc_flag;	/* Was the "-j" flag used? */
    i4		    duc_dmploc_flag;	/* Was the "-b" flag used? */
    i4		    duc_ckploc_flag;	/* Was the "-c" flag used? */
    i4		    duc_sortloc_flag;	/* Was the "-s" flag used? */
    i4		    duc_rdonly_flag;	/* Was the "-r" flag used? */
    i4		    duc_national_flag;	/* Was the "-n" flag used? */
    i4		    duc_unicode_nfc;	/* Was the "-i" flag used? */
    i4              duc_page_size;      /* Was the "-p" flag used? */
    /* TITAN modification */
    i4              duc_star_flag;      /* Creating a star database */
    /* Gateways modification */
    i4              duc_gw_flag;        /* Creating a gateway database */
    i4              duc_all_catalogs;   /*For modifying all system catalogs*/
    /* {@fix_me@} */
    char	    duc_progname[256];	/* The program name */
    char	    duc_client_name[DU_MAX_CLIENT_NAME+1];/*dictionary clients*/
}   DUC_MODE_VECT;

/*
[@type_definitions@]
*/
