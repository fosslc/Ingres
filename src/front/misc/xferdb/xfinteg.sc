
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include       <me.h>
# include	<iicommon.h>
# include	<er.h>
# include	<fe.h>
# include	<ug.h>
# include	<st.h>
# include       <si.h>
# include       <lo.h>
# include       <adf.h>
# include       <afe.h>
# include       <cm.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/
FUNC_EXTERN    void writewith(char *, i4, bool *, TXT_HANDLE*);

/**
** Name:	xfinteg.sc - write statement to create integrity.
**
** Description:
**	This file defines:
**
** 	xfinteg  	write statement to create old-style integrity.
**	xfconstraints	write statements to create table integrities.
**	xf_parse_constraint_text  Parse the constraint information and
**				  modify it if necessary. 
**
** History:
**	13-jul-87 (rdesmond) written.
**	14-sep-87 (rdesmond) 
**		changed att name "number" to "integrity_number" in iiintegrities
**	10-mar-88 (rdesmond)
**		now writes 'ret to all' and 'all to all' in SQL only, since
**		permit may be on view, and this is not allowed in QUEL;
**		removed trim() from target list for 'text'; writes language for
**		each permit before text of permit (based on first word of text
**		being 'create' or not); writes "\p\g" after each integrity.
**	18-aug-88 (marian)
**		Changed retrieve statement to reflect column name changes in
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**	04-may-90 (billc)
**		major rewrite, conversion to SQL.
**	12-sep-91 (billc)
**		fix 39773 - was writing extra \p\g on 1st integrity.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**      25-oct-1994 (sarjo01) Bug 62930
**              xfconstraints(): insert space after constraint name
**      28-oct-1994 (sarjo01) Bug 63642
**              xfconstraints(): add order by consid1 to get constraints
**              in correct dependency order
**	9-Nov-95 (johna)
**		Fix bug 72176 - only print the constraints for the tables
**		requested - change xfconstraints to take a list of tables
**		from the caller. Note that we cannot call xffilltables again
**		as this would generate a second "There are 4 tables owned ..
**		message.
**	15-Nov-95 (kch)
**		Supplement to fix for bug 72176 - only write the constraints
**		header if one or more of the requested tables is constrained.
**		Previously, the header would be written if the database
**		contained any constrained tables, regardless of whether
**		they were requested.
**	09-Dec-96 (i4jo01)
**		When checking whether a table name is identical to a table
**		found in a constraint list, make sure to trim out trailing
**		white space from name picked up from system catalog. (b79663)
**	23-Jul-99 (kitch01)
**		Bug 97600. Fix in xfconstraints() to prevent duplicate
**		constraint definitions in copy.in.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Jun-00 (gupsh01)
**		Added xf_parse_constraint_text to handle the user defined indexes
**		in constraint information. We parse the information read from the 
**		iiconstraint catalog and modify it if necessary.
**      15-Oct-2001 (hanal04) Bug 105924 INGSRV 1566
**          Modified xfconstraints() to prevent array out of bounds
**          when adding location name info to indexes.
**      01-nov-2002 (stial01)
**          xfconstraints() Added code for XF_DROP_INCLUDE, and added option 
**          to print only check constraints.
**      09-jan-2003 (stial01)
**          xfconstraints() removed unecessary check_constr_only parameter
**	09-apr-2003 (devjo01) b110000
**	    '-add_drop' cleanup.
**      8-feb-07 (kibro01) b117217
**         Added TXT_HANDLE ptr to writewith
**      25-Jan-2008 (hanal04) 119811
**         max is a reserved word in 2.0 and earlier. A copy over net
**         will fail unless we delimit max.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */

/* static's */

/*{
** Name:	xfinteg - write statement to create old-style integrity
**			on a given table..
**
** Description:
**
** Inputs:
**	op		the XF_TABINFO struct describing the table which
**			has the integrity.
**
** Outputs:
**
**	Returns:
**		none.
**
** Side Effects:
**	none.
**
** History:
**	13-jul-87 (rdesmond) written.
**	14-sep-87 (rdesmond) 
**		changed att name "number" to "integrity_number" in 
**				iiintegrities.
**	10-mar-88 (rdesmond)
**		removed trim() from target list for 'text'; writes language for
**		each integ before text of integ (based on first word of text
**		being 'create' or not); writes "\p\g" after each integrity.
*/

