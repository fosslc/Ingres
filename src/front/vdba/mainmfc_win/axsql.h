#pragma once
#include "font.h"

// Machine generated IDispatch wrapper class(es) created by Microsoft Visual C++

// NOTE: Do not modify the contents of this file.  If this class is regenerated by
//  Microsoft Visual C++, your modifications will be overwritten.

/////////////////////////////////////////////////////////////////////////////
// CuSqlquery wrapper class

class CuSqlquery : public CWnd
{
protected:
	DECLARE_DYNCREATE(CuSqlquery)
public:
	CLSID const& GetClsid()
	{
		static CLSID const clsid
			= { 0x634C383D, 0xA069, 0x11D5, { 0x87, 0x69, 0x0, 0xC0, 0x4F, 0x1F, 0x75, 0x4A } };
		return clsid;
	}
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
						const RECT& rect, CWnd* pParentWnd, UINT nID, 
						CCreateContext* pContext = NULL)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID); 
	}

    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, 
				UINT nID, CFile* pPersist = NULL, BOOL bStorage = FALSE,
				BSTR bstrLicKey = NULL)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
		pPersist, bStorage, bstrLicKey); 
	}

// Attributes
public:


// Operations
public:

// _DSqlquery

// Functions
//

	BOOL Initiate(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszFlags)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR VTS_BSTR ;
		InvokeHelper(0x14, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, lpszNode, lpszServer, lpszFlags);
		return result;
	}
	void SetDatabase(LPCTSTR lpszDatabase)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x15, DISPATCH_METHOD, VT_EMPTY, NULL, parms, lpszDatabase);
	}
	void Clear()
	{
		InvokeHelper(0x16, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Open()
	{
		InvokeHelper(0x17, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Save()
	{
		InvokeHelper(0x18, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void SqlAssistant()
	{
		InvokeHelper(0x19, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Run()
	{
		InvokeHelper(0x1a, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Qep()
	{
		InvokeHelper(0x1b, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Xml()
	{
		InvokeHelper(0x1c, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	BOOL IsRunEnable()
	{
		BOOL result;
		InvokeHelper(0x1d, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL IsQepEnable()
	{
		BOOL result;
		InvokeHelper(0x1e, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL IsXmlEnable()
	{
		BOOL result;
		InvokeHelper(0x1f, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void Print()
	{
		InvokeHelper(0x20, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	BOOL IsPrintEnable()
	{
		BOOL result;
		InvokeHelper(0x21, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void EnableTrace()
	{
		InvokeHelper(0x22, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	BOOL IsClearEnable()
	{
		BOOL result;
		InvokeHelper(0x23, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL IsTraceEnable()
	{
		BOOL result;
		InvokeHelper(0x24, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL IsSaveAsEnable()
	{
		BOOL result;
		InvokeHelper(0x25, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL IsOpenEnable()
	{
		BOOL result;
		InvokeHelper(0x26, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void PrintPreview()
	{
		InvokeHelper(0x27, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	BOOL IsPrintPreviewEnable()
	{
		BOOL result;
		InvokeHelper(0x28, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	SCODE Storing(LPUNKNOWN * ppStream)
	{
		SCODE result;
		static BYTE parms[] = VTS_PUNKNOWN ;
		InvokeHelper(0x29, DISPATCH_METHOD, VT_ERROR, (void*)&result, parms, ppStream);
		return result;
	}
	SCODE Loading(LPUNKNOWN pStream)
	{
		SCODE result;
		static BYTE parms[] = VTS_UNKNOWN ;
		InvokeHelper(0x2a, DISPATCH_METHOD, VT_ERROR, (void*)&result, parms, pStream);
		return result;
	}
	BOOL IsUsedTracePage()
	{
		BOOL result;
		InvokeHelper(0x2b, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void SetIniFleName(LPCTSTR lpszFileIni)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x2c, DISPATCH_METHOD, VT_EMPTY, NULL, parms, lpszFileIni);
	}
	void SetSessionDescription(LPCTSTR lpszSessionDescription)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x2d, DISPATCH_METHOD, VT_EMPTY, NULL, parms, lpszSessionDescription);
	}
	void SetSessionStart(long nStart)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x2e, DISPATCH_METHOD, VT_EMPTY, NULL, parms, nStart);
	}
	void InvalidateCursor()
	{
		InvokeHelper(0x2f, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Commit(short nCommit)
	{
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x30, DISPATCH_METHOD, VT_EMPTY, NULL, parms, nCommit);
	}
	BOOL IsCommitEnable()
	{
		BOOL result;
		InvokeHelper(0x31, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void CreateSelectResultPage(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszDatabase, LPCTSTR lpszStatement)
	{
		static BYTE parms[] = VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR ;
		InvokeHelper(0x32, DISPATCH_METHOD, VT_EMPTY, NULL, parms, lpszNode, lpszServer, lpszUser, lpszDatabase, lpszStatement);
	}
	void SetDatabaseStar(LPCTSTR lpszDatabase, long nFlag)
	{
		static BYTE parms[] = VTS_BSTR VTS_I4 ;
		InvokeHelper(0x33, DISPATCH_METHOD, VT_EMPTY, NULL, parms, lpszDatabase, nFlag);
	}
	void CreateSelectResultPage4Star(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszDatabase, long nDbFlag, LPCTSTR lpszStatement)
	{
		static BYTE parms[] = VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4 VTS_BSTR ;
		InvokeHelper(0x34, DISPATCH_METHOD, VT_EMPTY, NULL, parms, lpszNode, lpszServer, lpszUser, lpszDatabase, nDbFlag, lpszStatement);
	}
	BOOL Initiate2(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszOption)
	{
		BOOL result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR ;
		InvokeHelper(0x35, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, lpszNode, lpszServer, lpszUser, lpszOption);
		return result;
	}
	void SetConnectParamInfo(long nSessionVersion)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x36, DISPATCH_METHOD, VT_EMPTY, NULL, parms, nSessionVersion);
	}
	long GetConnectParamInfo()
	{
		long result;
		InvokeHelper(0x37, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long ConnectIfNeeded(short nDisplayError)
	{
		long result;
		static BYTE parms[] = VTS_I2 ;
		InvokeHelper(0x38, DISPATCH_METHOD, VT_I4, (void*)&result, parms, nDisplayError);
		return result;
	}
	BOOL GetSessionAutocommitState()
	{
		BOOL result;
		InvokeHelper(0x39, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL GetSessionCommitState()
	{
		BOOL result;
		InvokeHelper(0x3a, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL GetSessionReadLockState()
	{
		BOOL result;
		InvokeHelper(0x3b, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void SetHelpFile(LPCTSTR lpszFileWithoutPath)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x3c, DISPATCH_METHOD, VT_EMPTY, NULL, parms, lpszFileWithoutPath);
	}
	void SetErrorFileName(LPCTSTR lpszErrorFileName)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x3d, DISPATCH_METHOD, VT_EMPTY, NULL, parms, lpszErrorFileName);
	}
	void AboutBox()
	{
		InvokeHelper(DISPID_ABOUTBOX, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	short GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase)
	{
		short result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR ;
		InvokeHelper(0x3e, DISPATCH_METHOD, VT_I2, (void*)&result, parms, lpszNode, lpszDatabase);
		return result;
	}

// Properties
//

COleFont GetFont()
{
	LPDISPATCH result;
	GetProperty(DISPID_FONT, VT_DISPATCH, (void*)&result);
	return COleFont(result);
}
void SetFont(LPDISPATCH propVal)
{
	SetProperty(DISPID_FONT, VT_DISPATCH, propVal);
}
BOOL GetShowGrid()
{
	BOOL result;
	GetProperty(0x1, VT_BOOL, (void*)&result);
	return result;
}
void SetShowGrid(BOOL propVal)
{
	SetProperty(0x1, VT_BOOL, propVal);
}
BOOL GetAutoCommit()
{
	BOOL result;
	GetProperty(0x2, VT_BOOL, (void*)&result);
	return result;
}
void SetAutoCommit(BOOL propVal)
{
	SetProperty(0x2, VT_BOOL, propVal);
}
BOOL GetReadLock()
{
	BOOL result;
	GetProperty(0x3, VT_BOOL, (void*)&result);
	return result;
}
void SetReadLock(BOOL propVal)
{
	SetProperty(0x3, VT_BOOL, propVal);
}
long GetTimeOut()
{
	long result;
	GetProperty(0x4, VT_I4, (void*)&result);
	return result;
}
void SetTimeOut(long propVal)
{
	SetProperty(0x4, VT_I4, propVal);
}
long GetSelectLimit()
{
	long result;
	GetProperty(0x5, VT_I4, (void*)&result);
	return result;
}
void SetSelectLimit(long propVal)
{
	SetProperty(0x5, VT_I4, propVal);
}
long GetSelectMode()
{
	long result;
	GetProperty(0x6, VT_I4, (void*)&result);
	return result;
}
void SetSelectMode(long propVal)
{
	SetProperty(0x6, VT_I4, propVal);
}
long GetMaxTab()
{
	long result;
	GetProperty(0x7, VT_I4, (void*)&result);
	return result;
}
void SetMaxTab(long propVal)
{
	SetProperty(0x7, VT_I4, propVal);
}
long GetMaxTraceSize()
{
	long result;
	GetProperty(0x8, VT_I4, (void*)&result);
	return result;
}
void SetMaxTraceSize(long propVal)
{
	SetProperty(0x8, VT_I4, propVal);
}
BOOL GetShowNonSelectTab()
{
	BOOL result;
	GetProperty(0x9, VT_BOOL, (void*)&result);
	return result;
}
void SetShowNonSelectTab(BOOL propVal)
{
	SetProperty(0x9, VT_BOOL, propVal);
}
BOOL GetTraceTabActivated()
{
	BOOL result;
	GetProperty(0xa, VT_BOOL, (void*)&result);
	return result;
}
void SetTraceTabActivated(BOOL propVal)
{
	SetProperty(0xa, VT_BOOL, propVal);
}
BOOL GetTraceTabToTop()
{
	BOOL result;
	GetProperty(0xb, VT_BOOL, (void*)&result);
	return result;
}
void SetTraceTabToTop(BOOL propVal)
{
	SetProperty(0xb, VT_BOOL, propVal);
}
long GetF4Width()
{
	long result;
	GetProperty(0xc, VT_I4, (void*)&result);
	return result;
}
void SetF4Width(long propVal)
{
	SetProperty(0xc, VT_I4, propVal);
}
long GetF4Scale()
{
	long result;
	GetProperty(0xd, VT_I4, (void*)&result);
	return result;
}
void SetF4Scale(long propVal)
{
	SetProperty(0xd, VT_I4, propVal);
}
BOOL GetF4Exponential()
{
	BOOL result;
	GetProperty(0xe, VT_BOOL, (void*)&result);
	return result;
}
void SetF4Exponential(BOOL propVal)
{
	SetProperty(0xe, VT_BOOL, propVal);
}
long GetF8Width()
{
	long result;
	GetProperty(0xf, VT_I4, (void*)&result);
	return result;
}
void SetF8Width(long propVal)
{
	SetProperty(0xf, VT_I4, propVal);
}
long GetF8Scale()
{
	long result;
	GetProperty(0x10, VT_I4, (void*)&result);
	return result;
}
void SetF8Scale(long propVal)
{
	SetProperty(0x10, VT_I4, propVal);
}
BOOL GetF8Exponential()
{
	BOOL result;
	GetProperty(0x11, VT_BOOL, (void*)&result);
	return result;
}
void SetF8Exponential(BOOL propVal)
{
	SetProperty(0x11, VT_BOOL, propVal);
}
long GetQepDisplayMode()
{
	long result;
	GetProperty(0x12, VT_I4, (void*)&result);
	return result;
}
void SetQepDisplayMode(long propVal)
{
	SetProperty(0x12, VT_I4, propVal);
}
long GetXmlDisplayMode()
{
	long result;
	GetProperty(0x13, VT_I4, (void*)&result);
	return result;
}
void SetXmlDisplayMode(long propVal)
{
	SetProperty(0x13, VT_I4, propVal);
}


};
