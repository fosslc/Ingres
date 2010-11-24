/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** motest.c -- simple incomplete test/example program for mo interface.
**
** History:
**	22-May-90 (daveb)
**		Created.
**	1-Apr-91 (daveb)
**		Updated.
**	24-sep-91 (daveb)
**		modified to MO from MI.
**	30-Oct-91 (daveb)
**		Removed MOscan, err descs, etc.
**	14-Nov-91 (daveb)
**		New format access functions.
**	18-Nov-91 (daveb)
**		Methodize. Do getnext_name functions differently,
**		handing showtable a list of columns in the table of
**		interest.
**	27-jan-92 (daveb)
**		Update to MO.10; many huge changes.
**	06-sep 1993 (tad)
**		Bug #56449
**		Changed %x to %p in mymonitor() and main().
**	06-nov 1998 (kosma01)
**		Clean up some STprintf compile errors caught by AIX 4.x
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**
** Functions:
**	mystrfunc	example simple string object method.
**	show		format GET[NEXT] output nicely on one line.
**	showall		scan whole tree and show() each row.
**	drivecol	call function for all instances of a class.
**	driverow	call function for an instance of each class in a list
**	showrow		show() classes from a list for an instance.
**	per_row_show	Print a '\n' and then do a showrow.
**	showtable	show instances in an index of all classes in a list
**	usertables	attach a number of "user" rows in a table.
**	main		master test function
**
NEEDLIBS = COMPAT
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      03-nov-2010 (joea)
**          Declare local functions static.
*/

# include <compat.h>
# include <gl.h>
# include <cv.h>
# include <er.h>
# include <pc.h>
# include <si.h>
# include <st.h>
# include <tr.h>
# include <sp.h>
# include <erglf.h>
# include <mo.h>

# include "moint.h"

# define MAX_NUM_BUF	40

# define MAXCLASSID	80
# define MAXINSTANCE	80
# define MAXVAL		80

/*  number of "users" in test table */
# define MAXUSERS	5

/* for my_er_lookup */

static char ERbuf[ ER_MAX_LEN + 1 ];
static STATUS ERstat = OK;	

static char *str_ptr = "a string";
static char str_buf[100] = "a buffer";
static i4 int_val = 1024;

static i4 private_num = 512;

static char private_string[ 10 ] = { "priv str" };

# define BAD_MDATA	((PTR)~0)
# define GROUP_MDATA1	(PTR)1
# define GROUP_MDATA2	(PTR)2
# define ID_MDATA	(PTR)3
    

typedef struct
{
    i4		flags;		/* MO_CLASSID_CONST | MO_INSTANCE_CONST */
    char	*classid;	/* the classid name */
    char	*instance;	/* the instance string */
    PTR		idata;		/* the data for the instance */
} MO_INSTANCE_DEF;

/* simple ER lookup function */

static char *
er_decode( STATUS msg_id )
{
    STATUS old_err;
    STATUS errstat;
    CL_ERR_DESC err_code;
    i4  ret_len;
    char lbuf[ sizeof(ERbuf) ];

    if( msg_id == OK )
    {
	errstat = OK;
	STcopy( "OK", ERbuf );
    }
    else if ((errstat = ERslookup(msg_id,		/* msg num */
			    NULL,		/* input cl err */
			    0,			/* flags */
			    (char *) NULL,	/* sqlstate */
			    ERbuf,		/* buf */
			    sizeof(ERbuf),	/* sizeof buf */
			    -1,			/* language */
			    &ret_len,		/* msg len */
			    &err_code,		/* output err */
			    0,			/* num params */
			    NULL)		/* params */
	 ) != OK)
    {
	/* Recurse to decode ER error.  If we were processing error
	   already, give up to halt recursion */

	old_err = ERstat;	/* OK first time in */
	ERstat = errstat;	/* now not OK second time through */

	if( old_err == OK )
	    STcopy( lbuf, er_decode( errstat ) );
	else
	    STprintf(ERbuf, "ER err 0x%x", errstat );

	STprintf( ERbuf, "0x%x not found: %s", msg_id, lbuf ); 
    }

    /* Clear recursion check */
    ERstat = OK;

    return( ERbuf );
}

/* simple integer method -- wrap a private integer r/w */

