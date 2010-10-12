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
#include    <st.h>
#include    <bt.h>
#include    <me.h>
#include    <tr.h>
#include    <cv.h>
#include    <uls.h>
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
#define        OPC_TREETEXT      TRUE
#include    <opcd.h>
#include    <opxlint.h>
#include    <opclint.h>

#define             OPC_MAXTEXT         72
/* max number of characters for a single line of text, in the query tree to
** text conversion */

/*
** History:
**      31-mar-88 (seputis)
**          initial creation
**	24-jan-91 (stec)
**	    Fixed bug 33995 in opc_etoa().
**	    General cleanup of spacing, identation, lines shortened to 80 chars.
**	    Changed code to make certain that BT*() routines use initialized
**	    size of bitmaps, when applicable.
**      23-jun-93 (shailaja)
**          Used cast to queit ansi compiler
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	30-jun-93 (rickh)
**	    Added TR.H and CV.H.
**	02-sep-93 (barbara)
**	    Delimiting of names is based on existence of DB_DELIMITED_CASE
**	    capability in the LDB, not on OPEN/SQL_LEVEL. 
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	21-mar-95 (brogr02)
**	    Prevent translation of duplicate boolean factors (for STAR)
**	24-may-1996 (angusm)
**		Changes to fix w.r. duplicate boolean factors (bug 75990)
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	24-Aug-2009 (kschendel) 121804
**	    Fix up warnings.
**	    Need uls.h to satisfy gcc 4.3.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
[@history_template@]...
*/

/*
**  DRHFIXME - this belongs in the UL header.  I copied it here from
**  uldtreetext to get this to compile.
** Name: ULD_TSTATE - state of a building a linked list of text blocks
**
** Description:
**      A linked list of text blocks is created when converting a query
**      tree into text.  This structure describes the state of the linked
**      list and can be used to save the state of partially built linked
**      list.
**
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

/*{
** Name: opc_addseg	- Add a text segment to query text list
**
** Description:
**      Similar to opc_tsegstr, this routine will create a DD_PACKET
**	from either a string or a number that indicates
**	a temporary table name and add it to a text segment list. 
**
** Inputs:
**      global				global opc state variable
**      stringval                       string to make segment from
**	stringlen			length of the string, or 0
**	temp_num			temp table id, or -1 if string
**      seglist                         text segment list to add it to
**
** Outputs:
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      08-sep-88 (robin)
**          Created.
**	01-dec-88 (robin)
**	    Modified to build a DD_PACKET instead of a text segment
**	15-feb-93 (ed)
**	    fix prototype casting errors
*/
VOID
opc_addseg(
	OPS_STATE	   *global,
	char               *stringval,
	i4		   stringlen,
	i4		   temp_num,
	QEQ_TXT_SEG	   **seglist )

{
    QEQ_TXT_SEG		*segptr;
    DD_PACKET		*pktptr;
    i4                  size;

    size = sizeof( DD_PACKET );
    if (*seglist == NULL)
    {
	size += sizeof( QEQ_TXT_SEG );
    }

    if (temp_num <= -1 )
    {
	size +=  stringlen;
    }
    segptr = (QEQ_TXT_SEG *) opu_qsfmem( global, size);

    if (*seglist == NULL )
    {

	if (temp_num <= -1 )
	{
	    segptr->qeq_s1_txt_b = TRUE;	/* not a temp name */

	}
	else
	{
	    segptr->qeq_s1_txt_b = FALSE;	/* this is a temp table */
	}
	pktptr = (DD_PACKET *) (((PTR) segptr) + sizeof( QEQ_TXT_SEG ));
	segptr->qeq_s2_pkt_p = pktptr;
	segptr->qeq_s3_nxt_p = (QEQ_TXT_SEG *) NULL;
	*seglist = segptr;
    }
    else
    {
	pktptr = (*seglist)->qeq_s2_pkt_p;
	for (; pktptr->dd_p3_nxt_p != NULL;
	    pktptr = pktptr->dd_p3_nxt_p);
	pktptr->dd_p3_nxt_p = (DD_PACKET *) segptr;
	pktptr = (DD_PACKET *) segptr;
    }
    if (temp_num <= -1 )
    {
	/* this is text, not temp */

	pktptr->dd_p1_len = stringlen;;
	pktptr->dd_p2_pkt_p = (char *) (((PTR) pktptr) + sizeof(DD_PACKET));
	pktptr->dd_p4_slot = DD_NIL_SLOT;
	MEcopy( (PTR)stringval, stringlen, (PTR)pktptr->dd_p2_pkt_p);
    }
    else
    {
	pktptr->dd_p1_len = 0;
	pktptr->dd_p2_pkt_p = NULL;
	pktptr->dd_p4_slot = temp_num;
    }
    pktptr->dd_p3_nxt_p = NULL;
    return;
}

/*{
** Name: opc_atname	- get attribute name
**
** Description:
**      Routine will return the name of the attribute 
**      For temporary relations it will use the attribute number as the
**      name but attempt to suffix the name of the attribute of the
**      base relation if possible.  Based on opt_paname.
**
** Inputs:
**      subquery                          context within which to find name
**      attno                             joinop attribute number
**	forcetemp			  flag - use temporary-style name
**					    even if there appears to be a
**					    non-temp base relation.
**	attrname			  location to update with attribute
**					    name
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
**      30-sep-88 (robin)
**          Created from opt_paname
**	28-mar-89 (robin)
**	    Added textcb parameter, and special case handling for attributes
**	    named by their resdom number.
**	26-apr-89 (robin) 
**	    Moved tid check below function aggregates and forectemp to
**	    handle tids being carried up as regular attributes in temp
**	    tables.  Fix for bug 5473.
**	12-jul-89 (robin)
**	    Added union and union view test to get appropriate attribute
**	    names from temp tables created for these.
**	01-mar-93 (barbara)
**	    Delimit column names if LDB's OPEN/SQL_LEVEL >= 65.
**	26-apr-93 (ed)
**	    - remove obsolete code after fix to 49609
**	11-aug-93 (ed)
**	    - changed to use opt_catname to fix alpha stack overwrite
**	    problem
*/
VOID
opc_atname(
	OPC_TTCB	   *textcb,
	OPS_SUBQUERY       *subquery,
	OPZ_IATTS          attno,
	bool		   forcetemp,
	OPT_NAME           *attrname )
{
    OPS_STATE           *global;    /* ptr to global state variable */
    DD_CAPS		*cap_ptr;   /* ptr to LDB capabilities */
    OPD_SITE		*sitedesc;
    OPD_ISITE   	site;

    global = subquery->ops_global;  /* ptr to global state variable */

    if (attno < 0)
    {
	STcopy( " ", (PTR)attrname);
	return;
    }
    {
	/* copy the attribute name to the output buffer */
	DB_ATT_ID          dmfattr;
	i4                length;
	char		   *att_name;
	OPZ_ATTS           *attrp;
	OPV_GRV            *grvp;
	OPV_VARS	    *lrvp;
	i4		    idno;

	attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
	dmfattr.db_att_id = attrp->opz_attnm.db_att_id;
	if ( sizeof(DB_ATT_NAME) >= sizeof(OPT_NAME))
	    length = sizeof(OPT_NAME)-1;
	else
	    length = sizeof(DB_ATT_NAME);

	lrvp = subquery->ops_vars.opv_base->opv_rt[attrp->opz_varnm];
	grvp = lrvp->opv_grv;

	/*
	**  Note that the order of the following three IF statements is
	**  important.  When tids are being carried up for duplicate semantics
	**  into a temp table, we want to reference them using their attribute
	**  name IN THE TEMP (i.e. 'e1' or 'a1') not 'tid'.
	*/

	/* make sure that temp table names are checked before looking at
	** OPS_FAGG etc. a1 names, otherwise queries which use aggregate
	** temps will fail if a transfer action is used */

	/*
	**  Attribute names in temporary tables are handled specially here
	*/
	if (forcetemp)	    /* forcing temp attribute name because a temp
			    ** table was created for distributed */
	{
	    attrname->buf[0] = 'e';	    /* column name prefix */
            CVla( (i4)attrp->opz_equcls, (PTR)&attrname->buf[1]);

	    return;
	}

	if ( grvp->opv_created == OPS_FAGG ||
	     grvp->opv_created == OPS_UNION ||
	     grvp->opv_created == OPS_VIEW )
	{
	    /*
	    **  This variable is a materialized function aggregate, union,
	    **  or union view component.
	    */
	    
	    attrname->buf[0] = 'a';	    /* column name prefix */
            CVla( (i4)attrp->opz_attnm.db_att_id, (PTR)&attrname->buf[1]);
	    return;	
	}

	if (dmfattr.db_att_id == DB_IMTID)
	{
	    STcopy( "tid ", (PTR)attrname);
	    site = textcb->cop->opo_variant.opo_local->opo_operation;
	    sitedesc = textcb->state->ops_gdist.opd_base->opd_dtable[site];
	    cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps;

	    if(!(cap_ptr->dd_c1_ldb_caps & DD_2CAP_INGRES))  /* not ingres */
		/* putting "tid" in select list can give wrong results */
		opx_error(E_OP068C_BAD_TARGET_LIST);
	    return;
	}

	if (!grvp->opv_relation || forcetemp )
	{   /* special processing for temporary relation */
	    OPV_IGVARS  gvar;               /* global variable number */
	    for (gvar = 0; 
		grvp != global->ops_rangetab.opv_base->opv_grv[gvar];
		gvar++);
					    /* look in global range table for
                                            ** global range variable number */
	    if (lrvp->opv_itemp > OPD_NOTEMP )
		idno = lrvp->opv_itemp;
	    else
		idno = gvar;
	    attrname->buf[0] = 't';	    /* global table number */
            CVla( (i4)idno, (PTR)&attrname->buf[1]);
	    STcat( (char *)attrname, "a");  /* place attr number at end */
	    CVla( (i4)dmfattr.db_att_id,
		(PTR)&attrname->buf[STlength((char *)attrname)]);

	    {
		/* look for the resdom associated with the varnode of the
                ** temporary and get the attribute name if available */
		OPS_SUBQUERY	    *tempsubquery;
		PST_QNODE	    *qnode;
		for (tempsubquery = global->ops_subquery;
		    tempsubquery && (tempsubquery->ops_gentry != gvar);
		    tempsubquery = tempsubquery->ops_next); /* find the
					    ** subquery which produced
					    ** the temporary */
		if ((!tempsubquery) && grvp->opv_gsubselect)
		{   /* could also be a subselect subquery which has been
                    ** detached from the subquery list */
		    tempsubquery = grvp->opv_gsubselect->opv_subquery;
		}
		if (tempsubquery)
		{
		    for (qnode = tempsubquery->ops_root->pst_left;
			 qnode; qnode = qnode->pst_left)
		    {   /* find resdom used to define this attribute */
			if (qnode->pst_sym.pst_type == PST_RESDOM &&
			    qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno
				== dmfattr.db_att_id)
			{
			    /*
			    ** Found node so find name of attribute in 
			    ** a var node use to define this resdom
			    */
			    opt_catname(tempsubquery, qnode->pst_right, 
				attrname);
			    break;
			}
		    }
		}
	    }
	    return;
	}

	/*
	**  Handle attribute names for funtion attributes
	*/

	if (dmfattr.db_att_id == OPZ_FANUM)
	{   /* attempt to add name of attribute upon which function attribute
            ** is based to the name */
	    OPZ_FATTS		*fattp;	    /* ptr to function attribute being
					    ** analyzed */
	    fattp = subquery->ops_funcs.opz_fbase->
		    opz_fatts[attrp->opz_func_att];
            /* traverse the query tree fragment representing the  function
            ** attribute and obtain the name of the first node which would
            ** help identify the function
            ** - the name will be constructed as follows
            ** - FA <unique number> <name of attribute in function>
            */
	    STcopy( "fa", (char *)attrname); /* function attribute */
            CVna( (i4)fattp->opz_fnum, (char *)&attrname->buf[2] ); /* unique 
					    ** identifier for function attribute
					    */
	    opt_catname(subquery, fattp->opz_function, attrname);
	    return;
	}

	if ( grvp->opv_relation->rdr_obj_desc->dd_o9_tab_info.dd_t6_mapped_b )
	{
	    att_name = &grvp->opv_relation->rdr_obj_desc->
		dd_o9_tab_info.dd_t8_cols_pp[ dmfattr.db_att_id ]->
		dd_c1_col_name[0];	
	}
	else
	{
	    att_name = grvp->opv_relation->rdr_attr[dmfattr.db_att_id]->att_nmstr;
	    length = grvp->opv_relation->rdr_attr[dmfattr.db_att_id]->att_nmlen;
	}
	length = opt_noblanks((i4)length, (char *)att_name);

	/* BB_FIXME: May have to test if textcb->cop is null first */

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
	    attrname->buf[length]=0;		/* null terminate the string */
	}
	return;
    }
}

