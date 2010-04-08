/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<er.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<fdesc.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<oslconf.h>
#include	<ilops.h>
#include	<iltypes.h>

/**
** Name:	osrefform.c -	OSL Compiler Form Variable Access Module.
**
** Description:
**	Contains routines that process references to fields or columns of
**	a form.  These routines generate code that access the fields or
**	columns through "getform"s, "getrow"s, "putform"s and "putrow"s.
**	Defines:
**	
**	osval()		generate access IL for variable.
**	iiosRefGen()	generate access IL for reference.
**	os_lhs()	generate access IL for LHS variable.
**	osvardisplay()	display field value on form.
**	osrowops()	generate a row-access instruction.
**	osunload()	generate UNLOADTABLE instructions.
**	osexitunld()	generate instructions to exit from an UNLOADTABLE.
**	osgenunldcols()	unload values for table field column.
**	osall()		handle a .ALL expression.
**
** History:
**	Revision 5.1  86/10/17  16:40:50  wong
**	Initial revision (some routines modified from compiler version
**	others added for translator beginning 08/08.)
**
**	Revision 6.3/03/00  89/09  billc
**	Added routines for complex object support.
**	11/14/90 (emerson)
**		Check for global variables (OSGLOB)
**		and record attributes (OSRATTR)
**		as well as local variables (OSVAR).  (Bug 34415).
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures.
**
**	Revision 6.4/02
**	10/13/91 (emerson)
**		Part of fix for bug 39492 in iiosRefGen.
**		Removed unused routine iiosAtSym.
**	11/07/91 (emerson)
**		Modified _gendot; created osexitunld
**		(for bugs 39581, 41013, and 41014: array performance).
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Small change for bug 34846 in _gendot.
**
**	Revision 6.5
**	26-jun-92 (davel)
**		Correct calls to ostmpalloc(), which now takes an additional
**		precision arguemnt.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    Need oslconf.h to satisfy gcc 4.3.
*/

/*
** These static variables allows us to identify the object being unloaded.
*/
static OSSYM *UnldTmp = NULL;
static ILREF UnldRef = 0;

static STATUS	_gendot();

OSNODE	*iiosRefGen();

/*{
** Name:	osval() -	Get Value for Field.
**
** Description:
**	Generate IL code to access the value in a field.  This
**	sets the reference bit in the symbol table structure to show that
**	a reference is active.
**
** Input:
**	refn	{OSNODE *}  The reference node for the field (contains
**				 a reference to the symbol table entry.)
**
** Output:
**	{OSNODE*}	potentially-doctored node.
**
** History:
**	09/89 (billc) -- Written.
*/

OSNODE *
osval (refn)
OSNODE	*refn;
{
	return iiosRefGen(refn, FALSE);
}

/*{
** Name:	iiosRefGen() -	Generate Reference for Field.
**
** Description:
**	Generate IL code to access the value in a field.  This sets the
**	reference bit in the symbol table structure to show that a reference
**	is active.
**
** Input:
**	refn	{OSNODE *}  The reference node for the field (contains
**				 a reference to the symbol table entry.)
**	is_lhs	{bool}		Is this the LHS of an assignment?
**
** Output:
**	{OSNODE*}	potentially-doctored node.
**
** History:
**	09/89 (billc) -- Written.
**	10/13/91 (emerson)
**		Check for the unloadtable object at the top of the
**		VALUE and DOT cases, so that we'll recognize it
**		even in those cases where it isn't being dotted with something
**		(e.g. when unloadtable_object_array[] is specified
**		as a parameter or the as the target of a SELECT).
**		In the case of DOT, it was actually being recognized,
**		but lhstemp was being made to point to a spurious
**		and superfluous placeholder temp.
**		Part of fix for bug 39492.
**	11/07/91 (emerson)
**		Change the length in the ostmpalloc for an ARRAYREF temp
**		from 1 to 0, because the result of an ARRAYREF is a record,
**		not an array.  [This probably would have fixed bug 37109.
**		Since we created an alternate fix in abfrt!abrtclas.c,
**		the fix above probably isn't necessary, but doesn't hurt.]
*/

