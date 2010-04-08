/* Copyright 2004, Ingres Corporation*/

#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<gl.h>
#include	<sl.h>
#include	<st.h>
#include	<cm.h>
#include	<sl.h>
#include	<mh.h>
#include	<tm.h>
#include	<iicommon.h>
#include	<hshcrc.h>
#include	<adf.h>
#include	<ulf.h>
#include	<adfint.h>
#include	<adulcol.h>
#include	<aduucol.h>
#include	<aduint.h>

/*
** Name: adtwhichpart.c - Partition Determination functions
**
** Description:
**	This file contains routines used to determine which partition
**	of a horizontally partitioned table a particular row belongs to.
**	Four row distribution schemes are available: automatic (data
**	independent), hash, list, and range.
**
**	Automatic partitioning doesn't depend on the row data at all.
**	We return what is essentially a random number.  (Round-robin
**	would work just as well, except that we don't want to save any
**	state.)
**
**	Hash partitioning takes the distribution column(s), hashes them,
**	and extracts a partition number from the hash.  The extraction
**	simply uses the MOD function (hash mod number-of-partitions).
**
**	Range and list distributions depend on a set of value-to-
**	partition mappings specified by the DBA at table definition time.
**	List partitioning compares the distribution column(s) to each
**	specified value in turn, and when a match is found, that value's
**	partition is used.  Range partitioning compares the distribution
**	column(s) to each value looking for a value range that the
**	row falls into.  List partitioning also specifies a default
**	partition that is used if no match is found.  Range partitioning
**	does not use a default;  the partition definition assures that
**	the given ranges cover the entire data value domain without
**	overlaps or gaps.
**
**	Any one of these schemes may apply to a partitioning dimension.
**	There is also a driver routine which walks through the
**	dimensions, getting the partition sequence for each, and puts
**	it all together into a physical partition number.
**
**	It is important to recognize the difference between a partition
**	*sequence* and a partition *number*.  The former applies to one
**	dimension, while the latter is a physical partition identifier.
**	Additionally, partition sequences are one-origined (like Fortran!),
**	while physical partition numbers are zero-origined.
**	(Partition index might be a better term than partition sequence,
**	by analogy to array indexing;  but index means so many things
**	already...)
**
**	The partition determiners understand about the layout of a
**	partition definition in memory.  In order to figure out a
**	(composite) partitioning value, there is an array of DB_PART_LIST
**	descriptors, one per column.  For LIST and RANGE, there is
**	also a value-to-partition mapping table (a.k.a. breaks table),
**	which is a DB_PART_BREAKS structure.
**
**	The DB_PART_LIST entries give the data types and offsets
**	for each component of a value, while the DB_PART_BREAKS entries
**	give the comparison values, operators, and mapped-to partition
**	numbers.  (By the way, the breaks table has pointers to internal
**	format values, and pointers to uninterpreted text values.  We
**	don't need, look at, or touch in any way the uninterpreted
**	text value pointers.  They are only there for partition maintenance
**	operations, catalog reading and updating, and such.)
**
**	ATTENTION: Calling the AUTOMATIC determiner will return a
**	different number each time unless one provides a deterministic
**	hash value in adf_autohash. Don't call a partition determiner
**	multiple times without one and expect the same answer,
**	unless you know that there are no AUTOMATIC dimensions.
**
**	The functions defined here are:
**	(For all dimensions:)
**	adt_whichpart_no -- Determine a row's physical partition number
**
**	(For a single dimension:)
**	adt_whichpart_auto -- Determine a row's partition sequence: AUTOMATIC
**	adt_whichpart_hash -- Determine a row's partition sequence: HASH
**	adt_whichpart_list -- Determine a row's partition sequence: LIST
**	adt_whichpart_range -- Determine a row's partition sequence: RANGE
**
**	POSSIBLE for later:  do we want/need similar functions given a
**	partition value instead of a row?  only real benefits would be
**	a) value is probably smaller, and b) value would be aligned,
**	might go slightly faster for ints/floats.
**
** History:
**	5-Jan-2004 (schka24)
**	    Create for the Partitioned Table project.
**	04-Apr-2005 (jenjo02)
**	    Added optional adf_autohash for AUTOMATIC.
**	26-Aug-2009 (kschendel) 121804
**	    Need hshcrc.h to satisfy gcc 4.3.
**
*/


