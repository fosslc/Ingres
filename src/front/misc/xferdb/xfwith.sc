/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<er.h>
# include	<me.h>
# include	<st.h>
# include       <si.h>
# include       <lo.h>
# include	<fe.h>
# include       <ug.h>
# include       <adf.h>
# include       <afe.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:	xfwith.sc - Find the with clause information for the
**				create (table) statement.
**
** Description:
**	This file defines:
**
**	xfaddloc	Find all the locations of a given table.  Retrieve
**			the information from iimulti_locations.
**
** History:
**	23-sep-88 (marian) 	bug 3512
**		Written to handle multi-locations for tables.
**	26-sep-88 (marian)
**		Add support for STAR with clause additions.
**	18-apr-89 (marian)
**		Removed xfstartwith() routine.  This routine is no longer
**		required because of the modified STAR requirements.  STAR
**		will now create a 'register' statement when the user supplies
**		a create with a 'with' clause.  This information will then
**		be unloaded when the registered objects are unloadded.
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**      09-sep-91 (billc)
**              Fix bug 39517 (actually, a cleanup on the side).  xfaddloc
**              now called by different clients, must differentiate.  (hack!)
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**      06-jun-00 (gupsh01)
**              Added parameter buf to xfaddloc in order to write to buffer if
**              the buffer is provided else write to XF_in.
**      15-Oct-2001 (hanal04) Bug 105924 INGSRV 1566
**         Modified the above change to xfaddloc in order to provide
**         dynamic allocation of the buffer.
**	26-May-2002 (hanje04) 
**	   Fix for bug 105924 requires me.h to be included in order to properly
**	   relsolve MEreqmem.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */
GLOBALREF bool With_multi_locations;

/* static's */

/*{
** Name:	xfaddloc - Add the additional locations for a table.
**
** Description:
**	Retrieves location information from iimulti_locations if it
**	exists.  If iimulti_locations does not exist, return.
**
** Inputs:
**	name		Name of the curent table.
**	owner		Owner of the current table.
**	alloc		TRUE if a buffer is to be allocated and output 
**                      is to be written to that buffer, FALSE for writing 
**                      to XF_in.
**      buf_ptr		The address of a character ptr that will receive
**                      the address of the allocated memory if alloc is TRUE.
**
** Outputs:
**      buf_ptr		Filled with the address of the allocated character
**                      buffer if alloc is TRUE.
**
**	Returns:
**		none.
**
**	Exceptions:
**		none.
**
** Side Effects:
**	If alloc is TRUE xfaddloc() will allocate a character buffer.
**      The caller must deallocate this memory when it has finished
**      using the allocated buffer.
**
** History:
**	23-sep-88 (marian)
**		Written.
**	04-may-90 (billc) convert to SQL.
**      06-jun-00 (gupsh01)
**              Added parameter buf in order to write to buffer if
**              the buffer is provided else write to XF_in.
**      15-Oct-2001 (hanal04) Bug 105924 INGSRV 1566
**         The above change provide 240 characters in which to hold
**         a comma separated list of the location names for an index.
**         With a maximum of 256 locations per index this was not adequate.
**         New/Modified parameters shown above.
*/

void
xfaddloc(name, owner, alloc, buf_ptr)
EXEC SQL BEGIN DECLARE SECTION;
char	*name;
char	*owner;
bool    alloc;
char    **buf_ptr;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
    char	add_loc[DB_MAXNAME + 1];
    i2		loc_seq;
    i4          num_locs;
EXEC SQL END DECLARE SECTION;

    if ( !With_multi_locations)
	return;

    if (alloc)
    {
        /* Determine how many location names exist for the table.
        ** Allocate enough space for the the location name,
        ** comma and space required for each location plus an additional
        ** NULL terminator.
        */
        EXEC SQL SELECT COUNT(*) INTO :num_locs
	FROM iimulti_locations m
	WHERE m.table_name = :name
	    AND m.table_owner = :owner;
        *buf_ptr = (char *) XF_REQMEM((num_locs * (DB_MAXNAME + 2)) + 1, TRUE);
	if (*buf_ptr == NULL)
	{
	    IIUGerr(E_XF0060_Out_of_memory, UG_ERR_FATAL, 0);
	    return;
	}
    }

    /* There may be additional locations for this table */

    EXEC SQL REPEATED SELECT m.location_name, m.loc_sequence
	INTO :add_loc, :loc_seq
	FROM iimulti_locations m 
	WHERE m.table_name = :name
	    AND m.table_owner = :owner
	ORDER BY m.loc_sequence;
    EXEC SQL BEGIN;
    {
	/* add the locations to the list */
	(void) STtrmwhite(add_loc);
       if (alloc)
       {
                STcat(*buf_ptr, ", ");
                STcat(*buf_ptr, add_loc );
        }
	else
	{
		xfwrite(Xf_in, ERx(",\n\t\t"));
		xfwrite(Xf_in, add_loc);
	}
    }
    EXEC SQL END;
}