OSNODE *
iiosRefGen ( refn, is_lhs )
register OSNODE	*refn;
bool 		is_lhs;
{
	register OSSYM	*sym;

	switch ( refn->n_token )
	{
	  case VALUE:
		/*
		** Check for the unloadtable object.  If this is it, then
		** we've already generated code that creates a temp for it,
		** and we should reference it instead.  
		*/
		if (refn->n_flags & N_UNLOAD)
		{
			refn->n_sym = UnldTmp;
			refn->n_ilref = UnldRef;
			break;
		}

		/* generate GETFORM/GETROW instructions if this is an RHS
		** reference.
		*/
		if (is_lhs)
		{
			break;
		}

		if ( refn->n_tfref != NULL 
		  && refn->n_tfref->s_parent->s_kind == OSTABLE )
		{ /* tablefield column. */
			sym = refn->n_tfref;
		}
		else
		{
			sym = refn->n_sym;
		}

		if ( sym->s_kind == OSTEMP || sym->s_kind == OSFORM )
		{
			break;
		}

		if ( sym->s_parent != NULL && sym->s_parent->s_kind == OSTABLE )
		{ /* tablefield */

			/*
			** Generate access only if reference is not to the
			** current row of the table field or the table field
			** is not being unloaded.
			*/
			if (refn->n_sexpr != NULL 
			  || (sym->s_parent->s_ref & OS_TFUNLOAD) == 0)
			{
				refn->n_sref = 0;
				if (refn->n_sexpr != NULL)
				{ /* reference to specific row */
					refn->n_sref = osvalref(refn->n_sexpr);
					refn->n_sexpr = NULL;
				}
				IGgenStmt(IL_GETROW, (IGSID *)NULL, 2, 
					IGsetConst(DB_CHA_TYPE, 
							sym->s_parent->s_name
						), 
					refn->n_sref
				);
				IGgenStmt(IL_TL2ELM, (IGSID *)NULL, 2, 
					refn->n_sym->s_ilref,
					IGsetConst(DB_CHA_TYPE, sym->s_name)
				);
				IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
			}
		}
		else
		{ /* vanilla field */
			if (sym->s_kind == OSFIELD && !(sym->s_ref & OS_FLDREF))
			{
				IGgenStmt(IL_GETFORM, (IGSID *)NULL, 2, 
					IGsetConst(DB_CHA_TYPE, sym->s_name), 
					sym->s_ilref
				);
			}
			sym->s_parent->s_ref |= OS_FLDREF;
			sym->s_ref |= OS_FLDREF;
		}
		break;

	  case ARRAYREF:
	  case DOT:
	  {
		OSSYM		*sym;
		i4		len;

		/*
		** Check for the unloadtable object.  If this is it, then
		** we've already generated code that creates a temp for it,
		** and we should reference it instead.  
		*/
		if (refn->n_flags & N_UNLOAD)
		{
			refn->n_lhstemp = UnldTmp;
			refn->n_ilref = UnldRef;
			break;
		}

		/*
		** Generate a temp.  If this is an ARRAYREF, it had better be
		** of type DB_DMY_TYPE (we don't allow arrays of simple types),
		** and the appropriate temp length is 0.  This indicates
		** that temp is for a record, not an array (we don't allow
		** arrays of arrays, either).  If this is a dot, the
		** appropriate temp length is given by the symbol table
		** entry for the attribute (the right-hand node), whether
		** the attribute is simple or complex.
		*/
		len = 0;
		if (refn->n_token == DOT)
		{
			len = refn->n_right->n_sym->s_length;
		}
		/* davel: the refn's type/length/prec should be right; check */
		sym = ostmpalloc(refn->n_type, refn->n_length, refn->n_prec);

		/* We'll stash our temp here. */
		refn->n_lhstemp = sym;

		/* go travel the DOT tree. */
		if (_gendot(refn, sym, is_lhs) != OK)
		{
			refn->n_flags |= N_ERROR;
		}
		break;
	  }
  	  default:
		{
			char buf[64];

			_VOID_ STprintf(buf, ERx("iiosRefGen(node %d)"), 
					refn->n_token);
			osuerr(OSBUG, 1, buf);
		}
		break;
	}
	return refn;
}

