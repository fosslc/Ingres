/* Copyright (C) 2004 Ingres Corporation*/

#include <compat.h>
#include <st.h>
#include <er.h>
#include <gl.h>
#include <sl.h>
#include <me.h>
#include <iicommon.h>
#include <fe.h>
#include <erui.h>
#include <ui.h>

/* File: uipartdesc.sc - Partition description for TM, XFERDB
**
** Description:
**	This file contains routines for generating and formatting a
**	partition definition.  The definition is suitable for feeding
**	back into CREATE or MODIFY as a partition WITH-clause.
**
**	The copydb / unloaddb utilities attach the partition definition
**	to a MODIFY or possibly a CREATE statement.
**
**	The terminal monitor uses the same partition definition as
**	output for a HELP TABLE on a partitioned table.
**
**	In order to work with the various callers, uipartdesc will
**	expect the caller to supply a string-print routine (given
**	a null-terminated string of any length, append the string to the
**	output stream).  Thus, this file is insulated from any and
**	all I/O requirements.
**
** History:
**	25-Jun-2004 (schka24)
**	    Write (finally).
**	24-Aug-2009 (kschendel) 121804
**	    Need ui.h to satisfy gcc 4.3.
*/


/* Local definitions */

/* For partition location resolution, we need to keep around textual versions
** of the various partition locations.  Since we don't have memory streams
** at this level, define a simple chained allocator for sticking strings
** into.   If the first block on the chain is full, we add another.
** The chain is for freeing them all up at the end.  No smarts here...
*/

typedef struct _LOC_TEXT
{
    struct _LOC_TEXT *next;		/* Next block, or null */
    i4 size;				/* Total text size of this block */
    i4 putter;				/* Offset in text[] for next string */
    char text[1];			/* Text strings start here */
} LOC_TEXT;

#define LOC_TEXT_SIZE	5000		/* More or less arbitrary */


/* For each physical partition, we need to know where it's located (as
** a string), and whether it's been "resolved" (ie we've attached a
** location= string to some logical partition so that the physical
** partition's location is determined).
** There will be an extra entry [physical_partitions] so that we know
** the location info for the master table.
*/
typedef struct _PP_LOC
{
    char *loctn;			/* Actual partition location */
    bool resolved;			/* TRUE if PP loctn determined by LP */
} PP_LOC;


/* For each logical partition dimension, we keep around some info:
** The number of times a logical partition occurs == the product of
** number-of-partitions in all outer dimensions, or 1 at the outermost
** dimension;  the stride == number of physical partitions per logical
** partition == product of nparts in all inner dimensions, or 1 at the
** innermost dimension;  and a pointer to an array of locations (as
** string pointers), one entry for each logical partition in the dimension.
** (A null pointer means that the logical partition has no location=
** assignment.)
*/
typedef struct _DIM_LOC
{
    i4 nparts;				/* # of logical partitions */
    i4 noccur;				/* # of occurrences of each LP */
    i4 stride;				/* Stride, PP's per LP */
    char **lp_loctns;			/* Location for each LP */
} DIM_LOC;


/* It's convenient to have a little header structure for location
** resolution, pointing to the various areas we have around.
*/
typedef struct _LOC_HDR
{
    PP_LOC *pp_loc_array;		/* Array of physical partition locs */
    DIM_LOC *dim_array;			/* Array of dimension infos */
    LOC_TEXT *text_stuff;		/* Base of text holders */
} LOC_HDR;
    

/* Forward procedure definitions */

static char *put_string(LOC_HDR *loc_header,
	char *str);


static void resolve_locations(char *table_name,
	char *table_owner,
	i4 ndims,
	LOC_HDR *loc_hdr,
	void (*write_string)(void *,char *),
	void *write_cb);

