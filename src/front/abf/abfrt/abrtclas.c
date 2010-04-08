/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#include	<si.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<feconfig.h>
#include	<adf.h>
#include	<ade.h>
#include	<afe.h>
#include	<ooclass.h>
#include	<abfcnsts.h>
#include	<abftrace.h>
#include	<fdesc.h>
#include	<ilops.h>
#include	<abfrts.h>
#include	"rts.h"

/**
** Name:	abrtclas.c -	ABF Run-Time Class and Array module.
**
** Description:
**	Contains routines used to handle 4GL complex datatypes.
**
**  Defines:
**
**	iiarCicInitClass	initialize the class system.
**	iiarCnfEnterFrame	initialize class module at entry to frame.
**	iiarCxfExitFrame	deallocate class objects at exit from frame.
**	iiarCidInitDbdv		initialize a complex variable.
**	iiarCcnClassName	return name of class (for error msgs).
**
** 	iiarRiRecordInfo	return info about a record or array.
**	IIARoasObjAssign	assign objects (for arg passing).
**	IIARroReleaseObj	release an object.
**	IIARdocDotClear		clear a record.
**	IIARgtaGetAttr          return an attribute of a complex object.
**	IIARdotDotRef		resolve a DOT reference.
**	IIARdoaDotAssign	assign into a complex variable (for INSERTROW).
**
**	IIARarrArrayRef		resolve an ARRAY reference.
**	IIARariArrayInsert	insert an ARRAY row.
**	IIARardArrayDelete	delete an ARRAY row.
**	iiarArxRemoveRow	remove an ARRAY row.
**	iiarArClearArray	clear an ARRAY.
**	iiarArAllCount		count the elements in an ARRAY.
**	IIARaruArrayUnload	set up an ARRAY for unloadtable.
**	IIARarnArrayNext	get the next ARRAY row for unloadtable.
**	IIARareArrayEndUnload	terminate an unloadtable of an ARRAY.
**	iiarIarIsArray		See if a reference is to an ARRAY.
**
** History:
**	Revision 6.3/03/00  89/10  billc
**	Initial revision.
**	90/09  wong  Corrected memory allocation.  Bug #33263.
**	11/14/90 (emerson)
**		Created function iiarIarIsArray for bugs 34438 and 34440.
**	11/27/90 (emerson)
**		Added function iiarCtdTagsetDbdv (for bug 34663).
**	01/13/91 (emerson)
**		Fix bug 34845 (function iiarArAllCount).
**	03/23/91 (emerson)
**		Added function iiarRiRecordInfo (for look_up array support).
**	07-jun-91 (davel)
**		Fix bug 37412 (return OK status from iiarArClearArray)
**
**	Revision 6.4/02
**	10/13/91 (emerson)
**		Fix for bug 37109 in IIARoasObjAssign.
**	10/14/91 (emerson)
**		Fix for bug 40284 in IIARdotDotRef.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**
**		Boiled down the C_OBJ (complex object) structure.
**		Created the R_OBJ (record object) structure
**		and the A_OBJ (array object) structure.  Use an array
**		of pointers to array elements instead of a linked list.
**		Modified appropriate routines to maintain and use
**		revised data structures.
**
**		Made significant enhancements to memory management:
**		Use a separate memory tag for each unnested local array.
**		Maintain free lists of records of various sizes
**		for each unnested array; add removed array rows
**		to the appropriate free list.  Implemented a reference count
**		scheme for records, arrays, and memory pools (one per tag)
**		to prevent dangling references in the event that a called frame
**		or procedure clears an array (or removes some of its rows).
**
**		Also made source code format consistent: added braces
**		around single-statement IF and ELSE clauses; removed spaces
**		directly inside parentheses (except for function definitions
**		and cases where the matching parenthesis is on a different line)
**
**		Created iiarCnfEnterFrame and iiarCffFreeFields.
**		Created IIARroReleaseObj and IIARareArrayEndUnload.
**		Deleted the function iiarCtdTagsetDbdv (no longer used).
**		Deleted no-op functions iiarCifInitFields and iiarCffFreeFields.
**		Created and deleted sundry internal functions.
**		Virtually every function was modified (at least cosmetically).
**
**		Changed the order of the functions to be a bit more logical
**		(and to match the order in the list of functions above).
**	16-sep-92 (davel/emerson)
**		Fixed bug 46625 - Do not issue rlsObj until after
**		decrementing the reference count. Changes made in 
**		IIARroReleaseObj(), IIARdotDotRef(), and IIARarrArrayRef().
**	19-feb-93 (davel)
**		Fixed bug 49120 - save the record type in the db_prec member
**		of the DB_DATA_VALUE as part of the iiarCidInitDbdv() process.
**		Then there will always be a safe way to type-check input
**		parameters when calling a static frame.
**	12-feb-93 (essi)
**		Fixed bug 49512 per emerson's suggestions.
**
**	Revision 6.5
**
**	22-dec-92 (davel)
**		Added argument to iiarCcnClassName() for EXEC 4GL support.
**	28-dec-92 (davel)
**		Made static getAttr() global, and renamed to IIARgtaGetAttr().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	13-Jan-2005 (kodse01)
**	    replace %ld with %d for old long nat variables.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
[@history_template@]...
**/

/*
**	REF_COUNTER - A type suitable for holding an object's or pool's
**	reference count.  The maximum possible value is used to represent
**	a "permanent" object (PERM_OBJ_REF_COUNT).
**
**	The macros INCR_REF_COUNTER and DECR_REF_COUNTER should be used
**	to increment and decrement reference counts.
*/
typedef i4	REF_COUNTER;

#define PERM_OBJ_REF_COUNT	MAXI4

#define INCR_REF_COUNTER(x)	((x) == PERM_OBJ_REF_COUNT ? (x) : ++(x))
#define DECR_REF_COUNTER(x)	((x) == PERM_OBJ_REF_COUNT ? (x) : --(x))

/*
**	POOL_Q_HDR - A header for a queue of free buckets of equal size
**	for a given pool (see POOL_HDR below).  Note that bucket sizes
**	are always rounded up to a multiple of POOL_Q_HDR_SIZE (defined below).
*/
typedef struct _pool_q_hdr
{
	PTR		qh_free;	/* first bucket on free list (queue). */

	i2		qh_max_bkts;	/* the maximum number of buckets that
					** will fit into a block of size
					** MAX_RECOMMENDED_REQMEM (1 if
					** a bucket is bigger than that).
					*/
#define MAX_RECOMMENDED_REQMEM (8192 - 24)

	i4		qh_bkt_incr;	/* the number of buckets acquired by
					** the last MEreqlng done to replenish
					** the free list; this number will
					** start just big enough to fill a block
					** of size MIN_INITIAL_POOL_Q_REQMEM
					** and increase by 50% on each new
					** MEreqlng for this queue until we
					** reach qh_max_bkts.
					*/
#define MIN_INITIAL_POOL_Q_REQMEM 256

} POOL_Q_HDR;

/*
**	The size of a POOL_Q_HDR, rounded up to a power of two,
**	and subject to the constraint that it be at least as large as
**	a double (8 bytes).  For most platforms, this defined constant
**	will simply be 8, but it could be 16 for some platforms
**	(e.g. new MIPS boxes with 64-bit addresses).
*/
#define POOL_Q_HDR_SIZE ( sizeof(POOL_Q_HDR) <= sizeof(double) ? \
			  sizeof(double) : sizeof(double) << 1 )

/*
**	A macro to compute the size of a variable or type, rounded up
**	to a multiple of some number N (which must be a power of 2).
**	Implementation is modeled on the Sun4 ME_ALIGN_MACRO;
**	it assumes 2's complement arithmetic.
*/
#define ROUNDED_SIZEOF(obj, N) ((sizeof(obj) + ((N) - 1)) & ~((N) - 1))

/*
**	POOL_HDR - A header for a pool of storage for complex objects.
**	In the current implementation, there's one pool for each non-nested
**	array, one pool for all global objects outside arrays, and one pool per
**	frame or procedure on the call stack (for local objects outside arrays).
**
**	Each pool has its own memory tag.
**
**	The array pools have associated lists of free buckets.
*/
typedef struct _pool_hdr
{
	struct _pool_hdr *ph_next_pool;	/* for a non-array pool: first array
					** pool subordinate to this pool.
					** for an array pool: next array pool
					** subordinate to same non-array pool.
					*/
	REF_COUNTER	ph_ref_count;  	/* number of references to objects
					** in this pool from outside the pool.
					*/
	i2		ph_flags;	/* bitfield of internal info. */

#define PH_ARRAY	0x01		/* this pool is for an array. */

	TAGID		ph_tag;		/* pool's memory allocation tag. */

	i4		ph_max_bkt_size;/* for a non-array pool: irrelevant.
					** for an array pool: maximum bucket
					** size so far allocated for this pool.
					*/
	char		*ph_q_hdrs;	/* for a non-array pool: irrelevant.
					** for an array pool: points to
					** POOL_Q_HDR_SIZE bytes before a buffer
					** of size ph_max_bkt_size; this buffer
					** contains POOL_Q_HDR structures
					** for all bucket sizes encountered
					** so far for this pool; the POOL_Q_HDR
					** structures are indexed by bucket size
					*/
} POOL_HDR;

/*
**	C_OBJ - Private structure containing information about a complex object
**	(a record or an array).  Each array has a C_OBJ structure at the
**	beginning of its A_OBJ structure.  Each record (including each element
**	of each array) has a C_OBJ structure at the beginning of its RA_OBJ
**	structure.
*/
typedef struct _c_obj
{
	POOL_HDR	*co_pool;	/* pool header for this object. */

	REF_COUNTER	co_ref_count;  	/* object's reference count. */

	i2		co_flags;	/* bitfield of internal info. */

#define CO_ARRAY	0x01		/* object is an array. */
#define CO_ELEMENT	0x02		/* object is an element of an array. */

	i2		co_type;  	/* index of object's type descriptor. */
} C_OBJ;

/*
**	R_OBJ - Private structure containing information about a record.
**	Each record (including each element of each array) has a R_OBJ
**	structure at the beginning of its data area.
*/
typedef struct _r_obj
{
	C_OBJ		ro_co;		/* info common to all complex objects.*/

	i2		ro_state;	/* array element state (_STATE). */

#define RO_ST_UNCHANGED	2
#define RO_ST_DELETED	4

	i4		ro_record;	/* array element record num (_RECORD);
					** filled in by IIARarrArrayRef and
					** IIARarnArrayNext; *not* adjusted
					** thereafter (unless it's the current
					** unloaded record), even if records are
					** inserted or deleted earlier in array.
					*/
	C_OBJ		*ro_objs[1];	/* pointers to 0 or more
					** nested records or arrays.
					*/
} R_OBJ;

/*
**	A_OBJ - Private structure containing information about an array.
*/
typedef struct _a_obj
{
	C_OBJ		ao_co;		/* info common to all complex objects.*/

	POOL_HDR	*ao_pool;	/* pool from which elements of the array
					** will be allocated (for nested arrays
					** this is the same as ao_co.co_pool).
					*/

	R_OBJ		*ao_dummy;	/* array's "dummy" row (see NOTES). */

	R_OBJ		**ao_base;	/* 1 below bottom slot in pointer array
					** (see NOTES for pointer array info).*/
	R_OBJ		**ao_top;	/* top slot in pointer array */

	i4		ao_NumSlots;	/* number of slots in pointer array */

#define	INIT_PTR_ARRAY_SIZE	8	/* initial number of slots
					** in a 4GL array's pointer array.
					*/

	i4		ao_NumNonDel;	/* number of non-deleted rows */
	i4		ao_NegNumDel;	/* negation of number of deleted rows */

	struct _frame_elt *ao_UnldFrame;/* FRAME_ELT structure for top active
					** frame or procedure that's unloading
					** this array (NULL if none).
					*/
} A_OBJ;

/*
**	FRAME_ELT - Private structure containing class module information
**	for an executing (or suspended) frame or procedure (possibly local).
**	Currently, the only such information is a pool header (for local complex
**	objects outside arrays) and information r.e. UNLOADTABLEs for arrays.
**
**	The following points should be noted regarding UNLOADTABLEs for arrays:
**
**	(1) At most one UNLOADTABLE may be active within a single frame or
**	    procedure at any given time.  [This is enforced by the compiler:
**	    UNLOADTABLEs may not be lexically nested, and no 4GL mechanism
**	    other than CALLFRAME or CALLPROC is provided for "performing"
**	    a "lexically remote" group of 4GL statements.]
**	(2) Multiple UNLOADTABLEs of arrays may be concurrently active
**	    (in different frames or procedures in the call stack).
**	    [This is not true of UNLOADTABLEs of table fields].
**	    We don't explicitly document whether multiple UNLOADTABLEs of arrays
**	    may be concurrently active, but it has historically worked.
**	(3) We explicitly document that it's illegal to insert or delete
**	    rows in an array that's being unloaded.  However, we've never
**	    enforced this, and in fact we've handled these situations
**	    sensibly.  Therefore, we should probably continue to do so.
**	    [This means that fe_UnldRownum in one or more FRAME_ELT structures
**	    may require adjustment when a row is inserted or deleted].
**	(4) For an ENDLOOP out of an UNLOADTABLE of an array, the compiler
**	    used to generate no code to clean up the UNLOADTABLE.
**	    Because we must support existing imaged applications in shared
**	    segment environments such as VMS, the ABF RTS may not know
**	    that an UNLOADTABLE has terminated until the frame or procedure
**	    exits, or until another UNLOADTABLE is started.  This is why
**	    we need to keep track of which frame or procedure each
**	    UNLOADTABLE is associated with.  [If we always got control
**	    when an UNLOADTABLE terminated, we could just push an
**	    UNLOADTABLE element when an UNLOADTABLE loop starts, and pop it
**	    when the UNLOADTABLE loop ends].
*/
typedef struct _frame_elt
{
	struct _frame_elt *fe_next;	/* FRAME_ELT structure for calling
					** frame or procedure (NULL if none).
					*/
	A_OBJ		*fe_UnldArray;	/* array being unloaded:
					** array specified by last UNLOADTABLE
					** in this frame or procedure; NULL
					** if we know no UNLOADTABLE is active.
					*/
	struct _frame_elt *fe_NextSameArray;/* FRAME_ELT structure for ancestral
					** frame or procedure that's unloading
					** the same array as is this frame or
					** procedure (NULL if none).
					*/
	R_OBJ		*fe_UnldRec;	/* last row retrieved by an UNLOADTABLE;
					** NULL if we know no UNLOADTABLE is
					** active or if we haven't yet retrieved
					** the first row.
					*/
	i4		fe_UnldRownum;	/* record number (index) of last row
					** retrieved by an UNLOADTABLE; 0
					** if we know no UNLOADTABLE is active
					** or if we haven't yet retrieved
					** the first row.
					*/
	i4		fe_flags;	/* bitfield of internal info. */

#define FE_UNLD_IN_DLT	0x01		/* This UNLOADTABLE is now processing
					** the array's deleted rows.
					*/
	POOL_HDR	fe_pool_hdr;	/* pool header for local complex objects
					** outside arrays.
					*/
} FRAME_ELT;

