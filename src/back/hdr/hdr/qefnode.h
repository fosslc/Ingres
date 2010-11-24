/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEFNODE.H - Describe the nodes of a Query Execution Plan (QEP)
**
** Description:
**      A QEP is composed of one or more nodes describing some set of
**  required actions. The node types are
**  QEN_TJOIN   - tid join
**  QEN_KJOIN   - key probe join
**  QEN_SJOIN   - full sort/merge, inner_sorted, and cart_prod joins
**  QEN_HJOIN   - hash join
**  QEN_SEJOIN	- perform a join where inner is QEN_QP and executing
**		  a subselect
**  QEN_SORT    - initiate a sort
**  QEN_TSORT   - top sort node - load sorter and retrieve rows
**  QEN_ORIG    - get a row from a relation or sort file
**  QEN_QP      - get a row from a query plan
**  QEN_EXCH	- exchange rows between different threads executing
**		  the same plan
**  QEN_TPROC	- call table procedure in query plan
**  
**
** History:
**      19-mar-86 (daved)
**          written
**      7-apr-86 (daved)
**          references to sub nodes are now pointers to facilitate
**          argument passing. The use of pointers is OK in this instance
**          because they are not data dependent. Initialization
**          requires that the postfix notation be converted to pointers.
**	29-jan-87 (daved)
**	    added qen_sejoin
**	24-jul-87 (daved & puree)
**	    add qen_pjoin
**	25-jan-88 (puree)
**	    redefine literals for sejn_junc in QEN_SEJOIN.
**	26-sep-88 (puree)
**	    modified QEN_SORT and QET_TSORT for QEF in-memory sorter.
**	    moved QEN_HOLD into QEFDSH.H
**	29-jan-89 (paul)
**	    Aded support for rules rpocessing.
**	27-mar-89 (jrb)
**	    Added .attr_prec field to QEF_KATT struct for decimal project.
**      02-nov-89 (davebf)
**          Modified to support outer join.
**	17-dec-90 (stec)
**	    Modified type and meaning of sejn_hcreate in QEN_SEJOIN struct.
**	18-dec-90 (kwatts)
**	    Smart Disk integration: Added sd_program and sd_pgmlength to
**	    the QEN_NODE structure.
**	25-mar-92 (rickh)
**	    To support right joins, upgraded the tid sort array to be a
**	    hold file.  This meant replacing oj_shd with oj_tidHoldFile.
**	    Removed oj_create and oj_load, which weren't being used.
**	    Added oj_ijFlagsFile for right fsmjoins.
**	11-aug-92 (rickh)
**	    Added QEN_TEMP_TABLE, the descriptor for temporary tables.
**	04-dec-1992 (kwatts)
**	    Introduced QEN_TSB structure for Smart Disk and changed 
**	    description of QEN_ORIG to have this one field for Smart Disk.
**	26-mar-93 (jhahn)
**	    Added ttb_attr_count to QEN_TEMP_TABLE.
**	20-apr-93 (rickh)
**	    Added new DSH row to the OJINFO structure.  This row is where
**	    fulljoins expose to higher nodes the special equivalence classes
**	    they set.
**	20-jul-1993 (pearl)
**          Bug 45912.  Add qef_kor field to QEN_NOR structure to keep track
**          of which QEF_KOR structure it is associated with.
**          Add kor_id field to QEF_KOR structure.
**	07-feb-94 (rickh)
**	    Added key array and count to the temp table descriptor.
**      01-nov-95 (ramra01)
**          Join node optimization. Prepare noadf_mask to indicate 
**          presence of adf cbs for request codes, actually the
**          lack of adf cbs. 
**	7-nov-95 (inkdo01)
**	    Reorganized various node structures.
**	06-mar-96 (nanpr01)
**	    Added fields in QEN_SORT, QEN_TSORT, QEN_TEMP_TABLE for
**	    variable page support.
**      13-jun-97 (dilma04)
**          Added QEN_CONSTRAINT flag to QEN_ADF which indicates that 
**          procedure supports integrity constraint.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	26-feb-99 (inkdo01)
**	    Added hash join structure (QEN_HASHJOIN)
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	7-nov-03 (inkdo01)
**	    Added exchange node structure (QEN_EXCH)
**	7-Dec-2005 (kschendel)
**	    Rearrange a couple of the more common nodes to reduce
**	    excess intra-struct padding on 64-bits.  (Pointers to the
**	    front.)
**	12-Jan-2006 (kschendel)
**	    Get rid of the xcxcb.  Get rid of all the noadf-mask's, no
**	    longer used since '03.
**	17-Jul-2007 (kschendel) SIR 122513
**	    Revise partition qualification stuff to implement join-time
**	    and generalized partition qualification.
**      09-Sep-2008 (gefei01)
**          Added struct _QEN_TPROC.
**	4-Jun-2009 (kschendel) b122118
**	    Fix up keying comments, delete unused structure members.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query - Support cardinality requests in QEN_NODE
**	    instead of just in sejoin.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _QEN_ADF QEN_ADF;
typedef struct _QEN_SJOIN QEN_SJOIN;
typedef struct _QEN_KJOIN QEN_KJOIN;
typedef struct _QEN_HJOIN QEN_HJOIN;
typedef struct _QEN_NKEY QEN_NKEY;
typedef struct _QEN_PART_INFO QEN_PART_INFO;
typedef struct _QEN_PART_QUAL QEN_PART_QUAL;
typedef struct _QEN_OJINFO QEN_OJINFO;
typedef struct _QEN_NODE QEN_NODE;
typedef struct _QEN_NOR QEN_NOR;
typedef struct _QEN_ORIG QEN_ORIG;
typedef struct _QEN_QP QEN_QP;
typedef struct _QEN_SEJOIN QEN_SEJOIN;
typedef struct _QEN_SORT QEN_SORT;
typedef struct _QEN_TJOIN QEN_TJOIN;
typedef struct _QEN_TKEY QEN_TKEY;
typedef struct _QEN_TSORT QEN_TSORT;
typedef struct _QEN_EXCH QEN_EXCH;
typedef struct _QEN_TEMP_TABLE QEN_TEMP_TABLE;
typedef struct _QEN_TPROC QEN_TPROC;


typedef struct _QEF_AHD	QEF_AHD;
typedef struct _QEF_KAND QEF_KAND;
typedef struct _QEF_KATT QEF_KATT;
typedef struct _QEF_KEY QEF_KEY;
typedef struct _QEF_KOR	QEF_KOR;
typedef struct _QEF_QP_CB QEF_QP_CB;

/*}
** Name: QEN_BASE - initialize the base array for CX structs
**
** Description:
**      These elements describe the contents of array elements in the
**  ADE_CX struct.
**
** History:
**      31-jul-86 (daved)
**          written
**	23-aug-04 (inkdo01)
**	    Add union to store dsh_row index for parms.
**	3-Jun-2009 (kschendel) b122118
**	    Delete obsolete adf-ag-struct stuff.
[@history_line@]...
[@history_template@]...
*/
typedef struct _QEN_BASE
{
    i4	    qen_index;		/* offset in next array to find object
				** this value should be zero if the array
				** is QEN_VTEMP or QEN_DMF_ALLOC
				*/
    i4	    qen_array;		/* what array is offset in */
#define QEN_ROW		0	/* get a row address */
#define	QEN_KEY		1	/* get a key. offset should be set to 0
				** because all keys are in the same buffer
				*/
#define	QEN_PARM	2	/* get a user defined parameter */
#define	QEN_OUT		3	/* user's output buffer. offset should be 0 */
#define	QEN_VTEMP	4	/* space holder for CX VLUP temp buffer */
#define	QEN_DMF_ALLOC	5	/* space holder for tuple provided by DMF */
/*	notused		6	was obsolete old-style aggregate thing */
#define	QEN_PLIST	7	/* Get a call procedure parameter list	    */
				/* address */
    i4	    qen_parm_row;	/* dsh_row buffer index for QEN_PARM */
}   QEN_BASE;


/*}
** Name: QEN_ADF - QEF's ADF CX structure initializer
**
** Description:
**	This structure contains all the elements necessary
**  to process ADF compiled buffers. This includes
**  initialization parameters and the actual compiled buffer.
**
** History:
**     08-May-86 (daved)
**          written
**     01-sep-88 (puree)
**	    rename qen_adf to qen_ade_cx.
**	    rename qen_param to qen_pcx_idx and used it as the index (relative
**	    to 1) to the QEE_VLT array in QEE_DSH area.  This allows us to
**	    associate a CX with user's parameters to its VLUP/VLT space.
**      15-may-94 (eric)
**          Added QEN_HAS_PARAMS so that CX knows whether repeat query 
**          or db proc params are used in the CX.
**	16-oct-95 (inkdo01)
**	    Added QEN_HAS_PERIPH_OPND to indicate long_something operand
**	    (knowledge of this speeds up ADF execution logic)
**      13-jun-97 (dilma04)
**          Added QEN_CONSTRAINT flag to indicate that procedure supports
**          integrity constraint.
**	15-Jan-2010 (jonj)
**	    Constraint procedures without parameters won't have this QEN_ADF.
**	    Replace QEN_CONSTRAINT here with QEF_CP_CONSTRAINT in QEF_CALLPROC's
**	    ahd_flags.
*/
struct _QEN_ADF
{
    PTR		qen_ade_cx;	/* the compiled expression. Will be ADE_CX.
				** If the pointer to the qen-adf in a node or
				** action header is non-NULL, this will be
				** non-NULL (ie there will be a CX there).
				*/
    QEN_BASE    *qen_base;	/* array of values to initialize entries in
				** base array from location ADE_ZBASE on.
				** Entry N in this array initiliazes entry
				** ADE_ZBASE+N in the base array.
				*/
    i4		qen_pos;	/* index into DSH structure (dsh_cbs)
				** containing the ADE_EXCB parameter
				*/
    i4		qen_max_base;	/* number of elements in qen_base array */
    i4		qen_sz_base;	/* Number of elements used in qen_base.
				** the base array in the ADE_EXCB struct
				** will be ADE_ZBASE + qen_sz_base elements 
				** long.
				*/
    i4		qen_pcx_idx;	/* dsh-vlt array index (+1), if there are
				** parameters (VLUP) referenced in this CX.
				** This forces the base array to be initialized
				** at validation time.
				**
				** Zero if no vlup's in this one.
				*/
    i4		qen_uoutput;	/* index into qen_base containing QEN_BASE entry
				** for user defined output row if output
				** is user's output buffer. This only applies
				** for ACT_GET action items. When compiling
				** expressions, make no assumptions as to the
				** time the output buffer will be placed in the
				** CX struct.
				** If this is a QEA_RUP action, this output
				** value is used to contain the tuple
				** in the updatable relation.
				*/
    i4		qen_output;	/* Index into DSH->DSH_ROW containing output
				** row. Set to -1 if output is not a row
				*/
    i4		qen_input;	/* index into qen_base containing initializer
				** for DMF supplied tuple buffer. 
				*/
    i4		qen_mask;	/* Bit mask of CX info */
	/* The QEN_SVIRGIN, QEN_SINIT, QEN_SMAIN, and/or QEN_SFINIT bit(s)
	** will be on if the CX has virgin, init, main, and/or finit
	** segment instructions compiled into it.
	*/
#define	    QEN_SVIRGIN	    0x001
#define	    QEN_SINIT	    0x002
#define	    QEN_SMAIN	    0x004
#define	    QEN_SFINIT	    0x008

	/* QEN_HAS_PARAMS is set if the base array uses either QEN_PARM or
	** QEN_PLIST elements.
	*/
#define	    QEN_HAS_PARAMS  0x010
	/* QEN_HAS_PERIPH_OPND indicates that at least one operand of at
	** least one operation of this code block is long_something.
	*/
#define     QEN_HAS_PERIPH_OPND 0x020
};



