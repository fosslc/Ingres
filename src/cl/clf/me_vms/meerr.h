/*
** Copyright (c) 1985, Ingres Corporation
*/

/**
** Name: ME.H - Global definitions for the ME compatibility library.
**
** Description:
**      The file contains the local types used by ME.
**      These are used for memory manipulation.
**
** History:
**      03-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**/

/*
**  Forward and/or External function references.
*/

/* 
** Defined Constants
*/

/* ME return status codes. */

# define	 ME_GOOD	(E_CL_MASK + E_ME_MASK + 0x01)
	/*  ME routine: The Status returned was good  */
# define	 ME_BD_CHAIN	(E_CL_MASK + E_ME_MASK + 0x02)
	/*  MEdump: correct parameter value must be one of ME_D_ALLOC, 
        **  ME_D_FREE, ME_D_BOTH  */
# define	 ME_BD_CMP	(E_CL_MASK + E_ME_MASK + 0x03)
	/*  MEcmp: number of bytes to compare must be > 0  */
# define	 ME_BD_COPY	(E_CL_MASK + E_ME_MASK + 0x04)
	/*  MEcopy: number of bytes to copy must be > 0  */
# define	 ME_BD_FILL	(E_CL_MASK + E_ME_MASK + 0x05)
	/*  MEfill: number of bytes to fill must be > 0  */
# define	 ME_BD_TAG	(E_CL_MASK + E_ME_MASK + 0x06)
	/*  MEt[alloc, free]: tags must be > 0  */
# define	 ME_ERR_PROGRAMMER	(E_CL_MASK + E_ME_MASK + 0x07)
	/*  MEfree: There is something wrong with the way this 
        **  routine was programmed.  Sorry.  */
# define	 ME_FREE_FIRST	(E_CL_MASK + E_ME_MASK + 0x08)
	/*  ME[t]free: can't free a block before any blocks have 
        **  been allocated  */
# define	 ME_GONE	(E_CL_MASK + E_ME_MASK + 0x09)
	/*  ME[t]alloc: system can't allocate any more memory for 
        **  this process  */
# define	 ME_NO_ALLOC	(E_CL_MASK + E_ME_MASK + 0x0A)
	/*  ME[t]alloc: request to allocate a block with zero (or less) 
        **  bytes was ignored  */
# define	 ME_NO_FREE	(E_CL_MASK + E_ME_MASK + 0x0B)
	/*  MEfree: can't find a block with this address in the free 
        **  list  */
# define	 ME_NO_TFREE	(E_CL_MASK + E_ME_MASK + 0x0C)
	/*  MEtfree: process hasn't allocated any memory with this tag  */
# define	 ME_00_PTR	(E_CL_MASK + E_ME_MASK + 0x0D)
	/*  ME[t]alloc: passed a null pointer  */
# define	 ME_00_CMP	(E_CL_MASK + E_ME_MASK + 0x0E)
	/*  MEcmp: passed a null pointer  */
# define	 ME_00_COPY	(E_CL_MASK + E_ME_MASK + 0x0F)
	/*  MEcopy: passed a null pointer  */
# define	 ME_00_DUMP	(E_CL_MASK + E_ME_MASK + 0x10)
	/*  MEdump: passed a null pointer  */
# define	 ME_00_FILL	(E_CL_MASK + E_ME_MASK + 0x11)
	/*  MEfill: passed a null pointer  */
# define	 ME_00_FREE	(E_CL_MASK + E_ME_MASK + 0x12)
	/*  MEfree: passed a null pointer  */
# define	 ME_TR_FREE	(E_CL_MASK + E_ME_MASK + 0x13)
	/*  MEfree: the memory has been corrupted  */
# define	 ME_TR_SIZE	(E_CL_MASK + E_ME_MASK + 0x14)
	/*  MEsize: the memory has been corrupted  */
# define	 ME_TR_TFREE	(E_CL_MASK + E_ME_MASK + 0x15)
	/*  MEtfree: the memory has been corrupted  */
# define	 ME_OUT_OF_RANGE	(E_CL_MASK + E_ME_MASK + 0x16)
	/*  ME routine: address is out of process's data space, 
        **  referencing will cause bus error  */
# define	 ME_BF_OUT	(E_CL_MASK + E_ME_MASK + 0x17)
	/* MEneed: 'buf' doesn't have 'nbytes' left to allocate */
# define	 ME_BF_ALIGN	(E_CL_MASK + E_ME_MASK + 0x18)
	/* MEinitbuf: 'buf' not aligned */
# define	 ME_BF_FALIGN	(E_CL_MASK + E_ME_MASK + 0x19)
	/* MEfbuf: 'buf' not aligned */
# define	 ME_BF_PARAM	(E_CL_MASK + E_ME_MASK + 0x1A)
	/* MEfbuf: 'bytes' argument must come from call to MEfbuf() */
