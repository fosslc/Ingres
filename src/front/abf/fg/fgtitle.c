/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<cm.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<trimtxt.h>
# include       <abclass.h>
# include       <metafrm.h>
# include       <erfg.h>

/**
** Name:	fgtitle.c - Format Emerald form title
**
** Description:
**	This file defines:
**
**	IIFGfftFormatFrameTitle 	Format title for an Emerald Frame
**
** History:
**	8/25/89 (Mike S)	Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN i4          IIVQotObjectType();


/* static's */

static const	char _empty[] = ERx("");

/*
** Note that the array below in indexed on the MF_xxxx values defined in 
** metaframe.qsh. It must change if they do.
*/
static const ER_MSGID typenames[] =
{				
	S_FG0044_MenuFrame,
	S_FG0045_AppendFrame,
	S_FG0046_UpdateFrame,
	S_FG0048_BrowseFrame,
};

/*{
** Name:   IIFGfftFormatFrameTitle         Format title for an Emerald Frame
**
** Description:
**	Combine description string and frame type into a 1-line title.
**
** Inputs:
**	metaframe	METAFRAME * 	Frame's metaframe
**	width		i4		width of form
**
** Outputs:
**	trimtxt		TRIMTXT ** 	Trim produced
**	
**	Returns:
**		STATUS	OK
**			FAIL	if memory allocation fails
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	8/25/89 (Mike S)	Initial version
*/
STATUS
IIFGfftFormatFrameTitle(metaframe, width, trimtxt)
METAFRAME	*metaframe;
i4		width;
TRIMTXT		**trimtxt;
{
	USER_FRAME *apobj = (USER_FRAME *)metaframe->apobj;
				/* Application object from metaframe */
	char *framedesc;	/* Frame description */
	i4  descsize;		/* Its size */
	char *frametype;	/* Frame type */
	i4  typesize;		/* Its size */
	i4 type;		/* Frame type */
	TRIMTXT *tptr;

	/* Use frame description if it isn't blank, otherwise use name */
	framedesc = apobj->short_remark;
	if ((framedesc == NULL) || (*framedesc == EOS))
		framedesc = apobj->name;

	/* Get frame type string */
	type = IIVQotObjectType(apobj);
	if (type < 0 || type > MF_BROWSE)
		frametype = _empty;		/* This shouldn't happen */
	else
		frametype = ERget(typenames[type]);

	/*
	** Formatting algorithm:
	**
	**	If the frame type doesn't fit in the buffer, give up.
	**	Otherwise, right-justify it.
	**
	**	If we can fit the frame description, and leave at least
	**	two columns before the frame type, center it as best we can.
	**	Otherwise, truncate the frame description to get the two-column
	**	margin.
	*/
	descsize = STlength(framedesc);
	typesize = STlength(frametype);

	/* Make frame type trim */
	if (typesize > width)
	{
		*trimtxt = NULL;	/* Nothing fits -- no title */
		return (OK);
	}
	if ((tptr = (TRIMTXT *)FEreqmem(0, sizeof(TRIMTXT), FALSE, NULL)) 
	    == NULL)
		return (FAIL);
	*trimtxt = tptr;
	tptr->row = 0;
	tptr->column = width - typesize;
	if ((tptr->string = (char *)FEreqmem(0, typesize + 1, FALSE, NULL))
		== NULL)
		return (FAIL);
	STcopy(frametype, tptr->string);
	tptr->next = NULL;
	
	/* See if anything else fits */
	width -= typesize + 2;
	if (width <= 0) return (OK);

	/* Make frame description trim */
	if (descsize > width)
	{
		char *buffer;
		i4	length;

		/* Get a buffer and truncate description */
		if ((buffer = (char *)FEreqmem(0, width + 1, FALSE, NULL)) 
		    == NULL)
			return (FAIL);
		length = CMcopy(framedesc, width, buffer);
		buffer[length] = EOS;
		descsize = STlength(framedesc = buffer);
	}
	if ((tptr->next = (TRIMTXT *)FEreqmem(0, sizeof(TRIMTXT), FALSE, NULL))
	    == NULL)
		return (FAIL);
	tptr = tptr->next;
	tptr->row = 0; 
	tptr->column = (width - descsize) / 2;
	tptr->string = framedesc;
	tptr->next = NULL;
	return (OK);
}
