/*
** Copyright (c) 1986, 2004 Ingres Corporation
**
*/

/**
** Name: OPGLOBAL.H - Global constants and typedefs
**
** Description:
**      Global objects with dependencies from many header files
**
** History:
**      10-mar-86 (seputis)
**          initial creation
**      5-oct-88 (seputis)
**          update DB_QSMEMSIZE to reflect in memory sorts in QEF
**      2-oct-88 (seputis)
**          add DD_LDB_DESC temporarily so that OPF header files
**	    can remain compatible with titan path
**      12-jan-91 (seputis)
**          added OPN_DSORTBUF to limit size of DMF preallocated
**          sort files for single table scans.
**	21-jun-95 (wilri01)
**	    increased OPB_MAXBF to 1023
**	06-mar-96 (nanpr01)
**	    Removed DB_PAGESIZE. Added DB_MINPAGESIZE for initialization
**	    of ulm. Added NO_OF_POOL as define to add max no of
**	    different buffer pools.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	26-sep-00 (inkdo01)
**	    Reduced OPB_MAXBF to 256 (non-CNF support obviates need for 
**	    huge BF bit maps).
**      23-Oct-2002 (hanch04)
**          Make pagesize variables of type long (L)
**	3-dec-02 (my daughter's birthday)(inkdo01)
**	    Genericize range table size (part of range table expansion).
**	13-feb-03 (toumi01) bug 109609
**	    increase OPB_MAXBF to 1023 (Ingres 2.5 value), and create a
**	    separate OPB_MAXBF_CNF so that the threshold for doing the
**	    CNF transformations stays at the old OPB_MAXBF value of 255
**	30-dec-2004 (abbjo03)
**	    Remove VMS #ifdefs since DB_MAXVARS is now 128 which exceeds MAX_I1.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - moved the macro collections to enums to
**	    aid debugging.
**/

/*
**  Defines of other constants.
*/

/* this symbol defined only during linting - it is used to make visible
OPS_LINT         TRUE
** static procedures which lint can use for parameter checking
*/

#define                 OPF_VERSION      1
/* version of OPF currently running */

#define                 OPS_TXTSIZE      2
/* size of count field for text datatype - used until these references
** can be removed from OPF (FIXME)
*/

#define                 DB_MINPAGESIZE      2048L
/* FIXME - taken from bzarch.h -until placed inside some jupiter file
** number of bytes in a DMF disk block
*/

#define                 OPS_BSIZE       DB_MINPAGESIZE
/* - block size used for ULM memory management */

#define                 DB_HASHWIDTH     4
/* FIXME - width of a hash value which is appended to the end of the
** record reformatted to hash
*/

#define                 DB_BPTR          4
/* FIXME - taken from bzarch.h -until placed inside some jupiter file
** number of bytes in a pointer to a disk block
** - one such pointer is associated with each attribute valued entry
** in ISAM or BTREE
*/

#define                 DB_HFOVERHEAD    5
/* FIXME - taken from sjcost.c -
** disk i/o overheads for creating, opening, closing, and deleting hold files
*/

#define                 DB_COCRELATION   10
/* disk i/o's required for creating, opening and closing a temporary relation
*/

#define                 DB_HFBLOCKSIZE   (8*DB_MINPAGESIZE)
/* FIXME - taken from joinop.h TFBLOCKSIZE
** number of bytes in a temporary file block, used for hold files
*/

#define                 OPN_DSORTBUF    100000
/* if no enumeration is used, for a single table scan, then limit
** the number of bytes estimated for any top sort node since DMF does a
** preallocation based on this */

#define                 DB_SROVERHEAD    0
/* number of bytes overhead in a sort file record
** - currently no overhead, ... sorter will use a 4 byte overhead during the
** sort but when the data is placed into the hold file, this overhead is
** removed
*/

#define			DB_NO_OF_POOL	 6
/*
** There can be max of 6 buffer pools of size 2K, 4K, 8K, 16K, 32K and
** 64K respectively.
*/
#define			OPN_SCACHE       (2.0)
/* 2 pages are read into the page cache for any scan prior to accelerating to a group
** level scan, FIXME - read this number  from DMF */

#define			OPN_DCACHE       (30.0)
/* The local caching characteristics of the local DBMS are unknown but guess at 30 pages
** on average, for local the cache available is probed from DMF
*/

#define                 OPS_MILADJUST    10
/* factor by which "cost" estimate is adjusted to "cpu time" which is used
** to terminate the optimization by timeout
*/
#define                 OPS_EXSELECT    (0.01)
/* this is the default selectivity of a sargable clause which is an
** exact quality, when histograms are not available */
#define                 OPS_INEXSELECT  (0.10)
/* this is the default selectivity of a sargable clause which is an
** inexact quality (e.g. > < >= <=), when histograms are not available */

