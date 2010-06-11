/********************************************************************
**
**	Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**	  Project  : Ingres Visual DBA
**
**	  Source   : util.c
**
**	  TOOLS
**
**    History:
**    
**    09-Nov-99 (noifr01)
**      (bug #99447) removed incorrect change that was done in the AddItemtoList 
**      function for a partial fix of bug #97680 (support of object names with
**      special characters)
**    23-Dec-99 (schph01)
**      (bug #97680) (management of object names including special characters
**      was not complete in VDBA)
**      additional change : manage the string "(public)" in
**      AddItemToListQuoted() function.
**    20-Jan-2000  (uk$so01)
**      (bug #100063)   Eliminate the undesired compiler's warning
**    24-May 2000 (noifr01)
**     (bug 99242)  cleanup for DBCS compliance
**    09-Oct-2000 (schph01)
**      bug #102868 change the size of the buffer used in the
**      function NoLeadingEndingSpace()
**    21-Mar-2001 (noifr01)
**     (bug 99242) (DBCS) replaced usage of ambiguous function (in the
**     DBCS context) lstrcpyn with fstrncpy
**    26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**    05-Jun-2002 (noifr01)
**     (bug 107773) now have the VerifyObjectName() function always return
**     TRUE, since we want to let the backend return appropriate errors,
**     and this function didn't seem to work properly on certain platforms.
**     Also removed no more used VerifyDBName() function
**    28-Mar-2003 (schph01)
**     SIR 107523 Add CAListBoxFillSequences ()
**    06-May-2010 (drivi01)
**     Fix CAListBoxFillTables and ComboBoxFillTables
**     functions to strip out IVW tables.
**     CAListBoxFillTables used by usermod and auditDB to populate
**     list of tables in the dialog and ComboBoxFillTables is used
**     by VerifyDB to populate a list of tables.
**     The commands mentioned aren't supported by IVW.
**     Add ComboBoxFillTablesFiltered which will load combo box
**     of tables with either Ingres or VW tables depending
**     on which are requested.
**    02-Jun-2010 (drivi01)
**     Remove hard coded buffer sizes.
********************************************************************/

#include "dll.h"
#include "subedit.h"
#include "dba.h"
#include "domdata.h"
#include <ctype.h>
#include "stdlib.h"
#include "stdio.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "catospin.h"
#include "getdata.h"
#include "msghandl.h"
#include "error.h"
#include "prodname.h"



/* surli01 21-March-1996 Add DBCS handling code */
#ifdef DOUBLEBYTE
#define ISDBCSLEAD(c) IsDBCSLeadByte((c))
#else
#define ISDBCSLEAD(c) (FALSE)
#endif


BOOL OccupyComboBoxControl (HWND hwndCtl, LPCTLDATA lpdata)
/*
	Function:
		Fills the combobox drop down box with the given control data.

	Parameters:
		hwndCtl - Handle to the control window.
		lpdata	- Pointer to the data structure

	Returns:
		TRUE if successful.
*/
{
	int i = 0;
	BOOL bRetVal = TRUE;

	/* // Verify the function parameters */

	if (!IsWindow(hwndCtl) || !lpdata)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	/* // Loop through the data structure adding the strings to the combobox */

	while (lpdata[i].nId != -1)
	{
		char szBuffer[MAXOBJECTNAME*4];
		int nIdx;

		if (LoadString(hResource, lpdata[i].nStringId, szBuffer, sizeof(szBuffer)) == 0)
			bRetVal = FALSE;

		nIdx = ComboBox_AddString(hwndCtl, szBuffer);

		if (nIdx == CB_ERR)
			bRetVal = FALSE;
		else
			ComboBox_SetItemData(hwndCtl, nIdx, (LPARAM)lpdata[i].nId);

		i++;
	}

	/* // Set the first string in the list as the default */
	if (bRetVal)
		ComboBox_SetCurSel(hwndCtl, 0);

	return bRetVal;
}


BOOL OccupyCAListBoxControl (HWND hwndCtl, LPCTLDATA lpdata)
/*
	Function:
		Fills the CA list box with the given data.

	Parameters:
		hwndCtl - Handle to the control window.
		lpdata	- Pointer to the data structure

	Returns:
		TRUE if successful.
*/
{
	int i = 0;
	BOOL bRetVal = TRUE;

	/* // Verify the functions parameters */

	if (!IsWindow(hwndCtl) || !lpdata)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	/* // Loop through the data structure adding the strings to the CA listbox */

	while (lpdata[i].nId != -1)
	{
		char szBuffer[256];
		int nIdx;

		if (LoadString(hResource, lpdata[i].nStringId, szBuffer, sizeof(szBuffer)) == 0)
			bRetVal = FALSE;

		nIdx = CAListBox_AddString(hwndCtl, szBuffer);

		if (nIdx == CB_ERR)
			bRetVal = FALSE;
		else
			CAListBox_SetItemData(hwndCtl, nIdx, (LPARAM)lpdata[i].nId);

		i++;
	}

	return bRetVal;
}

BOOL SelectComboBoxItem(HWND hwndCtl, LPCTLDATA lpdata, int nId)
/*
	Function:
		Given the ID of a combobox item, this function searches the
		combobox for the item with that ID and then selects it.

	Parameters:
		hwndCtl - Handle to the combobox
		lpdata	- Pointer to the data structure for that combobox
		nId 	- ID of the item to find and select

	Returns:
		TRUE if item successfully selected.
*/
{
	int nIndex = 0;
	char szBuffer[256];
	BOOL bRetVal = FALSE;

	if (!IsWindow(hwndCtl) || !lpdata)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	while (lpdata[nIndex].nId != -1)
	{
		if (lpdata[nIndex].nId == nId)
		{
			if (LoadString(hResource, lpdata[nIndex].nStringId, szBuffer, sizeof(szBuffer)) != 0)
			{
				ComboBox_SelectString(hwndCtl, -1, szBuffer);
				bRetVal = TRUE;
			}

			break;
		}
		nIndex++;
	}

	return bRetVal;
}


BOOL VerifyObjectName (HWND hwnd,
					   LPSTR lpszObjectName,
					   BOOL bShowError,
					   BOOL bStartII,
					   BOOL bStartUnderscore)
{
	return TRUE; /* don't try any more to redo (possibly differently)
				    tests which will be done by the backend when the
					SQL query will be executed. This function was
                    anyhow not working on certain platforms */
}


BOOL VerifyVNodeName(HWND hwnd, LPSTR lpszVNode, BOOL bShowError)
{
	if (!IsWindow(hwnd) || !lpszVNode)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}


BOOL VerifyDeviceName(HWND hwnd, LPSTR lpszDevice, BOOL bShowError)
{
	if (!IsWindow(hwnd) || !lpszDevice)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}


BOOL VerifyUserName(HWND hwnd, LPSTR lpszUser, BOOL bShowError)
/*
	Function:
		TBD **** WHAT IS A VALID USER NAME ?????

	Parameters:
		hwnd			- Handle of the dialog control
		lpszUser		- Points to user name string
		bShowError	- TRUE if show error messages.

	Returns:
		TRUE if user name is valid
*/
{
	if (!IsWindow(hwnd) || !lpszUser)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}



BOOL VerifyGroupName(HWND hwnd, LPSTR lpszGroup, BOOL bShowError)
/*
	Function:
		TBD **** WHAT IS A VALID GROUP NAME ?????

	Parameters:
		hwnd			- Handle of the dialog control
		lpszGroup	- Points to group string to verify
		bShowError	- TRUE if show error messages.

	Returns:
		TRUE if group name is valid
*/
{
	if (!IsWindow(hwnd) || !lpszGroup)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}


BOOL VerifyListenAddress(HWND hwnd, LPSTR lpszListen, BOOL bShowError)
/*
	Function:
		TBD **** WHAT IS A VALID LISTEN ADDRESS ?????

	Parameters:
		hwnd			- Handle of the dialog control
		lpszListen	- Points to listen address string
		bShowError	- TRUE if show error messages.

	Returns:
		TRUE if listen address is valid
*/
{
	if (!IsWindow(hwnd) || !lpszListen)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}



void EditIncrement(HWND hwndEdit, int nIncrement)
/*
	Function:
		Increments the contents of a numeric edit control.

	Parameters:
		hwndEdit		- Handle of the edit control to increment
		nIncrement	- Amount to increment control by.
						  Use negative values to decrement edit control.

	Returns:
		None.

	Note:
		This function assumes that some method has been used to ensure
		that the edit control only contains numeric characters.
*/
{
	char szNumber[20];
	DWORD dwNumber;
	EDITMINMAX limit;

	if (!IsEditControl(GetParent(hwndEdit), hwndEdit))
	{
		ASSERT(FALSE);
		return;
	}

	ZEROINIT(limit);
	Edit_GetMinMax(hwndEdit, &limit);
	ASSERT(limit.dwMin < limit.dwMax);

	Edit_GetText(hwndEdit, szNumber, sizeof (szNumber));

	dwNumber = my_atodw(szNumber);

	/* // if the edit control is outside the valid range before the spin */
	/* // increment is taken into account, then reset the edit control */
	/* // to the min. or max. value */

	if (dwNumber > limit.dwMax)
		dwNumber = limit.dwMax;
	else
	{
		if (dwNumber < limit.dwMin)
			dwNumber = limit.dwMin;
		else
			dwNumber += nIncrement;
	}

	if (dwNumber <= limit.dwMax && dwNumber >= limit.dwMin)
	{
		my_dwtoa(dwNumber, szNumber, 10);
		Edit_SetText(hwndEdit, szNumber);
	}
	else
		MessageBeep(MB_ICONEXCLAMATION);
}