static ABRTSTYPE *ClassTable = NULL;
static i4	*ClassHash = NULL;
static i4	ClassCount = 0;
static ADF_CB	*Adf_cb = NULL;

static i2	*NumComplexAttrs = NULL;/* maps a type index into the number
					** of complex attributes in the type.
					*/
static FRAME_ELT *CurrentFrame = NULL;	/* FRAME_ELT structure for currently
					** executing frame or procedure.
					*/
static POOL_HDR	GlobalPoolHdr = {0};	/* pool header for global complex
					** objects outside arrays.
					*/
static STATUS	getState();

static A_OBJ	*ckArray();
static VOID	endCurrentUnload();
static R_OBJ	*insRow();
static R_OBJ	*delRow();
static R_OBJ	*rmvRow();
static R_OBJ	*rmvDelRow();
static A_OBJ	*newArray();
static R_OBJ	*newRec();

static VOID	rlsObj();
static C_OBJ	*clrArray();
static C_OBJ	*clrRec();
static VOID	initSimpleAttrs();
static PTR	getObj();
static VOID	freeObj();
static VOID	freeBlock();
static i4	findType();
static STATUS	calcOffsets();

static VOID	indexErr();
static PTR	fe_req();
static PTR	me_req();

/*
** NOTES:
**
** Layout for a record:
**
**  +---------------+      +-------+      +-----------+
**  | DB_DATA_VALUE | ---> | C_OBJ | ---> | ABRTSTYPE |
**  +---------------+      +-------+      +-----------+
**                         | rest  |            |
**                         | of    |            V
**                         | data  |      +------------+
**                         | area  |      | ABRTSATTR  |
**                         +-------+      +------------+
**                                        | ABRTSATTR  |
**                                        +------------+
**                                        | ABRTSATTR  |
**                                        +------------+
**
**	The example above shows a record with three attributes.
**	The DB_DATA_VALUE is the dbdv in the frame's stack.
**	Its db_data pointer points to the record's data area.
**	At the beginning of the data area is the record's C_OBJ,
**	which describes the complex object (in this case, the record).
**	The C_OBJ points to the ABRTSTYPE, which describes the class.
**	The ABRTSATTRs describe the attributes, and hold the offsets
**	into the data area.
**
** Layout for an array:
**
**  +------------------+
**  | DB_DATA_VALUE    |
**  +------------------+
**          | 
**          V
**  +------------------+
**  | C_OBJ for array  |--------------------------------------------------+
**  +------------------+                                                  |
**  | rest of A_OBJ:   |                                                  |
**  | base             |------->                                          |
**  | base + NumNonDel |-----+  +----+    +-------------------+           |
**  | top  + NegNumDel |---+ |  |    |--->| C_OBJ for row  1  |---------+ |
**  | top              |-+ | |  +----+    +-------------------+         | |
**  +------------------+ | | +->|    |-+  | rest of data area |         | |
**                       | |    +----+ |  +-------------------+         | |
**                       | |    |    | |                                | |
**                       | |    +----+ |  +-------------------+         | |
**                       | |    |    | +->| C_OBJ for row  2  |-------+ | |
**                       | |    +----+    +-------------------+       | | |
**                       | |    |    |    | rest of data area |       | | |
**                       | |    +----+    +-------------------+       | | |
**                       | +--->|    |                                | | |
**                       |      +----+    +-------------------+       | | |
**                       |      |    |--->| C_OBJ for row -1  |-----+ | | |
**                       |      +----+    +-------------------+     | | | |
**                       +----->|    |-+  | rest of data area |     | | | |
**                              +----+ |  +-------------------+     | | | |
**                                     |                            | | | |
**                                     |  +-------------------+     | | | |
**                                     +->| C_OBJ for row  0  |---+ | | | |
**                                        +-------------------+   | | | | |
**                                        | rest of data area |   | | | | |
**                                        +-------------------+   | | | | |
**                                                                V V V V V
**                                                              +-----------+
**                                                              | ABRTSTYPE |
**                                                              +-----------+
**                                                                    |
**                                                                    V
**                                                              +------------+
**                                                              | ABRTSATTR  |
**                                                              +------------+
**                                                              | ABRTSATTR  |
**                                                              +------------+
**                                                              | ABRTSATTR  |
**                                                              +------------+
**
**	As you can see, arrays are considerably more complex than records.
**
**	The example above shows an array with three attributes.
**	The array contains two non-deleted rows (rows 1 and 2),
**	and two deleted rows (rows -1 and 0).  Note: within structures
**	and arrays, storage addresses are depicted upside-down; that is,
**	with low addresses at the top.
**
**	The DB_DATA_VALUE is the dbdv in the frame's stack (as for a record).
**	Its db_data pointer, however, points to the array's A_OBJ.
**	At the beginning of the A_OBJ is the array's C_OBJ,
**	which is a model for the C_OBJ in each of the elements of the array.
**	[Note that a complex object's DB_DATA_VALUE.db_data pointer
**	can be thought of as a pointer to a C_OBJ structure, regardless of
**	whether the complex object is a record or an array].
**
**	The elements of the array are in fact just records.
**	They (the elements) are accessed via an array of pointers.
**	This pointer array initially contains a small number of slots;
**	it's doubled in size every time it overflows.  (The initial number
**	of slots is enough to fill one array row; this enables old, outgrown
**	pointer arrays to be reused as rows).  The pointer array contains
**	pointers to non-deleted records, followed by free pointer slots,
**	followed by pointers to deleted records.  The A_OBJ structure contains
**	a pointer to the *last* (top) slot in the pointer array;
**	it also contains a pointer to the "base" of the pointer array.
**	This base pointer actually points to the "-1" pointer
**	(off in the ozone); this simplifies pointer arithmetic.
**
** Memory mangagement:
**
**	'iiarCidInitDbdv()' initializes an individual object.
**	If the object is local, the current tag is used.
**	Each frame or procedure starts a tag block (see "abrtcall.qsc")
**	that is freed when it returns; this frees the local objects.
**	Otherwise, if the object is global (it's only initialized once
**	in an application), then memory is allocated using a tag allocated
**	in 'iiarCicInitClass()'.  This will never be freed.
**
**	Unnested arrays are special, however, in that memory for their rows
**	is allocated using a special tag so that they can be freed when cleared.
**	Local arrays are freed just before the frame or procedure returns.
**
**	Each unnested array has one or more free lists associated with it.
**	When a row of such an array is removed, it's added to a free list.
**	If and when a row is subsequently inserted into the array, memory
**	for it will be obtained from the free list.  If the free list
**	is empty, an MEreqlng will be issued to replenish it.
**	[Unless the rows are quite large, the MEreqlng will typically get
**	memory for more than one row.  For any given free list, the MEreqlng's
**	start out small; each new MEreqlng is about 1.5 times as big
**	as the previous until a fairly large ceiling is reached - see
**	the defines for MIN_INITIAL_POOL_Q_REQMEM and MAX_RECOMMENDED_REQMEM.]
**
**	An unnested array that contains nested records or arrays has more than
**	one free list: there's one for each record size that can occur within
**	the array.  [The sizes are rounded up to 8 or 16 - see the define for
**	POOL_Q_HDR_SIZE].  All arrays nested within the unnested array share
**	its memory pool.  When a nested array is cleared, its rows (and any
**	nested records or arrays) are added to the appropriate free list(s).
**
**	When a record contains a nested record, space for the nested record
**	is not obtained when the parent record is allocated: it's deferred until
**	the nested record is referenced.  If the record is part of an array,
**	space will be obtained from a free list (if possible), as discussed
**	above.  If the record is *not* part of an array, a separate FEreqlng
**	is issued for it; no free lists are maintained for objects outside
**	arrays.  There are a couple of reasons for this:  First, not many
**	records of the same size are likely to be allocated outside arrays.
**	Second, the chances are that the free lists would stay empty, because
**	the only 4GL language construct that can clear a record outside an
**	an array is a SELECT into a record (CLEARROW, as the name implies,
**	is strictly for rows of arrays).
**
** Reference counts:
**
**	Prior to 6.4/02, ABF/4GL did not maintain reference counts
**	for complex objects.  I (emerson) have introduced them because
**	they appear to be the only reasonably efficient way of preventing
**	certain user errors from causing (possibly silent) memory or data
**	corruption.
**
**	The sorts of user errors that need to be guarded against
**	are situations where a called frame or procedure removes a row
**	from an array, but the calling frame or procedure is about
**	to put something into the row (or take something out).
**	For example, suppose that calling procedure P with argument
**	arg1 set to "a" removes row i from array a.  Then without
**	reference counts, all of the following constructs are hazardous:
**
**	(1)	r = a[i].b + P(arg1 = a);
**	(2)	r = P(arg1 = a) + a[i].b;
**	(3)	a[i].b = P(arg1 = a);
**	(4)	r = P(arg1 = a, arg2 = byref(a[i].b));
**
**	In fact, (1) would probably work [whereas (2) wouldn't]
**	because expressions are in fact evaulated from left to right.
**	But users shouldn't count on any particular order of evaluation.
**
**	The remaining cases cause trouble because the l-value of a[i].b
**	is computed before P is called.  [That may not be obvious in (2);
**	but ABF/4GL in fact computes the l-values of all simple variables
**	and of all rows and/or attributes of complex variables (e.g. a[i].b)
**	in an expression before it even starts to evaluate the expression].
**
**	[Aside: prior to 6.4/02, the only case where trouble could actually
**	occur was when an unnested global array was cleared.  That's because
**	clearing any other sort of array, or removing a row in any array,
**	didn't cause memory to be freed (or marked for re-use).]
**
**	Reference counts prevent dangling references from occurring
**	in situations like those above:  The 4GL compiler now will
**	generate IL to bump necessary reference counts before doing
**	a CALLPROC or CALLFRAME, and to decrement them after the reference
**	to the object.  In the examples above, assuming that "b" is a
**	*simple* attribute of "a", the compiler computes the l-value of a[i]
**	and leaves it in a temp (the l-value of a[i].b now goes into a
**	different temp).  The IL_ARRAYREF instruction that put the l-value
**	of a[i] into its temp has a modifier bit set that will cause
**	IIARarrArrayRef to bump a[i]'s reference count.  After all the IL
**	to evaluate and assign the expression, the compiler generates
**	an IL_RELEASEOBJ instruction to "release" a[i].  A "release" of a[i]
**	was also done when P removed it, but all that release did was to
**	decrement a[i]'s reference count.  But when the IL_RELEASEOBJ
**	is executed, a[i]'s reference count is down to 1, and so it's
**	actually freed (added to a free list).
**
**	Reference counts are also used for entire memory pools:  we mustn't
**	free the memory associated with an unnested array if there are any
**	references into the array from outside it.  Rather than examining
**	the reference counts of all the rows in an unnested array
**	[and recursively examining the reference counts of nested objects],
**	we maintain a special reference counter for each unnested array
**	that represents the number of "dangerous" references into the array
**	from outside it.  [A reference isn't dangerous if we know it will
**	go away before any "clear" of the array].  If an unnested array
**	is cleared when there are still references into it from outside,
**	the "clear" is implemented as it would be if the array were nested:
**	all the array's rows are released, and wherever a release of an
**	object actually results in the object being freed, any objects
**	nested in the freed object are recursively released.
**
**	ABF/4GL programs compiled using an old (pre-6.4/02) compiler
**	will not generate code to explicitly hold or release objects;
**	when such programs are executed by the new ABF runtime, a complex
**	object inside an array will generally has a reference count of 1,
**	until a row containing it is removed, or an array containing it is
**	cleared, at which point it will be freed.  An array memory pool
**	will generally has a reference count of 0.  The only exception
**	occurs on an unloadtable: the current unloaded row will have
**	a reference count of 2, and its memory pool will have a reference
**	count of 1.  These will be decremented at exit from the unloadtable
**	*unless* the exit was done via an endloop.  In that case, the
**	reference counts will stay bumped, and the last unloaded row
**	is in effect permanent (although if the array is local, it will
**	all go away when its defining frame terminates - see below).
**
**	Memory pools associated with local arrays defined
**	in a particular frame are freed when the frame terminates.
**	This is safe because ABF/4GL provides no language constructs
**	for placing the address of a complex object into another object,
**	or passing the address of a local complex object back to a calling
**	routine.  We don't bother checking the pool's reference count:
**	if the frame (and all frames it calls) were compiled with a new
**	compiler, the reference count should be zero at this point,
**	and if they weren't all compiled with a new compiler, the reference
**	count can't be trusted anyway.
**
** Error Handling:
**
**	Error handling is stringent here.  When this module was originally
**	implemented, there was no dependency-checking of classes,
**	so someone could easily run a frame with IL code
**	that was out-of-date with respect to the class definition.
**	Error handling had to be thorough and graceful.
**
**	In handling a DOT error, we can assume that the frame is out of date,
**	and exit the frame.
**
**	An ARRAY error, if it's a bad index, is a user error and we must
**	recover; we direct the array reference to a special initialized row
**	(the "dummy" row).  If necessary, we'll allocate it and initialize it.
*/

