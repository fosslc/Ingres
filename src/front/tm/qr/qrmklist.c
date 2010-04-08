/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<qr.h>
# include	<ug.h>
# include	<me.h>
# include	<st.h>
# include	<er.h>
# include	<ui.h>
# include	"erqr.h"
# include	"qrhelp.h"

/**
** Name:	qrmklist.c -	Make list of table names
**
** Description:
**	This file makes lists of table names as specified for help
**	commands.  This file contains the routines:
**
**	IIQRmnl_MakeNameList	Make the name list
**	IIQRaln_AddListNode	Create a node and add to the name list
**	IIQRznl_ZapNameList	Free up the entire name list
**	IIQRgnn_GetNextNode	Return top node on name list
**	IIQRgno_GetNameOwner	Return name and owner from a node
**	IIQRznd_ZapNode		Free up a single name node
**	IIQRlnp_LastNodePtr	Point to last node in list
**	IIQRfnp_FirstNodePtr	Point to first node in list
**	IIQRnfn_NewFirstNode	Reset the first node in list
**	IIQRrnl_ResetNameList	Reset the Name List pointers to NULL
**
** History:
**	01-aug-88 (bruceb)
**		Written, with code yanked from the individual help routines.
**	08-aug-88 (bruceb)
**		Call IIQRznl at start of IIQRmnl; eliminates need to call
**		IIQRznl in signal handlers in monitor and fstm.
**	10-aug-88 (bruceb)
**		Changed from IIQRgnn_GetNextName to IIQRgnn_GetNextNode,
**		and added IIQRgno_GetNameOwner.
**	06-oct-88 (bruceb)
**		Add function pointer as argument to IIQRmnl; call that
**		function whenever a pattern matching character is discovered.
**	01-dec-88 (bruceb)
**		Call IIUGlbo_lowerBEobject for pattern matching strings as
**		well as for fully specified object names.
**	03-jan-89 (teresal)
**		Expand call to wildcard routines to pass an extra parameter:
**		primary object name.  This parameter will be used in the
**		help comment column command to pass the table (view or index)
**		name.
**	06-apr-90 (teresal)
**		Bug fix 9423 - object names are now verified as valid INGRES
**		object names via FEchkname or else a syntax error occurs.
**	10-apr-90 (teresal)
**		Bug fix 9414 - add stricter syntax checking for list of 
**		objects. Generate a syntax error for misuse of commas, e.g,
**		'HELP t1,,t2', 'HELP ,t1,t2,'.
**	 7-jul-92 (rogerl)
**		6.5 maint- use wildcard routines for all namelist entries
**	 21-oct-92 (daver)
**		Added routine IIQRfnp_FirstNodePtr() for loop_end(), which
**		throws out objects in the NameList the user doesn't
**		have access to.
**	 21-jun-93 (rogerl)
**		Need to pass help type all the way down to wildcard expanders
**		to support security_alarm restrictions.
**      13-Jul-95 (ramra01)
**              Tune the Zap node in loop_end to leave the non existant
**              objects for proper error msg display in help (b66258)
**      10-aug-95 (harpa06)
**              Normalize the table name the user wishes to see info on 
**		providing the table is not a delimited name.
**      25-sep-96 (mcgem01)
**              Global data moved to qrdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* # define's */

GLOBALREF IIQRlist	*IIQRnml_NameList;
static IIQRlist		*listhead = NULL;
static IIQRlist		*listtail = NULL;

/* extern's */

FUNC_EXTERN	VOID	qrprintf();
FUNC_EXTERN	char	*qrscanstmt();
FUNC_EXTERN	STATUS	IIQRaln_AddListNode();

/* static's */

static	bool	qrntobjsc( char **s, char *c );
static	STATUS	qrmnlerr( QRB *qrb, i4  idx, i4  *syntax_err, char *errstr );

