/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <me.h>
# include <st.h>
# include <ll.h>

/* 
** Name:	LL.c	- Linked-List function
**
** Specification:
** 	The linked-list module helps in maintaining a list of objects.
**
** Description:
**	Contains LL function 
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
**	To insert an entry into the linked-list
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
**	    Add LL_FindStr (Find a string key withing the object)
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static void ll_CountFun (void* obj, void *count_p);


/*{
** Name:	LL_Apply	- Apply a function on each object in the list
**
** Description:
**	Apply a function to every object represented by a list item
**
** Inputs:
**	lnk		The item at which the function is to be applied
**	apply_fun	The function that is applied to each object
**	control_block	An optional control block pointer
**
** Outputs:
**
** Returns:
**	OK		if succeeded
**	other		reportable error status.
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/

void	    LL_Apply(lnk, apply_fun, user_control_blk)
LNKDEF*     lnk;
LL_ApplyFun apply_fun;
void*	    user_control_blk;
{
  LNKDEF*    anchor;
  void*      obj;

  anchor = lnk;
  do
  {
    LNKDEF* lnknext = LL_GetNext(lnk);
    if (obj = LL_GetObject(lnk))
      apply_fun(obj, user_control_blk);
    lnk = lnknext;
  }
  while (lnk != anchor);
  return;
}

/*{
** Name:	LL_Find	- Look for a particular object in the list
**
** Description:
**	Apply a compare function to every object represented by a list item
**	Return if the compare returns TRUE.  If the compare routine returns
**	FALSE, keep progressing in the list
**
** Inputs:
**	lnk		The item at which the function is to be applied
**	key		A pointer to the key to search for
**
** Outputs:
**
** Returns:
**	OK		if succeeded
**	other		reportable error status.
**
** History:
**	27-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/
LNKDEF *LL_Find(lnk, key)
LNKDEF*     lnk;
void	    *key;
{
  LNKDEF*    anchor;
  void*      obj;

  anchor = lnk;
  while (TRUE)
  {
    if (lnk == NULL)
    	break;
    if (obj = LL_GetObject(lnk))
    {	
      if (obj == key)
    	return lnk;
    }
    lnk = LL_GetNext(lnk);
    if (lnk == anchor)
    	break;
  }
  return NULL;
}

/*{
** Name:	LL_FindBin 	- Apply a binary compare function on each object in the list
**
** Description:
**	Apply a binary compare function to every object represented by a list item
**	Return if the compare returns TRUE.  If the compare routine returns
**	FALSE, keep progressing in the list
**
** Inputs:
**	lnk			The item at which the function is to be applied
**	keyoffset	The offset of the key within the object
**	keylen		The length of the key within the object
**	key			A pointer to the key to search for
**
** Outputs:
**
** Returns:
**	OK		if succeeded
**	other		reportable error status.
**
** History:
**	29-May-1998 (shero03)
**	    Created
*/
LNKDEF *LL_FindBin(lnk, keyoff, keylen, key)
LNKDEF*     lnk;
i4			keyoff;
i4			keylen;
void	    *key;
{
  LNKDEF    *anchor;
  char	 	*obj;

  anchor = lnk;
  while (TRUE)
  {
    if (lnk == NULL)
    	break;
    if (obj = (char*)LL_GetObject(lnk))
    {
	obj += keyoff;
        if (MEcmp(obj, key, keylen) == 0)
	   return lnk;
    }

    lnk = LL_GetNext(lnk);
    if (lnk == anchor)
    	break;
  }
  return NULL;
}

/*{
** Name:	LL_FindStr - Apply a string compare function on each object in the list
**
** Description:
**	Apply a string compare function to every object represented by a list item
**	Return if the compare returns TRUE.  If the compare routine returns
**	FALSE, keep progressing in the list
**
** Inputs:
**	lnk		The item at which the function is to be applied
**	keyoffset	The offset of the key within the object
**	key		A pointer to the key to search for
**
** Outputs:
**
** Returns:
**	OK		if succeeded
**	other		reportable error status.
**
** History:
**	30-Jun-1998 (shero03)
**	    Created
*/
LNKDEF *LL_FindStr(lnk, keyoff, key)
LNKDEF*     lnk;
i4	    keyoff;
void	    *key;
{
  LNKDEF    *anchor;
  char	    *obj;

  anchor = lnk;
  while (TRUE)
  {
    if (lnk == NULL)
    	break;
    if (obj = (char*)LL_GetObject(lnk))
    {
	obj += keyoff;
        if (STcompare(obj, key) == 0)
	   return lnk;
    }

    lnk = LL_GetNext(lnk);
    if (lnk == anchor)
    	break;
  }
  return NULL;
}

/*{
** Name:	ll_CountFun	- Count function
**
** Description:
**	Increment the counter.
**
** Inputs:
**	lnk		The item at which the function is to be applied
**	count_p		Pointer to the count of elements
**
** Outputs:
**
** Returns:
**	OK		if succeeded
**	other		reportable error status.
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/
static void ll_CountFun(obj, count_p)
void*	    obj;
void*	    count_p;
{
  (*(i4 *)count_p)++;
  return;
}


/*{
** Name:	LL_Count	- Count function
**
** Description:
**	Count the number of objects represented by this list.
**
** Inputs:
**	lnk		The item at which the function is to be applied
**
** Outputs:
**
** Returns:
**	count		The number of object in the list
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/
i4  LL_Count(lnk)
LNKDEF*     lnk;
{
  i4	    count = 0;
  LL_Apply(lnk, (LL_ApplyFun*)ll_CountFun, (void *)&count);
  return count;
}


/*{
** Name:	LL_GetNth	- Get the Nth element
**
** Description:
**	Return the address of the Nth element in the list
**
** Inputs:
**	lnk		The head of the list
**
** Outputs:
**
** Returns:
**	lnk		The Nth element
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/
LNKDEF*     LL_GetNth(lnk, n)
LNKDEF*     lnk;
i4	    n;				/* n = 1 means the next item	    */
{
  i4	    i;

  for (i = 0; i < n; i++)
  {
    lnk = LL_GetNext(lnk);
    if (!LL_GetObject(lnk))		/* no object here?		    */
      lnk = LL_GetNext(lnk);		/* get the next one		    */
  }
  return lnk;
}


/*{
** Name:	LL_GetNthObject	- Get the addr of the Nth object in the list 
**
** Description:
**	Return the address of the Nth object in the list
**
** Inputs:
**	lnk		The head of the list
**
** Outputs:
**
** Returns:
**	void*		The Nth object
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/
void*	    LL_GetNthObject(lnk, n)
LNKDEF*     lnk;
i4	    n;				/* n = 1 means the next item	    */
{
  return LL_GetObject(LL_GetNth(lnk, n));
}

