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
# include	<adf.h>
# include	<afe.h>
# include	<ug.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:	xfproc.sc - write statement to create procedure.
**
** Description:
**	This file defines:
**
**	xfprocedures		write statements to create procedures.
**
** History:
**	16-aug-88 (marian) 
**		written to support stored procedures.
**	28-sep-88 (marian)
**		Retrieve the creation date so you can sort by it and 
**		guarantee procedures will be recreated in the correct
**		order.
**	03-oct-88 (marian)
**		Use proc_count to determine if any rows were returned instead
**		of inquire_ingres.
**	11-oct-88 (marian)
**		Took out message about number of rows retrieved and put
**		it into xfwrscrp.c.
**	21-apr-89 (marian)
**		Moved STcopy lower so it will be done the first time through.
**	02-oct-89 (marian)
**		Only retrieve db procedures that do not begin with ii or _ii.
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**	04-may-90 (billc)
**		Major rewrite.  Convert to SQL.
**	07/10/90 (dkh) - Removed include of cf.h so we don't need to
**			 pull in copyformlib when linking unloaddb/copydb.
**	25-jul-90 (marian)
**		Add to WHERE clause procedure_name != '_ii_sequence_value'
**		because createdb creates this procedure owned by the dba
**		and reload.ing produces errors when it attempts to create
**		the procedure again.  THIS SHOULD BE REMOVED ONCE WE FIGURE
**		OUT WHAT CREATEDB SHOULD REALLY BE DOING.  
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**	03-feb-1995 (wonst02) Bug #63766
**		For fips, look for both $ingres and $INGRES
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-jun-2002 (hanch04)
**	    keep track of the number of permissions statements printed,
**	    if one is printed before a procedure, we must print the go stmt.
**      10-Dec-2008 (hanal04) Bug 116958
**          Follow-up change to escape literal % in calls to STprintf().
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**      05-jan-2010 (frima01)
**          Use xfwrite_id instead of xfwrite for procedure names to
**          preserve white spaces.
**/
/* # define's */
/* GLOBALDEF's */
GLOBALREF bool db_replicated;

/* extern's */

/* static's */
static void writeproc(
	TXT_HANDLE	*tfd,
	char	*permit_grantor,
	i2	permit_number,
	char	*p_owner,
	char	*p_name,
	i4	text_sequence,
	i4	*proc_count,
	i4	*perm_count,
	char	*text_segment
	);

static void writestub(
	TXT_HANDLE	**tfd,
	char	*p_owner,
	char	*p_name
	);

/*{
** Name:	xfprocedures - write statements to create procedures.
**
** Description:
**	xfprocedures writes out CREATE PROCEDURE statements for all
**	procedures.  xfprocedures also writes out GRANT EXECUTE
**	statements interleaved with CREATE PROCEDURES.
**
** Inputs:
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**		none.
**
** Side Effects:
**	none.
**
** History:
**	16-aug-88 (marian) written.
**	28-sep-88 (marian)
**		Retrieve the creation date so you can sort by it and 
**		guarantee procedures will be recreated in the correct
**		order.
**	03-oct-88 (marian)
**		Use proc_count to determine if any rows were returned instead
**		of inquire_ingres.
**	23-mar-90 (marian)	bug 20667
**		Remove trim() from retrieve of iiprocedures.txt.  The trim
**		was causing necessary blanks to be removed which caused
**		syntax errors to occur during the reload.
**	10-jun-92 (billc)
**		remove date stuff: it doesn't work because orphan procedures
**		are allowed.  Go with stubbing scheme instead.
**	20-Sep-2004 (schka24)
**	    Don't exclude a procedure just because it's $ingres (eg
**	    imadb procs).  Look for system-use and ii-procedures instead,
**	    exclude those.
*/

