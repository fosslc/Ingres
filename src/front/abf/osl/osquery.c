/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include	<compat.h>
#include	<cm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<qg.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osloop.h>
#include	<osquery.h>
#include	<fdesc.h>
#include	<oslconf.h>
#include	<oskeyw.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<abqual.h>
#include	"traverse.h"
#include	<osglobs.h>

/**
** Name:	osquery.c -	OSL Translator Query Generation Module.
**
** Description:
**	Contains routines used to generate IL code for queries.
**
**	osqrygen()	generate query.
**	iiosQrNext()	generate next row for query.
**	osqgbreak()	generate end for query.
**
** History:
**	Revision 5.1  86/12/04  10:51:18  wong
**	Modified to correctly generate sort column specifications when
**	column positions are unknown due to being passed to another frame.
**	(11/86)  Corrected some bugs.  Also, now uses keyword/operator strings.
**	(10/86)  Initial revision.
**
**	Revision 6.0  87/06  wong
**	Added support for SQL.  Modified to generate query specifications
**	for the new QG implementation.
**
**	Revision 6.3/03/00  89/09  wong
**	Added support for generating separate specification sections for query
**	objects (i.e., parameter queries and eventually query objects.)  This
**	allows the WHERE clause to be easily modified.
**	11/13/90 (emerson)
**		Changes to osqrygen for bugs 34415 and 34440.
**
**	Revision 6.4
**	03/13/91 (emerson)
**		Separate arguments in lists in queries by ", " instead of ","
**		so that the back-end won't flag a syntax error if
**		II_DECIMAL = ','.
**
**	Revision 6.4/02
**	10/02/91 (emerson)
**		Fix in osqgbreak for bugs 34788, 36427, 40195, 40196.
**	10/15/91 (emerson)
**		Part of fix for bug 39492 in osqrygen.
**	11/07/91 (emerson)
**		Replaced iiIGgetOffset and iiIGupdStmt by iiIGlsaLastStmtAddr
**		in function osqrygen; modified osqgbreak again
**		(for bugs 39581, 41013, and 41014).
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Major changes to osqrygen, qryput, and iiosQrNext,
**		for bugs 34846, 39582, and 46761.
**
**	Revision 6.4/05
**	10/11/93 (donc) Bug 44318
**		Add support for numerical and string literals in UNIONed
**		SELECTs without having to provide the non-intuitive AS
**		clause.
**
**	Revision 6.5
**	26-jun-92 (davel)
**		correct call to ostmpalloc(), which now takes an additional
**		precision argument.
**
**  25-july-97 (rodjo04)
**      Added variable peval_qry, a pointer to eval_qry. 
**      Added variable on_clause. The values can be as follows;
**      0 Empty, on Join...On clause not encountered,
**      1 SELECT_JOIN_ON, In a Join...On clause.          
**      2 SUBSELECT_JOIN_ON, In a Sub select of Join...On clause. 
**  10-mar-2000 (rodjo04) BUG 98775.
**      Removed above change (bug 82837). 
**      Now when we come across a from_list we will evaluate the variable 
**      correctly. Added support for all datatypes. 
**  11-jun-2001 (rodjo04) Bug 104887
**      Added case for datatypes DB_CHA_TYPE and DB_VCH_TYPE, instead of 
**      using default so that these datatypes will be evaluated correctly.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-oct-2001 (somsa01)
**	    Backed out change 453178 (rodjo04) for bug 104887.
**  27-nov-2001  (rodjo04) Bug 106438
**      Again I am removeing the above changes (mar 10). 
**      I have created a new function set_onclause(). 
**  22-may-2002 (somsa01)
**	Corrected cross-integration.
**    25-Oct-2005 (hanje04)
**        Add prototype for qryput() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**  07-jun-2006 (smeke01) Bug 116193
**	Removed function set_onclause() as no longer required (see
**	comment for osqtrvfunc() in osqtrav.c).
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

static ILREF	tlargv_gen();
static i4	tl_count();
static VOID	spec_qry();
static VOID	spec_tlist();
static VOID	spec_print();
static VOID	spec_qeval();
static VOID	spec_eval();
static VOID	spec_pred();
static VOID	spec_flush();
static VOID	spec_end();
static i4	spec_count();
static VOID	qry_pred();
static VOID	qrysort();
static i4	qryput();


FUNC_EXTERN VOID      	IIOSgqiGenQryId();
FUNC_EXTERN VOID	ospredgen();
FUNC_EXTERN STATUS	osblkbreak();

extern bool may_be_field;
extern bool must_be_field;
extern bool under_select;

/*
** Reference (Name) Node Evaluation.
*/
static const
	TRV_FUNC	eval_names = {
				spec_print,
				spec_eval,
				NULL,
				NULL,
				NULL,
				2
};

/*
** Query Node Evaluation.
*/
static const
	TRV_FUNC	eval_qry = {
				spec_print,
				spec_qeval,
				spec_pred,
				spec_tlist,
				spec_eval,
				2
};



static struct {
	i4		s_cnt;
	char	*s_bp;
	char	s_buf[IL_MAX_STRING + 1];
} spec = {0, spec.s_buf, ERx("")};

