/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<ade.h>
#include	<fe.h>
#include	<fdesc.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osglobs.h>
#include	<oslconf.h>
#include	<iltypes.h>
#include	<ilops.h>

/**
** Name:	ossymnd.c -	OSL Parser Symbol Table Node Module.
**
** Description:
**	Contains routines that create nodes for variable references.  That is,
**	nodes that contain DOT symbol table entries.  Defines:
**
**	osvalnode()	create node for field value.
**	ostvalnode()	create node for table field value.
**	osdotnode()	create/doctor node for '.' dereference.
**	osqrydot()	create/doctor node for '.' dereference in query target.
**	osarraynode()	create/doctor node for subscript reference.
**	osnodename()	return name for a node.
**	osnodesym()	return OSSYM * for a node.
**
** History:
**	Revision 5.1  86/10/17  16:16:25  wong
**	Initial revision. (07/23)
**
**	Revision 6.3/03/00  89/09  billc
**	Extensively modified for complex object support.
**	11/15/90 (esd)
**		Fix to osqrydot.
**	
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures (osvalnode).
**
**	Revision 6.4/02
**	10/15/91 (emerson)
**		Replace error OSNOINDEX by OSNOROW in osdotnode.
**		Fix for bug 39492 in osarraynode and osqrydot.
**	12/19/91 (emerson)
**		Move forward declaration of osunldobj from inside osdotnode
**		to top of source file, because my previous change added
**		a reference to osunldobj in osarraynode.
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Major revisions for bug 34846 in osqrydot.
**		Small set of changes for bugs 34846 and 39582:
**		Wherever the N_UNLOAD bit was being set, also set
**		the new N_ROW bit (since the unloadtable object
**		is treated as a row).  Small refinement in osvalnode.
**		Fix for bug 44210 in ostvalnode.
**
**	Revision 6.5
**	29-jul-92 (davel)
**		add precision argument to osdatalloc().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    Need oslconf.h to satisfy gcc 4.3.
*/

FUNC_EXTERN	OSSYM 		*osnodesym();
FUNC_EXTERN	OSSYM		*osunldobj();

static		OSNODE	*mkdot();

/*{
** Name:	osvalnode() -	Create Node for Field Value.
**
** Description:
**	Creates a node for a simple field.
**
** Input:
**	form	The symbol table entry for the form.
**	name	The name of the field.
**
** History:
**	07/86 (jhw) -- Written.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Check for invalid reference to a local procedure.
**	09/20/92 (emerson)
**		Removed redundant "ret->n_ilref = sym->s_ilref" at bottom;
**		this is done by osmknode(VALUE, ...).
*/

OSNODE *
osvalnode ( form, name )
OSSYM	*form;
char	*name;
{
	register OSSYM	*sym;
	OSNODE		*ret;
	U_ARG		symp;

#ifdef OSL_TRACE
	SIprintf("osvalnode: formname=|%s|; fieldname=|%s|\n",
			form->s_name, name
		);
#endif /* OSL_TRACE */

	symp.u_symp = sym = osfld(form, name, OSOBJUNDEF);

	ret = osmknode(VALUE, &symp, (U_ARG *)NULL, (U_ARG *)NULL);

	if (sym->s_kind != OSVAR 
	 	&& sym->s_kind != OSRATTR  
	 	&& sym->s_kind != OSFIELD  
		&& sym->s_kind != OSGLOB )  
	{
		if (sym->s_kind == OSFORM)
		{ /* invalid reference to local procedure */
			oscerr(E_OS0068_BadProcRef, 1, sym->s_name);
		}
		else
		{ /* this can't be an LHS of an expr.  hence readonly. */ 
			ret->n_flags |= N_READONLY;
		}
	}

	if (sym->s_flags & (FDF_ARRAY | FDF_RECORD | FDF_READONLY))
	{ /* can't assign to complex objects. */
		ret->n_flags |= N_READONLY;
	}
			
	if (sym->s_kind == OSTABLE 
		|| sym->s_kind == OSCOLUMN  
	  	|| sym->s_kind == OSFIELD )  
	{ /* display object.  */ 
		ret->n_flags |= N_VISIBLE;
	}
		
#ifdef OSL_TRACE
	SIprintf("osvalnode: VALUE node returned: token/type/len/prec=%d %d %d %d\n",
			ret->n_token, ret->n_type, ret->n_length, ret->n_prec
		);
#endif /* OSL_TRACE */
	return ret;
}

