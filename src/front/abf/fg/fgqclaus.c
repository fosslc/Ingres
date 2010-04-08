/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include       <ooclass.h>
# include       <metafrm.h>
# include	"framegen.h"
# include       <fglstr.h>
# include       <erfg.h>

/**
** Name:	fgqclaus.c -	Routines to generate clauses for 4GL queries
**
** Description:
**	This file defines:
**
**	IIFGwc_whereClause	Where clause for DELETE and UPDATE
**	IIFGic_intoClause	Into clause for INSERT
**	IIFGvc_valuesClause	Values clause for INSERT
**	IIFGsc_setClause	Set clause for UPDATE
**	IIFGfc_fromClause	From clause for SELECT
**	IIFGac_assignClause	Assignments for SELECT
**	IIFGrc_restrictClause	Restriction-generated where clause for SELECT
**	IIFGjc_joinClause	Join-generated where clause for SELECT
**	IIFGqc_qualClause	Qualificiation clause for SELECT
**	IIFGoc_orderClause	Order by clause for SELECT
**	IIFGlac_lupvalAssignClause	Assignment clause for lookup validation
**
**	Throughout, it is assumed that the utility pointers for columns
**	point to hidden field names, and the utility pointers for tables point
**	to correlation names.
**
** History:
**	15 may 1989 (Mike S)	Initial version
**	11 jul 1989 (pete)	Added new argument 'terminate' to
**				IIFGwc_whereClause & IIFGvc_valuesClause. 
**	19 mar 1990 (pete)	Added code to generate different WHERE
**				clause in UPDATE/DELETE statements if table
**				has NULLable keys.
**	1/90 (MikeS)		Add qualfication in a tablefield for masters
**				in a tablefield.
**	1/91 (Mike S)		Include COL_VARIABLE columns from VQ except
**				in qualifications
**	6-dec-92 (blaise)
**		Added code generation for optimistic locking.
**	09-sep-93 (cmr) changed numcols to numkcols to avoid confusion.
**	17-nov-1993 (daver)
**		Implement the handling of delimited ids for frame generation.
**		Column names need to go thru IIUGxri_id() to double
**		quote them, as necessary. Local Variable/Field names are
**		already handled, since delim ids are not valid field names.
**	22-nov-1993 (daver)
**		Re-parameterized opt_lock_value(); fix for bug 55943.
**		Removed unneccesary dynamic memory alloc code; possible
**		fix for intermittant memory bug 56668. See comments in
**		opt_lock_value() for details.
**	18-jan-1993 (daver)
**		Backout piccolo change 408307 (fix for optim lock bug 57546)
**		as it caused bug 58827 (internal error when compiling).
**		Preserve mgw's changes to this file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */
FUNC_EXTERN	LSTR 		*IIFGaql_allocQueryLstr();
FUNC_EXTERN	char		*IIFGga_getassign();
FUNC_EXTERN	char		*IIFGgd_getdefault();
FUNC_EXTERN	LSTR		*IIFGmrs_makeRestrictString();
FUNC_EXTERN	bool		IIFGnkf_nullKeyFlags();
FUNC_EXTERN	IITOKINFO	*IIFGftFindToken();
FUNC_EXTERN	PTR		IIFGgm_getmem();

extern METAFRAME *G_metaframe;

/* static's */
static const char _asc[]             = ERx(" ASC");
static const char _desc[]            = ERx(" DESC");
static const char _from[]        	= ERx("FROM");
static const char _into[]        	= ERx("INTO ");
static const char _order_by[]	= ERx("ORDER BY");
static const char _qualification[]	= ERx("QUALIFICATION");
static const char _set[]		= ERx("SET");
static const char _values[]        	= ERx("VALUES");
static const char _where[]        	= ERx("WHERE");
static const char _isnull[]		= ERx(" IS NULL");
static const char _or[]		= ERx(" OR");
static const char _user[]		= ERx("USER");
static const char _now[]		= ERx("'NOW'");
static const char _optim[]		= ERx("optimistic");

static const char _zero[]		= ERx("0");
static const char _one[]		= ERx("1");

static const char _null[]        	= ERx("");
static const char _dot[]        	= ERx(".");
static const char _equal[]        	= ERx(" = ");
static const char _equal_deref[]    	= ERx(" = :");
static const char _space[]    	= ERx(" ");
static const char _lparen[]		= ERx("(");
static const char _rparen[]		= ERx(")");
static const char _colon[]		= ERx(":");
static const char _plus[]		= ERx(" + ");

static  char	*column_field_name();
static	VOID	do_assign();
static	MFCOL	*masterjcol();
static	char	*masterjname();
static  VOID	opt_lock_value();