/* Local prototype definitions */


/* Local defines */


/* Module data items defined here, if any */

/* Dispatch table (index by dimension distrule) giving the method for
** determining the partition sequence for a dimension.
** DBDS_RULE_xxx Order Sensitive, must match the rule codes!
*/
ADT_WHICHPART_FUNCP Adt_whichpart_disp[] = {
	adt_whichpart_auto,	/* DBDS_RULE_AUTOMATIC */
	adt_whichpart_hash,	/* DBDS_RULE_HASH */
	adt_whichpart_list,	/* DBDS_RULE_LIST */
	adt_whichpart_range	/* DBDS_RULE_RANGE */
};


/*
** Name: adt_whichpart_auto - Determine a row's partition: AUTOMATIC
**
** Description:
**	This routine is given a row, and returns the appropriate
**	partition sequence for an AUTOMATIC partitioning dimension.
**
**	Of course, automatic partitioning is data-independent, so
**	the contents of the supplied row are completely irrelevant!
**
**	USAGE WARNING: This routine may (probably will) return a 
**	different partition sequence each time you call it unless
**	you pass an u_i8 hash value of your choosing in adf_autohash.
**	Without an adf_autohash, don't call adt_whichpart_auto
**	multiple times for the same row and expect to get the
**	same answer each time!
**
**	The idea behind AUTOMATIC is to simply spread out the rows,
**	sort of like location or disk striping.  Round-robin would be
**	plenty good enough, except that doing it right would require
**	maintaining last-partition-assigned state on disk as well
**	as in memory.  Instead, we'll generate a pseudo-random sequence
**	starting with a date-time value.  The seed value is initialized
**	at the first call.  This technique should work reasonably
**	well even with multiple DBMS servers, as it's likely that they
**	will get different starting seeds.
**
**	The code seen here is deliberately not mutexed.  Normally,
**	updating a shared static variable requires either a mutex or use
**	of the CX compare-and-swap routine.  In our case, though,
**	all that matters is that someone gets a number out -- it really
**	doesn't matter if two threads collide!
**
**	Notice that TMhrnow is used to initialize the seed.  That's not
**	because we actually need high resolution;  it's because the
**	other obvious candidate, TMnow, does all sorts of strange and
**	goofy stuff that I'm not interested in.  I just want a number.
**	
** Inputs:
**	adfcb		ADF session control block
**	    adf_autohash optional pointer to value to hash on
**	dim_ptr		DB_PART_DIM pointer to dimension info
**	row_ptr		Ignored pointer to a row
**	ppartseq	Actually an output
**
** Outputs:
**	Returns the usual DB_STATUS
**	No ADF error possible at present, but adf_errcb would be filled in
**	ppartseq	Points to u_i2 to return the computed partition
**			sequence into (1..nparts).
**
** History:
**	6-Jan-2004 (schka24)
**	    Written.
**	04-Apr-2005 (jenjo02)
**	    If adf_autohash supplied, use it to hash partition 
**	    sequence rather than random number, used by MODIFY
**	    to produce a deterministic partition.
*/