/*{
** Name:	osqrygen() -	 Generate Query.
**
** Description:
**	Generates IL code that sets up the structures for a query,
**	and optionally retrieves the first row or all the rows.
**
** Generates:
**		QRY qvar form attr
**		QRYSZE nargv nspecs
**		[ QRYPRED type n TL2ELM fldcol spec ... ] ...
**		TL2ELM name var
**		...
**		ENDLIST
**		TL2ELM ref type
**		ENDLIST
**		QRYEND dml mode putlist
**		[ query child ... QRYCHILD qvar qcvar ]
**
**		[ optional IL to retrieve first row or all rows, depending on
**		  context and on type of l.h.s. of the SELECT, as follows: ]
** OS_QRYLOOP:
**		QRYSINGLE qvar endsid
**		topsid:
**		[ IL to set SELECT targets from temps that were selected into ]
**
** OS_SUBMENU (attached query into form):
**		QRYGEN qvar msg endsid
**		[ IL to set SELECT targets from temps that were selected into ]
**
** OS_SUBMENU (attached query into tablefield or array):
**		QRYGEN qvar msg endsid
**
** OS_SUBMENU (attached query into record):
**		QRYGEN qvar msg endsid
**		CLRREC record
**		[ IL to set attributes of record from temps selected into ]
**
** OS_SUBMENU (attached query into row of tablefield):
**		[ IL to set tfrownum ]
**		QRYGEN qvar msg endsid
**		CLRROW tfname tfrownum
**		ENDLIST
**		[ IL to set columns of tablefield row from temps selected into ]
**
** OS_QRYSTMT (select into form):
**		QRYSINGLE qvar endsid
**		[ IL to set SELECT targets from temps that were selected into ]
**		QRYBRK qvar
**		endsid:
**
** OS_QRYSTMT (select into tablefield or array):
**		QRYSINGLE qvar endsid
**		endsid:
**
** OS_QRYSTMT (select into record):
**		QRYROWGET qvar endsid
**		CLRREC record
**		QRYROWCHK qvar endsid
**		[ IL to set attributes of record from temps selected into ]
**		endsid:
**
** OS_QRYSTMT (select into row of tablefield):
**		[ IL to set tfrownum ]
**		QRYROWGET qvar endsid
**		CLRROW tfname tfrownum
**		ENDLIST
**		QRYROWCHK qvar endsid
**		[ IL to set columns of tablefield row from temps selected into ]
**		endsid:
**
** Inputs:
**	qnode	{OSNODE *}	Query node.
**	context	{nat}		OS_CALLF, OS_SUBMENU, OS_QRYLOOP or OS_QRYSTMT.
**				The context in which the query is being used.
**	frame	{bool}		Whether query is for frame (or procedure.)
**	endsid	{IGSID *}	End query SID.  Relevant only if context is
**				OS_SUBMENU or OS_QRYLOOP.
** Outputs:
**	topsid	{IGSID *}	SID of the "top" of the query.  Relevant only if
**				context is OS_QRYLOOP.  This will be set to
**				the IL that sets SELECT targets from temps that
**				were selected into.  The GOTO at the bottom
**				of the SELECT loop will specify this SID;
**				there will be no need to replicate the IL that
**				sets SELECT targets from temps.
**				A WHILE loop label (for codegen) will also
**				be generated for this SID.
** Side Effects
**
** History:
**	09/86 (jhw) -- Rewritten for OSL translator.
**	20-aug-1985 First Written
**
**      13-feb-1990 (Joe)
**          Added call to generate a IL_QID statement after a IL_QRYEND
**          statement of a repeated select.  This generates query ids
**          as unique as the embedded languages. JupBugs 10015
**
**	11/13/90 (emerson)
**		When the l.h.s. of a query is a record,
**		make the 3rd operand of the IL_QRY refer to the l.h.s.
**		(as was already being done when the l.h.s. was an array).
**		(Bug 34440).
**	11/13/90 (emerson)
**		Check for OSGLOB symbols as well as OSVAR symbols
**		(bug 34415).
**	10/15/91 (emerson)
**		Check for OSTEMP symbols as well as OSVAR  and OSGLOB symbols
**		to allow for a VALUE node representing the current row of
**		an array in an UNLOADTABLE loop (part of fix for bug 39492).
**	11/07/91 (emerson)
**		Replaced iiIGgetOffset and iiIGupdStmt by iiIGlsaLastStmtAddr
**		(for bugs 39581, 41013, and 41014).
**	09/20/92 (emerson)
**		Major changes for bugs 34846 and 39582.  Highlights:
**		Calling sequence changed; topsid added.
**		A select into record now generates zero for the 3rd
**		operand of IL_QRY; the whole section that computes
**		the 2nd and 3rd operands of IL_QRY has been reworked
**		(to make it clearer).  Completely reworked the logic
**		that generates code to do retrieve of first row:
**		This now uses the new IL_QRYROWGET, IL_QRYROWCHK, and
**		IL_CLRREC to clear row targets when appropriate.
**		Replaced iiIGlsaLastStmtAddr by iiIGnsaNextStmtAddr.
**	20-Jul-2007 (kibro01) b118760
**	    Further refinement of fix for b118229 to ensure that we only
**	    change previous behaviour if we are in a query - if we are, then
**	    parameters are assumed to be column/table names unless on the
**	    right hand side of a boolean operator (e.g. WHERE col = :val)
**	24-Sep-2007 (kibro01) b119170
**	    ORDER BY clauses should act like FROM clauses as far as assuming
**	    that variables represent field names rather than values.  This
**	    does work for both ordinals (:x being 2) as well as field names
**	    (:l being 'mycolname').
*/

