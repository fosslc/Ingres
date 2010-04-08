/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<duucatdef.h>

/**
**
**  Name: DUIFEDEF.C - Modify commands for standard catalogs
**
**  Description:
**	This file holds tables (and functions to initialize those
**	tables) which define the proper modify commands on the standard
**	catalogs.
**
**
**  History:    
**	12-nov-1988 (jrb)
**	    Added minpages option to ii_objects_index. 
**	28-nov-1988 (jrb)
**	    Removed iialternate_keys.
**	    Cleaned up to meet coding standards.
**      03-nov-1989 (alexc)
**          Remove word 'unique' from modify ii_abfobjects.
**	9-may-1990 (pete)
**	    Gut all the FE catalog info. In 6.4, FE cats are handled by a new
**	    flag (-f) and executable (modifyfe). Remove the array of character
**	    strings: "GLOBALDEF   char	*Dui_20fecat_commands[]	= {...}" -- no
**	    longer used.
**	12-Jul-1990 (pete)
**	    Totally remove things that were just NULLed out in the 9-may
**	    history. Remove: dui_fecats_def() and references to
**	    Dui_20fecat_commands. Change file comments to say that this only
**	    applies to standard catalogs now.
**      28-sep-96 (mcgem01)
**          Global data moved to duidata.c
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
*/

GLOBALREF   char	*Dui_50scicat_commands[];
GLOBALREF   DUU_CATDEF	Dui_51scicat_defs[DU_5MAXSCI_CATDEFS + 1];

/*
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
[@function_definition@]...
*/


/*{
** Name: dui_scicats_def - setup array of std cat modify definitions
**
** Description:
**	Modify and index commands for the standard catalogs.
**	
** Inputs:
**	none
**	
** Outputs:
**	none
**	
** Side Effects:
**	    Defines global array Dui_51scicat_defs
**
** History:
**	29-nov-1988 (jrb)
**	    Removed iialternate_keys (no longer a std cat).
**	    Removed ii_id (this is a fe cat and shouldn't have been here!).
*/
VOID
dui_scicats_def()
{
    /* iidbcapabilities definition */
    Dui_51scicat_defs[0].du_relname	= Dui_50scicat_commands[0];
    Dui_51scicat_defs[0].du_modify	= NULL;
    Dui_51scicat_defs[0].du_index	= NULL;
    Dui_51scicat_defs[0].du_index_cnt	= 0;
    Dui_51scicat_defs[0].du_requested	= FALSE;
    Dui_51scicat_defs[0].du_exists	= FALSE;

    Dui_51scicat_defs[1].du_relname	= NULL;
    Dui_51scicat_defs[1].du_modify	= NULL;
    Dui_51scicat_defs[1].du_index	= NULL;
    Dui_51scicat_defs[1].du_index_cnt	= 0;
    Dui_51scicat_defs[1].du_requested	= FALSE;
    Dui_51scicat_defs[1].du_exists	= FALSE;

    return;
}