/*
**      Defines for limits of tables used within the optimizer
*/
#define                 OPV_MAXVAR       DB_MAXVARS
/*
** maximum number of primary and secondary indexes
** - used to determine range table size
*/
#define                 OPM_MAXFAGG       OPV_MAXVAR
/*
** maximum number of aggregates which can be evaluated in
** a subquery
*/

#define                 OPV_BITMAPVARS   ((OPV_MAXVAR+7)/8)
/* number of bytes used in a bit map on the range table */

#define                 OPZ_MAXATT       (DB_MAX_COLS + OPV_MAXVAR)
/* maximum number of joinop attributes which can be used in a query
** - used to determine the size of the joinop attributes array
** - in other words the maximum number of unique (range variable/ attribute )
** pairs used in the query
*/

#define                 OPZ_BITMAPATT     ((OPZ_MAXATT+7)/8)
/* number of chars used in a bit map on the joinop attributes array
*/
#define                 OPE_MAXEQCLS      OPZ_MAXATT
/* maximum number of joinop equivalence classes which can be defined in a query
*/

#define                 OPE_BITMAPEQCLS   ((OPE_MAXEQCLS+7)/8)
/* number of chars used in a bit map on the equivalence class array 
*/

#define                 OPB_MAXBF         1023
/* maximum number of boolean factors; i.e. number of conjuncts in the
** conjunctive normal form of the qualification of the query
*/

#define                 OPB_MAXBF_CNF     255
/* maximum number of boolean factors for deciding whether or not to do
** conjunctive normal form transformations; queries with a number of
** boolean factors between OPB_MAXBF_CNF and OPB_MAXBF are cases where
** e.g. a lengthy hand coded IN clause results in many boolean factors
** independent of any optimizer transformations
*/

#define                 OPB_BITMAPBF      ((OPB_MAXBF+7)/8)
/* number of chars used in a bit map on the boolean factor array */

#define                 OPO_BITMAP        100
/* number of elements in ordering array */

#define			OPL_MAXOUTER	  OPV_MAXVAR
/* number of outer joins which can be defined, since each outer join
** requires 2 range table entries, it would be reasonable to limit
** this to maxvar */
#define                 OPL_BITMAPOJ   ((OPL_MAXOUTER+7)/8)
/* number of bytes used in a bit map of join group IDs */

/*
**      constants used to define the maximum limits of the joinop trees
*/

#define                 OPN_MAXLEAF    	  OPV_MAXVAR
/* maximum number of leaves in a join tree */

#define                 OPN_MAXNODE       (OPN_MAXLEAF*2+1)
/* maximum number of nodes in a join tree */

#define                 OPN_MAXSTACK      OPN_MAXLEAF
/* maximum stack size used to keep track of join trees */

#define                 OPN_MAXDEGREE     2
/* max degree fanout of a tree, currently heuristics can only handle binary 
** trees 
*/


/*}
** Name: OPX_FACILITY - error code from another facility
**
** Description:
**	Used whenever an error code from another facility is be retrieved.
**      The type should be equal to the type of "DB_ERROR.err_code"
**
** History:
**      3-jul-86 (seputis)
**          initial creation
*/
typedef i4 OPX_FACILITY;

/*}
** Name: OPS_DTLENGTH - abstract datatype length
**
** Description:
**	This is a typedef for the DB_DATA_VALUE.db_length type ... so that
**      these objects can be appropriately typed (probably should be a
**      DB_DTLENGTH)
**
** History:
**      31-may-86 (seputis)
**          initial creation
*/
typedef i4 OPS_DTLENGTH;

/*}
** Name: OPV_IGVARS - global range table element
**
** Description:
**      This is an index into the global range table OPV_GRT.  The index
**      is used to represent the global range table element in various
**      other structures.  The initial value in the var nodes of the query
**      tree will contain elements of this type, since the parser's range
**      table is used to built the global range table.
** Historical Note:
**      i1 were preferred since many arrays of this typedef are created, but
**      signed arithmetic is required so use i2 for other operating systems
**      or until define is made for signed i1 arithmetic being available
**
** History:
**      20-may-86 (seputis)
*/
typedef i2 OPV_IGVARS;

#define                 OPV_NOGVAR      (-1)
/* indicates that the global var number has not been assigned */

/*}
** Name: OPV_GBMVARS - bit map for the global range table
**
** Description:
**	This structure will contain a bit map of selected variables from
**      the global range table.  Each bit position represents an index
**      into the global range table.
**
** History:
**      20-may-86 (seputis)
**          initial creation
**	3-dec-02 (inkdo01)
**	    Range table expansion changes to larger bit map.
*/
typedef PST_J_MASK OPV_GBMVARS;
/* typedef struct _OPV_GBMVARS
{
    char opv_bm[OPV_BITMAPVARS];
}   OPV_GBMVARS; */

/*}
** Name: OPV_IINDEX - index into the array of pointers to index tuples
**
** Description:
**      This type is used whenever an index into the array of pointers to
**      index tuples (found in RDF) is required.
**
** History:
**      19-may-86 (seputis)
*/
typedef i2 OPV_IINDEX;
#define                 OPV_NOINDEX     ((OPV_IINDEX)(-1))
/* used if a variable of this type is not defined, or does not exist */

