# include 	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<equel.h>
# include	<equtils.h>
# include	<me.h>
# include	<ereq.h>

/*
+* Filename:	STRMAN.C
** Purpose:	String Table Manger.
**
** Defines:	str_new( size )		- Allocate new table for size bytes.
**		str_add( st, string )	- Add string to table st.
**		str_space( st, size )	- Grab size bytes from st.
**		str_mark( st )		- Mark a spot in st.
**		str_chadd( st, ch )	- Add one character to st.
**		str_free( st, ptr )	- Free space in table st back till ptr.
**		str_chget( st, ptr, f ) - Call f with each ch starting at ptr.
**		str_reset()		- Reset local string tables.
**		str_dump( st, name )	- Dump named table for debugging.
**
** Exports: 
**		See header file for macro defintion.
**	  	str_define()		- Macro to use with local buffer.
** Locals:
**		str_suballoc()		- Allocate space for a sub-size.
**		str_local()		- Local string table for general use.
-*		str_work[2]		- Global string table work space.
**
** Notes:
**	1.  Each string table is made up of a head and a list of buffers.
**	The head describes general stuff (size, etc) and each buffer holds as
**	many null terminated strings as possible.  The header has one data
**	pointer that points directly into the current buffer.
**	2.  This string manager assumes all data is added in stack form, so
**	when data is freed, all data after it is freed too.
**	3.  There is a general String Table work space, that can be referenced
**	by using a Null pointer.
**
** History:	
**		24-sep-1984	- Written (ncg)
**		29-jun-87 (bab)	Code cleanup.
**		13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**              7-oct-1993 (essi)
**                  When using -u flag we are not handling the switches
**                  correctly. The gc_dot and g_was_if require a much better
**                  handling specially if we generate nested loops or
**                  if-blocks. To fix this particular bug I introduced a
**                  new flag, g_in_if. This flag is set when we enter an
**                  if-block (gen_if only) and exit when we end an if-block.
**                  It required a hack to str_chget in order to remove the
**                  dots if we are inside of 'gen_if' (54571).
**		13-oct-1993 (essi)
**                  Placed the definition of g_in_if in this file since it
**                  is used by other preprocessors. Placed the reference in
**                  cobgen.c file.
**		24-nov-1993 (rudyw)
**		    Withdraw 7-oct and 13-oct code changes due to problems
**		    resulting in new bug. "Unstructured" needs more research.
**		18-mar-1996 (thoda04)
**		    Corrected cast of MEreqmem()'s tag_id (1st parm).
**		    Added function parameter prototypes for local functions.
**		    Upgraded str_chadd() to ANSI style func proto for Windows 3.1.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define STR_MAXBUF 400		/* Large one for real storage */

/*
** String table work space:
**
** There is a problem regarding name space, and string space used by the
** scanner and general Equel tools.  The main question is "When can this space
** be freed" ?  Before Equel was rewritten the space is freed after the grammer
** has reduced a statement.  This can cause problems if scanner tools have
** already look ahead and filled a buffer with data.  Thus user names, integer 
** and string constants filled by the scanner, and believed to be valid, may 
** be overwriten because they were freed by the grammar.
**
** The solution is to have two general string table work spaces.  When the
** grammar wants to reset string table space, str_reset(), instead of freeing
** the current string table, free the 'next' one and set the current string
** table to be that one.  The grammar would have to look ahead two statements
** (never happens) for the data to be freed prematurely.
*/

/* Array of two string tables, and current index into one of them */

static  STR_TABLE  	*str_work[2] 	= { STRNULL, STRNULL };
static  i4	  	str_cur 	= 0;
static	STR_TABLE	*str_local(void);

static  char   	  	*str_suballoc(STR_TABLE *st, i4  size);

/*
+* Procedure:	str_new 
** Purpose:	Allocate a new string table with siz bytes in buffer.
**
** Parameters: 	siz - i4  	- Size of new string table buffer(s).
-* Returns: 	STR_TABLE * 	- Pointer to string table.
*/

STR_TABLE *
str_new( siz )
register i4	siz;
{
    STR_TABLE 		 *st;			/* String table to return */
    register	STR_BUF	 *sb;			/* First string buffer */

    if ( siz <= 0 )
    {
	er_write( E_EQ0342_strSIZE, EQ_ERROR, 1, er_na(siz) );
	return STRNULL;
    }
    if ((st = (STR_TABLE *)MEreqmem((u_i2)0, (u_i4)(sizeof(STR_TABLE) + siz),
	TRUE, (STATUS *)NULL)) == NULL)
    {
	er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2, er_na(sizeof(STR_TABLE)+siz), 
		ERx("str_new()") ); 
    }
    sb = &st->s_buffer;
    st->s_size = siz;
    /* 
    ** The structures and the buffer were allocated together so set the
    ** real buffer pointer to be after the structure.
    */
    sb->bf_data = ((char *)st) + sizeof(STR_TABLE);
    sb->bf_end = sb->bf_data + siz;		/* For fast checks */
    sb->bf_next = SBNULL;
    st->s_ptr = sb->bf_data;			/* Start of data */
    st->s_curbuf = sb;				/* Current and first buffer */

    return st;
}