void
xfprocedures(i4 output_flags2)
{
EXEC SQL BEGIN DECLARE SECTION;
    char    text_segment[XF_PROCLINE + 1];
    char    p_name[DB_MAXNAME + 1];
    char    p_owner[DB_MAXNAME + 1];
    char    permit_grantor[DB_MAXNAME + 1];
    i4	    text_sequence;
    i2	    permit_number;
    i2	    permit_depth;
    char    stmtbuf[2048];
    char    stmtbuf2[1048];
EXEC SQL END DECLARE SECTION;
    i4	    proc_count;
    i4	    perm_count;
    TXT_HANDLE	    *tfd = NULL;

    proc_count = 0;
    perm_count = 0;

    /* 
    ** First we create stubs for every procedure.  This is because 
    ** procedures can have circular references, or may call procedures with
    ** create-dates LATER than the caller.  (All this is possible because 
    ** Ingres allows orphan ("abandoned") procedures.)  So the stubs mean
    ** that any EXECUTE PROCEDURE statement in a procedure won't produce an
    ** error at re-creation time.
    **
    ** A good standard catalog that showed dependencies would help us here;
    ** we'd only have to create stubs for procedures that were depended on
    ** by other procedures.
    */

    if (With_65_catalogs)
    {
        STprintf(stmtbuf, "SELECT DISTINCT p.procedure_owner, p.procedure_name \
FROM iiprocedures p WHERE ( p.procedure_owner = '%s' OR '' = '%s' ) \
AND LOWERCASE(p.procedure_name) NOT LIKE 'ii%%' AND p.system_use <> 'G' ", Owner, Owner);
    }
    else
    {
	/* No imadb in 6.4, leave it the old way */
        STprintf(stmtbuf, "SELECT DISTINCT p.procedure_owner, p.procedure_name \
FROM iiprocedures p WHERE ( p.procedure_owner = '%s' OR '' = '%s' ) \
AND p.procedure_owner <> '$ingres' AND p.procedure_owner <> '$INGRES' ", Owner, Owner);
    }

    if(output_flags2 & XF_NOREP && db_replicated)
        STcat(stmtbuf, "AND (TRIM(TRAILING FROM p.procedure_name) NOT IN ( \
SELECT DISTINCT p2.procedure_name FROM iiprocedures p2, dd_support_tables dd \
WHERE TRIM(TRAILING FROM p2.procedure_name) LIKE CONCAT(TRIM(TRAILING FROM \
LEFT (dd.table_name, LENGTH(dd.table_name) - 3)), 'rm_') AND RIGHT \
(TRIM(TRAILING FROM dd.table_name), 3) = 'sha' )) ");

    STcat(stmtbuf, "ORDER BY p.procedure_owner, p.procedure_name");

    EXEC SQL EXECUTE IMMEDIATE :stmtbuf INTO :p_owner, :p_name;
    EXEC SQL BEGIN;
    {
        writestub(&tfd, p_owner, p_name);        
    }
    EXEC SQL END;

    if (tfd != NULL)
    {
	/*
	** We retrieve procedures interleaved with the
	** permissions on each procedure.  For 6.4 we could do the permissions
	** later, but for FIPS namespace, where a user's procedure can call
	** someone else's procedure, the permissions must be issued immediately.
	*/

	if (With_65_catalogs)
	{
	    /* permit grantor added in 6.5 to support WITH GRANT OPTION */
	    /* system_use added in 6.5 for constraints support */

            STprintf(stmtbuf, "SELECT p.procedure_owner, p.procedure_name, '', \
-1, 0, p.text_sequence, p.text_segment FROM iiprocedures p \
WHERE ( p.procedure_owner = '%s' OR '' = '%s' ) \
AND LOWERCASE(p.procedure_name) NOT LIKE 'ii%%' AND p.system_use <> 'G' ", Owner, Owner);

            if(output_flags2 & XF_NOREP && db_replicated)
                STcat(stmtbuf, "AND (TRIM(TRAILING FROM p.procedure_name) NOT \
IN ( SELECT DISTINCT p2.procedure_name FROM iiprocedures p2, dd_support_tables \
dd WHERE TRIM(TRAILING FROM p2.procedure_name) LIKE CONCAT(TRIM(TRAILING FROM \
LEFT (dd.table_name, LENGTH(dd.table_name) - 3)), 'rm_') AND RIGHT \
(TRIM(TRAILING FROM dd.table_name), 3) = 'sha' )) ");

            STprintf(stmtbuf2, "UNION SELECT pe.object_owner, pe.object_name, \
pe.permit_grantor, pe.permit_depth, pe.permit_number, pe.text_sequence, \
pe.text_segment FROM iipermits pe WHERE ( pe.object_owner = '%s' OR \
'' = '%s' ) AND LOWERCASE(pe.object_name) NOT LIKE 'ii%%' \
AND pe.object_type = 'P' ORDER BY 1, 2, 4, 5, 6 ", Owner, Owner);

            STcat(stmtbuf, stmtbuf2);

            EXEC SQL EXECUTE IMMEDIATE :stmtbuf INTO :p_owner, 
                    :p_name, :permit_grantor, 
		    :permit_depth, :permit_number, 
		    :text_sequence, :text_segment;
	    EXEC SQL BEGIN;
	    {
		writeproc(tfd, permit_grantor, permit_number, 
			p_owner, p_name, 
			text_sequence, &proc_count, &perm_count, text_segment);
	    }
	    EXEC SQL END;
	}
	else
	{
	    /* Pre-6.5, the grantor is always the owner. */

            STprintf(stmtbuf, "SELECT p.procedure_owner, p.procedure_name, \
'', 0, p.text_sequence, p.text_segment FROM iiprocedures p \
WHERE ( p.procedure_owner = '%s' OR '' = '%s' ) ", Owner, Owner);

            if(output_flags2 & XF_NOREP && db_replicated)
                STcat(stmtbuf, "AND (TRIM(TRAILING from p.procedure_name) NOT \
IN ( SELECT DISTINCT p2.procedure_name FROM iiprocedures p2, dd_support_tables \
dd WHERE TRIM(TRAILING FROM p2.procedure_name) LIKE CONCAT(TRIM(TRAILING FROM \
LEFT (dd.table_name, LENGTH(dd.table_name) - 3)), 'rm_') AND RIGHT \
(TRIM(TRAILING FROM dd.table_name), 3) = 'sha' )) ");

            STprintf(stmtbuf2, "UNION SELECT pe.object_owner, pe.object_name, \
pe.object_owner, pe.permit_number, pe.text_sequence, pe.text_segment \
FROM iipermits pe WHERE ( pe.object_owner = '%s' OR '' = '%s' ) \
AND pe.object_type = 'P' ORDER BY 1, 2, 4, 5 ", Owner, Owner);

            STcat(stmtbuf, stmtbuf2);

            EXEC SQL EXECUTE IMMEDIATE :stmtbuf INTO :p_owner, :p_name, 
                    :permit_grantor, :permit_number, 
                    :text_sequence, :text_segment;
	    EXEC SQL BEGIN;
	    {
		writeproc(tfd, permit_grantor, permit_number, 
			p_owner, p_name, 
			text_sequence, &proc_count, &perm_count, text_segment);
	    }
	    EXEC SQL END;
	}
    }

    if (tfd != NULL)
	xfclose(tfd);

    /* Print a nice informative message for the user. */
    xf_found_msg(ERx("P"), proc_count);
}