/*{
** Name:    _gendot() -	Generate DOT and ARRAYREF instructions.
**
** Input:
**	refn	- the top node of the reference (see comments in ossymnd.c for
**			DOT and ARRAYREF nodes.)
**	tempsym - the symbol we'll dereference into.
**	islhs	- is this an LHS variable?
**
** History:
**	09/89 (billc) -- Written.
**	11/07/91 (emerson)
**		Changes for bugs 39581, 41013, and 41014 (array performance):
**		If we're doing a series of ARRAYREFs and DOTs into a simple
**		temp, allocate a complex (object) temp for the intermediate
**		steps instead of using the simple temp throughout.
**		Use new IL_ARRAYREF instead of IL_ARRAY.
**		Set new modifier bits in IL_ARRAYREF and IL_DOT.
**	09/20/92 (emerson)
**		Changes for bug 34846: Use iiIGnsaNextStmtAddr instead of
**		the now-unsupported iiIGlsaLastStmtAddr.
*/

static STATUS
_gendot (refn, tempsym, islhs)
register OSNODE	*refn;
OSSYM 		*tempsym;
bool 		islhs;
{
	OSNODE	*left = refn->n_left;
	OSSYM	*lhstemp = NULL;

	/* 
	** mark DOT and ARRAYREF nodes (but NOT leaf-nodes) with the ilref
	** of the temp symbol -- the dbv that we'll dereference through.
	*/
	refn->n_ilref = tempsym->s_ilref;

	if (left->n_token == ARRAYREF)
	{
		OSSYM *tsym = osnodesym(left);

		/* 
		** HACK ATTACK:
		** Odd that we do this check here, but we have to allow x.y[i],
		** where x.y is a string variable, for table operations (like
		** INSERTROW) where the name of the table is in the variable
		** x.y.  That means we might have an x.y[i].z, (where y is a
		** string, not an array), which we have to catch here.
		*/
		if ( !(tsym->s_flags & FDF_ARRAY))
		{
			oscerr(OSNOTARRAY, 1, tsym->s_name);
			return FAIL;
		}
	}

	/*
	** Check for the unloadtable object on the left.
	** If present, we've already generated code that creates a temp for it,
	** and we can just keep referencing it.  
	*/
	if (left->n_flags & N_UNLOAD)
	{
		left->n_ilref = UnldRef;
	}
	/*
	** If there's a non-leaf node (ARRAYREF or DOT) on the left,
	** we must call _gendot recursively on it.
	**
	** We'll reuse the same temp for a series of ARRAYREFs and DOTs
	** into objects (DB_DMY_TYPE), but we no longer (as of 11/07/91)
	** will use this same temp for a final DOT into a simple attribute
	** temp.  [We'll need the ILREF of the last object we DOT'ed into
	** so that we can later generate a RELEASEOBJ for it].
	**
	** Therefore, if we're at the DOT at the top of a DOT subtree,
	** and it's for a simple attribute, and there's an ARRAYREF or DOT
	** on the left, then we'll have to allocate a DB_DMY_TYPE temp for it.
	*/
	else if (left->n_token == ARRAYREF || left->n_token == DOT)
	{
		lhstemp = tempsym;
		if (refn->n_type != DB_DMY_TYPE)
		{
			left->n_lhstemp = lhstemp = ostmpalloc(DB_DMY_TYPE, 0,
								(i2)0);
		}

		if (_gendot(left, lhstemp, islhs) != OK)
		{
			return FAIL;
		}
	}

	if (refn->n_token == DOT)	/* Generate an IL_DOT */
	{
		ILREF 	nameref;
		char	buf[FE_MAXNAME + 1];

		/* 
		** The right-hand node has the name of the record member. 
		*/
		_VOID_ STlcopy( refn->n_right->n_sym->s_name, buf, 
					sizeof(buf) - 1 );
		CVlower(buf);
		nameref = IGsetConst(DB_CHA_TYPE, buf);

		/*
		** If we're DOTing or ARRAYREFing into a complex object,
		** then save the address of the IL_DOT or IL_ARRAYREF
		** in the temp's symbol-table entry.  See the comments
		** above similar instructions below under the IL_ARRAYREF case.
		*/
		if (refn->n_type == DB_DMY_TYPE)
		{
			tempsym->s_tmpil = (PTR)iiIGnsaNextStmtAddr();
		}

		IGgenStmt( IL_DOT, (IGSID*)NULL, 3,
			   left->n_ilref, nameref, refn->n_ilref );
	}
	else				/* Generate an IL_ARRAYREF */
	{
		ILREF	index;
		IL	modifiers = 0;

		/*
		** Generate IL to compute the array index.
		*/
		index = osvalref(refn->n_right);
		refn->n_right = NULL;		/* osvalref freed this. */

		/*
		** If the LHS node is a temp, we need to prevent it from
		** being freed by the IL that computed the array index
		** (if, for example, the array index is an expression
		** containing a call to a procedure that removes a row
		** that contains the array pointed to by the LHS temp).
		** We protect the LHS temp by retroactively setting
		** the ILM_HOLD_TARG modifier bit in the IL_DOT that set
		** this temp; we release the hold by setting the ILM_RLS_SRC
		** modifier bit in the IL_ARRAYREF we're about to generate.
		*/
		if (lhstemp != NULL)
		{
			*((IL *)lhstemp->s_tmpil) |= ILM_HOLD_TARG;
			modifiers |= ILM_RLS_SRC;
		}

		/*
		** If we're DOTing or ARRAYREFing into a complex object,
		** then save the address of the IL_DOT or IL_ARRAYREF
		** in the temp's symbol-table entry.  This enables the
		** instruction's ILM_HOLD_TARG to be subsequently set
		** if necessary.  That can happen in the logic above that
		** generates a subsequent IL_ARRAYREF, or it can happen
		** in those cases where ostmpall.c is generating IL to free
		** the temps it allocated and it was deemed necessary to hold
		** complex object temps across a temp block.
		*/
		if (refn->n_type == DB_DMY_TYPE)
		{
			tempsym->s_tmpil = (PTR)iiIGnsaNextStmtAddr();
		}

		/*
		** Generate the IL_ARRAYREF.  Set the ILM_LHS modifier bit
		** if this is a l.h.s. array reference.
		*/
		if (islhs)
		{
			modifiers |= ILM_LHS;
		}
		IGgenStmt( IL_ARRAYREF | modifiers, (IGSID*)NULL, 3,
			   left->n_ilref, index, refn->n_ilref );
	}
	return OK;
}

