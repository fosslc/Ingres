/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPHISTO.H - Structures used for histograms
**
** Description:
**      This file contains the structures used to manipulate histogram
**      information.
**
** History: 
**      10-mar-86 (seputis)
**          initial creation
**      06-nov-89 (fred)
**          Added constants in support of default histograms.  These are used
**	    when a datatype is encountered which has no histograms.
**	09-nov-92 (rganski)
**	    Added new fields oph_charnunique and oph_chardensity to
**	    OPH_INTERVAL for new character histogram statistics.
**	24-may-93 (rganski)
**	    Removed define of OPH_MAXHIST, which has been replaced by
**	    DB_MAX_HIST_LENGTH in dbms.h.
**	20-sep-93 (rganski)
**	    Added new field oph_version to OPH_INTERVAL.
**	7-jan-96 (inkdo01)
**	    Added per-cell array of repetition factors to OPH_HISTOGRAM.
[@history_line@]...
**/

/*
**	Default Histogram Support
**
**  These #define's provide support for default histograms, a VERY rudimentary
**  attempt to provide support for datatypes which lack histogram support.
**  This is provided in conjunction with large object support.
**
**  For such datatypes, the histogram datatype will be DB_CHA_TYPE, with a
**  length of 8, precision of 0.  All data elements will map to the string
**  "nohistos".
**
**  This functionality is placed in OPF so that it can be easily excised when
**  OPF provides support for the lack of histograms in a datatype.
*/
#define                 OPH_NH_TYPE     DB_CHA_TYPE
#define			OPH_NH_LENGTH	8   /* length of "nohistos" */
#define			OPH_NH_PREC	0
#define			OPH_NH_HELEM	"nohistos"
#define			OPH_NH_DHMIN	"aaaaaaaa"
#define			OPH_NH_DHMAX	"zzzzzzzz"
#define			OPH_NH_HMIN	"\000\000\000\000\000\000\000\000"
#define			OPH_NH_HMAX	"\377\377\377\377\377\377\377\377"

/*}
** Name: OPH_COUNTS - array of cell counts for a histogram
**
** Description:
**	This structure is used to represent an array of cell counts for
**      histograms.  There are two types of cell counts which can be used
**      TYPE 1 - A count which is a percentage of the number of tuples in the
**      relation (which contains an attribute whose histogram value falls
**      in this bucket)
**      TYPE 2 - A count which is a percentage of a "type 1" histogram cell
**
**      This first element of this array is 0.0 and is always ignored in
**      histogram processing
**
** History:
**      24-may-86 (seputis)
**          initial creation for bug 37172
[@history_line@]...
*/
typedef OPN_PERCENT *OPH_COUNTS;
#define                 OPH_EXACTKEY    (OPN_PERCENT)(-1.0)
/* used to represent an exact lookup value temporarily when building a
** modified histogram */

/*}
** Name: OPH_BOOL - used to track exact cells in inexact histograms
**
** Description:
**      Bool array that identifies exact histogram cells.
**
** History:
**      31-oct-05 (inkdo01)
**	    Written for join estimate improvements.
[@history_template@]...
*/
typedef	bool	    OPH_BTYPE;

typedef struct _OPH_BOOL
{
    OPH_BTYPE	oph_bool[1];
} OPH_BOOL;

/*}
** Name: OPH_COERCE - used in free space management of OPH_COUNTS structures
**
** Description:
**      Coercion structure use to manage a free list of cell count structures.
**
** History:
**      6-aug-86 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPH_COERCE
{
    struct _OPH_COERCE *oph_next;       /* ptr to next free element */
    i4              oph_size;           /* size of current element */
}   OPH_COERCE;

/*}
** Name: OPH_COMP - contains composite histogram-specific info
**
** Description:
**      Used in histogram to contain info specific to composite histograms.
**
** History:
**      26-sep-00 (inkdo01)
**          Written.
[@history_template@]...
*/
typedef struct _OPH_COMP
{
    struct _OPH_INTERVAL  *oph_interval;	/* ptr to histo data */
    OPZ_IATTS	   *oph_attrvec;	/* ptr to vector of attrs in histo. */
    struct _OPB_BFVALLIST  *oph_vallistp;  /* ptr to concatenated keys from BFs 
					** covered by histogram */
    OPE_BMEQCLS	   *oph_eqclist;	/* bit map of eqclasses of histo's attrs */
    OPB_BMBF	   oph_bfacts;		/* bit map of BFs covered by histogram */
    OPB_IBF	   oph_bfcount;		/* count of BFs covered by histogram */
}   OPH_COMP;

