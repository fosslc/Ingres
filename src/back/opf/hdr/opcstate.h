/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPCSTATE.H - Data structures required for query compilation.
**
** Description:
**	This file contains the definitions for all data structures that are
**	specific to query compilation. So for, this list includes:
**	    OPC_QUERYCOMP
**	    OPC_STATE
**	    OPC_OBJECT_ID
**
** History: $Log-for RCS$
**      22-jul-86 (eric)
**          created
**	28-jan-89 (paul)
**	    Added additional state information required for rules processing.
**	20-sep-89 (paul)
**	    Added support to allow the old and new TID values to be used as
**	    parameters to procedures called from rules. This is a nice
**	    feature for supporting referential integrity like constraints
**	    when the table does not have a primary key.
**	28-dec-90 (stec)
**	    Modified OPC_STATE struct.
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**	17-may-93 (jhahn)
**	    Added opc)invoke_rule_ahd to OPC_STATE.
**	26-aug-93 (rickh)
**	    Added OPC_SAVE_ROWCOUNT bit to opc_flags.
[@history_template@]...
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _OPC_STATE OPC_STATE;
typedef struct _OPC_OBJECT_ID OPC_OBJECT_ID;
typedef struct _OPC_BASE OPC_BASE;

/*
**  Forward and/or External function references.
*/
FUNC_EXTERN PTR		opu_qsfmem();
FUNC_EXTERN PTR		opu_memory();

/*
**  Defines of other constants.
*/

/*
** References to globals variables declared in other C files.
*/

/*}
** Name: OPC_STATE - Session state info specific to query compilation 
**
** Description:
**      This structure contains the information specific to the compilation of
**	one entire particular query ie. a query that is broken up into 
**	sub-queries. This information tells us where the top QP header is for
**	this query, and what to fill the QP with.
**
**	This structure will live inside the OPS_STATE structure.
**
** History:
**      22-jul-86 (eric)
**          defined
**	28-jan-89 (paul)
**	    Added support for rules processing. This includes identifiers
**	    for the before and after row from the triggering query, a field
**	    in which to anchor the generated rule action list and a list
**	    of descriptors for the parameter list descriptors to be generated
**	    in the DSH. See comments below for more details.
**	28-dec-90 (stec)
**	    Added a ptr to print buffer for OPC routines dealing with debug
**	    output for trace points like op150.
**      30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**	28-dec-92 (teresa)
**	    add OPC_REGPROC to opc_flags.
**	10-feb-93 (andre)
**	    defined OPC_STARVIEW over opc_flags.  This will serve as an
**	    indication that we are creating a STAR view
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**	17-may-93 (jhahn)
**	    Added opc)invoke_rule_ahd.
**	23-oct-98 (inkdo01)
**	    Added opc_retrowno, opc_retrowoff for row producing procs.
**	17-nov-03 (inkdo01)
**	    Added opc_temprow to address opc_rows entry for intermediate 
**	    CX results buffer.
**	1-mar-04 (inkdo01)
**	    Added OPC_EXCH_N flag for parallel queries.
**	7-may-2008 (dougi)
**	    Added opc_curnode to help with table procedures.
**	21-Jun-2010 (kschendel) b123775
**	    Delete unused stmtno.
*/
struct _OPC_STATE
{
    OPS_SUBQUERY    *opc_subqry;    /* The current subquery being compiled */
    QEF_QP_CB	    *opc_qp;   /* The QP header for this session's query */

	/* Opc_rows is a linked list of the row buffer information that QEF will
	** need to execute this query. By convention, there is one row to hold
	** the intermediate results of any and all expressions. This row will
	** be the first on the row list.
	*/
    OPC_OBJECT_ID   *opc_rows;

	/* Opc_qnnum is the next number to use for the QEN_NODE.qen_num field.
	*/
    i4		    opc_qnnum;

	/* Opc_holds is a pointer to a linked lists, just like
	** opc_rows. It contains the number and
	** sizes of the hold file buffers that QEF will need.
	*/
    OPC_OBJECT_ID   *opc_holds;

	/* opc_rparms is an array of pointers to OPC_BASEs. There is one
	** for each repeat query parameter that is used in this query. See 
	** OPS_STATE.ops_parmtotal for the size of the array.
	*/
    OPC_BASE	    *opc_rparms;