/*{
** Name:    os_lhs() -	generate access code for LHS of an assignment.
**
** Description:
**	Generate IL code to access the value in a field.  This
**	sets the reference bit in the symbol table structure to show that
**	a reference is active.
**
** Input:
**	refn	{OSNODE *}  The reference node for the field (contains
**				 a reference to the symbol table entry.)
**
** Output:
**	{OSNODE*}	potentially-doctored node.
**
** History:
**	09/89 (billc) -- Written.
*/

OSNODE *
os_lhs (refn)
OSNODE	*refn;
{
    if (refn->n_flags & N_READONLY)
    {
	if (!osiserr(refn))
		oscerr(OSASNOVAR, 1, osnodename(refn));
	refn->n_flags |= N_ERROR;
	return refn;
    }
    return iiosRefGen(refn, TRUE);
}

/*{
** Name:    osvardisplay() -	Display Field Value on Form.
**
** Description:
**	Produce code to do a putform or putrow for a field or tablefield
**	table field column.
**
** Parameters:
**	node	{OSNODE *}  Node containing field or table field reference.
*/
osvardisplay (node)
register OSNODE	*node;
{
    if (node->n_token == VALUE)
    {
	if (node->n_tfref == NULL)
	{
	    register OSSYM	*fld = node->n_sym;
    
	    if (fld->s_kind == OSFIELD)
		IGgenStmt(IL_PUTFORM, (IGSID *)NULL,
			2, IGsetConst(DB_CHA_TYPE, fld->s_name), fld->s_ilref
		);
	}
	else
	{
	    register OSSYM	*col = node->n_tfref;

	    if (node->n_sref == 0 && node->n_sexpr != NULL)
	    { 
		/* reference to specific row */
		node->n_sref = osvalref(node->n_sexpr);
		node->n_sexpr = NULL;
	    }
	    IGgenStmt(IL_PUTROW, (IGSID *)NULL,
		2, IGsetConst(DB_CHA_TYPE, col->s_parent->s_name), node->n_sref
	    );
	    IGgenStmt(IL_TL2ELM, (IGSID *)NULL,
		2, IGsetConst(DB_CHA_TYPE, col->s_name), node->n_sym->s_ilref
	    );
	    IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);

	    node->n_sref = 0;
	}
    }
}