/*{
** Name:	writeproc - write statements to create procedures.
**
** Description:
**	factors out the handling of data retrieved from the DBMS, since
**	we have different queries for different catalog versions.
**
** Inputs:
**	tfd			TXT_HANDLE ptr to write to.
**	permit_grantor		grantor of the permission
**	permit_number		the permission number
**	p_owner			owner of proc
**	p_name			name of proc
**	text_sequence		sequence number of text
**	proc_count		count of procs seen
**	text_segment		text of permit statement
**
** Outputs:
**	proc_count		count of procs seen
**
**	Returns:
**		none.
*/

static void
writeproc(
	TXT_HANDLE	*tfd,
	char	*permit_grantor,
	i2	permit_number,
	char	*p_owner,
	char	*p_name,
	i4	text_sequence,
	i4	*proc_count,
	i4	*perm_count,
	char	*text_segment
	)
{
    (void) STtrmwhite(p_name);
    xfread_id(p_owner);
	    
    if ((STbcompare(p_name, 0, ERx("_ii_sequence_value"), 0, TRUE) == 0)
	&& (STbcompare(p_owner, 0, ERx("$ingres"), 0, TRUE) == 0))
    {
	/* The reason for this test is lost in the mists of prehistory.  */
	return;
    }

    if (permit_number == 0)
    {
	/* This is procedure-creation text. */

	if (text_sequence == 1)
	{
	    /*
	    ** Either we have a new procedure, or it's the first time
	    ** we've been called.
	    */

	    if ((*proc_count != 0) || (*perm_count != 0))
		xfflush(tfd);

	    (*proc_count)++;

	    /* Does user id have to be reset? */
	    xfsetauth(tfd, p_owner);

	    /* 
	    ** Destroy the stub before creating the real thing.
	    ** We also DROP $ingres.ii_* procedures before 
	    ** recreating.
	    */
	    xfwrite(tfd, ERx("drop procedure "));
	    xfwrite_id(tfd, p_name);
	    xfflush(tfd);
	}
    }
    else
    {
	/* This is permission text */

	/* if new permit statement */
	if (text_sequence == 1)
	{
	    (*perm_count)++;
	    /* flush the previous statement. */
	    xfflush(tfd);

	    /* Does permit grantor user id have to be reset? */
	    (void) STtrmwhite(permit_grantor);
	    xfsetauth(tfd, permit_grantor);
	}
    }
    xfwrite(tfd, text_segment);
}

/*{
** Name:	writestub - write statements to create stub procedures.
**
** Description:
**	factors out the handling of data retrieved from the DBMS, since
**	we have different queries for different catalog versions.
**
** Inputs:
**	tfd			pointer to TXT_HANDLE ptr to write to.
**	p_owner			owner of proc
**	p_name			name of proc
**
** Outputs:
**	tfd			opened if needed.
**
**	Returns:
**		none.
*/

static void
writestub(
	TXT_HANDLE	**tfd,
	char	*p_owner,
	char	*p_name
	)
{
    if (*tfd == NULL)
    {
	xfwritehdr(PROCEDURES_COMMENT);

	*tfd = xfreopen(Xf_in, TH_IS_BUFFERED);

	/* set language for procedure (procs are only in sql) */
	xfsetlang(*tfd, DB_SQL);
    }
    else
    {
	xfflush(*tfd);
    }

    (void) STtrmwhite(p_name);
    xfread_id(p_owner);

    /* Does user id have to be reset? */
    xfsetauth(*tfd, p_owner);

    xfwrite(*tfd, ERx("create procedure "));
    xfwrite_id(*tfd, p_name);
    xfwrite(*tfd, ERx(" as begin return; end"));
    xfflush(*tfd);

    xfwrite(*tfd, ERx("grant execute on procedure "));
    xfwrite_id(*tfd, p_name);
    xfwrite(*tfd, ERx(" to public"));
    xfflush(*tfd);
}

