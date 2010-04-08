# include 	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<equel.h>
# include	<equtils.h>
# include	<ereq.h>

/*
+* Filename:	IDEQ.C
** Purpose:	Maintains routines for managing Equel identifiers (user 
**		variables in final fully-qualified format).
**
** Defines:	id_getname( i )		  - Return internal name.
**		id_add( dat )		  - Add data to Identifier.
**		id_key( dat )		  - Isolate with spaces and add.
**		id_free()		  - Free up all Id information.
**		id_dump( name )		  - Dump local Ids for debugging.
** Locals:
-*		id_local		  - Array of two identifiers.
**		
** An Equel identifier description consists of a symbol table entry and a 
** buffer of a list of names (or parts of a names). For example the C variable:
** 	x.y->z 
** is entered as:
**	a symbol table entry for C, plus the sequence of strings
**	"x", ".", "y", "->" and "z"
** stored internally as:
**	"x.y->z"
**
** Note:
**  1.  There is a maximum length of a fully qualified name - ID_MAXLEN.
**
** History:	10-nov-1984	- Written (ncg)
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

typedef struct {	/* Equel identifier */
	i4		id_len;			/* Total length of ids */
	i4		id_err;			/* Error in building */
	char		id_buf[ ID_MAXLEN +1];	/* Buffer for data */
} EQ_ID;

/* 2 Local identifiers for external usage and an index into them */
static		EQ_ID	 id_local[2] ZERO_FILL;
static		i4	 id_dex = 0;


/*
+* Procedure:	id_getname 
** Purpose:	Get the full name for whatever is stored in the indexed Id.
**
** Parameters: 	None
-* Returns: 	char * - Pointer to full name.
*/

char 	*
id_getname()
{
    return id_local[ id_dex ].id_buf;
}

/*
+* Procedure:	id_add 
** Purpose:	Add a variable name (or part of a name) to the Id buffer.
**
** Parameters: 	data - char * - String to add to qualify Id.
-* Returns: 	None
**
** If errors occur when adding clean up.
*/

void
id_add( data )
char	*data;
{
    register EQ_ID	*eqid;
    i4			len;

    eqid = &id_local[ id_dex ];
    if ( data == (char *)0 || eqid->id_err )
	return;

    len = STlength( data );
    if ( eqid->id_len + len > ID_MAXLEN )
    {
	eqid->id_err = TRUE;
	if ( eqid->id_len > 0 )			/* Dump what we have so far */
	{
	    er_write( E_EQ0217_scIDLONG, EQ_ERROR, 3, er_na(ID_MAXLEN), 
		    eqid->id_buf, data );
	}
	else					/* This piece is too big */
	{
	    /* Dump what was just added, and add part of it */
	    er_write( E_EQ0217_scIDLONG, EQ_ERROR, 3, er_na(ID_MAXLEN), 
		    ERx(""), data ); 
	    len = CMcopy( data, ID_MAXLEN, eqid->id_buf );
	    eqid->id_buf[ len ] = '\0';
	    eqid->id_len = len;
	}
    }
    else
    {
	STcat( eqid->id_buf, data );
	eqid->id_len += len;
    }
}

/*
+* Procedure:	id_key 
** Purpose:	Add a data element between separated by spaces.  
**
** Parameters: 	data - char * - String to add between spaces, to qualify Id.
-* Returns: 	None
**
** For example in Cobol: x in y must have spaces around the 'in', so it is 
** stored as:  "x in y".
*/

void
id_key( data )
char	*data;
{
    register EQ_ID	*eqid;
    i4			len;

    eqid = &id_local[ id_dex ];
    if ( (len = eqid->id_len) > 0 && eqid->id_buf[len-1] != ' ' )
	id_add( ERx(" ") );
    id_add( data );
    id_add( ERx(" ") );		/* Make sure it is followed by a space */
}


/*
+* Procedure:	id_free 
** Purpose:	Free the next Id information.
**
** Parameters: 	None
-* Returns: 	None
*/

void
id_free()
{
    register    EQ_ID	*eqid;

    id_dex = 1 - id_dex;		/* Toggle Id's and free */
    eqid = &id_local[ id_dex ];
    eqid->id_len = 0;
    eqid->id_err = FALSE;
    eqid->id_buf[0] = '\0';
}


/* 
+* Procedure:	id_dump 
** Purpose:	Dump local information for debugging.
**
** Parameters: 	calname - char * - Caller's name.
-* Returns: 	None
*/

void
id_dump( calname )
char	*calname;
{
    register EQ_ID	*eqid;
    register FILE	*df = eq->eq_dumpfile;
    i4			ind;

    if ( calname == NULL )
	calname = ERx("Equel_id");
    SIfprintf( df, ERx("ID_DUMP: %s\n"), calname );
    for ( ind = 0; ind < 2; ind++ )
    {
	eqid = &id_local[ind];
	SIfprintf( df,ERx(" eqid[%s%d], id_len: %d, id_err: %d,\n"),
		   (ind == id_dex) ? ERx("*") : ERx(" "), ind, eqid->id_len, 
		   eqid->id_err );
	SIfprintf( df, ERx(" id_buf:  \"%s\"\n"), eqid->id_buf );
    }
    SIflush( df );
}
