/******************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Visual DBA
**
**    Source : dbasqlv2.c
**    sql build functions and some internal statics one
**
**    Author : Sotheavut UK
**
**    History after 04-May-1999:
**
**    29-June-1999 (schph01)
**      bug #97072 : impact on "alter table": only generate the "with null" syntax
**      when adding a column in "alter table", since the "with [no] default" syntax
**      is normally irrelevant in this case, and the interface has been modified
**      accordingly, so that the 2 visual controls for nullability don't appear
**      any more in "alter table"
**    02-Jul-1999 (schph01)
**      bug #97690 :
**      Generate the syntax for decimal column data type without
**      precision and scale when no values are defined in the structure.
**    06-Jul-1999 (schph01)
**      Fixed bug 97688 :
**        Generate the syntax for "Byte Varying" with the length define in
**        the structure
**    03-Mar-2000 (noifr01)
**     (bug 100695) if journaling is specified as being on, generate the
**     corresponding syntax in any case, because it is not the default (as 
**     apparently previously documented) if the default journaling has been
**     set to off for the DBMS servers parameters in [V]CBF
**    08-Mar-2000 (uk$so01)
**       BUG #100790:
**       Missising to copy of Index enhancement structure to the 
**       create unique key when calling from the alter table dialog box.
**    13-Jun-2000 (schph01)
**       Bug #101825
**       IsVDBA25xIndexEnforcementAvailable() return true only when the DBMS
**       version is ingres 2.5.
**    08-Dec-2000 (schph01)
**       SIR 102823
**       Add syntax "with system_maintained" in fonction VDBA20xColumnFormat()
**    15-Dec-2000(schph01)
**       SIR 102819 :  Generate the "with clause" when the checkbox
**       "Create Table As Select" is checked.
**    26-Mar-2001 (noifr01)
**       (sir 104270) removal of code for managing Ingres/Desktop
**    30-Mar-2001 (noifr01)
**       (sir 104378) differentiation of II 2.6.
**    21-Jun-2001 (schph01)
**       (SIR 103694) STEP 2 support of UNICODE datatypes.
**    18-Sep-2001 (schph01)
**       (BUG 105386) remove the "default" syntax generated for
**       type long byte, long varchar or long nvarchar.
**    15-Jan-2002 (schph01)
**       (BUG 106802) The Index Constraint Syntax whith key word 'no index'
**       is generated 'with no index' instead 'with ( no index )'
**    15-Jan-2002 (schph01)
**       (BUG 106801) to generate the Primary key Constraint syntax whith the index
**       type 'bases table structure' quoteifneeded() is not necessary.
**    12-May-2004 (schph01)
**      SIR #111507 Add management for new column type bigint
**    17-May-2004 (schph01)
**      (SIR 112514) Add management for Alter column syntax.
**    23-Mar-2010 (drivi01)
**      Add routine for creation of VectorWise table.
**    02-Jun-2010 (drivi01)
**      Update szCol to use MAXOBJECTNAME instead of hardcoded value.
*******************************************************************************/
#include <assert.h>
#include "dba.h"
#include "dbaset.h"
#include "dbaparse.h"
#include "error.h"
#include "dbadlg1.h"
#include "dbaginfo.h"
#include "rmcmd.h"
#include "domdata.h"
#include "msghandl.h"
#include "extccall.h"

static LPTSTR VDBA20xPrimaryKeySyntax (PRIMARYKEYSTRUCT* pPrimaryKey, int* m);
static LPTSTR VDBA20xForeignKeySyntax (LPTSTR lpszTbName, LPOBJECTLIST  lpReferences, int* m);
static LPTSTR VDBA20xUniqueKeySyntax  (LPTLCUNIQUE lpUnique, int* m);
static LPTSTR VDBA20xCheckSyntax      (LPTLCCHECK  lpCheck, int* m);
static LPTSTR VDBA20xIndexConstraintSyntax (INDEXOPTION* pIndex);

static BOOL LocalVdba20xTablePrimaryKey_Equal (PRIMARYKEYSTRUCT* pPrimaryKey1, PRIMARYKEYSTRUCT* pPrimaryKey2);
static BOOL VDBA20xTableUniqueKey_Equal  (LPTLCUNIQUE lpU1,    LPTLCUNIQUE lpU2);
static BOOL VDBA20xTableCheck_Equal      (LPTLCCHECK  lpCh1,   LPTLCCHECK lpCh2);


static BOOL IsVDBA25xIndexEnforcementAvailable()
{
    return (GetOIVers() >= OIVERS_25)? TRUE: FALSE;
}

LPOBJECTLIST VDBA20xTableReferences_CopyList (LPOBJECTLIST lpRef)
{
    LPOBJECTLIST o, obj, list = NULL, ls = lpRef;
    LPREFERENCEPARAMS lpRefP;
    LPREFERENCECOLS   lpCol;
    if (!lpRef)
        return NULL;
    while (ls)
    {
        lpRefP = (LPREFERENCEPARAMS)ls->lpObject;
        obj = AddListObjectTail (&list, sizeof (REFERENCEPARAMS));
        if (obj)
        {
            LPOBJECTLIST lx;
            LPREFERENCEPARAMS lpr = (LPREFERENCEPARAMS)obj->lpObject;
            lpr->lpColumns = NULL;
            lstrcpy (lpr->szConstraintName, lpRefP->szConstraintName);
            lstrcpy (lpr->szSchema,         lpRefP->szSchema);
            lstrcpy (lpr->szTable,          lpRefP->szTable);
            lstrcpy (lpr->szRefConstraint,  lpRefP->szRefConstraint);
            lpr->bAlter = lpRefP->bAlter;
            lpr->bUseDeleteAction = lpRefP->bUseDeleteAction;
            lpr->bUseUpdateAction = lpRefP->bUseUpdateAction;
            lstrcpy (lpr->tchszDeleteAction,  lpRefP->tchszDeleteAction);
            lstrcpy (lpr->tchszUpdateAction,  lpRefP->tchszUpdateAction);
            INDEXOPTION_COPY (&(lpr->constraintWithClause), &(lpRefP->constraintWithClause));

            lx = lpRefP->lpColumns;
            while (lx)
            {
                lpCol = (LPREFERENCECOLS)lx->lpObject;
                o     = AddListObjectTail (&(lpr->lpColumns), sizeof (REFERENCECOLS));
                if (o)
                {
                    LPREFERENCECOLS lpc = (LPREFERENCECOLS)o->lpObject;
                    lstrcpy (lpc->szRefingColumn, lpCol->szRefingColumn);
                    lstrcpy (lpc->szRefedColumn,  lpCol->szRefedColumn);
                } 
                else
                {
                    list = VDBA20xTableReferences_Done (list);
                    ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                    return NULL;
                }
                lx = lx->lpNext;
            }
        }
        else
        {
            list = VDBA20xTableReferences_Done (list);
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return NULL;
        }
        ls = ls->lpNext;
    }
    return list;
}


LPTLCUNIQUE  VDBA20xTableUniqueKey_CopyList  (LPTLCUNIQUE  lpList)
{
    LPTLCUNIQUE obj, ls = lpList, list = NULL;
    
    while (ls)
    {
        obj = (LPTLCUNIQUE)ESL_AllocMem (sizeof (TLCUNIQUE));
        if (!obj)
        {
             list = VDBA20xTableUniqueKey_Done (list);
             ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
             return NULL;
        }
        obj->lpcolumnlist = StringList_Copy (ls->lpcolumnlist);
        obj->bAlter = ls->bAlter;
        lstrcpy (obj->tchszConstraint, ls->tchszConstraint);
        INDEXOPTION_COPY (&(obj->constraintWithClause), &(ls->constraintWithClause));

        if (!obj->lpcolumnlist)
        {
             list = VDBA20xTableUniqueKey_Done (list);
             ESL_FreeMem (obj);
             ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
             return NULL;
        }
        list = VDBA20xTableUniqueKey_Add (list, obj);
        ls = ls->next;
    }
    return list;
}

LPTLCCHECK   VDBA20xTableCheck_CopyList (LPTLCCHECK lpList)
{
    LPTLCCHECK obj, ls = lpList, list = NULL;

    while (ls)
    {
        obj = (LPTLCCHECK)ESL_AllocMem (sizeof (TLCCHECK));
        if (!obj)
        {
             list = VDBA20xTableCheck_Done (list);
             ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
             return NULL;
        }
        lstrcpy (obj->tchszConstraint, ls->tchszConstraint);
        obj->lpexpression = (LPTSTR)ESL_AllocMem (lstrlen (ls->lpexpression)+1);
        if (!obj->lpexpression)
        {
             list = VDBA20xTableCheck_Done (list);
             ESL_FreeMem (obj->lpexpression);
             ESL_FreeMem (obj);
             ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
             return NULL;
        }
        lstrcpy (obj->lpexpression, ls->lpexpression);
        obj->bAlter = ls->bAlter;
        list = VDBA20xTableCheck_Add (list, obj);
        ls = ls->next;
    }
    return list;
}


// ------------------------------------------------------------------------------------------------
//
// VDBA20X UK.S

LPCOLUMNPARAMS VDBA20xTableColumn_Search (LPOBJECTLIST lpColumn, LPCTSTR lpszColumn)
{
    LPCOLUMNPARAMS lpCol;
    LPOBJECTLIST ls = lpColumn;
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        ls = ls->lpNext;
        if (lstrcmpi ((LPCTSTR)lpCol->szColumn, lpszColumn) == 0)
            return lpCol;
    }
    return NULL;
}

/*--------------------------------------------- VDBA20xTableColumn_SortedAdd -*/
/* Add a new object into the linked list.                                     */
/* Order by obj->nPrimary.                                                    */
/*                                                                            */
/* Parameter:                                                                 */
/*     1) first : Pointer to the first object in the list.                    */
/*     2) obj   : Object to be added to the list.                             */
/*                                                                            */
/* RETURN: Pointer to the first object in the list.                           */
/*----------------------------------------------------------------------------*/
LPCOLUMNPARAMS
VDBA20xTableColumn_SortedAdd (LPCOLUMNPARAMS first, LPCOLUMNPARAMS obj)
{
    LPCOLUMNPARAMS ls = first, Prec = NULL;
  
    while (ls && (obj->nPrimary > ls->nPrimary))
    {
        Prec = ls;
        ls = ls->next;
    }       
    if (Prec == NULL)
    {   //
        // Add abject at head.  
        //
        obj->next = first;
        first     = obj;
    }
    else
    if (ls == NULL)
    {   //
        // Add abject at tail.  
        //
        obj->next  = NULL;
        Prec->next = obj; 
    }
    else
    {   //
        // Add abject in the midle of the list.
        //
        Prec->next = obj;
        obj->next  = ls;
    }
    return first;
}

LPCOLUMNPARAMS
VDBA20xTableColumn_Done (LPCOLUMNPARAMS first)
{
    LPCOLUMNPARAMS ls = first, lpTmp;
  
    while (ls)
    {
        lpTmp = ls;
        ls = ls->next;
        ESL_FreeMem (lpTmp);
    } 
    return NULL;
}

LPOBJECTLIST VDBA20xColumnList_Copy (LPOBJECTLIST first)
{
    LPCOLUMNPARAMS lpObj, lpNewObj;
    LPOBJECTLIST lpColTemp;
    LPOBJECTLIST lpList = NULL, ls = first;

    while (ls)
    {
        lpColTemp = AddListObjectTail(&lpList, sizeof(COLUMNPARAMS));
        if (!lpColTemp)
        {   //
            // Need to free previously allocated objects.
            FreeObjectList(lpList);
            lpList = NULL;
            return NULL;
        }
        lpNewObj = (LPCOLUMNPARAMS)lpColTemp->lpObject;
        lpObj    = (LPCOLUMNPARAMS)ls->lpObject;
        //
        // Copy all non-pointer members:
        memcpy (lpNewObj,  lpObj, sizeof(COLUMNPARAMS));
        if (lpObj->lpszCheck)
        {
            lpNewObj->lpszCheck = ESL_AllocMem (lstrlen (lpObj->lpszCheck)+1);
            if (lpNewObj->lpszCheck)
                lstrcpy (lpNewObj->lpszCheck, lpObj->lpszCheck);
        }
        if (lpObj->lpszDefSpec)
        {
            lpNewObj->lpszDefSpec = ESL_AllocMem (lstrlen (lpObj->lpszDefSpec)+1);
            if (lpNewObj->lpszDefSpec)
                lstrcpy (lpNewObj->lpszDefSpec, lpObj->lpszDefSpec);
        }
        ls = (LPOBJECTLIST)ls->lpNext;
    }
    return lpList;
}

LPOBJECTLIST VDBA20xColumnList_Done (LPOBJECTLIST first)
{
    LPOBJECTLIST ls = first, lpTmp;
    LPCOLUMNPARAMS lpCol;
    while (ls)
    {
        lpTmp = ls;
        ls = ls->lpNext;
        lpCol = (LPCOLUMNPARAMS)lpTmp->lpObject;
        if (lpCol && lpCol->lpszCheck)
            ESL_FreeMem (lpCol->lpszCheck);
        if (lpCol && lpCol->lpszDefSpec)
            ESL_FreeMem (lpCol->lpszDefSpec);
    } 
    if (first)
        FreeObjectList (first);
    return NULL;
}