/*}
** Name: OPV_IVARS - index into the joinop range variable table
**
** Description:
**	This structure represents a "joinop range variable" which is
**      an index into the joinop range variable table.
** Historical Note:
**      i1 were preferred since many arrays of this typedef are created, but
**      signed arithmetic is required so use i2 for other operating systems
**      or until define is made for signed i1 arithmetic being available
**
** History:
**      19-may-86 (seputis)
**	    initial creation
**	21-may-93 (ed)
**	    removed obsolete define's
*/
typedef i2 OPV_IVARS;

#define                 OPV_NOVAR       (-1)
/* used when a joinop range variable is required but not available */


/*}
** Name: OPV_BMVARS - bitmap for joinop range table
**
** Description:
**	Defines a bit map structure for use with joinop range variables.  The
**      relative offset of each bit in this structure represents a joinop
**      range variable.
**
** History:
**      19-may-86 (seputis)
**          initial creation
*/
typedef struct _OPV_BMVARS
{
    char opv_bm[OPV_BITMAPVARS];
}   OPV_BMVARS;

/*}
** Name: OPM_IAGGS - unique ID for aggregate
**
** Description:
**	This represents a unique ID for a set of aggregates which can be
**	evaluated together
**
** History:
**      19-may-86 (seputis)
*/
typedef i2 OPM_IAGGS;
#define                 OPM_NOAGG       (-1)
/* used when no aggregate is available */


/*}
** Name: OPV_ABMVARS - array of variable bitmaps
**
** Description:
**      an array of bitmaps representing local range variables
**
** History:
**      18-jul-90 (seputis)
**          initial creation
*/
typedef struct _OPV_ABMVARS
{
    OPV_BMVARS	opv_abmvars[OPN_MAXLEAF];
}   OPV_ABMVARS;

/*}
** Name: OPM_BMAGGS - bitmap for joinop range table
**
** Description:
**	Defines a bit map structure for use with aggregates.  Since
**	each aggregate requires at least one table or virtual table
**	it should not be larger than the number of range variables
**	defined.  If aggregates can be evaluated together then
**	they would all be represented by the same aggregate number
**
** History:
**      19-nov-91 (seputis)
**          initial creation
*/
typedef struct _OPM_BMAGGS
{
    char opv_bm[OPV_BITMAPVARS];
}   OPM_BMAGGS;

/*}
** Name: OPV_BTYPE - boolean type
**
** Description:
**      Used to provide common element type for OPV_MVARS
**
** History:
**      3-dec-87 (seputis)
**          initial creation
*/
typedef char OPV_BTYPE;

/*}
** Name: OPV_MVARS - map of the joinop range variables
**
** Description:
**	Exactly the same type of information as in OPV_BMVARS except that
**      it uses a boolean element for speed.
**
** History:
**      20-may-86 (seputis)
*/
typedef struct _OPV_MVARS
{
    OPV_BTYPE opv_bool[OPV_MAXVAR];
}   OPV_MVARS;

/*}
** Name: OPZ_IATTS - joinop attribute number
**
** Description:
**	Index into the boolean factor array.  This type represents a
**      boolean factor number, which is used to uniquely identify the 
**	boolean factor.
**
** History:
**      21-may-86 (seputis)
**          initial creation
*/
typedef i2 OPZ_IATTS;

#define                 OPZ_NOATTR      (-2)
/* - indicates that joinop attribute number could not be found
** - care should be taken when defining "negative joinop attribute" to
** have special meanings
** - this value should never be stored as a joinop attribute number but is
** returned in from opz_findatt when the joinop attribute could not be
** found.
*/


/*}
** Name: OPZ_IFATTS - function attribute number
**
** Description:
**	This is an index into the function attribute array OPZ_FATTS.  It is
**      used to represent the function attribute in various other structures.
** Historical Note:
**      i1 were preferred since many arrays of this typedef are created, but
**      signed arithmetic is required so use i2 for other operating systems
**      or until define is made for signed i1 arithmetic being available
**
** History:
**      20-may-86 (seputis)
*/
typedef i2 OPZ_IFATTS;

#define                 OPZ_NOFUNCATT   ((OPZ_IFATTS) -1)
/* means function attribute is not associated with this joinop attribute element
*/

/*}
** Name: OPZ_BMATTS - bitmap for joinop attributes
**
** Description:
**	Structure is used to define bitmap representing selected equivalence
**      class elements from the subquery OPE_EQCLIST structure.
**
** History:
**      19-may-86 (seputis)
**          initial creation
*/
typedef struct _OPZ_BMATTS
{
    char opz_bm[OPZ_BITMAPATT];
}   OPZ_BMATTS;

/*}
** Name: OPE_IEQCLS - equivalence class number
**
** Description:
**	Index into the equivalence class array.  This type represents an
**      equivalence class number, which is used to uniquely identify the 
**	equivalence class
**
** History:
**      19-may-86 (seputis)
*/
typedef i2 OPE_IEQCLS;

