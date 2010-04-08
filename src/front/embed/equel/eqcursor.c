#define	STATIC	/* */
/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<si.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<equtils.h>
# include	<eqrun.h>
# include	<eqstmt.h>
# include	<eqlang.h>
# include	<me.h>
# include	<ereq.h>

/**
** Filename: eqcursor.c - EQUEL Jupiter preprocessor cursor utility routines.
**
** Description:
**	This module defines the routines used by the grammar to implement
**	semantics for cursors.
** Defines:
**   Major statements:
**	ecs_declcsr( cs_name, cs_sym )	- Allocate and initialize a cursor
**	ecs_open(cs_name,rd_only,sy,is_dyn) - Gen code to open a cursor
**	ecs_close( cs_name, is_dyn )	- Gen code to close a cursor
**	ecs_retrieve( cs_name, is_dyn, fetch_o, fetch_c, fetch_v )
**					- Gen code to set up fetching csr data
**	ecs_delete(cs_name,tbl_name,is_dyn) - Gen code to delete a row
**	ecs_replace(beg_end,cs_name,is_dyn) - Gen code to update a row
**   Minor statements:
** 	ecs_opquery( cs_name )		- Emit call to IIcsQuery; ESQL only.
**	ecs_get( var_name, sy, btype, 
**		ind_name, indsy )	- Gen code to get one datum from cursor
**	ecs_eretrieve( cs_name )	- Gen code to stop fetching cursor data
**	ecs_addtab( cs_name, tbl_name ) - Add a table to a cursor
**	ecs_colupd( cs_name, col_name, what ) - Set/Check a csr col as update
**	ecs_query( cs_name, init )	- Attach a query to a cursor
**	ecs_csrset( cs_name, val )	- Set flag bits of a cursor
**	ecs_gcsrbits( cs_name )		- Get cursor flag bits
**	ecs_namecsr()			- Get the name of the current cursor
**	ecs_chkupd( cs_name, col_name, sy )- Check that a column is updatable
**	ecs_recover()			- Clean up from grammar syntax errors
**	ecs_fixupd( cs_name )		- Fix update column list
**	ecs_clear()			- Reset all data structures
**	ecs_id_curcsr()			- Return a pointer to the cur csr id
**	ecs_coladd( col_name )		- Save a column name, temporarily
**	ecs_colrslv( doit )		- Check/toss saved column names
**	ecs_dump()			- Dump the cursor data structures
**   Locals:
**	ecs_gcursor( cs_name, def )	- Turn a cursor name into a cursor
**	ecs_buffer( ch )		- Stash away a char into a cursor qry
**	ecs_lookup( cs_name )		- Find a cursor name in the cursor list
**	ecs_dyn( qry_id, cs_name )	- Set id of dynamic cursor
**	ecs_nm_add()			- Gen the name of the cur cursor as args
**	ecs_isupdcsr( csr )		- Is this an updatable cursor?
**	ecs_malloc( size )		- Allocate space; exit on failure
**	ecs_stralloc( str )		- Stash away a string
**	ecs_ctype( typ )		- Format a cursor data type
**	ecs_flags( buf, flags )		- Format cursor flag bits
**	ecs_coflags( buf, flags )	- Format column flag bits
** Notes:
**	A. There is a (compile-time) notion of the "current cursor", set by
**	   major statements (declare, open, close, retrieve, delete, and
**	   replace).
**	   Any minor statement (eg, ecs_colupd is a minor statement of
**	   declare) may pass NULL for the cursor name to use the current cursor.
**	   If there is a cursor-specific error then a message will be issued
**	   and that cursor's error bit set; however, if the cursor's error bit 
**	   was already set then the message will be suppressed.
**
**	B. There is also a statement error flag, cleared by each major
**	   statement.  If an statement-specific error is encountered then
**	   the current statement-error flag is set, and an message printed
**	   if the error flags was previously clear.  For example, an attempt
**	   to REPLACE a read-only cursor will generate an error message, but
**	   update-validation checks on the column names will not print
**	   additional error messages.
**
**	C. Some sample statements, with the calls the grammar should generate:
**
**	   0.	int	val1, val2;
**
**	   1.	DECLARE CURSOR ca FOR RETRIEVE (ty.col1, tz.col2)
**
**		ecs_declcsr( "ca" );			-- declare the cursor
**		ecs_query( NULL, TRUE );		-- start buffering query
**		ecs_addtab( NULL, "ty" );		-- note table name used
**		ecs_colupd( NULL, "col1", ECS_ADD );	-- add column
**		ecs_addtab( NULL, "ty" );		-- note table name used
**		ecs_colupd( NULL, "col2", ECS_ADD );	-- add column
**		    -- now the RETRIEVE statement is parsed and emitted
**		    -- write query; it will be buffered into the cursor area
**		    -- qry: IIwritedb( "retrieve(ty.col1, tz.col2)" );
**		ecs_query( NULL, FALSE );		-- stop buffering query
**
**		-- generates:
**		--	Nothing.
**
**	   2.	DECLARE CURSOR cx FOR RETRIEVE [UNIQUE] (ty.col1, ty.col2) 
**			[WHERE ...] [SORT ...]
**			FOR [DEFERRED|DIRECT] UPDATE OF (col1, col2)
**
**		ecs_declcsr( "cx" );			-- declare the cursor
**		ecs_query( NULL, TRUE );		-- start buffering query
**		ecs_addtab( NULL, "ty" );		-- note table name used
**		ecs_colupd( NULL, "col1", ECS_ADD );	-- add column
**		ecs_addtab( NULL, "ty" );		-- note table name used
**		ecs_colupd( NULL, "col2", ECS_ADD );	-- add column
**		ecs_colupd( NULL, "", ECS_CHK|ECS_ISWILD );-- is var in tgtlist?
**		ecs_colupd( NULL, "col1", ECS_CHK );	-- get col1 descriptor
**		ecs_colupd( NULL, "col1", ECS_CHK|ECS_UPD );-- col1 = updatable
**		ecs_colupd( NULL, "", ECS_CHK|ECS_ISWILD );-- is var in tgtlist?
**		ecs_colupd( NULL, "col2", ECS_CHK );	-- get col2 descriptor
**		ecs_colupd( NULL, "col2", ECS_CHK|ECS_UPD );-- col2 = updatable
**		ecs_fixupd( NULL );			-- handle vars in update
**		ecs_setcsr( NULL, ECS_UPDATE );		-- set cursor updatable
**		    -- now the RETRIEVE statement is parsed and emitted
**		    -- write query; it will be buffered into the cursor area
**		    -- qry: IIwritedb( "retrieve(ty.col1, ty.col2)" );
**		    --	    IIwritedb( "for update of (col1, col2)" );
**		ecs_query( NULL, FALSE );		-- stop buffering query
**
**		-- generates:
**		--	Nothing.
**
**	   3.	OPEN CURSOR cx [FOR READONLY]
**
**		ecs_open( "cx", NULL, 0 );		-- open the cursor
**
**		-- generates:
**		-- {
**		--	IIcsOpen( "cx",num1,num2 );
**		--	IIwritedb( "declare cursor cx for retrieve" );
**		--	IIwritedb( "(ty.col1, ty.col2)" );
**		--	IIwritedb( "for update of (col1, col2)" );
**		--	[IIwritedb( " FOR READONLY" );]
**	    	--	IIcsQuery();
**		-- }
**
**	   4.	CLOSE CURSOR cx
**
**		ecs_close( "cx", num1, num2 );		-- close the cursor
**
**		-- generates:
**		-- {
**		--	IIcsClose( "cs", num1, num2 );
**		-- }
**
**	   5.	RETRIEVE CURSOR cx (var1, var2)
**
**		ecs_retrieve( "cx" );			       -- start retrieve
**		ecs_get( "var1", var1_symtab_ptr, var1_type ); -- get datum #1
**		ecs_get( "var2", var2_symtab_ptr, var2_type ); -- get datum #2
**		ecs_eretrieve( NULL );			       -- end retrieve
**
**		-- generates:
**		-- {
**		--	if (IIcsRetrieve("cx",num1,num2) != 0) {
**		--		IIcsGet( 1, type, length, &var1 );
**		--		IIcsGet( 1, type, length, &var2 );
**		--		IIcsERetrieve();
**		--	}
**		-- }
**
**	   6.	DELETE CURSOR cx
**
**		ecs_delete( "cx" );			-- delete current row
**
**		-- generates:
**		-- {
**		--	IIcsDelete( "cx", num1, num2 );
**		-- }
**
**	   7.	REPLACE CURSOR cx (col1 = val1, col2 = val2)
**
**		ecs_replace( TRUE, "cx" );		-- start replace cursor
**		ecs_chkupd( NULL, "col1", 0 );		-- column #1 updatable?
**		ecs_colupd( NULL, "col1", ECS_CHK );	-- first get descriptor
**		ecs_chkupd( NULL, "col2", 0 );		-- column #2 updatable?
**		ecs_colupd( NULL, "col2", ECS_CHK );	-- first get descriptor
**		db_xxx calls ouputting the target list ...
**		ecs_replace( FALSE, NULL );		-- end replace cursor
**
**		-- generates:
**		-- {
**		--	IIcsReplace( "cx", num1, num2 );
**		--	IIwritedb( "(col1=val1, col2=val2)" );
**		--	IIcsEReplace( "cx", num1, num2 );
**		-- }
** History:
**	11-aug-86	- Written (mrw)
**	29-jun-87 (bab)	Code cleanup.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	08-feb-89 (bjb) Added support for dynamic cursor names.
**	17-mar-89 (ttl) Changed error message EQ0427 to output table name.
**	18-jan-90 (barbara)
**	    Fixed ecs_get to understand varchar type.  Also added
**	    a 'default' case to ecs_get switch to take care of DBV_TYPE
**	16-nov-1992 (lan)
**	    Added new parameter, arg, to ecs_get.
**	19-nov-1992 (lan)
**	    When the datahandler argument name is null, generate (char *)0.
**	08-dec-1992 (lan)
**	    Removed redundant code in ecs_get.
**	08-feb-1993 (lan)
**	    Removed error message EQ0400 generated when the "update...where
**	    current of" statement is used on a cursor that was declared
**	    without the presence of the FOR UPDATE clause.
**      04-mar-1993 (smc)
**          Cast parameter of MEfree to match prototype.
**	25-jun-1993 (kathryn) 	Bug 52796
**	    Change ecs_get to include datahandler in target list count.
**	29-mar-1994 (teresal)
**	    Add new ESQL/C++ preprocessor support. Set the indent level
**	    for C++ the same as for C.
**	11-may-1994 (timt) 	Bug 62370
**	    Add check to make sure E_EQ0418 is only issued for equel.
**	    See tt62370 for all the changes.
**	13-mar-1996 (thoda04)
**	    Changed ecs_eretrieve() last parm from int to i4  since
**	    callers are passing i4  arguments.  int==nat usually, but not always. 
**	    Added function parameter prototypes for local functions.
**	    Fixed call to MEreqmem(); first parm tag is u_i2, not nat.
**	    General cleanup of other compiler warnings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-feb-2003 (abbjo03)
**	    Remove PL/1 as a supported language.
**/


/*
** Data Structures
*/

/*}
** Name: ECS_NM_LIST	- A list of table names for a cursor.
**
** Description:
**	A singly-linked list of names, used to hold the tables associated
**	with a given query.
**
** History:
**	11-aug-86	- Written. (mrw)
*/

typedef struct ecs_nm_list {
    char	*ecs_nlname;	/* The table name */
    struct ecs_nm_list
		*ecs_nlnext;	/* The next table descriptor */
} ECS_NM_LIST;

/*}
** Name: ECS_COL_LIST	- Descriptions of the columns of a cursor.
**
** Description:
**	A singly-linked list of column names and flages, derived from
**	the target and update lists.
**
** History:
**	11-aug-86	- Written. (mrw)
*/

typedef struct ecs_col_list {
    char	*ecs_clname;	/* Column name */
    i4		ecs_clflags;  /* EXISTS,EXPR,UPD,MANY,CONSTANT,ISWILD,DYNAMIC */
    struct ecs_col_list
		*ecs_clnext;	/* Next column descriptor */
} ECS_COL_LIST;

