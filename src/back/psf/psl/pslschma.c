/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <ci.h>
#include    <qu.h>
#include    <st.h>
#include    <cm.h>
#include    <er.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <psftrmwh.h>
#include    <sxf.h>
#include    <psyaudit.h>
#include    <cui.h>

/*
** Name: PSLSCHMA.C:	this file contains functions used in semantic actions
**			for CREATE SCHEMA in SQL
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_CS<number>_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**
**      if the function will be called only by the SQL grammar, <number> ust be
**      followed by 'S';
**
**      if the function will be called only by the QUEL grammar, <number>must
**      be followed by 'Q';
**
**	(all routines in this file are called by the SQL grammar only....)
**	
** Description:
**
**	This file contains functions used by the SQL grammar in
**	parsing of the CREATE SCHEMA statement
**
**	Routines:
**
**	    psl_cs01s_create_schema	- actions for create_schema:
**
**	    psl_cs02s_create_schema_key	- actions for create_schema_key:
**
**	    psl_cs03s_create_table - 	Semantic actions for create_table:
**					when issued within CREATE SCHEMA wrapper
**
**	    psl_cs04s_create_view - 	Semantic actions for create_view:
**					when issued within CREATE SCHEMA wrapper
**
**	    psl_cs05s_grant - 		Semantic actions for grant:
**					when issued within CREATE SCHEMA wrapper
**
**	    psl_cs06s_obj_spec - 	Semantic actions for obj_spec:
**					when issued within CREATE SCHEMA wrapper
**				  	on behalf of CREATE VIEW substatement
**
** History:
**	25-nov-1992 (ralph)
**	    Written for CREATE SCHEMA
**	23-dec-1992 (ralph)
**	   Reorder PST_STATEMENT nodes according to dependencies
**	   pss_object now points to PST_PROCEDURE node
**	20-jan-1993 (ralph)
**	    Changed call interface to psq_tmulti_out() to conform to new version
**	19-feb-1993 (walt)
**		Added (u_char *) casts to the variable 'stmtstart' in 
**		psl_cs03s_create_table, psl_cs04s_create_view, psl_cs05_grant
**		where the Alpha compiler was complaining about subtracting it from
**		sess_cb->pss_prvtok which has that type.
**	21-jan-1993 (ralph)
**	    Call psl_cons_text() in psl_cs03s_create_table()
**	    to generate CONSTRAINT text
**	22-jan-1993 (ralph)
**	    Add internal error codes
**	    Prevent creation of foreign schemae
**	    Convert calls from psl_idunorm to cui_idunorm
**	16-mar-1993 (rblumer)
**	    Change 4Ax error numbers to 4Cx.
**	17-mar-1993 (rblumer)
**	    Move code for generating ALTER TABLE text into psl_gen_alter_text
**	    in pslcons.c.
**	09-apr-1993 (ralph)
**	   remove bypass for prevention of creation of foreign schemae
**	24-may-1993 (ralph)
**	   fix bug with GRANT WGO and CREATE VIEW WCO:
**	   change semantic interface to psl_alloc_exec_imm(),
**	   fill in PST_INFO.pst_srctext.
**	   Create query text buffers for all user specified statements
**	   after the entire CREATE SCHEMA statement is parsed.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	16-jul-1993 (rblumer)
**	    in psl_reorder_schema,
**	    added code to set EMPTY_TABLE bit for ALTER TABLE statements
**	    (since we know tables will always be empty during CREATE SCHEMA);
**	    then the ALTER TABLE code will skip the data validation check.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	4-oct-93 (stephenb)
**	    Added call to psy_secaudit() when failing to create a schema.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	21-Oct-2010 (kiria01) b124629
**	    Use the macro symbol with ult_check_macro instead of literal.
*/

/* external declarations */


/* static functions and constants declaration */


