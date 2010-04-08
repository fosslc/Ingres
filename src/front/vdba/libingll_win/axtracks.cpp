/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : axtracks.cpp: implementation file.
**    Project  : AxtiveX Component
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Keep tracks of the created controls in the ocx.
**
** History:
**
** 11-Mar-2002 (uk$so01)
**    Created
**/

#include "stdafx.h"
#include "axtracks.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CaActiveXControlEnum::CaActiveXControlEnum()
{
	m_hMutex = CreateMutex(NULL, FALSE, NULL);
}

CaActiveXControlEnum::~CaActiveXControlEnum()
{
	if (m_hMutex)
		CloseHandle(m_hMutex);
	m_arrayHwnd.RemoveAll();
}


void CaActiveXControlEnum::Add (HWND hWnd)
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, 1000*15))
	{
		m_arrayHwnd.Add(hWnd);
		ReleaseMutex(m_hMutex);
	}
}

void CaActiveXControlEnum::Remove(HWND hWnd)
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, 1000*15))
	{
		int nSize = m_arrayHwnd.GetSize();
		for (int i=0; i<nSize; i++)
		{
			if (m_arrayHwnd[i] == hWnd)
			{
				m_arrayHwnd.RemoveAt(i);
				break;
			}
		}
		ReleaseMutex(m_hMutex);
	}
}

void CaActiveXControlEnum::Notify(HWND hWndIgnore, WPARAM w, LPARAM l)
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, 1000*15))
	{
		HWND hWnd = NULL;
		int nSize = m_arrayHwnd.GetSize();
		for (int i=0; i<nSize; i++)
		{
			hWnd = m_arrayHwnd[i];
			if (hWndIgnore && (hWnd == hWndIgnore))
				continue;
			if (IsWindow (hWnd))
				::PostMessage (hWnd, WMUSRMSG_UPDATEDATA, w, l);
		}
		ReleaseMutex(m_hMutex);
	}
}