/*{
** Name:	IIFGwc_whereClause	where clause for DELETE and UPDATE
**
** Description:
**	Generate the WHERE clause for DELETE or UPDATE, using the specified
**	list of columns.
**
** Inputs:
**	table		MFTAB	*table being changed
**	numkcols	i4	number of key columns
**	keycols		i4 []  key columns
**	intblfld	bool	TRUE if we're in a table field
**	bool		terminate   tells to terminate statement with ';'
**
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated 4GL code
**
**	Exceptions:
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 May 1989 (Mike S)	Initial version
**	26 jun 1989 (Mike S) 	
**		Check for detail table combination join field and unique key
**      17-nov-1992 (daver)
**              Implement the handling of delimited ids for frame generation.
*/
LSTR *
IIFGwc_whereClause( table, numkcols, keycols, intblfld, terminate )
MFTAB	*table;	
i4      numkcols;	/* number of items in array keycols[] */
i4      *keycols;
bool    intblfld;
bool	terminate;	/* tells whether to terminate statement with ';' */
{
	LSTR *lstr, *lstr0;
	i4 col;
	register MFCOL *colptr;
	char *hidden;	/* hidden field name */
	bool hidintf;	/* TRUE if the hidden field is in a tablefield */
	i4  nullkeycnt =0;	/* number of nullable keys encountered */
	char column_name[FE_UNRML_MAXNAME + 1];	/* extra buffer for delimid */

	/* create "WHERE" string */
	lstr = lstr0 = IIFGaql_allocQueryLstr((LSTR *)NULL, LSTR_NONE);
	lstr0->string = _where;

	/*
	**	for each key column, generate
	**	"column = :hidden_field" string.
	**
	**	If column is NULLable, then rather than generating:
	**		column = :hidden_field
	**	generate either:
	**	(1)	((:iiNullKeyFlag1 = 1 AND column IS NULL)
	**			OR (column = :hidden_field))
	**	or
	**	(2)	((:hidden_field IS NULL AND column IS NULL)
	**			OR (column = :hidden_field))
	**
	**	based on whether IIFGnkf_nullKeyFlags() returns TRUE or FALSE.
	**	(1) is required on 6.3 and previous Ingres, because of jupbug
	**	9821, but (2) is better because it doesn't require the
	**	hidden NullKeyFlag fields. However (2) only works on
	**	(I believe) 6.3/03/00 and later Ingres, and it works on all Gateways.
	**NOTE: because Ingres chokes on this, (2) was never more than
	**	visually tested (although Wes checked the query syntax on
	**	Gateways and says it's ok). Also note that IIFGnkf_nullKeyFlags
	**	currently ALWAYS returns TRUE.
	*/
	for (col = 0; col < numkcols; col++)
	{
		colptr = table->cols[keycols[col]];
		lstr = IIFGaql_allocQueryLstr(lstr, LSTR_AND);

		/* 
		** double quote the column name, if necessary, and place
		** it in a seperate buffer. required if column name is a
		** delim id, since it could cause syntax errors (contain
		** punctuation or be an sql keyword)
		*/
		IIUGxri_id(colptr->name, column_name);

		/* 
		** For the detail join field, the hidden field name is a simple
		** field associated with the master join field, and thus not
		** in the tablefield.
		*/
		if ( (table->tabsect == TAB_DETAIL) && 
		     ((colptr->flags & COL_SUBJOIN) != 0) )
		{
			hidden = (char *)
				 (masterjcol(table, keycols[col])->utility);
			hidintf = FALSE;
		}
		else
		{
			hidden = (char *)(colptr->utility);
			hidintf = intblfld;
		}

		if (colptr->type.db_datatype >= 0)
		{
		    STpolycat(5, 
			  column_name,
			  _equal_deref,
			  (hidintf) ? TFNAME : _null,
			  (hidintf) ? _dot : _null, 
			  hidden,
			  lstr->string);
		}
		else
		{
			/* NULLable key column */

			if (IIFGnkf_nullKeyFlags())
			{
				/* generate (1) (see for() loop header) */

				char akeynum[(DB_GW2_MAX_COLS/100) + 3];

				nullkeycnt++;
				CVna(nullkeycnt, akeynum);

		    		STpolycat(6, 
				    _lparen,
				    _lparen,
				    _colon,
				    ERget(F_FG0037_FlagFieldName),
				    akeynum,
				    ERx(" = 1"),
				    lstr->string);

				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_NONE);

		    		STpolycat(4, 
			  	    column_name,
			  	    _isnull,
				    _rparen,
				    _or,
				    lstr->string);

				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_AND);

		    		STpolycat(8, 
				    _lparen,
			  	    column_name,
				    _equal_deref,
			  	    (hidintf) ? TFNAME : _null,
			  	    (hidintf) ? _dot : _null, 
			 	    hidden,
				    _rparen,
				    _rparen,
				    lstr->string);
			}
			else
			{
				/* generate (2) (see for() loop header) */

		    		STpolycat(7, 
				    _lparen,
				    _lparen,
				    _colon,
			  	    (hidintf) ? TFNAME : _null,
			  	    (hidintf) ? _dot : _null, 
			 	    hidden,
			  	    _isnull,
				    lstr->string);

				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_NONE);

		    		STpolycat(4, 
			  	    column_name,
			  	    _isnull,
				    _rparen,
				    _or,
				    lstr->string);

				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_AND);

		    		STpolycat(8, 
				    _lparen,
			  	    column_name,
				    _equal_deref,
			  	    (hidintf) ? TFNAME : _null,
			  	    (hidintf) ? _dot : _null, 
			 	    hidden,
				    _rparen,
				    _rparen,
				    lstr->string);
			}
		}
	}
	lstr->follow = LSTR_NONE;
	lstr->nl = TRUE;
	if (terminate)
		lstr->sc = TRUE;
	return (lstr0);
}