/*{
** Name: opc_rdfname	- get the RDF table name
**
** Description:
**      This routine will retrieve the RDF name given the global table
**	information 
**
** Inputs:
[@PARAM_DESCR@]...
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
**      30-may-1989 (robin)
**	    Modified to call uls_ownname to determine whether to use
**	    owner name, whether to quote it, and what quote chars
**	    to use.  Added qmode as a parameter.  Fix for bug 5485.
**
**	03-aug-89 (robin)
**	    Fixed quote_char assignment from quotec, to correct
**	    Unix compile error.
**	17-apr-90 (georgeg)
**	    On an UPDATE CURSOR with ~V's textcb->cop is NULL, applied 
**	    a big kludge to fix it, this problem should not be fixed here.
**      26-mar-91 (fpang)
**          Renamed opdistributed.h to opdstrib.h
**	01-mar-93 (barbara)
**	    Delimit owner and table names if LDB's OPEN/SQL_LEVEL >= 605.
[@history_line@]...
[@history_template@]...
*/
VOID
opc_rdfname(
	OPC_TTCB	   *textcb,
	OPV_GRV	           *grv_ptr,
	OPCUDNAME	   *name,
	i4		   qmode )
{
    DD_OWN_NAME	*ownername;
    DD_TAB_NAME	*tabname;
    DD_CAPS	*cap_ptr;
    OPD_ISITE	site;
    OPD_SITE	*sitedesc;
    i4          tabnamesize;
    i4		ownnamesize;
    char	*bufptr;
    char	quotec[6];
    char	*quote_char;
    bool	use_owner_name;
    u_i4	unorm_len = (DB_OWN_MAXNAME + DB_TAB_MAXNAME) + 2;
    DB_STATUS	status;
    DB_ERROR	error;


    /*
    **  Determine whether to prefix the table name with the owner name, 
    **  and, if so, whether to quote it, and the quote style.
    */
    if (textcb->cop == (OPO_CO *)NULL)
    {
	/*
	** Update cursor with ~V does not work, it AV's because textcb->cop
	**is NULL this fix will make it work, it is not the proper place to
	** fix it but this is what Ed proposes.
	*/
	site = OPD_TSITE;
    }
    else
    { 
	site = textcb->cop->opo_variant.opo_local->opo_operation;
    }
    sitedesc = textcb->state->ops_gdist.opd_base->opd_dtable[site];
    cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps ;
    quote_char = quotec;
    ownername = (DD_OWN_NAME *) &grv_ptr->opv_relation->rdr_obj_desc->
	    dd_o9_tab_info.dd_t2_tab_owner[0];
    ownnamesize = opt_noblanks((i4)DB_OWN_MAXNAME, (char *) ownername);
    bufptr = (PTR) &name->buf[0];

    if (cap_ptr->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
    {
	status = cui_idunorm( (u_char *)ownername, (u_i4 *)&ownnamesize,
			(u_char *)bufptr, &unorm_len, CUI_ID_DLM, &error);
	if (DB_FAILURE_MACRO(status))
	    opx_verror(E_DB_ERROR, E_OP08A4_DELIMIT_FAIL, error.err_code);
	bufptr += unorm_len;
	*bufptr++ = '.';
    }
    else
    {
    	use_owner_name = uls_ownname( cap_ptr, qmode, 
	    textcb->state->ops_adfcb->adf_qlang, &quote_char );

	if (use_owner_name)
    	{
	    if (quote_char != NULL)
	    {    
	    	*bufptr++ = *quote_char;
	    }
	    MEcopy( (PTR)ownername, ownnamesize, (PTR)bufptr );
	    bufptr += ownnamesize;
 	    if (quote_char != NULL)
	    {
	    	*bufptr++ = *quote_char;
	    }
	    *bufptr++ = '.';
	}
    }

    tabname = (DD_TAB_NAME *) &grv_ptr->opv_relation->rdr_obj_desc->
	dd_o9_tab_info.dd_t1_tab_name[0];
    tabnamesize = opt_noblanks( (i4)DB_TAB_MAXNAME, (char *)tabname);

    MEcopy( (PTR)tabname, tabnamesize, (PTR)bufptr );

    if (cap_ptr->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
    {
	/* Unnormalize and delimit tablename */
	unorm_len = (DB_OWN_MAXNAME + DB_TAB_MAXNAME) + 2;
	status = cui_idunorm( (u_char *)tabname, (u_i4*)&tabnamesize,
		    (u_char *)bufptr, &unorm_len, CUI_ID_DLM, &error);
	if (DB_FAILURE_MACRO(status))
	    opx_verror(E_DB_ERROR, E_OP08A4_DELIMIT_FAIL,
					error.err_code);
	bufptr += unorm_len;
    }
    else 
    {
	MEcopy( (PTR)tabname, tabnamesize, (PTR)bufptr );
	bufptr += tabnamesize;
    }
    *bufptr = '\0';
}

/*{
** Name: opc_nat	-   Get table alias, and name or number
**
** Description:
**      Provided with the local range table number of a relation,
**	return to the caller:
**	    - an alias name created from the global range table number
**	      (or the temp table number + OPV_MAXVAR) of the form 'Tn'.
**	      Note that for correlated DELETE and UPDATE queries, the
**	      alias will be the tablename.
**	    - the table name, if it is not a temporary table
**	    - the table number if the table is a temporary.
**	      This will be used as the table ID for distributed QEF. 
**	Ops_subquery should be pointing to the current subquery 
**	referred to by the local range table number.
**
** Inputs:
**	textcb		Conversion state descriptor
**      statevar        Ptr to opf state variable
**	varno		Local range table number of
**			the table to look up.
**	name		Buffer to put table name in
**	alias		Buffer to put table alias in
**	tempno		Ptr to a i4  to update with 
**			the temp no., or -1 if
**			not a temp
**	is_result	Will be set to TRUE if the table is the 'result
**			table' for a delete or update.
**
** Outputs:
**	Returns:
**	    void.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-oct-88 (robin)
**          Created.
**      30-mar-89 (robin)
**	    Move function aggregate check below the local range
**	    variable check for a temp
**      01-may-89 (robin)
**	    Fixed bug 5643, so is_result is set correctly for
**	    updatable define cursor.
**      19-may-89 (robin)
**	    Changed is_result test to use OPC_NOFIXEDSITE instead
**	    of OPD_TOUSER.  Partial fix for bug 5481.
**	31-may-1989 (robin)
**	    Added qmode as input parameter since it is required
**	    by opc_rdfname. Partial fix for bug 5485.
**      08-jun-89 (robin)
**	    Added OPS_UNION and OPS_VIEW to test on opv_created as
**	    part of union/unionview support.
**	12-jun-90 (seputis)
**	    Add e_att_flag to indicate whether "e" names or "a" names
**	    should be used for attributes
**      21-oct-2004 (rigka01, crossed from unsubm 2.5 IP by hayke02)
**        Use base table rather than temporary table if there is no from
**        clause and we are processing the target list of an update query
**        (OPD_NOFROM). This change fixes bug 108409
[@history_line@]...
[@history_template@]...
*/
VOID
opc_nat(
	OPC_TTCB	  *textcb,
	OPS_STATE         *statevar,
	i4		   varno,
	OPCUDNAME	   *name,
	OPT_NAME	   *alias,
	i4		   *tempno,
	bool		   *is_result,
	i4		   qmode,
	bool		   *e_att_flag )
{
    OPV_GRV	     *grv_ptr;  /* ptr to global range variable */
    OPV_VARS     *lrv_ptr;	/* local range var pointer */
    i4	    tableno;


    *tempno = OPD_NOTEMP;
    name->buf[0] = '\0';
    alias->buf[0] = '\0';
    *is_result = FALSE;

    /*  Access the global range variable through the local range 
    **  table.  Start by getting a ptr to the global range table
    **  element.
    */
    lrv_ptr = textcb->subquery->ops_vars.opv_base->opv_rt[varno];
    grv_ptr = lrv_ptr->opv_grv; 


    if ( lrv_ptr->opv_itemp != OPD_NOTEMP 
      && !(textcb->subquery->ops_global->ops_gdist.opd_gmask & OPD_NOFROM)
	&& textcb->qryhdr->opq_q7_tablist != NULL)  /* defensive programming */
    {
	/*
	** A temporary table was created as part of processing the current
	** subquery that replaces the base table reference.
	** Use the temporary.
	*/

	*tempno = lrv_ptr->opv_itemp;
	tableno = (*tempno) + OPV_MAXVAR;
	*e_att_flag = TRUE;		/* use "e" type attribute names */
    }
    else if (grv_ptr->opv_created == OPS_FAGG ||
	     grv_ptr->opv_created == OPS_UNION ||
	     grv_ptr->opv_created == OPS_VIEW
	    )
    {
	/*
	**  This variable is in an aggregate temp table created from
	**  a previously processed subquery.
	*/
	*tempno = grv_ptr->opv_gquery->ops_compile.opc_distrib.opc_subqtemp;
	tableno = (*tempno) + OPV_MAXVAR;
	*e_att_flag = FALSE;		/* use "a" type attribute names */
    }
    else if ( textcb->subquery->ops_dist.opd_create != OPD_NOTEMP )
    {
	/*
	** A table was created to preserve duplicate semantics
	*/

	*tempno = textcb->subquery->ops_dist.opd_create;
	tableno = (*tempno) + OPV_MAXVAR;
	*e_att_flag = TRUE;		/* use "e" type attribute names */
    }
    else
    {
	/*
	**  Get the offset of this range variable in the global range
	**  table.  This will be used to build the alias name.
	*/

	tableno = lrv_ptr->opv_gvar;

	/*
	**  Create the alias name using the global range variable offset
	*/
        *e_att_flag = FALSE;		/* use "a" type attribute names */


	if (grv_ptr->opv_relation)
	{
	    /* there is a name if RDF information has been found - otherwise
	    ** it is a temporary relation produced by aggregates */
	    opc_rdfname(textcb, grv_ptr, name, qmode);

	    if (textcb->subquery->ops_sqtype == OPS_MAIN &&
		( !(statevar->ops_gdist.opd_gmask & OPD_NOFIXEDSITE) ) &&
		statevar->ops_qheader->pst_restab.pst_resvno == tableno )
	    {
		/*
		** This is an update or delete statement or updateable define
		** cursor statement, and the table being checked is the table
		** to be updated, i.e. the 'result table'.  
		*/

		*is_result = TRUE;
	    }
	}
	else
	{
	    /*
	    **  It's a temporary table. Need to return the temp number.
	    */

	    *tempno = tableno;
	}
    }

    alias->buf[0] = '\0';
    opcu_getrange( (OPCQHD *) textcb->qryhdr, tableno, &alias->buf[0] );
     
    return;
}


/*{
** Name: opc_one_etoa	- convert equivalence class to ONE attribute
**
** Description:
**      Routine accepts an equilvalence class and initializes the resexpr
**	symbol table node in the handle with attribute information. This
**	routine is similar to opc_etoa(), on which it was based, except
**	that it looks for one attribute, not 2.
**	It is used in building tid joins, where we already know the tid
**	half of the join, and are looking for the equivalence class in
**	the temp that contains the joining tid.
**
** Inputs:
**      textcb      creation state variable
**	eqcls	    equivalence class used to initialize
**                  range variable node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	Puts the attribute info in the RESEXPR node of the handle.
**
** History:
**      25-apr-89 (robin)
**          initial creation from opc_etoa for use in building tid join
**	18-jul-90 (seputis)
**	    fix access violation, if inner cop does not exist
[@history_line@]...
[@history_template@]...
*/
VOID
opc_one_etoa(
	OPC_TTCB	    *textcb,
	OPE_IEQCLS	    eqcls )
{
    OPS_SUBQUERY    *subquery;
    OPZ_BMATTS	    *attrmap;	    /* bitmap of attributes in eqc. */
    OPZ_AT	    *abase;
    OPZ_IATTS	    new_attno;
    bool	    found;	    /* an attribute was found */
    OPO_CO	    *cop;

    subquery = textcb->subquery;
    abase = subquery->ops_attrs.opz_base;
    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_attrmap;
    found = FALSE;
    cop = textcb->cop;

    for (new_attno = -1;
	 (new_attno = BTnext((i4)new_attno, (char *)attrmap,
	    (i4)subquery->ops_attrs.opz_av)) >= 0;
	)
    {
	/*
	** Look thru the list of joinop attribute associated with this
	** equivalence class and determine which ones exist in the
	** subtree associated with this CO node.
	*/
	OPZ_ATTS	*attrp;

	attrp = abase->opz_attnums[new_attno];
	if ( BTtest((i4)attrp->opz_varnm, 
	    (char *)cop->opo_outer->opo_variant.opo_local->opo_bmvars))
	{
	    /* range variable for this attribute is in subtree */
	    found = TRUE;
	    STRUCT_ASSIGN_MACRO(attrp->opz_dataval,
		textcb->qnodes.resexpr.pst_sym.pst_dataval);
	    textcb->qnodes.resexpr.pst_sym.pst_type = PST_VAR;
	    textcb->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_vno =
		attrp->opz_varnm;
	    textcb->qnodes.resexpr.pst_sym.pst_value.pst_s_var.
					pst_atno.db_att_id = new_attno;
	    break;			    /* var node is initialized */
	}
	else if( cop->opo_inner && BTtest((i4)attrp->opz_varnm, 
	    (char *)cop->opo_inner->opo_variant.opo_local->opo_bmvars))
	{
	    found = TRUE;
	    STRUCT_ASSIGN_MACRO(attrp->opz_dataval,
		textcb->qnodes.resexpr.pst_sym.pst_dataval);
	    textcb->qnodes.resexpr.pst_sym.pst_type = PST_VAR;
	    textcb->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_vno 
		= attrp->opz_varnm;
	    textcb->qnodes.resexpr.pst_sym.pst_value.pst_s_var.
					pst_atno.db_att_id = new_attno;
	    break;			    /* var node is initialized */
	}
    }

    /* FIXME - looks like "found" is not used, make it a consistency check */

    return;
}

/*{
** Name: opc_etoa	- convert equivalence class to attributes
**
** Description:
**      Routine accepts an equilvalence class and initializes PST_VAR nodes
**      such that a join exists
**
** Inputs:
**      textcb                          text creation state variable
**	eqcls				equivalence class used to initialize
**                                      range variable nodes
**
** Outputs:
**	nulljoin			Will be set to TRUE if a null join is
**					required, FALSE otherwise.
**	Returns:
**	    TRUE if join node successfully created
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-apr-88 (seputis)
**          initial creation
**	24-jan-91 (stec)
**	    Fixed bug 33995. It is possible to have a Kjoin with a multi-
**	    attribute key, such that some attribute values will be constants.
**	    Therefore they will not be specified in both inner and outer.
**	    To determine whether we are dealing with Kjoins, we will check
**	    the inner; it must be an ORIG node. Conjuncts eliminated in this
**	    way will be picked from the boolean factor bitmap specified for
**	    the node, as result generated query will have all constraints
**	    necessary.
**	2-jun-91 (seputis)
**	    jan 24 fix for 33995 fixed a symptom but not the problem
**	    fix for b37348
**	09-jul-93 (andre)
**	    recast first arg to pst_resolve() to (PSF_SESSCB_PTR) to agree
**	    with the prototype declaration
*/
bool
opc_etoa(
	OPC_TTCB	    *textcb,
	OPE_IEQCLS	    eqcls,
	bool		    *nulljoin )
{
    OPS_SUBQUERY    *subquery;
    OPZ_BMATTS	    *attrmap;	    /* bitmap of attributes in eqc. */
    OPZ_AT	    *abase;
    OPZ_IATTS	    new_attno;
    bool	    inner_found;    /* an attribute from the inner was found */
    bool	    outer_found;    /* an attribute from the outer was found */

    textcb->joinstate = OPT_JOUTER; /* use outer bitmap first */
    subquery = textcb->subquery;
    BTclear ((i4) eqcls, (char *)&textcb->bmjoin); /* eqc has been processed */
    abase = subquery->ops_attrs.opz_base;
    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_attrmap;
    *nulljoin = (bool) subquery->ops_eclass.ope_base->
	ope_eqclist[eqcls]->ope_nulljoin;
    inner_found = FALSE;
    outer_found = FALSE;
    for (new_attno = -1;
	 (new_attno = BTnext((i4)new_attno, (char *)attrmap,
	    (i4)subquery->ops_attrs.opz_av)) >= 0;
	)
    {
	/*
	** Look thru the list of joinop attribute associated with this
	** equivalence class and determine which ones exist in the
	** subtree associated with this CO node
	*/
	OPZ_ATTS    *attrp;

	attrp = abase->opz_attnums[new_attno];
	if (!outer_found &&
	    BTtest((i4)attrp->opz_varnm, 
	    (char *)textcb->cop->opo_outer->opo_variant.opo_local->opo_bmvars))
	{
	    /* range variable for this attribute is in subtree */

	    outer_found = TRUE;
	    STRUCT_ASSIGN_MACRO(attrp->opz_dataval,
		textcb->qnodes.leftvar.pst_sym.pst_dataval);
	    textcb->qnodes.leftvar.pst_sym.pst_value.pst_s_var.pst_vno =
							    attrp->opz_varnm;
	    textcb->qnodes.leftvar.pst_sym.pst_value.pst_s_var.
						pst_atno.db_att_id = new_attno;
	    if (inner_found)
		break;			    /* both var nodes are initialized */
	}
	else if(!inner_found &&
	    BTtest((i4)attrp->opz_varnm, 
	    (char *)textcb->cop->opo_inner->opo_variant.opo_local->opo_bmvars))
	{
	    inner_found = TRUE;
	    STRUCT_ASSIGN_MACRO(attrp->opz_dataval,
		textcb->qnodes.rightvar.pst_sym.pst_dataval);
	    textcb->qnodes.rightvar.pst_sym.pst_value.pst_s_var.pst_vno 
		= attrp->opz_varnm;
	    textcb->qnodes.rightvar.pst_sym.pst_value.pst_s_var.
						pst_atno.db_att_id = new_attno;
	    if (outer_found)
		break;			    /* both var nodes are initialized */
	}
    }

    if (inner_found && outer_found)
    {
	/* need to resolve the types for the new BOP equals node */

	DB_STATUS	res_status;
	DB_ERROR	error;

	res_status = pst_resolve((PSF_SESSCB_PTR) NULL,
	    subquery->ops_global->ops_adfcb, 
	    &textcb->qnodes.eqnode, 
	    subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang,
	    &error);

	if (DB_FAILURE_MACRO(res_status))
	    opx_verror( res_status, E_OP0685_RESOLVE, error.err_code);
	return(TRUE);
    }
    else
    {
	/* Report error in all cases but Kjoin (bug fix for #33995). */
	if (!(inner_found && textcb->cop->opo_inner->opo_sjpr == DB_ORIG))
	{
	    opx_error( E_OP0498_NO_EQCLS_FOUND); /* both eqcs must be found */
	}
	return(FALSE);
    }
}

/*{
** Name: opc_dstring - display string list to user
**
** Description:
**      This routine will print a ULD_TSTRING list to the user
**
** Inputs:
**      subquery        subquery state variable
**      tstring         linked list of strings to be printed
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
**      11-apr-88 (seputis)
**          initial creation
**      23-jun-93 (shailaja)
**	    used cast to quiet ansi compiler.   
[@history_template@]...
*/
static VOID
opc_dstring(
	OPS_SUBQUERY	    *subquery,
	char		    *header_string,
	ULD_TSTRING	    *tstring,
	i4		    maxline )
{
    OPS_STATE	    *global;
    
    global = subquery->ops_global;
    if (tstring)
    {   /* an non-empty boolean expression was created */
	i4	    current;
	current = 0;

	TRformat(opt_scc, (i4 *)NULL,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	     header_string);
	for (;tstring; tstring = tstring->uld_next)
	{
	    if ( (current > 0) &&(((i4)(tstring->uld_length + current) > maxline)))
	    {   /* start a new line since maxline is exceeded */
		TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		     "\n        %s", &tstring->uld_text[0]);
		current = tstring->uld_length;
	    }
	    else
	    {   /* concatenate lines as long as < maxline */
		TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		     "%s", &tstring->uld_text[0]);
		current += tstring->uld_length;
	    }
	}
    }
}