/*
** Name: psl_cs01s_create_schema	- Semantic actions for create_schema:
**
** Description:
**	This routine performs the semantic action for create_schema:
**
**	create_schema:  create_schema_key create_schema_auth create_schema_list
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	psq_cb		    PSF request CB
**
** Outputs:
**	psq_cb		    PSF request CB
**         psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	25-nov-1992 (ralph)
**	    Written for CREATE SCHEMA
**	23-dec-1992 (ralph)
**	   pss_object now points to PST_PROCEDURE node
**	22-jan-1993 (ralph)
**	   prevent creation of foreign schemae
**	27-mar-93 (rickh)
**	   Declared err_code to shut up the compiler when compiling
**	   without xDEBUG.
**	09-apr-1993 (ralph)
**	   remove bypass for prevention of creation of foreign schemae
**	24-may-1993 (ralph)
**	   fix bug with GRANT WGO and CREATE VIEW WCO:
**	   Create query text buffers for all user specified statements
**	   after the entire CREATE SCHEMA statement is parsed.
**	4-oct-93 (stephenb)
**	    Added call to psy_secaudit() when failing to create a schema
**	    because the username is not the same as the schema name.
**	22-feb-94 (robf)
**          Pass all params to psy_secaudit. Note we don't have a security
**	    label for a schema (yet), but may do one day.
*/
DB_STATUS
psl_cs01s_create_schema(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*authid)
{
    PST_PROCEDURE	*prnode;
    PST_STATEMENT	*stmt_list;
    PST_STATEMENT	*prev_node;
    PST_STATEMENT	*current_node;
    PST_EXEC_IMM	*curr_exec;
    PST_EXEC_IMM	*prev_exec;
    PST_CREATE_SCHEMA	*crt_node;
    i4			stmtlen;
    DB_STATUS		status;
    DB_ERROR		*err_blk= &psq_cb->psq_error;
    i4		err_code;

    prnode = (PST_PROCEDURE *) sess_cb->pss_object;
    stmt_list = (PST_STATEMENT *) prnode->pst_stmts;
    crt_node = &(stmt_list->pst_specific.pst_createSchema);

    /* Copy schema name and owner to the PST_CREATE_SCHEMA node */

    STmove(authid, ' ',
	sizeof(DB_SCHEMA_NAME), crt_node->pst_schname.db_schema_name);
    STmove(authid, ' ',
	sizeof(DB_OWN_NAME), crt_node->pst_owner.db_own_name);

    /* Verify that the schema name is the same as the user name */

    if (MEcmp((PTR)crt_node->pst_owner.db_own_name,
	      (PTR)sess_cb->pss_user.db_own_name,
	      sizeof(sess_cb->pss_user.db_own_name))
	    != 0)
    {
	/*
	** Currently we must audit this because it can be considered an
	** attempted security violation that a user is trying to create
	** a schema for someone else, if in the future it becomes possible
	** to create a schema with a name other that your username then this
	** may be removed.
	*/
	if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	{
	    DB_ERROR	e_error;

	    status = psy_secaudit(FALSE, sess_cb, 
		    crt_node->pst_schname.db_schema_name,
		    &sess_cb->pss_user,
		    sizeof(crt_node->pst_schname.db_schema_name),
		    SXF_E_SECURITY, I_SX203E_SCHEMA_CREATE,
		    SXF_A_FAIL | SXF_A_CREATE, 
		    &e_error);
	}

	/* Error - Schema name/owner must be effective userid */
	(VOID) psf_error(6661L, 0L, PSF_USERERR, &err_code,
			 err_blk, 2,
			 sizeof("CREATE SCHEMA")-1, "CREATE SCHEMA",
			 STlength(authid), authid);
	return(E_DB_ERROR);
    }

    /*
    ** Allocate query text space for user-specified statements.
    */
    prev_node = (PST_STATEMENT *) NULL;
    for (current_node = stmt_list;
         current_node != (PST_STATEMENT *) NULL;
         current_node = current_node->pst_next)
    {
	/* Ensure this is an PST_EXEC_IMM node */

	if (current_node->pst_type != PST_EXEC_IMM_TYPE)
	    continue;

	curr_exec = &(current_node->pst_specific.pst_execImm);

	/* If query text is present, skip this one */

	if (curr_exec->pst_qrytext != (DB_TEXT_STRING *) NULL)
	    continue;

	/* If this is the first node, skip it */

	if (prev_node == (PST_STATEMENT *) NULL)
	{
	    prev_node = current_node;
	    continue;
	}

	prev_exec = &(prev_node->pst_specific.pst_execImm);

	/* Allocate space for the previous node's query text */

	stmtlen = (i4)((u_char *)curr_exec->pst_info.pst_srctext -
			(u_char *)prev_exec->pst_info.pst_srctext);
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, stmtlen+3,
			    (PTR *) &prev_exec->pst_qrytext, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* Copy in the previous node's query text */

	prev_exec->pst_qrytext->db_t_count = stmtlen;
	MEmove (stmtlen, (PTR)prev_exec->pst_info.pst_srctext, EOS,
		stmtlen+1, (PTR)prev_exec->pst_qrytext->db_t_text);

	/* Process the next node (if any) */

	prev_node = current_node;
    }

    /*
    ** Process last node (if any).
    */
    if (prev_node != (PST_STATEMENT *) NULL)
    {
	prev_exec = &(prev_node->pst_specific.pst_execImm);

	/* Allocate space for the last node's query text */

	stmtlen = (i4)((u_char *)sess_cb->pss_endbuf -
			(u_char *)prev_exec->pst_info.pst_srctext)
		       + 1;
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, stmtlen+3,
			    (PTR *) &prev_exec->pst_qrytext, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* Copy in the last node's query text */

	prev_exec->pst_qrytext->db_t_count = stmtlen;
	MEmove (stmtlen, (PTR)prev_exec->pst_info.pst_srctext, EOS,
		stmtlen+1, (PTR)prev_exec->pst_qrytext->db_t_text);
    }

    /*
    ** Reorder statement nodes
    */
    status = psl_reorder_schema(sess_cb, psq_cb, prnode);
    if (DB_FAILURE_MACRO(status))
        return (status);

    return(E_DB_OK);
}

