/*
** Copyright (c) 2004 Ingres Corporation
*/

#ifndef		JF_INCLUDED
#define		JF_INCLUDED	1

#include	<di.h>

/**CL_SPEC
** Name: JF.H - Definitions of types and constants.
**
** Description:
**      This file contains the definitions of types, functions and constants
**	needed to make calls on the compatibility library JF functions.  These
**	routines perform all the journaling services for DMF.  Futhermore,
**	non-DMF uses of the JF routines are prohibited by definition.
**
** Specification:
**
**   Description:
**	File operations required to support journaling.
**
**   Intended Uses:
**	The JF services are intended for use by the DMF code to manage  the
**	set of journal files for a database.  A journal file contains a
**	history of all journaled changes made to a database.  A destroyed
**	database can be recovered from the contents of the journal file.  In
**	conjunction with a checkpoint, a subset of the journal files can be
**	used to recover a database. These files are named by 
**      DMF and reside at an INGRES location specified for journals 
**      by DB_ACCESS.
**
**   Assumptions:
**	The journal file can be written to disk or tape.  The current
**	implementation only considers disks, but future expansion to
**	include the use of tapes has been considered.
**
**   Definitions:
**
**(	Block sequence	- Each block written to the journal file is
**			  assumed to be assigned a sequence number.
**			  This sequence number is used to determine
**			  whether this block has been seen before.
**				  
**	Journal file	- A journal file is a operating system file that
**			  contains a sequence of database actions
**			  performed by DMF against a database over a
**			  certain period of time. A journal file name
**			  must be at least 12 bytes long.  The format
**			  is always Jnnnmmmm.JNL, where nnn and mmmm
**			  are numbers.  These number keep track of
**                        the versions of the journal files.  In case
**                        there are multiple versions since a checkpoint.
**                        These files reside at a location
**		          specified by the user when a database is 
**                        created or by DB_ACCESS.  In other words
**                        the path will be the value returned from
**		          LOingpath  when the what parameter is set
**                        to JNL. The file is 
**                        extended automatically by writing a block past 
**                        the EOF, it may not contain "holes", and 
**                        does not need to support concurrent opens.
** 
**	Journal block	- A journal file is created with a certain block
**			  size.  This block size governs the size of 
**			  each transfer to and from the journal file.
**			  The contents of the block are completely 
**			  ignored by this version of JF.  It only
**                        reads and writes the contents, but does not
**                        analyze it.  Journal i/o is performed by
**                        requesting logical block numbers.  The first
**                        logical block of a file is 1. Logical block
**		          zero is never used.  Physical blocks of 
**                        a journal file do not have to map directly
**                        onto logical blocks.  That is if a particular
**                        implementation of JF needed to reserve 
**                        10 physical blocks at the beginning of the
**                        file for control information, then physical
**                        block 11 would map to logical block 1 which
**                        would be the first logical block in the file.
**
**	Journal record	- The journal file actually contains a sequence
**			  of journal records.  The mapping of records 
**			  to blocks is considered a function of the
**			  client.  In future versions of JF, the record
**			  mapping may be included.
**
**	Journal set	- A single journal file contains a sequence
**			  of database changes for a specific time
**			  period.  All journal files from the first
**			  one created for a database to the last one
**			  used constitute a journal set.  This is the
**			  complete history of changes.
**)
**   Concepts:
**
**	The JF services are very simple and straight forward in the 
**	currrent form.  When the ability to write to tape and the inclusion
**	of record blocking are added the complexity of these routines will
**	increase substantially.  Most of the increase in complexity comes
**	from the manipulation of tape volumes.
**
**	A journal file is like any normal file, other than the fact that
**	each block is tagged with a header that records information used to
**	enhance the reliability of the software.
**
**	Journal blocks are not necessarily full.  They can be partially
**	filled.
**
**	Journal blocks are never logically rewritten on disks and never
**	physical rewritten on tapes.  Rewriting a tape block is not
**	supported on a lot of tape equipment, as well as being less 
**	reliable when used (Unless an intelligent tape controller is
**	using a mechanism similar to our block sequence writing.)
**
**	The block size of a journal file can range from 4096 to 32768 in
**	powers of 2.
**
**	The maximum number of blocks in a journal file is 2^32-1.
**
**	The maximum number of journal files in 9,999,999.
**
**    Unix Implementation:
**
**	JF files depend on the concept of user controlled end-of-file
**	independent of writes.  The user must be able to set the end-of-file
**	returned by JFopen() by a JFupdate().  The end-of-file set by JFupdate()
**	must persist across system crashes, so on UNIX must be stored as part
**	of the file.
**
**	The current implementation of JF is built on top of the DI routines.
**	The DI pagesize is set to 512, and the actual mapping of di-blocksize to
**	jf blocksize takes place by specifying multiple page reads and writes of
**	DI.
**
** History: $Log-for RCS$
**      24-sep-1986 (Derek)
**          Created.
**	22-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Remove superfluous "typedef" before structure.
**	26-apr-1993 (bryanp)
**	    Prototyping JF for 6.5.
**	26-apr-1993 (bryanp)
**	    Please note that arg_list is a PTR, not a (PTR *).
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add #ifdef JF_INCLUDED to expose dm0j
**	    to other spots in DMF.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _JFIO JFIO;

/*
**  Forward and/or External function references.
*/


