/*
**      Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <st.h>
# include       <er.h>
# include       <si.h>
# include       <lo.h>
# include       <fe.h>
# include       <ug.h>
# include       <adf.h>
# include       <afe.h>
EXEC SQL INCLUDE <xf.sh>;
# include       "erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:        xfrules.sc - write statements to handle rules, events,
**                      and some miscellany.
**
** Description:
**      This file defines:
**
**      xfrules         write statement to create rules.
**      xfevents        write statement to create events and associated
**                      GRANTs.
**	xfsequences	write statement to create sequences.
**	xfidcolumns	write statements to manage identity sequences
**      xfsynonyms      write stmt to create synonyms.
**
** History:
**      16-may-89 (marian)
**              written to support terminator RULES.
**      05-mar-1990 (mgw)
**              Changed #include <erxf.h> to #include "erxf.h" since this is
**              a local file and some platforms need to make the destinction.
**      04-may-90 (billc)
**              Major rewrite.  Convert to SQL.
**      04-mar-91 (billc)
**              Add support for events.
**      14-aug-91 (billc)
**              fix 39229: forgot to change "event" to "dbevent" for LRC change.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**      25-oct-1994 (sarjo01) Bug 63542
**              xfsynonyms(): synonym name was being output in single
**              quotes (?). Changed code to use xfwrite_id() to
**              see if correct delimiters are needed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-jan-03 (inkdo01)
**	    Add support for sequences.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**      26-Aug-2010 (horda03) b124304
**          Need to consider the version of Ingres connected to. Ingres prior
**          to Release 9.3 did not have an "unordered_flag" nor "ident_flag"
**          in the iisequences_view.
**      31-Aug-2010 (horda03) b124304
**          When I converted the EXEC SQL used to insert rows in the temp. table for
**          sequences, I failed to change a '%' to '%%' because in a STprintf the
**          '%' denotes a format specifier - need '%%' to get a '%' in the query.
**/

/* # define's */
/* GLOBALDEF's */
GLOBALREF bool With_sequences;
GLOBALREF i4   Objcount;
GLOBALREF bool db_replicated;
GLOBALREF PTR  Obj_list;

/* extern's */

/* static's */

/*{
** Name:        xfrules - write statement to create rules.
**
** Description:
**      Write CREATE RULE statements for all rules.
**
** Inputs:
**
** Outputs:
**
**      Returns:
**
** History:
**      16-may-89 (marian) written.
*/

static void write_rule(
    char        *rule_name,
    char        *rule_owner,
    char        *text_segment,
    i4          text_sequence,
    TXT_HANDLE  **tfd,
    i4          *rcount);

void
xfrules()
{
EXEC SQL BEGIN DECLARE SECTION;
    char    text_segment[XF_RULELINE + 1];
    char    rule_name[DB_MAXNAME + 1];
    char    rule_owner[DB_MAXNAME + 1];
    i4      text_sequence;
EXEC SQL END DECLARE SECTION;
    i4     rcount = 0;
    TXT_HANDLE    *tfd = NULL;

    if (With_65_catalogs)
    {
        EXEC SQL SELECT r.rule_name, r.rule_owner,
                        r.text_segment, r.text_sequence
            INTO :rule_name, :rule_owner, :text_segment, :text_sequence
            FROM iirules r
            WHERE (r.rule_owner = :Owner OR '' = :Owner)
                AND r.system_use <> 'G'                         -- 6.5 only
            ORDER BY r.rule_owner, r.rule_name, r.text_sequence;
        EXEC SQL BEGIN;
        {
            write_rule(rule_name, rule_owner, text_segment, text_sequence,
                                    &tfd, &rcount);
        }
        EXEC SQL END;
    }
    else
    {
        EXEC SQL SELECT r.rule_name, r.rule_owner,
                        r.text_segment, r.text_sequence
            INTO :rule_name, :rule_owner, :text_segment, :text_sequence
            FROM iirules r
            WHERE (r.rule_owner = :Owner OR '' = :Owner)
            ORDER BY r.rule_owner, r.rule_name, r.text_sequence;
        EXEC SQL BEGIN;
        {
            write_rule(rule_name, rule_owner, text_segment, text_sequence,
                                    &tfd, &rcount);
        }
        EXEC SQL END;
    }

    if (tfd != NULL)
        xfclose(tfd);

    xf_found_msg(ERx("R"), rcount);
}