BOOL VerifyNumericEditControl (HWND hwnd, HWND hwndCtl, BOOL bShowError, BOOL bResetLimit, DWORD dwMin, DWORD dwMax)
/*
	Function:
		Verifies that the value contained in an edit control is between
		the given expected values.	If not then the control is selected
		and an error message is displayed.

	Parameters:
		hwnd			- Handle of edit control parent
		hwndCtl 	- Handle of edit control
		bShowError	- Set to FALSE to inhibit any display changes and not show
						- the error message.
		bResetLimit - Set to TRUE if the edit control should be reset to the
						- limit it has exceeded.
		dwMin			- The minimum value the control should contain
		dwMax			- The maximum value the control should contain

	Returns:
		TRUE if the edit control is within the given range.
*/
{
	char szText[100];
	DWORD dwValue;
	BOOL bRetVal = TRUE;

	/* // Verify the function parameters */

	if (!IsWindow(hwnd) || !IsWindow(hwndCtl) || (dwMin > dwMax))
	{
		ASSERT(FALSE);
		return FALSE;
	}

   if (!IsWindowEnabled(hwndCtl))
	  return TRUE;

	/* // Get the value from the edit control */

	Edit_GetText (hwndCtl, szText, sizeof (szText));
	dwValue = my_atodw(szText);

	/* // Verify the edit control value and handle any error */

	if (dwValue > dwMax || dwValue < dwMin)
	{
		if (bShowError)
	   {
		   HWND currentFocus = GetFocus();
			ErrorBox2 (NULL, IDS_ERR_OUTSIDE_RANGE, MB_ICONEXCLAMATION | MB_OK| MB_TASKMODAL, dwMin, dwMax);
		   SetFocus (currentFocus);
	   }

		if (bResetLimit)
		{
			DWORD dwVal;

			if (dwValue > dwMax)
				dwVal = dwMax;
			else
				dwVal = dwMin;

			/* // Set the edit control to the limit that we passed. */

			my_dwtoa(dwVal, szText, 10);
			Edit_SetText(hwndCtl, szText);

			SetFocus(hwndCtl);
			Edit_SetSel(hwndCtl, 0, -1);
		}
		bRetVal = FALSE;
	}

	return bRetVal;
}



BOOL GetEditCtrlMinMaxValue (HWND hwnd, HWND hwndCtl, LPEDITMINMAX lpLimits, DWORD FAR * lpdwMin, DWORD FAR * lpdwMax)
/*
	Function:
		Fills out the min/max arguments for an edit control

	Parameters:
		hwnd		- Handle to the controls parent
		hwndCtl - Handle to the control
		lpDesc	- Pointer to the controls data structure

	Returns:
		TRUE if edit control min/max found.
*/
{
	int id;

	if (!IsWindow(hwnd) || !IsWindow(hwndCtl))
	{
		ASSERT(FALSE);
		return FALSE;
	}

	id = GetDlgCtrlID(hwndCtl);

	while (lpLimits->nEditId != -1)
	{
		if (lpLimits->nEditId == id)
		{
			*lpdwMin = lpLimits->dwMin;
			*lpdwMax = lpLimits->dwMax;
			return TRUE;
		}
		lpLimits++;
	}

	return FALSE;
}


void my_itoa (int value, char FAR * lpszBuffer, int radix)
{
	static char szBuf[20];

	if (!lpszBuffer)
	{
		ASSERT(NULL);
		return;
	}

	_itoa(value, szBuf, radix);
	lstrcpy(lpszBuffer, szBuf);
}


int my_atoi (char FAR * lpszBuffer)
{
	static char szBuf[20];

	if (!lpszBuffer)
	{
		ASSERT(NULL);
		return 0;
	}

	fstrncpy(szBuf, lpszBuffer, min(sizeof(szBuf), lstrlen(lpszBuffer) + 1));
	return atoi(szBuf);
}


DWORD my_atodw (char FAR * lpszBuffer)
{
	static char szBuf[20];
	double fTemp;

	if (!lpszBuffer)
	{
		ASSERT(NULL);
		return 0;
	}

	fstrncpy(szBuf, lpszBuffer, min(sizeof(szBuf), lstrlen(lpszBuffer) + 1));
	fTemp = atof(szBuf);
	return (DWORD)fTemp;
}


void my_dwtoa (DWORD value, char FAR * lpszBuffer, int radix)
{
	static char szBuf[20];

	if (!lpszBuffer)
	{
		ASSERT(NULL);
		return;
	}

	_ultoa(value, szBuf, radix);
	lstrcpy(lpszBuffer, szBuf);
}


void my_ftoa(double dValue, char FAR * lpszBuffer, size_t nBufSize, UINT uCount)
{
#ifdef MAINWIN
  /* _snprintf does not work under unix port */
	char szFormat[25];
	wsprintf(szFormat,"%%.%df",uCount);
	wsprintf(lpszBuffer, szFormat, dValue);
#else
	_snprintf(lpszBuffer, nBufSize, "%.*f", uCount, dValue);
#endif
}


void ErrorBox (HWND hwnd, int id, UINT fuStyle)
{
	char szTitle[BUFSIZE];
	char szText[512];

//	LoadString (hResource, IDS_ERR_TITLE, szTitle, sizeof (szTitle));
	VDBA_GetErrorCaption(szTitle, sizeof (szTitle));
	LoadString (hResource, id, szText, sizeof (szText));

	MessageBox (hwnd, szText, szTitle, fuStyle);
}


void ErrorBox1 (HWND hwnd, int id, UINT fuStyle, DWORD dw1)
{
	char szTitle[BUFSIZE];
	char szText[512];
	char szFormat[512];

//	LoadString (hResource, IDS_ERR_TITLE, szTitle, sizeof (szTitle));
	VDBA_GetErrorCaption(szTitle, sizeof (szTitle));
	LoadString (hResource, id, szFormat, sizeof (szFormat));

	wsprintf (szText, szFormat, dw1);

	MessageBox (hwnd, szText, szTitle, fuStyle);
}


void ErrorBox2 (HWND hwnd, int id, UINT fuStyle, DWORD dw1, DWORD dw2)
{
	char szTitle[BUFSIZE];
	char szText[512];
	char szFormat[512];

//	LoadString (hResource, IDS_ERR_TITLE, szTitle, sizeof (szTitle));
	VDBA_GetErrorCaption(szTitle, sizeof (szTitle));
	LoadString (hResource, id, szFormat, sizeof (szFormat));

	wsprintf (szText, szFormat, dw1, dw2);

	MessageBox (hwnd, szText, szTitle, fuStyle);
}

void ErrorBoxString(HWND hwnd, int id, UINT fuStyle, LPSTR lpszArg)
{
	char szTitle[BUFSIZE];
	char szText[512];
	char szFormat[512];

//	LoadString (hResource, IDS_ERR_TITLE, szTitle, sizeof (szTitle));
	VDBA_GetErrorCaption(szTitle, sizeof (szTitle));
	LoadString (hResource, id, szFormat, sizeof (szFormat));

	wsprintf (szText, szFormat, lpszArg);

	MessageBox (hwnd, szText, szTitle, fuStyle);
}


BOOL IsEditControl(HWND hwnd, HWND hwndTest)
{
	char szClass[50];

	GetClassName(hwndTest, szClass, sizeof(szClass));
	if (lstrcmpi(szClass, "edit") == 0
	 && IsChild(hwnd, hwndTest))
		return TRUE;
	return FALSE;
}


BOOL HandleUserMessages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case LM_ERROR:
		{
			int nResId = (int)wParam;
			UINT uFlags = (UINT)lParam;

			ErrorBox(hwnd, nResId, uFlags);
			break;
		}

		default:
			return FALSE;
	}

	return TRUE;
}


BOOL CALLBACK __export EnumNumericEditCntls (HWND hwnd, LPARAM lParam)
{
	LPENUMEDIT lpedit = (LPENUMEDIT)lParam;
	HWND hdlg = GetParent(hwnd);

	/* // If we can find the min/max then we know we have a numeric */
	/* // edit control */

	if (IsEditControl(hdlg, hwnd))
	{
		EDITMINMAX limit;

		/* // Found an edit control. */

		ZEROINIT(limit);
		Edit_GetMinMax(hwnd, &limit);
		if (limit.dwMin < limit.dwMax)
		{
			int nCtlId = GetDlgCtrlID(hwnd);

			switch (lpedit->editAction)
			{
				case EC_SUBCLASS:
				{
					SubClassEditCntl (hdlg, nCtlId, CSC_NUMERIC | CSC_KEYUP);
					break;
				}

				case EC_RESETSUBCLASS:
				{
					ResetSubClassEditCntl (hdlg, nCtlId);
					break;
				}

				case EC_LIMITTEXT:
				{
					char szNum[21];

					my_dwtoa(limit.dwMax, szNum, 10);
					Edit_LimitText(hwnd, lstrlen(szNum));
					break;
				}

				case EC_VERIFY:
				{
					BOOL bShowError = lpedit->action.verify.bShowError;
					BOOL bResetLimit = lpedit->action.verify.bResetLimit;

#if 0
					if (!VerifyNumericEditControl (hdlg, hwnd, bShowError, bResetLimit, limit.dwMin, limit.dwMax))
#else
					if (!VerifyNumericEditControl (hdlg, hwnd, FALSE, bResetLimit, limit.dwMin, limit.dwMax))
#endif
						return FALSE;

					break;
				}

				case EC_UPDATESPIN:
				{
					/* // Update the spin button position to that in the  */
					/* // associated edit control */

					HWND hwndSpin;

					/* // Match the edit control to its spin control */
					Edit_GetSpinControl(hwnd, &hwndSpin);
	
					if (hwndSpin != NULL)
					{
						DWORD dwValue;
						char szNumber[20];

						Edit_GetText (hwnd, szNumber, sizeof (szNumber));
						dwValue = my_atodw(szNumber);

						/* // Align the spin control with the new value in the edit */
						/* // control */

						SpinPositionSet (hwndSpin, dwValue);
					}
					break;
				}

				default:
					ASSERT(NULL);
			}
		}
	}

	return TRUE;
}


BOOL VerifyAllNumericEditControls(HWND hwnd, BOOL bShowError, BOOL bResetLimit)
{
	ENUMEDIT edit;

	edit.editAction = EC_VERIFY;
	edit.action.verify.bShowError = bShowError;
	edit.action.verify.bResetLimit = bResetLimit;

	return EnumChildWindows (hwnd, EnumNumericEditCntls, (LPARAM)(LPENUMEDIT)&edit);
}


void SubclassAllNumericEditControls(HWND hwnd, EDITACTION ea)
{
	ENUMEDIT edit;

	ASSERT(ea == EC_SUBCLASS || ea == EC_RESETSUBCLASS);

	edit.editAction = ea;

	EnumChildWindows (hwnd, EnumNumericEditCntls, (LPARAM)(LPENUMEDIT)&edit);
}


