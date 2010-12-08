/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<ol.h>

/**
** Name:	olret.c	-	Contains OLretassign.
**
** Description:
**	Contains the routine OLretassign which takes the return
**	value from a procedure called by OLpcall and puts the
**	result into an OLret.  This is a CL routine because
**	OLpcall must call a routine that can return a one of
**	four types (i4, f8, char *, or OL_STRUCT_PTR).  The way these are
**	returned varies from system to system, so OLpcall will
**	have to call this routine differently.
**
**	On the Pyramid, all OLpcall pretends all procedures return
**	a double.  OLpcall passes the address of the double to
**	OLretassign which pulls the right return value out of
**	the double.
**
** History:
**    4004 BC (Bishop Usher) Creation of the Earth
**    7/93 (Mike S) Add OL_PTR case.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      29-Nov-2010 (frima01) SIR 124685
**          Added void return type to OLretassign.
*/

/*{
** Name: 	OLretassign	- Move a return value into a OLret.
**
** Description:
**	Take the return value in the double pointed to by value and
**	move it into the OLret pointed to by retvalue.  The type
**	that is in the double is given by rettype.
**
** Inputs
**		value	-	Pointer to a double containing the value;
**		rettype -	The type of the value in value.
** Outputs
**		retvalue -	The place the put the value.
**
*/
void
OLretassign(value, rettype, retvalue)
double	*value;
i4	rettype;
OL_RET	*retvalue;
{
	if (retvalue == NULL)
		return;
	switch (rettype)
	{
	  case OL_NOTYPE:
		return;
	
	  case OL_I4:
		retvalue->OL_int = *((i4 *) value);
		return;
	
	  case OL_F8:
		retvalue->OL_float = *((f8 *) value);
		return;

	  case OL_STR:
		retvalue->OL_string = *((char **) value);
		return;

	  case OL_PTR:
		retvalue->OL_ptr = *((OL_STRUCT_PTR *)value);
		return;
	}
}