DB_STATUS
adt_whichpart_auto(ADF_CB *adfcb, DB_PART_DIM *dim_ptr,
	PTR row_ptr, u_i2 *ppartseq)
{

    u_i4 thing;				/* A "random" number */

    /* Static state for the automatic number assigner.  Not handled
    ** in a thread-safe manner, see routine header comment.
    */
    static bool inited = FALSE;
    static HRSYSTIME hrtime;		/* Only interested in the tv_sec member */

    if ( adfcb->adf_autohash )
    {
	/* The hash chomper likes an initial seed of -1 */
	thing = -1;
	HSH_CRC32((char*)adfcb->adf_autohash,
			sizeof(*adfcb->adf_autohash),
			&thing);
    }
    else
    {
	if (!inited)
	{
	    hrtime.tv_sec = 1234;		/* Just in case hrtime fails! */
	    (void) TMhrnow(&hrtime);	/* Get a "random" seed */
	    hrtime.tv_sec &= 0x7fffffff;	/* Work with 31-bit LFSR */
	    inited = TRUE;			/* Only need the syscall once */
	}

	/* Use a 31-bit linear feedback shift algorithm to generate the
	** next number (taps at bit positions 0 and 3).  XOR bits 0 and 3,
	** and recirculate into the high order bit, only using 31 bits.
	*/

	thing = hrtime.tv_sec;		/* Get our seed */
	/* Should never be zero, but with unsafe multi-thread updates who knows...
	** Zero doesn't work in the LFSR.
	*/
	if (thing == 0) thing = 1;
	thing = (thing >> 1) | ((thing ^ (thing>>3)) & 1) << 30;
	hrtime.tv_sec = thing;		/* Update with the new value */
    }

    /* The actual partition sequence is 1 .. nparts */
    *ppartseq = (thing % dim_ptr->nparts) + 1;

    return (E_DB_OK);
} /* adt_whichpart_auto */

/*
** Name: adt_whichpart_hash - Determine a row's partition: HASH
**
** Description:
**	This routine is given a row, and returns the appropriate
**	partition sequence for a HASH partitioning dimension.
**
**	There is actually little work to do here.  A hashing function
**	munches the bytes and returns the hash.  Luckily, the hasher
**	can be seeded with the output of a previous call;  thus, we
**	can hash discontiguous areas with multiple hash calls, and get
**	the same answer as we would if it were a contiguous area.
**
**	The hash function call is very low overhead, so we don't bother
**	attempting to discover contiguous areas in the row buffer.  We simply
**	hash each partition distribution column in turn.
**
**	There are a few datatypes that give trouble.  First, we need to
**	do something with NULLs.  The value part of a NULL is meaningless,
**	and hashing it would be a Bad Idea(TM).  Second, there are various
**	flags in a date value that aren't always set the same.  Third,
**	varchar comparison semantics say that trailing blanks should be
**	stripped, so 'foo' and 'foo ' had better hash the same.  There
**	may be issues with other string-ish datatypes as well.
**
**	Fortunately, all these problems have already been met and more-
**	or-less overcome by the hash storage structure code.  There's
**	a "hashprep" function that takes a value, and returns a
**	fixed-up value that is suitable for hashing.  We'll apply the
**	hashprep function to our row components before hashing each one.
**	
** Inputs:
**	adfcb		ADF session control block
**	dim_ptr		Pointer to DB_PART_DIM dimension definition
**	row_ptr		Pointer to a row, with partitioning column values
**			  found at the row_offsets in the dimension part_list
**	ppartseq	Actually an output
**
** Outputs:
**	Returns the usual DB_STATUS
**	Upon error, adf_errcb section of adfcb will be filled in.
**	ppartseq	Points to u_i2 to return the computed partition
**			sequence (1..nparts) into.
**
** History:
**	7-Jan-2004 (schka24)
**	    Written.
*/

