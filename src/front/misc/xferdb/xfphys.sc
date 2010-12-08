/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>		 
# include	<cv.h>		 
# include	<st.h>
# include       <tmtz.h>
# include	<er.h>
# include       <si.h>
# include       <lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include       <adudate.h>
# include	<ui.h>
# include       <ug.h>
# include	<uigdata.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

static bool xf_isrepcat();

/**
** Name:	xfphys.sc - write physical structure modifiers, 
**			i.e. modify on table, create index, modify index
**
** Description:
**	This file defines:
**
**	xfmodify	write modify statements for a given table/view.
**	xfsave		write "save until" statement for a given table/view.
**	xfindexes	write create index statements.
**
** History:
**	13-jul-87 (rdesmond) written.
**	18-aug-88 (marian)
**		Changed retrieve statement to reflect column name changes in
**	18-aug-88 (marian)
**		Took out #ifdef HACKFOR50 since it is no longer needed.
**	21-sep-88 (marian)	bug 2198
**		Changed STcompare on unique_rule so it looks for 'U' instead 
**		of 'Y' since that's what the standard catalogs use.
**	08-feb-89 (marian)	bug 4544
**		To avoid producing maxpages with the number 8388607 which is
**		the default entered into iirelation.relmax by the modify 
**		command when no maxpages is specified for a hash structure,
**		we will check here to see if maxpages is this value if so,
**		do not produce a maxpages clause.
**	24-mar-89 (marian)
**		Modified code to meet updated standard catalog changes.
**		Changed calls to iiingres_tables to now call iitables, and
**		calls to iikey_columns will now be made to iicolumns.
**	18-apr-89 (marian)	bug 5425
**		Create a modify statement whenever a unique storage structure
**		is found, even for isam.
**	04-may-90 (billc)
**		major rewrite.  Conversion to SQL.
**	07/10/90 (dkh) - Removed include of cf.h so we don't need to
**			 pull in copyformlib when linking unloaddb/copydb.
**	20-nov-90 (billc)
**		bug fix (trashathon, no number yet) - wasn't generating modify
**		for frontend catalogs.
**	8-may-1991 (pete)
**		Changed Standard catalog version check to use new function:
**		IIUIscl_StdCatLevel(). Bug 36993.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		Added __HIGHC__ pragam to break data segment that is greater
**		than 64KB into 2 pieces.
**      09-sep-91 (billc)
**              Fix bug 39517.  Handle multiple locations on indexes.
**      30-feb-92 (billc)
**		Major rewrite for FIPS in 6.5.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**	29-jul-1995 (harpa06)
**		Don't make an attempt to journal system catalogs since they are
**		automatically journaled. If not, the user is to use the
**		CKPDB +j to journal catalogs.
**      15-may-1996 (thaju02)
**		Modified xfmodify to append 'with page_size' clause to 
**		modify statement if page size is 2k or greater.
**      03-mar-98 (dacri01)
**              FIx bug 56536.  COPY.IN scripts did not build the create index
**              command with the correct compression info.
**	04-aug-1998 (somsa01 for abowler)
**		Fix bug 58755. copy.in scripts contained wrong save dates if
**		the year was > 2000. Corrected (until 2035 anyway...)
**	26-Feb-1999 (consi01) Bug 92376 Problem INGDBA 29
**		Write out range clause for RTREE indexes
**      22-jun-1999 (musro02)
**              Corrected pragma to #pragma
**      08-Feb-2000 (hanal04) Bug 99787 Problem INGSRV1075.
**              Removed used of _date() function in favour of gmt_timestamp()
**              for production of SAVE UNTIL dates.
**      15-feb-2000 (musro02)
**              Corrected pragma to #pragma
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Apr-2001 (hanal04) Bug 104511 INGSRV 1432
**          Ensure multiple location indexes are generated correctly.
**	21-may-2001 (abbjo03)
**	    Add support for index creation in parallel.
**	21-Aug-2001 (hanje04)
**	    When creating code for indexes if XF_CHECK_PERSIST is set and 
**	    index is persistent just skip current loop iteration instead of 
**	    trying to check for all possible conditions.
**	    Correction to fix for bug 104883.
**      03-oct-2002 (stial01)
**         If fe/be catalog, minpages will always be with MODIFY, not COPY.
**	06-feb-2003 (devjo01) b109544.
**	   Have xfmodify check XF_MODIFY_ALLOCATION flag to see if
**	   "allocation" WITH clause item should be included with MODIFY.
**	24-feb-2003 (devjo01) b109544 part 2
**	   Additional changes for b109544 to accomidate 'ii_sequence_values',
**	   a new catalogue which is hash organized.  Since catalogues are
**	   always journaled, bulk copy is never used, and minpages must
**	   be on the MODIFY.  Changes made to place ALLOCATION & MIN/MAX_PAGES
**	   always on the same statement.
**      05-mar-2003 (devjo01)
**         Previous change incorrectly handled XF_ORDER_CCM case.
**	09-apr-2003 (devjo01) b110000
**	    '-add_drop' cleanup.
**	12-mar-2004 (vansa02)
**		Issueno:12914728 Bug No: 110947 Problem No: INGSRV2538
**		When there is a change from one user to another user during the parallel creation of indexes,
**		before setauth executes for a new user printing the go statement should execute for final 
**		index that belongs previous user.
** 	20-jul-2004 (rigka01) bug#111823, INGSRV2891
**	   Usermod changes minpages for hash tables back to the default.
**         When -with_modify is specified in the copydb statement, be
**         sure that minpages and maxpages are specified in the modify 
**         statement when the table is a hash table
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**      8-feb-07 (kibro01) b117217
**         Added TXT_HANDLE ptr to writewith
**       3-Dec-2008 (hanal04) SIR 116958
**          Added -no_rep option to exclude all replicator objects.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**
**/

