/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : vsddml.cpp, implementation File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Comparing the properties of objects. 
** 
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 27-Feb-2003 (schph01)
**    SIR #109220 add comparison between location and add
**    VSD_CompareTableConsistent() function
** 22-Apr-2003 (schph01)
**    SIR 107523 Add Sequence Object
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 16-Jan-2004 (schph01)
**    SIR #111462, Utilize the new prototype of SetType(UINT id) method for
**    load the string resource.
** 17-May-2004 (uk$so01)
**    SIR #109220,(Eliminate the compiler warnings)
** 30-Dec-2004 (lazro01)
**	  Bug 113688, Ignore unwanted whitespaces while comparing schema of 
**	  database objects.
*/

#include "stdafx.h" 
#include "vsddml.h"
#include "usrexcep.h"
#include "compfile.h"
#include "resource.h"

#ifdef __cplusplus
extern "C"
{
#endif

# include <compat.h>
# include <cm.h>

#ifdef __cplusplus
}
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void VSD_HasDiscadItem(CtrfItemData* pItem, BOOL& bHas, BOOL bLookupInSubFolderOnly)
{
	if (!bLookupInSubFolderOnly)
	{
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
		if (pVsdItemData->IsDiscarded())
		{
			bHas = TRUE;
			return;
		}
	}

	CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
	POSITION pos = lo.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* pIt = lo.GetNext(pos);
		VSD_HasDiscadItem(pIt, bHas, FALSE);
		if (bHas)
			return;
	}
}

static CString strBoolValue(BOOL bVal)
{
	return (bVal == 0)? _T("N"): _T("Y");
}

static CString strBoolValues(BOOL bVal1, BOOL bVal2)
{
	CString str;
	str.Format(_T("(%s, %s)"), bVal1? _T("Y"):_T("N"),  bVal2? _T("Y"):_T("N"));
	return str;
}

static TCHAR tchDot [] = {'.', EOS};
static TCHAR tchComma [] = {',', EOS};
static TCHAR tchSpace [] = {' ', EOS};
static TCHAR tchQuote [] = {'"', EOS};
static TCHAR tchSQuote [] = {'\'', EOS};
static TCHAR tchStartBrace [] ={'(', EOS};
static TCHAR tchEndBrace [] ={')', EOS};
static TCHAR tchUnderscore [] ={'_', EOS};
static TCHAR tchMinus [] ={'-', EOS};
static TCHAR tchPlus [] ={'+', EOS};
static TCHAR tchStar [] ={'*', EOS};
static TCHAR tchForwardSlash [] ={'/', EOS};
static TCHAR tchEqualTo [] ={'=', EOS};
static TCHAR tchColon [] ={':', EOS};
static TCHAR tchSemiColon [] ={';', EOS};

BOOL isDelimiter (char *p1)
{
	// Any Ingres delimiter which separates Ingres tokens.
	if (CMwhite(p1))
		return TRUE;
	if (!CMcmpcase(p1,tchDot))
		return TRUE;
	if (!CMcmpcase(p1,tchComma))
		return TRUE;
	if (!CMcmpcase(p1,tchStartBrace))
		return TRUE;
	if (!CMcmpcase(p1,tchEndBrace))
		return TRUE;
	if (!CMcmpcase(p1,tchUnderscore))
		return TRUE;
	if (!CMcmpcase(p1,tchMinus))
		return TRUE;	
	if (!CMcmpcase(p1,tchPlus))
		return TRUE;	
	if (!CMcmpcase(p1,tchStar))
		return TRUE;	
	if (!CMcmpcase(p1,tchForwardSlash))
		return TRUE;
	if (!CMcmpcase(p1,tchEqualTo))
		return TRUE;
	if (!CMcmpcase(p1,tchColon))
		return TRUE;
	if (!CMcmpcase(p1,tchSemiColon))
		return TRUE;

	return FALSE;

}

// Routine to remove unwanted whitespaces from the string. 
void RemoveWhiteSpaces(CString *str1)
{
	BOOL dqcount,sqcount;
	LPTSTR lpbuf = str1->GetBuffer();
	LPTSTR lptemp = lpbuf;
	dqcount=FALSE;
	sqcount=FALSE;
	while (*lpbuf != EOS) 
	{
		// Copy the characters one by one, special processing is 
		// required only when whitespace appears outside double quotes
		// or single quotes.
		if ((!dqcount) && (!sqcount)&& (isDelimiter(lpbuf)))
		{
			// Ignore multiple whitespaces if any.
			while (CMwhite(lpbuf))
				CMnext(lpbuf);
			
			// Check whether the whitespace is followed by another
			// delimiter. It will certainly not be a whitespace as
			// they have been exhausted above.
			if (isDelimiter(lpbuf))
			{
				// Copy the delimiter. Ignore the whitespaces
				// following this delimiter.
				CMcpyinc(lpbuf,lptemp);
				while(CMwhite(lpbuf))
					CMnext(lpbuf); 

				continue;
			}
			else
			{
				// Whitespace is not followed by a delimiter, 
				// hence just copy one space character in place
				// of multiple whitespace characters 
				// encountered earlier.
				CMcpychar(tchSpace,lptemp);
				CMnext(lptemp);
			} 
		} 

		if ((!CMcmpcase(lpbuf,tchQuote)) && (!sqcount))
			dqcount = !dqcount;

		if ((!CMcmpcase(lpbuf,tchSQuote)) && (!dqcount))
			sqcount = !sqcount;

		CMcpyinc(lpbuf,lptemp);            		 
	} 
	CMcpychar(lpbuf,lptemp); // Copy the last character i.e. EOS character.
	str1->ReleaseBuffer();
}

int VSD_CompareWithIgnoreWhiteSpaces(CString str1, CString str2)
{
	RemoveWhiteSpaces(&str1);
	RemoveWhiteSpaces(&str2);
	return(str1.CompareNoCase(str2));
}
static void VSD_CompareURPPrivileges(
    CTypedPtrList< CObList, CaVsdDifference* >& listDiff, 
    CaURPPrivileges* pObj1, 
    CaURPPrivileges* pObj2, 
    BOOL bRole)
{
	// Security Audit Query Text:
	if (pObj1->IsSecurityAuditQueryText() != pObj2->IsSecurityAuditQueryText())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_QUERY_TXT); /*_T("Query Text")*/
		pObj->SetInfoSchema1 (strBoolValue(pObj1->IsSecurityAuditQueryText()));
		pObj->SetInfoSchema2 (strBoolValue(pObj2->IsSecurityAuditQueryText()));
		listDiff.AddTail(pObj);
	}
	// Security Audit All Event:
	if (pObj1->IsSecurityAuditAllEvent() != pObj2->IsSecurityAuditAllEvent())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_ALL_EVENT);/*_T("All Event")*/
		pObj->SetInfoSchema1 (strBoolValue(pObj1->IsSecurityAuditAllEvent()));
		pObj->SetInfoSchema2 (strBoolValue(pObj2->IsSecurityAuditAllEvent()));
		listDiff.AddTail(pObj);
	}
	// Limit Security Label:
	if (pObj1->GetLimitSecurityLabel().CompareNoCase(pObj2->GetLimitSecurityLabel()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_LIMIT_SEC);/*_T("Limit Security Label")*/
		pObj->SetInfoSchema1 (pObj1->GetLimitSecurityLabel());
		pObj->SetInfoSchema2 (pObj2->GetLimitSecurityLabel());
		listDiff.AddTail(pObj);
	}

	const int dim = 14;
	TCHAR tchPriv[14][32]=
	{
		_T("Create Database"),
		_T("Trace"),
		_T("Security"),
		_T("Operator"),
		_T("Maintain Location"),
		_T("Auditor"),
		_T("Maintain Audit"),
		_T("Maintain User"),
		_T("Write Down"),
		_T("Write Fixed"),
		_T("Write Up"),
		_T("Insert Down"),
		_T("Insert Up"),
		_T("Session Security Label")
	};
	UserRoleProfleFlags nVal [dim] =
	{
		URP_CREATE_DATABASE,
		URP_TRACE,
		URP_SECURITY,
		URP_OPERATOR,
		URP_MAINTAIN_LOCATION,
		URP_AUDITOR,
		URP_MAINTAIN_AUDIT,
		URP_MAINTAIN_USER,
		URP_WRITEDOWN,
		URP_WRITEFIXED,
		URP_WRITEUP,
		URP_INSERTDOWN,
		URP_INSERTUP,
		URP_SESSIONSECURITYLABEL
	};
	for (int i=0; i<dim; i++)
	{
		BOOL bPriv    = (pObj1->IsPrivilege(nVal[i]) != pObj2->IsPrivilege(nVal[i]));
		BOOL bDefPriv = (pObj1->IsDefaultPrivilege(nVal[i]) != pObj2->IsDefaultPrivilege(nVal[i]));

		if (bPriv || (!bRole && bDefPriv))
		{
			CString strPriv;
			if (bRole)
				strPriv.Format(IDS_USER, tchPriv[i]); /* _T("%s (users)") */
			else
				strPriv.Format(IDS_USER_DEFAULT, tchPriv[i]); /* _T("%s (users, default)") */

			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (strPriv);
			if (bRole)
			{
				pObj->SetInfoSchema1 (strBoolValue(pObj1->IsPrivilege(nVal[i])));
				pObj->SetInfoSchema2 (strBoolValue(pObj2->IsPrivilege(nVal[i])));
			}
			else
			{
				pObj->SetInfoSchema1 (strBoolValues(pObj1->IsPrivilege(nVal[i]), pObj1->IsDefaultPrivilege(nVal[i])));
				pObj->SetInfoSchema2 (strBoolValues(pObj2->IsPrivilege(nVal[i]), pObj2->IsDefaultPrivilege(nVal[i])));
			}
			listDiff.AddTail(pObj);
		}
	}
}