/*{
** Name:	ostvalnode() -	Create Node for Table Field Value.
**
** Description:
**	Creates a node for a table field value specified by row and column.
**
** Input:
**	form	The symbol table entry for the form.
**	table	The name of the table field.
**	expr	Expression tree denoting row of table field.
**	column	The name of the table field column.
**
** History:
**	07/86 (jhw) -- Written.
**	09/20/92 (emerson)
**		Fix bug 44210: When allocating a temporary for the second
**		or subsequent reference within a statement to a particular
**		tablefield column, give the temporary a non-empty s_name.
*/

OSNODE *
ostvalnode (form, table, expr, column)
OSSYM		*form;
char		*table;
register OSNODE	*expr;
char		*column;
{
	OSSYM	*sym;
	OSNODE	*node;
	U_ARG		symp;
	U_ARG		valuep;
	U_ARG		exprp;

#ifdef OSL_TRACE
	SIprintf("ostvalnode: formname=|%s|; tblfldname=|%s|, col=|%s|\n",
			form->s_name, table, column
		);
#endif /* OSL_TRACE */

	symp.u_symp = sym = ostab(form, table, column);
	exprp.u_nodep = expr;
	/*
	** Use temporary only if column reference is not to the current row
	** of the table field or the table field is not being unloaded.
	*/
	if ( (sym->s_kind == OSCOLUMN || sym->s_kind == OSHIDCOL)  
	  && (expr != NULL || (sym->s_parent->s_ref & OS_TFUNLOAD) == 0) )
	{
		if (sym->s_ref & OS_TFCOLREF)
		{
			valuep.u_symp = osdatalloc(sym->s_type,
						(i4)sym->s_length, 
						sym->s_scale);
			valuep.u_symp->s_name = sym->s_name;
		}
		else
		{
			valuep.u_symp = sym;
		}
		sym->s_ref |= OS_TFCOLREF;
	}
	else
	{
		valuep.u_symp = sym;
	}
	node = osmknode(VALUE, &valuep, &symp, &exprp);
	node->n_flags |= N_VISIBLE;
#ifdef OSL_TRACE
        SIprintf("ostvalnode: VALUE node returned: token/type/len/prec=%d %d %d %d\n",
                        node->n_token, node->n_type, node->n_length, node->n_prec
                );
#endif /* OSL_TRACE */

	return node;
}

/*
** Name:	ostfldnode() -	Create Node for Table Field that we thought
**				was a form.
**
** Description:
**	Creates a node for a table field value.  This is a very obscure case.
**	For reasons of backward compatibility, a tablefield and form could have
**	the same name.  When we make a node, we assume the node is a OSFORM
**	node.  This routine checks a name to see if there's an OSTABLE with it,
**	whose name matches that of the form.  If there is such a beast, make a
**	new node and return it.
**
** Input:
**	form	The symbol table entry for the form.
**	name	The name of the object.
**
** Returns:
**	{OSNODE *}	The new node, or NULL if it doesn't pan out.
**
** History:
*/

static OSNODE *
ostfldnode(form, name)
OSSYM	*form;
char	*name;
{
	OSNODE	*obj = NULL;
	OSSYM	*tsym = ossympeek(name, form); 

	if (tsym != NULL 
	  && tsym->s_kind == OSTABLE
	  && STbcompare(tsym->s_name, 0, form->s_name, 0, TRUE) == 0) 
	{
		U_ARG	symp;

		symp.u_symp = tsym;
		obj = osmknode(VALUE, &symp, (U_ARG*)NULL, (U_ARG*)NULL);
		obj->n_flags |= (N_VISIBLE | N_READONLY);
	}
	return obj;
}