/*}
** Name: OPH_CELLS - array of cell boundaries
**
** Description:
**	This structure is used to contain a variable number of cell boundaries.
**      Cell boundaries are histogram values which define the "histogram bucket"
**      i.e. Each boundary is defined such that the histogram cell i will 
**      represent values in the range  OPH_CELL[i-1] < value <= OPH_CELL[i] .
**      The number of bytes assigned to a OPH_CELL object equal to the length
**      of the histogram elements associated with this array.
**
**      There is a close association between this array of cell boundaries
**      and the array of cell counts.  Cell_count[i] will be associated with
**      the values in Cell_boundary[i-1]< x <= Cell_boundary[i].  The numcells
**      refers to the number of cell boundary elements (not the number of cell
**      count elements).  Thus, in the case of a default uniform int histogram;
**      the number of cells = 2; Cell_count[0] = 0.0 (as is the case in general
**      and is ignored in histogram processing) and Cell_count[1] = 1.0;
**      Cell_boundary[0]=0 and Cell_boundary[1]=1000.  This histogram means the
**      integers are uniformly distributed between 0 and 1000.
**
** History:
**      24-may-86 (seputis)
**          initial creation
**	11-jul-91 (seputis)
**	    change from PTR to char * for PC group
[@history_line@]...
*/
typedef char	*OPH_CELLS;