/*{
** Name:	iiarCicInitClass() -	Initialize class module for application.
**
** Description:
**	This is called once per application, and sets up our tables of classes.
**
** Inputs:
**	table	{ABRTSTYPE *} 	The table of classes.
**	hash	{nat *}		The hashtable of classes.
**	count	{nat}		The number of classes in the table of classes.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Save new 3rd parm in ClassCount.  Allocate NumComplexAttrs.
*/
VOID
iiarCicInitClass( table, hash, count )
ABRTSTYPE *table;
i4  *hash;
i4  count;
{
	ClassTable = table;
	ClassHash = hash;
	ClassCount = count;

	Adf_cb = FEadfcb();

	GlobalPoolHdr.ph_tag = FEgettag();
	GlobalPoolHdr.ph_ref_count = PERM_OBJ_REF_COUNT;

	if (count > 0)
	{
		i4	NumComplexAttrs_size = count * sizeof(i2);

		NumComplexAttrs = (i2 *)fe_req( GlobalPoolHdr.ph_tag,
						(u_i4)NumComplexAttrs_size,
						ERx("iiarCicInitClass") );

		MEfill((u_i2)NumComplexAttrs_size, '\0', (PTR)NumComplexAttrs);
	}
}

/*{
** Name:	iiarCnfEnterFrame() -	Initialize at entry to frame.
**
** Description:
**	This is called at entry to a frame (or procedure),
**	just after a memory tag has been acquired for the frame.
**	This tag has been made the current tag, and thus can referred to
**	as tag 0.
**
** Inputs:
**	tag	{TAGID}		the memory tag acquired for the frame.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
VOID
iiarCnfEnterFrame( tag )
TAGID	tag;
{
	FRAME_ELT	*tmp;

	/*
	** Get a FRAME_ELT for the new frame.
	*/
	tmp = (FRAME_ELT *)fe_req( tag, (u_i4)sizeof(FRAME_ELT),
				   ERx("iiarCnfEnterFrame") );

	MEfill((u_i2)sizeof(FRAME_ELT), '\0', (PTR)tmp);

	tmp->fe_pool_hdr.ph_tag = tag;
	tmp->fe_pool_hdr.ph_ref_count = PERM_OBJ_REF_COUNT;

	/*
	** Push the FRAME_ELT for the new frame.
	*/
	tmp->fe_next = CurrentFrame;
	CurrentFrame = tmp;
}

/*{
** Name:	iiarCxfExitFrame() -	Deallocate at exit from frame.
**
** Description:
**	This is called at exit from a frame (or procedure),
**	just before releasing the memory that has been acquired for the frame.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
VOID
iiarCxfExitFrame( )
{
	POOL_HDR *pool;

	/*
	** Terminate the current UNLOADTABLE of an array (if any).
	*/
	endCurrentUnload();

	/*
	** Free memory (and tags) for all local arrays in the terminating frame.
	**
	** Note that we call MEtfree before calling IIUGtagFree.
	** This necessary (and safe) because all memory allocations
	** against this pool used MEreqlng rather than FEreqlng,
	** and IIUGtagFree will skip its internal MEtfree (thinking
	** there is no memory to be freed).  We still have to call
	** IIUGtagFree to free the tag.
	*/
	for ( pool = CurrentFrame->fe_pool_hdr.ph_next_pool; pool != NULL;
	      pool = pool->ph_next_pool )
	{
		_VOID_ MEtfree((u_i2)pool->ph_tag);
		IIUGtagFree(pool->ph_tag);
	}

	/*
	** Pop the FRAME_ELT for the terminating frame.
	*/
	CurrentFrame = CurrentFrame->fe_next;
}

/*{
** Name:	iiarCidInitDbdv() -	Initialize a complex variable.
**
** Description:
**	Initializes a complex variable declared in a frame or procedure.
**	iiarCnfEnterFrame must have already been called for the frame
**	or procedure in which the variable is being declared.
**
** Inputs:
**	tname	{char *} 	name of the variable's class (a typename).
**	dbdv	{DB_DATA_VALUE *} the dbdv to fill in.
**	global	{bool}		whether this is a global variable
**				(local variables in static frames
**				are considered to be global here).
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Use newRec and newArray instead of newObj.
**		Logic to get a new tag for elements of global arrays
**		has been moved to newArray.
**	19-feb-93 (davel)
**		Fixed bug 49120 - save the index into the class table in the
**		db_prec member of the dbv.
*/
STATUS
iiarCidInitDbdv( tname, dbdv, global )
char		*tname;
DB_DATA_VALUE	*dbdv;
bool		global;
{
	i4	typ;
	POOL_HDR *pool;

	if (global)
	{
		pool = &GlobalPoolHdr;
	}
	else
	{
		pool = &CurrentFrame->fe_pool_hdr;
	}

	typ = findType(tname);
	if (typ < 0)
	{
		/* This frame is out of date ... we didn't get a class. */
		iiarUsrErr(BADCLASS, 1, tname);
		return FAIL;
	}

	/* db_length is non-zero for arrays, zero for simple objects. */
	if (dbdv->db_length != 0)
	{
		dbdv->db_data = (PTR)newArray(typ, pool);
	}
	else
	{
		dbdv->db_data = (PTR)newRec(typ, pool);
	}

	/* always save the index into the class table in the unused db_prec
	** member of the DBV.
	*/
	dbdv->db_prec = typ;

	return OK;
}

/*{
** Name:	iiarCcnClassName() -	return a typename, for errors.
**
** Description:
**	Given an object, return the name of its class.
**
** Inputs:
**	dbv		{DB_DATA_VALUE *}	- object whose name we seek.
**	buf		{AB_TYPENAME}		- where to put the name.
**	print_array	{bool}			- print "array of" prefix?
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Pointer to type is now obtained differently.  Simplified logic.
**	22-dec-92 (davel)
**		Added print_array argument for EXEC 4GL support.
*/
VOID
iiarCcnClassName( DB_DATA_VALUE *dbv, AB_TYPENAME buf, bool print_array )
{
	C_OBJ 		*desc = (C_OBJ *)dbv->db_data;
	char		*p = (char *)buf;

	if (dbv->db_datatype != DB_DMY_TYPE || desc == NULL)
	{
		p[0] = '?';
		p[1] = EOS;
		return;
	}
	if (desc->co_flags & CO_ARRAY && print_array)
	{
		MEcopy( (PTR)ERx("array of "),
			(u_i2)(sizeof(ERx("array of ")) - 1),
			(PTR)p );
		p += sizeof(ERx("array of ")) - 1;
	}
	STlcopy(ClassTable[desc->co_type].abrtname, p, sizeof(AB_TYPENAME) - 1);
}

/*{
** Name:	iiarRiRecordInfo() -	Return info about a record or array
**
** Description:
**	Given a complex object (a record, array, or array element),
**	return information about it.
**
** Inputs:
**	rec_dbv		{DB_DATA_VALUE *}	The dbv of the complex object.
**
** Outputs:
**	*rec_type	{ABRTSTYPE *}		A pointer to a structure 
**						describing the complex object's
**						record type.
**	*rec_data	{PTR}			A pointer to the record's data
**						area (relevant only if
**						the complex object is a record
**						or an array element).
**	*rec_num	{i4}		The record number (relevant
**						only if the complex object
**						is an array element).
** Returns:
**	STATUS	-- OK on success; FAIL if rec_dbv is not a dbv of a complex obj.
**
** History:
**	03/23/91 (emerson)
**		Written
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Pointer to record and type are now obtained differently.
*/
STATUS
iiarRiRecordInfo( rec_dbv, rec_type, rec_data, rec_num )
DB_DATA_VALUE	*rec_dbv;
ABRTSTYPE	**rec_type;
PTR		*rec_data;
i4		*rec_num;
{
	C_OBJ		*desc;

	desc = (C_OBJ *)rec_dbv->db_data;

	/* make sure this is a record or array. */
	if (rec_dbv->db_datatype != DB_DMY_TYPE || desc == NULL)
	{
		return FAIL;
	}

	*rec_type = &ClassTable[desc->co_type];

	*rec_data = (PTR)desc;

	if (desc->co_flags & CO_ARRAY)
	{
		*rec_num = 0;
	}
	else
	{
		*rec_num = ((R_OBJ *)desc)->ro_record;
	}

	return OK;
}

/*{
** Name:	IIARoasObjAssign() -	Assign an object to another.
**
** Description:
**	This is for assigning one complex object to another.  Currently it's
**	used only for parameter-passing.  This is a BYREF assignment.
**
** Inputs:
**	from 	{DB_DATA_VALUE *} Dbv holding a complex object.
**	to 	{DB_DATA_VALUE *} Dbv to assign into.
**
** Returns:
**	STATUS	-- OK on success.
**
** History:
**	10/89 (billc) - written.
**	10/13/91 (emerson)
**		Fix for bug 37109.  Determine whether the objects are arrays
**		by calling iiarIarIsArray, rather than looking db_length
**		(which isn't reliable for placeholder temporaries).
**	19-feb-93 (davel)
**		Fixed bug 49120 - assume the worst, that the db_data member
**		of the "to" DBV is now a garbage pointer.  This is possible
**		when the called frame is a static frame, and the previously
**		passed in complex object is now freed.  The type-check can
**		now be done by using the saved type index in the db_prec
**		member of the "to" dbv.
*/
STATUS
IIARoasObjAssign( from, to )
DB_DATA_VALUE *from;
DB_DATA_VALUE *to;
{
	char *fname;
	char *tname;
	bool to_is_array = (to->db_length != 0 ? TRUE : FALSE);

	if (  from->db_datatype != DB_DMY_TYPE
	   || from->db_datatype != to->db_datatype
	   || iiarIarIsArray(from) != to_is_array )
	{
		/* Complex-to-scalar is incompatible. so is array-to-nonarray.
		** Our caller will report the error.
		*/
		return FAIL;
	}
	fname = ClassTable[((C_OBJ *)(from->db_data))->co_type].abrtname;
	tname = ClassTable[to->db_prec].abrtname;

	/* They're both complex, but are they the same type? */
	if (!STequal(fname, tname))
	{
		return FAIL;
	}
	to->db_data = from->db_data;

	return OK;
}

/*{
** Name:	IIARroReleaseObj() -	release an object
**
** Description:
**	Release a record or array:  Decrement its reference count;
**	if it reaches 0, add the object to the appropriate free list
**	and recursively release its children (rows of an array, or
**	complex attributes of a record).
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding an object.
**
** Returns:
**	OK on success.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
STATUS
IIARroReleaseObj( obj )
register DB_DATA_VALUE	*obj;
{
	C_OBJ		*desc;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rRELEASE OBJ %s\n\r"),
		ClassTable[((C_OBJ *)(obj->db_data))->co_type].abrtname );
#endif /* CLAS_TRACE */

	desc = (C_OBJ *)obj->db_data;

	/* make sure this is a record or array. */
	if (obj->db_datatype != DB_DMY_TYPE || desc == NULL)
	{
		AB_TYPENAME	otype;

		iiarUsrErr(NOTARRAY, 1, iiarTypeName(obj, otype));
		return FAIL;
	}
	
	/*
	** Release the object after indicating that its memory pool
	** has one less reference from outside.
	*/
	DECR_REF_COUNTER(desc->co_pool->ph_ref_count);
	rlsObj(desc);

	obj->db_data = NULL;
	return OK;
}

/*{
** Name:	IIARdocDotClear() -	clear a record.
**
** Description:
**	'CLEAR' a record:  Re-initilize it to default values.
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding an array row.
**
** Returns:
**	OK on success.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Tightened up bullet-proofing to ensure that putative record
**		is not an array.  Use clrRec instead of initRow.
**
**		Return OK on success (FAIL was being returned -
**		not that anybody seemed to mind).
*/
STATUS
IIARdocDotClear( obj )
register DB_DATA_VALUE	*obj;
{
	R_OBJ		*rec;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rCLEAR DOT %s\n\r"),
		ClassTable[((C_OBJ *)(obj->db_data))->co_type].abrtname );
#endif /* CLAS_TRACE */

	rec = (R_OBJ *)obj->db_data;

	/* make sure this is a record. */
	if (  obj->db_datatype != DB_DMY_TYPE
	   || rec == NULL || (rec->ro_co.co_flags & CO_ARRAY) )
	{
		AB_TYPENAME	otype;

		iiarUsrErr(NOTARRAY, 1, iiarTypeName(obj, otype));
		return FAIL;
	}

	/*
	** Call clrRec to do the real work, but first temporarily
	** bump the record's reference count so that clrRec won't
	** decide to release the record.
	*/
	INCR_REF_COUNTER(rec->ro_co.co_ref_count);
	_VOID_ clrRec(rec);
	DECR_REF_COUNTER(rec->ro_co.co_ref_count);

	return OK;
}