/* # define's */

/* GLOBALDEF's */
GLOBALREF bool db_replicated;

/* extern's */

/* static's */
static char    *Col_names[DB_MAX_COLS + 1] = { NULL };
static  ADF_CB          *Adfcb = NULL;

/*{
** Name:	xfmodify - write physical structure modifiers 
**
** Description:
**        xfmodify writes out MODIFY statements, and will restore the
**	journalling state.
**
** Inputs:
**	t	pointer to XF_TABINFO describing the table to MODIFY.
**
** Outputs:
**
**	Returns:
**
** History:
**	13-jul-87 (rdesmond) written.
**	24-apr-89 (marian)	bug 4647
**		Removed call to xfmodify for indexes.  The index command
**		will now include a 'with' clause to modify the index to
**		the correct structure and fillfactor information.
**	15-may-1996 (thaju02)
**		Append 'with page_size' clause to modify statement if
**		page size is 2k or greater.
**	8-apr-98 (inkdo01)
**	    Seems we never got the "unique_scope" syntax into modify.
**      11-feb-1999 (nicph02)
**              minpages/maxpages clauses for HASH tables moved from
**              the 'modify' command to 'copy'. This ensures that a
**              bulk load will occur even if minpages>16 (bug 94487).
**       1-jun-1999 (wanfr01)
**       	Bug 97230, INGCBT 209
**		Highdata compression tables no longer unload as
**		'nocompression'
**	13-jun-2001 (gupsh01)
**	    Modified the input parameter list to include an output_flag, 
**	    This allows copydb flag -order_ccm to generate the create, 
**	    copy and modify statements in that order - [min, max] pages 
**	    options must be specified with the modify 
**	    rather than with the copy if -order_ccm is set.
**	15-jun-2001 (gupsh01)
**	    Added new flag -no_persist, for not writing out any indexes
**	    which are created with persistence into the copy.in script.
**	    This is required for usermod. (Bug 104883)
**	25-jun-2002 (gupsh01)
**	    Added new flag -no_repmod, to avoid modifying the replicator 
**	    catalogs.
**	29-jan-2003 (devjo01) b109560, b109565.
**	    Corrected logic which led to copydb with '-with_index' omitting
**	    the location portion of the where clause.  Also corrected
**	    problem with parallel index creation when skipping persistent
**	    indices.
**	06-feb-2003 (devjo01) b109544.
**	   Have xfmodify check XF_MODIFY_ALLOCATION flag to see if
**	   "allocation" WITH clause item should be included with MODIFY.
**	24-feb-2003 (devjo01) b109544 part 2
**	   Changes made to always place ALLOCATION & MIN/MAX_PAGES 
**	   together.  Either on MODIFY (here), or COPY.
** 	20-jul-2004 (rigka01) bug#111823, INGSRV2891
**	   Usermod changes minpages for hash tables back to the default.
**         When -with_modify is specified in the copydb statement, be
**         sure that minpages and maxpages are specified in the modify 
**         statement when the table is a hash table
**	16-feb-2005 (thaju02)
**	   Added -concurrent_updates for usermod.
**	31-Oct-2006 (kiria01) b116944
**	   Added 'nodependency_check' to ensure that the modify will
**	   not fail due to constraints checking.
**      13-Jan-2008 (hanal04) Bug 119826
**          Resolve problems in convtouni scripts:
**          Added the XF_MOD_TO_HEAP flag to allow callers such as convtouni
**          to generate a simple modify to heap for the specified table.
**	04-Feb-2009 (thaju02)
**	    Always specify "with page_size=..." clause. (B99758)
**	
*/