static STATUS
myintget( i4  offset,
	 i4  objsize,
	 PTR object,
	 i4  luserbuf,
	 char *userbuf )
{
    STATUS stat = OK;

    char buf[ 20 ];

    CVla( private_num, buf );
    STncpy( userbuf, buf, luserbuf ) ;
    userbuf[ luserbuf ] = EOS;
    if( STlength( buf ) != STlength( userbuf ) )
	stat = MO_VALUE_TRUNCATED;
    return( stat );
}

static STATUS
myintset( i4  offset,
	 i4  luserbuf,
	 char *userbuf,
	 i4  objsize,
	 PTR object )
{
    STATUS stat;
    i4 tval;

    stat = CVal( userbuf, &tval );
    if( stat == OK )
	private_num = tval;
    
    return( stat );
}

/* ---------------------------------------------------------------- */

/***************** completely imaginary data *****************/


/* output instdata is the instance as a string */

static STATUS
my_dynamic_index(i4 msg,
		 PTR cdata,
		 i4  linstance,
		 char *instance, 
		 PTR *instdata )
{
    STATUS stat = OK;
    i4 ival;
    char buf[ 20 ];

    switch( msg )
    {
    case MO_GET:
	*instdata = (PTR)instance;
	break;

    case MO_GETNEXT:
	if( *instance == EOS )
	    ival = 0;

	stat = CVal( instance, &ival ); /* value is the instance */
	if( stat != OK )
	    break;
	
	if( ival > 9 )		/* validate this instance */
	    stat = MO_NO_INSTANCE;
	else if( ++ival > 9 ) /* validate next */
	    stat = MO_NO_NEXT;

	if( stat != OK )
	    break;

	CVla( ival, buf );
	
	/* assumes caller doesn't blast instance before passing
	   it to a get/set method.  Otherwise hard to handle,
	   because we don't want to allocate memory here to
	   be freed in the get or set routine. */

	*instdata = (PTR)instance;
	
	STncpy( instance, buf, linstance );
	instance [ linstance ] = EOS;
	if( STlength( buf ) != STlen ( instance ))
	    stat = MO_INSTANCE_TRUNCATED;

	break;

    default:
	stat = MO_BAD_MSG;
    }
    return( stat );
}

static STATUS
my_dynamic_get( i4  offset,
	       i4  objsize,
	       PTR object,
	       i4  luserbuf,
	       char *userbuf )
{
    STATUS stat = OK;
    char *src = (char *)object;
    
    STncpy( userbuf, src, luserbuf );
    userbuf[ luserbuf ] = EOS;
    if( STlength( src ) != STlen( userbuf ))
	stat = MO_VALUE_TRUNCATED;
    return( stat );
}

/* ---------------------------------------------------------------- */

/***************** wrap a string buffer *****************/

static STATUS
mystrget( i4  offset,
	 i4  objsize,
	 PTR object,
	 i4  luserbuf,
	 char *userbuf )
{
    STATUS stat;
    
    STncpy( userbuf, private_string, luserbuf );
    userbuf[ luserbuf ] = EOS;
    if( STlength( private_string ) == STlen( userbuf ))
	stat = OK;
    else
	stat = MO_VALUE_TRUNCATED;

    return( stat );
}

static STATUS
mystrset( i4  offset,
    i4  luserbuf,
    char *userbuf,
    i4  objsize,
    PTR object )
{
    STATUS stat;
    
    STncpy( private_string, userbuf, sizeof( private_string ) );
    private_string[ luserbuf ] = EOS;
    if( STlength( userbuf ) == STlen( private_string ))
	stat = OK;
    else
	stat = MO_VALUE_TRUNCATED;

    return( stat );
}

/* ---------------------------------------------------------------- */

static MO_CLASS_DEF my_cdefs[] =
{
    { 0, "my_sptr", sizeof( str_ptr ), MO_READ, "",
	  0, MOstrpget, MOnoset, (PTR)&str_ptr, MOcdata_index },

    { 0, "my_sbuf", sizeof( str_buf ), MO_READ, "",
	  0, MOstrget, MOnoset, (PTR)str_buf, MOcdata_index },

    { 0, "my_int", sizeof(int_val), MO_READ|MO_WRITE, "",
	  0, MOintget, MOintset, (PTR)&int_val, MOcdata_index },

    { 0, "my_func_value", 0, MO_READ|MO_WRITE, "",
	  0, myintget, myintset, 0, MOcdata_index },

    { 0, "my_method_func", 0, MO_READ, "my_method_index",
	  0, my_dynamic_get, MOnoset, NULL, my_dynamic_index },

    { 0, "my_method_index", 0, MO_READ, "my_method_index",
	  0, my_dynamic_get, MOnoset, NULL, my_dynamic_index },

    { 0, "my_str_func", 0, MO_READ|MO_WRITE, "",
	  0, mystrget, mystrset, 0, MOcdata_index },
    
    { 0 }
};