/*}
** Name: OPH_DOMAIN - represents a count of domain values
**
** Description:
**	This type is used to represent a count of domain values in an attribute
**
** History:
**      5-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef f4 OPH_DOMAIN;

/*}
** Name: OPH_HISTOGRAM - normalized histogram structure
**
** Description:
**      This structure contains histogram statistics gathered by optimizedb.
**      Note that there is some histogram information in the joinop attributes
**      structure.  The division is made since OPH_HISTOGRAM structures can
**      be created dynamically to estimate the distribution of the result of
**      a join.  There is a many to one map of histogram structures to a joinop
**      attribute.
**
** History:
**     16-mar-86 (seputis)
**          initial creation
**	27-dec-90 (seputis)
**	    changed the complete flag to be FALSE by default
**	7-jan-96 (inkdo01)
**	    Added per-cell array of repetition factors for improved join
**	    cardinality computation.
**	15-may-00 (inkdo01)
**	    Changes for composite histograms.
**	26-sep-00 (inkdo01)
**	    Collected composite-specific fields into a single structure.
**	2-oct-00 (inkdo01)
**	    Added OPH_INCLIST, OPH_KESTIMATE flags for composite histos.
**	26-oct-05 (inkdo01)
**	    Added oph_exactcount, oph_numexcells for improved join estimates.
**	31-oct-05 (inkdo01)
**	    Added OPH_ALLEXACT flag and oph_excells pointer.
[@history_line@]...
*/
typedef struct _OPH_HISTOGRAM
{
    struct _OPH_HISTOGRAM *oph_next;    /* form a linked list of these elements
                                        */
    OPZ_IATTS       oph_attribute;      /* offset into opz_attnums
                                        ** - i.e. joinop attribute number
                                        ** - used to access the interval
                                        ** information for this histogram.
					** If -1, this is composite histogram
					** and attr vector is addressed by
					** oph_attrvec
                                        */
    OPH_COMP	    oph_composite;	/* composite histogram-specific stuff,
					** gathered into a single structure
					** (only filled if OPH_COMPOSITE) */
    OPN_PERCENT     oph_prodarea;       /* area of this histogram; i.e.
                                        ** sum of products (cell by cell)
                                        ** of this histogram (i.e. oph_fcnt)
                                        ** and the oph_fcnt array of the 
                                        ** previous version of this histogram
                                        ** (i.e. oph_pcnt)
                                        ** - represents selectivity of the
                                        ** entire relation for this histogram
                                        */
#define                 OPH_UNIFORM     (-1.0)
/* flag to indicate that this histogram is created rather than read from a
** catalog
*/
    OPN_PERCENT     *oph_pcnt;          /* pointer to a oph_fcnt array in
                                        ** another OPH_HISTOGRAM struct, used
                                        ** by opn_h1 to point to the previous
                                        ** version of the count array which is
                                        ** currently being modified into
                                        ** oph_fcnt
                                        ** - each cell contains the percentage
                                        ** (0.0 -> 1.0) of tuples of the
                                        ** relation which hit the interval
                                        ** represented by the cell
                                        ** - note that oph_pcnt will integrate
                                        ** to 1.0 ;i.e. sum of cells equals 1.0
                                        */
    OPN_PERCENT     oph_pnull;          /* percentage of attribute which
                                        ** contains nulls - extension of
                                        ** oph_pcnt */    

    /* - if this histogram was obtained from disk then the following information
    ** was obtained directly from the histogram system catalogs 
    ** - otherwise, the following information was generated assuming a uniform
    ** distribution with "worst case" assumptions
    */
    OPO_TUPLES      oph_reptf;          /* repitition factor - number of rows
                                        ** divided by number of unique values
                                        ** in active domain
                                        */
#define                 OPH_NODUPS      (1.0)
/* indicates no duplicates in repitition factor - default for histogram creation
*/

    OPH_DOMAIN      oph_nunique;        /* number of unique values in the 
                                        ** active domain for this attribute
                                        ** - thus 1.0 means all tuples in
                                        ** this relation have the same value
                                        ** - 2.0 would represent a boolean
                                        ** type of attribute (e.g. "M" or "F")
                                        */
    OPH_COUNTS      oph_fcnt;           /* counts for the cells in this
                                        ** histogram
                                        ** - points to an array of floats
                                        ** - this array has oph_numcells 
                                        ** elements
                                        ** - may be used as a "type 1" or
                                        ** type "2" array depending on which
                                        ** list this histogram belongs
                                        ** - type 1 - if it belongs to
                                        ** a OPN_RLS.opn_histogram list
                                        ** it will contain fractions of the
                                        ** entire relation tuple count
                                        ** -type 2 - if it belongs to
                                        ** an OPB_BOOLFACT.opb_histogram list...
                                        ** used as a working array when
                                        ** boolean factors are being analyzed
                                        ** where oph_fcnt[i] contains a
                                        ** percentage of elements referenced
                                        ** in oph_pcnt[i] by the boolean factors
                                        ** where 1.0 in cell means all tuples
                                        ** of the original cell are selected
                                        ** - also it will temporarily contain
                                        ** number of tuples rather than
                                        ** fractions prior to being normalized
                                        ** when it is used inside oph_join
                                        */
    OPO_TUPLES	   *oph_creptf;		/* per cell repetition factors
					** - addresses array of floats with
					** oph_numcell elements. Each is the
					** repetition factor (total rows in 
					** cell / unique vals in cell) for
					** corresponding histogram cell.
					*/
    OPN_PERCENT     oph_null;           /* percentage of attribute which
                                        ** contains nulls, extension of
                                        ** oph_fcnt */    
    OPO_TUPLES	    oph_origtuples;	/* used to make FSM joins and key
					** joins estimate tuples on a more
					** or less equal basis, when multi-
					** attribute joins are involved */
    OPN_PERCENT	    oph_exactcount;	/* percentage of rows covered by
					** "exact" cells */
    i4		    oph_numexcells;	/* count of exact cells */
    OPH_BOOL        *oph_excells;	/* ptr to array of bools indicating
					** exact cells (or NULL) */
    i4		    oph_mask;		/* mask of booleans */
#define                 OPH_OJLEFTHIST  1
/* TRUE - if the outer join distribution was obtained from the left
** histogram of the join */
#define			OPH_COMPOSITE	2
/* TRUE - if this is a composite histogram (on an index). */
#define			OPH_INCLIST	4
/* TRUE - if composite histogram has been chosen for selectivity
** computation (only used inside ophrelse.c) */
#define			OPH_KESTIMATE	8
/* TRUE - if BFs matchning this compositie histogram are all OPB_KESTIMATE
** implying this histogram is a key estimation histogram (only used inside
** ophrelse.c). */
#define			OPH_ALLEXACT	16
/* TRUE - if histogram consists of all exact cells */
    BITFLD          oph_unique:1;       /* TRUE - if all values in this
                                        ** attribute are unique
                                        ** - default for TID attribute is
                                        ** unique, otherwise non-unique is
                                        ** assumed, except in case of
                                        ** unique key on a single attribute
                                        ** in a storage structure
                                        */
#define                 OPH_DUNIQUE     FALSE
/* default flag is non-unique */
#define                 OPH_UFACTOR     (OPN_PERCENT)0.90
/* percentage of relation which must have unique values before the
** oph_unique flag can be set to TRUE in the iistatistics relation */

    BITFLD          oph_complete:1;     /* TRUE - if all values which exist
                                        ** in the database (for this domain)
                                        ** also exist in this attribute
                                        ** - default is TRUE if histogram
                                        ** does not exist in catalogs
                                        */
#define                 OPH_DCOMPLETE   FALSE
/* default flag is that all values do not exist in this attribute */
    BITFLD          oph_default:1;      /* TRUE - if this is a default histogram
                                        */
    BITFLD	    oph_odefined:1;	/* TRUE - if oph_origtuples is defined */
}   OPH_HISTOGRAM;


