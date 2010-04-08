/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPCSUBQRY.H - Define compilation structs to go in OPS_SUBQUERY
**
** Description:
[@comment_line@]...
**
** History:
**      25-aug-86 (eric)
**          created
**	29-jan-89 (paul)
**	    Added additional base to OPC_ADF for rule processing
**	27-mar-89 (jrb)
**	    Added opc_tprec to OPC_TGPART for decimal project.
**	13-jul-90 (stec)
**	    Added OPT_PBLEN define for length of the temporary
**	    buffer passed to TRformat().
**	14-jul-92 (fpang)
**	    Fixed compiler warnings, OPC_DIST.
[@history_template@]...
**/

#define	    OPC_KEYS	    1	/* Full keying */
#define	    OPC_NTKEYS	    2	/* No keying */
#define	    OPC_TIDKEY	    3	/* Tid keying */
#define	    OPC_EXKEYS	    4	/* Exact keying */

    /* OPC_CTEMP is the offset into both dsh_rows and any base array where
    ** the row that holds constant size temporary results.
    */
#define	    OPC_CTEMP	    ADE_IBASE

    /* OPC_NONCB is an offset into DSH->DSH_CBS that can never exist. */
#define	    OPC_NONCB	    -1

    /* Defines length of the temporary buffer used by TRformat() in
    ** OPTCOMP and OPTFTOC modules. The buffer itself is defined in
    ** OPTFTOC. This buffer will be used by code invoked by various
    ** OPC trace point related routines.
    */
#define	    OPT_PBLEN	    132

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _OPC_SUBQUERY OPC_SUBQUERY;
typedef struct _OPC_NODE OPC_NODE;
typedef struct _OPC_EQ OPC_EQ;
typedef struct _OPC_ADF OPC_ADF;
typedef struct _OPC_TGINFO OPC_TGINFO;
typedef struct _OPC_TGPART OPC_TGPART;
typedef struct _OPC_DIST OPC_DIST;


/*}
** Name: OPC_ADF - Struct for the compilation of a ADE CX.
**
** Description:
[@comment_line@]...
**
** History:
**      18-jan-87 (eric)
**          defined
**	29-jan-89 (paul)
**	    Added support for a base array of QEF_USR_PARAM structures
**	    used as parameter lists for nested procedure calls.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**	15-jul-98 (inkdo01)
**	    Added opc_truelab, opc_falselab, opc_noncnf to support non-CNF
**	    changes.
**	13-sep-99 (inkdo01)
**	    Added opc_labels to trigger verification call for branch offsets.
**	27-oct-99 (inkdo01)
**	    Added opc_whenlab & opc_whenlvl to compile "or"s inside case when 
**	    expressions.
**	23-feb-00 (hayke02)
**	    Removed opc_noncnf - we now test for OPS_NONCNF in the
**	    subquery's ops_mask directly in opc_cqual1(). This change
**	    fixes bug 100484.
**	12-Dec-2005 (kschendel)
**	    Additions for more generic QEN_ADF handling:  add pointer to
**	    qen-adf pointer in node/ahd, plus a flag indicating whether
**	    we allocated the QEN_ADF optimistically in QSF or not.
**	6-aug-2008 (dougi)
**	    Added opc_hash flag for hash aggregate BY expr.
**	2-Jun-2009 (kschendel) b122118
**	    Delete obsolete adf-ag thing.
*/
struct _OPC_ADF
{
    QEN_ADF		*opc_qeadf;
    QEN_ADF		**opc_pqeadf;	/* Where above pointer lives in the
					** node or action header being
					** generated.
					*/

	/* Opc_row_base is an array of nats that maps from row numbers
	** to QEN_BASE numbers plus one. 
	** ie. opc_row_base[rowno] = baseno + 1. Opc_nrows tells how big
	** the opc_row_base array is. If any element in opc_row_base is
	** zero that no base has been set up for that row.
	*/
    i4			opc_nrows;
    i4			*opc_row_base;
	/* An array of nats to map from call procedure parameter array	    */
	/* numbers to QEN_BASE numbers plus one. Similar to the support	    */
	/* for rows above. */
    i4			opc_ncparms;
    i4			*opc_cparm_base;
	/* Opc_nrepeats and opc_repeat_base works just like opc_nrows and
	** opc_row_base, but for repeat query parameters.
	*/
    i4			opc_nrepeats;
    i4			*opc_repeat_base;