/*{
** Name:	IIQRmnl_MakeNameList	- Make the name list.
**
** Description:
**	Parse the names specified as part of a help (including help
**	view/integrity/permit) command.  Create a list of these names
**	that the calling routines can reference to obtain each name.
**
**
** Inputs:
**	qrb		The ubiquitous QRB
**	s		The table name
**	pobjname	The primary object name - only used in the "help
**			comment column" command to store the table, view or
**			index name while 's' is storing the column name.
**			This parameter gets passed to the wildcard routines.
**	get_wldcrds	Call this routine to generate the appropriate
**			list of names.  i.e.  a* ===> 'atable', 'abc', 'axz'.
**			If permits aren't correct, they don't go in list.
**
** Outputs:
**	syntax_err	TRUE if syntax error.
**	qrb->outbuf	Places the formatted output in the qrb output buffer.
**
**	Return:
**		STATUS	FAIL if anything bad occured, OK otherwise
**
** History:
**	01-aug-88 (bruceb) First Written.
**	14-jul-92 (rogerl) Use wildcard expander to make all name list entries.
**			   Add bool param to get help on system table if the
**			   system table is explicitly named.  Otherwise, sys
**			   tables aren't put into name list.
**			   Move repeated error cleanup into qrmnlerr().
**			   This allows permits checking (6.5 specification)
**			   to be done in the wildcard expansion query.
**			   This saves a query in the case where there is
**			   a wildcard, and doesn't cost any more if there isn't
**	10-aug-95 (harpa06)
**			   Normalize the table name the user wishes to see info
**			   on providing the table is not a delimited name.
*/

STATUS
IIQRmnl_MakeNameList(qrb, s, pobjname, get_wldcrds, syntax_err, help_type)
register QRB	*qrb;
char		*s;
char		*pobjname;
VOID		(*get_wldcrds)();
i4		*syntax_err;
i4		help_type;
{
    char	savec;
    char	*t;
    i4		token = 0;
    bool	moretables = TRUE;
    IIQRlist	*head = NULL;
    IIQRlist	*tail = NULL;
    STATUS	retval = OK;
    STATUS	qrmnlerr();

    *syntax_err = FALSE;

    IIQRznl_ZapNameList();	/* Clear out any existing nodes (from ^C) */

    while (moretables)
    {
	/*
	** we're pointing at the next token
	** which should be the table name;
	** If we are pointing at a ',' give a syntax error as
	** this is where 'help t1,,t2' or 'help ,t1' is entered. (Bug 9414)
	*/
	if (*s == ',')
	{
	    *s = EOS;
	     return( qrmnlerr( qrb, E_QR000A_Syntax_err_correct_is,
		    syntax_err, ERx( "','" ) ) );
	}

	t = s;

	/* null term current object, saving overwritten char
	** do this preserving any "ed strings as whole
	*/
		/* only error currently found is non-null term del id */
	if ( ! qrntobjsc( &s, &savec ) )
	    return ( qrmnlerr( qrb, E_QR000A_Syntax_err_correct_is,
					syntax_err, ERx("','") ) );

	/* save ptr to remainder of HELP statement */

	qrb->s = s;

        /* Normalize the table to grab help on if it is not a delimited
	   table name. */

        if (IIUGdlm_ChkdlmBEobject (t,NULL,FALSE) != UI_DELIM_ID)
                IIUGdbo_dlmBEobject(t,FALSE);

	/*
	** go ahead and run 'em all through wildcard routines,
	** since permits/precedence enforcement is buried
	** down there somewhere (to avoid retraversing the name
	** list and issuing a query upon each object found in it)
	*/
	( *get_wldcrds )( qrb, t, pobjname, help_type );

	/*
	** FEchkname() might want to be run on t, but I don't
	** think so; the name list is now constructed only from
	** objects which are pulled from the DB anyway
	*/

	*s = savec;		/* restore what might be a ','
				** overwritting EOS
				*/

	/*
	** now look and see if there are more tables. if so, lets loop
	** through again. if not, then we're done, and return
	*/
	s = qrscanstmt(s, &token, TRUE);
	if (token < 0)
	{
	    /*
	    ** passing TRUE, and getting token < 0 means
	    ** we expected a comma seperated list, but a comma
	    ** wasn't there. null terminate the string that
	    ** we got instead, and issue a syntax error.
	    */
	    t = s;
	    while (*s != EOS && !CMwhite(s) && *s != ',')
		CMnext(s);

	    *s = EOS;

	     return( qrmnlerr( qrb, E_QR000A_Syntax_err_correct_is,
		    syntax_err, t ) );
	}


	/* we have more tables to do, or we're at the end of the qrb->stmt */
        if (*s == EOS)
	{
	    moretables = FALSE;  /* No more tables */
	    if (token > 0)
	    {
	        /*
		** Hit EOS but qrscanstmt found a token, this is
		** where an incomplete object list like 'help t1,' was entered.
		** Give a syntax error (Bug 9414)
		*/
		 return( qrmnlerr( qrb, E_QR000D_Syntax_error_on_EOF,
		    syntax_err, (char *) NULL ) );
	    }
	}
    }

    return (retval);
}