/*----------------------------------------------------- TableColumn_Included -*/
/* Test of inclusion.                                                         */
/*                                                                            */
/* Parameter:                                                                 */
/*     1) lpCol1  : Pointer to the first object in the list.                  */
/*     2) lpCol2  : Pointer to the first object in the list.                  */
/*                                                                            */
/* RETURN: TRUE if (lpCol1 is include in lpCol2)                              */
/*         ie all elements of lpCol1 are in lpCol2.                           */
/*----------------------------------------------------------------------------*/
BOOL 
VDBA20xTableColumn_Included (LPOBJECTLIST lpCol1, LPOBJECTLIST lpCol2)
{
    LPOBJECTLIST   l1;
    LPCOLUMNPARAMS lpCol, lpObj;

    if (lpCol1 == lpCol2)    // Only if they are NULL, or auto matching
        return TRUE;
    if (lpCol1 == NULL || lpCol2 == NULL)
        return FALSE;
    if (CountListItems (lpCol1) > CountListItems (lpCol2))
        return FALSE;

    l1 = lpCol1;
    while (l1)
    {
        lpCol = (LPCOLUMNPARAMS)l1->lpObject;
        lpObj = VDBA20xTableColumn_Search (lpCol2, lpCol->szColumn);
        if (!lpObj)
            return FALSE;
        l1 = l1->lpNext;
    }
    return TRUE;
}

static BOOL LocalVdba20xTablePrimaryKey_Equal (PRIMARYKEYSTRUCT* pPrimaryKey1, PRIMARYKEYSTRUCT* pPrimaryKey2)
{
    if (StringList_Equal (pPrimaryKey1->pListKey, pPrimaryKey2->pListKey))
        return TRUE;
    return FALSE;
}

//
// Check to see if 2 foreign keys constraints are equal.
// lpCol1, lpCol2 are the list of REFERENCEPARAMS
//
static BOOL VDBA20xTableForeignKey_Equal (LPOBJECTLIST lpCol1, LPOBJECTLIST lpCol2)
{
    LPREFERENCEPARAMS lpC1, lpC2, lpRef;
    LPOBJECTLIST      ls;
    int iC1, iC2;
    lpC1 = (LPREFERENCEPARAMS)lpCol1;
    lpC2 = (LPREFERENCEPARAMS)lpCol2;

    iC1 = CountListItems (lpCol1);
    iC2 = CountListItems (lpCol2);
    if (iC1 != iC2)
        return FALSE;
    ls = lpCol1;
    while (ls)
    {
        lpRef = (LPREFERENCEPARAMS)ls->lpObject;
        if (!VDBA20xForeignKey_Lookup (lpCol2,  lpRef))
            return FALSE;
        ls = ls->lpNext;
    }
    ls = lpCol2;
    while (ls)
    {
        lpRef = (LPREFERENCEPARAMS)ls->lpObject;
        if (!VDBA20xForeignKey_Lookup (lpCol1,  lpRef))
            return FALSE;
        ls = ls->lpNext;
    }
    return TRUE;
}



BOOL VDBA20xTable_Equal (LPTABLEPARAMS lpTS1, LPTABLEPARAMS lpTS2)
{
    if (lpTS1 == lpTS2)    // Only if they are NULL, or auto matching
        return TRUE;
    if (lpTS1 == NULL || lpTS2 == NULL)
        return FALSE;
    //
    // General Table Properties
    if (lstrcmpi (lpTS1->DBName, lpTS2->DBName) != 0)
        return FALSE;
    if (lstrcmpi (lpTS1->objectname, lpTS2->objectname) != 0)
        return FALSE;
    if (lstrcmpi (lpTS1->szSchema, lpTS2->szSchema) != 0)
        return FALSE;
    //
    // Matching the Table Column Names (Ignore the Column Attributes)
    if (!(VDBA20xTableColumn_Included (lpTS1->lpColumns, lpTS2->lpColumns) &&
          VDBA20xTableColumn_Included (lpTS2->lpColumns, lpTS1->lpColumns)))
        return FALSE;
    //
    // Matching the Table Primary Key
    if (!LocalVdba20xTablePrimaryKey_Equal (&(lpTS1->primaryKey), &(lpTS2->primaryKey)))
        return FALSE;
    //
    // Matching the Table Foreign Keys
    if (!VDBA20xTableForeignKey_Equal (lpTS1->lpReferences, lpTS2->lpReferences))
        return FALSE;
    //
    // Matching the Table Unique Key
    if (!VDBA20xTableUniqueKey_Equal (lpTS1->lpUnique, lpTS2->lpUnique))
        return FALSE;
    //
    // Matching the Table Check Constraint
    if (!VDBA20xTableCheck_Equal (lpTS1->lpCheck, lpTS2->lpCheck))
        return FALSE;
    return TRUE;
}