/*{
** Name:    osrowops() -	generate table operation.
**
** Description:
**	generate table operations INSERTROW, etc.
**
** Input:
**	form	{OSSYM*}	Symtab parent -- currently unused.
**	sid	{IGSID *}	The SID, where needed.
**	op	{ILOP}		The IL operation.
**	obj	{OSNODE *}  	The reference node for the field.
**
** History:
**	10/89 (billc) -- Written.
*/

static struct _opconv 
	{
		ILOP	tblop;
		ILOP 	arrop;
		char	*opname;
	} optab[] =
	{
		{ IL_CLRROW, IL_ARRCLRROW, 	ERx("CLEARROW") },
		{ IL_DELROW, IL_ARRDELROW, 	ERx("DELETEROW") },
		{ IL_INSROW, IL_ARRINSROW, 	ERx("INSERTROW") },
		{ IL_VALROW, 0, 		ERx("VALIDROW") },
		{ IL_LOADTABLE, 0, 		ERx("LOADTABLE") },
		{ 0, 		0,		ERx("<unknown>") }
	};

VOID
osrowops (form, sid, op, obj)
OSSYM *form;
IGSID *sid;
ILOP op;
OSNODE	*obj;
{
	OSSYM	*sym;
	ILREF	ilref;
	ILREF	ind_ref = 0;
	OSNODE	*tmp = NULL;

	if (osiserr(obj))
		return;

	sym = osnodesym(obj);

#ifdef OSL_TRACE
	SIprintf("osrowops: tok %d, name=\"%s\", s_kind=%d, s_flags=0x%x\n\r",
			obj->n_token, sym->s_name, sym->s_kind, sym->s_flags
		);
#endif /* OSL_TRACE */

	/* 
	** Is this an array object?  If so, transmogrify the operator. 
	** (May seem silly, but it avoids some extra work in the grammar
	** actions.)
	*/
	if (sym->s_flags & FDF_ARRAY)
	{
		i4 i;

		/* array op, so xlate to array IL instruction */
		for (i = 0; optab[i].tblop != 0 && optab[i].tblop != op; i++)
			continue;
		if (optab[i].arrop == 0)
		{
			/* This operation not supported on arrays. */
			oscerr(OSNOTTFLD, 1, optab[i].opname);
			obj->n_flags |= N_ERROR;
			return;
		}
		op = optab[i].arrop;
	}

	switch (obj->n_token)
	{
	  case ARRAYREF:
		/* 'right' has the array index, 'left' has the real variable. */
		if (obj->n_right != NULL)
			ind_ref = osvalref(obj->n_right);
		tmp = obj->n_left;

		obj->n_left = obj->n_right = NULL;
		ostrfree(obj);

		_VOID_ osval(tmp);
		ilref = osvalref(tmp);
		break;

	  case VALUE:
		if (sym->s_kind == OSTABLE)
		{
			/* special case -- this is a tablefield name */
			ilref = IGsetConst(DB_CHA_TYPE, sym->s_name);

			if ((tmp = obj->n_sexpr) != NULL)
			{
				obj->n_sexpr = NULL;
				ind_ref = osvalref(tmp);
			}
			ostrfree(obj);
			break;
		}
		else if ( !( sym->s_flags & FDF_ARRAY ) &&
			  ( sym->s_kind == OSVAR ||
			    sym->s_kind == OSGLOB ||
			    sym->s_kind == OSRATTR ) )
		{
			/* 
			** String constant with name of the tablefield.
			** This was already checked for validity (ie: must
			** be a string var) in the table_name rule in the
			** main grammar.
			*/
			_VOID_ osval(obj);
			ilref = osvalref(obj);
			break;
		}

		/* else fall through to... */

	  default:
		if (sym->s_flags & FDF_ARRAY)
		{
			/* an array, but no subscript. */
			oscerr(OSNOROW, 1, osnodename(obj));
			obj->n_flags |= N_ERROR;
		}
		else
		{
			/* not an array! */
			oscerr(OSNOTARRAY, 1, osnodename(obj));
			obj->n_flags |= N_ERROR;
		}
		return;
	}

	IGgenStmt(op, sid, 2, ilref, ind_ref); 
}