# ifdef	__HIGHC__					 
#pragma		Static_segment(ERx("XFPHYSDATA"));		 
# endif							 

void
xfmodify(t, output_flags, output_flags2)
XF_TABINFO	*t;
i4	output_flags;
i4	output_flags2;
{
    i4	    i;
    i4	    colcnt = 0;
    XF_COLINFO	*cp;
    auto bool firstmodatt = TRUE;
    char	tbuf[256];
    bool	db_replicated = FALSE;

    if ((output_flags && 
	(output_flags & XF_NO_REPMODIFY)) && 
	(db_replicated &&
	(xf_isrepcat(t))))
    {
	/* this is a replicator catalog */
	return;
    }

    /* no modify statement needed if default storage structure */
    /*
    ** bug 5425
    **	If this is a unique storage structure, you need a modify
    **  statement.  Check for !unique before returning.
    */
    if ( STequal(t->storage, ERx("heap"))
	&& t->is_data_comp[0] != 'Y' 
	&& t->is_key_comp[0] != 'Y' 
	&& t->is_unique[0] != 'U' 
	&& t->extend_size < 0 && 
	((t->allocation_size < 0) || !(output_flags & XF_MODIFY_ALLOCATION))
	) 
    {
	return;
    }

    xfwrite(Xf_in, ERx("modify "));
    xfwrite_id(Xf_in, t->name);
    xfwrite(Xf_in, ERx(" to "));
    if((output_flags2 & XF_MOD_TO_HEAP) == 0)
    {
        xfwrite(Xf_in, t->storage);
    }
    else
    {
        xfwrite(Xf_in, ERx("heap"));
        if (t->extend_size > 0)
        {
	    writewith(ERx("extend"), t->extend_size, &firstmodatt, Xf_in);
        }
        if (t->allocation_size > 0 && 
	     (output_flags & (XF_ORDER_CCM|XF_MODIFY_ALLOCATION)))
        {
	    writewith(ERx("allocation"), t->allocation_size, 
                      &firstmodatt, Xf_in);

        }
        xfwrite(Xf_in, GO_STMT);
        return;
    }

    if (t->is_unique[0] == 'U')
	xfwrite(Xf_in, ERx(" unique"));

    /* 
    ** Now write out the column names of the index.  We need to write
    ** them in key order, so we'll do an insertion sort, sort of. 
    ** (We fetched them from the database in column order, not key order.)
    */
    
    for (cp = t->column_list; cp != NULL; cp = cp->col_next)
    {
	if (cp->key_seq < 1)
	    continue;
    	
	Col_names[cp->key_seq - 1] = cp->column_name;
	colcnt++;
    }
    for (i = 0; colcnt > 0 && i < DB_MAX_COLS; i++)
    {
	if (Col_names[i] != NULL)
	{
	    xfwrite(Xf_in, i == 0 ? ERx(" on\n\t") : ERx(",\n\t"));
	    xfwrite_id(Xf_in, Col_names[i]);
	    Col_names[i] = NULL;
	    --colcnt;
	}
    }


    /* ifillpct and lfillpct are for btree storage structure only */
    if (STequal(t->storage, ERx("btree")))
    {
	if (t->ifillpct != 0)
	    writewith(ERx("nonleaffill"), t->ifillpct, &firstmodatt, Xf_in);

	if (t->lfillpct != 0)
	    writewith(ERx("leaffill"), t->lfillpct, &firstmodatt, Xf_in);
    }
    else if (STequal(t->storage, ERx("hash")) &&
      (output_flags & (XF_ORDER_CCM|XF_MODIFY_ALLOCATION|XF_TAB_MODONLY)))
    {
       /* minpages and maxpages are for hash storage structure only */
       if (t->minpages != 0)
           writewith(ERx("minpages"), t->minpages, &firstmodatt, Xf_in);
       if (t->maxpages != 0 && t->maxpages != XF_MAXPAGES)
           writewith(ERx("maxpages"), t->maxpages, &firstmodatt, Xf_in);
     }


    /* fillfactor is for btree, isam and hash */
    if (!STequal(t->storage, ERx("heap")) && t->dfillpct != 0)
	writewith(ERx("fillfactor"), t->dfillpct, &firstmodatt, Xf_in);

    if (t->extend_size > 0)
	writewith(ERx("extend"), t->extend_size, &firstmodatt, Xf_in);

    if (t->allocation_size > 0 && 
	 (output_flags & (XF_ORDER_CCM|XF_MODIFY_ALLOCATION)))
	writewith(ERx("allocation"), t->allocation_size, &firstmodatt, Xf_in);

    writewith(ERx("page_size"), t->pagesize, &firstmodatt, Xf_in);

    if (concurrent_updates)
    {
	(void) STprintf(tbuf, ERx("%sconcurrent_updates"),
		(firstmodatt ? ERx("\nwith ") : ERx(",\n\t")));
	xfwrite(Xf_in, tbuf);
	firstmodatt = FALSE;
    }
    /* 
    ** index compression currently only for btree. 
    ** Data compression can be on any storage type.
    */
    if (t->is_data_comp[0] == 'Y' || t->is_key_comp[0] == 'Y' || 
        t->is_data_comp[0] == 'H' )
    {
	(void) STprintf(tbuf, ERx("%scompression = (%skey, %sdata)"), 
		    (firstmodatt ? ERx("\nwith ") : ERx(",\n\t")), 
		    (t->is_key_comp[0] == 'Y' ? ERx("") : ERx("no")),
		    (t->is_data_comp[0] == 'Y' ? 
                        ERx("") : 
                        (t->is_data_comp[0] == 'H'? ERx("hi") : ERx("no"))
                    )
		    );
	xfwrite(Xf_in, tbuf);
	firstmodatt = FALSE;
    }
	    
    /* 6.5 stuff for referential integrity */
    if (t->unique_scope[0] == 'S')
	xfwrite(Xf_in, ERx(",\n\tunique_scope = statement"));

    /* If requested, ensure that the modify will not trip constraints */
    if (output_flags2 & XF_NODEPENDENCY_CHECK)
	xfwrite(Xf_in, ERx(",\n\tnodependency_check"));

    xfwrite(Xf_in, GO_STMT);
}