/*{
** Name:	osdotnode() -	Create Node for Record Attribute 
**
** Description:
**	Creates/doctors nodes (according to the node type) when a DOT operator
**	is seen.  The object/name pair can legally be:
**
**		record . attribute
**		form . tablefield
**		tablefield . column
**		dbtable . dbcolumn
**
**	Here is an overview of DOT and ARRAYREF nodes:
**
**	For an input string of
**		X.Y[i].Z
**	We'll produce a tree like this:
**
**		     <DOT>
**		      / \
**		     /   \
**	      <ARRAYREF>  <VALUE (Z)>   - The VALUE is the 'leaf' node.
**		  /   \
**		 /     \
**	     <DOT>    <int expr>	- this DOT has an FDF_ARRAY flag.
**	     /   \
**	    /     \
**   <VALUE (X)>  <VALUE (Y)>		- These nodes have type DB_DMY_TYPE.
**
** 	From this tree we will generate (in osreffrm.c) the following IL:
**
**		IL_DOT	 Xobj "Y" tmp
**		IL_ARRAY tmp  <int-expr> tmp
**		IL_DOT   tmp  "Z" tmp
**
**	'tmp' can then be assigned, or whatever.  (This is like a very 
**	elaborate version of MOVCONST.)  Each IL_DOT sets up 'tmp' to point
**	to the named member of the object to which 'tmp' currently points.
**	This allows us to use a single temporary to work our way through
**	the record-members until we get to a scalar leaf node.
**
**	This relatively simple model becomes very ugly, however, when we
**	get to 'unloadtable'.  A horrible command, but required for 
**	consistency with tablefield operations.
**
**	Another ugliness is introduced because table/array operations like
**	INSERTROW allow
**		:x.y [1]
** 	where x.y is a string variable holding the name of the tablefield.
**	We have to hack around this and check the tree later.
**
** Input:
**	form	The symbol table entry for the form.
**	obj	The node for the existing object.
**	attname	The name of the record attribute, or whatever it is.
**
** History:
**	09/89 (billc) -- Written.
**	10/09/91 (emerson)
**		Replace error OSNOINDEX (E_OS0257) by equivalent
**		but more informative OSNOROW (E_OS0260).
**		OSNOINDEX is now obsolete.
*/
OSNODE *
osdotnode ( form, obj, attname )
OSSYM		*form;
OSNODE		*obj;
char		*attname;
{
	OSSYM	*sym;

	/* 
	** The 'obj' node could be a 
	**	VALUE	- created by osvalnode().
	**	DOT	- produced by a previous call to osdotnode().
	*/

	if (osiserr(obj))
	{ /* we've already had an error, don't mess with it. */
		return obj;
	}

	sym = osnodesym(obj);

#ifdef OSL_TRACE
	SIprintf("osdotnode: tok %d: parent=%s, name=%s, s_kind=%d\n", 
		obj->n_token, sym->s_name, attname, sym->s_kind
		);
#endif /* OSL_TRACE */

	if (obj->n_token == VALUE)
	{
		/*
		** The VALUE node could be:
		**  OSVAR - a record or array (or an error).
		**  OSRATTR - a record attribute.
		**  OSTABLE - a tablefield.column reference.
		**  OSGLOB - a global record or array (or an error).
		*/

		switch (sym->s_kind)
		{
		  case OSRATTR:
		  case OSVAR:
		  case OSGLOB:
			if (sym->s_flags & FDF_ARRAY)
			{
				/*
				** It's an error if the node we're DOT-ing is
				** an array that hasn't been subscripted --
				** that is, if user input "a.y" and "a" is
				** declared as an array.  
				** The exception is in an unloadtable statement.
				*/
				if (sym == osunldobj())
				{
					obj->n_flags |= (N_UNLOAD | N_ROW);
					return mkdot(obj, attname);
				}
				oscerr(OSNOROW, 1, osnodename(obj));
				obj->n_flags |= N_ERROR;
				return obj;
			}
			else if (sym->s_flags & FDF_RECORD)
			{
				return mkdot(obj, attname);
			}
			/* record reference, but no record! */
			oscerr(OSBADDOT, 1, sym->s_name);
			break;

		  case OSTABLE:
			if (obj->n_tfref != NULL)
			{ /* tablefield or column already specified.*/
				oscerr(OSBADDOT, 1, obj->n_tfref->s_name);
			}
			else /* sym->s_kind == OSTABLE */ 
			{
				register OSNODE	*ret;

				/*
				** this is OK, since we'll always see the 
				** array subscript (if any) before we see the
				** '.' operator.
				*/
				ret = ostvalnode(form, sym->s_name, 
						obj->n_sexpr, attname
				);
				obj->n_flags &= ~N_READONLY;
				obj->n_sexpr = NULL;
				ostrfree(obj);
				return ret;
			}
			break;

		  default:
			oscerr(OSBADDOT, 1, sym->s_name);
			break;
		}

		/* 
		** If we fell out here, we have an error. mark the node as bad.
		*/
		obj->n_flags |= N_ERROR;
		return obj;
	}
	else if (obj->n_token == ARRAYREF && (sym->s_flags & FDF_ARRAY))
	{
		return mkdot(obj, attname);
	}
	else if (obj->n_token == DOT)
	{
		/*
		** It's an error if the node we're DOT-ing is an array that 
		** hasn't been subscripted -- that is, if user input "a.y" and
		** "a" is declared as an array.  
		** The exception is in an unloadtable statement.
		*/
		if (sym->s_flags & FDF_ARRAY)
		{
			if (sym == osunldobj())
			{
				/* unloading, so OK that there's no subscript */
				obj->n_flags |= (N_UNLOAD | N_ROW);
			}
			else
			{
				oscerr(OSNOROW, 1, osnodename(obj));
				obj->n_flags |= N_ERROR;
				return obj;
			}
		}
		return mkdot(obj, attname);
	}

	oscerr(OSBADDOT, 1, osnodename(obj));
	obj->n_flags |= N_ERROR;
	return obj;
}

