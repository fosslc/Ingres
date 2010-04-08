/*
** Copyright (c) 1992, 2004 Ingres Corporation
*/

# include <compat.h>
# include <iplist.h>
# include <st.h>
# include <si.h>
# include <lo.h>
# include <er.h>
# include <cm.h>
# include <me.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <gcn.h>
# include "nu.h"
# include <erst.h>

/*
**  Name: nufile.c -- control file handling functions for netutil
**
**  Description:
**	The functions in this file are used to parse and evaluate statements
**	in a netutil control file.  The control file is used as an alternative
**	to using the forms-based interactive interface.
**	
**	The main portion of netutil invokes these functions through a
**	single entry point, nu_file().  This function is passed a string
**	containing the name of a control file.  It returns after either
**	processing all the statements in the file or failing to do so due
**	to errors.
**
**  Entry points:
**	nu_file() -- Process the statements in a single control file
**
**  History:
**	30-jun-92 (jonb)
**	    Created.
**	22-mar-93 (jonb)
**	    Add & to location parameter in SIfopen call.
**      29-mar-93 (jonb)
**	    Changed format of some comments.
**	17-Oct-93 (joplin)
**	    Replaced 'cant update db' message with Name Server generated
**	    errors, and a specific description of each error.
**	21-Feb-96 (rajus01)
**	    Added nu_sqBr(), nu_spBr(), nu_qsBr() routines for stopping
**	    Bridge server through ingstop utility.
**	21-Aug-97 (rajus01)
**	    Added nu_create_attr(), nu_destroy_attribute(), nu_show_attr()
**	    routines. Fixed nu_destroy_connection() to delete all the
**	    entries for a given vnode, when wildcard is used for the 
**	    network address, protocol, listen address values.
**	31-Mar-98 (gordy)
**	    Use full spelling of 'attribute'.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-apr-2004 (abbjo03)
**	    Check the return status of LOfroms in nu_open.
**	19-May-2005 (thaju02)
**	    Change LOfroms flag from FILENAME to PATH & FILENAME.
**	    (B114544)
**	07-Jul-2005 (wansh01)
**	    Added logic to support escape char '\' in batch file.
**	    (Sir 114741 INGNET #175) 
**	30-Jan-2006 (rajus01)
**	    Service Desk Issue:101239; StarTrak #:14353210-01. 
**	    Netutil ( interacive mode operation ) allows spaces
**	    embedded in values whereas non-interactive mode operation 
**	    not. The space embedded values needs to be delimited by putting
**	    double quotes around the values. This also fixes the  SIR 114741.
**	    The fix for this SIR allows to escape the # character embedded in
**	    values but does not allow space character embedded in value be
**	    escaped because the data fields in the netutil control file are
**	    delimted by spaces.
**	    So, changes for the SIR 114741 *should not* be cross integrated 
**	    from ingres!main. To use # ( which is a comment ) in values put
**	    double quotes around them.
**	    Example: C P L "pay roll" ingres "ca#ingres" # create vnode  
**      06-Jun-08 (rajus01) SD issue 127237, Bug 120196
**          Running netutil -file as a user without NET_ADMIN privilege 
**          causes never ending loop. 
**      24-Nov-2009 (frima01) Bug 122490
**          Added include of iicommon.h, fe.h, ug.h and gcn.h  to
**          eliminate gcc 4.3 warnings.
**	       
*/

static STATUS nu_open();
static STATUS nu_process_stmt();
static void   nu_close();
static bool   nu_token();
static i4     nu_lookup();
static STATUS nu_data_tokens();
static void   nu_error();

static void   nu_create_login(bool);
static void   nu_create_conn(bool);
static void   nu_destroy_login(bool);
static void   nu_destroy_connection(bool);
static void   nu_show_login(bool);
static void   nu_show_conn(bool);
static void   nu_create_attr( bool );
static void   nu_destroy_attribute( bool );
static void   nu_show_attr( bool );

static void   nu_stop(bool);
static void   nu_quiesce(bool);
static void   nu_sq(bool);
static void   nu_sqBr(bool);

