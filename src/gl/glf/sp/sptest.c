/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
NEEDLIBS = COMPAT
*/

# include <compat.h>
# include <gl.h>
# include <tr.h>

main()
{
    CL_SYS_ERR sys_err;
    TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err);

    TRdisplay( "Float:  %f\n", 123.456789 );

    TRdisplay( "Float as int/int %f\n", (double)15 / 4 );

    exit( 0 );
}