/*}									     
** Name: QEN_TEMP_TABLE - Descriptor for temporary tables
**
** Description:								       
**	This descriptor contains all the information needed to allocate
**	a run time temporary table.
**
**
**
** History:								       
**	11-aug-92 (rickh)
**	    Created.
**	28-Oct-92 (jhahn)
**	    Added ttb_attr_list.
**	26-mar-93 (jhahn)
**	    Added ttb_attr_count.
**	07-feb-94 (rickh)
**	    Added key array and count to the temp table descriptor.
**	06-mar-96 (nanpr01)
**	    Added page size for temp table .
*/									       
struct	_QEN_TEMP_TABLE
{
    i4		ttb_tempTableIndex;	/* index into dsh array of temp table
					** descriptors.  -1 means no temp
					** table */

    i4		ttb_tuple;		/* index into dsh rows of storage
					** for this table's tuple */

    DMF_ATTR_ENTRY **ttb_attr_list;	/* pointer to list of attributes
					** for this table. If NULL, create
					** attribute list at run time. */
    i4		ttb_attr_count;		/* number of attributes or 0 */

    DMT_KEY_ENTRY  **ttb_key_list;	/* pointer to array of keys for this
					** table. */
    i4		ttb_key_count;		/* number of keys or 0 */
    i4	ttb_pagesize;		/* Temp table page size */
};

/*}									     
** Name: QEN_OJINFO - This structure holds information that is		        
**		      specific to "outer" join processing                       
**
** Description:								       
**  This structure holds information that is common to all join nodes that     
**  is required for "outer" join processing.				       
**									       
**  Oj_jntype tells the type of join to do. The join types INNER, LEFT,        
**  RIGHT, and FULL are legal here.					       
**									       
**  If oj_jntype indicates that a RIGHT or FULL join is to be done then	       
**  oj_rqual will be used to qualify right joins. In order for a right tuple   
**  to satisfy the outer join qualification, oj_rqual must return TRUE for     
**  each left tuple. Once a right tuple satisfies an outer join qualification  
**  oj_rnull should be executed, which will fill in NULL/default values for    
**  the left side tuple, before the right tuple is returned.		       
**									       
**  Oj_lqual and oj_lnull should be used for the left tuples in a like manner  
**  if oj_jntype is LEFT or FULL.					       
**									       
** History:								       
**	28-mar-86 (eric) 						       
**          written							       
**	25-mar-92 (rickh)
**	    To support right joins, upgraded the tid sort array to be a
**	    hold file.  This meant replacing oj_shd with oj_tidHoldFile.
**	    Removed oj_create and oj_load, which weren't being used.
**	    Added oj_heldTidRow also.  Added oj_ijFlagsFile for right fsmjoins.
**	20-apr-93 (rickh)
**	    Added new DSH row to the OJINFO structure.  This row is where
**	    fulljoins expose to higher nodes the special equivalence classes
**	    they set.
**	7-nov-95 (inkdo01)
**	    Replaced QEN_TEMP_TABLE instance by pointer.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Corrected nested comment
*/									       
struct _QEN_OJINFO						       
{									       
	/* *Oj_oqual qualifies a join which has passed the kqual join.         
	** If oqual is true, we have an inner join candidate.  If not,       
	** we may still have an outer join candidate.
	*/
    QEN_ADF	*oj_oqual;						       

	/* *Oj_equal processes a pair of rows which have passed both  
	** the kqual join and the oqual join  (ie. an inner join candidate)
	*/
    QEN_ADF	*oj_equal;						       

	/* Oj_rnull and oj_lnull provide the appropriate null/default values   
	** for a satisfied right or left outer join (respectively).
	*/								       
    QEN_ADF	*oj_rnull;						       
    QEN_ADF	*oj_lnull;						       

	/*
	** This describes a temporary table which remembers the TIDs of
	** inner hold file tuples which inner joined.  At the end of 
	** ISJOIN processing, this table is rescanned.  All inner tuples not 
	** represented in this file are emitted as outer joins.
	*/
    QEN_TEMP_TABLE	*oj_innerJoinedTIDs;

	/*								       
	** Oj_jntype tells whether we are doing an inner, left, right, or
	** full join
	*/
    DB_JNTYPE	oj_jntype;						       
									       
	/*
	** oj_tidHoldFile is an index into the DSH_HOLD array for the hold
	** file of tids used for right joins.  oj_heldTidRow is the dsh row
	** holding the tid.
	*/								       
    i4         oj_tidHoldFile;
    i4		oj_heldTidRow;

	/*
	** oj_ijFlagsFile is an index into the DSH_HOLD array for the hold
	** file of flags indicating which inner tuples inner joined with some
	** outer.  This is used for detecting right fsmjoins.  When a given
	** outer key has been exhausted, this hold file is rescanned.  Any
	** inner tuple on this key which hasn't yet joined with an outer is
	** then emitted as a right join.
	*/
    i4		oj_ijFlagsFile;
    i4		oj_ijFlagsRow;

	/*
	** This DSH row holds the equivalence classes overwritten by this
	** node.  Descendants of this node may force their own special
	** eqcs and fill in other eqcs.  It would be bad policy to overwrite
	** our descendants' eqcs
	** with transitory values.  For instance, it used to
	** be that an RNULL would overwrite eqcs from the
	** driving stream.  Unfortunately,
	** it could happen that after emitting a right join, an fsmjoin
	** could immediately emit its current outer tuple as a left join--
	** inheriting the overwritten eqcs from the driving stream.
	*/

    i4			oj_resultEQCrow;

	/*
	** This DSH row holds the "indicator" variable for the source of
	** an outer join whose columns are referenced higher in the query
	** plan.
	*/

    i4			oj_specialEQCrow;
};									       


/*
** Partition Qualification - overview
**
**	Partition qualification (a.k.a. partition elimination or
**	partition pruning) is relatively complicated, since its
**	effects can reach across the query plan.
**
**	The fundamental operation in partition qualification is to
**	generate a partitioning-scheme value (ie one or more column
**	values at scheme-defined offsets), and apply a "which partition"
**	operation to the value.  The returned partition number is
**	used to set (or clear!) one or more bits in a logical
**	partition bitmap, which can in turn be translated into a
**	physical partition bitmap.  The physical partition bitmap
**	is used by an ORIG (or ORIG-like node, e.g. kjoin inner)
**	to decide which partitions to read (or probe) and which to skip.
**
**	Partition qualification can take place either at query
**	startup (for constant qualifications) or at runtime (for
**	join-time qualifications).  A simple qualification like:
**
**	    ... where t.part_col = 1 and ...
**
**	can obviously be evaluated at query startup and applied to
**	the relevant ORIG node.  Complex conditions are dealt with by
**	setting bits based on the comparison operation (e.g. >=, <=)
**	into work maps and AND'ing or OR'ing with the result:
**
**	    ... where t.dim1_col = 1
**		    and (t.dim2_col = 5 or t.dim2_col between 10 and 20) ...
**
**	would be done with something like:
**		EVAL dim1 1 INTO result_map
**		EVAL dim2 5 INTO work_map #2
**		EVAL dim2 >=10 INTO work_map #3
**		EVAL dim2 <=20 INTO work_map #4
**		AND work_map #4 INTO work_map #3
**		OR work_map #3 INTO work_map #2
**		AND work_map #2 INTO result_map
**
**	obviously there are various ways of encoding the condition, but
**	the idea is to use a mini-program that can perform the basic
**	operations on the physical partition bitmaps.  The value
**	expressions are materialized using a CX;  in these examples, it's
**	just moving a constant to the right place, but it could be a
**	lot fancier.  (Actually, one CX lays down all the necessary
**	constants or expression results into a row, end-to-end;  this
**	takes more row space, but a CX is a sufficiently clumsy animal
**	that one big CX easily wins over many little ones.)
**
**	Join-time qualification is trickier, but can have major benefits
**	in some particular situations.  There are a couple different
**	cases here:  K-join qualification and merge/hash join qualification.
**
**	K-join inners can be qualified with constant predicates just
**	like ordinary orig nodes.  The only difference so far is that
**	there isn't an actual orig for the K-join inner, so the necessary
**	data structures have to be attached to the K-join node.
**	However, if the join predicates involve partitioned columns,
**	we can apply the qualification process as each outer row appears;
**	thus probing only the qualifying partitions of the inner rather
**	than every single partition.  Note that the relevant predicates need
**	not involve key columns;  consider
**
**	    ... where l1.c1 = t.key_col
**		    and t.dim_col between l2.lower and l2.upper ...
**
**	where l1 and l2 are tables on the outer side of the K-join.
**	It's perfectly possible to build a qualification bitmap from
**	each outer row, using the between predicate.
**
**	The final possibility for qualification is when the partitioned
**	table is somewhere below the inner side of a hash join or
**	FSM-join.  (The latter must involve an explicit outer sort for
**	this to work.)  The idea here is that *ALL* rows of the outer
**	are collected before fetching *ANY* rows of the inner.  (Either
**	for the hash table build, or for the outer sort which is done
**	before the inner sort.)  Thus, we can generate a qualification
**	bitmap from each outer row in the same manner as we do for
**	K-joins;  except, instead of applying it immediately, we OR
**	all the bitmaps together, and then AND that result into the
**	orig qualification bitmap below the inner of the join.
**	Note that this works even if the partitioned inner is way down
**	underneath somewhere;  it doesn't have to be immediately below
**	the hash/merge join!  Also, more than one join along the way
**	can contribute to pruning the ultimate bitmap.
**
**	An important optimization for this case is to recognize when
**	the join is no longer restricting the partitioning (i.e. the
**	bitmap has become all ones);  if this happens, we stop doing all
**	the laborious partitioning value calculations and just let
**	the join rip.  A related optimization occurs when some other
**	completed qualification has already restricted the table down
**	to something "small" (e.g. one partition);  we can turn off
**	the join-time qualifications at that point, since we're unlikely
**	to be able to improve things.
**
**	In the FSM join case, we actually attach the partition
**	qualification junk to the outer's TSORT node rather than
**	to the FSM-join node itself.  The TSORT is the collection
**	point for the outers, and it's easier to build up the
**	qualifier bitmaps there (on the way into the sort, as a
**	matter of fact).  For the hash join case, the partition
**	qualification stuff can attach to the HJOIN node, since
**	the hash join is going to read the entire outer first
**	and it can build the bitmaps as it goes.
**
**	FSM/hash joins that are already partition-compatible do not
**	do join time qualification.  PC-joins already proceed
**	partition by partition, so in the usual 1:1 case there's no
**	point doing all that work when we know what inner partition
**	we're reading anyway.  (Hash-partitioned tables can do a
**	1:N PC-join, but N is usually a very small number and it's
**	still not worth it.)
**
**	Outer joins complicate the picture, but only to the extent that
**	the optimizer has to recognize non-restricting ON qualifications
**	and not do partition restriction;  it's the optimizer's problem.
**
**	As a side note:  partition qualification might seem a little
**	bit like keying, but the big difference is that it's practical
**	to represent the qualification result in a bit-map (i.e. as a
**	physical partition map).  The predicate evaluation with any
**	ANDs and ORs can be simplistic or redundant, as long as it ends
**	up setting the right bits.  Keying can't produce a which-row
**	bitmap (and it would be impractical even if it could), so
**	keying has to get resolved into non-overlapping range lookups.
**	The non-overlapping requirement for keying causes all sorts of
**	other restrictions that don't apply to partition qualification.
**	So, while it was superficially tempting to try to share
**	mechanisms between the two, they ended up being entirely separate.
**
*/