/*}
** Name: ECS_TYPE_LIST	- Holds the fetch types of a cursor.
**
** Description:
**	A temporary singly-linked list of btypes, gathered during the first
**	retrieve.  Exists only during that first retrieve.  See the description
**	of the ecs_clisttyp field in the ECS_CURSOR structure for details.
**
** History:
**	11-aug-86	- Written. (mrw)
*/

typedef struct ecs_type_list {
    u_i2	ecs_tltype;		/* The base type of this column */
    struct ecs_type_list
		*ecs_tlnext;		/* Pointer to the next column type */
} ECS_TYPE_LIST;

/*}
** Name: ECS_CURSOR	- The compile-time structure of a cursor.
**
** Description:
**	Compile-time structure of a cursor.  Holds all the info about a cursor
**	throughout the scope of the cursor.
**
**   Description of the fields.
**	ecs_cnext:	Link to the next cursor descriptor.
**	ecs_cname:	The name of this cursor.
**	ecs_csym:	A pointer to the symtab entry for a dynamically
**			named cursor.  A null pointer for statically named
**			cursors.
**	ecs_ctable:	Linked list of tables associated with this cursor.
**			Since table names aren't known if they are in variables,
**			it is possible that this list might be empty.
**	ecs_ccols:	Linked list of target-list column names and flags.
**	ecs_clisttyp:	Before the first RETRIEVE on a cursor, nothing is
**			known about the types of the columns (might not even
**			know the number of them).  Therefore, during the first
**			RETRIEVE we make a linked list of retrieval types,
**			based on the type of the variables being used.  This
**			list is pointed to by ecs_clisttyp.  When the next
**			RETRIEVE is begun, an array of type-indicators with
**			as many elements as are in the list will be allocated
**			and pointed to by ecs_ctypes.  The nodes of the list
**			will be copied into this array, and the list freed.
**			This array is used in the second (and subsequent
**			RETRIEVEs to check for consistency).
**			IMPORTANT:  This list is stored in reverse order, for
**			insertion speed.  This is known only in ecs_get, where
**			the list is built, and in ecs_retrieve where it is read.
**			Well, ecs_dump also knows about it, but that's not so
**			important.
**	ecs_ctypes:	A pointer to an array of retrieval types.  Derived from
**			ecs_clisttyp, above.  Valid only after the second
**			RETRIEVE has begun.
**	ecs_cnmvals:	The number of retrieve-values we should get from the BE.
**			Like ecs_ctypes, valid only after the second RETRIEVE
**			has begun.  As the code for retrieving each column is
**			generated, we check that we're supposed to be getting
**			enough columns from the BE.
**	ecs_cfetched:	The number of column-values retrieved from this row at
**			this point in the RETRIEVE.  Only guaranteed to be valid
**			during a retrieve, but in fact will be 0 before the
**			first retrieve, and will have the final count of the
**			last retrieve until the next one.  When the retrieve
**			is finished, we check that we got all the columns that
**			the BE is planning to send.
**	ecs_cqry:	A pointer to the text of the associated query.
**	ecs_cflags:	Cursor-global flags.
**				ECS_ERR, ECS_REPEAT, ECS_DIRECTU, ECS_DEFERRU,
**				ECS_UPDATE, ECS_RDONLY, ECS_UNIQUE, ECS_SORTED
**				ECS_WHERE, ECS_UPDVAR, ECS_UPDLIST
**			Look in <eqstmt.h> for a description of these bits.
**	ecs_cnmtables:	The number of tables mentioned anywhere in the cursor
**			declaration.  Cursors with more than one table are not
**			updatable.
**	ecs_cupdcols:	The number of actually updatable columns in the target
**			list.  As we include any columns for which we can't
**			decide (because of the presence of variables in the
**			update list), we could be too lax here.  Not to worry --
**			the BE will catch it at runtime.
**
** History:
**	11-aug-86	- Modified from the ESQL cursor structure. (mrw)
**	08-feb-89	- Added ecs_csym fld for dynamically-named cursors (bjb)
*/

typedef struct ecs_cursor {
    struct ecs_cursor
		*ecs_cnext;	/* the next cursor */
    EDB_QUERY_ID
		ecs_cname;	/* cursor id */
    SYM		*ecs_csym;	/* cursor symtab pointer (may be null) */
    ECS_NM_LIST	*ecs_ctable;	/* list of associated tables */
    ECS_COL_LIST
		*ecs_ccols;	/* the list of targetlist column names */
    ECS_TYPE_LIST
		*ecs_clisttyp;	/* temp linked list of column RETRIEVE types */
    u_i2	*ecs_ctypes;	/* array of RETRIEVE type bytes */
    i4		ecs_cnmvals;	/* number of associated RETRIEVE values */
    i4		ecs_cfetched;	/* number of columns RETRIEVEd currently */
    STR_TABLE	*ecs_cqry;	/* the text of the associated query */
    i4		ecs_cflags;	/* flag bits */
    i4		ecs_cnmtables;	/* the number of tables in the targetlist */
    i4		ecs_cupdcols;	/* the number of updatable columns */
} ECS_CURSOR;

/*
** #defines and data declarations
*/

/* ecs_ctypes */
# define ECS_TEMPTY	0	/* We don't yet know the EQUEL Type */
# define ECS_TNONE	1	/* No EQUEL Type */
# define ECS_TNUMERIC	2	/* Numeric EQUEL Type */
# define ECS_TCHAR	3	/* Character EQUEL Type */
# define ECS_TUNDEF	4	/* Undefined EQUEL Type (probably error) */
# define ECS_THDLR	5	/* Datahandler Type */

/* Cursor updatable bits */
# define ECS_CSRISUPD	(ECS_DIRECTU|ECS_DEFERRU|ECS_UPDATE)

/* All the clause-bits that make a cursor non-updateable */
# define ECS_CLAUSE_RDO	\
	(ECS_UNIQUE|ECS_SORTED|ECS_UNION|ECS_GROUP|ECS_HAVING|ECS_FUNCTION)

/*
** Column updatable bits. Usage: if (ECS_COLISUPD(col)) ...
** This is because a column is potentially updatable iff it exists,
** has a unique name, and is neither an expression nor an expression.
*/
# define ECS_COLUPDFLAGS	(ECS_EXISTS|ECS_EXPR|ECS_MANY|ECS_CONSTANT)
# define ECS_COLISUPD(col)\
		(((col)->ecs_clflags & ECS_COLUPDFLAGS) == ECS_EXISTS)
/* Column is potentially updatable, but not already known as updatable. */
# define ECS_COLNEWISUPD(col)\
		(((col)->ecs_clflags & (ECS_COLUPDFLAGS|ECS_UPD)) == ECS_EXISTS)

/* Pieces of a cursor name */
# define ECS_CSRNAME(csr)	(EDB_QRYNAME(&((csr)->ecs_cname)))
# define ECS_CSR1NUM(csr)	(EDB_QRY1NUM(&((csr)->ecs_cname)))
# define ECS_CSR2NUM(csr)	(EDB_QRY2NUM(&((csr)->ecs_cname)))

/*
** miscellaneous defines
*/

# define ECS_BUFSIZE	400
# define ECS_NOSTMT_ERR	0
# define ECS_STMT_ERR	1			/* for ecs_stmt_err */


/*
** local data
*/

static ECS_CURSOR
		*ecs_cur_csr = NULL;		/* the current cursor */
static	OUT_STATE
		*ecs_outstate ZERO_FILL;
static	OUT_STATE
		*ecs_oldstate ZERO_FILL;
static ECS_CURSOR
		*ecs_first_csr = NULL;	/* ptr to head of cursor list */
static i4	ecs_stmt_err = ECS_NOSTMT_ERR;/* non-"cursor specific" errors */

/*
** local utility routines
*/

STATIC ECS_CURSOR *ecs_gcursor(char * cs_name, i4  use_def);
STATIC i4	ecs_buffer(char ch);
STATIC ECS_CURSOR *ecs_lookup(char * cs_name);
STATIC void	ecs_nm_add(void);
STATIC i4	ecs_isupdcsr(ECS_CURSOR *csr);
STATIC ECS_CURSOR *ecs_malloc(i4 size);
STATIC char	*ecs_stralloc(char *str);
STATIC char	*ecs_ctype(i4 typ);
STATIC char	*ecs_flags(char buf[], i4  flags);
STATIC char	*ecs_coflags(char buf[], i4  flags);
STATIC char	*ecs_why_rdo(i4 flags);
STATIC void	ecs_dyn(EDB_QUERY_ID *qry_id, char *cs_name);

/*
** Major Statements
*/

/*{
** Name: ecs_declcsr - Declare a cursor; allocate & initialize a csr descriptor.
** Inputs:
**	cs_name	- char *	- The name of the cursor
**	cs_sym	- SYM		- Symtab pointer; may be null
** Outputs:
**	None.
**	Returns:
**		Nothing.
**	Errors:
**		EcsDECNULL	- If cs_name is NULL or empty.
**		EcsREDECCSR	- If cs_name already exists.
** Description:
**	Allocates a new cursor descriptor, gets a unique name for it,
**	initializes all of its fields, links it into the cursor list,
**	and makes it the current cursor.
** Side Effects:
**	- Links the new cursor into the head of the active cursor list,
**	  pointed to by "ecs_first_csr".  Makes it the current cursor, pointed
**	  to by "ecs_cur_csr".
** Generates:
**	Nothing.
*/

void
ecs_declcsr( cs_name, cs_sym )
char	*cs_name;
SYM	*cs_sym;
{
    register ECS_CURSOR
		*csr;
    static char	*empty = ERx("??II");
    
    ecs_stmt_err = ECS_STMT_ERR;
    if (cs_name == NULL || *cs_name == '\0')
    {
	er_write( E_EQ0397_csDECNULL, EQ_ERROR, 0 );
	cs_name = empty;		/* Recover from empty strings */
    } else if (csr = ecs_lookup(cs_name))
    {
	er_write( E_EQ0402_csREDECCSR, EQ_ERROR, 1, cs_name );
	csr->ecs_cflags |= ECS_ERR;
	ecs_cur_csr = csr;
	return;
    }
    csr = ecs_malloc( sizeof(ECS_CURSOR) );
    if (cs_sym != (SYM *)0 && cs_name != empty)
    {
	ecs_dyn(&csr->ecs_cname, cs_name);
	csr->ecs_csym = cs_sym;
    }
    else
    {
	edb_unique( &csr->ecs_cname, cs_name );
	csr->ecs_csym = (SYM *)0;
    }
    csr->ecs_ctable = NULL;
    csr->ecs_cflags = CSR_DEF;
    csr->ecs_cnmvals = 0;
    csr->ecs_cfetched = 0;
    csr->ecs_ccols = NULL;
    csr->ecs_clisttyp = NULL;
    csr->ecs_cnmtables = 0;
    csr->ecs_cupdcols = 0;
    csr->ecs_cqry = NULL;
    csr->ecs_cnext = ecs_first_csr;
    ecs_first_csr = ecs_cur_csr = csr;
    ecs_stmt_err = ECS_NOSTMT_ERR;
    if (cs_name == empty)
	csr->ecs_cflags |= ECS_ERR;
} /* ecs_declcsr */

