/*
**  Copyright (c) 2004 Ingres Corporation
**  All rights reserved.
*/

#include	<me.h>

/**
** Name:	feloc.h -	Front-End Utilities Internal
**					Constant Definitions File.
**/

#ifdef FE_ALLOC_DBG
#undef	FE_ALLOC_DBG		/* Debug Flag ... (normally undef) */

#undef FE_ALLOC_DBGLIST		/* Debug Flag - to use, FE_ALLOC_DBG */
				/* Must be defined */

#undef	FE_ALLOC_DBGSUM		/* Debug Flag - to use, FE_ALLOC_DBG */
#undef FE_VM_STAT		/* Allows statistics gathering */
#endif /* FE_ALLOC_DBG */

/*
**	17-aug-91 (leighb) DeskTop Porting Change:  
**		MINTAGID must be larger for PMFE project - get from compat.h
**
**	7/93 (Mike S) 
**		Remove MAXTAGID, FETAGSIZ and MININDEX,
**		which no longer apply
**	22-mar-96 (chech02)
**		Added function prototypes for win 3.1 port.
**	3-apr-1997 (donc)
**		Bumped ALLOCSIZE from 512 to 2064 for OpenROAD 4.0
**	7-apr-1997 (donc)
**		Still not good enough. Bumped ALLOCSIZE from 2064 to 65534
**		for OpenROAD 4.0
**	3-jun-1997 (donc)
**		Set ALLOCSIZE back to 512,  as 65534 is no longer necessary
**		for OpenROAD. 
**	22-feb-2001 (somsa01)
**		Changed fe_bytfree to SIZE_TYPE.
*/

#define       ME_NODE_SIZE    16              /* Maximum ME_NODE size */

#define	ALLOCSIZE	(512 - ME_NODE_SIZE)     /* FE allocation size */

/*
** Typedefs
*/

typedef	struct	feblk			/* FE memory block header */
{
	SIZE_TYPE	fe_bytfree;	/* # bytes unused in the block */
	PTR		fe_freeptr;	/* Pointer to next free elmt in block */
	struct feblk 	*fe_next;	/* Pointer to next block in list */

# ifdef	CONCEPT32
	i4		fe_pad;		/* align to 8 bytes for Gould */
# endif

} FE_BLK;

#ifdef WIN16
PTR FEreqmem (u_i4,u_i4, bool, STATUS);
#endif /* WIN16 */