/*{
** Name:	IIFGic_intoClause	into clause for INSERT
**
** Description:
**	Generate the INTO clause for INSERT into the specified table.
**
** Inputs:
**	table		MFTAB * table
**
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated 4GL code
**
**	Exceptions:
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 May 1989 (Mike S)	Initial version
**      17-nov-1992 (daver)
**              Implement the handling of delimited ids for frame generation.
**		Take advantage of an LSTR structure's buffer member. Quote
**		a column name into the buffer (if necessary), and use
**		the quoted identifier in the into clause (e.g, INTO "select").
*/
LSTR *
IIFGic_intoClause(table)
MFTAB	*table;
{
	LSTR *lstr, *lstr0;
	i4 col;
	
	/* Create "INTO" string */
	lstr = lstr0 = IIFGaql_allocQueryLstr((LSTR *)NULL, LSTR_OPEN);
	STpolycat(2, _into, table->name, lstr0->string);

	/*
	**	We will insert something into each column of the table.
	*/
	for (col = 0; col < table->numcols; col++)
	{
		lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
		/* 
		** need to quote the column name if necessary.
		** put the possibly quoted column in the lstr's buffer,
		** a handy 256 byte thang. Code used to simply assign
		** lstr->string to table->cols[col]->name.
		*/
		IIUGxri_id(table->cols[col]->name, lstr->buffer);
		/*
		** now point the string to the buffer element 
		*/
		lstr->string = lstr->buffer;
	}
	lstr->follow = LSTR_CLOSE;
	return (lstr0);
}

/*{
** Name:	IIFGvc_valuesClause	VALUES clause for INSERT
**
** Description:
**	Generate the VALUES clause for an INSERT for the specified table.
**
** Inputs:
**	table		*MFTAB
**	intblfld	bool	TRUE if we're in a table field
**	assign		bool	TRUE to use the assignment string in the
**				metaframe.
**	terminate	bool	tells to terminate statement with ';'
**
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated 4GL code
**
**	Exceptions:
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 May 1989 (Mike S)	Initial version
**	 3 Jun 1989 (Mike S)	Use join value for detail join field.
**	 8 Jun 1989 (Mike S)	Put developer-specified values on their own
**				lines.
**	9/90 (Mike S)		Use assignemnt from master for non-displayed
**				master-detail join column
**	6-dec-92 (blaise)
**		Added support for optimistic locking: for an optimistic
**		locking column we need to insert the correct initial value
**		(see comments below).
*/
LSTR *
IIFGvc_valuesClause( table, intblfld, assign, terminate )
MFTAB	*table;
bool    intblfld;
bool	assign;
bool	terminate;	/* tells whether to terminate statement with ';' */
{
	LSTR *lstr, *lstr0, *prev_lstr;
	i4 col;
	MFCOL *column;
	IITOKINFO	*p_iitok;

	/* Create "VALUES" string" */
	lstr = lstr0 = IIFGaql_allocQueryLstr((LSTR *)NULL, LSTR_OPEN);
	lstr->string = _values;

	/*
	**	1. If we're using optimistic locking (substitution variable
	**	   $locks_held == "optimistic") and this is a column which
	**	   is being used for optimistic locking, insert:
	**		0	integer column
	**		'now'	date column
	**		user	char or varchar column
	**	2. For a displayed or variable field, we insert the 
	**	   displayed value or the variable's value.
	**	3. For the master-detail join field in the detail section,
	**	   we insert the value from the master section.
	**	   3a) if it is an update frame (assign == FALSE) and 
	**		the frame behavior says the database will do
	**		the update.. then we force use of the hidden value
	**	   3b) if not the above.. then we just use whatever
	**		appropriate value from the master section which
	**		may be either the alias name (if displayed or a 
	**		variable) or the hidden field name (if any) or the 
	**		assignment value 
	**	4. For a non-displayed, non-variable sequenced field, 
	**	   the proper value is saved in a hidden field.  We insert it.
	**	5. If none of the preceding holds, and we're in an APPEND
	**	   frame, we get the value assigned by the developer.
	**	6. Otherwise, we insert the default value for the datatype.
	*/
	for (col = 0; col < table->numcols; col++)
	{
		prev_lstr = lstr;
		lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
		column = table->cols[col];
		/* 
		** Are we using optimistic locking? Get the value of the 
		** $locks_held substitution variable
		*/
		p_iitok = IIFGftFindToken(ERx("$locks_held"));
		if (p_iitok != (IITOKINFO *)NULL &&
			STbcompare(p_iitok->toktran, 0, _optim, 0, TRUE) == 0 &&
			column->flags & COL_OPTLOCK)
		{
			/*
			** We're using optimistic locking and this is a column
			** assigned for that purpose. Insert vision-defined
			** values.
			**
			** Pass in lstr->string to opt_lock_value; copy
			** now done there. Part of fix for bug 55943 (daver).
			*/
			opt_lock_value(column, FG_VALUES_CLS, column->name,
					intblfld, lstr->string);
		}
		else if ((column->flags & (COL_USED|COL_VARIABLE)) != 0)
		{
			STpolycat(3,
				 intblfld ? TFNAME : _null,
				 intblfld ? _dot :_null,
				 column->alias,
				 lstr->string);
		}
		else if ((table->tabsect > TAB_MASTER) && 
			((column->flags & COL_SUBJOIN) != 0))
		{
			if (  assign == FALSE
			   && (table->flags & TAB_UPDNONE) != 0
			   )
			{
				lstr->string = 
					column_field_name(FALSE, 
						masterjcol(table, col));
			}
			else
			{
				MFCOL *mcol = masterjcol(table, col);

				if ((mcol->flags & (COL_USED|COL_VARIABLE)) 
					!= 0)
				{
					lstr->string = mcol->alias;
				}
				else if (mcol->utility != NULL && 
					 *mcol->utility != EOS)
				{
					lstr->string = mcol->utility;
				}
				else
				{
					lstr->nl = prev_lstr->nl = TRUE;
					lstr->string = IIFGga_getassign(mcol);
				}
			}
		}
		else if ((column->flags & COL_SEQUENCE) != 0)
		{
			STpolycat(3,
				 intblfld ? TFNAME : _null,
				 intblfld ? _dot :_null,
				 (char *)(column->utility),
				 lstr->string);
		}
		else if (assign)
		{
			lstr->nl = prev_lstr->nl = TRUE;
			lstr->string = IIFGga_getassign(column);
		}
		else
		{
			lstr->string = 
				IIFGgd_getdefault(column->type.db_datatype);
		}

		lstr->follow = LSTR_COMMA;
	}
	lstr->follow = LSTR_CLOSE;
	lstr->nl = TRUE;
	if (terminate)
		lstr->sc = TRUE;
	return (lstr0);
}

