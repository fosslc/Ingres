/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlprofi.h: interface for the CaProfile class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Profile object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLPROFILE_HEADER)
#define DMLPROFILE_HEADER
#include "dmlbase.h"
#include "dmloburp.h"

//
// Object: CaProfile 
// ************************************************************************************************
class CaProfile: public CaDBObject
{
	DECLARE_SERIAL (CaProfile)
public:
	CaProfile(): CaDBObject(){SetObjectID(OBT_PROFILE);}
	CaProfile(LPCTSTR lpszName): CaDBObject(lpszName, _T("")){SetObjectID(OBT_PROFILE);}
	CaProfile (const CaProfile& c);
	CaProfile operator =(const CaProfile& c);

	virtual ~CaProfile(){};
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaProfile* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

protected:
	void Copy (const CaProfile& c);

};

//
// Object: CaProfileDetail 
// ************************************************************************************************
class CaProfileDetail: public CaProfile, public CaURPPrivileges
{
	DECLARE_SERIAL (CaProfileDetail)
public:
	CaProfileDetail(): CaProfile(), CaURPPrivileges(){Init();}
	CaProfileDetail(LPCTSTR lpszName): CaProfile(lpszName), CaURPPrivileges(){Init();}
	CaProfileDetail (const CaProfileDetail& c);
	CaProfileDetail operator =(const CaProfileDetail& c);

	virtual ~CaProfileDetail(){};
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaProfileDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	//
	// GET:
	CString GetDefaultGroup(){return m_strDefaultGroup;}
	CString GetExpireDate(){return m_strExpireDate;}
	//
	// SET:
	void SetDefaultGroup(LPCTSTR lpszText){m_strDefaultGroup = lpszText;}
	void SetExpireDate(LPCTSTR lpszText){m_strExpireDate = lpszText;}

	void Init()
	{
		m_strDefaultGroup = _T("");
		m_strExpireDate = _T("");
	}
	CaURPPrivileges* GetURPPrivileges(){return (CaURPPrivileges*)this;}

protected:
	void Copy (const CaProfileDetail& c);

	CString m_strDefaultGroup;
	CString m_strExpireDate;

};

#endif // DMLPROFILE_HEADER