/*{
** Name: ecs_open - Generate code to open a cursor.
** Inputs:
**	cs_name	- char *	- The cursor name (must not be NULL).
**	ro_str	- char *	- The "read only" string (or variable).
**	sy	- SYM *		- If ro_str is a var, its sym ptr (else NULL).
**	is_dyn	- i4		- TRUE if cursor is dynamically-named.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCSR	- "cs_name" is NULL.
** Description:
**	- "cs_name" must not be NULL, empty, or invalid.
**	- Checks that a query is associated with the cursor and that there
**	  is no serious error for the cursor.
**	- Checks that declaration and use of cursor are compatible regarding
**	  static vs. dynamic name.  If lang is EQUEL and cursor was declared
**	  dynamically and referenced statically, this may mean that scope
**	  of cursor_name variable has been closed.  Therefore this routine
**	  updates the cursor state to generate static names so that code
**	  generator doesn't access an invalid SYM pointer.
**	- Adds in the "readonly" clause if needed.
** Side Effects:
**	- Makes the specified cursor the current one, by setting "cur_csr"
**	  to it.
**	- Sets/Clears the statement-level error flag "ecs_stmt_err".
** Generates:
**	IIcsOpen("cx",num1,num2);
**	IIwrite( "retrieve (res1=ty.val1, res2=tz.val2)" );
**	[IIwrite( "for readonly" ); | IIwrite( str_var_name );]
**	[IIcsQuery("cx",num1,num2);] -- not for ESQL
**
**	For dynamically-named cursors we generated variable
**	name instead of string constant; also cursor ids are 0, e.g.
**
**	IIcsOpen(csr_var, 0, 0);
**	IIwrite( ... );
**	IIcsQuery(csr_var, 0, 0);
**
** History:
**	18-apr-1988	- BUG 2164 - Modified to set csr to NULL (ncg)
**	08-feb-1989	- Added dynamic cursor names (bjb)
**	12-feb-2007 (dougi)
**	    Add support for scrollable cursors.
*/

void
ecs_open( cs_name, ro_str, sy, is_dyn )
char	*cs_name;
char	*ro_str;
SYM	*sy;
i4	is_dyn;
{
    register ECS_CURSOR
	    *csr = NULL;

    ecs_stmt_err = ECS_NOSTMT_ERR;
    if (ecs_cur_csr=ecs_gcursor(cs_name,FALSE))
    {
	csr = ecs_cur_csr;
	if (   (is_dyn && csr->ecs_csym == (SYM *)0)
	    || (!is_dyn && csr->ecs_csym != (SYM *)0)
	   )
	{
	    er_write( E_EQ0428_csCURSORNAME, EQ_ERROR, 1, cs_name ); 
	    if (dml_islang(DML_EQUEL) && !is_dyn)
		csr->ecs_csym = (SYM *)0;
	}
      /* No text in query must have been an error */
	if (csr->ecs_cqry == NULL)
	    return;	
      /* Repeat queries already handled by the repeat module. */
	if ((csr->ecs_cflags & ECS_REPEAT) == 0)
	{
	  /* add the arguments */
	    ecs_nm_add();		/* add <"csr_name", num, num> */
	    if (csr->ecs_cflags & ECS_SCROLL)
	    {
		/* For scrollable cursors, add parm and call csOpenScroll. */
		arg_str_add( ARG_CHAR, (csr->ecs_cflags & ECS_KEYSET) ?
		    "keyset scroll" : "scroll");
		gen_call( IICSOPENSCROLL );
	    }
	    else gen_call( IICSOPEN );
	}

      /* Set the run-time "for readonly" flag for repeat queries. */
	if (csr->ecs_cflags & ECS_REPEAT)
	{
	    if (ro_str)
	    {
		if (sy)
		{
		    arg_int_add( II_EXVARRDO );
		    arg_var_add( sy, ro_str );	/* ro_str is the var name */
		    gen_call( IICSRDO );
		} else
		{
		  /* If there's a string, it will always be "for readonly". */
		    arg_int_add( II_EXSETRDO );
		    arg_str_add( ARG_CHAR, (char *)0 );
		    gen_call( IICSRDO );
		}
	    }
	}

      /* Dump real text of query that was buffered away */
	if ((csr->ecs_cflags & ECS_ERR) == 0)
	    str_chget( csr->ecs_cqry, NULL, out_put_def );

      /* Repeat queries already handled by the repeat module. */
	if ((csr->ecs_cflags & ECS_REPEAT) == 0)
	{
	    if (ro_str)
	    {
		if (sy)
		{
		    db_op(ERx(" "));
		    db_var(DB_ID, sy, ro_str, (SYM *)0, (char *)0, (char *)0);
		} else
		    db_key( ro_str );
		db_send();
	    }
	    if (dml_islang(DML_EQUEL))		/* ESQL does this later */
	    {
		ecs_nm_add();
		gen_call( IICSQUERY );
	    }
	}
    } else
	ecs_stmt_err = ECS_STMT_ERR;

    /* If we've set the "readonly" flag, reset */
    if (csr && csr->ecs_cflags & ECS_REPEAT && ro_str)
    {
	arg_int_add( II_EXCLRRDO );
	arg_str_add( ARG_CHAR, (char *)0 );
	gen_call( IICSRDO );
    }
} /* ecs_open */

/*
** Name: ecs_close - Generate code to close a cursor.
** Inputs:
**	cs_name	- char *	- The cursor name (must not be NULL).
**	is_dyn	- i4		- Is a dynamically-named cursor.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
** Description:
**	"cs_name" must not be NULL, empty, or invalid.
** Side Effects:
**	- Clears the statement-level error flag "ecs_stmt_err".
**	- Makes this the current cursor.
** Generates:
**	IIcsClose("cx", num1, num2);
**
**	For dynamically-named cursors we generated variable
**	name instead of string constant; also cursor ids are 0, e.g.
**
**	IIcsClose(csr_var, 0, 0);
*/

void
ecs_close( cs_name, is_dyn )
char	*cs_name;
i4	is_dyn;
{
    register ECS_CURSOR
	    *csr = NULL;

    ecs_stmt_err = ECS_NOSTMT_ERR;
    if (ecs_cur_csr=ecs_gcursor(cs_name,FALSE))
    {
	csr = ecs_cur_csr;
	if (   (is_dyn && csr->ecs_csym == (SYM *)0)
	    || (!is_dyn && csr->ecs_csym != (SYM *)0)
	   )
	{
	    er_write( E_EQ0428_csCURSORNAME, EQ_ERROR, 1, cs_name ); 
	    if (dml_islang(DML_EQUEL) && !is_dyn)
		csr->ecs_csym = (SYM *)0;
	}
	ecs_nm_add();
	gen_call( IICSCLOSE );
    } else
	ecs_stmt_err = ECS_STMT_ERR;
} /* ecs_close */

/*{
** Name: ecs_retrieve - Generate code to set up for fetching data from a cursor.
** Inputs:
**	cs_name	- char *	- The cursor name (must not be NULL).
**	is_dyn	- i4		- Is a dynamically-named cursor.
**	fetch_o	- i4		- Fetch orientation for scrollable cursor or 0.
**	fetch_c	- i4		- Integer constant for ABSOLUTE/RELATIVE.
**	fetch_s	- SYM *		- Ptr to SYM thingy for hostvar name.
**	fetch_v	- char *	- Ptr to hostvar name with constant for
**				ABSOLUTE/RELATIVE or NULL.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
** Description:
**	- "cs_name" must not be NULL, empty, or invalid.
**	- We don't know anything about the columns until after the first
**	  retrieve.  If we have already fetched exactly one row, then we've
**	  saved away the types of the receiving variables in a linked list.
**	  Allocate an equivalent array, copy the list values into it, and
**	  free the list.  Clear the list pointer and set csr->ecs_nmvals.
** Side Effects:
**	- Sets/Clears the statement-level error flag "ecs_stmt_err".
**	- Makes this cursor the current one.
**	- Clears the count of "retrieve"d values for this row.
** Generates:
**	  if (IIcsRetrieve("cx", num1, num2) != 0) {
**
**	For dynamically-named cursors we generate variable
**	name instead of string constant; also cursor ids are 0, e.g.
**
**	  if (IIcsRetrieve(csr_var, 0, 0) != 0) {
**
** History:
**	12-feb-2007 (dougi)
**	    Add support for scrollable cursors.
**	08-mar-2008 (toumi01) BUG 119457
**	    For libq, set fetcho and fetchc to -1 for non-scrollable cursors.
*/

void
ecs_retrieve( cs_name, is_dyn, fetcho, fetchc, fetchs, fetchv )
char	*cs_name;
i4	is_dyn;
i4	fetcho, fetchc;
SYM	*fetchs;
char	*fetchv;
{
    register ECS_CURSOR
		*csr;
    
    ecs_stmt_err = ECS_NOSTMT_ERR;
    if (ecs_cur_csr=ecs_gcursor(cs_name,FALSE))
    {
	csr = ecs_cur_csr;
	if (csr->ecs_cflags & ECS_ERR)
	{
	    ecs_stmt_err = ECS_STMT_ERR;
	    return;
	}
	if (fetcho != 0 && !(csr->ecs_cflags & ECS_SCROLL))
	{
	    er_write( E_EQ0430_csNOTSCROLL, EQ_ERROR, 1, cs_name ); 
	    ecs_stmt_err = ECS_STMT_ERR;
	    return;
	}
	if (!(csr->ecs_cflags & ECS_SCROLL))
	{
	   /* Non-scrollable cursor has dummy args 4 & 5 to IIcsRetScroll */
	    fetcho = -1;
	    fetchc = -1;
	}
	if ((csr->ecs_cflags & ECS_SCROLL) && fetcho == 0)
	{
	    /* Scrollable cursor with plain ol' FETCH - make it 
	    ** FETCH NEXT. */
	    fetcho = fetNEXT;
	    fetchc = 1;
	}
	if (   (is_dyn && csr->ecs_csym == (SYM *)0)
	    || (!is_dyn && csr->ecs_csym != (SYM *)0)
	   )
	{
	    er_write( E_EQ0428_csCURSORNAME, EQ_ERROR, 1, cs_name ); 
	    if (dml_islang(DML_EQUEL) && !is_dyn)
		csr->ecs_csym = (SYM *)0;
	}
	csr->ecs_cfetched = 0;
      /* allocate room to hold the "fetch" types if we haven't already */
	if (csr->ecs_ctypes == NULL)
	{
	    if (csr->ecs_clisttyp)    /* no but already fetched a row */
	    {
		int		count = 0;
		register ECS_TYPE_LIST
				*tl,
				*last_tl;

	      /* how many values fetched? */
		for (tl=csr->ecs_clisttyp; tl; tl=tl->ecs_tlnext)
		    count++;
		csr->ecs_cnmvals = count;

		csr->ecs_ctypes = (u_i2 *) ecs_malloc( count*sizeof(u_i2) );
		last_tl = NULL;

		/*
		** Remembering that the ecs_clisttyp list is stored in reverse
		** order, we store the btypes into the ecs_ctypes array
		** backwards.
		*/
		for (tl=csr->ecs_clisttyp; tl; tl=tl->ecs_tlnext)
		{
		    if (last_tl)
			MEfree( (PTR)last_tl );	/* free the old descriptor */
		    csr->ecs_ctypes[--count] = tl->ecs_tltype;
		    last_tl = tl;
		}
		if (last_tl)
		    MEfree( (PTR)last_tl );
		csr->ecs_clisttyp = NULL;
	    }
	}
	ecs_nm_add();		/* add <"csr_name", num, num> */
	arg_int_add(fetcho);	/* add fetch orientation */
	if (fetchv)
	    arg_var_add(fetchs, fetchv);
	else arg_int_add(fetchc);	/* add fetch constant */
	gen_if( G_OPEN, IICSRETSCROLL, C_NOTEQ, 0 );
	/* gen_do_indent( 1 ); */
    } else
	ecs_stmt_err = ECS_STMT_ERR;
} /* ecs_retrieve */

/*{
** Name: ecs_delete - Generate code to delete the current row of given table.
** Inputs:
**	cs_name	 - char *	- The cursor name (NULL for current).
**	tbl_name - char *	- The table name
**	is_dyn	- i4		- Is a dynamically-named cursor.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**		EcsTBLUPD	- Cursor has more than one table.
**		EcsNOTUPD	- Cursor is not updatable.
**		EcsTBLBAD	- Bad table name in cursor.
** Description:
**	It's an error if the cursor is not updatable (which is always true
**	if more than one table was mentioned in the declare cursor statement).
**	For ESQL only, check that the table name is the one mentioned in the
**	cursor.
** Side Effects:
**	Database is changed.
** Generates:
**	IIcsDelete( tbl_name, "csr_name", num1, num2 );
**
**	For dynamically-named cursors we generated variable
**	name instead of string constant; also cursor ids are 0, e.g.
**
**	IIcsDelete( tbl_name, csr_var, 0, 0 );
**
** History:
**	11-aug-1986	- Modified from translated ESQL routines. (mrw)
**	07-jun-1988	- Modified ESQL generation to always pass the table
**			  name as gateways need a table name. (ncg)
*/