void
xfinteg(op)
EXEC SQL BEGIN DECLARE SECTION;
XF_TABINFO	*op;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
    char	text_segment[XF_INTEGLINE + 1];
    i2		number;
    i4		text_sequence;
EXEC SQL END DECLARE SECTION;
    i2		lastnumber;
    TXT_HANDLE		*tfd = NULL;

    if (op->has_integ[0] == 'N')
	return;

    lastnumber = 0;

    EXEC SQL REPEATED SELECT i.text_segment, i.integrity_number, 
		i.text_sequence
	INTO :text_segment, :number, :text_sequence
	FROM iiintegrities i
	WHERE i.table_name = :op->name
		AND i.table_owner = :op->owner
	ORDER BY i.integrity_number, i.text_sequence;
    EXEC SQL BEGIN;
    {
	if (tfd == NULL)
	    tfd = xfreopen(Xf_in, TH_IS_BUFFERED);

	/* if new integrity statement */
	if (number != lastnumber)
	{
	    /* if not the first integrity statement. */
	    if (lastnumber != 0)
		xfflush(tfd);

	    /* set language that the integrity is written in */
	    if (STbcompare(text_segment, 6, ERx("create"), 6, FALSE) == 0)
		xfsetlang(tfd, DB_SQL);
	    else
		xfsetlang(tfd, DB_QUEL);
	    lastnumber = number;
	}
	xfwrite(tfd, text_segment);
    }
    EXEC SQL END;

    if (tfd != NULL)
	xfclose(tfd);
}