/*{
** Name:	xfjournal - set journalling on for table, if needed
**
** Description:
**        xfjournal writes a SET JOURNALING ON statement if the reloaded
**	table is journalled.
**
** Inputs:
**	t	pointer to XF_TABINFO describing the table.
**
** Outputs:
**
**	Returns:
** History:
**	15-Mar-2010 (troal01)
**	    Check t->becat and t->fecat instead of the first two letters.
**	    Change necessary for spatial_ref_sys.
*/

void
xfjournal(t)
XF_TABINFO	*t;
{
 
    /* turn on journaling >after< the COPY, for better performance  
       providing the table is not a system table since it is automatically
       journaled when creating a new table. If not, then CKPDB +j must be
       used to journal system catalogs.
    */
    if ((t->journaled[0] == 'Y' || t->journaled[0] == 'C') &&
       (!t->becat) && (!t->fecat))
    {
	xfwrite(Xf_in, ERx("set journaling on "));
	xfwrite_id(Xf_in, t->name);
	xfwrite(Xf_in, GO_STMT);
    }
}

/*{
** Name:	xfsave - write SAVE UNTIL statement.
**
** Description:
**	Use the expiration date to write a 'save until' statement.
**	Fixes jup 21517.  (Yes, there may still be people that use
**	this feature!)
**
**	There is an ugly hack here, since the SAVE UNTIL statement won't
**	accept a date in standard Ingres date format!!!
**
** Inputs:
**	tp		pointer to XF_TABINFO structure describing object.
**
**	Returns:
**
** History:
**      28-May-2008 (hanal04 & Chris Dawe) Bug 120429
**          Do not go the the server to convert dates. When many SAVE UNTILs
**          are present this degrades performance unnecessarily.
*/

void
xfsave(tp)
EXEC SQL BEGIN DECLARE SECTION;
XF_TABINFO	*tp;
EXEC SQL END DECLARE SECTION;
{
    if (tp->expire_date != 0)
    {
        char         date_buf[AFE_DATESIZE + 1];
        HRSYSTIME    stime;
        AD_DATENTRNL idate;
        i4           error;
        DB_STATUS    status;

        if (Adfcb == NULL)
            Adfcb = FEadfcb();

        /* 
	** convert date into MM DD YYYY format to produce a "SAVE UNTIL" 
	** statement.  Some customers use a lot of SAVE UNTIL's so
        ** we we'll convert the time locally rather than going to the server.
	*/
        stime.tv_sec = tp->expire_date;
        stime.tv_nsec = 0;
 
        /* Adjust the stime to be a GMT value */
        stime.tv_sec += TMtz_search(Adfcb->adf_tzcb, 
                                    TM_TIMETYPE_GMT, 
                                    stime.tv_sec);

        /* Convert the system time to an internal time */
        status = adu_hrtimetodate(&stime, &idate, &error);

        if(status == E_DB_OK)
        {
            /* generate the date format required by the SAVE UNTIL */
            STprintf(date_buf, ERx("%d %d %d"), idate.dn_month, 
                               idate.dn_lowday, idate.dn_year);
        
            xfwrite(Xf_in, ERx("save "));
	    xfwrite_id(Xf_in, tp->name);
            xfwrite(Xf_in, ERx(" until "));
            xfwrite(Xf_in, date_buf);
            xfwrite(Xf_in, GO_STMT);
        }
        else
        {
            /* The time came from the DB so this should never happen */
            IIUGerr(E_XF0062_Invalid_Timestamp, UG_ERR_FATAL, 1, "SAVE UNTIL");
       }
    }
}