/*{
** Name:	IIQRaln_AddListNode	- Create a node; add to the name list.
**
** Description:
**	Create a node for a single name, and add to the end of the name list.
**	Initialize the 'head' and 'tail' pointers if this is the first time
**	in the routine for this list.
**
** Inputs:
**	qrb		The ubiquitous QRB
**	tname		a (table) name
**	towner		owner of the table
**	exist		0/1   loop_end or loop_proc calling fn
**
** Outputs:
**
**	Return:
**		STATUS	FAIL if memory allocation errors occur, OK otherwise
**
** History:
**	01-aug-88 (bruceb)	First written.
**	23-sep-92 (rogerl)	add syn field
**      13-Jul-95 (ramra01)
**              Tune the Zap node in loop_end to leave the non existant
**              objects for proper error msg display in help (b66258)
*/
STATUS
IIQRaln_AddListNode(qrb, tname, towner, synonym, exist)
QRB		*qrb;
char		*tname;
char		*towner;
char		*synonym;
i4		exist;
{
    IIQRlist	*node;

    if ((node = (IIQRlist *)MEreqmem((u_i4)0, (u_i4)sizeof(IIQRlist),
	FALSE, (STATUS *)NULL)) == NULL)
    {
	qrprintf(qrb, ERget(E_QR000F_Error_allocing_mem));
	return (FAIL);
    }

    node->exist = 0;

    if (exist)  
	node->exist = 1;

    if ((node->name = STalloc(tname)) == NULL)
    {
	qrprintf(qrb, ERget(E_QR000F_Error_allocing_mem));
	MEfree(node);
	return (FAIL);
    }

    if (towner == NULL)
    {
	node->owner = NULL;
    }
    else
    {
	if ((node->owner = STalloc(towner)) == NULL)
	{
	    qrprintf(qrb, ERget(E_QR000F_Error_allocing_mem));
	    MEfree(node->name);
	    MEfree(node);
	    return (FAIL);
	}
    }
		/* if this is a synonym, keep the name of the underlying
		** table (which is, unfortunately, in 'synonym') so we
		** can print it out in loghlp without a query
		*/
    if ( synonym != NULL )
    {
	if (( node->type = STalloc( synonym )) == NULL )
	{
	    qrprintf( qrb, ERget( E_QR000F_Error_allocing_mem ) );
	    MEfree( node->type );
	    MEfree( node );
	    return( FAIL );
	}
    }
    else
    {
	node->type = NULL;
    }

    node->next = (IIQRlist *)NULL;

    if (IIQRnml_NameList == NULL)
    {
	IIQRnml_NameList = node;
	listhead = node;
	listtail = node;
    }
    else
    {
	listtail->next = node;
	listtail = node;
    }

    return (OK);
}