void LimitNumericEditControls(HWND hwnd)
/*
	Function:
		Limits the edit control text length to the maximum number of
		characters necessary to input the max. value.

	Parameters:
		hwnd	- Handle of the dialog window

	Returns:
		None.
*/
{
	ENUMEDIT edit;

	edit.editAction = EC_LIMITTEXT;

	EnumChildWindows(hwnd, EnumNumericEditCntls, (LPARAM)(LPENUMEDIT)&edit);
}


void UpdateSpinButtons(HWND hwnd)
{
	ENUMEDIT edit;

	edit.editAction = EC_UPDATESPIN;

	EnumChildWindows(hwnd, EnumNumericEditCntls, (LPARAM)(LPENUMEDIT)&edit);
}


HWND GetEditSpinControl (HWND hwndEdit, LPEDITMINMAX lpLimits)
/*
	Function:
		Gets the spin button handle associated with the given edit control

	Parameters:
		hwndEdit	- Handle to the edit control

	Returns:
		The handle of the associated spin button if successful otherwise NULL.
*/
{
	int id;

	if (!IsWindow(hwndEdit))
	{
		ASSERT(FALSE);
		return NULL;
	}

	id = GetDlgCtrlID(hwndEdit);

	while (lpLimits->nEditId != -1)
	{
		if (lpLimits->nEditId == id)
			return GetDlgItem(GetParent(hwndEdit), lpLimits->nSpinId);

		lpLimits++;
	}

	return NULL;
}


HWND GetSpinEditControl (HWND hwndSpin, LPEDITMINMAX lpLimits)
/*
	Function:
		Gets the edit control handle associated with the given spin control

	Parameters:
		hwndSpin	- Handle to the spin control

	Returns:
		The handle of the associated edit control if successful otherwise NULL.
*/
{
	int id;

	if (!IsWindow(hwndSpin))
	{
		ASSERT(FALSE);
		return NULL;
	}

	id = GetDlgCtrlID(hwndSpin);

	while (lpLimits->nEditId != -1)
	{
		if (lpLimits->nSpinId == id)
			return GetDlgItem(GetParent(hwndSpin), lpLimits->nEditId);

		lpLimits++;
	}

	return NULL;
}


BOOL ProcessSpinControl(HWND hwndCtl, UINT codeNotify, LPEDITMINMAX lpLimits)
{
	/* // Handle the spin button notifications of increment and decrement */
#if 0
	BOOL bShift = GetKeyState(VK_SHIFT);
#else
	BOOL bShift = FALSE;
#endif
	int nIncrement = bShift ? 100 : 1;
	HWND hwnd = GetParent(hwndCtl);

	switch (codeNotify)
	{
		case SN_INCBUTTONUP:
		case SN_DECBUTTONUP:
		{
			HWND hwndEdit = GetSpinEditControl(hwndCtl, lpLimits);
			int nInc = (codeNotify == SN_INCBUTTONUP ? (nIncrement * -1) : nIncrement);

			ReleaseCapture();

			/* // Verify all the numeric controls that could lose the focus. */
			/* // If there is an error show it and break out without */
			/* // processing the spin button */

			if (!VerifyAllNumericEditControls(hwnd, TRUE, TRUE))
				return FALSE;

			/* // Process the spin button */

			EditIncrement(hwndEdit, nInc);

			SetFocus(hwndEdit);
			Edit_SetSel(hwndEdit, 0, -1);
			break;
		}

		case SN_POSINCREMENT:
		case SN_POSDECREMENT:
		{
			/* // We must capture all mouse control until the button has */
			/* // been released otherwise in error conditions the error */
			/* // message box eats our WM_KEYUP message and the spin */
			/* // button gets stuck down. */

			SetCapture(hwndCtl);
			break;
		}
	}

	return TRUE;
}


void CAListBox_SelectItems(HWND hwndCtl, UINT uFlags, LPCTLDATA lpdata)
{
	int nItem = 0;
	char szBuffer[100];

	while (lpdata[nItem].nId != -1)
	{
		if ((lpdata[nItem].nId & uFlags) != 0)
		{
			if (LoadString(hResource, lpdata[nItem].nStringId, szBuffer, sizeof(szBuffer)) != 0)
				CAListBox_SelectString(hwndCtl, -1, szBuffer);
		}
		nItem++;
	}
}


BOOL AllocDlgProp(HWND hwnd, LPVOID lpv)
{
	HLOCAL hmem = LocalAlloc(LHND, sizeof(DLGPROP));
	LPDLGPROP lpprop = hmem ? (LPDLGPROP)LocalLock(hmem) : NULL;
	if (lpprop)
	{
		lpprop->lpv = lpv;
		SetProp(hwnd, SZDLGPROP, hmem);
		LocalUnlock(hmem);
		return TRUE;
	}

	if (hmem)
		LocalFree(hmem);

	ASSERT(NULL);
	EndDialog(hwnd, -1);
	return FALSE;
}



void DeallocDlgProp(HWND hwnd)
{
	HLOCAL hmem = GetProp(hwnd, SZDLGPROP);
	LocalFree(hmem);
	RemoveProp(hwnd, SZDLGPROP);
}



LPVOID GetDlgProp(HWND hwnd)
{
	HLOCAL hmem = GetProp(hwnd, SZDLGPROP);
	LPDLGPROP lpprop = (LPDLGPROP)LocalLock(hmem);
	LPVOID lpv1 = lpprop->lpv;
	LocalUnlock(hmem);
	return lpv1;
}


HWND GetAppWindow(HWND hwnd)
{
	HWND hwndTemp;
	HWND hwndRet;

	ASSERT(IsWindow(hwnd));

	hwndRet = hwnd;

	/* // Keep finding our parent until there no longer is a parent. */

	while ((hwndTemp = GetParent(hwndRet)))
	{
		hwndRet = hwndTemp;
	}

	ASSERT(IsWindow(hwndRet));
	return hwndRet;
}



void richCenterDialog(HWND hdlg)
{
	RECT rect,rcAppli;
	HWND hwndApp = GetAppWindow(hdlg);

	GetWindowRect(hwndApp, &rcAppli);
	GetWindowRect(hdlg, &rect);
	OffsetRect(&rect, -rect.left, -rect.top);

	rect.left = ((rcAppli.left + rcAppli.right - rect.right) / 2 + 4) & ~7;
	rect.top = (rcAppli.top + rcAppli.bottom - rect.bottom) / 2;

	if (rect.left < 0)
		rect.left = 0;
	else if (rect.left + rect.right > GetSystemMetrics(SM_CXSCREEN))
		rect.left = GetSystemMetrics(SM_CXSCREEN) - rect.right;
	if (rect.top < 0)
		rect.top = 0;
	else if (rect.top + rect.bottom > GetSystemMetrics(SM_CYSCREEN))
		rect.top = GetSystemMetrics(SM_CYSCREEN) - rect.bottom;

	MoveWindow(hdlg, rect.left, rect.top, rect.right, rect.bottom, FALSE);
}

BOOL VerifyIndexName(HWND hwnd, LPSTR lpszIndex, BOOL bShowError)
{
	if (!IsWindow(hwnd) || !lpszIndex)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}


LPOBJECTLIST CreateList4CheckedObjects(HWND hwndCtl)
/*
   Function:
	  Get the names of objects that have been checked and
	  inserts them into a linked list of OBJECTLIST.

   Parameters:
	  hwndCtl  - Handle of the CA list box control to create the list for.

   Returns:
	  Pointer to created list if successful, otherwise NULL.
*/
{
   int nListCount = CAListBox_GetCount (hwndCtl);
   LPOBJECTLIST lpList = NULL;
   int nIndex;

   for (nIndex = 0; nIndex < nListCount; nIndex++)
   {
	  BOOL bChecked = CAListBox_GetSel (hwndCtl, nIndex);

	  if (bChecked)
	  {
		 LPCHECKEDOBJECTS lpObj;
		 LPOBJECTLIST lpTemp;

		 lpTemp = AddListObject(lpList, sizeof(CHECKEDOBJECTS));
		 if (!lpTemp)
		 {
			/* // Need to free previously allocated objects. */
			FreeObjectList(lpList);
			return NULL;
		 }
		 else
			lpList = lpTemp;

		 lpObj = (LPCHECKEDOBJECTS)lpList->lpObject;
		 CAListBox_GetText (hwndCtl, nIndex, lpObj->dbname);
		 lpObj->bchecked = TRUE;
	  }
   }

   return lpList;
}


LPOBJECTLIST AddListObject(LPOBJECTLIST lpList, int cbObjSize)
/*
   Function:
	  Adds a new element to the front of the list.

   Parameters:
	  lpList	  - Pointer to linked list.
	  cbObjSize   - Size of object to add to list

   Returns:
	  Pointer to the new list object if successful, otherwise NULL.
*/
{
   LPOBJECTLIST lpNewElement = ESL_AllocMem(sizeof(OBJECTLIST));

   if (lpNewElement)
   {
	  lpNewElement->lpObject = ESL_AllocMem(cbObjSize);

	  if (!lpNewElement->lpObject)
	  {
		 ESL_FreeMem(lpNewElement);
		 return NULL;
	  }

	  lpNewElement->lpNext = (LPVOID)lpList;
   }
   return lpNewElement;
}

LPOBJECTLIST AddListObjectTail(LPOBJECTLIST FAR * lplpList, int cbObjSize)
/*
   Function:
	  Adds a new element to the tail of the list.

   Parameters:
	  lplpList	  - Address of root pointer to the list.
	  cbObjSize   - Size of object to add to list

   Returns:
	  Pointer to the new list object if successful, otherwise NULL.
*/
{
   LPOBJECTLIST lpNewElement = ESL_AllocMem(sizeof(OBJECTLIST));

   if (lpNewElement)
   {
	  LPOBJECTLIST lpFind;

	  lpNewElement->lpObject = ESL_AllocMem(cbObjSize);

	  if (!lpNewElement->lpObject)
	  {
		 ESL_FreeMem(lpNewElement);
		 return NULL;
	  }

	  /* // Find the current last object in the list */

	  lpFind = *lplpList;
	  while (lpFind)
	  {
		 if (!lpFind->lpNext)
			break;

		 lpFind = lpFind->lpNext;
	  }

	  if (lpFind != NULL)
		 lpFind->lpNext = lpNewElement;
	  else
		 *lplpList = lpNewElement;

	  lpNewElement->lpNext = NULL;
   }

   return lpNewElement;
}