#define                 OPE_NOEQCLS       ((OPE_IEQCLS) -1)
/* represents an equivalence class which has not been assigned */
#define                 OPB_BFMULTI       ((OPE_IEQCLS) -2)
/* used to indicate that more than one equivalence class is referenced by this
** boolean factor
*/
#define                 OPB_BFNONE        ((OPE_IEQCLS) -3)
/* used to indicate that no equivalence classes exist in the boolean factor
** i.e. only constants exist
*/

/*}
** Name: OPE_BMEQCLS - bitmap for joinop equivalence classes
**
** Description:
**	Structure is used to define bitmap representing selected equivalence
**      class elements from the subquery OPE_EQCLIST structure.
**
** History:
**      23-may-86 (seputis)
**          initial creation
*/
typedef struct _OPE_BMEQCLS
{
    char ope_bmeqcls[OPE_BITMAPEQCLS];
}   OPE_BMEQCLS;

/*}
** Name: OPO_ISORT - defines an index into the the multi-attribute descriptors
**
** Description:
**      Used to define an index into the multi-attribute desciptor array.  This
**      array can be logical considered an extension of the equivalence class
**      array.
**
** History:
**      8-feb-87 (seputis)
**          initial creation
*/
typedef OPE_IEQCLS OPO_ISORT;

/*}
** Name: OPL_IOUTER - index into outer join table
**
** Description:
**      represents an offset into the outer join table
**
** History:
**      9-sep-89 (seputis)
**	    initial creation for outer join support
*/
typedef i2 OPL_IOUTER;

#define                 OPL_NOOUTER      ((OPL_IOUTER)-1)
/* used to indicate that an outer join has not been specified */
#define                 OPL_NOINIT       ((OPL_IOUTER)-2)
/* used to indicate that this variable has not been inited */

/*}
** Name: OPL_BMOJ - bitmap for join group IDs
**
** Description:
**	Structure is used to define bitmap representing selected join IDs
**
** History:
**      23-sep-89 (seputis)
**          initial creation for outer join support
*/
typedef struct _OPE_BMOJ
{
    char ope_bmoj[OPL_BITMAPOJ];
}   OPL_BMOJ;

/*}
** Name: OPB_IBF - boolean factor element
**
** Description:
**	Index into the boolean factor array.  This type represents a
**      boolean factor number, which is used to uniquely identify the 
**	boolean factor.
** Historical Note:
**      i1 were preferred since many arrays of this typedef are created, but
**      signed arithmetic is required so use i2 for other operating systems
**      or until define is made for signed i1 arithmetic being available
**
** History:
**      19-may-86 (seputis)
**          initial creation
**	21-jun-95 (wilri01)
**	    OPB_IBF i2 in VMS
*/
typedef i2 OPB_IBF;

#define                 OPB_NOBF          ((OPB_IBF) -1)
/* boolean factor has not been defined */

/*}
** Name: OPB_BMBF - bit map of the boolean factors selected
**
** Description:
**	Structure is used to define bitmap representing selected boolean
**      factor elements from the subquery OPB_BOOLEANFACT structure.
**
** History:
**      21-may-86 (seputis)
**          initial creation
*/
typedef struct _OPB_BMBF
{
    char opb_bmbf[OPB_BITMAPBF];
}   OPB_BMBF;

/*}
** Name: OPN_CTDSC - component of the tree descriptor array
**
** Description:
**	Element of the OPN_TDSC array type, which contains an encoded
**      node representation as described in OPNTREE.C
**      Note: the tree descriptions use ST routines which require null
**      terminated "strings" so do not change this type until ST routines
**      have been removed from the optimizer.
**
** History:
**      19-may-86 (seputis)
**          initial creation
*/
typedef u_i2        OPN_CTDSC;

/*}
** Name: OPN_ITDSC - index into the tree descriptor array
**
** Description:
**	This type is used for variables which are indexes into the OPN_TDSC
**      array structure.  Used to mark positions in the array which will
**      contain the structurally unique tree descriptor.
**
** History:
**      19-may-86 (seputis)
**          initial creation
*/
typedef u_i2        OPN_ITDSC;

/*}
** Name: OPN_TDSC - description of structurally unique tree
**
** Description:
**	This is an array of integers which describes a structurally unique
**	tree.  See OPN_SUNIQUE for the description of the encoding format
**      for the tree.  "+1" element is added to the size of the array
**      to allow for a NULL terminator which is used is some cases (so that
**      ST routines can be used)
**
** History:
**      19-may-86 (seputis)
**          initial creation
*/
typedef OPN_CTDSC   OPN_TDSC[OPN_MAXNODE+1];