DB_STATUS
adt_whichpart_hash(ADF_CB *adfcb, DB_PART_DIM *dim_ptr,
	PTR row_ptr, u_i2 *ppartseq)
{

    char buffer[DB_MAXSTRING + DB_CNTSIZE + 2];  /* Where to put hashprepped
					** value.  *Note* large, about 32K
					*/
    DB_DATA_VALUE dvprepped;		/* Points to value after hashprep */
    DB_DATA_VALUE dvrow;		/* Points to value in row */
    DB_PART_LIST *part_list;		/* Pointer to col descriptor array */
    DB_STATUS status;			/* The usual routine call status */
    i4 length;				/* Length of data item */
    PTR col_ptr;			/* Pointer to column within row */
    u_i4 hashval;			/* The result hash value */

    /* The hash chomper likes an initial seed of -1 */
    hashval = -1;

    /* Loop over partition columns, feed each one through the hasher */
    for (part_list = &dim_ptr->part_list[0];
	 part_list < &dim_ptr->part_list[dim_ptr->ncols];
	 ++part_list)
    {
	col_ptr = row_ptr + part_list->row_offset;
	length = part_list->length;
	/* Skip hashprep for a few possibly-common datatypes where we
	** know for sure that hashprep doesn't do anything: non-nullable
	** integer, float, money, char.  (There are others for which
	** hashprep is a no-op, but these are the obvious ones.)
	*/
	if (part_list->type != DB_INT_TYPE && part_list->type != DB_FLT_TYPE
	  && part_list->type != DB_MNY_TYPE && part_list->type != DB_CHA_TYPE)
	{
	    /* Need to hashprep. */
	    /* Note: depending on how the hashprep function is implemented,
	    ** it may or may not copy the value to a new area.  We have
	    ** to give it a buffer to place the value copy, but it may
	    ** choose to update the result db-data-value instead.
	    */
	    dvrow.db_datatype = part_list->type;
	    dvrow.db_length = length;
	    dvrow.db_prec = part_list->precision;
	    dvrow.db_data = col_ptr;
	    dvprepped.db_length = sizeof(buffer);
	    dvprepped.db_data = &buffer[0];
	    status = adc_hashprep(adfcb, &dvrow, &dvprepped);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    length = dvprepped.db_length;
	    col_ptr = dvprepped.db_data;
	}
	HSH_CRC32(col_ptr, length, &hashval);
    } /* for */

    /* The actual partition sequence is one origined */
    *ppartseq = (hashval % dim_ptr->nparts) + 1;

    return (E_DB_OK);
} /* adt_whichpart_hash */

/*
** Name: adt_whichpart_list - Determine a row's partition: LIST
**
** Description:
**	This routine is given a row, and returns the appropriate
**	partition sequence for a LIST partitioning dimension.
**
**	LIST partitioning attempts to find a match between the input
**	row, and one of a list of values.  Each value is associated
**	with a partition sequence.  If no match is found, there is
**	(always) a DEFAULT partition that is used.
**
**	The list values are stored in a breaks table, which is sorted
**	into (ascending) value order.  This makes it easy to search.
**	The DEFAULT entry is always the last table entry.  There will
**	be no duplicate values in the breaks table.
**
**	For partitioning purposes, NULL compares equal to NULL.
**	The lower level comparer deals with this, as well as other
**	data type specific peculiarities.
**	
** Inputs:
**	adfcb		ADF session control block
**	dim_ptr		Pointer to DB_PART_DIM dimension definition
**	row_ptr		Pointer to a row, with partitioning column values
**			  found at the row_offsets in the dimension part_list
**	ppartseq	Actually an output
**
** Outputs:
**	Returns the usual DB_STATUS
**	Upon error, adf_errcb section of adfcb will be filled in.
**	ppartseq	Points to u_i2 to return the computed partition
**			sequence (1..nparts) into.
**
** History:
**	7-Jan-2004 (schka24)
**	    Written.
**	17-Feb-2004 (schka24)
**	    Fix duh in binary search, divide by 2 is >>1 not >>2.
*/