/*{
** Name:	IIFGic_setClause	set clause for UPDATE
**
** Description:
**	Generate the SET clause for UPDATE into the specified table.
**
** Inputs:
**	table		MFTAB * table
**	intblfld	bool	TRUE if we're in a table field
**
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated 4GL code
**
**	Exceptions:
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 May 1989 (Mike S)	Initial version
**	 3 jun 1989 (Mike S)	Set detail join field
**	6-dec-92 (blaise)
**		Added support for optimistic locking: for an optimistic
**		locking column we need to update the contents to the correct
**		value (see comments below).
**	10-nov-93 (blaise)
**		Fixed bugs in code generated for optimistic locking: added
**		missing comma, removed spurious semicolon (bugs #56663, 56668)
**	11-nov-93 (blaise)
**		Fix to opt_lock_value(): prefix hidden field with a colon
**		in SET clause.
**      17-nov-1992 (daver)
**              Implement the handling of delimited ids for frame generation.
*/
LSTR *
IIFGsc_setClause(table, intblfld)
MFTAB	*table;
bool    intblfld;
{
	LSTR *lstr, *lstr0;
	i4 col;
	MFCOL *column;
	IITOKINFO	*p_iitok;
	char column_name[FE_UNRML_MAXNAME + 1];	/* extra buffer for delimid */
	
	/* Create the "SET" string */
	lstr = lstr0 = IIFGaql_allocQueryLstr((LSTR *)NULL, LSTR_NONE);
	lstr0->string = _set;

	/*
	**	1. If we're using optimistic locking (substitution variable
	**	   $locks_held == "optimistic") and this is a column which
	**	   is being used for optimistic locking, set column equal to:
	**		hidden name+1	integer column (i.e. increment record)
	**		'now'		date column
	**		user		char or varchar column
	**	2.  If the column is displayed or in a variable, we update it 
	**	    with the displayed value or the variable's value.
	**	3.  If the column is the master-detail join field, and we're
	**	    in the detail section, we update it with the 
	**	    value from the master section.
	*/
	for (col = 0; col < table->numcols; col++)
	{
		column = table->cols[col];

		/* 
		** double quote the column name, if necessary, and place
		** it in a seperate buffer. required if column name is a
		** delim id, since it could cause syntax errors (contain
		** punctuation or be an sql keyword)
		*/
		IIUGxri_id(column->name, column_name);

		/* 
		** Are we using optimistic locking? Get the value of the 
		** $locks_held substitution variable
		*/
		p_iitok = IIFGftFindToken(ERx("$locks_held"));
		if (p_iitok != (IITOKINFO *)NULL &&
			STbcompare(p_iitok->toktran, 0, _optim, 0, TRUE) == 0 &&
			column->flags & COL_OPTLOCK)
		{
			/*
			** We're using optimistic locking and this is a column
			** assigned for that purpose. Insert vision-defined
			** values.
			*/
			lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);

			/*
			** Pass in lstr->string to opt_lock_value; copy
			** now done there. Part of fix for bug 55943 (daver).
			*/
			opt_lock_value(column, FG_SET_CLS, column_name,
					intblfld, lstr->string);
		}
		else if ((column->flags & (COL_USED|COL_VARIABLE)) != 0)
		{
			lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
			STpolycat(5, 
			  	column_name, 
				_equal_deref,
				intblfld ? TFNAME : _null,
				intblfld ? _dot : _null,
				column->alias,
				lstr->string);
		}
		else if (  table->tabsect > TAB_MASTER
			&& (column->flags & COL_SUBJOIN) != 0
			&& (table->flags & TAB_UPDNONE) == 0
			)
		{
			lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
			STpolycat(3, 
			  	column_name, 
				_equal_deref,
				masterjname(table, col),
				lstr->string);
		}

	}
	lstr->follow = LSTR_NONE;
	return(lstr0);
}