/*
** Name: psl_reorder_schema - Reorder PST_EXEC_IMM nodes in CREATE SCHEMA
**
** Description:
**	This routine reorders PST_STATEMENT nodes containing PST_EXEC_IMM
**	statements that are part of the CREATE SCHEMA query tree.
**	The reordering is done to eliminate forward references.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	psq_cb		    PSF request CB
**	prnode		    & of PST_PROCEDURE node for CREATE SCHEMA
**
** Outputs:
**	psq_cb
**	    psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	12-dec-1992 (ralph)
**	    Written for CREATE SCHEMA
**	17-dec-1992 (ralph)
**	    Rewritten to reorder view nodes.
**	    OPC expects pst_link to point to next node; redo.
**	22-jan-1993 (ralph)
**	    Add internal error codes
**	16-jul-1993 (rblumer)
**	    Added code to set EMPTY_TABLE bit for ALTER TABLE statements
**	    (since we know tables will always be empty during CREATE SCHEMA);
**	    then the ALTER TABLE code will skip the data validation check.
*/
DB_STATUS
psl_reorder_schema(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PST_PROCEDURE	*prnode)
{
    PST_STATEMENT	*stmt_list;
    PST_STATEMENT	*current_node, *prev_node, *last_node, *temp_node;
    PST_STATEMENT	*reordered_nodes, *unordered_nodes, *alter_nodes;
    PST_OBJDEP		*dependency, *prev_dep;
    PST_EXEC_IMM	*exec_node, *exec_dep;
    PST_CREATE_SCHEMA	*schema_node;
    bool		progress_flag;
    bool		resolved_flag;
    bool		found_flag;
    DB_ERROR		*err_blk = &psq_cb->psq_error;
    i4             err_code;

    /*
    ** Setup phase
    */

    stmt_list = (PST_STATEMENT *) prnode->pst_stmts;

#ifdef	xDEBUG
    {
	i4		val1;
	i4		val2;

	if (ult_check_macro(&sess_cb->pss_trace,
				PSS_DBPROC_TREE_BY_NEXT_TRACE, &val1, &val2))
	{
	    (VOID) pst_display("\nCREATE SCHEMA TREE (before reordering)\n");
	    for (current_node = stmt_list;
	 	 current_node != (PST_STATEMENT *) NULL;
	 	 current_node = current_node->pst_next)
    	    {
		current_node->pst_link = current_node->pst_next;
    	    }
	    (VOID) pst_dbpdump(prnode, 0);
	}
    }
#endif

    /*
    ** First node should be a PST_STATEMENT with mode PSQ_CREATE_SCHEMA.
    ** Move it to the reordered_nodes list.
    */
    
    current_node = stmt_list;
    if (current_node->pst_mode != PSQ_CREATE_SCHEMA)
    {
	/* ERROR - internal - first node not CREATE SCHEMA */
	(VOID) psf_error(E_PS04C0_BAD_SCHEMA_TREE, 0L, PSF_INTERR,
			 &err_code, err_blk, 0);
	return(E_DB_ERROR);
    }
    unordered_nodes = current_node->pst_next;
    alter_nodes = (PST_STATEMENT *) NULL;
    current_node->pst_next = (PST_STATEMENT *) NULL;
    last_node = reordered_nodes = current_node;
    schema_node = &(current_node->pst_specific.pst_createSchema);

    /*
    ** Identify each PST_STATEMENT object
    */
    prev_node = (PST_STATEMENT *) NULL;
    for (current_node = unordered_nodes;
	 current_node != (PST_STATEMENT *) NULL;
	 current_node = current_node->pst_next)
    {
	/* Ensure this is an PST_EXEC_IMM node */

	if (current_node->pst_type != PST_EXEC_IMM_TYPE)
	{
	    /* ERROR - internal - non EXECUTE IMMEDIATE node */
	    (VOID) psf_error(E_PS04C1_BAD_SCHEMA_NODE, 0L, PSF_INTERR,
			     &err_code, err_blk, 0);
	    return(E_DB_ERROR);
	}
    	exec_node = &(current_node->pst_specific.pst_execImm);

	/* If a generated ALTER TABLE statement, take it off the chain */

	if (current_node->pst_mode == PSQ_ALTERTABLE)
	{
	    exec_node->pst_info.pst_objname = (DB_TAB_NAME *) NULL;
	    temp_node = current_node;
	    if (prev_node == (PST_STATEMENT *) NULL)
	    {
		unordered_nodes = temp_node->pst_next;
		current_node = unordered_nodes;
	    }
	    else
	    {
		prev_node->pst_next = temp_node->pst_next;
		current_node = prev_node;
	    }
	    temp_node->pst_next = alter_nodes;
	    alter_nodes = temp_node;
	    if (current_node == (PST_STATEMENT *) NULL)
		break;
	    continue;
	}

	/* If not a table or view, no base object asscoiated with this one */

	if ((current_node->pst_mode != PSQ_CREATE) &&
	    (current_node->pst_mode != PSQ_VIEW))
	{
	    /* Action doesn't represent a table or view */
	    exec_node->pst_info.pst_objname = (DB_TAB_NAME *) NULL;
	    prev_node = current_node;
	    continue;
	}

	/* Locate first dependency - should be current object */

	if ((dependency = exec_node->pst_info.pst_deplist)
		 == (PST_OBJDEP *) NULL)
	{
	    /* ERROR - internal - no object name when expected */
	    (VOID) psf_error(E_PS04C2_BAD_SCHEMA_OBJECT, 0L, PSF_INTERR,
			     &err_code, err_blk, 0);
	    return(E_DB_ERROR);
	}

	if (STskipblank(dependency->pst_depown.db_own_name,
			sizeof(dependency->pst_depown.db_own_name))
		!= (char *)NULL &&
	    MEcmp((PTR)dependency->pst_depown.db_own_name,
		  (PTR)schema_node->pst_schname.db_schema_name,
		  sizeof(dependency->pst_depown.db_own_name))
		!= 0)
	{
	    /* ERROR - invalid object - foreign schema */
	    (VOID) psf_error(6657L, 0L, PSF_USERERR, &err_code,
				err_blk, 2,
				sizeof("CREATE SCHEMA")-1, "CREATE SCHEMA",
				STlength((char *) exec_node->
					 	       pst_qrytext->db_t_text),
				exec_node->pst_qrytext->db_t_text);
	    (VOID) psf_error(6658L, 0L, PSF_USERERR, &err_code,
			     	err_blk, 3,
			     	sizeof("CREATE SCHEMA")-1, "CREATE SCHEMA",
			     	psf_trmwhite(sizeof(dependency->pst_depown),
					  (char *)&dependency->pst_depown),
			     	(PTR)&dependency->pst_depown,
			     	psf_trmwhite(sizeof(dependency->pst_depname),
					  (char *)&dependency->pst_depname),
			     	(PTR)&dependency->pst_depname);

	    return(E_DB_ERROR);
	}

	/* Put pointer to object name in pst_info */

	exec_node->pst_info.pst_objname = &dependency->pst_depname;

	/* Now remove the self-referential dependency entry */

	exec_node->pst_info.pst_deplist = dependency->pst_nextdep;

	prev_node = current_node;
    }

    /*
    ** Locate associated PST_STATEMENT for each dependency
    */

    for (current_node = unordered_nodes;
	 current_node != (PST_STATEMENT *) NULL;
	 current_node = current_node->pst_next)
    {
    	exec_node = &(current_node->pst_specific.pst_execImm);
	prev_dep = (PST_OBJDEP *) NULL;
	for (dependency = exec_node->pst_info.pst_deplist;
	     dependency != (PST_OBJDEP *) NULL;
	     dependency = dependency->pst_nextdep)
	{
	    /* Ensure the dependency is intraschema and non-self-referential */

	    if ((STskipblank(dependency->pst_depown.db_own_name,
			     sizeof(dependency->pst_depown.db_own_name)) &&
		 MEcmp((PTR)dependency->pst_depown.db_own_name,
		      (PTR)schema_node->pst_schname.db_schema_name,
		      sizeof(dependency->pst_depown.db_own_name)) != 0)
		||
		(exec_node->pst_info.pst_objname != (DB_TAB_NAME *) NULL &&
		 MEcmp((PTR)dependency->pst_depname.db_tab_name,
		      (PTR)exec_node->pst_info.pst_objname->db_tab_name,
		      sizeof(dependency->pst_depname.db_tab_name)) == 0))
	    {
		/* Dependency in foreign schema or self-referential; dis it */
		if (prev_dep == (PST_OBJDEP *) NULL)
		    exec_node->pst_info.pst_deplist = dependency->pst_nextdep;
		else
		    prev_dep->pst_nextdep = dependency->pst_nextdep;
		continue;
	    }

	    /* Locate PST_STATEMENT that corresponds to the dependency */
	    
	    for (temp_node = unordered_nodes;
		 temp_node != (PST_STATEMENT *) NULL;
		 temp_node = temp_node->pst_next)
	    {
		exec_dep = &(temp_node->pst_specific.pst_execImm);
		if (exec_dep->pst_info.pst_objname != (DB_TAB_NAME *) NULL &&
		    MEcmp((PTR)dependency->pst_depname.db_tab_name,
			  (PTR)exec_dep->pst_info.pst_objname->db_tab_name,
			  sizeof(dependency->pst_depname.db_tab_name))
			== 0)
		    break;
	    }

	    if (temp_node == (PST_STATEMENT *) NULL)
	    {
		/* ERROR - unresolved dependency */
	    	(VOID) psf_error(6657L, 0L, PSF_USERERR, &err_code,
				err_blk, 2,
				sizeof("CREATE SCHEMA")-1, "CREATE SCHEMA",
				STlength((char *) exec_node->
						       pst_qrytext->db_t_text),
				exec_node->pst_qrytext->db_t_text);
	    	(VOID) psf_error(6659L, 0L, PSF_USERERR,
			    	&err_code, err_blk, 2,
				sizeof("CREATE SCHEMA")-1, "CREATE SCHEMA",
			    	psf_trmwhite(sizeof(dependency->pst_depname),
					 (char *)&dependency->pst_depname),
			    	(PTR)&dependency->pst_depname);
	    	return(E_DB_ERROR);
	    }

	    /* If dependency not used in reordering, dis it */

	    if (dependency->pst_depuse & PST_DEP_NOORDER)
	    {
		if (prev_dep == (PST_OBJDEP *) NULL)
		    exec_node->pst_info.pst_deplist = dependency->pst_nextdep;
		else
		    prev_dep->pst_nextdep = dependency->pst_nextdep;
		continue;
	    }


	    /* Cross reference PST_STATEMENT to dependency */

	    dependency->pst_depstmt = temp_node;

	    prev_dep = dependency;
	}
    }

    /*
    ** Reorder phase
    */

    while (unordered_nodes != (PST_STATEMENT *) NULL)
    {
	progress_flag = FALSE;

	/* Migrate all objects whose dependencies have been resolved */

	prev_node = (PST_STATEMENT *) NULL;
	for (current_node = unordered_nodes;
	     current_node != (PST_STATEMENT *) NULL;
	     current_node = current_node->pst_next)
	{
	    /* Verify all dependencies for this object are resolved */

	    resolved_flag = TRUE;
	    exec_node = &(current_node->pst_specific.pst_execImm);
	    prev_dep = (PST_OBJDEP *) NULL;
	    for (dependency = exec_node->pst_info.pst_deplist;
	     	 dependency != (PST_OBJDEP *) NULL;
	     	 dependency = dependency->pst_nextdep)
	    {
		/* Locate dependency on resolved list */

		found_flag = FALSE;
	    	for (temp_node = reordered_nodes;
		     temp_node != (PST_STATEMENT *) NULL;
		     temp_node = temp_node->pst_next)
	    	{
		    if (dependency->pst_depstmt == temp_node)
		    {
			/* Dependency resolved; dis it */
			found_flag = TRUE;
			if (prev_dep == (PST_OBJDEP *) NULL)
		    	    exec_node->pst_info.pst_deplist =
				dependency->pst_nextdep;
			else
		    	    prev_dep->pst_nextdep =
				dependency->pst_nextdep;
			break;
		    }
		    if (temp_node == last_node)
			break;				/* End of list; exit */
	    	}

		resolved_flag &= found_flag;
		if (!found_flag)
		    prev_dep = dependency;
	    }

	    if (resolved_flag)
	    {
		/* All dependencies resolved; move PST_STATEMENT */

		last_node->pst_next = current_node;
		last_node = current_node;

		if (prev_node == (PST_STATEMENT *) NULL)
		    unordered_nodes = current_node->pst_next;
		else
		    prev_node->pst_next = current_node->pst_next;

		progress_flag = TRUE;
		continue;
	    }

	    prev_node = current_node;
	}

	if (!progress_flag)
	    break;				/* Circular dependency */
    }

    /*
    ** Place generated ALTER TABLE nodes at end of list
    */

    last_node->pst_next = alter_nodes;

#ifdef	xDEBUG
    {
	i4		val1;
	i4		val2;

	/*
	** Print the list of nodes that were reordered
	*/

	if (ult_check_macro(&sess_cb->pss_trace,
				PSS_DBPROC_TREE_BY_NEXT_TRACE, &val1, &val2))
	{
	    (VOID) pst_display("\nCREATE SCHEMA TREE (after reordering)\n");
	    for (current_node = reordered_nodes;
	 	 current_node != (PST_STATEMENT *) NULL;
	 	 current_node = current_node->pst_next)
    	    {
		current_node->pst_link = current_node->pst_next;
    	    }
	    (VOID) pst_dbpdump(prnode, 0);
	}
    }
#endif

    /*
    ** If not all PST_STATEMENT nodes were migrated to the reordered list,
    ** an intraschema circular dependency exists.
    */

    if (unordered_nodes != (PST_STATEMENT *) NULL)
    {
#ifdef	xDEBUG
    	{
	    i4		val1;
	    i4		val2;

	    /*
	    ** Print the list of nodes that failed reordering
	    */

	    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_DBPROC_TREE_BY_NEXT_TRACE, &val1, &val2))
	    {
	        (VOID) pst_display(
			"\nCREATE SCHEMA TREE REORDERING FAILED\n");
		prnode->pst_stmts = unordered_nodes;
	        (VOID) pst_display(
			"\nCREATE SCHEMA TREE (remaining statements)\n");
	    	for (current_node = unordered_nodes;
	 	     current_node != (PST_STATEMENT *) NULL;
	 	     current_node = current_node->pst_next)
    	    	{
		    current_node->pst_link = current_node->pst_next;
    	    	}
	        (VOID) pst_dbpdump(prnode, 0);
	    }
	}