static void   nu_spBr(bool);
static void   nu_qsBr(bool);

/* Token codes for the first keyword of a statement... */

# define tCreate 1
# define tDestroy 2
# define tShow 3
# define tStop 4
# define tQuiesce 5
# define tSpBr 6
# define tQsBr 7

/* Token codes for the second keyword of a statement... */

# define tPrivate 1
# define tGlobal 2

/* Token codes for the third keyword of a statement... */

# define tLogin 1
# define tConnection 2
# define tAttr	3

/* Structure containing one row of a translation table... */

typedef struct _XTBL
{
    char *full;		/* The full text of the keyword */
    char *abbr;		/* The minimum abbreviation */
    i4  code;		/* The token ID */
} XTBL;

/* Translation table for the first keyword of a statement... */

static XTBL xFunction[] =
{
    "Create",  "C", tCreate,
    "Destroy", "D", tDestroy,
    "Show",    "Sh",tShow,
    "Stop",    "St",tStop,
    "Quiesce", "Q", tQuiesce,
    "SpBr",    "Sp",tSpBr,
    "QsBr",    "Qs", tQsBr,
    NULL
};

/* Translation table for the second keyword of a statement... */

static XTBL xScope[] =
{
    "Private", "P", tPrivate,
    "Global",  "G", tGlobal,
    NULL
};

/* Translation table for the third keyword of a statement... */

static XTBL xObject[] =
{
    "Login",      "L", tLogin,
    "Connection", "C", tConnection,
    "Attribute",  "A", tAttr,
    NULL
};

/* Translation table array, indexed by keyword position in the statement ... */

static XTBL *xtbl_ary[] =
{
    xFunction,
    xScope,
    xObject
};

/* Number of keywords at the beginning of every statement... */

# define nKeywords (sizeof(xtbl_ary) / sizeof(xtbl_ary[0]))

/* 
** Structure defining a single row of a transfer table.  We select a handler
** for a statement based on the function (e.g. Create/Destroy) and object
** (e.g. Login/Connection) keywords.  The "num_data" field tells us how
** many additional data tokens to pick up and pass to the handler.  
*/

typedef struct _XFERTBL
{
    i4  funcID;
    i4  objID;
    i4  num_data;
    void (*handler)(bool);
} XFERTBL;

/* The transfer table itself... */

XFERTBL xfertbl[] =
{
    tCreate,  tLogin,      3, nu_create_login,
    tCreate,  tConnection, 4, nu_create_conn,
    tCreate,  tAttr, 	   3, nu_create_attr,
    tDestroy, tLogin,      1, nu_destroy_login,
    tDestroy, tConnection, 4, nu_destroy_connection,
    tDestroy, tAttr,	   3, nu_destroy_attribute,
    tShow,    tLogin,      1, nu_show_login,
    tShow,    tConnection, 4, nu_show_conn,
    tShow,    tAttr,	   3, nu_show_attr,
    tStop,    0,          -1, nu_stop,
    tQuiesce, 0,          -1, nu_quiesce,
    tSpBr,    0,        -1, nu_spBr,
    tQsBr, 0,        -1, nu_qsBr,
    0
};

/* The maximum number of data tokens in any statement... */

# define MAX_DATA 4

/* Array to hold data tokens... */

static char   parse_data[MAX_DATA][MAX_TXT];

static FILE *rfile;		    /* For current control file */
static char input_line[MAX_TXT+1];  /* Current input line */
static char *next_token;	    /* Pointer to next token in the line */
static char *input_file_name;	    /* Name of file, for error messages */
static i4  input_line_num;	    /* Line number, for error messages */

/*
**  nu_file() -- process a control file
**
**  This is the main entry point to the non-interactive part of netutil.
**  The name of the file to be processed is passed as an argument.  All
**  statements in the file are processed, or else they aren't, in which
**  case the appropriate error messages are issued.  No status is returned
**  from this function; when the function returns, whatever could be done
**  with the indicated file has been; the calling program doesn't need to
**  worry about it any more.
*/