/*{
** Name: opc_mapfunc - determine if function attribute can be evaluated
**		       in CO tree
**
** Description:
**	This routine will determine if the function attribute can be evaluated
**      in the CO subtree.
**
** Inputs:
**	textcb         state variable for tree to text 
**                     conversion
** Outputs:
**	Returns:
**	    TRUE if the function attribute is evaluated in the subtree
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-apr-88 (seputis)
**          initial creation
**	01-sep-92 (rickh)
**	    When this routine called itself, it tagged an extra level of
**	    indirection onto the last argument.  I remove the extra &s.
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
[@history_template@]...
*/
static bool
opc_mapfunc(
	OPS_SUBQUERY	    *subquery,
	OPO_CO		    *cop,
	OPE_IEQCLS	    eqcls,
	OPE_BMEQCLS         *func_eqcmap,
	OPE_BMEQCLS         *temp_eqcmap )
{
    bool	no_lower;   /* TRUE if function attribute is not 
			    ** calculated at this node
			    */

    no_lower = TRUE;
    if (cop->opo_outer && BTtest((i4)eqcls, (char *)&cop->opo_outer->opo_maps->opo_eqcmap))
    {
	no_lower = FALSE;
	if (opc_mapfunc(subquery, cop->opo_outer, eqcls,
		func_eqcmap, temp_eqcmap))
	    return(TRUE);
    }
    if (cop->opo_inner && BTtest((i4)eqcls, (char *)&cop->opo_inner->opo_maps->opo_eqcmap))
	return(opc_mapfunc(subquery, cop->opo_inner, eqcls,
	    func_eqcmap, temp_eqcmap));

    if (no_lower)
    {	/* 
	** The function attribute must be available at this node or
        ** not at all, find the set of equivalence classes which are
	** available at this node, and determine if it is a super set of the
        ** set of equivalence classes required to evaluate the function
	** attribute
	*/
	if (cop->opo_outer)
	{
	    if (cop->opo_inner)
	    {
		MEcopy ((PTR)&cop->opo_outer->opo_maps->opo_eqcmap,
		    sizeof(OPE_BMEQCLS), (PTR)temp_eqcmap);
		BTor((i4)BITS_IN(OPE_BMEQCLS), (PTR)&cop->opo_inner->opo_maps->opo_eqcmap,
		    (PTR)temp_eqcmap);
	    }
	    else
		temp_eqcmap = &cop->opo_outer->opo_maps->opo_eqcmap;
	}
	else
	{
	    if (cop->opo_inner)
		temp_eqcmap = &cop->opo_inner->opo_maps->opo_eqcmap;
	    else
	    {	/* this is an ORIG node in which the available equivalence
                ** classes are in the variable desciptor, the ORIG node
                ** has equilvalence classes in opo_maps->opo_eqcmap which are the
                ** same as the project-restrict */
		if ((cop->opo_sjpr == DB_ORIG)&& (cop->opo_union.opo_orig >= 0))
		    temp_eqcmap = &subquery->ops_vars.opv_base->
			opv_rt[cop->opo_union.opo_orig]->opv_maps.opo_eqcmap;
		else
		    temp_eqcmap = NULL;
	    }
	}
	return(BTsubset((char *)func_eqcmap, (char *)temp_eqcmap, 
	    (i4)subquery->ops_eclass.ope_ev));
    }
    return(FALSE);
}