/*{
** Name:	IIFGfc_fromClause - From clause for SELECT
**
** Description:
**	Generate the From clause for a SELECT for the indicated
**	tables.
**
** Inputs:
**	tabdx1		i4	First table.
**	tabdx2		i4	Last table.
**
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated clause
**
**	Exceptions
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 may 1989 (Mike S)	Initial version.
**	13 nov 1989 (Mike S)	
**		Check whether lookup tables have any dispalyed columns.
*/
LSTR *
IIFGfc_fromClause(tabdx1, tabdx2)
i4	tabdx1;
i4	tabdx2;
{
	i4     tbldx, coldx;
	MFTAB   *table;
	LSTR	*lstr0, *lstr;
	bool	any;		/* TRUE if any columns are displayed */

	/* Create the "FROM" string */
	lstr = lstr0 = IIFGaql_allocQueryLstr((LSTR *)NULL, LSTR_NONE);
	lstr0->string = _from;

	/* List all the tables in the section, with their correlation names */
	for (tbldx = tabdx1; tbldx <= tabdx2; tbldx++)
	{
		table = G_metaframe->tabs[tbldx];
		if (tbldx == tabdx1)
		{
			/* Primary tables will always have displayed columns */
			table->flags &= ~TAB_NODISPC;
		}
		else
		{
			/* See if any columns are displayed or in variables */
			any = FALSE;
			for (coldx = 0; coldx < table->numcols; coldx++)
			{
				if ((table->cols[coldx]->flags & 
				    (COL_USED|COL_VARIABLE)) !=0)
				{
					any = TRUE;
					break;
				}
			}
			if (any)
			{
				/* At least one was used */
				table->flags &= ~TAB_NODISPC;
			}
			else
			{
				/* None used.  Don't join to this table */
				table->flags |= TAB_NODISPC;
				continue;
			}
		}
		lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
		STpolycat(3,
			  table->name,
			  _space,
			  (char *)(table->utility),
			  lstr->string);
	}
	lstr->follow = LSTR_NONE;
	return (lstr0);
}

/*{
** Name:	IIFGac_assignClause - Assignment clause for SELECT
**
** Description:
**	Generate the assignment clause for a SELECT for the indicated
**	tables.
**
** Inputs:
**	tabdx1		i4	index of first table
**	tabdx2		i4	index of last table 
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated clause
**
**	Exceptions
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 may 1989 (Mike S)	Initial version.
**	26 jun 1989 (Mike S)	Remove tablefield name
**	9/90 (Mike S)		Ignore hidden fields for lookup tables
*/
LSTR *
IIFGac_assignClause(tabdx1, tabdx2)
i4	tabdx1;
i4	tabdx2;
{
	i4     tbldx;
	MFTAB   *table;
	i4     coldx;
	MFCOL   *column;
	LSTR   *lstr = (LSTR *)NULL, *lstr0 = (LSTR *)NULL;

	/* Loop through all tables in section */
	for (tbldx = tabdx1; tbldx <= tabdx2; tbldx++)
	{
		table = G_metaframe->tabs[tbldx];
		
		/* Loop through all column in table */
		for (coldx = 0; coldx < table->numcols; coldx++)
		{
			column = table->cols[coldx];

			/* 
			** If it's displayed, or in a variable, select into 
			** the form field or variable 
			*/
			if ((column->flags & (COL_USED|COL_VARIABLE)) != 0)
			{	
				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
				do_assign(lstr, column->name, column->alias,
					  (char *)(table->utility), FALSE);
				if (lstr0 == (LSTR *)NULL) lstr0 = lstr;
			}

			/* 
			** If it has a hidden field copy, and isn't in a lookup
			** table, select into it.
			*/
		  	if ( (table->usage != TAB_LOOKUP) &&
			     ((char *)(column->utility) != (char *)NULL) &&
			     (*(char *)(column->utility) != EOS)
			   )
			{	
				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
				do_assign(lstr, column->name, 
					  (char *)(column->utility),
					  (char *)(table->utility), FALSE);
				if (lstr0 == (LSTR *)NULL) lstr0 = lstr;
			}
		}
	}
	if (lstr != (LSTR *)NULL) lstr->follow = LSTR_NONE;
	return(lstr0);
}

/*
**	Create assignment string 
**
** History:
**      17-nov-1992 (daver)
**              Implement the handling of delimited ids for frame generation.
*/
static VOID do_assign(lstr, column, field, table, tblfldcol)
LSTR   *lstr;
char    *column;
char    *field;
char    *table;
bool	tblfldcol;	/* Is the target explicitly a tablefield column */
{
	char *cptr = lstr->string;
	char column_name[FE_UNRML_MAXNAME + 1];	/* extra buffer for delimid */

	/* 
	** double quote the column name, if necessary, and place
	** it in a seperate buffer. required if column name is a
	** delim id, since it could cause syntax errors (contain
	** punctuation or be an sql keyword)
	*/
	IIUGxri_id(column, column_name);

	/* Create assignment string "[:iitf[].]field = table.column" */
	if (tblfldcol)
	{
		STpolycat(3, _colon, TFNAME, ERx("[]."), cptr);
		cptr += STlength(cptr);
	}

	STpolycat(5, field, _equal, table, _dot, column_name, cptr);
}