/* Name: IIUIpd_PartitionDesc - Generate a partition description
**
** Description:
**	This routine generates a partition definition for a specified
**	table as text.  The definition is in the proper syntax for
**	supplying to a CREATE or MODIFY with-clause.  If the table has
**	no partitions, NOPARTITION is generated;  otherwise we generate:
**	 PARTITION = ( partition-dimension )
**	for a singly dimensioned partitioning scheme, or
**	 PARTITION = ( ( partition-dimension
**	 ) SUBPARTITION (
**	   partition-dimension
**	  ...
**	 ) )
**	for a multiply-dimensioned partitioning scheme.
**
**	A partition-dimension is:
**	rule [ON column, column]
**	followed by logical partition definitions.  For AUTOMATIC and HASH,
**	we'll generate:
**	[n] PARTITION[s] [with location=] for n un-named partitions, or
**	PARTITION name [with location=] for a named partition.  (We don't
**	attempt the n PARTITIONS (name, name, ...) syntax.)
**
**	For LIST and RANGE, we generate a PARTITION line for each logical
**	partition, in one of the following forms:
**	PARTITION [name] VALUES ( value, value, ... ) [with location=]
**	PARTITION [name] VALUES ( (valpart,valpart), ... ) [with location=]
**	PARTITION [name] VALUES oper value [with location=]
**	PARTITION [name] VALUES oper (valpart, valpart) [with location=]
**
**	The first two being LIST styles and the last two being RANGE
**	styles.  Extra parens are needed when a multicolumn value is
**	being printed.
**
**	Everything is indented by a tab except the first line.
**
**	We don't bother querying the catalogs to find out how many
**	dimensions, columns, values per partition, etc there are.
**	Thus, there's a fair amount of state to manipulate as we move
**	from query row to query row.
**
**	The caller can specify that the with location= stuff is not
**	of interest.  Figuring out LOCATION= is a significant bit of
**	extra work, so we skip it if the caller doesn't care.
**
**	No transaction control is done here.  The caller needs to do
**	commits if needed.
**
**	The caller must supply an output routine of the form:
**	    void write_string(void *cb, char *str);
**	Which writes a null-terminated string.  "cb" is any convenient
**	control block that the caller uses to keep track of things;  it's
**	passed through from the IIUIpd_PartitionDesc call.
**
** Inputs:
**	table_name		The table name
**	table_owner		The table owner
**	ndims			Number of partitioning dimensions
**	do_locations		Caller wants to see location stuff
**	write_string		Caller supplied output routine
**	write_cb		Caller supplied control block for write_string
**
** Outputs:
**	None (other than via write_string)
**
** History:
**	25-Jun-2004 (schka24)
**	    Written.
**	04-Aug-2004 (schka24)
**	    I forgot to null out the pointers I added for location resolution;
**	    mefree was not amused.
**  18-Aug-2009 (drivi01)
**		Clean up precedence warnings in effort to port to 
**		Visual Studio 2008.
*/

void
IIUIpd_PartitionDesc(table_name, table_owner, ndims, do_locations,
	write_string, write_cb)

	exec sql begin declare section;
	char *table_name, *table_owner;
	exec sql end declare section;
	i4 ndims;
	bool do_locations;
	void (*write_string)(void *,char *);
	void *write_cb;