VOID
osqrygen ( qnode, context, frame, endsid, topsid )
OSNODE	*qnode;
i4	context;
bool	frame;
IGSID	*endsid;
IGSID	*topsid;
{
	register OSNODE	*qn;
	register OSQRY	*qry;
	register OSNODE	*fobj;
	register OSSYM	*sym;
	bool	prev_under;

	OSSYM	*ostmpalloc();

	if (qnode == NULL)
		return;		/* JRC ERROR */

	for (qn = qnode; qn != NULL; qn = qn->n_child)
	{
		DB_LANG		dml;
		ILREF		putform_list;
		IL		*qrysze_stmt;
		ILREF		form_name;
		ILREF		obj_ref;

		if (qn->n_token != tkQUERY || qn->n_formobj == NULL/* ||
				(qry = qn->n_query)->qn_type != tkQUERY */)
			return;		/* JRC/AGH ERROR */

		qry = qn->n_query;
		dml = qry->qn_dml;

		/* Allocate query structure. */

		qn->n_ilref = ostmpalloc(DB_QUE_TYPE, 0, (i2)0)->s_ilref;

		/* Start query generation. */

		fobj = qn->n_formobj;

		/*
		** Compute the 2nd and 3rd operands of the IL_QRY statement
		** (so-called form_name and obj_ref).
		*/
		switch (fobj->n_token)
		{
		   case ATTR:
			form_name = IGsetConst(DB_CHA_TYPE, fobj->n_name);
			if (fobj->n_attr == NULL)
			{
				obj_ref = 0;
				break;
			}
			obj_ref = IGsetConst(DB_CHA_TYPE, fobj->n_attr);
			break;

		   case VALUE:
			sym = fobj->n_sym;
			switch (sym->s_kind)
			{
			   case OSFORM:
				if (!frame)
				{
					form_name = 0;
					obj_ref = 0;
					break;
				}
				form_name = IGsetConst(DB_CHA_TYPE,sym->s_name);
				obj_ref = 0;
				break;

			   case OSTABLE:
				if (fobj->n_flags & N_ROW)
				{
					form_name = 0;
					obj_ref = 0;
					break;
				}
				form_name = IGsetConst( DB_CHA_TYPE,
							sym->s_parent->s_name );
				obj_ref = IGsetConst( DB_CHA_TYPE,
						      sym->s_name );
				break;

			   case OSVAR:
			   case OSGLOB:
			   case OSTEMP:
				goto lhs_must_be_rec_or_array;

			   default:
				osuerr( OSBUG, 1,
					ERx("osqrygen(bad sym->s_kind)") );
			}
			break;

		   case DOT:
		   case ARRAYREF:
		   lhs_must_be_rec_or_array:
			if (!(fobj->n_flags & N_COMPLEXOBJ))
			{
				osuerr( OSBUG, 1,
					ERx("osqrygen(l.h.s. is simple obj)") );
			}
			form_name = 0;
			if (fobj->n_flags & N_ROW)
			{
				obj_ref = 0;
				break;
			}
			obj_ref = fobj->n_ilref;
			break;

		   default:
			osuerr(OSBUG, 1, ERx("osqrygen(bad fobj->n_token)"));
		}
		IGgenStmt( IL_QRY, (IGSID *)NULL, 3,
			   qn->n_ilref, form_name, obj_ref );
		qrysze_stmt = iiIGnsaNextStmtAddr();
		IGgenStmt(IL_QRYSZE, (IGSID *)NULL, 2, 0, 0);

		/*
		** Generate WHERE (and HAVING) Predicates (qualifications)
		** for all sub-queries including those in any aggregates
		** in any target list expressions.
		*/
		/*
		** This section is a query, so apply rules for whether
		** parameters can be table or column names instead of
		** literal values (kibro01) b118760
		*/
		prev_under = under_select;
		under_select = TRUE;
		qry_pred(qry->qn_body);
		under_select = prev_under;

		/* Generate Argv List and PUTFORM string. */
		putform_list = tlargv_gen(qry, frame);

		/* Generate Query Specifications. */
		prev_under = under_select;
		under_select = TRUE;
		spec_qry(dml, qry->qn_body, (bool)( context == OS_CALLF ));
		under_select = prev_under;

		/* Generate SORT or ORDER BY list. */
		if (qry->qn_sortlist != NULL)
		{
			if (context == OS_CALLF)
			{ /* separate sections for query objects */
				spec_end();
			}
			if (qry->qn_sortlist->srt_type == SRT_SORT)
				spec_print(ERx(" sort by "));
			else /* qry->qn_sortlist->srt_type == SRT_ORDER */
				spec_print(ERx(" order by "));
			must_be_field = TRUE;
			osevalsklist(qry->qn_sortlist, qrysort, dml);
			must_be_field = FALSE;
		}

		spec_end();
		/*
		** Backpatch.
		*/
		ILsetOperand(qrysze_stmt, 1, tl_count());
		ILsetOperand(qrysze_stmt, 2, spec_count());
		/*
		** End query generation
		*/
		{
			i4	mode = 0;

			mode = qry->qn_repeat ? QM_REPEAT : QM_NORMAL;
			if (qn != qnode->n_child && context != OS_QRYSTMT)
			{ /* not a child or a singleton */
				/* nest the query as a cursor
				** or by mapping to a file.
				*/
				mode |= qry->qn_cursor ? QM_CURSOR : QM_MAP;
			}
			if (qry->qn_repeat)
				IIOSgqiGenQryId(osFrm);
			IGgenStmt( IL_QRYEND, (IGSID *)NULL, 3,
				   dml, mode, putform_list );
		}
	} /* end for */

	/*
	** Set a Master/Detail.
	*/
	if (qnode->n_child != NULL)
	{
		IGgenStmt( IL_QRYCHILD, (IGSID *)NULL, 2,
			   qnode->n_ilref, qnode->n_child->n_ilref );
	}

	/*
	** Generate code to do initial retrieve.
	*/
	fobj = qnode->n_formobj;

	/*
	** If we're selecting into tablefield row, then compute its subscript.
	** This can arise for contexts of OS_SUBMENU or OS_QRYSTMT.
	** Note that we do this *before* we attempt to retrieve a row,
	** so that if evaluation of the subscript has any side effects,
	** they will occur even if the select fails.  This may seem wrong,
	** but it's consistent with the way we handle all other types of select
	** (and with the way we did things in previous releases):
	** we always compute the l-value of the l.h.s. before attempting
	** the select (even though we leave the l.h.s. itself alone if
	** the select fails).
	*/
	if ( (fobj->n_flags & (N_TABLEFLD|N_ROW)) == (N_TABLEFLD|N_ROW)
	   && fobj->n_sexpr != NULL
	   )
	{
		fobj->n_sref = osvalref(fobj->n_sexpr);
		fobj->n_sexpr = NULL;
	}

	/*
	** Now generate code to try to retrieve the first row.
	*/
	switch (context)
	{
	   case OS_CALLF:		/* SELECT passed in CALLFRAME */
		break;

	   case OS_QRYLOOP:		/* SELECT loop */
		/*
		** Select first row (into the form).  Exit the SELECT loop
		** (go to endsid) if the SELECT has an error or returns 0 rows.
		*/
		IGgenStmt(IL_QRYSINGLE, endsid, 1, qnode->n_ilref);

		/*
		** The select returned a row.  Copy the columns
		** from temps into the actual targets, as required.
		** The QRYNEXT at the bottom of the SELECT loop
		** will branch here, so we need to set the SID
		** which that QRYNEXT will specify,
		** and we need to generate an LSTHD for codegen.
		*/
		IGsetSID(topsid);
		IGstartStmt(0, IL_LB_WHILE);
		qryput(qnode);

		break;

	   case OS_SUBMENU:		/* attached query */
		/*
		** Select first row (into a form, rec, or tf row), or
		** select all rows (in a tf or arry).  Exit the attached query
		** (go to endsid) if the SELECT has an error or returns 0 rows.
		*/
		IGgenStmt(IL_QRYGEN, endsid, 2, qnode->n_ilref, TRUE);

		/*
		** Handle the case where the select returned a row:
		**
		** First, if we're selecting into a row object (rec,
		** array row, or tf row), then clear it.
		*/
		if (fobj->n_flags & N_ROW)
		{
			if (fobj->n_flags & N_COMPLEXOBJ) /* rec or array row */
			{
				IGgenStmt( IL_CLRREC, (IGSID *)NULL, 1,
					   fobj->n_ilref );
			}
			else				/* tablefield row */
			{
				IGgenStmt( IL_CLRROW, (IGSID *)NULL, 2,
					   IGsetConst( DB_CHA_TYPE,
						       fobj->n_sym->s_name ),
					   fobj->n_sref );

				IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
			}
		}

		/*
		** Now copy columns from temps into actual targets, as required.
		*/
		qryput(qnode);

		break;

	   case OS_QRYSTMT:		/* SELECT w/o loop or submenu */
		endsid = IGinitSID();

		if (!(fobj->n_flags & N_ROW))	/* form, tablefield, or array */
		{
			/*
			** Select one row (into the form), or
			** select all rows (into a tablefield or array).
			** Go to endsid on error (or zero rows).
			*/
			IGgenStmt(IL_QRYSINGLE, endsid, 1, qnode->n_ilref);

			/*
			** The select returned row(s).  Copy the columns
			** from temps into the actual targets, as required.
			** If we're selecting into an array or tablefield,
			** no copying will need to be done, but we go through
			** the motions of a no-op qryput on priciple:  All
			** queries go through it unless context == OS_CALLF.
			*/
			qryput(qnode);

			/*
			** If we're selecting into the form, then kill
			** the query.  (We only retrieved one row,
			** and there may be more).
			*/
			if ((fobj->n_flags & (N_TABLEFLD|N_COMPLEXOBJ)) == 0)
			{
				IGgenStmt( IL_QRYBRK, (IGSID *)NULL, 1,
					   qnode->n_ilref );
			}
		}
		else				/* rec, array row, or tf row */
		{
			/*
			** Select zero or one row; go to endsid on error
			*/
			IGgenStmt(IL_QRYROWGET, endsid, 1, qnode->n_ilref);

			/*
			** Select succeeded (but may have returned zero rows).
			** Now clear the row object.
			*/
			if (fobj->n_flags & N_COMPLEXOBJ) /* rec or array row */
			{
				IGgenStmt( IL_CLRREC, (IGSID *)NULL, 1,
					   fobj->n_ilref );
			}
			else				/* tablefield row */
			{
				IGgenStmt( IL_CLRROW, (IGSID *)NULL, 2,
					   IGsetConst( DB_CHA_TYPE,
						       fobj->n_sym->s_name ),
					   fobj->n_sref );

				IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
			}

			/*
			** If select returned zero rows (but succeeded),
			** go to endsid.
			*/
			IGgenStmt(IL_QRYROWCHK, endsid, 1, qnode->n_ilref);

			/*
			** The select returned a row.  Copy the columns
			** from temps into the record attributes, or do PUTROW.
			*/
			qryput(qnode);

			/*
			** Kill the query.
			*/
			IGgenStmt(IL_QRYBRK, (IGSID *)NULL, 1, qnode->n_ilref);
		}
		IGsetSID(endsid);

		break;

	   default:
		osuerr(OSBUG, 1, ERx("osqrygen(bad context)"));
	}
}