/*{
** Name:	IIARdotDotRef() -	resolve a DOT reference.
**
** Description:
**	Executes the core of the DOT command, resolving a object-membership
**	reference.
**
**	The 4GL code
**
**		X.Y[I].z
**
**	Becomes IL code
**
**		DOT 	Xobj "Y" tmp 	-- tmp now holds the x.y object
**		ARRAY	tmp Iexpr tmp	-- tmp holds the i'th element of x.y.
**		DOT	tmp "z"	tmp
**
**	'tmp' then holds the value of x.y[i].z.  The db_data pointer points into
**	the object's data area, so assignments work appropriately.
**
**	The 'tmp' object only gets its db_data pointer twiddled.  The 'tmp'
**	was allocated with the type of the value that finally goes into it.
**	So if z is an integer attribute of Y, 'tmp' is DB_INT_TYPE.  When we
**	finally DOT into a scalar, we check the datatypes for legitimate
**	matchup.
**
** Inputs:
**	src 	{DB_DATA_VALUE *} Dbv holding an object.
**	mname	{char *}	  Name of the object attribute.
**	targ 	{DB_DATA_VALUE *} Dbv to dereference into.
**	flags	{nat}		  Any combination of the following:
**				  ILM_HOLD_TARG: bump targ's reference count.
**				  ILM_RLS_SRC: release src (decrement its ref
**				  count, or free it if ref count is 1).
**				  1: (only for UNLOADTABLE),
**				  instead of just twiddling targ's db_data,
**				  the result is copied to targ
**				  (its db_data pointer is unchanged);
**				  data is type-coerced, if necessary.
**				  The caller must not specify this flag
**				  if either targ is of type DB_DMY_TYPE.
**
** Returns:
**	STATUS	-- OK on success.
**
** History:
**	10/89 (billc) - written.
**	10/14/91 (emerson)
**		Fix for bug 40284: Always do an assign if the 4th parm is TRUE.
**		Before, if a variable specified to receive an attribute
**		on an UNLOADTABLE statement had the same type as the attribute,
**		then it (the variable) was simply made to point to the
**		attribute.  This caused havoc.
**	11/07/91 (emerson)
**		New flags (modifier bits) are supported.
*/
STATUS
IIARdotDotRef( src, mname, targ, flags )
DB_DATA_VALUE	*src;
char		*mname;
DB_DATA_VALUE	*targ;
i4		flags;
{
	AB_TYPENAME	otype;
	DB_DATA_VALUE	ldbv;
	C_OBJ		*targ_obj;
	C_OBJ		*src_obj = (C_OBJ *)src->db_data;
	STATUS		rval = OK;

	ldbv.db_data = NULL;

	if (IIARgtaGetAttr(src, mname, &ldbv) != OK)
	{
		return FAIL;
	}

	if (flags & 1)	/* we need to do an assignment. */
	{
		rval = afe_cvinto(Adf_cb, &ldbv, targ);
		if (rval != OK)
		{
			rval = FAIL;
			if (  Adf_cb->adf_errcb.ad_errcode
			   == E_AD1012_NULL_TO_NONNULL )
			{
				FEafeerr(Adf_cb);
			}
			else
			{
				iiarUsrErr( FROMATTRTYPE, 2, mname,
					    iiarTypeName(src, otype) );
			}
		}
	}
	else		/* no assignment requested. */
	{
		if (ldbv.db_datatype == DB_DMY_TYPE)
		{
			/* not a scalar, so be delicate. */
			targ->db_data = ldbv.db_data;
		}
		else if (ldbv.db_datatype == targ->db_datatype)
		{
			/* we reached a scalar, and the types matched. */
			targ->db_length = ldbv.db_length;
			targ->db_prec = ldbv.db_prec;
			targ->db_data = ldbv.db_data;
		}
		else
		{
			rval = FAIL;
			iiarUsrErr( FROMATTRTYPE, 2, mname,
				    iiarTypeName(src, otype) );
		}
	}

	/*
	** If the target is to be held, increment its reference count
	** and its pool's reference count.
	*/
	if (flags & ILM_HOLD_TARG)
	{
		if (ldbv.db_datatype != DB_DMY_TYPE)
		{
			rval = FAIL;
			iiarUsrErr( FROMATTRTYPE, 2, mname,
				    iiarTypeName(src, otype) );
		}
		else
		{
			targ_obj = (C_OBJ *)targ->db_data;

			INCR_REF_COUNTER(targ_obj->co_ref_count);
			INCR_REF_COUNTER(targ_obj->co_pool->ph_ref_count);
		}
	}

	/*
	** If the source is to be released, then release the source object after
	** indicating that its memory pool has one less reference from outside.
	*/
	if (flags & ILM_RLS_SRC)
	{
		DECR_REF_COUNTER(src_obj->co_pool->ph_ref_count);
		rlsObj(src_obj);
	}

	return rval;
}

/*{
** Name:	IIARdoaDotAssign() -	for assigning to a scalar in a object.
**
** Description:
**	This is used in INSERTROW and CLEARROW.  This is for assigning to
**	an object attribute without using a pre-computed FID.  (This imitates
**	the handling of table-fields in INSERTROW.)
**	If the source object ('from') is null, re-initialize the named 'obj'
**	attribute.
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding a object.
**	mname	{char *}	  Name of the object attribute.
**	from 	{DB_DATA_VALUE *} Dbv to assign from.  If NULL, just
**					re-initialize 'obj'.
**
** Returns:
**	STATUS	-- OK on success.
**
** History:
**	10/89 (billc) - written.
*/
STATUS
IIARdoaDotAssign( obj, mname, from )
DB_DATA_VALUE	*obj;
char		*mname;
DB_DATA_VALUE	*from;
{
	DB_DATA_VALUE	dtmp;
	AB_TYPENAME	otype;

	/* get the object attribute. */
	if (IIARgtaGetAttr(obj, mname, &dtmp) != OK)
	{
		return FAIL;
	}

	/* We only allow this on scalars. */
	if (  dtmp.db_datatype == DB_DMY_TYPE
	   || (from != NULL && from->db_datatype == DB_DMY_TYPE) )
	{
		iiarUsrErr(NOTSCALAR, 2, iiarTypeName(obj, otype), mname);
	}

	/* dtmp is the dbv from the object, a scalar.  Assign into it.  */

	if (from == NULL)
	{
		iiarInitDbv(Adf_cb, &dtmp);
	}
	else if (afe_cvinto(Adf_cb, from, &dtmp) != OK)
	{
		if (Adf_cb->adf_errcb.ad_errcode == E_AD1012_NULL_TO_NONNULL)
		{
			FEafeerr(Adf_cb);
		}
		else
		{
			iiarUsrErr( INTOATTRTYPE, 2,
					mname, iiarTypeName(obj, otype) );
		}
		return FAIL;
	}

	return OK;
}

/*
** Name:	IIARgtaGetAttr() -	get the dbv for an object attribute.
**
** Description:
**	Given an object and an attribute name, get the object attribute.
**	In general, this function should be used to resolve DOT references,
**	as this routine does not reference count updating.
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding a object.
**	mname	{char *}	  Name of the object attribute.
**
** Outputs:
**	targ 	{DB_DATA_VALUE *} Dbv to load object attribute info into.
**				  all members of the Dbv are set.
**
** Returns:
**	STATUS	-- OK on success.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Treat an array object as an error (it should never happen).
**		Change the way we find the record's data area.
**		Use newRec and newArray instead of newObj.
**		Save the index of the class table entry for a complex
**		attribute (so we only have to find it once per application).
**		Logic to get a new tag for elements of global arrays
**		has been moved to newArray.
**
**		Also fix an apparent bug: the backwards sequential search
**		for a matching attribute was starting at abrtcnt instead of
**		at abrtcnt - 1.
**	28-dec-92 (davel)
**		Renamed from (static) getAttr to (global) IIARgtaGetAttr().
*/
STATUS
IIARgtaGetAttr( obj, mname, targ )
DB_DATA_VALUE *obj;
char *mname;
DB_DATA_VALUE *targ;
{
	R_OBJ		*rec;
	register ABRTSTYPE *typ;
	register ABRTSATTR *ap = NULL;
	register i4	i;
	PTR		datap	= NULL;
	DB_DATA_VALUE	*ldbv;
	DB_DATA_VALUE	ltmp;
	AB_TYPENAME	otype;

	rec = (R_OBJ *)obj->db_data;

	/* make sure this is legit. */
	if (rec == NULL || (rec->ro_co.co_flags & CO_ARRAY))
	{
		iiarUsrErr(NOATTR, 2, mname, iiarTypeName(obj, otype));
		return FAIL;
	}

	typ = &ClassTable[rec->ro_co.co_type];

	/* This could probably be optimized -- hashtable maybe? */
	i = typ->abrtcnt;
	for (;;)
	{
		i--;
		if (i < 0)
		{
			/*
			** We didn't find the named attribute.
			** Maybe it's _RECORD or _STATE.
			*/
			if (getState(rec, mname, &ltmp) != OK)
			{
				iiarUsrErr( NOATTR, 2, mname,
						iiarTypeName(obj,otype) );
				return FAIL;
			}
			ldbv = &ltmp;
			datap = ldbv->db_data;
			break;
		}
		if (STequal(mname, typ->abrtflds[i].abraname))
		{
			ap = &(typ->abrtflds[i]);
			ldbv = &(ap->abradattype);
			datap = (PTR)((char *)rec + ap->abraoff);
			break;
		}
	}

	/* now set up targ to point to the object element. */

	if (ldbv->db_datatype == DB_DMY_TYPE)
	{
		PTR *dat = (PTR *)datap;

		/* db_data points to a pointer to another descriptor. */

		if (*dat == NULL)
		{
			/* 
			** This is the first reference to this complex attribute
			** in this record.  We must create an object
			** of the appropriate complex type.
			**
			** Note: If this is the application's first reference
			** to this attribute (from any record), we must look up
			** the class table entry for the complex type.
			** We'll save the index of the class table entry
			** we find in the ABRTSATTR.abradattype.db_prec field
			** (which was set to -1 by the first findType on the
			** parent record type).  This saves us from doing
			** repetitive lookups.
			*/
			if (ldbv->db_prec < 0)
			{
				ldbv->db_prec = findType(ap->abratname);
				if (ldbv->db_prec < 0)
				{
					/*
					** This frame is out of date ...
					** we didn't get a class.
					*/
					iiarUsrErr(BADCLASS, 1, ap->abratname);
					return FAIL;
				}
			}
			if (ldbv->db_length != 0)
			{
				*dat = (PTR)newArray( (i4)ldbv->db_prec,
						      rec->ro_co.co_pool );
			}
			else
			{
				*dat = (PTR)newRec( (i4)ldbv->db_prec,
						    rec->ro_co.co_pool );
			}
		}
		targ->db_data = *dat;
	}
	else
	{ /* it's a scalar. */
		targ->db_data = datap;
	}
	targ->db_datatype = ldbv->db_datatype;
	targ->db_length = ldbv->db_length;
	targ->db_prec = ldbv->db_prec;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\r\tDOT returns type=%ld, len=%ld, data=0x%lx\n\r"),
		(i4)targ->db_datatype,
		(i4)targ->db_length,
		(i4)targ->db_data );
#endif /* CLAS_TRACE */

	return OK;
}

/*
** Name:	getState 	- get STATE or RECORD special attribute.
**
** Description:
**	This routine looks to see if the DOT reference is to one of the
**	'special' attributes, _STATE or _RECORD.  This routine should only be
**	called after the normal lookup has failed.
**
** Inputs:
**	rec 	{R_OBJ *} 	Descriptor of the object.
**	mname	{char *}	Name of the object attribute.
**
** Outputs:
**	dbv 	{DB_DATA_VALUE *} Dbv to load object attribute info into.
**
** Returns:
**	STATUS	-- OK on success.
*/
static STATUS
getState( rec, mname, dbv )
R_OBJ *rec;
char *mname;
DB_DATA_VALUE *dbv;
{
	/* _STATE or _RECORD reference allowed only on array rows. */
	if (!(rec->ro_co.co_flags & CO_ELEMENT))
	{
		return FAIL;
	}

	if (STbcompare(mname, 0, ERx("_STATE"), 0, (bool)TRUE) == 0)
	{
		dbv->db_length = sizeof(rec->ro_state);
		dbv->db_data = (PTR)&(rec->ro_state);
	}
	else if (STbcompare(mname, 0, ERx("_RECORD"), 0, (bool)TRUE) == 0)
	{
		dbv->db_length = sizeof(rec->ro_record);
		dbv->db_data = (PTR)&(rec->ro_record);
	}
	else
	{
		return FAIL;
	}
	dbv->db_datatype = DB_INT_TYPE;
	dbv->db_prec = 0;

	return OK;
}

/*{
** Name:	IIARarrArrayRef() -	resolve an ARRAY reference.
**
** Description:
**	See comments for IIARdotDotRef for some IL explanation.
**
** Note:
** 	Illegal array index is an easy user error, so we have to be very
**	careful to handle it nicely.  (Bad DOT references mean the frame is
**	out of date, so it's OK to exit the frame abruptly.)
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding an array object.
**	index	{i4}  	  The array index.
**	targ 	{DB_DATA_VALUE *} Dbv to dereference into.
**	flags	{nat}		  Any combination of the following:
**				  ILM_HOLD_TARG: bump targ's reference count.
**				  ILM_RLS_SRC: release obj (decrement its ref
**				  count, or free it if ref count is 1).
**				  ILM_LHS: allow a reference to 1 row beyond
**				  the last row in the array (create a new row).
**				  Note: code (IL or C) created before 6.4/02
**				  may specify flags = 1; this is equivalent to
**				  ILM_LHS.
** Returns:
**	OK on success.
**
** History:
**	10/89 (billc) - written.
**	08/90 (jhw) -- Allow non-positive indices.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Logic has been extensively revised for new data structures.
**		New flags (modifier bits) are supported.
*/
STATUS
IIARarrArrayRef( obj, index, targ, flags )
DB_DATA_VALUE	*obj;
i4		index;
DB_DATA_VALUE	*targ;
i4		flags;
{
	A_OBJ	*arr;
	R_OBJ	*rec = NULL;
	STATUS	rval = OK;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rARRAY %s[%ld] (flags=%ld)\n\r"),
		ClassTable[((C_OBJ *)(obj->db_data))->co_type].abrtname,
		index, (i4)flags );