{

    bool doing_list;			/* TRUE when outputting LIST values */
    bool need_comma;			/* TRUE for comma after partition */
    bool quote_val[32+1];		/* Entry [col_seq] is TRUE if need to
					** quote partitioning values
					*/
    char dim_rule[10+1];		/* Dimension's partitioning rule */
    char dlm_col_name[DB_MAXNAME+10+1];	/* Processed (delimited) column name */
    char str[100];			/* Generic string-print area */
    char *last_loctn;			/* Previous location string */
    char *lp_loctn;			/* Current location string */
    i4 last_partseq;			/* Previous partition sequence */
    i4 ncols;				/* Partitioning # columns this dim */
    i4 nparts;				/* Logical partitions this dim */
    i4 unnamed_nparts;			/* Number of unnamed partitions seen */
    LOC_HDR loc_header;			/* Location resolving info */
    LOC_TEXT *loc_next;			/* Location text holder pointer */

    exec sql begin declare section;
    char column_datatype[24+1];		/* Column datatype string */
    char oper[8+1];			/* Testing-oper or DEFAULT */
    char thing_name[DB_MAXNAME+1];	/* Rule, column name, or partition name */
    char value[1500+1];			/* A partitioning value */
    i2 value_null;			/* Null indicator for value */
    i4 col_seq;				/* Column sequencer */
    i4 dim;				/* Current dimension */
    i4 errnum;				/* Query error number */
    i4 part_seq;			/* Logical partition sequencer */
    i4 row_type;			/* 1=dimension, 2=ON-cols, 3-values */
    i4 val_seq;				/* Unhelpful value sequencer */
    exec sql end declare section;

    if (ndims == 0)
    {
	(*write_string)(write_cb, "nopartition");
	return;
    }

    loc_header.text_stuff = NULL;
    loc_header.dim_array = NULL;
    loc_header.pp_loc_array = NULL;
    if (do_locations)
    {
	resolve_locations(table_name, table_owner, ndims, &loc_header,
			write_string, write_cb);
	if (loc_header.dim_array == NULL)
	    return;
    }

    unnamed_nparts = 0;
    doing_list = FALSE;

    /* Fetch info about each dimension in order.
    ** Rowtype: 1 = a dimension row, 2 = an on-column row, 3 = a value
    ** fragment or (logical) partition row.
    ** varchar conversions are for blank stripping.
    */
    exec sql repeated select 1 as row_type, dimension, varchar(partition_rule),
	    partitioning_columns as col_seq, ' ',
	    logical_partitions as part_seq,
	    0 as val_seq, ' ', ' '
	into :row_type, :dim, :thing_name,
	    :col_seq, :column_datatype,
	    :part_seq,
	    :val_seq, :oper, :value:value_null
	from iidistschemes
	where table_name = :table_name and table_owner = :table_owner
    union all
	select 2 as row_type, dimension, varchar(column_name),
	    column_sequence, varchar(column_datatype),
	    0,
	    0, ' ', ' '
	from iidistcols
	where table_name = :table_name and table_owner = :table_owner
    union all
	select 3 as row_type, dimension, varchar(partition_name),
	    column_sequence, ' ',
	    logical_partseq,
	    value_sequence, varchar(operator), varchar(value)
	from iilpartitions
	where table_name = :table_name and table_owner = :table_owner
    order by dimension, row_type, part_seq, val_seq, col_seq;
    exec sql begin;
	if (row_type == 1)
	{
	    /* Start of a new dimension.  Wrap up the old one (if any). */
	    if (doing_list)
	    {
		/* Wrap up the closing ) of a VALUES (list), plus any
		** location= that's needed
		*/
		(*write_string)(write_cb, ")");
		if (do_locations && last_loctn != NULL)
		{
		    STprintf(str," with location=%s", last_loctn);
		    (*write_string)(write_cb, str);
		}
		(*write_string)(write_cb, "\n");
		doing_list = FALSE;
	    }
	    if (unnamed_nparts > 0)
	    {
		/* Flush out a pending "N partitions" */
		if (need_comma)
		    (*write_string)(write_cb, ",");
		STprintf(str, "\n\t  %d partition%s", unnamed_nparts,
			unnamed_nparts > 1 ? "s" : "");
		(*write_string)(write_cb, str);
		if (do_locations && last_loctn != NULL)
		{
		    STprintf(str," with location=%s", last_loctn);
		    (*write_string)(write_cb, str);
		}
		(*write_string)(write_cb, "\n");
		unnamed_nparts = 0;
	    }
	    else if (STcasecmp(dim_rule, "AUTOMATIC") == 0
		  || STcasecmp(dim_rule, "HASH") == 0)
	    {
		/* Flush end of last (named) partition line */
		(*write_string)(write_cb, "\n");
	    }

	    /* Introduce the new dimension */
	    if (dim == 1)
	    {
		if (ndims == 1)
		    (*write_string)(write_cb, "partition = (");
		else
		    (*write_string)(write_cb, "partition = ( (");
	    }
	    else
		(*write_string)(write_cb, "\t) subpartition (\n\t ");
	    (*write_string)(write_cb, thing_name);
	    STcopy(thing_name, dim_rule);
	    nparts = part_seq;
	    ncols = col_seq;
	    need_comma = FALSE;
	    doing_list = FALSE;
	    last_partseq = -1;
	    last_loctn = NULL;
	}
	else if (row_type == 2)
	{
	    /* Start or continue an ON column-list */
	    if (col_seq > 32)
	    {
		/* Hard-wired limit, don't overflow quote_val */
		(*write_string)(write_cb, "ERROR: too many columns");
		EXEC SQL ENDSELECT;
	    }
	    if (col_seq == 1)
		(*write_string)(write_cb, " on ");
	    else
		(*write_string)(write_cb, ", ");
	    /* Deal with potential delimited identifiers! */
	    IIUGxri_id(thing_name, &dlm_col_name[0]);
	    (*write_string)(write_cb, dlm_col_name);
	    quote_val[col_seq] = TRUE;
	    /* Numbers don't need their values quoted */
	    if (STcasecmp(column_datatype, "INTEGER") == 0
	      || STcasecmp(column_datatype, "FLOAT") == 0
	      || STcasecmp(column_datatype, "DECIMAL") == 0
	      || STcasecmp(column_datatype, "MONEY") == 0)
		quote_val[col_seq] = FALSE;
	}
	else if (row_type == 3)
	{
	    /* A logical partition or partition value or value component */
	    if (do_locations)
		lp_loctn = loc_header.dim_array[dim-1].lp_loctns[part_seq-1];
	    if (STcasecmp(dim_rule, "AUTOMATIC") == 0
	      || STcasecmp(dim_rule, "HASH") == 0)
	    {
		/* Hash and auto don't use values.  Look for unnamed
		** partitions and just count them.  A named partition
		** flushes whatever was saved and outputs a partition line.
		** Hash and auto do the newlines in write-behind fashion.
		** Range does the newlines as it goes since we know when
		** it's done.  List does newlines via doing_list.
		** "unnamed, and any of: not doing locations, or this is
		** the first of a possible bunch of unnamed's, or this
		** partition's loctn assignment is the same as the last one."
		*/
		if (STncasecmp(thing_name, "ii", 2) == 0
		  && (! do_locations
		      || unnamed_nparts == 0
		      || ((lp_loctn == NULL && last_loctn == NULL)
			  || (((lp_loctn != NULL) && (last_loctn != NULL))
				&& STcasecmp(lp_loctn,last_loctn) == 0))) )
		{
		    /* Just count up another partition */
		    if (unnamed_nparts == 0 && part_seq > 1)
			need_comma = TRUE;
		    ++ unnamed_nparts;
		}
		else
		{
		    /* Names, or unnamed on a different location */
		    /* Flush out what we might have had so far */
		    if (unnamed_nparts > 0)
		    {
			/* Flush out a pending "N partitions" */
			if (need_comma)
			    (*write_string)(write_cb, ",");
			STprintf(str, "\n\t  %d partition%s", unnamed_nparts,
				unnamed_nparts > 1 ? "s" : "");
			(*write_string)(write_cb, str);
			if (do_locations && last_loctn != NULL)
			{
			    STprintf(str," with location=%s", last_loctn);
			    (*write_string)(write_cb, str);
			}
			unnamed_nparts = 0;
		    }
		    if (STncasecmp(thing_name, "ii", 2) == 0)
		    {
			if (unnamed_nparts == 0 && part_seq > 1)
			    need_comma = TRUE;
			++ unnamed_nparts;
		    }
		    else
		    {
			/* Named partition, output it */
			if (part_seq > 1)
			    (*write_string)(write_cb, ",");
			STprintf(str, "\n\t  partition %s", thing_name);
			(*write_string)(write_cb, str);
			if (do_locations && lp_loctn != NULL)
			{
			    STprintf(str," with location=%s", lp_loctn);
			    (*write_string)(write_cb, str);
			}
		    }
		}
	    }
	    else
	    {
		/* RANGE or LIST.  We do a separate partition line for each
		** logical partition.  These rules need user values too.
		** I screwed up a little writing the value sequences into
		** the catalogs -- they don't reset to 1 for a new logical
		** partition.  We have to look at "partition changed" instead.
		*/

		if (part_seq != last_partseq && col_seq == 1)
		{
		    /* Start of a new logical partition series. */
		    /* Terminate ON-list if very first logical partition row */
		    if (part_seq == 1)
			(*write_string)(write_cb, "\n");
		    /* Finish out pending LIST VALUES (xx) from last time */
		    if (doing_list)
		    {
			/* Wrap up the closing ) of a VALUES (list), plus any
			** location= that's needed
			*/
			(*write_string)(write_cb, ")");
			if (do_locations && last_loctn != NULL)
			{
			    STprintf(str," with location=%s", last_loctn);
			    (*write_string)(write_cb, str);
			}
			(*write_string)(write_cb, ",\n");
			doing_list = FALSE;
		    }
		    (*write_string)(write_cb, "\t  partition ");
		    if (STncasecmp(thing_name, "ii", 2) != 0)
		    {
			(*write_string)(write_cb, thing_name);
			(*write_string)(write_cb, " ");
		    }
		    if (STcasecmp(dim_rule, "LIST") == 0)
		    {
			(*write_string)(write_cb, "values (");
			doing_list = TRUE;
		    }
		    else
		    {
			STprintf(str, "values %s ", oper);
			(*write_string)(write_cb, str);
		    }
		}
		else if (col_seq == 1)
		{
		    /* First part of N'th value, spit out comma */
		    (*write_string)(write_cb, ",");
		}
		/* Write a piece of a value */
		if (STcasecmp(oper, "DEFAULT") == 0)
		{
		    /* DEFAULT is special in that it's never written as a
		    ** composite value (default, default), it's written as
		    ** simply DEFAULT.
		    */
		    if (col_seq == 1)
			(*write_string)(write_cb, "DEFAULT");
		}
		else
		{
		    /* Composite value is in () */
		    if (col_seq == 1 && ncols > 1)
			(*write_string)(write_cb, "(");
		    /* Output the value itself, possibly quoted */
		    if (value_null)
			(*write_string)(write_cb, "NULL");
		    else if (! quote_val[col_seq])
			(*write_string)(write_cb, value);
		    else
		    {
			(*write_string)(write_cb, "'");
			(*write_string)(write_cb, value);
			(*write_string)(write_cb, "'");
		    }
		    /* For composite values, write separator or closer */
		    if (ncols > 1)
		    {
			if (col_seq == ncols)
			    (*write_string)(write_cb, ")");
			else
			    (*write_string)(write_cb, ",");
		    }
		}
		/* For RANGE, if we're finished with the value, end the line */
		if (STcasecmp(dim_rule, "RANGE") == 0 && col_seq == ncols)
		{
		    if (do_locations && lp_loctn != NULL)
		    {
			STprintf(str," with location=%s", lp_loctn);
			(*write_string)(write_cb, str);
		    }
		    if (part_seq < nparts)
			(*write_string)(write_cb, ",");
		    (*write_string)(write_cb, "\n");
		}
	    } /* if dim_rule */
	    last_partseq = part_seq;
	    last_loctn = lp_loctn;
	} /* if on row_type */
    exec sql end;
    exec sql inquire_sql (:errnum = errorno);
    if (errnum != OK)
    {
	STprintf(str, "Error (%d) fetching partition description", errnum);
	(*write_string)(write_cb, str);
	return;
    }

    /* Finish up the flush-last-partition stuff */
    if (doing_list)
    {
	/* Wrap up the closing ) of a VALUES (list), plus any
	** location= that's needed
	*/
	(*write_string)(write_cb, ")");
	if (do_locations && last_loctn != NULL)
	{
	    STprintf(str," with location=%s", last_loctn);
	    (*write_string)(write_cb, str);
	}
	(*write_string)(write_cb, "\n");
	doing_list = FALSE;
    }
    if (unnamed_nparts > 0)
    {
	/* Flush out a pending "N partitions" */
	if (need_comma)
	    (*write_string)(write_cb, ",");
	STprintf(str, "\n\t  %d partition%s", unnamed_nparts,
		unnamed_nparts > 1 ? "s" : "");
	(*write_string)(write_cb, str);
	if (do_locations && last_loctn != NULL)
	{
	    STprintf(str," with location=%s", last_loctn);
	    (*write_string)(write_cb, str);
	}
	(*write_string)(write_cb, "\n");
	unnamed_nparts = 0;
    }
    else if (STcasecmp(dim_rule, "AUTOMATIC") == 0
	  || STcasecmp(dim_rule, "HASH") == 0)
    {
	/* Flush end of last (named) partition line */
	(*write_string)(write_cb, "\n");
    }
    if (ndims > 1)
	(*write_string)(write_cb, "\t) )");
    else
	(*write_string)(write_cb, "\t)");

    /* Return location text holder blocks if we needed any */
    while (loc_header.text_stuff != NULL)
    {
	loc_next = loc_header.text_stuff->next;
	MEfree((PTR) loc_header.text_stuff);
	loc_header.text_stuff = loc_next;
    }
    if (loc_header.dim_array != NULL)
	MEfree((PTR) loc_header.dim_array);
    if (loc_header.pp_loc_array != NULL)
	MEfree((PTR) loc_header.pp_loc_array);

    return;
} /* IIUIpd_PartitionDesc */