//
// Caller will take care of destroying the return string
// when it is no longer used.
LPTSTR VDBA20xAlterColumnFormat (LPCOLUMNPARAMS lpCol, LPOBJECTLIST lpReferences, LPTABLEPARAMS lpTS)
{
    //
    // This function does not generate the column primary key constraint !!!.
    // The unique and check constraints doesn't available for Alter column syntax,
    // for the column level reference constraint.
    // Restriction default 'value' not available for Alter Column Syntax.
    int m;
    LPTSTR lpString = NULL;
    TCHAR  tchszType [64];
    BOOL   bGenerateDefault = TRUE;
    BOOL   bUsueSubDlg = FALSE;
    LPTLCUNIQUE ls  = lpTS->lpUnique;
    if (!lpCol && !lpCol->szColumn[0])
        return NULL;
    m  = ConcateStrings (&lpString, (LPTSTR)QuoteIfNeeded(lpCol->szColumn), " ", (LPTSTR)0);
    if (!m) return NULL;
    //
    // Generate the data type.
    switch (lpCol->uDataType)
    {
    case INGTYPE_C:
        wsprintf (tchszType, "c (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_CHAR:      
        wsprintf (tchszType, "char (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_TEXT:
        wsprintf (tchszType, "text (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_VARCHAR:
        wsprintf (tchszType, "varchar (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_LONGVARCHAR:
        m = ConcateStrings (&lpString, "long varchar ", (LPTSTR)0);
        bGenerateDefault = FALSE;
        break;
    case INGTYPE_BIGINT:
        m = ConcateStrings (&lpString, "bigint ", (LPTSTR)0);
        break;
    case INGTYPE_INT8:
        m = ConcateStrings (&lpString, "int8 ", (LPTSTR)0);
        break;
    case INGTYPE_INT4:
        m = ConcateStrings (&lpString, "int ", (LPTSTR)0);
        break;
    case INGTYPE_INT2:
        m = ConcateStrings (&lpString, "smallint ", (LPTSTR)0);
        break;
    case INGTYPE_INT1: 
         m = ConcateStrings (&lpString, "integer1 ", (LPTSTR)0);
        break;
    case INGTYPE_DECIMAL:
        if (lpCol->lenInfo.decInfo.nPrecision == 0 && lpCol->lenInfo.decInfo.nScale == 0)
            wsprintf (tchszType, "decimal ");
        else
            wsprintf (tchszType, "decimal (%d, %d) ", lpCol->lenInfo.decInfo.nPrecision, lpCol->lenInfo.decInfo.nScale);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_FLOAT8:  
        m = ConcateStrings (&lpString, "float ", (LPTSTR)0);
        break;
    case INGTYPE_FLOAT4: 
        m = ConcateStrings (&lpString, "real ", (LPTSTR)0);
        break;
    case INGTYPE_DATE:  
        m = ConcateStrings (&lpString, "date ", (LPTSTR)0);
        break;
    case INGTYPE_ADTE:  
        m = ConcateStrings (&lpString, "ansidate ", (LPTSTR)0);
        break;
    case INGTYPE_TMWO:  
        m = ConcateStrings (&lpString, "time without time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TMW:  
        m = ConcateStrings (&lpString, "time with time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TME:  
        m = ConcateStrings (&lpString, "time with local time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TSWO:  
        m = ConcateStrings (&lpString, "timestamp without time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TSW:  
        m = ConcateStrings (&lpString, "timestamp with time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TSTMP:  
        m = ConcateStrings (&lpString, "timestamp with local time zone ", (LPTSTR)0);
        break;
    case INGTYPE_INYM:  
        m = ConcateStrings (&lpString, "interval year to month ", (LPTSTR)0);
        break;
    case INGTYPE_INDS:  
        m = ConcateStrings (&lpString, "interval day to second ", (LPTSTR)0);
        break;
    case INGTYPE_IDTE:  
        m = ConcateStrings (&lpString, "ingresdate ", (LPTSTR)0);
        break;
    case INGTYPE_MONEY: 
        m = ConcateStrings (&lpString, "money ", (LPTSTR)0);
        break;
    case INGTYPE_BYTE:
        wsprintf (tchszType, "byte (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_BYTEVAR:
        wsprintf (tchszType, "byte varying (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_LONGBYTE: 
        m = ConcateStrings (&lpString, "long byte ", (LPTSTR)0);
        bGenerateDefault = FALSE;
        break;
    case INGTYPE_OBJKEY: 
        m = ConcateStrings (&lpString, "object_key ", (LPTSTR)0);
        break;
    case INGTYPE_TABLEKEY:
        m = ConcateStrings (&lpString, "table_key ", (LPTSTR)0);
        break;
    case INGTYPE_FLOAT: 
        m = ConcateStrings (&lpString, "float ", (LPTSTR)0);
        break;
    case INGTYPE_UNICODE_NCHR:
        wsprintf (tchszType, "nchar (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_UNICODE_NVCHR:
        wsprintf (tchszType, "nvarchar (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_UNICODE_LNVCHR:
        m = ConcateStrings (&lpString, "long nvarchar ", (LPTSTR)0);
        bGenerateDefault = FALSE;
        break;
    }
    if (!m) return NULL;
    //
    // with null
    if (lpCol->bNullable && !lpCol->bUnique)
    {
        m = ConcateStrings (&lpString, "with null ", (LPTSTR)0);
        if (!m) return NULL;
    }
    // 
    // not null (cannot be 'not null unique' because if unique is declared, it would have
    // already been generated.
    if (!lpCol->bNullable && !lpCol->bUnique)
    {
        m = ConcateStrings (&lpString, "not null ", (LPTSTR)0);
        if (!m) return NULL;
    }
    // 
    // with default
    if (bGenerateDefault)
    {
        if (lpCol->bDefault)
        {
            m = ConcateStrings (&lpString, "with default ", (LPTSTR)0);
            if (!m) return NULL;
        }
        else
        {
            BOOL bGenericGateway = FALSE;
            if (GetOIVers() == OIVERS_NOTOI)
                bGenericGateway = TRUE;
            if (!bGenericGateway) {
                if ( lpTS->bCreate )
                {
                    //
                    // not default
                    m = ConcateStrings (&lpString, "not default ", (LPTSTR)0);
                    if (!m) return NULL;
                }
            }
        }
    }

    return lpString;
}
//
// Caller will take care of destroying the return string
// when it is no longer used.
LPTSTR VDBA20xColumnFormat (LPCOLUMNPARAMS lpCol, LPOBJECTLIST lpReferences, LPTABLEPARAMS lpTS)
{
    //
    // This function does not generate the column primary key constraint !!!.
    int m;
    LPREFERENCEPARAMS lpRef;
    LPREFERENCECOLS   lpRefCol;
    LPTSTR lpString = NULL;
    LPOBJECTLIST list, lpFKey = lpReferences;
    TCHAR  tchszType [64];
    BOOL   bGenerateDefault = TRUE;
    BOOL   bUsueSubDlg = FALSE;
    LPTLCUNIQUE ls  = lpTS->lpUnique;
    if (!lpCol && !lpCol->szColumn[0])
        return NULL;
    m  = ConcateStrings (&lpString, (LPTSTR)QuoteIfNeeded(lpCol->szColumn), " ", (LPTSTR)0);
    if (!m) return NULL;
    //
    // Generate the data type.
    switch (lpCol->uDataType)
    {
    case INGTYPE_C:
        wsprintf (tchszType, "c (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_CHAR:      
        wsprintf (tchszType, "char (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_TEXT:
        wsprintf (tchszType, "text (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_VARCHAR:
        wsprintf (tchszType, "varchar (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_LONGVARCHAR:
        m = ConcateStrings (&lpString, "long varchar ", (LPTSTR)0);
        bGenerateDefault = FALSE;
        break;
    case INGTYPE_BIGINT:
        m = ConcateStrings (&lpString, "bigint ", (LPTSTR)0);
        break;
    case INGTYPE_INT8:
        m = ConcateStrings (&lpString, "int8 ", (LPTSTR)0);
        break;
    case INGTYPE_INT4:
        m = ConcateStrings (&lpString, "int ", (LPTSTR)0);
        break;
    case INGTYPE_INT2:
        m = ConcateStrings (&lpString, "smallint ", (LPTSTR)0);
        break;
    case INGTYPE_INT1: 
         m = ConcateStrings (&lpString, "integer1 ", (LPTSTR)0);
        break;
    case INGTYPE_DECIMAL:
        if (lpCol->lenInfo.decInfo.nPrecision == 0 && lpCol->lenInfo.decInfo.nScale == 0)
            wsprintf (tchszType, "decimal ");
        else
            wsprintf (tchszType, "decimal (%d, %d) ", lpCol->lenInfo.decInfo.nPrecision, lpCol->lenInfo.decInfo.nScale);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_FLOAT8:  
        m = ConcateStrings (&lpString, "float ", (LPTSTR)0);
        break;
    case INGTYPE_FLOAT4: 
        m = ConcateStrings (&lpString, "real ", (LPTSTR)0);
        break;
    case INGTYPE_DATE:  
        m = ConcateStrings (&lpString, "date ", (LPTSTR)0);
        break;
    case INGTYPE_ADTE:  
        m = ConcateStrings (&lpString, "ansidate ", (LPTSTR)0);
        break;
    case INGTYPE_TMWO:  
        m = ConcateStrings (&lpString, "time without time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TMW:  
        m = ConcateStrings (&lpString, "time with time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TME:  
        m = ConcateStrings (&lpString, "time with local time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TSWO:  
        m = ConcateStrings (&lpString, "timestamp without time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TSW:  
        m = ConcateStrings (&lpString, "timestamp with time zone ", (LPTSTR)0);
        break;
    case INGTYPE_TSTMP:  
        m = ConcateStrings (&lpString, "timestamp with local time zone ", (LPTSTR)0);
        break;
    case INGTYPE_INYM:  
        m = ConcateStrings (&lpString, "interval year to month ", (LPTSTR)0);
        break;
    case INGTYPE_INDS:  
        m = ConcateStrings (&lpString, "interval day to second ", (LPTSTR)0);
        break;
    case INGTYPE_IDTE:  
        m = ConcateStrings (&lpString, "ingresdate ", (LPTSTR)0);
        break;
    case INGTYPE_MONEY: 
        m = ConcateStrings (&lpString, "money ", (LPTSTR)0);
        break;
    case INGTYPE_BYTE:
        wsprintf (tchszType, "byte (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_BYTEVAR:
        wsprintf (tchszType, "byte varying (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_LONGBYTE: 
        m = ConcateStrings (&lpString, "long byte ", (LPTSTR)0);
        bGenerateDefault = FALSE;
        break;
    case INGTYPE_OBJKEY: 
        m = ConcateStrings (&lpString, "object_key ", (LPTSTR)0);
        break;
    case INGTYPE_TABLEKEY:
        m = ConcateStrings (&lpString, "table_key ", (LPTSTR)0);
        break;
    case INGTYPE_FLOAT: 
        m = ConcateStrings (&lpString, "float ", (LPTSTR)0);
        break;
    case INGTYPE_UNICODE_NCHR:
        wsprintf (tchszType, "nchar (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_UNICODE_NVCHR:
        wsprintf (tchszType, "nvarchar (%ld) ", lpCol->lenInfo.dwDataLen);
        m = ConcateStrings (&lpString, tchszType, (LPTSTR)0);
        break;
    case INGTYPE_UNICODE_LNVCHR:
        m = ConcateStrings (&lpString, "long nvarchar ", (LPTSTR)0);
        bGenerateDefault = FALSE;
        break;
    }
    if (!m) return NULL;
    //
    // Unique column must declare as not null !!!
    // Generate the column-level unique constraint only if it's not defined by using the Unique Sub-Dialog.
    ls = lpTS->lpUnique;
    while (ls)
    {
        if (StringList_Count (ls->lpcolumnlist) == 1 && lstrcmpi ((ls->lpcolumnlist)->lpszString, lpCol->szColumn) == 0)
        {
            bUsueSubDlg = TRUE;
            break;
        }
        ls = ls->next;
    }
    if (lpCol->bUnique && !bUsueSubDlg)
    {
        m = ConcateStrings (&lpString, "not null unique ", (LPTSTR)0);
        if (!m) return NULL;
    }
    if (lpCol->bUnique && bUsueSubDlg)
    {
        // Just specify not null, the constraint will be generated later at table level.
        m = ConcateStrings (&lpString, "not null ", (LPTSTR)0);
        if (!m) return NULL;
    }
    //
    // with null
    if (lpCol->bNullable && !lpCol->bUnique)
    {
        m = ConcateStrings (&lpString, "with null ", (LPTSTR)0);
        if (!m) return NULL;
    }
    // 
    // not null (cannot be 'not null unique' because if unique is declared, it would have
    // already been generated.
    if (!lpCol->bNullable && !lpCol->bUnique)
    {
        m = ConcateStrings (&lpString, "not null ", (LPTSTR)0);
        if (!m) return NULL;
    }
    // 
    // with default
    if (bGenerateDefault)
    {
        if (lpCol->bDefault)
        {
            if (lpCol->lpszDefSpec)
                m = ConcateStrings (&lpString, "default ", lpCol->lpszDefSpec, " ", (LPTSTR)0);
            else
                m = ConcateStrings (&lpString, "with default ", (LPTSTR)0);
            if (!m) return NULL;
        }
        else
        {
            BOOL bGenericGateway = FALSE;
            if (GetOIVers() == OIVERS_NOTOI)
                bGenericGateway = TRUE;
            if (!bGenericGateway) {
                if ( lpTS->bCreate )
                {
                    //
                    // not default
                    m = ConcateStrings (&lpString, "not default ", (LPTSTR)0);
                    if (!m) return NULL;
                }
            }
        }
    }

    // with system_maintained
    if (lpCol->bSystemMaintained &&( lpCol->uDataType == INGTYPE_OBJKEY || lpCol->uDataType == INGTYPE_TABLEKEY))
        m = ConcateStrings (&lpString, "with system_maintained ", (LPTSTR)0);

    //
    // The unique and check constaints have been generated if specified.
    // So we generate only the column level reference constraint here.
    //
    if (lpCol && lpCol->lpszCheck)
    {
        m = ConcateStrings (&lpString, "check  ", "(", lpCol->lpszCheck, ") ", (LPTSTR)0);
        if (!m) return NULL;
    }
    while (lpFKey)
    {
        lpRef = (LPREFERENCEPARAMS)lpFKey->lpObject;
        list  = lpRef->lpColumns;
        if (CountListItems (list) == 1)
        {
            lpRefCol = (LPREFERENCECOLS)list->lpObject;
            if (lstrcmpi (lpRefCol->szRefingColumn, lpCol->szColumn) == 0)
            {
                TCHAR szCol [MAXOBJECTNAME];
                wsprintf (szCol, " (%s)", lpRefCol->szRefedColumn);
                if (lpRef->szConstraintName [0] && lpRef->szConstraintName [0] != '<')
                {
                    m = ConcateStrings ( 
                        &lpString,
                        "constraint ",
                        (LPTSTR)QuoteIfNeeded(lpRef->szConstraintName), " ", 
                        "references ", 
                        (LPTSTR)QuoteIfNeeded(lpRef->szSchema), ".",
                        (LPTSTR)QuoteIfNeeded(lpRef->szTable),  szCol,
                        (LPTSTR)0);
                }
                else
                {
                    m = ConcateStrings ( 
                        &lpString,
                        "references ", 
                        (LPTSTR)QuoteIfNeeded(lpRef->szSchema), ".",
                        (LPTSTR)QuoteIfNeeded(lpRef->szTable),  szCol,
                        (LPTSTR)0);
                }
                if (!m) return NULL;
            }
            return lpString;
        }
        lpFKey = lpFKey->lpNext;
    }
    return lpString;
}

LPSTRINGLIST VDBA20xDropTableColumnSyntax(LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status)
{
    // 
    // All x in lpOLDTS and and x not in lpNEWTS -> Drop (x)
    LPCOLUMNPARAMS lpCol, lpFound;
    LPSTRINGLIST   lpStringList = NULL, lpStr;
    LPOBJECTLIST lo = lpOLDTS->lpColumns;
    while (lo)
    {
        lpCol   = (LPCOLUMNPARAMS)lo->lpObject;
        lpFound = VDBA20xTableColumn_Search (lpNEWTS->lpColumns, lpCol->szColumn);
        if (!lpFound)
        {
            TCHAR tchszSyntax [148];
            TCHAR tchszDelete [32];
            tchszSyntax [0] = '\0';

            lpStr = StringList_Search (lpOLDTS->lpDropObjectList, lpCol->szColumn);
            lstrcpy (tchszDelete, " restrict");
            if (lpStr && (lpStr->extraInfo1 == 1))
                lstrcpy (tchszDelete, " cascade");
            lstrcat (tchszSyntax, "alter table ");
            lstrcat (tchszSyntax, (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema));
            lstrcat (tchszSyntax, ".");
            lstrcat (tchszSyntax, (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname));
            lstrcat (tchszSyntax, " drop column ");
            lstrcat (tchszSyntax, (LPTSTR)QuoteIfNeeded(lpCol->szColumn));
            lstrcat (tchszSyntax, tchszDelete);
            lpStringList = StringList_Add  (lpStringList, tchszSyntax);
        }
        lo = lo->lpNext;
    }
    if (status) *status = 1;
    return lpStringList;
}


LPSTRINGLIST VDBA20xAddTableColumnSyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status)
{
    // 
    // All x in lpNEWTS and and x not in lpOLDTS -> Add (x)
    LPCOLUMNPARAMS lpCol, lpFound;
    LPSTRINGLIST   lpStringList = NULL;
    LPOBJECTLIST ln = lpNEWTS->lpColumns;
    while (ln)
    {
        lpCol   = (LPCOLUMNPARAMS)ln->lpObject;
        
        lpFound = VDBA20xTableColumn_Search (lpOLDTS->lpColumns, lpCol->szColumn);
        if (!lpFound)
        {
            int m;
            LPTSTR lpString = NULL;
            LPTSTR lpGenCol = VDBA20xColumnFormat (lpCol, lpNEWTS->lpReferences, lpNEWTS);

            if (!lpGenCol)
            {
                lpStringList = StringList_Done (lpStringList);
                return NULL;
            }
            m = ConcateStrings ( 
                &lpString,
                "alter table ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "add column " , lpGenCol,
                (LPTSTR)0);
            ESL_FreeMem (lpGenCol);
            if (!m)
            {
                lpStringList = StringList_Done (lpStringList);
                return NULL;
            }
            lpStringList = StringList_Add  (lpStringList, lpString);
            ESL_FreeMem (lpString);
        }
        ln = ln->lpNext;
    }
    if (status) *status = 1;
    return lpStringList;
}

static BOOL ColumnPropertyWasChanged(LPCOLUMNPARAMS lpOLDCS, LPCOLUMNPARAMS lpNEWCS)
{
    /* Compare nullable and default */
    if (lpOLDCS->bNullable != lpNEWCS->bNullable ||
        lpOLDCS->bDefault  != lpNEWCS->bDefault )
            return TRUE;

    /* Compare Column Data Type */
    if ( lpOLDCS->uDataType != lpNEWCS->uDataType )
        return TRUE;
    else
    {
        /* The Data type is the same verify the lenght*/
        if (lpOLDCS->uDataType == INGTYPE_C            ||
            lpOLDCS->uDataType == INGTYPE_CHAR         ||
            lpOLDCS->uDataType == INGTYPE_TEXT         ||
            lpOLDCS->uDataType == INGTYPE_VARCHAR      ||
            lpOLDCS->uDataType == INGTYPE_BYTE         ||
            lpOLDCS->uDataType == INGTYPE_BYTEVAR      ||
            lpOLDCS->uDataType == INGTYPE_UNICODE_NCHR ||
            lpOLDCS->uDataType == INGTYPE_UNICODE_NVCHR )
        {
            if ( lpOLDCS->lenInfo.dwDataLen != lpNEWCS->lenInfo.dwDataLen )
                return TRUE;
        }
        else if ( lpOLDCS->uDataType == INGTYPE_DECIMAL)
        {
            if ( lpOLDCS->lenInfo.decInfo.nPrecision != lpNEWCS->lenInfo.decInfo.nPrecision ||
                 lpOLDCS->lenInfo.decInfo.nScale != lpNEWCS->lenInfo.decInfo.nScale)
                     return TRUE;
        }
    }

    return FALSE;
}

LPSTRINGLIST VDBA20xAlterTableColumnSyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status)
{
    // 
    // All x in lpNEWTS and x in lpOLDTS without the same column type -> Alter (x)
    LPCOLUMNPARAMS lpCol, lpFound;
    LPSTRINGLIST   lpStringList = NULL;
    LPOBJECTLIST ln = lpNEWTS->lpColumns;
    while (ln)
    {
        lpCol   = (LPCOLUMNPARAMS)ln->lpObject;
        
        lpFound = VDBA20xTableColumn_Search (lpOLDTS->lpColumns, lpCol->szColumn);
        if (lpFound && ColumnPropertyWasChanged(lpFound, lpCol))
        {
            int m;
            LPTSTR lpString = NULL;
            LPTSTR lpGenCol = VDBA20xAlterColumnFormat (lpCol, NULL, lpNEWTS);

            if (!lpGenCol)
            {
                lpStringList = StringList_Done (lpStringList);
                return NULL;
            }
            m = ConcateStrings ( 
                &lpString,
                "alter table ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "alter column " , lpGenCol,
                (LPTSTR)0);
            ESL_FreeMem (lpGenCol);
            if (!m)
            {
                lpStringList = StringList_Done (lpStringList);
                return NULL;
            }
            lpStringList = StringList_Add  (lpStringList, lpString);
            ESL_FreeMem (lpString);
        }
        ln = ln->lpNext;
    }
    if (status) *status = 1;
    return lpStringList;
}

//
// NOTE: We use the extraInfo field of STRINGLIST as followings:
//       0: The string is generated for dropping.
//       1: The string is generated for adding.
//
// Nov 6 1998 Change: (Requested by FNN) Only Drop or Add primary at one time:
LPSTRINGLIST VDBA20xAlterPrimaryKeySyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status)
{
    int m;
    LPTSTR lpPk;
    LPTSTR lpString = NULL;
    LPSTRINGLIST lpSyntax = NULL;
    LPSTRINGLIST lpObj = NULL;
    if (status) *status = 1;

    switch (lpNEWTS->primaryKey.nMode)
    {
    case 0:
        //
        // Create:
        if (lpOLDTS->primaryKey.bAlter)
            break;
        lpPk = VDBA20xPrimaryKeySyntax (&(lpNEWTS->primaryKey), &m);
        if (!m)
        {
            if (status) *status = 0; // Mem error.
            return NULL;
        }
        if (!lpPk)
            break;
        if (lpOLDTS->szSchema [0])
            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "ADD ", lpPk, (LPTSTR)0);
        else
            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpNEWTS->objectname), " ",
                "ADD ", lpPk, (LPTSTR)0);
        if (lpPk)
            ESL_FreeMem (lpPk);
        if (!m)
        {
            if (status) *status = 0; // Mem error.
            return NULL;
        }
        lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
        if (lpString)
            ESL_FreeMem (lpString);
        if (!lpSyntax && status) *status = 0;
        if (lpObj)
            lpObj->extraInfo1 = 1;
        break;
    case 1:
        //
        // Drop restrict:
        if (lpOLDTS->primaryKey.tchszPKeyName[0] && lpOLDTS->primaryKey.bAlter)
        {
            if (lpOLDTS->szSchema [0])
                m = ConcateStrings (
                    &lpString, 
                    "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                    "DROP CONSTRAINT ", (LPTSTR)QuoteIfNeeded(lpOLDTS->tchszPrimaryKeyName), " RESTRICT ",
                    (LPTSTR)0);
            else
                m = ConcateStrings (
                    &lpString, 
                    "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                    "DROP CONSTRAINT ", (LPTSTR)QuoteIfNeeded(lpOLDTS->tchszPrimaryKeyName), " RESTRICT ",
                    (LPTSTR)0);
            if (!m)
            {
                if (status) *status = 0; // Mem error.
                return NULL;
            }
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            if (lpString)
                ESL_FreeMem (lpString);
            if (!lpSyntax && status) *status = 0;
            if (lpObj)
                lpObj->extraInfo1 = 0;
        }
        break;
    case 2:
        //
        // Drop cascade:
        if (lpOLDTS->primaryKey.tchszPKeyName[0] && lpOLDTS->primaryKey.bAlter)
        {
            if (lpOLDTS->szSchema [0])
                m = ConcateStrings (
                    &lpString, 
                    "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                    "DROP CONSTRAINT ",  (LPTSTR)QuoteIfNeeded(lpOLDTS->tchszPrimaryKeyName), " cascade ",
                    (LPTSTR)0);
            else
                m = ConcateStrings (
                    &lpString, 
                    "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                    "DROP CONSTRAINT ", (LPTSTR)QuoteIfNeeded(lpOLDTS->tchszPrimaryKeyName), " cascade ",
                    (LPTSTR)0);
            if (!m)
            {
                if (status) *status = 0; // Mem error.
                return NULL;
            }
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            if (lpString)
                ESL_FreeMem (lpString);
            if (!lpSyntax && status) *status = 0;
            if (lpObj)
                lpObj->extraInfo1 = 0;
        }
        break;
    default:
        return NULL;
        break;
    }
    return lpSyntax;
}


// Let fk the foreign key.
// A) For all fk, if fk is in lpOLDTS and fk is not in lpNEWTS => delete lpOLDTS.fk 
// B) For all fk, if fk is not in lpOLDTS and fk is in lpNEWTS => add lpNEWTS.fk
//
// NOTE: We use the extraInfo field of STRINGLIST as followings:
//       0: The string is generated for dropping.
//       1: The string is generated for adding.
LPSTRINGLIST VDBA20xAlterForeignKeySyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status)
{
    int m;
    LPTSTR lpString = NULL;
    LPSTRINGLIST lpSyntax = NULL, lpStr, lpObj = NULL;
    LPOBJECTLIST lpOldFk, lpNewFk;
    LPREFERENCEPARAMS lpRef;
    //
    // Condition A) is Verified.
    lpOldFk = lpOLDTS->lpReferences;
    while (lpOldFk)
    {
        lpRef = (LPREFERENCEPARAMS)lpOldFk->lpObject;
        if (!VDBA20xForeignKey_Search (lpNEWTS->lpReferences, lpRef->szConstraintName) || 
            !VDBA20xForeignKey_Lookup (lpNEWTS->lpReferences, lpRef))
        {
            //
            //  Delete old foreign key.
            TCHAR tchszDelete [32];
            lpStr = StringList_Search (lpNEWTS->lpDropObjectList, lpRef->szConstraintName);
            lstrcpy (tchszDelete, " restrict ");
            if (lpStr && (lpStr->extraInfo1 == 1))
                lstrcpy (tchszDelete, " cascade ");

            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "DROP CONSTRAINT ", (LPTSTR)QuoteIfNeeded(lpRef->szConstraintName), tchszDelete, (LPTSTR)0);
            if (!lpString)
            {
                if (status) *status = 0;
                return NULL;
            }
            //lpSyntax = StringList_Add (lpSyntax, lpString);
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            ESL_FreeMem (lpString);
            lpString = NULL;
            if (!lpSyntax && status)
            {
                if (status) *status = 0;
                return NULL;
            }
            if (lpObj)
                lpObj->extraInfo1 = 0;
        }
        lpOldFk = lpOldFk->lpNext;
    }
    //
    // Condition B) is Verified.
    lpString= NULL;
    lpNewFk = lpNEWTS->lpReferences;
    while (lpNewFk)
    {
        lpRef = (LPREFERENCEPARAMS)lpNewFk->lpObject;
        if (!VDBA20xForeignKey_Search (lpOLDTS->lpReferences, lpRef->szConstraintName) || 
            !VDBA20xForeignKey_Lookup (lpOLDTS->lpReferences, lpRef))
        {
            LPTSTR lpFk;
            OBJECTLIST obl;
            obl.lpObject = (LPVOID)lpRef;
            obl.lpNext   = (LPVOID)NULL;
            lpFk = VDBA20xForeignKeySyntax(lpOLDTS->objectname, &obl, &m);
            //
            //  Add new foreign key.
            if (!m)
            {
                if (lpFk) ESL_FreeMem (lpFk);
                if (status) status = NULL;
                StringList_Done (lpSyntax);
                return NULL;
            }
            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "ADD ", lpFk, (LPTSTR)0);
            if (lpFk) ESL_FreeMem (lpFk);
            if (!lpString)
            {
                if (status) *status = 0;
                StringList_Done (lpSyntax);
                return NULL;
            }
            //lpSyntax = StringList_Add (lpSyntax, lpString);
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            ESL_FreeMem (lpString);
            lpString = NULL;
            if (!lpSyntax && status)
            {
                if (status) *status = 0;
                return NULL;
            }
            if (lpObj)
                lpObj->extraInfo1 = 1;
        }
        lpNewFk = lpNewFk->lpNext;
    }
    return lpSyntax;
}


// Let un the unique key.
// A) For all un, if un is in lpOLDTS and un is not in lpNEWTS => delete lpOLDTS.un 
// B) For all un, if un is not in lpOLDTS and un is in lpNEWTS => add lpNEWTS.un
//
// NOTE: We use the extraInfo field of STRINGLIST as followings:
//       0: The string is generated for dropping.
//       1: The string is generated for adding.
LPSTRINGLIST VDBA20xAlterUniqueKeySyntax  (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status)
{
    int m;
    LPTSTR lpString = NULL;
    LPSTRINGLIST   lpSyntax = NULL, lpStr, lpObj = NULL;
    LPTLCUNIQUE    lpOldUn, lpNewUn, ls;
    LPOBJECTLIST   lpobjectlist;
    LPCOLUMNPARAMS lpcp;
    BOOL bDefined  = FALSE;
    //
    // Generate the Unique constraint specified at column level only if
    // the constraint was not defined at the table level and the current column is
    // an existing coulumn, ie not a newly added column.
    lpobjectlist = lpNEWTS->lpColumns;
    while (lpobjectlist)
    {
        lpcp     = (LPCOLUMNPARAMS)lpobjectlist->lpObject;
        ls       = lpNEWTS->lpUnique;
        bDefined = FALSE;
        while (ls)
        {
            if (StringList_Count (ls->lpcolumnlist) == 1 && lstrcmpi ((ls->lpcolumnlist)->lpszString, lpcp->szColumn) == 0)
            {
                bDefined = TRUE;
                break;
            }
            ls = ls->next;
        }
        if (lpcp->bUnique && !bDefined && VDBA20xTableColumn_Search (lpOLDTS->lpColumns, lpcp->szColumn))
        {
            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "ADD UNIQUE ", "(", (LPTSTR)QuoteIfNeeded(lpcp->szColumn), ")", " ", (LPTSTR)0);
            if (!m)
            {
                if (status) *status = 0;
                return NULL;
            }
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            ESL_FreeMem (lpString);
            lpString = NULL;
            if (!lpSyntax)
            {
                if (status) *status = 0;
                return NULL;
            }
            if (lpObj) lpObj->extraInfo1 = 1;
        }
        lpobjectlist = lpobjectlist->lpNext;
    }
    //
    // Condition A) is Verified.
    lpOldUn = lpOLDTS->lpUnique;
    while (lpOldUn)
    {
        if (!VDBA20xTableUniqueKey_Lookup (lpNEWTS->lpUnique, lpOldUn))
        {
            //
            //  Delete old Unique key.
            TCHAR tchszDelete [32];
            lpStr = StringList_Search (lpNEWTS->lpDropObjectList, lpOldUn->tchszConstraint);
            lstrcpy (tchszDelete, " restrict ");
            if (lpStr && (lpStr->extraInfo1 == 1))
                lstrcpy (tchszDelete, " cascade ");

            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "DROP CONSTRAINT ", (LPTSTR)QuoteIfNeeded(lpOldUn->tchszConstraint), tchszDelete, (LPTSTR)0);
            if (!lpString)
            {
                if (status) *status = 0;
                return NULL;
            }
            //lpSyntax = StringList_Add (lpSyntax, lpString);
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            ESL_FreeMem (lpString);
            lpString = NULL;
            if (!lpSyntax && status)
            {
                if (status) *status = 0;
                return NULL;
            }
            if (lpObj)
                lpObj->extraInfo1 = 0;
        }
        lpOldUn = lpOldUn->next;
    }
    //
    // Condition B) is Verified.
    lpString= NULL;
    lpNewUn = lpNEWTS->lpUnique;
    while (lpNewUn)
    {
        if (!VDBA20xTableUniqueKey_Lookup (lpOLDTS->lpUnique, lpNewUn))
        {
            LPTSTR lpUn = NULL;
            TLCUNIQUE unNew;
            memset (&unNew, 0, sizeof(unNew));
            unNew.next = NULL;  // Very important, it will generate only one constraint.
            INDEXOPTION_COPY (&(unNew.constraintWithClause), &(lpNewUn->constraintWithClause));
            unNew.lpcolumnlist = lpNewUn->lpcolumnlist;
            lstrcpy (unNew.tchszConstraint, (LPTSTR)QuoteIfNeeded(lpNewUn->tchszConstraint));
            lpUn = VDBA20xUniqueKeySyntax(&unNew, &m);
            //
            //  Add new unique key.
            if (!m)
            {
                if (lpUn) ESL_FreeMem (lpUn);
                if (status) *status = 0;
                StringList_Done (lpSyntax);
                return NULL;
            }
            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "ADD ", lpUn, (LPTSTR)0);
            if (lpUn) ESL_FreeMem (lpUn);
            if (!lpString)
            {
                if (status) *status = 0;
                StringList_Done (lpSyntax);
                return NULL;
            }
            //lpSyntax = StringList_Add (lpSyntax, lpString);
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            ESL_FreeMem (lpString);
            lpString = NULL;
            if (!lpSyntax && status)
            {
                if (status) *status = 0;
                return NULL;
            }
            if (lpObj)
                lpObj->extraInfo1 = 1;
        }
        lpNewUn = lpNewUn->next;
    }
    return lpSyntax;
}

// Let ch the check condition.
// A) For all ch, if ch is in lpOLDTS and ch is not in lpNEWTS => delete lpOLDTS.ch 
// B) For all ch, if ch is not in lpOLDTS and ch is in lpNEWTS => add lpNEWTS.ch
//
// NOTE: We use the extraInfo field of STRINGLIST as followings:
//       0: The string is generated for dropping.
//       1: The string is generated for adding.
LPSTRINGLIST VDBA20xAlterCheckSyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status)
{
    int m;
    LPTSTR lpString = NULL;
    LPSTRINGLIST lpSyntax = NULL, lpObj = NULL, lpStr;
    LPTLCCHECK lpOldCh, lpNewCh;
    LPOBJECTLIST   lpobjectlist;
    LPCOLUMNPARAMS lpcp;
    //
    // Generate the Check constraint specified at column level.
    lpobjectlist = lpNEWTS->lpColumns;
    while (lpobjectlist)
    {
        lpcp = (LPCOLUMNPARAMS)lpobjectlist->lpObject;
        if (lpcp->lpszCheck)
        {
            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "ADD CHECK ", "(", lpcp->lpszCheck, ")", " ", (LPTSTR)0);
            if (!m)
            {
                if (status) *status = 0;
                return NULL;
            }
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            ESL_FreeMem (lpString);
            lpString = NULL;
            if (!lpSyntax)
            {
                if (status) *status = 0;
                return NULL;
            }
            if (lpObj) lpObj->extraInfo1 = 1;
        }
        lpobjectlist = lpobjectlist->lpNext;
    }
    //
    // Condition A) is Verified.
    lpOldCh = lpOLDTS->lpCheck;
    while (lpOldCh)
    {
        if (!VDBA20xTableCheck_Lookup (lpNEWTS->lpCheck, lpOldCh))
        {
            //
            //  Delete old Check Condition.
            TCHAR tchszDelete [32];
            lpStr = StringList_Search (lpNEWTS->lpDropObjectList, lpOldCh->tchszConstraint);
            lstrcpy (tchszDelete, " restrict ");
            if (lpStr && (lpStr->extraInfo1 == 1))
                lstrcpy (tchszDelete, " cascade ");
            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "DROP CONSTRAINT ", (LPTSTR)QuoteIfNeeded(lpOldCh->tchszConstraint), tchszDelete, (LPTSTR)0);
            if (!lpString)
            {
                if (status) *status = 0;
                return NULL;
            }
            //lpSyntax = StringList_Add (lpSyntax, lpString);
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            ESL_FreeMem (lpString);
            lpString = NULL;
            if (!lpSyntax && status)
            {
                if (status) *status = 0;
                return NULL;
            }
            if (lpObj)
                lpObj->extraInfo1 = 0;
        }
        lpOldCh = lpOldCh->next;
    }
    //
    // Condition B) is Verified.
    lpString= NULL;
    lpNewCh = lpNEWTS->lpCheck;
    while (lpNewCh)
    {
        if (!VDBA20xTableCheck_Lookup (lpOLDTS->lpCheck, lpNewCh))
        {
            LPTSTR lpCh;
            TLCCHECK chNew;
            chNew.next = NULL;  // Very important, it will generate only one constraint.
            chNew.lpexpression = lpNewCh->lpexpression;
            lstrcpy (chNew.tchszConstraint, lpNewCh->tchszConstraint);
            lpCh = VDBA20xCheckSyntax(&chNew, &m);
            //
            //  Add new Check Condition.
            if (!m)
            {
                if (lpCh) ESL_FreeMem (lpCh);
                if (status) status = NULL;
                StringList_Done (lpSyntax);
                return NULL;
            }
            m = ConcateStrings (
                &lpString, 
                "ALTER TABLE ", (LPTSTR)QuoteIfNeeded(lpOLDTS->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpOLDTS->objectname), " ",
                "ADD ", lpCh, (LPTSTR)0);
            if (lpCh) ESL_FreeMem (lpCh);
            if (!lpString)
            {
                if (status) *status = 0;
                StringList_Done (lpSyntax);
                return NULL;
            }
            //lpSyntax = StringList_Add (lpSyntax, lpString);
            lpSyntax = StringList_AddObject  (lpSyntax, lpString, &lpObj);
            ESL_FreeMem (lpString);
            lpString = NULL;
            if (!lpSyntax && status)
            {
                if (status) *status = 0;
                return NULL;
            }
            if (lpObj)
                lpObj->extraInfo1 = 1;
        }
        lpNewCh = lpNewCh->next;
    }
    return lpSyntax;
}


LPTLCUNIQUE VDBA20xTableUniqueKey_Search (LPTLCUNIQUE lpUniques, LPCTSTR lpszConstraint)
{
    LPTLCUNIQUE ls = lpUniques;
    if (!lpUniques)
       return NULL;
    while (ls)
    {
        if (lstrcmpi (ls->tchszConstraint, lpszConstraint) == 0)
            return ls;
        ls = ls->next;
    }
    return NULL;
}


LPTLCUNIQUE VDBA20xTableUniqueKey_Lookup (LPTLCUNIQUE lpUniques, LPTLCUNIQUE lpUniqueKey)
{
    int C1, C2;
    LPTLCUNIQUE ls = lpUniques;
    C1 = StringList_Count (lpUniqueKey->lpcolumnlist);

    while (ls)
    {
        //if (ls->tchszConstraint[0] && lstrcmpi (ls->tchszConstraint, lpUniqueKey->tchszConstraint) == 0)
        //    return ls;
        
        C2 = StringList_Count (ls->lpcolumnlist);
        if (C1 == C2)
        {
            BOOL isEqual = TRUE;
            LPSTRINGLIST l1 = lpUniqueKey->lpcolumnlist;
            LPSTRINGLIST l2 = ls->lpcolumnlist;
            while (l1 && l2)
            {
                if (!StringList_Search (lpUniqueKey->lpcolumnlist, l2->lpszString))
                {
                    isEqual = FALSE;
                    break;
                }
                if (!StringList_Search (ls->lpcolumnlist, l1->lpszString))
                {
                    isEqual = FALSE;
                    break;
                }
                /*
                if (lstrcmpi (l1->lpszString, l2->lpszString) != 0)
                {
                    isEqual = FALSE;
                    break;
                }
                */
                l1 = l1->next;
                l2 = l2->next;
            }
            if (isEqual) return ls;
        }
        ls = ls->next;
    }
    return NULL;
}



LPTLCUNIQUE VDBA20xTableUniqueKey_Add (LPTLCUNIQUE lpUniques, LPTLCUNIQUE lpUniqueKey)
{
    if (!lpUniques)
    {
        lpUniqueKey->next = NULL;
        lpUniques         = lpUniqueKey;
    }
    else
    {
        LPTLCUNIQUE prev = NULL;
        LPTLCUNIQUE lp   = lpUniques;
       
        while (lp)
        {
            prev = lp;
            lp   = lp->next;
        }
        lpUniqueKey->next   = NULL;
        prev->next  = lpUniqueKey;
    }
    return lpUniques;
}

LPTLCUNIQUE VDBA20xTableUniqueKey_Remove (LPTLCUNIQUE lpUniques, LPCTSTR lpszConstraint)
{
    LPTLCUNIQUE ls     = lpUniques;
    LPTLCUNIQUE lpPrec = NULL, tmp;

    while (ls)
    {
        if (lstrcmpi (ls->tchszConstraint, lpszConstraint) == 0)
        {
            tmp = ls;
            if (lpPrec == NULL)             // Remove the head of the list 
                lpUniques = lpUniques->next;
            else
                lpPrec->next = ls->next;    // Remove somewhere else

            tmp->constraintWithClause.pListLocation = StringList_Done (tmp->constraintWithClause.pListLocation);
            tmp->lpcolumnlist = StringList_Done (tmp->lpcolumnlist);
            ESL_FreeMem (tmp);
            return lpUniques;
        }
        lpPrec = ls;
        ls = ls->next;
    }
    return lpUniques;
}


LPTLCCHECK VDBA20xTableCheck_Search (LPTLCCHECK lpCheck, LPCTSTR lpszConstraint)
{
    LPTLCCHECK ls = lpCheck;
    while (ls)
    {
        if (lstrcmpi (ls->tchszConstraint, lpszConstraint) == 0)
            return ls;
        ls = ls->next;
    }
    return NULL;
}

LPTLCCHECK VDBA20xTableCheck_Lookup (LPTLCCHECK lpCheck, LPTLCCHECK lpObj)
{
    LPTLCCHECK ls = lpCheck;
    while (ls)
    {
        if ((lstrcmpi (ls->tchszConstraint, lpObj->tchszConstraint) == 0) && 
            (lstrcmpi (ls->lpexpression, lpObj->lpexpression) == 0))
            return ls;
        ls = ls->next;
    }
    return NULL;
}

LPTLCCHECK VDBA20xTableCheck_Add (LPTLCCHECK lpCheck, LPTLCCHECK lpObj)
{
    if (!lpCheck)
    {
        lpObj->next  = NULL;
        lpCheck       = lpObj;
    }
    else
    {
        LPTLCCHECK prev = NULL;
        LPTLCCHECK lp   = lpCheck;
       
        while (lp)
        {
            prev = lp;
            lp   = lp->next;
        }
        lpObj->next = NULL;
        prev->next  = lpObj;
    }
    return lpCheck;
}

LPTLCCHECK VDBA20xTableCheck_Remove (LPTLCCHECK lpCheck, LPCTSTR lpszConstraint)
{
    LPTLCCHECK ls     = lpCheck;
    LPTLCCHECK lpPrec = NULL, tmp;

    while (ls)
    {
        if (lstrcmpi (ls->tchszConstraint, lpszConstraint) == 0)
        {
            tmp = ls;
            if (lpPrec == NULL)             // Remove the head of the list 
                lpCheck = lpCheck->next;
            else
                lpPrec->next = ls->next;    // Remove somewhere else
            
            if (tmp->lpexpression)
                ESL_FreeMem (tmp->lpexpression);
            ESL_FreeMem (tmp);
            return lpCheck;
        }
        lpPrec = ls;
        ls = ls->next;
    }
    return lpCheck;
}



static LPTSTR VDBA20xPrimaryKeySyntax (PRIMARYKEYSTRUCT* pPrimaryKey, int* m)
{
    int  err;
    BOOL bOneCol = TRUE;
    LPTSTR lpString = NULL;
    LPSTRINGLIST ls = pPrimaryKey->pListKey;

    if (m) *m = 1;

    if (pPrimaryKey->pListKey && StringList_Count (pPrimaryKey->pListKey) > 0)
    {
        LPTSTR lpWithClause = NULL;
        if (pPrimaryKey->tchszPKeyName[0] && pPrimaryKey->tchszPKeyName[0] != '$' && pPrimaryKey->tchszPKeyName[0] != '<')
            err = ConcateStrings (&lpString, "constraint ", (LPTSTR)QuoteIfNeeded(pPrimaryKey->tchszPKeyName), " primary key (", (LPTSTR)0);
        else
            err = ConcateStrings (&lpString, "primary key (", (LPTSTR)0);
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }
        while (ls)
        {
            if (bOneCol)
                err = ConcateStrings (&lpString, (LPTSTR)QuoteIfNeeded(ls->lpszString), (LPTSTR)0);
            else
                err = ConcateStrings (&lpString, ", ", (LPTSTR)QuoteIfNeeded(ls->lpszString), (LPTSTR)0);
            bOneCol = FALSE;
            if (!err)
            {
                if (m) *m = 0;
                return NULL;
            }
            ls = ls->next;
        }
        err = ConcateStrings (&lpString, ")", (LPTSTR)0);
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }
        if (IsVDBA25xIndexEnforcementAvailable())
        {
            lpWithClause = VDBA20xIndexConstraintSyntax (&(pPrimaryKey->constraintWithClause));
            if (lpWithClause)
            {
                err = ConcateStrings (&lpString, " ", lpWithClause, (LPTSTR)0);
                ESL_FreeMem ((LPVOID)lpWithClause);
            }
            if (!err)
            {
                if (m) *m = 0;
                return NULL;
            }
        }
    }
    return lpString;
}

static LPTSTR VDBA20xForeignKeySyntax (LPTSTR lpszTbName, LPOBJECTLIST  lpReferences, int* m)
{
    int err, err2;
    TCHAR tchszActionRule [MAXOBJECTNAME];
    BOOL bOneFKey = TRUE, bOneCol = TRUE;
    LPREFERENCEPARAMS lpRef;
    LPREFERENCECOLS lpRefCol;
    LPOBJECTLIST ls = lpReferences, lx;
    LPTSTR lpString = NULL, lpRefedCol = NULL;
    LPTSTR lpWithClause = NULL;

    if (m) *m = 1;
    while (ls)
    {
        lpRef    = (LPREFERENCEPARAMS)ls->lpObject;
        lx = lpRef->lpColumns;
        if (bOneFKey)
        {
            if (lpRef->szConstraintName[0] && lpRef->szConstraintName[0] != '<' && lpRef->szConstraintName[0] != '$')
                err = ConcateStrings (&lpString, "constraint ", (LPTSTR)QuoteIfNeeded(lpRef->szConstraintName), " foreign key", (LPTSTR)0);
            else
                err = ConcateStrings (&lpString, "foreign key", (LPTSTR)0);
        }
        else
        {
            if (lpRef->szConstraintName[0] && lpRef->szConstraintName[0] != '<' && lpRef->szConstraintName[0] != '$')
                err = ConcateStrings (&lpString, ", constraint ", (LPTSTR)QuoteIfNeeded(lpRef->szConstraintName), " foreign key", (LPTSTR)0);
            else
                err = ConcateStrings (&lpString, ", foreign key", (LPTSTR)0);
        }
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }
        bOneFKey = FALSE;
        bOneCol  = TRUE;
        lpRefedCol = NULL;
        //
        // Generate Foreign Key Columns
        err = ConcateStrings (&lpString,   "(", (LPTSTR)0);
        err2= ConcateStrings (&lpRefedCol, "(", (LPTSTR)0);
        if (!err || !err2)
        {
            if (m) *m = 0;
            return NULL;
        }
        while (lx)
        {
            lpRefCol = (LPREFERENCECOLS)lx->lpObject;
            if (bOneCol)
            {
                err = ConcateStrings (&lpString,  (LPTSTR)QuoteIfNeeded(lpRefCol->szRefingColumn), (LPTSTR)0);
                err2= ConcateStrings (&lpRefedCol,(LPTSTR)QuoteIfNeeded(lpRefCol->szRefedColumn),  (LPTSTR)0);
            }
            else
            {
                err = ConcateStrings (&lpString,   ", ", (LPTSTR)QuoteIfNeeded(lpRefCol->szRefingColumn), (LPTSTR)0);
                err2= ConcateStrings (&lpRefedCol, ", ", (LPTSTR)QuoteIfNeeded(lpRefCol->szRefedColumn),  (LPTSTR)0);
            }
            bOneCol = FALSE;
            if (!err || !err2)
            {
                if (m) *m = 0;
                return NULL;
            }
            lx = lx->lpNext;
        }
        err = ConcateStrings (&lpString,   ")", (LPTSTR)0);
        err2= ConcateStrings (&lpRefedCol, ")", (LPTSTR)0);
        if (!err || !err2)
        {
            if (m) *m = 0;
            return NULL;
        }
        assert (lpszTbName);
        if (lpRef->szTable[0] == '<')               // It is <Same Table>
            lstrcpy (lpRef->szTable, lpszTbName);   // Then use the current Table Name 
        if (lpRef->szSchema [0])
            err = ConcateStrings (&lpString, " references ", (LPTSTR)QuoteIfNeeded(lpRef->szSchema), ".", (LPTSTR)QuoteIfNeeded(lpRef->szTable), " ", lpRefedCol, (LPTSTR)0);
        else
            err = ConcateStrings (&lpString, " references ", (LPTSTR)QuoteIfNeeded(lpRef->szTable), " ", lpRefedCol, (LPTSTR)0);
        ESL_FreeMem (lpRefedCol);
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }
        // 2.6
        // For each foreign key, generate the Referential Action:
        tchszActionRule [0] = '\0';
        if (lpRef->bUseDeleteAction && lpRef->tchszDeleteAction[0])
        {
            lstrcat (tchszActionRule, " on delete ");
            lstrcat (tchszActionRule, lpRef->tchszDeleteAction);
        }
        if (lpRef->bUseUpdateAction && lpRef->tchszUpdateAction[0])
        {
            lstrcat (tchszActionRule, " on update ");
            lstrcat (tchszActionRule, lpRef->tchszUpdateAction);
        }
        if (tchszActionRule[0])
        {
            err = ConcateStrings (&lpString, tchszActionRule, (LPTSTR)0);
            if (!err)
            {
                if (m) *m = 0;
                return NULL;
            }
        }
        // 2.6
        // For each foreign key, generate the Enforcement Constraint Index:
        if (IsVDBA25xIndexEnforcementAvailable())
        {
            lpWithClause = VDBA20xIndexConstraintSyntax (&(lpRef->constraintWithClause));
            if (lpWithClause)
            {
                err = ConcateStrings (&lpString, " ", lpWithClause, (LPTSTR)0);
                ESL_FreeMem ((LPVOID)lpWithClause);
                if (!err)
                {
                    if (m) *m = 0;
                    return NULL;
                }
            }
        }
        ls = ls->lpNext;
    }

    return lpString;
}

static LPTSTR VDBA20xUniqueKeySyntax (LPTLCUNIQUE lpUnique, int* m)
{
    int err;
    BOOL bOneUnique = TRUE, bOneCol = TRUE;
    LPTLCUNIQUE ls = lpUnique;
    LPTSTR lpString = NULL;
    LPTSTR lpWithClause = NULL;
    LPSTRINGLIST lpColList;
    if (m) *m = 1;
    
    while (ls)
    {
        if (bOneUnique)
        {
            if (ls->tchszConstraint[0] && ls->tchszConstraint[0] != '<' && ls->tchszConstraint[0] != '$')
                err = ConcateStrings (&lpString, "constraint ", (LPTSTR)QuoteIfNeeded(ls->tchszConstraint), " unique", (LPTSTR)0);
            else
                err = ConcateStrings (&lpString, "unique", (LPTSTR)0);
        }
        else
        {
            if (ls->tchszConstraint[0] && ls->tchszConstraint[0] != '<' && ls->tchszConstraint[0] != '$')
                err = ConcateStrings (&lpString, ", constraint ", (LPTSTR)QuoteIfNeeded(ls->tchszConstraint), " unique", (LPTSTR)0);
            else
                err = ConcateStrings (&lpString, ", unique", (LPTSTR)0);
        }
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }
        bOneUnique = FALSE;
        bOneCol    = TRUE;
        lpColList  = ls->lpcolumnlist;
        //
        // Generate the Unique Columns
        err = ConcateStrings (&lpString,   "(", (LPTSTR)0);
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }
        while (lpColList)
        {
            if (bOneCol)
                err = ConcateStrings (&lpString, (LPTSTR)QuoteIfNeeded(lpColList->lpszString), (LPTSTR)0);
            else
                err = ConcateStrings (&lpString, ", ", (LPTSTR)QuoteIfNeeded(lpColList->lpszString), (LPTSTR)0);
            bOneCol = FALSE;
            if (!err)
            {
                if (m) *m = 0;
                return NULL;
            }
            lpColList = lpColList->next;
        }
        err = ConcateStrings (&lpString,   ")", (LPTSTR)0);
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }

        //
        // For each unique key, generate the Enforcement Constraint Index:
        if (IsVDBA25xIndexEnforcementAvailable())
        {
            lpWithClause = VDBA20xIndexConstraintSyntax (&(ls->constraintWithClause));
            if (lpWithClause)
            {
                err = ConcateStrings (&lpString, " ", lpWithClause, (LPTSTR)0);
                ESL_FreeMem ((LPVOID)lpWithClause);
                if (!err)
                {
                    if (m) *m = 0;
                    return NULL;
                }
            }
        }
        ls = ls->next;
    }

    return lpString;
}

static LPTSTR VDBA20xCheckSyntax (LPTLCCHECK  lpCheck, int* m)
{
    int err;
    BOOL bOneCheck = TRUE;
    LPTLCCHECK  ls = lpCheck;
    LPTSTR lpString = NULL;
    if (m) *m = 1;
    
    while (ls)
    {
        if (bOneCheck)
        {
            if (ls->tchszConstraint[0] && ls->tchszConstraint[0] != '<' && ls->tchszConstraint[0] != '$')
                err = ConcateStrings (&lpString, "constraint ", (LPTSTR)QuoteIfNeeded(ls->tchszConstraint), " ", (LPTSTR)0);
            else
                err = ConcateStrings (&lpString, " ", (LPTSTR)0);
        }
        else
        {
            if (ls->tchszConstraint[0] && ls->tchszConstraint[0] != '<' && ls->tchszConstraint[0] != '$')
                err = ConcateStrings (&lpString, ", constraint ", (LPTSTR)QuoteIfNeeded(ls->tchszConstraint), (LPTSTR)0);
            else
                err = ConcateStrings (&lpString, ",", (LPTSTR)0);
        }
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }
        bOneCheck  = FALSE;
        err = ConcateStrings (&lpString, " ", ls->lpexpression, (LPTSTR)0);
        if (!err)
        {
            if (m) *m = 0;
            return NULL;
        }
        ls = ls->next;
    }

    return lpString;
}

LPTSTR VDBA20xWithClauseSyntax(LPTABLEPARAMS lpTS, int* m)
{
    int x;
    BOOL bOneClause = TRUE;
    BOOL B [3];
    LPTSTR lpString = NULL;
    LPOBJECTLIST lc = lpTS->lpLocations;
    BOOL bStarWithClause = FALSE;

    if (GetOIVers() == OIVERS_NOTOI)
        return NULL;// With clause is not available on a Gateway.
	
    if (m) *m = 1;
    if (lpTS->uPage_size == 2048L)
       lpTS->uPage_size = 0L;
    
    B[0] = lpTS->bJournaling && lpTS->bDuplicates && (lpTS->nLabelGran == LABEL_GRAN_SYS_DEFAULTS);
    B[1] = (lpTS->nSecAudit == SECURITY_AUDIT_TABLE) && (lpTS->lpLocations == NULL) && (!lpTS->szSecAuditKey[0]);
    B[2] = (lpTS->uPage_size== 0L);

    // Star
    if (lpTS->bDDB) {
      if (!lpTS->bCoordinator || !lpTS->bDefaultName)
        bStarWithClause = TRUE;
    }

/*  03-Mar-2000 (noifr01) (bug 100695) never returnfrom this place any more, in order always to generate 
    the with clause , at least for the "with journaling" option which is not the default (doc issue) and
	needs to be generated anyhow*/
/*    if (B[0] && B[1] && B[2] && !bStarWithClause) */
/*        return NULL;    // Use the default option */

    x = ConcateStrings (&lpString, "with ", (LPTSTR)0);
    if (!x)
    {
        if (m) *m = 0;
        return NULL;
    }

    if (lpTS->bCreateVectorWise)
    {
	x = ConcateStrings (&lpString, "structure=vectorwise", (LPTSTR)0);
	if (!x)
	{
		if (m) *m=0;
		return NULL;
	}
    }
    else
    {
    // Star clauses first
    if (bStarWithClause) {
      if (!lpTS->bCoordinator) {
        x = ConcateStrings (&lpString, "node = ", 
                                       lpTS->szLocalNodeName,
                                       ", database = ",
                                       lpTS->szLocalDBName,
                                       (LPTSTR)0);
        bOneClause = FALSE;
        if (!x) {
          if (m) *m = 0;
          return NULL;
        }
      }
      if (!lpTS->bDefaultName) {
        if (bOneClause)
          x = ConcateStrings (&lpString, "table = ", lpTS->szLocalTblName, (LPTSTR)0);
        else
          x = ConcateStrings (&lpString, ", table = ", lpTS->szLocalTblName, (LPTSTR)0);
        bOneClause = FALSE;
        if (!x) {
          if (m) *m = 0;
          return NULL;
        }
      }
    }

    //
    // If Locations are specified.
    if (lc)
    {
        LPCHECKEDOBJECTS lpCol;
        BOOL bOneLoc = TRUE;

        // must test bOneClause because of potential previous STAR clause(s)
        if (bOneClause)
          x = ConcateStrings (&lpString, "location = (", (LPTSTR)0);
        else
          x = ConcateStrings (&lpString, ", location = (", (LPTSTR)0);
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
        while (lc)
        {
            lpCol = (LPCHECKEDOBJECTS)lc->lpObject;
            if (bOneLoc)
                x = ConcateStrings (&lpString, (LPTSTR)QuoteIfNeeded(lpCol->dbname), (LPTSTR)0);
            else
                x = ConcateStrings (&lpString, ", ", (LPTSTR)QuoteIfNeeded(lpCol->dbname), (LPTSTR)0);
            bOneLoc = FALSE;
            if (!x)
            {
                if (m) *m = 0;
                return NULL;
            }
            lc = lc->lpNext;
        }
        x = ConcateStrings (&lpString, ")", (LPTSTR)0);
        bOneClause = FALSE;
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
    }
    //
    /* Journaling syntax will now be generated in any case (bug 100695) */
    if (!lpTS->bJournaling)
    {
        if (bOneClause)
            x = ConcateStrings (&lpString, "nojournaling", (LPTSTR)0);
        else
            x = ConcateStrings (&lpString, ", nojournaling", (LPTSTR)0);
        bOneClause = FALSE;
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
    }
	else { /* generate the journaling clause anyhow */
        if (bOneClause)
            x = ConcateStrings (&lpString, "journaling", (LPTSTR)0);
        else
            x = ConcateStrings (&lpString, ", journaling", (LPTSTR)0);
        bOneClause = FALSE;
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
	}
    //
    // If Duplicated Row is not Allowed
    if (!lpTS->bDuplicates)
    {
        if (bOneClause)
            x = ConcateStrings (&lpString, "noduplicates", (LPTSTR)0);
        else
            x = ConcateStrings (&lpString, ", noduplicates", (LPTSTR)0);
        bOneClause = FALSE;
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
    }
    //
    // If Label Granularity is specified
    if (lpTS->nLabelGran != LABEL_GRAN_SYS_DEFAULTS && lpTS->nLabelGran != LABEL_GRAN_UNKOWN)
    {
        TCHAR tchszLG [32];
        (lpTS->nLabelGran == LABEL_GRAN_ROW)? lstrcpy (tchszLG, "label_granularity = row"): lstrcpy (tchszLG, "label_granularity = table");
        if (bOneClause)
            x = ConcateStrings (&lpString, tchszLG, (LPTSTR)0);
        else
            x = ConcateStrings (&lpString, ", ", tchszLG, (LPTSTR)0);
        bOneClause = FALSE;
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
    }
    //
    // If Security Audit is specified
    if (lpTS->nSecAudit != SECURITY_AUDIT_TABLE)
    {
        TCHAR tchszLG [48];
        switch (lpTS->nSecAudit)
        {
        case SECURITY_AUDIT_TABLEROW: 
            lstrcpy (tchszLG, "security_audit = (table, row)");
            break;
        case SECURITY_AUDIT_TABLENOROW: 
            lstrcpy (tchszLG, "security_audit = (table, norow)");
            break;
        case SECURITY_AUDIT_ROW:         
            lstrcpy (tchszLG, "security_audit = (row)");
            break;
        case SECURITY_AUDIT_NOROW:        
            lstrcpy (tchszLG, "security_audit = (norow)");
            break;
        case SECURITY_AUDIT_TABLE:        
            lstrcpy (tchszLG, "security_audit = (table)");
            break;
        case SECURITY_AUDIT_UNKOWN:
        default:
            lstrcpy (tchszLG, "");  // no "audit" clause
            bOneClause = TRUE;
            break;
        }
        if (bOneClause)
            x = ConcateStrings (&lpString, tchszLG, (LPTSTR)0);
        else
            x = ConcateStrings (&lpString, ", ", tchszLG, (LPTSTR)0);
        bOneClause = FALSE;
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
    }
    //
    // If Security Audit Key is specified
    if (lpTS->szSecAuditKey[0])
    {
        if (bOneClause)
            x = ConcateStrings (&lpString, "security_audit_key = (", (LPTSTR)QuoteIfNeeded(lpTS->szSecAuditKey), ")", (LPTSTR)0);
        else
            x = ConcateStrings (&lpString, ", ", "security_audit_key = (", (LPTSTR)QuoteIfNeeded(lpTS->szSecAuditKey), ")", (LPTSTR)0);
        bOneClause = FALSE;
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
    }
    //
    // If Page_size is specified. Note DBMS version 2.0 and later will specify page size.
    if (lpTS->uPage_size > 0L)
    {
        TCHAR tchszPS [24];
        wsprintf (tchszPS, "page_size = %ld", lpTS->uPage_size);
        if (bOneClause)
            x = ConcateStrings (&lpString, tchszPS , (LPTSTR)0);
        else
            x = ConcateStrings (&lpString, ", ", tchszPS, (LPTSTR)0);
        bOneClause = FALSE;
        if (!x)
        {
            if (m) *m = 0;
            return NULL;
        }
    }
    }
    return lpString;
}


//
// Return the syntax string if success else NULL
// We suppose that 'lpTS' has been correctly filled and valid.
//
// Emb Nov 12, 97: Take star parameters in account
//
LPTSTR VDBA20xCreateTableSyntax (LPTABLEPARAMS lpTS)
{
    int m;
    LPCOLUMNPARAMS lpCol;
    LPOBJECTLIST   ls;
    BOOL   bOneCol  = TRUE;
    LPTSTR lpGenCol = NULL, lpWith = NULL;
    LPTSTR lpPk     = NULL, lpFk = NULL, lpUnique = NULL, lpCheck = NULL;
    LPTSTR lpString = NULL;

    if (lpTS->bCreateAsSelect)
    {   
        //
        // Create Table as Select ....
        m = ConcateStrings (&lpString, "create table ", lpTS->objectname, " ", (LPTSTR)0);
        if (!m) return NULL;
        if (lpTS->lpColumnHeader)
        {
            m = ConcateStrings (&lpString, "(", lpTS->lpColumnHeader, ")", (LPTSTR)0);
            if (!m) return NULL;
        }
        m = ConcateStrings (&lpString, " as ", lpTS->lpSelectStatement, (LPTSTR)0);
        if (!m) return NULL;

        lpWith = VDBA20xWithClauseSyntax (lpTS, &m);
        if (!m)
        {
            ESL_FreeMem (lpString);
            return NULL;
        }
        if (lpWith)
        {
            m = ConcateStrings (&lpString, " ", lpWith, (LPTSTR)0);
            ESL_FreeMem (lpWith);
            if (!m) return NULL;
        }

        return lpString;
    }
    m = ConcateStrings (&lpString, "create table ", (LPTSTR)QuoteIfNeeded(lpTS->objectname), " ", (LPTSTR)0);
    if (!m) return NULL;
    //
    // Generate the list of columns. At least one column !!!
    // Note: VDBA20xColumnFormat will generate column level constraint <unique> and <check>.
    //       If the column-level unique constraint is not defined by using the Unique Sub-Dialog Box.
    m = ConcateStrings (&lpString, " (",  (LPTSTR)0);
    if (!m) return NULL;
    ls = lpTS->lpColumns;
    while (ls)
    {
        lpCol    = (LPCOLUMNPARAMS)ls->lpObject;
        lpGenCol = VDBA20xColumnFormat (lpCol, NULL, lpTS);
        if (!lpGenCol)
        {
            ESL_FreeMem (lpString);
            return NULL;
        }
        if (bOneCol)
            m = ConcateStrings (&lpString, lpGenCol, (LPTSTR)0);
        else
            m = ConcateStrings (&lpString, ", ", lpGenCol, (LPTSTR)0);
        ESL_FreeMem (lpGenCol);
        bOneCol = FALSE;
        if (!m) return NULL;
        ls = ls->lpNext;
    }
    //
    // Here, we generate the table level constraints. The primary key will be generate as
    // Table Level Constraint.
    lpPk = VDBA20xPrimaryKeySyntax (&(lpTS->primaryKey), &m);
    if (!m)
    {
        ESL_FreeMem (lpString);
        return NULL;
    }
    if (lpPk)
    {
        m = ConcateStrings (&lpString, ", ", lpPk, (LPTSTR)0);
        ESL_FreeMem (lpPk);
        if (!m) return NULL;
    }
    // 
    // Unique Keys.
    lpUnique = VDBA20xUniqueKeySyntax (lpTS->lpUnique, &m);
    if (!m)
    {
        ESL_FreeMem (lpString);
        return NULL;
    }
    if (lpUnique)
    {
        m = ConcateStrings (&lpString, ", ", lpUnique, (LPTSTR)0);
        ESL_FreeMem (lpUnique);
        if (!m) return NULL;
    }
    // 
    // Checks
    lpCheck = VDBA20xCheckSyntax (lpTS->lpCheck, &m);
    if (!m)
    {
        ESL_FreeMem (lpString);
        return NULL;
    }
    if (lpCheck)
    {
        m = ConcateStrings (&lpString, ", ", lpCheck, (LPTSTR)0);
        ESL_FreeMem (lpCheck);
        if (!m) return NULL;
    }
    // 
    // Foreign Keys.
    lpFk = VDBA20xForeignKeySyntax (lpTS->objectname, lpTS->lpReferences, &m);
    if (!m)
    {
        ESL_FreeMem (lpString);
        return NULL;
    }
    if (lpFk)
    {
        m = ConcateStrings (&lpString, ", ", lpFk, (LPTSTR)0);
        ESL_FreeMem (lpFk);
        if (!m) return NULL;
    }
    //
    // Close the RIGHT parenthese ')'.
    m = ConcateStrings (&lpString, ") ", (LPTSTR)0);
    if (!m) return NULL;
    //
    // Generate the WITH CLAUSE.
    lpWith = VDBA20xWithClauseSyntax (lpTS, &m);
    if (!m)
    {
        ESL_FreeMem (lpString);
        return NULL;
    }
    if (lpWith)
    {
        m = ConcateStrings (&lpString, " ", lpWith, (LPTSTR)0);
        ESL_FreeMem (lpWith);
        if (!m) return NULL;
    }

    return lpString;
}


LPREFERENCEPARAMS VDBA20xForeignKey_Search(LPOBJECTLIST lpReferences, LPCTSTR lpszConstraint)
{
    LPREFERENCEPARAMS lpRef;
    LPOBJECTLIST      ls = lpReferences;
    while (ls)
    {
        lpRef = (LPREFERENCEPARAMS)ls->lpObject;
        if (lstrcmpi (lpRef->szConstraintName, lpszConstraint) == 0)
        {
            return lpRef;
        }
        ls = ls->lpNext;
    }
    return NULL;
}

LPREFERENCEPARAMS VDBA20xForeignKey_Lookup(LPOBJECTLIST lpReferences, LPREFERENCEPARAMS lpRef)
{
    LPREFERENCEPARAMS lpObj;
    LPOBJECTLIST      ls = lpReferences;
    int C1, C2;

    C1 = CountListItems (lpRef->lpColumns);
    while (ls)
    {
        lpObj = (LPREFERENCEPARAMS)ls->lpObject;
        C2 = CountListItems (lpObj->lpColumns);
        if (C1 == C2)
        {
            LPOBJECTLIST l1, l2;
            LPREFERENCECOLS lpCol1, lpCol2;
            BOOL bEqual = TRUE;

            l1 = lpRef->lpColumns;
            l2 = lpObj->lpColumns;
            while (l1 && l2)
            {
                lpCol1 = (LPREFERENCECOLS)l1->lpObject;
                lpCol2 = (LPREFERENCECOLS)l2->lpObject;
                if (lstrcmpi (lpCol1->szRefingColumn, lpCol2->szRefingColumn) != 0)
                {
                    bEqual = FALSE;
                    break;
                }
                l1 = l1->lpNext;
                l2 = l2->lpNext;
            }
            if (bEqual && lstrcmpi (lpObj->szRefConstraint, lpRef->szRefConstraint) == 0)
            {
                return lpObj;
            }
        }
        ls = ls->lpNext;
    }
    return NULL;
}


LPOBJECTLIST VDBA20xTableReferences_Remove(LPOBJECTLIST list, LPCTSTR lpszConstraint)
{
    LPREFERENCEPARAMS lpRef;
    LPOBJECTLIST      ls = list;
    LPOBJECTLIST      lpPrec = NULL, tmp;

    while (ls)
    {
        lpRef = (LPREFERENCEPARAMS)ls->lpObject;
        if (lstrcmpi (lpRef->szConstraintName, lpszConstraint) == 0)
        {
            tmp = ls;
            if (lpPrec == NULL)             // Remove the head of the list 
                list = list->lpNext;
            else
                lpPrec->lpNext = ls->lpNext;    // Remove somewhere else
            
            lpRef->constraintWithClause.pListLocation = StringList_Done (lpRef->constraintWithClause.pListLocation);
            FreeObjectList (lpRef->lpColumns);
            ESL_FreeMem (tmp->lpObject);
            ESL_FreeMem (tmp);
            return list;
        }
        lpPrec = ls;
        ls = ls->lpNext;
    }
    return list;
}

LPOBJECTLIST VDBA20xForeignKey_Add (LPOBJECTLIST list, LPREFERENCEPARAMS lpRef)
{
    LPOBJECTLIST lpObj = (LPOBJECTLIST)ESL_AllocMem (sizeof (OBJECTLIST));
    if (!lpObj)
    {
        return VDBA20xTableReferences_Done (list);
    }
    lpObj->lpObject = (LPVOID)lpRef;
    lpObj->lpNext   = (LPVOID)0;
    if (!list)
    {
        list       = lpObj;
    }
    else
    {
        LPOBJECTLIST prev = NULL;
        LPOBJECTLIST lp   = list;
       
        while (lp)
        {
            prev = lp;
            lp   = lp->lpNext;
        }
        prev->lpNext  = lpObj;
    }
    return list;  
}


LPOBJECTLIST VDBA20xTableReferences_Done (LPOBJECTLIST list)
{
    LPREFERENCEPARAMS lpr;
    LPOBJECTLIST ls = list;
    while (ls)
    {
        lpr = (LPREFERENCEPARAMS)ls->lpObject;
        ls = ls->lpNext;
        if (lpr->lpColumns)
            FreeObjectList (lpr->lpColumns);
        lpr->constraintWithClause.pListLocation = StringList_Done (lpr->constraintWithClause.pListLocation);
    }
    if (list)
        FreeObjectList (list);
    return NULL;
}

LPTLCUNIQUE VDBA20xTableUniqueKey_Done (LPTLCUNIQUE lpUniques)
{
    LPTLCUNIQUE lpTemp;
    while (lpUniques)
    {
        lpTemp    = lpUniques;
        lpUniques = lpUniques->next;
        StringList_Done (lpTemp->lpcolumnlist);
        lpTemp->constraintWithClause.pListLocation = StringList_Done (lpTemp->constraintWithClause.pListLocation);
        ESL_FreeMem (lpTemp);
    }
    return NULL;
}

LPTLCCHECK VDBA20xTableCheck_Done (LPTLCCHECK lpCheck)
{
    LPTLCCHECK lpTemp;
    while (lpCheck)
    {
        lpTemp  = lpCheck;
        lpCheck = lpCheck->next;
        ESL_FreeMem (lpTemp->lpexpression);
        ESL_FreeMem (lpTemp);
    }
    return NULL;
}

static BOOL VDBA20xTableUniqueKey_Equal  (LPTLCUNIQUE lpU1,    LPTLCUNIQUE lpU2)
{
    LPTLCUNIQUE ls, l1, l2;
    int iC1 = 0, iC2 = 0;

    l1 = lpU1;
    l2 = lpU2;
    while (l1)
    {
        iC1++;
        l1 = l1->next;
    }
    while (l2)
    {
        iC2++;
        l2 = l2->next;
    }
    if (iC1 != iC2)
        return FALSE;
    ls = lpU1;
    while (ls)
    {
        if (!VDBA20xTableUniqueKey_Lookup (lpU2,  ls))
            return FALSE;
        ls = ls->next;
    }
    ls = lpU2;
    while (ls)
    {
        if (!VDBA20xTableUniqueKey_Lookup (lpU1,  ls))
            return FALSE;
        ls = ls->next;
    }
    return TRUE;
}

static BOOL VDBA20xTableCheck_Equal  (LPTLCCHECK  lpCh1,   LPTLCCHECK lpCh2)
{
    LPTLCCHECK ls, l1, l2;
    int iC1 = 0, iC2 = 0;

    l1 = lpCh1;
    l2 = lpCh2;
    while (l1)
    {
        iC1++;
        l1 = l1->next;
    }
    while (l2)
    {
        iC2++;
        l2 = l2->next;
    }
    if (iC1 != iC2)
        return FALSE;
    ls = lpCh1;
    while (ls)
    {
        if (!VDBA20xTableCheck_Lookup (lpCh2,  ls))
            return FALSE;
        ls = ls->next;
    }
    ls = lpCh2;
    while (ls)
    {
        if (!VDBA20xTableCheck_Lookup (lpCh1,  ls))
            return FALSE;
        ls = ls->next;
    }
    return TRUE;
}

BOOL VDBA20xHaveUserDefinedTypes (LPTABLEPARAMS lpTS)
{
    LPOBJECTLIST   ls;
    LPCOLUMNPARAMS lpCol;

    assert (lpTS != NULL);
    ls = lpTS->lpColumns;
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        if (lstrcmpi (lpCol->tchszInternalDataType, lpCol->tchszDataType) != 0)
            return TRUE;
        ls = ls->lpNext;
    }
    return FALSE;
}

