/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<er.h>
# include	<st.h>
# include       <si.h>
# include       <lo.h>
# include       <ug.h>
# include       <ui.h>
# include       <uigdata.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:	xffillobj.sc - fill array of XF_LOGTABLE structures with
**				description of tables and indexes.
**
** Description:
**	This file defines:
**
**	xffilltable	fill list of XF_TABINFO structures with table info.
**	xffillindex	fill list of XF_TABINFO structures with index info.
**
**	xfaddlist	initialize list of user-selected components
**	xfselected	determine whether object name is in the list.
**	xf_putobj	put a descriptor in hashfile for later retrieval.
**	xf_getobj	get a descriptor from the hashfile
**	xfifree		free a list of XF_TABINFO stucts.
**
** History:
**	13-jul-87 (rdesmond) 
**		written.
**	14-sep-87 (rdesmond) 
**		Changed att name "duplicates" to "duplicate_rows" and
**		"location" to "location_name" for DG compatibility.
**	23-oct-87 (rdesmond) 
**		Changed "location" to "location_nam" for HACKFOR50.
**		Set journaled and duplicates correctly.
**	18-aug-88 (marian)
**		Changed retrieve statements to reflect column name changes in
**		Standard Catalogs.
**	18-aug-88 (marian)
**		Took out #ifdef HACKFOR50 since it is no longer needed.
**	04-oct-88 (marian)
**		Added table_subtype to the where clause.
**	11-oct-88 (marian)
**		Added xffillproc().
**	24-mar-89 (marian)
**		Modified code to match updated standard catalog changes.
**		Removed calls to iiingres_tables.
**	28-mar-89 (marian)
**		Add _date() to retrieve on iitables.expire_date to change 
**		the bin time back to a 'date' format which was what expire_date
**		use to return.
**	16-may-89 (marian)
**		Add xffillrule() to support RULES.
**	26-may-89 (marian)
**		Add support for GRANT enhancements.  Unload iirole, iiusergroup,
**		and iidbpriv.
**	07-jul-89 (marian)	bug 6630
**		Modify xffillrel() so the STbcompare is not done if the
**		table name is < 3 char long.  This would cause an AV if the
**		table name is 'i'.
**	22-nov-89 (marian)	bug 8785
**		Add unique to retrieve on iiprocedures.  This was causing an AV
**		when procedures had multiple rows in iiprocedures (iiqrytext).
**		Added unique to retrieve on iirules.
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**	29-mar-1990 (marian)	bug 20967
**		Added a portable parameter to xffillrel() to determine if 
**		"ii_encoded_forms" should be unloaded or not.  If the user 
**		specified -c, the fe catalog "ii_encoded_forms" should not be 
**		unloaded.
**	04-may-90 (billc)
**		Major rewrite, for performance and simplification.  Also,
**		partial conversion to support FIPS namespace.  Convert to SQL.
**	07/10/90 (dkh) - Removed include of cf.h so we don't need to
**			 pull in copyformlib when linking unloaddb/copydb.
**      09-sep-91 (billc)
**              Fix bug 39517 (actually, a cleanup on the side).  Detect
**              multiple locations - avoid unneeded query on iimult_locations
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**      30-Oct-1994 (Bin Li)
**              Fix bug 63319, including gateway tables as part of the tables
**              which need to be created in order to reload the ima tables in
**              UNLOADDB.
**      03-may-1995 (harpa06)
**              Bug #68252 - Rewrote comparison function to work with delimited
**              mixed and non-mixed case table names.
**      13-sep-1995 (kch)
**              Ignore extended tables when retrieving table information.
**		This change fixes bug 71126.
**	13-nov-95 (pchang)
**	    Translate names of user-specified objects in accordance with
**	    database name case in xfaddlist() (B71130).
**	 1-jul-1997 (hayke02)
**	    In the function xffilltable(), we now only get XF_COLINFO lists
**	    with column info (via calls to xfgetcols()) for the tables which
**	    have been specified. This change fixes bug 83350.
**      15-may-1996 (nanpr01 & thaju02)
**              Modified xffilltable to retrieve pagesize from iitables.
**      22-sep-1997 (nanpr01)
**              bug 85565 : cannot unload a 1.2 database over net.
**      23-Jul-1998 (wanya01)
**              In the function xffilltable(), when call xfgetcols, we also 
**              pass table_owner as an argument. This change fixes bug 87905.
**      11-Dec-1998 (hanal04)
**              Retrieve table cache priority values into the XF_TABINFO
**              structures in xffilltable() and xffillindex(). b94494.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	26-Feb-1999 (consi01) Bug 92376 Problem INGDBA 29
**		Changed xffillindex() to pick up info from iirange table for
**		RTREE indexes.
**      21-Mar-2000 (hanal04) Bug 100911 INGDBA 63.
**          	Make Obj_list global so that xfpermits() can use it.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-mar-2001 (somsa01)
**		A prior change accidentally modified a string comparison
**		against "iirole" without checking for the return code from
**		STbcompare(). This allowed ALL core catalogs to pass and
**		be written to the copy.out script.
**      27-apr-2001 (stial01)
**              Fixed queries so table_name which is in order by is also in
**              target list. (This is needed to do copydb against Ingres 2.0
**              databases).
**	21-may-2001 (abbjo03)
**	    In xffillindex(), change ORDER BYs to allow for parallel index
**	    creation. Fix ORDER BY in xffilltable().
**	22-May-2001 (gupsh01)
**	    Added selecting row_width, table_relid and table_relidx 
**	    while selecting from iitables for genxml.
**	23-May-2001 (gupsh01)
**	    The static is removed from functions.
**      19-sep-2001 (stial01)
**          ORDER BY cols must be in target list for copydb against 2.0 db
**	07-Sep-2005 (thaju02) B115166
**	    Added global with_tbl_list.
**       3-Dec-2008 (hanal04) SIR 116958
**          Added -no_rep option to exclude all replicator objects.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**	21-apr-2010 (toumi01) SIR 122403
**	    Add encryption support and With_r1000_catalogs.
[@history_template@]...
**/

/* # define's */
/* GLOBALREF's */
GLOBALREF	i4	Objcount ;
GLOBALREF 	bool	With_20_catalogs;
GLOBALREF	bool	with_tbl_list;
GLOBALREF	bool	with_no_rep;
GLOBALREF	bool	db_replicated;


/* The list of objects specified by the user to COPYDB. */
GLOBALDEF	PTR	Obj_list = NULL;


/* extern's */

/*XF_TABINFO *get_empty_xf_tabinfo();*/
static bool	filltable(XF_TABINFO *tp, i4  *tcount);
static bool	fillindex(XF_TABINFO *ip);

