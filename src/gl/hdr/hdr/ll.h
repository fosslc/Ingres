/*
**	Copyright (c) 2004 Ingres Corporation
*/

# ifndef _LL_HDR
# define _LL_HDR 1

/* 
** Name:	LL.c	- Linked-List function externs
**
** Specification:
** 	The linked-list module helps in maintaining a list of objects.
**
** Description:
**	Contains LL function externs
**	Note that this is naive code, i.e. it always trusts its caller
**	Many of the simple routines are in macro form - see ll.h
**
**	The LNKDEF structure must be included in the structure definition for
**	the object to be placed on a doubly linked-list. The LNKDEF structure
**	contains forward and backward pointers, as well as, a pointer to the
**	beginning of object itself. The LNKDEF structure can be used to
**	define the head of the linked-list. In this situation the object
**	pointer would be zero. This allows scanning routines to use a null
**	object pointer as the end of list indicator.  Otherwise the scanning
**	routines will have to save off a pointer to the first element and use
**	that pointer to detect when the list has been completely scanned.
**
**	To initialize a linked-list entry:
**	   LL_Init(lnk, object);
** 	To unlink a list entry:
**	   LL_Unlink(lnk);
**  To insert an entry into the linked-list
**	   LL_InsertBefore(lnklist, lnk);
**	   LL_InsertAfter(lnklist, lnk);
**
**	To point to the next element in the list:
**	   LL_GetNext(lnk);
**	To point to the previous element in the list:
**	   LL_GetPrev(lnk);
**
**	To point to the object represented by this element:
**	   LL_GetObject(lnk);
**	To point to the object represented by the next element:
**	   LL_GetNextObject(lnk);
**	To point to the object represented by the previous element:
**	   LL_GetPrevObject(lnk);
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
**	29-May-1998 (shero03)
**	    Add LL_FindBin (Find a binary key within the object)
**	30-Jun-1998 (shero03)
**	    Add LL_FindStr (Find a string key within the object)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

typedef void LL_ApplyFun (void *object, void *user_control_blk);

struct _LNKDEF
{
	struct _LNKDEF	*lnknxt; 	/* ptr to next list item */
	struct _LNKDEF	*lnkprv; 	/* ptr to previous list itme */
	void	*lnkobj;		/* ptr to list item's object */
};

typedef struct _LNKDEF LNKDEF;

# define LL_GetNext(lnk)	((lnk)->lnknxt)
# define LL_GetNextObject(lnk)	((lnk)->lnknxt->lnkobj)
# define LL_GetPrev(lnk)	((lnk)->lnkprv)
# define LL_GetPrevObject(lnk)	((lnk)->lnkprv->lnkobj)
# define LL_GetObject(lnk)	((lnk)->lnkobj)
# define LL_Last(lnk)		(((lnk)->lnknxt == lnk)? TRUE: FALSE)

# define LL_Init(lnk, obj) \
{ \
	((LNKDEF *)(lnk))->lnknxt = ((LNKDEF*)(lnk))->lnkprv = (LNKDEF*)(lnk); \
	((LNKDEF *)(lnk))->lnkobj = (void*)(obj); \
}

# define LL_Unlink(lnk) \
{ \
	LNKDEF	*nextlnk; \
	LNKDEF	*prevlnk; \
	nextlnk = ((LNKDEF*)(lnk))->lnknxt; \
	prevlnk = ((LNKDEF*)(lnk))->lnkprv; \
	prevlnk->lnknxt = nextlnk; \
	nextlnk->lnkprv = prevlnk; \
	((LNKDEF*)(lnk))->lnknxt = ((LNKDEF*)(lnk))->lnkprv = ((LNKDEF*)(lnk)); \
}

# define LL_LinkAfter(listlnk, lnk) \
{ \
	((LNKDEF*)(lnk))->lnknxt = ((LNKDEF*)(listlnk))->lnknxt; \
	((LNKDEF*)(lnk))->lnkprv = ((LNKDEF*)(listlnk)); \
	((LNKDEF*)(listlnk))->lnknxt = ((LNKDEF*)(lnk)); \
	((LNKDEF*)(lnk))->lnknxt->lnkprv = ((LNKDEF*)(lnk)); \
}

# define LL_LinkBefore(listlnk, lnk) \
{ \
	((LNKDEF*)(lnk))->lnknxt = ((LNKDEF*)(listlnk)); \
	((LNKDEF*)(lnk))->lnkprv = ((LNKDEF*)(listlnk))->lnkprv; \
	((LNKDEF*)(listlnk))->lnkprv = ((LNKDEF*)(lnk)); \
	((LNKDEF*)(lnk))->lnkprv->lnknxt = ((LNKDEF*)(lnk)); \
}

void LL_Apply(LNKDEF *lnk, LL_ApplyFun apply_fun, void *user_control_blk);
i4  LL_Count(LNKDEF *lnk);
LNKDEF *LL_GetNth(LNKDEF *lnk, i4  n);
void *LL_GetNthObject(LNKDEF *lnk, i4  n);
LNKDEF *LL_Find(LNKDEF *lnk, void *key);
LNKDEF *LL_FindBin(LNKDEF *lnk, i4  keyOff, i4  keylen, void *key);
LNKDEF *LL_FindStr(LNKDEF *lnk, i4  keyOff, void *key);

# endif /* ! _LL_HDR */