void
nu_file(char *filename)
{
    if (OK != nu_open(filename))
    {
	SIfprintf(stderr,ERget(E_ST0039_cant_open_file),filename);
	return;
    }

    /* Process statements until nu_process_stmt returns non-OK */

    while (OK == nu_process_stmt()) 
	;

    nu_close();
}


/*
**  nu_process_stmt() -- parse and evaluate one statement from the file
**
**  Inputs: none
**
**  Returns: OK indicates that it's ok to proceed with processing.  Anything
**           else indicates that we're finished with the current file.  The
**           primary reason is, of, course, encountering EOF, but we can also
**           decide to ignore the rest of the file because of certain error
**           conditions.
*/

static STATUS
nu_process_stmt()
{
    i4  ix;
    i4  tok_id[nKeywords];  /* Storage for IDs of the keywords */
    bool first;
    char *token;

    /* 
    ** Accumulate IDs for the keywords, i.e. the fixed fields at the
    ** beginning of each statement. 
    */

    for (ix = 0; ix < nKeywords; ix++)
    {
	first = nu_token(&token); 
	if (token == NULL) 
	    return -1;

	/* 
	** The first token in a statement must be the first token on
	** a text line.  Otherwise we'll never know if we're in synch. 
	*/

	if (ix == 0 && !first)
	{
	    nu_error(ERget(E_ST003A_general_syntax));
	    return OK;
	}

	/* Translate the current token to a token ID... */

	tok_id[ix] = nu_lookup(token,xtbl_ary[ix]);
	if (tok_id[ix] < 0)  /* If the translation failed... */
	{
	    char msg[MAX_TXT];
	    STprintf(msg,ERget(E_ST003B_unrecognized_token),token);
	    nu_error(msg); /* Note: nu_error blows away rest of current line */
	    return OK;     /* ...so we'll just go on trying to process */
	}
	 
	/* 
	** Slight hack: "Stop" and "Quiesce" commands aren't like the
	** others.  If that's what we're looking at, leave now. 
	*/

	if (ix == 0 && (tok_id[ix] == tStop || tok_id[ix] == tQuiesce))
	{
	    tok_id[2] = 0;
	    break;
	}

	if (ix == 0 && (tok_id[ix] == tSpBr || tok_id[ix] == tQsBr))
	{
	    tok_id[2] = 0;
	    break;
	}
    }

    /* 
    ** Find the row in the transfer table corresponding to this statement,
    ** pick up however many additional data tokens we're supposed to, and
    ** head off to the handler function.  
    */

    for (ix = 0; xfertbl[ix].funcID; ix++)
    {
        if ( tok_id[0] == xfertbl[ix].funcID &&
	     tok_id[2] == xfertbl[ix].objID )
	{
	    if (OK == nu_data_tokens(xfertbl[ix].num_data))
	        (*xfertbl[ix].handler)( tok_id[1]==tPrivate );
	    return OK;
	}
    }

    /* 
    ** Not found in transfer table?  Wierd.  Should never happen.  (Famous
    ** last words, I know.)  
    */

    nu_error(ERget(E_ST003A_general_syntax));
    return -1;
}

/*
**  nu_lookup() -- look up a token in a translation table.
**
**  This function finds the translation table entry corresponding to a text
**  token, and returns the ID field from the entry.  The token text must be
**  at least as long as the abbreviation specified in the translation table,
**  must be no longer than the full text, and must match the full text for
**  the entire length of the token.  Matching is case-insensitive.
**
**  Inputs:
**
**    token -- pointer to the text of the token
**    xltbl -- pointer to the translation table to use
**
**  Returns:
**
**    Token ID, or < 0 if no match is found.
*/

static i4
nu_lookup(char *token, XTBL *xltbl)
{
    for ( ; xltbl->full != NULL ; xltbl++ )
    {
	if ( 0 == STbcompare(token,0,xltbl->abbr,STlength(xltbl->abbr),TRUE) &&
	     STlength(token) <= STlength(xltbl->full) &&
	     0 == STbcompare(token,STlength(token),xltbl->full,0,TRUE) )
	{
	    return xltbl->code;
	}
    }
    return -1;
}

