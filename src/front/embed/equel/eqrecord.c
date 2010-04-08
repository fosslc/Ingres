/*
** Copyright (c) 2004 Ingres Corporation
*/

# include 	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<si.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqscan.h>
# include	<eqrun.h>
# include	<equel.h>
# include	<eqlang.h>
# include	<equtils.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqstmt.h>
# include	<ereq.h>

/**
+*  Name: eqrecord.c - Handles record assignments and null indicator arrays.
**
**  Description:
**	Some ESQL statements allow the use of host records for assignments.
**	Together with those host records, there may also be arrays of null
**	indicators. This file provides the routines to collect record elements
**	while parsing and generate specific calls for each record element
**	(plus the null indicator).  This also abstracts the operations of
**	the main ESQL grammar from the host language grammar, which should
**	know about records but not the associated operations.
**
**	For the dynamic select statements using a descriptor (8.0), we use
**	this module to save the name of the descriptor until we are ready
**	to generate the descriptor name in a call.
**
**  Defines:
**	erec_setup	- Initialize record element list
**	erec_mem_add	- Add a record member
**	erec_ind_add	- Add an indicator variable
**	erec_use	- Generate/use the record members
**	erec_vars	- Are there any record members ?
**	erec_desc	- Save/process the name of user's descriptor 
**	erec_arg_add	- Add a datahandler argument
**
**  Notes:
**	Example:
**
**	struct {
**		int	i;
**		char	buf[10];
**	} s, t;
**	short   inds[2], tinds[2];
**
**	EXEC SQL SELECT * INTO :s:inds, :t:tinds FROM emp;
**	EXEC SQL INSERT INTO emp VALUES (:s:inds);
**
**	Calls:
**
**						 - SELECT
**	  erec_setup(1);			 - Clear local storage
**	  erec_mem_add("s.i", i_sym, T_INT);	 - Add two members
**	  erec_mem_add("s.buf", buf_sym, T_CHAR);
**	  if (erec_vars() > 0)			 - Add indicator array
**	    erec_ind_add(0, "inds", "[%d]", ind_sym); - (0 for C, 1 for BASIC)
**	  erec_setup(0);			 - New variable
**	  erec_mem_add("t.i", ti_sym, T_INT);	 - Add two members
**	  erec_mem_add("t.buf", tbuf_sym, T_CHAR);
**	  erec_setup(0);			 - End of list
**	  if (erec_vars() > 0)			 - Add indicator array
**	    erec_ind_add(0, "tinds", "[%d]", tind_sym); - (0 for C, 1 for BASIC)
**	  erec_use(IIRETDOM, FALSE, (char *)0); - Generate IIretdom calls 
**
**						 - INSERT
**	  erec_setup(1);			 - Clear local storage
**	  erec_mem_add("s.i", i_sym, T_INT);	 - Add two members
**	  erec_mem_add("s.buf", buf_sym, T_CHAR);
**	  if (erec_vars() > 0)			 - Add indicator array
**	    erec_ind_add(0, "inds", "[%d]", ind_sym);
**	  erec_use(IISETDOM, FALSE, ",");	 - Generate IIsetdom calls 
**
**	Example of use with dynamic select statement:
**
**	EXEC SQL EXECUTE IMMEDIATE :select_stmt USING DESCRIPTOR d;
**
**	erec_desc("d");			- Save 'd'
**	erec_desc((char *)0);		- Generate IIcsDaGet call
-*
**  History:
**    	06-mar-1987	- Written (ncg)
**	08-sep-1987	- Extended for dynamic select statement (bjb)
**	09-nov-1992	- Added a new member to REC_ELEM.  Added new routine
**			  erec_arg_add to add datahandler argument.  Also
**			  added new parameters to ret_add calls. (lan)
**	16-nov-1992 (lan)
**		Added history at appropriate places.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	09-may-1994 (geri)
**		Bug 63226: use macro eq_genlang() to generate the host
**		language arguments to runtime calls so that the C++
**		language is specified correctly to the runtime layer as C
**		rather than C++.
**	15-mar-1996 (thoda04)
**		Cleaned up compile warnings.
**/


/*}
+*  Name: REC_ELEM - Structure to use with the queue manager.
**
**  Description:
**	This structure stores enough information to keep track of record
**	members and their corresponding indicator variables.
-*
**  History:
**    	06-mar-1987	- Written (ncg)
**	09-nov-1992 (lan)
**		Added new member rec_arg.
*/