/*
** Name: QEN_PART_QUAL - QP information for partition qualification
**
** Description:
**
**	This structure defines the information needed to do partition
**	qualification.  It may exist as part of an orig;  a K-join
**	or T-join(*) node;  or a FSM or hash join node.
**
**	(*) qualifying against a T-join node isn't currently implemented.
**	There's nothing wrong with the idea;  it would eliminate base
**	table reads for tids that point to non-qualifying partitions.
**	I just haven't gotten around to doing the code yet.  FIXME.
**
**	The info needed for FSM/hash join nodes is slightly different
**	than what we need for origs or K-join inners, but splitting
**	it up into two structs would only save a dozen or so bytes per
**	node;  so I haven't bothered.  I will note which members are
**	used for which kinds of nodes.  Structure members that aren't
**	used in some context (such as the constant-qual-map index in a
**	hash-join node context) are indeterminate and must not be
**	used for anything.
**
**	Shorthand:  PP = physical partition, LP = logical partition.
**
**	The QEN_PART_QUAL contains:
**	- A copy of the node qen_type (e.g. QE_ORIG, QE_HJOIN, etc).
**	This could be useful for doing generic processing of a qual
**	struct, to know what is filled in and what isn't.  [all]
**
**	- An index into the dsh_part_qual array of QEE_PART_QUAL
**	structures, which hold runtime info related specifically to this
**	particular execution of this qualification.  [all]
**
**	- A pointer to the QEN_PQ_EVAL directives that compute the
**	qualifying bitmap for constant predicates.  The result AND's
**	into the constant-qual-map below.  [orig, K/T-join]
**
**	- A pointer to the QEN_PQ_EVAL directives that compute the
**	qualifying bitmap for joining predicates.  The result OR's
**	into the local-result map.  [K/T-join, fsm/hash joins]
**	(We need two separate sets of directives thanks to the lack of
**	a real ORIG for K/T-join inners.)
**
**	- A pointer to the next QEN_PART_QUAL that needs to be evaluated
**	at this join node.  [fsm/hash joins]
**
**	- The DSH row number for a constant-qualification bitmap.
**	This PP bitmap starts out as all ones, and is AND'ed with the
**	qualification results from any constant qualifications.
**	This is done just once, at execution-time setup.
**	[orig, K/T-join]
**
**	- The DSH row number for the local-result map row, a pp map row.
**	For FSM/hash joins, this is where we accumulate (via OR) the
**	qualifying partitions resulting from all the outers (starting
**	out from zero at a node reset).
**	For K/T joins, the same idea, except that we reset lresult to zero
**	for each outer.  So, local-result is qualifying partitions for
**	this outer row.
**	For ORIG and K/T joins, this bitmap eventually holds the final
**	qualifying partition map:  the AND of the row local-result
**	(K/T joins), all higher-up qualification maps, and the constant
**	bitmap.
**	Note: for ORIG, this local-result bitmap ignores the effects
**	of PC-join grouping.  PC-join grouping is dealt with independently
**	inside the ORIG.  (K/T-joins don't PC-join.)
**	[all]
**
**	- A pointer to the orig node for which we're gathering the
**	qualifications.  As a convenience, for orig-like things (orig,
**	K-join, T-join), this will be a "pointer to self".  [all]
**
**	- A pointer to the DB_PART_DEF partition definition, with at least
**	the dimensions used for qualification (at this node and elsewhere)
**	filled in.  [orig, K/T-join]
**
**	- The DSH row number for a bitmap work row, used for LP -> PP
**	conversion.  This row can be omitted (left as -1) if the table
**	only has a single partitioning dimension, since in that case
**	logical and physical partitioning is the same.  Since this is
**	a strictly temporary-use thing, it's only listed in the orig-like
**	node, but all related quals use the same temp row.  [orig, K/T-join]
**
** History:
**	23-Jul-2007 (kschendel)
**	    Completely revise partition qualification for generalized
**	    qualification, join-time qualification.
*/

/* Start by defining the QEN_PQ_EVAL structure that implements the
** mini-program used to compute the physical partition bitmap resulting
** from qualification predicates.
** An array of these things starts with a PQE_EVAL_HDR variant, and
** continues with VALUE instructions and possibly AND/OR instructions.
** It's up to the query compiler to ensure that the ultimate result
** bitmap ends up someplace useful, either directly or via AND/OR MAP
** instruction.
*/
struct _QEN_PQ_EVAL
{
    i2		pqe_size_bytes;		/* Size of this variant, bytes;
					** could compute it from eval-op but
					** let opc do it once, save qef the
					** effort.  (and there's room anyway.)
					*/
    i2		pqe_eval_op;		/* What to do, see below */
    i4		pqe_result_row;		/* DSH row number of bitmap result */
    union {
	struct {			/* This variant if op is VALUE */
	    i4	pqe_start_offset;	/* Where value(s) start in value row */
	    i4	pqe_nvalues;		/* Number of values to apply relop to,
					** most commonly used for IN */
	    i4	pqe_value_len;		/* Length of value, ie how much to
					** advance when nvalues is > 1
					*/
	    i2	pqe_dim;		/* Dimension we're qualifying against,
					** index into part-def dim array */
	    i2	pqe_relop;		/* How to use value to set the bitmap,
					** see below */
	} value;
	struct {			/* This variant for AND/OR ops */
	    i4	pqe_source_row;		/* DSH row number of bitmap source */
	} andor;
	struct {			/* This variant if op is HDR */
	    QEN_ADF *pqe_cx;		/* QEN_ADF describing the CX that
					** generates all necessary values for
					** this array, into the value row.
					** Values are stuck in end-to-end.
					*/
	    i4	pqe_value_row;		/* DSH row number of values generated
					** by the above CX
					*/
	    i4	pqe_nbytes;		/* total size of this eval in bytes */
	    i4	pqe_ninstrs;		/* total size of this eval in entries */
	} hdr;
	/* The ZERO and NOT MAP ops omit the union entirely */
    } un;
    /* NOTE! the union must be at the end, since it's not always
    ** included.
    ** Base size (of a ZERO/NOT op) is sizeof(struct) - sizeof(union);
    ** Size of a VALUE variant is base size + sizeof(value struct);
    ** Size of an AND/OR variant is base size + sizeof(andor struct);
    ** Size of a HDR variant is base size + sizeof(hdr struct).
    */
};

typedef struct _QEN_PQ_EVAL QEN_PQ_EVAL;

/* Possibilities for the pqe_eval_op instruction */
#define PQE_EVAL_HDR	0	/* Header, always first, run CX */
#define PQE_EVAL_ZERO	1	/* Zero out result row bitmap */
#define PQE_EVAL_ANDMAP	2	/* AND source bitmap into result row */
#define PQE_EVAL_ORMAP	3	/* OR source bitmap into result row */
#define PQE_EVAL_NOTMAP	4	/* NOT the result-row bitmap, unused for now */
#define PQE_EVAL_VALUE	5	/* Apply whichpart to one or more values in
				** value-row, then use the relop (below) to
				** set result-row bits accordingly.
				** (Result row does not need to be pre-zeroed)
				*/

/* Possibilities for the pqe_relop value: */
#define PQE_RELOP_EQ	0	/* Turn on result bits for partitions
				** corresponding to the exact value(s) */
#define PQE_RELOP_NE	1	/* Light bits for partitions that DON'T
				** hold the given value(s).
				** Only used/usable for LIST partitioning. */
#define PQE_RELOP_LE	2	/* Turn on result bits for all partitions
				** that hold the given value, or less */
#define PQE_RELOP_GE	3	/* Turn on result bits for all partitions
				** that hold the given value, or more */

/* Some notes about the relop definitions:
** - the main reason for the multi-value capability is to handle IN-lists
**   more efficiently.  An IN-list could be done with a long series of OR'ed
**   individual PQE_EVAL_VALUE operations, but that would make the
**   directives longer and more tedious.
**   The multi-value capability is also useful for multi-column (composite)
**   dimensions, as it makes the OPC code a lot simpler.  (In that situation,
**   the OR's are not so apparent since they're splattered across multiple
**   boolean factors for the different columns.  Ugh.)
** - NE is *only* allowed for list partitioning, as that is the only case
**   where we can reasonably expect that no other values share the partition.
** - LE and GE are not usable with hash partitioning.
** - There is no LT or GT, just LE and GE.  The reason is that the only way
**   you could actually do an LT or GT would be to hit the exact break
**   value;  otherwise you have to include the "eq" partition anyway.
** - NOT is currently unused, because not'ing a value set is different
**   from not'ing a physical partition set in most cases.  In other
**   words, just because col=value selects say partition 21 doesn't mean
**   that NOT col=value selects all-but-21;  other rows in 21 might have
**   col != value.  Fortunately, explicit NOT's in boolean factors are
**   rare, they are usually removed by the CNF transform.
*/


/* Ok, having done all that preparation, here's the actual QEN_PART_QUAL
** structure definition.  See the introduction for elaboration, and
** keep in mind that not all members are filled in for all nodes.
*/

struct _QEN_PART_QUAL
{
    QEN_PQ_EVAL	*part_const_eval;	/* Constant predicate evaluation */
    QEN_PQ_EVAL *part_join_eval;	/* Join predicate evaluation */
    QEN_PART_QUAL *part_qnext;		/* Next qualification to evaluate */
    DB_PART_DEF	*part_qdef;		/* Partition definition */
    QEN_NODE	*part_orig;		/* Corresponding orig node, or
					** our owning node if it's orig-like
					*/
    i4		part_pqual_ix;		/* QEE_PART_QUAL array index */
    i4		part_constmap_ix;	/* Constant-qual bitmap ix */
    i4		part_lresult_ix;	/* Local-result pp bitmap row ix */
    i4		part_work1_ix;		/* LP-to-PP work row index */
    i4		part_node_type;		/* Copy of qen_type just in case */
};


/*}									     
** Name: QEN_PART_INFO	- orig information on partitioned tables
**
** Description:								       
**	This descriptor contains information necessary to execute
**	KJOIN, ORIG and TJOIN nodes involving partitioned tables.
**
**	Apart from partition qualification, which we've already taken
**	care of (above), what we need here mostly applies to information
**	needed for orig's that feed into partitioning compatible joining.
**	(PC-join orig's.)  For now, we can only PC-join along one
**	dimension.
**
**	For PC joining, we need partition dimension info and a group
**	count, indicating how many logical partitions should be
**	returned before indicating end-of-group.  For details see
**	qenorig.c.
**
**	For K/T-joins, we keep a couple of extra bitmaps around, since
**	these operations look like both orig's and joins to some extent.
**	(Here we're talking about partition qualification on the inner,
**	which is necessarily a table and not a sub-plan like other joins.)
**	One bitmap holds the intersection (AND) of the constant map and
**	all higher-up join contributions, since that AND is constant for
**	the life of the join.  Another bitmap holds a map of partitions
**	that did not contain the specified key;  if the next outer
**	produces the same key, we can mask off those partitions from
**	consideration.
**
** History:								       
**	2-feb-04 (inkdo01)
**	    Written for table partitioning.
**	13-apr-04 (inkdo01)
**	    Additions/deletions for partition compatible join logic.
**	29-apr-04 (inkdo01)
**	    A few more for combined partition compatible joins/partition
**	    qualification.
**	22-Jul-2005 (schka24)
**	    Add a work bitmap row index for multi-dimensional qualification.
**	16-Jun-2006 (kschendel)
**	    Factor out partition qualification stuff so that we can
**	    qualify against multiple dimensions.
**	20-Jul-2007 (kschendel) SIR 122513
**	    Reduce the part-info to just DMR/DMT and pcjoin-group stuff,
**	    everything else is in the new qualification struct.
**	9-Jul-2008 (kschendel) SIR 122513
**	    Add a couple K/T-join bitmaps.
*/