BOOL VSD_Installation(LPCTSTR lpszFile)
{
	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE;
	DWORD grfMode2= STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE ;
	IStream* pStream = NULL;
	CaCompoundFile cmpFile(lpszFile, grfMode);
	IStorage* pRoot = cmpFile.GetRootStorage();
	ASSERT(pRoot);
	if (!pRoot)
		return FALSE;
	BOOL bInstallation = FALSE;
	//
	// Load Schema Information:
	pStream = cmpFile.OpenStream(NULL, _T("SCHEMAINFO"), grfMode2);
	if (pStream)
	{
		int nVersion = 0;
		CString strItem;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);

		ar >> strItem;  // Date
		ar >> nVersion; // Version
		ar >> strItem;  // Node
		ar >> strItem;  // Database
		if (strItem.IsEmpty())
			bInstallation = TRUE;
		ar >> strItem;  // Schema
		ar.Flush();
		pStream->Release();
	}

	return bInstallation;
}

void VSD_CompareUser (CaVsdUser* pDiff, CaUserDetail* pObj1, CaUserDetail* pObj2)
{
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();

	// Default group:
	if (pObj1->GetDefaultGroup().CompareNoCase(pObj2->GetDefaultGroup()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_DEF_GROUP);/*_T("Default group")*/
		pObj->SetInfoSchema1 (pObj1->GetDefaultGroup());
		pObj->SetInfoSchema2 (pObj2->GetDefaultGroup());
		listDiff.AddTail(pObj);
	}
	// Default profile:
	if (pObj1->GetDefaultProfile().CompareNoCase(pObj2->GetDefaultProfile()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_DEF_PROF);/*_T("Default profile")*/
		pObj->SetInfoSchema1 (pObj1->GetDefaultProfile());
		pObj->SetInfoSchema2 (pObj2->GetDefaultProfile());
		listDiff.AddTail(pObj);
	}
	// External Password:
	if (pObj1->IsExternalPassword() != pObj2->IsExternalPassword())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_EXT_PASSWORD);/*_T("External password")*/
		pObj->SetInfoSchema1 (pObj1->IsExternalPassword());
		pObj->SetInfoSchema2 (pObj2->IsExternalPassword());
		listDiff.AddTail(pObj);
	}
	// Expire Date:
	if (pObj1->GetExpireDate().CompareNoCase(pObj2->GetExpireDate()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_EXP_DATE);/*_T("Expire date")*/
		pObj->SetInfoSchema1 (pObj1->GetExpireDate());
		pObj->SetInfoSchema2 (pObj2->GetExpireDate());
		listDiff.AddTail(pObj);
	}
	VSD_CompareURPPrivileges(listDiff, pObj1->GetURPPrivileges(), pObj2->GetURPPrivileges(), FALSE);
}

void VSD_CompareGroup (CaVsdGroup* pDiff, CaGroupDetail* pObj1, CaGroupDetail* pObj2)
{
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	CString strDiff1 = _T("");
	CString strDiff2 = _T("");
	CStringList& l1 = pObj1->GetListMember();
	CStringList& l2 = pObj2->GetListMember();

	POSITION pos = l1.GetHeadPosition();
	while (pos != NULL)
	{
		CString strMember = l1.GetNext(pos);
		if (l2.Find(strMember) == NULL)
		{
			if (!strDiff1.IsEmpty())
				strDiff1 += _T(", ");
			strDiff1 += strMember;
		}
	}

	pos = l2.GetHeadPosition();
	while (pos != NULL)
	{
		CString strMember = l2.GetNext(pos);
		if (l1.Find(strMember) == NULL)
		{
			if (!strDiff2.IsEmpty())
				strDiff2 += _T(", ");
			strDiff2 += strMember;
		}
	}

	CString csNotAvailable;
	csNotAvailable.LoadString(IDS_NOT_AVAILABLE);

	// Group Member (missing member):
	if (!strDiff1.IsEmpty())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_MISSING_MEMBERS);/*_T("Missing Members")*/
		pObj->SetInfoSchema1 (strDiff1);
		pObj->SetInfoSchema2 (csNotAvailable);
		listDiff.AddTail(pObj);
	}
	if (!strDiff2.IsEmpty())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_MISSING_MEMBERS);/*_T("Missing Members")*/
		pObj->SetInfoSchema1 (csNotAvailable);
		pObj->SetInfoSchema2 (strDiff2);
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareRole (CaVsdRole* pDiff, CaRoleDetail* pObj1, CaRoleDetail* pObj2)
{
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();

	VSD_CompareURPPrivileges(listDiff, pObj1->GetURPPrivileges(), pObj2->GetURPPrivileges(), TRUE);
}

void VSD_CompareProfile (CaVsdProfile* pDiff, CaProfileDetail* pObj1, CaProfileDetail* pObj2)
{
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();

	// Default group:
	if (pObj1->GetDefaultGroup().CompareNoCase(pObj2->GetDefaultGroup()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_DEF_GROUP);/*_T("Default group")*/
		pObj->SetInfoSchema1 (pObj1->GetDefaultGroup());
		pObj->SetInfoSchema2 (pObj2->GetDefaultGroup());
		listDiff.AddTail(pObj);
	}
	// Expire Date:
	if (pObj1->GetExpireDate().CompareNoCase(pObj2->GetExpireDate()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_EXP_DATE);/*_T("Expire date")*/
		pObj->SetInfoSchema1 (pObj1->GetExpireDate());
		pObj->SetInfoSchema2 (pObj2->GetExpireDate());
		listDiff.AddTail(pObj);
	}
	VSD_CompareURPPrivileges(listDiff, pObj1->GetURPPrivileges(), pObj2->GetURPPrivileges(), FALSE);
}