/*{
** Name: opc_init - initialize the state variable for tree to text conversion
**
** Description:
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
**      4-apr-88 (seputis)
**          initial creation
**	26-dec-91 (seputis)
**	    fix b41505 - a multi-variable group by
**	       aggregate may result in a syntax error as in
**		select gf.yr, gf.limit, sum(sg.charged) from sg, gf
**		    where sg.attr1 = '56' and gf.yr = sg.yr
**		    group by gf.yr, gf.limit
**	13-jan-93 (puru)
**	    Unknown behaviour will result if we pass negative values
**	    less than -1 to BTnext() in opc_tblnam(). Therefore initialize 
**	    opt_vno to -1 here itself.
[@history_template@]...
*/
static VOID
opc_init(
	OPC_TTCB	   *handle,
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	ULM_RCB		   *ulmrcb,
	QEQ_TXT_SEG	   **wheretxt,
	OPCQHD		   *qthdr,
	bool		    remdup )
{
    handle->subquery = subquery;
    handle->state = subquery->ops_global;
    handle->cop = cop;
    handle->query_stmt = NULL;
    handle->where_cl = *wheretxt;
    handle->qryhdr = qthdr;    
    handle->seglist = NULL;
    handle->hold_targ = NULL;
    handle->opt_ulmrcb = ulmrcb;
    handle->remdup = remdup;
    handle->anywhere = FALSE;
    handle->noalias = FALSE;
    handle->nulljoin = FALSE;
    handle->joinstate = OPT_JNOTUSED; /* remainder of qualifications are not
				    ** used */
    MEcopy((PTR)&subquery->ops_global->ops_mstate.ops_ulmrcb,
	sizeof(ULM_RCB),
	(PTR)ulmrcb);		    /* use copy of ULM_RCB in case
                                    ** it gets used by other parts
                                    ** of OPF */
    ulmrcb->ulm_streamid_p = &subquery->ops_global->ops_mstate.ops_streamid;
				    /* allocate
				    ** memory from the main memory
                                    ** stream */
    MEfill(sizeof(handle->qnodes), (u_char)0, (PTR)&handle->qnodes);
    if ( cop != NULL && cop->opo_sjpr == DB_SJ)
    {
	/*
	** A join has equivalence classes from the left and right sides
        ** but the boolean factor for the join has been removed so it 
        ** needs be to created; use temporary space in the handle state
        ** control block to create each join, one conjunct at a time
        ** to be placed into the qualification */

	/* initialize PST_VAR node for the outer subtree */
	handle->qnodes.leftvar.pst_sym.pst_type = PST_VAR;
	handle->qnodes.leftvar.pst_sym.pst_len = sizeof(PST_VAR_NODE);
	MEfill(
	    sizeof(handle->qnodes.leftvar.pst_sym.pst_value.pst_s_var.pst_atname), 
	    ' ', 
	    (PTR)&handle->
		qnodes.leftvar.pst_sym.pst_value.pst_s_var.pst_atname);
				    /* init attribute name */

	/* initialize PST_VAR node for the inner subtree */
	handle->qnodes.rightvar.pst_sym.pst_type = PST_VAR;
	handle->qnodes.rightvar.pst_sym.pst_len = sizeof(PST_VAR_NODE);
	MEfill(
	    sizeof(handle->qnodes.rightvar.pst_sym.pst_value.pst_s_var.pst_atname), 
	    ' ', 
	    (PTR)&handle->
		qnodes.rightvar.pst_sym.pst_value.pst_s_var.pst_atname);
				    /* init attribute name */

	/* initialize an operator node for the join */
	handle->qnodes.eqnode.pst_left = &handle->qnodes.leftvar; 
				    /* initialize the temp
                                    ** query tree nodes so that a join
				    ** is specified
				    */
	handle->qnodes.eqnode.pst_right = &handle->qnodes.rightvar;
	handle->qnodes.eqnode.pst_sym.pst_type = PST_BOP;
	handle->qnodes.eqnode.pst_sym.pst_len = sizeof(PST_OP_NODE);
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_opno =
	    subquery->ops_global->ops_cb->ops_server->opg_eq;
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_opinst =
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_oplcnvrtid = 
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = 
								ADI_NOFI;
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_distinct = 
								PST_DONTCARE;
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_opmeta = 
								PST_NOMETA;

	MEcopy((PTR)&cop->opo_outer->opo_maps->opo_eqcmap, sizeof(OPE_BMEQCLS),
	    (PTR)&handle->bmjoin);

	/* find the possible joining equivalence classes */
	BTand((i4)subquery->ops_eclass.ope_ev,
	    (char *)&cop->opo_inner->opo_maps->opo_eqcmap, (char *)&handle->bmjoin);
    }

    /* initialize PST_VAR node for the resdom list */
    handle->qnodes.resexpr.pst_sym.pst_type = PST_VAR;
    handle->qnodes.resexpr.pst_sym.pst_len = sizeof(PST_VAR_NODE);
    MEfill(
	sizeof(handle->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_atname), 
	' ', 
	(PTR)&handle->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_atname);
				/* init attribute name */

    /* initialize an operator node for the join */
    handle->qnodes.resnode.pst_right = &handle->qnodes.resexpr;
				/* initialize the temp
				** query tree nodes so that a join
				** is specified
				*/
    handle->qnodes.resnode.pst_sym.pst_type = PST_RESDOM;
    handle->qnodes.resnode.pst_sym.pst_len = sizeof(PST_RSDM_NODE);
    handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsflags = PST_RS_PRINT;
    handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsupdt = FALSE;
    handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsno = 0;
    handle->reqcls = OPE_NOEQCLS; /* traverse the equivalence classes
				** which are retrieved and required
				** by the parent of this CO node */

    handle->jeqcls = OPE_NOEQCLS;
    handle->bfno = OPB_NOBF;
    handle->opt_vno = -1;
    handle->jeqclsp = NULL;
}

