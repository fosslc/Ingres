/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cui.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <qefmain.h>
#include    <ex.h>
#include    <cm.h>
#include    <st.h>
#include    <cv.h>
#include    <me.h>
#include    <ulx.h>
#include    <bt.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>
#include    <opckey.h>
#include    <opcd.h>
#include    <opxlint.h>
#include    <opclint.h>

# define OPCU_MAXRES	DD_MAXCOLUMN
/* max no. columns in a target list */


/*}
** Name: ULD_TSTATE - state of a building a linked list of text blocks
**
** Description:
**      A linked list of text blocks is created when converting a query
**      tree into text.  This structure describes the state of the linked
**      list and can be used to save the state of partially built linked
**      list.
**
** History:
**      31-mar-88 (seputis)
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-jun-2001 (somsa01)
**	    In opcu_prconst(), to save stack space, dynamically allocate
**	    space for convbuf.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**      10-sep-08 (gupsh01,stial01)
**          When creating query text from query tree, don't generate unorm() 
**	11-Nov-2009 (kiria01) SIR 121883
**	    Tightened detection of factored sub-queries whose presence
**	    is marked by a PST_VAR node. We cannot use the flawed uld_ssvar
**	    code to do this as sub-selects are not the only things that
**	    sit beneath opmeta annotated BOPs.
**      22-mar-2010 (joea)
**          Remove special handling for boolean in opcu_prconst.
*/
typedef struct _ULD_TSTATE
{
	char	    uld_tempbuf[ULD_TSIZE]; /* temp buffer used to build string
                                        ** this should be larger than any other
                                        ** component */
    i4		    uld_offset;		/* location of next free byte */
    i4		    uld_remaining;	/* number of bytes remaining in tempbuf 
					*/
    ULD_TSTRING     **uld_tstring;      /* linked list of text strings to return
                                        ** to the user */
} ULD_TSTATE;

/*}
** Name: ULD_TTCB - control block for tree to text conversion
**
** Description:
**      Internal control block for tree to text conversion
**
** History:
**      31-mar-88 (seputis)
**          initial creation
**      26-aug-88 (seputis)
**          fix casting for *conjunct
**      26-mar-91 (fpang)
**          Renamed opdistributed.h to opdstrib.h
**	20-may-94 (davebf)
**	    added uld_ssvar
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	07-jan-2002 (toumi01)
**	    Replace DD_300_MAXCOLUMN with DD_MAXCOLUMN (1024).
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_line@]...
[@history_template@]...
*/
typedef struct _ULD_TTCB
{
    PTR             handle;             /* user handle */
    ADF_CB	    *adfcb;             /* adf session control block */
    ULM_RCB         *ulmcb;             /* ulm memory for text string
                                        ** output */
    i4		    uld_mode;		/* mode of the query */
    ULD_TSTRING	    *uld_rnamep;        /* ptr to name of the result variable
                                        ** or NULL if it is not needed, a struct
                                        ** is used to that the caller can keep
                                        ** a ptr to the text string if necessary */
    PST_QNODE       *uld_qroot;		/* root of user's query tree to convert 
                                        ** - if a root node is supplied then the
                                        ** entire statement is generated, if a
                                        ** non root node is supplied then an
                                        ** expression is generated */
    i4		    uld_linelength;	/* max length of line to be
                                        ** returned or 0 if ULD_TSIZE is
                                        ** to be used */
    bool	    pretty;             /* TRUE - if output is to be used for
                                        ** visual display, causes "from list"
                                        ** for SQL to be linked into query
                                        ** so that caller has control */
    bool	    resdomflg;		/* call routine in order to obtain
					** the next resdom of a query
                                        ** - useful for traversing bitmaps
                                        ** of equivalence classes */
    DB_ERROR        *error;		/* error status to return to the user*/
    DB_STATUS	    uld_retstatus;
    ULD_TSTATE      uld_primary;        /* describes current state of text string
                                        ** being built */
    ULD_TSTATE      uld_secondary;      /* used to save alternate state of a text build
                                        */
    DB_LANG	    language;		/* language that should be used to build
					** the query text */
    bool	    is_quel;		/* TRUE if query language is QUEL */
    bool	    is_sql;             /* TRUE if query language to be generated 
                                        ** SQL */
    bool	    qualonly;		/* just generate WHERE clause text */
    PST_QNODE	    *uld_nodearray[ OPCU_MAXRES ];
    i4		    uld_nnodes;

} ULD_TTCB;

/*}
** Name: NULL_COERCE - vector of null coercion descriptors
**
** Description:
**      An array of descriptors used to coerce NULL into specific data types.
**	Used to handle "create table ... as select ..., NULL, ..." generated
**	queries (as when a select NULL is unioned to other selects which 
**	supply context for the NULL). These previously generated syntax errors 
**	when parsed on the target node.
**
** History:
**      1-oct-97 (inkdo01) 
**          initial creation
**	30-aug-2006 (gupsh01)
**	    Added ANSI datetime types.
[@history_template@]...
*/
typedef struct _NULL_COERCE
{
    DB_DATA_VALUE	type;		/* type info from corr. RESDOM */
    char		*name;		/* name of Ingres coercion function */
} NULL_COERCE;

/*
**  Static constant declarations.
*/

static NULL_COERCE	nc_array[] =
	{{{NULL, 2,  (DB_DT_ID)-DB_BYTE_TYPE,  0,-1}, "byte"},
	 {{NULL, 2,  (DB_DT_ID)-DB_CHR_TYPE,   0,-1}, "c"},
	 {{NULL, 2,  (DB_DT_ID)-DB_CHA_TYPE,   0,-1}, "char"},
	 {{NULL, 13, (DB_DT_ID)-DB_DTE_TYPE,   0,-1}, "ingdate"},
	 {{NULL, 5, (DB_DT_ID)-DB_ADTE_TYPE,   0,-1}, 
						  "ansidate"},
	 {{NULL, 11, (DB_DT_ID)-DB_TMWO_TYPE,   0,-1}, 
				    "time without time zone "},
	 {{NULL, 11, (DB_DT_ID)-DB_TMW_TYPE,   0,-1}, 
				    "time with time zone"},
	 {{NULL, 11, (DB_DT_ID)-DB_TME_TYPE,   0,-1}, 
				    "time with local time zone"},
	 {{NULL, 15, (DB_DT_ID)-DB_TSWO_TYPE,   0,-1}, 
				    "timestamp without time zone"},
	 {{NULL, 15, (DB_DT_ID)-DB_TSW_TYPE,   0,-1}, 
				    "timestamp with time zone"},
	 {{NULL, 15, (DB_DT_ID)-DB_TSTMP_TYPE,   0,-1}, 
				"timestamp with local time zone"},
	 {{NULL, 4, (DB_DT_ID)-DB_INYM_TYPE,   0,-1}, 
				"interval year to month"},
	 {{NULL, 13, (DB_DT_ID)-DB_INDS_TYPE,   0,-1}, 
				"interval day to second"},
	 {{NULL, 5,  (DB_DT_ID)-DB_FLT_TYPE,   0,-1}, "float4"},
	 {{NULL, 3,  (DB_DT_ID)-DB_INT_TYPE,   0,-1}, "int2"},
	 {{NULL, 9,  (DB_DT_ID)-DB_MNY_TYPE,   0,-1}, "money"},
	 {{NULL, 3,  (DB_DT_ID)-DB_VCH_TYPE,   0,-1}, "varchar"},
	 {{NULL, 4,  (DB_DT_ID)-DB_VBYTE_TYPE, 0,-1}, "varbyte"}};
#define NCA_SIZE sizeof(nc_array)/sizeof(nc_array[0])


/*
**  Forward and/or External function references.
*/

static VOID
opcu_msignal(
	ULD_TTCB           *ttcbptr,
	DB_ERRTYPE          error,
	DB_STATUS          status,
	DB_ERRTYPE	   facility );

static VOID
opcu_resright(
ULD_TTCB	*ttcbptr,
PST_QNODE	*resdomptr );

static VOID
opcu_wend(
ULD_TTCB    *ttcbptr );

static VOID
opcu_sort(
	ULD_TTCB    *ttcbptr );

static VOID
opcu_prexpr(
	ULD_TTCB           *ttcbptr,
	PST_QNODE          *qnode );

static bool
opc_tblnam(
	ULD_TTCB	   *ttcbptr,
	OPC_TTCB           *global,
	ULD_TSTRING        **namepp,
	ULD_TSTRING	   **aliaspp,
	i4		   *tempnum,
	bool		   *is_result );

static PST_QNODE *
opc_resdom(
	ULD_TTCB	   *ttcbptr,
	bool		   use_list,
	PST_QNODE          *resdomp,
	char               *namep,
	i4		   *attno,
	bool		   base_colname );


static VOID
opc_prbuf(
	OPC_TTCB	   *textcb,
	char               *stringp,
	ULD_TSTATE	   *bufstruct );


/*{
** Name: OPCU_handler	- exception handler for ULF tree to text
**
** Description:
**      This exception handler was declared to exit out of utility routine
**      in case of an error.
**
** Inputs:
**      none
**
** Outputs:
**      none
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-apr-88 (seputis)
**          initial creation
**	02-sep-1992 (rog)
**          Modified to use the new ulx_exception function.
**	02-jul-1993 (rog)
**          We should be calling ulx_exception() as OPF, not ULF.
[@history_line@]...
*/
static STATUS
opcu_handler(
	EX_ARGS		*args )
{
    if (EXmatch(args->exarg_num, 1, EX_JNP_LJMP) != 1)
    {
        (VOID) ulx_exception(args, DB_OPF_ID, E_OP08A2_EXCEPTION, TRUE);
    }
    return(EXDECLARE);
}

/*{
** Name: opcu_signal	- this routine will report an error
**
** Description:
**      This routine will report a E_DB_ERROR error.  The routine
**      will not return but instead generate an internal exception and
**      exit via the exception handling mechanism.
**
** Inputs:
**      ttcbptr				ptr to global state variable
**      error                           error code to report
**
** Outputs:
**	Returns:
**	    - routine does not return
**	Exceptions:
**	    - internal exception generated
**
** Side Effects:
**
** History:
**	8-apr-88 (seputis)
**          initial creation
**	09-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
[@history_line@]...
*/
static VOID
opcu_signal(
	ULD_TTCB           *ttcbptr,
	DB_ERRTYPE          error )
{
    ttcbptr->uld_retstatus = E_DB_ERROR;         /* return status code */
    ttcbptr->error->err_code = error;		/* error code */
    ulx_rverror("Tree to Text", E_DB_ERROR, error, 0, ULE_LOG, (DB_FACILITY)DB_ULF_ID);
    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)error);/* signal a long jump */
}

/*{
** Name: opcu_msignal	- this routine will report another facility's error
**
** Description:
**      This routine will report an error from another facility.
**      It will check the status and see the error
**      is recoverable.  The routine will not return if the status is fatal
**      but instead generate an internal exception and
**      exit via the exception handling mechanism.  
**
** Inputs:
**      ttcbptr				ptr to state variable
**      error                           error code to report
**      facility                        facility's error code
**      status                          facility's return status
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    - internal exception generated
**
** Side Effects:
**
** History:
**	8-apr-88 (seputis)
**          initial creation
**	02-sep-1992 (rog)
**          Removed test for successful status because we never call this
**	    function with a successful status.
**	09-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
[@history_line@]...
*/
static VOID
opcu_msignal(
	ULD_TTCB           *ttcbptr,
	DB_ERRTYPE          error,
	DB_STATUS          status,
	DB_ERRTYPE	   facility )
{
    ttcbptr->uld_retstatus = status;	/* save status code */
    ttcbptr->error->err_code = error;	/* error code */
    ttcbptr->error->err_data = facility;	/* error code */
    ulx_rverror("Tree to Text", status, error, facility, ULE_LOG, DB_ULF_ID); 

    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)error); /* signal a long
                                                ** jump with error code */
}

/*{
** Name: opcu_printf	- place string into buffer
**
** Description:
**      This routine will move a null terminated string into the buffer
**	If the caller has provided a string printing routine, it will be
**	called, otherwise this routine does the work.
**	For distributed query compilation, another routine is provided
**	since the string must be 'printed' into a query compilation
**	data structure.
**
** Inputs:
**      ttcbptr                          ptr to global state variable
**      stringp                         ptr to string to copy
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-mar-88 (seputis)
**          initial creation
**      04-oct-88 (robin)
**          Modified to use a routine provided by the caller
[@history_line@]...
[@history_template@]...
*/
static VOID
opcu_printf(
	ULD_TTCB           *ttcbptr,
	char               *stringp )
{

	opc_prbuf( (OPC_TTCB *) ttcbptr->handle, stringp,
	    &ttcbptr->uld_primary);
	return;
}

/*{
** Name: opc_flshb	- flush characters from temporary into qry text list
**
** Description:
**      Move any remaining characters out of the temp buffer and into
**      the list of query text segments for distributed compiled queries.
**
** Inputs:
**	ttcbptr				Global state structure
**	tstring				String list to print then flush or NULL
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
[@history_line@].
**      03-oct-88 (robin)
**          Created from uld_flush
[@history_line@]...
[@history_template@]...
*/
static VOID
opc_flshb(
	OPC_TTCB	   *textcb,
	ULD_TSTRING	   *tstring,
	ULD_TSTATE	   *tempbuf )
{
    /* 
    **  Write strings from the temporary buffer
    **  out into text segments first
    */

    if ( (tempbuf != NULL) && (tempbuf->uld_offset > 0 ))
    {
	opc_addseg( textcb->state, &tempbuf->uld_tempbuf[0], 
	    tempbuf->uld_offset, (i4) -1, &textcb->seglist );

	tempbuf->uld_offset = 0;	    /* reset buffer to be empty */
	tempbuf->uld_remaining = ULD_TSIZE - 1;	/* space for null terminator */
    }	

    /*  Print any input strings into text segments */

    while (tstring)
    {
        opc_addseg( textcb->state, tstring->uld_text, 
	    tstring->uld_length, (i4) -1, &textcb->seglist );
	tstring = tstring->uld_next;
    }

    return;
}




/*{
** Name: opcu_flush	- flush characters out of temporary buffer
**
** Description:
**      Move any remaining characters out of the temp buffer and into
**      heap memory.  This is not done if the "pretty" flag is selected
**      since then the text is used for human display.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
opcu_flush(
	ULD_TTCB           *ttcbptr,
	ULD_TSTRING	   *tstring )
{

	opc_flshb( (OPC_TTCB *) ttcbptr->handle, tstring,
	    &ttcbptr->uld_primary);
	return;
}


/*{
** Name: opc_printtemp-	    Print a temp table to a query segment
**
** Description:
**
** Inputs:
**	ttcbptr				Global state structure
**	tempnum				No. of the temp table
**	bufstruct			Text generation state
**					structure including temp
**					buffer.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
[@history_line@].
**      12-oct-88 (robin)
[@history_line@]...
[@history_template@]...
*/
static VOID
opc_printtemp(
	OPC_TTCB    *ttcbptr,
	i4	    tempnum,
	ULD_TSTATE  *bufstruct )
{
	if ( tempnum > -1 )
	{
	    opc_flshb( ttcbptr, NULL, bufstruct);
	    opc_addseg( ttcbptr->state, NULL, (i4) 0, tempnum, &ttcbptr->seglist );
	}
	return;
}



/*{
** Name: opcu_tabprintf	- place table name into text stream
**
** Description:
**      This routine will place a table name into the text stream.
**	Table names are special because they may consist of temp
**	numbers (to be filled in by QEF) rather than actual
**	name strings.  Note that ALIAS names will not be printed
**	by this routine.
**
** Inputs:
**      ttcbptr                          ptr to state variable
**      tname				ptr to table name string
**	tempnum				temp number if > -1.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-oct-88 (robin)
**          initial creation
[@history_template@]...
*/
static VOID
opcu_tabprintf(
	ULD_TTCB       *ttcbptr,
	ULD_TSTRING    *tname,
	i4	       tempnum )
{
	if (tempnum > OPD_NOTEMP )
	{
	    opc_printtemp( (OPC_TTCB *) ttcbptr->handle, tempnum,
			    &ttcbptr->uld_primary);
	}
	else
	{
	    opcu_flush(ttcbptr, tname);    
	}

	return;
}


/*{
** Name: opcu_from	- generate the from list
**
** Description:
**      Generate the from list of the query for SQL.  The table name
**	is isolated so that eventually it can be possibly replaced by
**      a unique temp table name.  This would be useful for shared 
**      query plans.
**
** Inputs:
**      ttcbptr                          query translation state variable
**	resultonly			 If true, generate a 'tablename rangename'
**					   string for the result table (i.e. the
**					   table to be modified in an update or
**					   delete).
**	noresult			 If true, do NOT include the result
**					   table in the FROM clause - used in
**					   flattened, correlated subqueries
**					   
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
[@history_line@]...
**      12-oct-88 (robin)
**          Replaced direct uld_flush call with call to
**	    uld_tabprintf so that temp tables would
**	    be handled correctly.
**      15-may-89 (robin)
**	    Partial fix for bug 5477.  If this is a 'no COtree'
**	    (i.e. constant-only) subquery, but there are constant boolean 
**	    factors for the WHERE clause, the LDB requires a FROM clause.
**	    However, because there is no CO tree, this subquery does not
**	    directly reference any variables (tables).  So generate
**	    a dummy FROM clause referencing iidbconstants.
**	25-may-89 (robin)
**	    Always generate a FROM clause. Use iidbconstants if no
**	    FROM clause is generated by normal means.  Reason is that
**	    gateway SQL always required a FROM clause.  This fix replaces
**	    the previous one for 5477, since this is the more general
**	    case.  Fix for bug 5484.	    
**	31-may-89 (robin)
**	    Added ttcbptr parm to opc_tblnam call. Part of fix for bug
**	    5485.
[@history_template@]...
*/
static VOID
opcu_from(
	ULD_TTCB           *ttcbptr,
	bool		    resultonly,
	bool		    noresult )
{
    bool	    first_time;
    ULD_TSTRING	    *opcutextp;	/* return a text block descriptor
				** which will be linked in directly
				** so that the caller can keep a
				** ptr to it */
    ULD_TSTRING	    *aliasp;	/* ptr to alais name if necessary
				** or NULL otherwise */
    i4		    tempnum;
    i4		    num_printed;
    OPD_ITEMP	    printed_temps[ OPV_MAXVAR ];
    i4		    x;
    bool	    printed;
    bool	    is_result;
    bool	    have_from;
    OPC_TTCB	    *textcb = (OPC_TTCB *)ttcbptr->handle;

    have_from = FALSE;
    first_time = TRUE;
    num_printed = 0;

    while (!opc_tblnam(ttcbptr,  (OPC_TTCB *) ttcbptr->handle,
		&opcutextp, &aliasp, &tempnum, &is_result))
    {
	if(tempnum != OPD_NOTEMP && 
	      textcb->cop->opo_sjpr == DB_SJ &&
	      textcb->cop->opo_variant.opo_local->opo_jtype == OPO_SEJOIN)
	    continue;

	if ( resultonly == TRUE && is_result == FALSE)
	{
	    continue;
	}

	if (noresult == TRUE && is_result == TRUE)
	{
	    continue;
	}

	have_from = TRUE;
	if (tempnum > OPD_NOTEMP)
	{
	    /* see if this table name has been printed yet */
	    for ( x = 0, printed = FALSE; x < num_printed; x++ )
	    {
		if ( printed_temps[x] == tempnum )
		{
		    printed = TRUE;
		    break;
		}
	    }
	    if (!printed)
	    {
		printed_temps[ num_printed ] = tempnum;
		num_printed++;
	    }
	}
	else 
	    printed = FALSE;

	if (!printed )
	{
	    if (first_time )
	    {   /* print from list only if one table exists */
		if ( !resultonly )
		{
		    opcu_printf(ttcbptr, " from ");
		}
		first_time = FALSE;
	    }
	    else
	    {
		opcu_printf(ttcbptr, ",");
	    }

	    opcu_tabprintf( ttcbptr, opcutextp, tempnum);

	    if (aliasp && aliasp->uld_length > 0 )
	    {
		opcu_printf(ttcbptr, " ");
		opcu_printf(ttcbptr, &aliasp->uld_text[0]);
	    }
	}
    }

    if (!have_from)
    {
    /*  fix for buf 5484.  Always generate a FROM clause, since
    **  gateway SQL requires it.  This includes the following
    **  special case, which was bug no. 5477:
    **  if the subquery has no CO tree (i.e. is constant only)
    **  but has boolean factors for the WHERE clause, generate
    **  a 'dummy' FROM clause since the LDB requires it.
    */
	opcu_printf(ttcbptr, " from iidbconstants ");
    }    

    return;

}