struct	_QEN_PART_INFO
{
    DB_PART_DIM	*part_pcdim;	/* ptr to dimension descriptor to 
				** generate partitions for partition
				** compatible join */
    i4		part_gcount;	/* count of log. partitions in a "group"
				** (before end of partition is returned) */
    i4		part_totparts;	/* number of physical partitions in table */
    i4		part_groupmap_ix;  /* DSH row of PC-join grouping bitmap */
    i4		part_ktconst_ix;  /* DSH row of K/T-join constant+higher
				** join physical partition bitmap */
    i4		part_knokey_ix;	/* DSH row of partitions not containing the
				** current outer key, for K-joins only */
    i4		part_dmrix;	/* index to dsh_cbs entry for 1st 
				** partition DMR_CB */
    i4		part_dmtix;	/* index to dsh_cbs entry for 1st 
				** partition DMT_CB */
    i4		part_threads;	/* count of threads accessing partitions
				** (for || query plans), or 1 */
};

/*}
** Name: QEN_SJOIN  - Join info for QE_FSMJOIN, QE_ISJOIN, and QE_CPJOIN nodes 
**									       
** Description:								       
**	This structure is used for the nodes QE_FSMJOIN (full sort merge),     
**	QE_ISJOIN (inner sort join), and QE_CPJOIN (cart-prod join).	       
**	The full sort merge join assumes that the data coming from both	       
**	the inner and outer nodes is sorted. The inner sort join assumes       
**	that the data from the inner node is sorted (not that the inner node   
**	is a sort node), and that the outer is not sorted. Finally, the	       
**	cart-prod join assumes that the data from neither the inner nor the    
**	outer is sorted. These assumptions about the sortedness of data	       
**	are used to minimize the number of tuple fetches and comparisons.      
**									       
**	The QE_ISJOIN and QE_CPJOIN nodes will need to rescan the data from    
**	the inner node. Sometimes it is more efficient to store the inner data 
**	in a hold file and rescan this data. Other times is is more efficient  
**	to simply rescan the inner node directly. The optimizer will make      
**	this decision, and indicate the decision to QEF through the sjn_hget   
**	and sjn_hcreate fields. When RIGHT, or FULL joins are required, then   
**	hold files will always be provided, since they are required for	       
**	processing.							       
**									       
**	The QE_FSMJOIN will never rescan the inner data from the beginning.    
**	Instead, a full sort merge will, at times, rescan small portions of    
**	of the inner data. Since the inner node is only able to reset itself   
**	to the beginning of it's data, a hold file will always be provided     
**	for a full sort merge.						       
**									       
** History:								       
**	14-July-89 (eric)						       
**	    created from the old QEN_JOIN.				       
**	25-mar-92 (rickh)
**	    Removed sjn_hcreate and sjn_hget, which weren't being used.
**      01-nov-95 (ramra01)
**          Join node optimization. Prepare noadf_mask to indicate
**          presence of adf cbs for request codes, actually the
**          lack of adf cbs.
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF and QEN_OJINFO structure instances by pointers.
*/									       
struct _QEN_SJOIN						       
{									       
    QEN_NODE   *sjn_out;       /* outer node */				       
    QEN_NODE   *sjn_inner;     /* inner node */				       
									       
	/*								       
	** sjn_hfile is an index into the DSH_HOLD array for the hold	       
	** file structure.
	*/								       
    i4         sjn_hfile;						       
									       
    i4		sjn_krow;	/* DSH buffer for key value comparison
				*/
    bool        sjn_kuniq;      /* is the inner relation uniquely keyed	       
				** or do we only want one value from inner     
				** (existence only) TRUE-FALSE		       
				*/					       
    QEN_ADF    *sjn_itmat;	/* materialize the inner tuple */	       
    QEN_ADF    *sjn_okmat;	/* materialize the join key into the	       
				** a row buffer				       
				*/					       
    QEN_ADF    *sjn_okcompare;	/* compare current key created by okmat	       
				** to the next key in sjn_outer.	       
				*/					       
    QEN_ADF    *sjn_joinkey;	/* qualify tuples on the join key */	       
    QEN_ADF    *sjn_jqual;	/* qualify the joined tuple */		       
    QEN_OJINFO *sjn_oj;		/* "outer" join information */		       

};									       

/*}
** Name: QEN_KJOIN - keyed lookup on inner node
**
** Description:
**      The outer node generates a set of attributes that can
**  be used to key the inner relation. For each outer tuple, the
**  inner relation is accessed for tuples that match the joining attributes
**  and satisfy applicable qualifications.
**
**	In the keyed join, the outer tuple is set by calling the proper
**  function on the kjoin_out node. Kjoin_key produces the attributes in the
**  join that can facilitate access of the inner relation. These attributes
**  are materialized into a row buffer stored in a row buffer, the row
**  buffer number is the one used in the output slot of the ADE param in
**  kjoin_key. This row buffer is also used by kjoin_get. Once the keys
**  are materialized, kjoin_get is used to access the inner relation and
**  generate joined tuples. kjoin_jqual is used to further qualify the tuple.
**  
**  Kjoin_kys specifies the keys (attribute number, operator (<,>,=,etc),
**  and offset within key row buffer. 
**
**  Kjoin_kuniq is used to limit scans of the inner relation. If this field
**  is set (TRUE), then for a unique outer, only one inner tuple will be
**  found. Thus, kuniq should only be set if doing an equality join.
**  Kjoin_kcompare is used to compare the current key values to the
**  previous key values. This is needed so that we can tell if the next join
**  value is the same as the current join value.
**
**  NOTE on qualifying tuple. If a tid is in the qualification, coming from
**  the inner relation, kjoin_tqual must be set, else cleared. If it is set,
**  kjoin will qualify the tuple, else DMF will qualify the tuple. In the latter
**  case, the tuple begin qualified will be a QEN_DMF_ALLOC in the 
**  qualification's QEN_ADF struct. As such, the qen_input field of the
**  qualification's QEN_ADF struct will need to be initialized with the
**  base number of the QEN_DMF_ALLOC row.
**
** History:
**     28-mar-86 (daved)
**          written
**      01-nov-95 (ramra01)
**          Join node optimization. Prepare noadf_mask to indicate
**          presence of adf cbs for request codes, actually the
**          lack of adf cbs.
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF and QEN_OJINFO structure instances by pointers.
**	12-jun-96 (inkdo01)
**	    Added kjoin_nloff to locate null indicator of  nullable outer.
**	12-nov-98 (inkdo01)
**	    Added kjoin_ixonly.
**	13-apr-04 (inkdo01)
**	    Added kjoin_pcompat for partitioning.
**	21-jan-05 (inkdo01)
**	    Added kjoin_find_only.
*/
struct _QEN_KJOIN
{
    QEN_NODE   *kjoin_out;      /* outer node */
    QEN_ADF    *kjoin_key;	/* produce the join key */
    QEN_ADF    *kjoin_kcompare;	/* compare current join value to previous */
    QEN_ADF    *kjoin_kqual;	/* high level outer-inner compare */
    QEN_ADF    *kjoin_jqual;	/* qualify joined rows */
    QEN_ADF    *kjoin_iqual;	/* qualify inner tuple only */
    QEF_KEY    *kjoin_kys;      /* description of keys */
    QEN_OJINFO *kjoin_oj;	/* "outer" join information */		       
    QEN_PART_INFO *kjoin_part;	/* partitioning info (if target is
				** partitioned table) */
    QEN_PART_QUAL *kjoin_pqual;	/* Partition qualification for partitioned
				** inner */
    i4         kjoin_get;      /* Index into DSH->DSH_CBS to find DMR_CB.*/
    i2		kjoin_nloff;	/* offset in key buffer of null indicator
				** (if outer is nullable) - Rtree only */
    bool        kjoin_kuniq;    /* is the inner relation uniquely keyed
				** or do we only want one value from inner
				** (existence only) (TRUE-FALSE).
				*/
    bool	kjoin_ixonly;	/* TRUE if access is to index leaf nodes of
				** inner table base table (Btree), because all
				** needed cols are index keys.
				*/
    bool	kjoin_pcompat;	/* TRUE if join partition definition and
				** key structure are identical; i.e., if each 
				** group of outers map to exactly one 
				** partition */
    bool	kjoin_find_only; /* TRUE if row doesn't need to be materialized
				** for QEF (semijoin) */
    i4		kjoin_tid;	/* Offset of location to place tid if needed.
				** else, set to -1.
				*/
    i4		kjoin_tqual;	/* set to true if a tid is used to qualify
				** a tuple in the inner relation. QEF must
				** perform the qualification itself because
				** QEF will put the tid into place.
				*/
    i4		kjoin_krow;	/* DSH row buffer to hold key for compares
				*/

};

/*}
** Name: QEN_HJOIN  - Join info for QE_HASHJOIN node
**									       
** Description:								       
**	This structure is used for the node QE_HASHJOIN. Hash joins make 
**	no assumption concerning input source ordering. Instead, they 
**	build a hash table from the outer join source and probe it with
**	rows of the inner source (using the same hash function on the 
**	join columns of each source) looking for matching rows (i.e. join
**	candidates). 
**									       
** History:								       
**	26-feb-99 (inkdo01)
**	    Created (cloned) from QEN_SJOIN.
**	20-dec-99 (inkdo01)
**	    Dropped a few superfluous fields and added hjoin_bmem.
**	8-feb-00 (inkdo01)
**	    Tidied up header.
**	7-Dec-2005 (kschendel)
**	    Replace attr descriptors with DB_CMP_LIST entries.
**	10-Apr-2007 (kschendel) SIR 122512
**	    Add flag to control read-from-probe heuristic.
*/
struct _QEN_HJOIN
{
    QEN_NODE   *hjn_out;        /* outer node */
    QEN_NODE   *hjn_inner;      /* inner node */
    QEN_ADF    *hjn_btmat;	/* materialize the build (outer) tuple */
    QEN_ADF    *hjn_ptmat;	/* materialize the probe (inner) tuple */
    QEN_ADF    *hjn_jqual;	/* qualify the joined tuple */
    QEN_OJINFO *hjn_oj;		/* "outer" join information */
    DB_CMP_LIST *hjn_cmplist;	/* Join column key descriptors.  Note:
				** If all keys are non-nullable, opc reduces
				** this to a single BYTE(n) key of the
				** appropriate length.
				*/
    QEN_PART_QUAL *hjn_pqual;	/* Pointer to list of partition qualifications
				** that can run at this hash-join.
				*/
    i4		hjn_hkcount;	/* count of key descriptors */

    i4		hjn_brow;	/* buffer into which build rows materialize */
    i4		hjn_prow;	/* buffer into which probe rows materialize */
    i4		hjn_bmem;	/* estimated memory for whole build source
				** (est. tuples * build row size) */
    i4  	hjn_hash;	/* is the index into the dsh_hash array
				** of the hash structure entry for this join
				*/

    i4		hjn_dmhcb;	/* index into dsh_cbs of DMH_CB for hash
				** partition file operations
				*/

    i2		hjn_seltype;	/* select node type. See QEN_SEJOIN */

    bool	hjn_kuniq;	/* existence only hash join */
    bool	hjn_nullablekey;  /* TRUE if any key is nullable */
    bool	hjn_innercheck;	/* TRUE if read-inner-first heuristic for
				** detecting an empty inner is allowed */

				/* If we get many more bools make them flags */
};									       