void FreeObjectList(LPOBJECTLIST lpList)
{
   while (lpList)
   {
	  LPOBJECTLIST lpTemp = (LPOBJECTLIST)lpList->lpNext;
	  ESL_FreeMem(lpList->lpObject);
	  ESL_FreeMem(lpList);
	  lpList = lpTemp;
   }
}


LPOBJECTLIST DeleteListObject(LPOBJECTLIST lpList, LPOBJECTLIST lpDelElement)
/*
   Function:
	  Deletes an object from the list

   Parameters:
	  lpList		 - Head of list from which element is to be deleted.
	  lpDelElement	 - Element to be deleted

   Returns:
	  The start of the list.  This must be used in case the head of the list
	  is deleted.
*/
{
/* // vut  if (!lpList || !lpDelElement) */
/* //	   return NULL; */

   if (!lpDelElement)
	  return lpList;
   if (!lpList)
	  return NULL;


   if (lpList == lpDelElement)
   {
	  /* // Deleting the head of the list. */

	  lpList = lpDelElement->lpNext;
   }
   else
   {
	  /* // Find the element before the element to be deleted. */

	  LPOBJECTLIST lpPrev = lpList;

	  while (lpPrev && lpPrev->lpNext != lpDelElement)
		 lpPrev = lpPrev->lpNext;

	  if (!lpPrev)
		 /* // The element to be deleted was not found. */
		 return lpList;

	  /* // Point the previous element to the one after the element to be deleted. */

	  lpPrev->lpNext = lpDelElement->lpNext;
   }
   if (lpDelElement->lpObject)
		ESL_FreeMem(lpDelElement->lpObject);
   lpDelElement->lpNext = NULL;
   lpDelElement->lpObject = NULL;
   ESL_FreeMem(lpDelElement);

   return lpList;
}


LPOBJECTLIST FindPrevObject(LPOBJECTLIST lplist, LPOBJECTLIST lpcurrent)
{
   /* // Check for object being at the head of the list. */
   
   if (lplist == lpcurrent)
	  return NULL;

   while (lplist)
   {
	  /* // If the next object is the current one, then this object */
	  /* // must be the previous. */

	  if (lplist->lpNext == lpcurrent)
		 return lplist;

	  lplist = lplist->lpNext;
   }
   
   /* // The object does not exist in the given list. */

   ASSERT(NULL);
   return NULL;
}


int CountListItems(LPOBJECTLIST lplist)
{
   int nRet = 0;

   while (lplist)
   {
	  nRet++;
	  lplist = lplist->lpNext;
   }

   return nRet;
}


void Rectangle3D(HDC hdc, int nLeft, int nTop, int nRight, int nBottom)
{
   HPEN hpen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
   HPEN hpenOld;

   hpenOld = SelectObject(hdc, hpen);

   /* // Get the correct background */

   Rectangle(hdc, nLeft + 1, nTop + 1, nRight - 1, nBottom - 1);

   /* // Draw right and bottom border in white */

	MoveToEx(hdc, nRight - 1, nTop + 1, NULL);
	LineTo(hdc, nRight - 1, nBottom);

	MoveToEx(hdc, nLeft + 1, nBottom - 1, NULL);
	LineTo(hdc, nRight, nBottom - 1);

   /* // Draw left and top border in dark grey */

   SelectObject(hdc, hpenOld);
   DeleteObject(hpen);
   hpen = CreatePen(PS_SOLID, 1, RGB(127, 127, 127));
   SelectObject(hdc, hpen);

	MoveToEx(hdc, nLeft, nTop, NULL);
	LineTo(hdc, nRight - 1, nTop);

	MoveToEx(hdc, nLeft, nTop, NULL);
	LineTo(hdc, nLeft, nBottom - 1);

   /* // Draw inside top and left border in black */

   SelectObject(hdc, hpenOld);
   DeleteObject(hpen);
   hpen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
   SelectObject(hdc, hpen);

	MoveToEx(hdc, nLeft + 1, nTop + 1, NULL);
	LineTo(hdc, nRight - 2, nTop + 1);

	MoveToEx(hdc, nLeft + 1, nTop + 1, NULL);
	LineTo(hdc, nLeft + 1, nBottom - 1);

   /* // Draw inside right and bottom border in light grey */

   SelectObject(hdc, hpenOld);
   DeleteObject(hpen);
   hpen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
   SelectObject(hdc, hpen);

	MoveToEx(hdc, nRight - 2, nTop + 1, NULL);
	LineTo(hdc, nRight - 2, nBottom - 1);

	MoveToEx(hdc, nLeft + 2, nBottom - 2, NULL);
	LineTo(hdc, nRight - 1, nBottom - 2);

   SelectObject(hdc, hpenOld);
   DeleteObject(hpen);
}


void DrawCheck3D(HDC hdc, int nLeft, int nTop, int nRight, int nBottom, BOOL bGrey)
{
   HPEN hpen, hpenOld = NULL;

   nLeft += 3;
   nTop += 3;
   nRight -= 3;
   nBottom -= 3;

   if (bGrey)
   {
	  hpen = CreatePen(PS_SOLID, 1, DARK_GREY);
	  hpenOld = SelectObject(hdc, hpen);
   }

   MoveToEx(hdc, nLeft, nTop, NULL);
   LineTo(hdc, nRight, nBottom);

   MoveToEx(hdc, nLeft, nTop + 1, NULL);
   LineTo(hdc, nRight - 1, nBottom);

   MoveToEx(hdc, nLeft + 1, nTop, NULL);
   LineTo(hdc, nRight, nBottom - 1);

   MoveToEx(hdc, nRight - 1, nTop, NULL);
   LineTo(hdc, nLeft - 1, nBottom);

   MoveToEx(hdc, nRight - 2, nTop, NULL);
   LineTo(hdc, nLeft - 1, nBottom - 1);

   MoveToEx(hdc, nRight - 1, nTop + 1, NULL);
   LineTo(hdc, nLeft, nBottom);

   if (bGrey)
   {
	  SelectObject(hdc, hpenOld);
	  DeleteObject(hpen);
   }
}


void ScreenToClientRect(HWND hwndClient, LPRECT lprect)
{
   POINT pt;

   pt.x = lprect->left;
   pt.y = lprect->top;

   ScreenToClient(hwndClient, &pt);

   lprect->left = pt.x;
   lprect->top = pt.y;

   pt.x = lprect->right;
   pt.y = lprect->bottom;

   ScreenToClient(hwndClient, &pt);

   lprect->right = pt.x;
   lprect->bottom = pt.y;
}



void PreselectGrantee (HWND hwndCtl, const char* selectString)
{
   CAListBox_SelectString (hwndCtl, -1, selectString);
}


//    09-Nov-99 (noifr01)
//      (bug #99447) removed incorrect change that was done in the AddItemtoList 
//      function for a partial fix of bug #97680 (support of object names with
//      special characters)

// AddITemToList: this function provides a linked list including the checked
// objects in a "checkbox list".
// Object names are specified to remain "exact" (no quoting if the object includes
// special ambiguous characters). For this reason, it should not be used for objects
// where a schema can prefix the objectname (such as table lists for example). For such
// objects, the AddItemToListWithOwner() function should be used instead.
// (more detail will come with the final IP for bug #97680)
LPOBJECTLIST AddItemToList (HWND hwndCtl)
{
	LPOBJECTLIST list = NULL;
	LPOBJECTLIST obj;
	char buff  [MAXOBJECTNAME +1];
	int	i, max_item_number;
	BOOL checked;
	
	max_item_number = CAListBox_GetCount (hwndCtl);
	
	for (i = 0; i < max_item_number; i++)
	{
		checked = CAListBox_GetSel (hwndCtl, i);
		if (checked == 1)
		{
			CAListBox_GetText  (hwndCtl, i, buff);

			if (x_strcmp (buff, lppublicdispstring()) == 0)
				obj = AddListObject (list, x_strlen (buff) +1);
			else
				obj = AddListObject (list, x_strlen (buff) +1);

			if (obj)
			{
				if (x_strcmp (buff, lppublicdispstring()) == 0)
					x_strcpy ((UCHAR *)obj->lpObject, lppublicsysstring());
				else
					x_strcpy ((UCHAR *)obj->lpObject, buff);
				list = obj;
			}
			else
			{
				/* // */
				/* // Cannot allocate memory */
				/* // */
				FreeObjectList (list);
				list = NULL;
				return (NULL);
			}
		}
	}
	return list;
}


LPOBJECTLIST AddItemToListWithOwner (HWND hwndCtl)
{
	LPOBJECTLIST
		list = NULL,
		obj;
	char
		buffa  [MAXOBJECTNAME +1],
		buffb  [MAXOBJECTNAME +1],
		buffowner [(MAXOBJECTNAME*2) +1];

	int	i, max_item_number;
	BOOL checked;
	
	max_item_number = CAListBox_GetCount (hwndCtl);
	
	for (i = 0; i < max_item_number; i++)
	{
		checked = CAListBox_GetSel (hwndCtl, i);
		if (checked)
		{
			CAListBox_GetText  (hwndCtl, i, buffa); /* // Object */
			x_strcpy (buffb,  (char *) CAListBox_GetItemData (hwndCtl, i));
			wsprintf(buffowner,"%s.%s",QuoteIfNeeded(buffb),
				                       QuoteIfNeeded(RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(buffa))));
			obj = AddListObject (list, x_strlen (buffowner) +1);
			if (obj)
			{
				x_strcpy ((UCHAR *)obj->lpObject, buffowner);
				list = obj;
			}
			else
			{
				/* // */
				/* // Cannot allocate memory */
				/* // */
				FreeObjectList (list);
				list = NULL;
				return (NULL);
			}
		}
	}
	return list;
}

// Quote the object name for the SQL Syntax, only for the objects who don't
// have schema name.
LPOBJECTLIST AddItemToListQuoted (HWND hwndCtl)
{
	LPOBJECTLIST
		list = NULL,
		obj;
	char buffa  [MAXOBJECTNAME +1];
	LPCTSTR buffb;

	int	i, max_item_number;
	BOOL checked;
	
	max_item_number = CAListBox_GetCount (hwndCtl);
	
	for (i = 0; i < max_item_number; i++)
	{
		checked = CAListBox_GetSel (hwndCtl, i);
		if (checked)
		{
			CAListBox_GetText  (hwndCtl, i, buffa); /* // Object */

			if (x_strcmp (buffa, lppublicdispstring()) == 0)
				buffb = lppublicsysstring();
			else
				buffb = QuoteIfNeeded(buffa);

			obj = AddListObject (list, x_strlen (buffb) +1);

			if (obj)
			{
				x_strcpy ((UCHAR *)obj->lpObject, buffb);
				list = obj;
			}
			else
			{
				/* // */
				/* // Cannot allocate memory */
				/* // */
				FreeObjectList (list);
				list = NULL;
				return (NULL);
			}
		}
	}
	return list;
}


