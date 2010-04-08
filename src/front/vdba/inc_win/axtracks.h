/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : axtracks.h: header file.
**    Project  : AxtiveX Component
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Keep tracks of the created controls in the ocx.
**
** History:
**
** 11-Mar-2002 (uk$so01)
**    Created
**/

#if !defined (ACTIVEX_CONTROLTRACKER_MANAGEMENT_HEADER)
#define ACTIVEX_CONTROLTRACKER_MANAGEMENT_HEADER

class CaActiveXControlEnum: public CObject
{
public:
	CaActiveXControlEnum();
	virtual ~CaActiveXControlEnum();

	void Add (HWND hWnd);
	void Remove(HWND hWnd);
	//
	// hWndIgnore can be NULL. If not NULL, the window that will not be notified.
	void Notify(HWND hWndIgnore, WPARAM w, LPARAM l);

protected:
	CArray< HWND, HWND > m_arrayHwnd;
private:
	HANDLE m_hMutex;
};


#endif // ACTIVEX_CONTROLTRACKER_MANAGEMENT_HEADER