/*}
** Name: QEN_TJOIN - Perform a tid lookup into the inner node
**
** Description:
**   An outer row value is positioned by calling the function that
**  will produce the outer row. The node CB for the outer tuple is in
**  tjoin_out. Tjoin_get is an index into the DSH->DSH_CBS that refers
**  to a positioned DMR_CB. tjoin_key contains the structures necessary
**  to materialize the tid into the DMR_CB in tjoin_get so that the
**  inner row can be fetched. tjoin_jqual is used to qualify
**  the joined rows. Rows are retrieved from the outer and inner until
**  a qualified row is found.
**	
**
** History:
**     28-mar-86 (daved)
**          written
**	oct-90 (stec)
**	    introduced tjoin_coerce, tjoin_otidval, tjoin_crow, tjoin_coffset
**	22-oct-90 (davebf)
**	    removed tjoin_coerce, tjoin_otidval, tjoin_crow, tjoin_coffset
**      01-nov-95 (ramra01)
**          Join node optimization. Prepare noadf_mask to indicate
**          presence of adf cbs for request codes, actually the
**          lack of adf cbs.
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF and QEN_OJINFO structure instances by pointers.
**	3-may-05 (inkdo01)
**	    Added tjoin_flag to house TJOIN_4BYTE_TIDP.
*/
struct _QEN_TJOIN
{
    QEN_NODE	    *tjoin_out;     /* outer node */
    i4		    tjoin_get;      /* Index into DSH->DSH_CBS for the DMR_CB
				    ** pointer.
				    */
    i4		    tjoin_orow;	    /* index into DSH->DSH_ROW for row 
				    ** containing tid for the join
				    ** (from the outer).
				    */
    i4		    tjoin_ooffset;  /* offset in above row of tid. */
    i4		    tjoin_irow;	    /* index into DSH->DSH_ROW for row 
				    ** containing tid for the join
				    ** (from the inner).
				    */
    i4		    tjoin_ioffset;  /* offset in above row of tid. */
    QEN_ADF	    *tjoin_jqual;   /* qualification for join */
    QEN_ADF	    *tjoin_isnull;  /* If not null, points to a CX to be
				    ** executed to determine if the TID from
				    ** the outer (described by tjoin_orow,
				    ** tjoin_ooffset) has a NULL value.
				    */ 		       
    QEN_OJINFO	    *tjoin_oj;	    /* "outer" join information */
    QEN_PART_INFO   *tjoin_part;    /* partitioning info (if target is
				    ** partitioned table) */
    QEN_PART_QUAL   *tjoin_pqual;   /* Partition qualification to apply to
				    ** T-join inner.  FOR NOW, ALWAYS NULL;
				    ** see opc_tjoin_build for why.
				    */
    i4		    tjoin_flag;	    /* flag mask */
#define	    TJOIN_4BYTE_TIDP	0x0001	/* TID is 4 bytes (probably a tidp
				    ** column from an index on a non-
				    ** partitioned table) - else 8 bytes */

};

/*}
** Name: QEN_SEJOIN  - select join node
**
** Description:
**	This join node is used when performing a subselect join. The inner
**  node must always be a QEN_QP node. When called, this node will get
**  an outer row and try to satisfy the qualification according to the
**  semantics of the select type {all or blank}. If all, then the inner
**  will be scanned and must reach EOF to succeed. If blank, then, if another
**  inner can be found, an error condition exists.
**
**  QEN_JUNC (junction type)
**	QESE1_CONT_AND
**	    The inner AND outer values must be TRUE before returning success.
**	    If either the inner or the outer fail then CONTinue processing
**	    with the next outer until there are no more outer rows.
**
**	QESE2_CONT_OR
**	    Like above, the inner OR outer values must be true before 
**	    returning success.  If the outer is TRUE, return success.  If
**	    the outer value fails but the inner is TRUE, return success.
**	    If both the outer and inner fail, the CONTinue processing with
**	    the next outer unless a ZERO_ONE error was generated.  In this 
**	    case, all processing should stop.
**
**	QESE3_RETURN_OR
**	    The inner OR outer values must be TRUE before returning success.
**	    If the outer is TRUE, return success.  If the outer fails but the
**	    inner value is TRUE, return success.  If both the outer and the
**	    inner fail, then return the worse error.  Notice: ZERO_ONE error
**	    is worse than NOT_ALL.
**
**	QESE4_RETURN_AND
**	    Like above, both the inner AND outer values must be TRUE before
**	    returning sucess.  However, if either the inner or the outer
**	    fails then return the error instead of continuing with the
**	    next outer.
**
**	    (per Eric memo - puree).
**
** History:
**     29-jan-87 (daved)
**          written
**     25-jan-88 (puree)
**	    redefine literals for sejn_junc.
**	17-dec-90 (stec)
**	    Modified type and meaning of sejn_hcreate in QEN_SEJOIN struct.
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF structure instances by pointers.
**	12-Dec-2005 (kschendel)
**	    Replaced remaining QEN_ADF instances by pointers.
**	4-Jun-2009 (kschendel) b122118
**	    Move things around a little for less padding.
**	10-Sep-2010 (kschendel) b124341
**	    Delete kcompare, add cvmat.  okmat is now a combination materialize
**	    and compare CX.
*/
struct _QEN_SEJOIN
{
    QEN_NODE   *sejn_out;       /* outer node */
    QEN_NODE   *sejn_inner;     /* inner node */
    i4		sejn_hget;	/* Index into DSH->DSH_CB for the DMR_CB
				** to get/put tuple to hold file.
				*/
    i4          sejn_hfile;     /* hold file number - index into DSH->DSH_HOLD
				** structure. This number may be identical in
				** more than one SEjoin node, which indicates
				** that the hold file is to be shared. See
				** description of sejn_hcreate.
				*/
    QEN_ADF	*sejn_itmat;	/* materialize the inner tuple */
    QEN_ADF	*sejn_okmat;	/* materialize the join key into a row buffer.
				** Also compares previous and current keys.
				** INIT segment is materialize, MAIN is compare
				*/
    QEN_ADF	*sejn_cvmat;	/* Materialize outer correlation values into
				** the row buffer.
				*/
    QEN_ADF	*sejn_ccompare; /* compare correlation key created by cvmat
				** to the next key in sejn_outer.
				*/
    QEN_ADF	*sejn_kqual;	/* qualify tuples on the join key. that's
				** all there can be.
				*/
    QEN_ADF	*sejn_oqual;	/* qualify the outer tuple */
    i2		unused1;	/* sejn_seltype move to qen_flags SELTYPE */

    i2		sejn_junc;	/* AND/OR processing semantics */
#define QESE1_CONT_AND	    1
#define QESE2_CONT_OR	    2
#define QESE3_RETURN_OR	    3 	
#define QESE4_RETURN_AND    4
/* fix me */
#define QESE1_CHECK_AND	    QESE1_CONT_AND
#define QESE2_RETURN_AND    QESE2_CONT_OR
#define QESE4_CHECK_OR	    QESE4_RETURN_AND
/* */
    i2		sejn_oper;	/* operator type */
#define SEL_LESS	    -1	/* <,<= */
#define SEL_EQUAL	    0	/* = */
#define SEL_GREATER	    1	/* >, >= */
#define SEL_OTHER	    2	/* any other operator: !=, ...	*/
    bool	sejn_hcreate;	/* A flag indicating if a hold file is to be
				** created by this node. Normally will be set to
				** TRUE. In certain cases, when inner QP trees
				** are shared by SEjoin nodes (and so is the 
				** corresponding hold file), all SEjoin nodes
				** concerned will have this flag set to FALSE
				** with the exception of the node that will be
				** executed first.
				*/ 
};

/*}
** Name: QEN_SORT - load the sort table
**
** Description:
**	Read rows into a temporary sort table. This node is used if its
**  parent is a QEN_SIJOIN and this node is the inner relation.
**  This node creates, opens and loads the sort relation. At a future time,
**  the table could be created with a separate action and opened in the
**  validation list.
**  
**
** History:
**     1-apr-86 (daved)
**          written
**     26-sep-88 (puree)
**	    Modified for QEF in-memory sorter.  Added sort_shd to identify
**	    the sort node number.
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF structure instance by pointer.
**     06-mar-96 (nanpr01)
**	    Add page size parameter to pick up the right size page for
**	    sorting if DMF sort is used.
**	2-Dec-2005 (kschendel)
**	    Add compare-list pointer-array pointer.
[@history_line@]...
*/
struct _QEN_SORT
{
    QEN_NODE       *sort_out;   /* outer node */
    i4              sort_create;/* Index into DSH->DSH_CBS to find DMT_CB to
				** create sort temp.
				*/
    i4              sort_load;  /* Index into DSH->DSH_CBS to find DMR_CB to
				** load rows. The row used in this CB is found
				** in the qen_output field of the sort_mat
				** QEN_ADF struct.
				*/
    QEN_ADF	   *sort_mat;	/* materialize input tuple */
    /* elements used to initialize runtime environment */
    i4		    sort_dups;	/* DMR_NODUPLICATES if duplicates should 
				** be removed. Else, set to zero.
				*/
    i4		    sort_acount;/* number of attributes in created table */
    DMF_ATTR_ENTRY  **sort_atts;/* attributes to create in table */
    i4		    sort_sacount;/* number of attributes to sort on */
    DMT_KEY_ENTRY   **sort_satts;/* attributes to sort on */
    DB_CMP_LIST	    *sort_cmplist;  /* Compare-list array for sort keys */
    i4		    sort_shd;	/* index into dsh_shd to find the sort buffer
				** descriptor for this node */
    i4	    sort_pagesize;/* Sort page size     */
};



/*}
** Name: QEN_TSORT - top sort node
**
** Description:
**   The same as the QEN_SORT node except that subsequent calls to this
**  node return sorted tuples. That is, on the first call the sort table
**  is loaded. The first sorted tuple is then returned. On subsequent calls
**  the other sorted tuples are returned. This node is used as an outer
**  node to a join node or as the top sort node in a QEP.  Pretty much
**  anywhere that sorting is needed, other than the inside of an FSMJOIN.
**
** History:
**     1-apr-86 (daved)
**          written
**     26-sep-88 (puree)
**	    Modified for QEF in-memory sorter.  Added tsort_shd to identify
**	    the sort node number.
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF structure instance by pointer.
**     06-mar-96 (nanpr01)
**	    Add page size parameter to pick up the right size page for
**	    sorting if DMF sort is used.
**	2-Dec-2005 (kschendel)
**	    Add compare-list pointer-array pointer.
[@history_line@]...
*/
struct _QEN_TSORT
{
    QEN_NODE           *tsort_out;      /* outer node */
    QEN_PART_QUAL	*tsort_pqual;	/* Partition qualifications to do
					** as rows enter this sort.  Only
					** used for FSM-join outers when an
					** inner can be partition-qualified.
					** otherwise, null.
					*/
    i4                  tsort_create;   /* Index into DSH->DSH_CBS to find
					** DMT_CB to create sort table
					*/
    i4                  tsort_load;     /* Index into DSH->DSH_CBS to find
					** DMR_CB to load sort table.
					** The row used in this CB is found
					** in the qen_output field of the 
					** tsort_mat QEN_ADF struct.
					*/
    QEN_ADF	       *tsort_mat;	/* materialize input tuple */
    i4                  tsort_get;      /* Index into DSH->DSH_CBS to find
					** DMR_CB control block for get call
					** to return sorted tuples
					** The row used in this CB is found
					** in the qen_output field of the 
					** tsort_mat QEN_ADF struct.
					*/
    /* elements used to initialize runtime environment */
    i4			tsort_dups;	/* DMR_NODUPLICATES if duplicates 
					** should be removed. Else, set to zero.
					*/
    i4			tsort_acount;	/* number of attributes in created 
					** table 
					*/
    DMF_ATTR_ENTRY	**tsort_atts;	/* attributes to create in table */
    i4			tsort_scount;	/* number of attributes to sort on */
    DMT_KEY_ENTRY	**tsort_satts;	/* attributes to sort on */
    DB_CMP_LIST		*tsort_cmplist;	/* Compare-list array for keys */
    i4			tsort_shd;	/* index to into dsh_shd to find the
					** sort buffer descriptor */
    i4		tsort_pagesize; /* Top Sort page size     */
};


