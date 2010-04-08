/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqlang.h>
# include	<iirowdsc.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<lqgdata.h>

/**
+*  Name: iiglblvar.c - Application run-time library globals.
**
**  Defines:
** 	IIglbcb 	- Pointer to a global II_GLB_CB structure containing
**			  information that is used for query processing across
**			  multiple database sessions.  Currently there is one
**			  static II_GLB_CB pointed at by IIglbcb for the
**			  duration of the program.
**
**	History:
**		11/30/89 (Mike S)	Move global data into structure.
-*
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPEMBEDLIBDATA
**	    in the Jamfile. Added LIBOBJECT hint which creates a rule
**	    that copies specified object file to II_SYSTEM/ingres/lib
*/

/*
**Jam hints
**
LIBRARY = IMPEMBEDLIBDATA
**
LIBOBJECT = iiglobal.c
**
*/

/* Define global LIBQ query processing structure */
static 	  II_GLB_CB 	__iiglbcb ZERO_FILL;
GLOBALDEF II_GLB_CB	*IIglbcb = &__iiglbcb;

/* Globals to support other front ends 
**
**      Only the VMS transfer vector refers to sIILIBQgdata directly; all
**      mainline code MUST reference the pointer IILIBQgdata.
*/
 
GLOBALDEF LIBQGDATA       sIILIBQgdata =
{
0,    		/*form_on:	Forms flag*/
0,              /*os_errno:	Remember OS errors */
NULL,     	/*f_end:	Set/reset Forms Mode*/
NULL,   	/*win_err:	FRS error function */
NULL   	        /*excep:	Error exception routine */
};
	 
GLOBALDEF LIBQGDATA       *IILIBQgdata = &sIILIBQgdata;