#endif /* CLAS_TRACE */

	arr = ckArray(obj);
	if (arr == NULL)
	{
		targ->db_data = obj->db_data;
		return FAIL;
	}

	if (index > 0)	/* reference to a non-deleted record */
	{
		if (index <= arr->ao_NumNonDel)
		{
			rec = arr->ao_base[index];
		}
		if ((flags & (ILM_LHS | 1)) && index == (arr->ao_NumNonDel + 1))
		{
			/*
			** This is an "x[i+1].a = n" situation,
			** where we automatically insert a new record.
			*/
			rec = insRow(arr, index);
		}
	}
	else		/* reference to a deleted record */
	{
		if (index > arr->ao_NegNumDel)
		{
			rec = arr->ao_top[index];
		}
	}

	/*
	** If we didn't find the array element, print an error and
	** default to the dummy element. (Allocate it if necessary).
	** If this a RHS reference, clear the dummy element.
	*/
	if (rec == NULL)
	{
		rval = FAIL;
		indexErr(index);
		rec = arr->ao_dummy;
		if (rec == NULL)
		{
			rec = newRec(arr->ao_co.co_type, arr->ao_pool);
			rec->ro_co.co_flags |= CO_ELEMENT;
			rec->ro_state = RO_ST_UNCHANGED;
			arr->ao_dummy = rec;
		}
		else if (!(flags & (ILM_LHS | 1)))
		{
			INCR_REF_COUNTER(rec->ro_co.co_ref_count);
			_VOID_ clrRec(rec);
			DECR_REF_COUNTER(rec->ro_co.co_ref_count);
		}
	}

	/*
	** If the target is to be held, increment its reference count
	** and its pool's reference count.
	*/
	if (flags & ILM_HOLD_TARG)
	{
		INCR_REF_COUNTER(rec->ro_co.co_ref_count);
		INCR_REF_COUNTER(rec->ro_co.co_pool->ph_ref_count);
	}

	/*
	** If the source is to be released, then release the array
	** after decrementing its pool's reference count.
	*/
	if (flags & ILM_RLS_SRC)
	{
		DECR_REF_COUNTER(arr->ao_co.co_pool->ph_ref_count);
		rlsObj((C_OBJ *)arr);
	}

	/*
	** Fill in the record's row number and return it.
	*/
	rec->ro_record = index;
	targ->db_data = (PTR)rec;
	return rval;
}

/*{
** Name:	IIARariArrayInsert() -	insert an ARRAY row.
**
** Description:
**	Insert a new row into the array at the given row number.
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding an array object.
**	index	{i4}  	  The 1-rel index of the row *after* which
**				  the new row is to be inserted.
** Returns:
**	OK on success.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Logic has been extensively revised for new data structures.
*/
STATUS
IIARariArrayInsert( obj, index )
DB_DATA_VALUE	*obj;
i4		index;
{
	A_OBJ	*arr;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rINSERT ARRAY %s[%ld]\n\r"),
		ClassTable[((C_OBJ *)(obj->db_data))->co_type].abrtname,
		index );
#endif /* CLAS_TRACE */

	arr = ckArray(obj);
	if (arr == NULL)
	{
		return FAIL;
	}
	if (index >= 0 && index <= arr->ao_NumNonDel)
	{
		_VOID_ insRow(arr, (i4)(index + 1));
		return OK;
	}
	indexErr(index);
	return FAIL;
}

/*{
** Name:	IIARardArrayDelete() -	Delete an ARRAY Record.
**
** Description:
**	Delete a record from an array.
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding an array object.
**	index	{i4}  	  The array index.
**
** Returns:
**	OK on success.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Logic has been extensively revised for new data structures.
*/
STATUS
IIARardArrayDelete( obj, index )
DB_DATA_VALUE	*obj;
i4		index;
{
	A_OBJ	*arr;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rDELETE ARRAY %s[%ld]\n\r"),
		ClassTable[((C_OBJ *)(obj->db_data))->co_type].abrtname,
		index );
#endif /* CLAS_TRACE */

	arr = ckArray(obj);
	if (arr == NULL)
	{
		return FAIL;
	}
	if (index > 0 && index <= arr->ao_NumNonDel)
	{
		_VOID_ delRow(arr, index);
		return OK;
	}
	indexErr(index);
	return FAIL;
}

/*{
** Name:	iiarArxRemoveRow() -	Remove an ARRAY Record.
**
** Description:
**	Remove a record from an array.
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding an array object.
**	index	{i4}  	  The record index.
**
** Returns:
**	{STATUS}  OK on success.
**
** History:
**	08/90 (jhw) - written from 'IIARardArrayDelete()'.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Logic has been extensively revised for new data structures.
*/
STATUS
iiarArxRemoveRow( obj, index )
DB_DATA_VALUE	*obj;
i4		index;
{
	register A_OBJ	*arr;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rDELETE ARRAY %s[%ld]\n\r"),
		ClassTable[((C_OBJ *)(obj->db_data))->co_type].abrtname,
		index );
#endif /* CLAS_TRACE */

	arr = ckArray(obj);
	if (arr == NULL)
	{
		return FAIL;
	}
	if (index > 0)
	{
		if (index <= arr->ao_NumNonDel)
		{
			rlsObj((C_OBJ *)rmvRow(arr, index));
			return OK;
		}
	}
	else
	{
		if (index > arr->ao_NegNumDel)
		{
			rlsObj((C_OBJ *)rmvDelRow(arr, index));
			return OK;
		}
	}
	indexErr(index);
	return FAIL;
}

/*{
** Name:	iiarArClearArray() -	Clear an Array.
**
** Description:
**	Clear all records from the array.
**
** Inputs:
**	adbv 	{DB_DATA_VALUE *} Data value holding an array object.
**
** Returns:
**	{STATUS}  OK on success.
**
** History:
**	08/90 (jhw) - written.  (Part of correction for #32963.)
**
**	07-jun-91 (davel)
**		Fix bug 37412 (return OK status)
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Logic has been extensively revised for new data structures.
*/
STATUS
iiarArClearArray( adbv )
DB_DATA_VALUE	*adbv;
{
	A_OBJ	*arr;

	arr = ckArray(adbv);
	if (arr == NULL)
	{
		return FAIL;
	}

	/*
	** Call clrArray to do the real work, but first temporarily
	** bump the array's reference count so that clrArray won't
	** decide to release the array.
	*/
	INCR_REF_COUNTER(arr->ao_co.co_ref_count);
	_VOID_ clrArray(arr);
	DECR_REF_COUNTER(arr->ao_co.co_ref_count);

	return OK;
}

/*{
** Name:	iiarArAllCount() -	count the elements in an array.
**
** Description:
**	Count the rows in an array.
**
** Input:
**	obj 	{DB_DATA_VALUE *} Dbv holding an array object.
**
** Outputs:
**	count	{i4 *}  	  where to put the count of undeleted rows.
**	dcount	{i4 *}  	  where to put the count of deleted rows.
**
** Returns:
**	{STATUS}  OK on success.
**
** History:
**	10/89 (billc) - written.
**	01/13/91 (emerson)
**		Add missing return statement at end of function (bug 34845).
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Logic has been extensively revised for new data structures.
*/
STATUS
iiarArAllCount( obj, count, dcount )
DB_DATA_VALUE	*obj;
i4		*count;
i4		*dcount;
{
	register A_OBJ	*arr;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rCOUNT ARRAY %s\n\r"),
		ClassTable[((C_OBJ *)(obj->db_data))->co_type].abrtname );
#endif /* CLAS_TRACE */

	arr = ckArray(obj);
	if (arr == NULL)
	{
		return FAIL;
	}
	if (count != NULL)
	{
		*count = arr->ao_NumNonDel;
	}
	if (dcount != NULL)
	{
		*dcount = - arr->ao_NegNumDel;
	}
	return OK;
}

/*{
** Name:	IIARaruArrayUnload() -	setup ARRAY for unloadtable.
**
** Description:
**	This sets an array up for UNLOADTABLE.
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding an array object.
**	tmpobj 	{DB_DATA_VALUE *} Dbv to hold the temp that we'll cycle through
**
** Returns:
**	OK on success.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Logic has been extensively revised for new data structures.
*/
STATUS
IIARaruArrayUnload( obj, tmpobj )
DB_DATA_VALUE *obj;
DB_DATA_VALUE *tmpobj;
{
	A_OBJ	*arr;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rUNLOAD ARRAY %s\n\r"),
		ClassTable[((C_OBJ *)(obj->db_data))->co_type].abrtname );
#endif /* CLAS_TRACE */

	/*
	** Terminate the current unloadtable, if any.
	*/
	endCurrentUnload();

	/*
	** Initialize the unloadtable temp to point to the array.
	** [This probably isn't necessary and in fact doesn't make
	** a whole lot of sense; IIARarnArrayNext will soon make
	** the unloadtable temp point to a *record*.  I've left this
	** assignment here because I don't fully understand what's
	** going on.  Emerson (11/07/91).]
	*/
	tmpobj->db_data = obj->db_data;

	/*
	** Make sure we have an array.
	*/
	arr = ckArray(obj);
	if (arr == NULL)
	{
		return FAIL;
	}

	/*
	** Indicate that the current frame is unloading the array;
	** push the current frame onto the array's list of active unloadtables.
	*/
	CurrentFrame->fe_UnldArray = arr;
	CurrentFrame->fe_NextSameArray = arr->ao_UnldFrame;
	arr->ao_UnldFrame = CurrentFrame;

	/*
	** Indicate that a reference into the array's memory pool
	** from outside in the form of the unloadtable temp
	** now exists (or will soon).
	*/
	INCR_REF_COUNTER(arr->ao_pool->ph_ref_count);

	return OK;
}

/*{
** Name:	IIARarnArrayNext() -	next ARRAY element for unloadtable.
**
** Description:
**	This gets the next element in an array.
**	It's called at the top of the UNLOADTABLE loop (on each iteration).
**
** Inputs:
**	tmpobj 	{DB_DATA_VALUE *} Dbv whose db_data will be set to point to
**				  the next element in the array, if found.
**
** Returns:
**	OK on success, FAIL if no more rows.
**
** History:
**	10/89 (billc) - written.
**	09/90 (jhw) - Don't check for array since the child usually is just
**		an element (array record.)
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Logic has been extensively revised for new data structures.
*/
STATUS
IIARarnArrayNext( tmpobj )
DB_DATA_VALUE *tmpobj;
{
	A_OBJ	*arr;
	R_OBJ	*rec;
	i4	index;

#ifdef CLAS_TRACE
	Tprint( ERx("\n\rARRAY NEXT %s\n\r"),
		ClassTable[((C_OBJ *)(tmpobj->db_data))->co_type].abrtname );
#endif /* CLAS_TRACE */

	/*
	** Make sure an unloadtable is active in the current frame.
	*/
	arr = CurrentFrame->fe_UnldArray;
	if (arr == NULL)
	{
		tmpobj->db_data = NULL;
		return FAIL;
	}

	/*
	** Bump unload row number.  If we go off the end of
	** non-deleted records, move into the deleted records.
	** If we don't find a record, terminate the unload.
	*/
	if (!(CurrentFrame->fe_flags & FE_UNLD_IN_DLT))
	{
		index = ++CurrentFrame->fe_UnldRownum;
		if (index <= arr->ao_NumNonDel)
		{
			rec = arr->ao_base[index];
			goto gotRec;
		}
		CurrentFrame->fe_flags |= FE_UNLD_IN_DLT;
		CurrentFrame->fe_UnldRownum = arr->ao_NegNumDel;
	}
	index = ++CurrentFrame->fe_UnldRownum;
	if (index <= 0)
	{
		rec = arr->ao_top[index];
		goto gotRec;
	}

	endCurrentUnload();

	tmpobj->db_data = NULL;
	return FAIL;

gotRec:	/* Come here if we find a record. */

	/*
	** Hold the record we found and set its _record attribute.
	*/
	INCR_REF_COUNTER(rec->ro_co.co_ref_count);
	rec->ro_record = index;

	/*
	** Release the last unloaded row, if any.
	*/
	if (CurrentFrame->fe_UnldRec != NULL)
	{
		rlsObj((C_OBJ *)CurrentFrame->fe_UnldRec);
	}

	/*
	** Make the record we found the current unloaded row, and return it.
	*/
	CurrentFrame->fe_UnldRec = rec;
	tmpobj->db_data = (PTR)rec;
	return OK;
}

/*{
** Name:	IIARareArrayEndUnload() - terminate an unloadtable of an ARRAY.
**
** Description:
**	This cleans up for an ENDLOOP out of an UNLOADTABLE.
**
** Inputs:
**	tmpobj 	{DB_DATA_VALUE *} Dbv used to hold the temp for the UNLOADTABLE.
**
** Returns:
**	OK on success.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
STATUS
IIARareArrayEndUnload( tmpobj )
DB_DATA_VALUE *tmpobj;
{
#ifdef CLAS_TRACE
	Tprint( ERx("\n\rARRAY END UNLOAD %s\n\r"),
		ClassTable[((C_OBJ *)(tmpobj->db_data))->co_type].abrtname );
#endif /* CLAS_TRACE */

	endCurrentUnload();

	tmpobj->db_data = NULL;
	return OK;
}

/*{
** Name:	iiarIarIsArray() -	see if a reference is to an ARRAY.
**
** Description:
**	This routine just checks an object to see if it's a real array.
**	(An element of an array doesn't count).
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding a possible array object.
**
** Returns:
**	bool		- is obj a real array?
**
** History:
**	11/14/90 (emerson)
**		written (for bugs 34438 and 34440).
*/
bool
iiarIarIsArray( DB_DATA_VALUE *obj )
{
	C_OBJ *desc;

	desc = (C_OBJ *)obj->db_data;

	if (desc == NULL || !(desc->co_flags & CO_ARRAY))
	{ /* No array. */
		return FALSE;
	}
	return TRUE;
}