void VSD_CompareLocation (CaVsdLocation* pDiff, CaLocationDetail* pObj1, CaLocationDetail* pObj2)
{
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Data Usage:
	if (pObj1->GetDataUsage() != pObj2->GetDataUsage())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_DATA_USAGE);/*_T("Data Usage")*/
		pObj->SetInfoSchema1 (pObj1->GetDataUsage());
		pObj->SetInfoSchema2 (pObj2->GetDataUsage());
		listDiff.AddTail(pObj);
	}
	// Journal Usage:
	if (pObj1->GetJournalUsage() != pObj2->GetJournalUsage())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_JNL_USAGE);/*_T("Journal Usage")*/
		pObj->SetInfoSchema1 (pObj1->GetJournalUsage());
		pObj->SetInfoSchema2 (pObj2->GetJournalUsage());
		listDiff.AddTail(pObj);
	}
	// Check Point Usage:
	if (pObj1->GetCheckPointUsage() != pObj2->GetCheckPointUsage())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_CHECKPOINT_USAGE);/*_T("Check Point Usage")*/
		pObj->SetInfoSchema1 (pObj1->GetCheckPointUsage());
		pObj->SetInfoSchema2 (pObj2->GetCheckPointUsage());
		listDiff.AddTail(pObj);
	}
	// Work Usage:
	if (pObj1->GetWorkUsage() != pObj2->GetWorkUsage())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_WORK_USAGE);/*_T("Work Usage")*/
		pObj->SetInfoSchema1 (pObj1->GetWorkUsage());
		pObj->SetInfoSchema2 (pObj2->GetWorkUsage());
		listDiff.AddTail(pObj);
	}
	// Dump Usage:
	if (pObj1->GetDumpUsage() != pObj2->GetDumpUsage())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (DS_TYPE_DUMP);/*_T("Dump Usage")*/
		pObj->SetInfoSchema1 (pObj1->GetDumpUsage());
		pObj->SetInfoSchema2 (pObj2->GetDumpUsage());
		listDiff.AddTail(pObj);
	}
	// Location Area:
	if (pObj1->GetLocationArea().CompareNoCase(pObj2->GetLocationArea()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_LOC_AREA);/*_T("Location Area")*/
		pObj->SetInfoSchema1 (pObj1->GetLocationArea());
		pObj->SetInfoSchema2 (pObj2->GetLocationArea());
		listDiff.AddTail(pObj);
	}
}


void VSD_CompareDatabase (CaVsdDatabase* pDiff, CaDatabaseDetail* pObj1, CaDatabaseDetail* pObj2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());

	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	if (pObj1->GetService() != pObj2->GetService())
	{
		// Read Only:
		if (pObj1->IsReadOnly() != pObj2->IsReadOnly())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_READ_ONLY);/*_T("Read Only")*/
			pObj->SetInfoSchema1 (strBoolValue(pObj1->IsReadOnly()));
			pObj->SetInfoSchema2 (strBoolValue(pObj2->IsReadOnly()));
			listDiff.AddTail(pObj);
		}
		// Private:
		if (pObj1->IsPrivate() != pObj2->IsPrivate())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_PRIVATE);/*_T("Private")*/
			pObj->SetInfoSchema1 (strBoolValue(pObj1->IsPrivate()));
			pObj->SetInfoSchema2 (strBoolValue(pObj2->IsPrivate()));
			listDiff.AddTail(pObj);
		}
		// Star:
		if (pObj1->GetStar() != pObj2->GetStar())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_DB_TYPE);/*_T("Database type")*/
			switch (pObj1->GetStar())
			{
			case OBJTYPE_STARLINK:
				pObj->SetInfoSchema1 (_T("Star linked"));
				break;
			case OBJTYPE_STARNATIVE:
				pObj->SetInfoSchema1 (_T("Star native"));
				break;
			default:
				pObj->SetInfoSchema1 (_T("Normal"));
				break;
			}

			switch (pObj2->GetStar())
			{
			case OBJTYPE_STARLINK:
				pObj->SetInfoSchema1 (_T("Star linked"));
				break;
			case OBJTYPE_STARNATIVE:
				pObj->SetInfoSchema1 (_T("Star native"));
				break;
			default:
				pObj->SetInfoSchema1 (_T("Normal"));
				break;
			}

			listDiff.AddTail(pObj);
		}
	}
	if (pObj1->GetUnicodeEnable() != pObj2->GetUnicodeEnable())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_UNICODE);/*_T("Unicode Enable")*/
		pObj->SetInfoSchema1 (strBoolValue(pObj1->GetUnicodeEnable()));
		pObj->SetInfoSchema2 (strBoolValue(pObj2->GetUnicodeEnable()));
		listDiff.AddTail(pObj);
	}
	if ( pObj1->GetLocationCheckPoint().CompareNoCase(pObj2->GetLocationCheckPoint()) != 0 )
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_CHECKPOINT_LOC);/*_T("Checkpoint Location")*/
		pObj->SetInfoSchema1 (pObj1->GetLocationCheckPoint());
		pObj->SetInfoSchema2 (pObj2->GetLocationCheckPoint());
		listDiff.AddTail(pObj);
	}
	if ( pObj1->GetLocationDatabase().CompareNoCase(pObj2->GetLocationDatabase()) != 0 )
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_DTA_LOC);/*_T("Data Location")*/
		pObj->SetInfoSchema1 (pObj1->GetLocationDatabase());
		pObj->SetInfoSchema2 (pObj2->GetLocationDatabase());
		listDiff.AddTail(pObj);
	}
	if ( pObj1->GetLocationJournal().CompareNoCase(pObj2->GetLocationJournal()) != 0 )
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_JNL_LOC);/*_T("Journal Location")*/
		pObj->SetInfoSchema1 (pObj1->GetLocationJournal());
		pObj->SetInfoSchema2 (pObj2->GetLocationJournal());
		listDiff.AddTail(pObj);
	}
	if ( pObj1->GetLocationDump().CompareNoCase(pObj2->GetLocationDump()) != 0 )
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_DUMP_LOC);/*_T("Dump Location")*/
		pObj->SetInfoSchema1 (pObj1->GetLocationDump());
		pObj->SetInfoSchema2 (pObj2->GetLocationDump());
		listDiff.AddTail(pObj);
	}
	if ( pObj1->GetLocationWork().CompareNoCase(pObj2->GetLocationWork()) != 0 )
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_WORK_LOC);/*_T("Work Location")*/
		pObj->SetInfoSchema1 (pObj1->GetLocationWork());
		pObj->SetInfoSchema2 (pObj2->GetLocationWork());
		listDiff.AddTail(pObj);
	}
	if ( pObj1->GetSecurityLabel().CompareNoCase(pObj2->GetSecurityLabel()) != 0 )
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_SEC_LABEL);/*_T("Security Label")*/
		pObj->SetInfoSchema1 (pObj1->GetSecurityLabel());
		pObj->SetInfoSchema2 (pObj2->GetSecurityLabel());
		listDiff.AddTail(pObj);
	}
}