/*
** Name:	qryput() -	Generate IL to set query targets
**
** Description:
**	For the query nodes, generates IL to assign a selected row
**	into its targets.  This includes (re)computing their l-values,
**	and doing PUTROWs for table field cells.  It does *not* include
**	doing any PUTFORMs for simple fields; that's handled via the
**	PUTFORM string (which is inspected by the ABF run time query
**	support routine).
**
** History:
**	07/90 (jhw) -- Written.
**	09/20/92 (emerson)
**		Removed the logic that was generating PUTROWs for any
**		tblfld[i].col target elements of a select into the form;
**		that's now being handled by the target_name:var_colon rule
**		in sql.sy; emit the generated IL here.
**		The old logic didn't generate proper IL to recompute
**		the "i" in tblfld[i].col after each NEXT.  (Bug 34846).
*/
static VOID	tblrow_put();

static
qryput ( qry )
register OSNODE	*qry;
{
	for ( ; qry != NULL; qry = qry->n_child )
	{
		register OSNODE	*fobj = qry->n_formobj;

		/*
		** If any IL was saved off to assign a selected row
		** into its targets, emit it here.
		** This includes PUTROWs for individual targets that are
		** tablefield cells (the l.h.s. of the SELECT is the form).
		** Note that no IL will have been generated for QUEL;
		** only the target_list_end rule in sql.sy generates such IL.
		*/
		if (qry->n_targlfrag != NULL)
		{
			iiIGifIncludeFragment(qry->n_targlfrag);
		}

		/*
		** For the case where the l.h.s. of the SELECT was
		** a table field row, we need to generate a PUTROW.
		** IL has already been emitted to compute the table field row
		** subscript; we *don't* want to recompute it after each row.
		** Note that although it's undocumented, it's apparently worked
		** since 6.3/03 to do a QUEL retrieve into a table field row.
		*/
		if ((fobj->n_flags & (N_TABLEFLD|N_ROW)) == (N_TABLEFLD|N_ROW))
		{
			register OSQRY	*qryp = qry->n_query;

			IGgenStmt( IL_PUTROW, (IGSID *)NULL, 2,
				   IGsetConst(DB_CHA_TYPE, fobj->n_sym->s_name),
				   fobj->n_sref );

			while ( qryp->qn_type != tkQUERY )
				qryp = qryp->qn_left;

			ostltrvfunc(qryp->qn_tlist, tblrow_put);

			IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
		}
	}
}

static VOID
tblrow_put ( ele, value, dim, comma )
OSNODE	*ele;
OSNODE	*value;
OSNODE	*dim;
bool	comma;
{
	register OSSYM	*sym = ele->n_sym;

	IGgenStmt( IL_TL2ELM, (IGSID *)NULL, 2,
		   IGsetConst(DB_CHA_TYPE, sym->s_name), sym->s_ilref );
}

/*{
** Name:	iiosQrNext() -	Generate QRYNEXT for Query.
**
** Description:
**	Generates the IL code necessary to get the next row of a query.
**	This function may be called either (1) at the bottom of a SELECT loop,
**	or (2) for a NEXT statement.  In case (1), the caller
**	must supply non-null "qry" and "sid" input parameters;
**	"sid" is assumed to represent a label at the top of the SELECT loop,
**	just above IL to re-compute the targets of the SELECT loop.
**	In case (2), the caller must supply a NULL "sid" input parameter.
**
** Inputs:
**	qry	{OSNODE *} 	The query node.  May be NULL in case (2).
**	sid	{IGSID *}	See description.
**	msgon	{bool}		Whether to print a message on end.
**
** History:
**	12/89 (jhw) -- Written.
**	09/20/92 (emerson)
**		Revamped the logic for bugs 34846 and 46761.
**		Changed the meaning of the "sid" input parameter.
**		Removed the static function qry_ref that was being called
**		only by this function.
*/
VOID
iiosQrNext ( qry, sid, msgon, settargs )
OSNODE	*qry;
IGSID	*sid;
bool	msgon;
{
	/*
	** If no query node is provided, generate a QRYNEXT
	** that will fall through on success or failure.
	*/
	if (qry == NULL)
	{
		IGgenStmt(IL_QRYNEXT, (IGSID *)NULL, 2, (ILREF)0, msgon);
		return;
	}

	/*
	** If we're at the bottom of a SELECT loop,  generate a QRYNEXT
	** that will branch back to the top of the SELECT loop on success.
	*/
	if (sid != NULL)
	{
		IGgenStmt( IL_QRYNEXT|ILM_BRANCH_ON_SUCCESS, sid, 2,
			   qry->n_ilref, msgon );
		return;
	}

	/*
	** If we're implementing a NEXT within an attached query or SELECT loop,
	** then generate a QRYNEXT and following IL that will:
	** (1) Break out of the attached query or SELECT loop (cleaning up
	** any intervening loops or submenus), if the QRYNEXT fails.
	** (2) Re-compute the targets of the attached query or SELECT loop
	** and then fall through, if the QRYNEXT succeeds.
	*/
	sid = IGinitSID();

	IGgenStmt( IL_QRYNEXT|ILM_BRANCH_ON_SUCCESS, sid, 2,
		   qry->n_ilref, msgon );

	(VOID) osblkbreak(LP_QUERY|LP_QWHILE, (char *)NULL);

	IGsetSID(sid);
	qryput(qry);
}

