# include	 <compat.h> 
#include    <gl.h>
# include	 <st.h> 
# include	 <LO.h> 
# include	 <cm.h> 

/*
**	Copyright 1984 Ingres Corporation
**
**	LOdbname - get the database name from the LOCATION of the database directory
**
**	Parameters:
**		dir_loc	- LOCATION of the directory to get the dbname from
**		buf	- place to put the dbname
**
**	Returns:
**		OK	- buf has been filled with the dbname
**		FAIL	- dir_loc can't be a database directory location
**
**	History:
**		03/28/84 (lichtman)	- written
**		08/17/84 (lichtman)	- check for ".DIR" extension was inverted; corrected it
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
*/

# define	DIR_EXT		".DIR"

STATUS
LOdbname(dir_loc, buf)
LOCATION	*dir_loc;
char		*buf;
{
	register char	*p;
	register char	*q;
	register char	*s;
	i4		size;
	i4		ret_val = FAIL;

	s = dir_loc->string;
	*buf = '\0';		/* on a failure the buffer should be set to empty */
	size = STlength(s);

	for (;;)		/* not a loop, just used to break */
	{
		if (size < sizeof(DIR_EXT))	/* there should at least be room for "X.DIR" */
			break;

		p = s + size - sizeof(DIR_EXT) + 1;		/* find the beginning of the ".DIR" extension */
		if (STcompare(p, DIR_EXT))			/* error if no ".DIR" extension */
			break;

		for (q = s; q < p; CMnext(q))				/* copy over the filename in lowercase w/o extension */
		{
			CMtolower(q,buf);
			CMnext(buf);
		}
		*buf = '\0';

		ret_val = OK;
		break;
	}

	return (ret_val);
}