/*}
** Name: OPN_LEAVES - a "number of leaves"
**
** Description:
**    	This type is used to represent a "leaf" index or count.  It has the
**      same subrange as OPV_IVARS, but OPV_IVARS is a special case which
**      was important enough to separate into its own type.  Thus, the maximum
**      number of leaves is equal to the maximum number of range variables
**      than can be defined.
**
** History:
**      19-may-86 (seputis)
**          initial creation
*/
typedef i4  OPN_LEAVES;

/*}
** Name: OPN_PARTSZ - array of "number of relations" in a partition
**
** Description:
**	Array which contains the number of relations in the respective partition
**      of a set of relations.
**
** History:
**      19-may-86 (seputis)
**          initial creation
*/
typedef OPN_LEAVES OPN_PARTSZ[OPN_MAXDEGREE];

/*}
** Name: OPN_STLEAVES - represents the subtree leaves
**
** Description:
**	This structure is used to contain the leaves of a subtree being
**      analyzed.  The elements of this array are range variable numbers.
**
** History:
**      20-may-86 (seputis)
**          initial creation
*/
typedef OPV_IVARS OPN_STLEAVES[OPN_MAXLEAF];

/*}
** Name: OPN_SSTLEAVES - define structure to reference OPN_STLEAVES
**
** Description:
**      Due to unix lint complaints, always work with a pointer to
**	a structure rather than an array.
**
** History:
**      18-may-90 (seputis)
**          initial creation for b21582
*/
typedef struct _OPN_SSTLEAVES
{
    OPN_STLEAVES	stleaves;
}   OPN_SSTLEAVES;

/*}
** Name: OPN_ISTLEAVES - represents an index into the OPN_STLEAVES array
**
** Description:
**	This type is used whenever an index into the OPN_STLEAVES type is
**      required.
**
** History:
**      20-may-86 (seputis)
**          initial creation
*/
typedef OPN_LEAVES OPN_ISTLEAVES;

/*}
** Name: OPN_PERCENT - percentage indicator
**
** Description:
**	This type is used as a percentage indicator for selectivity, where
**      0.0 indicates 0 percent and 1.0 indicates 100 percent.
**
** History:
**      22-may-86 (seputis)
**          initial creation
*/
typedef f4 OPN_PERCENT;
#define                 OPN_100PC       ((OPN_PERCENT) 1.0)
/* 100 percent selectivity */
#define                 OPN_50PC        ((OPN_PERCENT) 0.5)
/* 50 percent selectivity */
#ifdef    IBM370
#define                 OPN_MINPC       ((OPN_PERCENT) 1.0E-40)
/* IBM FMIN is the absolute minimum value and the floating point hardware
** underflows unless a larger value is used here
*/
#else
#define                 OPN_MINPC       ((OPN_PERCENT) FMIN)
#endif
/* small (non-zero) selectivity */
#define                 OPN_0PC         ((OPN_PERCENT) 0.0)
/* 0 percent selectivity */

#define                 OPH_D1CELL      OPN_0PC
#define                 OPH_D2CELL      OPN_100PC
/* Defaults for creation of uniform distribution for histogram where
** the first cell boundary is at the lower range of the domain and the second 
** cell boundary is at the upper range.  Thus, all elements of the histogram
** fall into the second cell so percentage is 1.0 = 100%
*/

#define                 OPN_DUNKNOWN    OPN_50PC
/* Default selectivity for clauses which are not of the form
**      (attribute op const)   where op is one of >=, =, <=, >, <
*/

/*}
** Name: OPN_DPERCENT - percentage indicator
**
** Description:
**	This type is used as a percentage indicator for selectivity, where
**      0.0 indicates 0 percent and 1.0 indicates 100 percent.  This type
**      is double precision and is used when computation are performed
**      which may result in loss of significant digits
**
** History:
**      22-may-86 (seputis)
**          initial creation
*/
typedef f4 OPN_DPERCENT;
#define                 OPN_D100PC       ((OPN_DPERCENT) 1.0)
/* 100 percent selectivity */
#define                 OPN_D50PC        ((OPN_DPERCENT) 0.5)
/* 50 percent selectivity */
#define                 OPN_D0PC         ((OPN_DPERCENT) 0.0)
/* 0 percent selectivity */


/*}
** Name: OPO_BLOCKS - represents a number of disk blocks
**
** Description:
**      This type is used whenever a number of disk blocks needs to be
**      represented e.g. estimated number of disk blocks read/written
**
** History:
**      28-may-86 (seputis)
**          initial creation
*/
typedef f4 OPO_BLOCKS;

#define                 OPO_NOBLOCKS     ((OPO_BLOCKS) 0.0)
/* represents a count of 0 disk blocks */
#define                 OPO_1BLOCK       ((OPO_BLOCKS) 1.0)
/* represents a count of 1 disk blocks */

/*}
** Name: OPO_DBLOCKS - represents a number of disk blocks
**
** Description:
**      This type is used whenever a number of disk blocks needs to be
**      represented e.g. estimated number of disk blocks read/written
**      This is the double precision version used for temporary working
**      storage variables
**
** History:
**      28-may-86 (seputis)
**          initial creation
*/
typedef f8 OPO_DBLOCKS;