/*}
** Name: OPH_INTERVAL - histogram type and interval information
**
** Description:
**      This structure is part of the joinop attributes element.  It describes
**      characteristics of the histogram for the attribute that are unchanging
**      such as the min/max values for cells (interval info), type etc.  This
**      is differenct from OPH_HISTOGRAM which can be dynamically calculated
**      for the result of a join.
**
**	NOTE: oph_domain is assigned only 1 value and is therefore useless.
**	oph_mindomain and oph_maxdomain aren't used anywhere and are also
**	useless (both are assigned values by functions which are never called)
**	inkdo01 15/5/00.
**
** History:
**     10-mar-86 (seputis)
**          initial creation
**	09-nov-92 (rganski)
**	    Added new fields oph_charnunique and oph_chardensity for new
**	    character histogram statistics gathered by optimizedb on
**	    character-type attributes. oph_charnunique is the number of unique
**	    characters per character position (up to the length of the
**	    histogram values) and oph_chardensity is the character set density
**	    per character position. The character set density is a measure of
**	    how complete the character set in a character position is, stored
**	    as a floating point number between 0 and 1: a density of 1.0
**	    indicates a complete character set, given the number of unique
**	    characters; lower densities indicate sparser character sets. See
**	    the Character Histogram Enhancements specification for more
**	    details.
**	18-jan-93 (rganski)
**	    Changed comment about db_length: no longer has max of 8.
**	20-sep-93 (rganski)
**	    Added new field oph_version, which indicates the version number of
**	    the histogram.
[@history_line@]...
*/
typedef struct _OPH_INTERVAL
{
    DB_DATA_VALUE   oph_dataval;        /* datatype of histogram where
                                        ** - db_length of histogram cell; same
                                        ** as attribute len except for chars,
                                        ** when it is determined by
					** adc_hg_dtln() or by user.
                                        ** - db_datatype of histogram cell;
                                        ** INTEGER if oph_type 
                                        ** is an abstract datatype
                                        ** else equal to joinop attr type; i.e.
                                        ** OPZ_ATTS.opz_dataval.db_datatype
                                        ** - db_data is not used
                                        */
    i4              oph_domain;         /* domain of this attribute from
                                        ** the zopt1stat relation, gives
                                        ** meaning to the oph_complete domain
                                        ** flag
                                        */
#define                 OPH_NODOMAIN    (-1)
/* flag to indicate that this histogram component was created rather then read
** from the system catalogs
*/
#define			OPH_DDOMAIN     FALSE
/* default oph_domain value - indicates that the complete flag is not
** valid */
    i4              oph_numcells;       /* number of histogram cells in the
                                        ** oph_histmm array
                                        */
#define                 OPH_NOCELLS     (-1)
/* used to indicate that histogram has not been assigned yet */
#define                 OPH_DNUMCELLS   2
/* default number of cells for a uniform histogram */


    /* The structure of this untyped pointer depends on the datatype of the
    ** attribute.  One cell in this structure is oph_length bytes long
    ** and there are oph_numcells.  Thus, the storage occupied by this structure
    ** is equal to (oph_numcells*oph_length) bytes.  Each value in
    ** this structure is a boundary value for a histogram cell; i.e. these
    ** values partition the active domain into histogram cells.
    */
    OPH_CELLS       oph_histmm;         /* min,max values of histogram cells
                                        ** - defaults for uniform distribution
                                        ** obtained from ADF function
                                        */
    PTR             oph_mindomain;      /* ptr to the minimum histogram value
                                        ** for domain of this attribute
                                        ** - NULL if not initialized yet
                                        */
    PTR             oph_maxdomain;      /* ptr to the maximum histogram value
                                        ** for domain of this attribute
                                        ** - NULL if not initialized yet
                                        */
    i4		    *oph_charnunique;	/* array: number of unique characters
					** per character position, for
					** character-type attributes
					*/
    OPH_COUNTS	    oph_chardensity;	/* array: character set density per
					** character position, for
					** character-type attributes
					*/
    DB_STAT_VERSION oph_version;	/* String containing of version
					** number of histogram
					*/
}   OPH_INTERVAL;