static void
write_rule( char        *rule_name,
    char        *rule_owner,
    char        *text_segment,
    i4          text_sequence,
    TXT_HANDLE  **tfd,
    i4          *rcount)
{
    if (*tfd == NULL)
    {
        /*
        ** First time in loop.  Write a comment indicating that
        ** we're writing rules now.
        */

        xfwritehdr(RULES_COMMENT);

        *tfd = xfreopen(Xf_in, TH_IS_BUFFERED);

        /* Set language for rules (rules are supported only in sql). */
        xfsetlang(*tfd, DB_SQL);
    }

    (void) STtrmwhite(rule_name);
    xfread_id(rule_owner);

    if (text_sequence == 1)
    {
        /* We have a new rule. */

        if (*rcount > 0)
            xfflush(*tfd);

        /* Does user id have to be reset? */
        xfsetauth(*tfd, rule_owner);
        (*rcount)++;
    }
    xfwrite(*tfd, text_segment);
}
/*{
** Name:        xfevents - write statements to create events and their
**                      associated GRANTs.
**
** Description:
**      Write CREATE DBEVENT statements for all events, interleaved with
**      GRANT statements.
**
** Inputs:
**      output_flags
**
** Outputs:
**
**      Returns:
**              none.
**
**      Exceptions:
**              none.
**
** Side Effects:
**      none.
**
** History:
**      04-mar-91 (billc) written.
**	9-Sep-2004 (schka24)
**	    permit-only output for upgradedb.
*/

static void write_dbevent(
    char        *e_name,
    char        *e_owner,
    char        *permit_grantor,
    char        *p_text,
    i2          permit_number,
    i4          text_sequence,
    TXT_HANDLE  **tfd,
    i4          *ecount,
    i4		output_flags);

void
xfevents(i4 output_flags, i4 output_flags2)
{
EXEC SQL BEGIN DECLARE SECTION;
    char        e_name[DB_MAXNAME + 1];
    char        e_owner[DB_MAXNAME + 1];
    char        permit_grantor[DB_MAXNAME + 1];
    char        p_text[XF_PERMITLINE + 1];
    i4          text_sequence;
    i2          permit_number;
    i2          permit_depth;
    char        stmtbuf1[2024];
    char        stmtbuf2[1024];
EXEC SQL END DECLARE SECTION;

    i4          ecount = 0;
    TXT_HANDLE  *tfd = NULL;

    if (With_65_catalogs)
    {
        /* permit_grantor, permit_depth debut in 6.5 */

        STprintf(stmtbuf1, "SELECT e.event_owner, e.event_name, '', -1, 0, 0, \
'' FROM iievents e WHERE (e.event_owner = '%s' OR '' = '%s') ", Owner, Owner);

        STprintf(stmtbuf2, "UNION SELECT p.object_owner, p.object_name, \
p.permit_grantor, p.permit_depth, \
p.permit_number, p.text_sequence, p.text_segment FROM iipermits p \
WHERE (p.object_owner = '%s' OR '' = '%s') AND p.object_type = 'E' \
ORDER BY 1, 2, 4, 5, 6", Owner, Owner);

        if(output_flags2 & XF_NOREP && db_replicated)
            STcat(stmtbuf1, "AND (e.event_name NOT LIKE 'dd_%server%') AND (e.event_name NOT IN (SELECT dbevent FROM dd_events)) ");

        STcat(stmtbuf1, stmtbuf2);

        EXEC SQL EXECUTE IMMEDIATE :stmtbuf1 INTO
            :e_owner, :e_name, :permit_grantor, :permit_depth,
            :permit_number, :text_sequence, :p_text;
        EXEC SQL BEGIN;
        {
            write_dbevent(e_name, e_owner, permit_grantor, p_text,
                        permit_number, text_sequence, &tfd, &ecount,
			output_flags);
        }
        EXEC SQL END;
    }
    else
    {
        /* 6.4 and earlier catalogs. */

        STprintf(stmtbuf1, "SELECT e.event_owner, e.event_name, '', 0, 0, '' \
FROM iievents e WHERE (e.event_owner = '%s' OR '' = '%s') ", Owner, Owner);

        STprintf(stmtbuf2, "UNION SELECT p.object_owner, p.object_name, \
p.object_owner, -- object owner is the grantor before 6.5 \
p.permit_number, p.text_sequence, p.text_segment FROM iipermits p \
WHERE (p.object_owner = '%s' OR '' = '%s') AND p.object_type = 'E' \
ORDER BY 1, 2, 4, 5", Owner, Owner);

        if(output_flags2 & XF_NOREP && db_replicated)
            STcat(stmtbuf1, "AND (e.event_name NOT LIKE 'dd_%server%') AND (e.event_name NOT IN (SELECT dbevent FROM dd_events)) ");

        STcat(stmtbuf1, stmtbuf2);

        EXEC SQL EXECUTE IMMEDIATE :stmtbuf1 INTO
            :e_owner, :e_name, :permit_grantor, 
            :permit_number, :text_sequence, :p_text;
        EXEC SQL BEGIN;
        {
            write_dbevent(e_name, e_owner, permit_grantor, p_text,
                        permit_number, text_sequence, &tfd, &ecount,
			output_flags);
        }
        EXEC SQL END;
    }

    if (tfd != NULL)
        xfclose(tfd);

    xf_found_msg(ERx("E"), ecount);
}

