/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmloburp.h, Header File 
**    Project  : COM Server
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data manipulation (User, Role, Profile)
**
** History:
**
** 26-Oct-1999 (uk$so01)
**    Created.
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLOBURP_HEADER)
#define DMLOBURP_HEADER


enum UserRoleProfleFlags
{
	URP_CREATE_DATABASE     =   1,
	URP_TRACE               = URP_CREATE_DATABASE   << 1,
	URP_SECURITY            = URP_TRACE             << 1,
	URP_OPERATOR            = URP_SECURITY          << 1,
	URP_MAINTAIN_LOCATION   = URP_OPERATOR          << 1,
	URP_AUDITOR             = URP_MAINTAIN_LOCATION << 1,
	URP_MAINTAIN_AUDIT      = URP_AUDITOR           << 1,
	URP_MAINTAIN_USER       = URP_MAINTAIN_AUDIT    << 1,
	//
	// MAC Privileges
	URP_WRITEDOWN           = URP_MAINTAIN_USER     << 1,
	URP_WRITEFIXED          = URP_WRITEDOWN         << 1,
	URP_WRITEUP             = URP_WRITEFIXED        << 1,
	URP_INSERTDOWN          = URP_WRITEUP           << 1,
	URP_INSERTUP            = URP_INSERTDOWN        << 1,
	URP_SESSIONSECURITYLABEL= URP_INSERTUP          << 1
};

//
// Base for classes  CaUserDetail, CaRoleDetail, CaProfileDetail
class CaURPPrivileges
{
public:
	CaURPPrivileges();
	virtual ~CaURPPrivileges(){}
	void Init();
	virtual void Serialize (CArchive& ar);

	//
	// GET
	BOOL IsPrivilege(UserRoleProfleFlags nFlag);
	BOOL IsDefaultPrivilege(UserRoleProfleFlags nFlag);
	BOOL IsSecurityAuditAllEvent(){return m_bAllEvent;}
	BOOL IsSecurityAuditQueryText(){return m_bQueryText;}
	CString GetLimitSecurityLabel(){return m_strLimitSecurityLabel;}
	UINT GetPrivelegeFlag(){return m_nPriveFlag;}
	UINT GetDefaultPrivelegeFlag(){return m_nDefaultPriveFlag;}
	//
	// SET:
	void SetPrivilege(UserRoleProfleFlags nFlag);
	void SetDefaultPrivilege(UserRoleProfleFlags nFlag);
	void SetLimitSecurityLabel(LPCTSTR lpszText){m_strLimitSecurityLabel = lpszText;}
	void SetSecurityAuditAllEvent(BOOL bSet){m_bAllEvent=bSet;}
	void SetSecurityAuditQueryText(BOOL bSet){m_bQueryText=bSet;}
	//
	// DELETE:
	void DelPrivilege(UserRoleProfleFlags nFlag);
	void DelDefaultPrivilege(UserRoleProfleFlags nFlag);

protected:
	//
	// The or "|" operation. It can be one or more of the enumeration
	// UserRoleProfleFlags above:
	UINT m_nPriveFlag;
	UINT m_nDefaultPriveFlag;

	// Security Audit (all_event, default_event, query_text):
	BOOL m_bAllEvent;
	BOOL m_bQueryText;
	// Limit Security Label:
	CString m_strLimitSecurityLabel;

};


#endif // DMLOBURP_HEADER