	/* The rest of these fields hold the base array number for thier
	** corresponding QEN_* as defined in QEN_BASE.
	*/
    i4			opc_key_base;
    i4			opc_out_base;
    i4			opc_vtemp_base;
    i4			opc_dmf_alloc_base;

	/* Opc_qlang and opc_cxlang should be large enough for all
	** the segements in a CX (ie. should be ADE_SMAX + 1)
	*/
    DB_LANG		opc_qlang[5];
    DB_LANG		opc_cxlang[5];

	/* Opc_memflag tells what ULM stream of memory to allocate temp
	** stuff (eg. opc_row_base) from. If it's OPC_STACK_MEM, then
	** opu_Gsmemory_get() should be called. Otherwise it should be 
	** OPC_REG_MEM and opu_memory() should be called.
	*/
    i4			opc_memflag;
#define	    OPC_STACK_MEM	1
#define	    OPC_REG_MEM		2
	/* Added fields for non-CNF Boolean expression support. Coding
	** them here saves parameters in opc_cqual1, a heavily recursive
	** function with stack frame size problems. Did same for tidatt
	** to further reduce stack frame size.
	*/
    i4			*opc_tidatt;
    PTR 		opc_truelab;	/* NOTE: these are declared "char *"
					** because of the many references to
					** this header from modules without 
					** adf.h. In fact, the pointers are
					** ALWAYS to ADE_OPERANDs. 
					*/
    PTR			opc_falselab;
    PTR			opc_whenlab;	/* Used for branching inside "case" */
    i4			opc_whenlvl;	/* Current nesting count of when exprs
					** (used for "or" tests inside a 
					** case "when") */
    bool		opc_labels;	/* TRUE - CX contains "branch" instrs
					** to verify at close time.
					*/
    bool		opc_qenfromqsf;	/* TRUE if QEN_ADF was optimistically
					** allocated from QSF.  Otherwise it
					** was allocated from an opf stream
					** and we have to copy it to QSF if
					** any code was actually generated.
					*/
    bool		opc_inhash;	/* TRUE if we're compiling the BY-expr
					** for a hash aggregate - to trigger
					** ADE_MECOPYN instead of ADE_MECOPY.
					*/
};

/*}
** Name: OPC_TGINFO - Target list information
**
** Description:
[@comment_line@]...
**
** History:
**      11-apr-87 (eric)
**          written
[@history_template@]...
*/
struct _OPC_TGINFO
{
	/* Opc_tsoffsets, opc_nresdoms, and  opc_hiresdom
	** are filled in when sort nodes appear
	** at the top of co/qen trees. The issue here is that top sort nodes
	** sort the target list expressions, not eq class attributes like
	** other sort nodes. Therefore, we need to record where each of
	** the target list expressions are, so that we can use them when we
	** are filling in the action header. Opc_tsoffsets is an array of
	** offsets into the output row of the top sort node. The resdom
	** numbers in the target list should be used to index into 
	** opc_tsoffsets. The dsh row number for the sorted target list
	** can be found in QEN_NODE.qen_row of the top sort node. The
	** data types and lengths can be found in the target list.
	** Opc_tsoffsets is opc_hiresdom+1 in size.
	*/
    i4		    opc_nresdom;
    i4		    opc_hiresdom;
    OPC_TGPART	    *opc_tsoffsets;
    i4		    opc_tlsize;	    /* sizeof the target list */
};

/*}
** Name: OPC_TGPART - Struct to hold info about where to get target list
**				    info.
**
** Description:
[@comment_line@]...
**
** History:
**      11-feb-87 (eric)
**          defined
**	27-mar-89 (jrb)
**	    Added opc_tprec for decimal datatype.
**	18-dec-00 (inkdo01)
**	    Added aggwoffset, aggwlen for inline aggregate code.
**	8-feb-01 (inkdo01)
**	   Added opc_aggooffset for hash aggregates.
**	22-june-01 (inkdo01)
**	   Added opc_aggooffset2 for binary aggregates.
**	17-dec-04 (inkdo01)
**	    Added opc_tcollID for collation support.
[@history_template@]...
*/
struct _OPC_TGPART
{
	/* data type length and offset of data in the target list */
    DB_DT_ID	    opc_tdatatype;
    i4		    opc_tlength;
    i2		    opc_tprec;
    i2		    opc_tcollID;
    i4		    opc_toffset;