static void
write_dbevent(
    char        *e_name,
    char        *e_owner,
    char        *permit_grantor,
    char        *p_text,
    i2          permit_number,
    i4          text_sequence,
    TXT_HANDLE  **tfd,
    i4          *ecount,
    i4		output_flags)
{
    if (*tfd == NULL)
    {
        /* First time in loop.  Write an informative comment.  */
        xfwritehdr(EVENTS_COMMENT);

        *tfd = xfreopen(Xf_in, TH_IS_BUFFERED);

        /* set language for events (events only in sql) */
        xfsetlang(*tfd, DB_SQL);
    }

    if (permit_number <= 0)
    {
        /* This is "CREATE DBEVENT" stuff.  No text. */

	/* No print if just permits-only */

	if (!output_flags
	  || (output_flags & XF_EVENT_ONLY)
	  || (output_flags & XF_PRINT_TOTAL) )
	{

	    if (*ecount > 0)
		xfflush(*tfd);

	    /* Does user id have to be reset? */
	    xfread_id(e_owner);
	    xfsetauth(*tfd, e_owner);

	    /* fix 39229: forgot to change "event" to "dbevent" (LRC change) */
	    xfwrite(*tfd, ERx("create dbevent "));
	    xfread_id(e_name);
	    xfwrite_id(*tfd, e_name);
	    (*ecount)++;
	}
    }
    else
    {
        /* This is permission text */

        /* if new permit statement */
        if (text_sequence == 1)
        {
            /* flush the previous statement. */
            xfflush(*tfd);

            /* Does grantor's user id need to be reset? */
            (void) STtrmwhite(permit_grantor);
            xfsetauth(*tfd, permit_grantor);
        }
        xfwrite(*tfd, p_text);
    }
}