/*}
** Name: OPO_DTUPLES - count of number of tuples
**
** Description:
**      This type is used whenever a number of tuples needs to be
**      represented e.g. estimated number of tuples read/written
**      This is the double precision version used for temporary working
**      storage variables
**
** History:
**      28-may-86 (seputis)
**          initial creation
*/
typedef f8 OPO_DTUPLES;

/*}
** Name: OPO_TUPLES - count of number of tuples
**
** Description:
**      This type is used whenever a number of tuples needs to be
**      represented e.g. estimated number of tuples read/written
**
** History:
**      28-may-86 (seputis)
**          initial creation
*/
typedef f4 OPO_TUPLES;
#define                 OPO_MINTUPLES    (0.00001)
/* represents minimum non-zero value for this type - effectively 0 */

/*}
** Name: OPO_CPU - measure of CPU usage
**
** Description:
**	This type is used to represent a measure of CPU activity during
**      cost evaluation.  It is approximetely equal to the number of tuple
**      compares and moves required.
**
** History:
**      28-may-86 (seputis)
**          initial creation
*/
typedef f4 OPO_CPU;

/*}
** Name: OPO_COST - measure of the cost of producing a particular tree
**
** Description:
**	This type is used to represent a measure of cost of a plan
**      as a function of CPU and DISK i/o activity.
**
** History:
**      28-may-86 (seputis)
**          initial creation
**      12-nov-90 (seputis)
**          fixed bull bug, floating point comparison problem
*/
typedef f4 OPO_COST;
#define                 OPO_INFINITE	((OPO_COST)FMAX)
/* This value is used to represent the infinite cost of an illegal
** site movement in the distributed optimizer */
#define                 OPO_CINFINITE   ((OPO_COST)FMAX/2.0)
/* This value is used to compare 2 "infinite" values since equality
** comparisons does not always work due to round-off*/

/*}
** Name: OPO_DCOST - measure of the cost of producing a particular tree
**
** Description:
**	This type is used to represent a measure of cost of a plan
**      as a function of CPU and DISK i/o activity.  It is the double
**      precision version of OPO_COST.
**
** History:
**      28-may-86 (seputis)
**          initial creation
*/
typedef f8 OPO_DCOST;
#define                 OPO_LARGEST     FMAX
/* Use largest allowable float number to represent largest cost.  This constant
** is used as an initial value when comparing new CO tree costs to determine
** if a better plan has been found
*/

/*}
** Name: OPN_IOMTYPE - type of interesting orderings requested
**
** Description:
**      This structure is used to contain the type of interesting orderings
**      upon which to reformat the intermediate relations.  There is a
**      heirarchy in which HASH > ISAM > SORT, and if any type of reformatting
**      is chosen then the "lower" types are also done.  This feature is not
**      supported by the query compilation phase, so that only reformats to
**      SORT will be done.  OPC will return error if anything other than a
**      sort is done.
**
** History:
**     28-may-86 (seputis)
**          initial creation
*/
typedef enum _OPN_IOMTYPE {
	OPN_IOBTREE =	3, /* not implemented yet */
	OPN_IOHASH =	2, /* implies HASH, ISAM and SORT reformats are done */
	OPN_IOISAM =	1, /* implies ISAM, and SORT reformats are done */
	OPN_IOSORT =	0, /* implies SORT reformats are done */
} OPN_IOMTYPE;

/*}
** Name: OPS_DUPLICATES - handling of duplicates
**
** Description:
**	This typedef describes the handling of duplicates for a particular
**      subquery.  There are currently 3 states
**	    1) do not care
**          2) keep duplicates
**          3) remove duplicates
**
**      Since the "do not care" case is not deterministic, it may be removed
**      in the future but is added here for compatibility with previous
**      versions of Ingres.
**
** History:
**      23-jul-86 (seputis)
**          initial creation
*/
typedef enum _OPS_DUPLICATES {
	OPS_DUNDEFINED =	1, /* do not care about duplicates in the result */
	OPS_DREMOVE =		2, /* remove duplicates - add a SORT node to the
				   ** top of the query plan */
	OPS_DKEEP =		3, /* keep duplicates - add tids to query so
				   ** duplicates are not discarded */
} OPS_DUPLICATES;

/*}
** Name: OPT_TRACE - tracing information
**
** Description:
**      This structure contains tracing information used for printing
**      QEP trees, and CO trees.  The tree formatting routines are
**      traversed recursively, and information such as range variables etc.
**      are not immediately available.  The structure will be in the global
**      state variable which can be reached via the session control block
**      which can be obtained from the SCF.
**
** History:
**     18-jul-86 (seputis)
**          initial creation
**      18-apr-91 (seputis)
**          support for trace point op188
*/
typedef struct _OPT_TRACE
{
    struct _OPS_SUBQUERY *opt_subquery;      /* current subquery being traced */
    char		opt_trformat[200];  /* size of TR format buffer for
					    ** tracing information */
    i4			opt_nodeid;	    /* used for CO tree printing
                                            ** routines to label nodes */
    struct _OPO_CO	*opt_conode;        /* used to print partial query plans */
    PTR			justincase;	    /* dummy for any overruns */
}   OPT_TRACE;