//
// Condition of not generating the with clause is:
// 'tchszIndexName' = "<use default>".
static LPTSTR VDBA20xIndexConstraintSyntax (INDEXOPTION* pIndex)
{
    TCHAR tchszBuffer [320];
    int m = 1;
    BOOL   bGenerateProperty = FALSE;
    BOOL   bGenerateName = FALSE;
    LPTSTR lpString  = NULL;
    LPSTRINGLIST ls = pIndex->pListLocation;

    switch (pIndex->nGenerateMode)
    {
    case INDEXOPTION_USEDEFAULT:
        //
        // Do not generate the syntax:
        return NULL;
    case INDEXOPTION_NOINDEX:
        m = ConcateStrings (&lpString, "with no index", (LPTSTR)0);
        if (!m) return NULL;
        return lpString;
        break;
    case INDEXOPTION_BASETABLE_STRUCTURE:
        m = ConcateStrings (&lpString, "with index = base table structure", (LPTSTR)0);
        if (!m) return NULL;
        return lpString;
        break;
    case INDEXOPTION_USEEXISTING_INDEX:
        break;
    case INDEXOPTION_USER_DEFINED:
        break;
    case INDEXOPTION_USER_DEFINED_PROPERTY:
        bGenerateProperty = TRUE;
        break;
    default:
        bGenerateProperty = FALSE;
        break;
    }
    if (pIndex->bAlter)
    {
        //
        // From the GetDetailInfo:
        bGenerateProperty = TRUE;
    }
    //
    // Name can be empty:
    if (pIndex->tchszIndexName[0] && pIndex->tchszIndexName[0] != '$')
    {
        bGenerateName = TRUE;
        wsprintf (tchszBuffer, "index = %s ", QuoteIfNeeded(pIndex->tchszIndexName));
        m = ConcateStrings (&lpString, " with (", tchszBuffer, (LPTSTR)0);
        if (!m) return NULL;
        if (!bGenerateProperty)
        {
            m = ConcateStrings (&lpString, ")", (LPTSTR)0);
            if (!m) return NULL;
            return lpString;
        }
    }

    if (!bGenerateProperty)
        return lpString;
    //
    // Generate the property only:
    if (!bGenerateName)
    {
        m = ConcateStrings (&lpString, " with (", (LPTSTR)0);
        if (!m) return NULL;
    }

    tchszBuffer [0] = '\0';
    //
    // Structure is always specified:
    if (pIndex->tchszStructure[0])
    {
        if (bGenerateName)
            lstrcat (tchszBuffer, ", structure = ");
        else
            lstrcat (tchszBuffer, " structure = ");
        lstrcat (tchszBuffer, pIndex->tchszStructure);
    }

    if (pIndex->tchszFillfactor[0])
    {
        lstrcat (tchszBuffer, ", fillfactor = ");
        lstrcat (tchszBuffer, pIndex->tchszFillfactor);
    }

    if (pIndex->tchszMinPage[0])
    {
        lstrcat (tchszBuffer, ", minpages = ");
        lstrcat (tchszBuffer, pIndex->tchszMinPage);
    }

    if (pIndex->tchszMaxPage[0])
    {
        lstrcat (tchszBuffer, ", maxpages = ");
        lstrcat (tchszBuffer, pIndex->tchszMaxPage);
    }

    if (pIndex->tchszLeaffill[0])
    {
        lstrcat (tchszBuffer, ", leaffill = ");
        lstrcat (tchszBuffer, pIndex->tchszLeaffill);
    }

    if (pIndex->tchszNonleaffill[0])
    {
        lstrcat (tchszBuffer, ", nonleaffill = ");
        lstrcat (tchszBuffer, pIndex->tchszNonleaffill);
    }

    if (pIndex->tchszAllocation[0])
    {
        lstrcat (tchszBuffer, ", allocation = ");
        lstrcat (tchszBuffer, pIndex->tchszAllocation);
    }

    if (pIndex->tchszExtend[0])
    {
        lstrcat (tchszBuffer, ", extend = ");
        lstrcat (tchszBuffer, pIndex->tchszExtend);
    }

    m = ConcateStrings (&lpString, tchszBuffer, (LPTSTR)0);
    if (!m) return NULL;

    if (ls != NULL)
    {
        BOOL bOneLoc = TRUE;
        m = ConcateStrings (&lpString, ", location = (", (LPTSTR)0);
        if (!m) return NULL;
        while (ls != NULL)
        {
            if (bOneLoc)
                m = ConcateStrings (&lpString, ls->lpszString, (LPTSTR)0);
            else
                m = ConcateStrings (&lpString, ", ", ls->lpszString, (LPTSTR)0);
            bOneLoc = FALSE;
            if (!m) return NULL;
            ls = ls->next;
        }
        m = ConcateStrings (&lpString, ")", (LPTSTR)0);
        if (!m) return NULL;
    }
    m = ConcateStrings (&lpString, ")", (LPTSTR)0);
    if (!m) return NULL;
    return lpString;
}