/*{
** Name:	xffilltable - 
**
** Description:
**	Retrieves table information from the database and stores it in
**	a linked list of internal structures.
**
** Inputs:
**	numobjs		ptr to value indicating number of objects to copy.
**
** Outputs:
**
**	Returns:
**		a linked list of XF_TABINFO structs describing all tables
**		to be unloaded/reloaded.
**
** History:
**	09-feb-92 (billc)
**		new.
**	20-nov-1993 (jpk)
**		improved.  Propogate is_readonly info.  Enable
**		allocation_size and extend_size.  Get row estimate.
**	2-sep-93 (robf)
**	  Add support for security info
**      13-sep-1995 (kch)
**              Ignore extended tables when retrieving table information.
**		This change fixes bug 71126.
**	 1-jul-1997 (hayke02)
**	    We now only get XF_COLINFO lists with column info (via calls to
**	    xfgetcols()) for the tables which have been specified. This
**	    change fixes bug 83350.
**	15-may-1996 (nanpr01 & thaju02)
**		Retrieve pagesize from iitables.
**	22-sep-1997 (nanpr01)
**	    bug 85565 : cannot unload a 1.2 database over net.
**	8-apr-98 (inkdo01)
**	    Add "unique_scope" info for tables, too.
**      23-Jul-1998 (wanya01)
**              In the function xffilltable(), when call xfgetcols, we also
**              pass table_owner as an argument. This change fixes bug 87905.
**      11-Dec-1998 (hanal04)
**              Retrieve the cache priority value and update tp->priority
**              where cache priorities are supported. Select 0 into
**              tp->priority for older releases to ensure writecreate()
**              does not add the 'with priority' clause when it is not
**              supported. b94494.
**	16-Nov-2000 (gupsh01)
**	    Added sort order by table_name as well, in order to arrange the 
**	    output in alphabetical order. 
**	21-may-2001 (abbjo03)
**	    Correct the ORDER BY clause so that output is sorted by owner the
**	    way it used to be (ascending alphabetical).
**      22-May-2001 (gupsh01)
**          Added selecting row_width, table_relid and table_relidx 
**          while selecting from iitables for genxml.
**	7-Jun-2004 (schka24)
**	    Added physical-partition info, skip partitions.
**	15-Apr-2005 (thaju02)
**	    copydb performance enhancement: if table list specified, build IN 
**	    clause. Use execute immediate. (B114375)
**	07-sep-2005 (thaju02) B115166
**	    Set with_tbl_list if With_r3_catalogs and objects specified.
*/
XF_TABINFO *
xffilltable(numobjs, output_flags2)
i4		*numobjs;
i4              output_flags2;
{
EXEC SQL BEGIN DECLARE SECTION;
    XF_TABINFO	*tp;
    char	stmtbuf[1024];
    char	*obj_name;
EXEC SQL END DECLARE SECTION;

    XF_TABINFO	*tablist = NULL;
    XF_TABINFO	*tlist;
    i4		tcount = 0;
    DB_STATUS	status;
    char	*entry_key;

    /* get an empty node to fill in. */
    tp = get_empty_xf_tabinfo();

    if (With_r1000_catalogs )
    {
	/* Partitioned tables in r3 and beyond */

	STprintf(stmtbuf, "SELECT t.table_name, t.table_owner, \
t.table_permits, t.all_to_all, t.ret_to_all, t.table_type, \
t.table_integrities, t.is_journalled, t.expire_date, t.multi_locations, \
t.location_name, t.duplicate_rows, t.storage_structure, t.is_compressed, \
t.key_is_compressed, t.unique_rule, t.unique_scope, t.table_ifillpct, \
t.table_dfillpct, t.table_lfillpct, t.table_minpages, t.table_maxpages, \
t.allocation_size, t.extend_size, t.is_readonly, t.num_rows, \
t.row_security_audit, t.table_pagesize, t.table_reltcpri, t.row_width, \
t.table_reltid, t.table_reltidx, t.phys_partitions, t.partition_dimensions, \
t.encrypted_columns, t.encryption_type \
FROM iitables t WHERE t.table_type = 'T' AND t.table_subtype = 'N' \
AND ( t.table_owner = '%s' OR '' = '%s' ) AND t.table_name NOT LIKE 'iietab%%' \
AND t.table_reltidx >= 0", Owner, Owner); 

	if (Objcount && Obj_list)
	{
	    EXEC SQL DECLARE GLOBAL TEMPORARY TABLE 
		session.tbl_list (tabname char(32) not null) 
		ON COMMIT PRESERVE ROWS WITH NORECOVERY;

	    with_tbl_list = TRUE;
            status = IIUGhsHtabScan(Obj_list, FALSE, &entry_key, &obj_name);

            while(status)
            {
		EXEC SQL INSERT into session.tbl_list values (:obj_name);
                status = IIUGhsHtabScan(Obj_list, TRUE, &entry_key, &obj_name);
            }

            /* IN clause */
	    STcat(stmtbuf, " AND t.table_name IN ( SELECT tabname FROM session.tbl_list )");
	}

        if (output_flags2 & XF_NOREP && db_replicated)
        {
	    with_no_rep = TRUE;

            STcat(stmtbuf, " AND (t.table_name NOT IN ( SELECT table_name FROM dd_support_tables ))");
        }

	STcat(stmtbuf, " ORDER BY t.table_owner DESC, t.table_name DESC"); 

	EXEC SQL EXECUTE IMMEDIATE :stmtbuf INTO
                :tp->name, :tp->owner, :tp->has_permit,
                :tp->alltoall, :tp->rettoall, :tp->table_type,
                :tp->has_integ, :tp->journaled,
                :tp->expire_date, :tp->multi_locations, :tp->location,
                :tp->duplicates,
                :tp->storage,
                :tp->is_data_comp, :tp->is_key_comp, :tp->is_unique,
                :tp->unique_scope,
                :tp->ifillpct, :tp->dfillpct,
                :tp->lfillpct, :tp->minpages, :tp->maxpages,
                :tp->allocation_size, :tp->extend_size,
                :tp->is_readonly, :tp->num_rows,
                :tp->row_sec_audit,
                :tp->pagesize, :tp->priority,
                :tp->row_width, :tp->table_reltid, :tp->table_reltidx,
                :tp->phys_partitions, :tp->partition_dimensions,
                :tp->encrypted_columns, :tp->encryption_type;
        EXEC SQL BEGIN;
        {
            if (filltable(tp, &tcount))
            {
                tp->tab_next = tablist;
                tablist = tp;

                /* get a new empty node to fill in. */
                tp = get_empty_xf_tabinfo();
            }
        }
        EXEC SQL END;
    }
    else if (With_r3_catalogs )
    {
	/* Partitioned tables in r3 and beyond */

	STprintf(stmtbuf, "SELECT t.table_name, t.table_owner, \
t.table_permits, t.all_to_all, t.ret_to_all, t.table_type, \
t.table_integrities, t.is_journalled, t.expire_date, t.multi_locations, \
t.location_name, t.duplicate_rows, t.storage_structure, t.is_compressed, \
t.key_is_compressed, t.unique_rule, t.unique_scope, t.table_ifillpct, \
t.table_dfillpct, t.table_lfillpct, t.table_minpages, t.table_maxpages, \
t.allocation_size, t.extend_size, t.is_readonly, t.num_rows, \
t.row_security_audit, t.table_pagesize, t.table_reltcpri, t.row_width, \
t.table_reltid, t.table_reltidx, t.phys_partitions, t.partition_dimensions \
FROM iitables t WHERE t.table_type = 'T' AND t.table_subtype = 'N' \
AND ( t.table_owner = '%s' OR '' = '%s' ) AND t.table_name NOT LIKE 'iietab%%' \
AND t.table_reltidx >= 0", Owner, Owner); 

	if (Objcount && Obj_list)
	{
	    EXEC SQL DECLARE GLOBAL TEMPORARY TABLE 
		session.tbl_list (tabname char(32) not null) 
		ON COMMIT PRESERVE ROWS WITH NORECOVERY;

	    with_tbl_list = TRUE;
            status = IIUGhsHtabScan(Obj_list, FALSE, &entry_key, &obj_name);

            while(status)
            {
		EXEC SQL INSERT into session.tbl_list values (:obj_name);
                status = IIUGhsHtabScan(Obj_list, TRUE, &entry_key, &obj_name);
            }

            /* IN clause */
	    STcat(stmtbuf, " AND t.table_name IN ( SELECT tabname FROM session.tbl_list )");
	}

        if (output_flags2 & XF_NOREP && db_replicated)
        {
	    with_no_rep = TRUE;

            STcat(stmtbuf, " AND (t.table_name NOT IN ( SELECT table_name FROM dd_support_tables ))");
        }

	STcat(stmtbuf, " ORDER BY t.table_owner DESC, t.table_name DESC"); 

	EXEC SQL EXECUTE IMMEDIATE :stmtbuf INTO
                :tp->name, :tp->owner, :tp->has_permit,
                :tp->alltoall, :tp->rettoall, :tp->table_type,
                :tp->has_integ, :tp->journaled,
                :tp->expire_date, :tp->multi_locations, :tp->location,
                :tp->duplicates,
                :tp->storage,
                :tp->is_data_comp, :tp->is_key_comp, :tp->is_unique,
                :tp->unique_scope,
                :tp->ifillpct, :tp->dfillpct,
                :tp->lfillpct, :tp->minpages, :tp->maxpages,
                :tp->allocation_size, :tp->extend_size,
                :tp->is_readonly, :tp->num_rows,
                :tp->row_sec_audit,
                :tp->pagesize, :tp->priority,
                :tp->row_width, :tp->table_reltid, :tp->table_reltidx,
                :tp->phys_partitions, :tp->partition_dimensions;
        EXEC SQL BEGIN;
        {
            if (filltable(tp, &tcount))
            {
                tp->tab_next = tablist;
                tablist = tp;

                /* get a new empty node to fill in. */
                tp = get_empty_xf_tabinfo();
            }
        }
        EXEC SQL END;
    }
    else if (With_20_catalogs)
    {
        /* Column iitables.table_pagesize debut in 2.0 .  */

	EXEC SQL SELECT t.table_name, t.table_owner, t.table_permits, 
		t.all_to_all, t.ret_to_all, t.table_type,
		t.table_integrities, t.is_journalled, 
		t.expire_date, t.multi_locations, t.location_name, 
		t.duplicate_rows, 
		t.storage_structure,
		t.is_compressed, t.key_is_compressed, t.unique_rule,
		t.unique_scope,
		t.table_ifillpct, t.table_dfillpct,
		t.table_lfillpct, t.table_minpages, t.table_maxpages,
		t.allocation_size, t.extend_size,
		t.is_readonly, t.num_rows,
	        t.row_security_audit,
		t.table_pagesize, t.table_reltcpri,
		t.row_width, t.table_reltid, t.table_reltidx
	    INTO
		:tp->name, :tp->owner, :tp->has_permit, 
		:tp->alltoall, :tp->rettoall, :tp->table_type,
		:tp->has_integ, :tp->journaled,
		:tp->expire_date, :tp->multi_locations, :tp->location,
		:tp->duplicates, 
		:tp->storage,
		:tp->is_data_comp, :tp->is_key_comp, :tp->is_unique,
		:tp->unique_scope,
		:tp->ifillpct, :tp->dfillpct,
		:tp->lfillpct, :tp->minpages, :tp->maxpages,
		:tp->allocation_size, :tp->extend_size,
		:tp->is_readonly, :tp->num_rows,
	        :tp->row_sec_audit,
		:tp->pagesize, :tp->priority,
		:tp->row_width, :tp->table_reltid, :tp->table_reltidx
	    FROM iitables t
	    WHERE t.table_type = 'T'
		AND t.table_subtype = 'N'
		AND ( t.table_owner = :Owner
		    OR '' = :Owner  -- an abf trick - per owner or all
		    )
                /* Bug 71126 - ignore extended tables */
		AND t.table_name NOT LIKE 'iietab%'
	    ORDER BY t.table_owner DESC, t.table_name DESC;
	EXEC SQL BEGIN;
	{
	    if (filltable(tp, &tcount))
	    {
		tp->tab_next = tablist;
		tablist = tp;

		/* get a new empty node to fill in. */
		tp = get_empty_xf_tabinfo();
	    }
	}
	EXEC SQL END;
    }
    else if (With_65_catalogs)
    {
        /* Column iitables.allocation_size and extend_size debut in 6.5.  */

	EXEC SQL SELECT t.table_name, t.table_owner, t.table_permits, 
		t.all_to_all, t.ret_to_all, t.table_type,
		t.table_integrities, t.is_journalled, 
		t.expire_date, t.multi_locations, t.location_name, 
		t.duplicate_rows, 
		t.storage_structure,
		t.is_compressed, t.key_is_compressed, t.unique_rule,
		t.unique_scope,
		t.table_ifillpct, t.table_dfillpct,
		t.table_lfillpct, t.table_minpages, t.table_maxpages,
		t.allocation_size, t.extend_size,
		t.is_readonly, t.num_rows,
	        t.row_security_audit,
		2048, 0
	    INTO
		:tp->name, :tp->owner, :tp->has_permit, 
		:tp->alltoall, :tp->rettoall, :tp->table_type,
		:tp->has_integ, :tp->journaled,
		:tp->expire_date, :tp->multi_locations, :tp->location,
		:tp->duplicates, 
		:tp->storage,
		:tp->is_data_comp, :tp->is_key_comp, :tp->is_unique,
		:tp->unique_scope,
		:tp->ifillpct, :tp->dfillpct,
		:tp->lfillpct, :tp->minpages, :tp->maxpages,
		:tp->allocation_size, :tp->extend_size,
		:tp->is_readonly, :tp->num_rows,
	        :tp->row_sec_audit,
		:tp->pagesize, :tp->priority
	    FROM iitables t
	    WHERE t.table_type = 'T'
		AND t.table_subtype = 'N'
		AND ( t.table_owner = :Owner
		    OR '' = :Owner  -- an abf trick - per owner or all
		    )
                /* Bug 71126 - ignore extended tables */
		AND t.table_name NOT LIKE 'iietab%'
	    ORDER BY t.table_owner, t.table_name DESC;
	EXEC SQL BEGIN;
	{
	    if (filltable(tp, &tcount))
	    {
		tp->tab_next = tablist;
		tablist = tp;

		/* get a new empty node to fill in. */
		tp = get_empty_xf_tabinfo();
	    }
	}
	EXEC SQL END;
    }
    else if (With_64_catalogs)
    {
        /* Column iitables.key_is_compressed debuts in 6.4.  */

	EXEC SQL SELECT t.table_name, t.table_owner, 
		t.table_permits, 
		t.all_to_all, t.ret_to_all, t.table_type,
		t.table_integrities, t.is_journalled, 
		t.expire_date, 
		t.multi_locations, t.location_name, 
		t.duplicate_rows, 
		t.storage_structure,
		t.is_compressed, t.key_is_compressed, 
		t.unique_rule, '',
		t.table_ifillpct, t.table_dfillpct,
		t.table_lfillpct, t.table_minpages, 
		t.table_maxpages, -1, -1, 'N', 2048, 0
	    INTO
		:tp->name, :tp->owner, :tp->has_permit, 
		:tp->alltoall, :tp->rettoall, 
		:tp->table_type,
		:tp->has_integ, :tp->journaled, 
		:tp->expire_date,
		:tp->multi_locations, :tp->location, 
		:tp->duplicates, 
		:tp->storage, :tp->is_data_comp,
		:tp->is_key_comp, -- 				new in 6.4
		:tp->is_unique,
		:tp->unique_scope,
		:tp->ifillpct, :tp->dfillpct, :tp->lfillpct, 
		:tp->minpages, :tp->maxpages,
		:tp->allocation_size, :tp->extend_size,
	        :tp->row_sec_audit, :tp->pagesize,
                :tp->priority
	    FROM iitables t
	    WHERE t.table_type = 'T'
		AND t.table_subtype = 'N'
		AND ( t.table_owner = :Owner
		    OR '' = :Owner  -- an abf trick - per owner or all
		    )
	    ORDER BY t.table_owner, t.table_name DESC;
	EXEC SQL BEGIN;
	{
	    if (filltable(tp, &tcount))
	    {
		tp->tab_next = tablist;
		tablist = tp;

		/* get a new empty node to fill in. */
		tp = get_empty_xf_tabinfo();
	    }
	}
	EXEC SQL END;
    }
    else
    {
        /* 6.3/03 or older standard catalogs.  */

	EXEC SQL SELECT t.table_name, t.table_owner, t.table_permits, 
		t.all_to_all, t.ret_to_all, t.table_type,
		t.table_integrities, t.is_journalled, t.expire_date, 
		t.multi_locations, t.location_name, 
		t.duplicate_rows, t.storage_structure,
		t.is_compressed, t.unique_rule, '',
		t.table_ifillpct, t.table_dfillpct,
		t.table_lfillpct, t.table_minpages, t.table_maxpages, 
		 -1, -1, 'N', 2048, 0
	    INTO
		:tp->name, :tp->owner, :tp->has_permit, 
		:tp->alltoall, :tp->rettoall, :tp->table_type,
		:tp->has_integ, :tp->journaled, :tp->expire_date,
		:tp->multi_locations, :tp->location, :tp->duplicates, 
		:tp->storage, :tp->is_data_comp,
		:tp->is_unique,
		:tp->unique_scope,
		:tp->ifillpct, :tp->dfillpct, :tp->lfillpct, :tp->minpages,
		:tp->maxpages,
		:tp->allocation_size, :tp->extend_size,
	        :tp->row_sec_audit, :tp->pagesize,
		:tp->priority
	    FROM iitables t
	    WHERE t.table_type = 'T'
		AND t.table_subtype = 'N'
		AND ( t.table_owner = :Owner
		    OR '' = :Owner	-- an abf trick - per owner or all
		    )
	    ORDER BY t.table_owner, t.table_name DESC;
	EXEC SQL BEGIN;
	{
	    if (filltable(tp, &tcount))
	    {
		tp->tab_next = tablist;
		tablist = tp;

		/* get a new empty node to fill in. */
		tp = get_empty_xf_tabinfo();
	    }
	}
	EXEC SQL END;
    }

    if (tablist != NULL)
    {
	/* get XF_COLINFO lists with column info for specified tables. */
	for (tlist = tablist; tlist; tlist = tlist->tab_next)
	{
	    char	*seqvalp = NULL, *namep;

	    xfgetcols(tlist->name, tlist->owner, &seqvalp);
	    
	    if (seqvalp)
	    {
		/* Hack identity sequence name/owner from "default"
		** string for later building of "alter sequence" statement. */
		namep = &tlist->idseq_owner[0];
		seqvalp = &seqvalp[16];		/* address the name */
		while ((*namep = *seqvalp) != '"')	/* copy owner */
		    namep++, seqvalp++;
		*namep = 0x0;			/* terminate owner */

		namep = &tlist->idseq_name[0];
		seqvalp = &seqvalp[3];		/* skip over ..."."... */
		while ((*namep = *seqvalp) != '"')	/* copy seqname */
		    namep++, seqvalp++;
		*namep = 0x0;			/* terminate seqname */
	    }
	    else
	    {
		tlist->idseq_name[0] = 0x0;
		tlist->idseq_owner[0] = 0x0;
	    }
	}
    }

    /* Print a message for the user. */
    xf_found_msg(ERx("T"), tcount);
    *numobjs += tcount;

    return (tablist);
}

