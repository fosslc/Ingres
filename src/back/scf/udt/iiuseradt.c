/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

# include <compat.h>
# include <dbms.h>
# include <adf.h>
# include <add.h>

/**
**
**  Name: IIUSERADT.C - Default stub for user ADT entry point
**
**  Description:
**      This routine simply returns.  It is a stub presented to the system for
**	defining no user defined datatypes.  There is no complexity. 
**
**          IIudadt_register - Returns.
**
**
**  History:
**      4-may-1989 (fred)
**          Created.
**/

/*{
** Name: IIudadt_register	- Default datatype registration function
**
** Description:
**      As mentioned above, this routine does absolutely nothing.  It simply
**	returns.  This is the default activity for a system in which there are
**	no user defined datatypes.  Note that on systems for which user defined
**	datatype support has not been purchased, this routine will not be called. 
**
** Inputs:
**      add_block                       Pointer to pointer to block to filled in
**					with the datatypes being defined.
**
** Outputs:
**      *add_block                      Were this function doing something, the
**					add block pointer would be filled with a
**					pointer to the datatypes to be defined.
**					However, since the intent of this
**					routine is to define NO datatypes, this
**					value is untouched -- that is, it
**					remains at zero.
**
**	Returns:
**	    0 -- indicating success.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-may-1989 (fred)
**          Created.
**	2-Jul-1993 (daveb)
**	    prototyped, added header inclusions for right types.
[@history_template@]...
*/

int
IIudadt_register( ADD_DEFINITION  **add_block, ADD_CALLBACKS *callbacks )
{
    return(0);
}

