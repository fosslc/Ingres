/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM0M.H - This defines the memory manager interface.
**
** Description:
**      The file defines the calls to the DMF memory manager as
**	well as the parameter constants needed.
**
** History:
**      22-oct-1985 (derek)
**          Created new for Jupiter.
**	25-feb-1991 (rogerk)
**	    Added func_extern for dm0m_info routine.
**	07-jul-92 (jrb)
**	    Prototyping DMF.
**      07-oct-93 (johnst)
**	    Bug #56442
**          Changed type of dm0m_search() argument "arg" from (i4) to
**          (i4 *) to match the required usage by its callers from dmdcb.c.
**          The dm0m_search() user-supplied search function args must now be
**          passed by reference to correctly support 64-bit pointer arguments.
**          On DEC alpha, for example, ptrs are 64-bit and ints are 32-bit, so
**          ptr/int overloading will no longer work!
**	06-mar-1996 (stial01 for bryanp)
**	    Add prototypes for dm0m_tballoc, dm0m_tbdealloc, dm0m_pgalloc,
**		dm0m_pgdealloc
**	    Add prototypes for dm0m_lcopy, dm0m_lfill.
**      10-mar-1997 (stial01)
**          Changed prototype for dm0m_tballoc()
**	02-dec-1998 (nanpr01)
**	    Added the DM0M_LOCKED flag for cache locking(mlock).
**	15-dec-1999 (nanpr01)
**	    Pass pointer to a pointer for dm0m_deallocate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Oct-2002 (hanch04)
**	    removed prototypes for dm0m_lcopy, dm0m_lfill.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      18-feb-2004 (stial01)
**          Change prototype for dm0m_tballoc.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
*/


/*
**  Defines of other constants.
*/

/*
** 	All the defines for dm0m_allocate have been moved
**	to dm.h
*/


/*
**      Constants used for flag argument of dm0m_verify().
*/
#define                 DM0M_READONLY	    0x01L
#define			DM0M_WRITEABLE	    0x02L
#define			DM0M_POOL_CHECK	    0x04L

/*
**  Constants used for type calls to dm0m_search().
*/

#define			DM0M_ALL	((i4)0)


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS dm0m_allocate(		/* Allocate memory. */
			SIZE_TYPE	size,
			i4		flag,
			i4		TypeWithClass,
			i4		tag,
			char		*owner,
			DM_OBJECT	**block,
			DB_ERROR	*dberr);

FUNC_EXTERN VOID dm0m_deallocate(	        /* Deallocate memory. */
			DM_OBJECT	**object);

FUNC_EXTERN DB_STATUS dm0m_destroy(		/* Destroy memory manager DB */
			DML_SCB		*scb,
			DB_ERROR	*dberr);

FUNC_EXTERN DB_STATUS dm0m_search(		/* Search for control blocks. */
			DML_SCB		*scb,
			i4		type,
			VOID		(*function)(),
			i4		*arg,
			DB_ERROR	*dberr);

FUNC_EXTERN VOID dm0m_verify(	        	/* Verify pool. */
			i4		flag);
			
FUNC_EXTERN VOID      dm0m_info(		/* Describe allocated memory. */
			SIZE_TYPE	*total_memory,
			SIZE_TYPE	*allocated_memory,
			SIZE_TYPE	*free_memory);

FUNC_EXTERN char	*dm0m_tballoc(SIZE_TYPE tuple_size);

FUNC_EXTERN VOID	dm0m_tbdealloc(char **tuplebuf);

FUNC_EXTERN char	*dm0m_pgalloc(i4 page_size);

FUNC_EXTERN VOID	dm0m_pgdealloc(char **pagebuf);

FUNC_EXTERN VOID	dm0m_stats_by_type(i4 turning_on);

FUNC_EXTERN VOID	dm0m_init(void);

FUNC_EXTERN VOID	dm0m_init_scb(DML_SCB *scb);