/*{
** Name:        xfsequences - write statement to create sequences.
**
** Description:
**      Write CREATE SEQUENCE statements for all sequences.
**
** Inputs:
**
** Outputs:
**
**      Returns:
**
** History:
**      29-jan-03 (inkdo01)
**	    Written.
**	30-july-04 (inkdo01)
**	    Change so copy out of sequence sets start value to current 
**	    next value.
**      29-Jan-2008 (hanal04) Bug 119827
**          The fix for Bug 117574 reintroduced Bug 114150. If we were
**          given a table list on the command line we should only dump
**          sequences that are required to create the specified tables.
**      19-Jan-2008 (hanal04) Bug 119945
**          E_AD2093 error when column_default_val contains float values
**          not associated with sequences. Exclused non-sequence values by
**          checking for "next value for%". 
**	15-june-2008 (dougi)
**	    Add support for unordered (random) sequences.
**	12-aug-2008 (dougi)
**	    Add support for 64-bit integer sequences.
**	14-nov-2008 (dougi)
**	    Don't copy sequences defined with identity columns and drop all
**	    options for unordered sequences.
**      26-Aug-2010 (horda03) b124304
**          If connected to an Ingres version prior to 9.3, then "Unordered_flag"
**          will not exist.
**      31-Aug-2010 (horda03) b124304
**          Correct use of '%' in the INSERT query, now that STprintf is being used.
*/

