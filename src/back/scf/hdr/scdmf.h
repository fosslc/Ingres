/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: SCDMF.H - Definitions necessary for working with DMF
**
** Description:
**      This file contains structures necessary to interact with
**      DMF.  It is separated from SCS.H in order to reduce
**      the necessity on inclusion of dmf header files in all SCF
**      modules.
**
** History: $Log-for RCS$
**      18-Jun-1986 (fred)
**          Created.
**/
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]

/*}
** Name: ICS_LOCATIONS - List of locations for the database to dmf
**
** Description:
**      This structure defines an array which contains all locations known
**      for this database.  This array is filled in at session startup, and
**      can be deallocated once the database is opened.
**
** History:
**     18-Jun-1986 (fred)
**          Created.
*/
typedef struct _ICS_LOCATIONS
{
    DMC_LOC_ENTRY    loc_entry[];    /* A single location */
}   ICS_LOCATIONS;
[@type_definitions@]