/*  nu_read_line -- read a single line from the input file
**
*/

static char *
nu_read_line()
{
    input_line_num++;
    if (OK != SIgetrec(input_line, MAX_TXT, rfile))
        return NULL;
    return input_line;
}

/*  nu_token() -- return the next token from the input file
**
**  Sets a pointer to the next token in the input file, or to NULL if
**  there are no more tokens in the input file.  Returns TRUE if the token
**  being pointed at happens to be the very first token on its text line,
**  which is a feature that is required of certain tokens.
*/

static bool
nu_token(char **tokptr)
{
    bool rtn = FALSE;
    char *eosptr;
    bool quotes = FALSE;
    char escape = '"';

    if (next_token != NULL)    /* Already pointing to next token? */
	*tokptr = next_token;  /* Right, then that's what we'll return */
    else                       /* Otherwise we'll have to hit the input file */
    {
	rtn = TRUE;   /* Whatever we return will be first on its line */

	/* Read input lines until we get one with something useful on it... */

        do
	{
	    if (NULL == (*tokptr = nu_read_line()))
		return rtn;   /* Hit EOF */
	    for ( ; CMwhite(*tokptr) && **tokptr != EOS ; CMnext(*tokptr))
		;
	} while (**tokptr == EOS || **tokptr == '#');
    }


    /* 
    ** At this point, *tokptr points to the next token, one way or another.
    ** Now we'll set next_token for next time we're here.  First we'll find
    ** the end of the current token... 
    */

    if( **tokptr == escape )
    {
         quotes = TRUE;
         for ( next_token = ++*tokptr;; CMnext(next_token) )
            if( *next_token == escape )
            {
                CMnext(next_token);
	        if( !CMwhite(next_token) ) ; 
		else
                      break; 
            }
    }
    else
    {
	for ( next_token = *tokptr ;
	  !CMwhite(next_token) && *next_token != EOS && *next_token != '#' ; 
	  CMnext(next_token) )
	    ;
    }

    eosptr = next_token;  /* Where the EOS for the current token will go */

    /* Skip over whitespace before next token... */

    if (CMwhite(next_token))  
    {
	for ( ; CMwhite(next_token) && *next_token != EOS && *next_token != '#';
                CMnext(next_token) )
            ;
    }

    /* 
    ** If that's all there is on this line, we'll need to get a new one next
    ** time we're here... 
    */
   
    if (*next_token == EOS || *next_token == '#')
        next_token = NULL;

    if( quotes )
       *(eosptr-1) = EOS;
    else
        *eosptr = EOS;  /* Terminate current token with an EOS */

    return rtn;
    
}

/*  nu_data_tokens() -- fill array with data tokens
**
**  This function fills the parse_data array with pointers to the
**  data elements for a single statement.
**
**  Inputs:
**
**    ntoks -- number of tokens to read
**
**  Returns:
**
**    OK if a line ends with the last token, < 0 otherwise.
*/

static STATUS
nu_data_tokens(i4 ntoks)
{
    i4  tokn;
    char *tp;
     
    if (ntoks < 0) 
	return OK;

    for (tokn = 0; tokn < ntoks; tokn++)
    {
	(void) nu_token(&tp);
	if (tp == NULL)
	{
	    nu_error(ERget(E_ST003C_insuff_data));
	    return -1;
	}
	STlcopy(tp,&parse_data[tokn][0],MAX_TXT-1);
    }
    if (next_token == NULL)
        return OK;

    nu_error(ERget(E_ST003D_too_much_data));
    return -1;
}

/*  nu_open() -- open the specified filename for reading
*/

