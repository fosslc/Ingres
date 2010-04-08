/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : vsddml.h, Header File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Data manipulation: access the lowlevel data through 
**            Library or COM module.
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 22-Apr-2003 (schph01)
**    SIR 107523 Add define for VSD_CompareSequence() function.
*/

#if !defined (_VSDDML_HEADER)
#define _VSDDML_HEADER
#include "qryinfo.h" 
#include "usrexcep.h"
#include "vsdadoc.h"
#include "vsdtree.h"

void VSD_HasDiscadItem(CtrfItemData* pItem, BOOL& bHas, BOOL bLookupInSubFolderOnly);
BOOL VSD_Installation(LPCTSTR lpszFile);

void VSD_CompareUser (CaVsdUser* pDiff, CaUserDetail* pObj1, CaUserDetail* pObj2);
void VSD_CompareGroup (CaVsdGroup* pDiff, CaGroupDetail* pObj1, CaGroupDetail* pObj2);
void VSD_CompareRole (CaVsdRole* pDiff, CaRoleDetail* pObj1, CaRoleDetail* pObj2);
void VSD_CompareProfile (CaVsdProfile* pDiff, CaProfileDetail* pObj1, CaProfileDetail* pObj2);
void VSD_CompareLocation (CaVsdLocation* pDiff, CaLocationDetail* pObj1, CaLocationDetail* pObj2);
void VSD_CompareDatabase (CaVsdDatabase* pDiff, CaDatabaseDetail* pObj1, CaDatabaseDetail* pObj2);
void VSD_CompareTable (CaVsdTable* pDiff, CaTableDetail* pTable1, CaTableDetail* pTable2);
void VSD_CompareIndex (CaVsdIndex* pDiff, CaIndexDetail* pObj1, CaIndexDetail* pObj2);
void VSD_CompareRule (CaVsdRule* pDiff, CaRuleDetail* pObj1, CaRuleDetail* pObj2);
void VSD_CompareIntegrity (CaVsdIntegrity* pDiff, CaIntegrityDetail* pObj1, CaIntegrityDetail* pObj2);

void VSD_CompareView (CaVsdView* pDiff, CaViewDetail* pObj1, CaViewDetail* pObj2);
void VSD_CompareDBEvent (CaVsdDBEvent* pDiff, CaDBEventDetail* pObj1, CaDBEventDetail* pObj2);
void VSD_CompareProcedure (CaVsdProcedure* pDiff, CaProcedureDetail* pObj1, CaProcedureDetail* pObj2);
void VSD_CompareSynonym (CaVsdSynonym* pDiff, CaSynonymDetail* pObj1, CaSynonymDetail* pObj2);
void VSD_CompareGrantee (CaVsdGrantee* pDiff, CaGrantee* pObj1, CaGrantee* pObj2);
void VSD_CompareAlarm (CaVsdAlarm* pDiff, CaAlarm* pObj1, CaAlarm* pObj2);
void VSD_CompareSequence (CaVsdSequence* pDiff, CaSequenceDetail* pObj1, CaSequenceDetail* pObj2);

#endif // _VSDDML_HEADER