/*}
** Name: QEN_ORIG - The origination node
**
** Description:
**	Zero or more keys are used to access a base relation and 
**  originate tuples into the QEP stream. If there are no keys, the
**  relation is scanned. Keys are used to limit the scan of the relation.
**
**  NOTE: The use of keys can often get tricky. Take for example, r.tid =
**	s.a + 3. If 'r' is the inner, there will be a TJOIN node performing
**	the join. The TJOIN node needs to read the tid (s.a + 3) from the
**	orig row buffer. DMF will currently use the user's row buffer
**	to qualify tuples (because some rows are compressed). In this case,
**	the qualification CX struct should also contain code to produce
**	the tid value. For non-TJOIN types of joins, CX structs exist to
**	produce all function values.
**  
**  Note on qualifying tuples:
**	In retrieves where the tid is not involved in the qualification,
**	use the QEN_DMF_ALLOC field in the QEN_BASE for qualifying the tuple.
**	If a tid is used in the qual, QEN_ORIG will move the tid into the
**	this node's row buffer and qualify the tuple. Thus, the ADF_QUAL
**	should use the QEN_ROW field in the QEN_BASE to mark the incoming
**	row (instead of QEN_DMF_ALLOC). 
**
**  Note, if the tid is used for lookup (a key) qen_ukey must be set.
**
** History:
**     19-mar-86 (daved)
**          written
**	18-dec-90 (kwatts)
**	    Smart Disk integration: Added sd_program and sd_pgmlength.
**	04-dec-1992 (kwatts)
**	    Smart Disk development - changed to one orig_sdinfo pointer.
**      19-jun-95 (inkdo01)
**          Collapsed orig_tkey/ukey nat's into single flag word orig_flag
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF structure instance by pointer.
**	11-feb-97 (inkdo01)
**	    Changes for MS Access OR transformed in-memory constant tables.
**	11-aug-98 (inkdo01)
**	    Changes for descending sort top sort removal.
**	10-nov-98 (inkdo01)
**	    Added ORIG_IXONLY to indicate we only need to access index pages
**	    and NOT base table data pages.
**	3-nov-99 (inkdo01)
**	    Added orig_qualrows to help group read determination.
**	18-dec-03 (inkdo01)
**	    Added various fields, flags for partitioned table handling.
**	21-jan-04 (inkdo01)
**	    Added orig_qpart, orig_pdim for qualification of partitions
**	    in partitioned table (since subsumed into *orig_part).
**	13-jan-05 (inkdo01)
**	    Added ORIG_FIND_ONLY flag.
[@history_line@]...
*/
struct _QEN_ORIG
{
    QEF_KEY    *orig_pkeys;     /* list of possible keys for this node. set
				** to zero if not used.
				** This is the reference (readonly) list.
				 */
    QEN_ADF	*orig_qual;	/* qualify tuples */
    QEN_PART_INFO *orig_part;	/* partitioning info (if target is
				** partitioned table) */
    QEN_PART_QUAL *orig_pqual;	/* Partition qualification (just one) for
				** this orig, if partitioned table. */

    i4		orig_keys;	/* control block number of runtime key info.
				** dsh_cbs[orig_keys] contains pointer to
				** QEN_TKEY. Set to 0 if not used.
				** if pkeys is set, this value must be valid.
				*/
    i4          orig_get;       /* Index into DSH->DSH_CBS for DMR_CB used
				** to get rows from the base relation
				*/
    i4          orig_tid;       /* set if tid needs to be returned in 
                                ** row buffer. The value here is the offset
				** into the row buffer where the tid should
				** be placed and will be refered to in any
				** qualification. Set to -1 if tid not needed
                                */
    i4          orig_flag;      /* flag word
                                */
#define ORIG_TKEY    0x01	/* TRUE if tids are in the qualification. */
#define ORIG_UKEY    0x02	/* unique key relation - ie we are using
				** unique keys (TIDS or full keys). TRUE-FALSE
				** This should NOT be set if doing a range
				** lookup (even if keys are unique). If this
				** field is set, only one attempt to find
				** a record will be made for each key.
				*/
#define ORIG_MAXOPT  0x04       /* TRUE if optimizeable max being executed
				*/           
#define ORIG_MINOPT  0x08       /* TRUE if optimizeable min being executed
				*/           
#define ORIG_RTREE   0x10	/* TRUE if this is Rtree retrieval
				*/           
#define ORIG_MCNSTTAB 0x20	/* TRUE if this ORIG corresponds to a
				** MS Acess OR transformed in memory constant
				** table (orig_get is then index to table addr)
				*/
#define ORIG_READBACK 0x40	/* TRUE if we're supposed to read backwards
				** through this structure (better be Btree!).
				** It implements topsort-less descending sort. 
				*/
#define ORIG_IXONLY   0x80	/* TRUE if we can get by only reading base table 
				** index pages and NOT data pages. Index covers
				** all cols ref'ed in this table */
#define	ORIG_FIND_ONLY	0x100	/* TRUE if rows of the table don't actually
				** need to be materialized for QEF */
#define	ORIG_MAXMIN	0x200	/* TRUE if max/min may result in aggregate
				** optimization - NOT key columns */
    i4          orig_ordcnt;    /* For READBACK nodes, count of elements in
				** corresponding ORDER BY clause - used to 
				** create correct DMF position calls.
				*/
    i4		orig_qualrows;	/* estimated count of rows qualified by
				** keycolumn predicates, used to predict
				** utility of readahead. -1 if no key
				** selectivity, or if selectivity based on
				** default histograms.
				*/
};

/*}
** Name: QEN_QP - get a row from another query plan
**
** Description:
**      This node makes a recursive call to the query
**  plan executor to retrieve rows from an associated
**  query plan. This is most often used to compute unions,
**  where the associated QP has a list of action items,
**  each a fetch from a QEP.
**
**  Currently, the results of this node (like an orig node)
**  are viewed as a heap relation. That is, the associated
**  QP will not except parameters that differ from those
**  in effect at the time QEF was called. The use of this
**  node should buffer the results if they are to be viewed
**  more than once (ie use a hold file).
**
**  This 'query plan' uses the same DSH as the calling query plan; thus,
**  the QP header (QEF_QP_CB) is largely a place keeper. In order to
**  conserve space, this node will point to a list of actions (QEF_AHD).
**
** History:
**     22-apr-86 (daved)
**          written
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF structure instance by pointer.
**	8-mar-04 (inkdo01)
**	    Added flag to indicate QP as owner of || union.
[@history_line@]...
*/
struct _QEN_QP
{
    QEF_AHD		*qp_act;	    /* associated query plan */
    QEN_ADF		*qp_qual;	    /* qualify tuples */
    i4			qp_flag;	    /* flag field */
#define	QP_PARALLEL_UNION 0x01		    /* TRUE if QP owns union
					    ** under exchange node */
};

/*}
** Name: QEN_EXCH - exchange buffers between different threads executing
**	the same query plan
**
** Description:
**	Generate one or more threads to execute plan fragment beneath this
**	node and use exchange buffer concept to pass retrieved row images
**	back to thread executing plan fragment above this node. Additional
**	semantics (union, merge, dups removal, ...) may be applied
**	depending on query requirements. This node implements parallel
**	query processing, both inter-node and intra-node, in Ingres.
**
** History:
**	7-nov-03 (inkdo01)
**          Written.
**	8-feb-04 (toumi01)
**	    Add exch_exchcb.
**	26-feb-04 (inkdo01)
**	    Added bit map to track structures to be allocated for 
**	    exchange children.
**	13-may-04 (toumi01)
**	    Add exch_tot_cnt = total bits in DSH child copy map.
**	19-may-04 (inkdo01)
**	    Add exch_key_cnt for QEN_TKEY structures.
**	21-july-04 (inkdo01)
**	    Change bit map to pair of arrays of DSH ptr array indexes.
**	19-May-2010 (kschendel) b123759
**	    Add array entry for PQUAL indexes.
*/
struct _QEN_EXCH
{
    QEN_NODE	*exch_out;	/* outer node */
    QEN_ADF	*exch_mat;	/* materialize result row into exch buffer */
    i4		exch_ccount;	/* child thread count */
    i4		exch_flag;	/* flag indicating node specifics */
#define	EXCH_NCHILD	0x01	/* TRUE subtree under exchnage is executed
				** by more than one thread (could mean
				** a higher parent exch is 1:n). Used for 
				** child DSH formatting */
#define	EXCH_UNION	0x02	/* TRUE if subtree is QP node atop a set
				** of union'ed selects */
    i4		exch_dups;	/* DMR_NODUPLICATES if duplicates should 
				** be removed. Else, set to zero.
				*/
    i4		exch_exchcb;	/* index into dsh_cbs of EXCH_CB to hold
				** node context across calls
				*/
    i4		exch_acount;	/* number of attributes in result row */
    DMF_ATTR_ENTRY **exch_atts;	/* attributes in result row */
    i4		exch_macount;	/* number of attributes to merge on */
    DMT_KEY_ENTRY **exch_matts;	/* attributes to merge on */

/* Child threads created from an exchange node require their own DSH
** structures to assure thread safety of || query execution. The count
** values and arrays that follow are designed to allow the minimal number
** of buffers and control structures to be allocated with the DSH for 
** each child thread. The arrays are divided into subarrays containing 
** indexes within the various DSH pointer arrays for the buffers and control 
** blocks to be allocated. The count values show the number of entries in each
** subarray and, when added up, the start of each successive subarray.
** A i4 array is used for the indexes into dsh_cbs because it is anticipated
** that the number of dsh_cbs entries may be quite large for complex queries
** involving partitioned tables.
**
** Finally, there is an array of the count and array information in the 
** event of a parallel union, in which each select in the union has its own
** set of counts and indexes.
**
** The index array scheme replaces an earlier attempt to use a subdivided
** bit map. The bit map used excessive amounts of storage due to its
** sparseness and was dropped in favour of the current arrays.
*/
    struct _EXCH_IX {
	i2	exch_row_cnt;	/* number of array entries for row buffers */
	i2	exch_hsh_cnt;	/* number of array entries for hash structs */
	i2	exch_hsha_cnt;	/* number of array entries for hash agg structs */
	i2	exch_stat_cnt;	/* number of array entries for node statuses */
	i2	exch_ttab_cnt;	/* number of array entries for temp table strs */
	i2	exch_hld_cnt;	/* number of array entries for hold structs */
	i2	exch_shd_cnt;	/* number of array entries for SHD structs */
	i2	exch_pqual_cnt;	/* Number of array entries for PQUALs */
	i2	exch_cx_cnt;	/* number of array entries for ADE_EXCBs */
	i2	exch_dmr_cnt;	/* number of array entries for DMR_CBs */
	i2	exch_dmt_cnt;	/* number of array entries for DMT_CBs */
	i2	exch_dmh_cnt;	/* number of array entries for DMH_CBs */
	i2	*exch_array1;	/* Array of i2 indexes for row ptr, hash, hash
				** aggregate, node status, temp table, hold,
				** SHD, and PQUAL arrays */
	i4	*exch_array2;	/* array of i4 indexes for ADE_EXCB, DMR_CB, 
				** DMT_CB and DMH_CB entries in dsh_cbs array */
    } exch_ixes[1];
};