typedef struct {
	Q_ELM		rec_qlink;	/* Queue manager stuff */
	char		*rec_mem;	/* Variable name for record member */
	i4		rec_count;	/* How many members of this struct */
	SYM		*rec_msym;	/* Symbol table entry for variable */
	char		*rec_ind;	/* Name of indicator variable */
	SYM		*rec_isym;	/* Symbol table entry for indicator */
	i4		rec_typ;	/* Symtab base type */
	i4		rec_ishead;	/* Am I the head of a struct? */
	char		*rec_arg;	/* for use in datahandler only */
} REC_ELEM;

/*}
+*  Name: REC_DESC - Local record element queue descriptor.
**
**  Description:
**	This structure maintains the record member queue, and the number
**	of members in the queue.
**	rec_current points to the current variable, or the first member
**	of the current structure (if the current variable is a structure),
**	or is NULL if we've finished with the current variable and haven't
**	started a new one yet.
-*
**  History:
**    	06-mar-1987	- Written (ncg)
*/
static struct _rec_desc {
    Q_DESC	rec_q;			/* Queue descriptor */
    REC_ELEM	*rec_current;		/* Current node */
    i4		rec_vars;		/* Number of variables */
} rec_desc = {{0}, NULL, -1};		/* -1 flags not yet initialized */

/*
static REC_ELEM *erec_endq();
*/


/* Place to save descriptor name */
static char erec_dyn_desc[SC_WORDMAX +1];

/*{
+*  Name: erec_setup - Initialize local storage
**
**  Description:
**	Called just before the first element in a structure this routine
**	initializes local storage of record members.  In case of syntax errors
**	local data may have been left around from the last record-style
**	statement.
**
**  Inputs:
**	first_time	- Completely initialize or mark start of struct (1/0)
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**    	06-mar-1987	- Written (ncg)
*/

void
erec_setup( first_time )
i4	first_time;
{
    if (first_time)
    {
	if (rec_desc.rec_vars < 0)		/* No queue allocated yet */
	{
	    q_init(&rec_desc.rec_q, sizeof(REC_ELEM));
	}
	else				/* Free rest of queue */
	{
	    if (q_first((REC_ELEM *), &rec_desc.rec_q))
		q_free(&rec_desc.rec_q, NULL);
	}
	rec_desc.rec_vars = 0;
    }
    rec_desc.rec_current = NULL;
}

/*{
+*  Name: erec_mem_add - Add a record member to the queue.
**
**  Description:
**	Called for each record member, this routine saves away the name
**	and symbol table pointer for each variable.  If any are null then
**	the member is not added.
**
**  Inputs:
**	mem_id	- Language dependent identifier.
**	mem_sym - Symbol table pointer.
**	mem_typ - Base type of identifier.
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**    	06-mar-1987	- Written (ncg)
**	09-nov-1992 (lan)
**		Also check if mem_typ is T_HDLR since datahandler information
**		is not kept in symbol table.
*/

void
erec_mem_add( mem_id, mem_sym, mem_typ )
char	*mem_id;
SYM	*mem_sym;
i4	mem_typ;
{
    REC_ELEM	*rec;

    if (mem_id == (char *)0 || mem_sym == (SYM *)0 && mem_typ != T_HDLR)
	return;

    q_new((REC_ELEM *), rec, &rec_desc.rec_q);	/* Get a new record member */
    rec->rec_mem = str_add(STRNULL, mem_id);	/* Assign fields */
    rec->rec_msym = mem_sym;
    rec->rec_ind = (char *)0;			/* No indicator yet */
    rec->rec_isym = (SYM *)0;
    rec->rec_typ = mem_typ;
    rec->rec_count = 0;
    q_enqueue(&rec_desc.rec_q, rec);		/* Add to the queue */
    rec_desc.rec_vars++;			/* One more variable */
    if (rec_desc.rec_current == NULL)
    {
	rec_desc.rec_current = rec;		/* Head of this struct */
	rec->rec_ishead = TRUE;
    }
    rec_desc.rec_current->rec_count++;		/* One more for this struct */
}