/*{
** Name:	IIQRznl_ZapNameList	- Free up the entire name list.
**
** Description:
**	Free up the memory associated with all the nodes on the name list.
**
** Inputs:
**
** Outputs:
**
**	Return:
**		VOID
**
** Side Effects:
**	Resets IIQRnml_NameList (header for the name list) to NULL.
**
** History:
**	01-aug-88 (bruceb)	First written.
*/
VOID
IIQRznl_ZapNameList()
{
    register IIQRlist	*this = IIQRnml_NameList;
    register IIQRlist	*that;

    while (this != NULL)
    {
	MEfree(this->name);
	if (this->owner != NULL)
	    MEfree(this->owner);
	that = this->next;
	MEfree(this);
	this = that;
    }

    IIQRnml_NameList = listtail = NULL;
}

/*{
** Name:	IIQRgnn_GetNextNode	- Return top node on name list.
**
** Description:
**	Return pointer to the top node on the name list.  Name list is
**	updated to point to the next node (possibly NULL).
**
** Inputs:
**
** Outputs:
**
**	Return:
**		IIQRlist *	top node on the list
**
** History:
**	01-aug-88 (bruceb)	First written.
*/
IIQRlist	*
IIQRgnn_GetNextNode()
{
    IIQRlist	*node;

    node = IIQRnml_NameList;

    if (IIQRnml_NameList != NULL)
	IIQRnml_NameList = IIQRnml_NameList->next;
    
    return (node);
}

/*{
** Name:	IIQRznd_ZapNode	- Free up a single name node.
**
** Description:
**	Free the memory associated with a single name node.
**
** Inputs:
**	node	The node to free.
**
** Outputs:
**
**	Return:
**		VOID
**
** History:
**	01-aug-88 (bruceb)	First written.
*/
VOID
IIQRznd_ZapNode(node)
IIQRlist	*node;
{
	MEfree(node->name);
	if (node->owner != NULL)
	    MEfree(node->owner);
	MEfree(node);
}

/*{
** Name:	IIQRgno_GetNameOwner	- Return name/owner from a node.
**
** Description:
**	Return the name and owner strings from the specified node,
**	previously returned by IIQRgnn_GetNextNode.
**
** Inputs:
**	node	Node from which to grab name and owner.
**
** Outputs:
**	name	Name of table.
**	owner	Owner of table.
**
**	Return:
**		VOID
**
** History:
**	10-aug-88 (bruceb)	First written.
*/
VOID
IIQRgno_GetNameOwner(node, name, owner)
IIQRlist	*node;
char		**name;
char		**owner;
{
    *name = node->name;
    *owner = node->owner;
}

/*{
** Name:	IIQRlnp_LastNodePtr	- Point to last node in list
**
** Description:
**	Return pointer to last node in the name list.
**
**	Used to determine whether any nodes have been added to the name
**	list as the result of wildcard activity.
**
** Inputs:
**
** Outputs:
**
**	Return:
**		IIQRlist *	Address of the last node in the name list.
**				This may be NULL.
**
** History:
**	10-aug-88 (bruceb)	First written.
*/
IIQRlist	*
IIQRlnp_LastNodePtr()
{
    return(listtail);
}

/*{
** Name:	IIQRfnp_FirstNodePtr	- Point to first node in list
**
** Description:
**	Return pointer to first node in the name list.
** 	Used when scanning the namelist in loop_end() to throw out those
**	objects we don't have access to.
**
** Returns:
**	IIQRlist *	Address of the first node in the name list.
**
** History:
**	21-oct-92 (daver)	First written.
*/
IIQRlist	*
IIQRfnp_FirstNodePtr()
{
    return(listhead);
}