LPOBJECTLIST APublicUser ()
{
   LPOBJECTLIST list = NULL;
   LPOBJECTLIST obj;

   obj = AddListObject (list, x_strlen (lppublicsysstring()) +1);
   if (obj)
   {
	   x_strcpy ((UCHAR *)obj->lpObject, lppublicsysstring());
	   list = obj;
   }
   else
   {
	   /* // */
	   /* // Cannot allocate memory */
	   /* // */
	   list = NULL;
   }
   return (list);
}

/* ________________________
//
// FUNCTION: local function
//
// objectType bust be : (OT_USER or OT_GROUP or OT_ROLE or OT_DATABASE)
//
// ____________________________________________________________________
*/
static void FillComboWithObject (HWND hwndCtl, int objectType)
{
   int	   hdl, ires;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
	 
   ZEROINIT (buf);
   ZEROINIT (buffilter);

   hdl		= GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
	   
   ires 	= DOMGetFirstObject (hdl, objectType, 0, NULL, bwsystem, NULL, buf, NULL, NULL);
   while (ires == RES_SUCCESS)
   {
	   if (x_strcmp (buf, lppublicdispstring()) != 0)
		   ComboBox_AddString (hwndCtl, buf);
	   ires  = DOMGetNextObject (buf, buffilter, NULL);
   }
}

/* ________________________
//
// FUNCTION: local function
//
// objectType bust be : (OT_USER or OT_GROUP or OT_ROLE or OT_DATABASE)
//
// ____________________________________________________________________
*/
static void FillCAListWithObject (HWND hwndCtl, int objectType)
{
   int	   hdl, ires;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
	 
   ZEROINIT (buf);
   ZEROINIT (buffilter);

   hdl		= GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
	   
   ires 	= DOMGetFirstObject (hdl, objectType, 0, NULL, bwsystem, NULL, buf, NULL, NULL);
   while (ires == RES_SUCCESS)
   {
	   if (x_strcmp (buf, lppublicdispstring()) != 0)
		   CAListBox_AddString (hwndCtl, buf);
	   ires  = DOMGetNextObject (buf, buffilter, NULL);
   }
}

void ComboBoxFillUsers	   (HWND hwndCtl) { FillComboWithObject (hwndCtl, OT_USER);    }
void CAListBoxFillUsers    (HWND hwndCtl) { FillCAListWithObject(hwndCtl, OT_USER);    }
void ComboBoxFillGroups    (HWND hwndCtl) { FillComboWithObject (hwndCtl, OT_GROUP);   }
void ComboBoxFillProfiles  (HWND hwndCtl) { FillComboWithObject (hwndCtl, OT_PROFILE); }
void CAListBoxFillGroups   (HWND hwndCtl) { FillCAListWithObject(hwndCtl, OT_GROUP);   }
void CAListBoxFillProfiles (HWND hwndCtl) { FillCAListWithObject(hwndCtl, OT_PROFILE); }
void ComboBoxFillRoles	   (HWND hwndCtl) { FillComboWithObject (hwndCtl, OT_ROLE);    }
void CAListBoxFillRoles    (HWND hwndCtl) { FillCAListWithObject(hwndCtl, OT_ROLE);    }
void ComboBoxFillDatabases (HWND hwndCtl) { FillComboWithObject (hwndCtl, OT_DATABASE);}
void CAListBoxFillDatabases(HWND hwndCtl) { FillCAListWithObject(hwndCtl, OT_DATABASE);}
void ComboBoxFillLocations (HWND hwndCtl) { FillComboWithObject (hwndCtl, OT_LOCATION);}
void CAListBoxFillLocations(HWND hwndCtl) { FillCAListWithObject(hwndCtl, OT_LOCATION);}

void FillGrantees (HWND hwndCtl, int GranteeType)
{
   switch (GranteeType)
   {
	   case OT_USER:
		   CAListBoxFillUsers (hwndCtl);
		   break;
	   case OT_GROUP:
		   CAListBoxFillGroups(hwndCtl);
		   break;
	   case OT_ROLE:
		   CAListBoxFillRoles (hwndCtl);
		   break;
	   default:
		   break;
   }
}

/* ________________________
//
// FUNCTION: local function
//
// objectType bust be : (OT_TABLE or OT_PROCEDURE or OT_VIEW or OT_DBEVENT)
//
// ____________________________________________________________________
*/
static void FillCAListWithObject2 (HWND hwndCtl, LPUCHAR DatabaseName, int objectType)
{
   int	   hdl, ires;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   parentstrings [0] = DatabaseName;
   parentstrings [1] = NULL;

   hdl		= GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   ires = DOMGetFirstObject (hdl, objectType, 1,
							 parentstrings, bwsystem, NULL, buf, NULL, NULL);
   while (ires == RES_SUCCESS)
   {
	   CAListBox_AddString (hwndCtl, buf);
	   ires    = DOMGetNextObject (buf, buffilter, NULL);
   }
}


/* ________________________
//
// FUNCTION: local function
//
// objectType bust be : (OT_TABLE or OT_PROCEDURE or OT_VIEW or OT_DBEVENT)
//
// ____________________________________________________________________
*/
static void FillComboBoxWithObject2 (HWND hwndCtl, LPUCHAR DatabaseName, int objectType)
{
   int	   hdl, ires;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   parentstrings [0] = DatabaseName;
   parentstrings [1] = NULL;

   hdl		= GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   ires = DOMGetFirstObject (hdl, objectType, 1,
							 parentstrings, bwsystem, NULL, buf, NULL, NULL);
   while (ires == RES_SUCCESS)
   {
	   ComboBox_AddString (hwndCtl, buf);
	   ires    = DOMGetNextObject (buf, buffilter, NULL);
   }
}



static BOOL FillComboBoxWithOwner (HWND hwndCtl, LPUCHAR DatabaseName, int objectType)
{
   int	   hdl, ires, idx;
   BOOL    bwsystem;
   char    buf		 [MAXOBJECTNAME];
   char    szOwner	 [MAXOBJECTNAME];
   char*   buffowner;

   LPUCHAR parentstrings [MAXPLEVEL];

   ZEROINIT (buf);
   parentstrings [0] = DatabaseName;
   parentstrings [1] = NULL;

   hdl		= GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   ires = DOMGetFirstObject (
	   hdl,
	   objectType,
	   1,
	   parentstrings,
	   bwsystem,
	   NULL,
	   buf,
	   szOwner,
	   NULL);
   while (ires == RES_SUCCESS)
   {
	   idx = ComboBox_AddString   (hwndCtl, buf);
	   buffowner = ESL_AllocMem (x_strlen (szOwner) +1);
	   if (!buffowner)
		   return FALSE;
	   x_strcpy (buffowner, szOwner);
	   ComboBox_SetItemData (hwndCtl, idx, buffowner);

	   ires    = DOMGetNextObject (buf, szOwner, NULL);
   }
   return TRUE;
}

/* // */
/* // ___________________________________________________________________________________ */
/* // */
static BOOL FillCAListBoxWithOwner (HWND hwndCtl, LPUCHAR DatabaseName, int objectType)
{
   int	   hdl, ires, idx;
   BOOL    bwsystem;
   char    buf		 [MAXOBJECTNAME];
   char    szOwner	 [MAXOBJECTNAME];
   char*   buffowner;

   LPUCHAR parentstrings [MAXPLEVEL];

   ZEROINIT (buf);
   parentstrings [0] = DatabaseName;
   parentstrings [1] = NULL;

   hdl		= GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   ires = DOMGetFirstObject (
	   hdl,
	   objectType,
	   1,
	   parentstrings,
	   bwsystem,
	   NULL,
	   buf,
	   szOwner,
	   NULL);
   while (ires == RES_SUCCESS)
   {
	   idx = CAListBox_AddString   (hwndCtl, buf);
	   buffowner = ESL_AllocMem (x_strlen (szOwner) +1);
	   if (!buffowner)
		   return FALSE;
	   x_strcpy (buffowner, szOwner);
	   CAListBox_SetItemData (hwndCtl, idx, buffowner);

	   ires    = DOMGetNextObject (buf, szOwner, NULL);
   }
   return TRUE;
}


BOOL ComboBoxFillProcedures (HWND hwndCtl, LPUCHAR DatabaseName)
{
   return (FillComboBoxWithOwner (hwndCtl, DatabaseName, OT_PROCEDURE));
}

BOOL CAListBoxFillProcedures(HWND hwndCtl, LPUCHAR DatabaseName)
{
   return (FillCAListBoxWithOwner (hwndCtl, DatabaseName, OT_PROCEDURE));
}

BOOL ComboBoxFillDBevents (HWND hwndCtl, LPUCHAR DatabaseName)
{
   return (FillComboBoxWithOwner (hwndCtl, DatabaseName, OT_DBEVENT));
}

BOOL CAListBoxFillDBevents (HWND hwndCtl, LPUCHAR DatabaseName)
{
   return (FillCAListBoxWithOwner (hwndCtl, DatabaseName, OT_DBEVENT));
}

BOOL CAListBoxFillSequences (HWND hwndCtl, LPUCHAR DatabaseName)
{
   return (FillCAListBoxWithOwner (hwndCtl, DatabaseName, OT_SEQUENCE));
}






BOOL ComboBoxFillTables (HWND hwndCtl, LPUCHAR DatabaseName)
{
	int		hdl, ires, idx;
	BOOL		bwsystem;
	char		buf 	 [MAXOBJECTNAME];
	char		szOwner	 [MAXOBJECTNAME];
	char		szExtra [MAXOBJECTNAME];
	char*		buffowner;

	LPUCHAR parentstrings [MAXPLEVEL];

	ZEROINIT (buf);
	ZEROINIT (szExtra);
	parentstrings [0] = DatabaseName;
	parentstrings [1] = NULL;

	hdl		= GetCurMdiNodeHandle ();
	bwsystem = GetSystemFlag ();

	ires = DOMGetFirstObject (
		hdl,
		OT_TABLE,
		1,
		parentstrings,
		bwsystem,
		NULL,
		buf,
		szOwner,
		szExtra);
	idx = CB_ERR;
	while (ires == RES_SUCCESS)
	{
		if (getint(szExtra+STEPSMALLOBJ+STEPSMALLOBJ) == 0)
		  idx = ComboBox_AddString	(hwndCtl, buf);
		buffowner = ESL_AllocMem (x_strlen (szOwner) +1);
		if (!buffowner)
			return FALSE;
		x_strcpy (buffowner, szOwner);
		if (idx != CB_ERR && idx != CB_ERRSPACE)
		{
		  ComboBox_SetItemData (hwndCtl, idx, buffowner);
		  idx = CB_ERR;
		}

		ires	  = DOMGetNextObject (buf, szOwner, szExtra);
	}
	return TRUE;
}