void
ecs_delete( cs_name, tbl_name, is_dyn )
char		*cs_name;
char		*tbl_name;
i4		is_dyn;
{
    register ECS_CURSOR
		*csr;

    ecs_stmt_err = ECS_NOSTMT_ERR;
    csr = ecs_cur_csr = ecs_gcursor(cs_name,FALSE);
    if (csr)
    {
	if (csr->ecs_cflags & ECS_ERR)
	{
	    ecs_stmt_err = ECS_STMT_ERR;
	    return;
	}

	if (   (is_dyn && csr->ecs_csym == (SYM *)0)
	    || (!is_dyn && csr->ecs_csym != (SYM *)0)
	   )
	{
	    er_write( E_EQ0428_csCURSORNAME, EQ_ERROR, 1, cs_name ); 
	    if (dml_islang(DML_EQUEL) && !is_dyn)
		csr->ecs_csym = (SYM *)0;
	}

	/*
	** EQUEL requires that a cursor be updatable in order to delete it.
	** A where clause in a variable is implicitly updatable, as the
	** update clause may have been tacked on at the end of the variable.
	** ESQL allows a cursor to be deleted as long as it is not readonly.
	*/
	if (dml_islang(DML_EQUEL))
	{
	    if ((csr->ecs_cflags & (ECS_CSRISUPD|ECS_WHERE))==0)
	    {
		er_write( E_EQ0400_csNOTUPD, EQ_ERROR, 2, ERx("DELETE"), 
			ECS_CSRNAME(csr) );
	     /* ecs_stmt_err = ECS_STMT_ERR;	/* go ahead and generate code */
	     /* return; */
	    }
	} else
	{
	    if (csr->ecs_cflags & ECS_CLAUSE_RDO)
	    {
		er_write( E_EQ0400_csNOTUPD, EQ_ERROR, 2, ERx("DELETE"), 
			ECS_CSRNAME(csr) );
	     /* ecs_stmt_err = ECS_STMT_ERR;	/* go ahead and generate code */
	     /* return; */
	    }
	}

	if (csr->ecs_cnmtables > 1)
	{
	    er_write( E_EQ0419_csTBLUPD, EQ_ERROR, 2, ERx("DELETE"), 
		    ECS_CSRNAME(csr) );
	 /* ecs_stmt_err = ECS_STMT_ERR;	/* go ahead and generate code */
	 /* return; */
	}

	if (dml_islang(DML_ESQL))
	{
	    char	*name = NULL;

	    /* Check the table name and always send it */
	    if (csr->ecs_ctable)
		name = csr->ecs_ctable->ecs_nlname;
	    if (name && (STbcompare(tbl_name, 0, name, 0, TRUE)) != 0 )
	    {
		er_write(E_EQ0427_csTBLBAD, EQ_ERROR, 2, tbl_name,
			 ERx("DELETE"));
	    }
	    arg_str_add(ARG_CHAR, tbl_name );
	}
	else
	{
	    /* EQUEL does not deal with a table name */
	    arg_str_add( ARG_CHAR, (char *)0 );
	}

	ecs_nm_add();
	gen_call( IICSDELETE );
    } else
	ecs_stmt_err = ECS_STMT_ERR;		/* error already issued */
} /* ecs_delete */

/*{
** Name: ecs_replace - Generate code to update the current row of given table.
** Inputs:
**	begin_end	- i4		- TRUE ==> begin, FALSE ==> end
**	cs_name		- char *	- The cursor name (must not be NULL).
**	is_dyn		- i4		- Is a dynamically-named cursor.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**		EcsNOTUPD	- Cursor is not updatable.
**		EcsTBLUPD	- Cursor has more than one table.
**		EcsCLMNUPD	- Cursor has no updatable columns.
** Description:
**	It's an error if the cursor is not updatable (which is always true
**	if more than one table was mentioned in the declare cursor statement).
** Side Effects:
**	- Sets/Clears the statement-level error flag "ecs_stmt_err".
** Generates:
**	IIcsReplace( csr_name, num1, num2 );
**	IIwritedb( text and variables );		-- Non-PARAM
**	IIparset( tgt_list, var_address, translation );	-- PARAM
**	IIcsEReplace( csr_name, num1, num2 );
** History:
**	18-apr-1988	- BUG 2428 - Cursor declared for Dynamic SQL statment
**			  should be considered updateable as columns are
**			  unknown. (ncg)
*/

void
ecs_replace( begin_end, cs_name, is_dyn )
i4		begin_end;
char		*cs_name;
i4		is_dyn;
{
    register ECS_CURSOR
		*csr;

    if (begin_end)
    {
	ecs_stmt_err = ECS_NOSTMT_ERR;
	csr = ecs_cur_csr = ecs_gcursor(cs_name,FALSE);
	if (csr)
	{
	    char	*cmd_name = ERx("REPLACE");	/* for error messages */

	    if (   (is_dyn && csr->ecs_csym == (SYM *)0)
		|| (!is_dyn && csr->ecs_csym != (SYM *)0)
	       )
	    {
		er_write( E_EQ0428_csCURSORNAME, EQ_ERROR, 1, cs_name ); 
		if (dml_islang(DML_EQUEL) && !is_dyn)
		    csr->ecs_csym = (SYM *)0;
	    }

	    if (dml_islang(DML_ESQL))
		cmd_name = ERx("UPDATE");

	    if (dml_islang(DML_EQUEL))	/* ESQL doesn't start replaces */
	    {
		ecs_nm_add();
		gen_call( IICSREPLACE );
	    }

	    if (csr->ecs_cflags & ECS_ERR)
	    {
		ecs_stmt_err = ECS_STMT_ERR;
		return;
	    }

	    /* Has UPDATE clause, run-time WHERE clause, or is SQL stmt name */
	    if ((csr->ecs_cflags & (ECS_CSRISUPD|ECS_WHERE|ECS_DYNAMIC))==0)
		return;

	    /* Has an UPDATE list or is SQL statement name */
	    /* E_EQ0418 is only applicable to EQUEL        tt62370 */
	    if ( dml_islang(DML_EQUEL) &&
    		((csr->ecs_cflags & (ECS_UPDLIST|ECS_DYNAMIC)) == 0) )
	    {
		er_write( E_EQ0418_csNOLISTUPD, EQ_ERROR, 1, ECS_CSRNAME(csr) );
		ecs_stmt_err = ECS_STMT_ERR;
		return;
	    }

	    if (csr->ecs_cnmtables > 1)
	    {
		er_write( E_EQ0419_csTBLUPD, EQ_ERROR, 2, cmd_name, 
			ECS_CSRNAME(csr) );
	     /* ecs_stmt_err = ECS_STMT_ERR;	/* go ahead and generate code */
		return;
	    }

	    /* If no columns specified, and not for SQL stmt name */
	    if (   csr->ecs_ccols == 0
	        && (csr->ecs_cflags & ECS_DYNAMIC) == 0)
		    er_write(E_EQ0412_csCLMNUPD, EQ_ERROR, 1, ECS_CSRNAME(csr));
	} else
	    ecs_stmt_err = ECS_STMT_ERR;
    } else	/* end replace */
    {
	/*
	** The grammar has generated IIwritedb calls during the body
	** of the REPLACE.  db_close will flush them and then generate
	** the IIcsEReplace call.
	*/
	db_send();
	ecs_nm_add();
	db_close( IICSEREPLACE );
    }
} /* ecs_replace */

/*
** Minor Statements
*/

/*{
**  Name: ecs_opquery - Emit call to IIcsQuery; ESQL only.
**
**  Description:
**	Emit "IIcsQuery(cursor_descriptor)".  Used only from ESQL.
**	This is a delayed part of EcsOpen.  Don't check for incompatible
**	use of static and dynamic cursor names because ecs_open has
**	already checked and given error.
**  Inputs:
**	cs_name	- char *	- The cursor name (may be NULL).
**
**  Outputs:
**	Returns:
**	    None.
**	Errors:
**	    EcsNOCURCSR	- if there isn't a current cursor
**
**  Side Effects:
**	
**  History:
**	31-mar-1987 - Written (mrw)
*/

void
ecs_opquery( cs_name )
char	*cs_name;
{
    if (ecs_stmt_err == ECS_STMT_ERR)
	return;
    if (ecs_gcursor(cs_name,TRUE))
    {
	ecs_nm_add();
	gen_call( IICSQUERY );
    } else
	ecs_stmt_err = ECS_STMT_ERR;
}

/*{
** Name: ecs_get - Generate code to retrieve one datum from current cursor.
** Inputs:
**	var_name	- char *	- Fully qualified name of variable
**	sy		- SYM *		- Symbol table pointer for variable
**	sy_btype	- i4		- The base type for the variable
**	ind_name	- char *	- Name of indicator variable.
**	indsy		- SYM *		- Symbol table pointer for indicator
**								variable.
**	arg		- char *	- Datahandler argument name.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**		EcsGETMANY	- More retrieve vars than columns.
**		EcsBADTYPE	- Type of retrieve var doesn't match column.
** Description:
**	- Neither var_name nor sy may be NULL.
**	- ecs_retrieve() MUST have already been called.  The fetch is
**	  silently ignored if you never called ecs_retrieve() for this cursor.
**	- If we don't know the types of the columns, then allocate a new
**	  type node, save the type in it, and attach it to the node list.
**	  The next ecs_retrieve will fill in the ecs_ctypes array and free up
**	  this node list.
**	- Otherwise this isn't the first retrieve for this cursor and so we
**	  know both the types of the columns and how many there are.  Check
**	  that we aren't retrieving too many and that the type of the receiving
**	  variable is compatible with the corresponding one from the first
**	  retrieve.
**	- Generate a call to IIcsGet to fetch the data.
** Side Effects:
**	- Counts the number of fetched vars for this row (in csr->ecs_cfetched).
**	- If there was no target list and this is the first row being retrieved
**	  then save the type of the retrieve var for later compatibility
**	  checking.  Sets cur_csr->ecs_clisttyp.
**	- Check for data-type compatibility with the column (or previous
**	  retrieve if column info unknown).
** Generates:
**	IIcsGet( isVar, type, len, &var_name );
** History:
**    10-jun-1993 (kathryn)
**       Add calls to esq_sqmods() around calls to user defined datahandlers.
**       The scope of the EQ_CHRPAD flag applies only to the file currently
**       being pre-processed. The datahandler being called may be in another
**       file.  Since IIsqMods changes the runtime behavior, we want to turn
**       it off before calling a handler and turn it back on after calling
**       the handler.  We want the handler to have whatever behavior was
**       set by the pre-processor flags when the handler was pre-processed.
**    25-jun-1993 (kathryn) 	Bug 52796.
**	 Include datahandlers in the target list count (ecs_clisttyp). 
*/