/* Name: resolve_locations - Resolve logical partition locations
**
** Description:
**	The system catalogs store location information at the physical
**	partition level.  But, when defining partitions, one specifies
**	locations at the logical partition level.  I didn't bother
**	storing the logical partition location anywhere in the catalogs,
**	and now it's time to pay the price.
**
**	The algorithm for reconstructing the logical partition locations
**	is relatively simple.  One starts at the innermost dimension
**	and works outwards, assigning locations at the lowest dimension
**	possible.
**
**	After processing all dimensions,  we look at the assignments for
**	the outermost dimension that HAS any assignments.  Any location
**	assignments in that dimension which are identical to the master
**	table location can be eliminated.
**
**	Location assignment for a given logical partition works by looking
**	at the various occurrences of the logical partition to see if
**	they are all at the same physical location.  A logical partition's
**	physical location is determined by looking at any physical partition
**	in that logical partition that has not already had its location
**	assigned logically.  (All such physical partitions must necessarily
**	be in the same location, as no lower dimension has overridden
**	location assignments.)  If all physical partitions for some
**	logical partition occurrence have had locations assigned, the
**	logical partition occurrence is irrelevant and is skipped.
**
**	This algorithm will produce a partitioning definition that
**	accurately portrays the actual state of the table (* but see below).
**	It may NOT necessarily be identical to the user's table definition,
**	simply because location assignment is non-unique.  In particular,
**	it's possible for a user to apply location= clauses which are
**	completely overridden by inner dimensions, rendering them useless.
**	There is no way we can reconstruct such pointless location= clauses
**	because there is no catalog that remembers them.
**
**	(*) FIXME FIXME
**	The above claim is true only in the absence of MODIFY t PARTITION p
**	TO REORGANIZE statements.  Explicit partition reorganization can
**	result in a partition location scheme that CANNOT be described
**	in partition definition form.  For example, consider a 2-dimensional
**	partitioning such as:
**	    (HASH ON i 2 PARTITIONS (p1,p2))
**	  SUBPARTITION (HASH ON j 2 PARTITIONS (p3, p4))
**	and then apply MODIFY t PARTITION p1.p3 TO REORGANIZE.  That
**	leaves the first physical partition in the new location and there is
**	no way to describe that.  The only reasonable thing to do is to
**	leave those physical partitions unresolved, and later on return to
**	output a bunch of MODIFY TO REORGANIZE statements (if this is
**	copydb or unloaddb).  That will have to be done eventually, but
**	I'm leaving it as a known issue for now (Jul '04).
**
**	By the way, even with all the whirling around going on here,
**	I'd still rather decipher the locations in some front-end utilities,
**	than make the back-end recompute things after a MODIFY partition
**	TO REORGANIZE...
**
** Inputs:
**	table_name		Name of the partitioned master
**	table_owner		Owner of same
**	ndims			Number of partitioning dimensions
**	loc_header		Address of LOC_HDR to be filled in
**	write_string		See caller, for error messages
**	write_cb		See caller
**
** Outputs:
**	loc_header will point to resolved location stuff via
**	loc_header.dim_array[].lp_loctns[].
**	No return status.  If an error occurs, loc_header.dim_array is
**	left NULL.  The caller can assume that error messages have
**	been issued.
**
** History:
**	6-Jul-2004 (schka24)
**	    Initial version.
*/

