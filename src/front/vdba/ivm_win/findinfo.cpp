/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : findinfo.cpp, Implementation file.
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Find management
**
** History:
**
** 02-Mar-2000 (uk$so01)
**    created
**    SIR #100635, Add the Functionality of Find operation.
** 24-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "findinfo.h"
#include "environm.h"
#include "wmusrmsg.h"
#include "edlsinpr.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Find the text in the list control "CuEditableListCtrlInstallationParameter"
LONG FIND_ListCtrlParameter (WPARAM wParam, LPARAM lParam)
{
	CaFindInformation* pFindInfo = (CaFindInformation*)lParam;
	ASSERT (pFindInfo);
	if (!pFindInfo)
		return 0L;
	CuEditableListCtrlInstallationParameter* pCtrl = (CuEditableListCtrlInstallationParameter*)wParam;
	ASSERT (pCtrl);
	if (!pCtrl)
		return 0L;

	CString strItem;
	int i, nStart, nCount = pCtrl->GetItemCount();
	CString strWhat = pFindInfo->GetWhat();
	if (nCount == 0)
		return FIND_RESULT_NOTFOUND;
	if (pFindInfo->GetListWindow() != pCtrl || pFindInfo->GetListWindow() == NULL)
	{
		pFindInfo->SetListWindow(pCtrl);
		pFindInfo->SetListPos (0);
	}
	if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
		strWhat.MakeUpper();
	
	nStart = pFindInfo->GetListPos();
	CaEnvironmentParameter* pParameter = NULL;
	for (i=pFindInfo->GetListPos (); i<nCount; i++)
	{
		pParameter = (CaEnvironmentParameter*)pCtrl->GetItemData(i);
		pFindInfo->SetListPos (i);
		if (!pParameter)
			continue;

		int nCol = 0;
		while (nCol < 3)
		{
			switch (nCol)
			{
			case 0:
				strItem = pParameter->GetName();
				break;
			case 1:
				strItem = pParameter->GetValue();
				break;
			case 2:
				strItem = pParameter->GetDescription();
				break;
			default:
				ASSERT (FALSE);
				break;
			}

			if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
				strItem.MakeUpper();

			int nBeginFind = 0, nLen = strItem.GetLength();
			int nFound = -1;

			if (!(pFindInfo->GetFlag() & FR_WHOLEWORD))
			{
				nFound = strItem.Find (strWhat);
				if (nFound != -1)
				{
					pFindInfo->SetListPos (i+1);
					pCtrl->EnsureVisible(i, FALSE);
					pCtrl->SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

					return FIND_RESULT_OK;
				}
			}
			else
			{
				TCHAR* chszSep = (TCHAR*)(LPCTSTR)theApp.m_strWordSeparator; // Seperator of whole word.
				TCHAR* token = NULL;
				// 
				// Establish string and get the first token:
				token = _tcstok ((TCHAR*)(LPCTSTR)strItem, chszSep);
				while (token != NULL )
				{
					//
					// While there are tokens in "strResult"
					if (strWhat.Compare (token) == 0)
					{
						pFindInfo->SetListPos (i+1);
						pCtrl->EnsureVisible(i, FALSE);
						pCtrl->SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

						return FIND_RESULT_OK;
					}

					token = _tcstok(NULL, chszSep);
				}
			}

			//
			// Next column:
			nCol++;
		}
	}

	if (nCount > 0 && nStart > 0)
		return FIND_RESULT_END;
	return FIND_RESULT_NOTFOUND;
}


//
// Find text in the edit control:
LONG FIND_EditText (WPARAM wParam, LPARAM lParam)
{
	CaFindInformation* pFindInfo = (CaFindInformation*)lParam;
	ASSERT (pFindInfo);
	if (!pFindInfo)
		return 0L;

	CEdit* pCtrl = (CEdit*)wParam;
	ASSERT (pCtrl);
	if (!pCtrl)
		return 0L;

	if (pFindInfo->GetEditWindow() != pCtrl || pFindInfo->GetEditWindow() == NULL)
	{
		pFindInfo->SetEditWindow(pCtrl);
		pFindInfo->SetEditPos (0);
	}

	CString strBuffer;
	pCtrl->GetWindowText(strBuffer);
	CString strWhat = pFindInfo->GetWhat();
	int nLength = strBuffer.GetLength();

	if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
	{
		strBuffer.MakeUpper();
		strWhat.MakeUpper();
	}

	int nStartFind = pFindInfo->GetEditPos ();
	int nFound = -1;
	
	if (!(pFindInfo->GetFlag() & FR_WHOLEWORD))
	{
		nFound = strBuffer.Find (strWhat, nStartFind);
		if (nFound != -1)
		{
			pFindInfo->SetEditPos (nFound+strWhat.GetLength());
			pCtrl->SetSel (nFound, nFound+strWhat.GetLength());

			return FIND_RESULT_OK;
		}
	}
	else
	{
		CString strSeparators = theApp.m_strWordSeparator; // Seperator of whole word.
		nFound = strBuffer.Find (strWhat, nStartFind);

		while (nFound != -1)
		{
			if (nFound > 0)
			{
				//
				// Check if there is a separator:
				TCHAR* pPrev =  (TCHAR* )(LPCTSTR)strBuffer +nFound;
				pPrev = _tcsdec((const TCHAR* )strBuffer, pPrev);
				TCHAR tchSep1 = pPrev? *pPrev: _T('\0'); //strBuffer.GetAt (nFound -1);
				TCHAR tchSep2 = strBuffer.GetAt (nFound + strWhat.GetLength());
				if (strSeparators.Find(tchSep1) != -1 && strSeparators.Find(tchSep2) != -1)
				{
					pFindInfo->SetEditPos (nFound+strWhat.GetLength());
					pCtrl->SetSel (nFound, nFound+strWhat.GetLength());

					return FIND_RESULT_OK;
				}
				else
				{
					nStartFind = nFound+strWhat.GetLength();
					nFound = strBuffer.Find (strWhat, nStartFind);
				}
			}
		}
	}

	pFindInfo->SetEditPos (nLength);
	if (nLength > 0 && nStartFind > 0)
		return FIND_RESULT_END;

	return FIND_RESULT_NOTFOUND;
}