/*{
** Name:    osunload() -	generate unloadtable operation.
**
** Description:
**	generate UNLOADTABLE operations.
**
** Input:
**	form	{OSSYM*}	Symtab parent -- currently unused.
**	sid	{IGSID *}	The SID, where needed.
**	sym	{OSSYM*}	symbol of the object.
**	obj	{OSNODE *}  	The reference node for the field.
**
** History:
**	10/89 (billc) -- Written.
**	11/07/91 (emerson)
**		Change the length in the ostmpalloc for the unload temp
**		from 1 to 0, because it's a record, not an array.
**		[It apparently didn't hurt anything, but it was wrong].
**		Also clear UnldTmp and UnldTmp at entry (so osexitunld can
**		later tell whether we're unloading a tablefield or an array).
*/

OSSYM *
osunload (form, sid, obj)
OSSYM *form;
IGSID *sid;
OSNODE	*obj;
{
	ILREF ilref;
	OSSYM *sym;

	UnldTmp = NULL;
	UnldRef = 0;

	sym = osnodesym(obj);

#ifdef OSL_TRACE
	SIprintf("osunload: tok %d,name=%s,s_kind=%d,s_flags=0x%x,ilref=%d\n\r",
		obj->n_token, sym->s_name, sym->s_kind, 
		sym->s_flags, sym->s_ilref
		);
#endif /* OSL_TRACE */

	switch (obj->n_token)
	{
	  case VALUE:
		if (sym->s_kind == OSTABLE)
		{
			/* special case -- this is a tablefield name */
			ilref = IGsetConst(DB_CHA_TYPE, sym->s_name);

			if (obj->n_sexpr != NULL)
			{
				oscerr(OSNOTARRAY, 1, sym->s_name);
				obj->n_flags |= N_ERROR;
				return NULL;
			}
			ostrfree(obj);
			break;
		}

		/* else fall through to... */

	  case DOT:
		if (sym->s_flags & FDF_ARRAY)
		{
			/* We have the ilref of the unload object.
			** Now make a temp for the unloading.
			*/
			UnldTmp = ostmpalloc(DB_DMY_TYPE, 0, (i2)0);
			UnldRef = UnldTmp->s_ilref;

			_VOID_ osval(obj);
			ilref = osvalref(obj);

			break;
		}

		/* else fall through to... */

	  default:
		/* not an array! */
		oscerr(OSNOTARRAY, 1, osnodename(obj));
		obj->n_flags |= N_ERROR;
		return NULL;
	}

	if (sym->s_flags & FDF_ARRAY)
	{
		/* Generate unload stmt for array. */
		IGgenStmt(IL_ARR1UNLD, sid, 2, ilref, UnldRef); 
		IGpushSID(IGinitSID());		/* unload SID */
		IGsetSID(IGtopSID());
		IGgenStmt(IL_ARR2UNLD, sid, 1, UnldRef);
	}
	else
	{
		/* Generate unload stmt for tablefield. */
		IGgenStmt(IL_UNLD1, sid, 1, ilref); 
		IGpushSID(IGinitSID());		/* unload SID */
		IGsetSID(IGtopSID());
		IGgenStmt(IL_UNLD2, sid, 0);
	}

	/* mark this as our unload-object. */
	osmarkunld(form, sym);

	return sym;
}