BOOL ComboBoxFillTablesFiltered (HWND hwndCtl, LPUCHAR DatabaseName, BOOL bVW)
{
	int		hdl, ires, idx;
	BOOL		bwsystem;
	char		buf 	 [MAXOBJECTNAME];
	char		szOwner	 [MAXOBJECTNAME];
	char		szExtra [MAXOBJECTNAME];
	char*		buffowner;

	LPUCHAR parentstrings [MAXPLEVEL];

	ZEROINIT (buf);
	ZEROINIT (szExtra);
	parentstrings [0] = DatabaseName;
	parentstrings [1] = NULL;

	hdl		= GetCurMdiNodeHandle ();
	bwsystem = GetSystemFlag ();

	ires = DOMGetFirstObject (
		hdl,
		OT_TABLE,
		1,
		parentstrings,
		bwsystem,
		NULL,
		buf,
		szOwner,
		szExtra);
	idx = CB_ERR;
	while (ires == RES_SUCCESS)
	{
		if ((!bVW && getint(szExtra+STEPSMALLOBJ+STEPSMALLOBJ) == 0)
			|| (bVW && getint(szExtra+STEPSMALLOBJ+STEPSMALLOBJ) > 0))
			idx = ComboBox_AddString	(hwndCtl, buf);
		buffowner = ESL_AllocMem (x_strlen (szOwner) +1);
		if (!buffowner)
			return FALSE;
		x_strcpy (buffowner, szOwner);
		if (idx != CB_ERR && idx != CB_ERRSPACE)
		{
		  ComboBox_SetItemData (hwndCtl, idx, buffowner);
		  idx = CB_ERR;
		}

		ires	  = DOMGetNextObject (buf, szOwner, szExtra);
	}
	return TRUE;
}

BOOL CAListBoxFillTables (HWND hwndCtl, LPUCHAR DatabaseName)
{
	int		hdl, ires, idx;
	BOOL		bwsystem;
	char		buf		[MAXOBJECTNAME];
	char		szOwner	[MAXOBJECTNAME];
	char		szExtra [MAXOBJECTNAME];
	char*		buffowner;

	LPUCHAR parentstrings [MAXPLEVEL];

	ZEROINIT (buf);
	ZEROINIT (szOwner);
	ZEROINIT (szExtra);

	parentstrings [0] = DatabaseName;
	parentstrings [1] = NULL;

	hdl		= GetCurMdiNodeHandle ();
	bwsystem = GetSystemFlag ();
	
	ires = DOMGetFirstObject (
		hdl,
		OT_TABLE,
		1,
		parentstrings,
		bwsystem,
		NULL,
		buf,
		szOwner,
		szExtra);
	if (ires != RES_SUCCESS)
		return FALSE;
	idx = CB_ERR;
	while (ires == RES_SUCCESS)
	{
		// Do not display VectorWise tables b/c this table type
		// doesn't support this command.
		if (getint(szExtra+STEPSMALLOBJ+STEPSMALLOBJ) == 0)
			idx = CAListBox_AddString	 (hwndCtl, buf);
		buffowner = ESL_AllocMem (x_strlen (szOwner) +1);
		if (!buffowner)
			return FALSE;
		x_strcpy (buffowner, szOwner);
		if (idx != CB_ERR && idx != CB_ERRSPACE)
		{
		  CAListBox_SetItemData (hwndCtl, idx, buffowner);
		  idx = CB_ERR;
		}
			
		ires	  = DOMGetNextObject (buf, szOwner, szExtra);
	}
	return TRUE;
}



BOOL ComboBoxFillViews (HWND hwndCtl, LPUCHAR DatabaseName)
{
	int		hdl, ires, idx;
	BOOL		bwsystem;
	char		buf 	 [MAXOBJECTNAME];
	char		szOwner	 [MAXOBJECTNAME];
	char*		buffowner;

	LPUCHAR parentstrings [MAXPLEVEL];

	ZEROINIT (buf);
	parentstrings [0] = DatabaseName;
	parentstrings [1] = NULL;

	hdl		= GetCurMdiNodeHandle ();
	bwsystem = GetSystemFlag ();
	
	ires = DOMGetFirstObject (
		hdl,
		OT_VIEW,
		1,
		parentstrings,
		bwsystem,
		NULL,
		buf,
		szOwner,
		NULL);
	while (ires == RES_SUCCESS)
	{
		idx = ComboBox_AddString	(hwndCtl, buf);
		buffowner = ESL_AllocMem (x_strlen (szOwner) +1);
		if (!buffowner)
			return FALSE;
		x_strcpy (buffowner, szOwner);
		ComboBox_SetItemData (hwndCtl, idx, buffowner);

		ires	  = DOMGetNextObject (buf, szOwner, NULL);
	}
	return TRUE;
}

/* // */
/* // _________ */
/* // */
BOOL CAListBoxFillViews (HWND hwndCtl, LPUCHAR DatabaseName)
{
	int		hdl, ires, idx;
	BOOL		bwsystem;
	char		buf 	 [MAXOBJECTNAME];
	char		szOwner	 [MAXOBJECTNAME];
	char*		buffowner;

	LPUCHAR parentstrings [MAXPLEVEL];

	ZEROINIT (buf);
	parentstrings [0] = DatabaseName;
	parentstrings [1] = NULL;

	hdl		= GetCurMdiNodeHandle ();
	bwsystem = GetSystemFlag ();
	
	ires = DOMGetFirstObject (
		hdl,
		OT_VIEW,
		1,
		parentstrings,
		bwsystem,
		NULL,
		buf,
		szOwner,
		NULL);
	if (ires != RES_SUCCESS)
		return FALSE;
	while (ires == RES_SUCCESS)
	{
		idx = CAListBox_AddString	 (hwndCtl, buf);
		buffowner = ESL_AllocMem (x_strlen (szOwner) +1);
		if (!buffowner)
			return FALSE;
		x_strcpy (buffowner, szOwner);
		CAListBox_SetItemData (hwndCtl, idx, buffowner);

		ires	  = DOMGetNextObject (buf, szOwner, NULL);
	}
	return TRUE;
}


void ComboBoxDestroyItemData (HWND hwndCtl)
{
	char* item;
	int	 i, n = ComboBox_GetCount (hwndCtl);

	for (i=0; i<n; i++)
	{
		item = (char *)ComboBox_GetItemData (hwndCtl, i);
		if (item)
			ESL_FreeMem (item);
	}
}


void CAListBoxDestroyItemData (HWND hwndCtl)
{
   char* item;
   int	 i, n = CAListBox_GetCount (hwndCtl);

   for (i=0; i<n; i++)
   {
	   item = (char *)CAListBox_GetItemData (hwndCtl, i);
	   if (item)
		  ESL_FreeMem (item);
   }
}



void ComboBoxSelectFirstStr (HWND hwndCtl)
{
	if (ComboBox_GetCount  (hwndCtl) > 0)
	{
		ComboBox_SetCurSel (hwndCtl, 0);
	}
}

void CAListBoxSelectFirstStr(HWND hwndCtl)
{
	if (CAListBox_GetCount	(hwndCtl) > 0)
	{
		CAListBox_SetCurSel (hwndCtl, 0);
	}
}


void CAListBoxSelectStringWithOwner (HWND hwndCtl, const char* aString, char* aOwner)
{
	char szItem [MAXOBJECTNAME];
	char szOwner [MAXOBJECTNAME];
	char szAll	[MAXOBJECTNAME];
	char szS1	[MAXOBJECTNAME];
	char szS2	[MAXOBJECTNAME];
	char szSample[MAXOBJECTNAME];
	int	i;
	int	nItem = CAListBox_GetCount (hwndCtl);

	if (x_strlen (aString) == 0 || x_strlen (aOwner) == 0)
		return;
	x_strcpy (szS1, aString);
	x_strcpy (szS2, aOwner);
	StringWithOwner (szS1, szS2, szSample);
	for (i=0; i<nItem; i++)
	{
		ZEROINIT (szItem);
		ZEROINIT (szOwner);
		CAListBox_GetText(hwndCtl, i, szItem);
		x_strcpy (szOwner, (char *)CAListBox_GetItemData (hwndCtl, i));
		StringWithOwner	(szItem, szOwner, szAll);
		if (x_strcmp (szSample, szAll) == 0)
		{
			CAListBox_SetSel (hwndCtl, TRUE, i);
			break;
		}
	}
}


void ComboBoxSelectStringWithOwner	(HWND hwndCtl, const char* aString, char* aOwner)
{
	char szItem [MAXOBJECTNAME];
	char szOwner [MAXOBJECTNAME];
	char szAll	[MAXOBJECTNAME];
	char szS1	[MAXOBJECTNAME];
	char szS2	[MAXOBJECTNAME];
	char szSample[MAXOBJECTNAME];
	int	i;
	int	nItem = ComboBox_GetCount (hwndCtl);

	if (x_strlen (aString) == 0 || x_strlen (aOwner) == 0)
		return;
	x_strcpy (szS1, aString);
	x_strcpy (szS2, aOwner);
	StringWithOwner (szS1, szS2, szSample);
	for (i=0; i<nItem; i++)
	{
		ZEROINIT (szItem);
		ZEROINIT (szOwner);
		ComboBox_GetLBText	 (hwndCtl, i, szItem);
		x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndCtl, i));
		StringWithOwner (szItem, szOwner, szAll);
		if (x_strcmp (szSample, szAll) == 0)
		{
			ComboBox_SetCurSel (hwndCtl, i);
			break;
		}
	}
}