static void VSD_CompareStorageStructure(
    CTypedPtrList< CObList, CaVsdDifference* >& listDiff, 
    CaStorageStructure& stg1,
    CaStorageStructure& stg2)
{
	// Storage Structure:
	if (stg1.GetStructure().CompareNoCase(stg2.GetStructure()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_STOR_STRUCT);/*_T("Storage Structure")*/
		pObj->SetInfoSchema1 (stg1.GetStructure());
		pObj->SetInfoSchema2 (stg2.GetStructure());
		listDiff.AddTail(pObj);
	}
	// Page Size:
	if (stg1.GetPageSize() != stg2.GetPageSize())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_PAGE_SIZE);/*_T("Page Size")*/
		pObj->SetInfoSchema1 (stg1.GetPageSize());
		pObj->SetInfoSchema2 (stg2.GetPageSize());
		listDiff.AddTail(pObj);
	}
	// Extend:
	if (stg1.GetExtend() != stg2.GetExtend())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_EXTEND);/*_T("Extend")*/
		pObj->SetInfoSchema1 (stg1.GetExtend());
		pObj->SetInfoSchema2 (stg2.GetExtend());
		listDiff.AddTail(pObj);
	}
	// Allocation:
	if (stg1.GetAllocation() != stg2.GetAllocation())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_ALLOC);/*_T("Allocation")*/
		pObj->SetInfoSchema1 (stg1.GetAllocation());
		pObj->SetInfoSchema2 (stg2.GetAllocation());
		listDiff.AddTail(pObj);
	}
	// Page Allocated:
	if (stg1.GetAllocatedPage() != stg2.GetAllocatedPage())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_ALLOCATED_PAGES);/*_T("Allocated Pages")*/
		pObj->SetInfoSchema1 (stg1.GetAllocatedPage());
		pObj->SetInfoSchema2 (stg2.GetAllocatedPage());
		listDiff.AddTail(pObj);
	}
	// Unique Scope : (if not HEAP and not HEAPSORT)
	if (stg1.GetStructure().CompareNoCase (_T("HEAP")) != 0 && 
	    stg1.GetStructure().CompareNoCase (_T("HEAPSORT")) != 0 && 
	    stg1.GetStructure().CompareNoCase(stg2.GetStructure()) == 0)
	{
		if (stg1.GetUniqueScope() != stg2.GetUniqueScope())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_UNIQUE_SCOPE);/*_T("Unique Scope")*/
			pObj->SetInfoSchema1 (stg1.GetUniqueScope());
			pObj->SetInfoSchema2 (stg2.GetUniqueScope());
			listDiff.AddTail(pObj);
		}
	}

	// Unique Rule:
	if (stg1.GetUniqueRule() != stg2.GetUniqueRule())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_UNIQUE_RULE);/*_T("Unique Rule")*/
		pObj->SetInfoSchema1 (stg1.GetUniqueRule());
		pObj->SetInfoSchema2 (stg2.GetUniqueRule());
		listDiff.AddTail(pObj);
	}
	// Compress:
	if (stg1.GetCompress() != stg2.GetCompress())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_COMPRESS);/*_T("Compress")*/
		pObj->SetInfoSchema1 (stg1.GetCompress());
		pObj->SetInfoSchema2 (stg2.GetCompress());
		listDiff.AddTail(pObj);
	}
	// Key Compress:
	if (stg1.GetKeyCompress() != stg2.GetKeyCompress())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_KEY_COMPRESS);/*_T("Key Compress")*/
		pObj->SetInfoSchema1 (stg1.GetKeyCompress());
		pObj->SetInfoSchema2 (stg2.GetKeyCompress());
		listDiff.AddTail(pObj);
	}

	// FillFactor: (if not HEAP and not HEAPSORT)
	if (stg1.GetStructure().CompareNoCase (_T("HEAP")) != 0 && 
	    stg1.GetStructure().CompareNoCase (_T("HEAPSORT")) != 0 && 
	    stg1.GetStructure().CompareNoCase(stg2.GetStructure()) == 0)
	{
		if (stg1.GetFillFactor() != stg2.GetFillFactor())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_FILL_FACTOR);/*_T("Fill Factor")*/
			pObj->SetInfoSchema1 (stg1.GetFillFactor());
			pObj->SetInfoSchema2 (stg2.GetFillFactor());
			listDiff.AddTail(pObj);
		}
	}
	// LeafFill and Non LeafFill: (BTREE only)
	if (stg1.GetStructure().CompareNoCase (_T("BTREE")) == 0 && 
	    stg1.GetStructure().CompareNoCase(stg2.GetStructure()) == 0)
	{
		if (stg1.GetLeafFill() != stg2.GetLeafFill())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_LEAFFILL);/*_T("LeafFill")*/
			pObj->SetInfoSchema1 (stg1.GetLeafFill());
			pObj->SetInfoSchema2 (stg2.GetLeafFill());
			listDiff.AddTail(pObj);
		}
		if (stg1.GetNonLeafFill() != stg2.GetNonLeafFill())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_NON_LEAFFILL);/*_T("Non LeafFill")*/
			pObj->SetInfoSchema1 (stg1.GetNonLeafFill());
			pObj->SetInfoSchema2 (stg2.GetNonLeafFill());
			listDiff.AddTail(pObj);
		}
	}
	// MinPage and MaxPage: (HASH only)
	if (stg1.GetStructure().CompareNoCase (_T("HASH")) == 0 &&
	    stg1.GetStructure().CompareNoCase(stg2.GetStructure()) == 0)
	{
		if (stg1.GetMinPage() != stg2.GetMinPage())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_MIN_PAGE);/*_T("Min Page")*/
			pObj->SetInfoSchema1 (stg1.GetMinPage());
			pObj->SetInfoSchema2 (stg2.GetMinPage());
			listDiff.AddTail(pObj);
		}

		if (stg1.GetMaxPage() != stg2.GetMaxPage())
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_MAX_PAGE);/*_T("Max Page")*/
			pObj->SetInfoSchema1 (stg1.GetMaxPage());
			pObj->SetInfoSchema2 (stg2.GetMaxPage());
			listDiff.AddTail(pObj);
		}
	}
}

static void VSD_CompareColumn (
    CTypedPtrList< CObList, CaVsdDifference* >& listDiff, 
    CTypedPtrList< CObList, CaColumn* >& lc1,
    CTypedPtrList< CObList, CaColumn* >& lc2)
{
	// Columns Counts:
	if (lc1.GetCount() != lc2.GetCount())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_NUM_COLS);/*_T("Number of Columns")*/
		pObj->SetInfoSchema1 (lc1.GetCount());
		pObj->SetInfoSchema2 (lc2.GetCount());
		listDiff.AddTail(pObj);
	}
	// Order of Columns:
	BOOL bOne = TRUE;
	POSITION pos = lc1.GetHeadPosition();
	CString strColumnOrder1 = _T("");
	CString strColumnOrder2 = _T("");
	while (pos != NULL)
	{
		CaColumn* pCol = lc1.GetNext(pos);
		if (!bOne)
			strColumnOrder1 += _T(", ");
		strColumnOrder1 += pCol->GetName();
		bOne = FALSE;
	}
	bOne = TRUE;
	pos = lc2.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = lc2.GetNext(pos);
		if (!bOne)
			strColumnOrder2 += _T(", ");
		strColumnOrder2 += pCol->GetName();
		bOne = FALSE;
	}
	if (strColumnOrder1.CompareNoCase(strColumnOrder2) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_ORDER_COLS);/*_T("Order of Columns")*/
		pObj->SetInfoSchema1 (strColumnOrder1);
		pObj->SetInfoSchema2 (strColumnOrder2);
		listDiff.AddTail(pObj);
	}

	CString csNotAvailable;
	csNotAvailable.LoadString(IDS_NOT_AVAILABLE);
	// Missing Columns:
	CTypedPtrList< CObList, CaColumn* > lc4Attribute;
	pos = lc1.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = lc1.GetNext(pos);
		CaColumn* pFound = CaColumn::FindColumn(pCol->GetName(), lc2);
		if (pFound)
		{
			lc4Attribute.AddTail(pCol);
			lc4Attribute.AddTail(pFound);
		}
		else
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_COLUMN);
			pObj->SetInfoSchema1 (pCol->GetName());
			pObj->SetInfoSchema2 (csNotAvailable);
			listDiff.AddTail(pObj);
		}
	}
	pos = lc2.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = lc2.GetNext(pos);
		CaColumn* pFound = CaColumn::FindColumn(pCol->GetName(), lc1);
		if (!pFound)
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_COLUMN);
			pObj->SetInfoSchema1 (csNotAvailable);
			pObj->SetInfoSchema2 (pCol->GetName());
			listDiff.AddTail(pObj);
		}
	}
	// Attribute of Columns:
	pos = lc4Attribute.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol1 = lc4Attribute.GetNext(pos);
		CaColumn* pCol2 = lc4Attribute.GetNext(pos);
		CString strStatement1;
		CString strStatement2;
		pCol1->GenerateStatement(strStatement1);
		pCol2->GenerateStatement(strStatement2);
		if (strStatement1.CompareNoCase(strStatement2) != 0)
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_COL_ATTRIB);/*_T("Column Attribute")*/
			pObj->SetInfoSchema1 (strStatement1);
			pObj->SetInfoSchema2 (strStatement2);
			listDiff.AddTail(pObj);
		}
	}
}