/*{
** Name:	filltable - factor out the fill-in stuff.
**
** Description:
**     This routine does the real work of getting the catalog information
**     into our descriptor struct.
**
** Inputs:
**	tp		ptr to XF_TABINFO containing information read
**			directly from the standard catalogs.
**
** Outputs:
**	tp		filltable massages the struct.
**	tcount		count of tables seen.
**
**	Returns:
**		TRUE - we are interested in this table and filled in the
**			struct.
**		FALSE - this table is not interesting to us.
*/

static bool
filltable(XF_TABINFO *tp, i4	*tcount)
{
    /* 
    ** Was this item listed in copydb?  If the list exists and this item 
    ** is not on it, ignore this item. 
    */
    xfread_id(tp->name);
    if (!xfselected(tp->name))
	return (FALSE);
    xfread_id(tp->owner);


    /* Ingres object?  If so, ignore (usually) unless it's an FE catalog. */
    if ((STbcompare(tp->owner, 0, INGRES_NAME, 0, TRUE) == 0) 
	&& xf_is_cat(tp->name))
    {
	/* 
	** This item is a BE or FE catalog.  
	*/
	   /* Fix bug 63319, if the tables are gateway tables, we need to 
	   create them in order to reload the ima tables.   */
	if (xf_is_fecat(tp->name) ||
	    (STbcompare(tp->name, 0, ERx("iigw07_relation"), 0, TRUE ) == 0)
	   || (STbcompare(tp->name, 0, ERx("iigw07_attribute"), 0, TRUE ) == 0)
	      || (STbcompare(tp->name, 0, ERx("iigw07_index"), 0, TRUE) == 0))
	{
	    /* FE catalog.  mark it. */
	    tp->fecat = TRUE;
	}
	else if (( STbcompare(tp->name, 0, ERx("iirole"), 0, TRUE) == 0 )
	  || ( STbcompare(tp->name, 0, ERx("iiuser"), 0, TRUE) == 0 )
	  || ( STbcompare(tp->name, 0, ERx("iiusers"), 0, TRUE) == 0 )
	  || ( STbcompare(tp->name, 0, ERx("iiusergroup"), 0, TRUE) == 0 )
	  || ( STbcompare(tp->name, 0, ERx("iiprofile"), 0, TRUE) == 0 )
	  || ( STbcompare(tp->name, 0, ERx("iidbpriv"), 0, TRUE) == 0 ))
	{
	    /* BE catalog, special case for GRANT enhancements. */
	    tp->becat = TRUE;
	}
	else
	{
	    /* BE catalog.  not interested. */
	    return (FALSE);
	}
    }

    (void) STtrmwhite(tp->location);
    (void) STtrmwhite(tp->storage);
    CVlower(tp->storage);

    /* 
    ** Add to a hash table.  We'll use the hash table to associate
    ** columns with tables later.
    */
    xf_putobj(tp);

    (*tcount)++;
    return (TRUE);
}