/*{
** Name:	osqrydot() -	Create Node for Attribute in Query Target. 
**
** Description:
**	Creates nodes when a DOT operator needs to be built
**	for a query target element with respect to the query target.
**	The target/element pair can legally be:
**
**		record . attribute
**		array  . attribute
**
**	A true (non-placeholder) temp is allocated.  If the query target is a
**	record, IL is genereated to assign the true temp to the record attr.
**
** Input:
**	qrytarg	{OSNODE *} 	The query target assignment node.
**	attrsym {OSSYM  *}	The symbol table entry of the target list elt.
**
** History:
**	10/90 (jhw) -- Written.
**	11/15/90 (emerson)
**		Don't treat ARRAYREF qrytarg as if it were array (bug 34438).
**	11/20/90 (emerson)
**		Remove inapplicable stuff from functional description.
**	11/20/90 (emerson)
**		When searching for symbol table entry of parent of target
**		list element, don't stop on a temporary symbol (bug 34512).
**	11/24/90 (emerson)
**		Revise calling sequence: make caller pass in formobj and attrsym
**		(instead of the attribute name character string).  This improves
**		performance (because the caller generally has these already),
**		and it makes it easier for ostlqryall to call osqrydot
**		(to fix bug 34590).  This change supersedes the previous one.
**	10/15/91 (emerson)
**		If qrytarg is the UNLOADTABLE object, then don't treat it
**		as if it were an array (part of fix for bug 39492).
**	09/20/92 (emerson)
**		Major revisions for bug 34846.  Simplified the check for array
**		(using new info in tree nodes), and remove now-unneeded formobj
**		parameter.  Change non-array processing to allocate a true temp
**		and to generate IL to assign it to the ultimate target.
*/
OSNODE *
osqrydot ( qrytarg, attrsym )
register OSNODE	*qrytarg;
register OSSYM	*attrsym;
{
	U_ARG	qryval;
	U_ARG	attr;
	OSNODE	*tmpnode;	/* node for placeholder temp to DOT into */
	OSNODE	*datnode;	/* node for true temp to select into */

	attr.u_symp = osdatalloc(attrsym->s_type, (i4)attrsym->s_length,
							attrsym->s_scale);
	attr.u_symp->s_name = attrsym->s_name;

	datnode = osmknode(VALUE, &attr, (U_ARG *)NULL, (U_ARG *)NULL);

	if (!(qrytarg->n_flags & N_ROW))	/* array */
	{
		return datnode;
	}

	attr.u_symp = attrsym;
	attr.u_nodep = osmknode(VALUE, &attr, (U_ARG *)NULL, (U_ARG *)NULL);

	qryval.u_symp = ( qrytarg->n_token == VALUE )
				? qrytarg->n_sym : qrytarg->n_lhstemp;
	qryval.u_nodep = osmknode(VALUE, &qryval, (U_ARG *)NULL, (U_ARG *)NULL);

	tmpnode = os_lhs(osmknode(DOT, (U_ARG *)NULL, &qryval, &attr));

	IGgenStmt( IL_ASSIGN, (IGSID *)NULL, 3,
		   tmpnode->n_ilref, datnode->n_ilref,
		   oschkcoerce(datnode->n_type, tmpnode->n_type) );

	ostrfree(tmpnode);
	return datnode;
}

