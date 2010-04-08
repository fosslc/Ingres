/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sortlist.cpp: implement the interface for sorting
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


#include "stdafx.h"
#include "sortlist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void SORT_DichotomySort(CObArray& T, PFNEVENTCOMPARE compare, LPARAM lParamSort, CProgressCtrl* pCtrl)
{
	int nCount = (int)T.GetSize();
	if (T.GetSize() < 2)
		return;
	BOOL bStep = (pCtrl && IsWindow(pCtrl->m_hWnd))? TRUE: FALSE;
	CObject* e = NULL;
	int nStart = 1;

	if (bStep)
	{
		pCtrl->SetRange(0, nCount);
		pCtrl->SetStep(1);
	}

	while (nStart < nCount)
	{
		e = T[nStart];
		T.RemoveAt(nStart);

		SORT_DichoInsertSort (T, 0, nStart-1, e, lParamSort, compare);
		nStart++;
		if (bStep)
			pCtrl->StepIt();
	}
}

int SORT_DichoInsertSort (CObArray& T, int nMin, int nMax, CObject* e, LPARAM sr, PFNEVENTCOMPARE compare)
{
	int nPos = SORT_GetDichoRange (T, nMin, nMax, e, sr, compare);
	int i = nMax;
	//
	// Use the advantage of programing langage to gain the performance, eg
	// to shift the element to the right, we use InsertAt instead of moving
	// element one by one:
	T.InsertAt (nPos, e);
	return nPos;
}

int SORT_GetDichoRange (CObArray& T, int nMin, int nMax, CObject* e, LPARAM sr, PFNEVENTCOMPARE compare)
{
	int nCmp, m;
	int left = nMin;
	int right= nMax;
	while (left <= right)
	{
		m = (left + right)/2;
		nCmp = compare ((LPARAM)e, (LPARAM)T[m], (LPARAM)sr);

		if (nCmp < 0)
		{
			right = m-1;
		}
		else
		{
			left = m+1;
		}
	}
	return left;
}