/*{
** Name:	xffillindex - 
**
** Description:
**	Retrieves index information from the database, and builds a linked
**	list of XF_TABINFO structs describing them.
**
** Inputs:
**	numobjs		ptr to value indicating number of objects to copy.
**	basetable	table whose indexes you are interested in.  
**			This value is NULL if you want all indexes in the db.
**
** Outputs:
**
**	Returns:
**		linked list of XF_TABINFO structs describing all the indexes
**		that interest us.
**
** History:
**	09-feb-92 (billc)
**		new.
**	15-may-1996 (thaju02)
**		Modified xffillindex to retrieve page size from iitables.
**	22-sep-1997 (nanpr01)
**	    bug 85565 : cannot unload a 1.2 database over net.
**      11-Dec-1998 (hanal04)
**              Retrieve the cache priority value and update ip->priority
**              where cache priorities are supported. Select 0 into
**              ip->priority for older releases to ensure writeindex()
**              does not add the 'with priority' clause when it is not
**              supported. b94494.
**	26-Feb-1999 (consi01) Bug 92376 Problem INGDBA 29
**		Modified the select loop that retrieves 2.0 index details
**		to use an outer join on table iirange.  This allows the 
**		range qualifier to be rebuilt for RTREE indexes.
**      07-June-2000 (gupsh01)
**              Modified xffillindex to accept a constr flag. If constr is
**              1 then query for indexes without the restriction  t.system_use <> 'G'
**	16-Nov-2000 (gupsh01)
**		added the table_name in the order by sequence, in order to print
**		the index names in alphabetical order.
**	17-may-2001 (abbjo03)
**	    Change ORDER BYs to allow for parallel index creation.
**      15-Apr-2005 (thaju02)
**          copydb performance enhancement: if table list specified, build IN
**          clause. Use execute immediate. (B114375)
**	24-Aug-2009 (wanfr01)
**	    b122319 - only fetch index information for the requested table.
*/
XF_TABINFO *
xffillindex(i4 constr, XF_TABINFO *basetable)
{
EXEC SQL BEGIN DECLARE SECTION;
    XF_TABINFO	*ip;
    short	nullind;
    char	stmtbuf[1024];
EXEC SQL END DECLARE SECTION;
    char	clausebuf[1024];

    XF_TABINFO	*indlist = NULL;
    XF_TABINFO	*tp;

    /* get an empty node to fill in. */
    ip = get_empty_xf_tabinfo();

    if (With_20_catalogs)
    {
        /* 
        ** Columns iiindexes.unique_scope, iiindexes.persistent new in 6.5
        */

	STprintf(stmtbuf, ERx("SELECT t.table_owner, i.index_name, \
i.base_name, i.base_owner, t.table_indexes, t.storage_structure, \
t.is_compressed, t.key_is_compressed, t.unique_rule, i.unique_scope, \
i.persistent, t.table_ifillpct, t.table_dfillpct, t.table_lfillpct, \
t.table_minpages, t.table_maxpages, t.location_name, t.multi_locations, \
t.table_pagesize, t.table_reltcpri, r.rng_ll1, r.rng_ll2, r.rng_ll3, \
r.rng_ll4, r.rng_ur1, r.rng_ur2, r.rng_ur3, r.rng_ur4 \
FROM ((iitables t JOIN iiindexes i ON i.index_name=t.table_name) \
left join iirange r ON t.table_reltid=r.rng_baseid AND \
t.table_reltidx=rng_indexid) \
WHERE i.index_owner = t.table_owner AND t.table_type = 'I' \
AND (i.index_owner = '%s' OR '' = '%s')"), Owner, Owner);

	if (!constr)
	    STcat(stmtbuf, " AND t.system_use <> 'G'"); 

	/* Fetch only specified table if requested */
	if (basetable)
	{
	    STprintf(clausebuf, " AND i.base_name = '%s' and i.base_owner = '%s' ",basetable->name,basetable->owner);
	    STcat(stmtbuf, clausebuf);
	}

	/* if user specified table list, build IN clause */
	if (with_tbl_list)
	    STcat(stmtbuf, " AND i.base_name IN ( SELECT tabname from session.tbl_list )");

        /* if user specified no replicator, build NOT I clause */
        if (with_no_rep)
            STcat(stmtbuf, " AND (i.index_name NOT IN ( SELECT relid FROM iirelation r WHERE reltidx > 0 AND r.reltid IN (SELECT r2.reltid FROM iirelation r2, dd_support_tables dd WHERE r2.relid = dd.table_name) ))");

	STcat(stmtbuf, " ORDER BY base_owner DESC, base_name DESC, index_name DESC");

	EXEC SQL EXECUTE IMMEDIATE :stmtbuf INTO
                :ip->owner, :ip->name, :ip->base_name, :ip->base_owner,
                :ip->indexes, :ip->storage, :ip->is_data_comp,
                :ip->is_key_comp, -- doesn't appear in pre-6.4 catalog
                :ip->is_unique,
                :ip->unique_scope, :ip->persistent,
                :ip->ifillpct, :ip->dfillpct, :ip->lfillpct, :ip->minpages,
                :ip->maxpages, :ip->location, :ip->otherlocs, :ip->pagesize,
                :ip->priority,
                :ip->rng_ll1:nullind, :ip->rng_ll2:nullind,
                :ip->rng_ll3:nullind, :ip->rng_ll4:nullind,
                :ip->rng_ur1:nullind, :ip->rng_ur2:nullind,
                :ip->rng_ur3:nullind, :ip->rng_ur4:nullind;
	EXEC SQL BEGIN;
	{
	    if (fillindex(ip))
	    {
		if ( (!constr) || (constr && (ip->name[0] != '$')) )
		{
		    ip->tab_next = indlist;
		    indlist = ip;

		    /* get a new empty node to fill in. */
		    ip = get_empty_xf_tabinfo();
		}
	    }
	}
	EXEC SQL END;
    }
    else if (With_65_catalogs)
    {
        /* 
        ** Columns iiindexes.unique_scope, iiindexes.persistent new in 6.5
        */
	if (!constr)
	{
	EXEC SQL SELECT 
		t.table_owner, t.table_name, i.base_name, i.base_owner, 
		t.table_indexes, t.storage_structure,
		t.is_compressed, t.key_is_compressed, t.unique_rule,
		i.unique_scope, i.persistent, 		-- new in 6.5
		t.table_ifillpct, t.table_dfillpct, 
		t.table_lfillpct, t.table_minpages, t.table_maxpages,
		t.location_name, t.multi_locations, 2048, 0
	    INTO
		:ip->owner, :ip->name, :ip->base_name, :ip->base_owner,
		:ip->indexes, :ip->storage, :ip->is_data_comp, 
		:ip->is_key_comp, -- doesn't appear in pre-6.4 catalog
		:ip->is_unique,
		:ip->unique_scope, :ip->persistent,
		:ip->ifillpct, :ip->dfillpct, :ip->lfillpct, :ip->minpages, 
		:ip->maxpages, :ip->location, :ip->otherlocs, :ip->pagesize,
                :ip->priority
	    FROM iitables t, iiindexes i
	    WHERE 
		i.index_name = t.table_name
		AND i.index_owner = t.table_owner
		AND t.table_type = 'I'
		AND (i.index_owner = :Owner OR '' = :Owner)
		AND t.system_use <> 'G'  -- new in 6.5
	    ORDER BY t.table_owner, t.table_name DESC;
	EXEC SQL BEGIN;
	{
	    if (fillindex(ip))
	    {
		ip->tab_next = indlist;
		indlist = ip;

		/* get a new empty node to fill in. */
		ip = get_empty_xf_tabinfo();
	    }
	}
	EXEC SQL END;
	}
	else
	{
        EXEC SQL SELECT
                t.table_owner, t.table_name, i.base_name, i.base_owner,
                t.table_indexes, t.storage_structure,
                t.is_compressed, t.key_is_compressed, t.unique_rule,
                i.unique_scope, i.persistent,           -- new in 6.5
                t.table_ifillpct, t.table_dfillpct,
                t.table_lfillpct, t.table_minpages, t.table_maxpages,
                t.location_name, t.multi_locations, 2048, 0
            INTO
                :ip->owner, :ip->name, :ip->base_name, :ip->base_owner,
                :ip->indexes, :ip->storage, :ip->is_data_comp,
                :ip->is_key_comp, -- doesn't appear in pre-6.4 catalog
                :ip->is_unique,
                :ip->unique_scope, :ip->persistent,
                :ip->ifillpct, :ip->dfillpct, :ip->lfillpct, :ip->minpages,
                :ip->maxpages, :ip->location, :ip->otherlocs, :ip->pagesize,
                :ip->priority
            FROM iitables t, iiindexes i
            WHERE
                i.index_name = t.table_name
                AND i.index_owner = t.table_owner
                AND t.table_type = 'I'
                AND (i.index_owner = :Owner OR '' = :Owner)
            ORDER BY t.table_owner, table_name DESC;
        EXEC SQL BEGIN;
        {
            if (fillindex(ip) &&
                (ip->name[0] != '$'))
            {
                ip->tab_next = indlist;
                indlist = ip;

                /* get a new empty node to fill in. */
                ip = get_empty_xf_tabinfo();
            }
        }
        EXEC SQL END;
	}
    }
    else if (With_64_catalogs)
    {
        /* Column iitables.key_is_compressed new in 6.4.  */

	EXEC SQL SELECT 
		t.table_owner, i.index_name, i.base_name, i.base_owner, 
		t.table_indexes, t.storage_structure,
		t.is_compressed, t.key_is_compressed, t.unique_rule,
		'', '', 
		t.table_ifillpct, t.table_dfillpct, 
		t.table_lfillpct, t.table_minpages, t.table_maxpages,
		t.location_name, t.multi_locations, 2048, 0
	    INTO
		:ip->owner, :ip->name, :ip->base_name, :ip->base_owner,
		:ip->indexes, :ip->storage, :ip->is_data_comp, 
		:ip->is_key_comp, -- doesn't appear in pre-6.4 catalog
		:ip->is_unique,
		:ip->unique_scope, :ip->persistent,
		:ip->ifillpct, :ip->dfillpct, :ip->lfillpct, :ip->minpages, 
		:ip->maxpages, :ip->location, :ip->otherlocs, :ip->pagesize,
		:ip->priority
	    FROM iitables t, iiindexes i
	    WHERE 
		i.index_name = t.table_name
		AND i.index_owner = t.table_owner
		AND t.table_type = 'I'
		AND (i.index_owner = :Owner OR '' = :Owner)
	    ORDER BY t.table_owner DESC;
	EXEC SQL BEGIN;
	{
	    if (fillindex(ip))
	    {
		ip->tab_next = indlist;
		indlist = ip;

		/* get a new empty node to fill in. */
		ip = get_empty_xf_tabinfo();
	    }
	}
	EXEC SQL END;
    }
    else
    {
        /* 6.3/03 or older standard catalogs.
        ** Column iitables.key_is_compressed does not exist.
        */
	EXEC SQL SELECT 
		t.table_owner, i.index_name, i.base_name, i.base_owner, 
		t.table_indexes, t.storage_structure,
		t.is_compressed, t.unique_rule,
		'', '',
		t.table_ifillpct, t.table_dfillpct, 
		t.table_lfillpct, t.table_minpages, t.table_maxpages,
		t.location_name, t.multi_locations, 2048, 0
	    INTO
		:ip->owner, :ip->name, :ip->base_name, :ip->base_owner,
		:ip->indexes, :ip->storage, :ip->is_data_comp, 
		:ip->is_unique,
		:ip->unique_scope, :ip->persistent,
		:ip->ifillpct, :ip->dfillpct, :ip->lfillpct, :ip->minpages, 
		:ip->maxpages, :ip->location, :ip->otherlocs, :ip->pagesize,
		:ip->priority
	    FROM iitables t, iiindexes i
	    WHERE 
		i.index_name = t.table_name
		AND i.index_owner = t.table_owner
		AND (i.index_owner = :Owner OR '' = :Owner)
	    ORDER BY t.table_owner DESC;
	EXEC SQL BEGIN;
	{
	    if (fillindex(ip))
	    {
		ip->tab_next = indlist;
		indlist = ip;

		/* get a new empty node to fill in. */
		ip = get_empty_xf_tabinfo();
	    }
	}
	EXEC SQL END;
    }

    if (indlist != NULL)
    {
	/* Now read column information about the indexes we found. */
	/* reuse stmtbuf */
	xfgeticols(basetable);
    }

    return (indlist);
}