/* // */
/* // The user call public is added into the list */
/* // */
void ComboBoxFillUsersP  (HWND hwndCtl)
{
	int		hdl, ires;
	BOOL		bwsystem;
	char		buf [MAXOBJECTNAME];
	char		buffilter [MAXOBJECTNAME];
	ZEROINIT (buf);
	ZEROINIT (buffilter);

	hdl		= GetCurMdiNodeHandle ();
	bwsystem = GetSystemFlag ();
		
	ires	= DOMGetFirstObject (hdl, OT_USER, 0, NULL, bwsystem, NULL, buf, NULL, NULL);
	while (ires == RES_SUCCESS)
	{
		ComboBox_AddString (hwndCtl, buf);
		ires	= DOMGetNextObject (buf, buffilter, NULL);
	}
}



/* // */
/* // The user call public is added into the list */
/* // */

void CAListBoxFillUsersP (HWND hwndCtl)
{
	int		hdl, ires;
	BOOL		bwsystem;
	char		buf [MAXOBJECTNAME];
	char		buffilter [MAXOBJECTNAME];
	ZEROINIT (buf);
	ZEROINIT (buffilter);

	hdl		= GetCurMdiNodeHandle ();
	bwsystem = GetSystemFlag ();
		
	ires	= DOMGetFirstObject (hdl, OT_USER, 0, NULL, bwsystem, NULL, buf, NULL, NULL);
	while (ires == RES_SUCCESS)
	{
		CAListBox_AddString (hwndCtl, buf);
		ires	= DOMGetNextObject (buf, buffilter, NULL);
	}
}

void GetExactDisplayString (
	const   char * aString,
	const   char* aOwner,
	int		aObjectType,
	LPUCHAR parentstrings [MAXPLEVEL],
	char*   buffExactDispString)
{
	char sample [MAXOBJECTNAME];
	char owner	[MAXOBJECTNAME];
	int	hdl, ires, level, xx1, xx2;
	BOOL bwsystem;
	LPUCHAR resultparentstrings[MAXPLEVEL];
	UCHAR buf[MAXPLEVEL][MAXOBJECTNAME];
	int i;
	for (i=0;i<MAXPLEVEL;i++) 
	  resultparentstrings[i]=buf[i];

	switch (aObjectType)
	{
		case OT_TABLE:
			level = 1;
			break;
		case OT_VIEW:
			level = 1;
			break;
		case OT_PROCEDURE:
			level = 1;
			break;
		case OT_DBEVENT:
			level = 1;
			break;
		case OT_RULE:
			level = 2;
			break;
		case OT_INDEX:
			level = 2;
			break;
		case OT_SYNONYMOBJECT:
			level = 1;
			break;
		default:
			x_strcpy (buffExactDispString, "");
			return;
	}


	x_strcpy (sample, Quote4DisplayIfNeeded(aString));
	x_strcpy (owner,  aOwner);
	hdl		= GetCurMdiNodeHandle ();
	bwsystem = GetSystemFlag ();
	
	ires = DOMGetObject (
		hdl,
		sample,					/* // Sample object name */
		owner,					/* // Owner */
		aObjectType,
		level,
		parentstrings,
		bwsystem,
		&xx1, 				/* // Not used */
		&xx2, 				/* // Not used */
		resultparentstrings, 	/* // Parentstring not used */
		buffExactDispString,
		NULL,
		NULL);
}






/*
// src is the forme '[XXXXX] yyyyy'
// 
// This function copy the sub string 'yyyyy' to the buffer 'dest'
// if src does not contain [] then dest will be the same as src.
*/
void ExcludeBraceString (LPUCHAR dest, const char* src)
{
	char * pCloseBrace = x_strchr(src,']');
	if (pCloseBrace) {
		if (x_strlen(pCloseBrace)>2)
			x_strcpy(dest,pCloseBrace+2);
		else {
			myerror(ERR_OLDCODE);
			x_strcpy(dest,pCloseBrace+1);

		}
	}
	else 
		x_strcpy (dest, src);
}

void CAListBoxFillTableColumns (HWND hwndCtl, LPUCHAR DBName, LPUCHAR TableName)
{
	int		err;
	char	  szObject [MAXOBJECTNAME];
	char	  szFilter [MAXOBJECTNAME];
	LPUCHAR aparents [MAXPLEVEL];

	ZEROINIT (aparents);
	ZEROINIT (szObject);
	ZEROINIT (szFilter);
	aparents[0] = DBName;
	aparents[1] = TableName;

	err = DOMGetFirstObject
			(GetCurMdiNodeHandle (),
			OT_COLUMN,
			2,
			aparents,
			GetSystemFlag (),
			NULL,
			szObject,
			NULL,
			NULL);

	while (err == RES_SUCCESS)
	{
		CAListBox_AddString (hwndCtl, szObject);
		err = DOMGetNextObject (szObject, szFilter, NULL);
	}
}

BOOL CAListBoxFillTableColumns_x_Types (HWND hwndCtl, LPUCHAR DBName, LPUCHAR TableName)
{
	int		idx, err;
	char	  szObject [MAXOBJECTNAME];
	char	  szFilter [MAXOBJECTNAME];
	char	  szColType[MAXOBJECTNAME];
	char*   buffColType;
	LPUCHAR aparents [MAXPLEVEL];

	ZEROINIT (aparents);
	ZEROINIT (szObject);
	ZEROINIT (szFilter);
	ZEROINIT (szColType);
	aparents[0] = DBName;
	aparents[1] = TableName;

	err = DOMGetFirstObject
			(GetCurMdiNodeHandle (),
			OT_COLUMN,
			2,
			aparents,
			FALSE,
			NULL,
			szObject,
			NULL,
			szColType);

	if (err != RES_SUCCESS)
		return FALSE;
	while (err == RES_SUCCESS)
	{
		idx = CAListBox_AddString (hwndCtl, szObject);
		/* //int nType = getint (szColTyp); */
		buffColType = ESL_AllocMem (MAXOBJECTNAME);
		if (!buffColType)
			return FALSE;
		memcpy (buffColType, szColType, 2 * STEPSMALLOBJ);
		buffColType[2 * STEPSMALLOBJ]='\0';

		CAListBox_SetItemData (hwndCtl, idx, buffColType);
		err = DOMGetNextObject (szObject, szFilter, szColType);
	}
	return TRUE;
}

BOOL CAListBoxFillViewColumns_x_Types (HWND hwndCtl, LPUCHAR DBName, LPUCHAR ViewName)
{
	int		idx, err;
	char	  szObject [MAXOBJECTNAME];
	char	  szFilter [MAXOBJECTNAME];
	char	  szColType[MAXOBJECTNAME];
	char*   buffColType;
	LPUCHAR aparents [MAXPLEVEL];

	ZEROINIT (aparents);
	ZEROINIT (szObject);
	ZEROINIT (szFilter);
	ZEROINIT (szColType);
	aparents[0] = DBName;
	aparents[1] = ViewName;

	err = DOMGetFirstObject
			(GetCurMdiNodeHandle (),
			OT_VIEWCOLUMN,
			2,
			aparents,
			FALSE,
			NULL,
			szObject,
			NULL,
			szColType);

	if (err != RES_SUCCESS)
		return FALSE;
	while (err == RES_SUCCESS)
	{
		idx = CAListBox_AddString (hwndCtl, szObject);
		/* //int nType = getint (szColTyp); */
		buffColType = ESL_AllocMem (MAXOBJECTNAME);
		if (!buffColType)
			return FALSE;
		memcpy (buffColType, szColType, 2 * STEPSMALLOBJ);
		CAListBox_SetItemData (hwndCtl, idx, buffColType);
		err = DOMGetNextObject (szObject, szFilter, szColType);
	}
	return TRUE;
}


void CAListBoxFillViewColumns  (HWND hwndCtl, LPUCHAR DBName, LPUCHAR ViewName)
{
	int		err;
	char	  szObject [MAXOBJECTNAME];
	char	  szFilter [MAXOBJECTNAME];
	LPUCHAR aparents [MAXPLEVEL];

	ZEROINIT (aparents);
	ZEROINIT (szObject);
	ZEROINIT (szFilter);
	aparents[0] = DBName;
	aparents[1] = ViewName;


	err = DOMGetFirstObject (GetCurMdiNodeHandle (), OT_VIEWCOLUMN, 2, aparents, GetSystemFlag (),
				NULL, szObject, NULL, NULL);

	while (err == RES_SUCCESS)
	{
		CAListBox_AddString (hwndCtl, szObject);
		err = DOMGetNextObject (szObject, szFilter, NULL);
	}
}



LPCHECKEDOBJECTS FindStringObject (const LPCHECKEDOBJECTS a, const char* szFind)
{
	LPCHECKEDOBJECTS ls;
	char* item;

	if ((a == NULL) || (szFind == NULL))
		return NULL;

	ls = a;
	while (ls)
	{
		item = (char *)ls->dbname;
		if (x_strcmp (item, szFind) == 0)
			return ls;
		else
			ls = ls->pnext;
	}
	return NULL;
}



LPCHECKEDOBJECTS DupList (const LPCHECKEDOBJECTS a)
{
	LPCHECKEDOBJECTS c = NULL;
	LPCHECKEDOBJECTS	obj, ls;
	char* item;
 
	if (a == NULL)
		return NULL;
	else
	{
		ls = a;
		while (ls)
		{
			item = (char *)ls->dbname;
			obj	= ESL_AllocMem (sizeof (CHECKEDOBJECTS));
			if (obj)
			{
				x_strcpy (obj->dbname, item);
				obj->pnext = NULL;
				c = AddCheckedObject (c, obj);
			}
			else
			{
				/* // */
				/* // Cannot allocate memory */
				/* // */
				c = FreeCheckedObjects	(c);
				return (NULL);
			}
			ls = ls->pnext;
		}
	}
	return(c);
}



LPCHECKEDOBJECTS Union (const LPCHECKEDOBJECTS a, const LPCHECKEDOBJECTS b)
{
	LPCHECKEDOBJECTS c = NULL;
	LPCHECKEDOBJECTS obj, ls;
	char* item;

	if (a == NULL)
	{
		c = DupList (b);
		return c;
	}

	c = DupList (a);
	ls = b;
	while (ls)
	{
		item = (char *)ls->dbname;
		if (!FindStringObject (a, item))
		{
			obj	= ESL_AllocMem (sizeof (CHECKEDOBJECTS));
			if (obj)
			{
				x_strcpy (obj->dbname, item);
				obj->pnext = NULL;
				c = AddCheckedObject (c, obj);
			}
			else
			{
				/* // */
				/* // Cannot allocate memory */
				/* // */
				c = FreeCheckedObjects	(c);
				return (NULL);
			}
		}
		ls = ls->pnext;
	}
	return c;
}