#endif
	/*
	** Print all statement that are involved in the circular dependency,
	** and all unresolved dependencies for each of those statements.
	*/
	for (current_node = unordered_nodes;
	     current_node != (PST_STATEMENT *) NULL;
	     current_node = current_node->pst_next)
	{
	    exec_node = &(current_node->pst_specific.pst_execImm);
	    /* ERROR - Circular dependency detected */
	    (VOID) psf_error(6657L, 0L, PSF_USERERR, &err_code,
			     err_blk, 2,
			     sizeof("CREATE SCHEMA")-1, "CREATE SCHEMA",
			     STlength((char *) exec_node->
				      		    pst_qrytext->db_t_text),
			     exec_node->pst_qrytext->db_t_text);
	    for (dependency = exec_node->pst_info.pst_deplist;
	     	 dependency != (PST_OBJDEP *) NULL;
	     	 dependency = dependency->pst_nextdep)
	    {
		/* Print list of remaining dependencies */
		(VOID) psf_error(6660L, 0L, PSF_USERERR, &err_code,
				err_blk, 2,
				sizeof("CREATE SCHEMA")-1, "CREATE SCHEMA",
				psf_trmwhite(sizeof(dependency->pst_depname),
					  (char *)&dependency->pst_depname),
				(PTR)&dependency->pst_depname);
	    }
	}
	
	return(E_DB_ERROR);
    }

    /*
    ** Well, OPF/OPC expect pst_link to point to the next
    ** statement, not pst_next.  So, step thru and fix thing up....
    */

    for (current_node = reordered_nodes;
	 current_node != (PST_STATEMENT *) NULL;
	 current_node = current_node->pst_next)
    {
	current_node->pst_link = current_node->pst_next;
    }

    return(E_DB_OK);
}