/*
+* Procedure:	str_add 
** Purpose:	Add string to a passed string table and return the string entry.
**
** Parameters: 	st     - STR_TABLE * - String table pointer,
**		string - char *      - String to add.
-* Returns: 	char * 		     - Pointer to location of added string.
** 
** Extra allocation of new buffers, or updating of current pointers.
*/

char	*
str_add( st, string )
register STR_TABLE   *st;
char		    *string;
{
    register  i4	len;			/* Length of string */
    char		*result;		/* String entry in table */

    if ( string == (char *)NULL )
	return (char *)NULL;
    if ( st == STRNULL )
	st = str_local();

    len = STlength( string ) +1;

    /* Truncate if string is too large to fit inside one buffer */
    if ( len > st->s_size )
    {
	er_write( E_EQ0344_strTRUNC, EQ_ERROR, 2, string, er_na(st->s_size) );
	result = str_suballoc( st, st->s_size );
	len = CMcopy(string, st->s_size - 1, result);
	result[len] = '\0';
    }
    else
    {
	result = str_suballoc( st, len );
	MEcopy( string, (u_i2)len, result );
    }
    return result;
}



/*
+* Procedure:	str_space 
** Purpose:	Reserve size bytes from string table.
**
** Parameters: 	st   - STR_TABLE * - String table pointer,
**		size - i4	   - Size of space to allocate in table.
-* Returns: 	char *		   - Pointer to location of added space.
** 
** Extra allocation of new buffers, or updating of current pointers.
*/

char	*
str_space( st, size )
register STR_TABLE   *st;
register i4	    size;
{
    if ( st == STRNULL )
	st = str_local();
    /* Round out size to even boundary */
    if (size % 2)
	size++;
    /* Truncate if size is too large to fit inside one buffer */
    if ( size > st->s_size || size < 0 )
    {
	er_write( E_EQ0343_strSPACE, EQ_ERROR, 1, er_na(size) );
	size = st->s_size;
    }
    return str_suballoc( st, size );
}


/*
+* Procedure:	str_mark 
** Purpose:	Reserve a marked spot in string table.
**
** Parameters: 	st - STR_TABLE * - String table pointer.
-* Returns: 	char *		 - Pointer to location of marked spot.
*/

char	*
str_mark( st )
register STR_TABLE   *st;
{
    if ( st == STRNULL )
	st = str_local();
    return str_suballoc( st, 1 );
}

/*
+* Procedure:	str_chadd 
** Purpose:	Add one character to string table (without Null).
**
** Parameters: 	st - STR_TABLE * - String table pointer,
**		ch - char 	 - Character to add.
-* Returns: 	None
** 
** As the user may not use this as a string pointer (no where is a terminating
** Null ensured) this does not return any pointer to the caller.
** The only way to get at the data, is by calling str_chget().
*/

void
str_chadd( register STR_TABLE *st, char ch )
{
    char	*result;		/* String entry in table */

    if ( st == STRNULL )
	st = str_local();
    result = str_suballoc( st, 1 );
    *result = ch;
}   


/*
+* Procedure:	str_suballoc 
** Purpose:	Allocate size more bytes in current string table. Allocation
**		may not be required, just updating data pointers.
**
** Parameters: 	st       - STR_TABLE * - String table pointer,
**		size     - i4	       - Size to allocate out of table's buffer.
-* Returns: 	char *	 - Pointer to location of newly allocated chunk.
*/