/*{
** Name:	IIFGrc_restrictClause - Restriction  clause for SELECT
**
** Description:
**	Generate the restriction-generated part of the WHERE clause for a 
**	SELECT for the indicated table.
**
** Inputs:
**	table		MFTAB *	primary table.
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated clause
**
**	Exceptions
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 may 1989 (Mike S)	Initial version.
**      17-nov-1992 (daver)
**              Implement the handling of delimited ids for frame generation.
**		Extended size, changed name of variable to hold 
**		"table.column"; quoted column name if necessary
**		
*/
LSTR *
IIFGrc_restrictClause(table)
MFTAB   *table;                 /* Primary table for this section */
{
	MFCOL   **cols = table->cols;
	i4     coldx;
	char    tbl_dot_column[2 * FE_UNRML_MAXNAME +1];
	LSTR	*lstr0 = (LSTR *)NULL, *lstr, *last = (LSTR *)NULL, *new_last;
	char	column_name[FE_UNRML_MAXNAME + 1]; /* extra buf for delimid */


	/* Loop through all columns in table */
	for (coldx = 0; coldx < table->numcols; coldx++)
	{
		if (cols[coldx]->info != NULL && *cols[coldx]->info != EOS)
		{
			/* 
			** double quote the column name, if necessary, and 
			** place it in a seperate buffer. required if column 
			** name is a delim id, since it could cause syntax 
			** errors (contain punctuation or be an sql keyword)
			*/
			IIUGxri_id(cols[coldx]->name, column_name);

			STprintf(tbl_dot_column, ERx("%s.%s"), 
				 (char *)(table->utility),
				 column_name);

		 	lstr = IIFGmrs_makeRestrictString(
				tbl_dot_column, cols[coldx]->info, &new_last);
			/*
			** If a restiction was created, link it onto the 
			** restriction chain.
			*/
			if (lstr != (LSTR *)NULL)
			{
				if (lstr0 == (LSTR *)NULL)
					lstr0 = lstr;
				else
					last->next = lstr;
				last = new_last;
				last->follow = LSTR_AND;
			}
		 }
	}
	if (last != (LSTR *)NULL) last->follow = LSTR_NONE;
	return(lstr0);	
}

/*{
** Name:	IIFGjc_joinClause - Join  clause for SELECT
**
** Description:
**	Generate the join-generated part of the WHERE clause for a 
**	SELECT.
**
** Inputs:
**	sectdx		i4[TAB_DETAIL+1][2] 
**				array of table indices
**	sect1		i4	first section
**	sect2		i4	second section
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated clause
**
**	Exceptions
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 may 1989 (Mike S)	Initial version.
**	3 Jun 1989  (Mike S)	Use visible field for master_detail join
**      17-nov-1992 (daver)
**              Implement the handling of delimited ids for frame generation.
*/
LSTR *
IIFGjc_joinClause(sectdx, sect1, sect2)
i4  sectdx[TAB_DETAIL+1][2];
i4  sect1;
i4  sect2;
{
        register MFJOIN *joinptr;
        register MFTAB  *table1, *table2;
        i4              joindx;
	LSTR		*lstr = (LSTR *)NULL, *lstr0 = (LSTR *)NULL;
			
	char		table1_col1[FE_UNRML_MAXNAME + 1];  /* extra buffers */
	char		table2_col2[FE_UNRML_MAXNAME + 1];  /* for delimid */

	/* Loop through all joins in the metaframe */
        for (joindx = 0; joindx < G_metaframe->numjoins; joindx++)
        {
		/*
		** See if the current join is for the sections we're 
		** interested in.
		*/
                joinptr = G_metaframe->joins[joindx];
                if ( (sectdx[sect1][0] <= joinptr->tab_1) &&
                     (joinptr->tab_1 <= sectdx[sect1][1]) &&
                     (sectdx[sect2][0] <= joinptr->tab_2) &&
                     (joinptr->tab_2 <= sectdx[sect2][1]) )
                {
                        table1 = G_metaframe->tabs[joinptr->tab_1];
                        table2 = G_metaframe->tabs[joinptr->tab_2];

			/* 
			** double quote the column name, if necessary, and 
			** place it in a seperate buffer. required if column 
			** name is a delim id, since it could cause syntax 
			** errors (contain punctuation or be an sql keyword)
			*/
			IIUGxri_id(table1->cols[joinptr->col_1]->name,
					table1_col1);
			IIUGxri_id(table2->cols[joinptr->col_2]->name,
					table2_col2);

			/* 
			** Master-detail isn't a database join; the detail 
			** table value must equal the master section form 
			** field.
			*/
			if ( (sect1 == TAB_MASTER) && (sect2 == TAB_DETAIL) )
			{
				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_AND);
				if (lstr0 == (LSTR *)NULL) lstr0 = lstr;

				STpolycat(5,
			    	  	  (char *)(table2->utility),
			    	  	  _dot,
					  table2_col2,
			    	  	  _equal_deref,
					  column_field_name(TRUE,
					     table1->cols[joinptr->col_1]),
			    	  	  lstr->string);
			}
			/*
			** Lookup is a database join.  We make sure the lookup
			** table has displayed columns.
			*/
			else if ((table2->flags & TAB_NODISPC) == 0)
			{
				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_AND);
				if (lstr0 == (LSTR *)NULL) lstr0 = lstr;
				STpolycat(7,
			    	  	  (char *)(table1->utility),
			    	  	  _dot,
					  table1_col1,
			    	  	  _equal,
			    	  	  (char *)(table2->utility),
			    	  	  _dot,
					  table2_col2,
			    	  	  lstr->string);
			}
                }
        }
	if (lstr != (LSTR *)NULL) lstr->follow = LSTR_NONE;
	return(lstr0);
}