/*{
** Name: opcu_joinwhere	- Join WHERE clause to query text
**
** Description:
**	Joins two text segments, the where clause and
**	the main query text.  TYpically the where clause
**	has been built from other, inner nodes
**
** Inputs:
**      ttcbptr                          ptr to state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-nov-88 (robin)
**          initial creation
**      02-may-89 (robin)
**	    Set opq_q14_need_close flag when an open paren
**	    is generated for a nested subselect, to indicate
**	    that the close paren will be needed later.
**	23-aug-89 (robin)
**	    Change format of dummy target list for nested
**	    subselect to use 'as' instead of '='.
[@history_line@]...
[@history_template@]...
*/
static i4
opcu_joinwhere(
	ULD_TTCB       *ttcbptr )
{
	OPC_TTCB	*textcb;
	i4		retval;

	textcb = (OPC_TTCB *) ttcbptr->handle;
	retval = 0;

	opcu_flush( ttcbptr, NULL );

	if (textcb->where_cl == NULL )
	    return (retval);

	if (textcb->anywhere == TRUE)
	{
	    opcu_printf( ttcbptr, " and " );
	}
	else if (!ttcbptr->qualonly)
	{
	    if ( textcb->qryhdr->opq_q9_flattened && 
	          textcb->cop == textcb->subquery->ops_bestco )
	    {
		/*
		**  there is a subselect in the where clause.  Since this
		**  is always an 'exists' subselect, the target list is
		**  a dummy that is only included for syntactic correctness.
		*/

		opcu_printf( ttcbptr, " where exists ( select 1 as e0 " );
		opcu_from (ttcbptr, (bool) FALSE, (bool) TRUE);	    /* generate from list for SQL */		
		textcb->qryhdr->opq_q14_need_close = TRUE;	/* closing paren will be needed
								** for subselect */
	    }
	    opcu_printf( ttcbptr, " where ");
	}

	textcb->anywhere = TRUE;
	opcu_flush( ttcbptr, NULL );
	opcu_joinseg( &textcb->seglist, textcb->where_cl );
	textcb->where_cl = NULL;
	retval = 1;
	    
	return( retval );
}

/*{
** Name: opcu_intprintf	- place integer into text stream
**
** Description:
**      This routine will place an integer into the text stream.
**
** Inputs:
**      ttcbptr                          ptr to state variable
**      intvalue                        integer to place into text buffer
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opcu_intprintf(
	ULD_TTCB           *ttcbptr,
	i4            intvalue )
{
    char                outstring[20];

    CVla((i4)intvalue, &outstring[0]);
    opcu_printf(ttcbptr, &outstring[0]);
}

/*{
** Name: opc_pttresname	- print temp table resdom name
**
** Description:
**      print attribute name for resdom given a temporary table
**	i.e. print 'a' followed by resdom number 
**
** Inputs:
**      ttcbptr                         state variable
**      resnum                          resdom number
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-apr-93 (ed)
**          initial creation
*/
static VOID
opc_pttresname(
	ULD_TTCB           *ttcbptr,
	i4		   resnum)
{
    opcu_printf(ttcbptr, "a");
    opcu_intprintf(ttcbptr, (i4)resnum);
}

/*{
** Name: opc_varnode	- return var node ID information
**
** Description:
**      This is a routine called by the query tree to text conversion 
**      routines used to abstract the range table information of the 
**      caller.  It will produce a string which represents the equivalence
**      class for the joinop attribute.
[@comment_line@]...
**
** Inputs:
**	textcb                          state variable for tree to text 
**                                      conversion
**      varno                           range variable number of node
**      atno                            attribute number of node
**
** Outputs:
**      namepp				ptr to list of attributes in
**					the form alias.attrname, optionally
**					surrounded with curly braces.
**					    
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-apr-88 (seputis)
**          initial creation
**      20-sep-88 (robin)
**	    Added check of noalias and nobraces flags in the OPC_TTCB
**	    control block for distributed query text generation support.
**	31-may-89 (robin)
**	    Added mode parameter to opc_nat call. Partial fix for bug
**	    5485.
**	10-oct-89 (seputis)
**	    Fixed function attribute problem in joins
**      18-dec-92 (ed)
**          fix cursor problem, in which target relation is required and
**	    an EXISTS type query is being created
**      30-mar-95 (brogr02)
**	    Fix for bug 67299
**      23-Jul-98 (wanya01)
**          X-integrate change 427920 (author sarjo01). 
**          Bug 78930: force choice of original target column name for 
**          generated SELECT list.
**  12-jan-99 (kitch01)
**		Bug 90928. Re-work change for bug 78930 to resolve this bug.
**		If the attribute we find is not nullable but the result attribute
**		is continue searching. This forces the attribute found to be
**		nullable as well and prevents 78930.[@history_template@]...
**  29-feb-00 (wanfr01)
**          Bug 100491, INGSTR 35: 
**          Adjusted fix for bug 427920 so restriction was only applied to
**          cursor queries.  For non-cursor queries, it would cause the correct
**          attribute to be skipped also.
**	31-aug-01 (inkdo01)
**	    Add var map check for func atts to assure we've got the right one.
**      21-Oct-2003 (hanal04) Bug 103856 INGSRV2557
**          Extend fix for bug 78930 so that it covers nullable vs 
**          non-nullable AND the reverse. This prevents E_LQ0040 in
**          front end ESQL programs which access tbales via STAR.
*/
static VOID
opc_varnode(
	ULD_TTCB	    *ttcbptr,
	i4		    varno,
	i4		    atno )
{
    OPC_TTCB            *textcb;
    OPS_SUBQUERY    *subquery;
    OPZ_BMATTS	    *attrmap;	    /* bitmap of attributes in equivalence
				    ** class */
    OPT_NAME	    attrname;	    /* temporary in which to build
				    ** attribute name */
    OPCUDNAME	    tablename;	    /* temporary in which to build the
                                    ** table name */
    OPT_NAME	    aliasname;	    /* temp in which to build the table alias
				    ** name */
    OPZ_AT	    *abase;
    OPV_BMVARS	    *bmvars;	    /* bitmap to determine attribute subset
				    ** of equivalence class to use */
    OPO_CO	    *func_cop;      /* this COP provides the scope in which the
                                    ** function attribute will be evaluated */
    bool	    forcetemp;
    i4		    tempno;
    bool	    result_tab;
    bool	    first_time;
    i4		    new_attno;
    bool	    haveone;
    ULM_SMARK	    mark;
    OPV_BMVARS	    temp_bmvars;

    textcb = (OPC_TTCB *) ttcbptr->handle;
    subquery = textcb->subquery;
    if ( textcb->qryhdr->opq_q11_duplicate == OPQ_2DUP )
    {
	opc_pttresname(ttcbptr, atno); /* print resdom name for special case
				    ** non-printing resdom temporary */
	return;
    }
    abase = subquery->ops_attrs.opz_base;
    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[
	abase->opz_attnums[atno]->opz_equcls]->ope_attrmap;
    switch (textcb->joinstate)
    {
    case OPT_JOUTER:		    /* consider only attributes from the outer
                                    ** for this variable in the join */
	func_cop = textcb->cop->opo_outer;
	bmvars = func_cop->opo_variant.opo_local->opo_bmvars;
	textcb->joinstate = OPT_JINNER; /* next attribute from inner */
	break;
    case OPT_JINNER:		    /* consider only attributes from the inner
                                    ** for this variable in the join */
	func_cop = textcb->cop->opo_inner;
	bmvars = func_cop->opo_variant.opo_local->opo_bmvars;
	textcb->joinstate = OPT_JNOTUSED; /* no next attribute */
	break;
    case OPT_JNOTUSED:
    {
	func_cop = textcb->cop;
	bmvars = func_cop->opo_variant.opo_local->opo_bmvars;
        if ( textcb->qryhdr->opq_q9_flattened
	    &&
	    (textcb->cop == textcb->subquery->ops_bestco ) /* processing top
				    ** node of query plan */
	    &&
	    (	subquery->ops_vars.opv_base->opv_rt[varno]->opv_gvar 
		== 
		textcb->state->ops_qheader->pst_restab.pst_resvno
	    )
	    )
	{   /* if an "EXISTS" type where clause is being generated then
	    ** choose the result variable if possible */
	    bmvars = &temp_bmvars;
	    MEfill(sizeof(temp_bmvars), (u_char)0, (char *)bmvars);
	    BTset((i4)varno, (char *)bmvars);
	}
	break;
    }
    case OPT_JBAD:
    default:
	opx_error(E_OP068B_BAD_VAR_NODE);
    }


	first_time = TRUE;

	/*
	** When generating a target list as part of SQL text for distributed, only need one 
	** attribute for each equivalence class - DRH
	*/


	for (haveone = FALSE, new_attno = -1;
	     ((new_attno = BTnext((i4)new_attno, (char *)attrmap, (i4)OPZ_MAXATT)) >= 0
	      && !haveone);)
	{   /* look thru the list of joinop attribute associated with this
	    ** equivalence class and determine which ones exist in the
	    ** subtree associated with this CO node */
	    OPZ_ATTS		*attrp;
	    bool		have_func;

	    attrp = abase->opz_attnums[new_attno];
	    have_func = FALSE;

	    if ((   (attrp->opz_attnm.db_att_id != OPZ_FANUM)
		    &&
		    BTtest((i4)attrp->opz_varnm, (char *)bmvars) /* if this is a non function
						** attribute then the var needs to be available
                                                ** for this attribute to be included */
                )
                ||
	        (   (attrp->opz_attnm.db_att_id == OPZ_FANUM)
		    &&
		    BTtest((i4)attrp->opz_varnm, (char *)bmvars)
		    &&
		    (have_func = opc_fchk(textcb, new_attno, func_cop)) /* check
						** if enough equivalence classes exist 
                                                ** for evaluation of the function attribute 
						** and funcatt matches var map */
                )
	       )
	    { /* range variable for this attribute is in subtree */

		/* Bug 78930. If the attribute we found is not nullable but the result attribute is, 
		** then continue searching.
		*/
		if ( (attrp->opz_dataval.db_datatype !=
		        subquery->ops_attrs.opz_base->opz_attnums[atno]->opz_dataval.db_datatype )
		&& (ttcbptr->uld_mode == PSQ_DEFCURS))
		 continue;

		haveone = TRUE;
		opc_nat(textcb, subquery->ops_global, attrp->opz_varnm, 
		    &tablename, &aliasname, &tempno, &result_tab, 
		    ttcbptr->uld_mode, &forcetemp); /* get the table name */

		opc_atname(textcb, subquery, new_attno, 
		    forcetemp, &attrname); /* get the attribute name */

		   /* second pass used allocated memory */
		    if (first_time)
			first_time = FALSE;
		    else
			opcu_printf(ttcbptr, ",");	/* separate names in same equivalence class
							** by commas */

		    /* Check for the case where we have a function attribute referenced in a 
		       temp table, and that function attribute is a constant.  If so, we will go 
		       ahead and evaluate the expression (see below) and thus output the constant 
		       instead of a temp table column reference.  This is needed to handle the 
		       constants that opj_cartprod() inserts into the tree (bug 67299). */

		    if (have_func && forcetemp)
		    {
	    		OPZ_FATTS	*fattrp;
			PST_QNODE       *qroot;

			fattrp = subquery->ops_funcs.opz_fbase->opz_fatts[attrp->opz_func_att];
		        qroot = fattrp->opz_function->pst_right;
			if (qroot->pst_sym.pst_type == PST_CONST  /* if a constant */
			    &&
			    !(qroot->pst_sym.pst_value.pst_s_cnst.pst_parm_no)) /* and not 
									a repeat query parm */
			{
			    forcetemp = FALSE;  /* allow call to opc_funcatt(), below */
			}
		    }
			

		    if (have_func && !forcetemp )
		    {	/* move the function attribute text to the variable text descriptor
                        ** Note - do not evaluate the function attribute again if the
			** attribute is coming from a temp */

			opcu_flush( ttcbptr, NULL );
			opc_funcatt( textcb, new_attno, func_cop, &mark );
			opcu_flush( ttcbptr, NULL );
		    }
		    else
		    {
			if ((!textcb->noalias) )
			{
			    opcu_printf( (ULD_TTCB *) ttcbptr, (char *) &aliasname );
			    opcu_printf( ttcbptr, "." );
			}
			opcu_printf( ttcbptr, (char *) &attrname );
		    }
		}
	    }
}


/*{
** Name: OPCU_PRVAR  - add variable name to string
**
** Description:
**      Insert description of variable into text string
**
** Inputs:
**      ttcbptr                          ptr to state variable
**      varp                            ptr to var node
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	31-mar-88 (seputis)
**          initial creation
**	20-may-94 (davebf)
**	    added support for singlesite nested queries
*/
static VOID
opcu_prvar(
	ULD_TTCB	*ttcbptr,
	PST_QNODE	*varp )
{
    OPC_TTCB		*textcb = (OPC_TTCB *) ttcbptr->handle;
    OPS_SUBQUERY	*subquery = textcb->subquery;
    OPS_STATE		*global = (OPS_STATE *)subquery->ops_global;
    OPV_IGVARS          vno = varp->pst_sym.pst_value.pst_s_var.pst_vno;
    DB_ATTNUM           att_id = varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
    OPZ_ATTS		*attp;
    OPV_VARS		*opvvar;
    OPV_SUBSELECT	*opvsub;
    OPV_GRV		*opvgrv;
    OPV_GSUBSELECT	*opvgsub;
    QEF_AHD		*actptr;
    QEQ_D1_QRY		*qryptr;
    DD_PACKET		*pktptr;
    DD_PACKET		*ddp;
    DD_PACKET		*newpkt;
    bool		is_sq_var = FALSE;

    /* Sanity check for likely sub-query reference */
    if (subquery->ops_attrs.opz_base &&
	att_id < subquery->ops_attrs.opz_av &&
        subquery->ops_attrs.opz_base &&
        (attp = subquery->ops_attrs.opz_base->opz_attnums[att_id]) &&
        subquery->ops_vars.opv_base &&
        attp->opz_varnm < subquery->ops_vars.opv_rv &&
        (opvvar = subquery->ops_vars.opv_base->opv_rt[attp->opz_varnm]) &&
        (opvsub = opvvar->opv_subselect))  /* not a subselect var */
    {
	OPS_SUBQUERY *sq = global->ops_osubquery;
	/* Possible sub-query reference - look for ops_graft pointer
	** tou our var node. */
	while (sq)
	{
	    if (sq->ops_agg.opa_graft &&
		*sq->ops_agg.opa_graft == varp)
	    {
		is_sq_var = TRUE;
		break;
	    }
	    sq = sq->ops_next;
	}
	
    }
    if (!is_sq_var)
    {
	opc_varnode(ttcbptr, vno, att_id);
    }
    else			/* this is a subselect var */
    {
	opvgrv = global->ops_rangetab.opv_base->opv_grv[opvsub->opv_sgvar];
	opvgsub = opvgrv->opv_gsubselect;
	actptr = opvgsub->opv_ahd;
	qryptr = &actptr->qhd_obj.qhd_d1_qry;

	/* bracket and retrieve subselect text */
	opcu_printf(ttcbptr, "(");
	opcu_flush(ttcbptr, NULL);
    
	/* find end of current seglist */
	for (pktptr = textcb->seglist->qeq_s2_pkt_p;
		pktptr->dd_p3_nxt_p != NULL;
		pktptr = pktptr->dd_p3_nxt_p );

	/* make copy of packet list from subselect in case its reused
	** and attach it to seglist */
	for ( ddp = qryptr->qeq_q4_seg_p->qeq_s2_pkt_p;
		ddp != NULL; ddp = ddp->dd_p3_nxt_p )
	{
	    newpkt = (DD_PACKET *) opu_qsfmem(global,
				    (i4) (sizeof(DD_PACKET) + ddp->dd_p1_len));
	    newpkt->dd_p1_len = ddp->dd_p1_len;
	    newpkt->dd_p2_pkt_p = (char *) newpkt + sizeof( DD_PACKET);
	    newpkt->dd_p4_slot = DD_NIL_SLOT;
	    MEcopy( (PTR) ddp->dd_p2_pkt_p,
		    ddp->dd_p1_len,
		    (PTR) newpkt->dd_p2_pkt_p );
	    newpkt->dd_p3_nxt_p = (DD_PACKET *) NULL;
	    pktptr->dd_p3_nxt_p = newpkt;
	    pktptr = newpkt;
	}

	opcu_printf(ttcbptr, ")");


    }
}