/*{
** Name:	xfindexes - write create index, modify index statements.
**
** Description:
**	xfindexes calls xffillindex to get information about all indexes
**	of interest.  xfindexes then calls writeindex to write out the info
**	for each one.
**
** Inputs:
**	output_flags 		This is passed to writeindex
**	basetable - table whose index information is to be retrieved.
**                  This parameter is null if you want all indices in the db
**
** Outputs:
**
**	Returns:
**		none.
**	07-jun-2000 (gupsh01)
**	    Modified the parameter to rxffillindex function, as it is modifies.
**	17-nov-2000 (gupsh01)
**	    Added new parameter output_flags.	
**	21-may-2001 (abbjo03)
**	    Add support for creating indexes in parallel.
**	30-jan-2003 (devjo01) b109565
**	    Corrected situation where bad syntax was emitted if parallel
**	    index creation was enable, and persistent indices were skipped,
**	    and last index processed for a table was persistent.
**       4-May-2004 (hanal04) Bug 112265 INGSRV2814
**          Parallel index creation can blow the range variable limit
**          during reload. Limit the number of indexes we create in
**          a given statement to XF_PST_NUMVARS.
**	15-Apr-2005 (thaju02)
**	    Add tlist param to xfindexes and xffillindex.
**      13-Jan-2008 (hanal04) Bug 119826
**          Resolve problems in convtouni scripts:
**          Added the XF_NOCREATE flag to allow callers such as convtouni
**          to generate just the drop statements for the target list.
**      17-Mar-2008 (hanal04) SIR 101117
**          If indexes are being grouped by table don't write the INDEX comment.
**	24-Aug-2009 (wanfr01) b122319 - renamed tlist to basetable for
**	    clarification
*/
void
xfindexes(
i4		output_flags,
i4              output_flags2,
XF_TABINFO	*basetable)
{
    XF_TABINFO	*ilist;
    XF_TABINFO	*ip;
    char	last_owner[DB_MAXNAME + 1];
    char	last_base[DB_MAXNAME + 1];
    char	buf[64];
    int		gopending = 0;
    i4          rngvars = 0;

    if ((ilist = xffillindex(0, basetable)) == NULL)
        return;

    if(GroupTabIdx == FALSE)
    {
        xfwritehdr(INDEXES_COMMENT);
    }

    /* If '-add_drop', and base table not involved emit DROP stmts. */
    if ( XF_DROP_INCLUDE == ( output_flags &
	 (XF_DROP_INCLUDE|XF_TAB_CREATEONLY|XF_PRINT_TOTAL) ) )
    {
	for (ip = ilist; ip != NULL; ip = ip->tab_next)
	{
            if((GroupTabIdx == TRUE) && 
               (
               (STscompare(ip->base_name, sizeof(ip->base_name),
                      basetable->name, sizeof(basetable->name)) != 0) ||
               (STscompare(ip->base_owner, sizeof(ip->base_owner), 
                      basetable->owner, sizeof(basetable->owner)) != 0)
               ) )
                continue;

	    /* Does user id have to be reset? */
	    xfsetauth(Xf_in, ip->owner);

	    /* Skip persistent if requested */
	    if ( (output_flags & XF_CHECK_PERSIST ) &&
		 (ip->persistent[0] == 'Y') )
		continue;
	    xfwrite(Xf_in, ERx("drop index "));
	    xfwrite_id(Xf_in, ip->name);
	    xfwrite(Xf_in, GO_STMT);
	}
    }

    if(output_flags2 & XF_NOCREATE)
    {
        if ( gopending ) xfwrite(Xf_in, GO_STMT);
        xfifree(ilist);
        return;
    }

    *last_owner = *last_base = EOS;
    for (ip = ilist; ip != NULL; ip = ip->tab_next)
    {
        if((GroupTabIdx == TRUE) && 
           (
               (STscompare(ip->base_name, sizeof(ip->base_name),
                      basetable->name, sizeof(basetable->name)) != 0) ||
               (STscompare(ip->base_owner, sizeof(ip->base_owner), 
                      basetable->owner, sizeof(basetable->owner)) != 0)
           ) )
            continue;

	rngvars++;
	if (CreateParallelIndexes)
	{
	    if (STcmp(last_owner, ip->base_owner) != 0 ||
		    STcmp(last_base, ip->base_name) != 0 ||
		    rngvars >= XF_PST_NUMVARS)
	    {
		if ( gopending ) xfwrite(Xf_in, GO_STMT);
            }	
	}	
	/* Does user id have to be reset? */
	xfsetauth(Xf_in, ip->owner);

	/* if the ip has persistence index then do not write it for 
	** check persistence flag.     
	** do not create indexes that are with persistent 
	** do not create indexes that are with nopersistent 
	*/
	if ( (output_flags & XF_CHECK_PERSIST ) &&
	     (ip->persistent[0] == 'Y') )
	    continue;

	if (CreateParallelIndexes)
	{
	    if (STcmp(last_owner, ip->base_owner) != 0 ||
		    STcmp(last_base, ip->base_name) != 0 ||
		    rngvars >= XF_PST_NUMVARS)
	    {
		gopending = 1;
		rngvars = 0;
		xfwrite(Xf_in, ERx("create index ("));
		STcopy(ip->base_owner, last_owner);
		STcopy(ip->base_name, last_base);
	    }
	    else
	    {
		xfwrite(Xf_in, ERx(",\n("));
	    }
	    writeindex(ip, output_flags);
	    xfwrite(Xf_in, ERx(")"));
	}
	else
	{
	    STprintf(buf, ERx("create %sindex "),
                (ip->is_unique[0] == 'U' ? ERx("unique ") : ERx("")));
	    xfwrite(Xf_in, buf);
	    writeindex(ip, output_flags); 
	    xfwrite(Xf_in, GO_STMT);
	}
    }
    if ( gopending ) xfwrite(Xf_in, GO_STMT);
    xfifree(ilist);
}

