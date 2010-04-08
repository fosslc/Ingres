/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmprdta.h, Header File.
**
**
**    Project  : Visual DBA
**    Author   : UK Sotheavut
**
**    Purpose  : The implementation of serialization of IPM Page (modeless dialog)
**
** History:
** xx-Apr-1997 (uk$so01)
**    created.
** 20-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
*/

#ifndef IPMPRDTA_HEADER
#define IPMPRDTA_HEADER
#include "statfrm.h"

extern "C"
{
#include "dbaset.h"
}


class CuIpmPropertyData: public CObject
{
	DECLARE_SERIAL (CuIpmPropertyData)
public:
	CuIpmPropertyData();
	CuIpmPropertyData(LPCTSTR lpszClassName);

	virtual ~CuIpmPropertyData(){};

	virtual void NotifyLoad (CWnd* pDlg);  
		// NotifyLoad() Send message WM_USER_IPMPAGE_LOADING to the Window 'pDlg'
		// WPARAM: The class name of the Sender.
		// LPARAM: The data structure depending on the class name of the Sender.
	virtual void Serialize (CArchive& ar);
	CString& GetClassName() {return m_strClassName;}
protected:
	CString m_strClassName;
};


//
// DETAIL: Location
class CuIpmPropertyDataDetailLocation: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailLocation)
public:
	CuIpmPropertyDataDetailLocation();
	virtual ~CuIpmPropertyDataDetailLocation(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);

	CaPieInfoData* m_pPieInfo;  // Do not delete this member.
};


//
// PAGE: Location (Page 2, Page1 is call Detail)
class CuIpmPropertyDataPageLocation: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageLocation)
public:
	CuIpmPropertyDataPageLocation();
	virtual ~CuIpmPropertyDataPageLocation(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	CaPieInfoData* m_pPieInfo;  // Do not delete this member.
};



//
// PAGE: Location Usage (Page 3)
class CuIpmPropertyDataPageLocationUsage: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageLocationUsage)
public:
	CuIpmPropertyDataPageLocationUsage();
	CuIpmPropertyDataPageLocationUsage(LPLOCATIONPARAMS pData);
	virtual ~CuIpmPropertyDataPageLocationUsage(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);

	LOCATIONPARAMS m_dlgData;
};


//
// SUMMARY: Locations
class CuIpmPropertyDataSummaryLocations: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataSummaryLocations)
public:
	CuIpmPropertyDataSummaryLocations();
	virtual ~CuIpmPropertyDataSummaryLocations(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);

	UINT m_nBarWidth;
	UINT m_nUnit;
	CaBarInfoData* m_pDiskInfo;    // Do not delete this member.
};

#endif