/*{
** Name: opc_funcatt	- return text for function attribute
**
** Description:
**
** Inputs:
**	textcb                          state variable for tree to text 
**                                      conversion
**      varno                           range variable number of node
**      atno                            attribute number of node
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
**      1-apr-88 (seputis)
**          initial creation
**      023-mar-89 (robin)
**	    fixed incomplete opc_init parm list
**	12-mar-89 (robin)
**	    removed check of whether funcattr can
**	    be evaluated to separate routine.  Changed
**	    to return VOID.
[@history_line@]...
[@history_template@]...
*/
VOID
opc_funcatt(
	OPC_TTCB	    *textcbp,
	OPZ_IATTS	    attno,
	OPO_CO              *cop,
	ULM_SMARK	    *markp )
{
    OPS_SUBQUERY    *subquery;
    ULD_TSTRING	    *textp;
    OPZ_ATTS	    *attrp;
    OPZ_FATTS       *fattrp;	    /* ptr to func attribute being analyzed */
    QEQ_TXT_SEG	    *wheretxt;
	DB_STATUS	status;
	OPC_TTCB	temp_handle;
	DB_ERROR	error;
	ULM_RCB		ulmrcb;


    subquery = textcbp->subquery;
    textp = NULL;
    attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
    fattrp = subquery->ops_funcs.opz_fbase->opz_fatts[attrp->opz_func_att];
    
    /* assume the CO subtree has enough equivalence classes
    ** to evaluate this function attribute */

	wheretxt = NULL;
	opc_init(&temp_handle, subquery, cop, &ulmrcb, &wheretxt,
	    textcbp->qryhdr, textcbp->remdup);
				    /* initialize the temp
				    ** state variable so that the function
			            ** function attribute parse tree can be
			            ** processed out of line */
	status = opcu_tree_to_text((PTR) &temp_handle,
	    (i4)PSQ_RETRIEVE, /* set up as a retrieve */
	    NULL, /* no target relation name */
	    subquery->ops_global->ops_adfcb, 
	    textcbp->opt_ulmrcb, 
	    OPC_MAXTEXT/* max number of characters in one string */, 
	    TRUE /* use pretty printing where possible */, 
	    fattrp->opz_function->pst_right, /* ptr to fatt expression */ 
	    FALSE /* don't use routine to get the next conjunct to attach */,
	    FALSE /* no resdom list is needed */, 
	    &textp, &error,
	    subquery->ops_global->ops_adfcb->adf_qlang,
	    (bool) FALSE  /* not qualification only */,
	    (bool) FALSE  /* not finish */,
	    (bool) TRUE); /* not a WHERE clause */

	if (DB_FAILURE_MACRO(status))
	    opx_verror(E_DB_ERROR, E_OP068A_BAD_TREE, error.err_code);

	if (temp_handle.seglist != NULL)
	{
	    opcu_joinseg( &textcbp->seglist, temp_handle.seglist );
	}

    return;
}