/*{
** Name:	writeindex - write statements to create indexes.
**
** Description:
**
** Inputs:
**	ip - pointer to XF_TABINFO struct describing an index.
**
** Outputs:
**	none.
**
**	Returns:
**		none.
**
** History:
**	13-jul-87 (rdesmond) written.
**	24-apr-89 (marian)	bug 4647
**		Modified the way the index command is built.  Stole parts
**		of xfmodify to use here.  Those pieces can be pulled from
**		both routines later and called from xfmodify and xfindex.
**		The index command will now use the new 'with' clause instead
**		of an additional modify command.
**	26-jul-89 (marian)	bug 6935
**		Add location for index.  This information was never being
**		retrieved from iitables.  Add loc_name to hold this information.
**	09-sep-91 (billc)	bug 39517
**		Add multiple locations for index.  
**      11-Dec-1998 (hanal04)
**         In writeindex() add 'with priority' clause if the table's cache
**         priority is not the default value of 0. b94494.
**	26-Feb-1999 (consi01) Bug 92376 Problem INGDBA 29
**		Write out range clause for RTREE indexes.
**      23-Apr-2001 (hanal04) Bug 104511 INGSRV 1432
**         Ensure a NULL buff parameter is passed to xfaddloc().
**         Failure to do so prevents multiple locations from being
**         written to the copy.in file if the memory location
**         is not NULL.
**	17-nov-2000 (gupsh01)
**	    Added new input parameter output_flags in order to identify
**	    if the location information is to printed out for the index. 
**	21-may-2001 (abbjo03)
**	    Move some code to xfindexes() above to support creation of indexes
**	    in parallel. Add code to tag a parallel index as UNIQUE.
**      15-Oct-2001 (hanal04) Bug 105924 INGSRV 1566
**         Modified the parameter list for xfaddloc as per the new definition
**         for xfaddloc.
**	29-jan-2003 (devjo01) b109560
**	    Corrected logic which led to copydb with '-with_index' omitting
**	    the location portion of the WITH clause.
**	04-Feb-2009 (thaju02)
**	    Always specify "with page_size=..." clause. (B99758)
*/

