/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** motest2.c -- simple incomplete test/example program for mo interface.
**
** History:
**	7-jul-92 (daveb)
**		stripped from motest.c
**
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


/* format and print something nicely */

static void
show( char *classid, 
     char *instance, 
     char *sval, 
     i4  perms,
     PTR arg )
{
    SIprintf("%-40s|%-40s|%-40s|0%o\n", classid, instance, sval, perms );
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


int
main(argc, argv)
int argc;
char **argv;
{
    CL_SYS_ERR sys_err;

    TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err);

    showall();
# if 0
    spptree( MO_classes );
    spptree( MO_strings );
    spptree( MO_instances );
    spptree( MO_monitors );
# endif
    PCexit( OK );
    return( FAIL );
}