static STATUS
nu_open(char *filename)
{
    LOCATION fileloc;
    char lobuf[MAX_LOC+1];
    STATUS status;

    input_line_num = 0;
    next_token = NULL;

    if (!STcompare(filename,ERx("-")))
    {
	rfile = stdin;
	input_file_name = NULL;
	return OK;
    }

    input_file_name = filename;
    STlcopy(filename, lobuf, sizeof(lobuf));
    status = LOfroms(PATH & FILENAME, lobuf, &fileloc);
    if (status != OK)
	return status;
    return SIfopen(&fileloc, ERx("r"), SI_TXT, MAX_TXT, &rfile);
}

/*  nu_close() -- close the currently open input file
*/

static void
nu_close()
{
    SIclose(rfile);
}

/*  nu_error() -- display an error message
**
**  This function displays the specified error text, together with an
**  indicator showing the current position in the file.
*/

static void
nu_error(char *ermsg)
{
    if (input_file_name)
	SIfprintf(stderr, input_file_name);
    SIfprintf(stderr, ERx("[%d]: %s\n"), input_line_num, ermsg);
    next_token = NULL;
}

/*
**  Special action functions for stop and quiesce...
*/

static void
nu_stop(bool dummy)
{
    nu_sq(FALSE);
}

static void
nu_spBr(bool dummy)
{
    nu_sqBr(FALSE);
}

static void
nu_quiesce(bool dummy)
{
    nu_sq(TRUE);
}

static void
nu_qsBr(bool dummy)
{
    nu_sqBr(TRUE);
}

static void
nu_sq(bool qflag)
{
    char *tp;
    bool flag;

    if (next_token == NULL)
	for (flag = TRUE; NULL != (tp = nu_comsvr_list(flag)); flag = FALSE)
	{
	    STtrmwhite(tp);
	    nu_shutdown(qflag,tp);
	}
    else 
	do
	{
	    nu_token(&tp);
	    nu_shutdown(qflag,tp);
	} while (next_token != NULL);
}

static void
nu_sqBr(bool qflag)
{
    char *tp;
    bool flag;

    if (next_token == NULL)
	for (flag = TRUE; NULL != (tp = nu_brgsvr_list(flag)); flag = FALSE)
	{
	    STtrmwhite(tp);
	    nu_shutdown(qflag,tp);
	}
    else 
	do
	{
	    nu_token(&tp);
	    nu_shutdown(qflag,tp);
	} while (next_token != NULL);
}

/*
**  Action functions...
*/

# define MATCH(x,y) (STequal(x,ERx("*")) || !STbcompare(x,0,y,0,TRUE))

/* Positions of data items in the "data" argument array... */

# define VNODE &parse_data[0][0]

# define LOGIN &parse_data[1][0]
# define PASSWORD &parse_data[2][0]

# define ADDRESS &parse_data[1][0]
# define PROTOCOL &parse_data[2][0]
# define LISTEN &parse_data[3][0]

# define NAME &parse_data[1][0]
# define VALUE &parse_data[2][0]

/*  nu_create_login() -- create a login
**
**  Required data items: VNODE, LOGIN, PASSWORD
*/

static void
nu_create_login(bool private)
{
    STATUS rtn;

    if (OK == (rtn = nv_add_auth(private, VNODE, LOGIN, PASSWORD)))
    {
        nu_change_auth(VNODE, private, LOGIN);
    }
    else
    {
	IIUGerr(rtn, 0, 0);
        nu_error(ERget(E_ST0021_create_login_failed));
    }
}

/*  nu_create_conn() -- create a connection entry
**
**  Required data items: VNODE, ADDRESS, PROTOCOL, LISTEN
**
**  If the specified entry already exists, this function is a no-op.
*/

static void
nu_create_conn(bool private)
{
    STATUS rtn;

    if (NULL == nu_locate_conn(VNODE,private,ADDRESS,LISTEN,PROTOCOL,NULL))
    {
	if (OK == (rtn = nv_merge_node(private,VNODE,PROTOCOL,ADDRESS,LISTEN)))
	{
	    nu_new_conn(VNODE,private,ADDRESS,LISTEN,PROTOCOL);
	}
	else
	{
	    IIUGerr(rtn, 0, 0);
            nu_error(ERget(E_ST0022_create_conn_failed));
	}
    }
}