/*
** Name: psl_cs02s_create_schema_key - Semantic actions for create_schema_key
**
** Description:
**	This routine handles the semantic actions for parsing the
**	create_schema_key: (SQL) production.
**
**	create_schema_key:      CRTSCHEMA
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	psq_cb		    PSF request CB
**
** Outputs:
**	psq_cb
**	    psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	25-nov-1992 (ralph)
**	    Written for CREATE SCHEMA
*/
DB_STATUS
psl_cs02s_create_schema_key(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    DB_STATUS		status;
    DB_ERROR		*err_blk = &psq_cb->psq_error;

    /*
    ** Call pst_crt_schema() to open memory stream, etc.
    */

    status = pst_crt_schema(sess_cb, psq_cb->psq_mode, err_blk);

    if (status != E_DB_OK)
	return (status);

    return(E_DB_OK);
}

/*
** Name: psl_alloc_exec_imm - Allocate PST_EXEC_IMM node
**
** Description:
**      This routine allocates, initializes, and links in a
**	PST_STATEMENT node of type PST_EXEC_IMM to represent
**	a substatement within the CREATE SCHEMA wrapper.
**
** Inputs:
**      sess_cb             ptr to a PSF session CB
**      psq_cb              PSF request CB
**	stmtstart	    pointer to start of query text for substatement;
**			    null pointer for no text.
**	stmtlen		    length of the substatement query text;
**			    zero for user-specified statement.
**      deplist             ptr to a list of dependency info blocks
**	qmode		    query mode of the substatement
**	basemode	    original query mode of the substatement
**
** Outputs:
**      psq_cb              PSF request CB
**         psq_error        will be filled in if an error occurs
**
** Returns:
**      E_DB_{OK, ERROR}
**
** History:
**      10-dec-1992 (ralph)
**          Written for CREATE SCHEMA
**	23-dec-1992 (ralph)
**	   pss_object now points to PST_PROCEDURE node
**	   PST_STATEMENT.pst_link for the CREATE SCHEMA node now points
**	   to the last DDL statement node allocated; ordering of
**	   statements is preserved during initial pass.
**	   Also, changed parameters to allow omission of text string.
**	24-may-1993 (ralph)
**	   fix bug with GRANT WGO and CREATE VIEW WCO:
**	   change semantic interface to psl_alloc_exec_imm(),
**	   fill in PST_INFO.pst_srctext.
*/
DB_STATUS
psl_alloc_exec_imm(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
	char		*stmtstart,
	i4		stmtlen,
	PST_OBJDEP	*deplist,
	i4		qmode,
	i4		basemode)
{
    PST_PROCEDURE       *prnode;
    PST_STATEMENT       *stmt;
    PST_STATEMENT       *stmt_list;
    PST_EXEC_IMM	*exec_node;
    DB_STATUS           status;
    DB_ERROR            *err_blk= &psq_cb->psq_error;

    prnode = (PST_PROCEDURE *) sess_cb->pss_object;
    stmt_list = (PST_STATEMENT *) prnode->pst_stmts;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
                        sizeof(PST_STATEMENT), (PTR *) &stmt, err_blk);
    if (DB_FAILURE_MACRO(status))
        return (status);

    /* initialize the statement node
     */
    MEfill( sizeof(PST_STATEMENT), (u_char) 0, (PTR) stmt);

    stmt->pst_mode = qmode;
    stmt->pst_type = PST_EXEC_IMM_TYPE;