/*{
** Name:	osqgfree() -	Free Query Node Structures.
**
** Description:
**	Free up query retrieve structures in comma list node.  This routine is
**	called by 'osqrygen()' (above) to free the query structures in a query
**	node.  However, it is also called by 'osgencall()' to free up any extra
**	query nodes allowed syntatically in a call frame parameter list.  So,
**	it must be careful not to free the query structures more than once.
**
** Inputs:
**	qnode	{OSNODE *}  The query node.
**
** History:
**	08/85 (joe) -- Written.
*/
VOID
osqgfree (qnode)
OSNODE	*qnode;
{
	register OSNODE	*qp = qnode;;

	/* Skip comma nodes */
	for ( ; qp != NULL && (qp->n_token == COMMA || qp->n_token == BLANK) ;
		qp = qp->n_left )
	{
		if (qp->n_right->n_token == tkQUERY)
		{
			qp = qp->n_right;
			break;
		}
	}

	/* Free up query retrieve structures */
	for (; qp != NULL && qp->n_token == tkQUERY ; qp = qp->n_child)
	{
		if (qp->n_query != NULL)
		{
			osqryfree(qp->n_query);
			qp->n_query = NULL;
		}
	}
}

/*{
** Name:	osqgbreak() -	Generate IL Code to break out of a Query.
**
** Description:
**	This routine generates code to break out of a query.
**
** Input:
**	qnode	{OSNODE *}  Query node.
**
** History:
**	09/86 (jhw) -- Written.
**	10/90 (jhw) -- Modified to get the query type (context) and to generate
**		POPSUBMENU if for an attached query.  Part of fix for #31929.
**	10/02/91 (emerson)
**		Removed conditional logic to generate POPSUBMENU.
**		Changed second parm to a bool.
**		(For bugs 34788, 36427, 40195, 40196).
**	11/07/91 (emerson)
**		Changes for bugs 39581, 41013, and 41014:
**		removed second parm (which specified whether the run-time
**		query structures should be freed); osblkbreak in osblock.c
**		now call ostmprls, which (among other things) issues
**		the necessary IL_QRYFREE instructions.
*/
VOID
osqgbreak ( qnode )
OSNODE	*qnode;
{
	IGgenStmt(IL_QRYBRK, (IGSID *)NULL, 1, qnode->n_ilref);
}

/*
** Name:	null() -	Do Nothing Function.
**
** Description:
**	Returns immediately.
**
** History:
**	05/87 (jhw) -- Written.
*/
static VOID
null ()
{
	return;
}

/*
** Name:	tlargv_gen() -	Generate Target Name/Value List.
**
** Description:
**	Generate the target name/value argument list for a query.
**
** Returns:
**	{char *}  The `putform' list for the target list.
**
** History:
**	02/88 (jhw) -- Written.
*/

/*
** Structures for `putform' buffer.
*/
struct _list {
	char	*l_bp;			/* buffer pointer */
	char	l_buf[IL_MAX_STRING+1];	/* buffer */
};

static VOID	list_init();
static VOID	list_print();
static char	*list_get();

static VOID	qry_tlargv();

static ILREF
tlargv_gen (qry, frame)
OSQRY	*qry;
bool	frame;
{
	struct _list	plist;
	register char	*pl;

	list_init(&plist);
	qry_tlargv(qry->qn_body);
	pl = list_get();
	IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
	return ( ( 	STindex(pl, ERx("a"), 0) != NULL ||
				STindex(pl, ERx(" "), 0) != NULL ||
				STindex(pl, ERx("A"), 0) ) && frame )
					? IGsetConst(DB_CHA_TYPE, pl) : 0;
}

static VOID	tlput_form();

static VOID
qry_tlargv (qry)
register OSQRY	*qry;
{
	if ( qry->qn_type == tkQUERY )
	{
		if ( qry->qn_tlist != NULL )
			ostltrvfunc(qry->qn_tlist, tlput_form);
	}
	else /* ( qry->qn_type == PARENS || qry->qn_type == OP ) */
	{
		qry_tlargv(qry->qn_left);
	}
}

/*
** Name:	tlput_form() -	Generate PUTFORM String and Target Name/Value
**					List for Target List Element.
** Description:
**	Generates a PUTFORM string for a target list element and outputs
**	a name/value list element for the target.
**
** Input:
**	ele	{OSNODE *}  Target list element target node.
**	value	{OSNODE *}  Target list element value.
**	dim	{OSNODE *}  NULL.
**	comma	{bool}  TRUE if not last element in list.
**
** History:
**	09/86 (jhw) -- Written.
**	10/90 (jhw) - Added support for DOT attributes (to support owner.table)
**		Bug #33655.
**	09/20/92 (emerson)
**		Revamped the logic so that I could understand it.
*/
static i4	tlargv_count = 0;

/*ARGSUSED*/
static VOID
tlput_form (ele, value, dim, comma)
OSNODE	*ele;
OSNODE	*value;
OSNODE	*dim;
bool	comma;
{
	char	*putform_string;
	char	*name;
	i2	ilref;
	OSSYM	*sym;

	++tlargv_count;

	switch (ele->n_token)
	{
	   case ATTR:
		if (osw_compare(ele->n_attr, _All))
		{
			name = ERx("");
			ilref = 0;
			putform_string = ERx("%A");
		}
		else
		{
			name = ele->n_attr;
			ilref = 0;
			putform_string = ERx(" ");
		}
		break;

	   case tkID:
		name = ele->n_value;
		ilref = 0;
		putform_string = ERx(" ");
		break;

	   case DOT:
		if (ele->n_left->n_token == tkID)
		{
			name = ele->n_right->n_value;
			ilref = 0;
			putform_string = ERx(" ");
		}
		else
		{
			name = ele->n_right->n_sym->s_name;
			ilref = ele->n_lhstemp->s_ilref;
			putform_string = ERx("d");
		}
		break;

	   case VALUE:
		sym = ele->n_sym;
		name = sym->s_name;
		ilref = sym->s_ilref;
		putform_string = sym->s_kind == OSFIELD ? ERx("a") : ERx("d");
		break;

	   default:
		osuerr(OSBUG, 1, ERx("osqrygen(put_form)"));
	}
	list_print(putform_string);
	IGgenStmt( IL_TL2ELM, (IGSID *)NULL, 2,
		   IGsetConst(DB_CHA_TYPE, name), ilref );
}

static i4
tl_count()
{
	register i4	save = tlargv_count;

	tlargv_count = 0;
	return save;
}