/*}
** Name: OPS_WIDTH - width of tuple
**
** Description:
**	Used whenever the width of a tuple in bytes is required
**
** History:
**     23-oct-86 (seputis)
**          initial creation
**     06-mar-96 (nanpr01)
**          Width of the tuple should be i4 what is returned
**	    from DMT_TBL_ENTRY.
*/
typedef i4 OPS_WIDTH;

/*}
** Name: OPS_SQTYPE - type of subquery
**
** Description:
**	This typedef is used to define the type of subquery.
**
** History:
**      28-jul-86 (seputis)
**          initial creation
**	19-jan-01 (inkdo01)
**	    Added OPS_HFAGG for hash aggregation
**	31-Aug-2006 (kschendel)
**	    Comment updates only.
*/
typedef enum _OPS_SQTYPE {
	OPS_MAIN =			0,
/* main query */
	OPS_SAGG = 			1,
/* simple aggregate - resdom nodes will point to AOP nodes which are
** the aggregrates; where each AOP node has the aggregate expression
** as the left child, and NULL as the right child
** - each AOP node will contain a "constant parameter" number which will
** represent the result of the simple aggregate
** A simple aggregate can be computed by itself, independently of other
** stuff going on in the query, and results in a single row of scalar
** aggregate-result values.
*/
	OPS_FAGG =			2,
/* function aggregate - 
** - there is an implicit resdom node with result number of 1 for the count 
** field
** - each resdom node which has an AOP node as a right child, represents an
** aggregate to be evaluated; by convention all these nodes will be above
** (i.e. be parents) of any bylist node, and are immediately to the left of
** the root node
** - the remaining resdom nodes are the by list expressions for the aggregate
** A function aggregate loads a temp table with (bylist,agg result) tuples.
** If preceded by a projection, any bylist values in the projection that
** aren't returned by the OPS_FAGG subquery are sent to the result with
** zeros for the agg-result columns.
*/
	OPS_PROJECTION =		3,
/* projection of by list for function aggregate
** - no AOP nodes will be found in the resdom list
** - by convention, OPC query compilation, will assume that the projection
** subquery will immediately preceed the associated aggregate function.
** A projection produces a (sorted) list of by-values for use with a
** following FAGG subquery.
*/
	OPS_SELECT =			4,
/* sub-select defined in the main query
** - this subselect has been detached from the main query and should
** only be executed in conjunction with an SEJOIN node
*/
	OPS_UNION =			5,
/* sub-select is defined in a union of multiple subselects connected
** by the ops_union field of the subqury structure
** - the first subselect of this linked list is also part of the union
** but defined as a OPS_MAIN, OPS_SELECT, OPS_VIEW
*/
	OPS_VIEW =			6,
/* sub-select defines a view which will be treated like a "ORIG" co node 
** - normally views are querymoded into the subquery, but if one
** "union view" exists then this is not done because duplicate handling
** becomes difficult
*/
	OPS_RFAGG =			7,
/* This is a function aggregate which is not placed into a temporary relation
** but instead tuples are retrieved and used directly from the query plan.  
** This will be used in all cases except if a temporary would help avoid
** calculating an aggregate function twice.  It will always be used in cases
** where the function aggregate is corelated.
** This subquery type assumes that the query plan produces tuples in
** sorted by-list order, and hence is in general less desirable than
** OPS_HFAGG (unless sorting naturally occurs by virtue of btrees or
** FSM-joins in the query plan).
*/
	OPS_RSAGG =			8,
/* This is used for a simple aggregate which is corelated.
** An RSAGG computes a single scalar row like SAGG, but cannot be
** evaluated independently.  So it needs some of the machinery that
** function aggregates depend on.
*/
	OPS_HFAGG =			9,
/* This is a function aggregate which is to be evaluated using hash 
** aggregation.  It is otherwise identical to OPS_RFAGG.
** Since hash aggregation does not require sorted input, HFAGG is
** preferred over OPS_RFAGG.  However there is not as yet any
** hash analog to OPS_FAGG that produces a temp table.
*/
} OPS_SQTYPE;


/*}
** Name: OPS_SETTYPE - type of subquery set operator 
**
** Description:
**	This typedef is used to define the type of set operator subqueries.
**
** History:
**      22-sep-00 (inkdo01)
**          initial creation
*/
typedef i1 OPS_SETTYPE;
#define                 OPS_SETUNION    0
/* Union operator (also the default). */
#define			OPS_SETEXCEPT	1
/* Except (difference) operator. */
#define			OPS_SETINTERSECT  2
/* Intersect operator. */