static char	*
str_suballoc( st, size )
register STR_TABLE *st;
register i4	  size;
{
    STR_BUF *sb;
    char   *result;

    /*
    ** If a last addition caused s_ptr to go past last byte then s_ptr + the
    ** new length will not fit into buffer.  Go to a new string buffer 
    ** if the new string (or new space) will not fit.
    */
    if ( (st->s_ptr+(size-1)) >= st->s_curbuf->bf_end ) 
    {
	/* If no more on buffer list allocate new buffer and attach */
	if ( (sb = st->s_curbuf->bf_next) == SBNULL )	
	{
	    if ((sb = (STR_BUF *)MEreqmem((u_i2)0,
		(u_i4)(sizeof(STR_BUF) + st->s_size),
		TRUE, (STATUS *)NULL)) == NULL)
	    {
		er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2, 
			er_na(sizeof(STR_BUF)+st->s_size), ERx("str_suballoc()"));
	    }
	    /* Structure and buffer were allocated together */
	    sb->bf_data = ((char *)sb) + sizeof(STR_BUF);
	    sb->bf_end = sb->bf_data + st->s_size;
	    sb->bf_next = SBNULL;
	    st->s_curbuf->bf_next = sb;		/* Attach to buffer list */
	}
	st->s_curbuf = sb;			/* Update buffer information */
	st->s_ptr = sb->bf_data;
    }
    result = st->s_ptr;
    st->s_ptr += size;
    return result;
}


/*
+* Procedure:	str_free 
** Purpose:	Free internal space up till specified pointer.
**
** Parameters: 	st 	- STR_TABLE * - String table pointer,
**		bufptr	- char *      - Spot to free till (Null if to start).
-* Returns: 	None
**
** If buffers are freed they are not deallocated, but just unused. Data may 
** overflow into them at a later time.  
** If caller's free mark is null then reset to start of string table, else 
** check to see that the caller's free spot is within range of buffers.
*/

void
str_free( st, bufptr )
register STR_TABLE *st;
register char	  *bufptr;		/* Caller's free spot */
{
    register STR_BUF   *sb;

    if ( st == STRNULL )
	st = str_local();
    /* Find the buffer which has the "free point" */
    if ( bufptr == (char *)NULL )
    {
	sb = &st->s_buffer;
	bufptr = sb->bf_data;
    }
    else
    {
	for ( sb = &st->s_buffer; sb != SBNULL ; sb = sb->bf_next )
	{
	    if ( (bufptr >= sb->bf_data) && (bufptr < sb->bf_end) )
		break;		/* Found the range */
	}
	if ( sb == SBNULL )
	{
	    er_write( E_EQ0341_strBADPTR, EQ_ERROR, 0 );
	    return;
	}
    }
    *bufptr = '\0';			/* Just in case its used */
    st->s_ptr = bufptr;			/* Update general buffer info */
    st->s_curbuf = sb;
}



/*
+* Procedure:	str_chget 
** Purpose:	Walk through string table returning charactars.
**
** Parameters: 	st 	- STR_TABLE * - String table pointer,
**		bufptr	- char *      - Spot to start collecting chars from,
**		chfunc	- i4  (*)()   - Function to call with each character.
-* Returns: 	None
**
** Starting at the caller's mark walk through till hitting the ptr, calling
** a passed function.
** If caller's mark is null then begin at start of string table, else 
** check to see that the caller's spot is within range of buffers.
*/

void
str_chget( st, bufptr, chfunc )
register STR_TABLE *st;
register char	  *bufptr;		/* Caller's marked spot */
i4		  (*chfunc)();
{
    register STR_BUF   *sb;

    if ( st == STRNULL )
	st = str_local();
    /* Find the buffer which has the marked point */
    if ( bufptr == (char *)NULL )
    {
	sb = &st->s_buffer;
	bufptr = sb->bf_data;		/* Not gotten through str_mark */
    }
    else
    {
	for ( sb = &st->s_buffer; sb != SBNULL ; sb = sb->bf_next )
	{
	    if ( (bufptr >= sb->bf_data) && (bufptr < sb->bf_end) )
		break;		/* Found the range */
	}
	if ( sb == SBNULL )
	{
	    er_write( E_EQ0341_strBADPTR, EQ_ERROR, 0 );
	    return;
	}
	bufptr++;			/* Gotten through str_mark - skip */
    }
    /* Walk through all string buffers returning characters */
    for ( ; bufptr != st->s_ptr; )
    {
	if ( bufptr == sb->bf_end )
	{
	    sb = sb->bf_next;
	    bufptr = sb->bf_data;
	    continue;			/* Check if we are at s_ptr */
	}
	(*chfunc)( *bufptr++ );
    } 
}


/*
+* Procedure:	str_reset 
** Purpose:	Reset a local string table (used by all).
**
** Parameters: 	None
-* Returns:    	None
*/

void
str_reset()
{
    if ( str_work[0] != STRNULL )
    {
	str_cur = 1 - str_cur;				/* Toggle tables */
	str_free( str_work[str_cur], (char *)NULL );	/* Free full table */
    }
}


/*
+* Procedure:	str_local 
** Purpose:	Return pointer to local buffer.
**
** Parameters: 	None
-* Returns:	STR_TABLE * - Pointer to local string table.
**
** If the current string table is not initializa then allocate a new one.
*/