/*{
** Name:	IIQRnfn_NewFirstNode	- Reset first node in list
**
** Description:
**	Reset pointer to first node in the name list.
** 	Used when scanning the namelist in loop_end() to throw out those
**	objects we don't have access to (in this case, if the first
**	object back is inaccessable, we want to be able to reset the head 
**	of the list).
**
** Input:
**	newhead {IIQRlist *}	Address of the first node in the name list.
**
** Output:
**	None
**
** Returns:
**	None
**
** History:
**	22-oct-92 (daver)	First written.
*/
VOID
IIQRnfn_NewFirstNode(newhead)
IIQRlist *newhead;
{
    IIQRnml_NameList = newhead;
    listhead = newhead;
}

/*{
** Name:	IIQRrnl_ResetNameList	- Reset NameList pointers to NULL
**
** Description:
**	Sets both IIQRnml_NameList and listhead to null. 
** 	Used during a wildcard search that didn't match anything.
**	objects we don't have access to (in this case, if the first
**	object back is inaccessable, we want to be able to reset the head 
**	of the list).
**
** History:
**	22-oct-92 (daver)	First written.
*/
VOID
IIQRrnl_ResetNameList()
{
    IIQRnml_NameList = NULL;
    listhead = NULL;
}

/*{
** Name:	qrmnlerr	- issue error and clean up
**
** Description:
**		At error in MakeNameList, emit error, kill name list
**
** Inputs:
**	idx	error message 
**
** Returns:
**	FAIL
**
** Side Effects:
**	Loads error message into qrb output buffer
**	kills name list
**	sets syntax_err to true
**
** History:
**	14-jul-92 (rogerl)	abstracted from MakeNameList
*/
static STATUS
qrmnlerr( QRB *qrb, i4  idx, i4  *syntax_err, char *errstr )
{
    qrprintf( qrb, ERget( idx ), errstr );
    IIQRznl_ZapNameList();	/* Clear out the existent nodes */
    *syntax_err = TRUE;
    return ( FAIL );
}

/*{
** Name:	qrntobjsc	- null term one object, saving overwritten char
**
** Description:
**
** Inputs:
**	s	string containing objects
**	savec	ptr to char to stuff save char into 
**
** Returns:
**	TRUE	done
**	FALSE	syntax error (non-null term delimited id), or (xxx.xxx.xxx)
**
** Side Effects:
**	write EOS into s at end of first object, may be del id, '"' is quoted
**	using '"' (as in "")
**
** History:
**	14-jul-92 (rogerl)	written
**	20-may-93 (rogerl)	rewritten to properly scan foo."bar"
*/

static bool
qrntobjsc(
    char **s,
    char *c
) {
    bool    in_quote;

    if ( **s == '"' )
    {
	CMnext( *s );
        in_quote = TRUE;
    }
    else
    {
        in_quote = FALSE;
    }

    while ( **s != EOS )
    {
        switch( **s )
        {
			/* these + EOS == set of terminators for object
			*/
	case ',':
	case ' ':  /* space */
	case '	': /* tab */
	case '\n': /* newline */
	    if ( in_quote == TRUE )  /* ',' in a quoted string, skip it */
	    {
		CMnext( *s );
		break;
	    }

	    *c = **s;  /* save the 'terminator' */
	    **s = EOS; /* really terminate the input string */

	    return ( TRUE );

        case '"':
            CMnext( *s );

            if  ( ( **s != '"' ) && ( in_quote ) )
                in_quote = FALSE;
	   
	    if ( ( **s == '"' ) && ( in_quote ) ) /* embedded quote, doubled */
		CMnext( *s );

            break;

        case '.':
            if  ( ! in_quote )
            {
                CMnext( *s );
                if ( **s == '"' )
                {
                    CMnext( *s );
                    in_quote = TRUE;
                }
            }
            else
            {
                CMnext( *s );
            }
            break;

        default:
            CMnext( *s );
            break;
        }
    }

    if ( **s == EOS)
    {
	*c = EOS;
	return( TRUE );
    }

    return( FALSE );
}