/* 
** Name: xf_parse_constraint_text - parses the constraints text_segment, obtained 
** 				    from iiconstraints catalog.
**
**
** Description:
**
** we wish to find out if the index name is provided by 
** the user while providing constraints.  
**
** If the user provided a name for the index
** then grab the values for :
** structure , fillfactor, minpages, maxpages 
** leaffill, nonleaffill, allocation, extend and 
** location
**
** REMARK also check these values in the iitables catalogue. 
** for changes in :
**		key, compression, persistence, 	unique_scope, 
**		range, page_size
**
** if a difference is found then append that statement into the 
** Alter table with clause,most important changes are changing the 
** structure and or changing the location of the index. 
**
** Inputs:
**	cmd_line	A char * describing the text_segment from iicatalogues
**
**
**	Returns:
**				A char * describing the modified text_segment 
**		 
** History:
**	09-may-2000 (gupsh01)
**	    created. 
**      15-jan-2002 (gupsh01)
**	    Increased the size of the buffer to accomodate the full length of
**	    alter table statements. The buffer should be long enough to
**	    accomodate max length of cmd_line ie (XF_INTEGLINE + 1) and 
**	    maximum length possible of an alter table with clause ie 
**	    XF_MAX_INTEGBUF this is calculated based on the current 
**	    definition of alter table statements. 
**	19-mar-2002 (gupsh01)
**	    Modifed the function signature of xf_parse_constraint_text. 
** 	    Now the returning string will be passsed in the parameter list 
** 	    as buffer. 
**       1-Feb-2005 (hanal04) Bug 115686
**          Handle delimited index names to avoid truncation and SEGVs.
**	30-Nov-2006 (kibro01) b117217
**	    Pass in a boolean to say if we've added the "with" already
**/ 
void
xf_parse_constraint_text(cmd_line, ilist, buffer, first_addition)
char 		*cmd_line;
XF_TABINFO	*ilist;
char 		*buffer;
bool		*first_addition;
{
    XF_TABINFO	*ip;
  	char index_name[XF_INTEGLINE + 1]; 
  	char *ptr = 0 ;
 	char tmp_ptr[XF_INTEGLINE + 1];  
	int copy_len = 0;
	int bytestocopy = 0;
	int total_length = 0;
	int with_length = 0;
	char temp_string[XF_INTEGLINE + 1];
	bool delimited = FALSE;

	ptr = STstrindex ( cmd_line, "WITH", 0, 0);
	if (ptr) 
	{
	 if (STbcompare( ptr, 13, "WITH (INDEX =", 13, 0) == 0)
	 { 
          if (!(STbcompare( ptr, 24,
		"WITH (INDEX = BASE TABLE", 24, 0) == 0))
	  {
		total_length = STlength (cmd_line);
		with_length = STlength (ptr);
		bytestocopy = STlength (cmd_line) - STlength (ptr) - 1 ; 
		STlcopy(cmd_line, buffer, bytestocopy);
		*first_addition = FALSE;
	
        ptr = (ptr + 15);
	if(*(ptr-1) == '"')
	    delimited = TRUE;
        STcopy(ptr, tmp_ptr);
	while(ptr && !((CMwhite (ptr) && delimited == FALSE) || 
	          ( *ptr == '"') || (( *ptr == ',') && delimited == FALSE)))
	{
	    /* get the index name */
	    ptr = CMnext (ptr);
	    copy_len++;
	}
        STlcopy( tmp_ptr, index_name, copy_len ); 
  	/* 
	** we have positively identified that the WITH CLAUSE is provided 
	** and also the user defined index name is provided now we should 
	** construct the following statement for the constraint and 
	** return it. Get the information from the catalogs.
	*/ 		
 	xfread_id(index_name);		

 	 for (ip = ilist; ip != NULL; ip = ip->tab_next)
         {		
	 	if (STcompare(ip->name,index_name) == 0)
			break;
	 }
	 STcat(buffer, " WITH (INDEX = \"" );
	 STcat(buffer, ip->name );				/* index_name */
	 STcat(buffer, "\"");
	 
	 STcat(buffer, " ,LOCATION = (" );
	 STcat(buffer, ip->location );			/* location_name */
	 if (ip->otherlocs[0] == 'Y')	   
	 STcat(buffer,  ip->location_list);	   
         STcat(buffer, ")");

	 STcat(buffer, " ,STRUCTURE = " );
	 STcat(buffer, ip->storage );			/* structure */

	 if (!STequal(ip->storage, ERx("heap")))
	 {
	   if(ip->ifillpct != 0)
	   {
	   STcat(buffer, " ,FILLFACTOR = " );
	   CVla(ip->ifillpct, temp_string);
	   STcat(buffer, temp_string );			/* fill factor */
	   }
	 }
         if( STcompare(ip->storage,"hash") == 0)
	 {
	   if(ip->minpages != 0)
	   {
	   STcat(buffer, " ,MINPAGES = " );
   	   CVla(ip->minpages, temp_string);
	   STcat(buffer, temp_string );			/* min pages */ 
	   }
	   if(ip->maxpages != 0)
	   {
	   STcat(buffer, " ,MAXPAGES = " );
	   CVla(ip->maxpages , temp_string);
	   STcat(buffer, temp_string );  		/* max pages */
	   }
	 }
	 if(STcompare(ip->storage,"btree") == 0)
	 {
	 STcat(buffer, " ,LEAFFILL = " );
	 CVla(ip->lfillpct, temp_string);
	 STcat(buffer, temp_string );			/* leaffill */
	 STcat(buffer, " ,NONLEAFFILL = " );
	 CVla(ip->dfillpct, temp_string);
	 STcat(buffer, temp_string );			/* nonleaffill */ 
	 }
	 if(ip->extend_size > 0)
	 {
	 CVla(ip->extend_size, temp_string);
	 STcat(buffer, " ,EXTEND = " );
	 STcat(buffer, temp_string );			/*	extend	  */
	 }
	 if(ip->allocation_size > 0)
	 {
   	 CVla(ip->allocation_size, temp_string);
	 STcat(buffer, " ,ALLOCATION = ");
	 STcat(buffer, temp_string );			/*	allocation   */
	 }
	 STcat(buffer, ")");	
	}
      }
    }
STcopy(cmd_line, buffer);
}