/*{
** Name:	mkdot() -	Create DOT Node.
**
** Description:
**	Creates a DOT node.
**
** Input:
**	record	The name of the record.
**	attname	The name of the record attribute.
**
** History:
**	09/89 (billc) -- Written.
*/
static OSNODE *
mkdot (record, attname)
OSNODE		*record;
char		*attname;
{
	U_ARG	u_rec;
	U_ARG	u_att;
	OSNODE	*tmp;
	OSSYM	*sym;

	sym = osnodesym(record);

	if (ossympeek(attname, sym) == NULL)
	{
		oscerr(OSNOTMEM, 2, attname, sym->s_name);
		record->n_flags |= N_ERROR;
		return record;
	}

	/* Create a VALUE node for this attribute.  */
	tmp = osvalnode(sym, attname);

	u_att.u_nodep = tmp;
	u_rec.u_nodep = record;

	return osmknode(DOT, (U_ARG*)NULL, &u_rec, &u_att); 
}

/*{
** Name:	osnodename() -	return name from a node.
**
** Description:
**	given a node, try to find some character string that might sensibly
**	serve as the node's name.  Serious ad-libbing may take place.
**
** Input:
**	node	The node to name.
**
** Returns:
**	{char*}	the name
**
** History:
**	09/89 (billc) -- Written.
*/

char *
osnodename (node)
OSNODE		*node;
{
	char *ret;

	switch (node->n_token)
	{
	  case DOT:
		ret = osnodename(node->n_right);
		break;

	  case ARRAYREF:
		ret = osnodename(node->n_left);
		break;

	  case VALUE:
		ret = node->n_sym->s_name;
		break;

	  case ATTR:
		if (node->n_attr != NULL)
			ret = node->n_attr;
		else
			ret = node->n_name;
		break;

	  default:
		if (!tkIS_CONST_MACRO(node->n_token))
		{
			char buf[64];
			osuerr(OSBUG, 1, STprintf(buf, 
				ERx("osnodename(node %d)"), node->n_token)
			);
		}
		ret = ERx("<unknown>");
		break;
	}
	return ret;
}

/*{
** Name:	osnodesym() -	return symbol from node.
**
** Description:
**	Given a node, find a symbol pointer for it, even if it means
**	rummaging around in the tree for it.
**
** Input:
**	node	The node to name.
**
** Returns:
**	{OSSYM*}	the symbol, if found.
**
** History:
**	09/89 (billc) -- Written.
*/

static OSSYM DummySym;

OSSYM *
osnodesym (node)
OSNODE		*node;
{
	OSSYM *ret = NULL;

	switch (node->n_token)
	{
	  case DOT:
		/*
		** If we've generated a temp for the DOT, we stashed it in
		** n_lhstemp.  Otherwise, return the most-recently digested
		** element of the DOT reference.  For example, if the DOT tree
		** is complete but no IL generated, we'll get the symbol entry
		** for the scalar leaf-node.
		*/
		if (node->n_lhstemp != NULL)
			return node->n_lhstemp;
		else
			return osnodesym(node->n_right);
		break;

	  case ARRAYREF:
		return osnodesym(node->n_left);
		break;

	  case VALUE:
	  case ATTR:
		return node->n_sym;
		break;

	  default:
		if (!tkIS_CONST_MACRO(node->n_token))
		{
			char buf[64];
			osuerr(OSBUG, 1, STprintf(buf, 
				ERx("osnodesym(node %d)"), node->n_token)
			);
		}
		/* fall through to... */

	  case tkID:
		ret = &DummySym;
		ret->s_kind = OSDUMMY;
		ret->s_flags = 0;
		break;
	}
	return ret;
}

