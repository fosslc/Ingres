/*
** Copyright (c) 2004 Ingres Corporation
*/

#ifndef		SR_INCLUDED
#define		SR_INCLUDED	1

#include	<di.h>

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
**	The following assumptions apply to the SR routines:
**	    o  Files are not shared.
**          o  Files are temporary.
**          o  Files are created with a name but never opened by name.
**          o  Should be able to handle variable length I/O.  That is,
**	       multiple physical blocks can be as a single logical
**	       request as long as the logical request is a multiple of
**	       the physical block size.
**	    o  Sorting I/O must be as efficient as possible.
**
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
**	    Prototyping SR for 6.5.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**  Forward and/or External typedef/struct references.
*/

/*
The following structures are defined below:
*/
typedef DI_IO	SR_IO;

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
# define		SR_BADPARAM	(E_CL_MASK + E_SR_MASK + 0x0a)
# define		SR_BADDIR	(E_CL_MASK + E_SR_MASK + 0x0b)

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
#define                 SR_IO_ASCII_ID     DI_IO_ASCII_ID
#ifdef NOT
typedef struct _SR_IO
{
    i4	    io_system_eof;	/* End of file information. */
    i4	    io_alloc_eof;	/* Last allocated disk block. */
    i4	    io_type;            /* Control block identifier. */
    i4	    io_mode;            /* mode the file was opened with */
#define                 SR_IO_ASCII_ID     'IOCB'
    i4 	    io_bytes_per_page;	/* File block size. */
    i4	    io_log2_bksz;	/* Log 2 of io_block_size. */
    i4	    io_blks_log2;	/* Log 2 of io_block_size / 512. */
#define			DI_IO_BLK_SIZE		512
    i4	    io_logical_eof;	/* logical end of file */
    DI_UNIQUE_ID    io_uniq;		/* uniq identifier for this file */
    i4	    	    io_l_pathname;	/* length of pathname */
    i4	   	    io_l_filename;	/* length of filename */
    char	    io_pathname[DI_PATH_MAX];
					/* pathname copied from DIopen() */
    char	    io_filename[DI_FILENAME_MAX];
					/* filename copied from DIopen() */
}
#endif

/* 
** Related symbolic constants used to initialize SR_IO 
** information.
*/

#define                 SR_IO_NODELETE     0
#define                 SR_IO_DELETE       1
#define                 SR_IO_NOCREATE     0
#define                 SR_IO_CREATE       1

#endif		/* SR_INCLUDED */