/*}
** Name: QEN_TPROC - call row producing procedure (table procedure) from
**	within query plan
**
** Description:
**	Issue call to database procedure (must be readonly, row producing
**	procedure) from within the query tree of a query plan. Result rows
**	of procedure are returned similar to any other row source (table,
**	index) and are available for access in other parts of the query
**	plan. 
**
** History:
**	5-may-2008 (dougi)
**          Written.
**      09-Sep-2008 (gefei01)
**          Add tproc_qpix;
**	8-dec-2008 (dougi)
**	    Replaced tproc_qpix with tproc_qpidptr.
**	17-Jun-2010 (kschendel) b123775
**	    Put qpidptr back to an index (resource entry index).  We'll
**	    store the translated cursor ID in the DSH resource entry,
**	    not the QP resource entry.  (Probably can get away with doing
**	    it in the QP entry, even though it's supposed to be readonly,
**	    but it's just as easy and less worry in the dsh side.)
*/

struct _QEN_TPROC
{
    DB_TAB_ID		tproc_dbpID;		/* ID of procedure  */

    QEN_ADF		*tproc_parambuild;	/* CX to build actual parms */
    QEN_ADF		*tproc_qual;	    /* qualify tuples */
    struct _QEF_CP_PARAM *tproc_params;		/* Actual parameter list */
    i4			tproc_pcount;		/* Actual parameter count. */

    i4			tproc_dshix;		/* CBs offset of proc DSH ptr */
    i4			tproc_resix;		/* Resource entry index, for
						** storing cursor ID */
    i4			tproc_flag;	    /* flag field */
};


/*}
** Name: QEF_KEY - describe the key structure
**
** Description:
**
**  The QEF_KEY structure and its subordinates describe keying into some
**  keyed relation.  It is designed to deal with various situations like
**  partial keys, multiple keys, multi-part keys, and parameterized
**  queries (repeat or DBP parameters as key values).  Ultimately, the
**  keying structures result in <attribute,value,positioning operator>
**  elements being passed to DMF in a DMR_CB control block.
**      
**      A key for a node is a moderately complicated structure. A key can
**  be one of four types, equality, minimum, maximum or range.
**  For example: r.a = 65, r.a < 65, r.a > 65, or (r.a >65 and r.a < 75).
**  There can also be multiple values for any key (r.a=65 or r.a=75) or
**  (r.a < 65 and r.a < 75).  While OPF goes to considerable lengths to
**  analyze the keying and eliminate dups and overlaps, there may still be
**  situations where the proper key values and key ordering are left to
**  be decided at runtime.
**
**  Most if not all of the discussion that follows applies to keying for
**  ORIG nodes.  K-join nodes use QEF_KEY structures, but in a minimal
**  way:  only the equality conditions are described, and the only thing
**  that QEF does with K-join keying info is use it to allocate DMF
**  position-bound descriptors (DMR_ATTR_ENTRY's).
**
**  Depending on circumstances, given a one-sided inequality such as
**  r.a > 10, the optimizer may decide to compile a range with one end
**  at infinity.  (r.a >= 10 AND r.a <= +infinity).  LIKE pattern
**  matching with a fixed prefix (str LIKE 'tw%') can also end up
**  looking like a range key.
**
**  The key structure has the following appearance:
**
**	QEF_KEY (top of structure)
**	    |
**	QEF_KOR (first set of keys) - QEF_KAND (first keyed att) - QEF_KAND
**	    |			     (min, max, or eq)
**	    |				|
**	QEF_KOR			      QEF_KATT (attribute desc)
**	    |				|
**	    |			      QEF_KATT
**	QEF_KOR (third set of keys)
**
**  Thus the key structure is in "disjunctive normal form", OR's of AND's.
**  
**  The QEF_KEY represents the set of keys for one orig or join node,
**  and is basically the header.  It does very little and we could probably
**  get rid of it if we tried.
**
**  Each QEF_KOR is in essence one key lookup, ie a key value or range.
**  It points to the key elements (KANDs) making up that range, and
**  points to the next key (KOR).  OPF makes sure that each QEF_KOR
**  describes a disjoint key range.  If it can't (due perhaps to
**  parameterization), OPF will be conservative, to the point of generating
**  only one KOR with the widest possible bounds in some cases.
**
**  Each QEF_KAND node represents some key value or range bound for one
**  attribute.  There may be multiple KAND's for one lookup (KOR) in
**  a couple situations:
**  - multi-column keys, there will be (at least) one KAND per attribute;
**  - range keys, such as (i>=1 and i<=5), when there is one KAND for the
**    lower limit and another KAND (for the same attribute) for the upper
**    limit.
**
**  There are never multiple equality KAND's for the same attribute,
**  under the same KOR.  Such a thing would represent a key clause
**  like (k=2 and k=4) which is nonsense.  If the user query actually
**  does such a thing in a context where the optimizer can't just turn it
**  into FALSE (e.g. when the values are repeat query constants, or DBP
**  variables), we still only do one equality KOR, and let the orig
**  qualification CX figure the truth out by applying both tests.
**
**  You might imagine that the KAND would indicate the column and the
**  keying value, but it's not that simple.  The query might have multiple
**  range or half-range non-constant values:
**    k>=:a1 AND k<=:a2 AND k>=:a3 AND k<=:a4
**  which may be a result of:
**    (k BETWEEN :a1 AND :a2) OR (k BETWEEN :a3 AND :a4)
**  remember, if OPF can't guarantee disjoint ranges, it falls back on
**  conservative keying.  To this end, each KAND points to one or
**  more KATT entries which represent the actual keying values.  The
**  KAND has a type indicator that says how to winnow the KATT values:
**  find the smallest lower bound (>= operator), or the largest upper
**  bound (<= operator), or EQ for just-a-single-value.
**  FIXME it would actually make a lot more sense for OPF to simply
**  generate the disjuncts as if everything were constant and disjoint,
**  and let an improved version of qeq_ksort figure it all out at query
**  startup.
**
**  You might also imagine that the KAND would contain the DMR positioning
**  operator, but it doesn't;  those are in the KATT's as well.  This
**  oddity probably happened because the DMR_CB wants the positioning
**  operator in the DMR_ATTR_ENTRY's that describe a key to DMF, and
**  someone picked on the name similarity.  Whatever.
**
**  To take a couple of examples:
**  (assuming a 2-column btree key <i,str>)
**	select * from t
**	where ((i between 1 and 3 or i=6) and str<'lll')
**	   or (i between 15 and 20 and str='twenty')
**
**  produces:
**  KEY -> KOR -> KAND[i,MIN] -> KATT[i,GE,value 1]
**         |      KAND[i,MAX] -> KATT[i,LE,value 3]
**         |      KAND[str,MIN] -> KATT[str,GE, value '']
**         |      KAND[str,MAX] -> KATT[str,LE, value 'lll']
**         V
**         KOR -> KAND[i,EQ] -> KATT[i,EQ, value 6]
**         |      KAND[str,MIN] -> KATT[str,GE, value '']
**         |      KAND[str,MAX] -> KATT[str,LE, value 'lll']
**         V
**         KOR -> KAND[i,MIN] -> KATT[i,GE, value 15]
**                KAND[i,MAX] -> KATT[i,LE, value 20]
**                KAND[str,EQ] -> KATT[str,EQ, value 'twenty']
**
**  Note a few things here:  the OR inside the first boolean factor has
**  been broken out so that there are three keys;  and the str<'lll'
**  condition has been turned into the range (str>='' and str<='lll').
**  The key range is wider than the exact range, which is OK.
**  The orig qualification CX will apply the exact str<'lll' test.
**
**  Another example:
**	repeated select * from t
**	where i < :val1 and i < :val2;
**
**  produces:
**  KEY -> KOR -> KAND[i,MAX] -> KATT[i,LE, value val1]
**                               KATT[i,LE, value val2]
**
**  which is an example of when the runtime has to examine a list of KATT
**  values and choose one.  Unfortunately, this example falls afoul of
**  the conservatism rules mentioned above, and it ends up picking the
**  worst (largest) upper bound instead of the best.  Oh well!
**
**  And one more:
**	repeated select * from t
**	where (i between :a1 and :a2) or (i between :a3 and :a4)
**
**  produces:
**  KEY -> KOR -> KAND[i,MIN] -> KATT[i,GE, value a1]
**                |              KATT[i,GE, value a3]
**                V
**                KAND[i,MAX] -> KATT[i,LE, value a2]
**                               KATT[i,LE, value a4]
**
**  and this is why we have to choose the worst bounds instead of
**  the best.  (Consider a1=1, a2=3, a3=10, a4=12;  choosing the
**  tightest bounds clearly returns no rows!)
**
**  Keep in mind that the QEF_KEY / KOR / KAND / KATT stuff is part of
**  the QP, and as such are readonly at runtime.  Additional structure
**  is needed to properly sort and winnow the key values at runtime.
**  See the QEN_TKEY / NOR / NKEY stuff below.
**
**  TID's are not considered key columns, at least not normally.
**  Given both tid and key qualification, the QEF_KEY structure will only
**  have one or the other.  If there is nothing but tid qualification,
**  as in "select * from foo where tid=1024", the optimizer may generate
**  a QEF_KEY for the tid comparison, but if so then it's guaranteed that
**  the tid column will be the only attribute referenced anywhere in
**  that key structure.  (One reason for this is that tid lookup needs
**  a position-by-tid call to DMF, and NOT a position-by-key call.
**  DMF would refuse a position-by-key request that included a tid column.)
**
** History:
**     29-jul-86 (daved)
**          re-written
**	19-Apr-2007 (kschendel) b122118
**	    Comment updating as I try to understand better.
[@history_line@]...
*/
struct _QEF_KEY
{
    QEF_KOR	*key_kor;	/* first range of values */
    i4		key_width;	/* maximum number of QEF_KAND nodes off
				** of one QEF_KOR nodes; i.e. the max number
				** of DMR_ATTR_ENTRY's needed to pass the
				** most complex key bounds to DMF.
				*/
};

/*}
** Name: QEF_KOR - key lookup header
**
** Description:
**
**	The QEF_KOR heads one key lookup.  Multiple key alternatives
**	such as an IN clause are represented by multiple KOR's,
**	one linked to the next.
**
**	As part of the key sorting and winnowing, QEF follows the
**	KOR list, but then needs to access the query-specific
**	(and writable) QEN_NOR equivalent.  This is done by indexing
**	a DSH pointer array with the kor_id index.  (It's not allowed
**	to have readonly QP stuff pointing at mutable runtime stuff.)
**
**	Note the misnaming of kor_numatt:  it's a KAND counter, not an
**	attribute counter.
**
** History:
**      29-jul-86 (daved)
**          written
**	20-jul-1993 (pearl)
**          Bug 45912.  Add qef_kor field to QEN_NOR structure to keep track
**          of which QEF_KOR structure it is associated with.
**          Add kor_id field to QEF_KOR structure.
[@history_template@]...
*/
struct _QEF_KOR
{
    QEF_KOR	*kor_next;		/* next range */
    QEF_KAND	*kor_keys;		/* keys for this range */
    i4		kor_numatt;		/* number of KAND's in this key */
    i4		kor_id;			/* KOR index unique to the QP */
};