/*{
** Name:	ckArray() -	check an ARRAY reference for validity.
**
** Description:
**	This routine just checks an object to make sure it's a real array.
**
** Inputs:
**	obj 	{DB_DATA_VALUE *} Dbv holding, we hope, an array object.
**
** Returns:
**	{A_OBJ *}	- the descriptor for the array object, or NULL.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Changed interface to ckArray: removed 2nd arg.
*/
static A_OBJ *
ckArray( obj )
DB_DATA_VALUE	*obj;
{
	C_OBJ *desc;

	desc = (C_OBJ *)obj->db_data;

	if (desc == NULL || !(desc->co_flags & CO_ARRAY))
	{ /* No array. */
		AB_TYPENAME	otype;

		iiarUsrErr(NOTARRAY, 1, iiarTypeName(obj, otype));

		return NULL;
	}
	return (A_OBJ *)desc;
}

/*
** Name:	endCurrentUnload() -	End current UNLOADTABLE (if any)
**
** Description:
**	This routine ends the UNLOADTABLE loop (if any) that's currently
**	active in the current frame or procedure.  [Note that no more
**	than one UNLOADTABLE may be active within a single frame or
**	procedure at any given time.  See notes for FRAME_ELT.]
**	All relevant fields in the current FRAME_ELT structure are cleared.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static VOID
endCurrentUnload( )
{
	A_OBJ	*arr;

	/*
	** Return immediately if no unloadtable is active in the current frame.
	*/
	arr = CurrentFrame->fe_UnldArray;
	if (arr == NULL)
	{
		return;
	}

	/*
	** Release the last unloaded row, if any.
	*/
	if (CurrentFrame->fe_UnldRec != NULL)
	{
		rlsObj((C_OBJ *)CurrentFrame->fe_UnldRec);
	}

	/*
	** Indicate that the reference into the array's memory pool
	** from outside in the form of the unloadtable temp
	** no longer exists.
	*/
	DECR_REF_COUNTER(arr->ao_pool->ph_ref_count);

	/*
	** Pop the current FRAME_ELT structure off the array's list
	** of active unloadtables.
	*/
	arr->ao_UnldFrame = CurrentFrame->fe_NextSameArray;

	/*
	** Clear all relevant fields in the current FRAME_ELT structure.
	*/
	CurrentFrame->fe_UnldArray = NULL;
	CurrentFrame->fe_NextSameArray = NULL;
	CurrentFrame->fe_UnldRec = NULL;
	CurrentFrame->fe_UnldRownum = 0;
	CurrentFrame->fe_flags = 0;
}

/*{
** Name:	insRow() -		insert an ARRAY row.
**
** Description:
**	Internal routine to insert a new row into an array.
**
** Inputs:
**	arr 	{A_OBJ *}	Array into which row is to be inserted.
**	index	{i4}  	The 1-rel index of the row to be inserted.
**				The caller must ensure this is a legit index.
** Returns:
**	{R_OBJ *}	The inserted row.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static R_OBJ *
insRow( arr, index )
A_OBJ	*arr;
i4	index;
{
	R_OBJ	**recp, **index_recp, *rec;
	FRAME_ELT *frame;

	/*
	** If pointer array is full, create a new one twice as big.
	** If there's no pointer array yet, create one that's exactly
	** as big as a record of the array (that way we can reuse
	** an old pointer array for record(s) when have to double it).
	*/
	if (arr->ao_NumNonDel - arr->ao_NegNumDel >= arr->ao_NumSlots)
	{
		R_OBJ	**ptr_array;
		R_OBJ	**old_base = arr->ao_base;
		R_OBJ	**old_top  = arr->ao_top;
		POOL_HDR *pool = arr->ao_pool;
		i4	recsize = ClassTable[arr->ao_co.co_type].abrtsize;

		if (arr->ao_NumSlots == 0)	/* new pointer array */
		{
			arr->ao_NumSlots = recsize / sizeof(R_OBJ *);

			ptr_array = (R_OBJ **)getObj(recsize, pool);
		}
		else				/* pre-existing pointer array */
		{
			u_i4	num_bytes;

			arr->ao_NumSlots <<= 1;	/* double the number of ptrs */

			num_bytes = arr->ao_NumSlots * sizeof(R_OBJ *);
			ptr_array = (R_OBJ **)me_req( pool->ph_tag, num_bytes,
						      ERx("insRow") );
		}

		arr->ao_base = ptr_array - 1;
		arr->ao_top  = arr->ao_base + arr->ao_NumSlots;

		if (arr->ao_NumNonDel > 0)
		{
			IIUGclCopyLong( (PTR)(old_base + 1),
				(u_i4)(arr->ao_NumNonDel * sizeof(R_OBJ *)),
				(PTR)(arr->ao_base + 1) );
		}
		if (arr->ao_NegNumDel < 0)
		{
			IIUGclCopyLong( (PTR)(old_top + arr->ao_NegNumDel + 1),
				(u_i4)(- arr->ao_NegNumDel * sizeof(R_OBJ *)),
				(PTR)(arr->ao_top + arr->ao_NegNumDel + 1) );
		}

		freeBlock( (PTR)(old_base + 1),
			   (char *)old_top - (char *)old_base,
			   recsize, pool );
	}

	/*
	** Shift non-deleted row slots up above insertion point;
	** bump the number of non-deleted rows.
	*/
	index_recp = arr->ao_base + index;
	recp = arr->ao_base + arr->ao_NumNonDel;
	++arr->ao_NumNonDel;
	while (recp >= index_recp)
	{
		*(recp + 1) = *recp;
		--recp;
	}

	/*
	** Create the new row and store its address in the pointer array.
	*/
	*index_recp = rec = newRec(arr->ao_co.co_type, arr->ao_pool);

	rec->ro_co.co_flags |= CO_ELEMENT;
	rec->ro_state = RO_ST_UNCHANGED;

	/*
	** If there are any unloadtables in progress against the array,
	** make any necessary adjustment to the index of the current
	** unloaded row.
	*/
	for ( frame = arr->ao_UnldFrame; frame != NULL;
	      frame = frame->fe_NextSameArray )
	{
		if (frame->fe_UnldRownum >= index)
		{
			frame->fe_UnldRec->ro_record = ++frame->fe_UnldRownum;
		}
	}

	/*
	** Return the newly-inserted row.
	*/
	return rec;
}

/*{
** Name:	delRow() -		delete an ARRAY row.
**
** Description:
**	Internal routine to delete a row from an array.
**
** Inputs:
**	arr 	{A_OBJ *}	Array from which row is to be deleted.
**	index	{i4}  	The 1-rel index of the row to be deleted.
**				The caller must ensure this is a legit index.
** Returns:
**	{R_OBJ *}	The deleted row.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static R_OBJ *
delRow( arr, index )
A_OBJ	*arr;
i4	index;
{
	R_OBJ	*rec;

	/*
	** Remove the record from the non-deleted rows of the array.
	*/
	rec = rmvRow(arr, index);

	/*
	** Mark the record as deleted;
	** stick it at the bottom of the deleted rows of the array;
	** bump the number of deleted rows.
	*/
	rec->ro_state = RO_ST_DELETED;
	arr->ao_top[arr->ao_NegNumDel] = rec;
	--arr->ao_NegNumDel;

	/*
	** Return the newly-deleted row.
	*/
	return rec;
}

/*{
** Name:	rmvRow() -		remove a non-deleted ARRAY row.
**
** Description:
**	Internal routine to remove a non-deleted row from an array.
**	The row is *not* released.
**
** Inputs:
**	arr 	{A_OBJ *}	Array from which row is to be removed.
**	index	{i4}  	The 1-rel index of the row to be removed.
**				The caller must ensure this is a legit index.
** Returns:
**	{R_OBJ *}	The removed row.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static R_OBJ *
rmvRow( arr, index )
A_OBJ	*arr;
i4	index;
{
	R_OBJ	**recp, **end_recp, *rec;
	FRAME_ELT *frame;

	/*
	** Decrement the number of non-deleted rows;
	** mark the record as removed by setting its state to 0
	** (not really necessary, but copacetic);
	** shift non-deleted row slots down above removal point.
	*/
	--arr->ao_NumNonDel;
	recp = arr->ao_base + index;
	rec = *recp;
	rec->ro_state = 0;
	end_recp = arr->ao_base + arr->ao_NumNonDel;
	while (recp <= end_recp)
	{
		*recp = *(recp + 1);
		++recp;
	}

	/*
	** If there are any unloadtables in progress against the array,
	** make any necessary adjustment to the index of the current
	** unloaded row.  Note that if we're removing the current
	** unloaded row, we decrement fe_UnldRownum so that the next
	** iteration of the unloadtable won't skip the next row,
	** but we don't decrement the ro_record in the removed row.
	*/
	for ( frame = arr->ao_UnldFrame; frame != NULL;
	      frame = frame->fe_NextSameArray )
	{
		if (frame->fe_UnldRownum >= index)
		{
			i4	unld_index;

			unld_index = --frame->fe_UnldRownum;
			if (unld_index >= index)
			{
				frame->fe_UnldRec->ro_record = unld_index;
			}
		}
	}

	/*
	** Return the newly-removed row.
	*/
	return rec;
}

/*{
** Name:	rmvDelRow() -		remove a deleted ARRAY row.
**
** Description:
**	Internal routine to remove a deleted row from an array.
**	The row is *not* released.
**
** Inputs:
**	arr 	{A_OBJ *}	Array from which row is to be removed.
**	index	{i4}  	The 1-rel index of the row to be removed.
**				The caller must ensure this is a legit index.
** Returns:
**	{R_OBJ *}	The removed row.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static R_OBJ *
rmvDelRow( arr, index )
A_OBJ	*arr;
i4	index;
{
	R_OBJ	**recp, **end_recp, *rec;
	FRAME_ELT *frame;

	/*
	** Decrement the number of deleted rows;
	** mark the record as removed by setting its state to 0
	** (not really necessary, but copacetic);
	** shift deleted row slots up below removal point.
	*/
	++arr->ao_NegNumDel;
	recp = arr->ao_top + index;
	rec = *recp;
	rec->ro_state = 0;
	end_recp = arr->ao_top + arr->ao_NegNumDel;
	while (recp > end_recp)
	{
		*recp = *(recp - 1);
		--recp;
	}

	/*
	** If there are any unloadtables in progress against the array,
	** make any necessary adjustment to the index of the current
	** unloaded row.
	*/
	for ( frame = arr->ao_UnldFrame; frame != NULL;
	      frame = frame->fe_NextSameArray )
	{
		if (frame->fe_UnldRownum < index)
		{
			frame->fe_UnldRec->ro_record = ++frame->fe_UnldRownum;
		}
	}

	/*
	** Return the newly-removed row.
	*/
	return rec;
}

/*
** Name:	newArray() -	Create a new array.
**
** Description:
**	This routine creates an array of a given class.
**
** Inputs:
**	typ	{nat}		index in class table of the class (datatype).
**	pool	{POOL_HDR *}	pool to allocate from.
**
** Returns:
**	{A_OBJ *}	pointer to the descriptor.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static A_OBJ *
newArray( typ, pool )
i4	typ;
POOL_HDR *pool;
{
	A_OBJ	*arr;
	i4	len = ROUNDED_SIZEOF(A_OBJ, POOL_Q_HDR_SIZE); /* memory reqd */

	/*
	** If this is a "primary" (non-nested) array, then it will get its own
	** memory pool, so we'll get a POOL_HDR structure with the same
	** request as for the array's A_OBJ structure.  If the array has
	** no complex attributes, we also get a single POOL_Q_HDR structure,
	** for buckets of the size of the array's rows [we know this is the
	** only bucket size we'll encounter, so we can make our "array"
	** of POOL_Q_HDR structures consist of just this one POOL_Q_HDR].
	**
	** Also note that "primary" objects (objects not within an array)
	** are allocated via individual FEreqlng requests: non-array pools
	** don't use free lists (because objects outside arrays are few
	** and seldom freed).  To prevent clrArray from attempting to free
	** a primary array (which would manipulate non-existant free lists),
	** we set its reference count to PERM_OBJ_REF_COUNT instead of 1.
	*/
	if (!(pool->ph_flags & PH_ARRAY))
	{
		char	*ptr;
		POOL_HDR *arr_pool;

		len += ROUNDED_SIZEOF(POOL_HDR, POOL_Q_HDR_SIZE);

		if (NumComplexAttrs[typ] == 0)
		{
			len += POOL_Q_HDR_SIZE;
		}

		ptr = (char *)fe_req( pool->ph_tag, (u_i4)len,
				      ERx("newArray") );
		MEfill((u_i2)len, '\0', (PTR)ptr);

		arr = (A_OBJ *)ptr;
		ptr += ROUNDED_SIZEOF(A_OBJ, POOL_Q_HDR_SIZE);
		arr_pool = (POOL_HDR *)ptr;

		if (NumComplexAttrs[typ] == 0)
		{
			i4	rowsize = ClassTable[typ].abrtsize;

			ptr += ROUNDED_SIZEOF(POOL_HDR, POOL_Q_HDR_SIZE);
			arr_pool->ph_max_bkt_size = rowsize;
			arr_pool->ph_q_hdrs = ptr - rowsize;
		}

		arr_pool->ph_flags = PH_ARRAY;
		arr_pool->ph_tag = FEgettag();

		arr_pool->ph_next_pool = pool->ph_next_pool;
		pool->ph_next_pool = arr_pool;

		arr->ao_co.co_ref_count = PERM_OBJ_REF_COUNT;
		arr->ao_pool = arr_pool;
	}
	else	/* new array is nested within another array */
	{
		arr = (A_OBJ *)getObj(len, pool);
		MEfill((u_i2)len, '\0', (PTR)arr);

		arr->ao_co.co_ref_count = 1;
		arr->ao_pool = pool;
	}

	/*
	** Set the remaining non-zero fields in the A_OBJ structure.
	*/
	arr->ao_co.co_pool = pool;
	arr->ao_co.co_type = typ;
	arr->ao_co.co_flags = CO_ARRAY;

	return arr;
}