static	STR_TABLE *
str_local()
{
    if ( str_work[0] == STRNULL )
    {
	str_work[0] = str_new( STR_MAXBUF );
	str_work[1] = str_new( STR_MAXBUF );
	str_cur = 0;
    }
    return str_work[ str_cur ];
	
}


# define LINECNT 73

/*
+* Procedure:	str_dump 
** Purpose:	Dump the contents of a string table for debugging.
**	      	Try to dump the area after st->s_ptr.
**
** Parameters: 	st     - STR_TABLE * - String table pointer,
**		stname - char *      - Caller's name of string table.
-* Returns: 	None
** History:
**
**      8-Sep-2008 (gupsh01)
**          Fix usage of CMbytecnt == 2 for UTF8 and multibyte character sets
**          which can can be upto four bytes long. Use CMbytecnt to ensure
**          we do not assume that a multbyte string can only be a maximum
**          of two bytes long. (Bug 120865)
*/

void
str_dump( st, stname )
STR_TABLE  *st;
char	  *stname;
{
    register STR_BUF  *sb;
    register char     *cp;			/* Character counter */
    i4		      b;			/* Buffer counter */
    register i4       charcnt;			/* Counter for new lines */
    char	      *bufhead;			/* Buffer title */
    i4		      headlen;			/*    and length of title */
    char	      *cpbrk;			/* Avoid all those Nulls */
    FILE	      *df = eq->eq_dumpfile;
    i4		      iter, byte_cnt;

    SIfprintf( df, ERx("STR_DUMP: null as '$', spaces as '~',") );
    SIfprintf( df, ERx(" Char pointer preceded by '@'.\n") );

    if ( st == STRNULL )
    {
	st = str_local();
	stname = ERx("Str_local");
    }
    else if ( stname == (char *)0 )
	stname = ERx("Local");
    
    SIfprintf( df, ERx(" Name = \"%s\", Buffer size = %d,"), 
	       stname, st->s_size );
    /* If ptr is within a printable range dump some data */
    if ( st->s_ptr >=  st->s_curbuf->bf_end )
	SIfprintf( df, ERx(" Char marker = Out of bounds,") );
    SIfprintf( df, ERx("\n") );

    /* Trim off Nulls of last buffer */
    for ( sb = st->s_curbuf; sb->bf_next != SBNULL; sb = sb->bf_next )
	;
    for ( cp = sb->bf_end -1; *cp == '\0' && cp >= sb->bf_data; cp-- )
    {
	if ( cp == st->s_ptr )
	    break;
    }
    /* 
    ** Even if s_ptr was at bf_end cpbrk will be in unreachable memory.
    ** In any case add a few Nulls here.
    */
    cpbrk  = cp + 4;			/* Will always be a >0 address */
	
    bufhead = ERx(" s_buf[%2d] = ");
    headlen = STlength( bufhead ) -1;	/* -1 because %2d is length 2 not 3 */
    for ( sb = &st->s_buffer, b = 0; sb != SBNULL ; sb = sb->bf_next, b++ )
    {
	SIfprintf( df, bufhead, b );
	charcnt = headlen;
	for ( cp = sb->bf_data; cp < sb->bf_end; CMnext(cp) )
	{
	    if ( cp == cpbrk )
	    {
		SIfprintf( df, ERx(" ...") );
		charcnt += 4;
		cpbrk = (char *)0;		/* Terminate outer loop */
		break;
	    }
	    if ( cp == st->s_ptr )
	    {
		charcnt++;
		SIputc( '@', df );
	    }
	    if ( *cp && CMcntrl(cp) )
	    {
		SIfprintf( df, CMunctrl(cp) );
		charcnt += 2;	/* ^ and a letter are added */
	    }
	    else 
	    {
		charcnt += 1;
		if ( CMspace(cp) )
		    SIputc( '~', df );
		else if ( *cp )
		{
	    	  byte_cnt = CMbytecnt(cp);
	    	  for (iter = 0; iter < byte_cnt; iter++)
			SIputc( *(cp +iter), df );
		  charcnt += (byte_cnt - 1);
		}
		else
		    SIputc( '$', df );
	    }
	    if ( charcnt >= LINECNT )
	    {
		SIfprintf( df, ERx("\n") );
		for ( charcnt = 0; charcnt < headlen; charcnt++ )
		    SIputc( ' ', df );
	    }
	}
	if ( charcnt )
	    SIfprintf( df, ERx("\n") );
	if ( cpbrk == (char *)0 )
	    break;
    }
    SIflush( df );
}