/*{
** Name:	IIFGqc_qualClause - Qualification clause for SELECT
**
** Description:
**	Generate the qualification part of the WHERE clause for a 
**	SELECT.  We are always in the master section.
**
** Inputs:
**	tabdx1	i4	First table in master section
**	tabdx2	i4	Last table in master section
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated clause
**
**	Exceptions
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 may 1989 (Mike S)	Initial version.
**      17-nov-1992 (daver)
**              Implement the handling of delimited ids for frame generation.
*/
LSTR *
IIFGqc_qualClause( tabdx1, tabdx2 )
i4	tabdx1;
i4	tabdx2;	
{
        i4  tbldx;
        MFTAB *table;
        i4  coldx;
        MFCOL *column;
	LSTR *lstr, *lstr0;
	char column_name[FE_UNRML_MAXNAME + 1]; /* extra buf for delimid */

	/* Create "QUALIFICATION string */
	lstr = lstr0 = IIFGaql_allocQueryLstr((LSTR *)NULL, LSTR_OPEN);
	lstr0->string = _qualification;

	/* Every displayed field in the section can contain a qualification */
        for (tbldx = tabdx1; tbldx <= tabdx2; tbldx++)
        {
                table = G_metaframe->tabs[tbldx];
                for (coldx = 0; coldx < table->numcols; coldx++)
                {
                        column = table->cols[coldx];

			/* 
			** double quote the column name, if necessary, and 
			** place it in a seperate buffer. required if column 
			** name is a delim id, since it could cause syntax 
			** errors (contain punctuation or be an sql keyword)
			*/
			IIUGxri_id(column->name, column_name);

                        if ((column->flags & COL_USED) != 0)
                        {
				lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
				if ((G_metaframe->mode & MFMODE) == MF_MASTAB)
					STpolycat(7,
						(char *)(table->utility),
						_dot,
						column_name,
						_equal,
						TFNAME,
						_dot,
						column->alias,
						lstr->string);
				else
					STpolycat(5,
						(char *)(table->utility),
						_dot,
						column_name,
						_equal,
						column->alias,
						lstr->string);
                        }
                }
        }
	if (lstr == lstr0)
	{
		return((LSTR *)NULL);
	}
	else
	{	
		lstr->follow = LSTR_CLOSE;
		return(lstr0);
	}
}

/*{
** Name:	IIFGoc_orderClause - Order clause for SELECT
**
** Description:
**	Generate the Order By clause for a SELECT.
**
** Inputs:
**	table		MFTAB *	primary table.
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated clause
**
**	Exceptions
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	15 may 1989 (Mike S)	Initial version.
**	25 jul 1989 (Mike S)	Don't use tablefield name
*/
LSTR *
IIFGoc_orderClause(table)
MFTAB	*table;
{
        i4  sort_cols[DB_GW2_MAX_COLS];
        i4  col;
        i4  sort;
        i4  num_sorts;
        MFCOL *colptr;
	LSTR *lstr0, *lstr;

	/* Collect all sort caolumns */
        for (col = 0, num_sorts = 0; col < table->numcols; col++)
        {
                sort = table->cols[col]->corder;
                if (sort > 0)
                {
                        sort_cols[sort-1] = col;
                        num_sorts = max(sort, num_sorts);
                }
        }

	/* 
	** If any sort columns were specified, list them in order.  Each is
	** explicitly followed by "ASC" or "DESC".
	*/
        if (num_sorts > 0)
        {
		lstr = lstr0 = IIFGaql_allocQueryLstr((LSTR *)NULL, LSTR_NONE);
		lstr0->string = _order_by;
                for (col = 0; col < num_sorts; col++)
                {
			lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
                        colptr = table->cols[sort_cols[col]];
			STpolycat(2,
				  column_field_name(TRUE, colptr),
				  ((colptr->flags & COL_DESCEND) == 0) ?
					_asc : _desc,
				  lstr->string);
                }
		lstr->follow = LSTR_NONE;
		return(lstr0);
        }
	else
	{
		return((LSTR *)NULL);
	}
}

/*{
** Name:	IIFGlac_lupvalAssignClause - Assignment clause for Lookup 
*					     validation
**
** Description:
**	Generate the assignment clause for a Lookup validation for the 
**	indicated table and columns.
**
** Inputs:
**	table		MFTAB *	lookup table
**	mcol		MFCOL *	primary join field/column
**	intblfld	bool	Are we in a tablefield?
** Outputs:
**	none
**
**	Returns:
**			LSTR *	generated clause
**
**	Exceptions
**		none
**
** Side Effects:
**		Allocates dynamic memory.
**
** History:
**	21 jun 1989 (Mike S)	Initial version.
**	24 jul 1990 (Mike S)	Select into master join field/column
**	15 oct 1990 (Mike S)	Select into table field columns
**	4-jan-1994 (mgw) Bug #58241
**		Implement owner.table for query frame creation.
*/
LSTR *
IIFGlac_lupvalAssignClause(table, mcol, intblfld)
MFTAB	*table;		/* Lookup table */
MFCOL	*mcol;		/* Primary join column */
bool	intblfld;	/* Are we in a tablefield */
{
	i4 	coldx;
	MFCOL 	*column;	
        LSTR   	*lstr = (LSTR *)NULL, *lstr0 = (LSTR *)NULL;
	char	tmpname[FE_MAXNAME+1];
	char	tmpown[FE_MAXNAME+1];
	FE_RSLV_NAME tmprslv;

	/*
	** Loop through all columns in table; use a column if
	** 1.	It's displayed or in a variable, or 
	** 2.	It's the sub-join field.  In this case, get the form field name
	**	from the primary join field.
	*/
	for (coldx = 0; coldx < table->numcols; coldx++)
	{
		column = table->cols[coldx];
		tmpname[0] = EOS;
		tmpown[0] = EOS;
		tmprslv.name = table->name;
		tmprslv.is_nrml = TRUE;
		tmprslv.owner_dest = tmpown;
		tmprslv.name_dest = tmpname;
		FE_decompose(&tmprslv);

		if ((column->flags & (COL_USED|COL_VARIABLE)) != 0)
		{	
			lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
			do_assign(lstr, column->name, column->alias, 
				  tmpname, intblfld);
			if (lstr0 == (LSTR *)NULL) lstr0 = lstr;
		}
		else if ((column->flags & COL_SUBJOIN) != 0)
		{
			lstr = IIFGaql_allocQueryLstr(lstr, LSTR_COMMA);
			do_assign(lstr, column->name, 
				column_field_name(TRUE, mcol), tmpname, 
				intblfld);
			if (lstr0 == (LSTR *)NULL) lstr0 = lstr;
		}
	}
	if (lstr != (LSTR *)NULL) lstr->follow = LSTR_NONE;
	return(lstr0);
}