DB_STATUS
adt_whichpart_list(ADF_CB *adfcb, DB_PART_DIM *dim_ptr,
	PTR row_ptr, u_i2 *ppartseq)
{

    DB_PART_LIST *part_list_ptr;	/* Pointer to component def entry */
    DB_STATUS status;			/* The usual routine call status */
    i4 break_index;			/* Current break table index */
    i4 hi, lo;				/* Binary search hi/lo limits */
    i4 result;				/* Comparison result */

    /* Binary search for the proper list entry */
    lo = 0;
    hi = dim_ptr->nbreaks - 2;		/* Omit highest entry, it's DEFAULT */
    result = -1;			/* Init to no-match */
    while (hi >= lo)
    {
	break_index = (hi + lo) >> 1;	/* Middle entry */
	status = adt_tupvcmp(adfcb,
			dim_ptr->ncols, dim_ptr->part_list,
			row_ptr, dim_ptr->part_breaks[break_index].val_base,
			&result);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	/* Either we found it, or we move one of the limits */
	if (result == 0)
	    break;
	else if (result < 0)
	    hi = break_index - 1;	/* We're too high, look lower */
	else
	    lo = break_index + 1;	/* Too low, look higher */
    }
    /* If we matched, return the partition sequence.
    ** If we didn't, return the DEFAULT partition sequence.
    */
    if (result != 0)
	break_index = dim_ptr->nbreaks-1;	/* DEFAULT entry index */
    *ppartseq = dim_ptr->part_breaks[break_index].partseq;

    return (E_DB_OK);
} /* adt_whichpart_list */

/*
** Name: adt_whichpart_no - Determine a row's physical partition number
**
** Description:
**	This routine is given a row, and returns the physical partition
**	number that the row falls into.
**
**	The physical partition is a zero-origined number that includes
**	all of the partitioning dimensions.  (As opposed to a partition
**	"sequence" which is a one-origined index into a given dimension.)
**	All that this routine needs to do is evaluate each dimension,
**	multiply by the dimension's stride, and add 'em up.
**
** Inputs:
**	adfcb		ADF session control block
**	part_def	Pointer to DB_PART_DEF partitioning definition
**	row_ptr		Pointer to a row, with partitioning column values
**			  found at the row_offsets in each dimension part_list
**	ppartno		Actually an output
**
** Outputs:
**	Returns the usual DB_STATUS
**	Upon error, adf_errcb section of adfcb will be filled in.
**	ppartno		Points to u_i2 to return the computed physical
**			partition number into.
**
** History:
**	8-Jan-2004 (schka24)
**	    Written.
**	17-feb-04 (inkdo01)
**	    Minor fix to start value of for-loop.
*/

DB_STATUS
adt_whichpart_no(ADF_CB *adfcb, DB_PART_DEF *part_def,
	PTR row_ptr, u_i2 *ppartno)
{

    DB_PART_DIM *dim_ptr;		/* Dimension info pointer */
    DB_STATUS status;			/* The usual routine call status */
    u_i2 partno;			/* Physical partition number */
    u_i2 partseq;			/* Computed sequence for a dimension */

    partno = 0;
    /* Loop thru the dimensions, gathering up the index for each dimension */
    for (dim_ptr = &part_def->dimension[part_def->ndims-1];
	 dim_ptr >= &part_def->dimension[0];
	 --dim_ptr)
    {
	status = (*Adt_whichpart_disp[dim_ptr->distrule])(adfcb, dim_ptr,
			row_ptr, &partseq);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	partno = partno + (partseq - 1) * dim_ptr->stride;
    }
    *ppartno = partno;			/* return the answer */
    return (E_DB_OK);
} /* adt_whichpart_no */