void
ecs_get( var_name, sy, sy_btype, ind_name, indsy, arg )
char	*var_name;
SYM	*sy;
i4	sy_btype;
char	*ind_name;
SYM	*indsy;
char	*arg;
{
    register ECS_TYPE_LIST
		*tl = NULL;
    register ECS_CURSOR
		*csr;
    u_i2	typ;

    if (ecs_stmt_err)
	return;

    csr = ecs_gcursor( (char *)NULL, TRUE );
    if (csr == NULL)
	return;

  /* check for type compatibility */
    switch (sy_btype)
    {
      case T_NONE:
	typ = ECS_TNONE;
	break;
      case T_BYTE:
      case T_PACKED:
      case T_INT:
      case T_FLOAT:
	typ = ECS_TNUMERIC;
	break;
      case T_CHAR:
      case T_VCH:
	typ = ECS_TCHAR;
	break;
      case T_HDLR:
	typ = ECS_THDLR;
	break;
      case T_UNDEF:
      default:
	typ = ECS_TUNDEF;
	break;
    }

    if (!csr->ecs_ctypes)	/* Don't know types (or how many) yet */
    {

        /*
        ** Link the new node into the HEAD, not the TAIL of the list.
        ** This is faster than finding the its tail.
        ** ecs_retrieve knows to walk backwards.
        */
        tl = (ECS_TYPE_LIST *) ecs_malloc( sizeof(*tl) );
        tl->ecs_tlnext = csr->ecs_clisttyp;
        csr->ecs_clisttyp = tl;
        tl->ecs_tltype = typ;

	if (typ == ECS_THDLR)
	{
	    /* Datahandler - Generates IILQldh_LoDataHandler */
 	    if (eq->eq_flags & EQ_CHRPAD)
            	esq_sqmods(IImodNOCPAD);
	    arg_int_add( 1 );
	    arg_var_add( indsy, ind_name );
	    arg_str_add( ARG_RAW, var_name );
	    arg_str_add( ARG_RAW, arg );
	    gen_call( IIDATAHDLR );
	    if (eq->eq_flags & EQ_CHRPAD)
                esq_sqmods(IImodCPAD);

	}
	else
	{
	    if (indsy != (SYM *)0)
	        arg_var_add( indsy, ind_name );
	    else
	        arg_int_add( 0 );
	    arg_var_add( sy, var_name );
	    gen_call( IICSGET );
	}
    } else if (csr->ecs_cfetched >= csr->ecs_cnmvals)  /* cuz nmvals is valid */
    {
      /* Only print this error once per row */
	if ((ecs_stmt_err==ECS_NOSTMT_ERR) && ((csr->ecs_cflags & ECS_ERR)==0))
	{
	    er_write( E_EQ0421_csGETMANY, EQ_ERROR, 1, ECS_CSRNAME(csr) );
	    /* ecs_stmt_err = ECS_STMT_ERR; /* go ahead and generate code */
	}
    } else
    {
	i4	i, val;

	if (typ == ECS_THDLR)
	{
	    /* Datahandler - Generates IILQldh_LoDataHandler */
	    if (eq->eq_flags & EQ_CHRPAD)
		esq_sqmods(IImodNOCPAD);
	    arg_int_add( 1 );
	    arg_var_add( indsy, ind_name );
	    arg_str_add( ARG_RAW, var_name );
	    arg_str_add( ARG_RAW, arg );
	    gen_call( IIDATAHDLR );
	    if (eq->eq_flags & EQ_CHRPAD)
		esq_sqmods(IImodCPAD);
        }
	else
	{
	    if (indsy != (SYM *)0)
	        arg_var_add( indsy, ind_name );
	    else
	        arg_int_add( 0 );
	    arg_var_add( sy, var_name );
	    gen_call( IICSGET );
	}
	i = csr->ecs_cfetched++;
	val = csr->ecs_ctypes[i];
	if (val != ECS_TEMPTY)
	{
	        if (typ != ECS_TUNDEF && val != ECS_TUNDEF && val != typ)
		    er_write( E_EQ0396_csBADTYPE, EQ_ERROR, 2, ECS_CSRNAME(csr),
			    var_name );
	} else
	    	csr->ecs_ctypes[i] = typ;
    }
} /* ecs_get */

/*{
** Name: ecs_eretrieve - Generate code to stop fetching data from a cursor.
** Inputs:
**	cs_name	- char *	- The cursor name (NULL for current).
**	uses_sqlda - i4	- TRUE ==> don't check number of retrieves
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**		EcsGETFEW	- Too few retrieves for this cursor.
** Description:
**	If there are more cursor arg-variables than were fetched
**	then we complain, but don't set ECS_ERR in the cursor.
**	Don't bother checking if the fetch used USING DESCRIPTOR desc_name
**	as it number of retrieve calls won't correspond to the number of
**	retrieves at runtime.
** Side Effects:
**	None.
** Generates:	IIcsERetrieve();
*/

void
ecs_eretrieve( cs_name, uses_sqlda )
char	*cs_name;
i4	uses_sqlda;
{
    register ECS_CURSOR
		*csr;

    if (ecs_stmt_err)
	return;

    if (csr=ecs_gcursor(cs_name,TRUE))
    {
	if (!uses_sqlda && (csr->ecs_cfetched < csr->ecs_cnmvals))
	{
	    if (ecs_stmt_err == ECS_NOSTMT_ERR)
	    {
		er_write( E_EQ0422_csGETFEW, EQ_ERROR, 1, ECS_CSRNAME(csr) );
	     /* ecs_stmt_err = ECS_STMT_ERR;	/* go ahead and generate code */
	    }
	}

	if (ecs_stmt_err == ECS_NOSTMT_ERR)
	{
	    gen_call( IICSERETRIEVE );
	    /* gen_do_indent( -1 ); */
	    gen_if( G_CLOSE, IICSRETRIEVE, C_0, 0 );
	}
    }
} /* ecs_eretrieve */

/*{
** Name: ecs_addtab - Add a table name to a (named or current) cursor.
** Inputs:
**	cs_name	- char *	- The cursor name (NULL for current).
**	tbl_name		- char *	- The table name.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**		EcsOBJNULL	- Tablename is NULL.
** Notes:
**	"tbl_name" must not be NULL or empty.
** Side Effects:
**	Adds entries onto the csr->ecs_ctable field.
*/

void
ecs_addtab( cs_name, tbl_name )
char	*cs_name;
char	*tbl_name;
{
    register ECS_CURSOR
		*csr;
    register ECS_NM_LIST
		*nm,
		*nl_p,
		*last_nm;
    
    if (ecs_stmt_err)
	return;

    if (csr=ecs_gcursor(cs_name,TRUE))
    {
	if (tbl_name == NULL)
	{
	    er_write( E_EQ0401_csOBJNULL, EQ_ERROR, 1, ECS_CSRNAME(csr) );
	    return;
	}

	last_nm = (ECS_NM_LIST *)0;
	for (nm=csr->ecs_ctable; nm; nm=nm->ecs_nlnext)
	{
	    last_nm = nm;
	  /* Don't add the same table more than once */
	    if (STbcompare(nm->ecs_nlname, 0, tbl_name, 0, TRUE) == 0)
		return;
	}
	nl_p = (ECS_NM_LIST *) ecs_malloc( sizeof(ECS_NM_LIST) );
	nl_p->ecs_nlname = ecs_stralloc( tbl_name );
	if (last_nm)
	    last_nm->ecs_nlnext = nl_p;
	else
	    csr->ecs_ctable = nl_p;
	csr->ecs_cnmtables++;
    }
} /* ecs_addtab */

/*{
** Name: ecs_colupd - Set/Check a column of a cursor as "for update".
** Inputs:
**	cs_name		- char *- The cursor name (NULL for current).
**	col_name	- char *- The column name
**	what		- i4	- ECS_ADD, ECS_CHK (ORed with option bits)
** Outputs:
**	Returns:
**		i4 - The previous value of the flags word.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**		EcsOBJNULL	- Col_name is NULL or empty.
**	
** Description:
**	- Options bits are:
**	  ECS_EXISTS	- Column exists
**	  ECS_EXPR	- Column is the result of an expression
**	  ECS_UPD	- Column is updatable
**	  ECS_MANY	- Column name is not unique
**	  ECS_CONSTANT	- Column is a constant
**	  ECS_ISWILD	- Column name is in a variable
**	- ECS_ADD adds the column name if it was not already present.
**	  If it was already there then we complain but don't set ECS_ERR,
**	  nor do we add the node again, but we do mark the existing
**	  column as having a non-unique name.  Always set ECS_EXISTS.
**	- ECS_CHK checks if it is there; if it is not then we complain
**	  (again without setting ECS_ERR).  It returns 0 if it wasn't there,
**	  or the old value of ecs_clflags if it was.
**	- ECS_ISWILD may be ORed in to mark the name as that of a variable
**	  (or '*'), not that of a column (in fact, we'll pass in the empty
**	  string in that case).  There are never any ADD conflicts with ISWILD
**	  names.
** Side Effects:
**	- Sometimes adds a node into the csr->ecs_ccols field.
**	- ECS_ADD and ECS_CHK will both OR in any extra option bits into the
**	  csr->ecs_ccols->ecs_clflags field.
*/

i4
ecs_colupd( cs_name, col_name, what )
char	*cs_name;
char	*col_name;
i4	what;
{
    register ECS_CURSOR
		*csr;
    register ECS_COL_LIST
		*col;
    ECS_COL_LIST
		*cur_col;
    i4		val;

    if (ecs_stmt_err)
	return 0L;

    if (csr=ecs_gcursor(cs_name,TRUE))
    {
	if (col_name == NULL)
	{
	    er_write( E_EQ0401_csOBJNULL, EQ_ERROR, 1, ECS_CSRNAME(csr) );
	    return 0L;
	}

      /* Walk the target column list. */
	cur_col = NULL;		/* Set to a matching column, if any. */
	for (col=csr->ecs_ccols; col; col=col->ecs_clnext)
	{
	    /*
	    ** Incoming ISWILDs match (only) an ISWILD; nothing else does.
	    ** The "else" clause is for a real column name coming in;
	    ** its "if" statement ensures that we never call STbcompare
	    ** on a variable (their names are, by convention, "").
	    */
	    if (what & ECS_ISWILD)	/* incoming */
	    {
		if (col->ecs_clflags & ECS_ISWILD)	/* list element */
		{
		    cur_col = col;			/* found it */
		    break;
		}
	    } else if ((col->ecs_clflags & ECS_ISWILD) == 0)
	    {
		if (STbcompare(col->ecs_clname,0,col_name,0,TRUE) == 0)
		{
		    cur_col = col;
		    break;
		}
	    }
	    if (col->ecs_clnext == NULL)	/* Stop on the last one. */
		break;
	}

	/*
	** Now:
	**    col == NULL	means there was no column list.
	**    cur_col == NULL	means that we didn't find the desired column.
	**			If we should add a new column then:
	**			    if col==NULL then
	**				make it the head of the column list
	**			    else
	**				add it to the tail of the list
	**				(pointed to by col).
	**			    end if
	**			end if
	**    cur_col != NULL	means that cur_col points to the desired node.
	*/

	switch ((i4)(what & (ECS_ADD|ECS_CHK)))
	{
	  case ECS_ADD:
	    if (cur_col)	/* found it -- conflict for column names */
	    {
		i4	options = 0;

		val = cur_col->ecs_clflags;

	      /* Never a conflict with variables. */
		if ((what & ECS_ISWILD) == 0)
		    options = ECS_MANY;
		cur_col->ecs_clflags |= options | (what & ~(ECS_ADD|ECS_CHK));
		return val;
	    }

	    if (col == NULL)		/* No column list yet. */
	    {
		csr->ecs_ccols =
		    (ECS_COL_LIST *) ecs_malloc( sizeof(ECS_COL_LIST) );
		col = csr->ecs_ccols;
	    } else			/* Add to the tail. */
	    {
		col->ecs_clnext =
		    (ECS_COL_LIST *) ecs_malloc( sizeof(ECS_COL_LIST) );
		col = col->ecs_clnext;
	    }
	    col->ecs_clname = ecs_stralloc( col_name );
	    col->ecs_clflags = ECS_EXISTS | (what & ~(ECS_ADD|ECS_CHK));
	    return 0L;

	  case ECS_CHK:
	    if (cur_col)		/* Found it. */
	    {
		val = cur_col->ecs_clflags;
		cur_col->ecs_clflags |= what & ~(ECS_ADD|ECS_CHK);
		return val;		/* the old flag value */
	    }

	  /* Not found.  Grammar must complain and/or add it if it wants to */
	    return 0L;
	} /* switch */
    } /* if (ecs_gcursor) */
    return 0L;
} /* ecs_colupd */

/*{
** Name: ecs_query - Set the query of a cursor to the passed query.
** Inputs:
**	cs_name	- char *	- The cursor name (NULL for current)
**	init	- i4		- {Start/Stop saving the query}/End the query
** Outputs:
**	Returns:
**		i4	- FALSE on any errors, TRUE on success.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
** Description:
**	- If "cs_name" is NULL then the current cursor is used.
**	- Only one query per cursor; we'll ignore (silently) any extras.
**	  No grammar should ever do this.
**	- If "init == ECS_START_QUERY", then allocate a new string table,
**	  and switch emit-output state to go to it.  Indent appropriately.
**	- If "init == ECS_STOP_QUERY", then switch back to normal output,
**	  and un-indent.
**	- If "init == ECS_END_QUERY", then forget the current cursor
**	  and reset the output stream.
**	- When this is called the grammar is about to parse the user's
**	  SELECT query.  The actual query is not sent till cursor OPEN time,
**	  therefore the query must be buffered.  By switching the code-emitter
**	  state to use the output state we indirectly buffer the complete query
**	  (within its host language calls) into the current cursor's ecs_cqry
**	  string table (see ecs_buffer). This string table with its query is
**	  dumped at OPEN time within the construct of an IF block.
** Side Effects:
**	Redirects output and associates it with this query.
** Generates:
**	Nothing.
*/