/*{
** Name:	xfconstraints - write statements to create table integrities.
**
** Description:
**
** Inputs:
**	op		the XF_TABINFO struct describing the table which
**			has the integrity.
**
** Outputs:
**
**	Returns:
**		none.
**
** History:
**	09-oct-92 (billc) written.
**      25-oct-94 (sarjo01) Bug 62930
**                insert space after constraint name to prevent syntax error
**      28-oct-94 (sarjo01) Bug 63642
**                Add order by consid1 to query to get constraints in correct
**                dependency order
**	9-Nov-95 (johna)
**		Fix bug 72176 - only print the constraints for the tables
**		requested - change xfconstraints to take a list of tables
**		from the caller. Note that we cannot call xffilltables again
**		as this would generate a second "There are 4 tables owned ..
**		message.
**	15-Nov-95 (kch)
**		Supplement to fix for bug 72176 - only write the header
**		if one or more of the requested tables is constrained.
**		Previously, the header would be written if the database
**		contained any constrained tables, regardless of whether
**		they were requested.
**	09-Dec-96 (i4jo01)
**		Eliminate trailing spaces from table name picked up from 
**		system catalog. Use string compare function to check
**		that lengths match between requested table and system
**		catalog stored table.
**	28-Jul-1998 (nicph02)
**		Bug 89439: Side effect of bug 63642 fix (order by consid1). The
**              iiintegrity table can store constraints with identical names
**              that belong to different tables. Added a join on iirelation to
**              make sure we select constraints of the same table.
**	23-Jul-1999 (kitch01)
**		Bug 97600. A variation of 89439. Identical constraints for 
**		indentical tables but with different schema names, results
**		in duplicate constraint definitions in copy.in. Rework the SQL
**		statement to use the system catalog tables rather than the view
**		iiconstraints as we were previously joining the view to the 
**		tables it was based on.
**	07-jul-2000 (gupsh01) 
**		Added code to handle user name indexes in constraints 
**		definition. 
**      15-Oct-2001 (hanal04) Bug 105924 INGSRV 1566
**         Modified call to xfaddloc() so that ip->location_list is
**         dynamically allocated and populated by xfaddloc(). Once
**         we are finished using ip->location_list we must also
**         free the allocated memory. This prevents array out of
**         bounds errors which lead to SIGSEGVs and memory corruption.
**	19-mar-2002 (gupsh01)
**	    The xf_parse_constraint_text is now modified so that the return
**	    string is
**		passed into the function parameter.
**	23-aug-2001 (fruco01)
**	        Call xfwrite_id() instead of xfwrite() to cater for delimited
**		names in the ALTER statement when generating copyin 
**		script. Bug # 105380.
**	7-Jul-2004 (schka24)
**	    Delete macaccess stuff.
**	15-Apr-2005 (thaju02)
**	    Add tlist param to xffillindex.
**	30-Nov-2006 (kibro01) b117217
**	    Get page_size in case user has changed it from default and
**	    pass in a bool into constraint parser to work out whether to
**	    add the "WITH" clause.
**	14-Jun-2007 (kibro01) b118503
**	    If a constraint is added directly using alter table, any page_size
**	    modification is in the qrytext.  If it is added in a create table
**	    statement but then altered using a direct modify on the constraint,
**	    the change in page size is not in qrytext.  We therefore need to
**	    check whether the with clause contains "page_size" before deciding
**	    whether to add another one.
**      14-Aug-2007 (hanal04) Bug 118949
**          When "WITH (INDEX = " is present it is not valid to have a 
**          page_size clause.
**          Also corrected the page_size value when used. We should be
**          using the page_size of the constraint relation not the
**          base table upo which the constraint is based. If the constraint is
**          using a user table/index we'll get a null page_size. It won't be
**          used but just in case we need a value use the base table's
**          page_size.
**       2-Nov-2007 (hanal04) Bug 119404
**          Do not add a WITH PAGE_SIZE clause until we have processed the
**          last segment of query text in the sequence for a given constraint.
**	28-Dec-2007 (kibro01) b119659
**	    Add in the page_size inside brackets if there are brackets for
**	    something else, and determine whether there is already a WITH
**	    clause, since then we don't want another WITH.
**	18-Jan-2007 (kibro01) b119778
**	    Test for INDEX = BASE TABLE STRUCTURE or NO INDEX and don't add
**	    page size after those constructs.
**	16-Oct-2008 (kiria01) b97600
**	    The LOJ iirelation in the select needs constraining back to the
**	    same owner as the inner iirelation otherwise duplicates will ensue.
**	04-Feb-2009 (thaju02)
**	    Always specify "with page_size=..." clause. (B99758)
**      24-Aug-2009 (wanfr01) B122319
**	    Pass NULL to xffillindex to indicate to fetch all constraint index
**	    information
*/