/*
** Name:	target_list() -	Generate Target List Specification List.
**
** Description:
**	Generates a query specification list for a target list element.
**
** Input:
**	ele		{OSNODE *}  Target list element target node.
**	value	{OSNODE *}  Target list element value.
**	dim		{OSNODE *}  NULL.
**	comma	{bool}  TRUE if not last element in list.
**
** History:
**	09/86 (jhw) -- Written.
**	04/87 (jhw) -- Modified for Version 1 QG.
**	10/90 (jhw) - Added support for DOT attributes (to support owner.table)
**		Bug #33655.
*/
/*ARGSUSED*/
static VOID
target_list (ele, value, dim, comma)
OSNODE	*ele;
OSNODE	*value;
OSNODE	*dim;
bool	comma;
{
	if ( ele->n_token != VALUE && ele->n_token != tkID 
	  && ele->n_token != tkSCONST && ele->n_token != ATTR
	  && ele->n_token != DOT && ele->n_token != tkICONST
	  && ele->n_token != tkDCONST && ele->n_token != tkFCONST
          && ele->n_token != tkNULL
        )
	{
		osuerr(OSBUG, 1, ERx("osqrygen(target_list)"));
	}

	if (ele->n_token == ATTR && osw_compare(ele->n_attr, _All))
	{
		spec_flush();
		spec_print(ERx("%A"));
		spec_print(ele->n_name);
		spec_flush();
	}
	else
	{
		if ( ele->n_token == ATTR
			|| ( ele->n_token == DOT
				&& ele->n_left->n_token == tkID ) )
		{
			osqtrvfunc( &eval_names, ele );
			ostrfree( ele );
		}
		else
		{
			register char	*name;

			if (ele->n_token == tkID)
				name = ele->n_value;
			else if (ele->n_token == tkSCONST || 
				 ele->n_token == tkICONST ||
          			 ele->n_token == tkNULL   ||
				 ele->n_token == tkFCONST ||
				 ele->n_token == tkDCONST )
				name = ele->n_const;
			else if ( ele->n_token == VALUE )
				name = ele->n_sym->s_name;
			else
				name = ele->n_right->n_sym->s_name;

			if (osldml == OSLSQL)
			{
				/* SQL case.  do "Col AS OSLvar" */

				if ( value != NULL )
				{
					osqtrvfunc( &eval_qry, value );
					ostrfree( value );
					spec_print( ERx(" as ") );
				}

                                if ( ele->n_token == tkSCONST ) {
                                    spec_print("'");
				    spec_print(name);
                                    spec_print("'");
				}
				else 
				    spec_print(name);
			}
			else
			{
				/* QUEL case.  do "OSLvar = Col" */

				spec_print(name);

				if ( value != NULL )
				{
					spec_print( _Equal );
					osqtrvfunc( &eval_qry, value );
					ostrfree( value );
				}
			}
		}
	}
	if (comma)
		spec_print(ERx(", "));
}

/*
** Name:    spec_tlist() -	Generate Query Specifications
**					for DBMS Sub-select Target List Element.
** Description:
**	Generates query specifications to output a sub-select target list
**	element to the DBMS.  (This target list does not specify assignment to
**	form fields or table field columns, but should be output as is to the
**	DBMS.)
**
**	A target list element can have no 'value' when it is just a name, value,
**	or attribute, or an expression in SQL sub-selects.  In the later case,
**	it must be output as an expression using 'osqtrvfunc()'.
**
** History:
**	08/88 (jhw) -- Written (from 'osdbqtle()' in "osdbgen.c".)
*/
/*ARGSUSED*/
static VOID
spec_tlist ( ele, value, dim, comma )
OSNODE	*ele;
OSNODE	*value;
OSNODE	*dim;
bool	comma;
{
	if ( value == NULL && (ele->n_token != VALUE && ele->n_token != tkSCONST
			&& ele->n_token != ATTR) )
	{
		osqtrvfunc( &eval_qry, ele );
		ostrfree( ele );
	}
	else
	{
		spec_eval(ele);
		if ( value != NULL )
		{
			spec_print( _Equal );
			osqtrvfunc( &eval_qry, value );
			ostrfree( value );
		}
	}
	if ( comma )
		spec_print(ERx(", "));
}

/*
** Name:	spec_qry() -	Generates IL For Query Specifications.
**
** Description:
**	Note that this routine is used recursively.  Also, note that only the
**	left-most child query will have a separate WHERE clause for UNION
**	queries.
**
** Inputs:
**	dml	{DB_LANG}  The query language.
**	qry	{OSQRY *}  The query for which specifications are
**				to be generated.
**	where	{bool}  Whether the left-most child query is to have a
**			separate WHERE clause specification.
** History:
**	08/88 (jhw) -- Written.
**	09/89 (jhw) -- Added 'where' parameter to support query objects
**			with separate WHERE clause (so that they can be
**			modified.)
**	08-sep-92 (davel)
**		Use osqtrvfunc() instead of (obsolete) ostrvfromlist() when
**		generating the from clause.
**	23-feb-1994 (mgw) Bug #59678
**		Use eval_names, not eval_qry, when calling osqtrvfunc() for
**		evaluating a from clause.
**      4-Sep-2007 (kibro01) b118760/b119054
**          Ensure that II_ABF_ASSUME_VALUE=Y still allows table names to
**          be unquoted, since there cannot be values there
**	24-Sep-2007 (kibro01) b119170
**	    Ensure that II_ABF_ASSUME_VALUE=Y still allows field names within
**	    group by clauses, where it isn't meaningful to use values.
*/
static VOID
spec_qry ( dml, qry, where )
DB_LANG		dml;
register OSQRY	*qry;
bool		where;
{
	bool prev_under,prev_may;

	if (qry->qn_type == tkQUERY)
	{
		spec_print(dml == DB_QUEL ? ERx("retrieve ") : ERx("select "));
		if ( qry->qn_unique != NULL && *qry->qn_unique != EOS)
		{
			spec_print(qry->qn_unique);
			if (dml == DB_SQL)
				spec_print(ERx(" "));
		}
		/*
		** Generate target list query specification.
		*/
		if ( qry->qn_tlist != NULL )
		{
			if (dml == DB_QUEL)
				spec_print(ERx("("));
			ostltrvfunc(qry->qn_tlist, target_list);
			if (dml == DB_QUEL)
				spec_print(ERx(")"));	/* QUEL only */
		}
		prev_under = under_select;
		prev_may = may_be_field;
		under_select = TRUE;
		may_be_field = TRUE;
		/*
		** Generate FROM list query specification.
		*/
		if (qry->qn_from != NULL)
		{
			spec_print(ERx(" from "));
			must_be_field = TRUE;
			osqtrvfunc(&eval_names, qry->qn_from);	/* 59678 */
			must_be_field = FALSE;
		}
		/*
		** Generate WHERE query specification 
		**	(with embedded predicate references.)
		*/
		if ( where )
			spec_end();
		if ( qry->qn_where != NULL )
		{
			spec_print(ERx(" where "));
			if (qry->qn_where->n_token != VALUE)
			{
				osqtrvfunc( &eval_qry, qry->qn_where );
			}
			else
			{ /* special case for "where :var" */
				spec_flush();
				++spec.s_cnt;
				IGgenStmt( IL_TL2ELM, (IGSID *)NULL, 2,
					   qry->qn_where->n_sym->s_ilref,
					   QS_QRYVAL );
			}
		}
		if ( where )
			spec_end();
		/*
		** Generate GROUP BY query specification.
		*/
		if (qry->qn_groupby != NULL)
		{
			spec_print(ERx(" group by "));
			must_be_field = TRUE;
			osqtrvfunc( &eval_names, qry->qn_groupby );
			must_be_field = FALSE;
		}
		/*
		** Generate HAVING query specification.
		*/
		if (qry->qn_having != NULL)
		{
			spec_print(ERx(" having "));
			osqtrvfunc( &eval_qry, qry->qn_having );
		}
		under_select = prev_under;
		may_be_field = prev_may;
	}
	else if (qry->qn_type == PARENS)
	{
		spec_print(ERx("("));
		spec_qry(dml, qry->qn_left, where);
		spec_print(ERx(")"));
	}
	else if (qry->qn_type == OP)
	{
		spec_qry(dml, qry->qn_left, where);
		spec_print(ERx(" "));
		spec_print(qry->qn_op);
		spec_print(ERx(" "));
		spec_qry(dml, qry->qn_right, FALSE);
	}
}