/*
** Name:	newRec() -	Create a new record.
**
** Description:
**	This routine creates a record for a given class.
**
** Inputs:
**	typ	{nat}		index in class table of the class (datatype).
**	pool	{POOL_HDR *}	pool to allocate from.
**
** Returns:
**	{R_OBJ *}	pointer to the descriptor.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Renamed from old newObj (arrays are now created by newArray).
**		Logic has been extensively revised for new data structures.
**
**		Third argument (flags) has been removed.
**		First argument is now an index of a class table entry
**		instead of a type name.  The logic to find the class table entry
**		given a type name has been moved into a new function (findType).
**		Previously, this lookup was done every time a row was inserted
**		into an array.
*/
static R_OBJ *
newRec( typ, pool )
i4	typ;
POOL_HDR *pool;
{
	R_OBJ		*rec;
	REF_COUNTER	ref_count;
	i4		recsize = ClassTable[typ].abrtsize;

	/*
	** "Primary" objects (objects not within an array)
	** are allocated via individual FEreqlng requests: non-array pools
	** don't use free lists (because objects outside arrays are few
	** and seldom freed).  To prevent clrRec or rlsObj from attempting to
	** free a primary rec (which would manipulate non-existant free lists),
	** we set its reference count to PERM_OBJ_REF_COUNT instead of 1.
	*/
	if (!(pool->ph_flags & PH_ARRAY))
	{
		rec = (R_OBJ *)fe_req( pool->ph_tag, (u_i4)recsize,
				       ERx("newRec"));

		ref_count = PERM_OBJ_REF_COUNT;
	}
	else	/* new record is within an array */
	{
		rec = (R_OBJ *)getObj(recsize, pool);

		ref_count = 1;
	}

	/*
	** Clear the "fixed" portion of the R_OBJ structure, *and*
	** all the pointers (if any) to nested complex objects.
	*/
	MEfill( (u_i2)( sizeof(R_OBJ)
		      + (NumComplexAttrs[typ] - 1) * sizeof(C_OBJ *) ),
		'\0', (PTR)rec );

	/*
	** Set the non-zero fields in the R_OBJ structure.
	*/
	rec->ro_co.co_ref_count = ref_count;
	rec->ro_co.co_pool = pool;
	rec->ro_co.co_type = typ;

	/*
	** Initialize simple attributes.
	*/
	initSimpleAttrs(rec);

	return rec;
}

/*
** Name:	rlsObj() -	Release an object
**
** Description:
**	This routine releases an object:  If its reference count is 1,
**	the object is freed (its memory is returned to its memory pool),
**	and all its children (records or arrays) are released.
**	If the object's reference count is greater than 1, it's decremented.
**
** Inputs:
**	obj	{C_OBJ *}	pointer to the object's descriptor
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static VOID
rlsObj( obj )
C_OBJ	*obj;
{
	C_OBJ	**obj_p, **end_obj_p;

	/*
	** If the object's reference count is greater than 1,
	** decrement it and return.
	*/
	if (obj->co_ref_count > 1)
	{
		DECR_REF_COUNTER(obj->co_ref_count);
		return;
	}

	/*
	** If the object is an array, free it by calling clrArray.
	*/
	if (obj->co_flags & CO_ARRAY)
	{
		_VOID_ clrArray((A_OBJ *)obj);
		return;
	}

	/*
	** If the object is a record, recursively release its children
	** and then free it.
	*/
	obj_p = ((R_OBJ *)obj)->ro_objs;
	end_obj_p = obj_p + NumComplexAttrs[obj->co_type];

	while (obj_p < end_obj_p)
	{
		if (*obj_p != NULL)
		{
			rlsObj(*obj_p);
		}
		++obj_p;
	}

	freeObj((PTR)obj, (i4)ClassTable[obj->co_type].abrtsize, obj->co_pool);
}

/*
** Name:	clrArray() -	Clear an array
**
** Description:
**	This routine clears an array:  all its rows are removed (released,
**	not cleared) along with its pointer array.  If the array is
**	not nested within another array, then its associated array of
**	POOL_Q_HDR structures is also removed (but the POOL_HDR remains).
**	If the array's reference count is 1 (which is possible only for
**	arrays nested within other arrays), the array itself is released.
**
** Inputs:
**	arr	{A_OBJ *}	pointer to the array's descriptor
**
** Returns:
**	{C_OBJ *}	the pointer to the array's descriptor,
**			or NULL if the array itself was released.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static C_OBJ *
clrArray( arr )
A_OBJ	*arr;
{
	FRAME_ELT	*frame;
	POOL_HDR	*pool = arr->ao_pool;

	/*
	** If there are any unloadtables in progress against the array
	** being cleared, reset them so they'll start over from the beginning.
	** (Actually, it's not supported to clear an array that's being
	** unloaded, but we try to handle it gracefully).
	*/
	for ( frame = arr->ao_UnldFrame; frame != NULL;
	      frame = frame->fe_NextSameArray )
	{
		frame->fe_UnldRownum = 0;
		frame->fe_flags = 0;
	}

	/*
	** If this is a "primary" (non-nested) array, then it has its own
	** memory pool.  If that pool has no references from outside,
	** we simply free the entire pool.  But we have to fix up free list
	** pointers:  If the array has no complex attributes, we allocated
	** a single POOL_Q_HDR (for buckets of the size of the array's rows)
	** using the *parent* storage tag - it persists, but we must clear it.
	** If, however, the array *has* complex attributes, there's an array
	** of POOL_Q_HDR structures allocated using the *array* tag.  Since
	** this will be freed, we must clear the fields containing its length
	** and address.
	*/
	if (  !(arr->ao_co.co_pool->ph_flags & PH_ARRAY)
	   && pool->ph_ref_count == 0 )
	{
		/*
		** Free the pool.  Note that we use MEtfree rather than FEfree
		** even though we acquired the tag using FEgettag.
		** This necessary (and safe) because all memory allocations
		** against this pool used MEreqlng rather than FEreqlng,
		** and FEfree would skip its internal MEtfree (thinking
		** there was no memory to be freed).
		*/
		_VOID_ MEtfree((u_i2)pool->ph_tag);

		if (NumComplexAttrs[arr->ao_co.co_type] == 0)
		{
			MEfill( (u_i2)POOL_Q_HDR_SIZE, '\0',
				(PTR)(pool->ph_q_hdrs + pool->ph_max_bkt_size)
			      );
		}
		else
		{
			pool->ph_max_bkt_size = 0;
			pool->ph_q_hdrs = NULL;
		}
	}
	/*
	** If this is a nested array, or if this is a non-nested array
	** that still has references from outside the array pool,
	** we fall back to plan B: we go through all the unreferenced objects
	** in the array and free them (add them to the appropriate free list).
	** Note that the pointer array itself has been cleverly allocated
	** to have a size that's a multiple of the size of the records
	** of the array, so we can divvy it up and add the pieces to the
	** free list for the records of the array.
	*/
	else
	{
		R_OBJ	**recp, **end_recp;

		recp = arr->ao_top;
		end_recp = recp + arr->ao_NegNumDel;
		while (recp > end_recp)
		{
			rlsObj((C_OBJ *)*recp);
			--recp;
		}

		recp = arr->ao_base;
		end_recp = recp + arr->ao_NumNonDel;
		while (recp < end_recp)
		{
			++recp;
			rlsObj((C_OBJ *)*recp);
		}

		if (arr->ao_dummy != NULL)
		{
			rlsObj((C_OBJ *)arr->ao_dummy);
		}

		freeBlock( (PTR)(arr->ao_base + 1),
			   (char *)(arr->ao_top) - (char *)(arr->ao_base),
			   (i4)ClassTable[arr->ao_co.co_type].abrtsize,
			   pool );
	}

	/*
	** If the array's reference count is 1, free its A_OBJ structure.
	** Otherwise, clear appropriate fields in the array's A_OBJ structure.
	*/
	if (arr->ao_co.co_ref_count <= 1)
	{
		freeObj((PTR)arr, ROUNDED_SIZEOF(A_OBJ, POOL_Q_HDR_SIZE), pool);

		return NULL;
	}

	arr->ao_dummy = NULL;
	arr->ao_base  = NULL;
	arr->ao_top   = NULL;

	arr->ao_NumSlots  = 0;
	arr->ao_NumNonDel = 0;
	arr->ao_NegNumDel = 0;

	return (C_OBJ *)arr;
}

/*
** Name:	clrRec() -	Clear a record
**
** Description:
**	This routine clears a record:  All its simple attributes
**	are initialized, all its array attributes are cleared, and
**	all its record attributes are cleared by recursively calling
**	this routine.
**
**	If the record's reference count is 1, and all recursively nested records
**	have reference counts of 1, and all their arrays have reference counts
**	of 1, the record is released rather than being cleared.
**
** Inputs:
**	rec	{R_OBJ *}	pointer to the record's descriptor
**
** Returns:
**	{C_OBJ *}	the pointer to the record's descriptor,
**			or NULL if the record was released.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static C_OBJ *
clrRec( rec )
R_OBJ	*rec;
{
	C_OBJ	*obj;
	C_OBJ	**obj_p = rec->ro_objs;
	C_OBJ	**end_obj_p = obj_p + NumComplexAttrs[rec->ro_co.co_type];
	bool	all_children_freed = TRUE;

	while (obj_p < end_obj_p)
	{
		obj = *obj_p;
		if (obj != NULL)
		{
			if (obj->co_flags & CO_ARRAY)
			{
				obj = clrArray((A_OBJ *)obj);
			}
			else
			{
				obj = clrRec((R_OBJ *)obj);
			}
			*obj_p = obj;

			if (obj != NULL)
			{
				all_children_freed = FALSE;
			}
		}
		++obj_p;
	}

	/*
	** If the record's reference count is 1,
	** and we were able to free all its children, then free it.
	** Otherwise, initialize its simple attributes.
	*/
	if (rec->ro_co.co_ref_count <= 1 && all_children_freed)
	{
		freeObj( (PTR)rec,
			 (i4)ClassTable[rec->ro_co.co_type].abrtsize,
			 rec->ro_co.co_pool );

		return NULL;
	}

	initSimpleAttrs(rec);

	return (C_OBJ *)rec;
}

/*
** Name:	initSimpleAttrs - Clear a Record's simple attributes.
**
** Description:
**	This routine initializes a record's simple attributes to default values.
**
** Inputs:
**	rec	{R_OBJ *} descriptor of the record.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
**		(Extracted from old initRow function).
*/
static VOID
initSimpleAttrs( rec )
register R_OBJ *rec;
{
	ABRTSATTR	*ap, *end_ap;
	ABRTSTYPE	*typ = &ClassTable[rec->ro_co.co_type];

	for (ap = typ->abrtflds, end_ap = ap + typ->abrtcnt; ap < end_ap; ap++)
	{
		DB_DATA_VALUE	*dbdv = &(ap->abradattype);

		if (dbdv->db_datatype != DB_DMY_TYPE)
		{
			dbdv->db_data = (PTR)((char *)rec + ap->abraoff);
			iiarInitDbv(Adf_cb, dbdv);
			dbdv->db_data = NULL;
		}
	}
}