i4
ecs_query( cs_name, init )
char	*cs_name;
i4	init;
{
    register ECS_CURSOR
		*csr;
    i4		indent_val;

    if (ecs_stmt_err)
	return FALSE;	/* Attempt to ignore other errors */

    if (csr=ecs_gcursor(cs_name,TRUE))
    {
	if (csr->ecs_cflags & ECS_ERR)
	    return FALSE;

	/*
	** set the indent level -- block-structured languages indent
	** for the block too.
	*/
	switch (eq->eq_lang)
	{
	  case EQ_C:
	  case EQ_CPLUSPLUS:
	  case EQ_PASCAL:
	    indent_val = 1;
	    break;
	  case EQ_ADA:		/* Uses if/then/endif, not block */
	  case EQ_FORTRAN:
	  case EQ_COBOL:
	  case EQ_BASIC:
	  default:
	    indent_val = 0;
	    break;
	}

	switch (init)
	{
	  case ECS_START_QUERY:	/* Start saving query */
	   /* Allocate new string table for query, and force output into it */
	    if (csr->ecs_cqry == NULL)
	    {
		csr->ecs_cqry = str_new( ECS_BUFSIZE );
		ecs_outstate = out_init_state( ecs_outstate, ecs_buffer );
		ecs_oldstate = out_use_state( ecs_outstate );
	      /* Cursor qry may be dumped in an IF (COBOL hook for gen_term) */
		gen_term( FALSE );
		gen_do_indent( indent_val );
		ecs_cur_csr = csr;
	    } else
	    {
	     /* er_write( EcsDUPQRY, 1, ECS_CSRNAME(csr) ); */
		return FALSE;
	    }
	    break;

	  case ECS_STOP_QUERY:	/* Stop saving query */
	   /* Go back to using default output state */
	    _VOID_ out_use_state( ecs_oldstate );
	    ecs_oldstate = OUTNULL;
	    gen_do_indent( -indent_val );
	    break;
	  
	  case ECS_END_QUERY:	/* End current query */
	    ecs_cur_csr = NULL;
	    _VOID_ out_use_state( OUTNULL );	/* Back to default output */
	    break;
	}
	return TRUE;
    }
    return FALSE;
} /* ecs_query */

/*{
** Name: ecs_csrset - Set flag bits for a cursor, with error checking.
** Inputs:
**	cs_name	- char *	- The cursor name (NULL for current).
**	val	- i4		- The bits to set.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**		EcsUPDTABLES	- Can't update cursors with more than one table.
**		EcsUPDUNIQ	- Can't update UNIQUE or SORTed cursors.
**		EcsUPDNOCOL	- Can't update cursors w/ no updatable columns.
**		EcsREPEAT	- Can't include REPEATED query.
** Description:
**	OR the given bits into the "ecs_cflags" field of the specified cursor
**	after checking for violation of the update rules.
** Side Effects:
**	Modifies cursor.ecs_cflags.
*/

void
ecs_csrset( cs_name, val )
char	*cs_name;
i4	val;
{
    register ECS_CURSOR
		*csr;

    if (ecs_stmt_err)
	return;		/* Attempt to ignore other errors */

    if (!(csr=ecs_gcursor(cs_name,TRUE)))
	return;

    /*
    ** You can't update cursors with more than one table,
    ** or that are SORTed or UNIQUE, or that don't have any updateable
    ** columns.
    ** The ECS_WHERE bit says that there is a WHERE clause in a variable.
    ** We'll assume that it includes a FOR UPDATE clause, but not give
    ** an error message if that's a conflict, 'cuz we probably guessed
    ** wrong.  Backend will check more completely.
    */
    if (val & ECS_CSRISUPD)
    {
	if (csr->ecs_cnmtables > 1)
	{
	    val &= ~ECS_CSRISUPD;
	    if (!(val & ECS_WHERE))
		er_write( E_EQ0407_csUPDTABLES, EQ_ERROR, 1, ECS_CSRNAME(csr) );
	} else if (csr->ecs_cflags & ECS_CLAUSE_RDO)
	{
	    val &= ~ECS_CSRISUPD;
	    if (!(val & ECS_WHERE))
	    {
	      /* FUNCTION gets its own message, because it's not a keyword */
		if (csr->ecs_cflags & ECS_FUNCTION)
		    er_write(E_EQ0410_csUPDFUNC, EQ_ERROR, 1, ECS_CSRNAME(csr));
		else
		    er_write( E_EQ0408_csUPDUNIQ, EQ_ERROR, 2, ECS_CSRNAME(csr),
			    ecs_why_rdo(csr->ecs_cflags) );
	    }
	} else if (!ecs_isupdcsr(csr))
	{
#if 0
	    val &= ~ECS_CSRISUPD;
	    if (!(val & ECS_WHERE))
		er_write( E_EQ0409_csUPDNOCOL, EQ_ERROR, 1, ECS_CSRNAME(csr) );
#endif
	}
    }
    else if (val & ECS_REPEAT)
    {
	er_write(E_EQ0426_csREPEAT, EQ_ERROR, 0);
    }

    csr->ecs_cflags |= val;
} /* ecs_csrset */

/*{
**  Name: ecs_gcsrbits - Return the flag bits for the named cursor.
**
**  Description:
**	Returns the flag bits for the named cursor, or 0 on error.
**  Inputs:
**	cs_name		- The name of the cursor (NULL for current).
**
**  Outputs:
**	Returns:
**	    The flag bits (0 if error).
**	Errors:
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**  Side Effects:
**	None.
**  History:
**	01-apr-1987 - Written (mrw)
*/

i4
ecs_gcsrbits( cs_name )
char	*cs_name;
{
    register ECS_CURSOR
		*csr;

    if (ecs_stmt_err)
	return (i4)0;		/* Attempt to ignore other errors */

    if (!(csr=ecs_gcursor(cs_name,TRUE)))
	return (i4)ECS_NONEXIST;

    return csr->ecs_cflags;
}

/*{
** Name: ecs_namecsr - Return a pointer to the name of the current cursor.
** Inputs:
**	None.
** Outputs:
**	Returns:
**		A pointer to the name of the current cursor, or NULL
**		if there is no current cursor.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- No current cursor.
** Description:
**	Just return the saved name pointer from the current cursor.
** Side Effects:
**	None.
*/

char *
ecs_namecsr()
{
    register ECS_CURSOR
		*csr;

    if (!(csr=ecs_gcursor((char *)NULL,TRUE)))
	return (char *)0;
    return ECS_CSRNAME(csr);
} /* ecs_namecsr */

/*{
** Name: ecs_chkupd - Check that a column in a REPLACE CURSOR is updatable.
** Inputs:
**	cs_name		- char *	- The cursor name (NULL for current).
**	col_name	- char *	- The name of the column.
**	sy		- SYM *		- Symtab entry for the name (if a var).
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
**		EcsPARAMUPD	- Param replace, but no updatable columns.
**		EcsNOUPDCOLS	- Column name in a var, but no updatable cols.
**		EcsCOLUPD	- Column isn't updatable.
**		EcsCOLNOSUCH	- No such column in that cursor.
** Description:
**	If "col_name" is NULL, then this is a PARAM REPLACE.  Just check
**	that there is at least one updatable column.
**	Check for:
**	  1.	Column name is in a variable, but no updatable columns.
**	  2.	Column name is in the column list and is not updatable.
**		Note that the presence of a variable in the update list
**		makes all potentially updatable columns actually updatable.
**	  3.	Column name is not in the column list and there were no
**		variables in the column list.
**	  4.	Column name is not in the column list and there were no
**		updatable variables in the column list.
** Examples:
**	   ## char *s;
**	1. ## declare cursor ca for retrieve (a=b.i+b.j);
**	   ## replace cursor ca (s=4);		-- violates (1)
**	2. ## declare cursor ca for retrieve (a=b.i+b.j, t.k) for update of (k);
**	   ## replace cursor ca (a=4);		-- violates (2)
**	2a ## declare cursor ca for retrieve (t.a, t.b) for update of (s);
**	   ## replace cursor ca (a=4);		-- legal
**	3. ## declare cursor ca for retrieve (t.a, t.b) for update of (a, s);
**	   ## replace cursor ca (c=4);		-- violates (3)
**	4. ## declare cursor ca for retrieve (a=4, b=5, s) for update of (a, b);
**	   ## replace cursor ca (c=4);		-- violates (4)
** Side Effects:
**	None.
*/

void
ecs_chkupd( cs_name, col_name, sy )
char	*cs_name;
char	*col_name;
SYM	*sy;
{
    i4		stat;
    register ECS_CURSOR
		*csr;

    if (ecs_stmt_err)
	return;		/* Attempt to ignore other errors */

    if (!(csr=ecs_gcursor(cs_name,TRUE)))
	return;

    if (col_name == NULL)	/* PARAM REPLACE */
    {
	if (csr->ecs_cupdcols == 0)	/* but no updatable columns */
	    er_write( E_EQ0417_csPARAMUPD, EQ_ERROR, 1, ecs_namecsr() );
	return;
    }

    /*
    ** Check the case where the column name is in a variable separately
    ** so we don't give an error message like
    **	Column "my_column_variable" is not updatable.
    */
    if (sy)	/* REPLACE column name is in a variable */
    {
	if (csr->ecs_cupdcols == 0)	/* but no updatable columns */
	    er_write( E_EQ0415_csNOUPDCOLS, EQ_ERROR, 1, ecs_namecsr() );
	return;
    }

    stat = ecs_colupd( (char *)NULL, col_name, (i4)ECS_CHK );
    if (stat & ECS_EXISTS)		/* column name is known */
    {
	if ((stat & ECS_UPD) == 0)		/* but isn't updatable */
	    er_write( E_EQ0414_csCOLUPD, EQ_ERROR, 2, col_name, ecs_namecsr() );
    } else				/* column name not known */
    {
	i4		is_wild;

	is_wild = ecs_colupd( (char *)NULL, ERx(""), (i4)ECS_CHK|ECS_ISWILD );
	if ((is_wild & ECS_EXISTS) == 0)	/* variable in target list */
	    er_write(E_EQ0416_csCOLNOSUCH, EQ_ERROR, 2, col_name, ecs_namecsr());
	else if ((is_wild & ECS_UPD) == 0)	/* but isn't updatable */
	    er_write( E_EQ0414_csCOLUPD, EQ_ERROR, 2, col_name, ecs_namecsr() );
    }
} /* ecs_chkupd */

/*{
** Name: ecs_recover - Reset cursor state on error.
** Inputs:
**	None.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		None.
** Description:
**	Clear the output state and error flag.
** Side Effects:
**	None.
*/

void
ecs_recover()
{
    db_send();
    _VOID_ out_use_state( ecs_oldstate );
    ecs_oldstate = OUTNULL;
    ecs_stmt_err = ECS_NOSTMT_ERR;
    if (ecs_cur_csr)
    {
	ecs_cur_csr->ecs_cflags &= ~ECS_ERR;
	str_free( ecs_cur_csr->ecs_cqry, NULL );
    }
} /* ecs_recover */

/*{
** Name: ecs_fixupd - Make potentially updatable columns updatable, if needed.
** Inputs:
**	cs_name	- char *	- The cursor name (NULL for current).
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
** Description:
**	If there was a variable in the update list (marked by ECS_UPDVAR
**	in the cursor flags) then mark each potentially updatable column
**	as really being updatable, and clear the ECS_UPDVAR bit.
**	Notes that the cursor had an update list.
**	Count the number of updatable columns.
** Side Effects:
**	Changes the cursor and column flags.
*/