/*{
** Name:	osarraynode() -	tack subscript info onto a node.
**
** Description:
**	Twiddles a node for array or tablefield reference by row. 
**	(See comments above, in osdotnode(), for a rundown on these.)
**
** Input:
**	obj	The name of the record or tablefield.
**	intexpr	Expression tree denoting row of record.
**	unambig	Indicates that the whole variable started with a ':'.
**
** History:
**	09/89 (billc) -- Written.
**	10/15/91 (emerson)
**		Check for user error "array[]" (bug 39492).
**		Allow array[] if we're in UNLOADTABLE array
**		(but don't allow array[][...]).
**		Also cleaned up logic a bit; in particular,
**		if we report error OSSYNTAX, don't also report
**		error OSNOTARRAY.
**	09/20/92 (emerson)
**		Use N_ROW bit of n_flags instead of n_sub.  (For bug 34846).
*/
OSNODE *
osarraynode (obj, intexpr, unambig)
OSNODE		*obj;
OSNODE		*intexpr;
bool		unambig;
{
	U_ARG	u_value;
	U_ARG	u_intexpr;
	OSSYM	*sym;
	i4	token;

	u_value.u_nodep = obj;
	u_intexpr.u_nodep = intexpr;

	/* 
	** The 'obj' node could be a 
	**	VALUE	- created by osvalnode().
	**	DOT	- produced by a previous call to osdotnode().
	**	ARRAYREF - created by osarraynode().
	*/

	if (osiserr(obj))
	{ /* we've already had an error, don't mess with it. */
		return obj;
	}

	sym = osnodesym(obj);

#ifdef OSL_TRACE
	SIprintf("osarraynode: tok %d, parent=%s, intx=0x%x, s_kind=%d\n", 
			obj->n_token, sym->s_name, intexpr, sym->s_kind
		);
#endif /* OSL_TRACE */

	token = obj->n_token;

	if (token == ARRAYREF || (obj->n_flags & N_UNLOAD))
	{
		i4 linenum = osscnlno();

		/* array index already specified.*/
		oscerr(OSSYNTAX, 2, &linenum, ERx("["));
		obj->n_flags |= N_ERROR;
		return obj;
	}
	if (token == VALUE)
	{
		/*
		** The VALUE node could be:
		**  OSHIDCOL - a hidden column (FDF_ARRAY will never be set).
		**  OSRATTR - a record attribute.
		**  OSVAR - a record or array (or an error).
		**  OSGLOB - a global record or array (or an error).
		**  OSTABLE - a tablefield.column reference.
		*/

		switch (sym->s_kind)
		{
		  case OSHIDCOL:
		  case OSRATTR:
		  case OSVAR:
		  case OSGLOB:
			/*
			** Treat these 4 kinds of VALUE nodes like DOT nodes.
			*/
			token = DOT;
			break;

		  case OSTABLE:
			if ( obj->n_flags & N_ROW )
			{
				i4 linenum = osscnlno();

				/* tablefield index already specified.*/
				oscerr(OSSYNTAX, 2, &linenum, ERx("["));
				obj->n_flags |= N_ERROR;
				return obj;
			}
			obj->n_sexpr = intexpr;
			obj->n_flags |= N_ROW;
			return obj;

		  default: /* error - not an array */
			break;
		}
	}
	if (token == DOT 
	  && ((sym->s_flags & FDF_ARRAY) || (unambig && oschkstr(obj->n_type))))
	{
		/*
		** We've got a DOT node, or a VALUE node of type
		** OSHIDCOL, OSRATTR, OSVAR, or OSGLOB.
		** The node represents either an array or a string variable.
		** We allow a string variable here for the case of tablefield
		** operations (INSERTROW, etc.) that regrettably allow 
		**	:x.y [1]
		** where x.y is a string variable holding the name of
		** the tablefield (or array).  osval() has to check for
		** abuses of this loophole.
		**
		** It's an error if the array index is missing (e.g. array[]),
		** except within an unloadtable statement, where the "array"
		** in the "array[]" is the array being unloaded.
		*/
		if (intexpr == NULL)
		{
			if (sym == osunldobj())
			{
				obj->n_flags |= (N_UNLOAD | N_ROW);
				return obj;
			}
			oscerr(OSNOROW, 1, osnodename(obj));
			obj->n_flags |= N_ERROR;
			return obj;
		}
		return osmknode(ARRAYREF, (U_ARG*)NULL, &u_value, &u_intexpr); 
	}

	/* whatever we have, it isn't an array. */
	oscerr(OSNOTARRAY, 1, osnodename(obj));
	obj->n_flags |= N_ERROR;
	return obj;
}