/*}
** Name: QEF_KAND - descriptions of key value or range bound
**
** Description:
**
**	The QEF_KAND describes one element of a key:  a value or
**	bound for a specific column (attribute).  The KAND gives
**	the attribute number.
**
**	The value or bound itself is not described in the KAND,
**	because there might be more than one (especially in a
**	parameterized-query context, where OPF can't evaluate
**	static values at compile time).  Instead, the KAND points
**	to one or more KATT's that give candidate values for the
**	key bound.  The KAND also specifies a winnowing operator
**	(find the largest [upper] or smallest [lower] bound).
**
** History:
**      29-jul-86 (daved)
**          written
[@history_template@]...
*/
struct _QEF_KAND
{
    QEF_KAND	*kand_next;	/* next key element */
    QEF_KATT	*kand_keys;	/* list of candidate key values */
    i4		kand_type;	/* type of list winnowing to do */
#define QEK_EQ  0               /* equality chain, length one, no winnowing */
#define QEK_MIN 1               /* Use smallest of KATT values */
#define QEK_MAX 2               /* Use largest of KATT values */
    i4	kand_attno;	/* attribute number */
};

/*}
** Name: QEF_KATT - describe a candidate key value
**
** Description:
**      This struct gives the operator and value for a key.
**	In the simple keyed-ORIG case, there's usually only one
**	KATT per KAND.  In a context where the optimizer can't
**	decide at compile time, there might be more than one
**	KATT per KAND, and then the qef runtime has to winnow
**	out the list according to the KAND's kand_type.
**
**	(The KATT list is not touched, of course;  it's readonly.
**	QEF winnows a mirror of the KATT list, see QEN_NKEY below.)
**
** History:
**      29-jul-86 (daved)
**          written
**	27-mar-89 (jrb)
**	    Added .attr_prec field for decimal project.
**	16-dec-04 (inkdo01)
**	    Added .attr_collID for column level collation project.
[@history_template@]...
*/
struct _QEF_KATT
{
    QEF_KATT	*attr_next;		/* next attribute in key list */
    i4		attr_attno;		/* attribute number */
    DB_DT_ID	attr_type;		/* its type - use values in DBMS.h*/
    i4		attr_length;		/* its length */
    i2		attr_prec;		/* its precision and scale */
    i2		attr_collID;		/* its collationID */
    i4		attr_operator;		/* dmf operator values for
					** dmr_position call.
					*/
    i4		attr_value;		/* offset in dsh->dsh_key array
					** where value will be found.
					** If this attribute is used
					** in a join node (as opposed to
					** an orig node), this value is
					** an offset into the qen_current
					** row in the ADE_CX structure
					** that produces the join key.
					*/
};

/*}
** Name: QEN_TKEY - top level runtime key list
**
** Description:
**
**	Since the QEF_KEY structure is part of the QP, it's not to be
**	changed at runtime.  However the runtime may need to fool with
**	the keys, especially if some key values are runtime query
**	parameters.
**
**	The solution is to maintain an almost-mirror of the QEF_KEY
**	structures that is attached to the DSH instead of the QP.
**	When a QP user needs to sort the KOR's or winnow the KATT's,
**	it does it against the mirror instead of the actual QEF_KEY
**	structure.
**
**  QEN_TKEY corresponds to QEF_KEY
**  QEN_NOR  corresponds to QEF_KOR
**  QEN_NKEY corresponds to QEF_KAND
**
**	There's no QEN structure corresponding to the QEF_KATT's,
**	it's OK for QEN_NKEY to point at the chosen QEF_KATT.
**
**	The runtime key stuff is only used for ORIG keys.  K-join
**	keys don't need it.
**  
**	All this runtime stuff (TKEY, NOR, NKEY) is allocated from the
**	same memory stream that the DSH came from, and is re-used when
**	a repeat query DSH is re-used.  It's not allowed to lose runtime
**	key elements that aren't executed, because they may be needed
**	next time.  To that end, qen_nkey is the key list for use by
**	the current execution, and qen_keys is the list of the original
**	set of QEN_NOR's allocated to the DSH.
**
**	([kschendel] If I were doing this, I might have simply mirrored the
**	QEF_KEY, rather than inventing a whole new set of structs;  but
**	I suppose this way is more fun.  Also, I don't know why this stuff
**	is here in back/hdr instead of being local to QEF.)
**
** History:
**     28-jul-86 (daved)
**          written
[@history_line@]...
*/
struct _QEN_TKEY
{
    QEN_NOR	    *qen_nkey;		/* next key to use for keying;
					** advances through the NOR list
					*/
    QEN_NOR	    *qen_keys;		/* Reference pointer to full NOR list.
					** (any NOR's unused by some particular
					** execution are put in front and
					** nkey skipped past them.)
					*/
};

/*}
** Name: QEN_NOR - a range of keys 
**
** Description:
**	This structure contains a list of keys in the proper order
**  for keying orig nodes.  It's the runtime mutable equivalent of
**  the KOR struct.
**
** History:
**     28-jul-86 (daved)
**          written
**	20-jul-1993 (pearl)
**          Bug 45912.  Add qef_kor field to QEN_NOR structure to keep track
**          of which QEF_KOR structure it is associated with.
**          Add kor_id field to QEF_KOR structure.
[@history_line@]...
*/
struct _QEN_NOR
{
    QEN_NOR	    *qen_next;		/* next key range */
    QEN_NKEY	    *qen_key;		/* the keys for this range */
    QEF_KOR         *qen_kor;           /* corresponding qef_kor structure;
					** used purely for double-checking
					** that kor-id found the right NOR */
};

/*}
** Name: QEN_NKEY - the key
**
** Description:
**      This structure points to the QEF_KATT struct containing
**  the chosen key value and operator for the attribute.  It's the
**  runtime mutable equivalent of a KAND key element.
**
** History:
[@history_template@]...
*/
struct _QEN_NKEY
{
    QEN_NKEY	*nkey_next;		    /* next key element */
    QEF_KATT	*nkey_att;		    /* chosen attribute key value */
};


/*}
** Name: QEN_NODE - The union of all possible nodes
**
** Description:
**	Nodes are the largest elements in a QEP. The node is a data structure
**  that, together with a function, will produce tuples.
**  
**
** History:
**     28-mar-86 (daved)
**          written
**	7-nov-95 (inkdo01)
**	    Replaced QEN_ADF structure instance by pointer and moved nodeqen 
**	    node structure to end to permit minimal memory allocation.
**	26-feb-99 (inkdo01)
**	    Added hash join node descriptor.
**	7-nov-03 (inkdo01)
**	    Added exchange node descriptor.
**	16-dec-03 (inkdo01)
**	    Added qen_high/low for 1:n exchange nodes.
**	2-jan-04 (inkdo01)
**	    Added qen_flags to contain partitioning (and other) directives.
**	17-june-04 (inkdo01)
**	    Added QEN_PQ_RESET flag.
**	27-may-05 (inkdo01)
**	    Added qen_nthreads for possible use in hash join prep (if not
**	    elsewhere).
**	10-Apr-2007 (kschendel) SIR 122513
**	    Add flag to tell joins to emit end-of-group's;  for PC join
**	    underneath another partition-compatible operation.
**	13-Nov-2009 (kiria01) 121883
**	    Distinguish between LHS and RHS card checks for 01.
**	14-May-2010 (kschendel) b123565
**	    Update nthreads comment.  Delete high/low, not used.
*/
struct _QEN_NODE
{
    i4                  qen_num;        /* node number. This should be
					** unique for all nodes in the QP
					*/
    i4                  qen_size;       /* size of this node in bytes*/
    i4                  qen_type;       /* type of this node. Use the
					** types defined in QEF_FUNC
					*/
    QEN_NODE           *qen_postfix;    /* next node in postfix list */
    DB_STATUS	       (*qen_func)();	/* function to compute this node 
					** QEF fills in this value. the
					** compiler does not.
					*/
    /* function attribute computation
    ** for join and orig nodes.
    */
    QEN_ADF		*qen_fatts;	/* compute function attributes. set
					** to zero if none.
					*/
    /* initialization info */
    i4			qen_natts;	/* number of attributes */
    DMT_ATT_ENTRY	**qen_atts;     /* an array of pointers to attributes
					** available at this node. 
                                        */
    QEN_ADF		*qen_prow;	/* print current row- for debugging */
    i4                  qen_row;        /* Index into DSH->DSH_ROW to find
					** row associated with this node.
					** this row is used to initialize
					** the DMR_CB for those nodes
					** that have them.
					** 
					*/
    i4			qen_nthreads;	/* no. of threads this node will
					** execute under.  Zero if node is
					** NOT under an EXCH, else 1 or more.
					*/
    i4			qen_frow;	/* DSH result row for qen_fatts
					** code (or -1).
					*/
    i4			qen_flags;	/* flag field */
#define	QEN_PART_SEPARATE	0x1	/* node should split partition groups */
#define QEN_PART_NESTED		0x2	/* Node should signal end-of-group to
					** caller when partition group ends.
					** (Only appears with PART-SEPARATE
					** turned on as well.) */
#define	QEN_PQ_RESET		0x4	/* TRUE if node is positioned such that
					** a RESET may be performed after
					** node initialization - allows early
					** cleanup of exchange threads */
#define QEN_CARD_MASK	0x38
#define QEN_CARD_NONE	(0<<3)	/* => nothing */
#define QEN_CARD_01		(1<<3)	/* => 0 or 1 on node */
#define QEN_CARD_01R		(2<<3)	/* => 0 or 1 on RHS */
#define QEN_CARD_01L		(3<<3)	/* => 0 or 1 on LHS */
#define QEN_CARD_ANY		(4<<3)	/* => >0 on RHS */
#define QEN_CARD_ALL		(5<<3)	/* => ALL on RHS */
#define QEN_SELTYPES ",SEL_01,SEL_01R,SEL_01L,SEL_ANY,SEL_ALL"
    /* Note on QEN_PART_SEPARATE and QEN_PART_NESTED:
    ** For non-joining nodes (orig, sort, tsort), QEN_PART_SEPARATE says both
    ** "expect end-of-group from below" and "operate partition-wise and send
    ** end-of-group to caller".  These nodes never operate group-wise unless
    ** they're in the midst of a partition-compatible section of the QP.
    **
    ** For joining nodes (hash, FSM), the situation is a little more complex
    ** because we might want to either a) join partition-group by partition-
    ** group and return the complete answer to the caller, or b) join
    ** partition-group-wise and tell the caller end-of-group after each
    ** group is done.  (b) is needed for a nested PC-join, while (a) is
    ** needed for a higher-level join (eg under something that doesn't
    ** understand QE00A5's,  like most actions).
    ** Case (a) just sets QEN_PART_SEPARATE, while (b) also sets
    ** QEN_PART_NESTED.
    **
    ** It would be cleaner to use QEN_PART_NESTED for non-join nodes too,
    ** it just wasn't needed (and QEN_PART_NESTED came later...)
    */

    i4                  qen_rsz;        /* size of row for this node */
    i4		qen_est_tuples;	/* estimated number of tuples for
					** DMF sorter */
    i4		qen_dio_estimate;   /* Logical IO estimate. */
    i4		qen_cpu_estimate;   /* CPU cost estimate */
    /* inkdo01 - moved node_qen from after qen_func to here to permit */
    /* allocation of minimal QEN_NODE - 1/11/95 */
    union
    {
        QEN_ORIG        qen_orig;
	QEN_SJOIN	qen_sjoin;					       
        QEN_TJOIN       qen_tjoin;
        QEN_KJOIN       qen_kjoin;
        QEN_HJOIN       qen_hjoin;
	QEN_SEJOIN	qen_sejoin;
        QEN_SORT        qen_sort;
        QEN_TSORT       qen_tsort;
        QEN_QP          qen_qp;
	QEN_EXCH	qen_exch;
        QEN_TPROC       qen_tproc;
    } node_qen;
};
