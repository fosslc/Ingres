/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef _ILL_H
# define _ILL_H

/*
** Name:    ill.h                   - Definitions for generic linked lists.
**
** Description:
**  Contains structures and prototypes used throughout ICE for linked list
**  management.
**
** History:
**  29-oct-96 (harpa06)
**      Created.
**  09-may-97 (harpa06)
**      Added ILLInsertElement() prototype 
**  06-May-98 (fanra01)
**      Add macro for returning pointer to structure containing a record.
**      Modify queueing structures to include semaphore protection.
**  14-May-98 (fanra01)
**      Add parameter to allow specifying where find should start.
**      Add parameter to allow specifying where to insert an entry.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-sep-2000 (mcgem01)
**	    replace nat and longnat with i4
*/

#define WITH_RECORD(address, type, field) ((type *)( \
                                           (char*)(address) - \
                                           (char*)(&((type *)0)->field)))

/* Structures */
typedef struct tagILL
{
    struct tagILL* pNext;
    struct tagILL* pPrev;
}ILL, *PILL;

typedef struct tagILLHDR
{
    i4              nMutexInit;
    CS_THREAD_ID*   pThreadOwner;
    i4*            pnSemCount;
    MU_SEMAPHORE*   pIllSem;
    PILL            pIllQueue;
}ILLHDR, *PILLHDR;

/* Prototypes */
ICERES ILLFindElement (void *list, void* start, i4  nQueue,
                       int (*cmp)(void *, i4, void *), void *key,
                       void **retVal);
void ILLAddElement (void *list, void *newelement);
void ILLAppendElement (void *list, void *element);
void ILLDropElement (void *list, void *element);
void ILLInsertElement (void *list, void *element, void* newelement);

# endif