	/* Opc_topahd is a pointer to the top action header of the current
	** action header. In other words, if the current action header is
	** pointed to by a QEN_QP (ran) node, then opc_topahd will point
	** to the action header that is above the QEN_QP node.
	*/
    QEF_AHD	    *opc_topahd;

	/* RDF relation descriptor which is used for cursors only */
    RDR_INFO        *opc_relation;

	/* Opc_firstahd is a pointer to the first action header that
	** is created.
	*/
    QEF_AHD	    *opc_firstahd;

	/* Opc_pvrow_dbp is the first of N rows for DB proc parameter values */
    i4		    opc_pvrow_dbp;

    i4		    opc_flags;	/* Flags for the state of compilation */
#define	    OPC_LVARS_FILLED	0x01    /* Local dbp vars have default values */
#define	    OPC_REGPROC		0x02	/* statement is a registered procedure*/
#define	    OPC_STARVIEW	0x04	/* creating a STAR view */
#define	    OPC_SAVE_ROWCOUNT	0x08
#define	    OPC_EXCH_N		0x10	/* currently under a 1:n EXCHANGE node */
	/*
	** This flag is set by the SELECT action in a CREATE TABLE AS
	** SELECT compilation.  Subsequent NOT NULL INTEGRITY actions will
	** translate this into a run-time flag that forces them to remember
	** the SELECT action's rowcount.  This makes it possible to emit a
	** proper rowcount following a CREATE TABLE AS SELECT where the source
	** table has NOT NULL columns.
	*/

    /*									    */
    /* The following fields are used to manage the compilation of rule	    */
    /* action lists. Rule actions are attached to update, delete and insert */
    /* actions. They represent additional actions that must be executed	    */
    /* after the triggering action updates, inserts or deletes a row in the */
    /* database. Currently these action lists may only contain IF and	    */
    /* CALLPROC actions. These actions are compiled in separate calls to    */
    /* OPC. During these calls the compiler must know where the before and  */
    /* after images of the updated row from the triggering action are	    */
    /* located. In addition, a description of the table updated is required */
    /* in order to locate values out of the before and after images. For    */
    /* the present we are assuming the before and after images are images   */
    /* of rows from a table, not images of a target list. */

    OPC_OBJECT_ID   *opc_cparms;	/* Linked list of descriptors for   */
					/* CALLPROC parameter lists being  */
					/* generated for this QP. Currently */
					/* used only during rules	    */
					/* processing. */
    i4		    opc_before_row;	/* Contains row number in DSH_ROWS  */
					/* for the before image of the row  */
					/* updated by the previous QP. This */
					/* field is only valid during rule  */
					/* statement list processing. */
    i4		    opc_after_row;	/* Contains row number in DSH_ROWS  */
					/* for the after image of the row   */
					/* updated by the previous QP. This */
					/* field is only valid during rule  */
					/* statement list processing. */
    i4		    opc_bef_tid;	/* Contains row number in DSH_ROWS
					** for the TID of the row before it
					** was updated by the previous QP.
					** This field is only valid during rule
					** statement list processing.
					*/
    i4		    opc_aft_tid;	/* Contains the row number in DSH_ROWS
					** for the TID of the row after it
					** was updated by the previous QP.
					** This field is only valid during rule
					** statement list processing.
					*/
    i4		    opc_retrowno;	/* Contains the row number in DSH_ROWS
					** for the "return row" buffer of
					** a row-producing dbproc
					*/
    i4		    opc_retrowoff;	/* Offset of next element in result
					** row buffer 
					*/
    PST_QNODE	    *opc_retrow_rsd;	/* Ptr to return row parse tree (for
					** building row descriptor)
					*/
    QEF_AHD	    *opc_rule_ahd;	/* Head of the action list into	    */
					/* which a rule statement list is   */
					/* compiled. This list is	    */
					/* eventually linked to the update  */
					/* action from tyhe original QP. */
    QEF_AHD	    *opc_invoke_rule_ahd; /* Head of the action list into   */
					/* which statement level rules are  */
					/* being compiled into. */
    RDR_INFO	    *opc_rdfdesc;	/* Pointer to the table descriptor  */
					/* for the table updated in the	    */
					/* previous QP. This is only valid  */
					/* during rule statement list	    */
					/* processing. */
    OPC_OBJECT_ID   *opc_tempTableRow;	/* Pointer to row descriptor for
					** temptable needed for handling
					** updates to tables with statement
					** level unique constraints.  Is
					** only valid in that case.
					*/
    OPC_OBJECT_ID  *opc_temprow;	/* Pointer to row descriptor for 
					** CX intermediate results buffer
					*/
	/* This is the top of the linked list of DECVAR statements that
	** are in current dbproc.
	*/
    PST_STATEMENT   *opc_topdecvar;