bool
opc_fchk(
OPC_TTCB	    *textcbp,
OPZ_IATTS	    attno,
OPO_CO              *cop )
{
    OPS_SUBQUERY    *subquery;
    OPZ_ATTS	    *attrp;
    OPZ_FATTS       *fattrp;	    /* ptr to func attribute being analyzed */
    OPE_BMEQCLS	    *func_eqcmap;   /* ptr to equivalence classes needed to
				    ** evaluate the function attribute */
    OPE_BMEQCLS     temp_eqcmap;    /* temporary bitmap used for evaluating
                                    ** whether function attribute can be
			            ** evaluated in this subtree */

    subquery = textcbp->subquery;
    attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
    fattrp = subquery->ops_funcs.opz_fbase->opz_fatts[attrp->opz_func_att];
    func_eqcmap = &fattrp->opz_eqcm;

    return (opc_mapfunc(subquery, cop, attrp->opz_equcls,
	func_eqcmap, &temp_eqcmap));

}

/*{
** Name: opc_makename	- Create an 'ownername.tablename'.
**
** Description:
**	Create an 'ownername.tablename' string.
**	The string will be surrounded by single quotes only if
**	necessary, i.e. when the ownername begins with a special
**	character.
**
** Inputs:
**      ownername                       table owner
**      tablename                       table name
**      bufptr                          buffer to build string in
**	length				length of final string
**
** Outputs:
**      bufptr				Contains the name string
**	length				Contains length of name string
**
**	Returns:
**	    VOID
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None
**
** History:
**      04-jan-89   (robin)
**	    Created.
**      06-mar-89   (robin)
**	    Allow NULL ownername parm
**	26-jul-90   (seputis)
**	    add parameters to call uls_ownname to fix DB2 gateway bug
**	01-mar-93   (barbara)
**	    Delimit owner and table names if LDB's OPEN/SQL_LEVEL >= 605.
**	    Allow caller to request generation of ownername only: NULL tabname
**	    means just generate ownername.
[@history_template@]...
*/
VOID
opc_makename(
	DD_OWN_NAME	*ownername,
	DD_TAB_NAME	*tabname,
	char		*bufptr,
	i2		*length,
	DD_CAPS		*cap_ptr,
	i4		qmode,
	OPS_STATE	*global )
{
    i4                  tabnamesize;
    i4		    	ownnamesize;
    u_i4		unorm_len = (DB_OWN_MAXNAME + DB_TAB_MAXNAME) + 2;
    bool		quote_it;
    char		*quote_char;
    DB_STATUS	    	status;
    DB_ERROR            error;
		
    *bufptr = '\0';
    *length = (i2) 0;

    if (tabname == NULL )
	return;

    *length = 0;
    if (ownername != NULL)
    {

	if (cap_ptr->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
	{
	    ownnamesize = opt_noblanks( (i4)DB_OWN_MAXNAME, (char *)ownername );
	    /* Unnormalize and delimit ownername */
	    status = cui_idunorm( (u_char *)ownername, (u_i4 *)&ownnamesize,
			(u_char *)bufptr, &unorm_len, CUI_ID_DLM, &error);
	    if (DB_FAILURE_MACRO(status))
            		    opx_verror(E_DB_ERROR, E_OP08A4_DELIMIT_FAIL,
							error.err_code);
	    bufptr += unorm_len;
	    if (tabname != NULL)
	    {
		*bufptr++ = '.';
		*length = unorm_len +1;
	    }
	}
	else if (   (ownername != NULL )
		 && uls_ownname( cap_ptr, qmode, 
				global->ops_adfcb->adf_qlang, &quote_char )
		)
	{
	    ownnamesize = opt_noblanks( (i4)DB_OWN_MAXNAME, (char *)ownername );

	    if (quote_char != NULL)
	    {    
		*bufptr++ = *quote_char;
		*length +=1;
	    }
	    MEcopy( (PTR)ownername, ownnamesize, bufptr );
	    bufptr += ownnamesize;
	    *length += ownnamesize;
	    if (quote_char != NULL)
	    {
		*bufptr++ = *quote_char;
		*length +=1;
	    }
	    if (tabname != NULL)
	    {
		*bufptr++ = '.';
		*length += 1;
	    }
	}
    }
    if (tabname != NULL)
    {
	tabnamesize = opt_noblanks( (i4)DB_TAB_MAXNAME, (char *)tabname);

	if (cap_ptr->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
	{
	    /* Unnormalize and delimit tablename */
	    unorm_len = (DB_OWN_MAXNAME + DB_TAB_MAXNAME) + 2;
	    status = cui_idunorm( (u_char *)tabname, (u_i4*)&tabnamesize,
			(u_char *)bufptr, &unorm_len, CUI_ID_DLM, &error);
	    if (DB_FAILURE_MACRO(status)) opx_verror(E_DB_ERROR, 
				E_OP08A4_DELIMIT_FAIL, error.err_code);
	    bufptr += unorm_len;
	    *length += unorm_len;
	}
	else
	{
	    MEcopy( (PTR)tabname, tabnamesize, bufptr );
	    bufptr += tabnamesize;
	    *length += tabnamesize;;
	}
    }
    *bufptr = '\0';
}

/*{
** Name: opc_treetext	- 
**
** Description:
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
**      4-apr-88 (seputis)
**          initial creation
**      20-sep-88 (robin)
**	    added qualonly and qtxtptr paramters for support
**	    of distributed query text generation.
**	16-jun-85 (robin)
**	    Initialize nulljoin flat in the jandle around opc_etoa
**	    calls.  Partial fix for bug 6555.
**	03-aug-89 (robin)
**	    Fixed uld_text reference in opc_makename call to
**	    correct compile error on Unix.
**      3-jun-91 (seputis)
**          fix for b37348
**      8-sep-92 (seputis)
**          fix name conflict
**	30-mar-93 (barbara)
**	    Set capabilities pointer (cap_ptr) before calling opc_makename.
**	13-jan-93 (puru)
**	    Unknown behaviour will result if we pass negative values
**	    less than -1 to BTnext() in opc_tblnam(). Therefore initialize 
**	    opt_vno to -1 here itself.
**	20-may-94 (davebf)
**	    Screen out SEjoin from normal join predicate translation
**	24-may-1996 (angusm)
**		Alter logic for 59112 fix: not all boolfacts in array apply
**		to join col, so bools can get lost (depending on order of
**		conditions in original query) (bug 75990)
**	16-Apr-2007 (kschendel) b122118
**	    Remove obsolete expr-LIKE flag check, flag was never set.
**	22-mar-10 (smeke01) b123457
**	    opu_qtprint signature has changed as a result of trace point op214.
[@history_template@]...
*/
VOID
opc_treetext(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	bool		    qualonly,
	bool		    remdup,
	QEQ_TXT_SEG	    **wheretxt,
	QEQ_TXT_SEG	    **qrytxt,
	OPCQHD		    *qthdr )
{
    OPC_TTCB	    handle;
    OPS_STATE	    *global;
    ULD_TSTRING	    *tstring;
    ULM_RCB	    ulmrcb;
    OPB_BMBF	    *bmbfp;
    OPE_IEQCLS	    maxeqcls;
    DB_STATUS	    status;
    DB_ERROR	    error;
    ULD_TSTRING	    *nameptr;
    PST_QNODE	    *qual;	/* first qualification to be printed */
    bool	    finish;
    i4		    mode;

    global = subquery->ops_global;
    if (global->ops_cb->ops_check &&
	    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
    { 
	/* Print the parser query tree */
	opu_qtprint( global, subquery->ops_root, (char *)NULL );
    }

    if (cop != NULL)
    {
	if (!cop->opo_variant.opo_local)
	    return;
	bmbfp = cop->opo_variant.opo_local->opo_bmbf;
    }
    else
    {
	bmbfp = NULL;
    }

    nameptr = (ULD_TSTRING *) NULL;
    opc_init(&handle, subquery, cop, &ulmrcb, 
	wheretxt, qthdr, remdup );  /* initialize the state variable used to 
				    ** control the tree to text conversion
				    */

    tstring = NULL;
    finish = FALSE;
    maxeqcls = subquery->ops_eclass.ope_ev;

    if (global->ops_cb->ops_check &&
	    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
    {
    	TRformat( opt_scc, (i4 *) global, (char *) 
    	    &global->ops_trace.opt_trformat[0], (i4) 
    	    sizeof( global->ops_trace.opt_trformat ),
    	    "%s","\n OPC_TREETEXT: ENTRY\n");

    	TRformat( opt_scc, (i4 *) global, (char *) 
    	    &global->ops_trace.opt_trformat[0], (i4) 
    	    sizeof( global->ops_trace.opt_trformat ),
    	    "%s","\nWHERE_CL segments on entry are:");
	opt_seg_dump( global, handle.where_cl);
    }

    if ( !qualonly )
    {   /* print the query without the qualification and print the
	** resdom list given the attributes which are required by
	** the parent */
	PST_QNODE	    *saveright;

	saveright = subquery->ops_root->pst_right;
	subquery->ops_root->pst_right = NULL; 
	handle.opt_vno = -1;

	if (subquery->ops_bestco == cop && subquery->ops_sqtype == OPS_MAIN)
	    mode = subquery->ops_mode;
	else
	    mode = PSQ_RETRIEVE;

	if (mode != PSQ_RETRIEVE)
	{
	    /*  
	    **  This is an update, delete or insert.
	    **  Generate a string that holds the
	    **  target tablename (i.e. the one being
	    **  modified).
	    */
		OPV_GRV	    *grv_ptr;
		OPV_IGVARS  gno;
		DD_TAB_NAME *tabname;
		DD_OWN_NAME *ownername;
		DD_CAPS	    *cap_ptr;

		tabname = NULL;
		ownername = NULL;
		if ( mode == PSQ_RETINTO )
		{
		    tabname = (DD_TAB_NAME *) &global->ops_qheader->pst_distr->
			pst_ddl_info->qed_d6_tab_info_p->dd_t1_tab_name[0];
		}
		else
		{
		    gno = global->ops_qheader->pst_restab.pst_resvno;

		    if (gno >= 0)
		    {
			grv_ptr = global->ops_rangetab.opv_base->opv_grv[gno];
			tabname = (DD_TAB_NAME *) &grv_ptr->opv_relation->
			    rdr_obj_desc->dd_o9_tab_info.dd_t1_tab_name[0];
			ownername = (DD_OWN_NAME *) &grv_ptr->opv_relation->
			    rdr_obj_desc->dd_o9_tab_info.dd_t2_tab_owner[0];
		    }
		}
		cap_ptr = &global->ops_gdist.opd_base->
			    opd_dtable[OPD_TSITE]->opd_dbcap->dd_p3_ldb_caps;
		nameptr = (ULD_TSTRING *) opu_qsfmem( global, 
		    sizeof( ULD_TSTRING));
		nameptr->uld_next = NULL;
		opc_makename( ownername, tabname, nameptr->uld_text, 
		    (i2 * ) &nameptr->uld_length, cap_ptr, mode, global );
	}	    

	status = opcu_tree_to_text((PTR) &handle, 
	    mode, /* query mode */
	    nameptr, /* target relation name */
	    global->ops_adfcb, &ulmrcb, 
	    OPC_MAXTEXT /* max number of characters in one string */, 
	    TRUE /* use pretty printing where possible */, 
	    subquery->ops_root /* query tree root */, 
	    OPCNOCONJ /* conjuncts are not wanted */, 
	    TRUE /* use resdom list  */, 
	    &tstring, &error,
	    global->ops_adfcb->adf_qlang,
	    qualonly,
	    finish,
	    (bool) FALSE);

	subquery->ops_root->pst_right = saveright ; /* restore qualification */
/*
**	subquery->ops_root->pst_left = saveleft ;
*/

	if (DB_FAILURE_MACRO(status))
	    opx_verror(E_DB_ERROR, E_OP068A_BAD_TREE, error.err_code);

        if (global->ops_cb->ops_check &&
	    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
	{
	    TRformat( opt_scc, (i4 *) global, (char *) 
		&global->ops_trace.opt_trformat[0], (i4) 
		sizeof( global->ops_trace.opt_trformat ),
		"%s",
		"\n OPC_TREETEXT: FIRST uld_tree_to_text call completed.\n");

	    TRformat( opt_scc, (i4 *) global, (char *) 
		&global->ops_trace.opt_trformat[0], (i4) 
		sizeof( global->ops_trace.opt_trformat ),
		"%s","\nWHERE_CL segments are:");
	    opt_seg_dump( global, handle.where_cl);

	    TRformat( opt_scc, (i4 *) global, (char *) 
		&global->ops_trace.opt_trformat[0], (i4) 
		sizeof( global->ops_trace.opt_trformat ),
		"%s","\nSEGLIST segments are:");
	    opt_seg_dump( global, handle.seglist );
        }
    }	    
    if (tstring)
    {
	opc_dstring(subquery, "\n    ", tstring, OPC_MAXTEXT);
    }

    if ( handle.qryhdr->opq_q11_duplicate == OPQ_2DUP )
    {
	    /*
	    ** The caller specified that NO where clause should be
	    ** generated.  This is typically the case when a
	    ** 'create as select' temp table was generated 
	    ** previously for the current CO node to solve
	    ** a duplicate handling problem.  A where clause
	    ** is not needed because it was already completely
	    ** created for the 'create as select'temp.
	    */
    }
    else
    {
	    /*
	    ** Build the where clause
	    */


	tstring = NULL;
	if ( cop != NULL && cop->opo_sjpr == DB_SJ
	     && cop->opo_variant.opo_local->opo_jtype != OPO_SEJOIN
	     && handle.qryhdr->opq_q16_expr_like != OPQ_2ELIKE )
	{
	    /*
	    ** A join has equivalence classes from the left and right sides
	    ** but the boolean factor for the join has been removed so it 
	    ** needs to be created; use temporary space in the handle state
	    ** control block to create each join, one conjunct at a time
	    ** to be placed into the qualification
	    */

	    if (cop->opo_sjeqc >= 0)
	    {
		bool	    joinfound;
		/*
		** a joining eqc exists, print out the part of the
		** qualification which deals with a join.
		*/
		if (cop->opo_sjeqc < maxeqcls)
		{	/* a single equivalence class is found */
		    joinfound = opc_etoa(&handle, cop->opo_sjeqc, 
			&handle.nulljoin);
		    handle.jeqclsp = NULL;
		}
		else
		{	/* a multi-attribute join is specified */
		    handle.jeqclsp = &subquery->ops_msort.opo_base->
			opo_stable[cop->opo_sjeqc - maxeqcls]
			->opo_eqlist->opo_eqorder[0];
		    do
		    {
			joinfound = opc_etoa(&handle, *handle.jeqclsp,
				&handle.nulljoin);
				/* at this point only exact eqcls
				** list of <= 2 elements is expected 
				*/
			++handle.jeqclsp;	/* update to point to next eqcls to be
					    ** printed 
					    */
		    } while (!joinfound && (*handle.jeqclsp != OPE_NOEQCLS));
		} 


		if (joinfound)
		{
		    tstring = NULL;
		    status = opcu_tree_to_text((PTR) &handle,
			(i4)PSQ_RETRIEVE, /* set up as a retrieve */
			NULL, /* no target relation name */
			global->ops_adfcb, &ulmrcb, 
			OPC_MAXTEXT /* max number of characters in one string */, 
			TRUE /* use pretty printing where possible */, 
			&handle.qnodes.eqnode, /* ptr to join element */
			OPCJOINCONJ /* get the next join conjunct to attach */, 
			FALSE /* no resdom list is needed */, 
			&tstring, &error,
			global->ops_adfcb->adf_qlang,
			qualonly,
			finish,
			(bool) FALSE);
		    handle.joinstate = OPT_JNOTUSED;	/* part of fix for b41505 */
		    if (DB_FAILURE_MACRO(status))
			opx_verror(E_DB_ERROR, E_OP068A_BAD_TREE, error.err_code);

		    if (global->ops_cb->ops_check &&
			    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
		    {
			TRformat( opt_scc, (i4 *) global, (char *) 
			    &global->ops_trace.opt_trformat[0], (i4) 
			    sizeof( global->ops_trace.opt_trformat ),
			    "%s",
		      "\n OPC_TREETEXT: SECOND uld_tree_to_text call completed.\n");

			TRformat( opt_scc, (i4 *) global, (char *) 
			    &global->ops_trace.opt_trformat[0], (i4) 
			    sizeof( global->ops_trace.opt_trformat ),
			    "%s","\nWHERE_CL segments are:");
			opt_seg_dump( global, handle.where_cl);

			TRformat( opt_scc, (i4 *) global, (char *) 
			    &global->ops_trace.opt_trformat[0], (i4) 
			    sizeof( global->ops_trace.opt_trformat ),
			    "%s","\nSEGLIST segments are:");
			opt_seg_dump( global, handle.seglist );
		    }

		    if (tstring)
		    {
			opc_dstring(subquery, "\n    join on ",
			    tstring, OPC_MAXTEXT);
		    }
		}
	    }
	    handle.nulljoin = FALSE;
	    for (handle.jeqcls = -1;
		(handle.jeqcls = BTnext((i4)handle.jeqcls, (char *)&handle.bmjoin,
		(i4)subquery->ops_eclass.ope_ev))>=0;)
	    {
		if (opc_etoa(&handle, handle.jeqcls, &handle.nulljoin))
		{
		    /* these are the joining clauses which were not used */
		    qual = &handle.qnodes.eqnode;
		    break;	    /* found valid joining eqcls */
		}
	    }
	    if (handle.jeqcls < 0)
	    {
		handle.jeqcls = OPE_NOEQCLS;
		handle.joinstate = OPT_JNOTUSED;    /* part of fix for b41505 */
		qual = NULL;
	    }
	}
	else
	{
	    handle.nulljoin = FALSE;
	    handle.jeqcls = OPE_NOEQCLS;
	    handle.joinstate = OPT_JNOTUSED;	/* part of fix for b41505 */
	    qual = NULL;
	}

	handle.bfno = OPB_NOBF;
	if (!qual && bmbfp)
	{
	/*
	** bug 75990 - not all bool factors are on join cols
	*/
	    for(; ((handle.bfno = BTnext( handle.bfno, (char *)bmbfp, 
		    (i4)OPB_MAXBF)) >= 0)
                &&
		(qual = subquery->ops_bfs.opb_base->opb_boolfact[handle.bfno]->opb_qnode)
		;)
		{
			if (
			   (subquery->ops_bfs.opb_base->opb_boolfact[handle.bfno]->opb_mask 
		     & OPB_TRANSLATED) )
	    	{
				qual = NULL;
				continue;
	    	}
	    	else
			{
			DD_CAPS *cap_ptr;  /* LDB capabilities */
			OPD_SITE *sitedesc;
			OPD_ISITE site;
				/*
				** If the LDB is Ingres, which automatically propogates 
				** predicates on join columns to all tables in the join,
				** turn on a flag indicating that this bool factor has 
				** been translated and added to the SQL query we are creating,
				** and does not need to be translated for the other tables
				** in the join.  This is to prevent increasing the number 
				** of predicates in the output SQL query, which can cause
				** a boolean factor overflow (bug 59112).
				** If the LDB is not Ingres, go ahead and generate the extra
				** predicates, which will help performance if the LDB does
				** not do transitive closure optimization.
				*/
				site = cop->opo_variant.opo_local->opo_operation;
				sitedesc = global->ops_gdist.opd_base->opd_dtable[site];
				cap_ptr =  &sitedesc->opd_dbcap->dd_p3_ldb_caps;

				if(cap_ptr->dd_c1_ldb_caps & DD_2CAP_INGRES)
					subquery->ops_bfs.opb_base->opb_boolfact[handle.bfno]->opb_mask
						|= OPB_TRANSLATED;
					break;
       	    	}  /* end if, */
			}/* end for */
	}

	if (qual)
	{
	    tstring = NULL;
	    status = opcu_tree_to_text((PTR) &handle, 
		(i4)PSQ_RETRIEVE, /* set up as a retrieve */
		NULL, /* no target relation name */
		global->ops_adfcb, &ulmrcb, 
		OPC_MAXTEXT /* max number of characters in one string */, 
		TRUE /* use pretty printing where possible */, 
		qual, /* ptr to boolean factor element or join element */ 
		OPCCONJ /* get the next conjunct to attach */, 
		FALSE /* no resdom list is needed */, 
		&tstring, &error,
		global->ops_adfcb->adf_qlang,
		qualonly,
		finish,
		(bool) FALSE);

	    if (DB_FAILURE_MACRO(status))
		opx_verror(E_DB_ERROR, E_OP068A_BAD_TREE, error.err_code);


	    if (global->ops_cb->ops_check &&
		    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
	    {
		    TRformat( opt_scc, (i4 *) global, (char *) 
			&global->ops_trace.opt_trformat[0], (i4) 
			sizeof( global->ops_trace.opt_trformat ),
			"%s",
		   "\n OPC_TREETEXT: THIRD uld_tree_to_text call completed.\n");

		    TRformat( opt_scc, (i4 *) global, (char *) 
			&global->ops_trace.opt_trformat[0], (i4) 
			sizeof( global->ops_trace.opt_trformat ),
			"%s","\nWHERE_CL segments are:");
		    opt_seg_dump( global, handle.where_cl);

		    TRformat( opt_scc, (i4 *) global, (char *) 
			&global->ops_trace.opt_trformat[0], (i4) 
			sizeof( global->ops_trace.opt_trformat ),
			"%s","\nSEGLIST segments are:");
		    opt_seg_dump( global, handle.seglist );
	    }

	    if (tstring)
	    {
		opc_dstring( subquery, "\n    where", tstring, OPC_MAXTEXT );
	    }
	}

	handle.nulljoin = FALSE;
	handle.bfno = OPB_NOBF;
	if (subquery->ops_bfs.opb_bfconstants != NULL)
	{
	    qual = subquery->ops_bfs.opb_bfconstants;
	    tstring = NULL;
	    status = opcu_tree_to_text((PTR) &handle, 
		(i4)PSQ_RETRIEVE, /* set up as a retrieve */
		NULL, /* no target relation name */
		global->ops_adfcb, &ulmrcb, 
		OPC_MAXTEXT /* max number of characters in one string */, 
		TRUE /* use pretty printing where possible */, 
		qual, /* ptr to boolean factor element or join element */ 
		OPCCONJ /* get the next conjunct to attach */, 
		FALSE /* no resdom list is needed */, 
		&tstring, &error,
		global->ops_adfcb->adf_qlang,
		qualonly,
		finish,
		(bool) FALSE);

	    if (DB_FAILURE_MACRO(status))
		opx_verror(E_DB_ERROR, E_OP068A_BAD_TREE, error.err_code);


	    if (global->ops_cb->ops_check &&
		    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
	    {
		    TRformat( opt_scc, (i4 *) global, (char *) 
			&global->ops_trace.opt_trformat[0], (i4) 
			sizeof( global->ops_trace.opt_trformat ),
			"%s",
		  "\n OPC_TREETEXT: FOURTH uld_tree_to_text call completed.\n");

		    TRformat( opt_scc, (i4 *) global, (char *) 
			&global->ops_trace.opt_trformat[0], (i4) 
			sizeof( global->ops_trace.opt_trformat ),
			"%s","\nWHERE_CL segments are:");
		    opt_seg_dump( global, handle.where_cl);

		    TRformat( opt_scc, (i4 *) global, (char *) 
			&global->ops_trace.opt_trformat[0], (i4) 
			sizeof( global->ops_trace.opt_trformat ),
			"%s","\nSEGLIST segments are:");
		    opt_seg_dump( global, handle.seglist );
	    }

	    if (tstring)
	    {
		opc_dstring( subquery, "\n    where", tstring, OPC_MAXTEXT );
	    }
        }

    }

    /*
    **  Make a final call to treeutil to finish the query if necessary
    */

    finish = TRUE;
    status = opcu_tree_to_text((PTR) &handle, 
	mode,/* use the real query mode */
	nameptr, /* target relation name needed for finish */
	global->ops_adfcb, &ulmrcb, 
	OPC_MAXTEXT /* max number of characters in one string */, 
	TRUE /* use pretty printing where possible */, 
	qual, /* ptr to boolean factor element or join element */ 
	OPCCONJ /* get the next conjunct to attach */, 
	FALSE /* no resdom list is needed */, 
	&tstring, &error,
	global->ops_adfcb->adf_qlang,
	qualonly,
	finish,
	(bool) FALSE);


    if ( qualonly )
    {
	if (handle.seglist != NULL)
	{
	    *wheretxt = handle.seglist;
	}
	if (global->ops_cb->ops_check &&
	    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
	{
	    TRformat( opt_scc, (i4 *) global, (char *) 
	    	    &global->ops_trace.opt_trformat[0], (i4) 
	    	    sizeof( global->ops_trace.opt_trformat ),
	    	    "%s","\n OPC_TREETEXT: returning seglist in wheretxt.\n");

	    TRformat( opt_scc, (i4 *) global, (char *) 
	    	    &global->ops_trace.opt_trformat[0], (i4) 
	    	    sizeof( global->ops_trace.opt_trformat ),
	    	    "%s","\nWHERETXT segments are:");
	    opt_seg_dump( global, *wheretxt);
	}
    }
    else
    {
	if (handle.seglist != NULL )
	{
	    *qrytxt = handle.seglist;
	}
	if (global->ops_cb->ops_check &&
	    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
	{
	    TRformat( opt_scc, (i4 *) global, (char *) 
	    	    &global->ops_trace.opt_trformat[0], (i4) 
	    	    sizeof( global->ops_trace.opt_trformat ),
	    	    "%s","\n OPC_TREETEXT: returning seglist in qrytxt.\n");

	    TRformat( opt_scc, (i4 *) global, (char *) 
	    	    &global->ops_trace.opt_trformat[0], (i4) 
	    	    sizeof( global->ops_trace.opt_trformat ),
	    	    "%s","\nWHERETXT segments are:");
	    opt_seg_dump( global, *qrytxt);
	}
    }
    return;
}