/*  nu_create_attr() -- create an attribute entry
**
**  Required data items: VNODE, NAME, VALUE 
**
**  If the specified entry already exists, this function is a no-op.
*/
static void
nu_create_attr( bool private )
{
    STATUS rtn;

    if( nu_locate_attr( VNODE, private, NAME, VALUE, NULL ) == NULL )
    {
	if( ( rtn = nv_merge_attr( private, VNODE, NAME, VALUE ) ) == OK )
	    nu_new_attr( VNODE, private, NAME, VALUE );
	else
	{
	    IIUGerr( rtn, 0, 0 );
            nu_error( ERget( E_ST020F_create_attr_failed ) );
	}
    }
}

/*  nu_destroy_login() -- destroy one or more login entries
**
**  Required data items: VNODE
**
**  This function allows wildcarding on the vnode.  All login entries
**  of the specified type (private/global), for either the specified vnode
**  or all vnodes if it's specified as a wildcard, are destroyed.  It is
**  not considered to be an error if no login entry matches the specification.
*/

static void
nu_destroy_login(bool private)
{
    char *glogin, *plogin, *login, *uid;
    char *vnm;
    bool flag;
    STATUS rtn;

    for (flag=TRUE; NULL != (vnm = nu_vnode_list(flag)); flag=FALSE)
    {
        if (MATCH(VNODE,vnm))
        {
	    nu_vnode_auth(vnm,&glogin,&plogin,&uid);
	    if (NULL != (login = private? plogin: glogin))
	    {
		if (OK == (rtn = nv_del_auth(private, vnm, login)))
		{
		    nu_change_auth(vnm, private, NULL);
		}
		else
		{
		    IIUGerr(rtn, 0, 0);
                    nu_error(ERget(E_ST0023_destroy_login_failed));
		}
	    }
	}
    }
}

/*  nu_destroy_connection() -- destroy one or more connection entries
**
**  Required data items: VNODE, ADDRESS, PROTOCOL, LISTEN
**
**  This function destroys any connection entry of the specified type
**  (private/global) which matches the data items.  Any data item may
**  be specified as a wildcard.  It is not an error if no matches are
**  found.
** 
**  History:
**    06-Jun-08 (rajus01) SD issue 127237, Bug 120196
**        Running netutil -file as a user without NET_ADMIN privilege 
**        causes never ending loop. 
*/

static void
nu_destroy_connection(bool private)
{
    char *vnm;
    bool flag1, flag2;
    i4  priv;
    char *addr, *list, *prot;
    STATUS rtn;

    for (flag1=TRUE; NULL != (vnm = nu_vnode_list(flag1)); flag1=FALSE)
    {
        if (MATCH(VNODE,vnm))
        {
	    for ( flag2 = TRUE;
	    	  nu_vnode_conn(flag2,&priv,vnm,&addr,&list,&prot) != NULL;
		)
	    {
	        if ( priv == private && MATCH(ADDRESS,addr) && 
		     MATCH(PROTOCOL,prot) && MATCH(LISTEN,list) )
                {
		    if (OK == (rtn=nv_del_node(private, vnm, prot, addr, list)))
		    {
			nu_remove_conn( vnm );

		    }
		    else
		    {
			IIUGerr(rtn, 0, 0);
                        nu_error(ERget(E_ST002A_destroy_conn_failed));
			/* Terminate the inner for loop */
		  	flag2 = FALSE;
		    }
	        }
		else
		    flag2 = FALSE;
	    }
	}
    }
}