/*{
+*  Name: erec_ind_add - Add an indicator array to the record members.
**
**  Description:
**	Called once after the record members were added, this routine takes
**	a single array identifier and associates all the stored record
**	members with an element in the array.
**
**  Inputs:
**	ind_start - Starting subscript for array (ie, 0 in C, 1 in FORTRAN).
**	ind_id	  - Indicator variable name.
**	ind_subs  - Printf style language dependent array subscript ("[%d]").
**		    May be a null pointer if no array subscripts required.
**	ind_sym   - Symbol table pointer for array.
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**    	06-mar-1987	- Written (ncg)
*/

void
erec_ind_add( ind_start, ind_id, ind_subs, ind_sym )
i4	ind_start;
char	*ind_id;
char	*ind_subs;
SYM	*ind_sym;
{
    REC_ELEM	*rec;
    i4		index;
    i4		count;
    char	indbuf[ID_MAXLEN + 20];
    char	*subs;

    if (ind_id == (char *)0 || ind_sym == (SYM *)0 || rec_desc.rec_vars <= 0 ||
        rec_desc.rec_current == NULL)
	return;
    
    if (ind_subs == (char *)0)
	ind_subs = ERx("");

    /*
    ** In order to avoid translating host % signs as data (ie, rec[num%HASH])
    ** copy the first name, and printf the subscripts in the loop.
    */
    STcopy(ind_id, indbuf);
    subs = indbuf + STlength(indbuf);		/* Points at null byte */

    for (rec=rec_desc.rec_current, index=ind_start, count=0;
	 count < rec_desc.rec_current->rec_count;
	 rec = q_follow((REC_ELEM *), rec), index++, count++)
    {
	STprintf(subs, ind_subs, index);
	rec->rec_ind = str_add(STRNULL, indbuf);	/* Assign fields */
	rec->rec_isym = ind_sym;
    }
}

/*{
+*  Name: erec_arg_add - Add a datahandler argument to the record members.
**
**  Description:
**
**  Inputs:
**	arg - Argument name.
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	09-nov-1992 (lan)
**		Created.
*/
void
erec_arg_add(arg)
char	*arg;
{
	REC_ELEM	*rec;

	if (rec_desc.rec_current == NULL)
	    return;

	rec = rec_desc.rec_current;
	rec->rec_arg = str_add( STRNULL, arg );
}

/*{
+*  Name: erec_use - Generate (or use) the list of record members.
**
**  Description:
**	Called after the record members and indicators were added, this 
** 	routine generates (or uses for later generation) the list of stored
**	members.
**
**	Because this routine is only used for a small subset of statements
**	(FETCH, SELECT, INSERT) this was not generalized to execute a 
**	variable number of generation operations.  The function flag
**	indicates what to do, the repeat flag indicates whether a repeated
**	query parameter (ie, for INSERT), and the separator may point
**	at a database token separator to generate between calls.
**
**  Inputs:
**	func_code - Function code.
**			IIRETDOM - Need to call ret_add,
**			IISETDOM - Need to call db_var,
**			IICSGET  - Cursor fetch, call ecs_get.
**	rep_flag  - Indicates whether a repeat query parameter for IISETDOM.
**	separator - String to separate when generating calls.
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**    	06-mar-1987	- Written (ncg)
**	08-sep-1989	- Extended for dynamic select statement (bjb)
**	16-nov-1992 (lan)
**		Check if type is T_HDLR (datahandler) and call ret_add, db_var,
**		rep_add and ecs_get with appropriate arguments to generate code
**		for datahandler.
**	19-nov-1992 (lan)
**		Also pass in the type to ret_add.
**      06-dec-1994 (chech02) bug#: 60688
**              If rep_flag is on, datahandler function is generated twice.
**              Fix: If rep_flag is on, call rep_add() only. Otherwise, 
**              call db_var().
*/

