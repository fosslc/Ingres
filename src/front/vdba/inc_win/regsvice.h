/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : regsvice.h, header file
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Check the Service (Installation, Running, Set Password)
**
** History:
**
** xx-May-1999 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**    And integrate change 01-Feb-2000 (noifr01) (SIR 100238) added proptotype for 
**    HasAdmistratorRights()  function
**/


#if !defined (REGISTRY_AND_SERVICE)
#define REGISTRY_AND_SERVICE
#include "usrexcep.h"

class CeServiceException: public CeException
{
public:
	CeServiceException(): CeException() {}
	CeServiceException(LONG lErrorCode): CeException(lErrorCode){}
	~CeServiceException(){};
};


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


//
// The following functions may throw CeExpcetion with error code:
#define E_SERVICE_CONNECT 100000 // Failed to connect to the Service Control Manager
#define E_SERVICE_STATUS  100001 // Cannot query the service status information
#define E_SERVICE_NAME    100002 // Cannot get the service's display name
#define E_SERVICE_CONFIG  100003 // Cannot change the service configuration, \nfail to set password

BOOL IsServiceInstalled (LPCTSTR lpszServiceName);
BOOL IsServiceRunning   (LPCTSTR lpszServiceName);
BOOL SetServicePassword (LPCTSTR lpszServiceName, const CaServiceConfig& cf);
BOOL HasAdmistratorRights();
BOOL FillServiceConfig  (CaServiceConfig& cf);
BOOL RunService (LPCTSTR lpszServiceName, DWORD& dwError);
BOOL StopService (LPCTSTR lpszServiceName, DWORD& dwError);
CString ServiceErrorMessage (DWORD dwError);

#endif // REGISTRY_AND_SERVICE