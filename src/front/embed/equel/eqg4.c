/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>
# include	<st.h>
# include	<er.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<g4defs.h>
# include	<g4consts.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<ereq.h>


/**
** Name:	eqg4.c		- Support routines for generating EXEC 4GL
**
** Description:
**	This file defines:
**
**	g4_check	Translate an INQUIRE_4GL or SET_4GL request
**
** History:
**	11/92 (Mike S) Initial version
**	8/93 (Mike S) Change 'recordtype' to 'recordtypename' to match docs.
**		Bug 53510
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* typedefs */
/* 
** valid request names for INQUIRE_4GL and SET_4GL.
** Note that the names must be in alphabetical order for the
** binary search to work.
*/
typedef struct requests
{
   char *request_name;
   bool need_object;
   i4   code;
} REQUESTS;

/* forward references */
/* extern's */
/* static's */

static REQUESTS inq_requests[] = 
{
    {ERx("allrows"),	TRUE,	G4IQallrows},
    {ERx("classname"),	TRUE,	G4IQclassname},
    {ERx("errorno"), 	FALSE, 	G4IQerrno},
    {ERx("errortext"),  FALSE,	G4IQerrtext},
    {ERx("firstrow"),	TRUE,	G4IQfirstrow},
    {ERx("isarray"),	TRUE,	G4IQisarray},
    {ERx("lastrow"),	TRUE,	G4IQlastrow},
    {ERx("recordtypename"),	TRUE,	G4IQclassname}
}; 

static REQUESTS set_requests[] =
{
    {ERx("messages"), 	FALSE, 	G4STmessages}
};


/*{
** Name:	g4_check - Check request name for INQUIRE_4GL or SET_4GL
**
** Description:
**	The code for the request name is returned, if the request is valid.
**	Otherwise an error is emitted and -1 returned.
**
** Inputs:
**	id		i4	SET or INQUIRE
**	request		char *	request name
**	isobject	bool	Was an object provided	
**
** Returns:
**	i4	code number
**
** Side Effects:
**	An error may be emitted, which will cause the proprocessor to return
**	failure.
**
** History:
**	11/92 (Mike S) Initial version
*/
i4
g4_check(i4 id, char *request, bool isobject)
{
    REQUESTS *requests;
    i4  bot;
    i4  top;
    i4  size;
    i4  probe;

    if (id == IIINQ4GL)
    {
	requests = inq_requests;
	size = sizeof(inq_requests) / sizeof(REQUESTS);
    }
    else
    {
	requests = set_requests;
	size = sizeof(set_requests) / sizeof(REQUESTS);
    }

    bot = 0;
    top = size - 1;
    do
    {
	i4 comp;

	probe = (top + bot) / 2;
	comp = STbcompare(request, 0, requests[probe].request_name, 0, TRUE);
	if (comp == 0)
	    break;
	else if (comp < 0)
	    top = probe - 1;
	else
	    bot = probe + 1;
    } while (bot <= top);

    if (bot > top)
    {
	/* Search failed */
	er_write(E_EQ0081_TL_ATTR_UNKNOWN, EQ_ERROR, 1, request);
    }
    else
    {
	/* We found a keyword match */
	if (requests[probe].need_object && !isobject)
	{
	    er_write(E_EQ0153_fsREQOBJ, EQ_ERROR, 1, request);
	}
	else if (!requests[probe].need_object && isobject)
	{
	    er_write(E_EQ0154_fsNOTOBJ, EQ_ERROR, 1, request);
	}
	else
	{
	    /* We're OK */
	    return requests[probe].code;
	}
    }

    /* Something was wrong */
    return -1;
}