static void VSD_CompareConstraints(
    CTypedPtrList< CObList, CaVsdDifference* >& listDiff,
    CaTableDetail* pTable1,
    CaTableDetail* pTable2)
{
	CString csNotAvailable;
	csNotAvailable.LoadString(IDS_NOT_AVAILABLE);
	// Constraint Primary key:
	CaPrimaryKey& pk1 = pTable1->GetPrimaryKey();
	CaPrimaryKey& pk2 = pTable2->GetPrimaryKey();
	if (!pk1.GetConstraintName().IsEmpty() && !pk2.GetConstraintName().IsEmpty())
	{
		// Primary Key Text
		if (pk1.GetConstraintText().CompareNoCase(pk2.GetConstraintText()) != 0)
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_TYPE_PRIMARY_KEY);/*_T("Primary Key Text")*/
			pObj->SetInfoSchema1 (pk1.GetConstraintText());
			pObj->SetInfoSchema2 (pk2.GetConstraintText());
			listDiff.AddTail(pObj);
		}
		// Index Option:
	}
	else
	{
		if (!pk1.GetConstraintName().IsEmpty() && pk2.GetConstraintName().IsEmpty())
		{
			// Missing Primary Key
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_PRIM_KEY);
			pObj->SetInfoSchema1 (pk1.GetConstraintText());
			pObj->SetInfoSchema2 (csNotAvailable);
			listDiff.AddTail(pObj);
		}
		else
		if (pk1.GetConstraintName().IsEmpty() && !pk2.GetConstraintName().IsEmpty())
		{
			// Missing Primary Key
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_PRIM_KEY);
			pObj->SetInfoSchema1 (csNotAvailable);
			pObj->SetInfoSchema2 (pk2.GetConstraintText());
			listDiff.AddTail(pObj);
		}
	}

	// Constraint Unique keys:
	CTypedPtrList < CObList, CaUniqueKey* >& lu1 = pTable1->GetUniqueKeys();
	CTypedPtrList < CObList, CaUniqueKey* >& lu2 = pTable2->GetUniqueKeys();
	POSITION pos = lu1.GetHeadPosition();
	while (pos != NULL)
	{
		CaUniqueKey* pUnique = lu1.GetNext(pos);
		if (!CaUniqueKey::FindUniqueKey(lu2, pUnique))
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_UNIQUE_KEY);
			pObj->SetInfoSchema1 (pUnique->GetConstraintText());
			pObj->SetInfoSchema2 (csNotAvailable);
			listDiff.AddTail(pObj);
		}
	}
	pos = lu2.GetHeadPosition();
	while (pos != NULL)
	{
		CaUniqueKey* pUnique = lu2.GetNext(pos);
		if (!CaUniqueKey::FindUniqueKey(lu1, pUnique))
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_UNIQUE_KEY);
			pObj->SetInfoSchema1 (csNotAvailable);
			pObj->SetInfoSchema2 (pUnique->GetConstraintText());
			listDiff.AddTail(pObj);
		}
	}
	// Constraint Foreign keys:
	CTypedPtrList < CObList, CaForeignKey* >& lf1 = pTable1->GetForeignKeys();
	CTypedPtrList < CObList, CaForeignKey* >& lf2 = pTable2->GetForeignKeys();
	pos = lf1.GetHeadPosition();
	while (pos != NULL)
	{
		CaForeignKey* pForeign = lf1.GetNext(pos);
		if (!CaForeignKey::FindForeignKey(lf2, pForeign))
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_FOREIGN_KEY);
			pObj->SetInfoSchema1 (pForeign->GetConstraintText());
			pObj->SetInfoSchema2 (csNotAvailable);
			listDiff.AddTail(pObj);
		}
	}
	pos = lf2.GetHeadPosition();
	while (pos != NULL)
	{
		CaForeignKey* pForeign = lf2.GetNext(pos);
		if (!CaForeignKey::FindForeignKey(lf1, pForeign))
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_FOREIGN_KEY);
			pObj->SetInfoSchema1 (csNotAvailable);
			pObj->SetInfoSchema2 (pForeign->GetConstraintText());
			listDiff.AddTail(pObj);
		}
	}

	// Constraint Checks:
	CTypedPtrList < CObList, CaCheckKey* >& lc1 = pTable1->GetCheckKeys();
	CTypedPtrList < CObList, CaCheckKey* >& lc2 = pTable2->GetCheckKeys();
	pos = lc1.GetHeadPosition();
	while (pos != NULL)
	{
		CaCheckKey* pCheck = lc1.GetNext(pos);
		if (!CaCheckKey::FindCheckKey(lc2, pCheck))
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_CHECK);
			pObj->SetInfoSchema1 (pCheck->GetConstraintText());
			pObj->SetInfoSchema2 (csNotAvailable);
			listDiff.AddTail(pObj);
		}
	}
	pos = lc2.GetHeadPosition();
	while (pos != NULL)
	{
		CaCheckKey* pCheck = lc2.GetNext(pos);
		if (!CaCheckKey::FindCheckKey(lc1, pCheck))
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_CHECK);
			pObj->SetInfoSchema1 (csNotAvailable);
			pObj->SetInfoSchema2 (pCheck->GetConstraintText());
			listDiff.AddTail(pObj);
		}
	}
}

