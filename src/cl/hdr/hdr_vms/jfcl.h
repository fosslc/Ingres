/*
** Copyright (c) 1986, 2000 Ingres Corporation
*/

#ifndef		JF_INCLUDED
#define		JF_INCLUDED	1

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
**	The JF services are intended for use by the DMF to manage  the
**	set of journal files for a database and the dump files generated
**	during an online backup.  A journal file contains a
**	history of all journaled changes made to a database.
**	A dump file contains the before images of database pages changed 
**	during an online backup.  In conjunction with a checkpoint, a subset 
**	of the journal files can be used to recover a database. 
**	These files are named by DMF and reside at an INGRES location 
**	specified for journals by ii_journal.
**
**   Assumptions:
**	The journal or dump file can be written to disk or tape.  The current
**	implementation only supports disks.
**
**   Definitions:
**	When a database is recovered, the following occurs:
**	    1. CKrestore overlays the database area with the contents of
**	       ii_checkpoint.
**	    2. If the backup was done online, the BIs in ii_dump are qpplied
**	       to the restored database.
**	    3. The journals are needed to "rollforward" the database.
**
**(	Block sequence	- Each block written to the journal or dump file is
**			  assumed to be assigned a sequence number.
**			  This sequence number is used to determine
**			  whether this block has been seen before.
**				  
**	Journal file	- A journal file is an operating system file that
**			  contains a sequence of database actions
**			  performed by DMF against a database over a
**			  certain period of time. A journal file name
**			  must be at least 12 bytes long.  The format
**			  is always Jnnnmmmm.JNL, where nnn and mmmm
**			  are numbers.  These numbers keep track of
**                        the versions of the journal files.  In case
**                        there are multiple versions since a checkpoint.
**                        These files reside at a location
**		          specified by the user when a database is 
**                        created or by ACCESSDB.  In other words
**                        the path will be the value returned from
**		          LOingpath  when the what parameter is set
**                        to JNL. The file is 
**                        extended automatically by writing a block past 
**                        the EOF, it may not contain "holes", and 
**                        does not need to support concurrent opens.
**
**	Dump file	- A dump file is an operating system file that
**			  contains a sequence of database before images 
**			  performed by DMF against a database during 
**			  online backup. A dump file name
**			  must be at least 12 bytes long.  The format
**			  is always Dnnnmmmm.DMP, where nnn and mmmm
**			  are numbers.  These numbers keep track of
**                        the versions of the dump files.  In case
**                        there are multiple versions since a checkpoint.
**                        These files reside at a location
**		          specified by the user when a database is 
**                        created or by ACCESSDB.  In other words
**                        the path will be the value returned from
**		          LOingpath  when the what parameter is set
**                        to DMP. The file is 
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
**	Dump block	- A dump file is created with a certain block
**			  size.  This block size governs the size of 
**			  each transfer to and from the dump file.
**			  The contents of the block are completely 
**			  ignored by this version of JF.  It only
**                        reads and writes the contents, but does not
**                        analyze it.  Dump i/o is performed by
**                        requesting logical block numbers.  The first
**                        logical block of a file is 1. Logical block
**		          zero is never used.  Physical blocks of 
**                        a dump file do not have to map directly
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
**	Dump record	- The dump file actually contains a sequence
**			  of dump records.  The mapping of records 
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
**
**	Dump set	- A single dump file contains a sequence
**			  of database changes for a specific time
**			  period.  All dump files from the first
**			  one created for a database to the last one
**			  used constitute a dump set.  This is the
**			  complete history of changes.
**)
**   Concepts:
**
**	The JF services are very simple and straight forward in the 
**	currrent form.  When the ability to write to tape and the inclusion
**	of record blocking are added the complexity of these routines will
**	increase substantially.  Most of the increase in complexity will come
**	from the manipulation of tape volumes.
**
**	A journal or dump file is like any normal file, other than the fact 
**	that each block is tagged with a header that records information used to
**	enhance the reliability of the software.
**
**	Journal or dump blocks are not necessarily full.  They can be partially
**	filled. JFtruncate is used to claim wasted wpace from dormant journal
**	or dump files.
**
**	Journal or dump blocks are never logically rewritten on disks and never
**	physical rewritten on tapes.  Rewriting a tape block is not
**	supported on a lot of tape equipment, as well as being less 
**	reliable when used (Unless an intelligent tape controller is
**	using a mechanism similar to our block sequence writing.)
**
**	The block size of a journal or dump file can range from 4096 to 32768 in
**	powers of 2.
**
**	The maximum number of blocks in a journal or dump file is 2^32-1.
**
**	The maximum number of journal or dump files in 9,999,999.
**
** History: $Log-for RCS$
**      24-sep-1986 (Derek)
**          Created.
**	26-apr-1993 (bryanp)
**	    Added function prototypes for 6.5.
**	26-apr-1993 (bryanp)
**	    Please note that arg_list is a PTR, not a (PTR *).
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add #ifdef JF_INCLUDED to expose dm0j
**	    to other spots in DMF.
[@history_template@]...
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
** Name: JFIO - Journal or dump file context.
**
** Description:
**      This structure contains information initialized and used by the JF
**	routines to manage an open journal or dump file.  The actual structure is
**	allocated by the caller.  This is the structure used on VAX/VMS.
**
** History:
**      24-sep-1986 (Derek)
**          Created
[@history_template@]...
*/
typedef struct _JFIO
{
    i4		jf_ascii_id;	    /*  Visible identifier. */
#define			    JF_ASCII_ID	    'JFIO'
    i4		jf_allocate;	    /*	Number of allocated VMS block. */
    i4		jf_blk_size;	    /*  Block size for transfer. */
    i4		jf_log2_bksz;	    /*  Log 2 of block size. */
    i4		jf_blk_cnt;	    /*  VMS blocks in a JF block. */
    i4		jf_cur_blk;	    /*  Current JF block number. */
    i4		jf_channel;	    /*  VMS I/O channel. */
    i4		jf_extra;
    short		jf_fid[4];	    /*  File identifier. */
};

#endif		/* JF_INCLUDED */