/*}
** Name: OPT_NAME - buffer used for name in tree formatting routines
**
** Description:
**	This structure is used to buffer a name to be printed in the
**      tree formatting routines.
**
** History:
**      18-jul-86 (seputis)
**          initial creation
**      26-dec-90 (seputis)
**          use define name for size of buffer
**	01-mar-93 (barbara)
**	    Increased size to hold delimited unnormalized names.
*/
typedef struct _OPT_NAME
{
    char        buf[DB_MAXNAME*2+3];
} OPT_NAME;

/*}
** Name: OPS_PNUM - represents a parameter number
**
** Description:
**      This define is used whenever a query tree parameter number is being
**	defined.
**
** History:
**      24-feb-89 (seputis)
**          written
*/
typedef i4  OPS_PNUM;

/*}
** Name: OPL_OJTYPE - outer join type
**
** Description:
**      This describes the various types of joins
**
** History:
**      22-sep-89 (seputis)
**          initial creation for outer join support
*/
typedef enum _OPL_OJTYPE {
	OPL_UNKNOWN =    0,
/* used for nodes which do not have any outer join operations */
	OPL_RIGHTJOIN =  1,
	OPL_LEFTJOIN =   2,
	OPL_FULLJOIN =   3,
	OPL_INNERJOIN =  4,
	OPL_FOLDEDJOIN = 5,
/* a folded join occurs in a strength reduction when a right,left,full join
** is reduced to an inner join due to the attributes which are refered to
** outside the from clause */
} OPL_OJTYPE;

/*}
** Name: OPA_VID - ID used to identify implicitly reference variables
**
** Description:
**      This ID is used to uniquely identify all the global range variables
**      which were implicitly referenced in a view.
**
** History:
**      13-oct-89 (seputis)
**          initial creation, fix for b8160
*/
typedef OPV_IGVARS OPA_VID;
#define                 OPA_NOVID	      ((OPA_VID)-1)
/* indicates that no view ID has been assigned */

/*}
**  Name: OPL_OJSPEQC - datatype for the special OJ eqc.
**
**  Description:
**	Defines below are added in connection with the special
**	equivalence class for outer joins. The values assummed
**	by the attribute (it is a function attribute) are 0 and 1.
**	The value of 0 indicates that no match occurred, 1
**	indicates a match.
**
**  22-feb-90 (stec)
**	created for outer join support
*/
typedef	i1  OPL_OJSPEQC;
# define    OPL_0_OJSPEQC	    0
# define    OPL_1_OJSPEQC	    1

/*}
** Name: OPO_JTYPE - join type
**
** Description:
**      Used to describe the join type of a CO node
**
** History:
**      13-mar-90 (seputis)
**          initial creation
**	26-feb-99 (inkdo01)
**	    Added OPO_SJHASH for hash joins.
*/
typedef enum _OPO_JTYPE {
	OPO_Unused =	0,
	OPO_SJFSM =	1, /* Full sort merge */
	OPO_SJPSM =	2, /* Partial sort merge */
	OPO_SEJOIN =	3, /* Subselect join */
	OPO_SJTID =	4, /* Tid join */
	OPO_SJKEY =	5, /* Key join */
	OPO_SJHASH =	6, /* Hash join */
	OPO_SJCARTPROD = 7, /* cartesean product */
	OPO_SJNOJOIN =	8, /* not a join node, either ORIG or PR */
}	OPO_JTYPE;

/*}
** Name: OPO_ACOST - an array of cost information
**
** Description:
**      This structure is used to define an array of cost information.
**
** History:
**      17-oct-91 (seputis)
**          initial creation for bug 33551
*/
typedef struct _OPO_ACOST
{
    OPO_COST        opo_costs[1];       /* first element of an array of costs */
}   OPO_ACOST;

/*}
** Name: OPO_COMAPS - various maps used in a QEP
**
** Description:
**      This is a pointer to a structure which contains info associated
**      with a QEP node which needs to be maintained during out of memory
**      conditions during enumeration.  Since CO lists can become long, a
**	ptr to a single structure is used in a CO node rather than numerous
**	pointers to structures.
**
** History:
**      8-feb-92 (seputis)
**          initial creation
**	15-feb-93 (ed)
**	    if def aggregate flattening structure not needed yet
**	2-Jun-2009 (kschendel) b122118
**	    ojfilter removal, clean out opo_filter.
*/
typedef struct _OPO_COMAPS
{
    OPE_BMEQCLS     opo_eqcmap;         /* map field indicating which
                                        ** equivalence classes should be
                                        ** returned, a slight exception is
                                        ** the DB_ORIG node in which the
                                        ** equivalence classes are the same
                                        ** as the project-restrict
                                        ** instead of the "total set"
                                        ** of attributes referenced from the
                                        ** relation, i.e. this is not = opv_eqcmp
                                        */
#ifdef OPM_FLATAGGON
    OPM_BMAGGS      opo_aggmap;         /* map of aggregates which will be evaluated
					** at this CO node */
#endif
}   OPO_COMAPS;