static void VSD_CompareTableConsistent(
    CTypedPtrList< CObList, CaVsdDifference* >& listDiff,
    CaTableDetail* pTable1,
    CaTableDetail* pTable2)
{
	BOOL bVal1,bVal2;

	bVal1 = pTable1->GetTablePhysInConsistent();
	bVal2 = pTable2->GetTablePhysInConsistent();
	if (bVal1 != bVal2)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_TBL_PHYSICALLY);/*_T("Table Physically Consistent")*/
		pObj->SetInfoSchema1 (strBoolValue( bVal1));
		pObj->SetInfoSchema2 (strBoolValue( bVal2));
		listDiff.AddTail(pObj);
	}
	bVal1 = pTable1->GetTableLogInConsistent();
	bVal2 = pTable2->GetTableLogInConsistent();
	if (bVal1 != bVal2)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_TBL_LOGICALLY);/*_T("Table Logically Consistent")*/
		pObj->SetInfoSchema1 (strBoolValue( bVal1));
		pObj->SetInfoSchema2 (strBoolValue( bVal2));
		listDiff.AddTail(pObj);
	}
	bVal1 = pTable1->GetTableRecoveryDisallowed();
	bVal2 = pTable2->GetTableRecoveryDisallowed();
	if (bVal1 != bVal2)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_TBL_RECOVERY);/*_T("Table Level Recovery Permitted")*/
		pObj->SetInfoSchema1 (strBoolValue( bVal1));
		pObj->SetInfoSchema2 (strBoolValue( bVal2));
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareTable (CaVsdTable* pDiff, CaTableDetail* pTable1, CaTableDetail* pTable2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());

	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	CaStorageStructure& stg1 = pTable1->GetStorageStructure();
	CaStorageStructure& stg2 = pTable2->GetStorageStructure();

	// Owner:
	if (!bIgnoreOwner && pTable1->GetOwner() != pTable2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 ((LPCTSTR)pTable1->GetOwner());
		pObj->SetInfoSchema2 ((LPCTSTR)pTable2->GetOwner());
		listDiff.AddTail(pObj);
	}
	// Journaling:
	if (pTable1->GetJournaling() != pTable2->GetJournaling())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_JOURNALING);/*_T("Journaling")*/
		pObj->SetInfoSchema1 (pTable1->GetJournaling());
		pObj->SetInfoSchema2 (pTable2->GetJournaling());
		listDiff.AddTail(pObj);
	}
	// Read Only:
	if (pTable1->GetReadOnly() != pTable2->GetReadOnly())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_READ_ONLY);/*_T("Read Only")*/
		pObj->SetInfoSchema1 (pTable1->GetReadOnly());
		pObj->SetInfoSchema2 (pTable2->GetReadOnly());
		listDiff.AddTail(pObj);
	}
	// Duplicated Row:
	if (pTable1->GetDuplicatedRows() != pTable2->GetDuplicatedRows())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_DUP_ROW);/*_T("Duplicated Row")*/
		pObj->SetInfoSchema1 (pTable1->GetDuplicatedRows());
		pObj->SetInfoSchema2 (pTable2->GetDuplicatedRows());
		listDiff.AddTail(pObj);
	}
	// Estimated Row Count:
	if (pTable1->GetEstimatedRowCount() != pTable2->GetEstimatedRowCount())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_ESTIMATE_ROW);/*_T("Estimated Row Count")*/
		pObj->SetInfoSchema1 (pTable1->GetEstimatedRowCount());
		pObj->SetInfoSchema2 (pTable2->GetEstimatedRowCount());
		listDiff.AddTail(pObj);
	}
	// Comment:
	if (pTable1->GetComment().CompareNoCase(pTable2->GetComment()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_COMMENT);/*_T("Comment")*/
		pObj->SetInfoSchema1 (pTable1->GetComment());
		pObj->SetInfoSchema2 (pTable2->GetComment());
		listDiff.AddTail(pObj);
	}
	// Expire Date:
	if (pTable1->GetExpireDate() != pTable2->GetExpireDate())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_EXP_DATE);/*_T("Expire Date")*/
		pObj->SetInfoSchema1 (pTable1->GetExpireDate());
		pObj->SetInfoSchema2 (pTable2->GetExpireDate());
		listDiff.AddTail(pObj);
	}
	// Location name:
	CString csListLoc1,csListLoc2;
	pTable1->GenerateListLocation(csListLoc1);
	pTable2->GenerateListLocation(csListLoc2);

	if (csListLoc1.CompareNoCase(csListLoc2) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_LOCATION);/*_T("Location(s)")*/
		pObj->SetInfoSchema1 (csListLoc1);
		pObj->SetInfoSchema2 (csListLoc2);
		listDiff.AddTail(pObj);
	}

	CTypedPtrList< CObList, CaColumn* >& lc1 = pTable1->GetListColumns();
	CTypedPtrList< CObList, CaColumn* >& lc2 = pTable2->GetListColumns();

	VSD_CompareStorageStructure(listDiff, stg1, stg2);
	VSD_CompareColumn (listDiff, lc1, lc2);
	VSD_CompareConstraints(listDiff, pTable1, pTable2);

	VSD_CompareTableConsistent(listDiff, pTable1, pTable2);

	// Cache Priority
	if (pTable1->GetStorageStructure().GetPriorityCache() != pTable2->GetStorageStructure().GetPriorityCache())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_CACHE_PRIORITY);/*_T("Cache Priority")*/
		pObj->SetInfoSchema1 (pTable1->GetStorageStructure().GetPriorityCache());
		pObj->SetInfoSchema2 (pTable2->GetStorageStructure().GetPriorityCache());
		listDiff.AddTail(pObj);
	}

}



void VSD_CompareIndex (CaVsdIndex* pDiff, CaIndexDetail* pObj1, CaIndexDetail* pObj2)
{
	CString csNotAvailable;
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	CaStorageStructure& stg1 = pObj1->GetStorageStructure();
	CaStorageStructure& stg2 = pObj2->GetStorageStructure();

	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	// Persistent:
	if (pObj1->GetPersistent() != pObj2->GetPersistent())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_PERSISTENT);/*_T("Persistent")*/
		pObj->SetInfoSchema1 (pObj1->GetPersistent());
		pObj->SetInfoSchema2 (pObj2->GetPersistent());
		listDiff.AddTail(pObj);
	}

	csNotAvailable.LoadString(IDS_NOT_AVAILABLE);

	VSD_CompareStorageStructure(listDiff, stg1, stg2);
	CTypedPtrList< CObList, CaColumnKey* >& lc1 = pObj1->GetListColumns();
	CTypedPtrList< CObList, CaColumnKey* >& lc2 = pObj2->GetListColumns();

	POSITION pos = lc1.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumnKey* pCol = lc1.GetNext(pos);
		CaColumnKey* pFound = CaColumnKey::FindColumn(pCol->GetName(), lc2);
		if (!pFound)
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_COLUMN);
			pObj->SetInfoSchema1 (pCol->GetName());
			pObj->SetInfoSchema2 (csNotAvailable);
			listDiff.AddTail(pObj);
		}
	}
	pos = lc2.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumnKey* pCol = lc2.GetNext(pos);
		CaColumnKey* pFound = CaColumnKey::FindColumn(pCol->GetName(), lc1);
		if (!pFound)
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_COLUMN);
			pObj->SetInfoSchema1 (csNotAvailable);
			pObj->SetInfoSchema2 (pCol->GetName());
			listDiff.AddTail(pObj);
		}
	}
	// Cache Priority
	if (pObj1->GetStorageStructure().GetPriorityCache() != pObj2->GetStorageStructure().GetPriorityCache())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_CACHE_PRIORITY);/*_T("Cache Priority")*/
		pObj->SetInfoSchema1 (pObj1->GetStorageStructure().GetPriorityCache());
		pObj->SetInfoSchema2 (pObj2->GetStorageStructure().GetPriorityCache());
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareRule (CaVsdRule* pDiff, CaRuleDetail* pObj1, CaRuleDetail* pObj2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	// statement text:
	if (VSD_CompareWithIgnoreWhiteSpaces(pObj1->GetDetailText(),pObj2->GetDetailText()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetSpecialDetail(TRUE);
		pObj->SetType (IDS_TYPE_STATEMENT);/*_T("Statement")*/
		pObj->SetInfoSchema1 (pObj1->GetDetailText());
		pObj->SetInfoSchema2 (pObj2->GetDetailText());
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareIntegrity (CaVsdIntegrity* pDiff, CaIntegrityDetail* pObj1, CaIntegrityDetail* pObj2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	/* no owner for integrity
	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (_T("Owner"));
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	*/
	// statement text:
	if (VSD_CompareWithIgnoreWhiteSpaces(pObj1->GetDetailText(),pObj2->GetDetailText()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetSpecialDetail(TRUE);
		pObj->SetType (IDS_TYPE_STATEMENT);/*_T("Statement")*/
		pObj->SetInfoSchema1 (pObj1->GetDetailText());
		pObj->SetInfoSchema2 (pObj2->GetDetailText());
		listDiff.AddTail(pObj);
	}
}


void VSD_CompareView (CaVsdView* pDiff, CaViewDetail* pObj1, CaViewDetail* pObj2)
{
	CString csNotAvailable;
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	// View statement text:

	if (VSD_CompareWithIgnoreWhiteSpaces(pObj1->GetDetailText(),pObj2->GetDetailText()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetSpecialDetail(TRUE);
		pObj->SetType (IDS_TYPE_STATEMENT);/*_T("Statement")*/
		pObj->SetInfoSchema1 (pObj1->GetDetailText());
		pObj->SetInfoSchema2 (pObj2->GetDetailText());
		listDiff.AddTail(pObj);
	}

	//
	// List of columns:
	CTypedPtrList< CObList, CaColumn* >& lc1 = pObj1->GetListColumns();
	CTypedPtrList< CObList, CaColumn* >& lc2 = pObj2->GetListColumns();
	
	csNotAvailable.LoadString(IDS_NOT_AVAILABLE);

	POSITION pos = lc1.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = lc1.GetNext(pos);
		CaColumn* pFound = CaColumn::FindColumn(pCol->GetName(), lc2);
		if (!pFound)
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_COLUMN);
			pObj->SetInfoSchema1 (pCol->GetName());
			pObj->SetInfoSchema2 (csNotAvailable);
			listDiff.AddTail(pObj);
		}
	}
	pos = lc2.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = lc2.GetNext(pos);
		CaColumn* pFound = CaColumn::FindColumn(pCol->GetName(), lc1);
		if (!pFound)
		{
			CaVsdDifference* pObj = new CaVsdDifference();
			pObj->SetType (IDS_MISSING_COLUMN);
			pObj->SetInfoSchema1 (csNotAvailable);
			pObj->SetInfoSchema2 (pCol->GetName());
			listDiff.AddTail(pObj);
		}
	}

	if (pObj1->GetCheckOption()  != pObj2->GetCheckOption())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_CHECK_OPT);/*_T("Check Option")*/
		pObj->SetInfoSchema1 (pObj1->GetCheckOption());
		pObj->SetInfoSchema2 (pObj2->GetCheckOption());
		listDiff.AddTail(pObj);

	}
	if ( pObj1->GetLanguageType() !=  pObj2->GetLanguageType())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_LANGUAGE);/*_T("Language")*/
		pObj->SetInfoSchema1 (pObj1->GetLanguageType());
		pObj->SetInfoSchema2 (pObj2->GetLanguageType());
		listDiff.AddTail(pObj);
	}

}


