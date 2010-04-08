/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlgroup.h: interface for the CaGroup class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Group object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLGROUP_HEADER)
#define DMLGROUP_HEADER
#include "dmlbase.h"

//
// Object: CaGroup 
// ************************************************************************************************
class CaGroup: public CaDBObject
{
	DECLARE_SERIAL (CaGroup)
public:
	CaGroup(): CaDBObject(){SetObjectID(OBT_GROUP);}
	CaGroup(LPCTSTR lpszName): CaDBObject(lpszName, _T("")){SetObjectID(OBT_GROUP);}
	CaGroup(const CaGroup& c);
	CaGroup operator =(const CaGroup& c);

	BOOL Matched (CaGroup* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	virtual ~CaGroup(){};
	virtual void Serialize (CArchive& ar);
protected:
	void Copy(const CaGroup& c);

};

//
// Object: CaGroupDetail 
// ************************************************************************************************
class CaGroupDetail: public CaGroup
{
	DECLARE_SERIAL (CaGroupDetail)
public:
	CaGroupDetail(): CaGroup(){}
	CaGroupDetail(LPCTSTR lpszName): CaGroup(lpszName){}
	CaGroupDetail(const CaGroupDetail& c);
	CaGroupDetail operator =(const CaGroupDetail& c);

	CStringList& GetListMember(){return m_listMember;}
	BOOL Matched (CaGroupDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	virtual ~CaGroupDetail(){};
	virtual void Serialize (CArchive& ar);
protected:
	void Copy(const CaGroupDetail& c);

	CStringList m_listMember;

};

#endif // DMLGROUP_HEADER