/*  nu_destroy_attribute() -- destroy one or more attribute entries
**
**  Required data items: VNODE, NAME, VALUE
**
**  This function destroys any attribute entry of the specified type
**  (private/global) which matches the data items.  Any data item may
**  be specified as a wildcard.  It is not an error if no matches are
**  found.
*/
static void
nu_destroy_attribute( bool private )
{
    char 	*vnm;
    bool 	flag1, flag2 = TRUE;
    i4  	priv;
    char 	*nm, *val;
    STATUS 	rtn;

    for( flag1 = TRUE;
	 ( vnm = nu_vnode_list( flag1 ) ) != NULL;
	 flag1 = FALSE )
    {
        if( MATCH( VNODE, vnm ) )
        {
	    while( 1  )
	    {
		if( nu_vnode_attr( flag2, &priv, vnm, &nm, &val ) != NULL )
	    	{
	            if( priv == private && MATCH( NAME, nm ) && 
		     			MATCH( VALUE, val ) )
                    {
		    	if( (rtn = nv_del_attr( private, vnm, nm, val ) )
									!= OK )
		    	{
			    IIUGerr( rtn, 0, 0 );
                            nu_error( ERget( E_ST020D_destroy_attr_failed ) );
		    	}
		    	else
			    nu_remove_attr( vnm );
	            }
		    else
		    	flag2 = FALSE;
	        }
		else
		    break;
	    }
	}
    }
}

/*  nu_show_login() -- display information about a login entry
**
**  Required data item: VNODE
**
**  This function will display the login name of the specified type for
**  the specified vnode, or for all vnodes if the vnode name is given as
**  a wildcard.
*/

static void
nu_show_login(bool private)
{
    char *glogin, *plogin, *login, *uid;
    char *vnm;
    bool flag;

    for (flag=TRUE; NULL != (vnm = nu_vnode_list(flag)); flag=FALSE)
    {
        if (MATCH(VNODE,vnm))
        {
	    nu_vnode_auth(vnm,&glogin,&plogin,&uid);
	    if (NULL != (login = private? plogin: glogin))
	    {
	        SIprintf( ERx("%s login %s %s\n"),
		          private? ERx("Private"): ERx("Global"), vnm, login );
	    }
        }
    }
}

/*  nu_show_conn() -- Display information about one or more connection entries
**
**  Required data items: VNODE, ADDRESS, PROTOCOL, LISTEN
**
**  This function will display information about all connection entries
**  which match the input specification.  Any of the four required data
**  items may be specified as a wildcard.
*/

static void
nu_show_conn(bool private)
{
    char *vnm;
    bool flag1, flag2;
    i4  priv;
    char *addr, *list, *prot;

    for (flag1=TRUE; NULL != (vnm = nu_vnode_list(flag1)); flag1=FALSE)
    {
        if (MATCH(VNODE,vnm))
        {
	    for ( flag2 = TRUE ; 
	          NULL != nu_vnode_conn(flag2,&priv,vnm,&addr,&list,&prot);
	          flag2 = FALSE )
	    {
	        if ( priv == private && MATCH(ADDRESS,addr) && 
		     MATCH(PROTOCOL,prot) && MATCH(LISTEN,list) )
                {
		    SIprintf( ERx("%s connection %s %s %s %s\n"),
			      private? ERx("Private"): ERx("Global"), vnm,
			      addr, prot, list);
	        }
	    }
	}
    }
}

/*  nu_show_attr() -- Display information about one or more attribute entries
**
**  Required data items: VNODE, NAME, VALUE
**
**  This function will display information about all attribute entries
**  which match the input specification.  Any of the four required data
**  items may be specified as a wildcard.
*/
static void
nu_show_attr( bool private )
{
    char 	*vnm;
    bool 	flag1, flag2;
    i4  	priv;
    char 	*nm, *val;

    for( flag1 = TRUE;
	 ( vnm = nu_vnode_list( flag1 ) ) != NULL; flag1 = FALSE )
    {
        if( MATCH( VNODE, vnm ) )
        {
	    for( flag2 = TRUE; 
	         nu_vnode_attr( flag2, &priv, vnm, &nm, &val ) != NULL;
	         flag2 = FALSE )
	    {
	        if( priv == private && MATCH( NAME, nm ) && 
		     MATCH( VALUE, val ) )
                {
		    SIprintf( ERx( "%s attribute %s %s %s \n" ),
			      private? ERx( "Private" ): ERx( "Global" ),
			      vnm, nm, val );
	        }
	    }
	}
    }
}
