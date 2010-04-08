/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<er.h>
# include	<fe.h>
# include	<st.h>
# include       <si.h>
# include       <lo.h>
# include	<ug.h>
# include        <adf.h>
# include        <afe.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:	xfcomm.sc - write statements to create dbms comments.
**
** Description:
**	This file defines:
**
**	xfcomments		write statements to create comment.
**
** History:
**	24-aug-90 (billc) 
**		written.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**      24-nov-1999 (ashco01)
**              Reworked xfcomments() to accept pointer to XF_TABINFO
**              (which describes specific table) for which the 'COMMENT ON'
**              statement is to be generated.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**/

/*{
** Name:	xfcomments - write COMMENT ON statements.
**
** Description:
**	Probably overimplemented, since currently we don't support short 
**	comments.  Long comments are limited to 1600 characters, so 
**	there is only one comment per tuple.  This is a nice simplification.
**
** Inputs:
**      t       pointer to XF_TABINFO describing the table for which
**              to generate the 'COMMENT ON' statement.	
**
** Outputs:
**	writes to the reload scripts.
**
**	Returns:
**		none.
**
** History:
**	24-aug-90 (billc) written.
**      24-nov-1999 (ashco01)
**          Reworked to accept pointer to XF_TABINFO and generate 'COMMENT ON'
**          statment for specific table only. The 'COMMENT ON' statement is now
**          written as part of the TABLES section rather than in a COMMENTS 
**          section of the COPY.IN.
**       1-Apr-2003 (hanal04) Bug 109584 INGSRV 2102
**          No need for TH_BLANK_PAD padding hack now we have rewritten
**          F_CU format in f_colfmt().
**      31-May-2006 (smeke01) Bug 116153
**          Ensured that xfcomments looks for [table- & table-column-] 
**          comments on strictly 1 table by restricting the search 
**          qualification to a named table and a named table-owner.
**          Previously table-owner was optional, since the global
**          variable Owner could be empty, so > 1 table of the same
**          name could be processed by xfcomments, leading to bug
**          116153.
*/

void
xfcomments(t)
XF_TABINFO  *t;
{
EXEC SQL BEGIN DECLARE SECTION;
    char   obj_name[DB_MAXNAME + 1];
    char   sub_obj_name[DB_MAXNAME + 1];
    char   obj_owner[DB_MAXNAME + 1];
    char   short_rem[XF_SHORTREMLINE + 1];
    char   long_rem[XF_LONGREMLINE + 1];
    char   obj_type[2];
    char   *tabinfo_name;
    char   *tabinfo_owner;
EXEC SQL END DECLARE SECTION;

    i4	   com_count = 0;
    char    esc_buf[(XF_LONGREMLINE * 2) + 1];
    TXT_HANDLE    *tfd = NULL;

    /* Determine current table name */
    tabinfo_name = t->name;
    tabinfo_owner = t->owner;

    EXEC SQL REPEATED SELECT c.object_owner, c.object_name, '', 'T',
			c.short_remark, c.long_remark
	INTO :obj_owner, :obj_name, :sub_obj_name,
		    :obj_type, :short_rem, :long_rem
	    FROM iidb_comments c
	    WHERE c.object_owner = :tabinfo_owner
		AND c.object_type = 'T'
                AND c.object_name = :tabinfo_name
	UNION SELECT sc.object_owner, sc.object_name, sc.subobject_name, 'C',
			sc.short_remark, sc.long_remark
	    FROM iidb_subcomments sc
	    WHERE sc.object_owner = :tabinfo_owner
		AND sc.subobject_type = 'C'
                AND sc.object_name = :tabinfo_name;
    EXEC SQL BEGIN;
    {
	xfread_id(obj_name);
	xfread_id(obj_owner);
	if (obj_type[0] == 'C')
	    xfread_id(sub_obj_name);

	if (tfd == NULL)
	{

	    /* Open our text-buffer */
	    tfd = xfreopen(Xf_in, TH_IS_BUFFERED);

	    /* Make sure we're in SQL */
	    xfsetlang(tfd, DB_SQL);
	}

	/* Does user id have to be reset? */
	xfsetauth(tfd, obj_owner);

	xfwrite(tfd, ERx("comment on "));
	if (obj_type[0] == 'T')
	{
	    xfwrite(tfd, ERx("table "));
	    xfwrite_id(tfd, obj_name);
	}
	else
	{
	    xfwrite(tfd, ERx("column "));
	    xfwrite_id(tfd, obj_name);
	    xfwrite(tfd, ERx("."));
	    xfwrite_id(tfd, sub_obj_name);
	}
	xfwrite(tfd, ERx(" is '"));

	xfescape_quotes(long_rem, esc_buf);
        xfwrite(tfd, esc_buf);

	xfwrite(tfd, ERx("'"));

	/* 
	** Short remarks are not yet supported in 6.5, so this stuff may not
	** work.  Short-remarks in the catalogs are still poorly specified.
	*/
	if (STtrmwhite(short_rem) > 0)
	{
	    xfwrite(tfd, ERx("\n\twith short_remark='"));
	    xfescape_quotes(short_rem, esc_buf);
	    xfwrite(tfd, esc_buf);
	    xfwrite(tfd, ERx("'"));
	}
	xfflush(tfd);

	com_count++;
    }
    EXEC SQL END;

    if (tfd != NULL)
	xfclose(tfd);

}