void
ecs_fixupd( cs_name )
char	*cs_name;
{
    register ECS_CURSOR
		*csr;
    register ECS_COL_LIST
		*col;
    i4		count = 0;
    i4		upd;

    if (ecs_stmt_err)
	return;		/* Attempt to ignore other errors */

    if (!(csr=ecs_gcursor(cs_name,TRUE)))
	return;

    upd = csr->ecs_cflags & ECS_UPDVAR;
    csr->ecs_cflags &= ~ECS_UPDVAR;
    csr->ecs_cflags |= ECS_UPDLIST;
    for (col=csr->ecs_ccols; col; col=col->ecs_clnext)
    {
	/*
	** If column is potentially updatable, but not already marked as
	** as updatable, mark it now.
	*/
	if (upd && ECS_COLNEWISUPD(col))
	    col->ecs_clflags |= ECS_UPD;
	if (col->ecs_clflags & ECS_UPD)
	    count++;
    }
    csr->ecs_cupdcols = count;
} /* ecs_fixupd */

/*{
** Name: ecs_clear - Clear all static cursor info and free all allocated memory.
** Inputs:
**	None.
** Outputs:
**	Returns:
**		None.
**	Errors:
**		None.
** Description:
**	- Walk through the cursor list, freeing all allocated memory
**	  (cursors and their associated pieces).  Reset all static variables
**	  (ecs_first_csr, ecs_cur_csr, ecs_outstate, ecs_stmt_err).
**	- This routine should be called at the beginning of processing an input
**	  file, since the scope of cursors is just the source file in which
**	  they are declared.  Clearing at the beginning of a file rather than
**	  at the end saves us the trouble of throwing away data structures
**	  for the last file.
** Side Effects:
**	Civilization, as we know it, will cease to exist.
*/

void
ecs_clear()
{
    register ECS_CURSOR
		*csr, *ncsr;
    register ECS_NM_LIST
		*nl, *nnl;
    register ECS_COL_LIST
		*col, *ncol;
    register ECS_TYPE_LIST
		*tl, *ntl;

    _VOID_ out_use_state( ecs_oldstate );
    ecs_oldstate = OUTNULL;

    for (csr=ecs_first_csr; csr; csr=ncsr)
    {
	ncsr = csr->ecs_cnext;
	for (nl=csr->ecs_ctable; nl; nl=nnl)
	{
	    nnl = nl->ecs_nlnext;
	    if (nl->ecs_nlname)
		MEfree( nl->ecs_nlname );
	    MEfree( (PTR)nl );
	}
	csr->ecs_ctable = NULL;
	for (col=csr->ecs_ccols; col; col=ncol)
	{
	    ncol = col->ecs_clnext;
	    if (col->ecs_clname)
		MEfree( col->ecs_clname );
	    MEfree( (PTR)nl );
	}
	csr->ecs_ccols = NULL;
	for (tl=csr->ecs_clisttyp; tl; tl=ntl)
	{
	    ntl = tl->ecs_tlnext;
	    MEfree( (PTR)tl );
	}
	csr->ecs_clisttyp = NULL;
	if (csr->ecs_cqry)
	    str_free( csr->ecs_cqry, NULL );
	csr->ecs_cqry = NULL;
	MEfree( (PTR)csr );
    }

  /* clear the static data */
    ecs_first_csr = NULL;
    ecs_cur_csr = NULL;
    ecs_outstate = NULL;
    ecs_stmt_err = ECS_NOSTMT_ERR;
} /* ecs_clear */

/*{
**  Name: ecs_coladd - Save away an UPDATE column name for later checking
**
**  Description:
**	This routine is called only from an ESQL UPDATE statement.
**	Save away a column name.  Later ecs_colrslv will be called,
**	either to toss all of the saved names if this turns out to be
**	a normal UPDATE, or to ecs_colupd each name if it turns out to be
**	a CURSOR UPDATE.
**
**  Inputs:
**	name		- The name to save.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
**
**  Side Effects:
**	
**  History:
**	18-mar-1987 - Written. (mrw)
*/

void
ecs_coladd( name )
char	*name;
{
}

/*{
**  Name: ecs_colrslv - Check/toss saved update column names
**
**  Description:
**	This routine is called only from an ESQL UPDATE statement.
**	If doit is TRUE, check all of the saved names (call
**	ecs_colupd(NULL, name, ECS_CHK) for each name in turn).
**	This may produce error messages.  In any case, toss all
**	of the saved names.  Doit will be TRUE only for CURSOR UPDATEs;
**	it will be FALSE for non-cursor UPDATEs.
**
**  Inputs:
**	doit		- TRUE means check the names, FALSE means toss them.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
**
**  Side Effects:
**	
**  History:
**	18-mar-1987 - Written. (mrw)
*/

void
ecs_colrslv( doit )
i4	doit;
{
}

/*{
** Name: ecs_dump - Dump the cursor data structures.
** Inputs:
**	None.
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		None.
** Description:
**	Prints a structured dump of the cursors to eq->eq_dumpfile.
**
**		("Name" is starred if it is the current one)
**
**		Cursor Table Dump:
**		Statement Error = TRUE/FALSE
**		Name=%s  Num1=%d Num2=%d Sym=%x NumVals=%d NumFetched=%d
**		    Flags=%s%x
**		    Query='%s'
**		    Tbl=%s  Tbl=%s ...
**		    Col=%s Btype=%s Flags=%s  Col=%s Btype=%s Flags=%s ...
**		    Typ=%s  Typ=%s ...
**		Temp Type List:
**		    Type=%s  Type=%s ...
**		Name=%s  Num1=%d Num2=%d Sym=%x NumVals=%d NumFetched=%d
**		    Flags=%s%x
**		    Query='%s'
**		    Tbl=%s  Tbl=%s ...
**		    Col=%s Btype=%s Flags=%s  Col=%s Btype=%s Flags=%s ...
**		    Typ=%s  Typ=%s ...
**		Temp Type List:
**		    Type=%s  Type=%s ...
**		...
** Side Effects:
**	None.
*/

i4
ecs_dump()
{
    register ECS_CURSOR
		*csr;
    register ECS_COL_LIST
		*col;
    register ECS_NM_LIST
		*nl;
    register ECS_TYPE_LIST
		*tl;
    register FILE
		*out_fp = eq->eq_dumpfile;
    char	buf[128];
    void	ecs_print();
    char	*ecs_flags(), *ecs_coflags();

    SIfprintf(out_fp, ERx("\n------------------------------------------------\n"));
    SIfprintf( out_fp, ERx("Cursor Table Dump:\n") );
    SIfprintf( out_fp, ERx(" [Statement Error = %s]\n"),
	ecs_stmt_err ? ERx("TRUE"):ERx("FALSE") );

    for (csr=ecs_first_csr; csr; csr=csr->ecs_cnext)
    {
	register i4	i;

	SIfprintf(out_fp, ERx("------------------------------------------------\n"));
	SIfprintf( out_fp, ERx("%c"), (ecs_cur_csr==csr ? '*' : ' ') );
	SIfprintf( out_fp,
	    ERx("Name='%s' Num1=0x%x Num2=0x%x Sym=0x%x NumVals=%d NumUpdCols=%d\n"),
	    ECS_CSRNAME(csr), ECS_CSR1NUM(csr), ECS_CSR2NUM(csr),
	    csr->ecs_csym, csr->ecs_cnmvals, csr->ecs_cupdcols );
	SIfprintf( out_fp, ERx("  NumFetched=%d Flags=%s\n"),
	    csr->ecs_cfetched, ecs_flags(buf, csr->ecs_cflags) );
	if (csr->ecs_cqry)
	    str_dump( csr->ecs_cqry, ERx("Cursor Query") );
	if (nl=csr->ecs_ctable)
	{
	    for (; nl; nl=nl->ecs_nlnext)
		SIfprintf( out_fp, ERx("    Tbl=%-8s"), nl->ecs_nlname );
	    SIfprintf( out_fp, ERx("\n") );
	}
	if (col=csr->ecs_ccols)
	{
	    for (; col; col=col->ecs_clnext)
		SIfprintf( out_fp, ERx("    Col=%s, Flags=%s\n"),
		    col->ecs_clflags & ECS_ISWILD ? ERx("\"\"") : col->ecs_clname,
		    ecs_coflags(buf, col->ecs_clflags) );
	}
	if (csr->ecs_ctypes)
	{
	    for (i=csr->ecs_cnmvals-1; i>=0; i-- )
		SIfprintf( out_fp,
		    ERx("    Typ=%s"), ecs_ctype((i4)csr->ecs_ctypes[i]) );
	    SIfprintf( out_fp, ERx("\n") );
	}
	if (tl = csr->ecs_clisttyp)
	{
	    SIfprintf( out_fp, ERx("  Temp Type List:\n") );
	    for (; tl; tl=tl->ecs_tlnext)
		SIfprintf( out_fp,
			ERx("    Typ=%s"), ecs_ctype((i4)tl->ecs_tltype) );
	    SIfprintf( out_fp, ERx("\n") );
	}
    }
    SIfprintf(out_fp, ERx("------------------------------------------------\n"));
    SIfprintf( out_fp, ERx("\n") );
    SIflush( out_fp );
    return(0);
} /* ecs_dump */

/*{
** Name: ecs_id_curcsr - Return a pointer to the current cursor query id.
** Inputs:
**	None.
** Outputs:
**	Returns:
**		EDB_QUERY_ID *	- A pointer to the current query id,
**				  or NULL if none.
**	Errors:
**		None.
** Description:
**	Return NULL if there is no current cursor, else return the address
**	of its query id.
**	Called by the grammar (EQGRAM.Y).
** Side Effects:
**	None.
*/

EDB_QUERY_ID *
ecs_id_curcsr()
{
    if (ecs_cur_csr == NULL)
	return NULL;
    return &ecs_cur_csr->ecs_cname;
} /* ecs_id_curcsr */

/*
** Local Utilities
*/

/*{
** Name: ecs_gcursor - (local) Get the desired cursor from its name.
** Inputs:
**	cs_name	- char *	- The cursor name (NULL for default).
**	use_def	- i4		- If TRUE and !cs_name, use default csr.
** Outputs:
**	Returns:
**		ECS_CURSOR *	- The cursor pointer (NULL for error).
**	Errors:
**		EcsCSRNULL	- cs_name is empty.
**		EcsNOCSR	- cs_name is an invalid cursor name.
**		EcsNOCURCSR	- cs_name is NULL and there is no current csr.
** Description:
**	Lookup the name and print a message if there is an error.
** Side Effects:
**	None.
*/

STATIC ECS_CURSOR *
ecs_gcursor( cs_name, use_def )
char	*cs_name;
i4	use_def;
{
    register ECS_CURSOR
		*csr = NULL;
    
    if (cs_name)
    {
	if (*cs_name)
	    csr = ecs_lookup( cs_name );
	else
	    er_write( E_EQ0420_csCSRNULL, EQ_ERROR, 0  );
    } else if (use_def)
	csr = ecs_cur_csr;

    if (csr == NULL)
    {
	if (cs_name && *cs_name)
	    er_write( E_EQ0398_csNOCSR, EQ_ERROR, 1, cs_name );
	else if (cs_name == NULL && ecs_stmt_err == ECS_NOSTMT_ERR)
	    er_write( E_EQ0399_csNOCURCSR, EQ_ERROR, 0 );
    }
    return csr;
} /* ecs_gcursor */

/*{
** Name: ecs_buffer - (local) Buffer characters that are written by the code
**				emitter to a cursor query string table buffer.
** Inputs:
**	ch - char 	- Character to buffer.
** Outputs:
**	Returns:
**		i4 	- Zero (unused).
**	Errors:
**		EcsNOCURCSR	- No current cursor.
** Description:
**	Indirectly called via the code emitter state.
** Side Effects:
**	- On error the output state is reset to normal; the input char
**	  is then re-emitted.
**	- Normally the char is just added to the query buffer.
*/