void
xfsequences()
{
EXEC SQL BEGIN DECLARE SECTION;
    char	seqname[DB_MAXNAME + 1];
    char	seqowner[DB_MAXNAME + 1];
    char	seqtype[8];
    int		seqlen, seqprec, seqcache;
    char	seqstart[31], seqincr[31], seqmin[31], seqmax[31];
    char	seqstartf[2], seqincrf[2], seqminf[2], seqmaxf[2];
    char	seqrestartf[2], seqcachef[2], seqcyclef[2], seqorderf[2];
    char	sequnord[2];
    char        *obj_name;
    char        tmp_tbl_cmd [1024];
EXEC SQL END DECLARE SECTION;

    char	buf[256];
    i4		scount = 0;
    i4		seqtlen;
    bool	first = TRUE;
    TXT_HANDLE	*tfd = NULL;
    DB_STATUS   status;
    char        *entry_key;


    if (With_sequences && With_20_catalogs)
    {
        STprintf(tmp_tbl_cmd, 
            "DECLARE GLOBAL TEMPORARY TABLE "
            "session.seq_list AS SELECT seq_name, seq_owner, data_type, "
                "seq_length, seq_precision, trim(char(next_value)) as next_value, "
                "trim(char(increment_value)) as increment_value,  "
                "trim(char(min_value)) as min_value, "
                "trim(char(max_value)) as max_value, cache_size, start_flag, "
                "incr_flag, min_flag, max_flag, restart_flag, "
                "cache_flag, cycle_flag, order_flag, %s "
            "FROM iisequences "
            "WHERE (1 = 0) "
            "ON COMMIT PRESERVE ROWS WITH NORECOVERY",
            With_r930_catalogs ? "unordered_flag" : "'N' as unordered_flag");

        EXEC SQL EXECUTE IMMEDIATE :tmp_tbl_cmd;

        if (Objcount && Obj_list)
        {
            status = IIUGhsHtabScan(Obj_list, FALSE, &entry_key, &obj_name);

            while(status)
            {
                STprintf(tmp_tbl_cmd, 
                    "INSERT into session.seq_list SELECT seq_name, "
                        "seq_owner, data_type, seq_length, seq_precision, "
                        "trim(char(next_value)), trim(char(increment_value)), "
                        "trim(char(min_value)), trim(char(max_value)), cache_size, "
                        "start_flag, incr_flag, min_flag, max_flag, restart_flag, "
                        "cache_flag, cycle_flag, order_flag, %s "
                    "FROM iisequences, iicolumns "
                    "WHERE ( (seq_owner = '%s' OR '' = '%s') AND "
                            "(table_name = '%s') AND "
                            "(column_default_val like 'next value for%%') AND "
                            "(seq_name = substring (column_default_val from "
                                        "( locate(column_default_val, '.') + 2 ) "
                                        "for ( length(column_default_val) - "
                                        "locate(column_default_val, '.') - 2 ) ) ) )",
                    With_r930_catalogs ? "unordered_flag" : "'N'",
                    Owner, Owner, obj_name);

                EXEC SQL EXECUTE IMMEDIATE :tmp_tbl_cmd;

                status = IIUGhsHtabScan(Obj_list, TRUE, &entry_key, &obj_name);
            }
        }
        else
        {
            STprintf(tmp_tbl_cmd, 
                "INSERT into session.seq_list SELECT seq_name, seq_owner,  "
                        "data_type, seq_length, seq_precision,  "
                        "trim(char(next_value)), trim(char(increment_value)),  "
                        "trim(char(min_value)), trim(char(max_value)), cache_size,  "
                        "start_flag, incr_flag, min_flag, max_flag, restart_flag, "
                        "cache_flag, cycle_flag, order_flag, %s "
                    "FROM iisequences "
                    "WHERE (seq_owner = '%s' OR '' = '%s') %s",
                 With_r930_catalogs ? "unordered_flag" : "'N'",
                 Owner, Owner,
                 With_r930_catalogs ? "AND (ident_flag <> 'Y')" : "");

            EXEC SQL EXECUTE IMMEDIATE :tmp_tbl_cmd;
        }

        /* SELECT DISTINCT as Objlist may have generated duplicates */
	EXEC SQL REPEATED SELECT DISTINCT seq_name, seq_owner, data_type, 
		seq_length, seq_precision, next_value, 
		increment_value, min_value, 
		max_value, cache_size, start_flag, 
		incr_flag, min_flag, max_flag, restart_flag, 
		cache_flag, cycle_flag, order_flag, unordered_flag
	    INTO :seqname, :seqowner, :seqtype, :seqlen, :seqprec, 
		:seqstart, :seqincr, :seqmin, :seqmax, :seqcache, 
		:seqstartf, :seqincrf, :seqminf, :seqmaxf, :seqrestartf, 
		:seqcachef, :seqcyclef, :seqorderf, :sequnord
	    FROM session.seq_list
            ORDER BY seq_owner, seq_name;
        EXEC SQL BEGIN;
        {
	    if (first)
	    {
		first = FALSE;
		xfwritehdr(SEQUENCES_COMMENT);
        	tfd = xfreopen(Xf_in, TH_IS_BUFFERED);

        	/* Set language for sequences (sequences are supported 
		** only in sql). */
        	xfsetlang(tfd, DB_SQL);
	    }

            /* Write the bits and pieces of the sequence definition. */
	    xfread_id(seqname);
	    xfread_id(seqowner);

	    if (scount > 0)
		xfflush(tfd);

            /* Does user id have to be reset? */
            xfsetauth(tfd, seqowner);
            scount++;
	    xfwrite(tfd, ERx("create sequence "));
	    xfwrite_id(tfd, seqname);
	
	    /* Explicitly write data type. */
	    if (seqtype[0] == 'i')
	    {
		if (seqlen == 4)
		    STprintf(buf, " as integer\n");
		else STprintf(buf, " as integer8\n");
	    }
	    else STprintf(buf, " as decimal(%d)\n", seqprec);
	    xfwrite(tfd, buf);
	    /* If UNORDERED, only "start with" option can be used. */
	    if (sequnord[0] == 'Y')
	    {
		STprintf(buf, "    start with %s unordered\n", seqstart);
		goto seqEnd;
	    }
	    /* Start and increment values. */
	    STprintf(buf, "    start with %s increment by %s\n",
		seqstart, seqincr);
	    xfwrite(tfd, buf);
	    /* Min and max values. */
	    STprintf(buf, "    minvalue %s maxvalue %s\n",
		seqmin, seqmax);
	    xfwrite(tfd, buf);
	    /* Cache (or nocache) and the other flags. */
	    STprintf(buf, "    ");
	    if (seqcachef[0] == 'N')
		STprintf(&buf[4], "no cache ");
	    else STprintf(&buf[4], "cache %d ", seqcache);
	    seqtlen = STlength(buf);
	    if (seqcyclef[0] != ' ')
		STprintf(&buf[seqtlen-1], " %s cycle ",
		    (seqcyclef[0] == 'N') ? ERx("no") : ERx(" "));
	    seqtlen = STlength(buf);
	    if (seqorderf[0] != ' ')
		STprintf(&buf[seqtlen-1], " %s order ",
		    (seqorderf[0] == 'N') ? ERx("no") : ERx(" "));
	seqEnd:
	    xfwrite(tfd, buf);
        }
        EXEC SQL END;

        EXEC SQL DROP session.seq_list;
    }

    if (tfd != NULL)
        xfclose(tfd);

    xf_found_msg(ERx("Q"), scount);
}

