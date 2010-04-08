/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<iicommon.h>
# include	<iisqlca.h>
# include	<iilibq.h>

/* {
** Name: iilqadr.c - Return pointer to static copy of argument.
**
** Description:
**	These functions enable the generated ESQL/C and EQUEL/C interface to
**	get around the C mechanism of passing by value.  They are used to give
**	a consistent interface to the various INPUT functions in the INGRES
**	run-time libraries.
**
**	Data in variables are passed to run-time by reference using the `&`
**	operator.  The functions in this module allow literals and symbolic
**	constants to be passed by reference too.
**	
**	A consistent interface brings ESQL closer to ANSI/C compatibility.
**	It also avoids the problem on some RISC architectures where double
**	parameters by value are passed differently from pointers, ie, doubles
**	are passed in a special set of registers.  The run-time functions,
**	which expect a generic pointer argument, will not work with by
**	value doubles. 
**
**	An ESQL statement and the generated code follows.  It shows the use
**	of one of the functions in this module.
**
**	EXEC SQL BEGIN DECLARE SECTION;
**	#define dbl_const 12.34
**	EXEC SQL END DECLARE SECTION;
**
**	EXEC SQL INSERT INTO tab VALUES (:dbl_const);
**
**	generates:
**
**	    IIsqInit(...);
**	    IIwritio(...);
**	    IIputdomio((short *)0, 1, 31, 8, (char *)IILQdbl(:dbl_const));
**	    IIwritio(...);
**	    IIsyncup(...);
**
** Defines:
**	IILQdbl	- Return pointer to static copy of double passed in.
**	IILQint - Return pointer to static copy of integer passed in.
**
** History:
**	28-nov-1990	(barbara)
**	    Written to correct problem with EQL interface on DecStations.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Mar-2003 (hanch04)
**	    changed PTR to void * because it caused compilation errors 
**	    in less tolerant compilers
*/

/*{
** Name:	IILQdbl - Return pointer to static copy of double argument.
**
** Description:
**	Called by ESQL/C and EQUEL/C programs.  See comments in module
**	header.
** Inputs:
**	dblval		A double
** Outputs:
**	None.
** Returns:
**	Pointer to static that has been assigned value of 'dblval'.
** Errors:
**	None.
** Side Effects
**	None.
*/
void *
IILQdbl( dblval )
double dblval;
{
    II_THR_CB	*thr_cb = IILQthThread();
    thr_cb->ii_th_fparm = dblval;
    return( (void *)&thr_cb->ii_th_fparm );
}

/*{
** Name:	IILQint - Return pointer to static copy of integer argument.
**
** Description:
**	Called by ESQL/C and EQUEL/C programs.  See comments in module header.
** Inputs:
**	intval		An integer
** Outputs:
**	None.
** Returns:
**	Pointer to static that has been assigned value of 'intval'.
** Errors:
**	None.
** Side Effects
**	None.
*/
void *
IILQint( intval )
i4  intval;
{
    II_THR_CB	*thr_cb = IILQthThread();
    thr_cb->ii_th_iparm = intval;
    return( (void *)&thr_cb->ii_th_iparm );
}
