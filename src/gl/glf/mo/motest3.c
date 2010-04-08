/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
NEEDLIBS = COMPAT
*/

# include <compat.h>
# include <gl.h>
# include <si.h>
# include <tr.h>
# include <sp.h>
# include <erglf.h>
# include <mo.h>

int
main( int argc, char **argv )
{
    char buf[ 80 ];
    CL_SYS_ERR sys_err;

    TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err);

    MOulongout( 0, ~0, 80, buf );
    SIprintf("buf of ~0 is %s\n", buf );

    MO_dumpmem( buf, sizeof( buf ) );
    exit( 0 );
}

