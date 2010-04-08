# include 	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<equel.h>
# include	<equtils.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqstmt.h>
# include	<ereq.h>

/*
+* Filename:	RETRIEVE.C
** Purpose:	Maintains all the routines for managing Retrieve statements.
**
** Defines:	ret_add(varsym,varid,indsym,indid) - Add a Retrieve var 
**					[with ind] to queue.
**		ret_close()	   -	Dump queue of buffered Retrieves in
**					specific format.
**		ret_dump( caller ) - 	Dump the local retrieve list for debug.
**		ret_stats()   	   -    Local static dump.
** Locals:
**		ret_clrelm( r )	   -	Clear fields when freeing.
**		
** This is used for Retrieve statements - to keep track of user variables 
** and optional indicator variables used as result columns.  This module, 
** besides queuing the variable and indicator for dumping, also prints out 
-* the variable name as a result for a database retrieve.
**
** Notes:
**  1.  Preprocessor Operation:
**	For each result variable in the Retrieve statement, a corresponding 
**	result column name is created (usually full name of variable) and sent
**	to the database, while the variable, and its fully qualified name is
**	buffered away by the local Retrieve routines to be later dumped inside
**	IIretdom() calls - the runtime calls that actually retrieve data into
**	program variables.
**  2.  The null-indicator variable (if present) is buffered simultaneously
**	and is dumped as an argument to an IIindret() call -- a runtime
**	call that sets things up so that a negative value is retrieved into 
**	the indicator variable if the retrieved data is null valued.
**
** History:
**		03-oct-1984 	- Written (ncg)
**		10-sep-1986	- Added null support (bjb)
**		09-nov-1992	- Added a new member to RET_ELM.  Added two
**				  new parameters to ret_add.  Also modified
**				  ret_close to gen_call datahandler. (lan)
**	16-nov-1992 (lan)
**		Replaced IIPUTDATAHDLR and IIGETDATAHDLR with IIDATAHDLR.
**	19-nov-1992 (lan)
**		When the datahandler argument name is null, generate (char *)0.
**	08-dec-1992 (lan)
**		Removed redundant code in ret_close.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

typedef struct {
	Q_ELM		r_qlink;	/* Queue manager stuff */
	char		*r_vname;	/* Variable name for result column */
	SYM		*r_vsym;	/* Symbol table entry for variable */
	char		*r_iname;	/* Name of indicator variable */
	SYM		*r_isym;	/* Symbol table entry for indicator */
	char		*r_arg;		/* Datahandler argument */
	i4		r_typ;		/* Datahandler type */
} RET_ELM;

# define	RETNULL		(RET_ELM *)0

/* Local Retrieve Queue Manager */
static		Q_DESC		ret_q ZERO_FILL;

/* When freeing list of Retrive elements clear each one out */
static		i4		ret_clrelm();


/*
+* Procedure:	ret_add 
** Purpose:	Parser passed an Equel representation of the variable.
**
** Parameters: 	varsym  - SYM *  - Variable to add to retrieve list.
**		varid   - char * - Fully qualified name.
**		indsym	- SYM *  - Indicator variable
**		indid	- char * - Fully qualified name of indicator.
**		arg	- char * - Datahandler argument name.
-* Returns: 	None
**
** Reformat the representation of the user variable and indicator (if any)
** into a local buffer and store. Dump a legal Quel name (<=12 chars) as a 
** result column for the original Retrieve. This name is the name that the 
** symbol table entry is pointing at.
*/

void
ret_add( varsym, varid, indsym, indid, arg, type )
register SYM		*varsym, *indsym;
char			*varid, *indid, *arg;
i4			type;
{
    register RET_ELM 	*ret;
    static   i4	init = FALSE;

    /* Save the Retrieve variable */
    if ( (varsym == (SYM *)0) && (type != T_HDLR) )
    {
	er_write( E_EQ0386_retVAR, EQ_ERROR, 1, varid );
	db_key( ERx("IIretv") );
    }
    else 
    {
	if ( init == FALSE )
	{
	    init = TRUE;
	    q_init( &ret_q, sizeof(RET_ELM) );
	}
	q_new( (RET_ELM *), ret, &ret_q );
	ret->r_vname = str_add( STRNULL, varid );
	ret->r_vsym = varsym;
	if (indid == (char *)0)
	    indid = ERx("$");		/* Don't print garbage in ret_dump() */
	ret->r_iname = str_add( STRNULL, indid );
	ret->r_isym = indsym;		/* May be null */
	ret->r_arg = arg;
	ret->r_typ = type;
	q_enqueue( &ret_q, ret );
	/* Dump Quel result column name to output (not the whole Id name) */
	if (dml_islang(DML_EQUEL))
	    db_attname( sym_str_name(varsym), 0 );
    }
}