/*{
** Name:	fillindex - factor out the fill-in stuff.
**
** Description:
**     This routine does the real work of getting the catalog information
**     about an index into our descriptor struct.
**
** Inputs:
**	tp		ptr to XF_TABINFO containing information read
**			directly from the standard catalogs.
**
** Outputs:
**	tp		fillindex massages the struct.
**
**	Returns:
**		TRUE - we are interested in this index and filled in the
**			struct.
**		FALSE - this index is not interesting to us.
*/

static bool
fillindex(XF_TABINFO *ip)
{
    /* 
    ** Was the base table listed in copydb?  If the list exists and this item 
    ** is not on it, ignore this item. 
    */
    xfread_id(ip->base_name);
    if (!xfselected(ip->base_name))
	return (FALSE);
    xfread_id(ip->owner);

    /* Ingres object?  */
    if ((STbcompare(ip->owner, 0, INGRES_NAME, 0, TRUE) == 0)
	&& xf_is_cat(ip->base_name))
    {
	/* 
	** This item indexes a catalog.  
	*/
	if (xf_is_fecat(ip->name))
	{
	    /* FE catalog.  mark it. */
	    ip->fecat = TRUE;
	}
	else
	{
	    /* BE catalog.  not interested. */
	    return (FALSE);
	}
    }

    xfread_id(ip->name);
    (void) STtrmwhite(ip->location);
    (void) STtrmwhite(ip->storage);
    CVlower(ip->storage);

    /* 
    ** Add to hash table.  We'll need to retrieve this later when we 
    ** read in the info about the columns of the index.
    */
    xf_putobj(ip);

    return (TRUE);
}