#ifdef xDEBUG
    stmt->pst_lineno = sess_cb->pss_lineno;
#endif

    exec_node = &(stmt->pst_specific.pst_execImm);

    /*
    ** Initialize the PST_INFO portion of the PST_EXEC_IMM area
    */

    exec_node->pst_info.pst_baseline	 = sess_cb->pss_lineno;
    exec_node->pst_info.pst_basemode	 = basemode;
    exec_node->pst_info.pst_execflags	 = PST_SCHEMA_MODE;
    exec_node->pst_info.pst_deplist 	 = deplist;
    exec_node->pst_info.pst_srctext	 = stmtstart;

    /* since we know no data can be entered into a table during CREATE SCHEMA,
    ** we can set a bit telling ALTER TABLE to skip the 'existing data check.'
    ** 
    ** NOTE: if/when we allow CREATE TABLE .. AS SELECT in CREATE SCHEMA,
    ** this will not always be true, and we'll have to set this correctly.
    */
    if (qmode == PSQ_ALTERTABLE)
    {
	exec_node->pst_info.pst_execflags |= PST_EMPTY_TABLE;
    }
    
    /*
    ** If a text string was provided,
    ** and if this is not a user-specified statement,
    ** allocate buffer for substatement query text and
    ** anchor it in the PST_EXEC_IMM portion of the PST_STATEMENT node.
    ** Storage for user-specified statements is allocated
    ** after the entire CREATE SCHEMA statement is parsed.
    */
    if ((stmtstart != (char *) NULL) &&
	(stmtlen != 0))
    {
    	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			stmtlen+3, (PTR *) &exec_node->pst_qrytext, err_blk);
    	if (DB_FAILURE_MACRO(status))
            return (status);
	exec_node->pst_qrytext->db_t_count = stmtlen;
    	MEmove (stmtlen, (PTR)stmtstart, EOS,
		stmtlen+1, (PTR)&exec_node->pst_qrytext->db_t_text);
    }

    /*
    ** Chain this node to the query tree
    **
    ** NOTE: instead of keeping a separate "last" pointer stored away in the
    ** session block or somewhere, we will overload the pst_link pointer in
    ** the head stmt_list node (which is always the CREATE SCHEMA node).
    ** This pst_link pointer will always point to the LAST node in the
    ** statement list.
    ** In psl_reorder_schema, after we finish processing and re-ordering
    ** the list (and just before we pass the list back to OPF), we will
    ** scan the list and reset all the pst_link's correctly.
    */
    stmt_list->pst_link->pst_next = stmt;
    stmt_list->pst_link = stmt;

    return(E_DB_OK);
}

