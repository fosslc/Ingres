/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<er.h>
#include        <cs.h>      /* Needed for "erloc.h" */
#include	"erloc.h"

#define	ALLCLASS    -1


/*{
** Name:	ERrelease - release fast messages for class from memory.
**
** Description:
**	This procedure frees area of number table and message data for class
**	'class_no'.  It returns	all memory associated with that class.
**	Subsequent calls to ERget may reinitialize the class files.
**
**	It must not be used in server.
**
** Inputs:
**	ER_CLASS class_no	class number to free area 
**
** Outputs:
**	Returns:
**		STATUS 	OK			if everything worked
**			ER_NOFREE		if something cannot do MEfree
**			ER_BADCLASS		if class_no is incorrect
**
**	Exceptions:
**		none
**
** Side Effects:
**	Nulls out any pointers to fast class messages that may be in use.
**
** History:
**	01-Oct-1986 (kobayashi) - first written
**	17-mar-1989 (Mike S) - "data pointer" points to message text
**	08-feb-1993 (smc) - cast MEfree pram to PTR to clear cc warnings.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
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
*/
STATUS
ERrelease(class_no)
ER_CLASS class_no;
{
i4  startvalue;
i4  endvalue;
i4  i;
i4  erindex;
ER_CLASS_TABLE	*ERclass;
i4  language = -1;
STATUS status;

	/*
	**	 find the index of ERmulti 
	*/

	if ((erindex = cer_fndindex(language)) == -1)
	{
	/* That language isn't opened yet and  set up class buffer yet. */
	    return(OK);
	}
	ERclass = ERmulti[erindex].class;
	if (ERclass == NULL)
	{	/* No classes yet allocated */
	    return(OK);
	}

	/*
	**	 Is class_no correct 
	*/

	if (class_no != ALLCLASS && (class_no < 0 || 
	class_no >= ERmulti[erindex].number_class))
	{
	/* Bad class number */
		return(ER_BADCLASS);
	}

	/*
	**	To use loop, set start value and end value here 
	*/

	if (class_no == ALLCLASS) 
	{
		/* Free all class */
		startvalue = 0;
		endvalue = ERmulti[erindex].number_class;
	}
	else
	{
	/* Free one class */
		startvalue = class_no;
		endvalue = startvalue + 1;
	}
	
	/* 
	** 	free loop
	*/

	status = OK;
	for (i = startvalue; i < endvalue; ++i) 
	{
		if (ERclass[i].message_pointer != (u_char **)NULL) 
		{
			
			/* 
			**	free number table and message ares 
			*/
			
			if (MEfree((PTR)ERclass[i].data_pointer) != OK)
			{
			/* Can't free */
				return(ER_NOFREE);
			}
			if (MEfree((PTR)ERclass[i].message_pointer) != OK)
			{
			/* Can't free */
				return(ER_NOFREE);
			}
			ERclass[i].message_pointer = (u_char **)NULL;
			ERclass[i].data_pointer = (u_char *)NULL;
			ERclass[i].number_message = 0;
		}
		else if (class_no != ALLCLASS)
		    status = ER_BADCLASS;
	}
	return(status);
}