/*
** Name:	qrysort() -	Generate Sort Specification Element.
**
** Description:
**	Generates or adds to a query specification list for a sort/order
**	list element.
**
** Input:
**	name		{OSNODE *}  Node representing the sort name.
**	dir		{OSNODE *}  Node representing the sort direction.
**	separator	{char *}  DML dependent separator for name/direction.
**	comma		{bool}  Comma
**
** History:
**	06/87 (jhw) -- Written.
*/
static VOID
qrysort (name, dir, separator, comma)
OSNODE	*name;
OSNODE	*dir;
char	*separator;
bool	comma;
{
	spec_eval(name);
	if (dir != NULL)
	{
		spec_print(separator);
		spec_eval(dir);
	}
	if (comma)
		spec_print(ERx(", "));
}

/*
** List Generation Module.
**
** Description:
**	Contains routines used to concatenate strings while traversing a list.
*/

static struct _list	*List = NULL;

/*
** Name:	list_init() -	Initialize for List String Generation.
**
** Description:
**	Reset the list buffer for list string generation.
*/
static VOID
list_init (lp)
struct _list	*lp;
{
	lp->l_buf[0] = EOS;
	lp->l_bp = lp->l_buf;
	List = lp;
}
/*
** Name:	list_print() -	Add String to List Buffer.
**
** Description:
**	Concatenates the input string to the list buffer.
**
** Input:
**	str	{char *}  The string.
*/
static VOID
list_print (str)
char	*str;
{
	register i4	s_len;

	s_len = STlength(str);
	if (List->l_bp - List->l_buf + s_len > sizeof(List->l_buf))
		osuerr(OSBUG, 1, ERx("osqrygen(list_print)"));
	STcopy(str, List->l_bp); List->l_bp += s_len;
}
/*
** Name:	list_get() -	Return List Buffer String.
**
** Description:
**	Returns the list buffer string.
*/
static char *
list_get ()
{
	return List->l_buf;
}

/*
** Name:	qry_pred() -	Generate Query Predicates.
**
** Description:
*/
static VOID	tl_pred();
static VOID	where_pred();

static const
		TRV_FUNC	eval_pred = {
					null,
					null,
					where_pred,
					null,
					null
};

static VOID
qry_pred (qry)
register OSQRY	*qry;
{
	bool prev_under,prev_may;
	if ( qry->qn_type == tkQUERY )
	{
		prev_under = under_select;
		under_select = TRUE;
		prev_may = may_be_field;
		may_be_field = TRUE;
		if ( qry->qn_tlist != NULL )
			ostltrvfunc(qry->qn_tlist, tl_pred);
		if (qry->qn_where != NULL)
			osqtrvfunc( &eval_pred, qry->qn_where );
		if (qry->qn_having != NULL)
			osqtrvfunc( &eval_pred, qry->qn_having );
		under_select = prev_under;
		may_be_field = prev_may;
	}
	else
	{
		/* assert: (qry->qn_type == PARENS || qry->qn_type == OP) */
		qry_pred(qry->qn_left);
		if (qry->qn_type == OP)
			qry_pred(qry->qn_right);
	}
}

/*
** Name:	tl_pred() -	Generate Predicates for Target List Element.
**
*/
/*VARARGS3*/
static VOID
tl_pred (ele, value, dim)
OSNODE	*ele;
OSNODE	*value;
OSNODE	*dim;
{
	if ( ele->n_token != VALUE && ele->n_token != tkID 
	  && ele->n_token != tkSCONST && ele->n_token != ATTR
	  && ele->n_token != DOT && ele->n_token != tkICONST 
	  && ele->n_token != tkDCONST && ele->n_token != tkNULL 
           )
	{
		osuerr(OSBUG, 1, ERx("osqrygen(target_pred)"));
	}
	
	if (value != NULL)
		osqtrvfunc( &eval_pred, value );
}

/*
** Name:	where_pred() -	Generate Query Expression Predicates
**					(Qualification)
** History:
**	07/90 (jhw) - Allow OSTABLE as an error case for predicates. Bug #30784.
**	1/90 (Mike S) Abstracted into ospredgen
*/
static VOID
where_pred ( node )
OSNODE	*node;
{
	ospredgen(node, TRUE);
}

/*
** Name:	osl_assume_value() -	Override assumption of value/field
**
** Description:
**	If II_ABF_ASSUME_VALUE is set to Y or y, anywhere where we examine
**	under_select and may_be_field to determine whether we have a field or
**	a value, this overrides to force it to be a value.
**	This allows such possibilities as "WHERE :a between x and y" to work
**	with a value rather than a field.
**
** History:
**    24-Jul-2007 (kibro01) b118760
**	Created
*/

bool
osl_assume_value()
{
	static bool checked_II_ABF_ASSUME_VALUE = FALSE;
	static bool II_ABF_ASSUME_VALUE = FALSE;

	if (!checked_II_ABF_ASSUME_VALUE)
	{
		char *cp;
		NMgtAt(ERx("II_ABF_ASSUME_VALUE"), &cp);
		if (cp!=NULL && *cp!=EOS && (*cp=='Y' || *cp=='y'))
			II_ABF_ASSUME_VALUE=TRUE;
		checked_II_ABF_ASSUME_VALUE=TRUE;
	}
	return (II_ABF_ASSUME_VALUE);
}

/*
** Query Specification Generator Module.
**
** Description:
**	These routines generate IL code to output query specification elements
**	as part of query initialization.  These specifications are part of the
**	various clauses for a query recognized by the OSL translator.
**
**	Strings are concatenated into larger strings for efficiency, and are
**	flushed when they reach a certain size, or when code for output of a
**	variable or predicate is generated, or on completion of the code
**	generation for the clause.
*/

