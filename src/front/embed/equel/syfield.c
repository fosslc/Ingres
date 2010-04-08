# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<equtils.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<ereq.h>

/*
+* Filename:	syfield.c
** Purpose:	Handle record field names.
**
** Defines:
** (Routines):
**	sym_finit	- Initialize the field stack (for var references).
**	sym_fpush	- Push an entry onto the field stack.
**	sym_fpop	- Pop an entry from the field stack.
**	sym_f_count	- How many entries are on the field stack?
**	sym_f_name	- Get the name of the i'th entry on the field stack.
**	sym_f_entry	- Get a pointer to the i'th entry on the field stack.
**	sym_f_list	- Turn the field stack into a linked list.
**	sym_f_stack	- Turn a linked list into a field stack.
** (Local Data):
**	syFldStack	- The field stack itself.
-*	syFldPtr	- Pointer to the top of the stack.
** Notes:
**
** History:
**	08-Oct-85	- Initial version. (mrw)
**	19-mar-96 (thoda04)
**	    Cleaned up warning about return type of sym_f_stack().
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#define	FLD_STACK_SZ	20
static SYM	*syFldStack[FLD_STACK_SZ] ZERO_FILL;
static SYM	**syFldPtr = syFldStack;

/*
+* Procedure:	sym_f_init
** Purpose:	Initalize the field stack (used for var references).
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	Just clear the stack quickly.
**
** Imports modified:
**	None.
*/

void
sym_f_init()
{
    syFldPtr = syFldStack;
}

/*
+* Procedure:	sym_fpush
** Purpose:	Push an entry onto the field name stack.
** Parameters:
**	s	- SYM *	- Pointer to the entry to push.
** Return Values:
-*	None.
** Notes:
**	Check for overflow first.
**
** Imports modified:
**	None.
*/

void
sym_fpush( s )
SYM	*s;
{
    static	bool err = FALSE;

    if (syFldPtr > &syFldStack[FLD_STACK_SZ-1])
    {
	if (!err)
  	  /* Don't pass name - maybe no var */
	    er_write( E_EQ0255_syFSTAKOFLO, EQ_ERROR, 0 );
	err = TRUE;
    }
    else
    {
	*syFldPtr++ = s;
	err = FALSE;
    }
}

/*
+* Procedure:	sym_fpop
** Purpose:	Pop an entry from the field name stack.
** Parameters:
**	s	- SYM *	- Pointer to the entry to push.
** Return Values:
-*	SYM *	- The popped entry.
** Notes:
**	Check for underflow first.
**
** Imports modified:
**	None.
*/

SYM *
sym_fpop()
{
    if (syFldPtr > syFldStack)
	return *--syFldPtr;
    else
	return (SYM *) NULL;
}

/*
+* Procedure:	sym_f_count
** Purpose:	Count the entries on the field stack.
** Parameters:
**	None.
** Return Values:
-*	i4	- The number of entries on the stack.
** Notes:
**	None.
**
** Imports modified:
**	None.
*/

i4
sym_f_count()
{
    return syFldPtr - syFldStack;
}

/*
+* Procedure:	sym_f_name
** Purpose:	Get a pointer to the i'th name on the field stack.
** Parameters:
**	i	- i4	- Index of the entry desired.
** Return Values:
-*	char *	- Pointer to the i'th name.
** Notes:
**	Check for legal index.
**
** Imports modified:
**	None.
*/

char *
sym_f_name( i )
i4	i;
{
    if (i < 0 || i > sym_f_count()-1)
	return ERx("<Field index out-of-bounds>");
    else
	return (char *) sym_str_name(syFldStack[i]);
}

/*
+* Procedure:	sym_f_entry
** Purpose:	Get a pointer to the i'th entry on the field stack.
** Parameters:
**	i	- i4	- Index of the entry desired.
** Return Values:
-*	SYM *	- Pointer to the desired entry.
** Notes:
**	Check for a legal index.
**
** Imports modified:
**	None.
*/

SYM *
sym_f_entry( i )
i4	i;
{
    if (i < 0 || i > sym_f_count()-1)
	return NULL;
    else
	return syFldStack[i];
}

/*
+* Procedure:	sym_f_list
** Purpose:	Turn the stack of entries into a list.
** Parameters:
**	None.
** Return Values:
-*	SYM *	- Pointer to the head of the list.
** Notes:
**	- Take a stack of entries and make it into a linked list,
**	  with the first entry being the first entry in the stack.
**	- Link the entries through st_type.
**
** Imports modified:
**	Clears the stack.
*/

SYM *
sym_f_list()
{
    register SYM
		*r, *s;
    i4		i, count;

    count = sym_f_count();
    if (count == 0)
	return NULL;
    r = s = sym_f_entry(count-1);
    s->st_type = NULL;
    for (i=count-2; i>=0; i--)
    {
	s = sym_f_entry(i);
	r->st_type = s;
	r = s;
    }
    sym_f_init();
    return s;
}

/*
+* Procedure:	sym_f_stack
** Purpose:	Turn a list into a stack.
** Parameters:
**	s	- SYM *	- Pointer to the head of the list.
** Return Values:
-*	None.
** Notes:
**	Take a list, linked through st_type, and make it a stack
**	(stacked from start to end).
**
** Imports modified:
**	Modifies the field stack.
*/

void
sym_f_stack( s )
SYM	*s;
{
    sym_f_init();
    while (s)
    {
	sym_fpush( s );
	s = s->st_type;
    }
}