/*{
** Name:        xfidcolumns - write statements to deal with identity
**		columns.
**
** Description:
**      Write DECLARE GLOBAL TEMP & matching copy ... into to OUT script
**	to create ALTER SEQUENCE statements that set correct restart values
**	for any identity columns, then write "/i" for file in IN script.
**
** Inputs:
**      none.
**
** Outputs:
**
**      Returns:
**              none.
**
**      Exceptions:
**              none.
**
** Side Effects:
**      none.
**
** History:
**      16-nov-2008 (dougi)
**	    Written for identity column support.
**	26-mar-2009 (gupsh01)
**	    Added set session authorization before each identity column
**	    sequence so reloading after unloaddb does not fail.
*/

void
xfidcolumns(XF_TABINFO *tlistp)
{
    char	seqbuf[80];
    char	*orconn;
    XF_TABINFO	*tabp;

    xfwritehdr(IDENTITY_COMMENT);

    /* Simply write 2 statements to "copy.out" script and 1 to "copy.in". */

    xfwrite(Xf_out, "DECLARE GLOBAL TEMPORARY TABLE session.identsequences as\n");
    xfwrite(Xf_out, "    SELECT 'set session authorization \"' ||  TRIM(TRAILING FROM seq_owner) || '\"; ' ||\n");
    xfwrite(Xf_out, "    'alter sequence \"' || TRIM(TRAILING FROM seq_owner)\n");
    xfwrite(Xf_out, "        || '\".\"' || TRIM(TRAILING FROM seq_name) ||\n");
    xfwrite(Xf_out, "        '\" restart with ' || TRIM(TRAILING FROM CHAR(next_value, 20)) ||\n");
    xfwrite(Xf_out, "        TRIM(TRAILING FROM case WHEN incr_flag = 'Y' THEN ' increment by ' ||\n");
    xfwrite(Xf_out, "		CHAR(increment_value, 20) ELSE ' ' END) ||\n");
    xfwrite(Xf_out, "        TRIM(TRAILING FROM case WHEN min_flag = 'Y' THEN ' minvalue ' ||\n");
    xfwrite(Xf_out, " 		CHAR(min_value, 20) ELSE ' ' END) ||\n");
    xfwrite(Xf_out, "        TRIM(TRAILING FROM case WHEN max_flag = 'Y' THEN ' maxvalue ' ||\n");
    xfwrite(Xf_out, " 		CHAR(max_value, 20) ELSE ' ' END) ||\n");
    xfwrite(Xf_out, "        TRIM(TRAILING FROM case WHEN cache_flag = 'Y' THEN ' cache ' ||\n");
    xfwrite(Xf_out, " 		CHAR(cache_size, 20) WHEN cache_flag = 'N' THEN ' nocache'\n");
    xfwrite(Xf_out, " 		ELSE ' ' END) ||\n");
    xfwrite(Xf_out, "        TRIM(TRAILING FROM case WHEN cycle_flag = 'Y' THEN ' cycle '\n");
    xfwrite(Xf_out, "		ELSE ' ' END) ||\n");
    xfwrite(Xf_out, "        TRIM(TRAILING FROM case WHEN seql_flag = 'Y' THEN ' sequential '\n");
    xfwrite(Xf_out, "		ELSE ' ' END) ||\n");

    /* Note: Identity columns added in Ingres 9.3, so OK to use unordered_flag here. This function will
    **       not be called is COPYDB/UNLOADDB an earlier Ingres release.
    */
    xfwrite(Xf_out, "        TRIM(TRAILING FROM case WHEN unordered_flag = 'Y' THEN ' unordered '\n");
    xfwrite(Xf_out, "		ELSE ' ' END) || ';\\p\\g'\n");
       
    xfwrite(Xf_out, "    FROM iisequences \n");

    /* Loop over tables looking for identity sequences. For each, paste
    ** "seq_owner = 'abc' and seq_name = 'xyz' into the WHERE clause. */
    orconn = "WHERE";
    for (tabp = tlistp; tabp; tabp = tabp->tab_next)
    {
	if (tabp->idseq_owner[0] == 0x0)
	    continue;

	/* Paste together a line and write it. */
        STprintf(seqbuf, ERx("%s seq_owner = '%s' and seq_name = '%s'\n"),
		orconn, tabp->idseq_owner, tabp->idseq_name);
	xfwrite(Xf_out, seqbuf);
	orconn = "OR";			/* rest are OR'ed */
    }
    xfwrite(Xf_out, " ON COMMIT PRESERVE ROWS WITH NORECOVERY\n");
    xfwrite(Xf_out, "\\p\\g\n");
    xfwrite(Xf_out, "COPY session.identsequences (col1 = c0nl) INTO 'copy.idseqs.in'\n");
    xfwrite(Xf_out, "\\p\\g\n");

    xfwrite(Xf_in, "\\i copy.idseqs.in\n");
}


