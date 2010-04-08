/*
**    Copyright (c) 1991, 2000 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include 	<me.h>
# include	<jpidef.h>
# include	<lib$routines.h>

/*
**  MEuse.c
**
**  History
**
**	4/91 (Mike S)
**		Keep ME statistics; these can be used for debugging.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      08-feb-93 (pearl)
**              Add #include for me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    unconditional prototypes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/
i4
MEuse(
	VOID)
{
    i4	size;
    i4	code = JPI$_FREP0VA;

    static i4 first = 0;

    /*
    ** On VMS, we can't distinguish the heap.  We use the current size of 
    ** P0 space.
    */
    lib$getjpi(&code, NULL, NULL, &size);
    if (first == 0)
    {
	first = size;
	return 0;
    }
    else
    {
	return (i4) size - first;
    }

}
