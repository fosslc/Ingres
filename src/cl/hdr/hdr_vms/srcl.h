/*
** Copyright (c) 1985, 2000 Ingres Corporation
*/

/**CL_SPEC
** Name: SR.H - This file contains the external structs for SR.
**
** Description:
**      The SR.H file is used to contain all structures and 
**      definitions required to call the compatability library
**      SR routines.  These routines perform all direct file
**      I/O for all sorts.  The main structure used to keep 
**      track of operating system information
**      needed to perform the I/O is called SR_IO and is defined
**      below.  This structure varies significantly depending
**      on the operating system.
**
** Specification:
**
**  Description:  
**	The SR routines perform all the direct file I/O for 
**      all sorts. The SR routines have been separated from the DI
**	routines, which perform direct I/O on shared files, to allow
**      easy modification of the sort package without affecting the DI
**	routines.  This also provides for special I/O handling to insure
**	that SORTs perform as fast as possible. It has also been separated
**	out in the hope that the entire sort package can be moved to the CL
**	to  provide a better way of implementing specialized sort packages
**	such as the ACCEL sorter with minimal changes to the DBMS code.  
**      
**  Intended Uses:
**	The SR routines are intended for the sole use of the SORT
**      routines in DMF.  
**      
**  Assumptions:
**(	The following assumptions apply to the SR routines:
**	    o  Files are not shared.
**          o  Files are temporary.
**          o  Files are created with a name but never opened by name.
**          o  Should be able to handle variable length I/O.  That is,
**	       multiple physical blocks can be as a single logical
**	       request as long as the logical request is a multiple of
**	       the physical block size.
**	    o  Sorting I/O must be as efficient as possible.
**)
**  Definitions:
**
**  Concepts:
**
**	The page size is determined at open.  The page size can vary from
**	4096 to 32768 in powers of 2.  The allocates must always be 
**      obtained in the maximum size of 32K to insure that the file
**      can be read even if it was written with 4K and read with 32K.
**      
** History:
**      30-sep-85 (jennifer)
**          Created new for 5.0.
**	27-jul-87 (rogerk)
**	    Added return code SR_EXCEED_LIMIT.
**	26-apr-1993 (bryanp)
**	    Prototyping code for 6.5.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	15-Oct-2009 (kschendel) SIR 122512
**	    Need an SR_BADPARAM error code.
**/

/*
**  Forward and/or External typedef/struct references.
*/

/*
The following structures are defined below:
*/
typedef struct _SR_IO SR_IO;

/*
**  Forward and/or External function references.
*/


/*
** Defined Constants
*/

/* SR return status codes. */

# define                SR_OK           0
# define		SR_BADFILE	(E_CL_MASK + E_SR_MASK + 0x01)
# define                SR_ENDFILE      (E_CL_MASK + E_SR_MASK + 0x02)
# define		SR_BADOPEN	(E_CL_MASK + E_SR_MASK + 0x03)
# define		SR_BADCLOSE	(E_CL_MASK + E_SR_MASK + 0x04)
# define		SR_BADWRITE	(E_CL_MASK + E_SR_MASK + 0x05)
# define		SR_BADREAD	(E_CL_MASK + E_SR_MASK + 0x06)
# define		SR_BADEXTEND	(E_CL_MASK + E_SR_MASK + 0x07)
# define		SR_BADCREATE	(E_CL_MASK + E_SR_MASK + 0x08)
# define		SR_EXCEED_LIMIT	(E_CL_MASK + E_SR_MASK + 0x09)
# define		SR_BADPARAM	(E_CL_MASK + E_SR_MASK + 0x0A)

/*{
** Name: SR_IO - This is the I/O structure needed to do sort I/O.
**
** Description:
**      The SR_IO structure is used to contain all the operating
**      system dependent I/O information required for direct I/O.
**      It is very operating system specific.  The memory for this
**      structure is allocated by the caller of the SR routines.
**
** History:
**     30-sep-85 (jennifer)
**          Created new for 5.0.
*/
typedef struct _SR_IO
{
    i4	    io_allocated;	/* Last allocated disk block. */
    i4	    io_type;		/* Control block identifier. */
#define                 SR_IO_ASCII_ID     'IOCB'
    i4	    io_channel;		/* VMS channel used for this file */
    i4	    io_block_size;	/* File block size. */
    i4	    io_log2_blk;	/* Log 2 of io_block_size. */
    i4	    io_blks;		/* VMS block per SR block. */
    struct
    {
     i4	    str_len;		/* Length of described item. */
     char   *str_str;		/* Address of described item. */
    }	    io_fib_desc;        /* FIB descriptor. */
    char    io_fib[32];		/* VMS File Information Block. */
};

/* 
** Related symbolic constants used to initialize SR_IO 
** information.
*/

#define                 SR_IO_NODELETE     0
#define                 SR_IO_DELETE       1
#define                 SR_IO_NOCREATE     0
#define                 SR_IO_CREATE       1