/*
**	Find the primary column for a master-detail join.
**
**	(Maintenance note: see also IIFGmj_MasterJoinCol() in fgutils.c,
**	which is identical, except doesn't do error reporting).
*/
static MFCOL
*masterjcol(detbl, detcol)
MFTAB *detbl;		/* Detail table */
i4  detcol;		/* Detail column index */
{
	i4 i;
	MFJOIN *joinptr;

	/*
	** Search all joins in metastructure for a match.
	*/
	for (i = 0; i < G_metaframe->numjoins; i++)
	{
		joinptr = G_metaframe->joins[i];
		if ( joinptr->type == JOIN_MASDET &&
		     detbl == G_metaframe->tabs[joinptr->tab_2] &&
		     joinptr->col_2 == detcol )
		     return G_metaframe->tabs[joinptr->tab_1]->
				cols[joinptr->col_1];
	}
	/* No join found -- declare error */
	IIFGqerror(E_FG001C_Query_NoJoin, TRUE, 2, 
		   detbl->name, detbl->cols[detcol]->name);
	/*NOTREACHED*/
}

/*
**	Find the name of the field (displayed or hidden) for a
**	column;
*/
static char *
column_field_name(allow_alias, column)
bool  allow_alias;
MFCOL *column;
{
	/* If no utility name was created, signal an error */
	if (allow_alias && (column->flags & (COL_USED|COL_VARIABLE)) != 0)
		return (column->alias);
	else if (column->utility != NULL && *column->utility != EOS)
		return (column->utility);
	else
		IIFGqerror(E_FG004C_Query_NoHiddenName, FALSE, 1, column->name);
		/*NOTREACHED*/
}

/*
**	Find the name of the field (displayed or hidden) for the
**	primary column for a master-detail join.
*/
static char
*masterjname(detbl, detcol)
MFTAB *detbl;		/* Detail table */
i4  detcol;		/* Detail column index */
{
	return column_field_name(TRUE, masterjcol(detbl, detcol));
}

/*
** Name:	opt_lock_value		- get value for optim lock column
**
** Description:
**	Copy the insert or update value for an optimistic locking column, i.e.
**
**				VALUES clause	SET clause
**
**	integer column		'0'		col_name = :[iitf.]col_name + 1
**	date column		'now'		col_name = 'now'
**	char/varchar col	USER		col_name = USER
**
** Inputs:
**	col		MFCOL *	->type.db_datatype holds datatype of column
**	clause		i4	either FG_VALUES_CLS or FG_SET_CLS
**	col_name	char *	table's column name, possibly delimited
**	in_tblfld	bool	TRUE if iitf.<col_name> needed; FALSE otherwise
**	dest		char *	pointer to current lstr->buffer to hold result
**	
** Outputs:
**	dest		char *	appropriate string is copied into this buffer
**	
** History:
**	22-nov-1993 (daver)
**		Documented and changed parameterization; included bool 
**		is_tblfld, string dest, and possibly delimited column name 
**		col_name. Removed code to dynamicly allocate memory; 
**		unnecessary since the contents of the memory chunk was 
**		immediately STcopy'd to an awaiting buffer (this could fix 
**		intermittant memory bug 56668). Passed in pointer to awaiting 
**		buffer (dest) and explicitly write to it here. Changed return 
**		value to VOID. Fixes bug 55943.
*/
static VOID
opt_lock_value(col, clause, col_name, in_tblfld, dest)
MFCOL	*col;
i4	clause;
char	*col_name;
bool	in_tblfld;
char	*dest;
{

	switch (abs(col->type.db_datatype))
	{
		case DB_INT_TYPE:
		/*
		** Update counter. If we're inserting (VALUES clause) we
		** want to initialize the record with 0. If we're updating
		** (SET clause) we want to increment the value in the record,
		** i.e. we return "hname + 1" where hname is the column's
		** hidden name.
		*/
			if (clause == FG_VALUES_CLS)
			{
				STcopy(_zero, dest);
			}
			else	/* FG_SET_CLS */
			{
				if (in_tblfld)
				{
					STpolycat(7, col_name, _equal_deref, 
							TFNAME, _dot, 
							col->utility, _plus, 
							_one, dest);
				}
				else
				{
					STpolycat(5, col_name, _equal_deref, 
							col->utility, _plus, 
							_one, dest);
				}
			}
			break;
		case DB_DTE_TYPE:
		/* date/time stamp */
			if (clause == FG_VALUES_CLS)
			{
				STcopy(_now, dest);
			}
			else	/* FG_SET_CLS */
			{
				STpolycat(3, col_name, _equal, 
						_now, dest);
			}
			break;
		case DB_CHA_TYPE:
		case DB_VCH_TYPE:
		/* username */
			if (clause == FG_VALUES_CLS)
			{
				STcopy(_user, dest);
			}
			else	/* FG_SET_CLS */
			{
				STpolycat(3, col_name, _equal, 
						_user, dest);
			}
	}
}