LPCHECKEDOBJECTS Intersection (const LPCHECKEDOBJECTS a, const LPCHECKEDOBJECTS b)
{
	LPCHECKEDOBJECTS c = NULL;
	LPCHECKEDOBJECTS obj, ls;
	char* item;

	if ((a == NULL) || (b == NULL))
	{
		return NULL;
	}
	
	ls = a;
	while (ls)
	{
		item = (char *)ls->dbname;
		if (FindStringObject (b, item) && !FindStringObject (c, item))
		{
			obj	= ESL_AllocMem (sizeof (CHECKEDOBJECTS));
			if (obj)
			{
				x_strcpy (obj->dbname, item);
				obj->pnext = NULL;
				c = AddCheckedObject (c, obj);
			}
			else
			{
				c = FreeCheckedObjects	(c);
				return (NULL);
			}
		}
		ls = ls->pnext;
	}

	ls = b;
	while (ls)
	{
		item = (char *)ls->dbname;
		if (FindStringObject (a, item) && !FindStringObject (c, item))
		{
			obj	= ESL_AllocMem (sizeof (CHECKEDOBJECTS));
			if (obj)
			{
				x_strcpy (obj->dbname, item);
				obj->pnext = NULL;
				c = AddCheckedObject (c, obj);
			}
			else
			{
				c = FreeCheckedObjects	(c);
				return (NULL);
			}
		}
		ls = ls->pnext;
	}
	
	return c;
}


LPCHECKEDOBJECTS Difference (const LPCHECKEDOBJECTS a, const LPCHECKEDOBJECTS b)
{
	LPCHECKEDOBJECTS c = NULL;
	LPCHECKEDOBJECTS obj, ls;
	char* item;

	if (a == NULL)
	{
		return NULL;
	}
	ls = a;
	while (ls)
	{
		item = (char *)ls->dbname;
		if (!FindStringObject (b, item))
		{
			obj	= ESL_AllocMem (sizeof (CHECKEDOBJECTS));
			if (obj)
			{
				x_strcpy (obj->dbname, item);
				obj->pnext = NULL;
				c = AddCheckedObject (c, obj);
			}
			else
			{
				c = FreeCheckedObjects	(c);
				return (NULL);
			}
		}
		ls = ls->pnext;
	}

	return c;
}

LPCHECKEDOBJECTS
AddCheckedObjectTail (LPCHECKEDOBJECTS lc, LPCHECKEDOBJECTS obj)
{
	LPCHECKEDOBJECTS ls = lc;

	if (obj)
	{
		obj->pnext = NULL;
		if (!lc)
			lc = obj;
		else
		{
			while (ls->pnext) ls = ls->pnext;
			ls->pnext = obj;
		}
	}
	return (lc);
}

char* ResourceString (UINT idString)
{
	int	  nByte;
	static UINT call_counter;
	static char R_String [MAX_STRINGARRAY][MAX_STRINGLENGTH];

	call_counter = call_counter % MAX_STRINGARRAY;
	nByte = LoadString (hResource, (UINT)idString, R_String [call_counter], MAX_STRINGLENGTH);
	if (nByte == 0)
		return NULL;
	else
	{
		int i = call_counter;
		call_counter++;
		return R_String [i];
	}
}



BOOL IsValidObjectChar(UINT ch)
/*
	Function:
	  Determines if the character is valid for an object name

	Parameters:
	  ch				- Character to test.

	Returns:
	  TRUE if character is valid.
*/
{
	//char szSpecial[] = "#@$_&*:,#\"=/<>()-%.+?;' |";
	BOOL bAlpha;
	BOOL bNumeric;
	BOOL bUnderscore;
	BOOL bSpecial;
#ifdef DOUBLEBYTE
	/* A touch naughty this, but this function will be called, and we need to
	 save last byte that came in */
	static BOOL bLastByteDBCSLead=0;
#endif

#ifdef DOUBLEBYTE
	if (IsCharAlpha((char)ch) || bLastByteDBCSLead)
	{
		bLastByteDBCSLead=FALSE;
		bAlpha=TRUE;
	} else bAlpha=bLastByteDBCSLead=IsDBCSLeadByte((char)ch);
#else
	bAlpha = IsCharAlpha((char)ch);
#endif
	bNumeric = (IsCharAlphaNumeric((char)ch) && !bAlpha);
	bUnderscore = (ch == '_');
	bSpecial = (_fstrchr(LIST_VALID_SPECIAL_CHARACTERS_4_OBJ_NAME, ch) != NULL) ? TRUE : FALSE;

	if (bAlpha || bNumeric || bSpecial || bUnderscore)
	  return TRUE;

	return FALSE;
}



void SelectAllCALBItems(HWND hwnd, BOOL bSelect, int nIdx)
{
	int nCount = CAListBox_GetCount(hwnd);
	int i;

	for (i = 0; i < nCount; i++)
	{
	  if (i == nIdx)
		 continue;

	  CAListBox_SetSel(hwnd, bSelect, i);
	}
}




LPSTACKOBJECT StackObject_TOP  (LPSTACKOBJECT L)
{
	if (L)
		return L;
	else
		return NULL;
}

LPSTACKOBJECT	StackObject_POP	(LPSTACKOBJECT L)
{
	LPSTACKOBJECT tmp;

	if (L)
	{
		tmp = L;
		L = L->next;
		ESL_FreeMem (tmp);  
	}
	return L;
}

LPSTACKOBJECT	StackObject_PUSH (LPSTACKOBJECT L, LPSTACKOBJECT S)
{
	if (S)
	{
	  S->next = L;
	  L = S;
	}
	return (L);
}

LPSTACKOBJECT StackObject_INIT (UINT ContextNum)
{
	LPSTACKOBJECT sto;
	BOOL		 b52;

	/* // Emb 18/10/95 : we don't want error msg for help stack management */
	if (gMemErrFlag == MEMERR_STANDARD) {
	 b52 = TRUE;
	 gMemErrFlag = MEMERR_NOMESSAGE;
	}
	else
	 b52 = FALSE;
	sto = ESL_AllocMem (sizeof (STACKOBJECT));
	if (b52)
	 gMemErrFlag = MEMERR_STANDARD;
	if (sto)
	{
		sto->next = NULL;
		sto->Id	 = ContextNum;
		return sto;
	}
	else
	{
		/* // Masqued Emb 18/10/95, see comment up */
		/* //ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR); */
		return NULL;
	}
}


LPSTACKOBJECT StackObject_FREE (LPSTACKOBJECT L)
{
	LPSTACKOBJECT tmp;

	while (L)
	{
	  tmp = L;
	  L	= L->next; 
	  ESL_FreeMem (tmp);  
	}
	return NULL;
}



/* ---------------------------------------------------- NoLeadingEndingSpace -*/
/*                                                                            */
/* Eliminate the leading and ending space of a string.                        */
/*                                                                            */
/*  1) aString      : The string you want to remove leading and ending        */
/*                   space out of it.                                         */
/*                                                                            */
/* ---------------------------------------------------------------------------*/
void
NoLeadingEndingSpace (char* aString)
{
	int	i, len;
	char temp [max(MAXOBJECTNAME,_MAX_PATH)];
	char*pos;

	if (aString)
	{
		len = x_strlen (aString);
		for (i=0; i<len; i+=CMbytecnt(aString+i))
		{
			if (aString [i] != ' ') break;
		}
		pos = &aString [i];
		x_strcpy (temp, pos);
		suppspace (temp);
		x_strcpy (aString, temp);
	}
}

#ifdef MAINWIN
/* moved from port.h */
unsigned long GetTextExtent(HDC hdc, LPSTR lpstr, size_t cb)
{
	SIZE size;
	GetTextExtentPoint32(hdc, lpstr, cb, &size);
	return (DWORD)MAKELONG(size.cx, size.cy);
}

#endif

//
// Star management
//
// correspondence between product type and string to be generated for front-end
void ProdNameFromProdType(DWORD dwtype, char *lpSzName)
{
	lpSzName[0] = '\0';
	switch (dwtype) {
	case PROD_INGRES:		x_strcpy(lpSzName, "ingres"); 	  break;
	case PROD_VISION:		x_strcpy(lpSzName, "vision"); 	  break;
	case PROD_W4GL: 		x_strcpy(lpSzName, "windows_4gl");  break;
	case PROD_INGRES_DBD:	x_strcpy(lpSzName, "ingres/dbd");   break;
	case PROD_NOFECLIENTS:	x_strcpy(lpSzName, "nofeclients");  break;
	}
}


/*----------------------------------------------------------- ConcateStrings -*/
/* The function that concates strings (ioBuffer, aString, ..)                 */
/*                                                                            */
/* Parameter:                                                                 */
/*     1) ioBuffer: In Out Buffer                                             */
/*     2) aString : String to be concated.                                    */
/*     3) ...     : Ellipse for other strings...with NULL terminated (char*)0 */
/* Return:                                                                    */
/*     return  1 if Successful.                                               */   
/*     return  0 otherwise.                                                   */   
/*----------------------------------------------------------------------------*/
int
ConcateStrings (LPSTR* ioBuffer, LPSTR aString, ...)
{
   int     size;
   va_list Marker;
   LPSTR  strArg;

   if (!aString)
       return 0;

   if (!(*ioBuffer))
   {
       *ioBuffer = (LPSTR)ESL_AllocMem (lstrlen (aString) +1);
   }
   else
   {
       size      = lstrlen  ((LPSTR)(*ioBuffer)) + lstrlen (aString) +1;
       *ioBuffer = (LPSTR)ESL_ReAllocMem ((LPSTR)(*ioBuffer), size, 0);
   }

   if (!*ioBuffer)
       return 0;
   else
       lstrcat (*ioBuffer, aString);

   va_start (Marker, aString);
   while (strArg = va_arg (Marker, LPSTR))
   {
       size      = lstrlen  (*ioBuffer) + lstrlen (strArg) +1;
       *ioBuffer = (LPSTR)ESL_ReAllocMem (*ioBuffer, size, 0);
       if (!*ioBuffer)
           return 0;
       else
           lstrcat (*ioBuffer, strArg);
   }
   va_end (Marker);
   return 1;
}