void 
writeindex(ip, output_flags)
XF_TABINFO	*ip;
i4 output_flags;
{
    auto bool	bfalse = FALSE;
    i4		i;
    char	buf[512];
    i4		colcnt = 0;
    XF_COLINFO	*cp;

    xfwrite_id(Xf_in, ip->name);
    xfwrite(Xf_in, ERx(" on "));
    xfwrite_id(Xf_in, ip->base_name);
    xfwrite(Xf_in, ERx(" (\n\t"));

    /* write out the column names. */
    for (cp = ip->column_list; cp != NULL; cp = cp->col_next)
    {
	if (cp->col_seq > 1)
	    xfwrite(Xf_in, ERx(",\n\t"));

	xfwrite_id(Xf_in, cp->column_name);
    }
    xfwrite(Xf_in, ERx(")"));

    if (CreateParallelIndexes && ip->is_unique[0] == 'U')
	xfwrite(Xf_in, ERx(" unique "));

    /* add "with" clause for other modify attributes */

    xfwrite(Xf_in, ERx("\nwith structure = "));
    xfwrite(Xf_in, ip->storage);

    /* b94494 - If priority is not default of 0 then add priority clause */
    if(ip->priority != 0)
    {
       (void) STprintf(buf, ERx(",\n\tpriority = %d"), ip->priority);
       xfwrite(Xf_in, buf);
    }

    STprintf(buf, ERx(",\n\tpage_size = %d"), ip->pagesize);
    xfwrite(Xf_in, buf);

    /* write the compression information */
    if (ip->is_data_comp[0] == 'Y' || ip->is_key_comp[0] == 'Y')
    {
        (void) STprintf(buf, ERx(",\n\tcompression = (%skey, %sdata)"),
                    (ip->is_key_comp[0] == 'Y' ? ERx("") : ERx("no")),
                    (ip->is_data_comp[0] == 'Y' ? ERx("") : ERx("no")));
        xfwrite(Xf_in, buf);
    }
    else
        xfwrite(Xf_in, ERx(",\n\tnocompression"));

    /* add key clause. */
    xfwrite(Xf_in, ERx(",\n\tkey = ("));

    /* 
    ** Now write out the column names of the index.  We need to write
    ** them in key order, so we'll do an insertion sort, sort of. 
    ** (We couldn't fetch them in key order, since we fetched the columns
    ** in column order.)
    */

    for (cp = ip->column_list; cp != NULL; cp = cp->col_next)
    {
	if (cp->key_seq < 1)
	    continue;
	
	Col_names[cp->key_seq - 1] = cp->column_name;
	colcnt++;
    }
    for (i = 0; colcnt > 0 && i < DB_MAX_COLS; i++)
    {
        if (Col_names[i] != NULL)
        {
	    if (i != 0)
		xfwrite(Xf_in, ERx(", "));
            xfwrite_id(Xf_in, Col_names[i]);
            Col_names[i] = NULL;
            --colcnt;
        }
    }

    xfwrite(Xf_in, ERx(")"));

    /* 6.5 stuff for referential integrity */
    if (ip->unique_scope[0] == 'S')
	xfwrite(Xf_in, ERx(",\n\tunique_scope = statement"));
    if (ip->persistent[0] == 'Y')
	xfwrite(Xf_in, ERx(",\n\tpersistence"));

    /* ifillpct and lfillpct are for btree storage structure only */
    if (STequal(ip->storage, ERx("btree")))
    {
	if (ip->ifillpct != 0)
	    writewith(ERx("nonleaffill"), ip->ifillpct, &bfalse, Xf_in);
	if (ip->lfillpct != 0)
	    writewith(ERx("leaffill"), ip->lfillpct, &bfalse, Xf_in);
    }

    /* minpages and maxpages are for hash storage structure only */
    if (STequal(ip->storage, ERx("hash")))
    {
	if (ip->minpages != 0)
	    writewith(ERx("minpages"), ip->minpages, &bfalse, Xf_in);
	if (ip->maxpages != 0 && ip->maxpages != XF_MAXPAGES)
	    writewith(ERx("maxpages"), ip->maxpages, &bfalse, Xf_in);
    }

    /* fillfactor is for btree, isam and hash */
    if (ip->dfillpct != 0)
	writewith(ERx("fillfactor"), ip->dfillpct, &bfalse, Xf_in);

    /* range clause for RTREEs */
    if (STequal(ip->storage, ERx("rtree")))
    {
	STprintf(buf, ERx(",\n\trange = ((%.2f,%.2f"), ip->rng_ll1,'.', 
                                                       ip->rng_ll2,'.');
	xfwrite(Xf_in, buf);
	if (!(ip->rng_ll3 == 0.0 && ip->rng_ur3 == 0.0))
	{
	    STprintf(buf, ERx(",%.2f"), ip->rng_ll3, '.');
	    xfwrite(Xf_in, buf);
            if (!(ip->rng_ll4 == 0.0 && ip->rng_ur4 == 0.0))
            {
	        STprintf(buf, ERx(",%.2f"), ip->rng_ll4, '.');
	        xfwrite(Xf_in, buf);
            }
	}
	STprintf(buf, ERx("),\n\t\t (%.2f,%.2f"), ip->rng_ur1, '.',
                                                  ip->rng_ur2, '.');
	xfwrite(Xf_in, buf);
	if (!(ip->rng_ll3 == 0.0 && ip->rng_ur3 == 0.0))
	{
	    STprintf(buf, ERx(",%.2f"), ip->rng_ur3, '.');
	    xfwrite(Xf_in, buf);
            if (!(ip->rng_ll4 == 0.0 && ip->rng_ur4 == 0.0))
            {
	        STprintf(buf, ERx(",%.2f"), ip->rng_ur4, '.');
	        xfwrite(Xf_in, buf);
            }
	}
	xfwrite(Xf_in, ERx("))"));
    }

    /*
    ** bug 6935
    ** add location name for index.
    */
    if ( !(output_flags & (XF_NO_LOCATIONS | XF_XMLFLAG)) )
    {
    xfwrite(Xf_in, ERx(",\n\tlocation = ("));
    xfwrite(Xf_in, ip->location);

    /* fix 39517 - handle multiple locations on secondary index. */
    if (ip->otherlocs[0] == 'Y')
	xfaddloc(ip->name, ip->owner, FALSE, NULL);
    xfwrite(Xf_in, ERx(")"));
    }
}

