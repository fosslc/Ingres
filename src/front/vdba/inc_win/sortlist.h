/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sortlist.h: interface for sorting
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Sort list and array of CObject derived classes
**
** History:
**
** 15-Nov-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    Created
**/

#if !defined (SORTLIST_HEADER)
#define SORTLIST_HEADER


typedef int (CALLBACK* PFNEVENTCOMPARE)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
//
// nMin, nMax: zero base index range of elements in the array:
int  SORT_DichoInsertSort (CObArray& T, int nMin, int nMax, CObject* e, LPARAM sr, PFNEVENTCOMPARE compare);
void SORT_DichotomySort(CObArray& T, PFNEVENTCOMPARE compare, LPARAM lParamSort, CProgressCtrl* pCtrl);
int  SORT_GetDichoRange(CObArray& T, int nMin, int nMax, CObject* e, LPARAM sr, PFNEVENTCOMPARE compare);


#endif // SORTLIST_HEADER