/*
** {
** Name: opcu_prconst	- convert constant to text
**
** Description:
**      This routine will convert the internal representation of a
**      constant to a text form and insert it into the query text
**      string
**
** Inputs:
**      ttcbptr                          global state variable
**      constp                          ptr to constant node
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    places constant text form into query text string
**
** History:
**      2-apr-88 (seputis)
**          initial creation
**      30-jan-89 (robin)
**	    Modified to use constant text saved by the parser if
**	    available.
**      06-feb-89 (robin)
**	    Trim trailing white space after TMcvt call.
**	20-feb-89 (robin)
**	    Print ~V<no> if parser supplied non-zero parm number
**      22-feb-89 (robin)
**	    Always append a space after a ~V to handle an
**	    idiosyncracy (bug) in the scanner.
**      31-may-91 (seputis)
**          fix bug 37850 - negative sign ignored in star
**       6-Oct-1993 (fred)
**          Added byte string datatypes support.
**	20-may-94 (davebf)
**	    added support for singlesite nested queries
**	06-apr-01 (inkdo01)
**	    Changed adc_tmlen parms from *i2 to *i4.
**	11-jun-2001 (somsa01)
**	    To save stack space, dynamically allocate space for convbuf.
**	16-jan-06 (hayke02)
**	    Compare worst_width to DB_MAXSTRING rather than sizeof(convbuf),
**	    now that convbuf is a pointer. This  change fixes bug 113941.
**      18-dec-2009 (maspa05) bug 122893
**          check for boolean values generated by constant folding and put
**          back an appropriate expression (1 = 1) or (1 = 0) so that we
**          don't generate syntax the local DBMS will fail on.
[@history_template@]...
*/
static VOID
opcu_prconst(
	ULD_TTCB        *ttcbptr,
	PST_QNODE	*constp )
{
    OPC_TTCB		*textcb = (OPC_TTCB *)ttcbptr->handle;
    OPS_SUBQUERY	*subquery = (OPS_SUBQUERY *)textcb->subquery;
    OPS_STATE		*global = subquery->ops_global;
    OPV_GRV		**opvgrv_p;
    OPV_SEPARM		*separm;
    OPT_NAME		aname;		   
    OPT_NAME		*attrname = &aname;
    OPT_NAME		aliasname;
    DD_CAPS		*cap_ptr;
    OPD_SITE		*sitedesc;
    OPD_ISITE   	site;
    i4			atno; 
    i4			vno; 
    i4			length;
    char		*att_name;
    int			i;
    bool		found = (bool)FALSE;


    if(global->ops_qheader->pst_distr->pst_stype & PST_1SITE)
    {
	
	for(opvgrv_p = global->ops_rangetab.opv_base->opv_grv, i = 0;
	    i < global->ops_rangetab.opv_gv;
	    ++i)
	{
	    if(opvgrv_p[i] == (OPV_GRV *)NULL)  
		continue;				/* sparse array */
	    if(!(opvgrv_p[i]->opv_gsubselect))
		continue;				/* not a subselect */
	
	    for(separm = opvgrv_p[i]->opv_gsubselect->opv_parm;
		separm;
		separm = separm->opv_senext)
	    {
		if(constp == separm->opv_seconst)	/* this is our constant */
		{
		    found = TRUE;
		    break;
		}
	    }
	    if(found) break;
	}
	if(found)
	{
	    /* simulate opcu_prvar with separm->opv_sevar */
	    
	    vno =  
	      separm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_vno;
	    atno = 
	      separm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;

	    /* construct alias name from global var num*/
	    STprintf((PTR) &aliasname, "t%d", vno );  

	    /* copy the attribute name to the output buffer */
	    /* adapted from opc_atname */

	    if ( sizeof(DB_ATT_NAME) >= sizeof(OPT_NAME))
		length = sizeof(OPT_NAME)-1;
	    else
		length = sizeof(DB_ATT_NAME);

	    if (opvgrv_p[vno]->opv_relation->
		rdr_obj_desc->dd_o9_tab_info.dd_t6_mapped_b )
	    {
		att_name = &opvgrv_p[vno]->opv_relation->rdr_obj_desc->
		    dd_o9_tab_info.dd_t8_cols_pp[atno]->
		    dd_c1_col_name[0];	
	    }
	    else
	    {
		att_name = (char *)&opvgrv_p[vno]->opv_relation->
		    rdr_attr[atno]->att_name.db_att_name[0];
	    }
	    length = opt_noblanks((i4)length, (char *)att_name);

	    site = textcb->cop->opo_variant.opo_local->opo_operation;
	    sitedesc = textcb->state->ops_gdist.opd_base->opd_dtable[site];
	    cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps;

	    if (cap_ptr->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
	    {
		u_i4	    unorm_len = sizeof(OPT_NAME) -1;
		DB_STATUS	    status;
		DB_ERROR	    error;

		/* Unnormalize and delimit column name */
		status = cui_idunorm( (u_char *)att_name, (u_i4*)&length,
				(u_char *)attrname, &unorm_len, CUI_ID_DLM, 
				&error);

		if (DB_FAILURE_MACRO(status))
		    opx_verror(E_DB_ERROR, E_OP08A4_DELIMIT_FAIL, error.err_code);

		attrname->buf[unorm_len]=0;
	    }
	    else
	    {
		MEcopy((PTR)att_name, length, (PTR)attrname);
		attrname->buf[length]=0;    	/* null terminate the string */
	    }
	    if (!textcb->noalias)
	    {
		opcu_printf((ULD_TTCB *) ttcbptr, (char *) &aliasname );
		opcu_printf(ttcbptr, "." );
	    }
	    opcu_printf(ttcbptr, (char *) attrname );
	    return;
	}
	/* else assume a normal constant */
    }


    if (constp->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
    {	/* a repeat parameter exist */
  	     opcu_printf(ttcbptr, " ~V");
	     opcu_intprintf(ttcbptr, 
		  (i4)( constp->pst_sym.pst_value.pst_s_cnst.pst_parm_no - 1));
	     opcu_printf(ttcbptr, " ");
    }
    else if (constp->pst_sym.pst_value.pst_s_cnst.pst_origtxt != NULL)
    {
	/*
	**  parser saved the user-specified constant text so use it
	*/
	bool	    negative;
	PTR	    dataptr;

	negative = FALSE;
	/* print out negative sign if appropriate */
	if (constp->pst_sym.pst_dataval.db_datatype == DB_INT_TYPE)
	{
	    dataptr = constp->pst_sym.pst_dataval.db_data;
	    switch (constp->pst_sym.pst_dataval.db_length)
	    {
	    case 1:
		if (I1_CHECK_MACRO(((DB_ANYTYPE *)dataptr)->db_i1type) < 0)
		    negative = TRUE;
		break;

	    case 2:
		if (((DB_ANYTYPE *)dataptr)->db_i2type < 0)
		    negative = TRUE;
		break;

	    case 4:
		if (((DB_ANYTYPE *)dataptr)->db_i4type < 0)
		    negative = TRUE;
		break;
	    default:
		opcu_msignal(ttcbptr, E_OP0799_LENGTH, E_DB_ERROR, 0);
	    }
	}
	else if (constp->pst_sym.pst_dataval.db_datatype == DB_FLT_TYPE)
	{
	    dataptr = constp->pst_sym.pst_dataval.db_data;
	    switch (constp->pst_sym.pst_dataval.db_length)
	    {
	    case 4:
		if (((DB_ANYTYPE *)dataptr)->db_f4type < 0.0)
		    negative = TRUE;
		break;

	    case 8:
		if (((DB_ANYTYPE *)dataptr)->db_f8type < 0.0)
		    negative = TRUE;
		break;
	    default:
		opcu_msignal(ttcbptr, E_OP0799_LENGTH, E_DB_ERROR, 0);
	    }
	}
	if (negative)
	    opcu_printf(ttcbptr, "-");
	opcu_printf( ttcbptr, constp->pst_sym.pst_value.pst_s_cnst.pst_origtxt );
    }
    else
    {
	i4                  default_width;
	i4                  worst_width;
	{
	    DB_STATUS           tmlen_status;
	    tmlen_status = adc_tmlen( ttcbptr->adfcb, &constp->pst_sym.pst_dataval, 
		    &default_width, &worst_width);
	    if (DB_FAILURE_MACRO(tmlen_status))
		opcu_msignal(ttcbptr, E_UL0300_BAD_ADC_TMLEN, tmlen_status,
		    ttcbptr->adfcb->adf_errcb.ad_errcode);
	}

	{
	    DB_STATUS           tmcvt_status;
	    i4			outlen;
	    DB_DATA_VALUE       dest_dt;
	    i4			datatype;
	    u_i4		trimlen;
	    char		*convbuf;

	    convbuf = (char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);

	    if ((worst_width-1) > DB_MAXSTRING)
		opcu_msignal(ttcbptr, E_UL0302_BUFFER_OVERFLOW, E_DB_ERROR, 0);
	    dest_dt.db_length = default_width;
	    dest_dt.db_data = (PTR)&convbuf[0];
	    tmcvt_status= adc_tmcvt( ttcbptr->adfcb, 
		&constp->pst_sym.pst_dataval, &dest_dt, &outlen); /* use the
					** terminal monitor conversion routines
					** to display the results */
	    if (DB_FAILURE_MACRO(tmcvt_status))
	    {
		MEfree((PTR)convbuf);
		opcu_msignal(ttcbptr, E_UL0301_BAD_ADC_TMCVT, tmcvt_status,
		    ttcbptr->adfcb->adf_errcb.ad_errcode);
	    }

	    convbuf[outlen]=0;		/* null terminate the buffer before
					** printing */
	    trimlen = STtrmwhite(&convbuf[0]);
	    /* FIXME - do we need to look for SQL NULLs */
	    if ((datatype = constp->pst_sym.pst_dataval.db_datatype) < 0 )
		datatype = -datatype;	/* nullable datatypes are negative*/
	    switch (datatype)
	    {	/* FIXME - need to check for language in order to construct
		** the correct string constant delimiters */
		case DB_CHR_TYPE:
		case DB_CHA_TYPE:
		case DB_TXT_TYPE:
		case DB_VCH_TYPE:
	        case DB_BYTE_TYPE:
	        case DB_VBYTE_TYPE:
	        {
		    char	*quoted; /* quoted string delimiter */
		    switch (ttcbptr->language)
		    {	/* find quoted string delimiter */
		    case DB_SQL:
		    case DB_NOLANG:
			quoted = "\'";
			break;
		    case DB_QUEL:
			quoted = "\"";
			break;
		    default:
			MEfree((PTR)convbuf);
			opcu_msignal(ttcbptr, E_UL030A_BAD_LANGUAGE, E_DB_ERROR,
				    0);
		    }
		    opcu_printf(ttcbptr, quoted);
		    opcu_printf(ttcbptr, &convbuf[0]);
		    opcu_printf(ttcbptr, quoted);
		    break;
		}
		default:
		    opcu_printf(ttcbptr, &convbuf[0]);
	    }

	    MEfree((PTR)convbuf);
	}
    }
}

/*{
** Name: opc_uop	- text generation for uop operator
**
** Description:
**      This routine will generate text for the PST_UOP operator.  Note
**	that since prexpr is highly recursive it is good to have non-recursive
**	routines which process the nodes, so that the stack usage for local
**	variables is not cumulative.
**
** Inputs:
**      ttcbptr                         text generation state variable
**      qnode                           PST_UOP parse tree node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places equivalent text in output buffer pointed at by ttcbptr
**
** History:
**      20-mar-89 (seputis)
**          initial creation, use new ADF routines for proper text generation
**      09-oct-2009 (huazh01)
**          Both int(), int4() and integer() are defined as ADI_I4_OP. Use
**          'int4()' when compiling query text for these functions. Earlier
**          version of Ingres may not have int() and integer(). (b122218)
[@history_template@]...
*/
static VOID
opc_uop(
	ULD_TTCB           *ttcbptr,
	PST_QNODE          *qnode )
{
    DB_STATUS	    status;
    ADI_OPINFO	    adi_info;
    bool	    brackets;
    DD_CAPS     *cap_ptr = NULL;
    OPD_ISITE   site;
    OPD_SITE   *sitedesc;
    OPC_TTCB    *textcb;

    textcb = (OPC_TTCB *) ttcbptr->handle;

    if (textcb->cop != (OPO_CO *) NULL)
    {
        site = textcb->cop->opo_variant.opo_local->opo_operation;
        sitedesc = textcb->state->ops_gdist.opd_base->opd_dtable[site];
        cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps;
    }

    status = adi_op_info(ttcbptr->adfcb, 
	qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_info);
    if (DB_FAILURE_MACRO(status))
	opcu_msignal(ttcbptr, E_UL0304_ADI_OPNAME, status, 
	    ttcbptr->adfcb->adf_errcb.ad_errcode);
    switch (adi_info.adi_optype)
    {
    case ADI_COMPARISON:
    case ADI_OPERATOR:
    case ADI_COERCION:
    case ADI_COPY_COERCION:
	brackets = FALSE;
	break;
    case ADI_AGG_FUNC:
    case ADI_NORM_FUNC:
	brackets = TRUE;
	if (adi_info.adi_opid == ADI_UNORM_OP)
	    brackets = FALSE;
	break;
    default:
	opx_error(E_OP0798_OP_TYPE);
    }
    switch (adi_info.adi_use)
    {
    case ADI_PREFIX:
    {
	if (adi_info.adi_opid != ADI_UNORM_OP)
	{
            if (adi_info.adi_opid == ADI_I4_OP &&
                cap_ptr && 
                cap_ptr->dd_c1_ldb_caps & DD_2CAP_INGRES) 
            {
               opcu_printf(ttcbptr, "int4");
            }
            else
               opcu_printf(ttcbptr, (char *)&adi_info.adi_opname);
	}
	if (brackets)
	    opcu_printf(ttcbptr, "(");
	else
	    opcu_printf(ttcbptr, " "); 
	opcu_prexpr(ttcbptr, qnode->pst_left);
	if (brackets)
	    opcu_printf(ttcbptr, ")");
	else
	    opcu_printf(ttcbptr, " ");
	break;
    }
    case ADI_POSTFIX:
    {
	if (brackets)
	    opcu_printf(ttcbptr, "(");
	else
	    opcu_printf(ttcbptr, " "); /* important to use only
				** blanks for the IS NULL case */
	opcu_prexpr(ttcbptr, qnode->pst_left);
	if (brackets)
	    opcu_printf(ttcbptr, ")");
	else
	    opcu_printf(ttcbptr, " ");
	opcu_printf(ttcbptr, (char *)&adi_info.adi_opname);
	break;
    }
    case ADI_INFIX:
    default:
	opx_error(E_OP0797_OP_USE);
    }
}

/*{
** Name: opc_bop	- text generation for bop operator
**
** Description:
**      This routine will generate text for the PST_BOP operator.  Note
**	that since prexpr is highly recursive it is good to have non-recursive
**	routines which process the nodes, so that the stack usage for local
**	variables is not cumulative.
**
** Inputs:
**      ttcbptr                         text generation state variable
**      qnode                           PST_BOP parse tree node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places equivalent text in output buffer pointed at by ttcbptr
**
** History:
**      20-mar-89 (seputis)
**          initial creation, use new ADF routines for proper text generation
**	11-nov-91 (fpang)
**          Check for 'like', check for isescape, and tack the escape part on
**	    if present.
**	    Fixes B40753.
**	10-oct-93 (tam)
**	    For decimal() function in star, decode the right node constant 
**	    (which is the encoded form of precision, scale) into two user 
**	    arguments. The local server, on receipt of the query, expects to 
**	    see the decoded form of precision and scale. The two arguments 
**	    are added to the user argument list.  (b55511)
**	20-may-94 (davebf)
**	    added support for singlesite nested queries
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	    Widened pst_escape.
**	05-Sep-2008 (kiria01) SIR120473
**	    Correct flag test for escape and use opid to scope like operators.
**      14-aug-2009 (huazh01)
**          add support for generating text for "in-list" constants values.
**          (b122391)
**	05-Nov-2009 (kiria01) b122822
**	    Support PST_INLIST for NOT IN as well as IN.
[@history_template@]...
*/
static VOID
opc_bop(
	ULD_TTCB           *ttcbptr,
	PST_QNODE          *qnode )
{
    DB_STATUS	    status;
    ADI_OPINFO	    adi_info;
    i4		    i;
    u_i4	    pat_flags;
    PST_QNODE       *cnst;
    bool            in_list = FALSE;

    status = adi_op_info(ttcbptr->adfcb, 
	qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_info);
    if (DB_FAILURE_MACRO(status))
	opcu_msignal(ttcbptr, E_UL0304_ADI_OPNAME, status, 
	    ttcbptr->adfcb->adf_errcb.ad_errcode);

    switch (adi_info.adi_use)
    {
    case ADI_INFIX:
    {	/* infix operator found */
	opcu_printf(ttcbptr, "(");

#if 0
	/* this code might be useful if gateways object to the
	** 1 = ANY (select any(*)..... form generated below    */
	if(qnode->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_NOMETA
	&& qnode->pst_left->pst_sym.pst_type == PST_CONST
	&& qnode->pst_left->pst_sym.pst_value.pst_s_cnst.pst_origtxt 
		!= (PTR)NULL)
	{
	    /* we have an [not]exists */   /* good assumption???? */

	    CVan(qnode->pst_left->pst_sym.pst_value.pst_s_cnst.pst_origtxt, &i);
	    if(i = 1)
		opcu_printf(ttcbptr, " EXISTS ");
	    else
		opcu_printf(ttcbptr, " NOT EXISTS ");
	}
	else
#endif

	    opcu_prexpr(ttcbptr, qnode->pst_left);

	opcu_printf(ttcbptr, " ");	    /* add blanks for operators
					** such as "like" */

	pat_flags = qnode->pst_sym.pst_value.pst_s_op.pst_pat_flags;

        if ((qnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) &&
		(qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_EQ_OP ||
		 qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_NE_OP) &&
		qnode->pst_right->pst_sym.pst_type == PST_CONST)
        {
           in_list = TRUE;
	   if (qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_NE_OP)
		opcu_printf(ttcbptr, "NOT ");
           opcu_printf(ttcbptr, "IN(");
           cnst = qnode->pst_right;
           while (cnst)
           {
               if (cnst != qnode->pst_right)
                    opcu_printf(ttcbptr, ",");
               opcu_prconst(ttcbptr, cnst);
               cnst = cnst->pst_left;
           }
           opcu_printf(ttcbptr, ")");
        }
	else if (qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_LIKE_OP ||
	    qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_NLIKE_OP)
	{
	    u_i4 typ = (pat_flags & AD_PAT_FORM_MASK);
	    static char likes[][16] = {
#define _DEFINE(n,v,s) {s},
#define _DEFINEEND
		AD_PAT_FORMS
#undef _DEFINEEND
#undef _DEFINE
	    };
	    if (pat_flags & AD_PAT_FORM_NEGATED)
		opcu_printf(ttcbptr, "NOT ");
	    if (typ >= AD_PAT_FORM_MAX)
		typ = AD_PAT_FORM_LIKE;
	    opcu_printf(ttcbptr, likes[typ>>8]);
	}
	else
	    opcu_printf(ttcbptr, (char *)&adi_info.adi_opname);

	opcu_printf(ttcbptr, " ");

	if(qnode->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_NOMETA)
	{
	    if(qnode->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_ANY)
	    {
		opcu_printf(ttcbptr, " ANY ");
	    }
	    if(qnode->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_ALL)
	    {
		opcu_printf(ttcbptr, " ALL ");
	    }
	}

      	if (!in_list) 
           opcu_prexpr(ttcbptr, qnode->pst_right);

	/* Tack on the escape part if it is present.*/
	if (pat_flags & AD_PAT_HAS_ESCAPE)
	{
	    char escape_str[20];
	    char *d = escape_str;
	    if ((pat_flags & AD_PAT_ISESCAPE_MASK) == AD_PAT_HAS_UESCAPE)
	    {
		u_i4 s = qnode->pst_sym.pst_value.pst_s_op.pst_escape;
		STprintf( escape_str, "U&'\%04x' ", s);
	    }
	    else
	    {
		u_char *s = (u_char*)&qnode->pst_sym.pst_value.pst_s_op.pst_escape;
		*d++ = '\'';
		CMcpyinc(s,d);
		*d++ = '\'';
		*d++ = ' ';
		*d++ = EOS;
	    }
	    opcu_printf(ttcbptr, " ESCAPE ");
	    opcu_printf(ttcbptr, escape_str);
	}
	if (pat_flags & AD_PAT_WO_CASE)
	    opcu_printf(ttcbptr, " WITHOUT CASE");
	if (pat_flags & AD_PAT_WITH_CASE)
	    opcu_printf(ttcbptr, " WITH CASE");

	opcu_printf(ttcbptr, ")");
	break;
    }
    case ADI_PREFIX:
    {	/* prefix operator */
	opcu_printf(ttcbptr, (char *)&adi_info.adi_opname);
	opcu_printf(ttcbptr, "(");
	opcu_prexpr(ttcbptr, qnode->pst_left);
	opcu_printf(ttcbptr, ",");

	if (qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_DEC_OP)
	{
	    /* b55511 decimal() function in star */

            OPC_TTCB        *textcb;
	    OPS_STATE       *global;
	    PST_QNODE	    *newqnode;
	    DB_DATA_VALUE   *dataval;
	    i2		    *i2_int, prec_scale;
	    
	    textcb = (OPC_TTCB *)ttcbptr->handle;
	    global = textcb->state;

	    /* 
	    ** right node must be the encoded precision_scale, i2 constant; 
	    ** decode this constant to obtain precision, scale
	    */
	    dataval = &qnode->pst_right->pst_sym.pst_dataval;
	    i2_int	= (i2 *) dataval->db_data;
	    prec_scale	= *i2_int;

	    /*
	    ** overwrite right node dbvalue with decoded precision 
	    */
	    *i2_int	= DB_P_DECODE_MACRO(prec_scale);

	    /*
	    ** add parameter to user argument list for decoded scale 
	    */ 
	    newqnode = opv_intconst(global, DB_S_DECODE_MACRO(prec_scale));
	    opv_uparameter(global, newqnode);
	    /*
	    ** opv_uparameter only increments global->ops_parmtotal;
	    ** so also increment global->ops_gdist.opd_user_parameters
	    */
	    global->ops_gdist.opd_user_parameters++;

	    opcu_prexpr(ttcbptr, qnode->pst_right);
	    opcu_printf(ttcbptr, ",");
	    opcu_prexpr(ttcbptr, newqnode);
	    opcu_printf(ttcbptr, ")");
	    break;
	}

	/* normal BOP function procession */

	opcu_prexpr(ttcbptr, qnode->pst_right);
	opcu_printf(ttcbptr, ")");
	break;
    }
    case ADI_POSTFIX:
    default:
	opcu_msignal(ttcbptr, E_UL0303_BQN_BAD_QTREE_NODE, E_DB_ERROR, 0);
    }
}


/*{
** Name: opc_mop	- text generation for mop operator
**
** Description:
**      This routine will generate text for the PST_MOP operator.
**	This node represents n-ary operator or function parameters. Note
**	that since prexpr is highly recursive it is good to have non-recursive
**	routines which process the nodes, so that the stack usage for local
**	variables is not cumulative.
**
** Inputs:
**      ttcbptr                         text generation state variable
**      qnode                           PST_MOP parse tree node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places equivalent text in output buffer pointed at by ttcbptr
**
** History:
**      22-Sep-2006 (kiria01) b116669
**          initial star support for MOP nodes
*/
static VOID
opc_mop(
         ULD_TTCB           *ttcbptr,
         PST_QNODE          *qnode )
{
    DB_STATUS       status;
    ADI_OPINFO      adi_info;
    i4              i = 1;
    i4              opno = qnode->pst_sym.pst_value.pst_s_op.pst_opno; /* ADI_DEC_OP */

    /* Determine the characteristics of the operator/function */
    status = adi_op_info(ttcbptr->adfcb, opno, &adi_info);
    if (DB_FAILURE_MACRO(status))
        opcu_msignal(ttcbptr, E_UL0304_ADI_OPNAME, status, 
                     ttcbptr->adfcb->adf_errcb.ad_errcode);

    /* For traditional functions emit name */
    if (adi_info.adi_use == ADI_PREFIX) opcu_printf(ttcbptr, (char *)&adi_info.adi_opname);
    opcu_printf(ttcbptr, "(");
    /* Output initial parameter */
    opcu_prexpr(ttcbptr, qnode->pst_right);
    while(qnode = qnode->pst_left)
    {
        i++;
        if (adi_info.adi_use == ADI_INFIX)
        {
             /* Surround infix operators with blanks for such as "and" */
            opcu_printf(ttcbptr, " ");
            opcu_printf(ttcbptr, (char *)&adi_info.adi_opname);
            opcu_printf(ttcbptr, " ");

        }
        else if (opno == ADI_SUBSTRING_OP)
        {
            /* Special case SUBSTRING function to support its different syntax */
            if (i == 2)
                opcu_printf(ttcbptr, " FROM ");
            else
                opcu_printf(ttcbptr, " FOR ");
        }
        else
        {
            /* The more usual comma separator */
            opcu_printf(ttcbptr, ",");
        }
        /* Output next parameter - qnode will be PST_OPERAND */
        opcu_prexpr(ttcbptr, qnode->pst_right);
    }
    opcu_printf(ttcbptr, ")");
    if (adi_info.adi_use == ADI_POSTFIX) opcu_printf(ttcbptr, (char *)&adi_info.adi_opname);
}



/*{
** Name: opc_case	- text generation for CASEOP related operators
**
** Description:
**      This routine will generate text for the PST_CASEOP, PST_WHLIST and
**	PST_WHOP operators.
**	These nodes represent both simple and searched CASE expressions but they
**	also represent the NULLIF(), IF() and COALESCE() functions which are all
**	shorthand forms of the searched form. However, we can't distinguish
**	between these as the tree contains only the effective nodes but that is
**	not a problem as we just need functional equivalence so that the tree can
**	be re-constructed at the far end.
**
** Inputs:
**      ttcbptr                         text generation state variable
**      qnode                           PST_MOP parse tree node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places equivalent text in output buffer pointed at by ttcbptr
**
** History:
**      20-Jan-2009 (kiria01) b110832
**          initial star support for CASEOP etc. nodes
*/
static VOID
opc_case(
         ULD_TTCB           *ttcbptr,
         PST_QNODE          *qnode )
{
    DB_STATUS       status;
    PST_QNODE *whlistp = qnode->pst_left;

    opcu_printf(ttcbptr, " CASE ");
    if (qnode->pst_sym.pst_value.pst_s_case.pst_casetype == PST_SIMPLE_CASE)
    {
	opcu_prexpr(ttcbptr, qnode->pst_right);
	while (whlistp && whlistp->pst_right && whlistp->pst_right->pst_left)
	{
	    opcu_printf(ttcbptr, " WHEN ");
	    opcu_prexpr(ttcbptr, whlistp->pst_right->pst_left->pst_right);
	    opcu_printf(ttcbptr, " THEN ");
	    opcu_prexpr(ttcbptr, whlistp->pst_right->pst_right);
	    whlistp = whlistp->pst_left;
	}
    }
    else
    {
	while (whlistp && whlistp->pst_right && whlistp->pst_right->pst_left)
	{
	    opcu_printf(ttcbptr, "WHEN ");
	    opcu_prexpr(ttcbptr, whlistp->pst_right->pst_left);
	    opcu_printf(ttcbptr, " THEN ");
	    opcu_prexpr(ttcbptr, whlistp->pst_right->pst_right);
	    whlistp = whlistp->pst_left;
	}
    }

    if (whlistp && whlistp->pst_right)
    {
	opcu_printf(ttcbptr, " ELSE ");
	opcu_prexpr(ttcbptr, whlistp->pst_right->pst_right);
    }
    opcu_printf(ttcbptr, " END ");
}

/*{
** Name: opc_gatname	- get attribute name given ingres attribute number
**
** Description:
**      Routine will return the name of the attribute, it should not be used
**      for temporary tables, only for ingres tables.
**
** Inputs:
**      global				  context within which to find name
**      gvarno                            global range table number
**      ingres_atno			  ingres attribute number
**	attrname			  location to update with attribute
**					    name
**	cap_ptr			  	  Pointer to capabilities of
**					  target LDB
**
** Outputs:
**      attrname                          Will contain attribute name
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-mar-89 (seputis)
**          Created from opc_atname
**      02-sep-93 (barbara)
**          Delimiting of names is based on existence of DB_DELIMITED_CASE
**          capability in the LDB, not on OPEN/SQL_LEVEL.
[@history_template@]...
*/
VOID
opc_gatname(
	OPS_STATE	   *global,
	OPV_IGVARS	   gvarno,
	DB_ATT_ID	   *ingres_atno,
	OPT_NAME           *attrname,
	DD_CAPS		   *cap_ptr )
{
    OPV_GRV		*grvp;
    i4			dmfattr;
    i4			length;
    char		*att_name;

    dmfattr = ingres_atno->db_att_id;
    if (dmfattr == 0)
    {
	STcopy( "tid ", (PTR)attrname);
	return;
    }
    
    grvp = global->ops_rangetab.opv_base->opv_grv[gvarno];
    if ((grvp->opv_created != OPS_MAIN)
	||
	(dmfattr < 0))
	opx_error(E_OP0A8E_SUB_TYPE);	/* expecting this routine
					** to be used by FOR UPDATE lists
					** and UPDATE CURSOR lists only */
    if ( grvp->opv_relation->rdr_obj_desc->dd_o9_tab_info.dd_t6_mapped_b )
	att_name = (char *) &grvp->opv_relation->rdr_obj_desc->dd_o9_tab_info.
	    dd_t8_cols_pp[ dmfattr ]->dd_c1_col_name[0];	
    else
	att_name = (char *)&grvp->opv_relation->
	    rdr_attr[dmfattr]->att_name.db_att_name[0];

    if ( sizeof(DB_ATT_NAME) >= sizeof(OPT_NAME))
	length = sizeof(OPT_NAME)-1;	/* cannot use more space than
					** available */
    else
	length = sizeof(DB_ATT_NAME);	/* get max length to use */
    length = opt_noblanks((i4)length, (char *)att_name);
    
    if (cap_ptr->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
    {
	u_i4	    unorm_len = sizeof(OPT_NAME)-1;
	DB_STATUS   status;
	DB_ERROR    error;

	status = cui_idunorm( (u_char *)att_name, (u_i4 *)&length,
		(u_char *)attrname, &unorm_len, CUI_ID_DLM, &error);
	if (DB_FAILURE_MACRO(status))
	    opx_verror(E_DB_ERROR, E_OP08A4_DELIMIT_FAIL, error.err_code);
	attrname->buf[unorm_len]=0;
    }
    else
    {
    	MEcopy((PTR)att_name, length, (PTR)attrname);
    	attrname->buf[length]=0;		/* null terminate the string */
    }
}

/*{
** Name: opcu_curval	- handle replace cursor nodes
**
** Description:
**      This routine will print out the text string of the name of
**      a column in the target relation, referenced in an expression
**      of a replace cursor statement 
[@comment_line@]...
**
** Inputs:
**      ttcbptr                         state variable for printing routine
**      atno				attribute number to be printed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-mar-89 (seputis)
**          initial creation
**	01-mar-93 (barbara)
**	    Support for delimited identifiers in Star.  Pass in pointer to
**	    LDB capabilities to opc_gatname.
**      18-feb-94 (ed)
**	    - bug 58993 - delimited ID changes cannot reference 
**	    CO node since typically none exist for replace cursor 
**	    statement, instead the OPD_TSITE constant should be used 
**	    for target site
[@history_line@]...
[@history_template@]...
*/
static VOID
opcu_curval(
	ULD_TTCB	    *ttcbptr,
	PST_QNODE	    *qnode )
{
    OPC_TTCB        *textcb;
    OPS_STATE	    *global;
    DD_CAPS	    *cap_ptr;
    OPT_NAME	    attrname;

    textcb = (OPC_TTCB *)ttcbptr->handle;
    global = textcb->state;
    cap_ptr =  &global->ops_gdist.opd_base->opd_dtable[OPD_TSITE]
	->opd_dbcap->dd_p3_ldb_caps;
    opc_gatname(global, (OPV_IGVARS)global->ops_qheader->pst_restab.pst_resvno, 
	&qnode->pst_sym.pst_value.pst_s_curval.pst_curcol, &attrname, cap_ptr);
    opcu_printf( ttcbptr, (char *) &attrname );
}

/*{
** Name: opcu_prexpr	- convert a query tree expression
**
** Description:
**      Accepts the following query tree nodes 
**
**	expr :		PST_VAR
**		|	expr BOP expr
**		|	expr Uop
**		|	AOP AGHEAD qual
**			  \
**			   expr
**		|	BYHEAD AGHEAD qual
**			/    \
**		tl_clause     AOP
**				\
**				 expr
**		|	PST_CONST
**
**
** Inputs:
**      ttcbptr                          state variable
**      qnode                           query tree node to convert
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-mar-88 (seputis)
**          initial creation
**      18-sep-92 (ed)
**          b44990 - check for special case of count aggregate
**	    ANSI sql does not allow constants in the operand of an aggregate
**      23-may-94 (peters)
**          b51491 - 'select count(distinct 1) from ...' returns an error
**	    because when the querytree is disparsed, a '*' is substituted for
**	    the '1' and the LDB server does not accept 'count (distinct *)'.
**	    Changed the code to catch the case of 'count(distinct constant)'
**	    and use whatever constant appears instead of substituting a '*'.
**	22-Sep-2006 (kiria01) b116669
**	    Hook in star support for MOP nodes.
**      28-Nov-2006 (hanal04) Bug 117147
**          Added empty braces to parameterless PST_COP nodes. Otherwise
**          randomf() gets sent from Star server to DBMS as randomf and
**          reported as an unknown column.
*/
static VOID
opcu_prexpr(
	ULD_TTCB           *ttcbptr,
	PST_QNODE          *qnode )
{
    OPC_TTCB	    *textcb;
    DB_STATUS	    status;
    ADI_OP_NAME	    adi_oname;

    textcb = (OPC_TTCB *)ttcbptr->handle;

    switch (qnode->pst_sym.pst_type)
    {
    case PST_VAR:
	opcu_prvar(ttcbptr, qnode);
	break;
    case PST_OR:
    {
	opcu_printf(ttcbptr, "(");
	opcu_prexpr(ttcbptr, qnode->pst_left);
	opcu_printf(ttcbptr, ") OR (");
	opcu_prexpr(ttcbptr, qnode->pst_right);
	opcu_printf(ttcbptr, ")");
	break;
    }
    case PST_AND:
    {
	if (!qnode->pst_right
	    ||
	    (qnode->pst_right->pst_sym.pst_type == PST_QLEND)
	   )
	    opcu_prexpr(ttcbptr, qnode->pst_left); /* degenerate AND can occur
						** on the right side */
	else
	{
	    opcu_printf(ttcbptr, "(");
	    opcu_prexpr(ttcbptr, qnode->pst_left);
	    opcu_printf(ttcbptr, ") AND (");
	    opcu_prexpr(ttcbptr, qnode->pst_right);
	    opcu_printf(ttcbptr, ")");
	}
	break;
    }
    case PST_MOP:
    {
        opc_mop(ttcbptr, qnode);
        break;
    }
    case PST_BOP:
    {
	opc_bop(ttcbptr, qnode);
	break;
    }
    case PST_UOP:
    {
	opc_uop(ttcbptr, qnode);
	break;
    }

    case PST_COP:
    {
	status = adi_opname(ttcbptr->adfcb, 
	    qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_oname);
	if (DB_FAILURE_MACRO(status))
	    opcu_msignal(ttcbptr, E_UL0304_ADI_OPNAME, status, 
		ttcbptr->adfcb->adf_errcb.ad_errcode);
	opcu_printf(ttcbptr, (char *)&adi_oname);
        /* If we have a COP wiht no parameters add the braces */
        if(qnode->pst_left == NULL && qnode->pst_right == NULL) 
        {
	    opcu_printf(ttcbptr, "()");
        }
	break;
    }

    case PST_CONST:
	opcu_prconst(ttcbptr, qnode);
	break;

    case PST_AOP:
    {
	if (textcb->qryhdr->opq_q11_duplicate == OPQ_1DUP)
	{
	    /*
	    **  A special temp table is being created to preserve
	    **  duplicate semantics. Evaluate all expressions below
	    **  the aggregate node, but not the aggregate itself.
	    */
	    opx_error(1);
	}
	else
	{
	    status = adi_opname( ttcbptr->adfcb,
		qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_oname);
	    if (DB_FAILURE_MACRO(status))
		opcu_msignal(ttcbptr, E_UL0304_ADI_OPNAME, status, 
		    ttcbptr->adfcb->adf_errcb.ad_errcode);

	    if ( qnode->pst_left &&
		    textcb->qryhdr->opq_q13_projfagg == TRUE &&
		    qnode->pst_sym.pst_value.pst_s_op.pst_opno ==
		    textcb->state->ops_cb->ops_server->opg_any)
	    {
		/* 
		** This ANY function aggregate follows a projection
		** subquery.  Generate a '1' to indicate that
		** the ANY aggregate is TRUE, since SQL does not
		** support ANY aggregates.
		*/
		opcu_printf( ttcbptr, " 1 ");
	    }
	    else
	    {
		opcu_printf(ttcbptr, (char *)&adi_oname);
		if (qnode->pst_left)
		{   /* special case hack for count(*) which is returned totally
		    ** in adi_oname */
		    opcu_printf( ttcbptr, "(");
		    if (qnode->pst_sym.pst_value.pst_s_op.pst_distinct == PST_DISTINCT)
		    {
			opcu_printf( ttcbptr, " distinct ");
		    }
		    if ((qnode->pst_left->pst_sym.pst_type == PST_CONST)
			&&
			(
			    (   qnode->pst_sym.pst_value.pst_s_op.pst_opno 
				==
				textcb->state->ops_cb->ops_server->opg_count
			    )
			    ||
			    (   qnode->pst_sym.pst_value.pst_s_op.pst_opno 
				==
				textcb->state->ops_cb->ops_server->opg_scount
			    )
			)
		        &&
			(   /* Not allowed to have 'count(distinct *)', so
			    ** don't substitute '*' for a PST_CONST if this
			    ** is a distinct-query--use whatever is in
			    ** qnode->pst_left, instead.
			    */
			    qnode->pst_sym.pst_value.pst_s_op.pst_distinct
			    !=
			    PST_DISTINCT
			)
		       )
			opcu_printf( ttcbptr, "*"); /* do not use constants
					    ** in the operand of an aggregate
					    ** if at all possible */
		    else
			opcu_prexpr( ttcbptr, qnode->pst_left );
		    opcu_printf( ttcbptr, ")");
		}
	    break;
	    }
	}
	break;
    }
    case PST_CURVAL:
    {
	opcu_curval(ttcbptr, qnode);
	break;
    }
    case PST_CASEOP:
    {
	opc_case(ttcbptr, qnode);
	break;
    }
    default:
	opcu_msignal(ttcbptr, E_UL0303_BQN_BAD_QTREE_NODE, E_DB_ERROR, 0);
    }
}

/*{
** Name: opcu_swapf
**
** Description:
[@comment_line@]...
**
**
** Inputs:
**      ttcbptr                          state variable
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
opcu_swapf(
	ULD_TTCB           *ttcbptr )
{
    QEQ_TXT_SEG	*hseg;
    OPC_TTCB	*textcb;    

    /*
    **  Flush current temp buffer text out into the main
    **  list of text segments
    */

    opcu_flush( ttcbptr, (ULD_TSTRING *) NULL);

    textcb = ( OPC_TTCB *) ttcbptr->handle;

    hseg = textcb->hold_targ;
    textcb->hold_targ = textcb->seglist;
    textcb->seglist = hseg;    

}

/*{
** Name: opcu_resright	- print expression for resdom
**
** Description:
**      Given the resdom node, print the expression. 
**
** Inputs:
**      ttcbptr                         ptr to state variable
**      resdomptr                       ptr to resdom to be printed
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-apr-93 (ed)
**          created missing header for function
**      5-apr-93 (ed)
**          -b49609, if temp used to remove duplicates then ignore expressions
**	    and use the attribute of the temp directly
[@history_template@]...
*/
static VOID
opcu_resright( 
	ULD_TTCB	*ttcbptr,
	PST_QNODE	*resdomptr)
{
	OPC_TTCB	*textcb;

	textcb = (OPC_TTCB *) ttcbptr->handle;

	if ((textcb->qryhdr->opq_q11_duplicate == OPQ_1DUP )
	    &&
	    (resdomptr->pst_right->pst_sym.pst_type == PST_AOP))
	{   /* temporary created for non-printing resdoms */
	    resdomptr = resdomptr->pst_right;	/* evaluate aggregate
				    ** expression only at this point, and then
				    ** evaluate aggregate on OPQ_2DUP pass */
	    if (resdomptr->pst_left)
		opcu_prexpr( ttcbptr,resdomptr->pst_left );
	    else
	    {
		/*
		**  This is COUNT * so simply create a
		**  constant value to hold the attribute's
		**  place in the tuple.
		*/

		opcu_printf( ttcbptr, " 1 " );
	    }
	}
	else if ( textcb->qryhdr->opq_q11_duplicate == OPQ_2DUP )
	{   /* a temporary table has been created to remove duplicates
	    ** given that non-printing resdoms exist, the target list
	    ** has already been evaluated, so use the resdom names of
	    ** the target list since the temp table was created with
	    ** these names */
	    if (resdomptr->pst_right->pst_sym.pst_type == PST_AOP)
	    {	/* print the aggregate and reference the temporary attribute
		** created in the OPQ_1DUP pass above */
		PST_QNODE	*qnodep;
		qnodep = resdomptr->pst_right->pst_left;
		if (qnodep)
		{
		    textcb->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
			resdomptr->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
		    resdomptr->pst_right->pst_left = &textcb->qnodes.resexpr;
		    opcu_prexpr( ttcbptr,resdomptr->pst_right ); /* print aggregate
					** referencing temporary table resdom */
		    resdomptr->pst_right->pst_left = qnodep; /* restore expr */
		}
		else
		    opcu_prexpr( ttcbptr,resdomptr->pst_right ); /* for count(*) 
					** avoid var node handling */
	    }
	    else
		opc_pttresname(ttcbptr, (i4)resdomptr->pst_sym.pst_value.
		    pst_s_rsdm.pst_rsno);
	}
	else
	    opcu_prexpr( ttcbptr,resdomptr->pst_right );
}

/*{
** Name: opcu_target	- produce a target list
**
** Description:
**      create a target list text string from the query tree
[@comment_line@]...
**
** Inputs:
**      ttcbptr                          state variable
**      delimiter                       pointer to string which is the delimiter
**                                      between target list elements
**      equals                          ptr to string which is either "=" or "as" -
**					    if =, then 'colname = expr' is generated,
**					    else 'expr as colname' is generated.
**					the element name and its expression
**
** Outputs:
**      targetpp                        NULL if expression list is not required
**                                      to be separated from the element names
**                                      - non-NULL if the expression list is
**                                      required to be separated from the
**                                      respective element names
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
**	14-dec-88 (robin)
**	    Modified to walk the resdom list before using equivalence
**	    classes when building the target list for exterior nodes.
**      25-may-89 (robin)
**	    Added base_colname flag to opc_resdom call to get base column 
**	    names, not resdom names, for insert and update.  Partial fix 
**	    for bug 5638.
**	25-jul-89 (robin)
**	    Changed from 'name = expression' to 'expression as name' per
**	    LRC decision for common SQL.
**	23-aug-89 (robin)
**	    Make 'name = expression' an alternative to 'as'.  Used by
**	    update, etc.
**	20-aug-94 (ed)
**	    - bug 57958, build column array only once since opcu_target can
**	    be called more than once for the same textcb
**	    -Check if internal attribute count is exceeded rather than
**	    trashing ULM memory pointers
**	28-mar-1995 (rudtr01)
**	    fixed bugs 59561 and 67744 by adding PSQ_REPCURS to the
**	    list of actions that require base column names. also 
**	    feeble attempt to make the code more readable.
[@history_template@]...
*/
static VOID
opcu_target(
	ULD_TTCB           *ttcbptr,
	QEQ_TXT_SEG	   **targetpp,
	char		   *delimiter,
	char               *equals )
{
    OPC_TTCB	    *textcb;
    PST_QNODE	    *resdomp;
    char	    namebuf[DB_ATT_MAXNAME*2+3];    
    char	    *namep = namebuf;
    bool	    first_2;
    bool	    first_time;
    bool	    use_list;
    bool	    base_colname;
    i4		    attno;
    i4		    size;
    u_i4	    len;
    bool	    build_colarray; /* TRUE if column array needs to be built */


    textcb = (OPC_TTCB *) ttcbptr->handle;
    textcb->noalias = FALSE;
    textcb->nobraces = TRUE;
    base_colname = FALSE;

    /* 
    ** expression list is required to be separated, so
    ** initialize the beginning of the list 
    */
    if (targetpp)
	*targetpp = NULL;

    first_time = TRUE;
    first_2 = TRUE;
    if ((resdomp = ttcbptr->uld_qroot->pst_left) 
	&& 
	(resdomp->pst_sym.pst_type == PST_RESDOM)
	&&
	(textcb->cop == textcb->subquery->ops_bestco)
	&&
	(textcb->qryhdr->opq_q16_expr_like != OPQ_1ELIKE ))
    {
	use_list = TRUE;	    /* a non-empty target list exists */

    }
    else
    {	/* no target list exists but call resdom routine to get target
        ** lists dynamically built from equivalence class bit maps */
	use_list = FALSE;

    }
    if ( 
	 ( textcb->cop == textcb->subquery->ops_bestco) &&
	 ( ttcbptr->uld_mode == PSQ_APPEND ||
	   ttcbptr->uld_mode == PSQ_REPLACE ||
	   ttcbptr->uld_mode == PSQ_REPCURS
	 ) &&
	 ( textcb->subquery->ops_sqtype == OPS_MAIN ) &&
	 ( textcb->qryhdr->opq_q11_duplicate != OPQ_1DUP )
       )
    {
	base_colname = TRUE;
    }

    resdomp = NULL;
    while ( (resdomp = opc_resdom(ttcbptr, use_list, resdomp, 
				namep, &attno, base_colname ))
	    && 
	    (resdomp->pst_sym.pst_type == PST_RESDOM)
          )
    {	/* target list exists so traverse down the list */

	if ( resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT ||
	     textcb->qryhdr->opq_q11_duplicate == OPQ_1DUP )
	{ 
	    /*
	    **  Either this is a printable resdom, or it's a
	    **  non_printing one that should be printed to
	    **  maintain correct duplicate semantics.
	    */
	    if (first_time)
	    {
		/*
		** do not print the delimiter the first time through
		** but all subsequent times so that we get - 
		** element, element, element 
		*/
		first_time = FALSE;

		/* 
		** bug 57958 build column array only once since opcu_target()
		** can be called more than once with the same textcb 
		*/
		build_colarray = (( textcb->qryhdr->opq_q2_type == OPQ_XFER )
		    &&
		    !textcb->qryhdr->opq_q8_curtable->opq_t5_ncols);
	    }
	    else
	    {
		opcu_printf(ttcbptr, delimiter);
	    }

	    if (targetpp)
	    {   
		/* 
		** the column names are required to be separate from the
		** expressions so create two parallel text streams to
		** collect this information 
		*/
		opcu_printf(ttcbptr, namebuf );	/* add name of target
							list element */
		/* 
		** save the context for the ULD text building
                ** routines so that the expression list can be
                ** build out of line 
		*/
		opcu_swapf(ttcbptr);    

		if (first_2)
		{
		    first_2 = FALSE;
		}
		else 
		{
		    opcu_printf(ttcbptr, delimiter);
		}

		opcu_resright(ttcbptr, resdomp); /* print expression associated 
							with resdom node */
		opcu_swapf(ttcbptr);	/* restore the state for the next 
							element name */
	    }
	    else
	    {
		/* 
		** column names are either not needed, or are adjacent 
		** to the expression which is the right hand side of 
		** the assignment 
		*/
		if (!equals)
		{
		    /*
		    **  column names are not needed
		    */
		    opcu_resright(ttcbptr, resdomp); /* print expression 
							associated with
							   resdom node */
		}
		else if ( *equals == '=' )
		{
		    /*
		    **  Generate colname = expression
		    */
		    opcu_printf(ttcbptr, namebuf );	/* get name of target 
								list element */
		    opcu_printf(ttcbptr, " = ");
		    opcu_resright(ttcbptr, resdomp);	/* print expression 
							associated with
							  ** resdom node */
		}
		else
		{
		    /* 
		    **  Generate expression as colname
		    */
		    opcu_resright(ttcbptr, resdomp); /* print expression 
							associated with
							  ** resdom node */
		    opcu_printf(ttcbptr, " as " );
		    opcu_printf(ttcbptr, namebuf );	/* get name of target 
							list element */
		}
	    }
	
	    if ( build_colarray )
	    {
		i4	    tcolno;

		/* 
		** Add the result name to the column array
		*/
		size = STlength( namep ) +1;	    
		tcolno = ++(textcb->qryhdr->opq_q8_curtable->opq_t5_ncols);
		if (tcolno >= OPZ_MAXATT)
		    opx_error(E_OP0201_ATTROVERFLOW); /* check if too many 
							columns are
							 being defined */
		textcb->qryhdr->opq_q8_curtable->opq_t6_tcols[ (tcolno-1)].opq_c2_col_atnum = attno;
		textcb->qryhdr->opq_q8_curtable->opq_t6_tcols[ tcolno -1].opq_c1_col_name = opu_qsfmem( textcb->state, size );
		STcopy( namebuf, textcb->qryhdr->opq_q8_curtable->opq_t6_tcols[ tcolno -1].opq_c1_col_name);
	    }
	} 
    }
}

/*{
** Name: opcu_winit	- Initialize for WHERE clause build
**
** Description:
**      This routine initializes the global data structures 
**	for building a WHERE clause.  A WHERE clause, or a piece
**	of one, is always generated in a separate segment list
**	and then added either to the query being built, or to
**	the previous contents of the WHERE clause in progress.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-nov-88  (robin)
**	    created
*/

static VOID
opcu_winit(
	ULD_TTCB    *ttcbptr )
{

    OPC_TTCB	*textcb;


    opcu_flush( ttcbptr, (ULD_TSTRING *) NULL);
    textcb = ( OPC_TTCB *) ttcbptr->handle;

    textcb->hold_where = textcb->where_cl;
    textcb->hold_seg = textcb->seglist;

    textcb->where_cl = NULL;
    textcb->seglist = NULL;

    return;
}


VOID
opcu_joinseg(main_seg, seg_to_add )
QEQ_TXT_SEG	**main_seg;
QEQ_TXT_SEG	*seg_to_add;
{
	QEQ_TXT_SEG	*segptr;
	DD_PACKET	*pktptr;

    /*
    ** Originally written to join 2 QEQ_TXT_SEG lists, this routine
    ** has been modified to join the DD_PACKET list of the segmet to
    ** be added on to the end of the DD_PACKET list in main_seg.
    ** This modification means that the QEQ_TXT_SEG for the segment
    ** to be added should be thrown away.  DRHFIXME
    */

	segptr= *main_seg;
	if ( segptr != NULL )
	{
	    for( ; segptr->qeq_s3_nxt_p != NULL;
		segptr = segptr->qeq_s3_nxt_p);
	    for (pktptr = segptr->qeq_s2_pkt_p;
		pktptr->dd_p3_nxt_p != NULL;
		pktptr = pktptr->dd_p3_nxt_p );	
	    pktptr->dd_p3_nxt_p = seg_to_add->qeq_s2_pkt_p;
	}
	else
	    *main_seg = seg_to_add;
	return;
}

static VOID
opcu_wend(
ULD_TTCB    *ttcbptr )
{
    OPC_TTCB	*textcb;
    QEQ_TXT_SEG	*holdit;

    opcu_flush( ttcbptr, (ULD_TSTRING *) NULL);
    textcb = (OPC_TTCB *) ttcbptr-> handle;

    if ( textcb->seglist != NULL )
    {
	/*
	**  Anything in the segment list now must be from
	** building the where clause
	*/
	if (textcb->where_cl != NULL)
	{
	    holdit = textcb->seglist;
	    textcb->seglist = textcb->where_cl;
	    opcu_printf( ttcbptr, " and ");
	    opcu_flush( ttcbptr, NULL );
	    textcb->seglist = holdit;
	}
	opcu_joinseg( &textcb->where_cl, textcb->seglist );
	textcb->seglist = NULL;
    }    

    if (textcb->hold_where != NULL)
    {
	holdit = textcb->seglist;
	textcb->seglist = textcb->where_cl;
	opcu_printf( ttcbptr, " and ");
	opcu_flush( ttcbptr, NULL );
	textcb->seglist = holdit;

	opcu_joinseg( &textcb->where_cl, textcb->hold_where);
	textcb->hold_where = NULL;
    }
    
    textcb->seglist = textcb->hold_seg;
    textcb->hold_seg = NULL;

    if ( textcb->where_cl != NULL )
    {
	opcu_joinwhere( ttcbptr );
	textcb->where_cl = NULL;
    }

    return;
}

/*{
** Name: opcu_where	- generate a where clause if necessary
**
** Description:
**      [@comment_text@] 
[@comment_line@]...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static VOID
opcu_where(
	ULD_TTCB           *ttcbptr )
{

    /* Initialize for WHERE clause build */

    opcu_winit( ttcbptr );

    if (ttcbptr->uld_qroot->pst_right
	&&
	(ttcbptr->uld_qroot->pst_right->pst_sym.pst_type != PST_QLEND))
    {	/* non null qualification exists so generate where clause */
	opcu_prexpr(ttcbptr, ttcbptr->uld_qroot->pst_right);
    }

    /* finalize after WHERE clause build */

    opcu_wend( ttcbptr );
}

/*{
** Name: opcu_isconstant	- test for constant expression
**
** Description:
**      In some cases of view flattening a constant expression
**	can be inserted into a group by expression.  This
**	routine will test for the existence of a constant
**	expression so that it can be eliminated from the
**	group by list. 
**
** Inputs:
**      qnodep                          ptr to parse tree expression
**
** Outputs:
**
**	Returns:
**	    TRUE - if constant expression is found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-may-91 (seputis)
**	    initial creation - partial fix for b37348
**	FIXME a better fix would be to eliminate the resdom node in
**	aggregate processing in OPF, but this may mean a function
**	aggregate becomes a simple aggregate,... also
**	the opa_bylist needs to be adjusted in some cases, and
**	concurrent aggregates need to be taken into account.
*/
static bool
opcu_isconstant(
	PST_QNODE          *qnodep )
{
    for (;qnodep;qnodep = qnodep->pst_left)
	if ((qnodep->pst_sym.pst_type != PST_CONST)
	    ||
	    (	qnodep->pst_right
		&&
		!opcu_isconstant(qnodep->pst_right)
	    ))
	    return(FALSE);
    return(TRUE);
}

/*{
** Name: opcu_grpby	- produce a group by clause
**
** Description:
**      create a group by clause from the query tree
**
** Inputs:
**      ttcbptr                          state variable
**
** Outputs:
**	
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-feb-89 (robin)
**	    Created.
**      29-mar-89 (robin)
**	    Modified to call opcu_resright instead of prexpr so that
**	    attribute names from temps would be handled correctly.
**      28-may-91 (seputis)
**          ignore constant expressions in group by list, part of fix
**	    for b37348
[@history_line@]...
[@history_template@]...
*/
static VOID
opcu_groupby(
	ULD_TTCB           *ttcbptr )
{
    OPC_TTCB	    *textcb;
    PST_QNODE	    *resdomp;
    bool	    first_time;

    textcb = (OPC_TTCB *) ttcbptr->handle;
    textcb->noalias = FALSE;
    textcb->nobraces = TRUE;

    if (textcb->subquery->ops_byexpr == NULL)
	return;	    /* there is no group by clause */

    first_time = TRUE;

    opcu_arraybld( textcb->subquery->ops_byexpr, (i4) PST_RESDOM, (PST_QNODE *) NULL,
	&ttcbptr->uld_nodearray[0], &ttcbptr->uld_nnodes );
    ttcbptr->uld_nnodes--;

    while ( ttcbptr->uld_nnodes >= 0 )
    {	/* by list exists so traverse down the list */

	resdomp = ttcbptr->uld_nodearray[ ttcbptr->uld_nnodes ];
	ttcbptr->uld_nnodes--;
	if ((resdomp->pst_right->pst_sym.pst_type != PST_VAR)
	    &&
	    opcu_isconstant(resdomp->pst_right)
	    )
	    continue;		    /* ignore constant expressions 
				    ** in group by list */
	if (first_time)
	{
	    opcu_flush( ttcbptr, NULL );
	    opcu_printf( ttcbptr, " group by ");
	    first_time = FALSE;
	}
	else
	{
	    opcu_printf(ttcbptr, ", ");
	}
	opcu_resright(ttcbptr, resdomp );
    }
    if (!first_time)
	opcu_flush( ttcbptr, NULL );
    return;
}

/*{
** Name: opcu_for	- Generate 'for update...' in define cursor
**
** Description:
**      Create the FOR UPDATE ... list for a define cursor statement 
**
** Inputs:
**      ttcbptr                         ptr to state control block
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places string into output buffer
**
** History:
**      15-mar-89 (seputis)
**          initial creation
**	01-mar-93 (barbara)
**	    Support for delimited identifiers in Star.  Pass in pointer to
**	    LDB capabilities to opc_gatname.
**      19-may-2004 (huazh01)
**          if the remote database server is not Ingres, then
**          we should not add the keyword 'direct'/'deferred' when 
**          generating the text for 'define cursor ... for update' statement.
**          They are all ingres-specific keyword and should be removed 
**          before sending the query text to non-Ingres database server 
**          This fixes INGSTR64, b111547. 
**	4-aug-05 (inkdo01)
**	    Above change fails when cop is NULL (which can legitimately
**	    happen - e.g. in agg queries). Make gwdb FALSE when no cop
**	    to fix 114904.
[@history_template@]...
*/
static VOID
opcu_for(
	ULD_TTCB    *ttcbptr )
{
    OPC_TTCB	*textcb;
    OPT_NAME	attrname;
    i4		new_attno;
    bool	first, gwdb;
    char	*attrmap;
    PST_QTREE	*qheader;
    DD_CAPS     *cap_ptr; 
    OPD_ISITE   site; 
    OPD_SITE   *sitedesc; 

    textcb = (OPC_TTCB *) ttcbptr->handle;
    qheader = textcb->state->ops_qheader;

    if (textcb->cop != (OPO_CO *) NULL)
    {
	site = textcb->cop->opo_variant.opo_local->opo_operation;
	sitedesc = textcb->state->ops_gdist.opd_base->opd_dtable[site];
	cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps;

	gwdb = (MEcmp(cap_ptr->dd_c7_dbms_type, "INGRES", 
                 sizeof(cap_ptr->dd_c7_dbms_type)) != 0);
    }
    else gwdb = FALSE;

    switch(qheader->pst_updtmode)
    {
    case PST_UNSPECIFIED:
    case PST_READONLY:
    {	/* Currently PSF will not distinguish between these
	** 2 cases so that the pst_delete flag should be looked at
	** when deciding if a cursor is readonly */
	if (!qheader->pst_delete)
	    opcu_printf( ttcbptr, " for readonly ");
	return;    
    }
    case PST_DIRECT:
    {
	if (!gwdb)
           opcu_printf( ttcbptr," for direct update of ");
        else
           opcu_printf( ttcbptr," for update of "); 
	break;
    }
    case PST_DEFER:
    {
	if (!gwdb)
          opcu_printf( ttcbptr," for deferred update of ");
        else
          opcu_printf( ttcbptr," for update of "); 
	break;
    }
    default:
	opx_error(E_OP0A8F_UPDATE_MODE);
    }

    /*
    **  Interpret bit map of columns to be updated 
    */
    attrmap = (char *) &qheader->pst_updmap;
    for (first = TRUE, new_attno = -1;
	 (new_attno = BTnext((i4)new_attno, (char *)attrmap, (i4)BITS_IN(qheader->pst_updmap))) >= 0
	      ;)
    {
	DB_ATT_ID	ingres_attr;

	/* note that table names cannot prefix the for update column list 
	** or a syntax error will result
	** and since this is an update it cannot be a temporary table */

	/* BB_FIXME: May have to test textcb->cop for null */
	site = textcb->cop->opo_variant.opo_local->opo_operation;
	sitedesc = textcb->state->ops_gdist.opd_base->opd_dtable[site];
	cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps ;
	ingres_attr.db_att_id = new_attno;

	opc_gatname(textcb->state, (OPV_IGVARS)qheader->pst_restab.pst_resvno, 
	    &ingres_attr, &attrname, cap_ptr);
	if (first)
	    first = FALSE;
	else
	    opcu_printf( ttcbptr, ", ");
	opcu_printf( ttcbptr, (char *) &attrname );
    }    
    if (first)
	opx_error(E_OP0A90_NO_ATTRIBUTES);  /* no attribute in the for update list */
    opcu_flush( ttcbptr, NULL );
}



/*{
** Name: opcu_quel	- produce quel text
**
** Description:
**      This is main entry point for producing quel text 
**
** Inputs:
**      ttcbptr                          ptr to state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
**	10-Jun-96 (abowler)
**	    Changed delimiter to " , " (added spaces) to avoid problems
**	    in LDB scanning when II_DECIMAL = ','.
[@history_template@]...
*/
static VOID
opcu_quel(
	ULD_TTCB           *ttcbptr )
{
    char	    *comma;
    char	    *equals;

    comma = " , ";
    equals = "=";
    switch (ttcbptr->uld_mode)
    {
    case PSQ_DEFCURS:	/* define cursor statement - new */
    case PSQ_QRYDEFED:	/* define cursor statement */
    case PSQ_RETRIEVE:	/* retrieve statement */
    {
	
	opcu_printf(ttcbptr, "retrieve(");
	opcu_target(ttcbptr, (QEQ_TXT_SEG **)NULL, comma, equals); /* generate target list */
	opcu_printf(ttcbptr, ")");
	opcu_where (ttcbptr);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_RETINTO:	/* retrieve into statement */
    {
	opcu_printf(ttcbptr, "retrieve into ");
	opcu_flush(ttcbptr, ttcbptr->uld_rnamep);
	opcu_printf(ttcbptr, "(");
	opcu_target(ttcbptr, (QEQ_TXT_SEG **)NULL, comma, equals); /* generate target list */
	opcu_printf(ttcbptr, ")");
	opcu_where (ttcbptr);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_APPEND:	/* append statement */
    {
	opcu_printf(ttcbptr, "append ");
	opcu_flush(ttcbptr, ttcbptr->uld_rnamep);
	opcu_printf(ttcbptr, "(");
	opcu_target(ttcbptr, (QEQ_TXT_SEG **)NULL, comma, equals); /* generate target list */
	opcu_printf(ttcbptr, ")");
	opcu_where (ttcbptr);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_REPLACE:	/* replace statement */
    {
	opcu_printf(ttcbptr, "replace ");
	opcu_flush(ttcbptr, ttcbptr->uld_rnamep);
	opcu_printf(ttcbptr, "(");
	opcu_target(ttcbptr, (QEQ_TXT_SEG **)NULL, comma, equals); /* generate target list */
	opcu_printf(ttcbptr, ")");
	opcu_where (ttcbptr);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_DELETE:	/* delete statement */
    {
	opcu_printf(ttcbptr, "delete ");
	opcu_flush(ttcbptr, ttcbptr->uld_rnamep);
	opcu_where (ttcbptr);	    /* generate where clause if it exists */
	break;
    }
    case PSQ_COPY:	/* copy statement */
    case PSQ_CREATE:	/* create statement */
    case PSQ_DESTROY:	/* destroy statement */
    case PSQ_HELP:	/* help statement (on relations) */
    case PSQ_INDEX:	/* index statement */
    case PSQ_MODIFY:	/* modify statement */
    case PSQ_PRINT:	/* print statement */
    case PSQ_RANGE:	/* range statement */
    case PSQ_SAVE:	/* save statement */

    default:
	opcu_msignal(ttcbptr, E_UL030B_BAD_QUERYMODE, E_DB_ERROR, 0);
    }
}

VOID
opcu_getrange(
OPCQHD	    *qhdptr,
i4	    tno,	/* global range table number */
char	    *rangename )
{
	OPCTABHD    *tabptr;
	i4	    tidx;

	if (tno >= OPV_MAXVAR)
	{
	    /* this is an optimizer-created temp */

	    tidx = tno - OPV_MAXVAR;	    /* cnvrt to index */

	    for (tabptr = qhdptr->opq_q7_tablist;
		tabptr->opq_t2_tnum != tidx;
		tabptr = tabptr->opq_t4_tnext);
	    
	    if (tabptr->opq_t1_rname[0] == '\0')
	    {
		STprintf( rangename, "t%d", tno );
		STprintf( tabptr->opq_t1_rname, "t%d", tno );		
	    }
	    else
	    {
		STcopy( tabptr->opq_t1_rname, rangename );
	    }
	}	
	else
	{
	    STprintf( rangename, "t%d", tno );
	}
	return;
}

/*{
** Name: opc_tblnam	- return a table name and an alias
**
** Description:
**      This routine is used to return a table name and an alias to the
**      specifically for the FROM list created by the query text generation routines.  
**	If the table is a temp, then the temp number is returned instead of the name.  
**
** Inputs:
**      global				state variable for tree
**					to text conversion
**
** Outputs:
**      namepp                          ptr to table name, or empty string
**					if a temporary table
**                                      the name only
**      aliaspp                         ptr to alias name 
**	tempnum				Temp number or -1 if not a temp
**	is_result			True if table is the 'result
**					  table' in an update or delete.
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      30-sep-88 (robin)
**          Created.
**	31-may-89 (robin)
**	    Added ttcbptr as input parm.  Part of fix for
**	    bug 5485.
*/
static bool
opc_tblnam(
	ULD_TTCB	   *ttcbptr,
	OPC_TTCB           *global,
	ULD_TSTRING        **namepp,
	ULD_TSTRING	   **aliaspp,
	i4		   *tempnum,
	bool		   *is_result )
{

    *is_result = FALSE;
    *aliaspp = &global->opt_adesc;
    *namepp = &global->opt_tdesc;
    global->opt_tdesc.uld_next = NULL;	/* ptr to next element or NULL */
    global->opt_adesc.uld_next = NULL;
    global->opt_adesc.uld_length = 0;
    global->opt_tdesc.uld_length = 0;
    global->opt_adesc.uld_text[0] = '\0';
    global->opt_tdesc.uld_text[0] = '\0';
    *tempnum = -1;			/* initialize to 'not a temp' */

    if (global->cop == NULL)
    {
	/*  This is a no-cotree subquery */
	return( TRUE );
    }

    global->opt_vno = BTnext((i4)global->opt_vno,
	(char *)global->cop->opo_variant.opo_local->opo_bmvars,
	global->subquery->ops_vars.opv_rv); /* get next variable to consider */
    if (global->opt_vno >= 0)
    {
	bool	    dummy;
	opc_nat( global, global->subquery->ops_global, global->opt_vno,
	    (OPCUDNAME *) global->opt_tdesc.uld_text, 
	    (OPT_NAME *) global->opt_adesc.uld_text, tempnum, is_result, ttcbptr->uld_mode,
	    &dummy);

	    global->opt_adesc.uld_length
		= STlength((char *)&global->opt_adesc.uld_text[0]);

	    global->opt_tdesc.uld_length
		= STlength((char *)&global->opt_tdesc.uld_text[0]);
	return (FALSE);			/* end of list not reached */
    }
    else
	return(TRUE);			/* end of list is reached */
}



/*{
** Name: opcu_sql	- produce sql text
**
** Description:
**      This is main entry point for producing sql text 
**
** Inputs:
**      handle                          ptr to state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-88 (seputis)
**          initial creation
**      03-mar-89 (robin)
**	    Use opd_dv_cnt, NOT opd_dv_base to generate
**	    a number for the ~q parameter of a define 
**	    cursor.
**	21-apr-89 (robin)
**	    Replaced opcu_from call with call to opc_rdfname and
**	    opcu_getrange to generate name of
**	    table to be updated in an UPDATE or DELETE statement.
**	    Part of fix for bug XX1.
**	01-may-89 (robin)
**	    Added similar change to 4/21 to define cursor - if
**	    the cursor doesn't return results to the user, it may
**	    be used for delete, so must reference one, non-temp
**	    table only as it's target. Bug 5643.  Made rangename and
**	    tablename variables available to entire routine. 
**	03-may-89 (robin)
**	    Don't print 'distinct' for cursor definition if the query
**	    was flattened.
**      19-may-89 (robin)
**	    In define cursor case, use OPD_NOFIXEDSITE instead of
**	    OPD_TOUSER to test for updatability.  Parital fix for
**	    bug 5481.
**	 31-may-89 (robin)
**	    Added mode parameter to all opc_rdfname calls. Partial
**	    fix for bug 5485.
**       08-jun-89 (robin)
**	    Added 'union' to PSQ_RETRIEVE case.
**	14-jun-89 (robin)
**	    Fixed bug 6537, always select DISTINCT for a projection
**	    action.
**	23-aug-89 (robin)
**	    Specify when target list is 'as' not '='. 
**      20-jun-90 (seputis)
**          moved checking of OPD_DISTINCT opcqtxt.c to fix  b30453
**      17-jun-91 (seputis)
**          fix bug 35119 - check if optimizer has turned off OPD_EXISTS
**	    flag in the case that the user had specified the FROM
**	    list extension in the replace statement, and generate
**	    a from list
**	22-nov-91 (fpang)
**	    Call to opcu_from() in 'create as select' case was missing
**	    the 'noresult' parameter. Supplied a value of FALSE, because
**	    the result table should be printed.
**	11-nov-92 (barbara)
**	    For delete cursor, no text to be generated.
**	13-mar-1993 (fpang)
**	    Reset opt_q2_type in case subselect had set it to XFER.
**	    Fixes B49497. Insert into select ... gets AV if multisite and
**	    columns > 160.
**	10-Jun-96 (abowler)
**	    Changed delimiter to " , " (added spaces) to avoid problems
**	    in LDB scanning when II_DECIMAL = ','.
**      21-oct-2004 (rigka01, crossed from unsubm 2.5 IP by hayke02)
**        Set OPD_NOFROM if this update query has no from clause - this will
**        ensure that we don't use temporary table references in the target
**        list. This change fixes bug 108409.
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
[@history_line@]...
[@history_template@]...
*/
static VOID
opcu_sql(
	ULD_TTCB           *ttcbptr )
{
    OPC_TTCB	*textcb;
    char	*comma;
    char	*equals;
    OPCUDNAME   tablename;	/* temporary in which to build the
				** table name */
    OPCUDNAME	rangename;
    textcb = (OPC_TTCB *) ttcbptr->handle;
    comma = " , ";
    equals = "=";

    switch (ttcbptr->uld_mode)
    {
	case PSQ_RETRIEVE:	/* retrieve statement */
	{
	    if ( textcb->subquery->ops_sqtype == OPS_UNION )
	    {
		opcu_printf( ttcbptr, " union ( " );
	    }
	    opcu_printf(ttcbptr, "select ");
	    if ( textcb->remdup || (textcb->subquery->ops_sqtype == OPS_PROJECTION)
	       )
	    {
		opcu_printf( ttcbptr, "distinct ");
	    }
	    equals = "as";
	    opcu_target(ttcbptr, (QEQ_TXT_SEG **)NULL, comma, equals); /* generate target list */
	    opcu_from (ttcbptr, (bool) FALSE, (bool) FALSE);	    /* generate from list for SQL */		
	    opcu_where (ttcbptr);	    /* generate where clause if it exists */
			    
	    break;
	}
	case PSQ_APPEND:	/* append statement */
	{   /* insert into tablename 
	    **         [(column {,column})] values (expression{,expression})]
	    **	|  [subquery]
	    */
	    QEQ_TXT_SEG *expression_list;
	    OPC_TTCB	*textcb = (OPC_TTCB *)ttcbptr->handle;
	    textcb->qryhdr->opq_q2_type = OPQ_NOTYPE;
	    opcu_printf(ttcbptr, "insert into ");
	    opcu_flush(ttcbptr, ttcbptr->uld_rnamep);
	    opcu_printf(ttcbptr, " (");
	    opcu_flush( ttcbptr, NULL);
	    opcu_target(ttcbptr, &expression_list, comma, NULL); /* generate target list */
	    opcu_printf(ttcbptr, ") ");
	    opcu_flush( ttcbptr, NULL );
	    if (!BTcount((char *)&ttcbptr->uld_qroot->pst_sym.pst_value.
			pst_s_root.pst_tvrm, PST_NUMVARS))
	    {   /* if no relations in the from list then only expressions need to be
		** evaluated so a VALUE clause can be used */
		opcu_printf(ttcbptr, " VALUES (");
		opcu_flush(ttcbptr, NULL);
		opcu_joinseg( &textcb->seglist, textcb->hold_targ );
		textcb->hold_targ = NULL;
		opcu_flush( ttcbptr, NULL);
		opcu_printf(ttcbptr, ")");
	    }
	    else
	    {   /* generate the subquery since variables are referenced */
		opcu_printf(ttcbptr, " select ");
		if ( textcb->remdup )
		    opcu_printf( ttcbptr, "distinct ");
		opcu_flush(ttcbptr, NULL);
		opcu_joinseg( &textcb->seglist, textcb->hold_targ );
		textcb->hold_targ = NULL;
		opcu_from( ttcbptr, (bool) FALSE, (bool) FALSE );
		opcu_where (ttcbptr);	    /* generate where clause if it exists */
	    }
	    opcu_flush( ttcbptr, NULL );
	    break;
	}
	case PSQ_REPCURS:	/* replace cursor statement */
	{   /* this would normally be handled as single site except for the
	    ** case of updatable views , no local range table exists so that
	    ** all dereferencing needs to be made via the global range table */
	    opcu_printf(ttcbptr, "update ");
	    opc_rdfname(textcb, 
		textcb->state->ops_rangetab.opv_base->opv_grv
		    [textcb->state->ops_qheader->pst_restab.pst_resvno],
		&tablename, ttcbptr->uld_mode );
	    opcu_printf( ttcbptr, (char *) &tablename );
	    opcu_printf(ttcbptr, " set ");
	    opcu_flush(ttcbptr, NULL);
	    opcu_target(ttcbptr, (QEQ_TXT_SEG **)NULL, comma, equals); /* generate target list */
	    opcu_printf(ttcbptr, " where current of ~Q");
	    if ( textcb->subquery->ops_dist.opd_dv_cnt > 0 )
		opcu_intprintf(ttcbptr, 
		    (i4) textcb->subquery->ops_dist.opd_dv_cnt);
	    opcu_printf(ttcbptr, " "); /* add blank just in case */
	    break;
	}
	case PSQ_REPLACE:	/* replace statement */
	{

	    opcu_printf(ttcbptr, "update ");
	    opc_rdfname(textcb, 
		textcb->state->ops_rangetab.opv_base->opv_grv
		    [textcb->state->ops_qheader->pst_restab.pst_resvno],
		&tablename, ttcbptr->uld_mode );
	    opcu_printf( ttcbptr, (char *) &tablename );
	    opcu_printf( ttcbptr, " " );
	    opcu_getrange( textcb->qryhdr,
		textcb->state->ops_qheader->pst_restab.pst_resvno, (char *) &rangename);
	    opcu_printf( ttcbptr, (char *) &rangename );
	    if (!( textcb->subquery->ops_global->ops_gdist.opd_gmask & OPD_EXISTS)
		&&
		(BTcount((char *)&textcb->subquery->ops_root->pst_sym.
		    pst_value.pst_s_root.pst_tvrm, (i4)BITS_IN(textcb->subquery->
		    ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm)) > 1
		))
	    {
		opcu_from (ttcbptr, (bool) FALSE, (bool) TRUE);	    /* generate from list for SQL */
				/* case in which the FROM list extension
				** of ingres to SQL was used, so generate
				** FROM list to retain equivalent semantics
				** - avoid from list if possibly constant views were
				** used and flattened, i.e. check for a from list
				** of more than one variable
				*/
	    }
            else
                textcb->subquery->ops_global->ops_gdist.opd_gmask |= OPD_NOFROM;
	    opcu_printf(ttcbptr, " set ");
	    opcu_flush(ttcbptr, NULL);
	    opcu_target(ttcbptr, (QEQ_TXT_SEG **)NULL, comma, equals); /* generate target list */
            textcb->subquery->ops_global->ops_gdist.opd_gmask &= (~OPD_NOFROM);
	    opcu_where(ttcbptr);
	    break;
	}
	case PSQ_DELETE:	/* delete statement */
	{
	    opcu_printf(ttcbptr, "delete from ");
	    opc_rdfname(textcb, 
		textcb->state->ops_rangetab.opv_base->opv_grv
		    [textcb->state->ops_qheader->pst_restab.pst_resvno],
		&tablename, ttcbptr->uld_mode);
	    opcu_printf( ttcbptr, (char *) &tablename );
	    opcu_printf( ttcbptr, " " );
	    opcu_getrange( textcb->qryhdr,
		textcb->state->ops_qheader->pst_restab.pst_resvno, (char *) &rangename);
	    opcu_printf( ttcbptr, (char *) &rangename );
	    opcu_printf( ttcbptr, " ");
	    opcu_flush( ttcbptr, NULL );
	    opcu_where(ttcbptr);
	    break;
	}

	case PSQ_DEFCURS:	/* open cursor */
	{
	    opcu_printf(ttcbptr, "open ~Q");
	    if ( textcb->subquery->ops_dist.opd_dv_cnt > 0 )
	    {
		opcu_intprintf(ttcbptr, (i4) textcb->subquery->ops_dist.opd_dv_cnt);
	    }
	    opcu_printf(ttcbptr," cursor for select ");
	    if ( textcb->remdup )
		opcu_printf( ttcbptr, "distinct ");
	    equals = "as";
	    opcu_target(ttcbptr, (QEQ_TXT_SEG **)NULL, comma, equals); /* generate target list */

	    if ( textcb->subquery->ops_global->ops_gdist.opd_gmask & OPD_NOFIXEDSITE)
	    {
		opcu_from (ttcbptr, (bool) FALSE, (bool) FALSE);	    /* generate from list for SQL */		
	    }
	    else
	    {
		/*
		**  The cursor being opened may be used for delete, so it must refer
		**  to one table only. Bug 5643.
		*/
		opcu_printf( ttcbptr, " from ");
		opc_rdfname(textcb, 
		    textcb->state->ops_rangetab.opv_base->opv_grv
		    [textcb->state->ops_qheader->pst_restab.pst_resvno],
		    &tablename, ttcbptr->uld_mode );
		opcu_printf( ttcbptr, (char *) &tablename );
		opcu_printf( ttcbptr, " " );
		opcu_getrange( textcb->qryhdr,
			textcb->state->ops_qheader->pst_restab.pst_resvno,
			(char *) &rangename);
		opcu_printf( ttcbptr, (char *) &rangename );
		opcu_printf( ttcbptr, " " );
	    }
	    opcu_where (ttcbptr);	    /* generate where clause if it exists */
			    
	    break;
	}

	case PSQ_RETINTO:	/* create table as select statement */
	{
	    QEQ_TXT_SEG *expression_list;

	    opcu_printf(ttcbptr, "create table ");
	    opcu_flush(ttcbptr, ttcbptr->uld_rnamep);
	    opcu_printf(ttcbptr, "(");
	    opcu_flush( ttcbptr, NULL);
	    opcu_target(ttcbptr, &expression_list, comma, NULL); /* generate target list */
	    opcu_printf(ttcbptr, ")");
	    opcu_flush( ttcbptr, NULL );
	    opcu_printf(ttcbptr, "as select ");
	    if ( textcb->remdup )
		opcu_printf( ttcbptr, "distinct ");
	    opcu_flush(ttcbptr, NULL);
	    opcu_joinseg( &textcb->seglist, textcb->hold_targ );
	    textcb->hold_targ = NULL;
	    opcu_from (ttcbptr, (bool) FALSE, (bool) FALSE);	    /* generate from list for SQL */		
	    opcu_where (ttcbptr);	    /* generate where clause if it exists */
	    break;
	}

	case PSQ_DELCURS:	/* Delete cursor -- no text */
	    break;

	case PSQ_COPY:	/* copy statement */
	case PSQ_CREATE:	/* create statement */
	case PSQ_DESTROY:	/* destroy statement */
	case PSQ_HELP:	/* help statement (on relations) */
	case PSQ_INDEX:	/* index statement */
	case PSQ_MODIFY:	/* modify statement */
	case PSQ_PRINT:	/* print statement */
	case PSQ_RANGE:	/* range statement */
	case PSQ_SAVE:	/* save statement */

	default:
	    opcu_msignal(ttcbptr, E_UL030B_BAD_QUERYMODE, E_DB_ERROR, 0);
    }

}

/*{
** Name: opc_conjunct	- return the next conjunct to attach to the text
**
** Description:
**      This is a co-routine which will return the next conjunct to 
**      attach to a qualification text string being constructed
**
** Inputs:
**      handle                          state variable for text string
**                                      construction
**
** Outputs:
**      qtreepp                         return location for next conjunct
**	Returns:
**	    TRUE - if no more conjuncts exist to be added to the text
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      4-apr-88 (seputis)
**          initial creation
**      15-may-89 (robin)
**	    Fixed bug 5477- in a simple agg only query with a having clause, this
**	    routine will be called although there is no cotree for the final
**	    subquery.  So, check the existence of handle->cop before calling
**	    BTnext.
**	15-jun-89 (robin)
**	    Always initialize the null join flag in the handle, since opc_etoa,
**	    which sets it, is not always called.  Bug 6555.
**      3-jun-91 (seputis)
**          fix for b37348
**      8-jul-91 (seputis)
**          fix for star bug 7381
**	8-sep-92 (seputis)
**	    fix name conflict
**	16-Apr-2007 (kschendel) b122118
**	    Remove dead expr-LIKE code, flag was never turned on.
[@history_line@]...
[@history_template@]...
*/
static bool
opc_conjunct(
	OPC_TTCB           *handle,
	PST_QNODE          **qtreepp )
{

    handle->nulljoin = FALSE;
    if ((handle->jeqcls >= 0)		/* if the last conjunct was a join */
        &&
	( handle->qryhdr->opq_q16_expr_like != OPQ_2ELIKE )
	)
    while((handle->jeqcls = BTnext((i4)handle->jeqcls, 
				  (char *)&handle->bmjoin,
				  (i4)BITS_IN(handle->bmjoin)
                                 )	/* and there are still joins to be
					** processed */
          ) >= 0
        )
    {
	if (opc_etoa(handle, handle->jeqcls,
	    &handle->nulljoin))		/* these are the joining clauses
					** which were not used */
	{
	    *qtreepp = &handle->qnodes.eqnode;
	    return(FALSE);		    /* handle next non-key join eqcls */
	}    
    }

    handle->joinstate = OPT_JNOTUSED; /* remainder of qualifications are not
				    ** used */
    if ( handle->cop == NULL )	    /*  No more conj if no cotree */
    {                               
	return(TRUE);
    }


    while((handle->bfno = BTnext((i4)handle->bfno, 
	    (char *)handle->cop->opo_variant.opo_local->opo_bmbf, 
	    (i4)OPB_MAXBF)
	 ) >= 0)
    {
	/* there is at least one more boolean factor to process */
	OPB_BOOLFACT	*bfp;

	bfp = handle->subquery->ops_bfs.opb_base->opb_boolfact[handle->bfno];
	*qtreepp = bfp->opb_qnode;
				    /* ptr to boolean factor element
				    ** next expression to obtain convert */

	return(FALSE);			/* end of list not reached */
    }
    return(TRUE);			/* end of conjunct list is reached */
}




/*{
** Name: opc_jconjunct	- return the next JOIN conjunct to attach to the text
**
** Description:
**      This is a co-routine which will return the next JOIN conjunct to 
**      attach to a qualification text string being constructed.  Conjuncts
**      in this list are special since the participate in the join operation
**      in some way, as opposed to being evaluated a qualification.
**
** Inputs:
**      handle                          state variable for text string
**                                      construction
**
** Outputs:
**      qtreepp                         return location for next conjunct
**	Returns:
**	    TRUE - if no more conjuncts exist to be added to the text
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      4-apr-88 (seputis)
**          initial creation
**	15-jun-89 (robin)
**	    Always initialize the null join flag in the handle to FALSE, since
**	    opc_etoa is not always called.  Bug 6555.
**      3-jun-91 (seputis)
**          fix for b37348
**      12-jul-91 (seputis)
**          fix for bug 7381
[@history_line@]...
[@history_template@]...
*/
static bool
opc_jconjunct(
	OPC_TTCB           *handle,
	PST_QNODE          **qtreepp )
{
    handle->nulljoin = FALSE;

    if (handle->qryhdr->opq_q16_expr_like == OPQ_2ELIKE)
    {
	/*
	**  All joins were handled when the temp table was
	**  created to materialize the expression-LIKE.
	*/

	return;
    }

    while (handle->jeqclsp
	&&
	((*handle->jeqclsp) != OPE_NOEQCLS)
       )
    {
	if (opc_etoa(handle, *(handle->jeqclsp++),
	    &handle->nulljoin))		/* get the next EQCLS participating in the 
					** multi attribute join */
	{
	    *qtreepp = &handle->qnodes.eqnode; /* ptr to join element
					    ** to convert */
	    return(FALSE);			/* end of list not reached */
	}
    }
    return(TRUE);			/* end of conjunct list is reached */
}


/*{
** Name: opcu_nullj	- Generate a NULL join
**
** Description:
**
** Inputs:
**      ttcbptr                         state variable
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      20-mar-89 (robin)
**	    Created
**	26-dec-91 (seputis)
**	    fix bug 41505 - a multi-variable group by
**	       aggregate may result in a syntax error as in
**		select gf.yr, gf.limit, sum(sg.charged) from sg, gf
**		    where sg.attr1 = '56' and gf.yr = sg.yr
**		    group by gf.yr, gf.limit
[@history_template@]...
*/
static VOID 
opcu_nullj(
	ULD_TTCB    *ttcbptr,
	OPC_TTCB    *handle )
{
	PST_QNODE   *qnode;

	handle->joinstate = OPT_JOUTER;
	qnode = ttcbptr->uld_qroot;
        opcu_printf( ttcbptr, " or ( " );
	if (qnode->pst_left != NULL )
	{
	    opcu_prexpr(ttcbptr, qnode->pst_left);
	    opcu_printf(ttcbptr, " is null ");

	    opcu_printf( ttcbptr, " and " );
	}
	
	if (qnode->pst_right != NULL)
	{
	    opcu_prexpr(ttcbptr, qnode->pst_right);
	    opcu_printf(ttcbptr, " is null ");
	}
	opcu_printf(ttcbptr, " ) ");
	handle->joinstate = OPT_JNOTUSED;
    return;
}


/*{
** Name: opcu_sort	- Generate an 'order by' clause.
**
** Description:
**	Generate an 'order by' clause of the form 
**	'order by n' where n is the relative position of
**	target list attribute to sort on.
**
** Inputs:
**      ttcbptr                         state variable
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      13-jun-90 (seputis)
**          fix b21615 - traverse right-most rather than left most list
[@history_line@]...
[@history_template@]...
*/
static VOID
opcu_sort(
	ULD_TTCB    *ttcbptr )
{
    PST_QNODE	*sort_node;
    PST_QNODE	*sort_list;
    OPC_TTCB	*textcb;
    OPT_NAME	name_buf;
    bool	first;

    textcb = (OPC_TTCB *) ttcbptr->handle;
    sort_list = sort_node = textcb->state->ops_qheader->pst_stlist;    


    if ( textcb->state->ops_qheader->pst_stflag == FALSE ||
	    sort_list == NULL )
    {
	return;	    /* there is no sort list */
    }

    opcu_printf( ttcbptr, " order by " );
    first = TRUE;

    do
    {
	if ( !first )
	{
	    opcu_printf(ttcbptr, ", ");
	}

	STprintf( (char *)&name_buf, "%d ",
	    sort_node->pst_sym.pst_value.pst_s_sort.pst_srvno );
	opcu_printf( ttcbptr, (char *) &name_buf );

	if (sort_node->pst_sym.pst_value.pst_s_sort.pst_srasc == FALSE)
	{
	    opcu_printf( ttcbptr, " desc ");
	}
	first = FALSE;
	sort_node = sort_node->pst_right;
    }while ( sort_node != NULL && sort_node != sort_list );
    
    opcu_flush( ttcbptr, NULL);

}

/*{
** Name: opcu_arraybld	- Build an array of parse tree nodes
**
** Description:
**	This routine will build an array of parse tree nodes of
**	the same type that are children of the start node provided.
**	Arrays of this type may be used to operate on or from parse
**	tree nodes without recursion.  It is specifically used
**	in the distributed optimizer to generate a target list
**	from the resdoms in the parse tree, which are stored in
**	'reverse' order.
**	
**	Note that the array will be built in the order that the 
**	nodes are found as the tree is descended.  For resdoms,
**	this means that it should be traversed in reverse order
**	to match the user-specified order of the target list.
**
**
** Inputs:
**      root                            Parse tree node to start at
**	nodetype			Type of node to look for
**	stop_at				Node to stop on, or NULL
**
** Outputs:
**      nodearray			Ptr to an array provided by the caller,
**					    the array will be updated to 
**					    contain ptrs to all nodes of the
**					    specified type in the subtree
**      nnodes				Ptr to a i4, it will contain the number
**					    of node elements in the array.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-jan-89  (robin)
**	    Created 
[@history_template@]...
*/
VOID
opcu_arraybld(
	PST_QNODE   *root,
	i4	    nodetype,
	PST_QNODE   *stop_at,
	PST_QNODE   *nodearray[],
	i4	    *nnodes )
{
    i4		x;
    PST_QNODE	*node_ptr;


    for ( x=0, node_ptr = root; node_ptr != NULL && node_ptr != stop_at; 
	  node_ptr=node_ptr->pst_left )
    {
	if ( node_ptr->pst_sym.pst_type == nodetype )
	{
	    nodearray[x] = node_ptr;
	    x++;
	}
	if (x > OPCU_MAXRES - 1)
	    break;
    }
    
    *nnodes = x;
    return;
}


/*{
** Name: opc_resdom	- return next resdom to include in list
**
** Description:
**      This is a co-routine used to construct the text for the 
**      resdom target list.
**
** Inputs:
**      ttcbptr				ptr to text state variable
**	use_list			if true, return left son of
**					resdomp as next resdom.
**      resdomp                         ptr to resdom node from which
**                                      a name is required
**                                      NULL if the next resdom is
**                                      required
**
** Outputs:
**      namep                           return name of resdom
**	attno				return attribute number
**	Returns:
**	    ptr to resdom to analyze, or NULL if no more resdoms exist
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-apr-88 (seputis)
**          initial creation
[@history_line@]...
**      24-may-89 (robin)
**	    Added base_colname parameter to cause the routine to return the 
**	    base column name (rather than the resdom name).  This is needed
**	    for update and insert.  Partial fix for bug 5638.
**      10-jul-89 (robin)
**	    Added change from SUN port to assign pst_rsno to a DB_ATT_ID
**	    before passing it to opc_gatname.
**	01-mar-93 (barbara)
**	    Support for delimited identifiers in Star.
**	    1. Copy (and possibly delimit) result column name into caller's
**	       buffer instead of returning pointer to pst_rsname field
**	    2  Pass LDB capabilities pointer to opc_gatname.
**	14-may-93 (barbara)
**	    Delimit name in case where there is a user-supplied result column
**	    name.  Previous fix only took care of case where result column
**	    is same as base column.
**	24-may-93 (barbara)
**	    Cast args to MEcopy to agree with prototype and Ed.
**      02-sep-93 (barbara)
**          Delimiting of names is based on existence of DB_DELIMITED_CASE
**          capability in the LDB, not on OPEN/SQL_LEVEL.
**      23-may-94 (peters)
**          b59561 - When column names are mapped, a query of the form
**	    "update ... where current of cursor" will use the mapped name for
**	    the resdom instead of the name in the LDB.  Added call to
**	    opcu_curval() to get the LDB name.
**	28-mar-1995 (rudtr01)
**	    fixed bug 67744 by removing previous fix and inserting
**	    correct fix in opcu_target().
**	1-oct-97 (inkdo01)
**	    Fixed bug 78960 by introducing a coercion of naked NULLs in
**	    select list to datatype indicated in corresponding RESDOM.
**	22-Dec-08 (kibro01) b121426
**	    Do not add a NOOP into the RESDOM tree if no coercion is required
**	    since the STAR translation back to text gets confused by it, and
**	    it is pointless.
[@history_template@]...
*/
static PST_QNODE *
opc_resdom(
	ULD_TTCB	   *ttcbptr,
	bool		   use_list,
	PST_QNODE          *resdomp,
	char               *namep,
	i4		   *attno,
	bool		   base_colname )
{
    OPC_TTCB           *handle;
    OPS_SUBQUERY	*subquery;
    char		*rescolp;
    i4			length;
    DB_DATA_VALUE	*dvp;


    handle = (OPC_TTCB *) ttcbptr->handle;
    subquery = handle->subquery;

    if ( use_list )
    {
	DD_CAPS	    *cap_ptr;
	OPD_ISITE   site;
	OPD_SITE    *sitedesc;

	if ( resdomp == NULL)
        {
		opcu_arraybld( ttcbptr->uld_qroot->pst_left, (i4) PST_RESDOM,
		    NULL, &ttcbptr->uld_nodearray[0], &ttcbptr->uld_nnodes );
        }
	ttcbptr->uld_nnodes--;
	if ( ttcbptr->uld_nnodes >= 0 )
	{
	    resdomp = ttcbptr->uld_nodearray[ ttcbptr->uld_nnodes ];
	}
	else
	{
	    resdomp = NULL;		    
	}
	if (resdomp != NULL )
	{	
	    *namep = '\0';
	    if ( subquery->ops_sqtype != OPS_MAIN )
	    {
		/*
		**  The pst_rsname field will not have a valid name
		**  for aggregates in an aggregate subquery.  Invent
		**  one using 'a' concatenated to pst_rsno.
		*/
		resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsname[0] = 'a';
		CVla( (i4)resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
		    (char *) &resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsname[1] );
		MEcopy( (PTR) resdomp->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname, DB_ATT_MAXNAME, (PTR) namep );
		length = opt_noblanks(DB_ATT_MAXNAME, namep);
		namep[ length ] = '\0';
	    }
	    else if ( handle->qryhdr->opq_q11_duplicate == OPQ_1DUP )
	    {
		handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsname[0]
		    = 'a';		
		CVla( (i4)resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
			(char *)&handle->qnodes.resnode.pst_sym.pst_value.
			pst_s_rsdm.pst_rsname[1]);

		handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsno 
		    = resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;

		handle->qnodes.resnode.pst_right = resdomp->pst_right;
		resdomp = &handle->qnodes.resnode;
		MEcopy( (PTR) resdomp->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname, DB_ATT_MAXNAME, (PTR) namep );
		length = opt_noblanks(DB_ATT_MAXNAME, namep);
		namep[ length ] = '\0';

	    }
	    else if (!(resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	    {
		/*
		**  The pst_rsname field will not have a valid name
		**  for non-printing resdoms (tids) either.  Invent
		**  one using 't' concatenated to pst_rsno.
		*/
		resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsname[0] = 't';
		CVla( (i4)resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
		    (char *) &resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsname[1] );
		MEcopy( (PTR) resdomp->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname, DB_ATT_MAXNAME, (PTR) namep );
		length = opt_noblanks(DB_ATT_MAXNAME, namep);
		namep[ length ] = '\0';
	    }
	    else if (base_colname == TRUE)
	    {
		/*
		**  Use the column name, not the resdom name.  Typically this
		**  will happen when building the 'target list' for inserts
		**  and updates.
		*/

		DB_ATT_ID   tmp_dbatt;

		tmp_dbatt.db_att_id =
		    (DB_ATTNUM) resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
		if (handle->cop == NULL)
		    site = OPD_TSITE;
		else
		    site = handle->cop->opo_variant.opo_local->opo_operation;
		sitedesc = handle->state->ops_gdist.opd_base->
				opd_dtable[site];
		cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps;

		opc_gatname( handle->state, 
			handle->state->ops_qheader->pst_restab.pst_resvno,
		   	&tmp_dbatt , (OPT_NAME *) namep, cap_ptr);

	    }

	    if (*namep == '\0')
	    {
		/* 
		** No need to delimit if cop is null -- query will
		** be evaluated on CDB
		*/
		if (handle->cop != NULL)
		{
		    site = handle->cop->opo_variant.opo_local->opo_operation;
		    sitedesc = handle->state->ops_gdist.opd_base->
				opd_dtable[site];
		    cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps;
		}

		if (handle->cop == NULL || 
			cap_ptr->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
		{
		    u_i4 	unorm_len = sizeof(OPT_NAME)-1;
		    DB_STATUS   status;
		    DB_ERROR    error;

		    /* copy/unnormalize name of resdom into caller's buffer */
		    char	*result_name = resdomp->pst_sym.pst_value.
				    pst_s_rsdm.pst_rsname;
		    length = opt_noblanks(DB_ATT_MAXNAME, result_name);
		    status = cui_idunorm( (u_char *)result_name,
			    (u_i4 *)&length, (u_char *)namep, &unorm_len,
			    CUI_ID_DLM, &error);
		    if (DB_FAILURE_MACRO(status))
			opx_verror(E_DB_ERROR, E_OP08A4_DELIMIT_FAIL,
				error.err_code);
		    namep[ unorm_len ] = '\0';
		}
		else
		{
		    /* copy name of resdom into caller's buffer */
		    MEcopy( (PTR) resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			    DB_ATT_MAXNAME, (PTR) namep );
		    length = opt_noblanks(DB_ATT_MAXNAME, namep);
		    namep[ length ] = '\0';
		}
	    }
	    *attno = (i4)resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	}
    }
    else
    {

        /* only simple equivalence classes are needed for interior nodes */

        if ((handle->reqcls = BTnext((i4)handle->reqcls, 
		    (char *)&handle->cop->opo_maps->opo_eqcmap, 
		    (i4)OPE_MAXEQCLS  )
		) >= 0)
        {
		OPZ_IATTS	new_attr;
		OPZ_ATTS	*attrp;

	    /* name all attribute of temporaries by using the "e" followed
	    ** by the equivalence class number, since at any node this
	    ** will represent a unique attribute */

	    handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsname[0]
		= 'e';		/* all resdom names begin
				** with an 'e' */
	    CVla((i4)handle->reqcls, (char *)&handle->qnodes.resnode.pst_sym.
		pst_value.pst_s_rsdm.pst_rsname[1]);

	    /* copy name of resdom into caller's buffer */
	    rescolp = &handle->qnodes.resnode.pst_sym.
			pst_value.pst_s_rsdm.pst_rsname[0];
	    MEcopy( (PTR) rescolp, DB_ATT_MAXNAME, (PTR) namep );
	    length = opt_noblanks(DB_ATT_MAXNAME, namep);
	    namep[ length ] = '\0';
	    handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsno 
		= handle->reqcls;

	    new_attr = 
	    handle->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
		BTnext((i4)-1, (char *)&subquery->ops_eclass.
		ope_base->ope_eqclist[handle->reqcls]->ope_attrmap,
		(i4)OPZ_MAXATT);
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[new_attr];
	    STRUCT_ASSIGN_MACRO(attrp->opz_dataval, 
		handle->qnodes.resexpr.pst_sym.pst_dataval);
	    STRUCT_ASSIGN_MACRO(attrp->opz_dataval, 
		handle->qnodes.resnode.pst_sym.pst_dataval);
	    handle->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_vno 
		= attrp->opz_varnm;
	    resdomp = &handle->qnodes.resnode;
	    *attno = new_attr;
	}
	else
	    resdomp = NULL;		/* return indicator that the end of the
				    ** list has been found */
    }

    /* Check for "select ..., NULL, ...". These can arise during the 
    ** decomposition of union selects, and can cause syntax errors at the
    ** target node. The solution is to replace NULL with a coercion of
    ** NULL to the datatype indicated in the corresponding RESDOM. */
    if (resdomp && resdomp->pst_right->pst_sym.pst_type == PST_CONST 
	&& resdomp->pst_right->pst_sym.pst_value.pst_s_cnst.pst_tparmtype
							== PST_USER
	&& (dvp = &(resdomp->pst_right->pst_sym.pst_dataval))->db_datatype < 0
	&& dvp->db_data && dvp->db_data[dvp->db_length-1])
					/* test for resdom expression which is
					** nullable constant with non-zero
					** indicator byte */
    {
	/* Got our NULL. Now loop over coercion array looking for match on
	** resdom result type, to allow us to insert coercion function to
	** give the NULL context when standing alone in the select list. 
	** This approach fixes bug 78960. */
	PST_QNODE	*cptr;		/* coercion UOP node ptr */
	ADI_OP_ID	coercion_op;	/* ADF opcode for coercion operator */
	i4		i;

	for (i = 0; i < NCA_SIZE; i++)
	 if (nc_array[i].type.db_datatype == 
			resdomp->pst_sym.pst_dataval.db_datatype) break;
	if (i < NCA_SIZE)		/* did we get a match? */
	{
	    adi_opid(subquery->ops_global->ops_adfcb, nc_array[i].name,
		&coercion_op);		/* get opcode */
	    /* If there is no coercion required, don't add a useless NOOP */
	    if (coercion_op != ADI_NOOP)
	    {
	       cptr = opv_opnode(subquery->ops_global, PST_UOP, coercion_op, 0);
					/* alloc. new parse tree node */
	       STRUCT_ASSIGN_MACRO(nc_array[i].type, cptr->pst_sym.pst_dataval);
					/* copy data type info */
	       cptr->pst_left = resdomp->pst_right;
					/* splice everything together */
	       resdomp->pst_right = cptr;
	    }
	}
    }

    return(resdomp);
}

/*{
** Name: opc_prbuf	- place string into buffer for distributed compiled query
**
** Description:
**      This routine will move a null terminated string into either the temporary
**	string buffer, or, when that buffer is full, into the list of query text
**	segments.
**	
**
** Inputs:
**	textcb				text generation control block
**	stringp				ptr to the null-terminated string to be added
**	bufstruct			ptr to the temporary buffer structure
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-oct-88 (robin)
**          Created from uld_printf.
*/
static VOID
opc_prbuf(
	OPC_TTCB	   *textcb,
	char               *stringp,
	ULD_TSTATE	   *bufstruct )
{
    i4		len;
    OPS_STATE	*global;

    global = textcb->state;

    if (stringp[0] && !stringp[1] && bufstruct->uld_remaining)
    {	/* special case single character write */
	bufstruct->uld_tempbuf[bufstruct->uld_offset++]=stringp[0];
	bufstruct->uld_remaining--;
	return;
    }
    len = STlength(stringp);
    while (len > 0)
    {
	if (bufstruct->uld_remaining >= len)
	{
	    /*  The string will fit completely in the temporary buffer */

	    MEcopy((PTR)stringp, len, 
		(PTR)&bufstruct->uld_tempbuf[bufstruct->uld_offset]);
	    bufstruct->uld_offset += len;
	    bufstruct->uld_remaining -= len;
	    break;
	}
	else
	{   
	    /*
	    **  The string will only partially fit in the temporary, so create a text segment
	    **  and put it there.
	    */

	    i4		length_to_move;
	    char	*source;

	    if (bufstruct->uld_offset <= 0)
	    {	/* need to copy directly to allocated buffer since size 
                ** of string is larger than temporary buffer */

		length_to_move = len;
		source = stringp;
		len = 0;
	    }
	    else
	    {	/* move from temporary buffer */
		length_to_move = bufstruct->uld_offset;
		source = &bufstruct->uld_tempbuf[0];
	    }

	    opc_addseg( global, source, length_to_move, (i4) -1, &textcb->seglist );

	    bufstruct->uld_tstring = NULL;
	    bufstruct->uld_offset = 0;	    /* reset buffer to be empty */
	    bufstruct->uld_remaining = ULD_TSIZE - 1;	/* space for null terminator */
	}
    }
}



/*{
** Name: opcu_tidjoin	- Generate a tid join to the table to update
**
** Description:
**   In some cases when a query does not return results to the user (i.e. update type
**   queries), the table at the root of the last subquery will be a temp table or a
**   secondary index, not the table to be updated.  When this is the case, this routine 
**   will generate a final join on tid between the temp table or the index, and the real 
**   table to update.
**
** Inputs:
**      ttcbptr                         global structure pointer
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-may-89 (robin)
**	    Created.
*/

static VOID
opcu_tidjoin(
	ULD_TTCB           *ttcbptr )
{
    OPC_TTCB	*textcb;
    i4		varno;
    bool	found;
    OPV_VARS	*lrv_ptr;
    OPE_IEQCLS	tideqc;
    OPCUDNAME	rangename;

    textcb = (OPC_TTCB *) ttcbptr->handle;

    if (textcb->cop == NULL)
    {
	/*  This is a no-cotree subquery */
	return;
    }

    if (textcb->subquery->ops_localres == OPV_NOVAR)
    {
	return;
    }    


    varno = -1;
    found = FALSE;

    varno = textcb->subquery->ops_localres;
    found = BTtest( (i4) varno, (char *) textcb->cop->opo_variant.opo_local->opo_bmvars);

    if (found)
    {
	/*
	** The table to be updated was found in the bitmap of variables
	** available at this node.  Check to see if the table is a
	** temporary.  If it is, then it has to be joined back via a tid join to
	** original table to be updated.
	*/
	lrv_ptr = textcb->subquery->ops_vars.opv_base->opv_rt[varno];
	if (lrv_ptr->opv_itemp != OPD_NOTEMP || lrv_ptr->opv_grv->opv_created == OPS_FAGG)
	{
	    /*  The table is a temp table */

	}
	else
	{
	    return;	/* no special processing needed */
	}
    }
    else
    {
	/*
	** The table to be updated may be represented by an index (only)
	** at this point.  If so, need to find the base table to update
	** and join it to the index via a tid join.
	*/
	varno = -1;
	found = FALSE;
	
	do
	{
	    varno = BTnext((i4)varno, (char *)textcb->cop->opo_variant.opo_local->opo_bmvars,
		    textcb->subquery->ops_vars.opv_rv);
	    lrv_ptr = textcb->subquery->ops_vars.opv_base->opv_rt[varno];
	    if ( lrv_ptr->opv_index.opv_poffset == textcb->subquery->ops_localres )
	    {
		/* found the base table for the index */
		found = TRUE;
		
		/* point to the base table, not the index */		
		lrv_ptr = textcb->subquery->ops_vars.opv_base->opv_rt[ lrv_ptr->opv_index.opv_poffset ];		
	    }
	} while ( varno >= 0 && found == FALSE );

	if ( !found )
	{
	    opx_error( E_OP0A97_OPCTIDIDX );
	}
    }


    tideqc = lrv_ptr->opv_primary.opv_tid;
    if ( tideqc == OPE_NOEQCLS )
    {
	opx_error( E_OP0A98_OPCTIDEQC );
    }

    /* Call opc_etoa(?)  then print 'AND table-to-update.tid = lrv_ptr->(tablename).tid-eq-class-attr-name' */

    opc_one_etoa( textcb, tideqc );

    opcu_winit( ttcbptr );	/* WHERE clause initialization */
    opcu_flush( ttcbptr, NULL);
    opcu_printf( ttcbptr, " ( ");
    opcu_getrange( textcb->qryhdr,
	textcb->state->ops_qheader->pst_restab.pst_resvno,
	(char *) &rangename);
    opcu_printf( ttcbptr, (char *) &rangename );
    opcu_printf( ttcbptr, ".tid = ");

    opcu_prexpr( ttcbptr, &textcb->qnodes.resexpr);
    opcu_printf( ttcbptr," ) ");

    opcu_wend( ttcbptr );   /* WHERE clause finalization */
    return;
}


/*{
** Name: OPCU_TREE_TO_TEXT	- routine to convert query tree to text
**
** Description:
**      This routine will convert a query tree to a text string
**      and is written to be a utility routine which can be called by 
**      any facility i.e. (PSF or OPF) 
[@comment_line@]...
**
** Inputs:
**      handle                          handle passed to all caller defined
**                                      defined routines
**      ulmcb				ptr to ULM control block used to do
**                                      ulm_palloc's for any internally defined
**                                      memory.
**      linelength                      max length of one line to be returned
**                                      0 if max length of ULD_TSTRING is to be
**                                      used.
**      pretty                          TRUE - if non-machine use anticipated
**                                      so that indentation etc. should be
**                                      used if possible
**      qroot				ptr to a query tree to be converted
**                                      - if a root node then an entire statement
**                                      is generated, otherwise an expression is
**                                      generated 
**                                      
**      resdomflg                       use routine to add the next resdom to the
**                                      target list, used to traverse bitmaps
**                                      of equivalence classes at intermediate
**                                      nodes
**
** Outputs:
**      tstring                         ptr to text string list null terminated
**                                      strings
**	Returns:
**	    DB_STATUS - result of conversion
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-mar-88 (seputis)
**          initial creation
**      02-may-89 (robin)
**	    Check opq_q14_need_close flag instead of opq_q9_flatten
**	    when finishing and deciding whether a subselect has been
**	    added to the where clause that requires a close parenthesis.
**      10-may-89 (robin)
**	    Added call to opcu_tidjoin to the 'finish' process, to generate
**	    a join between a temp table or index at the root of the final
**	    subquery, and the real table to be updated.
**      15-may-1985 (robin)
**	    Fixed bug 5475 by changing 'finish' code to generate an order
**	    by only for the main subquery.
**	26-may-94 (davebf)
**	    Added initialization of uld_ssvar.
**	22-mar-10 (smeke01) b123457
**	    Added trace point op214 call to opu_qtprint.
[@history_line@]...
[@history_template@]...
*/
DB_STATUS
opcu_tree_to_text(
	PTR                handle,
	i4		   qmode,
	ULD_TSTRING	   *rnamep,
	ADF_CB             *adfcb,
	ULM_RCB		   *ulmcb,
	ULD_LENGTH	   linelength,
	bool               pretty,
	PST_QNODE          *qroot,
	i4		   conjflag,
	bool		   resdomflg,
	ULD_TSTRING        **tstring,
	DB_ERROR           *error,
	DB_LANG		   language,
	bool		   qualonly,
	bool		   finish,
	bool		   notqual )
{
    ULD_TTCB	    ttcbptr;		/* state variable for tree to text
                                        ** conversion */
    OPC_TTCB	    *textcb;
    EX_CONTEXT      excontext;	    /* context block for exception handler*/

    ttcbptr.error = error;
    error->err_code = E_UL0000_OK;
    error->err_data = 0;
    ttcbptr.uld_retstatus = E_DB_OK;

    if ( EXdeclare(opcu_handler, &excontext) != OK)
    {
	/* - this exception handler was established to catch any access
	** violations from invalid trees, if the error code has been
	** filled in then a normal error exit is assumed, otherwise
	** this point should only have been reached if an
        ** unexpected exception such as an access violation
        ** occurred
        */
	EXdelete();
	if (error->err_code == E_UL0000_OK)
	{
	    /*
	    ** An exception was processed and it was not
	    ** generated by an internal error signal.
	    */
	    ttcbptr.uld_retstatus = E_DB_SEVERE;
	}
	return(ttcbptr.uld_retstatus);
    }
    ttcbptr.handle = handle;
    textcb = (OPC_TTCB *) handle;
    ttcbptr.uld_mode = qmode;
    ttcbptr.uld_rnamep = rnamep;
    ttcbptr.adfcb = adfcb;
    ttcbptr.ulmcb = ulmcb;

    if ((linelength > ULD_TSIZE) || (linelength <= 0))
	ttcbptr.uld_linelength = ULD_TSIZE;
    else
	ttcbptr.uld_linelength = linelength;

    ttcbptr.pretty = pretty;

    ttcbptr.uld_qroot = qroot;
    ttcbptr.resdomflg = resdomflg;
    ttcbptr.qualonly = qualonly;
    ttcbptr.language = language;
    switch(language)
    {
    case DB_QUEL:
	ttcbptr.is_quel = TRUE;
	ttcbptr.is_sql = FALSE;
	break;
    case DB_SQL:
	ttcbptr.is_quel = FALSE;
	ttcbptr.is_sql = TRUE;
	break;
    default:
	opcu_msignal( (ULD_TTCB * ) &handle, E_UL030A_BAD_LANGUAGE,
		E_DB_ERROR, 0);
    }

    ttcbptr.uld_primary.uld_tstring = tstring;

    /* initialize temp buffer state */
    ttcbptr.uld_primary.uld_remaining = ttcbptr.uld_linelength;
    ttcbptr.uld_primary.uld_offset = 0;    
    ttcbptr.uld_primary.uld_tempbuf[0] = 0;

    ttcbptr.uld_secondary.uld_remaining = ttcbptr.uld_linelength;
    ttcbptr.uld_secondary.uld_offset = 0;    
    ttcbptr.uld_secondary.uld_tempbuf[0] = 0;
    
    {	OPS_SUBQUERY *subquery  = textcb->subquery;
	OPS_STATE *global = subquery->ops_global;

# ifdef OPT_F086_DUMP_QTREE2
	if (global->ops_cb->ops_check &&
	    opt_strace(global->ops_cb, OPT_F086_DUMP_QTREE2))
	{	/* op214 - dump parse tree in op146 style */
	    opu_qtprint(global, qroot, "STAR tree");
	}
# endif
        if (global->ops_cb->ops_check &&
                opt_strace(global->ops_cb, OPT_F042_PTREEDUMP))    
        {         /* op170 - dump parse tree */
	    opt_pt_entry(global, global->ops_qheader, qroot, "STAR tree");
        }
    }

    if ( finish )
    {
	/* 
	** Finish the query.  This currently means that if a where
	** clause was generated that contains a subselect, a closing
	** parenthesis must be printed.  A sort clause must be
	** generated if there was a sort node at the root of
	** the CO tree and this is the final subquery.  And, if this 
	** was a 'select' stmt, possibly
	** a GROUP BY clause must be created.
	*/
	if ( textcb->cop == textcb->subquery->ops_bestco &&
	      textcb->qryhdr->opq_q11_duplicate != OPQ_1DUP)
	{
	    /*
	    ** The current CO node is the root of
	    ** the subquery's CO tree
	    */

	    if (textcb->subquery->ops_localres != OPV_NOVAR)
	    {
		opcu_tidjoin( &ttcbptr );
	    }	    

	    if (textcb->qryhdr->opq_q14_need_close)
	    {
		opcu_printf( &ttcbptr, ")" );
	    }	    

	    if (ttcbptr.uld_mode == PSQ_RETRIEVE ||
		ttcbptr.uld_mode == PSQ_RETINTO ||
		ttcbptr.uld_mode == PSQ_DEFCURS )
	    {
		opcu_groupby( &ttcbptr );
	    }

	    if (textcb->qryhdr->opq_q10_sort_node != NULL &&
		textcb->subquery->ops_sqtype == OPS_MAIN)
	    {
		opcu_sort( &ttcbptr );
	    }

	    if (ttcbptr.uld_mode == PSQ_DEFCURS )
	    {
		/*
		**  Generate a 'for update' clause
		*/

		opcu_for( &ttcbptr );
	    }
	}
    }
    else if (  qroot->pst_sym.pst_type != PST_ROOT &&
		qroot->pst_sym.pst_type != PST_AGHEAD &&
		qroot->pst_sym.pst_type != PST_SUBSEL )
    {	/* only an expression was given */
	bool		end_of_list;


	if ( !notqual )
	{
	    opcu_winit( &ttcbptr );		/* init where clause build */

	    opcu_printf(&ttcbptr, "(");
	}
	do
	{
	    opcu_flush( &ttcbptr, NULL );
	    opcu_prexpr(&ttcbptr, qroot);
	    if (textcb->nulljoin)
	    {
		opcu_nullj( &ttcbptr, (OPC_TTCB *) handle);
	    }
	    if (conjflag == OPCJOINCONJ )
	    {
		end_of_list = opc_jconjunct( (OPC_TTCB *) handle, &qroot); /* while 
					    ** end of list is not
					    ** read continue adding conjuncts from
					    ** caller */
	    }
	    else if (conjflag == OPCCONJ )
	    {
		end_of_list = opc_conjunct( (OPC_TTCB *) handle, &qroot);
	    }
	    else
		end_of_list = TRUE;
	    if (!end_of_list)
		opcu_printf(&ttcbptr, ") AND (");
	    
	}
	while (!end_of_list);
	if ( !notqual )
	{
	    opcu_printf(&ttcbptr, ")");
	    opcu_wend( &ttcbptr );		/* finish where clause build */
	}
	else
	{
	    opcu_flush( &ttcbptr, NULL );
	}
    }
    else
    {	/* query tree root was supplied so translate entire query */
	switch(language)
	{
	case DB_QUEL:
	    opcu_quel(&ttcbptr);		/* return QUEL text */
	    break;
	case DB_SQL:
	    opcu_sql(&ttcbptr);		/* return SQL text */
	    break;
	default:
	    opcu_msignal( (ULD_TTCB * ) &handle, E_UL030A_BAD_LANGUAGE,
		E_DB_ERROR, 0);
	}
    }

    textcb->joinstate = OPT_JNOTUSED;	    /* fix for b41505, initialize join state */
    opcu_flush(&ttcbptr, (ULD_TSTRING *)NULL); /* move all data out of temporary buffer and
					    ** place into linked list */
    EXdelete();                             /* delete exception handler */
    return(ttcbptr.uld_retstatus);			    
}