/*{
** Name:	writewith - write a with-clause item.
**
** Description:
**	Handy utility routine to write out WITH clause items.
**
** Inputs:
**	withname	name of the with option.
**	value		value of the option.
**	first		bool, is this the first item?
**
**	Returns:
**		none.
**	16-may-1996 (nanpr01)
**	  Took out the static declaration.
**	8-feb-2007 (kibro01) b117217
**	  Added handle to allow a different file to Xf_in
*/

void 
writewith(withname, value, first, handle)
char	*withname;
i4	value;
bool	*first;
TXT_HANDLE	*handle;
{
    char 	tbuf[256];

    (void) STprintf(tbuf, ERx("%s%s = %d"), 
		(*first ? ERx("\nwith ") : ERx(",\n\t")), withname, value);
    xfwrite(handle, tbuf);
    *first = FALSE;
}

/*{
** Name:        xf_isrepcat - finds out if a given table is 
**			      a replicator catalog.
**
** Description:
**	This function inquires dd_suppport tables to determine if this
**	table is a replicator catalog. This routine should be called
**	if dd_support_tables exists as determined by xf_dbreplicated()
**	else the users may be unnecessarily warned about the
**	dd_support_table does not exists for non replicated databases.
**
** Inputs:
**      t           pointer to the XF_TABINFO structure for table to
**		    search.
**      Returns:
**              TRUE : t is a replicator catalog.
**              FALSE: t is not a replicator catalog.
**      28-Jun-2002 (gupsh01)
**	    Created.
*/

static bool xf_isrepcat(t) 
XF_TABINFO      *t;
{
EXEC SQL BEGIN DECLARE SECTION;
    char        tabname[DB_MAXNAME + 1];
    char        table[DB_MAXNAME + 1];
    bool        result = FALSE;
    i4          len;
EXEC SQL END DECLARE SECTION;

   STcopy(t->name, table);

   EXEC SQL SELECT TRIM(table_name) 
   INTO :tabname   
   FROM dd_support_tables 
   WHERE LOWERCASE(table_name) = :table;	

   if ( FEinqerr() == OK && FEinqrows() > 0 )
     result = TRUE;

    return (result);
}

/*{
** Name:        xf_dbreplicated - determines if the database has 
**				  replicator catalogs.
**
** Description:
**
**	If the database is replicated then the replicator catalogs
**	dd_support_tables will contain information about which 
**	catalogs are created for replicator.
**	This funtion predetermines if this table dd_support_tables 
**	exists before trying to find a match for the replicator
**	catalog.
** Inputs:
**
**      Returns:
**
**         TRUE  if the replicator catalog dd_support_tables exists.
**         FALSE if not found.
**
**      28-Jun-2002 (gupsh01)
**          Created.
*/

bool xf_dbreplicated(void)
{
EXEC SQL BEGIN DECLARE SECTION;
    char        tabname[DB_MAXNAME + 1];
    bool        result = FALSE;
EXEC SQL END DECLARE SECTION;

   EXEC SQL SELECT TRIM(relid) 
   INTO :tabname
   FROM iirelation 
   WHERE LOWERCASE(relid) = 'dd_support_tables';

   if ( FEinqerr() == OK && FEinqrows() == 1 )
     result = TRUE;
    
   return (result);
}