static MO_CLASS_DEF ex_cdefs[] =
{
    { MO_CDATA_CLASSID, "my_id", 0, MO_READ, "my_id",
	  0, MOstrget, MOnoset, "my_id", MOname_index },

    { MO_CDATA_CLASSID, "my_group", 0, MO_READ, "my_id",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "my_role", 0, MO_READ, "my_id",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "my_user", 0, MO_READ, "my_id", 
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { 0 }
};

static MO_INSTANCE_DEF ex_idefs[] =
{
    { 0, "my_id", "1000", (PTR)NULL }, 
    { 0, "my_group", "1000", (PTR)"wheel" }, 
    { 0, "my_role", "1000", (PTR)"admin" }, 
    { 0, "my_user", "1000", (PTR)"him" }, 
    { 0, "my_id", "2000", (PTR)NULL }, 
    { 0, "my_group", "2000", (PTR)"users" }, 
    { 0, "my_role", "2000", (PTR)"my_user" }, 
    { 0, "my_user", "2000", (PTR)"me" }, 

    { 0 }
};

/* table definitions for "showtable" */

static char *method_tbl [] =
{
    "my_method_index", 
    "my_method_func", 
    0
};

static char *id_tbl[] =
{
    "my_id", 
    "my_user", 
    "my_group", 
    "my_role", 
    0
};

static char *big_tbl [] =
{
    "mib.sqlSessIndex", 
    "mib.sqlSessDatabase", 
    "mib.sqlSessRealuser", 
    "mib.sqlSessUser", 
    "mib.sqlSessGroup", 
    "mib.sqlSessRole", 
    "mib.sqlSessSuccStatements", 
    "mib.sqlSessErrStatements", 
    "mib.sqlSessStatements", 
     0
};

/* ---------------------------------------------------------------- */

typedef VOID DRIVE_FUNC( char *classid, char *instance, char *sval,
			i4 perms,  PTR arg );

/* format and print something nicely */

static void
show( char *classid, 
     char *instance, 
     char *sval, 
     i4  perms,
     PTR arg )
{
    char classidbuf[ MAXCLASSID + MAXINSTANCE + 1 ];

    STprintf( classidbuf, "%s:%s:", classid, instance );
    SIprintf(classidbuf, "%-50s '%-12s'\t0%o\n", classidbuf, sval, perms );
}

/* show everything in the table, by classid */

static void
showall(void)
{
    i4  lsbuf;
    char classid[ MAXCLASSID ];
    char instance[ MAXINSTANCE ];
    char sbuf[ MAXVAL ];
    i4  perms;

    classid[0] = EOS;
    instance[0] = EOS;

    for( lsbuf = sizeof(sbuf); ; lsbuf = sizeof( sbuf ) )
    {
	if( OK != MOgetnext( ~0, sizeof(classid), sizeof(instance),
			    classid, instance,
			    &lsbuf, sbuf, &perms ) )
	    break;

	show( classid, instance, sbuf, perms, (PTR)NULL );
    } 
    SIprintf("\n");
}

static void
drivecol( char *colclassid, DRIVE_FUNC *func, PTR arg ) 
{
    STATUS stat;
    i4  lsbuf;
    char classid[ MAXCLASSID ];
    char instance[ MAXINSTANCE ];
    char sbuf[ MAXVAL ];
    i4  perms;

    STcopy( colclassid, classid );
    instance[0] = EOS;

    for( lsbuf = sizeof( sbuf );
	(stat = MOgetnext(  ~0, sizeof(classid), sizeof(instance),
			  classid, instance,
			  &lsbuf, sbuf, &perms ) ) == OK
	&& STequal( colclassid, classid ) ;
	lsbuf = sizeof( sbuf ) )
    {
	(*func)( classid, instance, sbuf, perms, arg );
    } 
}

static void
driverow( char *instance, char **cols, DRIVE_FUNC *func, PTR arg ) 
{
    STATUS stat;
    char classid[ MAXCLASSID ];
    char rinstance[ MAXINSTANCE ];
    char sbuf[ MAXVAL ];
    i4  lsbuf;
    i4  perms;

    if( cols != NULL )	/* get exact columns */
    {
	for( lsbuf = sizeof(sbuf);
	    *cols != NULL &&
	    OK == (stat = MOget( ~0, *cols, instance, &lsbuf, sbuf, &perms ) );
	    cols++, lsbuf = sizeof( sbuf ) )
	{
	    (*func)( *cols, instance, sbuf, perms, arg );
	}
    }
    else			/* stupid scan for instance match */
    {
	classid[0] = EOS;
	rinstance[0] = EOS;

	for( lsbuf = sizeof(sbuf);
	    OK == MOgetnext( ~0, sizeof(classid), sizeof(rinstance),
			    classid, rinstance,
			    &lsbuf, sbuf, &perms );
	    lsbuf = sizeof( sbuf ) )
	{
	    if( STequal( instance, rinstance ) )
		(*func)( classid, rinstance, sbuf, perms, (PTR) NULL );
	} 
    }
}

/* ---------------------------------------------------------------- */

/* 
** This function prints all the values associated with a "row"
** of various classids having the same instance. 
*/
static void
showrow( char *instance, PTR colclassids )
{
    driverow( instance, (char **)colclassids, show, (PTR) NULL );
}


/*
** Called once for each row of an index column; arg contains
** a list of column classids to show for a row with that instance.
*/
static void
per_row_show( char *classid, char *instance, char *sval, i4  perms, PTR arg )
{
    SIprintf("\n");
    showrow( instance, arg );
}


/*
** Show a table with the instances in an index/id column
*/
static void
showtable( char *indexcol, char **colclassids )
{
    SIprintf("\nShowing table indexed by %s\n", indexcol );
    drivecol( indexcol, per_row_show, (PTR)colclassids );
}


/* 
** This function scans and prints all instance (rows) of a "column" of 
** a given classid.  The scan is terminated when we get a new classid.
*/
static void
showcol( char *colclassid )
{
    SIprintf("\nShowing column of %s\n", colclassid );
    drivecol( colclassid, show, (PTR)NULL );
}

/* ---------------------------------------------------------------- */

static STATUS
mymonitor( char *classid, char *twinid, char *instance, char *value,
	  PTR mon_data, i4  msg )
{
    SIprintf( "mymonitor.%p called for %s (oid %s):%s, value `%s', msg %d\n", 
	     mon_data,
	     classid,
	     twinid ? twinid : "<NULL>",
	     instance,
	     value ? value : "<NULL>",
	     msg);

    return( OK );
}

/* ---------------------------------------------------------------- */

MO_CLASS_DEF mib_cdefs[] =
{
    { MO_CDATA_CLASSID, "mib.sqlSessIndex", 
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, "mib.sqlSessIndex", MOname_index },
	   
    { MO_CDATA_CLASSID, "mib.sqlSessDatabase",
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "mib.sqlSessRealuser", 
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "mib.sqlSessUser", 
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "mib.sqlSessGroup", 
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "mib.sqlSessRole", 
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "mib.sqlSessSuccStatements", 
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "mib.sqlSessErrStatements", 
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { MO_CDATA_CLASSID, "mib.sqlSessStatements", 
	  0, MO_READ, "mib.sqlSessIndex",
	  0, MOstrget, MOnoset, 0, MOidata_index },

    { 0 }
};

/*  instance filled in later, dynamically */


MO_INSTANCE_DEF mib_idefs[] =
{
    { MO_INSTANCE_VAR, "mib.sqlSessIndex", NULL, NULL },
    { MO_INSTANCE_VAR, "mib.sqlSessDatabase", NULL, (PTR)"iidbdb" },
    { MO_INSTANCE_VAR, "mib.sqlSessRealuser", NULL, (PTR)"ralph" },
    { MO_INSTANCE_VAR, "mib.sqlSessUser", NULL, (PTR)"daveb" },
    { MO_INSTANCE_VAR, "mib.sqlSessGroup", NULL,(PTR)"wheel" },
    { MO_INSTANCE_VAR, "mib.sqlSessRole", NULL, (PTR)"dbadmin" },
    { MO_INSTANCE_VAR, "mib.sqlSessSuccStatements", NULL, (PTR)"100" },
    { MO_INSTANCE_VAR, "mib.sqlSessErrStatements", NULL, (PTR)"5" },
    { MO_INSTANCE_VAR, "mib.sqlSessStatements", NULL, (PTR)"105" },
    { 0 }
};

static STATUS
tbl_attach( i4  nelem, MO_INSTANCE_DEF *idp )
{
    STATUS ret_val = OK;
    
    for( ; nelem-- && idp->classid != NULL ; idp++ )
	if( (ret_val = MOattach( idp->flags, idp->classid,
			     idp->instance, idp->idata ) ) != OK )
	    break;

    if( ret_val != OK )
	SIprintf("stat attaching %s:%s:\n%s\n",
		 idp->classid, idp->instance, er_decode( ret_val ));


    return( ret_val );
}

static STATUS
tbl_detach( i4  nelem, MO_INSTANCE_DEF *idp )
{
    STATUS ret_val = OK;
    
    for( ; nelem-- && idp->classid != NULL; idp++ )
	if( (ret_val = MOdetach( idp->classid, idp->instance ) ) != OK )
	    break;

    if( ret_val != OK )
	SIprintf("stat detaching %s:%s:\n%s\n",
		 idp->classid, idp->instance, er_decode( ret_val ));


    return( ret_val );
}

static void
addusertables(void)
{
    i4  i;
    char inststr[ 10 ];
    MO_INSTANCE_DEF *idp;
    STATUS ret_stat;
    
    (VOID) MOclassdef( MAXI2, mib_cdefs );

    for( idp = mib_idefs; idp->classid != NULL; idp++ )
	idp->instance = inststr;

    for( i = 1; i < MAXUSERS ; i++ )
    {
	CVlx( i * 64, inststr );
        ret_stat = tbl_attach( MAXI2, mib_idefs );
	SIprintf("stat attaching user %s: %s\n",
		 inststr,
		 er_decode( ret_stat ) );
    }
}


static void
delusertables(void)
{
    i4  i;
    char inststr[ 10 ];
    MO_INSTANCE_DEF *idp;
    STATUS ret_stat;
    
    for( idp = mib_idefs; idp->classid != NULL; idp++ )
	idp->instance = inststr;

    for( i = 1; i < MAXUSERS ; i++ )
    {
	CVlx( i * 64, inststr );
        ret_stat = tbl_detach( MAXI2, mib_idefs );
    }
}

static void
spptree()
{}

int
main(argc, argv)
int argc;
char **argv;
{
    STATUS rstat;
    CL_SYS_ERR sys_err;
    MO_MONITOR_FUNC *oldmon;
    char *s;

    TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err);

    SIprintf("Show before startup:\n");
    showall();

    SIprintf("\n\nAdding objects from main\n");
    
    rstat = MOclassdef( MAXI2, my_cdefs );
    SIprintf( "stat defining cdefs: %s\n\n", er_decode( rstat ) );

    SIprintf("\nshow after startup:\n");
    showall();

    SIprintf("\nChecking int set operations\n");
    rstat = MOset( ~0, "my_int", "0", "666" );
    SIprintf("stat setting my_int to 666: %s\n\n", er_decode( rstat ) );
    showcol( "my_int" );
    s = "555";
    rstat = MOset( ~0, "my_int", "0", s );
    SIprintf("stat setting my_int to \"%s\": %s\n\n", s, er_decode( rstat ) );
    showcol( "my_int" );
    s = "555";

    showcol( "my_func_value" );
    rstat = MOset( ~0, "my_func_value", "0", "999" );
    SIprintf("stat setting my_func_value to 999: %s\n\n", er_decode( rstat ) );
    showcol( "my_func_value" );

    SIprintf("\nTesting string set functions\n");
    showcol( "my_str_func" );
    s = "new str";
    rstat = MOset( ~0, "my_str_func", "0", s );
    SIprintf("stat setting my_str_func to \"%s\": %s\n\n", s,
	     er_decode( rstat ) );
    showcol( "my_str_func" );
    s = "a longer new str";
    rstat = MOset( ~0, "my_str_func", "0", s );
    SIprintf("stat setting my_str_func to \"%s\": (expect trunc)\n%s\n\n", s,
	     er_decode( rstat ) );
    showcol( "my_str_func" );
    rstat = MOset( ~0, "my_str_func", "0", "16" );
    SIprintf("stat setting my_str_func to 16: %s\n\n", er_decode( rstat ) );
    showcol( "my_str_func" );

    showcol( "my_method_index" );
    showtable( "my_method_index", method_tbl );

    SIprintf("\nAdding table objects\n");
    rstat = MOclassdef( MAXI2, ex_cdefs );
    SIprintf("stat defining ex_cdefs: %s\n\n", er_decode( rstat ) );
    rstat = tbl_attach( MAXI2, ex_idefs );
    SIprintf("stat attaching ex_idefs: %s\n\n",  er_decode( rstat ) );
    showall();

    showcol("my_id");
    showtable("my_id", id_tbl);

    SIprintf("\nMonitors...\n");
    rstat = MOset_monitor( "my_group", GROUP_MDATA2,	
	  (char *)NULL, mymonitor, &oldmon );
    SIprintf("stat monitoring my_group.%p: %s\n\n",
	     GROUP_MDATA2, er_decode( rstat ) ); 
    rstat = MOset_monitor( "my_group", GROUP_MDATA1,
		  (char *)NULL, mymonitor, &oldmon );
    SIprintf("stat monitoring my_group.%p: %s\n\n",
	     GROUP_MDATA1, er_decode( rstat ) );
    rstat = MOset_monitor( "my_id", ID_MDATA, (char *)NULL,
			  mymonitor, &oldmon );
    SIprintf("stat monitoring my_id.%p: %s\n\n",
	     ID_MDATA, er_decode( rstat ) );
    SIprintf("Added monitors for ID and group\n");
    showrow("0", (PTR)NULL );
    showrow("1000", (PTR)NULL );
    showrow("2000", (PTR)NULL );

    SIprintf("\nObjects with monitors\n");
    showall();

    SIprintf("\nDeleting monitors...\n");

    rstat = MOset_monitor( "my_group", BAD_MDATA, (char *)NULL,
			 (MO_MONITOR_FUNC *)NULL, &oldmon );
    SIprintf("stat deleting wrong group monitor: (expect err)\n%s\n\n",
	     er_decode( rstat ) );

    rstat = MOset_monitor( "my_group", GROUP_MDATA1, (char *)NULL,
			 (MO_MONITOR_FUNC *)NULL, &oldmon );
    SIprintf("stat deleting group monitor: %s\n\n", er_decode( rstat ) );

    rstat = MOset_monitor( "my_group", GROUP_MDATA1, (char *)NULL,
			 (MO_MONITOR_FUNC *)NULL, &oldmon );
    SIprintf("stat deleting group monitor again: (expect err)\n%s\n\n",
	     er_decode( rstat ) );

    rstat = MOset_monitor( "my_group", GROUP_MDATA2, (char *)NULL,
			 (MO_MONITOR_FUNC *)NULL, &oldmon );
    SIprintf("stat deleting second group monitor: %s\n\n",
	     er_decode( rstat ) );

    rstat = MOset_monitor( "my_id", ID_MDATA, (char *)NULL,
			 (MO_MONITOR_FUNC *)NULL, &oldmon );
    SIprintf("stat deleting id monitor: %s\n\n", er_decode( rstat ) );

    SIprintf("\nAdding some users to a session table\n");
    addusertables();
    showcol("mib.sqlSessIndex");
    showtable("mib.sqlSessIndex", big_tbl );

    SIprintf("\nBefore delete:\n");
    showall();

    SIprintf("\ndeleted user entries -- should be no string refs\n");
    delusertables();
    showcol("mib.sqlSessIndex");
    showtable("mib.sqlSessIndex", big_tbl );

    SIprintf("\nAt the end:\n");
    showall();

    SIprintf("MO_classes tree %p\n", MO_classes );
    spptree( MO_classes );

    SIprintf("MO_strings tree %p\n", MO_strings );
    spptree( MO_strings );

    SIprintf("MO_instances tree %p\n", MO_instances );
    spptree( MO_instances );

    SIprintf("MO_monitors tree %p\n", MO_monitors );
    spptree( MO_monitors );

    PCexit( OK );
    return( FAIL );
}