	/* The associated resdom */
    PST_QNODE	    *opc_resdom;

	/* The attribute number of this resdom as it appears from the sorter.
	** This will be 0 if it is missing. Note that this is different
	** from pst_rsno, and pst_ntargno. I would *like* to describe how
	** they are different, but they are *way* too complicated, so I can't.
	** This is an area (pst_rsno, pst_ntargno) that needs to be cleaned
	** up through out PSF, OPF, and OPC.
	*/
    i4		    opc_sortno;
    i4		    opc_aggwoffset;	/* offset in work buffer to 
					** aggregation work fields */
    i4		    opc_aggwlen;	/* length of agg work fields */
    i4		    opc_aggooffset;	/* offset in overflow buffer to
					** hash agg overflow fields */
    i4		    opc_aggooffset2;	/* offset in overflow buffer to
					** 2nd operand of binary agg */
};

/*}
** Name: OPC_DIST - Distributed OPC subquery-related  information
**
** Description:
**	This structure contains subquery-related information that
**	is used by distributed OPC.
**
**	This structure lives inside the OPS_SUBQUERY structure.
**
** History:
**      15-nov-88 (robin)
**          created
**      14-jul-92 (fpang)
**          Removed typedef, it's forward declared above.
[@history_template@]...
*/
struct _OPC_DIST
{
    OPD_ITEMP	opc_subqtemp;	/* temp table representing this
				** subquery, or OPD_NOTEMP */
    i4		opc_spad1;	/* for future use */
    i4		opc_spad2;	/* for future use */
};

/*}
** Name: OPC_SUBQUERY - Query compilation info required for each sub-query.
**
** Description:
**      This structure contains the information required to compile a sub-query
**	into an QEF action. Specifically, we hold a pointer to the current
**	QEF action header being built, and the QP header that the action header
**	belongs to.
**
**	This structure lives inside the OPS_SUBQUERY structure.
**
** History:
**      22-jul-86 (eric)
**          defined
[@history_template@]...
*/
struct _OPC_SUBQUERY
{
    QEF_AHD	    *opc_ahd;	    /* current action header */

	/* Opc_mkey hold adf info for the ahd_mkey field in the action
	** header pointed to above.
	*/
    OPC_ADF	    opc_mkey;

	/* Opc_bgpostfix and opc_endpostfix point to the beginning and end
	** nodes in a post order traversal of the QEN_NODE tree currently
	** being built by OPC.
	*/
    QEN_NODE	    *opc_bgpostfix;
    QEN_NODE	    *opc_endpostfix;

    OPC_TGINFO	    opc_sortinfo;

    PTR		    opc_pad1;
    OPC_DIST	    opc_distrib;    /* distributed OPC only */
};

/*}
** Name: OPC_NODE - Information about a co node and/or QEF_NODE struct.
**
** Description:
[@comment_line@]...
**
** History:
**      4-aug-86 (eric)
**          defined
**	28-june-05 (inkdo01)
**	    Added opc_subex_offset[] to support optimized CXs.
**	12-may-2008 (dougi)
**	    Added opc_oceq for table procedure support.
*/
struct _OPC_NODE
{
    OPO_CO	    *opc_cco;	    /* current co node */

	/* Opc_level tells whether opc_cco is the inner or outer of another
	** co node, or if it's the top (root) of a co tree.
	*/
    i4		    opc_level;
#define	    OPC_COTOP	1
#define	    OPC_COINNER	2
#define	    OPC_COOUTER	3