BOOL VDBA20xTablePrimaryKey_Copy (PRIMARYKEYSTRUCT* pDest, PRIMARYKEYSTRUCT* pSource)
{
    memset (pDest, 0, sizeof(PRIMARYKEYSTRUCT));
    memcpy (pDest, pSource, sizeof(PRIMARYKEYSTRUCT));
    pDest->pListTableColumn = StringList_Copy (pSource->pListTableColumn);
    pDest->pListKey = StringList_Copy (pSource->pListKey);
    INDEXOPTION_COPY (&(pDest->constraintWithClause), &(pSource->constraintWithClause));
    return TRUE;
}

BOOL VDBA20xTablePrimaryKey_Done(PRIMARYKEYSTRUCT* pStruct)
{
    pStruct->pListKey = StringList_Done (pStruct->pListKey);
    pStruct->pListTableColumn = StringList_Done (pStruct->pListTableColumn);
    pStruct->constraintWithClause.pListLocation = StringList_Done (pStruct->constraintWithClause.pListLocation);
    return TRUE;
}




//
// Duplicate the structure:
// nMember is one or more of TABLEPARAMS_XX above:
BOOL VDBA20xTableStruct_Copy (LPTABLEPARAMS pDest,   LPTABLEPARAMS pSource, UINT nMember)
{
    memset (pDest, 0, sizeof (TABLEPARAMS));
    memset (&(pDest->primaryKey), 0, sizeof(PRIMARYKEYSTRUCT));
    lstrcpy (pDest->DBName,    pSource->DBName);
    lstrcpy (pDest->szSchema,  pSource->szSchema);
    lstrcpy (pDest->objectname,pSource->objectname);

    if (nMember & TABLEPARAMS_COLUMNS)
        pDest->lpColumns = VDBA20xColumnList_Copy (pSource->lpColumns);
    if (nMember & TABLEPARAMS_FOREIGNKEY)
        pDest->lpReferences = VDBA20xTableReferences_CopyList (pSource->lpReferences);
    if (nMember & TABLEPARAMS_UNIQUEKEY)
        pDest->lpUnique = VDBA20xTableUniqueKey_CopyList (pSource->lpUnique);
    if (nMember & TABLEPARAMS_CHECK)
        pDest->lpCheck = VDBA20xTableCheck_CopyList (pSource->lpCheck);
    if (nMember & TABLEPARAMS_PRIMARYKEY)
        VDBA20xTablePrimaryKey_Copy (&(pDest->primaryKey), &(pSource->primaryKey));
    if (nMember & TABLEPARAMS_LISTDROPOBJ)
        pDest->lpDropObjectList = StringList_Copy (pSource->lpDropObjectList);
    return TRUE;
}