/*
**  Definitions of JF error codes.
*/

# define		JF_BAD_FILE	(E_CL_MASK + E_JF_MASK + 0x01)
# define                JF_END_FILE     (E_CL_MASK + E_JF_MASK + 0x02)
# define		JF_BAD_OPEN	(E_CL_MASK + E_JF_MASK + 0x03)
# define		JF_BAD_CLOSE	(E_CL_MASK + E_JF_MASK + 0x04)
# define		JF_BAD_WRITE	(E_CL_MASK + E_JF_MASK + 0x05)
# define		JF_BAD_READ	(E_CL_MASK + E_JF_MASK + 0x06)
# define		JF_BAD_ALLOC	(E_CL_MASK + E_JF_MASK + 0x07)
# define		JF_BAD_UPDATE	(E_CL_MASK + E_JF_MASK + 0x08)
# define		JF_BAD_CREATE   (E_CL_MASK + E_JF_MASK + 0x09)
# define		JF_BAD_PARAM	(E_CL_MASK + E_JF_MASK + 0x0A)
# define		JF_NOT_FOUND	(E_CL_MASK + E_JF_MASK + 0x0B)
# define		JF_BAD_DELETE	(E_CL_MASK + E_JF_MASK + 0x0C)
# define		JF_DIR_NOT_FOUND (E_CL_MASK + E_JF_MASK + 0x0D)
# define		JF_BAD_LIST	(E_CL_MASK + E_JF_MASK + 0x0E)
# define		JF_EXISTS	(E_CL_MASK + E_JF_MASK + 0x0F)
# define		JF_BAD_DIR	(E_CL_MASK + E_JF_MASK + 0x10)
# define		JF_BAD_TRUNCATE	(E_CL_MASK + E_JF_MASK + 0x11)

/*}
** Name: JFIO - Journal file context.
**
** Description:
**      This structure contains information initialized and used by the JF
**	routines to manage an open journal file.  The actual structure is
**	allocated by the caller.  This is the structure used on VAX/VMS.
**
** History:
**      24-sep-1986 (Derek)
**          Created
[@history_template@]...
*/
struct _JFIO
{
    i4		jf_ascii_id;	    /*  Visible identifier. */
#define			    JF_ASCII_ID	    CV_C_CONST_MACRO('J', 'F', 'I', 'O')
    i4		jf_allocate;	    /*	Number of allocated VMS block. */
    i4		jf_blk_size;	    /*  Block size for transfer. */
    i4		jf_log2_bksz;	    /*  Log 2 of block size. */
    i4		jf_blk_cnt;	    /*  blocks in a JF block. */
    i4		jf_cur_blk;	    /*  Current JF block number. */
    DI_IO		jf_di_io;	    /*  JF IO will use DI */
};

#endif		/* JF_INCLUDED */
