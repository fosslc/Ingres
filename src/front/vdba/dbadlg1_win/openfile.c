/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : openfile.c
//
//    Dialog box for opening a file
//
********************************************************************/

#include "dll.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include <commdlg.h>

typedef struct tagFILTERS
{
	int nString;
	int nFilter;
} FILTERS, FAR * LPFILTERS;

static FILTERS filterTypes[] =
{
	IDS_CFGFILES,	IDS_CFGFILES_FILTER,
	IDS_ALLFILES,	IDS_ALLFILES_FILTER,
	-1		// terminator
};

static void CreateFilterString (LPSTR lpszFilter, int cMaxBytes);

BOOL WINAPI __export DlgOpenFile (HWND hwndOwner, LPTOOLSOPENFILE lpToolsOpen)
/*
	Function:
		Shows the File Open dialog for configuration files.

	Paramters:
		hwndOwner		- Handle of the parent window for the dialog.
		lpToolsOpen		- Points to structure containing information used to 
							- initialise the dialog. The dialog uses the same
							- structure to return the result of the dialog.

	Returns:
		TRUE if successful.
*/
{
	OPENFILENAME ofn;
	char szFilter[256];
  BOOL bResult;

	if (!IsWindow(hwndOwner) || !lpToolsOpen)
	{
		ASSERT(NULL);
		return FALSE;
	}

	CreateFilterString(szFilter, sizeof(szFilter));

	ZEROINIT(ofn);

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwndOwner;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = lpToolsOpen->szFile;
	ofn.nMaxFile = sizeof(lpToolsOpen->szFile);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

  // help number defined in dlgres.h head section
  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_OPENWORKSPACEBOX));
  bResult = GetOpenFileName(&ofn);
  lpHelpStack = StackObject_POP (lpHelpStack);

  return bResult;
}

static void CreateFilterString (LPSTR lpszFilter, int cMaxBytes)
{
	int nFilter = 0;
	int nLen;

	while (filterTypes[nFilter].nString != -1)
	{
		if ((nLen = LoadString(hResource, filterTypes[nFilter].nString, lpszFilter, cMaxBytes)) != 0)
		{
			lpszFilter += nLen + 1;
			cMaxBytes -= (nLen + 1);

			if ((nLen = LoadString(hResource, filterTypes[nFilter].nFilter, lpszFilter, cMaxBytes)) != 0)
			{
				lpszFilter += nLen + 1;
				cMaxBytes -= (nLen + 1);
			}
		}
		nFilter++;
	}
	if (cMaxBytes > 0)
		*lpszFilter = (char)NULL;
}