void VSD_CompareDBEvent (CaVsdDBEvent* pDiff, CaDBEventDetail* pObj1, CaDBEventDetail* pObj2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	// statement text:
	if (VSD_CompareWithIgnoreWhiteSpaces(pObj1->GetDetailText(),pObj2->GetDetailText()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_STATEMENT);/*_T("Statement")*/
		pObj->SetInfoSchema1 (pObj1->GetDetailText());
		pObj->SetInfoSchema2 (pObj2->GetDetailText());
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareProcedure (CaVsdProcedure* pDiff, CaProcedureDetail* pObj1, CaProcedureDetail* pObj2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	// statement text:
	if ( VSD_CompareWithIgnoreWhiteSpaces(pObj1->GetDetailText(),pObj2->GetDetailText()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetSpecialDetail(TRUE);
		pObj->SetType (IDS_TYPE_STATEMENT);/*_T("Statement")*/
		pObj->SetInfoSchema1 (pObj1->GetDetailText());
		pObj->SetInfoSchema2 (pObj2->GetDetailText());
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareSequence (CaVsdSequence* pDiff, CaSequenceDetail* pObj1, CaSequenceDetail* pObj2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}

	if (pObj1->GetDecimalType() != pObj2->GetDecimalType())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_TYPE);/*_T("Type")*/
		pObj->SetInfoSchema1 (pObj1->GetDecimalType()?_T("Decimal"):_T("Integer"));
		pObj->SetInfoSchema2 (pObj2->GetDecimalType()?_T("Decimal"):_T("Integer"));
		listDiff.AddTail(pObj);
	}
	else
	{
		// If the sequence is Decimal type, verify the precision.
		if (pObj1->GetDecimalType() == TRUE && pObj2->GetDecimalType() == TRUE )
		{
			if (pObj1->GetDecimalPrecision().CompareNoCase(pObj2->GetDecimalPrecision()) != 0)
			{
				CaVsdDifference* pObj = new CaVsdDifference();
				pObj->SetType (IDS_TYPE_DEC_PREC);/*_T("Decimal Precision")*/
				pObj->SetInfoSchema1 (pObj1->GetDecimalPrecision());
				pObj->SetInfoSchema2 (pObj2->GetDecimalPrecision());
				listDiff.AddTail(pObj);
			}
		}
	}

	if (pObj1->GetCycle() != pObj2->GetCycle())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_CYCLE);/*_T("Cycle")*/
		pObj->SetInfoSchema1 (strBoolValue(pObj1->GetCycle()));
		pObj->SetInfoSchema2 (strBoolValue(pObj2->GetCycle()));
		listDiff.AddTail(pObj);
	}

	if (pObj1->GetOrder() != pObj2->GetOrder())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_ORDER);/*_T("Order")*/
		pObj->SetInfoSchema1 (strBoolValue(pObj1->GetOrder()));
		pObj->SetInfoSchema2 (strBoolValue(pObj2->GetOrder()));
		listDiff.AddTail(pObj);
	}

	if (pObj1->GetMaxValue().CompareNoCase(pObj2->GetMaxValue()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_MAX_VAL);/*_T("Max Value")*/
		pObj->SetInfoSchema1 (pObj1->GetMaxValue());
		pObj->SetInfoSchema2 (pObj2->GetMaxValue());
		listDiff.AddTail(pObj);
	}

	if (pObj1->GetMinValue().CompareNoCase(pObj2->GetMinValue()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_MIN_VAL);/*_T("Min Value")*/
		pObj->SetInfoSchema1 (pObj1->GetMinValue());
		pObj->SetInfoSchema2 (pObj2->GetMinValue());
		listDiff.AddTail(pObj);
	}

	if (pObj1->GetStartWith().CompareNoCase(pObj2->GetStartWith()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_START_WITH);/*_T("Start With")*/
		pObj->SetInfoSchema1 (pObj1->GetStartWith());
		pObj->SetInfoSchema2 (pObj2->GetStartWith());
		listDiff.AddTail(pObj);
	}

	if (pObj1->GetIncrementBy().CompareNoCase(pObj2->GetIncrementBy()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_INCREMENT_BY);/*_T("Increment By")*/
		pObj->SetInfoSchema1 (pObj1->GetIncrementBy());
		pObj->SetInfoSchema2 (pObj2->GetIncrementBy());
		listDiff.AddTail(pObj);
	}

	if (pObj1->GetNextValue().CompareNoCase(pObj2->GetNextValue()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_NEXT_VALUE);/*_T("Next Value")*/
		pObj->SetInfoSchema1 (pObj1->GetNextValue());
		pObj->SetInfoSchema2 (pObj2->GetNextValue());
		listDiff.AddTail(pObj);
	}

	if (pObj1->GetCacheSize().CompareNoCase(pObj2->GetCacheSize()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_CACHE);/*_T("Cache")*/
		pObj->SetInfoSchema1 (pObj1->GetCacheSize());
		pObj->SetInfoSchema2 (pObj2->GetCacheSize());
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareSynonym (CaVsdSynonym* pDiff, CaSynonymDetail* pObj1, CaSynonymDetail* pObj2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Owner:
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_OWNER);/*_T("Owner")*/
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	// Synonym of what ?:
	if (pObj1->GetOnType() != pObj2->GetOnType())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_TYPE);/*_T("Type")*/
		pObj->SetInfoSchema1 (pObj1->GetOnType());
		pObj->SetInfoSchema2 (pObj2->GetOnType());
		listDiff.AddTail(pObj);
	}

	// Synonym on Object:
	if (pObj1->GetOnObject().CompareNoCase(pObj2->GetOnObject()) != 0 ||
	    pObj1->GetOnObjectOwner().CompareNoCase(pObj2->GetOnObjectOwner()) != 0 )
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_SYNONYM_ON);/*_T("Synonym on")*/
		CString strFormat;
		strFormat.Format (_T("[%s].%s"), (LPCTSTR)pObj1->GetOnObjectOwner(), (LPCTSTR)pObj1->GetOnObject());

		pObj->SetInfoSchema1 (strFormat);
		strFormat.Format (_T("[%s].%s"), (LPCTSTR)pObj2->GetOnObjectOwner(), (LPCTSTR)pObj2->GetOnObject());
		pObj->SetInfoSchema2 (strFormat);
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareGrantee (CaVsdGrantee* pDiff, CaGrantee* pObj1, CaGrantee* pObj2)
{
	BOOL bOne = TRUE;
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	CTypedPtrList < CObList, CaPrivilegeItem* >& lp1 = pObj1->GetListPrivilege();
	CTypedPtrList < CObList, CaPrivilegeItem* >& lp2 = pObj2->GetListPrivilege();
	CString strDiff1 = _T("");
	CString strDiff2 = _T("");

	POSITION pos = lp1.GetHeadPosition();
	while (pos != NULL)
	{
		CaPrivilegeItem* pPriv = lp1.GetNext(pos);
		CaPrivilegeItem* pFound = CaPrivilegeItem::FindPrivilege(pPriv, lp2);
		if (!pFound)
		{
			if (!bOne)
				strDiff1 += _T(", ");
			if (pPriv->IsPrivilege())
			{
				if (pPriv->GetLimit() != -1)
				{
					CString strNameValue;
					strNameValue.Format(_T("(%s, %d)"), (LPCTSTR)pPriv->GetPrivilege(), pPriv->GetLimit());
					strDiff1 += strNameValue;
				}
				else
				{
					strDiff1 += pPriv->GetPrivilege();
				}
			}
			else
			{
				strDiff1 += _T("no ");
				strDiff1 += pPriv->GetPrivilege();
			}

			bOne = FALSE;
		}
	}

	bOne = TRUE;
	pos = lp2.GetHeadPosition();
	while (pos != NULL)
	{
		CaPrivilegeItem* pPriv = lp2.GetNext(pos);
		CaPrivilegeItem* pFound = CaPrivilegeItem::FindPrivilege(pPriv, lp1);
		if (!pFound)
		{
			if (!bOne)
				strDiff2 += _T(", ");
			if (pPriv->IsPrivilege())
			{
				if (pPriv->GetLimit() != -1)
				{
					CString strNameValue;
					strNameValue.Format(_T("(%s, %d)"), (LPCTSTR)pPriv->GetPrivilege(), pPriv->GetLimit());
					strDiff2 += strNameValue;
				}
				else
				{
					strDiff2 += pPriv->GetPrivilege();
				}
			}
			else
			{
				strDiff2 += _T("no ");
				strDiff2 += pPriv->GetPrivilege();
			}

			bOne = FALSE;
		}
	}

	CString csNotAvailable;
	if (!strDiff1.IsEmpty())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		csNotAvailable.LoadString(IDS_NOT_AVAILABLE);
		pObj->SetType (IDS_MISSING_PRIV);
		pObj->SetInfoSchema1 (strDiff1);
		pObj->SetInfoSchema2 (csNotAvailable);
		listDiff.AddTail(pObj);
	}
	if (!strDiff2.IsEmpty())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		csNotAvailable.LoadString(IDS_NOT_AVAILABLE);
		pObj->SetType (IDS_MISSING_PRIV);
		pObj->SetInfoSchema1 (csNotAvailable);
		pObj->SetInfoSchema2 (strDiff2);
		listDiff.AddTail(pObj);
	}
}

