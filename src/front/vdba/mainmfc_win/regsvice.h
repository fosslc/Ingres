/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : regsvice.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Check the Service (Installation, Running, Set Password)               //
****************************************************************************************/
#if !defined (REGISTRY_AND_SERVICE)
#define REGISTRY_AND_SERVICE

class CaServiceConfig: public CObject
{
public:
	CaServiceConfig(): m_strAccount(_T("")), m_strPassword(_T("")){};
	CaServiceConfig(LPCTSTR lpszAccount, LPCTSTR lpszPassword)
	{
		m_strAccount = lpszAccount?  lpszAccount:  _T("");
		m_strPassword= lpszPassword? lpszPassword: _T("");
	}
	virtual ~CaServiceConfig(){}

	CString m_strAccount;
	CString m_strPassword;
};


BOOL IsServiceInstalled (LPCTSTR lpszServiceName);
BOOL IsServiceRunning   (LPCTSTR lpszServiceName);
BOOL SetServicePassword (LPCTSTR lpszServiceName, const CaServiceConfig& cf);
BOOL FillServiceConfig  (CaServiceConfig& cf);
BOOL RunService (LPCTSTR lpszServiceName, DWORD& dwError);

#endif