void
erec_use(func_code, rep_flag, separator)
i4	func_code;
i4	rep_flag;
char	*separator;
{
    REC_ELEM	*rec;
    i4		count;

    for (rec = q_first((REC_ELEM *), &rec_desc.rec_q), count = 0;
	 count < rec_desc.rec_vars;
	 rec = q_follow((REC_ELEM *), rec), count++)
    {
	switch (func_code)
	{
	  case IIRETDOM:
	    if (rec->rec_typ == T_HDLR)
	        ret_add(rec->rec_msym, rec->rec_mem, rec->rec_isym,
			rec->rec_ind, rec->rec_arg, rec->rec_typ);
	    else
	        ret_add(rec->rec_msym, rec->rec_mem, rec->rec_isym,
			rec->rec_ind, (char *)0, 0);
	    break;
	  case IISETDOM:
	    if (rep_flag)
		rep_param();
	    if (rec->rec_typ == T_HDLR)
	    {
	      /* 60688 */
	        if (rep_flag)
		    rep_add(rec->rec_msym, rec->rec_mem, rec->rec_isym,
			rec->rec_ind, rec->rec_arg);
                else
	            db_var(DB_HDLR, rec->rec_msym, rec->rec_mem, rec->rec_isym,
		        rec->rec_ind, rec->rec_arg);
	    }
	    else
	    {
	        db_var(DB_REG, rec->rec_msym, rec->rec_mem, rec->rec_isym,
		    rec->rec_ind, (char *)0);
	        if (rep_flag)
		    rep_add(rec->rec_msym, rec->rec_mem, rec->rec_isym,
			rec->rec_ind, (char *)0);
	    }
	    if (separator && count < rec_desc.rec_vars - 1)
		db_op(separator);
	    break;
	  case IICSGET:
	    if (rec->rec_typ == T_HDLR)
	        ecs_get(rec->rec_mem, rec->rec_msym, rec->rec_typ,
		        rec->rec_ind, rec->rec_isym, rec->rec_arg);
	    else
	        ecs_get(rec->rec_mem, rec->rec_msym, rec->rec_typ,
		        rec->rec_ind, rec->rec_isym, (char *)0);
	    break;
	}
    }
    rec_desc.rec_vars = 0;	/* But, leave the queue around for dumping */
    rec_desc.rec_current = NULL; /* No current structs, either */
}


/*{
+*  Name: erec_vars - Are there any record members stored?
**
**  Description:
**	Return the number of record members stored for the current hostvar.
**	This should be used before allowing null indicator arrays to be used.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    Number of locally stored current record members.
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**    	06-mar-1987	- Written (ncg)
*/

i4
erec_vars()
{
    if ((rec_desc.rec_vars <= 0) || (rec_desc.rec_current == NULL))
	return 0;
    else
	return rec_desc.rec_current->rec_count;
}

/*{
**  Name: erec_dump - Dump local data structures
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
**  History:
**    	06-mar-1987	- Written (ncg)
*/

void
erec_dump()
{
    register REC_ELEM	*rec;
    register i4	i;
    FUNC_EXTERN char	*trBaseType(i4 btype);

    SIfprintf(eq->eq_dumpfile, ERx("Record List:\n"));
    SIfprintf(eq->eq_dumpfile, ERx(" rec_vars = %d\n"), rec_desc.rec_vars);
    for (i = 0, rec = q_first((REC_ELEM *), &rec_desc.rec_q);
	 rec != (REC_ELEM *)0;
	 i++, rec = q_follow((REC_ELEM *), rec))
    {
	SIfprintf( eq->eq_dumpfile, 
		ERx(" %srec[%3d]: member=%s, count=%d, memsym=0x%p,\n"),
		(rec_desc.rec_current==rec) ?
		ERx("*"):(rec->rec_ishead ? ERx("+"):ERx(" ")),
		i, rec->rec_mem, rec->rec_count, rec->rec_msym );
	SIfprintf( eq->eq_dumpfile, ERx("\tind=%s, indsym=0x%p, typ=%s\n"),
	       rec->rec_ind ? rec->rec_ind : ERx("$"), rec->rec_isym,
	       trBaseType(rec->rec_typ) );
    }
    SIflush(eq->eq_dumpfile);
}

/*{
**  Name: erec_desc - Save descriptor name or emit call to IIcsDaGet
**
**  Inputs:
**	desc_id	- 	Name of user's sqlda descriptor (may be null)
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
**
**  History:
**	08-sep-1989	- Written (bjb)
*/

void
erec_desc(desc_id)
char	*desc_id;
{
    if (desc_id != (char *)0)
    {
	STcopy(desc_id, erec_dyn_desc);
    }
    else
    {
	arg_int_add( eq_genlang(eq->eq_lang) );
	arg_str_add( ARG_RAW, erec_dyn_desc );
	gen_call( IICSDAGET );
	erec_dyn_desc[0] = '\0';
    }
	
}
