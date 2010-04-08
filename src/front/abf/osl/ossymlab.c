/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<oserr.h>
#include	<ossym.h>

/**
** Name:    ossymlab.c -	OSL Parser Symbol Table Labels Module.
**
** Description:
**	Contains routines used to enter and manipulate label symbol table
**	entries.  Defines:
**
**	oslblbeg()	enter label in symbol table.
**	oslblend()	mark label inactive in symbol table.
**	oslblcheck()	return symbol table entry for label.
**
** History:
**	Revision 6.4/02
**	10/02/91 (emerson)
**		Minor modifications to oslblbeg, oslblend, and oslblcheck.
**		(For bugs 34788, 36427, 40195, 40196).
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Made Labels static; initialize its s_brightness field.
**
**	Revision 6.0  87/06  wong
**	Made labels symbol table local to this module.  02/87  (jhw).
**
**	Revision 5.1  86/10/17  16:16:21  wong
**	Extracted from "oslooptmp.c", 07/86.
**/

/* Labels Symbol Table */

static	OSSYM	Labels ZERO_FILL;

/*{
** Name:    oslblbeg() -	Enter Label in Symbol Table.
**
** Description:
**	Enters a label used in an OSL `while' loop into the symbol table.
**
** Input:
**	name	{char *}  The label name.
**	block	{PTR}	  A pointer to an opaque control block
**			  (to be returned to the caller of oslblcheck).
**
** History:
**	10/02/91 (emerson)
**		Added new parameter BLOCK.  Store it in the new symbol table
**		entry (in s_block), in lieu of setting OS_LABELACT in s_ref.
**		(For bugs 34788, 36427, 40195, 40196).
*/

OSSYM *
oslblbeg (name, block)
char	*name;
PTR	block;
{
    register OSSYM	*label;

    extern char	*osFrm;

    Labels.s_name = ERx("$LABEL");
    Labels.s_kind = OSLABEL;
    Labels.s_brightness = OS_DARK;
    if ((label = ossympeek(name, &Labels)) != NULL)
    {
	oscerr(OSDUPLABEL, 2, name, osFrm);
	return NULL;
    }
    label = ossymput(name, OSLABEL, (DB_DATA_VALUE*)NULL, &Labels, 0);
    label->s_block = block;

    return label;
}

/*{
** Name:    oslblend() -	Mark Label Inactive in Symbol Table Entry.
**
** Description:
**	Mark a label used in an OSL 'while' loop as no longer active
**	when the matching 'endwhile' statement has been reached.
**
** History:
**	10/02/91 (emerson)
**		Clear s_block (in lieu of clearing OS_LABELACT in s_ref).
**		(For bugs 34788, 36427, 40195, 40196).
*/

VOID
oslblend (label)
OSSYM	*label;
{
    if (label != NULL)
    {
        label->s_block = NULL;
    }
}

/*{
** Name:    oslblcheck() -	Return Symbol Table Entry for Label.
**
** Description:
**	This routine checks that a label of the specified name exists
**	and is still active.  If so, this routine returns the pointer
**	to the opaque control block that was specified when the label's
**	symbol table entry was created (by oslblbeg).  If not, it returns NULL.
**
** History:
**	10/02/91 (emerson)
**		Check s_block (in lieu of checking OS_LABELACT in s_ref).
**		Return s_block instead of the symbol table pointer.
**		(For bugs 34788, 36427, 40195, 40196).
*/

PTR
oslblcheck (name)
char	*name;
{
    register OSSYM   *label;

    if (name == NULL || *name == EOS)
    {
	return NULL;
    }

    label = ossympeek(name, &Labels);

    if (label == NULL)
    {
	oscerr(OSNOLABEL, 1, name);
	return NULL;
    }
    if (label->s_block == NULL)
    {
	oscerr(OSBADLABEL, 1, name);
	return NULL;
    }
    return label->s_block;
}
