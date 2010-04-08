/*
**    Copyright (c) 1985, 2000 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <TR.h>

/**
**
**  Name: TRTEST.C - Test the TR compatibility library routines
**
**  Description:
**      This file contains the test program for the TR routines.
**
{@func_list@}...
**
**
**  History:    $Log-for RCS$
**      01-oct-1985 (derek)    
**          Created new for 5.0.
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**/

/*{
** Name: MAIN	- Main function of test routine.
**
** Description:
**      This function gets control from the operating system, and performs
**	tests of the TR routines.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    0
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-oct-1985 (derek)
**          Created new for 5.0.
*/

# undef main

i4
main()
{
    i4	    status;
    i4	    *err_code;
    char	    buffer[256];
    i4		    *amount_read;

    /*	Open a log file and a terminal file. */

    status = TRset_file(TR_F_OPEN, "trtest_file.output", 18, &err_code);
    if (status)
	errmsg("main(1):Unexpected error from TRset_file", status, err_code);

    status = TRset_file(TR_T_OPEN, "trtest_terminal.output", 22, &err_code);
    if (status)
	errmsg("main(2):Unexpected error from TRset_file", status, err_code);

    /*	Try all the format specifiers. */

    TRdisplay("Integer: %d\nInteger: %d\n", 12345, -67890);
    TRdisplay("Float: %f\nFloat: %f\n", 12345.678, 1234567890123456.534354);
    TRdisplay("String: %s\n", "A C style string");
    TRdisplay("Hexidecimal: 0x%x:\n", 0x1234fedc);
    TRdisplay("Text: %t\n", 25, "A length described string");
    TRdisplay("Hex Dump   0x%x:%< %4.4{%x %}%2< >%16.1{%c%}<\n", "0123456789ABCDEF", 0);
    TRdisplay("Try wrapping %132.1{%c%}\n", buffer);
    TRdisplay("Word1=%w Word2=%w Word3=%w.\n", "one,two", 0, "one,two", 1, "one,two", 2);
    TRdisplay("Field=%v.\n", "zero,one,two,three,four,five", 0xff);

    /*	Close the terminal file. */

    status = TRset_file(TR_T_CLOSE, 0, 0, &err_code);    
    if (status)
	errmsg("main(3):Unexpected error from TRset_file", status, err_code);

    /* Open the input file as the old terminal output file. */

    status = TRset_file(TR_I_OPEN, "trtest_terminal.output", 22, &err_code);
    if (status)
	errmsg("main(4):Unexpected error from TRset_file", status, err_code);

    /*	Read the input file until end of file. */

    for (;;)
    {
	status = TRrequest(buffer, sizeof(buffer), &amount_read, &err_code);
	if (status)
	    if (status == TR_ENDINPUT)
		break;
	    else
		errmsg("main(5):Unexpected error from TRset_file",
		    status, err_code);
	
	TRdisplay("Line read >%t<\n", amount_read, buffer);
    }

    /*	Close the rest of the files. */

    status = TRset_file(TR_I_CLOSE, 0, 0, &err_code);
    if (status)
	errmsg("main(6):Unexpected error from TRset_file", status, err_code);

    status = TRset_file(TR_F_CLOSE, 0, 0, &err_code);
    if (status)
	errmsg("main(6):Unexpected error from TRset_file", status, err_code);

    return (1);
}

/*{
** Name: errmsg	- Format error message about failing test.
**
** Description:
**      Write an error message to the output device.
**
** Inputs:
**      string                          The string to print.
**	status				The status code.
**	err_code			CL error code.
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-sep-1985 (derek)
**          Created new for 5.0.
*/
errmsg(err_string, status, err_code)
char               *err_string;
i4            status;
i4            err_code;
{

    printf("TRTEST: %s : status %%%X : err_code %%%X\n",
	err_string, status, err_code);
}