/*{
** Name:    osexitunld() -	generate instructions to exit from UNLOADTABLE.
**
** Description:
**	generate instructions to exit from the current UNLOADTABLE loop.
**
** History:
**	11/07/91 (emerson)
**		Created for bugs 39581, 41013, and 41014 (array performance):
**		osblock.c now calls this function instead of issueing
**		an IL_ENDUNLD (which wasn't appropriate for an UNLOADTABLE
**		of an array, but apparently did no serious damage).
*/

VOID
osexitunld ()
{
	if (UnldTmp != NULL)
	{
		IGgenStmt(IL_ARRENDUNLD, (IGSID *)NULL, 1, UnldRef);
	}
	else
	{
		IGgenStmt(IL_ENDUNLD, (IGSID *)NULL, 0);
	}
}

/*{
** Name:    osgenunldcols() -	Unload Values for Table Field Columns.
**
** Description:
**	Generate a list of IL instructions used to unload the columns of a
**	table field as part of an UNLOADTABLE statement.
**
** Input:
**	sym	{OSSYM *}  The table field or array node.
*/

VOID
osgenunldcols (sym)
OSSYM	*sym;
{
	OSSYM	*col;

	if (sym->s_kind == OSTABLE)
	{
		for (col = sym->s_columns; col != NULL; col = col->s_sibling)
		{
			IGgenStmt(IL_TL2ELM, (IGSID *)NULL,
					2, col->s_ilref, 
					IGsetConst(DB_CHA_TYPE, col->s_name)
			);
		}
	}

	/* no action on arrays. */
}

/*{
** Name:    _formall() -	Get Values for All Fields on Form.
**
** Description:
**	This routine generates IL code to access the values in each regular
**	field on a form (but not table fields or hidden fields.)  At the same
**	time, it also builds a node list of references for the field values and
**	returns this list.
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the form.
**
** Returns:
**    {OSNODE *}	A list of field symbols for the values.
**
** History:
**	09/86 (jhw) -- Written.
**	03/87 (jhw) -- Modified to build and return node list.
*/
static OSNODE *
_formall (form)
OSSYM	*form;
{
    register OSNODE	*list = NULL;

    if (form->s_kind == OSFORM && form->s_fields != NULL)
    { /* generate access for all fields on form */
	register OSSYM	*fld;

	form->s_ref |= OS_FLDREF;
	for (fld = form->s_fields ; fld != NULL ; fld = fld->s_sibling)
	{
	    U_ARG	symp;
	    U_ARG	nlist;

	    if (fld->s_kind == OSFIELD && (fld->s_ref & OS_FLDREF) == 0)
	    {
		fld->s_ref |= OS_FLDREF;
		IGgenStmt(IL_GETFORM, (IGSID *)NULL,
		    2, IGsetConst(DB_CHA_TYPE, fld->s_name), fld->s_ilref
		);
	    }
	    symp.u_symp = fld;
	    symp.u_nodep = osmknode(VALUE, &symp, (U_ARG *)NULL, (U_ARG *)NULL);
	    nlist.u_nodep = list;
	    list = osmknode(NLIST, &symp, &nlist, (U_ARG *)NULL);
	}
    }
    return list;
}