void VSD_CompareAlarm (CaVsdAlarm* pDiff, CaAlarm* pObj1, CaAlarm* pObj2)
{
	CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pDiff->GetDisplayInfo();
	CaVsdRefreshTreeInfo* pRefresh = (CaVsdRefreshTreeInfo*)pDisplayInfo->GetRefreshTreeInfo();
	BOOL bIgnoreOwner = (pRefresh->GetSchema1().CompareNoCase(pRefresh->GetSchema2()) != 0 && !pRefresh->GetSchema2().IsEmpty());
	CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pDiff->GetListDifference();
	// Owner:
	/* No Owner for alarm
	if (!bIgnoreOwner && pObj1->GetOwner() != pObj2->GetOwner())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (_T("Owner"));
		pObj->SetInfoSchema1 (pObj1->GetOwner());
		pObj->SetInfoSchema2 (pObj2->GetOwner());
		listDiff.AddTail(pObj);
	}
	*/

	// Subject type:
	if (pObj1->GetSubjectType() != pObj2->GetSubjectType())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_SUBJECT_TYPE);/*_T("Subject Type")*/
		pObj->SetInfoSchema1 (pObj1->GetSubjectType());
		pObj->SetInfoSchema2 (pObj2->GetSubjectType());
		listDiff.AddTail(pObj);
	}
	// Security User:
	if (pObj1->GetSecurityUser().CompareNoCase(pObj2->GetSecurityUser()) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_SECURITY_USER);/*_T("Security user")*/
		pObj->SetInfoSchema1 (pObj1->GetSecurityUser());
		pObj->SetInfoSchema2 (pObj2->GetSecurityUser());
		listDiff.AddTail(pObj);
	}
	// If success|failure:
	CString strText1 = _T("");
	CString strText2 = _T("");

	if (pObj1->GetIfSuccess())
		strText1 += _T("success");
	if (pObj1->GetIfFailure())
	{
		if (!strText1.IsEmpty())
			strText1 += _T(" | failure");
		else
			strText1 += _T("failure");
	}
	if (pObj2->GetIfSuccess())
		strText2 += _T("success");
	if (pObj2->GetIfFailure())
	{
		if (!strText2.IsEmpty())
			strText2 += _T(" | failure");
		else
			strText2 += _T("failure");
	}
	if (strText1.CompareNoCase(strText2) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_ALARM_CONDITION);/*_T("Alarm condition")*/
		pObj->SetInfoSchema1 (strText1);
		pObj->SetInfoSchema2 (strText2);
		listDiff.AddTail(pObj);
	}

	// Alarm event action:
	BOOL bOne = TRUE;
	strText1 = _T("");
	strText2 = _T("");
	CStringList& le1 = pObj1->GetListEventAction();
	CStringList& le2 = pObj2->GetListEventAction();
	POSITION pos = le1.GetHeadPosition();
	while (pos != NULL)
	{
		CString strAction = le1.GetNext(pos);
		if (le2.Find(strAction) == NULL)
		{
			if (!bOne)
				strText1 += _T(", ");
			strText1 += strAction;
			bOne = FALSE;
		}
	}

	bOne = TRUE;
	pos = le2.GetHeadPosition();
	while (pos != NULL)
	{
		CString strAction = le2.GetNext(pos);
		if (le1.Find(strAction) == NULL)
		{
			if (!bOne)
				strText2 += _T(", ");
			strText2 += strAction;
			bOne = FALSE;
		}
	}
	CString csTemp;
	if (!strText1.IsEmpty())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		csTemp.LoadString(IDS_NOT_AVAILABLE);
		pObj->SetType (IDS_MISSING_ALARM);
		pObj->SetInfoSchema1 (strText1);
		pObj->SetInfoSchema2 (csTemp);
		listDiff.AddTail(pObj);
	}
	if (!strText2.IsEmpty())
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		csTemp.LoadString(IDS_NOT_AVAILABLE);
		pObj->SetType (IDS_MISSING_ALARM);
		pObj->SetInfoSchema1 (csTemp);
		pObj->SetInfoSchema2 (strText2);
		listDiff.AddTail(pObj);
	}

	strText1.Format(_T("[%s].%s"), (LPCTSTR)pObj1->GetDBEventOwner(), (LPCTSTR)pObj1->GetDBEvent());
	strText2.Format(_T("[%s].%s"), (LPCTSTR)pObj2->GetDBEventOwner(), (LPCTSTR)pObj2->GetDBEvent());
	if (VSD_CompareWithIgnoreWhiteSpaces(strText1,strText2) != 0)
	{
		CaVsdDifference* pObj = new CaVsdDifference();
		pObj->SetType (IDS_TYPE_ALARM_RAISES);/*_T("Alarm raises DBEvent")*/
		pObj->SetInfoSchema1 (strText1);
		pObj->SetInfoSchema2 (strText2);
		listDiff.AddTail(pObj);
	}
}