	/* Opc_invent_sort is TRUE if a sort node (either a QEN_SORT or
	** QEN_TSORT node) is to be invented on top of opc_co. Normally,
	** there is a one to one mapping between the CO nodes that OPF
	** gives me and the QEN nodes that OPC produces. In this situation
	** however, OPF just sets a bit in a join CO node to indicate that
	** a sort node should appear. If opc_invent_sort is FALSE, then no
	** extra sort nodes should be created.
	*/
    i4		    opc_invent_sort;

	/* Opc_qefnode points to the root QEN_NODE struct that corresponds to 
	** the CO node pointed to by ops_cco. 
	*/
    QEN_NODE	    *opc_qennode;

	/* Opc_ceq is an array of the current eq to att mappings for this 
	** co/qef node. The size of this array is the same as ??EJLFIX. Use
	** an eq class number to index into this array to find out what
	** attribute is representing the eq at the current node.
	*/
    OPC_EQ	    *opc_ceq;

	/* Opc_oceq is the OPC_EQ array available from the outer side of
	** a join. It is usually NULL, but valued when this is a CPjoin and
	** the inner is a table procedure whose parameter list has dependencies
	** on the outer side of the join. For nested joins, attributes from 
	** the outer side are available. */
    OPC_EQ	    *opc_oceq;

	/* Opc_below_rows gives the number of DSH rows that are used by
	** the current QEN node and below. This is usefull for estimating
	** the maximum number of bases a CX will need.
	*/
    i4		    opc_below_rows;

	/* Opc_subex_offset is a vector of offsets in the work buffer
	** for a given CX for the subexpression results generated by
	** the pre-evaluation of common subexpressions. PST_VAR nodes
	** that index the offset vector entries replace the subexpressions
	** in the CX and the vector allows the computed values to be
	** turned into operands in the CX. */
    i4		    opc_subex_offset[200];
};

/*}
** Name: OPC_EQ - eqc info for a co node.
**
** Description:
**	This struct provides mapping of an eqc to an attribute number
**	when available for a CO node and info about where in a DSH row
**	will reside. This structure is filled in stages therefore two
**	flags exist to indicate whether fields have been filled in. 
**
** History:
**      04-aug-86 (eric)
**          defined
**      28-mar-90 (stec)
**          Moved OPC_ATT struct contents. Added the OPC_NOATT define and
**	    opc_eqcdv.
*/
struct _OPC_EQ
{
	/* Opc_eqavailable is TRUE if this structure is filled in. */
    i4		    opc_eqavailable;

	/* Opc_attno is the joinop attribute number that this eq class
	** translates to for the current co node. It can be used as an
	** index into an OPZ_ATTS array. Opc_attno will contain valid
	** information iff opc_eqavailable is TRUE, otherwise it is undefined.
	** In some cases (full outer join) an eqc will not have an attribute
	** number assigned; it will be initialized to OPC_NOATT.
	*/
    i4		    opc_attno;
# define	OPC_NOATT	-1

	/* Opc_attavailable is TRUE if this joinop attribute is available at 
	** the current CO node and/or QEN_NODE , FALSE otherwise. There is a
	** special case, when, in the outer join case, the equivalence class
	** is not derived directly from any attributes of the children CO
	** nodes, in this case the flag will be set but the opc_attno will be
	** set to OPC_NOATT.
	*/
    i4		    opc_attavailable;

	/* Opc_dshrow and opc_dshoffset tell where QEF will hold this attribute.
	** Opc_dshrow gives the index to be used into the row array in QEF's
	** DSH struct. Opc_dshoffset gives the offset to the attribute from
	** the beginning of the row. Note that opc_dshoffset may be usefull
	** when dealing with function atts. These two fields will contain valid
	** information only when opc_attavailable is TRUE, otherwise they are
	** undefined.
	*/

    i4		    opc_dshrow;
    i4		    opc_dshoffset;

	/* Opc_eqcdv will hold info about datatype, which describes this
	** eqc. Note that in case of outer joins, an eqc resulting from
	** joining two eqc's may have a different datype than the joining
	** eqc's. This is caused by the fact, that the OPC parent node needs
	** to see the values in a uniform way, hence in the case of a full
	** outer join the source data will have to be coerced to the desired
	** target datatype.
	*/

    DB_DATA_VALUE   opc_eqcdv;
};
