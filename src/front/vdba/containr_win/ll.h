/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * LL.H
 *
 * Contains definitions and prototypes for the linked list manager.
 ****************************************************************************/

typedef struct tagLINKNODE
   {
   struct tagLINKNODE FAR *lpNext;
   struct tagLINKNODE FAR *lpPrev;
   } LINKNODE;

typedef LINKNODE     *  PLINKNODE ;
typedef LINKNODE FAR * LPLINKNODE ;

#define NEXT(p) (((LPLINKNODE)(p))->lpNext)
#define PREV(p) (((LPLINKNODE)(p))->lpPrev)

LPLINKNODE LLFindHead(LPLINKNODE lpNode);
LPLINKNODE LLFindTail(LPLINKNODE lpNode);
LPLINKNODE LLAddHead(LPLINKNODE lpPos, LPLINKNODE lpNew);
LPLINKNODE LLAddTail(LPLINKNODE lpPos, LPLINKNODE lpNew);
LPLINKNODE LLRemoveHead(LPLINKNODE lpPos, LPLINKNODE FAR *lpOldHead);
LPLINKNODE LLRemoveTail(LPLINKNODE lpPos, LPLINKNODE FAR *lpOldTail);
LPLINKNODE LLRemoveNode(LPLINKNODE lpNode);
LPLINKNODE LLInsertBefore(LPLINKNODE lpPos, LPLINKNODE lpNew);
LPLINKNODE LLInsertAfter(LPLINKNODE lpPos, LPLINKNODE lpNew);
LPLINKNODE LLScrollList(LPLINKNODE lpPos, LONG lDelta);