/*
** Name:	getObj() -	Get an object
**
** Description:
**	This routine gets an object from a free list.
**	If the free list is empty, it is replenished.
**	The object is assumed to be a row of an array,
**	or a record or array nested within a row of an array.
**
** Inputs:
**	len	{nat}		the length of the object
**				(must be a multiple of POOL_Q_HDR_SIZE).
**	pool	{POOL_HDR *}	the pool containing the free list
**				from which the object should be obtained.
**
** Returns:
**	datap	{PTR}		pointer to the object to be freed.
**
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static PTR
getObj( len, pool )
i4	len;
POOL_HDR *pool;
{
	char		*datap;
	u_i4	num_bytes;
	POOL_Q_HDR	*pool_q;

	/*
	** If the buffer currently at pool->ph_q_hdrs + POOL_Q_HDR_SIZE
	** doesn't handle objects as big as the current request,
	** then we'll need to enlarge it.  We'll allocate a new buffer
	** just big enough for the current request, and copy the old buffer
	** into it.  The old buffer is then put on the free list for its size.
	*/
	if (len > pool->ph_max_bkt_size)
	{
		i4	bkts, actual_bkts, max_bkts, old_buffer_size;
		char	*old_buffer;

		/*
		** Determine how many buckets we'll pack into our biggest
		** MEreqlng for this queue, and determine how many we'll pack
		** into the initial MEreqlng for this queue (below).
		** Note that we'll be using the first bucket as the new
		** pool->ph_q_hdrs buffer.
		*/
		max_bkts = MAX_RECOMMENDED_REQMEM / len;
		if (max_bkts == 0)
		{
			max_bkts = 1;
		}
		actual_bkts = bkts = (MIN_INITIAL_POOL_Q_REQMEM - 1) / len + 1;
		if (actual_bkts < max_bkts)
		{
			actual_bkts += 1; /* account for the new buffer */
		}

		/*
		** Get the storage block.
		*/
		num_bytes = actual_bkts * len;
		datap = (char *)me_req(pool->ph_tag, num_bytes, ERx("getObj"));

		/*
		** Copy the old pool->ph_q_hdrs buffer to the new
		** (which we'll place at the beginning of the block we got).
		** Fill the remainder of the new buffer with binary zeroes.
		**
		** Free the old pool->ph_q_hdrs buffer
		** and make pool->ph_q_hdrs point to the new one.
		*/
		old_buffer = pool->ph_q_hdrs + POOL_Q_HDR_SIZE;
		old_buffer_size = pool->ph_max_bkt_size;

		pool->ph_q_hdrs = datap - POOL_Q_HDR_SIZE;
		pool->ph_max_bkt_size = len;

		if (old_buffer_size > 0)
		{
			MEmove( (u_i2)old_buffer_size, (PTR)old_buffer,
				'\0', (u_i2)len, (PTR)datap );

			freeObj((PTR)old_buffer, old_buffer_size, pool);
		}
		else
		{
			MEfill( (u_i2)len, '\0', (PTR)datap );
		}

		/*
		** Fill in the max number of buckets per MEreqlng
		** and the number of buckets in the first MEreqlng
		** for the new free list (queue).  Note that we fudge
		** the latter, because we want to compensate for the fact
		** that we tacked an extra bucket (for the new pool->ph_q_hdrs
		** buffer) onto that initial MEreqlng.
		*/
		pool_q = (POOL_Q_HDR *)(pool->ph_q_hdrs + len);
		pool_q->qh_max_bkts = max_bkts;
		pool_q->qh_bkt_incr = bkts;

		/*
		** We've used the first bucket in the storage block
		** for the new pool->ph_q_hdrs buffer.
		** Now divvy up the remainder of the storage block
		** into buckets and add them to the new free list.
		*/
		freeBlock((PTR)(datap + len), num_bytes - len, len, pool);
	}

	/*
	** Find the free list for size of bucket we want.
	** If it's non-empty, we'll take the first bucket and return it.
	** If it's empty, we'll replenish it.
	*/
	pool_q = (POOL_Q_HDR *)(pool->ph_q_hdrs + len);
	datap = (char *)pool_q->qh_free;
	if (datap != NULL)
	{
		pool_q->qh_free = *((PTR *)datap);
		return (PTR)datap;
	}

	/*
	** The free list is empty.  We must replenish it.
	** The first step is to determine the number of buckets to acquire.
	** If this is a new free list, we'll compute qh_max_bkts and acquire
	** at least MIN_INITIAL_POOL_Q_REQMEM bytes worth of buckets.
	** If this is a pre-existing free list, we'll acquire
	** 1.5 times (rounded up!) as many buckets as we did
	** the last time we replenished this free list (except that
	** we never acquire more than qh_max_bkts buckets).
	*/
	if (pool_q->qh_bkt_incr == 0)	/* new list */
	{
		pool_q->qh_max_bkts = MAX_RECOMMENDED_REQMEM / len;

		if (pool_q->qh_max_bkts == 0)
		{
			pool_q->qh_max_bkts = 1;
		}
		pool_q->qh_bkt_incr = (MIN_INITIAL_POOL_Q_REQMEM - 1) / len + 1;
	}
	else				/* pre-existing list */
	{
		pool_q->qh_bkt_incr += ((pool_q->qh_bkt_incr + 1) >> 1);

		if (pool_q->qh_bkt_incr > pool_q->qh_max_bkts)
		{
			pool_q->qh_bkt_incr = pool_q->qh_max_bkts;
		}
	}

	/*
	** Get a block of storage to replenish the list.
	*/
	num_bytes = pool_q->qh_bkt_incr * len;
	datap = (char *)me_req(pool->ph_tag, num_bytes, ERx("getObj"));

	/*
	** Divvy up the storage block into buckets and add them to the free list
	** (except for the last bucket, which we'll return).
	*/
	freeBlock((PTR)datap, num_bytes - len, len, pool);

	return (PTR)(datap + num_bytes - len);
}

/*
** Name:	freeObj() -	Free an object
**
** Description:
**	This routine adds an object to a free list.
**
** Inputs:
**	datap	{PTR}		pointer to the object to be freed.
**	len	{nat}		the length of the object;
**				must be a multiple of POOL_Q_HDR_SIZE.
**	pool	{POOL_HDR *}	the pool containing the free list
**				to which the object should be added.
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static VOID
freeObj( datap, len, pool )
PTR	datap;
i4	len;
POOL_HDR *pool;
{
	POOL_Q_HDR	*pool_q = (POOL_Q_HDR *)(pool->ph_q_hdrs + len);

	*((PTR *)datap) = pool_q->qh_free;
	pool_q->qh_free = datap;
}

/*
** Name:	freeBlock() -	Free a block of objects
**
** Description:
**	This routine adds a block of contiguous objects to a free list.
**
** Inputs:
**	blk_p	{PTR}		pointer to the block of objects to be freed.
**	blk_len	{nat}		the length of the block;
**				must be a multiple of obj_len (0 is okay).
**	obj_len	{nat}		the length of an individual object in the block;
**				must be a multiple of POOL_Q_HDR_SIZE.
**	pool	{POOL_HDR *}	the pool containing the free list
**				to which the objects should be added.
** History:
**	11/07/91 (emerson)
**		Created (for revamped array processing 
**		for bugs 39581, 41013, and 41014).
*/
static VOID
freeBlock( blk_p, blk_len, obj_len, pool )
PTR	blk_p;
i4	blk_len;
i4	obj_len;
POOL_HDR *pool;
{
	POOL_Q_HDR	*pool_q = (POOL_Q_HDR *)(pool->ph_q_hdrs + obj_len);
	char		*obj_p = (char *)blk_p;
	char		*end_blk_p = obj_p + blk_len;

	while (obj_p < end_blk_p)
	{
		*((PTR *)obj_p) = pool_q->qh_free;
		pool_q->qh_free = (PTR)obj_p;
		obj_p += obj_len;
	}
}

/*
** Name:	findType() -	Given a type name, find the type in class table
**
** Description:
**	This routine locates the entry in the class table for a specified type.
**	If necessary, the data area size and the offsets of each attribute
**	in the class table entry will be calculated.
**
** Inputs:
**	tname	{char *}	name of the class (datatype).
**
** Returns:
**	{nat}		index of the type's entry in the class table,
**			or -1 (if not found, or if an error occurs
**			while calculating attribute offsets).
**
** History:
**	11/07/91 (emerson)
**		Created from old newObj (for revamped array processing).
*/
static i4
findType( tname )
char *tname;
{
	ABRTSTYPE	*typ = NULL;
	register i4	h;
	char		nbuf[ABBUFSIZE+1];

	i4	iiarThTypeHash();

	if (ClassTable == NULL)
	{
		return -1;
	}

	/* copy the name -- iiarThTypeHash lowercases it in place. */
	STlcopy(tname, nbuf, sizeof(nbuf));
	for (h = ClassHash[iiarThTypeHash(nbuf)]; h != -1; h = typ->abrtnext)
	{
		typ = &(ClassTable[h]);
		if (STequal(tname, typ->abrtname))
		{
			/*
			** We found the datatype name.
			** If this is the first reference to the datatype's
			** class table entry, we must calculate the data area
			** size and the offsets of the attributes.
			*/
			if (typ->abrtsize < 0)
			{
				if (calcOffsets(h) != OK)
				{
					return -1;
				}
			}
			break;
		}
	}
	return h;
}

/*
** Name:	calcOffsets() -	calculate offsets for an ABRTSTYPE.
**
** Description:
**	This routine figures out the data offsets and sundry other info
**	for a class.  It only gets called (at most) once per application
**	on any particular class.
**
**	The information computed is:
**
**	(1) The offsets of each attribute (within a rec of the specified type).
**	    Complex attributes are placed before simple attributes.
**	(2) The total size of a record of the specified type, rounded up
**	    to a multiple of POOL_Q_HDR_SIZE.
**	(3) The number of complex attributes.
**	    (This is placed in NumComplexAttrs[typ_num]).
**
** Inputs:
**	typ_num	{nat} - index of type for which information is to be computed.
**
** History:
**	10/89 (billc) - written.
**	11/07/91 (emerson)
**		Revamped array processing for bugs 39581, 41013, and 41014:
**		Input is now an index, not a pointer.
**		Leave room at beginning of data area for R_OBJ structure.
**		Round up abrtsize.  Put complex attribute pointers ahead of
**		simple attribute data areas.  Calculate NumComplexAttrs.
**
**		Make dp and olddp char* instead of PTR,
**		because we do pointer arithmetic on them.
*/
static STATUS
calcOffsets( typ_num )
i4  typ_num;
{
	char	*dp;
	char	*olddp;
	ABRTSATTR *ap;
	ABRTSTYPE *typ = &ClassTable[typ_num];
	ABRTSATTR *bgn_ap = typ->abrtflds;
	ABRTSATTR *end_ap = bgn_ap + typ->abrtcnt;

	i4	num_complex_attrs = 0;

	/*
	** We need a safe pointer to figure offsets from.
	** A pointer to any legitimate address will do,
	** since we're not really putting anything there.
	*/
	dp = (char *)&dp;
	dp = (char *)ME_ALIGN_MACRO((PTR)dp, POOL_Q_HDR_SIZE);
	olddp = dp;

	/*
	** First pass through the attributes of the object:
	** look for complex attributes.
	** Put them at the beginning of the record.
	** (Note that the record just contains a pointer to the nested
	** record or array).
	**
	** Also initialize abradattype.db_prec to -1; IIARgtaGetAttr will stash
	** the type index of the complex attribute thereat when (and if)
	** the attribute is first referenced.
	*/
	dp = (char *)&((R_OBJ *)dp)->ro_objs[0];

	for (ap = bgn_ap; ap < end_ap; ap++)
	{
		if (ap->abradattype.db_datatype == DB_DMY_TYPE)
		{
			ap->abradattype.db_prec = -1;
			ap->abraoff = (i4)(dp - olddp);
			dp += sizeof(C_OBJ *);
			++num_complex_attrs;
		}
	}
	NumComplexAttrs[typ_num] = num_complex_attrs;

	/*
	** Second pass through the attributes of the object:
	** look for simple attributes.
	*/
	for (ap = bgn_ap; ap < end_ap; ap++)
	{
		i4	pad;
		DB_DT_ID dtype = ap->abradattype.db_datatype;

		if (dtype != DB_DMY_TYPE)
		{
			/* get the pad for this data item */
			if (afe_pad(Adf_cb, dp, dtype, &pad) != OK)
			{
				abproerr( ERx("calcOffsets"), CLASSFAIL,
					typ->abrtname, (char *)NULL );
				/* NOTREACHED */
			}
			dp += pad;

			/* calculate the offset and increment the magic ptr. */
			ap->abraoff = (i4)(dp - olddp);
			dp += ap->abradattype.db_length;
		}
	}

	/*
	** Now we have the size of the whole data area.
	** We round it up to a multiple of POOL_Q_HDR_SIZE.
	** If we somehow have a data area with 0 length
	** (perhaps a record with no attributes), make the length positive.
	*/
	dp = (char *)ME_ALIGN_MACRO((PTR)dp, POOL_Q_HDR_SIZE);
	typ->abrtsize = (i4)(dp - olddp);
	if (typ->abrtsize <= 0)
	{
		typ->abrtsize = POOL_Q_HDR_SIZE;
	}

	return OK;
}

/*
** Name:	[mf]e_req() -	Request main memory
**
** Description:
**	These routines are covers for [FM]Ereqlng (uninitialized storage).
**	Each issues the request and aborts if the request fails.
**
** Inputs:
**	tag	{TAGID}		the memory tag.
**	bytes	{u_i4}	the number of bytes.
**	routine	{char *}	a string identifying the requesting routine
**				(for a message, in case of an error).
**
** Returns:
**	{PTR}		a pointer to the acquired block.
**
** History:
**	11/07/91 (emerson)
**		Created for revamped array processing.
*/
static PTR
fe_req( tag, bytes, routine )
TAGID		tag;
u_i4	bytes;
char		*routine;
{
	PTR	datap;

	datap = FEreqlng(tag, bytes, (bool)FALSE, (STATUS *)NULL);
	if (datap == NULL)
	{
		abproerr(routine, OUTOFMEM, (char *)NULL);
		/* NOTREACHED */
	}
	return datap;
}

static PTR
me_req( tag, bytes, routine )
TAGID		tag;
u_i4	bytes;
char		*routine;
{
	PTR	datap;

	datap = MEreqmem(tag, bytes, (bool)FALSE, (STATUS *)NULL);
	if (datap == NULL)
	{
		abproerr(routine, OUTOFMEM, (char *)NULL);
		/* NOTREACHED */
	}
	return datap;
}

/*
** Name:	indexErr	- print out indexing error.
*/
static VOID
indexErr( index )
i4	index;
{
	char buf[16];
	iiarUsrErr(BADINDEX, 1, STprintf(buf, ERx("%d"), index));
}

#ifdef CLAS_TRACE
/*
** Name:	Tprint() -	print a debugging trace message.
**
** Description:
**	Currently a no-op (although you can breakpoint on XXXp and look at
**	the msg).  This routine should figure out an output file and use it.
**
** Inputs:
**	fmt	{char *} -  Pointer to the printf-style format for the message.
**			    The first substitution format must be %s.
**			    Remaining substitution formats, if any,
**			    must be %ld, %lo, or %lx.
**	a1	{char *} -  First substitution variable.
**	a2, ...	{i4} - Remaining substitution variables, if any.
**
** History:
**	11/07/91 (emerson)
**		Changed argument types from int to char* and i4;
**		changed invocations of this function accordingly.
**		(Before, types typically didn't match).
*/
Tprint( fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9 )
char *fmt;
char *a1;
i4 a2, a3, a4, a5, a6, a7, a8, a9;
{
	char buf[1024];

	STprintf(buf, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	XXXp(buf);
}
XXXp( buf )
char *buf;
{
	*buf = '\0';
}
#endif /* CLAS_TRACE */