/*
** Name: psl_cs03s_create_table - Semantic actions for create_table:
**				  when issued within CREATE SCHEMA wrapper
**
** Description:
**      This routine performs the semantic action for create_table:
**	when called within the CREATE SCHEMA wrapper.
**
** Inputs:
**      sess_cb             ptr to a PSF session CB
**      psq_cb              PSF request CB
**	stmtstart	    Pointer to start of query text for substatement
**	deplist		    ptr to a list of dependency info blocks
**	cons_list	    ptr to a list of constraint info blocks
**
** Outputs:
**      psq_cb              PSF request CB
**         psq_error        will be filled in if an error occurs
**
** Returns:
**      E_DB_{OK, ERROR}
**
** History:
**      10-dec-1992 (ralph)
**          Written for CREATE SCHEMA
**	08-jan-1993 (ralph)
**	    Generate ALTER TABLE nodes
**	20-jan-1993 (ralph)
**	    Changed call interface to psq_tmulti_out() to conform to new version
**	19-feb-1993 (walt)
**		Added (u_char *) cast to 'stmtstart' because Alpha compiler complained
**		about subtracting it from sess_cb->pss_prvtok which has that type.
**	21-jan-1993 (ralph)
**	    Call psl_cons_text() in psl_cs03s_create_table()
**	    to generate CONSTRAINT text
**	17-mar-1993 (rblumer)
**	    Move code for generating ALTER TABLE text into psl_gen_alter_text
**	    in pslcons.c.
**	24-may-1993 (ralph)
**	   fix bug with GRANT WGO and CREATE VIEW WCO:
**	   change semantic interface to psl_alloc_exec_imm().
*/
DB_STATUS
psl_cs03s_create_table(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
	char		*stmtstart,
	PST_OBJDEP	*deplist,
	PSS_CONS	*cons_list)
{
    PSS_CONS		*current_cons;
    DB_STATUS           status;
    DB_ERROR            *err_blk= &psq_cb->psq_error;

    /*
    ** Allocate a PST_STATEMENT for the CREATE TABLE substatement
    */
    status = psl_alloc_exec_imm(sess_cb, psq_cb, stmtstart, 0,
				deplist, PSQ_CREATE, PSQ_CREATE);
    if (DB_FAILURE_MACRO(status))
        return (status);

    /*
    ** Generate ALTER TABLE statements for REFERENTIAL constraints
    */
    for (current_cons = cons_list;
	 current_cons != (PSS_CONS *) NULL;
	 current_cons = current_cons->pss_next_cons)
    {
	DB_TEXT_STRING	*query_text;

	status = psl_gen_alter_text(sess_cb, &sess_cb->pss_ostream,
				    &sess_cb->pss_memleft,
				    &deplist->pst_depown,
				    &deplist->pst_depname,
				    current_cons, &query_text, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* Allocate node for ALTER TABLE */
	status = psl_alloc_exec_imm(sess_cb, psq_cb,
				    (char *) query_text->db_t_text,
				    query_text->db_t_count,
				    (PST_OBJDEP *) NULL, 
				    PSQ_ALTERTABLE, PSQ_CREATE);
    	if (DB_FAILURE_MACRO(status))
            return (status);
    }

    return(E_DB_OK);
}

/*
** Name: psl_cs04s_create_view - Semantic actions for create_view:
**				  when issued within CREATE SCHEMA wrapper
**
** Description:
**      This routine performs the semantic action for create_view:
**	when called within the CREATE SCHEMA wrapper.
**
** Inputs:
**      sess_cb             ptr to a PSF session CB
**      psq_cb              PSF request CB
**	stmtstart	    Pointer to start of query text for substatement
**	deplist		    ptr to a list of dependency info blocks
**
** Outputs:
**      psq_cb              PSF request CB
**         psq_error        will be filled in if an error occurs
**
** Returns:
**      E_DB_{OK, ERROR}
**
** History:
**      10-dec-1992 (ralph)
**          Written for CREATE SCHEMA
**	19-feb-1993 (walt)
**		Added (u_char *) cast to 'stmtstart' because Alpha compiler complained
**		about subtracting it from sess_cb->pss_prvtok which has that type.
**	24-may-1993 (ralph)
**	   fix bug with GRANT WGO and CREATE VIEW WCO:
**	   change semantic interface to psl_alloc_exec_imm().
*/
DB_STATUS
psl_cs04s_create_view(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
	char		*stmtstart,
	PST_OBJDEP	*deplist)
{
    DB_STATUS           status;
    DB_ERROR            *err_blk= &psq_cb->psq_error;

    status = psl_alloc_exec_imm(sess_cb, psq_cb, stmtstart, 0,
				deplist, PSQ_VIEW, PSQ_VIEW);
    if (DB_FAILURE_MACRO(status))
        return (status);

    return(E_DB_OK);
}

/*
** Name: psl_cs05s_grant - Semantic actions for grant:
**				  when issued within CREATE SCHEMA wrapper
**
** Description:
**      This routine performs the semantic action for grant:
**	when called within the CREATE SCHEMA wrapper.
**
** Inputs:
**      sess_cb             ptr to a PSF session CB
**      psq_cb              PSF request CB
**	stmtstart	    Pointer to start of query text for substatement
**      deplist             ptr to a list of dependency info blocks
**
** Outputs:
**      psq_cb              PSF request CB
**         psq_error        will be filled in if an error occurs
**
** Returns:
**      E_DB_{OK, ERROR}
**
** History:
**      10-dec-1992 (ralph)
**          Written for CREATE SCHEMA
**	19-feb-1993 (walt)
**		Added (u_char *) cast to 'stmtstart' because Alpha compiler complained
**		about subtracting it from sess_cb->pss_prvtok which has that type.
**	24-may-1993 (ralph)
**	   fix bug with GRANT WGO and CREATE VIEW WCO:
**	   change semantic interface to psl_alloc_exec_imm().
*/
DB_STATUS
psl_cs05s_grant(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
	char		*stmtstart,
	PST_OBJDEP	*deplist)
{
    DB_STATUS           status;
    DB_ERROR            *err_blk= &psq_cb->psq_error;

    status = psl_alloc_exec_imm(sess_cb, psq_cb, stmtstart, 0,
				deplist, PSQ_GRANT, PSQ_GRANT);

    if (DB_FAILURE_MACRO(status))
        return (status);

    return(E_DB_OK);
}

/*
** Name: psl_cs06s_obj_spec - Semantic actions for obj_spec:
**				  when issued within CREATE SCHEMA wrapper
**				  on behalf of CREATE VIEW substatement
**
** Description:
**      This routine performs the semantic action for grant:
**	when called within the CREATE SCHEMA wrapper.
**
** Inputs:
**      sess_cb             ptr to a PSF session CB
**      psq_cb              PSF request CB
**	obj_spec	    object specification
**	obj_use		    flags that indicate object use;
**			    copied to PST_OBJDEP
**
** Outputs:
**      deplistp            ptr to list of PST_OBJDEP nodes
**      psq_cb              PSF request CB
**         psq_error        will be filled in if an error occurs
**
** Returns:
**      E_DB_{OK, ERROR}
**
** History:
**      10-dec-1992 (ralph)
**          Written for CREATE SCHEMA
**	29-dec-1992 (ralph)
**	    Preserve the first entry on the dependency "stack".
**	    That way, the first entry always contains the object
**	    name for CREATE TABLE/VIEW statements.
**	    Pass in object use flag, so that object reordering
**	    can validate intraschema references are resolved,
**	    yet not use an object dependency during reordering.
*/
DB_STATUS
psl_cs06s_obj_spec(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
	PST_OBJDEP	**deplistp,
	PSS_OBJ_NAME	*obj_spec,
	u_i4	obj_use)
{
    PST_OBJDEP		*dep;
    DB_STATUS           status;
    DB_ERROR            *err_blk= &psq_cb->psq_error;

    /*
    ** Allocate a PST_OBJDEP node to represent the dependency
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_OBJDEP),
                        (PTR *) &dep, err_blk);
    if (status != E_DB_OK)
        return (status);

    /*
    ** initialize the structure
    */
    MEfill(sizeof(PST_OBJDEP), (u_char) 0, (PTR) dep);

    /*
    ** chain it in
    */
    if (*deplistp == (PST_OBJDEP *) NULL)
    {
	/* List was empty; this is the only one */
	*deplistp = dep;
    }
    else
    {
	/* List not empty; stack this one after header */
	dep->pst_nextdep = (*deplistp)->pst_nextdep;
	(*deplistp)->pst_nextdep = dep;
    }

    /*
    ** fill in object schema and name
    */

    MEcopy ((PTR)&obj_spec->pss_owner,
	    sizeof(dep->pst_depown),
	    (PTR)&dep->pst_depown);

    MEcopy ((PTR)&obj_spec->pss_obj_name,
	    sizeof(dep->pst_depname),
	    (PTR)&dep->pst_depname);

    dep->pst_depuse = obj_use;

    return(E_DB_OK);
}
