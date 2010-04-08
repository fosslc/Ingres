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
**  Name: IIUSERADT.C - Default stub for class object entry point
**
**  Description:
**      This routine simply returns.  It is a stub presented to the system for
**	defining no class objects.  There is no complexity. 
**
**          IIclsadt_register - Returns.
**
**
**  History:
**     28-aug-1993 (stevet)
**          Created, largely stolen from IIudadt_register.
**/

/*{
** Name: IIclsadt_register	- Default class library registration function
**
** Description:
**      As mentioned above, this routine does absolutely nothing.  It simply
**	returns.  This is the default activity for a system in which there are
**	no class library objects.  Note that on systems for which class library
**	support has not been purchased, this routine will not be called. 
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
**	28-aug-1993 (stevet)
**	    Created.  Mostly copy from IIudadt_register().
[@history_template@]...
*/

int
IIclsadt_register( ADD_DEFINITION  **add_block, ADD_CALLBACKS *callbacks )
{
    return(0);
}