void
xfconstraints(output_flags, tlist)
i4	    output_flags;
XF_TABINFO  *tlist;
{
EXEC SQL BEGIN DECLARE SECTION;
    char	constraint_name[DB_MAXNAME + 1];
    char	table_name[DB_MAXNAME + 1];
    char	schema_name[DB_MAXNAME + 1];
    char	constraint_type[2];
    char	text_segment[XF_INTEGLINE + 1];
    char	*new_text = 0;
    i4		text_sequence;
    i4		max_sequence;
    i4          consid1;
    XF_TABINFO  *tp;
    i4		cnt;
    bool	process_table = FALSE;
    i4          cpage_size;
EXEC SQL END DECLARE SECTION;

    TXT_HANDLE		*tfd = NULL;
    XF_TABINFO	*ilist;
    XF_TABINFO  *ip;
    char list[XF_INTEGLINE + 1];
    i4		ccount = 0;
    char buffer[XF_INTEGLINE + XF_MAX_INTEGBUF + 1];
    bool first_addition;
    char	*ps_text = ERx("page_size");
    char        *wi_text = ERx("WITH (INDEX = ");
    char	*wi_no_text = ERx("WITH NO INDEX");
    char        *with_alone_text = ERx(" WITH ");
    bool extra_bracket;
    char	*where_with = NULL;

    ilist = xffillindex(1, NULL);
    /* for each of the indexes obtained above 
    ** if these are multilocational then create a 
    ** list of locations and store in the ilist
    */
    for (ip = ilist; ip != NULL; ip = ip->tab_next)
    {
	if (ip->otherlocs[0] == 'Y')
	    xfaddloc( ip->name, ip->owner, TRUE, &ip->location_list);
    }

    if ( !With_65_catalogs)
	return;

    /* Need to establish max sequence number for a given sequence */
    EXEC SQL DECLARE GLOBAL TEMPORARY TABLE session.cqrytext AS
        SELECT i.consname, r.relid, r.relowner, max(q.seq+1) as "max" 
        FROM iirelation r,
                iiqrytext q, iiintegrity i LEFT JOIN iirelation p
             ON ((p.relid = concat('$', i.consname) OR
                 p.relid = i.consname) AND
                 (locate(p.relid, '$') = 1))
        WHERE  (r.relowner = :Owner OR '' = :Owner) and
                i.inttabbase=r.reltid and
                i.inttabidx=r.reltidx and i.intqryid1=q.txtid1 and
                i.intqryid2=q.txtid2 and q.mode=20 and i.consflags!=0 and
                i.intseq=0
        GROUP BY i.consname, r.relid, r.relowner
        ON COMMIT PRESERVE ROWS WITH NORECOVERY;

    EXEC SQL SELECT i.consname, r.relid, r.relowner,
		charextract('UC R    P', mod(i.consflags, 16)),
		text_sequence=q.seq +1, m."max", q.txt, i.consid1,
		ifnull(p.relpgsize, r.relpgsize)
	INTO :constraint_name, :table_name, :schema_name,
		:constraint_type,
		:text_sequence, :max_sequence, :text_segment, :consid1,
		:cpage_size
	FROM iirelation r, session.cqrytext m,
             iiqrytext q, iiintegrity i LEFT JOIN iirelation p
             ON ((p.relid = concat('$', i.consname) OR
                 p.relid = i.consname) AND
                 (locate(p.relid, '$') = 1) AND
		 i.inttabbase=p.reltid AND
		 i.inttabidx=p.reltidx)
	WHERE  (r.relowner = :Owner OR '' = :Owner) and
               ((i.consname = m.consname) and
                (m.relid = r.relid) and
                (m.relowner = r.relowner)) AND
		i.inttabbase=r.reltid and
		i.inttabidx=r.reltidx and i.intqryid1=q.txtid1 and
		i.intqryid2=q.txtid2 and q.mode=20 and i.consflags!=0 and i.intseq=0
	ORDER BY i.consid1, r.relowner, r.relid, i.consname, text_sequence;

    EXEC SQL BEGIN;
    {

	if (tfd == NULL)
	{
	    tfd = xfreopen(Xf_in, TH_IS_BUFFERED);
	    xfsetlang(tfd, DB_SQL);
	}

	/* check to see if this table should be processed */
	process_table = FALSE;
	xfread_id(table_name);
	for (tp = tlist; tp != NULL; tp = tp->tab_next) {
		if (STcompare(tp->name,				
				table_name) == 0) {
			process_table = TRUE;
			break;
		}
	}

	if (process_table) 
	{
	    if (text_sequence == 1)
	    {
		/* new constraint. */

		if (ccount == 0)
	    	xfwritehdr(CONSTRAINTS_COMMENT);  

		if (ccount != 0)
		    xfflush(tfd);

		xfread_id(schema_name);
		xfread_id(constraint_name);
		xfread_id(table_name);

		/* Do we need a SET USER command? */
		xfsetauth(tfd, schema_name);

		if ( XF_DROP_INCLUDE == ( output_flags &
		     (XF_DROP_INCLUDE|XF_TAB_CREATEONLY|XF_PRINT_TOTAL) ) )
		{
		    xfwrite(tfd, ERx("alter table "));
		    xfwrite_id(tfd, table_name);
		    xfwrite(tfd, ERx(" drop "));
		    xfwrite(tfd, ERx("constraint "));
		    xfwrite_id(tfd, constraint_name);
		    xfwrite(tfd, ERx(" restrict "));
		    xfflush(tfd);
		}

		xfwrite(tfd, ERx("alter table "));
		xfwrite_id(tfd, table_name);
		xfwrite(tfd, ERx(" add "));

		/* 
		** If user doesn't explicitly name the constraint, the
		** system generated a name starting with '$'.  So if this is
		** a generated name, don't use it.
		*/
		if (constraint_name[0] != '$')
		{
		    xfwrite(tfd, ERx("constraint "));
		    xfwrite_id(tfd, constraint_name);
		    xfwrite(tfd, ERx(" "));
		}

		++ccount;
	    }
	    /* 
	    ** reformat the text_segment so that we can take care of
	    ** problems with user modifying indexes defined as a 
	    ** part of constraints. 
	    ** Output "with page_size" if the size has been changed 
	    ** from the default (b117217)
	    */
	    first_addition = TRUE;
	    STcopy(text_segment, buffer);
	    if (ilist)
	    {
		xf_parse_constraint_text(text_segment,ilist, buffer,
			&first_addition);
	    }
	    /* If we're going to write the page size, and we have a WITH clause
	    ** in the output already, then get rid of any trailing bracket at
	    ** this stage so we can squeeze the page_size bit inside the 
	    ** brackets, but remember to add them back in again later
	    ** (kibro01) b119659
	    */
	    extra_bracket = FALSE;
	    where_with = STstrindex(buffer, with_alone_text, 0, TRUE);

	    if (where_with &&
                (text_sequence == max_sequence) &&
                !(STstrindex(buffer, wi_text, 0, TRUE)) &&
		!(STstrindex(buffer, ps_text, 0, TRUE)) &&
		buffer[STlength(buffer)-1] == ')')
	    {
		buffer[STlength(buffer)-1] = '\0';
		extra_bracket = TRUE;
	    }

	    xfwrite(tfd, buffer);

	    /* If there's a WITH in the original description, don't add
	    ** another one
	    */
	    if (first_addition && where_with)
		first_addition = FALSE;

	    /* Don't output page_size again if it was in the text
	    ** in iiqrytext (kibro01) b118503
	    ** or if it has NO INDEX or BASE TABLE STRUCTURE b119778
	    */
	    if ((text_sequence == max_sequence) &&
                !(STstrindex(buffer, wi_text, 0, TRUE)) &&
                !(STstrindex(buffer, wi_no_text, 0, TRUE)) &&
		!(STstrindex(buffer, ps_text, 0, TRUE)))
	    {
		writewith(ps_text,cpage_size,&first_addition,tfd);
	    }
	    if (extra_bracket)
	    {
		xfwrite(tfd, ERx(")"));
	    }
	}
    }
    EXEC SQL END;

    EXEC SQL DROP session.cqrytext;

    for (ip = ilist; ip != NULL; ip = ip->tab_next)
    {
        if(ip->location_list != NULL)
            MEfree((PTR)ip->location_list);
    }

    if (tfd != NULL)
	xfclose(tfd);
}