    char	    *opc_prbuf;		/* Ptr to print buffer needed 
					** for OPC trace code.
					*/
    QEN_NODE	    *opc_curnode;	/* Ptr to current query tree node
					** (used to build TPROC parm list)
					*/
};

/*}
** Name: OPC_OBJECT_ID - ID for an object that QEF will use during execution.
**
** Description:
**      This structure helps keep track of the various objects that QEF, and
**	ADE, will use in executing a query. The objects include row buffers,
**	control blocks, hold file buffers, as well as others. For information
**	on how QEF uses these objects, see the QEF_QP_CB and DSH structs in
**	QEF. For the purposes of query compilation, we 
**	don't need the actual buffer
**	allocated; we only need to know that the buffer will be allocated when
**	needed, how big it will be, and an indirect reference of where the 
**	object will live.
**
**      OPC_OBJECT_ID is a single element that forms a linked list of 
**	OPC_OBJECT_ID's.
**
** History:
**      22-jul-86 (eric)
**          defined
[@history_template@]...
*/
struct _OPC_OBJECT_ID
{
	/* Opc_onext and opc_oprev are pointers to other OPC_OBJECT_ID's that
	** are on a link list. Note that the opc_onext in the OPC_OBJECT_ID at
	** the end of a list points to the beginning of the list; likewise, the
	** opc_oprev at the beginning of the list points to the end of the list.
	*/
    OPC_OBJECT_ID	    *opc_onext;
    OPC_OBJECT_ID	    *opc_oprev;

	/* Opc_onum is the number of this object. It will ultimatly be used 
	** as an index into an array in the DSH struct, eg. DSH.DSH_ROWS or
	** DSH.DSH_CBS, to find the object buffer. Opc_rlen
	** gives the length of the object buffer.
	*/
    i4		    opc_onum;
    i4		    opc_olen;
};

/*}
** Name: OPC_BASE - Info to use to fill in a QEN_BASE struct
**
** Description:
[@comment_line@]...
**
** History:
**      18-jan-87 (eric)
**          defined
[@history_template@]...
*/
struct _OPC_BASE
{
	/* Opc_bvalid is TRUE if the other fields in this struct hold
	** valid info. Otherwise opc_bvalid will hold FALSE.
	*/
    i4			opc_bvalid;

	/* Opc_index is an index into the array described by opc_array */
    i4			opc_index;

	/* Opc_array is describes which array in QEF to use, ie. rows, 
	** parms, etc. This holds the same values as QEN_BASE.qen_array
	*/
    i4			opc_array;

	/* Opc_offset gives the offset into the tuple, found by using opc_index
	** and opc_array, to find the data for this parameter.
	*/
    i4			opc_offset;

	/* Opc_dv describes the data type, length, etc of this base.
	** NULL if no available info.
	*/
    DB_DATA_VALUE	opc_dv;
};

/*}
** Name: OPC_PST_STATEMENT - OPC specific info for DB proc statements
**
** Description:
**      This structure holds OPC specific information that relates to
**      a PST_STATEMENT structure. The PST_STATEMENT.pst_opf PTR points
**      to this structure. When OPF defines a similiar structure, then
**      this should be merged in with it.
[@comment_line@]...
**
** History:
**      23-may-88 (eric)
**          defined
[@history_template@]...
*/
typedef struct _OPC_PST_STATEMENT
{
	/* The first DSH row to hold the local var values. This is used only
	** if this is a DECVAR statement.
	*/
    i4			opc_lvrow;

	/* This is the first action header for this statement. */
    QEF_AHD		*opc_stmtahd;

	/* There can be several decvar statements for a dbproc. This forms
	** a linked list of them.
	*/
    PST_STATEMENT	*opc_ndecvar;
}   OPC_PST_STATEMENT;