/*
** Name: adt_whichpart_range - Determine a row's partition: RANGE
**
** Description:
**	This routine is given a row, and returns the appropriate
**	partition sequence for a RANGE partitioning dimension.
**
**	Range partitioning looks for a value range that contains the
**	row's partitioning value.  (The ranges must be defined in such a
**	way that every possible value is covered by exactly one range; at
**	present, we do not support a DEFAULT, although it wouldn't be hard.)
**
**	The range endpoints are stored in a breaks table, which is sorted
**	into (ascending) value order.  Each break value is associated with
**	an operator, one of <, <=, >, or >=.  If any break value occurs
**	more than once, it is guaranteed that the first entry for that
**	value has operator < or <=, and the second has operator >= or >.
**	(No gaps, no overlaps.)  This rule makes searching for the
**	proper range pretty simple, even in the presence of seemingly
**	arbitrary comparison operator directions.
**
**	For partitioning purposes, NULL compares equal to NULL,
**	and in some determinate direction to non-NULL values.  (Whether
**	NULLs are less than or greater than values doesn't actually
**	matter;  if you care, go look at the lower level comparer.)
**	The lower level comparer deals with this, as well as other
**	data type specific peculiarities.
**	
** Inputs:
**	adfcb		ADF session control block
**	dim_ptr		Pointer to DB_PART_DIM dimension definition
**	row_ptr		Pointer to a row, with partitioning column values
**			  found at the row_offsets in the dimension part_list
**	ppartseq	Actually an output
**
** Outputs:
**	Returns the usual DB_STATUS
**	Upon error, adf_errcb section of adfcb will be filled in.
**	ppartseq	Points to u_i2 to return the computed partition
**			sequence (1..nparts) into.
**
** History:
**	7-Jan-2004 (schka24)
**	    Written.
**	17-Feb-2004 (schka24)
**	    Fix duh in binary search, divide by 2 is >>1 not >>2.
**	    Fix nonterminated comment that broke the return value.
*/

DB_STATUS
adt_whichpart_range(ADF_CB *adfcb, DB_PART_DIM *dim_ptr,
	PTR row_ptr, u_i2 *ppartseq)
{

    DB_PART_BREAKS *break_ptr;		/* Pointer to a break table entry */
    DB_PART_LIST *part_list_ptr;	/* Pointer to component def entry */
    DB_STATUS status;			/* The usual routine call status */
    i4 break_index;			/* Current break table index */
    i4 hi, lo;				/* Binary search hi/lo limits */
    i4 oper;				/* Entry's DBDS_OP_xxx opcode */
    i4 result;				/* Comparison result */

    /* The first step is to binary search for the proper break table
    ** entry.  Either we'll find one that matches, or we won't.
    ** If we don't find a match, we'll finish up either on the
    ** entry immediately less than the row, or immediately greater
    ** than the row.
    */
    lo = 0;
    hi = dim_ptr->nbreaks - 1;
    while (hi >= lo)
    {
	break_index = (hi + lo) >> 1;	/* Middle entry */
	status = adt_tupvcmp(adfcb,
			dim_ptr->ncols, dim_ptr->part_list,
			row_ptr, dim_ptr->part_breaks[break_index].val_base,
			&result);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	/* Either we found it, or we move one of the limits */
	if (result == 0)
	    break;
	else if (result < 0)
	    hi = break_index - 1;	/* We're too high, look lower */
	else
	    lo = break_index + 1;	/* Too low, look higher */
    }

    /* Ok, either we landed right on a range boundary (we found a
    ** break table match), or we didn't.
    ** In reading the next part, remember that we're guaranteed that
    ** there are no gaps or overlaps;  the breaks table is in sort;
    ** for equal entries, the entry for < or <= will always precede the
    ** entry for > or >=; and the entire value domain is covered, so
    ** that there is no possibility that a row falls outside of any
    ** valid range.
    */
    break_ptr = &dim_ptr->part_breaks[break_index];
    oper = break_ptr->oper;
    if (result == 0)
    {
	/* We matched a break table entry.  Check its opcode;
	** if it's <= or >=, clearly this is the correct partition.
	** If it's not <= or >=, move one entry opposite to the sense
	** of the entry operator.  In other words, move up one for <,
	** and down one for >.
	** Example: The row value is 30.  The binary search landed on
	** >30.  The next-lower break table entry MUST be either <= 30,
	** or >/>= some smaller value.  (No gaps, no overlaps.)  The
	** next-higher entry has to have a value larger than 30.
	** Therefore the next-lower break entry has to be the right one.
	** (Also, by the rules, there has to be a next-lower entry.)
	** By a similar example/argument, if we landed on <30, the
	** proper entry must be the next higher one.
	*/
	if (oper != DBDS_OP_LTEQ && oper != DBDS_OP_GTEQ)
	    if (oper == DBDS_OP_LT)
		++ break_ptr;
	    else
		-- break_ptr;
    }
    else
    {
	/* Didn't match a break table entry.  This case is a little
	** harder to explain, although it's not harder to do.
	** As an example, consider successive break table entries: >20, >30
	** and imagine a row value of 25.  The binary search might
	** land on the 20 or 30.  Clearly the >20 entry is the one
	** we want, but how do you tell?
	** It might seem that another comparison is needed, but actually
	** that is not the case.
	** The binary search loop tells us which direction the very
	** last comparison ended up;  we just have to look at "result".
	** If "result" is pointing in the same direction as the entry
	** we landed on, then that's the one to use.  If not, then
	** we move one entry in the direction of "result" and use that.
	** In the example, if we landed on 20, "result" will be +,
	** which is the same direction as >.  If we landed on >30, "result"
	** will be -, which points in the opposite direction from >;
	** we need to back up one and that must be the right entry.
	** (No gaps, no overlaps.)
	*/
	if (result < 0
	  && (oper != DBDS_OP_LT && oper != DBDS_OP_LTEQ))
	    /* Row is less but entry wants gt/ge, back one */
	    -- break_ptr;
	else if (result > 0
	  && (oper != DBDS_OP_GT && oper != DBDS_OP_GTEQ))
	    /* Row is greater but entry wants lt/le, forward one */
	    ++ break_ptr;
    }
    *ppartseq = break_ptr->partseq;

    return (E_DB_OK);
} /* adt_whichpart_range */