/*{
** Name:        xfsynonyms - write statements to create synonyms.
**
** Description:
**      Write CREATE SYNONYM statements for all synonyms
**
** Inputs:
**      none.
**
** Outputs:
**
**      Returns:
**              none.
**
**      Exceptions:
**              none.
**
** Side Effects:
**      none.
**
** History:
**      04-mar-91 (billc) written.
**      25-oct-94 (sarjo01) Bug 63542
**                removed use of single quotes in create synonym stmt;
**                use xfwrite_id() to handle delimited identifiers.
**	16-nov-2000 (gupsh01)
**	    Added the synonym_name to the order by sequence, in order to
**	    list the synonyms alphabetically.
*/

void
xfsynonyms()
{
EXEC SQL BEGIN DECLARE SECTION;
    char        s_name[DB_MAXNAME + 1];
    char        s_owner[DB_MAXNAME + 1];
    char        t_name[DB_MAXNAME + 1];
    char        t_owner[DB_MAXNAME + 1];
EXEC SQL END DECLARE SECTION;
    i4          scount = 0;
    char        tmpbuf[1028];

    EXEC SQL SELECT s.synonym_owner, s.synonym_name,
                s.table_owner, s.table_name
            INTO :s_owner, :s_name, :t_owner, :t_name
            FROM iisynonyms s
            WHERE (s.synonym_owner = :Owner OR '' = :Owner)
            ORDER BY s.synonym_owner, s.synonym_name;
    EXEC SQL BEGIN;
    {
        if (scount == 0)
        {
            /* First time in loop.  Write an informative comment.  */
            xfwritehdr(SYNONYMS_COMMENT);

            /* set language for synonyms (synonyms only in sql) */
            xfsetlang(Xf_in, DB_SQL);
        }

        xfread_id(s_name);
        xfread_id(s_owner);
        xfread_id(t_name);
        xfread_id(t_owner);

        /* Does user id have to be reset? */
        xfsetauth(Xf_in, s_owner);
/*
        STprintf(tmpbuf, ERx("create synonym %s for '%s'.%s%s"),
                s_name, t_owner, t_name, GO_STMT);
        xfwrite(Xf_in, tmpbuf);
*/
        xfwrite(Xf_in, "create synonym ");
        xfwrite_id(Xf_in, s_name);
        xfwrite(Xf_in, " for ");
        xfwrite_id(Xf_in, t_owner);
        xfwrite(Xf_in, ".");
        xfwrite_id(Xf_in, t_name);
        xfwrite(Xf_in, GO_STMT);

        scount++;
    }
    EXEC SQL END;

    if (scount > 0)
        xf_found_msg(ERx("S"), scount);
}

