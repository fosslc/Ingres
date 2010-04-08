# include	<compat.h>
#include    <gl.h>
# include	<si.h>

/*
**	SIdata.c
**
**	Define file structures for SI.
**
**	History:
**		???
**		4/91 (Mike S) Allow vectoring of the data structures.
**      16-jul-93 (ed)
**	    added gl.h
*/

/* 
** external file structures 
**
**	SI_iobuffers is the array of FILE structures used by SI.  Only the CL
**	transfer vector is allowed to reference it.  All other reference to
**	the FILE structures must be through the pointer SI_iob.  The front-end
**	CL transfer vector (cl.mar) will redefine SI_iob in its transfer vector.
*/
globaldef {"iicl_gvar$not_ro"}	noshare FILE SI_iobuffers[_NFILE];
globaldef {"iicl_gvar$not_ro"}  noshare FILE *SI_iobptr = SI_iobuffers;