/*
** Name: adt_partbreak_compare	- compares the set of part break values
**	for 2 dimensions
**
** Description:
**	This routine is given 2 dimension descriptors and compares the
**	contained value breaks for equality. It determines the length
**	of each break, then loops over the break arrays comparing the
**	values.   The comparison is used to determine partition
**	compatibility of 2 joined tables, so we need to consider
**	anything that would interfere with PC-joining, and allow
**	anything that wouldn't.
**
**	We assume that the caller has checked the obvious stuff,
**	specifically: same number of columns, same number of breaks,
**	compatible types, and same distribution rules.
**
** Inputs:
**	adfcb		Ptr to ADF_CB for error handling
**	pdim1p		Ptr to DB_PART_DIM structure for one of the
**			comparands
**	pdim2p		Ptr to DB_PART_DIM structure for the other 
**			comparand
**
** Outputs:
**
** Returns:
**	TRUE		if the value break sets are identical, otherwise
**	FALSE
** History:
**	23-apr-04 (inkdo01)
**	    Written.
**	29-May-2007 (kschendel) SIR 122513
**	    Relax to allow different lengths and nullability as long as
**	    the actual break values are the same.
*/

bool 
adt_partbreak_compare(
	ADF_CB		*adfcb,
	DB_PART_DIM	*pdim1p,
	DB_PART_DIM	*pdim2p)

{
    DB_PART_BREAKS	*pbrk1p, *pbrk2p;
    DB_STATUS		status;
    i4			i, cmp;

    /* We could check for completely identical types/lengths/precisions
    ** and use one big MEcmp;  however this routine is not at present
    ** performance critical, so just compare each break column-wise.
    */

    /* Loop over value breaks, comparing each pair. */
    for (i = 0; i < pdim1p->nbreaks; i++)
    {
	pbrk1p = &pdim1p->part_breaks[i];
	pbrk2p = &pdim2p->part_breaks[i];
	if (pbrk1p->oper != pbrk2p->oper ||
	  pbrk1p->partseq != pbrk2p->partseq)
	    return (FALSE);
	status = adt_vvcmp(adfcb, pdim1p->ncols, pdim1p->part_list,
		pbrk1p->val_base, pbrk2p->val_base,
		&cmp);
	if (status != E_DB_OK || cmp != 0)
	  return(FALSE);	/* breaks (including values) 
				** must be identical */
    }

    return(TRUE);		/* success, if we get here */
}