static void
resolve_locations(char *table_name, char *table_owner,
	i4 ndims, LOC_HDR *loc_header,
	void (*write_string)(void *,char *), void *write_cb)
{

    bool found;				/* Found-assignments flag */
    char *lp_loctn;			/* Logical partition loctn string */
    char *pp_loctn;			/* Phys partition loctn string */
    char **next_loctn;			/* For assigning loctn arrays */
    char str[100];			/* Random string holder */
    DIM_LOC *dim_array;			/* Base of dimension info array */
    DIM_LOC *dim_ptr;			/* Info for one dimension */
    i4 dim;				/* Dimension number */
    i4 errnum;				/* Ingres sql error number */
    i4 last_npart;			/* Previous nparts */
    i4 lp, pp;				/* Logical/physical partition index */
    i4 noccur;				/* # times LP occurs */
    i4 occ;				/* Occurrence number 1..N */
    i4 phys_nparts;			/* Physical partitions */
    i4 size;				/* Memory allocation size */
    i4 stride;				/* Stride (PPs per LP) */
    PP_LOC *pp_loc_array;		/* Physical partition info */
    STATUS st;

    exec sql begin declare section;
    char loctn[DB_MAXNAME+1];		/* Individual location name */
    i4 loc_seq;				/* Location sequencer */
    i4 nparts;				/* Partition number or count */
    exec sql end declare section;

    loc_header->dim_array = NULL;
    /* Allocate memory for text holders */
    loc_header->text_stuff = (LOC_TEXT *) MEreqmem(0, LOC_TEXT_SIZE,
		FALSE, &st);
    if (st != OK)
    {
	STprintf(str,"No memory (%d) for partition info text",LOC_TEXT_SIZE);
	(*write_string)(write_cb, str);
	return;
    }
    /* (+1 because there's one char in the header */
    loc_header->text_stuff->size = LOC_TEXT_SIZE - sizeof(LOC_TEXT) + 1;
    loc_header->text_stuff->next = NULL;
    loc_header->text_stuff->putter = 0;

    /* Allocate memory for dimension array and logical partition string
    ** pointers.  We need to know how many total logical partitions there
    ** are, do a little select for this.
    */
    exec sql select sum(logical_partitions)
	into :nparts
	from iidistschemes
	where table_name = :table_name and table_owner = :table_owner;
    exec sql inquire_sql (:errnum = errorno);
    if (errnum != OK)
    {
	STprintf(str, "Error (%d) fetching iidistschemes", errnum);
	(*write_string)(write_cb, str);
	return;
    }
    size = ndims * sizeof(DIM_LOC) + nparts * sizeof(char *);
    dim_array = (DIM_LOC *) MEreqmem(0, size,
		TRUE, &st);
    if (st != OK)
    {
	STprintf(str,"No memory (%d) for partition loc info",size);
	(*write_string)(write_cb, str);
	return;
    }
    /* Lay out LP location string pointers after dimensions.
    ** We asked MEreqmem to clear out all the pointers to null.
    ** Note: catalog dimensions are 1-origin, we want 0-origin.
    */
    noccur = 1;
    next_loctn = (char **) ((PTR) dim_array + ndims * sizeof(DIM_LOC));
    exec sql select dimension-1, logical_partitions
	into :dim, :nparts
	from iidistschemes
	where table_name = :table_name and table_owner = :table_owner
	order by dimension;
    exec sql begin;
	dim_array[dim].nparts = nparts;
	dim_array[dim].noccur = noccur;
	dim_array[dim].lp_loctns = next_loctn;
	next_loctn = next_loctn + nparts;
	noccur = noccur * nparts;
    exec sql end;
    exec sql inquire_sql (:errnum = errorno);
    if (errnum != OK)
    {
	STprintf(str, "Error (%d) fetching iidistschemes", errnum);
	(*write_string)(write_cb, str);
	return;
    }
    /* Stride is computed inside to outside */
    stride = 1;
    for (dim = ndims - 1; dim >= 0; --dim)
    {
	dim_array[dim].stride = stride;
	stride = stride * dim_array[dim].nparts;
    }

    /* The other thing we need from the catalogs is the actual location
    ** info for the physical partitions, including the master.
    ** By asking for the PP's in reverse order, we get the master first
    ** (since # of physical partitions = max PP number + 1), and thus
    ** can avoid one pre-select for the number of PP's.
    ** "phys_partitions" uniquely identifies a partition (or master) and
    ** there's no need to pull out the table name or reltid/reltidx.
    ** The varchar() conversions do trailing blank stripping.
    */
    pp_loc_array = NULL;
    last_npart = -1;
    str[0] = '(';
    str[1] = '\0';
    exec sql repeated
	select phys_partitions, 0 as loc_sequence, varchar(location_name)
	into :nparts, :loc_seq, :loctn
	from iitables
	where table_reltid = (select table_reltid
		from iitables
		where table_name = :table_name and table_owner = :table_owner)
	    and table_type in ('T', 'P')
	union all
	select t.phys_partitions, m.loc_sequence, varchar(m.location_name)
	from iitables t, iimulti_locations m
	where t.table_name = m.table_name and t.table_owner = m.table_owner
	    and table_reltid = (select table_reltid
		from iitables
		where table_name = :table_name and table_owner = :table_owner)
	    and table_type in ('T', 'P')
	order by phys_partitions desc, loc_sequence asc;
    exec sql begin;
	if (pp_loc_array == NULL)
	{
	    phys_nparts = nparts;
	    /* +1 because we need entry [nparts] for the master */
	    size = (nparts+1) * sizeof(PP_LOC);
	    pp_loc_array = (PP_LOC *) MEreqmem(0, size, FALSE, &st);
	    if (st != OK)
	    {
		STprintf(str,"No memory (%d) for physical partition loc info",size);
		(*write_string)(write_cb, str);
		exec sql endselect;
	    }
	}
	/* If we're seeing a new PP, finish off the old one if any */
	if (loc_seq == 0 && last_npart != -1)
	{
	    strcat(str,")");
	    pp_loc_array[last_npart].loctn = put_string(loc_header, str);
	    if (pp_loc_array[last_npart].loctn == NULL)
	    {
		STprintf(str,"No memory (%d) for physical partition loc string",LOC_TEXT_SIZE);
		(*write_string)(write_cb, str);
		pp_loc_array = NULL;
		exec sql endselect;
	    }
	    pp_loc_array[last_npart].resolved = FALSE;
	    str[0] = '(';
	    str[1] = '\0';
	}
	/* Append this location name to the loctn string we're building. */
	if (loc_seq > 0)
	    strcat(str,",");
	strcat(str,loctn);
	last_npart = nparts;
    exec sql end;
    exec sql inquire_sql (:errnum = errorno);
    if (errnum != OK || pp_loc_array == NULL)
    {
	STprintf(str, "Error (%d) fetching physical partition locations", errnum);
	(*write_string)(write_cb, str);
	return;
    }
    /* Finish up the last location string */
    strcat(str,")");
    pp_loc_array[last_npart].loctn = put_string(loc_header, str);
    if (pp_loc_array[last_npart].loctn == NULL)
    {
	STprintf(str,"No memory (%d) for physical partition loc string",LOC_TEXT_SIZE);
	(*write_string)(write_cb, str);
	pp_loc_array = NULL;
	return;
    }
    pp_loc_array[last_npart].resolved = FALSE;

    /* All the information is finally in place.  Assign locations to logical
    ** partitions from the inside out, as described in the intro.
    ** By the way, this is coded more for clarity (such as it is) rather
    ** than efficiency;  the most common case will be a singly dimensioned
    ** partitioning with straightward location assignments, if any.
    */
    for (dim_ptr = &dim_array[ndims-1];
	 dim_ptr >= &dim_array[0];
	 -- dim_ptr)
    {
	/* Look at each logical partition in this dim */
	for (lp = 0; lp < dim_ptr->nparts; ++lp)
	{
	    lp_loctn = NULL;
	    /* Look at all occurrences of this logical partition */
	    for (occ = dim_ptr->noccur; occ > 0; --occ)
	    {
		/* Look at PP's making up this LP occurrence and get
		** any (ie the first) unresolved PP's location string.
		** There are (stride) pp's in the lp.
		*/
		pp_loctn = NULL;
		stride = dim_ptr->stride;
		pp = stride * (lp + (occ-1) * dim_ptr->nparts);
		do
		{
		    if (! pp_loc_array[pp].resolved)
		    {
			pp_loctn = pp_loc_array[pp].loctn;
			break;
		    }
		    ++ pp;
		} while (--stride > 0);
		/* If the logical partition occurrence was entirely resolved
		** already, just move on.  Otherwise see if this occurrence
		** lives at the same location as the previous one.
		*/
		if (pp_loctn != NULL && lp_loctn != NULL
		  && STcasecmp(pp_loctn, lp_loctn) != 0)
		    break;
		/* Remember LP location string if first one */
		if (lp_loctn == NULL)
		    lp_loctn = pp_loctn;
	    }
	    /* If we made it through all occurrences without breaking out,
	    ** assign our chosen location string to the logical partition,
	    ** and also mark all PP's covered by the LP as "resolved".
	    */
	    if (occ == 0 && lp_loctn != NULL)
	    {
		dim_ptr->lp_loctns[lp] = lp_loctn;
		for (occ = dim_ptr->noccur; occ > 0; --occ)
		{
		    stride = dim_ptr->stride;
		    pp = stride * (lp + (occ-1) * dim_ptr->nparts);
		    do
		    {
			/* FIXME to address the issue mentioned in the
			** intro, we probably should only mark "resolved"
			** those unresolved pp's where loctn matches lp_loctn.
			*/
			pp_loc_array[pp].resolved = TRUE;
			++ pp;
		    } while (--stride > 0);
		}
	    } /* if resolved */
	} /* for each lp */

    } /* for each dim */

    /* Now go thru the dimensions, outer to inner, looking for the
    ** first one with any assignments.  We can erase any location
    ** assignments at that dimension that match the master table.
    ** This is not necessary, it simply cleans up the output a bit.
    */
    pp_loctn = pp_loc_array[phys_nparts].loctn;
    found = FALSE;
    for (dim_ptr = &dim_array[0];
	 dim_ptr <= &dim_array[ndims-1];
	 ++ dim_ptr)
    {
	/* Look at each logical partition in this dim */
	for (lp = 0; lp < dim_ptr->nparts; ++lp)
	{
	    if (dim_ptr->lp_loctns[lp] != NULL)
	    {
		found = TRUE;
		if (STcasecmp(dim_ptr->lp_loctns[lp], pp_loctn) == 0)
		    dim_ptr->lp_loctns[lp] = NULL;
	    }
	}
	/* Don't look at more dims if assignments in this dim */
	if (found)
	    break;
    }

    loc_header->dim_array = dim_array;
    loc_header->pp_loc_array = pp_loc_array;
    return;
} /* resolve_locations */

