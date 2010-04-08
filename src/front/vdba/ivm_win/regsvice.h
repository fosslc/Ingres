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
/* History after 01-01-2000
**
** 01-Feb-2000 (noifr01)
**  (SIR 100238) added proptotype for HasAdmistratorRights()  function
** 28-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
*/
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


BOOL IVM_IsServiceInstalled (LPCTSTR lpszServiceName);
BOOL IVM_IsServiceRunning   (LPCTSTR lpszServiceName);
BOOL IVM_SetServicePassword (LPCTSTR lpszServiceName, const CaServiceConfig& cf);
BOOL IVM_HasAdmistratorRights();
BOOL IVM_FillServiceConfig  (CaServiceConfig& cf);
BOOL IVM_RunService (LPCTSTR lpszServiceName, DWORD& dwError);
BOOL IVM_StopService (LPCTSTR lpszServiceName, DWORD& dwError);
CString IVM_ServiceErrorMessage (DWORD dwError);

#endif