STATIC i4
ecs_buffer( char ch )
{
    if (!ecs_cur_csr)
    {
	static char	buf[] = ERx("x");

	er_write( E_EQ0399_csNOCURCSR, EQ_ERROR, 0 );
	_VOID_ out_use_state( ecs_oldstate );	/* reset output state */
	ecs_oldstate = OUTNULL;
	buf[0] = ch;
	out_emit( buf );
    } else if (ch != '\0')
	str_chadd( ecs_cur_csr->ecs_cqry, ch );
    return 0;
} /* ecs_buffer */

/*{
** Name: ecs_lookup - (local) Find the cursor associated with a cursor name.
** Inputs:
**	cs_name	- char *	- The cursor name
** Outputs:
**	Returns:
**		ECS_CURSOR *	- NULL for error, else address of the cursor.
**	Errors:
**		None.
** Description:
**	- Linear search through the cursor list.
**	- ecs_gcursor guarantees that the cs_name is non-NULL and points
**	  to a non-empty name.
** Side Effects:
**	None.
*/

STATIC ECS_CURSOR *
ecs_lookup( cs_name )
char	*cs_name;
{
    register ECS_CURSOR
		*csr;

    for (csr=ecs_first_csr; csr; csr=csr->ecs_cnext )
    {
	if (STbcompare(cs_name,0,ECS_CSRNAME(csr),0,TRUE) == 0)
	    return csr;
    }

    return NULL;
} /* ecs_lookup */

/*{
** Name: ecs_nm_add - (local) Add the internal name of the current cursor as
**			ARG arguments.
** Inputs:
**	None.
** Outputs:
**	Returns:
**		None.
**	Errors:
**		None.
** Description:
**	Adds the cursor name and the cursor ids to the arg-manager's gen-
**	function-call arg list.  For dynamically-named cursors, the name
**	will be represnted by the variable name:
**	    (cursor_var_name, 0, 0)
**	For statically-named cursors, the name will be in quotes:
**	    ("cursor_name", number_one, number_two).
** Side Effects:
**	The arguments will appear in the argument list of the next generated
**	function call.
**	
**  History:
*/

STATIC void
ecs_nm_add()
{
    register ECS_CURSOR
		*csr;

    if (!(csr=ecs_gcursor((char *)NULL,TRUE)))
    {
	arg_str_add( ARG_CHAR, (char *)0 );
	arg_int_add( 0 );
	arg_int_add( 0 );
	return;
    }
    if (csr->ecs_csym != (SYM *)0)
	arg_var_add( csr->ecs_csym, ECS_CSRNAME(csr) );
    else
	arg_str_add( ARG_CHAR, ECS_CSRNAME(csr) );
    arg_int_add( ECS_CSR1NUM(csr) );
    arg_int_add( ECS_CSR2NUM(csr) );
} /* ecs_nm_add */

/*{
** Name: ecs_isupdcsr - (local) Is this an updatable cursor?
** Inputs:
**	csr	- ECS_CURSOR *	- The cursor to check.
** Outputs:
**	None.
**	Returns:
**		i4	- 1 iff this cursor has potentially updatable columns,
**			  else 0.
**	Errors:
**		None.
** Description:
**	A column is potentially updatable if it is named, is not an expression
**	or a constant, and no other column in this cursor has the same name.
**	If it is a variable then we know nothing about it and assume that
**	it is potentially updatable.
**
**	Walk the target list, returning 1 if we find a potentially updatable
**	column.  If we find none, return 0.
** Side Effects:
**	None.
*/

STATIC i4
ecs_isupdcsr( csr )
ECS_CURSOR	*csr;
{
    register ECS_COL_LIST
		*col;

    for (col=csr->ecs_ccols; col; col=col->ecs_clnext)
    {
	if (ECS_COLISUPD(col))
	    return 1;
	if (col->ecs_clflags & ECS_ISWILD)
	    return 1;
    }
    return 0;
} /* ecs_isupdcsr */

/*{
** Name: ecs_malloc - (local) Get memory and exit if we fail.
** Inputs:
**	size	- i4		- How many bytes of memory to allocate.
** Outputs:
**	Returns:
**		char *		- A pointer to the allocated space.
**	Errors:
**		EeqNOMEM	- MEreqmem failed to acquire memory.
** Description:
**	Let MEreqmem do all of the work -- have it clear memory first.
** Side Effects:
**	- The allocated memory is cleared.
*/

STATIC ECS_CURSOR *
ecs_malloc( size )
i4	size;
{
    ECS_CURSOR	*p;

    if ((p = (ECS_CURSOR *)MEreqmem((u_i2)0, (u_i4)size,
	TRUE, (STATUS *)NULL)) != NULL)
    {
	return(p);
    }

    er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2, er_na(size), ERx("ecs_malloc()") );
    /* NOTREACHED */
} /* ecs_malloc */

/*{
** Name: ecs_stralloc - (local) Stash a string away in allocated memory.
** Inputs:
**	str	- char *	- The string to save.
** Outputs:
**	Returns:
**		char *	- A pointer to the stored string.
**	Errors:
**		None.
** Description:
**	Use ecs_malloc to get storage (and check for errors).
**	Copy the string to the new area.
** Side Effects:
**	Dynamic memory used.
*/

STATIC char *
ecs_stralloc( str )
char	*str;
{
    i4		len;
    char	*mem;

    len = STlength( str );
    mem = (char *) ecs_malloc( len+1 );
    STcopy( str, mem );
    return mem;
} /* ecs_stralloc */

/*{
** Name: ecs_ctype - (local) Return a string version of the given cursor btype.
** Inputs:
**	typ	- i4	- The type to translate.
** Outputs:
**	Returns:
**		char *	- A pointer to the string.
**	Errors:
**		None.
** Description:
**	Look the number up in a table and return its name.  If the number
**	is not in the table, return its decimal string.  The returned
**	pointer is always to static data.
** Side Effects:
**	None.
*/

STATIC char *
ecs_ctype( typ )
i4	typ;
{
    static struct type_names {
	i4	tn_val;
	char	*tn_name;
    } typ_names[] = {
	{ECS_TEMPTY,	ERx("EMPTY")},
	{ECS_TNONE,	ERx("NONE")},
	{ECS_TNUMERIC,	ERx("NUMERIC")},
	{ECS_TCHAR,	ERx("CHAR")},
	{ECS_TUNDEF,	ERx("UNDEF")},
	{0,		NULL}
    };
    static char	buf[32];
    register struct type_names
		*tn;
    
    for (tn=typ_names; tn->tn_name; tn++)
	if (tn->tn_val == typ)
	    return tn->tn_name;
    STprintf( buf, ERx("%d"), typ );
    return buf;
} /* ecs_ctype */

/*{
** Name: ecs_flags - (local) Decode a cursor's flag bits as a printable string.
** Inputs:
**	flags	- i4	- The flag bits to decode
*	buf	- char *- Destination buffer for the string.
** Outputs:
**	Returns:
**		char *	- A pointer to the printable string.
**	Errors:
**		None.
** Description:
**	Look through a table of bits; add the string for any that match.
** Side Effects:
**	None.
*/

STATIC char *
ecs_flags( buf, flags )
char	buf[];
i4	flags;
{
    static struct flag_bits {
	i4	fb_val;
	char	*fb_char;
    } bits[] = {
	{ ECS_ERR,	ERx("ERROR") },
	{ ECS_REPEAT,	ERx("REPEAT") },
	{ ECS_DIRECTU,	ERx("DIRECT") },
	{ ECS_DEFERRU,	ERx("DEFERRED") },
	{ ECS_UPDATE,	ERx("UPDATE") },
	{ ECS_RDONLY,	ERx("READONLY") },
	{ ECS_UNIQUE,	ERx("UNIQUE") },
	{ ECS_SORTED,	ERx("SORT") },
	{ ECS_WHERE,	ERx("WHERE") },
	{ ECS_UPDVAR,	ERx("UPDVAR") },
	{ ECS_UPDLIST,	ERx("UPDLIST") },
	{ ECS_DYNAMIC,	ERx("DYNAMIC") },
	{ ECS_UNION,	ERx("UNION") },
	{ ECS_GROUP,	ERx("GROUP_BY") },
	{ ECS_HAVING,	ERx("HAVING") },
	{ ECS_FUNCTION,	ERx("FUNCTION") },
	{ 0,		'\0' }
    };
    register struct flag_bits
		*fb;
    register char
		*p = buf;
    char	*comma = ERx("");

    if (flags == 0)
	return ERx("NONE");
    p = buf;
    for (fb=bits; fb->fb_val; fb++)
    {
	if (flags & fb->fb_val)
	{
	    STprintf( p, ERx("%s%s"), comma, fb->fb_char );
	    p += STlength(comma) + STlength(fb->fb_char);
	    comma = ERx(",");
	}
    }
    return buf;
} /* ecs_flags */
 
/*{
** Name: ecs_coflags - (local) Decode a column's flag bits as printable string.
** Inputs:
**	flags	- i4	- The flag bits to decode
** Outputs:
**	Returns:
**		char *	- A pointer to the printable string.
**	Errors:
**		None.
** Description:
**	Look through a table of bits; add the string for any that match.
**	The string is static and will be overwritten on the next call.
** Side Effects:
**	None.
*/

STATIC char *
ecs_coflags( buf, flags )
char	buf[];
i4	flags;
{
    static struct flag_bits {
	i4	fb_val;
	char	*fb_char;
    } bits[] = {
	{ ECS_EXISTS,	ERx("EXISTS") },
	{ ECS_EXPR,	ERx("EXPR") },
	{ ECS_UPD,	ERx("UPDATE") },
	{ ECS_MANY,	ERx("MANY") },
	{ ECS_CONSTANT,	ERx("CONSTANT") },
	{ ECS_ISWILD,	ERx("ISWILD") },
	{ 0,		'\0' }
    };
    register struct flag_bits
		*fb;
    register char
		*p = buf;
    char	*comma = ERx("");

    if (flags == 0)
	return ERx("NONE");
    p = buf;
    for (fb=bits; fb->fb_val; fb++)
    {
	if (flags & fb->fb_val)
	{
	    STprintf( p, ERx("%s%s"), comma, fb->fb_char );
	    p += STlength(comma) + STlength(fb->fb_char);
	    comma = ERx(",");
	}
    }
    *p = '\0';
    return buf;
} /* ecs_coflags */

/*{
** Name: ecs_why_rdo - (local) Why is this cursor readonly?
** Inputs:
**	flags	- i4	- The flag bits to decode
** Outputs:
**	Returns:
**		char *	- A pointer to the printable string.
**	Errors:
**		None.
** Description:
**	Look through a table of bits; return the name of the first matching
**	bit.
** Side Effects:
**	None.
*/

STATIC char *
ecs_why_rdo( flags )
i4	flags;
{
    static struct flag_bits {
	i4	fb_val;
	char	*fb_equel;
	char	*fb_esql;
    } bits[] = {
	{ ECS_UNIQUE,	ERx("UNIQUE"),	ERx("DISTINCT") },
	{ ECS_SORTED,	ERx("SORT"),	ERx("ORDER BY") },
	{ ECS_UNION,	ERx("UNION"),	ERx("UNION") },
	{ ECS_GROUP,	ERx("BY"),	ERx("GROUP BY") },
	{ ECS_HAVING,	ERx("HAVING"),	ERx("HAVING") },
	{ 0,		'\0' }
    };
    register struct flag_bits
		*fb;

    for (fb=bits; fb->fb_val; fb++)
    {
	if (flags & fb->fb_val)
	    return dml_islang(DML_ESQL) ? fb->fb_esql : fb->fb_equel;
    }
    return ERx("UNKNOWN");		/* huh? */
} /* ecs_why_rdo */

/*{
** Name: ecs_dyn - (local) Set cursor ids for dynamically-named csr.
** Inputs:
**	qry_id	- EDB_QUERY_ID *	- The cursor id
**	cs_name	- char *	- The cursor name
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		None.
** Description:
**	- Set cursor ids to zero and copy name. 
** Side Effects:
**	None.
*/

void
ecs_dyn( qry_id, cs_name )
EDB_QUERY_ID	*qry_id;
char		*cs_name;
{
    STlcopy( cs_name, qry_id->edb_qry_name, EDB_QRY_MAX );
    qry_id->edb_qry_id[0] = 0;
    qry_id->edb_qry_id[1] = 0;
} /* ecs_dyn */