/*{
** Name:	xfselected - determine whether object should be unloaded.
**
** Description:
**	Given a name determine if the object should be unloaded.
**	If no objects were specified in xfaddlist, then always return TRUE.
**
** Inputs:
**	name		name to check for.
**
**	Returns:
**		TRUE	unload the object.
**		FALSE	don't unload the object.
**
**	Exceptions:
**		none.
**
** History:
**	04-may-90 (billc) changed to hashed lookup.  made global.
**	08-feb-92 (billc) changed to use IIUG hash routines.
*/
bool
xfselected(char *name)
{
    auto PTR	dat;

    /* if no list given, then all items are "found" */
    if (Owner[0] == EOS 
	|| Obj_list == NULL 
	|| IIUGhfHtabFind(Obj_list, name, &dat) == OK)
    {
	return (TRUE);
    }
    return (FALSE);
}

/*{
** Name:	xfaddlist - add names to the object list.
**
** Description:
**	Add names to a list of user-specified objects to unload.  THese
**	names will be hashed for later lookup by xfselected;
**
**	This hashing business is probably overkill.  Most users will
**	only specify a few objects for unloading, so a sequential scan
**	of a list would probably do the trick just fine.
**
** Inputs:
**	name		name of object to unload.
**
**	Returns:
**		none.
**
** History:
**	04-may-90 (billc) Written.
**	08-feb-92 (billc) changed to use IIUG routines.
**	13-nov-95 (pchang)
**	    Translate names of user-specified objects in accordance with
**	    database name case (B71130).
**      24-Jul-2001 (hanal04) Bug 101617 INGSRV 1191
**          Restore delimiters for call to IIUGdlm_ChkdlmBEobject()
**          if name has embedded spaces and is not already delimited.
**          Taken from fix for bug 71612.
**      27-Apr-2009 (coomi01) b121994
**          Only count an object once in the hash tab.
*/