//
// Free the structure (some or all members):
BOOL VDBA20xTableStruct_Free (LPTABLEPARAMS pStruct, UINT nMember)
{
    if (nMember & TABLEPARAMS_COLUMNS)
        pStruct->lpColumns = VDBA20xColumnList_Done (pStruct->lpColumns);
    if (nMember & TABLEPARAMS_FOREIGNKEY)
        pStruct->lpReferences = VDBA20xTableReferences_Done (pStruct->lpReferences);
    if (nMember & TABLEPARAMS_UNIQUEKEY)
        pStruct->lpUnique = VDBA20xTableUniqueKey_Done (pStruct->lpUnique);
    if (nMember & TABLEPARAMS_CHECK)
        pStruct->lpCheck = VDBA20xTableCheck_Done (pStruct->lpCheck);
    if (nMember & TABLEPARAMS_PRIMARYKEY)
        VDBA20xTablePrimaryKey_Done (&(pStruct->primaryKey));
    if (nMember & TABLEPARAMS_LISTDROPOBJ)
        pStruct->lpDropObjectList = StringList_Done (pStruct->lpDropObjectList);
    return TRUE;
}

//
// L1 is included in L2 ?
BOOL StringList_Included (LPSTRINGLIST lpList1, LPSTRINGLIST lpList2)
{
    LPSTRINGLIST  l1 = lpList1;
    LPSTRINGLIST  l2 = lpList2;
    if (StringList_Count (l1) > StringList_Count (l2))
        return FALSE;
    while (l1)
    {
        if (!StringList_Search (l2, l1->lpszString))
            return FALSE;
        l1 = l1->next;
    }
    return TRUE;
}


BOOL StringList_Equal (LPSTRINGLIST lpList1, LPSTRINGLIST lpList2)
{
    LPSTRINGLIST  l1 = lpList1;
    LPSTRINGLIST  l2 = lpList2;
    if (StringList_Count (l1) != StringList_Count (l2))
        return FALSE;
    return (StringList_Included (l1, l2) && StringList_Included (l2, l1))? TRUE: FALSE;
}
