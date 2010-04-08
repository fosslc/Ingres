/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<st.h>
# include	<er.h>
# include       <si.h>
# include       <lo.h>
# include	<fe.h>
# include	<ug.h>
# include       <adf.h>
# include       <afe.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:	xfregstr.sc - This file is used to write out the registrations
**				definitions for INGRES STAR.
**
** Description:
**	This file defines:
**
**	xfregistrations		write statements to create the 
**				appropriate registrations.
**
** History:
**	29-sep-88 (marian) 
**		written to support REGISTRATIONS.
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**	04-may-90 (billc)
**		Major rewrite.  Convert to SQL.
**	19-aug-91 (billc)
**		fix 39294: was doing \p\g twice after register command. 
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-sep-2002 (xu$we01)
**	    For some platforms (for example DG/UX) after an ERx constant
**	    string is assigned to a pointer, its element address become 
**	    inaccessible. Trying to assign a new value to its element
**	    causing SIGBUS error. To fix it, assign "*type" directly
**	    to an array element.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */
static void	writereg();

/*{
** Name:	xfregistrations - write statement to create registrations.
**
** Description:
**	This now works for vanilla Ingres and for STAR, since both
**	use the iiregistrations table.
**
** Inputs:
**	type		type of object.  "T" for table,
**			"V" for view and "I" for index.
**
** Outputs:
**	numobjs		pointer to integer, so we can return the number 
**			of registrations found.
**
**	Returns:
**		none.
**
**	Exceptions:
**		none.
**
** Side Effects:
**	none.
**
** History:
**	29-sep-88 (marian) written.
**	21-apr-89 (marian)
**		Add is_distrib so the range variable can be set correctly
**		for STAR.
**	03-may-89 (marian)
**		Don't unload INDEXes for STAR.
**	27-may-89 (marian)
**		Change is_distrib to a global with_distrib
*/

void
xfregistrations(type, numobjs)
EXEC SQL BEGIN DECLARE SECTION;
char  	*type;
EXEC SQL END DECLARE SECTION;
i4	*numobjs;
{
EXEC SQL BEGIN DECLARE SECTION;
    char    text_segment[XF_REGLINE + 1];
    char    name[DB_MAXNAME + 1];
    char    owner[DB_MAXNAME + 1];
    char    dml[9];
    i4	    text_sequence;
EXEC SQL END DECLARE SECTION;
    char    printcode[4];
    auto TXT_HANDLE	    *tfd = NULL;
    i4	    regcount = 0;

    EXEC SQL SELECT r.object_name, r.object_owner, r.text_segment,
			r.text_sequence, r.object_dml
	    INTO :name, :owner, :text_segment, :text_sequence, :dml
	    FROM iiregistrations r
	    WHERE ( r.object_owner = :Owner  OR '' = :Owner )
	        AND r.object_type = :type 
	    ORDER BY r.object_owner, r.object_name, r.text_sequence;
    EXEC SQL BEGIN;
    {
	xfread_id(name);

	/* Are we interested in this object? */
	if (!xfselected(name))
	    continue;

	if (tfd == NULL)
	{
	    /* Write a comment saying that we're doing registrations now. */
	    xfwritehdr(REGISTRATIONS_COMMENT);
	    tfd = xfreopen(Xf_in, TH_IS_BUFFERED);
	}

	xfread_id(owner);
	if (text_sequence == 1)
	{
	    /* We have a new registration. */

	    if (regcount > 0)
		xfflush(tfd);

	    regcount++;

	    /* Does user id have to be reset? */
	    xfsetauth(tfd, owner);

	    /* set the language that the registration is written in */
	    xfsetlang(tfd, (dml[0] == 'Q' ? DB_QUEL : DB_SQL));
	}
	xfwrite(tfd, text_segment);
    }
    EXEC SQL END;

    if (tfd != NULL)
        xfclose(tfd);

    if (numobjs != NULL)
	*numobjs += regcount;
    if (regcount > 0)
    {
	printcode[0] = *type;
	printcode[1] = '*';
	printcode[2] = '\00';
	xf_found_msg(printcode, regcount);
    }
}