void
xfaddlist(char *name)
{
    char	normname[DB_MAXNAME + 1];
    char	*name_ptr, namebuf[(DB_MAXNAME + 3)];
    auto PTR	dat;

    if (name == NULL)
	return;

    /* 
    ** The user gave us this name, so we have to check for a delimited
    ** identifier.  Internally we handle all names in "normalized" form,
    ** since that's how they're stored in the DBMS.
    */
    
    name_ptr = name;
    if (*name_ptr != '"' && STindex(name_ptr, " ", 0) != NULL)
    {
        STpolycat(3, "\"", name_ptr, "\"", namebuf);
        name_ptr = namebuf;
    }
    if (IIUGdlm_ChkdlmBEobject(name_ptr, normname, FALSE))
    {
	/* 
	** Must save the normalized name, since the hashing stuff 
	** just points to our data.
	*/
	name = STalloc(normname);
    }
    else
    {
	if (IIUIdbdata()->name_Case == UI_UPPERCASE)
	    CVupper(name);
	else
	    CVlower(name);
    }

    if (Obj_list == NULL)
    {
	/* 
	** Initialize hash table.  Since we don't pass a pointer to an
	** 'allocfail' routine (a routine to be called on memory allocation
	** failure) IIUGhiHtabInit will never return unless it's successful.
	** Likewise, IIUGheHtabEnter will succeed or abort.
	**
	** 1st arg is table size.  Buckets chain, so hash table doesn't need to
	** be very big.
	*/
	(void) IIUGhiHtabInit(64, (i4 (*)())NULL, (i4 (*)())NULL, 
				(i4 (*)())NULL, &Obj_list);
    }

    /* 
    ** See if we have met this object before
    */
    if ( OK != IIUGhfHtabFind(Obj_list, name,  &dat) )
    {
	/* 
	** No - then add to list and increment count
	*/
	(void) IIUGheHtabEnter(Obj_list, name, (PTR) name);
	Objcount++;
    }
}

/*
** The following code is to stash and retrieve information about objects
** whose descriptions have already been read from the DBMS.  This code
** could be merged with xfaddlist and xfselected, but there's some tricky
** issues: COPYDB doesn't have a username until after we get the object list.
** UNLOADDB must use the ownernames from the DBMS.
**
** Our policy for reading object information is that we hash the names of
** all tables that interest us.  Then we read information about ALL of the
** columns in the database (if we're called by UNLOADDB) or all the columns
** in tables owned by the user (if we're called by COPYDB).  Then, for each
** column name we fetch the table descriptions from the hash table.
** This way, we get all column names with a single SELECT, rather than one
** SELECT per table.  Yes, it's wasteful when the COPYDB user specifies a
** single table but owns many, but I want to optimize for the UNLOADDB case.
**
** (If UD/CD went to a dynamic SQL scheme, one could goober up an IN (x, y, z) 
** clause for all the named objects.  Yes, dynamic SQL is tempting...)
*/

static	PTR	Info_tab = NULL;