/*{
** Name:	_tabfall() -	Get Values for Table Field Row.
**
** Description:
**	Builds a node list of references for the column/row values that will be
**	returned to the caller.  At the same time, it generates IL code to
**	access all the values in the columns for a table field row (but not
**	hidden columns) if a specific row is referenced or the table field
**	is being unloaded.  (Unreferenced table field access refers to the
**	row being unloaded when in an UNLOADTABLE loop.)
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the form.
**	table	{char *}  The name of the table field.
**	expr	{OSNODE *}  The table field row reference expression.
**
** Returns:
**	{OSNODE *}	A list of table field symbols for the values.
**
** History:
**	09/86 (jhw) -- Written.
**	04/88 (jhw) -- Modified to not generate IL code to fetch values when
**			row not referenced in unload loop (which was a Bug!)
*/

static OSNODE *
_tabfall (form, table, expr)
OSSYM	*form;
char	*table;
OSNODE	*expr;
{
	register OSNODE	*list = NULL;
	register OSSYM	*tfld;

	if ( (tfld = ossymlook(table, form)) == NULL || tfld->s_kind != OSTABLE )
		oscerr( OSNOTBLFLD, 2, table, form->s_name );
	else
	{ /* a table field ... */
		register OSSYM	*col;
		bool	fetch;

		fetch = expr != NULL || (tfld->s_ref & OS_TFUNLOAD) == 0;

		ostmpbeg();
		if ( fetch )
		{ /* will be fetching row ... */
			IGgenStmt( IL_GETROW, (IGSID *)NULL,
				2, IGsetConst(DB_CHA_TYPE, tfld->s_name),
					(expr == NULL ? 0 : osvalref(expr))
			);
		}
		ostmpend();
		for (col = tfld->s_columns ; col != NULL ; col = col->s_sibling)
		{
			/* ... all visible columns */
			if ( col->s_kind == OSCOLUMN )
			{
				register OSNODE	*symn;
				U_ARG	symp;
				U_ARG	nlist;

				symn = ostvalnode(tfld, tfld->s_name, 
						(OSNODE*)NULL, col->s_name);
				if ( fetch )
				{ /* fetch column value ... */
					IGgenStmt(IL_TL2ELM, (IGSID *)NULL,
					   2, symn->n_sym->s_ilref,
						IGsetConst(DB_CHA_TYPE, col->s_name)
					);
				}
				symp.u_nodep = symn;
				nlist.u_nodep = list;
				list = osmknode(NLIST, &symp, &nlist, (U_ARG *)NULL);
			}
		} /* end for */
		IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
	}
	return list;
}

/*{
** Name:	osall() -	handle a .ALL expression
**
** Description:
**	handle a .ALL expression for form or tablefield.
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the form.
**	table	{OSNODE *} The name of the table field.
**
** Returns:
**	{OSNODE *}	A list of symbols for the values.
**
** History:
**	09/89 (billc) -- Written.
*/

OSNODE *
osall (form, table)
OSSYM	*form;
OSNODE	*table;
{
	OSNODE *ret = NULL;

	if (table->n_token == VALUE)
	{
		if (STequal(form->s_name, table->n_sym->s_name))
		{
			ret = _formall(form);
		}
		else
		{
			ret = _tabfall(form, table->n_sym->s_name, 
						table->n_sexpr);
		}
	}
	else
	{
		char buf[64];

		_VOID_ STprintf(buf, ERx("osall(node %d)"), table->n_token);
		osuerr(OSBUG, 1, buf);
	}

	return ret;
}