/*
+* Procedure:	ret_close 
** Purpose:	Walk through the list of Retrieve elements and write the 
**	       	corresponding II calls to output.
**
** Parameters: 	None
-* Returns: 	None
**
** When done free up the queue, and reserved string space.
** History:
**    10-jun-1993 (kathryn)
**       Add calls to esq_sqmods() around calls to user defined datahandlers.
**       The scope of the EQ_CHRPAD flag applies only to the file currently
**       being pre-processed. The datahandler being called may be in another
**       file.  Since IIsqMods changes the runtime behavior, we want to turn
**       it off before calling a handler and turn it back on after calling
**       the handler.  We want the handler to have whatever behavior was
**       set by the pre-processor flags when the handler was pre-processed.
*/

void
ret_close()
{
    register RET_ELM	*ret;

    for ( ret = q_first((RET_ELM *), &ret_q); ret != RETNULL; 
	  ret = q_follow((RET_ELM *), ret) )
    {
      if ( ret->r_typ == T_HDLR )
      {
	/* Datahandler - Generates IILQldh_LoDataHandler */
	if (eq->eq_flags & EQ_CHRPAD)
	    esq_sqmods(IImodNOCPAD);
	arg_int_add( 1 );
	arg_var_add( ret->r_isym, ret->r_iname );
	arg_str_add( ARG_RAW, ret->r_vname );
	arg_str_add( ARG_RAW, ret->r_arg );
	gen_call( IIDATAHDLR );
	if (eq->eq_flags & EQ_CHRPAD)
	    esq_sqmods(IImodCPAD);
      }
      else
      {
	/* Write indicator/argument (and description) to output in a call */
	if (ret->r_isym)
	    arg_var_add( ret->r_isym, ret->r_iname );
	else
	    arg_int_add( 0 );
	arg_var_add(  ret->r_vsym, ret->r_vname );
	gen_call( IIRETDOM );
      }
    }

    /* Free up the queue space used by this Retrieve statement head */
    ret = q_first((RET_ELM *), &ret_q);
    if ( ret != RETNULL )
	q_free( &ret_q, ret_clrelm );
}


/*
+* Procedure:	ret_clrelm 
** Purpose:	Reset all Retrieve fields before freeing. Indirectly called
**		when freeing queue of Retrieve elements.
**
** Parameters: 	ret - RET_ELM * - Retrieve element.
-* Returns: 	i4 - 1 / 0 : Continue / Stop.
*/

static i4
ret_clrelm( ret )
register RET_ELM *ret;
{
    if ( ret != RETNULL )
    {
	ret->r_vname = (char *)NULL;
	ret->r_vsym = (SYM *)NULL;
	ret->r_iname = (char *)NULL;
	ret->r_isym = (SYM *)NULL;
	return 1;
    }
    return 0;
}


/*
+* Procedure:	ret_dump 
** Purpose:	Walk through the Retrieve list and dump its contents.
**
** Parameters: 	calname - char * - Caller's name.
-* Returns: 	None
*/

void
ret_dump( calname )
char	*calname;
{
    register RET_ELM	*ret;
    register i4	i;
    register FILE	*df = eq->eq_dumpfile;

    if ( calname == (char *)0 )
	calname = ERx("Retrieve_list");
    SIfprintf( df, ERx("RET_DUMP: %s\n"), calname );
    for ( i = 0, ret = q_first((RET_ELM *), &ret_q); ret != RETNULL; 
	  i++,   ret = q_follow((RET_ELM *), ret) )
    {
	/* Dump argument (and its description) to output */
	SIfprintf( df, 
	ERx(" ret[%3d]: addr=0x%p, name=%s, varptr=0x%p, ind=%s, indptr=0x%p "),
	    i, ret, ret->r_vname, ret->r_vsym, ret->r_iname, ret->r_isym );
	SIfprintf( df, ERx("type = %d\n"), sym_g_btype(ret->r_vsym) );
    }
    SIflush( df );
}

/*
+* Procedure:	ret_stats 
** Purpose:	Get local statics for dumping elsewhere.
**
** Parameters: 	None
-* Returns: 	None
*/

void
ret_stats()
{
    q_dump( &ret_q, ERx("Ret_q") );
}