/*{
** Name:	xf_putobj - put a table definition into the hash table.
**
** Description:
**	Note the comments above.  We hash a complete description of
**	a table.  We hash by owner name and table name.  Later, we'll
**	retrieve the description when we're getting column definitions
**	and other table information.
**
**	The stored owner/name pair may or may not have full table
**	information (an XF_TABINFO struct).  Lookup can pass an
**	XF_TABINFO pointer to the found name and attach it there.
**	That way, we can store just the name and later add the info.
**
** Inputs:
**	info		the XF_TABINFO describing the table.
**
**	Returns:
**		none.
**
**	Exceptions:
**		none.
**
** History:
**	08-feb-92 (billc) written.
**      03-may-95 (harpa06)
**	        Bug #68252 - Rewrote comparison function to work with delimited
**              mixed and non-mixed case table names. 
**	07-may-95 (harpa06)
**		Bug #69409 - Fixed the comparison function since it did not
**		accurately compare 2 closely-related strings.
*/

/* comparison function to pass to FE hash routines. */
static i4
tp_compare(char	*s1,char *s2)
{
    XF_TABINFO	*t1 = (XF_TABINFO *) s1;
    XF_TABINFO	*t2 = (XF_TABINFO *) s2;
    i4  x;
    i4  y;

    x=STbcompare(t1->name,0,t2->name,0,FALSE);
    y=STbcompare(t1->owner,0,t2->owner,0,FALSE);

    return (x | y);
}

/* hash function to pass to FE hash routines. */
static i4
tp_hash(char *ts, i4  size)
{
    register i4	rem = 0;
    register char	*s;
    XF_TABINFO	*tp = (XF_TABINFO *) ts;

    for (s = tp->name; *s != EOS; s++)
	rem = (rem * 128) + *s;
    for (s = tp->owner; *s != EOS; s++)
	rem = (rem * 128) + *s;

    if (rem < 0)
	rem = -rem;
    return ((i4) rem % size);
}

void 
xf_putobj(XF_TABINFO *info)
{
    XF_TABINFO 	*tmp;

    if (Info_tab == NULL)
    {
	/* 
	** Initialize hash table.  Since we don't pass a pointer to an
	** 'allocfail' routine (a routine to be called on memory allocation
	** failure) IIUGhiHtabInit will never return unless it's successful.
	** Likewise, IIUGheHtabEnter will succeed or abort.
	*/
	(void) IIUGhiHtabInit(XF_MAXOBJONCL, 
		(i4 (*)())NULL, tp_compare, tp_hash, &Info_tab);
    }

    /* Is this table info already in the hash table? */
    if (IIUGhfHtabFind(Info_tab, (char *) info, (PTR *) &tmp) != OK)
    {
	/*
	** yes, the key and data point to the same thing.  No problem,
	** since the hash routines just store pointers.
	*/
	(void) IIUGheHtabEnter(Info_tab, (char *) info, (PTR) info);
    }
}

/*{
** Name:	xf_getobj - get a table definition from the hash table.
**
** Description:
**	Note the comments above.  Using owner and table name, get the
**	description of the table.
**
** Inputs:
**	owner		name of table owner.
**	table		name of table.
**
**	Returns:
**		XF_TABINFO pointer if found.
**		NULL otherwise.
**
**	Exceptions:
**		none.
**
** History:
**	08-feb-92 (billc) written.
*/

XF_TABINFO *
xf_getobj(char *owner, char *table)
{
    XF_TABINFO	*tmp;
    XF_TABINFO	dummy;

    STlcopy(owner, dummy.owner, sizeof(dummy.owner) - 1);
    STlcopy(table, dummy.name, sizeof(dummy.name) - 1);

    if (Info_tab != NULL 
	&& IIUGhfHtabFind(Info_tab, (char *) &dummy, (PTR *) &tmp) == OK)
    {
	return (tmp);
    }
    return (NULL);
}

/*{
** Name:	get_empty_xf_tabinfo - return empty XF_TABINFO node.
**
** Description:
**	Return an empty XF_TABINFO node, allocating more if the freelist is
**	exhausted. We always allocate XFR_BLOCK nodes at a time, to avoid
**	excessive memory allocation calls.  The node memory is never freed.
**
** Inputs:
**	none.
**
** Returns:
**	{XF_TABINFO *}  the new node.
*/

/* The freelist */
static XF_TABINFO *FreeList = NULL;
# define XFR_BLOCK	20

XF_TABINFO *
get_empty_xf_tabinfo()
{
    XF_TABINFO *tmp;

    if (FreeList == NULL)
    {
	i4 i;

	FreeList = (XF_TABINFO *) XF_REQMEM(sizeof(*tmp) * XFR_BLOCK, FALSE);
	if (FreeList == NULL)
        {
	    IIUGerr(E_XF0060_Out_of_memory, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}
        /* set up the 'next' pointers. */
        for (i = 0; i < (XFR_BLOCK - 1); i++)
        {
             FreeList[i].name[0] = EOS;
             FreeList[i].tab_next = &FreeList[i+1];
        }
	FreeList[XFR_BLOCK - 1].tab_next = NULL;
    }
    tmp = FreeList;
    FreeList = FreeList->tab_next;

    MEfill((u_i2) sizeof(*tmp), (unsigned char) 0, (PTR) tmp);

    return (tmp);
}

/*{
** Name:	xfifree - free a list of XF_TABINFO nodes.
**
** Description:
**	Put a whole list of XF_TABINFO nodes on the freelist, making them
**	available for re-use.  Also free the hash table.
**
** Inputs:
**	list		{XF_TABINFO *}  the list to free.
**
** Returns:
**	none.
*/

void
xfifree(XF_TABINFO *list)
{
    XF_TABINFO         *rp;

    if (list == NULL)
        return;

    for (rp = list; rp->tab_next != NULL; rp = rp->tab_next)
	xfattfree(rp->column_list);

    rp->tab_next = FreeList;
    FreeList = list;

    if (Info_tab != NULL)
    {
	(void) IIUGhrHtabRelease(Info_tab);
	Info_tab = NULL;
    }
}

/* Name: update_tablist - updates the table list
**
**      output_flags2   verify if we are calling this routine
**                      for convtouni.
** History:
**
**      28-Apr-2004 (gupsh01)
**          Created.
**	18-Apr-2004 (gupsh01)
**	    Fixed initialization of the tablelist
**	    pointer. (bug 112335).
*/
void
update_tablist(tablist, output_flags2)
XF_TABINFO      **tablist;
i4              output_flags2;
{
    XF_TABINFO         *tp;
    XF_TABINFO         *rp;
    XF_TABINFO         *prev = NULL;

    /* only valid for convtouni mode */
    if (!(output_flags2 & XF_CONVTOUNI) ||
	!(tablist && *tablist))
      return;

    tp = *tablist;
    while (tp)
    {
        if (xfremovetab(tp))
        {
          rp = tp->tab_next;

          if (prev == NULL)
          /* remove First element from list */
            *tablist = (*tablist)->tab_next;
          else
            prev->tab_next = tp->tab_next;

          /* Free the column list */
          xfattfree((PTR)tp->column_list);

          /* add the tp to free list */
          tp->tab_next = (XF_TABINFO *)FreeList;
          FreeList = tp;

          /* adjust the walking pointer */
          if (prev == NULL)
           tp = *tablist;
          else
           tp = prev;
        }
        else
	{
          prev = tp;
	  tp = tp->tab_next;
	}
    }
}