/*
** Name:	spec_qeval() -	Evaluate Node for Specification.
**
** Description:
**	Evaluates a node that contains a query value for a specification.  That
**	is, it generates a QS_VALUE specification to send the value to the DBMS.
**
** History:
**    02-May-2007 (kibro01) b118229
**	As a result of the fix for bug 116193, qeval rather than eval is run
**	for FROM lists.  This change checks if we are in a FROM list and uses
**	QS_VALUE instead of QS_QRYVAL to avoid quotes being put around table
**	names.
**    20-Jul-2007 (kibro01) b118760
**	Further refinement of fix for b118229 to ensure that we only
**	change previous behaviour if we are in a query - if we are, then
**	parameters are assumed to be column/table names unless on the
**	right hand side of a boolean operator (e.g. WHERE col = :val)
**     4-Sep-2007 (kibro01) b118760/b119054
**      Ensure that II_ABF_ASSUME_VALUE=Y still allows table names to
**      be unquoted, since there cannot be values there
*/
static VOID
spec_qeval (nsym)
OSNODE	*nsym;
{
	if (nsym->n_token == ATTR)
	{
		spec_print(nsym->n_name);
		if (nsym->n_attr != NULL)
		{
			spec_print(ERx("."));
			spec_print(nsym->n_attr);
		}
	}
	else if ( nsym->n_token == DOT && nsym->n_right->n_token == tkID )
	{
		spec_eval(nsym->n_left);
		spec_print(ERx("."));
		spec_eval(nsym->n_right);
	}
	else if (nsym->n_token == tkID)
	{
		spec_print(nsym->n_value);
	}
	else if (tkIS_CONST_MACRO(nsym->n_token))
	{
		spec_print(nsym->n_const);
	}
	else
	{ /* field or column */
		spec_flush();
		/* If this is a query and we know this should be a
		** table or column name, make it QRYVAL.  Otherwise,
		** leave it as being VALUE (kibro01) b118760
		** NOTE spec_qeval defaults to VALUE while spec_eval
		** defaults to QRYVAL
		*/
		IGgenStmt(IL_TL2ELM, (IGSID *)NULL, 2, nsym->n_ilref,
			(must_be_field ||
			(may_be_field && under_select && !osl_assume_value())) ?
				QS_QRYVAL : QS_VALUE);
		++spec.s_cnt;
	}
	ostrfree(nsym);
}

/*
** Name:	spec_eval() -	Evaluate Name Node for Specification.
**
** Description:
**	Evaluates a node that contains a name for a specification.  That is,
**	it generates a QS_QRYVAL specification to send the name down if it
**	is a variable.
** History:
**	20-Jul-2007 (kibro01) b118760
**	    Further refinement of fix for b118229 to ensure that we only
**	    change previous behaviour if we are in a query - if we are, then
**	    parameters are assumed to be column/table names unless on the
**	    right hand side of a boolean operator (e.g. WHERE col = :val)
**      4-Sep-2007 (kibro01) b118760/b119054
**          Ensure that II_ABF_ASSUME_VALUE=Y still allows table names to
**          be unquoted, since there cannot be values there
*/
static VOID
spec_eval ( nsym )
OSNODE	*nsym;
{
	if (nsym->n_token == ATTR)
	{
		spec_print(nsym->n_name);
		if (nsym->n_attr != NULL)
		{
			spec_print(ERx("."));
			spec_print(nsym->n_attr);
		}
	}
	else if (nsym->n_token == tkID)
	{
		spec_print(nsym->n_value);
	}
	else if ( nsym->n_token == DOT && nsym->n_right->n_token == tkID )
	{ /* This is for the case "ID.ID" */
		spec_print(nsym->n_left->n_value);
                spec_print(ERx("."));
		spec_print(nsym->n_right->n_value);
	}
	else if (nsym->n_token == BLANK)
	{ /* This is for the case "ID.ID ID" and "ID ID" */
		spec_eval(nsym->n_left);
                spec_print(ERx(" "));
		spec_eval(nsym->n_right);
	}
	else if (tkIS_CONST_MACRO(nsym->n_token))
	{
		spec_print(nsym->n_const);
	}
	else
	{ /* field or column */
		spec_flush();
		/* If this is a query, and we know we are on the r.h.s. of
		** a comparison operator, use VALUE.  In all other cases,
		** use QRYVAL (making a column name or table name).
		** (kibro01) b118760
		** NOTE spec_qeval defaults to VALUE while spec_eval
		** defaults to QRYVAL
		*/
		IGgenStmt(IL_TL2ELM, (IGSID *)NULL, 2, nsym->n_ilref,
			(must_be_field ||
			(!osl_assume_value() &&
				(!under_select || may_be_field))) ?
				QS_QRYVAL : QS_VALUE);
		++spec.s_cnt;
	}
	ostrfree(nsym);
}

/*
** Name:	spec_print() -	Generate IL Code for Specification String.
**
** Description:
**	This routine concatenates the input string for output as part of a
**	query specification with other strings upto a specified size (of the
**	storage buffer.)  If this size is exceeded, this routine generates IL
**	code to output the other (previous) strings concatenated in the buffer.
**
** Input:
**	str	{char *}  The string to be output.
**
** History:
**	09/86 (jhw) -- Written.
**	04/87 (jhw) -- Modified for query specifications.
*/
static VOID
spec_print (str)
char	*str;
{
	register i4	s_len;

	if (str != NULL && *str != EOS)
	{
		s_len = STlength(str);
		if (spec.s_bp - spec.s_buf + s_len > sizeof(spec.s_buf))
		{
			IGgenStmt( IL_TL2ELM, (IGSID *)NULL, 2,
				   IGsetConst(DB_CHA_TYPE, spec.s_buf),
				   QS_TEXT );
			++spec.s_cnt;
			spec.s_bp = spec.s_buf;
		}
		STcopy(str, spec.s_bp); spec.s_bp += s_len;
	}
}

/*
** Name:	spec_pred() -	Generate IL Code for Specification Generator.
*/
/*ARGSUSED*/
static VOID
spec_pred (node)
OSNODE	*node;
{
	spec_flush();
	IGgenStmt(IL_TL2ELM, (IGSID *)NULL, 2, 0, QS_VALGEN);
	++spec.s_cnt;
}

/*
** Name:	spec_flush() -	Generate IL Code for Output of Specification String.
**
** Description:
**	Generates IL code that outputs the concatenated strings (through
**	'spec_print()') to the runtime query module.  This reinitializes the
**	storage buffer.
**
** History:
**	09/86 (jhw) -- Written.
*/
static VOID
spec_flush ()
{
	if (spec.s_bp > spec.s_buf)
	{
		IGgenStmt( IL_TL2ELM, (IGSID *)NULL, 2,
			   IGsetConst(DB_CHA_TYPE, spec.s_buf), QS_TEXT );
		++spec.s_cnt;
	}
	spec.s_bp = spec.s_buf;
}

/*
** Name:	spec_end() -	Flush Specifications and End List.
**
** Description:
**	Flushes the specifications that have been generated so far and then
**	generates an IL_ENDLIST for the specification list.  Also increments
**	the specification count by one for the NULL terminating specification.
**
** History:
**	09/89 (jhw) -- Written.
*/
static VOID
spec_end ()
{
	spec_flush();
	IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
	++spec.s_cnt;
}

/*
** Name:	spec_count() -	Return Count of Query Specifications Generated.
**
** Description:
**	Returns the count of the number of query specifications generated to
**	represent a query.
**
** Returns:
**	{nat}  The number of query specifications generated.
**
** Side Effects:
**	Resets the count.
**
** History:
**	02/88 (jhw) -- Written.
*/
static i4
spec_count()
{
	register i4	save = spec.s_cnt;

	spec.s_cnt = 0;
	return save;
}