/* Name: put_string
**
** Description:
**	This routine is used to stash a location string into the text
**	holder area.  If the current text holder is full, we'll grab
**	another.  We assume that the longest possible location string is
**	not longer than the size of a text holder block (else an error
**	is returned).
**
** Inputs:
**	loc_header			Location resolver stuff header
**	    .text_stuff			Current text holder
**	str				String to stash
**
** Outputs:
**	Returns address of stashed string, or NULL if memory alloc error
**
** History:
**	6-Jul-2004 (schka24)
**	    Initial version.
*/

static char *
put_string(LOC_HDR *loc_header, char *str)
{

    char *result;
    i4 len;				/* Including trailing null */
    LOC_TEXT *new_th;
    LOC_TEXT *th;			/* Text holder block */
    STATUS st;

    th = loc_header->text_stuff;
    len = STlength(str) + 1;
    if (th->putter + len > th->size)
    {
	/* Won't fit, add a new block */
	if (len > LOC_TEXT_SIZE - sizeof(LOC_TEXT))
	    return (NULL);
	new_th = (LOC_TEXT *) MEreqmem(0, LOC_TEXT_SIZE, 0, &st);
	if (st != OK)
	    return (NULL);
	new_th->next = th;
	loc_header->text_stuff = new_th;
	th = new_th;
	/* (+1 because there's one char in the header */
	th->size = LOC_TEXT_SIZE - sizeof(LOC_TEXT) + 1;
	th->putter = 0;
    }
    result = &th->text[th->putter];
    STcopy(str,result);
    th->putter += len;
    return (result);
} /* put_string */
