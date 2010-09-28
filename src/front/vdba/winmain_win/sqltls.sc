/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Visual DBA
**
**  Source : sqltls.sc
**  Sql tools
**
**  Authors : Lionel Storck
**            Emmanuel Blattes (errors management)
**            Schalk Philippe
**
**  History (after 04-May-1999):
**	06-May-1999 (schph01)
**	    Fixed GPF when the "remarks" control, when defining a
**	    replicator database was filled to its maximum of 80
**	    characters (buffersize increased from 80 to 81 for the
**	    trailing \0)
**	11-Jan-2000 (schph01)
**	    Bug #99966
**	    Copy the schema of the table in field :
**	    LPTABLEPARAMS->STORAGEPARAMS.objectowner.
**	 14-Jan-2000 (noifr01)
**	    Bug #100015
**	    Added the timestamp for Expiration dates, and centralized
**	    in sqltls.sc the formatting of the year with 4 digits (in
**	    order to support IngresII 2.0 installations, the _date4()
**	    has not been used. If the decision was made that we don't
**	    need to support any more such old versions, _date() should
**	    just be replaced by _date4() at 2 places in this source,
**	    and the calls to the temporary function AddCentury2ExpireDate()
**	    (and the function itself) should be removed 
**	20-Jan-2000 (uk$so01)
**	    Bug #100063
**	    Eliminate the undesired compiler's warning
**	24-Jan-2000 (noifr01)
**	    (SIR 100118) get the session description in the
**	    MonitorGetDetailSessionInfo() function. Mark it as "N/A"
**	    when connected to older versions of Ingres, which may not have
**	    the uptodate IMA tables (the code of the previous query is
**	    executed in this case, and just the new info is marked as N/A)
**	26-Jan-2000 (noifr01)
**	    (bug 100155) take into account the server pid/name in the query
**	    that gets the detail of a session (visible in Monitor windows)
**	14-Feb-2000 (somsa01)
**	    Added HP to the list of platforms requiring alignment on
**	    short boundary. All RISC platforms should be added to this list
**	    in future ports.
**  24-Feb-2000 (uk$so01)
**     Bug #100562, Add "nKeySequence" to the COLUMNPARAMS to indicate if the columns
**     is part of primary key or unique key.
**  01-Mar-2000 (schph01) 
**     Transfert Get_IIDATEFORMAT() function into dbatls.c
**     because no SQL statement are necessary to known the II_DATE_FORMAT.
**  22-May-2000 (schph01)
**     bug 99242 (DBCS) remove the function GetSQLstmType() never used in VDBA.
**  30-May-2000 (uk$so01)
**     bug 99242 (DBCS)  Change function prototype for DBCS compliant
**     and remove some unused code that is really complicated for DBCS
**  16-Nov-2000 (schph01)
**     SIR 102821 Add function to retrieve the comments for an table or view
**     and for columns.
**  08-Dec-2000 (schph01)
**     SIR 102823 retrieve "column_system_maintained" information when the
**     GetdetailInfo(OT_TABLE) is called.
**  04-Jan-2001 (noifr01)
**     (SIR 103640) don't put any more in the history of "errors" the positive
**      values 100 and 710 for sqlcode (that don't correspond to errors)
**  08-Jan-2001 (schph01)
**     SIR 102819 Add function SQLGetColumnList() to generate the
**     "create table as select" and get in IICOLUMNS the columns list, all
**     that is in the same session and without commit.
**  21-Feb-2001 (ahaal01)
**     Added AIX (rs4_us5) to the list of platforms requiring alignment
**      on short boundary.
**  21-Mar-2001 (noifr01)
**      (bug 104289) the index information for a check constraint was wrong in
**      the case where there was no index.
**  26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  09-May-2001 (schph01)
**     SIR 102509 manage raw data location.temporary disabled because of
**     concurrent changes in catalog.
**  06-Jun-2001(schph01)
**     (additional change for SIR 102509) update of previous change
**     because of backend changes
**  21-Jun-2001 (schph01)
**     (SIR 103694) (support of UNICODE datatypes) used definition in iicommon.h
**     for unicode data type.
**  27-Jun-2001 (noifr01)
**     (sir 103694) added functions for parsing on queries that include unicode
**     constants and executing "esqlc generated like" code by converting 
**     "unicode constants" to real unicode and passing them as parameters through
**     the "esqlc generated like" code
**  28-Jun-2001 (schph01)
**     (sir ) Add SQLGetTableStorageStructure() function to retrieve the actual
**     storage structure of the current table. launched from the new dialog to
**     change the page size in "alter table" dialog
**  18-Jul-2001 (noifr01)
**     (sir 103694) replaced U char with N for unicode constants
**  25-Oct-2001 (noifr01)
**     (sir 106156) extended ExecSQLImm functions for getting back the number
**     of affected rows
**  01-Nov-2001 (hanje04)
**	   Add Linux to list of platforms that require 'short' buffer alignment.
**     and cast 'szExpire' to (char *) before subtracting from 'pc2' to
**     stop compiler errors on Linux.
**  28-Mar-2002 (noifr01)
**     (sir 99596) removed additional unused resources/sources
**  08-Oct-2002 (noifr01)
**    (SIR 106648) moved Vdba_GetLenOfFileName() from collisio.sc to sqltls.sc,
**     since collisio.sc is no more used (given the usage of the Monitor
**     ActiveX)
**  15-Jan-2003 (schph01)
**     Sir 109430 Remove the translation of SQL_UNKOWN_TYPE into
**     INGTYPE_SHORTSECLBL or INGTYPE_SECURITYLBL in the IngToDbaTypes array.
**  18-Mar-2003 (schph01)
**     sir 107523 management of sequences
**  03-Nov-2003 (noifr01)
**     fixed errors from massive ingres30->main gui tools source propagation
**  06-Feb-2004 (schph01)
**     SIR 111752 Update the GetCapabilitiesInfo() function to retrieve DBEVENTS flag,
**     for managing DBEvent on Gateway.
**  01-Mar-2004 (schph01)
**     SIR 111752 manage lower case in GetCapabilitiesInfo() function for dbevents flag.
**  12-May-2004 (schph01)
**     SIR #111507 Add management for new column type bigint
**  07-Jun-2004 (schph01)
**     SIR #112460 Add function GetCatalogPageSize() to retrieve
**     "Catalogs Page Size" information.
**  10-Sep-2004 (uk$so01)
**    BUG #113002 / ISSUE #13671785 & 13671608, Removal of B1 Security
**    from the back-end implies the removal of some equivalent functionalities
**    from VDBA.
**  17-Dec-2004 (schph01)
**    BUG #113650 for the RTREE index, add statement to retrieve the Min,Max
**    Values
**  12-Oct-2006 (wridu01)
**    Sir 116835 Add support for Ansi Date/Time data types
**  17-Mar-2009 (drivi01)
**    In efforts to port to Visual Studio 2009, clean up some warnings.
**  23-Mar-2010 (drivi01)
**    Add VectorWise structure to the list.
**  12-May-2010 (drivi01)
**    Add new storage structures VECTORWISE and VECTORWISE_IX
**    to the list of recognized table storage structures in
**    StorageStruct structure.
** 30-Jun-2010 (drivi01)
**    Bug #124006
**    Add new BOOLEAN datatype.
** 17-Aug-2010 (thich01)
**    Add geospatial types to IISQ types.  Make changes to treat spatial
**    types like LBYTEs or NBR type as BYTE.
*/

#define BUGNOTNULLABLE     // remove when the bug is corrected at ingres level
#define STD_SQL_CALL
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include <sys/stat.h>
#include "port.h"
#include "error.h"
#include "dba.h"
#include "dbaset.h"
#include "dbaparse.h"
#include "dbadlg1.h"
#include "dbatime.h"
#include "settings.h"
#include "main.h"       // hwndMDIClient
exec sql include sqlca;
exec sql include sqlda;
#include "sqltls.h"
#include "domdata.h"
#include "replutil.h"
#define LONGVARCHARDISPCHAR 40
#define LVCNULLSTRING "<NULL>"
#define MAXLVC 2000
int * poslvc;
int * ilvc;
int nblvc = 0;
// for messages in log file
#include "dll.h"
#include "resource.h"
#include "dbaginfo.h"
#include "monitor.h"

#include "prodname.h"
#include "tchar.h"
#include "gl.h"
#include "iicommon.h"
#include "compat.h"
#include "nm.h"
#include "cv.h"

int VDBA2xGetTablePrimaryKeyInfo (LPTABLEPARAMS lpTS);
int VDBA2xGetTableUniqueKeyInfo  (LPTABLEPARAMS lpTS);
int VDBA2xGetTableCheckInfo      (LPTABLEPARAMS lpTS);

static int VDBA2xGetObjectCorruptionInfo  (LPSTORAGEPARAMS lpTS);

//
// lpIndex : pointer to the index structure to receive information about index:
// lpszConstraint: The constraint name.
int VDBA2xGetIndexConstraint (LPINDEXOPTION lpIndex, LPCTSTR lpszConstraint);
//
// Get the index enforcement of the constraint in 'lpCnst':
// Fill up the fields 'tchszIndex' and 'tchszSchema' of 'lpCnst'.
void VDBA2xGetIndexEnforcement (LPCONSTRAINTPARAMS lpCnst);

static void ChainRows(LPSQLROWSET lpSqlRowSet, LPROW lpnewrow);        
static LPOBJECTLIST AddSQLDescr(LPOBJECTLIST lpnewdescr, IISQLDA *pSQLda,
                                UCHAR *pSQLstm);
static int FetchAllRows(LPSQLROWSET lpSqlRowSet);
static LPSQLERRORINFO lpFirstEntry=NULL;  // begining of the linked lst for SQL error management 
static LPSQLERRORINFO lpNewestEntry=NULL;   // Last significant entry 
static LPSQLERRORINFO lpLastEntry=NULL;   // End of phisical chain
static int SQLCodeToDom(long sqlcode); // to be removed when SQLDynn OK

static BOOL   bSqlErrorManagement;
static BOOL   bSqlErrorSaveText;

INGDBATYPES IngToDbaTypes[] = {   // TO BE FINISHED SQL unknown types bellow
  { IISQ_CHR_TYPE,    INGTYPE_C,            0},   
  { IISQ_CHA_TYPE,    INGTYPE_CHAR,         0},
  { IISQ_TXT_TYPE,    INGTYPE_TEXT,         0},
  { IISQ_VCH_TYPE,    INGTYPE_VARCHAR,      0},
  { IISQ_LVCH_TYPE,   INGTYPE_LONGVARCHAR,  0},
  { IISQ_INT_TYPE,    INGTYPE_BIGINT,       8},
  { IISQ_BOO_TYPE,    INGTYPE_BOOLEAN,      0},
  { IISQ_INT_TYPE,    INGTYPE_INT8,         8},
  { IISQ_INT_TYPE,    INGTYPE_INT4,         4},
  { IISQ_INT_TYPE,    INGTYPE_INT2,         2},
  { IISQ_INT_TYPE,    INGTYPE_INT1,         1},
  { IISQ_DEC_TYPE,    INGTYPE_DECIMAL,      0},
  { IISQ_FLT_TYPE,    INGTYPE_FLOAT8,       8},
  { IISQ_FLT_TYPE,    INGTYPE_FLOAT4,       4},
  { IISQ_DTE_TYPE,    INGTYPE_DATE,         0},
  { DB_ADTE_TYPE,     INGTYPE_ADTE,         0},
  { DB_TMWO_TYPE,     INGTYPE_TMWO,         0},
  { DB_TMW_TYPE,      INGTYPE_TMW,          0},
  { DB_TME_TYPE,      INGTYPE_TME,          0},
  { DB_TSWO_TYPE,     INGTYPE_TSWO,         0},
  { DB_TSW_TYPE,      INGTYPE_TSW,          0},
  { DB_TSTMP_TYPE,    INGTYPE_TSTMP,        0},
  { DB_INYM_TYPE,     INGTYPE_INYM,         0},
  { DB_INDS_TYPE,     INGTYPE_INDS,         0},
  { DB_DTE_TYPE,      INGTYPE_IDTE,         0},
  { IISQ_MNY_TYPE,    INGTYPE_MONEY,        0},
  { IISQ_BYTE_TYPE,   INGTYPE_BYTE,         0},
  { IISQ_VBYTE_TYPE,  INGTYPE_BYTEVAR,      0},
  { IISQ_LBYTE_TYPE,  INGTYPE_LONGBYTE,     0},
  { IISQ_GEOM_TYPE,   INGTYPE_LONGBYTE,     0},
  { IISQ_POINT_TYPE,  INGTYPE_LONGBYTE,     0},
  { IISQ_MPOINT_TYPE, INGTYPE_LONGBYTE,     0},
  { IISQ_LINE_TYPE,   INGTYPE_LONGBYTE,     0},
  { IISQ_MLINE_TYPE,  INGTYPE_LONGBYTE,     0},
  { IISQ_POLY_TYPE,   INGTYPE_LONGBYTE,     0},
  { IISQ_MPOLY_TYPE,  INGTYPE_LONGBYTE,     0},
  { IISQ_NBR_TYPE,    INGTYPE_BYTE,         0},
  { IISQ_GEOMC_TYPE,  INGTYPE_LONGBYTE,     0},
  { IISQ_OBJ_TYPE,    INGTYPE_OBJKEY,       0},
  { IISQ_TBL_TYPE,    INGTYPE_TABLEKEY,     0},
  { DB_NCHR_TYPE,     INGTYPE_UNICODE_NCHR, 0},
  { DB_NVCHR_TYPE,    INGTYPE_UNICODE_NVCHR,0},
  { DB_LNVCHR_TYPE,   INGTYPE_UNICODE_LNVCHR,0},
  { -IISQ_CHR_TYPE,   INGTYPE_C,            0},
  { -IISQ_CHA_TYPE,   INGTYPE_CHAR,         0},
  { -IISQ_TXT_TYPE,   INGTYPE_TEXT,         0},
  { -IISQ_VCH_TYPE,   INGTYPE_VARCHAR,      0},
  { -IISQ_LVCH_TYPE,  INGTYPE_LONGVARCHAR,  0},
  { -IISQ_BOO_TYPE,   INGTYPE_BOOLEAN,      0},
  { -IISQ_INT_TYPE,   INGTYPE_BIGINT,       8},
  { -IISQ_INT_TYPE,   INGTYPE_INT8,         8},
  { -IISQ_INT_TYPE,   INGTYPE_INT4,         4},
  { -IISQ_INT_TYPE,   INGTYPE_INT2,         2},
  { -IISQ_INT_TYPE,   INGTYPE_INT1,         1},
  { -IISQ_DEC_TYPE,   INGTYPE_DECIMAL,      0},
  { -IISQ_FLT_TYPE,   INGTYPE_FLOAT8,       8},
  { -IISQ_FLT_TYPE,   INGTYPE_FLOAT4,       4},
  { -IISQ_DTE_TYPE,   INGTYPE_DATE,         0},
  { -DB_ADTE_TYPE,    INGTYPE_ADTE,         0},
  { -DB_TMWO_TYPE,    INGTYPE_TMWO,         0},
  { -DB_TMW_TYPE,     INGTYPE_TMW,          0},
  { -DB_TME_TYPE,     INGTYPE_TME,          0},
  { -DB_TSWO_TYPE,    INGTYPE_TSWO,         0},
  { -DB_TSW_TYPE,     INGTYPE_TSW,          0},
  { -DB_TSTMP_TYPE,   INGTYPE_TSTMP,        0},
  { -DB_INYM_TYPE,    INGTYPE_INYM,         0},
  { -DB_INDS_TYPE,    INGTYPE_INDS,         0},
  { -DB_DTE_TYPE,     INGTYPE_IDTE,         0},
  { -IISQ_MNY_TYPE,   INGTYPE_MONEY,        0},
  { -IISQ_BYTE_TYPE,  INGTYPE_BYTE,         0},
  { -IISQ_VBYTE_TYPE, INGTYPE_BYTEVAR,      0},
  { -IISQ_LBYTE_TYPE, INGTYPE_LONGBYTE,     0},
  { -IISQ_GEOM_TYPE,  INGTYPE_LONGBYTE,     0},
  { -IISQ_POINT_TYPE, INGTYPE_LONGBYTE,     0},
  { -IISQ_MPOINT_TYPE, INGTYPE_LONGBYTE,    0},
  { -IISQ_LINE_TYPE,  INGTYPE_LONGBYTE,     0},
  { -IISQ_MLINE_TYPE, INGTYPE_LONGBYTE,     0},
  { -IISQ_POLY_TYPE,  INGTYPE_LONGBYTE,     0},
  { -IISQ_MPOLY_TYPE, INGTYPE_LONGBYTE,     0},
  { -IISQ_NBR_TYPE,   INGTYPE_BYTE,         0},
  { -IISQ_GEOMC_TYPE, INGTYPE_LONGBYTE,     0},
  { -IISQ_OBJ_TYPE,   INGTYPE_OBJKEY,       0},
  { -IISQ_TBL_TYPE,   INGTYPE_TABLEKEY,     0},
  { -DB_NCHR_TYPE,    INGTYPE_UNICODE_NCHR, 0},
  { -DB_NVCHR_TYPE,   INGTYPE_UNICODE_NVCHR,0},
  { -DB_LNVCHR_TYPE,  INGTYPE_UNICODE_LNVCHR,0},
  { SQL_UNKOWN_TYPE,  INGTYPE_ERROR,        -1}};

int IngToDbaType(int iIngType, int iColLen)
{
   LPINGDBATYPES lpType=&IngToDbaTypes[0];
   while (lpType->iLen!=-1) {
      if (iIngType==lpType->iIngType)  // type found
         if (lpType->iLen==0)          // if length not significant  
            return lpType->iDbaType;   // return the dba type
         else if (lpType->iLen==iColLen) // if col length OKAY 
            return lpType->iDbaType;   // return the dba type
      lpType++;
   }
   return INGTYPE_ERROR;
}
int DbaToIngType(int iDbaType, int iColLen)
{
   LPINGDBATYPES lpType=&IngToDbaTypes[0];
   if ( iDbaType == INGTYPE_SECURITYLBL || iDbaType == INGTYPE_SHORTSECLBL )
      return SQL_UNKOWN_TYPE;
   while (lpType->iLen!=-1) {
      if (iDbaType==lpType->iDbaType)  // type found
         if (lpType->iLen==0)          // if length not significant  
            return lpType->iIngType;   // return the dba type
         else if (lpType->iLen==iColLen) // if col length OKAY 
            return lpType->iIngType;   // return the dba type
      lpType++;
   }
   return SQL_UNKOWN_TYPE;
}

typedef struct tagGATEWAYINGRESTYPE
{
    UCHAR GatewayType[MAXOBJECTNAME];
    int   IngType;
} GATEWAYINGRESTYPE, FAR *LPGATEWAYINGRESTYPE;

GATEWAYINGRESTYPE GatewayToIng [] = {
    {"CHAR"     , IISQ_CHA_TYPE },
    {"VARCHAR"  , IISQ_VCH_TYPE },
    {"INTEGER"  , IISQ_INT_TYPE },
    {"SMALLINT" , IISQ_INT_TYPE },
    {"DECIMAL"  , IISQ_DEC_TYPE },
    {"FLOAT"    , IISQ_FLT_TYPE },
    {"REAL"     , IISQ_FLT_TYPE },
    {"DATE"     , IISQ_DTE_TYPE },
    {""         ,-1             }};
int GatewayToIngType ( LPUCHAR tchszDataType)
{
   LPGATEWAYINGRESTYPE lpType = GatewayToIng;
   while (lpType->GatewayType[0] != '\0') {
       if (lstrcmpi(tchszDataType,lpType->GatewayType) == 0)
           return lpType->IngType;
   lpType++;
   }
   return SQL_UNKOWN_TYPE;
}

void SQLGetDbmsInfo(long *lplBio_cnt, long *lplCpu_ms)
{
   exec sql begin declare section;
   char Bio_cnt[15], Cpu_ms[15];
   exec sql end declare section;

   exec sql repeated select dbmsinfo('_bio_cnt') into :Bio_cnt; 
   
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   exec sql repeated select dbmsinfo('_cpu_ms') into :Cpu_ms;

   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (sqlca.sqlcode!=0) {
      *lplBio_cnt=-1;
      *lplCpu_ms=-1;
   }
   else {
      *lplBio_cnt=atol(Bio_cnt);
      *lplCpu_ms=atol(Cpu_ms);
   }
   return;
}

LPUCHAR SQLGetErrorText(long *lplret)
{  
   exec sql begin declare section;
   unsigned char *SQLErrorTxt;
   long lret=0;
   exec sql end declare section;

   SQLErrorTxt=ESL_AllocMem(SQLERRORTXTLEN);   // enough for now
   if (SQLErrorTxt) 
      exec sql inquire_sql (:SQLErrorTxt=errortext, :lret=errorno); 
   *lplret=lret;
   return SQLErrorTxt;
}

int Vdba_GetLenOfFileName( char *FileName)
{
	struct _stat StatusInfo;
	int result;
	
	if ( x_strlen(FileName) == 0	)
		return 0;

	/* Get data associated with "stat.c": */
	result = _stat( FileName, &StatusInfo );

	/* Check if statistics are valid: */
	if( result != 0 )
		return 0;
	return StatusInfo.st_size;
}

BOOL IsColumnNullable(LPOBJECTLIST lp, HSQL hsql, int iCol)
/******************************************************************
* Function :   Determines if the coulumn is NULLABLE or NOt       *
* Parameters :                                                    *
*     lpVlaue : Address of the variable value pointer             *
*     lp : Pointer to the front of the linked list                *
*     hsql : handle of the select statement                       *
*     iCol : column number                                        *
* returns :                                                       *
*     Column type or -1                                           *
******************************************************************/
{
   LPOBJECTLIST lpObj;
   LPSQLDA lpSqlda;

   if (!lp)
      return FALSE;
   lpObj=lp;

   while (lpObj) {      // looks for the given handle
      if (((LPSQLROWSET) (lpObj->lpObject))->hsql==hsql)
         break;
      lpObj=lpObj->lpNext;
   }

   if (!lpObj)      // handle not found, give up
      return FALSE;

   lpSqlda=((LPSQLROWSET) (lpObj->lpObject))->lpsqlda;

   if (iCol>=lpSqlda->sqld)   // column out of boudaries
      return FALSE;

   if (lpSqlda->sqlvar[iCol].sqltype<0)
      return TRUE;
   return FALSE;
}

int SqlGetHostInfos(void **lpValue,LPOBJECTLIST lp, HSQL hsql, int iCol, int * lpilen)
/******************************************************************
* Function :   Returns the type and a pointer of a host var       *
* Parameters :                                                    *
*     lpVlaue : Address of the variable value pointer             *
*     lp : Pointer to the front of the linked list                *
*     hsql : handle of the select statement                       *
*     iCol : column number                                        *
* returns :                                                       *
*     Column type or -1                                           *
******************************************************************/
{
   LPOBJECTLIST lpObj;
   LPSQLDA lpSqlda;
   LPROW lpRow;
   int iret, iSqlType;
   *lpilen=0;
   if (!lp)
      return 0;
   lpObj=lp;

   while (lpObj) {      // looks for the given handle
      if (((LPSQLROWSET) (lpObj->lpObject))->hsql==hsql)
         break;
      lpObj=lpObj->lpNext;
   }

   if (!lpObj) {     // handle not found, give up
      if (lpValue)
         *lpValue=NULL;
      return -1;
   }

   lpSqlda=((LPSQLROWSET) (lpObj->lpObject))->lpsqlda;

   if (iCol>=lpSqlda->sqld) {  // column out of boudaries
      if (lpValue)
         *lpValue=NULL;
      return -1;
   }
   if (lpValue) {
      lpRow=((LPSQLROWSET) (lpObj->lpObject))->lpcurrrow;
      *lpValue=lpRow->lpcol[iCol];

      if (lpRow->sNullInd[iCol]!=0) // Var is NULL 
         return SQL_NULL_TYPE;
   }

   iSqlType=lpSqlda->sqlvar[iCol].sqltype;

   if (iSqlType<0)   // if null indicator, ignore it 
      iSqlType*=-1;

   switch (iSqlType) {        // TO BE FINISHED FOR SECURITY LABEL TYPES
     case IISQ_DTE_TYPE :  
       if (lpValue)  
          iret=SQL_CHAR_TYPE; // give the type of the host returned.
       else
          iret=SQL_DATE_TYPE; // Not host to be returned, give col type.
       break;
     case IISQ_CHA_TYPE :  
       iret=SQL_CHAR_TYPE;
       break;
     case IISQ_VCH_TYPE :  
       iret=SQL_VCHAR_TYPE; 
       break;
     case IISQ_DEC_TYPE :  
     case IISQ_MNY_TYPE :  
     case IISQ_FLT_TYPE :  
       iret=SQL_FLT_TYPE;
       *lpilen=lpSqlda->sqlvar[iCol].sqllen;
       break;
     case IISQ_INT_TYPE :  
       iret=SQL_INT_TYPE;   
       break;
     case IISQ_VBYTE_TYPE :
     case IISQ_BYTE_TYPE : 
     case IISQ_NBR_TYPE : 
     case IISQ_LBYTE_TYPE :
     case IISQ_GEOM_TYPE :
     case IISQ_POINT_TYPE :
     case IISQ_MPOINT_TYPE :
     case IISQ_LINE_TYPE :
     case IISQ_MLINE_TYPE :
     case IISQ_POLY_TYPE :
     case IISQ_MPOLY_TYPE :
     case IISQ_GEOMC_TYPE :
       // Added Emb Jan 06, 97
       *lpilen=lpSqlda->sqlvar[iCol].sqllen;
       iret = SQL_BYTE_TYPE;
       break;
     default :             
       iret=SQL_UNKOWN_TYPE;
       *lpilen = lpSqlda->sqlvar[iCol].sqllen;  // Added Emb Jan 06, 97 to keep next columns alignment safe
   }
      
   return iret;
}


int SqlLocateFirstRow(LPOBJECTLIST lp, HSQL hsql)
/******************************************************************
* Function :   LOcates the firsrt row for a select                *
* Parameters :                                                    *
*     lp : Pointer to the front of the linked list                *
*     hsql : handle of the select statement                       *
* returns :                                                       *
*     RES_SUCCESS, RES_ENDOFDATA or RES_ERR                       *
******************************************************************/
{
   LPOBJECTLIST lpObj;
   LPSQLROWSET lpRowSet;

   if (!lp)
      return -1;
   lpObj=lp;

   while (lpObj) {      // looks for the given handle
      if (((LPSQLROWSET) (lpObj->lpObject))->hsql==hsql)
         break;
      lpObj=lpObj->lpNext;
   }

   if (!lpObj)       // handle not found, give up
      return RES_ERR;

   lpRowSet=lpObj->lpObject;
   lpRowSet->lpcurrrow=lpRowSet->lpfirstrow;

   if (!lpRowSet->lpcurrrow)
      return RES_ENDOFDATA;

   return RES_SUCCESS;

}
int SqlLocateNextRow(LPOBJECTLIST lp, HSQL hsql)
/******************************************************************
* Function :   Locates the subsequent for a select                *
* Parameters :                                                    *
*     lp : Pointer to the front of the linked list                *
*     hsql : handle of the select statement                       *
* returns :                                                       *
*     RES_SUCCESS, RES_ENDOFDATA or RES_ERR                       *
******************************************************************/
{
   LPOBJECTLIST lpObj;
   LPSQLROWSET lpRowSet;

   if (!lp)
      return -1;
   lpObj=lp;

   while (lpObj) {      // looks for the given handle
      if (((LPSQLROWSET) (lpObj->lpObject))->hsql==hsql)
         break;
      lpObj=lpObj->lpNext;
   }

   if (!lpObj)       // handle not found, give up
      return RES_ERR;

   lpRowSet=lpObj->lpObject;

   if (!lpRowSet->lpcurrrow)
      return RES_ERR;

   lpRowSet->lpcurrrow=lpRowSet->lpcurrrow->Next;

   if (!lpRowSet->lpcurrrow)
      return RES_ENDOFDATA;

   return RES_SUCCESS;

}


LPUCHAR SqlGetHostName(LPUCHAR lpColName, LPOBJECTLIST lp,
                     HSQL hsql, int iColNumber)
/******************************************************************
* Function :   Returns the name of an host variable of an sql     *
*              statement                                          *   
* Parameters :                                                    *
*     lp : Pointer to the front of the linked list                *
*     hsql : handle of the select statement                       *
*     iColNumber : Rank of the column whose name is to be         *
*                  returned (relative to 0)                       *
*     lpColName : Colmumn name to be returned                     *   
* returns :                                                       *
*     Pointer to the colmun name or NULL                          *
******************************************************************/
{
   LPOBJECTLIST lpObj;
   LPSQLDA lpSqlda;
   if (!lp)
      return NULL;
   lpObj=lp;

   while (lpObj) {      // looks for the given handle
      if (((LPSQLROWSET) (lpObj->lpObject))->hsql==hsql)
         break;
      lpObj=lpObj->lpNext;
   }

   if (!lpObj)       // handle not found, give up
      return NULL;

   lpSqlda=((LPSQLROWSET) (lpObj->lpObject))->lpsqlda;

   if (iColNumber>=lpSqlda->sqld)
      return NULL;
   fstrncpy(lpColName, lpSqlda->sqlvar[iColNumber].sqlname.sqlnamec,
            lpSqlda->sqlvar[iColNumber].sqlname.sqlnamel+1);
   return lpColName;
}


int SqlGetNumberOfHosts(LPOBJECTLIST lp, HSQL hsql)
/******************************************************************
* Function :   Returns the number of host variables an sql select *
*              envolves.                                          *   
* Parameters :                                                    *
*     lp : Pointer to the front of the linked list                *
*     hsql : handle of the select statement                       *
* returns :                                                       *
*     Number of host variables (0 to n) or -1 (errors)            *
******************************************************************/
{
   LPOBJECTLIST lpObj;

   if (!lp)
      return -1;
   lpObj=lp;

   while (lpObj) {      // looks for the given handle
      if (((LPSQLROWSET) (lpObj->lpObject))->hsql==hsql)
         break;
      lpObj=lpObj->lpNext;
   }

   if (!lpObj)       // handle not found, give up
      return -1;

   return ((LPSQLROWSET) (lpObj->lpObject))->lpsqlda->sqld;
}
void FreeSqlAreas(LPOBJECTLIST lp)
/******************************************************************
* Function :   Frees a linked list of SQLROWSET and all related   *
*              objects.                                           *   
* Parameters :                                                    *
*     lp : Pointer to the front of the linked list                *
* returns :                                                       *
*     void                                                        *
******************************************************************/
{
   LPOBJECTLIST lpObj;
   LPSQLROWSET lpRowSet;
   LPROW lpRow;
   void *lpToFree;

   if (!lp)
      return;
   lpObj=lp;

   while (lpObj) {
      lpRowSet=lpObj->lpObject;
      if (lpRowSet) {
         if (lpRowSet->lpsqlda);
            ESL_FreeMem((void *)lpRowSet->lpsqlda);    // free SQLDA
         if (lpRowSet->lpsqlstm);
            ESL_FreeMem((void *)lpRowSet->lpsqlstm);   // free SQL statement   
         lpRow=lpRowSet->lpfirstrow;
         while (lpRow) {
            ESL_FreeMem(lpRow->lpcol[0]);              // free host variables   
            lpToFree=lpRow;
            lpRow=lpRow->Next;
            ESL_FreeMem(lpToFree);     // free one row
         }
      }
      ESL_FreeMem(lpRowSet);           // free the rowset structure
      lpToFree=lpObj;
      lpObj=lpObj->lpNext;
      ESL_FreeMem(lpToFree);           // Free the object list structure
   }
   return;
}
LPOBJECTLIST FreeSqlStm(LPOBJECTLIST lp, HSQL *hsql)
/******************************************************************
* Function :   Remove an sql select from the linked list of       *  
*              SQLROWSET. Frees all related memory.               *   
* Parameters :                                                    *
*     lp : Pointer to the front of the linked list                *
*     hsql : handle of the sql statement to be removed            *
* returns :                                                       *
*     New pointer to the front element of the list                *
******************************************************************/
{
   LPOBJECTLIST lpObj, lpPrevObj=NULL, lpToReturn;
   LPSQLROWSET lpRowSet;
   LPROW lpRow;
   void *lpToFree;

   if (!lp)
      return lp;
   lpObj=lp;

   while (lpObj) {      // looks for the given handle
      if (((LPSQLROWSET) (lpObj->lpObject))->hsql==*hsql)
         break;
      lpPrevObj=lpObj;
      lpObj=lpObj->lpNext;
   }

   if (!lpObj)       // handle not found, give up
      return NULL;


   lpRowSet=lpObj->lpObject;
   if (lpRowSet) {
      if (lpRowSet->lpsqlda);
         ESL_FreeMem((void *)lpRowSet->lpsqlda);    // free SQLDA
      if (lpRowSet->lpsqlstm);
         ESL_FreeMem((void *)lpRowSet->lpsqlstm);   // free SQL statement   
      lpRow=lpRowSet->lpfirstrow;
      while (lpRow) {
         ESL_FreeMem(lpRow->lpcol[0]);              // free host variables   
         lpToFree=lpRow;
         lpRow=lpRow->Next;
         ESL_FreeMem(lpToFree);     // free one row
      }
   }
   if (lpPrevObj!=lp) {                 // this entry wasn't the first one
      lpPrevObj->lpNext=lpObj->lpNext;  // take elm out of the list
      lpToReturn=lp;
   }
   else 
      lpToReturn=lp->lpNext;        // Front of list changes;


   ESL_FreeMem(lpRowSet);           // free the rowset structure
   ESL_FreeMem(lpObj);              // Free the object list structure
   return lpToReturn;
}


static void ChainRows(LPSQLROWSET lpSqlRowSet, LPROW lpnewrow)         
/******************************************************************
* Function :   Chain a new row at the end of a linked list.       *
* Parameters :                                                    *
*     lpSqlRowSet : SQLROWSET structure, containing the first and *
*              last row pointers of the linked list.              *
*     lpnewrow : row to be added.                                 *
* returns :                                                       *
*     void                                                        *
******************************************************************/
{
   if (!lpSqlRowSet)
      return;

   if (!lpnewrow)
      return;

   lpnewrow->Next=NULL;                      /* last element */
   lpnewrow->Prev=lpSqlRowSet->lplastrow;    /* 'old' last is new prev */

   if (!lpnewrow->Prev) { /* new element is the only one in list */
      lpSqlRowSet->lpfirstrow=lpSqlRowSet->lplastrow=lpnewrow;
      return;     
   }
   lpSqlRowSet->lplastrow->Next=lpnewrow;   /* chain prev to new one */
   lpSqlRowSet->lplastrow=lpnewrow;         /* reset end of list ptr */ 
   return;
}

static LPOBJECTLIST AddSQLDescr(LPOBJECTLIST lpnewdescr, IISQLDA *pSQLda,
                                UCHAR *pSQLstm)
/******************************************************************
* Function : Add a new SQL descripor at the front of the list.    *
* Parameters :                                                    *
*           lpnewdescr : Pointer to the front elm of the list or  *   
*              NULL when first time called.                       *
*           pSQLda : sqlda of the statement to be added.          *
*           pSQLstm : statement to be added.                      *
* returns :                                                       *
*           Pointer to the newly created SQLROWSET (new front elm)*
*           or NULL.                                              *
******************************************************************/

{

   /* allocate the new SQLROWSET and chain it */

   lpnewdescr=AddListObject(lpnewdescr, sizeof(SQLROWSET));
   if (!lpnewdescr) {
      myerror(ERR_ALLOCMEM);
      return FALSE;
   }
   ((LPSQLROWSET) (lpnewdescr->lpObject))->lpsqlda=pSQLda;    /* sqlda ptr */ 
   ((LPSQLROWSET) (lpnewdescr->lpObject))->lpsqlstm=pSQLstm;  /* sqlstm ptr */
   ((LPSQLROWSET) (lpnewdescr->lpObject))->lpfirstrow=NULL;   /* row pointers */
   ((LPSQLROWSET) (lpnewdescr->lpObject))->lpcurrrow=NULL;
   ((LPSQLROWSET) (lpnewdescr->lpObject))->lplastrow=NULL;
   return lpnewdescr;
}   

static int FetchAllRows(LPSQLROWSET lpSqlRowSet)

/******************************************************************
* Function :   This is a boot strap to the SQLIMM function.       *
*              It allocates the host variable that SQLIMM will    *
*              return, it updates the SQLDA host pointers.        *
*              Then calls SQLIMM giving him the name of the proc  *
*              which fills up the structure for each row. (see    *   
*              sql imm.).                                         *
* Parameters :                                                    *
*     lpSqlRowSet : A row set structure                           *
* returns :                                                       *
*     either RES_SUCCES if the row was returned, RES_ENDOFDATA    *
*     if no rows are found or RES_ERR if any troubles             *
******************************************************************/
{
   int i,iret, BytesToAlloc=0;
   LPBYTE lptemp;            // to be one byte in length (care for DBCS)
   FILLHOSTPRM FillHostPrm;
   int nSqlType;
   BOOL bNullVar;

   /* Scan the SQLVAR to calculate the size of the buffer           */
   /* to be allocated for the hosts variables.                      */
   /* Change the sql types to a compatible C type, if appropriate   */

   nblvc = 0;
   poslvc=(int *)ESL_AllocMem(sizeof(int) * MAXLVC);
   ilvc  =(int *)ESL_AllocMem(sizeof(int) * MAXLVC);
   if (!poslvc || !ilvc)
     return RES_ERR;
   for (i=0; i<lpSqlRowSet->lpsqlda->sqld; i++) {
      bNullVar=FALSE;
#ifdef BUGNOTNULLABLE   // remove when bug is corrected at ingres level
      bNullVar=TRUE;    // Force to nullable whatever INGRES says   
      BytesToAlloc+=sizeof(short);
      if (lpSqlRowSet->lpsqlda->sqlvar[i].sqltype<0)         // null indic
         nSqlType=lpSqlRowSet->lpsqlda->sqlvar[i].sqltype*-1;
      else {
         nSqlType=lpSqlRowSet->lpsqlda->sqlvar[i].sqltype;   
         lpSqlRowSet->lpsqlda->sqlvar[i].sqltype*=-1;
      }

#else // to be kept
      if (lpSqlRowSet->lpsqlda->sqlvar[i].sqltype<0) {        // null indic
         bNullVar=TRUE;
         BytesToAlloc+=sizeof(short);
         nSqlType=lpSqlRowSet->lpsqlda->sqlvar[i].sqltype*-1;
      }
      else
         nSqlType=lpSqlRowSet->lpsqlda->sqlvar[i].sqltype;   
#endif
      switch (nSqlType) {
         case IISQ_DTE_TYPE :                // date length :
            BytesToAlloc+=IISQ_DTE_LEN+1;    // +1 for '\0'
            lpSqlRowSet->lpsqlda->sqlvar[i].sqllen=IISQ_DTE_LEN;
            if (lpSqlRowSet->lpsqlda->sqlvar[i].sqltype<0) 
               lpSqlRowSet->lpsqlda->sqlvar[i].sqltype=IISQ_CHA_TYPE*-1;
            else
               lpSqlRowSet->lpsqlda->sqlvar[i].sqltype=IISQ_CHA_TYPE;
            break;
         case IISQ_DEC_TYPE :                // decimal and money are ...
         case IISQ_MNY_TYPE :                // SQL/C float, C double.
            if (bNullVar)
               lpSqlRowSet->lpsqlda->sqlvar[i].sqltype=IISQ_FLT_TYPE*-1;
            else
               lpSqlRowSet->lpsqlda->sqlvar[i].sqltype=IISQ_FLT_TYPE;
            lpSqlRowSet->lpsqlda->sqlvar[i].sqllen=sizeof(double);
            BytesToAlloc+=lpSqlRowSet->lpsqlda->sqlvar[i].sqllen;
            break;
         case IISQ_BYTE_TYPE :               // byte and char length ....
         case IISQ_CHA_TYPE :                // is sqlen +1 ('\0')
         case IISQ_NBR_TYPE : 
            BytesToAlloc+=lpSqlRowSet->lpsqlda->sqlvar[i].sqllen+1;
            break;
         case IISQ_VBYTE_TYPE :              // Varbyte and varchar length 
         case IISQ_VCH_TYPE :                // sqllen + short(size) +1 (\0)
            BytesToAlloc+=lpSqlRowSet->lpsqlda->sqlvar[i].sqllen
                          +sizeof(short)+1;
            break;
         case IISQ_FLT_TYPE :                // Float and integer ...
            BytesToAlloc+=lpSqlRowSet->lpsqlda->sqlvar[i].sqllen;
            break;
         case IISQ_INT_TYPE :                // sqllen gives the length.
            BytesToAlloc+=sizeof(long); // lpSqlRowSet->lpsqlda->sqlvar[i].sqllen;
            lpSqlRowSet->lpsqlda->sqlvar[i].sqllen=sizeof(long);
            break;
      	 case IISQ_LVCH_TYPE:
            ilvc  [nblvc] = i;
            poslvc[nblvc] = BytesToAlloc;
            nblvc++;
            BytesToAlloc+=LONGVARCHARDISPCHAR+1;    // +1 for '\0'
            lpSqlRowSet->lpsqlda->sqlvar[i].sqllen=LONGVARCHARDISPCHAR;
            if (lpSqlRowSet->lpsqlda->sqlvar[i].sqltype<0) 
               lpSqlRowSet->lpsqlda->sqlvar[i].sqltype=IISQ_CHA_TYPE*-1;
            else
               lpSqlRowSet->lpsqlda->sqlvar[i].sqltype=IISQ_CHA_TYPE;
            break;
         case IISQ_LBYTE_TYPE :              // Long byte.
         case IISQ_GEOM_TYPE :
         case IISQ_POINT_TYPE :
         case IISQ_MPOINT_TYPE :
         case IISQ_LINE_TYPE :
         case IISQ_MLINE_TYPE :
         case IISQ_POLY_TYPE :
         case IISQ_MPOLY_TYPE :
         case IISQ_GEOMC_TYPE :
         default :                           // Integer.
            return RES_ERR;
      }
   }

   /**** Alloc the buffer which will contain the concatenated hostvalues ****/

#ifdef MAINWIN
#if defined(su4_us5) || defined(hp8_us5) || defined(hpb_us5) || \
    defined(rs4_us5) || defined(int_lnx)
   BytesToAlloc += ( sizeof(short) - 1 ) * lpSqlRowSet->lpsqlda->sqld;
#else
#error platform-specific code missing
#endif  /* su4_us5 hp8_us5 hpb_us5 rs4_us5 */
#endif  /* MAINWIN */

   FillHostPrm.ibufln=BytesToAlloc;
   FillHostPrm.lpsqlhostbfr=ESL_AllocMem(BytesToAlloc);  // allocate
   if (!FillHostPrm.lpsqlhostbfr)
      return RES_ERR;
   for (i=0;i<nblvc;i++)
     x_strcpy(FillHostPrm.lpsqlhostbfr+poslvc[i],LVCNULLSTRING);

   /* Update the sqlda variable part to provide SQLDYNN with the */
   /* host pointers                                              */

   lptemp=FillHostPrm.lpsqlhostbfr;
   for (i=0; i<lpSqlRowSet->lpsqlda->sqld; i++) {
      if (lpSqlRowSet->lpsqlda->sqlvar[i].sqltype>=0) 
         lpSqlRowSet->lpsqlda->sqlvar[i].sqlind=NULL;    // no NULL indicator
      else {
         lpSqlRowSet->lpsqlda->sqlvar[i].sqlind=(short *) lptemp; 
         lptemp+=sizeof(short);
      }
      lpSqlRowSet->lpsqlda->sqlvar[i].sqldata=lptemp;
      lptemp+=lpSqlRowSet->lpsqlda->sqlvar[i].sqllen;  // adjust pointer
      if (lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == IISQ_BYTE_TYPE ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == IISQ_CHA_TYPE  ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == IISQ_DTE_TYPE  ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == IISQ_NBR_TYPE  ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == -IISQ_NBR_TYPE  ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == -IISQ_BYTE_TYPE ||            
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == -IISQ_CHA_TYPE  ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == -IISQ_DTE_TYPE)
         lptemp+=1;
      if (lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == IISQ_VBYTE_TYPE ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == IISQ_VCH_TYPE   ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == -IISQ_VBYTE_TYPE ||
          lpSqlRowSet->lpsqlda->sqlvar[i].sqltype == -IISQ_VCH_TYPE)
         lptemp+=3;
#ifdef MAINWIN
#if defined(su4_us5) || defined(hp8_us5) || defined(hpb_us5) || \
    defined(rs4_us5) || defined(int_lnx)
      {
        // need to align on short boundary for sparc stations
        DWORD dwAddress;
        int   modulo;
        dwAddress = (DWORD)lptemp;
        modulo = dwAddress % sizeof(short);
        if (modulo) {
          dwAddress += sizeof(short) - modulo;
          lptemp = (LPBYTE)dwAddress;
        }
      }
#else
#error platform-specific code missing
#endif  /* su4_us5 hp8_us5 hpb_us5 rs4_us5 */
#endif  /* MAINWIN */

   }

   /* Execute the statement, FillHost will get control for each row */
   
   FillHostPrm.lpsqlrowset=lpSqlRowSet;
   iret=ExecSQLImm(lpSqlRowSet->lpsqlstm, FALSE, lpSqlRowSet->lpsqlda,
         FillHosts, (void *) &FillHostPrm);

   ESL_FreeMem(FillHostPrm.lpsqlhostbfr);    // free the host var bfr
   ESL_FreeMem(poslvc);
   ESL_FreeMem(ilvc);
   return iret;
}

static BOOL bLastErr50 = FALSE;

BOOL isLastErr50()
{
   return bLastErr50;
}

#define MAXUNICODECONSTANTLEN 2000

static BOOL GetNextUniConstant(LPUCHAR pc,wchar_t * bufuni,char **ppcstartuni, char **ppcafteruni)

{
	while (*pc) {
		switch (*pc) {
			case '\'' :
				pc=_tcschr(pc+1,'\''); /* sufficient to cover also the 'escaping' of the ' character */
				if (!pc)
					return FALSE;
				break;
			case '\"':
				pc=_tcschr(pc+1,'\"'); /* sufficient to cover also the 'escaping' of the " character */
				if (!pc)
					return FALSE;
				break;
			case 'N':
				if (*(pc+1)=='\'') {
					char *pc1,*pc2;
					char BufAnsiString[MAXUNICODECONSTANTLEN+1];
					*ppcstartuni = pc;
					pc1=_tcsinc(pc);
					pc1=_tcsinc(pc1);
					pc2=BufAnsiString;
					while (TRUE) {
						if (*pc1=='\0') 
							return FALSE;
						if (*pc1=='\'') {
							if (*(pc1+1)=='\'')  /* escaped ' character */
								pc1=_tcsinc(pc1);
							else {
								int ires;
								*ppcafteruni = pc1+1;
								*pc2='\0';
								ires = MultiByteToWideChar( CP_ACP, 0, BufAnsiString,-1,bufuni,MAXUNICODECONSTANTLEN+1);
								if (ires== 0)
									return FALSE;
								return TRUE;
							}
						}
						if ( (pc2-BufAnsiString)> MAXUNICODECONSTANTLEN -3)
							return FALSE;
						CMcpyinc(pc1,pc2);
					}
				}
				break;
		}
		pc = _tcsinc(pc);
	}
	return FALSE;
}
BOOL UnicodeMngmtNeeded(LPUCHAR lprequest)
{
	char * pc=lprequest;
	char * stmtInsert = "insert";
	char * stmtUpdate = "update";
	char * stmtCreate = "create";

	char * pcstartuni, *pcafteruni;
	wchar_t bufuni[MAXUNICODECONSTANTLEN+1];

	while (*pc==' ')
		pc++; /* skip leading spaces */


	while (TRUE) {
		if (_tcsnicmp(pc, stmtInsert,_tcslen(stmtInsert))==0){
			pc += _tcslen(stmtInsert);
			break;
		}
#if 0 
		if (_tcsnicmp(pc, stmtUpdate,_tcslen(stmtUpdate))==0){
			pc += _tcslen(stmtUpdate);
			break;
		}
		if (_tcsnicmp(pc, stmtCreate,_tcslen(stmtCreate))==0){
			char * Table  = "table" ;
			char * View   = "view"  ;
			pc += _tcslen(stmtCreate);
			while (*pc==' ')
				pc++; 
			if (_tcsnicmp(pc, Table,_tcslen(Table))==0){
				pc += _tcslen(Table);
				break;
			}
			if (_tcsnicmp(pc, View,_tcslen(View))==0){
				pc += _tcslen(View);
				break;
			}
		}
#endif
		return FALSE;
	}
	return GetNextUniConstant(pc,bufuni,&pcstartuni, &pcafteruni);
}

static char *strwhere = "where";
static char * Go2WhereClause(char *rawstm)
{
	char * pc=rawstm;
	char * strselect = "select";

	while (*pc==' ')
		pc++; /* skip leading spaces */

	if (_tcsnicmp(pc, strselect,_tcslen(strselect))!=0)
		return (char *)0;
		
	pc += _tcslen(strselect);

	while (*pc) {
		switch (*pc) {
			case '\'' :
				pc=_tcschr(pc+1,'\''); /* sufficient to cover also the 'escaping' of the ' character */
				if (!pc)
					return FALSE;
				break;
			case '\"':
				pc=_tcschr(pc+1,'\"'); /* sufficient to cover also the 'escaping' of the " character */
				if (!pc)
					return FALSE;
				break;
			default:
				if (_tcsnicmp(pc, strwhere,_tcslen(strwhere))==0)
					return pc;
				break;
		}
		pc = _tcsinc(pc);
	}
	return (char *)0;
}

BOOL HasUniConstInWhereClause(char *rawstm,char *pStmtWithoutWhereClause)
{
	char * pcstartuni, *pcafteruni;
	wchar_t bufuni[MAXUNICODECONSTANTLEN+1];
	BOOL bRes;

	char * pc = Go2WhereClause(rawstm);
	if (!pc)
		return FALSE;

	bRes = GetNextUniConstant(pc,bufuni,&pcstartuni, &pcafteruni);
	if (bRes && pStmtWithoutWhereClause )
		fstrncpy(pStmtWithoutWhereClause,rawstm, 1+ (pc-rawstm) );
	return bRes;
}

BOOL ManageDeclareAndOpenCur4Uni(char *cname, char *rawstmt) 
{
	wchar_t bufuni[MAXUNICODECONSTANTLEN+1];
	char * pcstartuni, *pcafteruni;

	char * pbuftemp;
	char *pc;
	char *pcstartsearch;

	pbuftemp =ESL_AllocMem(_tcslen(rawstmt)+1);
	if (!pbuftemp)
		return RES_ERR;
	
	EnableSqlSaveText();

	IIsqInit(&sqlca);
	IIcsOpen(cname,0,0);

	pcstartsearch = Go2WhereClause(rawstmt);
	if (!pcstartsearch)
		pcstartsearch=rawstmt;
	pc = rawstmt;

	while (*pc) {
		BOOL bres = GetNextUniConstant(pcstartsearch,bufuni,&pcstartuni, &pcafteruni);
		if (!bres)  {
			IIwritio(0,(short *)0,1,32,0,pc);
			break;
		}
		fstrncpy(pbuftemp,pc,(pcstartuni-pc)+1);
		IIwritio(0,(short *)0,1,32,0,pbuftemp);
		IIputdomio((short *)0,1,26,0,bufuni);
		pc=pcafteruni;
		pcstartsearch=pc;
	}
	IIcsQuery(cname,0,0);

 
	ESL_FreeMem(pbuftemp);

	DisableSqlSaveText();

	return TRUE;
}

int ExecSQLImmWithUni(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm)
{
	long lresultcount;
	return ExecSQLImmWithUni1(lprequest, bcommit, lpsqldav, lpproc, usrparm, &lresultcount);
}

int ExecSQLImmWithUni1(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
		BOOL (*lpproc)(void *), void *usrparm, long *presultcount)
{
	wchar_t bufuni[MAXUNICODECONSTANTLEN+1];
	char * pcstartuni, *pcafteruni;
	long lret;
	long lreturned;
	char * pbuftemp;
	char *pc = lprequest;
	BOOL not_fnd=FALSE;

	if (lpsqldav || usrparm)
		return RES_ERR; /* no need - not implemented for initial fix */

	pbuftemp =ESL_AllocMem(_tcslen(lprequest)+1);
	if (!pbuftemp)
		return RES_ERR;
	
	EnableSqlSaveText();

	IIsqInit(&sqlca);

	while (*pc) {
		BOOL bres = GetNextUniConstant(pc,bufuni,&pcstartuni, &pcafteruni);
		if (!bres)  {
			IIwritio(0,(short *)0,1,32,0,pc);
			break;
		}
		fstrncpy(pbuftemp,pc,(pcstartuni-pc)+1);
		IIwritio(0,(short *)0,1,32,0,pbuftemp);
		IIputdomio((short *)0,1,26,0,bufuni);
		pc=pcafteruni;
	}
	IIsyncup((char *)0,0);

	ESL_FreeMem(pbuftemp);

	DisableSqlSaveText();

	lret=sqlca.sqlcode;

	lreturned = (long) sqlca.sqlerrd[2];


	ManageSqlError(&sqlca); // Keep track of the SQL error if any

	if (lret<0)
		return RES_ERR;

	if (lret ==100L) /* success with warning */
		not_fnd=TRUE;

	*presultcount = lreturned;

	if (bcommit) {
		exec sql commit;
		ManageSqlError(&sqlca); // Keep track of the SQL error if any
		if (sqlca.sqlcode!=0 && sqlca.sqlcode!=710L)
			return RES_ERR;
	}

	if (not_fnd)
		return RES_ENDOFDATA;
	else
		return RES_SUCCESS;
}

int ExecSQLImm(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm)
{
	long lresultcount;
	return ExecSQLImm1(lprequest,bcommit, lpsqldav, lpproc, usrparm, &lresultcount);
}
// lpsqlda defined as void far * in the include
int ExecSQLImm1(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm, long *presultcount)
/******************************************************************
* Function : dynamicaly executes an sql statement                 *
* Parameters :                                                    *
*     lprequest : SQL statement to be executed.                   *
*     bcommit   : a boolean telling whenever or not the proc has  *
*                 commit after the statement has been executed.   *                             
*                 bcommit=TRUE and lpproc not NULL is not allowed *
*     lpsqldatav: SQLDA describing the host variables to be       *
*                 returned. The SQLDA can be initialized by using *
*                 SQLDAInit. A null pointer tells no sqlda.       * 
*     proc      : address of the function to be given control for *
*                 each row returned from the database. (select    *                             
*                 loop).                                          *
*                 The proc called processes each row by filling   * 
*                 up a specific structure. The address of this    *
*                 structure is given by the caller (void *argv).  * 
*                 A null pointer means no proc to execute.        *
*     usrparm   : address of the above proc structure, or NULL.   *
*     NOTE :      if lpsqlda is null, proc and usrpar are ignored.*
* Returns : an integer (see dba.h)                                *
*                 RES_SUCCESS if succesful,                       *
*                 RES_sqlerr if a 'handled' error occured         *     
*                 RES_ERR if the error is not handled             *
******************************************************************/
{
   long lret;
   long lreturned;
   exec sql begin declare section;
   char SQLErrTxt[SQLERRORTXTLEN];
   exec sql end declare section;
   TCHAR szErrNo09D[] = {"E_QE009D"};
   TCHAR szErrNo7FD[] = {"E_US07FD"};

   LPSQLDA lpsqlda = (LPSQLDA)lpsqldav;   // see comment

   BOOL not_fnd=FALSE;
#ifdef STD_SQL_CALL
   exec sql begin declare section;
   unsigned char *lpimmreq=lprequest;  
   exec sql end declare section;

   bLastErr50 = FALSE;
   if (lpproc && bcommit)  /* cannot do both updates and select loop */ 
      return RES_ERR;
   StartSqlCriticalSection();

   EnableSqlSaveText();

   if (lpsqlda)
      if (lpproc) {
         exec sql execute immediate :lpimmreq using descriptor lpsqlda;
         exec sql begin;     
               (*lpproc)(usrparm); 
         exec sql end;       
      }
      else
         exec sql execute immediate :lpimmreq using descriptor lpsqlda;
   else {
         exec sql execute immediate :lpimmreq;
   }
#else  /* DOES NOT support the "USING SQLDA" form and the BEGIN-END */
   EnableSqlSaveText();
   StartSqlCriticalSection();
   IIsqInit(&sqlca);
   IIwritio((long)0,(short *)0,(long)1,(long)32,(long)0,lprequest);
   IIsyncup((char *)0,(long)0);
#endif
   lret=sqlca.sqlcode;
#ifdef DEBUGSQL
   {  
   UCHAR SqlTxt[SQLERRORTXTLEN];
   UCHAR title[400];
   exec sql begin declare section;
   unsigned char SQLErrorTxt[SQLERRORTXTLEN];
   exec sql end declare section;
   if (lret!=0) {
      exec sql inquire_sql (:SQLErrorTxt=errortext); 
      wsprintf(SqlTxt, " SQLstm = %s \n\n Error= %s", lpimmreq, SQLErrorTxt);
   }
   else 
      wsprintf(SqlTxt, " SQLstm = %s ", lpimmreq);
   wsprintf(title, " Execute SQL immediate RC= %ld", lret);
   TS_MessageBox(NULL, SqlTxt, title, MB_OK | MB_ICONASTERISK | MB_TASKMODAL);
  }
#endif

   lreturned = (long) sqlca.sqlerrd[2];
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   StopSqlCriticalSection();
   switch (lret) {
     case 4702L:
       DisableSqlSaveText();
       return RES_TIMEOUT; 
       break;
     case -39100L:
       DisableSqlSaveText();
       exec sql inquire_sql (:SQLErrTxt=errortext);
       if (x_strncmp(SQLErrTxt,szErrNo09D,lstrlen(szErrNo09D)) == 0 )
          return RES_CHANGE_PAGESIZE_NEEDED;
       else
          return RES_SQLNOEXEC;
       break;
     case -30100L:
       DisableSqlSaveText();
       return RES_30100_TABLENOTFOUND;
       break;
     case -30200L:
       DisableSqlSaveText();
       return RES_ALREADYEXIST;
       break;
     case -50L:  // success with warning
       bLastErr50 = TRUE;
       break;
     case -34000L:
       DisableSqlSaveText();
       return RES_NOGRANT;
       break;
     case 100L:  // success with warning
       not_fnd=TRUE;
       break;
     case 0L:  // success 
       break;
     case 700L:   // success with warning (populate)
     case 710L:
       break;
/*     case 710L:
       exec sql commit;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       return RES_MOREDBEVENT;  
       break;     */
     case -36000L:
       exec sql inquire_sql (:SQLErrTxt=errortext); 
       if (x_strncmp(SQLErrTxt,szErrNo7FD,lstrlen(szErrNo7FD)) == 0 )
          return RES_PAGE_SIZE_NOT_ENOUGH;
       else
          return RES_ERR;
       break;
     default:
       DisableSqlSaveText();
       return RES_ERR;
   }
   *presultcount = lreturned;
   if (bcommit) {
      exec sql commit;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      if (sqlca.sqlcode!=0 && sqlca.sqlcode!=710L) {
        exec sql rollback;
        DisableSqlSaveText();
        return RES_ERR;
      }
   }

   DisableSqlSaveText();
   if (not_fnd)
     return RES_ENDOFDATA;
   else
     return RES_SUCCESS;
}

/******************************************************************
* Function :   creates the SQLDA from an SQL statement. Next the  *
*              function return a pointer to a ROWSET              *
* Parameters :                                                    *
*     hsql    : pointer to a SQL stmt handle.                     *
*     lpsqlstm : SQL statement to be parsed, it may not contain   *
*     format  strings %d %s ect                                   *
* returns :                                                       *
*     either pointer to an SQLROWSET or NULL. If an pointer is    *
*     passed the handle for the processed statement is incremented*
*******************************************************************/
LPOBJECTLIST RegisterSQLStmt(LPOBJECTLIST lpdescr, HSQL *hsql, LPCTSTR psqlstm)
{                     
   exec sql begin declare section;
   char *pOutStm;
   char dyn_stm[]="dbadynstm";
   exec sql end declare section;

   LPOBJECTLIST lpFirstObj;
   LPSQLDA pSQLda;
   long lret;

   /***  verify the input parameters ***/ 
   if (!psqlstm) {
      myerror(ERR_INVPARAMS);
      return NULL;
   }

   pOutStm = ESL_AllocMem(_tcslen (psqlstm)*sizeof(TCHAR) + sizeof(TCHAR));
   if (!pOutStm) {
      myerror(ERR_ALLOCMEM);
      return NULL;
   }

   /*** Build the sqlda ***/
   pSQLda=ESL_AllocMem(sizeof(IISQLDA));

   if (!pSQLda) {
      ESL_FreeMem(pOutStm);
      return NULL;
      }
   pSQLda->sqln=IISQ_MAX_COLS;

   _tcscpy (pOutStm, (LPCTSTR)psqlstm);
   exec sql prepare :dyn_stm from :pOutStm;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   lret=sqlca.sqlcode;
#ifdef DEBUGSQL
   {  
   UCHAR SqlTxt[SQLERRORTXTLEN];
   UCHAR title[400];
   exec sql begin declare section;
   unsigned char SQLErrorTxt[SQLERRORTXTLEN];
   exec sql end declare section;
   if (lret) {
      exec sql inquire_sql (:SQLErrorTxt=errortext); 
      wsprintf(SqlTxt, " SQLstm = %s \n\n Error= %s", pOutStm, SQLErrorTxt);
   }
   else 
      wsprintf(SqlTxt, " SQLstm = %s ", pOutStm);
   wsprintf(title, " Prepare RC= %d", lret);
   TS_MessageBox(NULL, SqlTxt, title, MB_OK | MB_ICONASTERISK | MB_TASKMODAL);
  }
#endif
   if (lret==0) { // do not describe if prepare failed 
      exec sql describe :dyn_stm into :pSQLda;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      lret=sqlca.sqlcode;
   }
#ifdef DEBUGSQL
   {  
   UCHAR SqlTxt[SQLERRORTXTLEN];
   UCHAR title[400];
   exec sql begin declare section;
   unsigned char SQLErrorTxt[SQLERRORTXTLEN];
   exec sql end declare section;
   if (lret) {
      exec sql inquire_sql (:SQLErrorTxt=errortext); 
      wsprintf(SqlTxt, " SQLstm = %s \n\n Error= %s", pOutStm, SQLErrorTxt);
   }
   else 
      wsprintf(SqlTxt, " SQLstm = %s ", pOutStm);
   wsprintf(title, " Describe RC= %d", lret);
   TS_MessageBox(NULL, SqlTxt, title, MB_OK | MB_ICONASTERISK | MB_TASKMODAL);
  }
#endif

   if (pSQLda->sqld == 0) {
      ESL_FreeMem(pSQLda);
      ESL_FreeMem(pOutStm);
      return NULL;
   }

   /*** allocate the new SQL descriptor entry and return the pointer ***/
 
   lpFirstObj=AddSQLDescr(lpdescr, pSQLda, pOutStm);
   if (!lpFirstObj) {
      ESL_FreeMem(pOutStm);
      return NULL;
   }

   if (!lpdescr)     // first time, init the handle.
      *hsql=0;
   else
      (*hsql)++;


   ((LPSQLROWSET) (lpFirstObj->lpObject))->hsql=*hsql;

   return lpFirstObj;
}
int FetchSQLStm(LPOBJECTLIST lpFirstdescr, HSQL SQLHinst, ...)
/******************************************************************
* Function :   Returns the next row for the SQL statement         *
*              given as the first parameter into the va_list of   *
*              host variables.                                    *
*              The first call calls the FetchAllRows static proc  *
*              which fills up the structure we are getting the    *
*              rows from.                                         *
* Parameters :                                                    *
*     lpFirstdescr : pointer to the front of a linked list of SQL *
*              statement descriptor.                              *   
*     SQLHinst: SQL statement instance to be fetched.             *
*     va_arg   : serie of host types, lenght and addresses.       *
* returns :                                                       *
*     either RES_SUCCESS if the row was returned, RES_ENDOFDATA   *
*     if no more rows are to be given or RES_ERR if any troubles  *
******************************************************************/
{
   int iret; /* work return code */
   LPOBJECTLIST lpfirstdescr=lpFirstdescr;
   LPSQLROWSET lpRowSet;
   LPSQL_HOSTVAR lpHostStruct;
   va_list ap;          /* argument pointer */ 

   if (!lpfirstdescr)     // called but no previous successfull Register */
      return RES_ERR;

   // look for the sql having the handle given by caller

   while (lpfirstdescr) {  
      if (((LPSQLROWSET) (lpfirstdescr->lpObject))->hsql==SQLHinst)
         break;
      lpfirstdescr=lpfirstdescr->lpNext;
   }

   if (!lpfirstdescr)   // wrong handle, give up 
      return RES_ERR;

   lpRowSet=lpfirstdescr->lpObject;

   /* if no linked list of rows, this is the first call */
   /* for this statement : call FetchAllRows            */
   /* If not the first call just set the current pointer*/

   if (!lpRowSet->lpfirstrow)  {
      iret=FetchAllRows(lpRowSet); 
      if (iret!=RES_SUCCESS)
         return iret;   // we've got some troubles or no row found
      lpRowSet->lpcurrrow=lpRowSet->lpfirstrow;
   }
   else
      lpRowSet->lpcurrrow=lpRowSet->lpcurrrow->Next;

   if (!lpRowSet->lpcurrrow)
      return RES_ENDOFDATA;
            
   va_start(ap, SQLHinst);
   while (lpHostStruct=va_arg(ap, LPSQL_HOSTVAR)) {
      switch (lpHostStruct->itype) {
         case SQL_TYPE_LPSQLROWSET : // to be finished for non sqlact pgm
            break;
         default :
            va_end(ap);
            return RES_ERR;
      }
   }

   va_end(ap); // restore the arg pile

   return RES_SUCCESS;
}
static BOOL islvc(int pos)
{
  int i;
  for (i=0;i<nblvc;i++) {
    if (ilvc[i]==pos)
      return TRUE;
  }
  return FALSE;
}
                    

BOOL FillHosts(void *lpHostv)
/******************************************************************
* Function :   Explode the sql host buffer into as many host as   *
*              the SQLDAVAR entries                               *
*              Chain the row to the prev. ones if any             *
* Parameters :                                                    *
*     lpHostv : pointer to a FILLHOSTPRM structure                *
* returns :                                                       *
*     True if succesfull, false if any troubles.                  *
******************************************************************/
{
   LPFILLHOSTPRM lpIIHost = (LPFILLHOSTPRM) lpHostv;
   LPSQLROWSET lpMyRowSet=lpIIHost->lpsqlrowset;
   LPBYTE lpHostVar, lpSqlHostBfr=(char *) lpIIHost->lpsqlhostbfr;
   LPROW lpNewRow=NULL;
   int iHostBytes, i, iSqlType;

   /**** Allocate a new row and chain it to previous ones (if any) ****/

   lpNewRow=ESL_AllocMem(sizeof(ROW));
   if (!lpNewRow) 
      return FALSE;

   for (i=0; i<IISQ_MAX_COLS; i++)
      lpNewRow->lpcol[i]=NULL;   // initialize the host pointers

   ChainRows(lpMyRowSet, lpNewRow);

   lpHostVar=(LPBYTE) ESL_AllocMem(lpIIHost->ibufln);
   if (!lpHostVar)
      return FALSE;

   /******************************************************************
   * For each host variable described in the sqlda :                 *
   *   I) calculate how nany bytes are to be copied, and store host  *                 
   *     variable address.                                           *
   *   II) copy the host value from the SQLHOSTBFR (concatenation of *
   *        of host values)                                          *
   ******************************************************************/
   

   for (i=0; i<lpMyRowSet->lpsqlda->sqld; i++) {
      BOOL bAdd0=FALSE;
      lpNewRow->lpcol[i]=lpHostVar;
      iSqlType=lpMyRowSet->lpsqlda->sqlvar[i].sqltype;
      lpNewRow->sNullInd[i]=0;
      if (lpMyRowSet->lpsqlda->sqlvar[i].sqltype<=0) {    // NULL ind.
         lpNewRow->sNullInd[i]=*(short *)lpSqlHostBfr;
         iSqlType*=-1;
         lpSqlHostBfr+=sizeof(short);
         if (lpNewRow->sNullInd[i]!=0) {
            if(islvc(i)) {
               bAdd0=TRUE;
               lpNewRow->sNullInd[i]=0;
            }
            else 
               *lpSqlHostBfr='\0';  // init to empty string
         }
      }
      switch (iSqlType) {
         case IISQ_DTE_TYPE :                // date length : +1 ("\0")
            iHostBytes=IISQ_DTE_LEN+1;
            break;           
         case IISQ_BYTE_TYPE :               // byte and char length ....
         case IISQ_CHA_TYPE :                // +1 byte for('\0')
         case IISQ_NBR_TYPE : 
            iHostBytes=lpMyRowSet->lpsqlda->sqlvar[i].sqllen+1;
            break;
         case IISQ_VBYTE_TYPE :              // Varbyte and varchar length 
         case IISQ_VCH_TYPE :                // doesn't include short(size)
            iHostBytes=lpMyRowSet->lpsqlda->sqlvar[i].sqllen+1+sizeof(short);
            break;
         case IISQ_FLT_TYPE :                // Float and integer ...
         case IISQ_INT_TYPE :                // sqllen gives the length.
            iHostBytes=lpMyRowSet->lpsqlda->sqlvar[i].sqllen;
            break;
         case IISQ_LBYTE_TYPE :              // Long byte.
         case IISQ_GEOM_TYPE :
         case IISQ_POINT_TYPE :
         case IISQ_MPOINT_TYPE :
         case IISQ_LINE_TYPE :
         case IISQ_MLINE_TYPE :
         case IISQ_POLY_TYPE :
         case IISQ_MPOLY_TYPE :
         case IISQ_GEOMC_TYPE :
         default :                           // Integer.
            return FALSE;
      }
      memcpy(lpNewRow->lpcol[i], lpSqlHostBfr, iHostBytes);
      if (bAdd0) 
         *( (char *)(lpNewRow->lpcol[i]) + (iHostBytes-1) )='\0';
      if (lpMyRowSet->lpsqlda->sqlvar[i].sqllen==LONGVARCHARDISPCHAR &&
          iSqlType==IISQ_CHA_TYPE)
          x_strcpy( (char *)lpSqlHostBfr,LVCNULLSTRING);

#ifdef MAINWIN
#if defined(su4_us5) || defined(hp8_us5) || defined(hpb_us5) || \
    defined(rs4_us5) || defined(int_lnx)
      {
        // need to align on short boundary for sparc stations
        int   modulo;
        modulo = iHostBytes % sizeof(short);
        if (modulo)
          iHostBytes += sizeof(short) - modulo;
      }
#else
#error platform-specific code missing
#endif  /* su4_us5 hp8_us5 hpb_us5 rs4_us5 */
#endif  /* MAINWIN */

      lpSqlHostBfr+=iHostBytes;
      lpHostVar+=iHostBytes;
   }
   return TRUE;
}

static int SQLCodeToDom(long sqlcode) // to be removed when SQLDynn OK
{

   switch (sqlcode) {
     case 700L:   // success with warning (populate)
     case 0L :
        return RES_SUCCESS;
        break;
     case 100L :
        return RES_ENDOFDATA;
        break;
     case 4702L:
       return RES_TIMEOUT; 
       break;
     case -30100L:
       return RES_30100_TABLENOTFOUND;
       break;
     case -30200L:
       return RES_ALREADYEXIST;
       break;
     case -50L:  // success with warning
       return RES_SUCCESS; // should be SUCCESSWANING   
       break;
     case -34000L:
       return RES_NOGRANT;
       break;
     default:
       return RES_ERR;
   }
}
int GetInfLoc(LPLOCATIONPARAMS locprm)
{
   int i=0;
   exec sql begin declare section;
   char DtUsage[2], JnUsage[2], CkUsage[2], WrUsage[2], DmUsage[2],
        LocArea[MAXAREANAME];
   char *locname=locprm->objectname;
   int RawPct;
   exec sql end declare section;

   if (GetOIVers() >= OIVERS_26)
   {
      EXEC SQL REPEATED SELECT   data_usage, jrnl_usage, ckpt_usage, 
                        work_usage, dump_usage, location_area, raw_pct
               INTO     :DtUsage, :JnUsage, :CkUsage, :WrUsage, 
                        :DmUsage, :LocArea, :RawPct
               FROM     iilocation_info
               WHERE    location_name = :locname;
           EXEC SQL BEGIN;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
              if (DtUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONDATABASE]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONDATABASE]=FALSE;

              if (JnUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONJOURNAL]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONJOURNAL]=FALSE;

              if (CkUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONCHECKPOINT]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONCHECKPOINT]=FALSE;

              if (WrUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONWORK]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONWORK]=FALSE;

              if (DmUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONDUMP]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONDUMP]=FALSE;

              switch (i) {
                 case 0 :   // no privileges set to TRUE.
                    locprm->bLocationUsage[LOCATIONNOUSAGE]=TRUE;
                    locprm->bLocationUsage[LOCATIONALL]=FALSE;
                    break;
                 case LOCATIONUSAGES : // all preivileges set to TRUE
                    locprm->bLocationUsage[LOCATIONNOUSAGE]=FALSE;
                    locprm->bLocationUsage[LOCATIONALL]=TRUE;
                    break;
                 default : // privileges mixed (TRUE and FALSE)
                    locprm->bLocationUsage[LOCATIONNOUSAGE]=FALSE;
                    locprm->bLocationUsage[LOCATIONALL]=FALSE;
              }
              fstrncpy(locprm->AreaName,LocArea,MAXAREANAME);
              locprm->iRawPct=RawPct;
           EXEC SQL END;
   }
   else
   {
      EXEC SQL REPEATED SELECT   data_usage, jrnl_usage, ckpt_usage, 
                        work_usage, dump_usage, location_area
               INTO     :DtUsage, :JnUsage, :CkUsage, :WrUsage,
                        :DmUsage, :LocArea
               FROM     iilocation_info
               WHERE    location_name = :locname;
           EXEC SQL BEGIN;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
              if (DtUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONDATABASE]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONDATABASE]=FALSE;

              if (JnUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONJOURNAL]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONJOURNAL]=FALSE;

              if (CkUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONCHECKPOINT]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONCHECKPOINT]=FALSE;

              if (WrUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONWORK]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONWORK]=FALSE;

              if (DmUsage[0]=='Y') {
                 i++;
                 locprm->bLocationUsage[LOCATIONDUMP]=TRUE;
              }
              else
                 locprm->bLocationUsage[LOCATIONDUMP]=FALSE;

              switch (i) {
                 case 0 :   // no privileges set to TRUE.
                    locprm->bLocationUsage[LOCATIONNOUSAGE]=TRUE;
                    locprm->bLocationUsage[LOCATIONALL]=FALSE;
                    break;
                 case LOCATIONUSAGES : // all preivileges set to TRUE
                    locprm->bLocationUsage[LOCATIONNOUSAGE]=FALSE;
                    locprm->bLocationUsage[LOCATIONALL]=TRUE;
                    break;
                 default : // privileges mixed (TRUE and FALSE)
                    locprm->bLocationUsage[LOCATIONNOUSAGE]=FALSE;
                    locprm->bLocationUsage[LOCATIONALL]=FALSE;
              }
              fstrncpy(locprm->AreaName,LocArea,MAXAREANAME);
              locprm->iRawPct= 0;
           EXEC SQL END;
   }
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   return SQLCodeToDom(sqlca.sqlcode);
}

int GetInfGrp(LPGROUPPARAMS grpprm)
{
   exec sql begin declare section;
   char Member[MAXOBJECTNAME];
   char *grpname=grpprm->ObjectName;
   exec sql end declare section;
   LPCHECKEDOBJECTS NewChkObj;

   exec sql repeated select groupmem
      into :Member
      from iiusergroup
      where groupid=:grpname
       and  groupmem != ' ';
   exec sql begin;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      suppspace(Member);
      if (*Member) {  // not rejected in where to return RES_SUCCESS instead 
                      // of RES_ENDOFDATA when the group has no users.
                      // RES_ENDOFDATA is returned when the grp doesn't exist
         NewChkObj=(LPCHECKEDOBJECTS) ESL_AllocMem(sizeof(CHECKEDOBJECTS));
         if (!NewChkObj) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
         x_strcpy(NewChkObj->dbname, Member);
         NewChkObj->bchecked=TRUE;
         NewChkObj->pnext=NULL;

         grpprm->lpfirstuser=AddCheckedObject(grpprm->lpfirstuser, NewChkObj);
      }
   exec sql end;
   ManageSqlError(&sqlca);  // Keep track of the SQL error if any
   if (sqlca.sqlcode==100L) // for this request, no user in the group is not 
     sqlca.sqlcode=0L;      // considered as an error -> return RES_SUCCESS

   return SQLCodeToDom(sqlca.sqlcode);
}            

static int GetInfUsrOpenSource(LPUSERPARAMS usrprm)
{
   BOOL bAllocOk=TRUE;
   exec sql begin declare section;
   char Default_group[MAXOBJECTNAME];
   char Profile_name[MAXOBJECTNAME];
   char Expire_date[MAXDATELEN];
   char *usrname=usrprm->ObjectName;
   char Creatdb[2], Trace[2], Audit_all[2], Security[2],
        Maintain_locations[2], Operator[2], Maintain_users[2],
        Maintain_audit[2], Auditor[2], Audit_query_text[2];

   char szWriteDown[2];
   char szWriteFixed[2];
   char szWriteUp[2];
   char szInsertDown[2];
   char szInsertUp[2];
   char szSessionSecurityLable[2];
   long InternFlag;

   exec sql end declare section;

  exec sql repeated select   default_group, createdb, trace,  security,
                     maintain_locations, operator, maintain_users,
                     maintain_audit, auditor, audit_all,
                     audit_query_text, expire_date, profile_name,
                     internal_flags,
                     char(charextract('NRY', mod((internal_status/ 131072),2)+  mod((internal_def_priv/ 131072),2)+1)),
                     char(charextract('NRY', mod((internal_status/ 8388608),2)+  mod((internal_def_priv/8388608),2)+1)),
                     char(charextract('NRY', mod((internal_status/  524288),2)+  mod((internal_def_priv/ 524288),2)+1)),
                     char(charextract('NRY', mod((internal_status/  262144),2)+  mod((internal_def_priv/ 262144),2)+1)),
                     char(charextract('NRY', mod((internal_status/  1048576),2)+  mod((internal_def_priv/ 1048576),2)+1)),
                     char(charextract('NRY', mod((internal_status/  4194304),2)+  mod((internal_def_priv/ 4194304),2)+1))
            into     :Default_group, :Creatdb, :Trace, :Security,
                     :Maintain_locations, :Operator, :Maintain_users,
                     :Maintain_audit, :Auditor, :Audit_all,
                     :Audit_query_text, :Expire_date, :Profile_name,
                     :InternFlag,
                     :szWriteDown,
                     :szWriteFixed,
                     :szWriteUp,
                     :szInsertDown,
                     :szInsertUp,
                     :szSessionSecurityLable
            from     iiusers
            where    user_name=:usrname;

   exec sql begin;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any

      if ( InternFlag & 131072L)
         usrprm->bExtrnlPassword = TRUE;
      else
         usrprm->bExtrnlPassword = FALSE;

      if (Creatdb[0]=='Y') {
         usrprm->Privilege[USERCREATEDBPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERCREATEDBPRIV]=TRUE;
      }
      else {
         if (Creatdb[0]=='R') {
            usrprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            usrprm->Privilege[USERCREATEDBPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            usrprm->Privilege[USERCREATEDBPRIV]=FALSE;
         }
      }

      if (Trace[0]=='Y') {
         usrprm->Privilege[USERTRACEPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERTRACEPRIV]=TRUE;
      }
      else {
         if (Trace[0]=='R') {
            usrprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            usrprm->Privilege[USERTRACEPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            usrprm->Privilege[USERTRACEPRIV]=FALSE;
         }
      }

      if (Security[0]=='Y') {
         usrprm->Privilege[USERSECURITYPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERSECURITYPRIV]=TRUE;
      }
      else {
         if (Security[0]=='R') {
            usrprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            usrprm->Privilege[USERSECURITYPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            usrprm->Privilege[USERSECURITYPRIV]=FALSE;
         }
      }

      if (Maintain_locations[0]=='Y') {
         usrprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERMAINTLOCPRIV]=TRUE;
      }
      else {
         if (Maintain_locations[0]=='R') {
            usrprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            usrprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            usrprm->Privilege[USERMAINTLOCPRIV]=FALSE;
         }
      }

      if (Operator[0]=='Y') {
         usrprm->Privilege[USEROPERATORPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USEROPERATORPRIV]=TRUE;
      }
      else {
         if (Operator[0]=='R') {
            usrprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            usrprm->Privilege[USEROPERATORPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            usrprm->Privilege[USEROPERATORPRIV]=FALSE;
         }
      }


      if (Maintain_users[0]=='Y') {
         usrprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERMAINTUSERPRIV]=TRUE;
      }
      else {
         if (Maintain_users[0]=='R') {
            usrprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            usrprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            usrprm->Privilege[USERMAINTUSERPRIV]=FALSE;
         }
      }


      if (Maintain_audit[0]=='Y') {
         usrprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=TRUE;
      }
      else {
         if (Maintain_audit[0]=='R') {
            usrprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            usrprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            usrprm->Privilege[USERMAINTAUDITPRIV]=FALSE;
         }
      }


      if (Auditor[0]=='Y') {
         usrprm->Privilege[USERAUDITALLPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERAUDITALLPRIV]=TRUE;
      }
      else {
         if (Auditor[0]=='R') {
            usrprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            usrprm->Privilege[USERAUDITALLPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            usrprm->Privilege[USERAUDITALLPRIV]=FALSE;
         }
      }
		//
		// MAC: write_down:
		if (szWriteDown[0]  == 'Y')
		{
			usrprm->Privilege[USERWRITEDOWN]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEDOWN]=TRUE;
		}
		else
		if (szWriteDown[0]  == 'R')
		{
			usrprm->Privilege[USERWRITEDOWN]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERWRITEDOWN]=FALSE;
			usrprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		//
		// MAC: write_fixed:
		if (szWriteFixed[0]  == 'Y')
		{
			usrprm->Privilege[USERWRITEFIXED]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEFIXED]=TRUE;
		}
		else
		if (szWriteFixed[0]  == 'R')
		{
			usrprm->Privilege[USERWRITEFIXED]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERWRITEFIXED]=FALSE;
			usrprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		//
		// MAC: write_up:
		if (szWriteUp[0]  == 'Y')
		{
			usrprm->Privilege[USERWRITEUP]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEUP]=TRUE;
		}
		else
		if (szWriteUp[0]  == 'R')
		{
			usrprm->Privilege[USERWRITEUP]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERWRITEUP]=FALSE;
			usrprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		//
		// MAC: insert_down:
		if (szInsertDown[0]  == 'Y')
		{
			usrprm->Privilege[USERINSERTDOWN]=TRUE;
			usrprm->bDefaultPrivilege[USERINSERTDOWN]=TRUE;
		}
		else
		if (szInsertDown[0]  == 'R')
		{
			usrprm->Privilege[USERINSERTDOWN]=TRUE;
			usrprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERINSERTDOWN]=FALSE;
			usrprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		//
		// MAC: insert_up:
		if (szInsertUp[0]  == 'Y')
		{
			usrprm->Privilege[USERINSERTUP]=TRUE;
			usrprm->bDefaultPrivilege[USERINSERTUP]=TRUE;
		}
		else
		if (szInsertUp[0]  == 'R')
		{
			usrprm->Privilege[USERINSERTUP]=TRUE;
			usrprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERINSERTUP]=FALSE;
			usrprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		//
		// MAC: session_security_label:
		if (szSessionSecurityLable[0]  == 'Y')
		{
			usrprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			usrprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=TRUE;
		}
		else
		if (szSessionSecurityLable[0]  == 'R')
		{
			usrprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			usrprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERSESSIONSECURITYLABEL]=FALSE;
			usrprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}


      if (Audit_all[0]=='Y')
         usrprm->bSecAudit[USERALLEVENT]=TRUE;
      else
         usrprm->bSecAudit[USERALLEVENT]=FALSE;

      if (Audit_query_text[0]=='Y')
         usrprm->bSecAudit[USERQUERYTEXTEVENT]=TRUE;
      else
         usrprm->bSecAudit[USERQUERYTEXTEVENT]=FALSE;

      suppspace(Default_group);
      x_strcpy(usrprm->DefaultGroup,Default_group);

      suppspace(Expire_date);
      x_strcpy(usrprm->ExpireDate,Expire_date);

      suppspace(Profile_name);
      x_strcpy(usrprm->szProfileName,Profile_name);

   exec sql end;    
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (!bAllocOk)
      return RES_ERR;   // allocation failed

   return SQLCodeToDom(sqlca.sqlcode);
}            

int GetInfUsr(LPUSERPARAMS usrprm)
{
   BOOL bAllocOk=TRUE;

   exec sql begin declare section;
   short sNullInd;
   char Default_group[MAXOBJECTNAME];
   char Profile_name[MAXOBJECTNAME];
   char Expire_date[MAXDATELEN];
   char *usrname=usrprm->ObjectName;
   char Lim_sec_label[MAXLIMITSECLBL];
   char Creatdb[2], Trace[2], Audit_all[2], Security[2],
        Maintain_locations[2], Operator[2], Maintain_users[2],
        Maintain_audit[2], Auditor[2], Audit_query_text[2];

   char szWriteDown[2];
   char szWriteFixed[2];
   char szWriteUp[2];
   char szInsertDown[2];
   char szInsertUp[2];
   char szSessionSecurityLable[2];
   long InternFlag;

   exec sql end declare section;

   if (GetOIVers() >= OIVERS_30)
       return GetInfUsrOpenSource(usrprm);

   exec sql repeated select   default_group, createdb, trace,  security,
                     maintain_locations, operator, maintain_users,
                     maintain_audit, auditor, audit_all,
                     audit_query_text, expire_date, profile_name,
                     lim_sec_label, internal_flags,
                     char(charextract('NRY', mod((internal_status/ 131072),2)+  mod((internal_def_priv/ 131072),2)+1)),
                     char(charextract('NRY', mod((internal_status/ 8388608),2)+  mod((internal_def_priv/8388608),2)+1)),
                     char(charextract('NRY', mod((internal_status/  524288),2)+  mod((internal_def_priv/ 524288),2)+1)),
                     char(charextract('NRY', mod((internal_status/  262144),2)+  mod((internal_def_priv/ 262144),2)+1)),
                     char(charextract('NRY', mod((internal_status/  1048576),2)+  mod((internal_def_priv/ 1048576),2)+1)),
                     char(charextract('NRY', mod((internal_status/  4194304),2)+  mod((internal_def_priv/ 4194304),2)+1))
            into     :Default_group, :Creatdb, :Trace, :Security,
                     :Maintain_locations, :Operator, :Maintain_users,
                     :Maintain_audit, :Auditor, :Audit_all,
                     :Audit_query_text, :Expire_date, :Profile_name,
                     :Lim_sec_label:sNullInd, :InternFlag,
                     :szWriteDown,
                     :szWriteFixed,
                     :szWriteUp,
                     :szInsertDown,
                     :szInsertUp,
                     :szSessionSecurityLable
            from     iiusers
            where    user_name=:usrname;

   exec sql begin;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any

      if ( InternFlag & 131072L)
         usrprm->bExtrnlPassword = TRUE;
      else
         usrprm->bExtrnlPassword = FALSE;

      if (Creatdb[0]=='Y') {
         usrprm->Privilege[USERCREATEDBPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERCREATEDBPRIV]=TRUE;
      }
      else {
         if (Creatdb[0]=='R') {
            usrprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            usrprm->Privilege[USERCREATEDBPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            usrprm->Privilege[USERCREATEDBPRIV]=FALSE;
         }
      }

      if (Trace[0]=='Y') {
         usrprm->Privilege[USERTRACEPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERTRACEPRIV]=TRUE;
      }
      else {
         if (Trace[0]=='R') {
            usrprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            usrprm->Privilege[USERTRACEPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            usrprm->Privilege[USERTRACEPRIV]=FALSE;
         }
      }

      if (Security[0]=='Y') {
         usrprm->Privilege[USERSECURITYPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERSECURITYPRIV]=TRUE;
      }
      else {
         if (Security[0]=='R') {
            usrprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            usrprm->Privilege[USERSECURITYPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            usrprm->Privilege[USERSECURITYPRIV]=FALSE;
         }
      }

      if (Maintain_locations[0]=='Y') {
         usrprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERMAINTLOCPRIV]=TRUE;
      }
      else {
         if (Maintain_locations[0]=='R') {
            usrprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            usrprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            usrprm->Privilege[USERMAINTLOCPRIV]=FALSE;
         }
      }

      if (Operator[0]=='Y') {
         usrprm->Privilege[USEROPERATORPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USEROPERATORPRIV]=TRUE;
      }
      else {
         if (Operator[0]=='R') {
            usrprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            usrprm->Privilege[USEROPERATORPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            usrprm->Privilege[USEROPERATORPRIV]=FALSE;
         }
      }


      if (Maintain_users[0]=='Y') {
         usrprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERMAINTUSERPRIV]=TRUE;
      }
      else {
         if (Maintain_users[0]=='R') {
            usrprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            usrprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            usrprm->Privilege[USERMAINTUSERPRIV]=FALSE;
         }
      }


      if (Maintain_audit[0]=='Y') {
         usrprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=TRUE;
      }
      else {
         if (Maintain_audit[0]=='R') {
            usrprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            usrprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            usrprm->Privilege[USERMAINTAUDITPRIV]=FALSE;
         }
      }


      if (Auditor[0]=='Y') {
         usrprm->Privilege[USERAUDITALLPRIV]=TRUE;
         usrprm->bDefaultPrivilege[USERAUDITALLPRIV]=TRUE;
      }
      else {
         if (Auditor[0]=='R') {
            usrprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            usrprm->Privilege[USERAUDITALLPRIV]=TRUE;
         }
         else {
            usrprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            usrprm->Privilege[USERAUDITALLPRIV]=FALSE;
         }
      }
		//
		// MAC: write_down:
		if (szWriteDown[0]  == 'Y')
		{
			usrprm->Privilege[USERWRITEDOWN]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEDOWN]=TRUE;
		}
		else
		if (szWriteDown[0]  == 'R')
		{
			usrprm->Privilege[USERWRITEDOWN]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERWRITEDOWN]=FALSE;
			usrprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		//
		// MAC: write_fixed:
		if (szWriteFixed[0]  == 'Y')
		{
			usrprm->Privilege[USERWRITEFIXED]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEFIXED]=TRUE;
		}
		else
		if (szWriteFixed[0]  == 'R')
		{
			usrprm->Privilege[USERWRITEFIXED]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERWRITEFIXED]=FALSE;
			usrprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		//
		// MAC: write_up:
		if (szWriteUp[0]  == 'Y')
		{
			usrprm->Privilege[USERWRITEUP]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEUP]=TRUE;
		}
		else
		if (szWriteUp[0]  == 'R')
		{
			usrprm->Privilege[USERWRITEUP]=TRUE;
			usrprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERWRITEUP]=FALSE;
			usrprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		//
		// MAC: insert_down:
		if (szInsertDown[0]  == 'Y')
		{
			usrprm->Privilege[USERINSERTDOWN]=TRUE;
			usrprm->bDefaultPrivilege[USERINSERTDOWN]=TRUE;
		}
		else
		if (szInsertDown[0]  == 'R')
		{
			usrprm->Privilege[USERINSERTDOWN]=TRUE;
			usrprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERINSERTDOWN]=FALSE;
			usrprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		//
		// MAC: insert_up:
		if (szInsertUp[0]  == 'Y')
		{
			usrprm->Privilege[USERINSERTUP]=TRUE;
			usrprm->bDefaultPrivilege[USERINSERTUP]=TRUE;
		}
		else
		if (szInsertUp[0]  == 'R')
		{
			usrprm->Privilege[USERINSERTUP]=TRUE;
			usrprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERINSERTUP]=FALSE;
			usrprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		//
		// MAC: session_security_label:
		if (szSessionSecurityLable[0]  == 'Y')
		{
			usrprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			usrprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=TRUE;
		}
		else
		if (szSessionSecurityLable[0]  == 'R')
		{
			usrprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			usrprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}
		else
		{
			usrprm->Privilege[USERSESSIONSECURITYLABEL]=FALSE;
			usrprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}


      if (Audit_all[0]=='Y')
         usrprm->bSecAudit[USERALLEVENT]=TRUE;
      else
         usrprm->bSecAudit[USERALLEVENT]=FALSE;

      if (Audit_query_text[0]=='Y')
         usrprm->bSecAudit[USERQUERYTEXTEVENT]=TRUE;
      else
         usrprm->bSecAudit[USERQUERYTEXTEVENT]=FALSE;

      suppspace(Default_group);
      x_strcpy(usrprm->DefaultGroup,Default_group);

      suppspace(Expire_date);
      x_strcpy(usrprm->ExpireDate,Expire_date);

      suppspace(Profile_name);
      x_strcpy(usrprm->szProfileName,Profile_name);

      if (sNullInd==0) {
         suppspace(Lim_sec_label);
         usrprm->lpLimitSecLabels=ESL_AllocMem(x_strlen(Lim_sec_label)+1);
         if (!usrprm->lpLimitSecLabels) {
            bAllocOk=FALSE;
            exec sql endselect;
         }
         x_strcpy(usrprm->lpLimitSecLabels,Lim_sec_label);
      }

   exec sql end;    
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (!bAllocOk)
      return RES_ERR;   // allocation failed

   return SQLCodeToDom(sqlca.sqlcode);
}

int GetInfRoleOpenSource(LPROLEPARAMS roleprm)
{
   BOOL bAllocOk=TRUE;

   exec sql begin declare section;
   long lInternpasswd;
   char *rolname=roleprm->ObjectName;
   char Creatdb[2], Trace[2], Audit_all[2], Security[2],
        Maintain_locations[2], Operator[2], Maintain_users[2],
        Maintain_audit[2], Auditor[2], Audit_query_text[2];

   char szWriteDown[2];
   char szWriteFixed[2];
   char szWriteUp[2];
   char szInsertDown[2];
   char szInsertUp[2];
   char szSessionSecurityLable[2];

   exec sql end declare section;

   exec sql repeated select   createdb, trace,  security, maintain_locations,
                     operator, maintain_users,maintain_audit, auditor,
                     audit_all, audit_query_text,
                     internal_flags,
                     char(charextract('NRY', mod((internal_status/ 131072),2)+  mod((internal_flags/ 131072),2)+1)),
                     char(charextract('NRY', mod((internal_status/ 8388608),2)+  mod((internal_flags/8388608),2)+1)),
                     char(charextract('NRY', mod((internal_status/  524288),2)+  mod((internal_flags/ 524288),2)+1)),
                     char(charextract('NRY', mod((internal_status/  262144),2)+  mod((internal_flags/ 262144),2)+1)),
                     char(charextract('NRY', mod((internal_status/  1048576),2)+  mod((internal_flags/ 1048576),2)+1)),
                     char(charextract('NRY', mod((internal_status/  4194304),2)+  mod((internal_flags/ 4194304),2)+1))
            into     :Creatdb, :Trace, :Security, :Maintain_locations,
                     :Operator, :Maintain_users, :Maintain_audit, :Auditor,
                     :Audit_all, :Audit_query_text, 
                     :lInternpasswd,
                     :szWriteDown,
                     :szWriteFixed,
                     :szWriteUp,
                     :szInsertDown,
                     :szInsertUp,
                     :szSessionSecurityLable
            from     iiroles
            where    role_name=:rolname;

   exec sql begin;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any

      if (lInternpasswd & 131072L)
         roleprm->bExtrnlPassword = TRUE;
      else
         roleprm->bExtrnlPassword = FALSE;

      if (Creatdb[0]=='Y') {
         roleprm->Privilege[USERCREATEDBPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERCREATEDBPRIV]=TRUE;
      }
      else {
         if (Creatdb[0]=='R') {
            roleprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            roleprm->Privilege[USERCREATEDBPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            roleprm->Privilege[USERCREATEDBPRIV]=FALSE;
         }
      }

      if (Trace[0]=='Y') {
         roleprm->Privilege[USERTRACEPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERTRACEPRIV]=TRUE;
      }
      else {
         if (Trace[0]=='R') {
            roleprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            roleprm->Privilege[USERTRACEPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            roleprm->Privilege[USERTRACEPRIV]=FALSE;
         }
      }

      if (Security[0]=='Y') {
         roleprm->Privilege[USERSECURITYPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERSECURITYPRIV]=TRUE;
      }
      else {
         if (Security[0]=='R') {
            roleprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            roleprm->Privilege[USERSECURITYPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            roleprm->Privilege[USERSECURITYPRIV]=FALSE;
         }
      }

      if (Maintain_locations[0]=='Y') {
         roleprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERMAINTLOCPRIV]=TRUE;
      }
      else {
         if (Maintain_locations[0]=='R') {
            roleprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            roleprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            roleprm->Privilege[USERMAINTLOCPRIV]=FALSE;
         }
      }

      if (Operator[0]=='Y') {
         roleprm->Privilege[USEROPERATORPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USEROPERATORPRIV]=TRUE;
      }
      else {
         if (Operator[0]=='R') {
            roleprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            roleprm->Privilege[USEROPERATORPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            roleprm->Privilege[USEROPERATORPRIV]=FALSE;
         }
      }


      if (Maintain_users[0]=='Y') {
         roleprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERMAINTUSERPRIV]=TRUE;
      }
      else {
         if (Maintain_users[0]=='R') {
            roleprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            roleprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            roleprm->Privilege[USERMAINTUSERPRIV]=FALSE;
         }
      }


      if (Maintain_audit[0]=='Y') {
         roleprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=TRUE;
      }
      else {
         if (Maintain_audit[0]=='R') {
            roleprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            roleprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            roleprm->Privilege[USERMAINTAUDITPRIV]=FALSE;
         }
      }


      if (Auditor[0]=='Y') {
         roleprm->Privilege[USERAUDITALLPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERAUDITALLPRIV]=TRUE;
      }
      else {
         if (Auditor[0]=='R') {
            roleprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            roleprm->Privilege[USERAUDITALLPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            roleprm->Privilege[USERAUDITALLPRIV]=FALSE;
         }
      }
		//
		// MAC: write_down:
		if (szWriteDown[0]  == 'Y')
		{
			roleprm->Privilege[USERWRITEDOWN]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEDOWN]=TRUE;
		}
		else
		if (szWriteDown[0]  == 'R')
		{
			roleprm->Privilege[USERWRITEDOWN]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERWRITEDOWN]=FALSE;
			roleprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		//
		// MAC: write_fixed:
		if (szWriteFixed[0]  == 'Y')
		{
			roleprm->Privilege[USERWRITEFIXED]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEFIXED]=TRUE;
		}
		else
		if (szWriteFixed[0]  == 'R')
		{
			roleprm->Privilege[USERWRITEFIXED]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERWRITEFIXED]=FALSE;
			roleprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		//
		// MAC: write_up:
		if (szWriteUp[0]  == 'Y')
		{
			roleprm->Privilege[USERWRITEUP]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEUP]=TRUE;
		}
		else
		if (szWriteUp[0]  == 'R')
		{
			roleprm->Privilege[USERWRITEUP]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERWRITEUP]=FALSE;
			roleprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		//
		// MAC: insert_down:
		if (szInsertDown[0]  == 'Y')
		{
			roleprm->Privilege[USERINSERTDOWN]=TRUE;
			roleprm->bDefaultPrivilege[USERINSERTDOWN]=TRUE;
		}
		else
		if (szInsertDown[0]  == 'R')
		{
			roleprm->Privilege[USERINSERTDOWN]=TRUE;
			roleprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERINSERTDOWN]=FALSE;
			roleprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		//
		// MAC: insert_up:
		if (szInsertUp[0]  == 'Y')
		{
			roleprm->Privilege[USERINSERTUP]=TRUE;
			roleprm->bDefaultPrivilege[USERINSERTUP]=TRUE;
		}
		else
		if (szInsertUp[0]  == 'R')
		{
			roleprm->Privilege[USERINSERTUP]=TRUE;
			roleprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERINSERTUP]=FALSE;
			roleprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		//
		// MAC: session_security_label:
		if (szSessionSecurityLable[0]  == 'Y')
		{
			roleprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			roleprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=TRUE;
		}
		else
		if (szSessionSecurityLable[0]  == 'R')
		{
			roleprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			roleprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERSESSIONSECURITYLABEL]=FALSE;
			roleprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}


      if (Audit_all[0]=='Y')
         roleprm->bSecAudit[USERALLEVENT]=TRUE;
      else
         roleprm->bSecAudit[USERALLEVENT]=FALSE;

      if (Audit_query_text[0]=='Y')
         roleprm->bSecAudit[USERQUERYTEXTEVENT]=TRUE;
      else
         roleprm->bSecAudit[USERQUERYTEXTEVENT]=FALSE;

   exec sql end;    
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (!bAllocOk)
      return RES_ERR;   // allocation failed

   return SQLCodeToDom(sqlca.sqlcode);
}

int GetInfRole(LPROLEPARAMS roleprm)
{
   BOOL bAllocOk=TRUE;

   exec sql begin declare section;
   long lInternpasswd;
   short sNullInd;
   char *rolname=roleprm->ObjectName;
   char Lim_sec_label[MAXLIMITSECLBL];
   char Creatdb[2], Trace[2], Audit_all[2], Security[2],
        Maintain_locations[2], Operator[2], Maintain_users[2],
        Maintain_audit[2], Auditor[2], Audit_query_text[2];

   char szWriteDown[2];
   char szWriteFixed[2];
   char szWriteUp[2];
   char szInsertDown[2];
   char szInsertUp[2];
   char szSessionSecurityLable[2];

   exec sql end declare section;

   if (GetOIVers() >= OIVERS_30)
      return GetInfRoleOpenSource(roleprm);
      
   exec sql repeated select   createdb, trace,  security, maintain_locations,
                     operator, maintain_users,maintain_audit, auditor,
                     audit_all, audit_query_text, lim_sec_label,
                     internal_flags,
                     char(charextract('NRY', mod((internal_status/ 131072),2)+  mod((internal_flags/ 131072),2)+1)),
                     char(charextract('NRY', mod((internal_status/ 8388608),2)+  mod((internal_flags/8388608),2)+1)),
                     char(charextract('NRY', mod((internal_status/  524288),2)+  mod((internal_flags/ 524288),2)+1)),
                     char(charextract('NRY', mod((internal_status/  262144),2)+  mod((internal_flags/ 262144),2)+1)),
                     char(charextract('NRY', mod((internal_status/  1048576),2)+  mod((internal_flags/ 1048576),2)+1)),
                     char(charextract('NRY', mod((internal_status/  4194304),2)+  mod((internal_flags/ 4194304),2)+1))
            into     :Creatdb, :Trace, :Security, :Maintain_locations,
                     :Operator, :Maintain_users, :Maintain_audit, :Auditor,
                     :Audit_all, :Audit_query_text, :Lim_sec_label:sNullInd,
                     :lInternpasswd,
                     :szWriteDown,
                     :szWriteFixed,
                     :szWriteUp,
                     :szInsertDown,
                     :szInsertUp,
                     :szSessionSecurityLable
            from     iiroles
            where    role_name=:rolname;

   exec sql begin;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any

      if (lInternpasswd & 131072L)
         roleprm->bExtrnlPassword = TRUE;
      else
         roleprm->bExtrnlPassword = FALSE;

      if (Creatdb[0]=='Y') {
         roleprm->Privilege[USERCREATEDBPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERCREATEDBPRIV]=TRUE;
      }
      else {
         if (Creatdb[0]=='R') {
            roleprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            roleprm->Privilege[USERCREATEDBPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            roleprm->Privilege[USERCREATEDBPRIV]=FALSE;
         }
      }

      if (Trace[0]=='Y') {
         roleprm->Privilege[USERTRACEPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERTRACEPRIV]=TRUE;
      }
      else {
         if (Trace[0]=='R') {
            roleprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            roleprm->Privilege[USERTRACEPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            roleprm->Privilege[USERTRACEPRIV]=FALSE;
         }
      }

      if (Security[0]=='Y') {
         roleprm->Privilege[USERSECURITYPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERSECURITYPRIV]=TRUE;
      }
      else {
         if (Security[0]=='R') {
            roleprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            roleprm->Privilege[USERSECURITYPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            roleprm->Privilege[USERSECURITYPRIV]=FALSE;
         }
      }

      if (Maintain_locations[0]=='Y') {
         roleprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERMAINTLOCPRIV]=TRUE;
      }
      else {
         if (Maintain_locations[0]=='R') {
            roleprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            roleprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            roleprm->Privilege[USERMAINTLOCPRIV]=FALSE;
         }
      }

      if (Operator[0]=='Y') {
         roleprm->Privilege[USEROPERATORPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USEROPERATORPRIV]=TRUE;
      }
      else {
         if (Operator[0]=='R') {
            roleprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            roleprm->Privilege[USEROPERATORPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            roleprm->Privilege[USEROPERATORPRIV]=FALSE;
         }
      }


      if (Maintain_users[0]=='Y') {
         roleprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERMAINTUSERPRIV]=TRUE;
      }
      else {
         if (Maintain_users[0]=='R') {
            roleprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            roleprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            roleprm->Privilege[USERMAINTUSERPRIV]=FALSE;
         }
      }


      if (Maintain_audit[0]=='Y') {
         roleprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=TRUE;
      }
      else {
         if (Maintain_audit[0]=='R') {
            roleprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            roleprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            roleprm->Privilege[USERMAINTAUDITPRIV]=FALSE;
         }
      }


      if (Auditor[0]=='Y') {
         roleprm->Privilege[USERAUDITALLPRIV]=TRUE;
         roleprm->bDefaultPrivilege[USERAUDITALLPRIV]=TRUE;
      }
      else {
         if (Auditor[0]=='R') {
            roleprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            roleprm->Privilege[USERAUDITALLPRIV]=TRUE;
         }
         else {
            roleprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            roleprm->Privilege[USERAUDITALLPRIV]=FALSE;
         }
      }
		//
		// MAC: write_down:
		if (szWriteDown[0]  == 'Y')
		{
			roleprm->Privilege[USERWRITEDOWN]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEDOWN]=TRUE;
		}
		else
		if (szWriteDown[0]  == 'R')
		{
			roleprm->Privilege[USERWRITEDOWN]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERWRITEDOWN]=FALSE;
			roleprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		//
		// MAC: write_fixed:
		if (szWriteFixed[0]  == 'Y')
		{
			roleprm->Privilege[USERWRITEFIXED]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEFIXED]=TRUE;
		}
		else
		if (szWriteFixed[0]  == 'R')
		{
			roleprm->Privilege[USERWRITEFIXED]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERWRITEFIXED]=FALSE;
			roleprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		//
		// MAC: write_up:
		if (szWriteUp[0]  == 'Y')
		{
			roleprm->Privilege[USERWRITEUP]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEUP]=TRUE;
		}
		else
		if (szWriteUp[0]  == 'R')
		{
			roleprm->Privilege[USERWRITEUP]=TRUE;
			roleprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERWRITEUP]=FALSE;
			roleprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		//
		// MAC: insert_down:
		if (szInsertDown[0]  == 'Y')
		{
			roleprm->Privilege[USERINSERTDOWN]=TRUE;
			roleprm->bDefaultPrivilege[USERINSERTDOWN]=TRUE;
		}
		else
		if (szInsertDown[0]  == 'R')
		{
			roleprm->Privilege[USERINSERTDOWN]=TRUE;
			roleprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERINSERTDOWN]=FALSE;
			roleprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		//
		// MAC: insert_up:
		if (szInsertUp[0]  == 'Y')
		{
			roleprm->Privilege[USERINSERTUP]=TRUE;
			roleprm->bDefaultPrivilege[USERINSERTUP]=TRUE;
		}
		else
		if (szInsertUp[0]  == 'R')
		{
			roleprm->Privilege[USERINSERTUP]=TRUE;
			roleprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERINSERTUP]=FALSE;
			roleprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		//
		// MAC: session_security_label:
		if (szSessionSecurityLable[0]  == 'Y')
		{
			roleprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			roleprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=TRUE;
		}
		else
		if (szSessionSecurityLable[0]  == 'R')
		{
			roleprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			roleprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}
		else
		{
			roleprm->Privilege[USERSESSIONSECURITYLABEL]=FALSE;
			roleprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}


      if (Audit_all[0]=='Y')
         roleprm->bSecAudit[USERALLEVENT]=TRUE;
      else
         roleprm->bSecAudit[USERALLEVENT]=FALSE;

      if (Audit_query_text[0]=='Y')
         roleprm->bSecAudit[USERQUERYTEXTEVENT]=TRUE;
      else
         roleprm->bSecAudit[USERQUERYTEXTEVENT]=FALSE;

      if (sNullInd==0) {
         suppspace(Lim_sec_label);
         roleprm->lpLimitSecLabels=ESL_AllocMem(x_strlen(Lim_sec_label)+1);
         if (!roleprm->lpLimitSecLabels) {
            bAllocOk=FALSE;
            exec sql endselect;
         }
         x_strcpy(roleprm->lpLimitSecLabels,Lim_sec_label);
      }

   exec sql end;    
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (!bAllocOk)
      return RES_ERR;   // allocation failed

   return SQLCodeToDom(sqlca.sqlcode);
}

static int GetInfProfOpenSource(LPPROFILEPARAMS profprm)
{
   BOOL bAllocOk=TRUE;
   exec sql begin declare section;
   long l1;
   char Default_group[MAXOBJECTNAME];
   char Expire_date[MAXDATELEN];
   char profname[MAXOBJECTNAME];
   char Creatdb[2], Trace[2], Audit_all[2], Security[2],
        Maintain_locations[2], Operator[2], Maintain_users[2],
        Maintain_audit[2], Auditor[2], Audit_query_text[2];

   char szWriteDown[2];
   char szWriteFixed[2];
   char szWriteUp[2];
   char szInsertDown[2];
   char szInsertUp[2];
   char szSessionSecurityLable[2];

   exec sql end declare section;
	x_strcpy(profname,profprm->ObjectName);
	if (!x_strcmp(profname,lpdefprofiledispstring())) {
		x_strcpy(profname,"");
		profprm->bDefProfile=TRUE;
	}

   exec sql repeated select  default_group,
        char(charextract('NRY', mod((p.status), 2) +mod((p.default_priv), 2) +1)),             /* createdb */
        char(charextract('NRY', mod((p.status/16), 2) +mod((p.default_priv/16), 2) +1)),       /*trace*/
        char(charextract('NRY', mod((p.status/32768), 2) +mod((p.default_priv/32768), 2) +1)), /*security*/
        char(charextract('NRY', mod((p.status/2048), 2) +mod((p.default_priv/2048), 2) +1)),   /* maintain_locations*/
        char(charextract('NRY', mod((p.status/512), 2) +mod((p.default_priv/512), 2) +1)),     /* operator */
        char(charextract('NRY', mod((p.status/65536), 2) +mod((p.default_priv/65536), 2) +1)), /* maintain users*/
        char(charextract('NRY', mod((p.status/16384), 2) +mod((p.default_priv/16384), 2) +1)), /* maintain_audit */
        char(charextract('NRY', mod((p.status/8192), 2) +mod((p.default_priv/8192), 2) +1)),  /* auditor */
        char(charextract('NY', mod((p.status/1024), 2) +1)),                                   /*audit_all */
        char(charextract('NY', mod((p.status/16777216), 2) +1)), /* audit_query_text */

        char(charextract('NRY', mod((p.status/131072),2)+mod((default_priv/131072),2)+1)),
        char(charextract('NRY', mod((p.status/8388608),2)+mod((default_priv/8388608),2)+1)),
        char(charextract('NRY', mod((p.status/524288),2)+mod((default_priv/524288),2)+1)),
        char(charextract('NRY', mod((p.status/262144),2)+mod((default_priv/262144),2)+1)),
        char(charextract('NRY', mod((p.status/1048576),2)+mod((default_priv/1048576),2)+1)),
        char(charextract('NRY', mod((p.status/4194304),2)+mod((default_priv/4194304),2)+1)),
        
        expire_date, 
        status
    into :Default_group, :Creatdb, :Trace, :Security,
        :Maintain_locations, :Operator, :Maintain_users,
        :Maintain_audit, :Auditor, :Audit_all,
        :Audit_query_text, 
        :szWriteDown,
        :szWriteFixed,
        :szWriteUp,
        :szInsertDown,
        :szInsertUp,
        :szSessionSecurityLable,
        :Expire_date, 
        :l1
    from     iiprofile p
    where    name=:profname;

   exec sql begin;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any

//      if ( l1 & 8192)
//        Auditor[0]='R';

      if (Creatdb[0]=='Y') {
         profprm->Privilege[USERCREATEDBPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERCREATEDBPRIV]=TRUE;
      }
      else {
         if (Creatdb[0]=='R') {
            profprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            profprm->Privilege[USERCREATEDBPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            profprm->Privilege[USERCREATEDBPRIV]=FALSE;
         }
      }

      if (Trace[0]=='Y') {
         profprm->Privilege[USERTRACEPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERTRACEPRIV]=TRUE;
      }
      else {
         if (Trace[0]=='R') {
            profprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            profprm->Privilege[USERTRACEPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            profprm->Privilege[USERTRACEPRIV]=FALSE;
         }
      }

      if (Security[0]=='Y') {
         profprm->Privilege[USERSECURITYPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERSECURITYPRIV]=TRUE;
      }
      else {
         if (Security[0]=='R') {
            profprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            profprm->Privilege[USERSECURITYPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            profprm->Privilege[USERSECURITYPRIV]=FALSE;
         }
      }

      if (Maintain_locations[0]=='Y') {
         profprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERMAINTLOCPRIV]=TRUE;
      }
      else {
         if (Maintain_locations[0]=='R') {
            profprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            profprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            profprm->Privilege[USERMAINTLOCPRIV]=FALSE;
         }
      }

      if (Operator[0]=='Y') {
         profprm->Privilege[USEROPERATORPRIV]=TRUE;
         profprm->bDefaultPrivilege[USEROPERATORPRIV]=TRUE;
      }
      else {
         if (Operator[0]=='R') {
            profprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            profprm->Privilege[USEROPERATORPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            profprm->Privilege[USEROPERATORPRIV]=FALSE;
         }
      }


      if (Maintain_users[0]=='Y') {
         profprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERMAINTUSERPRIV]=TRUE;
      }
      else {
         if (Maintain_users[0]=='R') {
            profprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            profprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            profprm->Privilege[USERMAINTUSERPRIV]=FALSE;
         }
      }


      if (Maintain_audit[0]=='Y') {
         profprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=TRUE;
      }
      else {
         if (Maintain_audit[0]=='R') {
            profprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            profprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            profprm->Privilege[USERMAINTAUDITPRIV]=FALSE;
         }
      }


      if (Auditor[0]=='Y') {
         profprm->Privilege[USERAUDITALLPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERAUDITALLPRIV]=TRUE;
      }
      else {
         if (Auditor[0]=='R') {
            profprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            profprm->Privilege[USERAUDITALLPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            profprm->Privilege[USERAUDITALLPRIV]=FALSE;
         }
      }
		//
		// MAC: write_down:
		if (szWriteDown[0]  == 'Y')
		{
			profprm->Privilege[USERWRITEDOWN]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEDOWN]=TRUE;
		}
		else
		if (szWriteDown[0]  == 'R')
		{
			profprm->Privilege[USERWRITEDOWN]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		else
		{
			profprm->Privilege[USERWRITEDOWN]=FALSE;
			profprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		//
		// MAC: write_fixed:
		if (szWriteFixed[0]  == 'Y')
		{
			profprm->Privilege[USERWRITEFIXED]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEFIXED]=TRUE;
		}
		else
		if (szWriteFixed[0]  == 'R')
		{
			profprm->Privilege[USERWRITEFIXED]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		else
		{
			profprm->Privilege[USERWRITEFIXED]=FALSE;
			profprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		//
		// MAC: write_up:
		if (szWriteUp[0]  == 'Y')
		{
			profprm->Privilege[USERWRITEUP]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEUP]=TRUE;
		}
		else
		if (szWriteUp[0]  == 'R')
		{
			profprm->Privilege[USERWRITEUP]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		else
		{
			profprm->Privilege[USERWRITEUP]=FALSE;
			profprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		//
		// MAC: insert_down:
		if (szInsertDown[0]  == 'Y')
		{
			profprm->Privilege[USERINSERTDOWN]=TRUE;
			profprm->bDefaultPrivilege[USERINSERTDOWN]=TRUE;
		}
		else
		if (szInsertDown[0]  == 'R')
		{
			profprm->Privilege[USERINSERTDOWN]=TRUE;
			profprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		else
		{
			profprm->Privilege[USERINSERTDOWN]=FALSE;
			profprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		//
		// MAC: insert_up:
		if (szInsertUp[0]  == 'Y')
		{
			profprm->Privilege[USERINSERTUP]=TRUE;
			profprm->bDefaultPrivilege[USERINSERTUP]=TRUE;
		}
		else
		if (szInsertUp[0]  == 'R')
		{
			profprm->Privilege[USERINSERTUP]=TRUE;
			profprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		else
		{
			profprm->Privilege[USERINSERTUP]=FALSE;
			profprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		//
		// MAC: session_security_label:
		if (szSessionSecurityLable[0]  == 'Y')
		{
			profprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			profprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=TRUE;
		}
		else
		if (szSessionSecurityLable[0]  == 'R')
		{
			profprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			profprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}
		else
		{
			profprm->Privilege[USERSESSIONSECURITYLABEL]=FALSE;
			profprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}


      if (Audit_all[0]=='Y')
         profprm->bSecAudit[USERALLEVENT]=TRUE;
      else
         profprm->bSecAudit[USERALLEVENT]=FALSE;

      if (Audit_query_text[0]=='Y')
         profprm->bSecAudit[USERQUERYTEXTEVENT]=TRUE;
      else
         profprm->bSecAudit[USERQUERYTEXTEVENT]=FALSE;

      suppspace(Default_group);
      x_strcpy(profprm->DefaultGroup,Default_group);

      suppspace(Expire_date);
      x_strcpy(profprm->ExpireDate,Expire_date);

   exec sql end;    
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (!bAllocOk)
      return RES_ERR;   // allocation failed

   return SQLCodeToDom(sqlca.sqlcode);
}            

int GetInfProf(LPPROFILEPARAMS profprm)
{
   BOOL bAllocOk=TRUE;

   exec sql begin declare section;
   short sNullInd;
   long l1;
   char Default_group[MAXOBJECTNAME];
   char Expire_date[MAXDATELEN];
   char profname[MAXOBJECTNAME];
   char Lim_sec_label[MAXLIMITSECLBL];
   char Creatdb[2], Trace[2], Audit_all[2], Security[2],
        Maintain_locations[2], Operator[2], Maintain_users[2],
        Maintain_audit[2], Auditor[2], Audit_query_text[2];

   char szWriteDown[2];
   char szWriteFixed[2];
   char szWriteUp[2];
   char szInsertDown[2];
   char szInsertUp[2];
   char szSessionSecurityLable[2];

   exec sql end declare section;

   if (GetOIVers() >= OIVERS_30)
      return GetInfProfOpenSource(profprm);
   
	x_strcpy(profname,profprm->ObjectName);
	if (!x_strcmp(profname,lpdefprofiledispstring())) {
		x_strcpy(profname,"");
		profprm->bDefProfile=TRUE;
	}
/******************
create view  iiprofiles(
					1	profile_name,
					2	createdb,
					3	trace, 
					4	audit_all,
					5	security,
					6	maintain_locations,
					7	operator, 
					8	maintain_users,
					9	maintain_audit, 
					10	auditor, 
					11	audit_query_text,
					12	expire_date, 
					13	lim_sec_label,
					14	default_group,
					15	internal_status)
						as select
					1	p.name,
					2	char(charextract('NRY', mod((p.status), 2) +mod((p.default_priv), 2) +1)),
					3	char(charextract('NRY', mod((p.status/16), 2) +mod((p.default_priv/16), 2) +1)),
					4	char(charextract('NY', mod((p.status/1024), 2) +1)), 
					5	char(charextract('NRY', mod((p.status/32768), 2) +mod((p.default_priv/32768), 2) +1)), 
					6	char(charextract('NRY', mod((p.status/2048), 2) +mod((p.default_priv/2048), 2) +1)),
					7	char(charextract('NRY', mod((p.status/512), 2) +mod((p.default_priv/512), 2) +1)),
					8	char(charextract('NRY', mod((p.status/65536), 2) +mod((p.default_priv/65536), 2) +1)),
					9	char(charextract('NRY', mod((p.status/16384), 2) +mod((p.default_priv/16384), 2) +1)),
					10	char(charextract('NRY', mod((p.status/8192), 2) +mod((p.default_priv/8192), 2) +1)), 
					11	char(charextract('NY', mod((p.status/16777216), 2) +1)),
					12	p.expire_date,
					13	p.lim_secid,
					14	p.default_group,
					15	p.status
						 from "$ingres". iiprofile p where p.name!=''
***************/

   exec sql repeated select  default_group,
					char(charextract('NRY', mod((p.status), 2) +mod((p.default_priv), 2) +1)),             /* createdb */
					char(charextract('NRY', mod((p.status/16), 2) +mod((p.default_priv/16), 2) +1)),       /*trace*/
					char(charextract('NRY', mod((p.status/32768), 2) +mod((p.default_priv/32768), 2) +1)), /*security*/
					char(charextract('NRY', mod((p.status/2048), 2) +mod((p.default_priv/2048), 2) +1)),   /* maintain_locations*/
					char(charextract('NRY', mod((p.status/512), 2) +mod((p.default_priv/512), 2) +1)),     /* operator */
					char(charextract('NRY', mod((p.status/65536), 2) +mod((p.default_priv/65536), 2) +1)), /* maintain users*/
					char(charextract('NRY', mod((p.status/16384), 2) +mod((p.default_priv/16384), 2) +1)), /* maintain_audit */
					char(charextract('NRY', mod((p.status/8192), 2) +mod((p.default_priv/8192), 2) +1)),  /* auditor */
					char(charextract('NY', mod((p.status/1024), 2) +1)),                                   /*audit_all */
					char(charextract('NY', mod((p.status/16777216), 2) +1)), /* audit_query_text */

					char(charextract('NRY', mod((p.status/131072),2)+mod((default_priv/131072),2)+1)),
					char(charextract('NRY', mod((p.status/8388608),2)+mod((default_priv/8388608),2)+1)),
					char(charextract('NRY', mod((p.status/524288),2)+mod((default_priv/524288),2)+1)),
					char(charextract('NRY', mod((p.status/262144),2)+mod((default_priv/262144),2)+1)),
					char(charextract('NRY', mod((p.status/1048576),2)+mod((default_priv/1048576),2)+1)),
					char(charextract('NRY', mod((p.status/4194304),2)+mod((default_priv/4194304),2)+1)),
					
					expire_date, 
					lim_secid,
					status
            into     :Default_group, :Creatdb, :Trace, :Security,
                     :Maintain_locations, :Operator, :Maintain_users,
                     :Maintain_audit, :Auditor, :Audit_all,
                     :Audit_query_text, 
                     :szWriteDown,
                     :szWriteFixed,
                     :szWriteUp,
                     :szInsertDown,
                     :szInsertUp,
                     :szSessionSecurityLable,
                     :Expire_date, :Lim_sec_label:sNullInd,:l1
            from     iiprofile p
            where    name=:profname;

   exec sql begin;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any

//      if ( l1 & 8192)
//        Auditor[0]='R';

      if (Creatdb[0]=='Y') {
         profprm->Privilege[USERCREATEDBPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERCREATEDBPRIV]=TRUE;
      }
      else {
         if (Creatdb[0]=='R') {
            profprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            profprm->Privilege[USERCREATEDBPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERCREATEDBPRIV]=FALSE;
            profprm->Privilege[USERCREATEDBPRIV]=FALSE;
         }
      }

      if (Trace[0]=='Y') {
         profprm->Privilege[USERTRACEPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERTRACEPRIV]=TRUE;
      }
      else {
         if (Trace[0]=='R') {
            profprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            profprm->Privilege[USERTRACEPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERTRACEPRIV]=FALSE;
            profprm->Privilege[USERTRACEPRIV]=FALSE;
         }
      }

      if (Security[0]=='Y') {
         profprm->Privilege[USERSECURITYPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERSECURITYPRIV]=TRUE;
      }
      else {
         if (Security[0]=='R') {
            profprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            profprm->Privilege[USERSECURITYPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERSECURITYPRIV]=FALSE;
            profprm->Privilege[USERSECURITYPRIV]=FALSE;
         }
      }

      if (Maintain_locations[0]=='Y') {
         profprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERMAINTLOCPRIV]=TRUE;
      }
      else {
         if (Maintain_locations[0]=='R') {
            profprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            profprm->Privilege[USERMAINTLOCPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERMAINTLOCPRIV]=FALSE;
            profprm->Privilege[USERMAINTLOCPRIV]=FALSE;
         }
      }

      if (Operator[0]=='Y') {
         profprm->Privilege[USEROPERATORPRIV]=TRUE;
         profprm->bDefaultPrivilege[USEROPERATORPRIV]=TRUE;
      }
      else {
         if (Operator[0]=='R') {
            profprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            profprm->Privilege[USEROPERATORPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USEROPERATORPRIV]=FALSE;
            profprm->Privilege[USEROPERATORPRIV]=FALSE;
         }
      }


      if (Maintain_users[0]=='Y') {
         profprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERMAINTUSERPRIV]=TRUE;
      }
      else {
         if (Maintain_users[0]=='R') {
            profprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            profprm->Privilege[USERMAINTUSERPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERMAINTUSERPRIV]=FALSE;
            profprm->Privilege[USERMAINTUSERPRIV]=FALSE;
         }
      }


      if (Maintain_audit[0]=='Y') {
         profprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=TRUE;
      }
      else {
         if (Maintain_audit[0]=='R') {
            profprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            profprm->Privilege[USERMAINTAUDITPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERMAINTAUDITPRIV]=FALSE;
            profprm->Privilege[USERMAINTAUDITPRIV]=FALSE;
         }
      }


      if (Auditor[0]=='Y') {
         profprm->Privilege[USERAUDITALLPRIV]=TRUE;
         profprm->bDefaultPrivilege[USERAUDITALLPRIV]=TRUE;
      }
      else {
         if (Auditor[0]=='R') {
            profprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            profprm->Privilege[USERAUDITALLPRIV]=TRUE;
         }
         else {
            profprm->bDefaultPrivilege[USERAUDITALLPRIV]=FALSE;
            profprm->Privilege[USERAUDITALLPRIV]=FALSE;
         }
      }
		//
		// MAC: write_down:
		if (szWriteDown[0]  == 'Y')
		{
			profprm->Privilege[USERWRITEDOWN]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEDOWN]=TRUE;
		}
		else
		if (szWriteDown[0]  == 'R')
		{
			profprm->Privilege[USERWRITEDOWN]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		else
		{
			profprm->Privilege[USERWRITEDOWN]=FALSE;
			profprm->bDefaultPrivilege[USERWRITEDOWN]=FALSE;
		}
		//
		// MAC: write_fixed:
		if (szWriteFixed[0]  == 'Y')
		{
			profprm->Privilege[USERWRITEFIXED]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEFIXED]=TRUE;
		}
		else
		if (szWriteFixed[0]  == 'R')
		{
			profprm->Privilege[USERWRITEFIXED]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		else
		{
			profprm->Privilege[USERWRITEFIXED]=FALSE;
			profprm->bDefaultPrivilege[USERWRITEFIXED]=FALSE;
		}
		//
		// MAC: write_up:
		if (szWriteUp[0]  == 'Y')
		{
			profprm->Privilege[USERWRITEUP]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEUP]=TRUE;
		}
		else
		if (szWriteUp[0]  == 'R')
		{
			profprm->Privilege[USERWRITEUP]=TRUE;
			profprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		else
		{
			profprm->Privilege[USERWRITEUP]=FALSE;
			profprm->bDefaultPrivilege[USERWRITEUP]=FALSE;
		}
		//
		// MAC: insert_down:
		if (szInsertDown[0]  == 'Y')
		{
			profprm->Privilege[USERINSERTDOWN]=TRUE;
			profprm->bDefaultPrivilege[USERINSERTDOWN]=TRUE;
		}
		else
		if (szInsertDown[0]  == 'R')
		{
			profprm->Privilege[USERINSERTDOWN]=TRUE;
			profprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		else
		{
			profprm->Privilege[USERINSERTDOWN]=FALSE;
			profprm->bDefaultPrivilege[USERINSERTDOWN]=FALSE;
		}
		//
		// MAC: insert_up:
		if (szInsertUp[0]  == 'Y')
		{
			profprm->Privilege[USERINSERTUP]=TRUE;
			profprm->bDefaultPrivilege[USERINSERTUP]=TRUE;
		}
		else
		if (szInsertUp[0]  == 'R')
		{
			profprm->Privilege[USERINSERTUP]=TRUE;
			profprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		else
		{
			profprm->Privilege[USERINSERTUP]=FALSE;
			profprm->bDefaultPrivilege[USERINSERTUP]=FALSE;
		}
		//
		// MAC: session_security_label:
		if (szSessionSecurityLable[0]  == 'Y')
		{
			profprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			profprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=TRUE;
		}
		else
		if (szSessionSecurityLable[0]  == 'R')
		{
			profprm->Privilege[USERSESSIONSECURITYLABEL]=TRUE;
			profprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}
		else
		{
			profprm->Privilege[USERSESSIONSECURITYLABEL]=FALSE;
			profprm->bDefaultPrivilege[USERSESSIONSECURITYLABEL]=FALSE;
		}


      if (Audit_all[0]=='Y')
         profprm->bSecAudit[USERALLEVENT]=TRUE;
      else
         profprm->bSecAudit[USERALLEVENT]=FALSE;

      if (Audit_query_text[0]=='Y')
         profprm->bSecAudit[USERQUERYTEXTEVENT]=TRUE;
      else
         profprm->bSecAudit[USERQUERYTEXTEVENT]=FALSE;

      suppspace(Default_group);
      x_strcpy(profprm->DefaultGroup,Default_group);

      suppspace(Expire_date);
      x_strcpy(profprm->ExpireDate,Expire_date);
      if (sNullInd==0) {
         suppspace(Lim_sec_label);
         profprm->lpLimitSecLabels=ESL_AllocMem(x_strlen(Lim_sec_label)+1);
         if (!profprm->lpLimitSecLabels) {
            bAllocOk=FALSE;
            exec sql endselect;
         }
         x_strcpy(profprm->lpLimitSecLabels,Lim_sec_label);
      }

   exec sql end;    
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (!bAllocOk)
      return RES_ERR;   // allocation failed

   return SQLCodeToDom(sqlca.sqlcode);
}            
int GetInfIntegrity(LPINTEGRITYPARAMS integprm)

{
   exec sql begin declare section;
   char Text[SEGMENT_TEXT_SIZE+1];
   long Number=integprm->number;
   char *Table_name=integprm->TableName;
   char *lpTableOwner=integprm->TableOwner;
   exec sql end declare section;
   int slen;

   // Set bOnlyText to TRUE since we get the full text and do not parse it
   integprm->bOnlyText = TRUE;

   // get all text segment an concatenate them

   exec sql repeated select text_segment 
            into   :Text
            from   iiintegrities
            where  table_name = :Table_name
             and   table_owner = :lpTableOwner
             and   integrity_number = :Number;

   exec sql begin;

      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      if (integprm->lpIntegrityText) {
         slen=x_strlen(integprm->lpIntegrityText);
         integprm->lpIntegrityText=
                  ESL_ReAllocMem(integprm->lpIntegrityText,
                                 slen+SEGMENT_TEXT_SIZE+1,
                                 slen); 
      }
      else
         integprm->lpIntegrityText=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);

      if (integprm->lpIntegrityText) 
         x_strcat(integprm->lpIntegrityText, Text);  // concatenates the strings.
      else
         exec sql endselect;

   exec sql end;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   
   if (!integprm->lpIntegrityText)   
      return RES_ERR;   // allocation failed
   return SQLCodeToDom(sqlca.sqlcode);

}

int GetInfProcedure(LPPROCEDUREPARAMS procprm)
{
   exec sql begin declare section;
   char Text[SEGMENT_TEXT_SIZE+1];
   char *Proc_name=procprm->objectname;
   char *ProcOwner=procprm->objectowner;
   exec sql end declare section;
   int slen;
   LPUCHAR lptemp;

   // Set bOnlyText to TRUE since we get the full text and do not parse it
   procprm->bOnlyText = TRUE;

   // get all text segment an concatenate them

   exec sql repeated select   text_segment
            into     :Text
            from     iiprocedures
            where    procedure_name = :Proc_name
             and     procedure_owner = :ProcOwner;


   exec sql begin;

      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      if (procprm->lpProcedureText) {
         slen=x_strlen(procprm->lpProcedureText);
         procprm->lpProcedureText=
                  ESL_ReAllocMem(procprm->lpProcedureText,
                                 slen+SEGMENT_TEXT_SIZE+1,
                                 slen);
      }
      else
         procprm->lpProcedureText=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);

      if (procprm->lpProcedureText)
      {
         lptemp=Text;
         while (lptemp=x_strpbrk(Text,"\n\r\t"))
            *lptemp=' ';
         x_strcat(procprm->lpProcedureText, Text);  // concatenates the strings.
      }
      else
         exec sql endselect;

   exec sql end;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   if (!procprm->lpProcedureText)
      return RES_ERR;   // allocation failed
   return SQLCodeToDom(sqlca.sqlcode);
}
int GetInfRule(LPRULEPARAMS ruleprm)

{
   exec sql begin declare section;
   char Text[SEGMENT_TEXT_SIZE+1];
   char *RuleOwner=ruleprm->RuleOwner;
   char *Rule_name=ruleprm->RuleName;
   char *Table_name=ruleprm->TableName;
   exec sql end declare section;
   int slen;

   // Set bOnlyText to TRUE since we get the full text and do not parse it
   ruleprm->bOnlyText = TRUE;

   // get all text segment an concatenate them

   exec sql repeated select   text_segment 
            into     :Text
            from     iirules
            where    rule_owner = :RuleOwner
             and     rule_name  = :Rule_name
             and     table_name = :Table_name;

   exec sql begin;

      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      if (ruleprm->lpRuleText) {
         slen=x_strlen(ruleprm->lpRuleText);
         ruleprm->lpRuleText=
                  ESL_ReAllocMem(ruleprm->lpRuleText,
                                 slen+SEGMENT_TEXT_SIZE+1,
                                 slen); 
      }
      else
         ruleprm->lpRuleText=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);

      if (ruleprm->lpRuleText) 
         x_strcat(ruleprm->lpRuleText, Text);  // concatenates the strings.
      else
         exec sql endselect;

   exec sql end;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   
   if (!ruleprm->lpRuleText)   
      return RES_ERR;   // allocation failed
   return SQLCodeToDom(sqlca.sqlcode);
}

int GetInfView(LPVIEWPARAMS viewprm)

{
   exec sql begin declare section;
    char Text[SEGMENT_TEXT_SIZE+1];
    char *pTextSeg;
    char szTableName[MAXOBJECTNAME];
    unsigned char *lpTableOwner;
    int ColLength,nTextNum;
   exec sql end declare section;
   int slen,iret;
   BOOL bGenericGateway = FALSE;

   x_strcpy(szTableName, viewprm->objectname);
   lpTableOwner=viewprm->szSchema;

   // Set bOnlyText to TRUE since we get the full text and do not parse it
   viewprm->bOnlyText = TRUE;

   if (GetOIVers() == OIVERS_NOTOI)
      bGenericGateway = TRUE;

   if (bGenericGateway) {
       // the length of the column "text_segment" will vary from gateway to
       // gateway.
       ColLength = 0;
       pTextSeg = NULL;
       // get length of column text_segment in iicolumns
       EXEC SQL repeated SELECT column_length 
                INTO :ColLength
                FROM iicolumns 
                WHERE (table_name = 'IIVIEWS'and column_name = 'TEXT_SEGMENT') 
				     OR (table_name = 'iiviews'and column_name = 'text_segment') ;
       // get all text segment an concatenate them
       if ( sqlca.sqlcode != 0) {
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          return SQLCodeToDom(sqlca.sqlcode);
       }
       pTextSeg = ESL_AllocMem(ColLength+1);
       if (!pTextSeg) {
           myerror(ERR_ALLOCMEM);
           return RES_ERR;   // allocation failed
       }
       iret=RES_SUCCESS;
       exec sql repeated select   text_segment ,text_sequence
                into     :pTextSeg , :nTextNum
                from     iiviews
                where    table_name = :szTableName
                 and     table_owner= :lpTableOwner
                 order by text_sequence;
       exec sql begin;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       if (viewprm->lpViewText) {
           slen=x_strlen(viewprm->lpViewText);
           viewprm->lpViewText=
                    ESL_ReAllocMem(viewprm->lpViewText,
                                   slen+ColLength+1,
                                   slen); 
       }
       else
          viewprm->lpViewText=ESL_AllocMem(ColLength+1);

       if (!viewprm->lpViewText) {
          iret=RES_ERR;
          exec sql endselect;
       }
       x_strcat(viewprm->lpViewText, pTextSeg);  // concatenates the strings.
       exec sql end;

       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       if (pTextSeg) {
           ESL_FreeMem (pTextSeg);
           pTextSeg = NULL;
       }
       if (iret==RES_ERR) { // allocation problem
           FreeAttachedPointers(viewprm, OT_VIEW);
           return RES_ERR;
       }

       iret=SQLCodeToDom(sqlca.sqlcode);
       if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
           FreeAttachedPointers(viewprm, OT_VIEW);

       return iret;
   }
   else {
       // get all text segment an concatenate them

       iret=RES_SUCCESS;
       exec sql repeated select   text_segment, text_sequence
                into     :Text ,:nTextNum
                from     iiviews
                where    table_name = :szTableName
                 and     table_owner= :lpTableOwner
                 order by text_sequence;
       exec sql begin;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       if (viewprm->lpViewText) {
          slen=x_strlen(viewprm->lpViewText);
          viewprm->lpViewText=
                   ESL_ReAllocMem(viewprm->lpViewText,
                                  slen+SEGMENT_TEXT_SIZE+1,
                                  slen); 
       }
       else
          viewprm->lpViewText=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);

       if (!viewprm->lpViewText) {
          iret=RES_ERR;
          exec sql endselect;
       }
       x_strcat(viewprm->lpViewText, Text);  // concatenates the strings.

       exec sql end;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any

       if (iret==RES_ERR) { // allocation problem
           FreeAttachedPointers(viewprm, OT_VIEW);
           return RES_ERR;
       }
       iret=SQLCodeToDom(sqlca.sqlcode);
       if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
           FreeAttachedPointers(viewprm, OT_VIEW);

       return iret;
   }
}

int GetInfIndex(LPINDEXPARAMS indexprm)

{
   BOOL bGenericGateway = FALSE;
   LPOBJECTLIST lpList = NULL;
   LPOBJECTLIST lpTemp = NULL;
   LPIDXCOLS lpObj;
   int iret;

   struct StorageStruct {
      LPUCHAR lpVal;
      int nVal;
   } StorageStructKnown[] = {
      {"btree",    IDX_BTREE     },
      {"BTREE",    IDX_BTREE     },
      {"isam",     IDX_ISAM      },
      {"ISAM",     IDX_ISAM      },
      {"hash",     IDX_HASH      },
      {"HASH",     IDX_HASH      },
      {"heap",     IDX_HEAP      },
      {"HEAP",     IDX_HEAP      },
      {"heapsort", IDX_HEAPSORT  },
      {"HEAPSORT", IDX_HEAPSORT  },
      {"rtree",    IDX_RTREE     },
      {"RTREE",    IDX_RTREE     },
      {"vectorwise", IDX_VW	 },
      {"VECTORWISE", IDX_VW	 },
      {"vectorwise_ix", IDX_VWIX  },
      {"VECTORWISE_IX", IDX_VWIX  },
      {"\0", 0}};
   struct StorageStruct *lpStorageStruct;

   BOOL bStarDatabase   = FALSE;

   exec sql begin declare section;
   unsigned char szIndexName[MAXOBJECTNAME];
   unsigned char *lpIndexOwner=indexprm->objectowner;
   unsigned char szTableName[MAXOBJECTNAME];
   unsigned char szTableOwner[MAXOBJECTNAME];
   unsigned char szStructure[MAXOBJECTNAME];
   unsigned char szKeyCompression[MAXOBJECTNAME];
   unsigned char szDataCompression[MAXOBJECTNAME];
   unsigned char szPersistence[MAXOBJECTNAME];
   unsigned char szUnique[MAXOBJECTNAME];
   int ilocseq;
   int nFillFactor;
   long nMinPages;
   long nMaxPages;
   int nLeafFill;
   int nNonLeafFill;
   long lAllocation;
   long lExtend;
   long lPage_size;
   long lAllocatedPages;

   unsigned char szMultiLocation[MAXOBJECTNAME];
   int nKeySequence;
   unsigned char szColumnName[MAXOBJECTNAME];
   unsigned char szLocation[MAXOBJECTNAME];
   unsigned char szMinX[32];
   unsigned char szMaxX[32];
   unsigned char szMinY[32];
   unsigned char szMaxY[32];
   exec sql end declare section;

   //--- Get informations from the iiindexes catalog table ---

   x_strcpy(szIndexName,indexprm->objectname);

   indexprm->nObjectType=OT_INDEX;

   if (GetOIVers() == OIVERS_NOTOI)
      bGenericGateway = TRUE;
   else {
     int nodeHandle = GetVirtNodeHandle(indexprm->szNodeName);
     if (nodeHandle >= 0)
       if (IsStarDatabase(nodeHandle, indexprm->DBName))
         bStarDatabase = TRUE;
   }

   if (bGenericGateway || bStarDatabase) {
       ZEROINIT (szKeyCompression);
       indexprm->bKeyCompression = FALSE;
       ZEROINIT (szPersistence);
       indexprm->bPersistence = FALSE;
       ZEROINIT (szUnique);
       indexprm->nUnique = IDX_UNIQUE_UNKNOWN;
       indexprm->uPage_size = lPage_size = -1;
       exec sql repeated select base_name   , base_owner    , storage_structure , is_compressed
           into :szTableName       , :szTableOwner , :szStructure      ,:szDataCompression
           from  iiindexes
           where index_name = :szIndexName and index_owner = :lpIndexOwner;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       iret=SQLCodeToDom(sqlca.sqlcode);
       if (iret!=RES_SUCCESS)
          return iret;

       suppspace(szTableName); 
       suppspace(szTableOwner);
       fstrncpy(indexprm->TableName, szTableName, MAXOBJECTNAME);
       fstrncpy(indexprm->TableOwner, szTableOwner, MAXOBJECTNAME);

       // Convert the storage structure string into the internal integer
       suppspace(szStructure);
       indexprm->nStructure = IDX_STORAGE_STRUCT_UNKNOWN;
       if (x_strlen(szStructure) >0) {
           lpStorageStruct=&StorageStructKnown[0];
           while (*(lpStorageStruct->lpVal)) {
              if (x_strcmp(lpStorageStruct->lpVal, szStructure)==0) {
                 indexprm->nStructure=lpStorageStruct->nVal;
                 break;
              }
              lpStorageStruct++;
           }
       }
       if (szDataCompression[0]=='Y')
          indexprm->bDataCompression=TRUE;
       else
          indexprm->bDataCompression=FALSE;
       // information retrieved in iitables not available for a Gateway
       lAllocation = -1;
       lExtend     = -1;
       nFillFactor = -1;
       nMinPages   = -1;
       nMaxPages   = -1;
       nLeafFill   = -1;
       nNonLeafFill= -1;
	   lAllocatedPages	= -1;
       ZEROINIT(szMultiLocation);
       ZEROINIT(szLocation);

       indexprm->nFillFactor=nFillFactor;
       indexprm->nMinPages=nMinPages;
       indexprm->nMaxPages=nMaxPages;
       indexprm->nLeafFill=nLeafFill;
       indexprm->nNonLeafFill=nNonLeafFill;
       indexprm->lAllocation=lAllocation;
       indexprm->lExtend=lExtend;
       indexprm->lAllocatedPages = lAllocatedPages;

       //--- get the index columns from iicolumns catlog
       //--- reject the columns that are not part of the table such as
       //--- tidp.
             
       indexprm->lpidxCols=NULL;
       exec sql repeated select   a.column_name, a.key_sequence
                into     :szColumnName, :nKeySequence
                from     iiindex_columns a
                where    index_name=:szIndexName
                 and     index_owner = :lpIndexOwner
                 and     exists(  select   *
                                  from     iicolumns b
                                  where    b.table_name  = :szTableName
                                  and      b.table_owner = :szTableOwner
                                  and      b.column_name = a.column_name)
                order by key_sequence desc; 
       exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          lpTemp = AddListObject(indexprm->lpidxCols, sizeof(IDXCOLS));
          if (!lpTemp) {   // Need to free previously allocated objects.
             FreeAttachedPointers(indexprm, OT_INDEX);
             iret=RES_ERR;
             exec sql endselect;
          }
          else
             indexprm->lpidxCols = lpTemp;

          lpObj = (LPIDXCOLS)indexprm->lpidxCols->lpObject;

          suppspace(szColumnName);
          x_strcpy(lpObj->szColName, szColumnName);
       exec sql end;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       iret=SQLCodeToDom(sqlca.sqlcode);
       if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
          FreeAttachedPointers(indexprm, OT_INDEX);
          return iret;
       }

       // scan all index columns to determine if they are part of the Key

       lpTemp=indexprm->lpidxCols;
       while (lpTemp) {

          lpObj = (LPIDXCOLS)lpTemp->lpObject;
          x_strcpy(szColumnName, lpObj->szColName);
          exec sql repeated select   key_sequence
                   into     :nKeySequence
                   from     iiindex_columns
                   where    index_name  = :szIndexName
                      and   column_name = :szColumnName
                      and   index_owner = :lpIndexOwner;

          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
             FreeAttachedPointers(indexprm, OT_INDEX);
             return iret;
          }
          if (sqlca.sqlcode==0)   // if found column is a key field
             lpObj->attr.bKey=TRUE;
          else 
             lpObj->attr.bKey=FALSE;
          lpTemp=lpTemp->lpNext;
       }
       return RES_SUCCESS;
   } // END bGenericGateway == TRUE
   else {
       if (GetOIVers() == OIVERS_12)
       {
            exec sql repeated select   base_name, base_owner, storage_structure,
                         key_is_compressed, is_compressed, persistent,
                         unique_scope
                into     :szTableName, :szTableOwner, :szStructure,
                         :szKeyCompression, :szDataCompression, :szPersistence,
                         :szUnique
                from     iiindexes
                where    index_name = :szIndexName
                 and     index_owner= :lpIndexOwner;
       }
       else
       {
            exec sql repeated select
                    base_name, 
                    base_owner, 
                    storage_structure, 
                    key_is_compressed, 
                    is_compressed, 
                    persistent,
                    unique_scope,
                    index_pagesize
                into     
                    :szTableName, 
                    :szTableOwner, 
                    :szStructure,
                    :szKeyCompression, 
                    :szDataCompression, 
                    :szPersistence,
                    :szUnique,
                    :lPage_size
                from  iiindexes
                where index_name = :szIndexName and index_owner = :lpIndexOwner;
       }
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       iret=SQLCodeToDom(sqlca.sqlcode);
       if (iret!=RES_SUCCESS)
          return iret;

       suppspace(szTableName); 
       suppspace(szTableOwner);
       fstrncpy(indexprm->TableName, szTableName, MAXOBJECTNAME);
       fstrncpy(indexprm->TableOwner, szTableOwner, MAXOBJECTNAME);

       // Convert the storage structure string into the internal integer
       suppspace(szStructure);
       lpStorageStruct=&StorageStructKnown[0];
       while (*(lpStorageStruct->lpVal)) {
          if (x_strcmp(lpStorageStruct->lpVal, szStructure)==0) {
             indexprm->nStructure=lpStorageStruct->nVal;
             break;
          }
          lpStorageStruct++;
       }
       if (GetOIVers() != OIVERS_12)
       {
          indexprm->uPage_size = lPage_size;
       }

       if (szKeyCompression[0]=='Y')
          indexprm->bKeyCompression=TRUE;
       else
          indexprm->bKeyCompression=FALSE;
      
       if (szDataCompression[0]=='Y')
          indexprm->bDataCompression=TRUE;
       else
          indexprm->bDataCompression=FALSE;

       if (szPersistence[0]=='Y')
          indexprm->bPersistence=TRUE;
       else
          indexprm->bPersistence=FALSE;
      
       switch (szUnique[0]) {
          case 'R' :
             indexprm->nUnique=IDX_UNIQUE_ROW;
             break;
          case 'S' :
             indexprm->nUnique=IDX_UNIQUE_STATEMENT;
             break;
          default :
             indexprm->nUnique=IDX_UNIQUE_NO;
             break;
       }

        //--- get the infos from the iitable catalog table ---    
           exec sql repeated select   table_dfillpct, table_minpages, table_maxpages,
                             table_lfillpct, table_ifillpct, allocation_size,
                             extend_size, multi_locations, location_name,
							 allocated_pages
                    into     :nFillFactor, :nMinPages, :nMaxPages,
                             :nLeafFill, :nNonLeafFill, :lAllocation,
                             :lExtend, :szMultiLocation, :szLocation,
							 :lAllocatedPages
                    from     iitables
                    where    table_name = :szIndexName
                     and     table_owner= :lpIndexOwner;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       iret=SQLCodeToDom(sqlca.sqlcode);
       if (iret!=RES_SUCCESS)
          return iret;

       indexprm->nFillFactor=nFillFactor;
       indexprm->nMinPages=nMinPages;
       indexprm->nMaxPages=nMaxPages;
       indexprm->nLeafFill=nLeafFill;
       indexprm->nNonLeafFill=nNonLeafFill;
       indexprm->lAllocation=lAllocation;
       indexprm->lExtend=lExtend;
       indexprm->lAllocatedPages = lAllocatedPages;

       indexprm->lpLocations=AddListObject(NULL, MAXOBJECTNAME);
       if (!indexprm->lpLocations) {   // Need to free previously allocated objects.
          FreeAttachedPointers(indexprm, OT_INDEX);
          return RES_ERR;
       }

       suppspace(szLocation);
       fstrncpy((LPUCHAR)indexprm->lpLocations->lpObject ,szLocation,
                MAXOBJECTNAME);

       //--- if the index is multi location get all the locations from   ---
       //--- iimulti_location catalog table                              --- 

       if (szMultiLocation[0]=='Y') {      
          iret=RES_SUCCESS;
          exec sql repeated select   location_name, loc_sequence 
                   into     :szLocation, :ilocseq
                   from     iimulti_locations
                   where    table_name=:szIndexName
                   order by loc_sequence desc;
          exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             suppspace(szLocation);
             if (*szLocation) {
                lpTemp=AddListObject(indexprm->lpLocations, MAXOBJECTNAME);
                if (!lpTemp) {
                   iret=RES_ERR;
                   exec sql endselect;
                }
                indexprm->lpLocations=lpTemp;
                fstrncpy((LPUCHAR)indexprm->lpLocations->lpObject ,szLocation,
                   MAXOBJECTNAME);
             }
          exec sql end;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          if (iret==RES_ERR) { // allocation problem
             FreeAttachedPointers(indexprm, OT_INDEX);
             myerror(ERR_ALLOCMEM);
             return RES_ERR;
          }
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
             FreeAttachedPointers(indexprm, OT_INDEX);
             return iret;
          }
       }

       // For Rtree index retrieve Max and Min Values
       if (indexprm->nStructure == IDX_RTREE)
       {
          exec sql repeated select char(a.rng_ll1),char(a.rng_ur1),
                                   char(a.rng_ll2),char(a.rng_ur2)
                   into :szMinX,:szMaxX,:szMinY,:szMaxY
                   from iirange a, iirelation b
                   where  b.reltid=a.rng_baseid and b.reltidx =a.rng_indexid
                    and b.relid=:szIndexName
                    and b.relowner=:lpIndexOwner;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS) {
             FreeAttachedPointers(indexprm, OT_INDEX);
             return iret;
          }
          x_strcpy(indexprm->minX, szMinX);
          x_strcpy(indexprm->maxX, szMaxX);
          x_strcpy(indexprm->minY, szMinY);
          x_strcpy(indexprm->maxY, szMaxY);
       }
       //--- get the index columns from iicolumns catlog  
       //--- reject the columns that are not part of the table such as
       //--- tidp.
             
       indexprm->lpidxCols=NULL;
       exec sql repeated select   a.column_name, a.column_sequence 
                into     :szColumnName, :nKeySequence
                from     iicolumns a
                where    table_name=:szIndexName       
                 and     table_owner = :lpIndexOwner   
                 and     exists(  select   *  
                                  from     iicolumns b
                                  where    b.table_name  = :szTableName
                                  and      b.table_owner = :szTableOwner
                                  and      b.column_name = a.column_name)
                order by column_sequence desc; 
       exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          lpTemp = AddListObject(indexprm->lpidxCols, sizeof(IDXCOLS));
          if (!lpTemp) {   // Need to free previously allocated objects.
             FreeAttachedPointers(indexprm, OT_INDEX);
             iret=RES_ERR;
             exec sql endselect;
          }
          else
             indexprm->lpidxCols = lpTemp;

          lpObj = (LPIDXCOLS)indexprm->lpidxCols->lpObject;

          suppspace(szColumnName);
          x_strcpy(lpObj->szColName, szColumnName);
       exec sql end;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       iret=SQLCodeToDom(sqlca.sqlcode);
       if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
          FreeAttachedPointers(indexprm, OT_INDEX);
          return iret;
       }

       // scan all index columns to determine if they are part of the Key

       lpTemp=indexprm->lpidxCols;
       while (lpTemp) {

          lpObj = (LPIDXCOLS)lpTemp->lpObject;
          x_strcpy(szColumnName, lpObj->szColName);
          exec sql repeated select   key_sequence
                   into     :nKeySequence
                   from     iiindex_columns
                   where    index_name  = :szIndexName 
                      and   column_name = :szColumnName
                      and   index_owner = :lpIndexOwner;

          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
             FreeAttachedPointers(indexprm, OT_INDEX);
             return iret;
          }
          if (sqlca.sqlcode==0)   // if found column is a key field
             lpObj->attr.bKey=TRUE;
          else 
             lpObj->attr.bKey=FALSE;
          lpTemp=lpTemp->lpNext;
       }

       iret = VDBA2xGetObjectCorruptionInfo (indexprm);
       if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) 
       {
          FreeAttachedPointers (indexprm, OT_INDEX);
          return iret;
       }

   return RES_SUCCESS;
   }
}

// the AddCentury2ExpireDate() is a temporary workaround to the fact that we don't want
// yet to use the _date4() ingres function (instead of _date(), because VDBA may connect to
// IngresII 2.0 builds that wouldn't have the patch yet

static void AddCentury2ExpireDate(LPUCHAR szExpire)
{
	char buftemp[40];
	char * pc1 = x_strchr(szExpire,'-');
	if (pc1!=NULL) {
		char * pc2 = x_strchr(pc1+1,'-');
		if (pc2!=NULL) {
			char * pc3=x_strstr(pc2,"   ");
			if (pc3==pc2+3) {
				char bufcentury[3];
				char bufyear[3];
				int year;
				fstrncpy(bufyear,pc2+1,3);// fstrncpy(..n) specified to copy n-1 chars and adds the \0
				year = atoi(bufyear);
				if (year<69) // 39 would be sufficient... 1969 possible in the past..if timezone<initial-timezone
					x_strcpy(bufcentury,"20");
				else
					x_strcpy(bufcentury,"19");
				fstrncpy(buftemp,szExpire,2 + (pc2-(char *)szExpire));
				x_strcat(buftemp,bufcentury);
				x_strcat(buftemp,pc2+1);
				x_strcpy(szExpire,buftemp);
				return;
			}
		}
	}
	x_strcpy(szExpire,"Date Format Error");
	return;
}

int GetInfTable(LPTABLEPARAMS tableprm)

{

   // TO BE FINISHED label granularity and security audit not yet available in
   // catalog tables 

   LPIDXCOLS lpObj;
   LPOBJECTLIST lpList = NULL;
   LPOBJECTLIST lpTemp = NULL;
   LPCOLUMNPARAMS lpCol;
   LPCONSTRAINTPARAMS lpConst;

   int freebytes, iret;
   BOOL bGenericGateway = FALSE;
   BOOL bStarDatabase   = FALSE;
   BOOL bRegisteredAsLink = FALSE;

   BOOL bMustQueryColumns   = TRUE;
   BOOL bMustQueryRemainder = TRUE;

   exec sql begin declare section;
   unsigned char uzConstraintType[2];
   unsigned char szTableName[MAXOBJECTNAME];
   unsigned char *lpTableOwner=tableprm->szSchema;
   unsigned char szJournaling[MAXOBJECTNAME];
   unsigned char szLocation[MAXOBJECTNAME];
   unsigned char szDuplicates[MAXOBJECTNAME];
   unsigned char szColumnName[MAXOBJECTNAME];
   unsigned char szNullable[MAXOBJECTNAME];
   unsigned char szDefault[MAXOBJECTNAME];
   unsigned char szDefSpec[MAXCOLDEFAULTLEN];
   unsigned char szConstraintName[MAXOBJECTNAME];
   unsigned char szText[SEGMENT_TEXT_SIZE];
   int iTextSequence;
   unsigned char szMultiLocation[MAXOBJECTNAME];
   unsigned char szStructure[MAXOBJECTNAME];
   unsigned char szUnique[MAXOBJECTNAME];   /* Added Emb+Vut Sep.01, 95 */
   unsigned char szLabelGranul[2];
   unsigned char szSecurAudit[2];
   unsigned char szIndexName[MAXOBJECTNAME];
   unsigned char buf01[MAXOBJECTNAME];
   unsigned char buf02[MAXOBJECTNAME];
   unsigned char buf03[MAXOBJECTNAME];
   unsigned char szSystemMaintained[STEPSMALLOBJ];

   char isCompressed [10];
   char key_isCompressed [10];

   short sNullInd;
   short sNullStat;
   int iSeq;
   int iDataType;
   int ilocseq;
   int iDataLen; 
   int nColSequence;
   int iScale;
   long nNumRows;
   int nFillFactor;
   long nMinPages;
   long nMaxPages;
   int nLeafFill;
   int nNonLeafFill;
   long lAllocation;
   long lExtend;
   int nPrimary;
   long lPage_size;
   long lAllocatedPages;
   int  iloc1;
   char tchszInternalDataType [32];
   char tchszDataType [32];
   long nExpireDate;
   char szExpireDate[32];
   char szIsReadonly[33];


   /* for table on a ddb */
   char szSubType[9];
   char szLdbObjName[33];
   char szLdbObjOwner[33];
   char szLdbDatabase[33];
   char szLdbNode[33];
   char szLdbDbmsType[33];
   unsigned char szLocalColumnName[MAXOBJECTNAME];

   exec sql end declare section; 

   struct StorageStruct {
      LPUCHAR lpVal;
      int nVal;
   } StorageStructKnown[] = {
      {"btree",    IDX_BTREE },
      {"BTREE",    IDX_BTREE },
      {"isam",     IDX_ISAM  },
      {"ISAM",     IDX_ISAM  },
      {"hash",     IDX_HASH  },
      {"HASH",     IDX_HASH  },
      {"heap",     IDX_HEAP  },
      {"HEAP",     IDX_HEAP  },
      {"heapsort", IDX_HEAPSORT  },
      {"HEAPSORT", IDX_HEAPSORT  },
      {"vectorwise", IDX_VW  },
      {"VECTORWISE", IDX_VW  },
      {"vectorwise_ix", IDX_VWIX },
      {"VECTORWISE_IX", IDX_VWIX },
      {"\0", 0}};
   struct StorageStruct *lpStorageStruct;

   // Prepare optimizations in terms of sql queries duration
   switch (tableprm->detailSubset) {
     case TABLE_SUBSET_NOCOLUMNS:
       bMustQueryColumns = FALSE;
       bMustQueryRemainder = TRUE;
       break;
     case TABLE_SUBSET_COLUMNSONLY:
       bMustQueryColumns = TRUE;
       bMustQueryRemainder = FALSE;
       break;
     case TABLE_SUBSET_ALL:
       bMustQueryColumns = TRUE;
       bMustQueryRemainder = TRUE;
       break;
     default:
       assert (FALSE);
       bMustQueryColumns = TRUE;
       bMustQueryRemainder = TRUE;
   }

   fstrncpy(tableprm->StorageParams.szSchema, tableprm->szSchema,
            MAXOBJECTNAME);
   tableprm->StorageParams.nStructure=OT_TABLE;
   tableprm->StorageParams.nObjectType=0;

   x_strcpy(szTableName, tableprm->objectname);

   szLabelGranul[0]='\0';
   szSecurAudit[0] ='\0';

   tableprm->bPrimKey=FALSE;
   tableprm->bUflag=FALSE;
   tableprm->UniqueSecIndexName[0]='\0';
   tableprm->bReadOnly=FALSE;

   if (GetOIVers() == OIVERS_NOTOI)
      bGenericGateway = TRUE;
   else {
     // cannot use tableprm->bDDB, need to call IsStarDatabase()
     int nodeHandle = GetVirtNodeHandle(tableprm->szNodeName);
     if (nodeHandle >= 0)
       if (IsStarDatabase(nodeHandle, tableprm->DBName))
         bStarDatabase = TRUE;
   }

   // PRELIMINARY DATA PICK FOR STAR DATABASE TABLE - ALWAYS DONE WHATEVER DETAIL SUBSET IS
   if (bStarDatabase) {
     // Specific for star table when registered as link
    ZEROINIT (szSubType);
    ZEROINIT (szLdbObjName);
    ZEROINIT (szLdbObjOwner);
    ZEROINIT (szLdbDatabase);
    ZEROINIT (szLdbNode);
    ZEROINIT (szLdbDbmsType);

    // 1) Check whether Registered as link
    exec sql repeated select table_subtype
             into :szSubType
             from iitables
             where table_name = :szTableName and table_owner = :lpTableOwner
               and table_type='T';
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    iret=SQLCodeToDom(sqlca.sqlcode);
    if (iret!=RES_SUCCESS)
      return iret;
    if (szSubType[0]=='L' || szSubType[0]=='l')
      bRegisteredAsLink = TRUE;

    // 2)  pick ldb object information from different system tables according to type (native/link)
    if (bRegisteredAsLink) {
      exec sql repeated select ldb_object_name, ldb_object_owner, ldb_database,
                      ldb_node,        ldb_dbmstype
               into   :szLdbObjName,   :szLdbObjOwner,   :szLdbDatabase,
                      :szLdbNode,      :szLdbDbmsType
               from   iiregistered_objects
               where  ddb_object_name = :szTableName and ddb_object_owner = :lpTableOwner;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      iret=SQLCodeToDom(sqlca.sqlcode);
      if (iret!=RES_SUCCESS)
        return iret;

      suppspace(szLdbObjName);
      suppspace(szLdbObjOwner);
      suppspace(szLdbDatabase);
      suppspace(szLdbNode);
      suppspace(szLdbDbmsType);

      x_strcpy(tableprm->szLdbObjName,  szLdbObjName);
      x_strcpy(tableprm->szLdbObjOwner, szLdbObjOwner);
      x_strcpy(tableprm->szLdbDatabase, szLdbDatabase);
      x_strcpy(tableprm->szLdbNode,     szLdbNode);
      x_strcpy(tableprm->szLdbDbmsType, szLdbDbmsType);
    }
    else {
      // Native: pick from several tables with 2 joints
      exec sql repeated select table_name, table_owner, ldb_database,
                      ldb_node, ldb_dbms
               into   :szLdbObjName,   :szLdbObjOwner,   :szLdbDatabase,
                      :szLdbNode,      :szLdbDbmsType
               from   iiddb_objects a, iiddb_tableinfo b, iiddb_ldbids c
               where  object_name = :szTableName and object_owner = :lpTableOwner
                      and b.object_base = a.object_base and c.ldb_id = b.ldb_id;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      iret=SQLCodeToDom(sqlca.sqlcode);
      if (iret!=RES_SUCCESS)
        return iret;

      suppspace(szLdbObjName);
      suppspace(szLdbObjOwner);
      suppspace(szLdbDatabase);
      suppspace(szLdbNode);
      suppspace(szLdbDbmsType);

      x_strcpy(tableprm->szLdbObjName,  szLdbObjName);
      x_strcpy(tableprm->szLdbObjOwner, szLdbObjOwner);
      x_strcpy(tableprm->szLdbDatabase, szLdbDatabase);
      x_strcpy(tableprm->szLdbNode,     szLdbNode);
      x_strcpy(tableprm->szLdbDbmsType, szLdbDbmsType);
    }

   }  // end of if (bStarDatabase)

   // Almost common code for generic gateway and star object,
   // with differences in columns information, especially if registered as link table
   if (bGenericGateway || bStarDatabase) {
       ZEROINIT( key_isCompressed );
       ZEROINIT( szUnique );
       ZEROINIT( szLabelGranul );
       ZEROINIT( szSecurAudit  );
       lAllocation = -1;
       lExtend     = -1;
       lPage_size  = -1;
	   lAllocatedPages = -1;

      if (bMustQueryRemainder) {

	      if (bGenericGateway) {
			   x_strcpy(szExpireDate,"N/A");
			   exec sql repeated select    is_journalled       , duplicate_rows    , multi_locations   , table_dfillpct,
							     table_minpages      , table_maxpages    , table_lfillpct    , table_ifillpct,
							     storage_structure   , location_name     , num_rows          , table_indexes,
							     unique_rule         , is_compressed     , expire_date       
						   into  :szJournaling   , :szDuplicates     , :szMultiLocation  , :nFillFactor,
							     :nMinPages      , :nMaxPages        , :nLeafFill        , :nNonLeafFill,
							     :szStructure    , :szLocation       , :nNumRows         , :buf02,
							     :buf03          , :isCompressed     , :nExpireDate      
						   from  iitables
						   where table_name= :szTableName and table_owner=:lpTableOwner;
		   }
		   else{
			   exec sql repeated select    is_journalled       , duplicate_rows    , multi_locations   , table_dfillpct,
							     table_minpages      , table_maxpages    , table_lfillpct    , table_ifillpct,
							     storage_structure   , location_name     , num_rows          , table_indexes,
							     unique_rule         , is_compressed     , expire_date       , char(_date(expire_date))+char('   ')+char(_time(expire_date))
						   into  :szJournaling   , :szDuplicates     , :szMultiLocation  , :nFillFactor,
							     :nMinPages      , :nMaxPages        , :nLeafFill        , :nNonLeafFill,
							     :szStructure    , :szLocation       , :nNumRows         , :buf02,
							     :buf03          , :isCompressed     , :nExpireDate      , :szExpireDate
						   from  iitables
						   where table_name= :szTableName and table_owner=:lpTableOwner;
		   }

          if (buf03[0]=='u' || buf03[0]=='U')
              tableprm->bUflag=TRUE;
          else 
              tableprm->bUflag=FALSE;

          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS)
             return iret;

         // expiration date
          if (nExpireDate) {
            tableprm->bExpireDate  = TRUE;
            tableprm->lExpireDate  = nExpireDate;
            x_strcpy (tableprm->szExpireDate, szExpireDate);
			AddCentury2ExpireDate(tableprm->szExpireDate);
          }
          else {
            tableprm->bExpireDate  = FALSE;
            tableprm->lExpireDate  = nExpireDate;
            x_strcpy (tableprm->szExpireDate, "");
          }

          tableprm->StorageParams.bDataCompression  = (isCompressed[0]=='Y'||isCompressed[0]=='y')? TRUE: FALSE;
          tableprm->StorageParams.bHiDataCompression= (isCompressed[0]=='H'||isCompressed[0]=='h')? TRUE: FALSE;
          tableprm->StorageParams.bKeyCompression  = (key_isCompressed[0]=='Y')? TRUE: FALSE;
          tableprm->StorageParams.nFillFactor=nFillFactor;
          tableprm->StorageParams.nMinPages=nMinPages;
          tableprm->StorageParams.nMaxPages=nMaxPages;
          tableprm->StorageParams.nLeafFill=nLeafFill;
          tableprm->StorageParams.nNonLeafFill=nNonLeafFill;
          tableprm->StorageParams.lAllocation=lAllocation;
          tableprm->StorageParams.lExtend=lExtend;
          tableprm->StorageParams.nNumRows=nNumRows;
	      tableprm->StorageParams.lAllocatedPages = -1;	/* allocated_pages column does not exist for star database */

          suppspace(szLocation);
	      if ( szLocation[0] )
	      {
		      // Get the first location 
		      lpTemp=AddListObject(tableprm->StorageParams.lpLocations,MAXOBJECTNAME);
		      if (!lpTemp) 
			     return RES_ERR;
		      tableprm->StorageParams.lpLocations=lpTemp;
		      lpTemp=AddListObject(tableprm->StorageParams.lpNewLocations,MAXOBJECTNAME);
		      if (!lpTemp) 
			     return RES_ERR;
		      tableprm->StorageParams.lpNewLocations=lpTemp;
		      suppspace(szLocation);
		      fstrncpy((LPUCHAR)tableprm->StorageParams.lpLocations->lpObject,
			     szLocation, MAXOBJECTNAME);
		      fstrncpy((LPUCHAR)tableprm->StorageParams.lpNewLocations->lpObject,
			     szLocation, MAXOBJECTNAME);
       
		      // TO BE FINISHED : the lplocation pointers are duplicate for MODIFY.
		      tableprm->lpLocations=tableprm->StorageParams.lpLocations;
          }
          // Convert the storage structure string into the internal integer
          suppspace(szStructure);
          tableprm->StorageParams.nStructure = IDX_STORAGE_STRUCT_UNKNOWN;
          if (x_strlen(szStructure) >0) {
              lpStorageStruct=&StorageStructKnown[0];
              while (*(lpStorageStruct->lpVal)) {
                 if (x_strcmp(lpStorageStruct->lpVal, szStructure)==0) {
                    tableprm->StorageParams.nStructure=lpStorageStruct->nVal;
                    break;
                 }
                 lpStorageStruct++;
              }
          }
          // columns unique_scope not filled with the gateway
          tableprm->StorageParams.nUnique=IDX_UNIQUE_UNKNOWN;
          tableprm->uPage_size = lPage_size;
          // TO BE FINISHED : supress duplicate due to MODIFY
          fstrncpy(tableprm->StorageParams.objectname, tableprm->objectname,
                   MAXOBJECTNAME);
          fstrncpy(tableprm->StorageParams.objectowner, tableprm->szSchema,
                   MAXOBJECTNAME);
          fstrncpy(tableprm->StorageParams.DBName, tableprm->DBName,
                   MAXOBJECTNAME);
          switch (szJournaling[0]) {
             case 'C' : case 'c' :
                tableprm->bJournaling=TRUE;
                tableprm->cJournaling = 'C';
                break;
             case 'Y' : case 'y' :
                tableprm->bJournaling=TRUE;
                tableprm->cJournaling = 'Y';
                break;
             case 'N' : case 'n' :           
                tableprm->bJournaling=FALSE;
                tableprm->cJournaling = 'N';
                break;
          }
          if (szDuplicates[0]=='D')
             tableprm->bDuplicates=TRUE;
          else
             tableprm->bDuplicates=FALSE;

          tableprm->nLabelGran = LABEL_GRAN_UNKOWN;
          tableprm->nSecAudit = SECURITY_AUDIT_UNKOWN;

       } // END OF if (bMustQueryRemainder)

       // --- get the table columns --- //
       // difference for star 'registered as link' table
       tableprm->lpColumns=NULL;
       iret=RES_SUCCESS;
       ZEROINIT(tchszInternalDataType);
       ZEROINIT(szDefault);
       ZEROINIT(szDefSpec);
       sNullInd = -1;

       if (bMustQueryColumns) {

          // if Generic or star native : almost same code
          if (bGenericGateway || ( bStarDatabase && !bRegisteredAsLink)) {
            exec sql repeated select   column_name    , column_sequence   , column_ingdatatype,
                              column_length  , column_scale      , key_sequence,
                              column_nulls   , column_datatype   , column_defaults
                     into     :szColumnName  , :nColSequence     , :iDataType,
                              :iDataLen      , :iScale           , :nPrimary,
                              :szNullable    , :tchszDataType    , :szDefault
                     from     iicolumns
                     where    table_name=:szTableName
                      and     table_owner=:lpTableOwner
                     order by column_sequence desc; 
            exec sql begin;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
               lpTemp = AddListObject(tableprm->StorageParams.lpidxCols,
                        sizeof(IDXCOLS));
               if (!lpTemp) {   // Need to free previously allocated objects.
                  FreeAttachedPointers(tableprm, OT_TABLE);
                  iret=RES_ERR;
                  exec sql endselect;
               }
               else
                  tableprm->StorageParams.lpidxCols = lpTemp;

               lpObj = (LPIDXCOLS)tableprm->StorageParams.lpidxCols->lpObject;

               suppspace(szColumnName);
               suppspace(tchszDataType);
               suppspace(tchszInternalDataType);
               x_strcpy(lpObj->szColName, szColumnName);
               lpObj->attr.nPrimary = nPrimary;

               if (nPrimary==0)
                  lpObj->attr.bKey=FALSE;
               else {
                  tableprm->bPrimKey=TRUE;
                  lpObj->attr.bKey=TRUE;
               }

               lpTemp = AddListObject(tableprm->lpColumns, sizeof(COLUMNPARAMS));
               if (!lpTemp) {   // Need to free previously allocated objects.
                  FreeAttachedPointers(tableprm, OT_TABLE);
                  iret=RES_ERR;
                  exec sql endselect;
               }
               else
                  tableprm->lpColumns = lpTemp;

               lpCol = (LPCOLUMNPARAMS)tableprm->lpColumns->lpObject;
               x_strcpy(lpCol->szColumn, szColumnName);

               // for star table: local column name same as column name
               if (bStarDatabase)
                 x_strcpy(lpCol->szLdbColumn, lpCol->szColumn);
               else
                 lpCol->szLdbColumn[0] = '\0';

               // tchszInternalDataType is not fill by the select statement.
               // copy tchszDataType in tchszInternalDataType,
               // for used the display code identical with the gateway or without.
               x_strcpy (lpCol->tchszInternalDataType,tchszDataType );
               x_strcpy (lpCol->tchszDataType, tchszDataType);

               if (iDataType == -1)  // columns type not filled by the select statement
                  iDataType = GatewayToIngType (tchszDataType);

               if (iDataType<=0)  // if negative, nullable useless here
                  iDataType*=-1;
               lpCol->uDataType=IngToDbaType(iDataType, iDataLen);
               if (iDataType==IISQ_DEC_TYPE) {
                  lpCol->lenInfo.decInfo.nPrecision=iDataLen;
                  lpCol->lenInfo.decInfo.nScale=iScale;
               }
               else
                  lpCol->lenInfo.dwDataLen=iDataLen;
         /*UK26JUNE1997, Today the key_sequence is set if the column is part of primary or unique key
                         So it not correct to consider it is a primary key. !!!
               lpCol->nPrimary=nPrimary;
         */
               lpCol->nPrimary=0;

               //
               // lpCol->nKeySequence indicates if the column is part of primary key or unique key, if so
               // its value is the column key sequence.
               lpCol->nKeySequence = nPrimary;

               if (tableprm->bUflag)
                 lpCol->nSecondary=nPrimary;
               else 
                 lpCol->nSecondary=0;

               if (szNullable[0]=='Y')
                  lpCol->bNullable=TRUE;
               else
                  lpCol->bNullable=FALSE;

               // Subtle Default field management: ignored for gateway, but useful for ddb table
               if (bGenericGateway)
                 lpCol->bDefault = FALSE;
               else
                 if (szDefault[0]=='Y')
                    lpCol->bDefault=TRUE;
                 else
                    lpCol->bDefault=FALSE;

               if (sNullInd==0) {  // column_has_default is significant
                  lpCol->lpszDefSpec=ESL_AllocMem(x_strlen(szDefSpec)+1);
                  if (!lpCol->lpszDefSpec) {
                     FreeAttachedPointers(tableprm, OT_TABLE);
                     iret=RES_ERR;
                     exec sql endselect;
                  }
                  x_strcpy(lpCol->lpszDefSpec, szDefSpec);
               }
               else
                  lpCol->lpszDefSpec=NULL;
            exec sql end;
          }    // end of if (bGenericGateway || ( bStarDatabase && !bRegisteredAsLink)
          else {
            // Special management for registered as link
            // Nb: cannot use "order by a.column_sequence", said to be ambiguous at run-time
            exec sql repeated select   a.column_name    , a.column_sequence   , a.column_ingdatatype,
                              a.column_length  , a.column_scale      , a.key_sequence,
                              a.column_nulls   , a.column_datatype   , a.column_defaults,
                              c.local_column
                     into     :szColumnName  , :nColSequence     , :iDataType,
                              :iDataLen      , :iScale           , :nPrimary,
                              :szNullable    , :tchszDataType    , :szDefault,
                              :szLocalColumnName
                     from     iicolumns a, iiddb_objects b, iiddb_ldb_columns c
                     where    b.object_name = a.table_name and b.object_owner = a.table_owner
                              and c.object_base = b.object_base
                              and c.column_sequence = a.column_sequence
                              and c.object_index = 0
                              and a.table_name=:szTableName and a.table_owner=:lpTableOwner
                     union
                     select   a.column_name    , a.column_sequence   , a.column_ingdatatype,
                              a.column_length  , a.column_scale      , a.key_sequence,
                              a.column_nulls   , a.column_datatype   , a.column_defaults,
                              a.column_name
                     from     iicolumns a , iiddb_objects b
                     where    b.object_name = a.table_name and b.object_owner = a.table_owner
                              and a.table_name=:szTableName and a.table_owner=:lpTableOwner
                              and not exists (
                                select * from iicolumns a , iiddb_objects b, iiddb_ldb_columns c
                                where  b.object_name = a.table_name and b.object_owner = a.table_owner
                                       and c.object_base = b.object_base
                                       and c.column_sequence = a.column_sequence
                                       and c.object_index = 0
                                       and a.table_name=:szTableName and a.table_owner=:lpTableOwner
                              )
                     order by 2 desc;
            exec sql begin;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
               lpTemp = AddListObject(tableprm->StorageParams.lpidxCols,
                        sizeof(IDXCOLS));
               if (!lpTemp) {   // Need to free previously allocated objects.
                  FreeAttachedPointers(tableprm, OT_TABLE);
                  iret=RES_ERR;
                  exec sql endselect;
               }
               else
                  tableprm->StorageParams.lpidxCols = lpTemp;

               lpObj = (LPIDXCOLS)tableprm->StorageParams.lpidxCols->lpObject;

               suppspace(szColumnName);
               suppspace(tchszDataType);
               suppspace(tchszInternalDataType);
               x_strcpy(lpObj->szColName, szColumnName);
               lpObj->attr.nPrimary = nPrimary;

               if (nPrimary==0)
                  lpObj->attr.bKey=FALSE;
               else {
                  tableprm->bPrimKey=TRUE;
                  lpObj->attr.bKey=TRUE;
               }

               lpTemp = AddListObject(tableprm->lpColumns, sizeof(COLUMNPARAMS));
               if (!lpTemp) {   // Need to free previously allocated objects.
                  FreeAttachedPointers(tableprm, OT_TABLE);
                  iret=RES_ERR;
                  exec sql endselect;
               }
               else
                  tableprm->lpColumns = lpTemp;

               lpCol = (LPCOLUMNPARAMS)tableprm->lpColumns->lpObject;
               x_strcpy(lpCol->szColumn, szColumnName);

               suppspace(szLocalColumnName);
               x_strcpy(lpCol->szLdbColumn, szLocalColumnName);

               // tchszInternalDataType is not fill by the select statement.
               // copy tchszDataType in tchszInternalDataType,
               // for used the display code identical with the gateway or without.
               x_strcpy (lpCol->tchszInternalDataType,tchszDataType );
               x_strcpy (lpCol->tchszDataType, tchszDataType);

               if (iDataType == -1)  // columns type not filled by the select statement
                  iDataType = GatewayToIngType (tchszDataType);

               if (iDataType<=0)  // if negative, nullable useless here
                  iDataType*=-1;
               lpCol->uDataType=IngToDbaType(iDataType, iDataLen);
               if (iDataType==IISQ_DEC_TYPE) {
                  lpCol->lenInfo.decInfo.nPrecision=iDataLen;
                  lpCol->lenInfo.decInfo.nScale=iScale;
               }
               else
                  lpCol->lenInfo.dwDataLen=iDataLen;
         /*UK26JUNE1997, Today the key_sequence is set if the column is part of primary or unique key
                         So it not correct to consider it is a primary key. !!!
               lpCol->nPrimary=nPrimary;
         */
               lpCol->nPrimary=0;
               if (tableprm->bUflag)
                 lpCol->nSecondary=nPrimary;
               else 
                 lpCol->nSecondary=0;

               if (szNullable[0]=='Y')
                  lpCol->bNullable=TRUE;
               else
                  lpCol->bNullable=FALSE;

               // Subtle Default field management: ignored for gateway, but useful for ddb table
               if (bGenericGateway)
                 lpCol->bDefault = FALSE;
               else
                 if (szDefault[0]=='Y')
                    lpCol->bDefault=TRUE;
                 else
                    lpCol->bDefault=FALSE;

               if (sNullInd==0) {  // column_has_default is significant
                  lpCol->lpszDefSpec=ESL_AllocMem(x_strlen(szDefSpec)+1);
                  if (!lpCol->lpszDefSpec) {
                     FreeAttachedPointers(tableprm, OT_TABLE);
                     iret=RES_ERR;
                     exec sql endselect;
                  }
                  x_strcpy(lpCol->lpszDefSpec, szDefSpec);
               }
               else
                  lpCol->lpszDefSpec=NULL;
            exec sql end;
          }
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          if (iret!=RES_SUCCESS) {
             FreeAttachedPointers(tableprm, OT_TABLE);
             return iret;
          }
          if (!tableprm->bUflag && (buf02[0]=='Y' || buf02[0]=='y')) { //search for secondary index
                   EXEC SQL repeated SELECT count(*)
                       INTO    :iloc1
                       FROM    iiindexes
                       WHERE   base_name = :szTableName
                       AND base_owner = :lpTableOwner
                       AND unique_rule = 'U';
               if (sqlca.sqlcode) 
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
               if (sqlca.sqlcode==0 && iloc1>0) {
                     EXEC SQL repeated SELECT MIN(index_name)
                       INTO    :szIndexName:sNullInd
                         FROM  iiindexes
                       WHERE   base_name = :szTableName
                       AND base_owner = :lpTableOwner
                       AND unique_rule = 'U';
                 if (sqlca.sqlcode) 
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
               else {
                           if (sNullInd != -1) {
                   suppspace(szIndexName);
                   fstrncpy(tableprm->UniqueSecIndexName,szIndexName,
                            sizeof(tableprm->UniqueSecIndexName));
                 }
               }
             }
             if (tableprm->UniqueSecIndexName[0]!='\0') {
                 LPOBJECTLIST  ls;
                 LPCOLUMNPARAMS lpCol;
                 EXEC SQL repeated select c.column_name,c.key_sequence
                     INTO :buf01, :iloc1
                     FROM iiindexes i, iicolumns c
                    WHERE c.table_name = i.index_name
                      AND c.table_owner =i.index_owner
                      AND i.index_name = :szIndexName;
                 exec sql begin;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
                 suppspace(buf01);
                 ls = tableprm->lpColumns;
                 while (ls) {
                    lpCol=(LPCOLUMNPARAMS)ls->lpObject; 
                    if (!x_stricmp(lpCol->szColumn,buf01)) {
                       lpCol->nSecondary=iloc1;
                       break;
                    }
                    ls=ls->lpNext;
                 }
                 exec sql end;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
          }
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
             FreeAttachedPointers(tableprm, OT_TABLE);
             return iret;
          }
       } // end of if (bMustQueryColumns)
       return RES_SUCCESS;
    } // end of if (bGenericGateway || bStarDatabase)

    else {
       // not a gateway and not a distributed table
		int nIngresVersion = GetOIVers();
       if (bMustQueryRemainder) {
          if (nIngresVersion == OIVERS_12)
          {
             exec sql repeated select   is_journalled, duplicate_rows,
                            multi_locations, table_dfillpct, table_minpages,
                            table_maxpages, table_lfillpct, table_ifillpct, 
                            allocation_size,extend_size, storage_structure,
                            location_name, num_rows, unique_scope,
                            label_granularity , row_security_audit,
                                table_indexes,unique_rule,
                            is_compressed, key_is_compressed,
						    allocated_pages

                   into     :szJournaling, :szDuplicates,
                            :szMultiLocation,:nFillFactor, :nMinPages,
                            :nMaxPages,:nLeafFill, :nNonLeafFill, 
                            :lAllocation,:lExtend, :szStructure,
                            :szLocation, :nNumRows, :szUnique,
                            :szLabelGranul,:szSecurAudit,
                                :buf02,:buf03,
                            :isCompressed,:key_isCompressed,
						    :lAllocatedPages
                   from     iitables
                   where    table_name= :szTableName
                    and     table_owner=:lpTableOwner;
          }
          else
          if (nIngresVersion <= OIVERS_26)
          {
             exec sql repeated select
                       is_journalled, 
                       duplicate_rows,
                       multi_locations, 
                       table_dfillpct, 
                       table_minpages,
                       table_maxpages, 
                       table_lfillpct, 
                       table_ifillpct, 
                       allocation_size,
                       extend_size, 
                       storage_structure,
                       location_name, 
                       num_rows, 
                       unique_scope,
                       table_pagesize,
                       label_granularity,
                       row_security_audit,
                       table_indexes,
                       unique_rule,
                       is_compressed,
                       key_is_compressed,
                       allocated_pages,
                       expire_date,
					   char(_date(expire_date))+char('   ')+char(_time(expire_date)),
					   is_readonly
                   into     
                       :szJournaling, 
                       :szDuplicates,
                       :szMultiLocation,
                       :nFillFactor, 
                       :nMinPages,
                       :nMaxPages,
                       :nLeafFill, 
                       :nNonLeafFill, 
                       :lAllocation,
                       :lExtend, 
                       :szStructure,
                       :szLocation, 
                       :nNumRows, 
                       :szUnique,
                       :lPage_size,
                       :szLabelGranul,
                       :szSecurAudit,
                       :buf02,
                       :buf03,
                       :isCompressed,
                       :key_isCompressed,
                       :lAllocatedPages,
                       :nExpireDate,
                       :szExpireDate,
					   :szIsReadonly

                   from  iitables
                   where table_name= :szTableName and table_owner=:lpTableOwner;
          }
          else /* nIngresVersion >= OIVERS_30 */
          {
             exec sql repeated select
                       is_journalled, 
                       duplicate_rows,
                       multi_locations, 
                       table_dfillpct, 
                       table_minpages,
                       table_maxpages, 
                       table_lfillpct, 
                       table_ifillpct, 
                       allocation_size,
                       extend_size, 
                       storage_structure,
                       location_name, 
                       num_rows, 
                       unique_scope,
                       table_pagesize,
                       row_security_audit,
                       table_indexes,
                       unique_rule,
                       is_compressed,
                       key_is_compressed,
                       allocated_pages,
                       expire_date,
					   char(_date(expire_date))+char('   ')+char(_time(expire_date)),
					   is_readonly
                   into     
                       :szJournaling, 
                       :szDuplicates,
                       :szMultiLocation,
                       :nFillFactor, 
                       :nMinPages,
                       :nMaxPages,
                       :nLeafFill, 
                       :nNonLeafFill, 
                       :lAllocation,
                       :lExtend, 
                       :szStructure,
                       :szLocation, 
                       :nNumRows, 
                       :szUnique,
                       :lPage_size,
                       :szSecurAudit,
                       :buf02,
                       :buf03,
                       :isCompressed,
                       :key_isCompressed,
                       :lAllocatedPages,
                       :nExpireDate,
                       :szExpireDate,
					   :szIsReadonly

                   from  iitables
                   where table_name= :szTableName and table_owner=:lpTableOwner;          
          }

          if (szIsReadonly[0]=='Y' || szIsReadonly[0]=='y')
              tableprm->bReadOnly=TRUE;
		    else
              tableprm->bReadOnly=FALSE;
          if (buf03[0]=='u' || buf03[0]=='U')
              tableprm->bUflag=TRUE;
          else 
              tableprm->bUflag=FALSE;

          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS)
             return iret;

          tableprm->StorageParams.bDataCompression  = (isCompressed[0]=='Y'||isCompressed[0]=='y')? TRUE: FALSE;
          tableprm->StorageParams.bHiDataCompression= (isCompressed[0]=='H'||isCompressed[0]=='h')? TRUE: FALSE;
          tableprm->StorageParams.bKeyCompression  = (key_isCompressed[0]=='Y')? TRUE: FALSE;
          tableprm->StorageParams.nFillFactor=nFillFactor;
          tableprm->StorageParams.nMinPages=nMinPages;
          tableprm->StorageParams.nMaxPages=nMaxPages;
          tableprm->StorageParams.nLeafFill=nLeafFill;
          tableprm->StorageParams.nNonLeafFill=nNonLeafFill;
          tableprm->StorageParams.lAllocation=lAllocation;
          tableprm->StorageParams.lExtend=lExtend;
          tableprm->StorageParams.nNumRows=nNumRows;
	      tableprm->StorageParams.lAllocatedPages = lAllocatedPages;

         // expiration date
          if (nExpireDate) {
            tableprm->bExpireDate  = TRUE;
            tableprm->lExpireDate  = nExpireDate;
            x_strcpy (tableprm->szExpireDate, szExpireDate);
			AddCentury2ExpireDate(tableprm->szExpireDate);
          }
          else {
            tableprm->bExpireDate  = FALSE;
            tableprm->lExpireDate  = nExpireDate;
            x_strcpy (tableprm->szExpireDate, "");
          }

          // Get the first location 

          lpTemp=AddListObject(tableprm->StorageParams.lpLocations,
                MAXOBJECTNAME);
          if (!lpTemp) 
             return RES_ERR;
          tableprm->StorageParams.lpLocations=lpTemp;
          lpTemp=AddListObject(tableprm->StorageParams.lpNewLocations,
                MAXOBJECTNAME);
          if (!lpTemp) 
             return RES_ERR;
          tableprm->StorageParams.lpNewLocations=lpTemp;
          suppspace(szLocation);
          fstrncpy((LPUCHAR)tableprm->StorageParams.lpLocations->lpObject,
             szLocation, MAXOBJECTNAME);
          fstrncpy((LPUCHAR)tableprm->StorageParams.lpNewLocations->lpObject,
             szLocation, MAXOBJECTNAME);

          //--- if the table is multi location get all the locations from   ---
          //--- iimulti_location catalog table                              --- 

          if (szMultiLocation[0]=='Y') {      
             iret=RES_SUCCESS;
             exec sql repeated select   location_name, loc_sequence 
                      into     :szLocation, :ilocseq
                      from     iimulti_locations
                      where    table_name=:szTableName
                       and     table_owner=:lpTableOwner
                      order by loc_sequence desc;
             exec sql begin;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
                suppspace(szLocation);
                if (*szLocation) {
                   lpTemp=AddListObject(tableprm->StorageParams.lpLocations,
                         MAXOBJECTNAME);
                   if (!lpTemp) {
                      iret=RES_ERR;
                      exec sql endselect;
                   }
                   tableprm->StorageParams.lpLocations=lpTemp;
                   fstrncpy((LPUCHAR)tableprm->StorageParams.lpLocations->lpObject,
                      szLocation, MAXOBJECTNAME);
                   lpTemp=AddListObject(tableprm->StorageParams.lpNewLocations,
                         MAXOBJECTNAME);
                   if (!lpTemp) {
                      iret=RES_ERR;
                      exec sql endselect;
                   }
                   tableprm->StorageParams.lpNewLocations=lpTemp;
                   fstrncpy((LPUCHAR)tableprm->StorageParams.lpNewLocations->lpObject,
                      szLocation, MAXOBJECTNAME);
                }
             exec sql end;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             if (iret==RES_ERR) { // allocation problem
                FreeAttachedPointers(tableprm, OT_TABLE);
                myerror(ERR_ALLOCMEM);
                return RES_ERR;
             }
             iret=SQLCodeToDom(sqlca.sqlcode);
             if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
                FreeAttachedPointers(tableprm, OT_TABLE);
                return iret;
             }
          }

          // TO BE FINISHED : the lplocation pointers are duplicate for MODIFY.
          tableprm->lpLocations=tableprm->StorageParams.lpLocations;

          // Convert the storage structure string into the internal integer
          suppspace(szStructure);
          lpStorageStruct=&StorageStructKnown[0];
          while (*(lpStorageStruct->lpVal)) {
             if (x_strcmp(lpStorageStruct->lpVal, szStructure)==0) {
                tableprm->StorageParams.nStructure=lpStorageStruct->nVal;
                break;
             }
             lpStorageStruct++;
          }

         // Added Emb+Vut Sep.01, 95 : manage unique_scope field
          switch (szUnique[0]) {
             case 'R' :
                tableprm->StorageParams.nUnique=IDX_UNIQUE_ROW;
                break;
             case 'S' :
                tableprm->StorageParams.nUnique=IDX_UNIQUE_STATEMENT;
                break;
             default :
                tableprm->StorageParams.nUnique=IDX_UNIQUE_NO;
                break;
          }
          if (GetOIVers() != OIVERS_12)
          {
             tableprm->uPage_size = lPage_size;
          }

          // TO BE FINISHED : supress duplicate due to MODIFY
          fstrncpy(tableprm->StorageParams.objectname, tableprm->objectname,
                   MAXOBJECTNAME);
          fstrncpy(tableprm->StorageParams.objectowner, tableprm->szSchema,
                   MAXOBJECTNAME);
          fstrncpy(tableprm->StorageParams.DBName, tableprm->DBName,
                   MAXOBJECTNAME);
          switch (szJournaling[0]) {
             case 'C' : case 'c' :
                tableprm->bJournaling=TRUE;
                tableprm->cJournaling = 'C';
                break;
             case 'Y' : case 'y' :
                tableprm->bJournaling=TRUE;
                tableprm->cJournaling = 'Y';
                break;
             case 'N' : case 'n' :           
                tableprm->bJournaling=FALSE;
                tableprm->cJournaling = 'N';
                break;
          }
          if (szDuplicates[0]=='D')
             tableprm->bDuplicates=TRUE;
          else
             tableprm->bDuplicates=FALSE;
          if (nIngresVersion <= OIVERS_26)
          {
          // TO BE FINISHED for gran and security audit : pls replace from here... 
          if ( szLabelGranul[0] == 'T')
             tableprm->nLabelGran=LABEL_GRAN_TABLE;
          else  {
             if ( szLabelGranul[0] == 'R')
                tableprm->nLabelGran=LABEL_GRAN_ROW;
             else
                tableprm->nLabelGran=LABEL_GRAN_SYS_DEFAULTS;
          }
          }
          // Get security audit on row . 
          // The security audit on a table is always actived, it's a default.
          if (szSecurAudit[0] == 'Y')
             tableprm->nSecAudit = SECURITY_AUDIT_TABLEROW;
          else   {
             if (szSecurAudit[0] == 'N') 
                tableprm->nSecAudit = SECURITY_AUDIT_TABLENOROW;
             else
                tableprm->nSecAudit = SECURITY_AUDIT_TABLE;
          }
          // --- get the security audit key ---
          // szSecAuditKey
          iret=RES_SUCCESS;
          exec sql repeated select   column_name
                   into     :szColumnName
                   from     iicolumns
                   where    table_name=:szTableName
                    and     table_owner=:lpTableOwner
                    and     security_audit_key = 'Y'; 
          exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             suppspace(szColumnName);
             x_strcpy(tableprm->szSecAuditKey, szColumnName);
          exec sql end;

       } // end of if (bMustQueryRemainder)

       // --- get the table colunms --- //
       tableprm->lpColumns=NULL;
       iret=RES_SUCCESS;

       // Note : we have to get the table columns if remainder is requested, because of table constraints
       if (bMustQueryColumns || bMustQueryRemainder) {

          exec sql repeated select   a.column_name,    a.column_sequence,         a.column_ingdatatype, 
                            a.column_length,  a.column_scale,            a.key_sequence,
                            a.column_nulls,   a.column_has_default,      a.column_default_val,
                            a.column_datatype,a.column_internal_datatype,b.text_sequence, a.column_system_maintained
                   into     :szColumnName, :nColSequence, :iDataType,
                            :iDataLen, :iScale, :nPrimary,
                            :szNullable, :szDefault, :szDefSpec:sNullInd, 
                            :tchszDataType, :tchszInternalDataType, :iSeq:sNullStat, :szSystemMaintained
                   from     (iicolumns a left join iihistograms  b on 
				                      a.table_name=b.table_name     and
								      a.table_owner=b.table_owner   and 
								      a.column_name = b.column_name and 
								      b.text_sequence=1)
                   where    a.table_name=:szTableName
                    and     a.table_owner=:lpTableOwner
                   order by column_sequence desc; 
          exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             lpTemp = AddListObject(tableprm->StorageParams.lpidxCols,
                      sizeof(IDXCOLS));
             if (!lpTemp) {   // Need to free previously allocated objects.
                FreeAttachedPointers(tableprm, OT_TABLE);
                iret=RES_ERR;
                exec sql endselect;
             }
             else
                tableprm->StorageParams.lpidxCols = lpTemp;

             lpObj = (LPIDXCOLS)tableprm->StorageParams.lpidxCols->lpObject;

             suppspace(szColumnName);
             suppspace(tchszDataType);
             suppspace(tchszInternalDataType);
             x_strcpy(lpObj->szColName, szColumnName);
             lpObj->attr.nPrimary = nPrimary;

             if (nPrimary==0)
                lpObj->attr.bKey=FALSE;
             else {
                tableprm->bPrimKey=TRUE;
                lpObj->attr.bKey=TRUE;
             }

             lpTemp = AddListObject(tableprm->lpColumns, sizeof(COLUMNPARAMS));
             if (!lpTemp) {   // Need to free previously allocated objects.
                FreeAttachedPointers(tableprm, OT_TABLE);
                iret=RES_ERR;
                exec sql endselect;
             }
             else
                tableprm->lpColumns = lpTemp;

             lpCol = (LPCOLUMNPARAMS)tableprm->lpColumns->lpObject;
             x_strcpy (lpCol->szColumn, szColumnName);
             x_strcpy (lpCol->tchszInternalDataType, tchszInternalDataType);
             x_strcpy (lpCol->tchszDataType, tchszDataType);

		     if (sNullStat==0)
                lpCol->bHasStatistics=TRUE;
             else
		        lpCol->bHasStatistics=FALSE;
			    
                lpCol->lpszDefSpec=NULL;
             if (iDataType<=0)  // if negative, nullable useless here
                iDataType*=-1;
             lpCol->uDataType=IngToDbaType(iDataType, iDataLen);
             if (iDataType==IISQ_DEC_TYPE) {
                lpCol->lenInfo.decInfo.nPrecision=iDataLen;
                lpCol->lenInfo.decInfo.nScale=iScale;
             }
             else
                lpCol->lenInfo.dwDataLen=iDataLen;
       /*UK26JUNE1997, Today the key_sequence is set if the column is part of primary or unique key
                       So it not correct to consider it is a primary key. !!!
             lpCol->nPrimary=nPrimary;
       */
             lpCol->nPrimary=0;
             //
             // lpCol->nKeySequence indicates if the column is part of primary key or unique key, if so
             // its value is the column key sequence.
             lpCol->nKeySequence = nPrimary;

             if (tableprm->bUflag)
               lpCol->nSecondary=nPrimary;
             else 
               lpCol->nSecondary=0;

             if (szNullable[0]=='Y')
                lpCol->bNullable=TRUE;
             else
                lpCol->bNullable=FALSE;
             if (szDefault[0]=='Y')
                lpCol->bDefault=TRUE;
             else
                lpCol->bDefault=FALSE;
             if (sNullInd==0) {  // column_has_default is significant
                lpCol->lpszDefSpec=ESL_AllocMem(x_strlen(szDefSpec)+1);
                if (!lpCol->lpszDefSpec) {
                   FreeAttachedPointers(tableprm, OT_TABLE);
                   iret=RES_ERR;
                   exec sql endselect;
                }
                x_strcpy(lpCol->lpszDefSpec, szDefSpec);
             }
             else
                lpCol->lpszDefSpec=NULL;
             if ( szSystemMaintained[0]=='Y') 
                lpCol->bSystemMaintained = TRUE;
             else
                lpCol->bSystemMaintained = FALSE;
          exec sql end;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          if (iret!=RES_SUCCESS) {
             FreeAttachedPointers(tableprm, OT_TABLE);
             return iret;
          }
       //   if (GetOIVers() == OIVERS_12)
       //      tableprm->bUflag=tableprm->bPrimKey;

          if (!tableprm->bUflag && (buf02[0]=='Y' || buf02[0]=='y')) { //search for secondary index
                   EXEC SQL repeated SELECT count(*)
                       INTO    :iloc1
                       FROM    iiindexes
                       WHERE   base_name = :szTableName
                       AND base_owner = :lpTableOwner
                       AND unique_rule = 'U';
               if (sqlca.sqlcode) 
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
               if (sqlca.sqlcode==0 && iloc1>0) {
                     EXEC SQL repeated SELECT MIN(index_name)
                       INTO    :szIndexName:sNullInd
                         FROM  iiindexes
                       WHERE   base_name = :szTableName
                       AND base_owner = :lpTableOwner
                       AND unique_rule = 'U';
                 if (sqlca.sqlcode) 
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
               else {
                           if (sNullInd != -1) {
                   suppspace(szIndexName);
                   fstrncpy(tableprm->UniqueSecIndexName,szIndexName,
                            sizeof(tableprm->UniqueSecIndexName));
                 }
               }
             }
             if (tableprm->UniqueSecIndexName[0]!='\0') {
                 LPOBJECTLIST  ls;
                 LPCOLUMNPARAMS lpCol;
                 EXEC SQL repeated select c.column_name,c.key_sequence
                     INTO :buf01, :iloc1
                     FROM iiindexes i, iicolumns c
                    WHERE c.table_name = i.index_name
                      AND c.table_owner =i.index_owner
                      AND i.index_name = :szIndexName;
                 exec sql begin;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
                 suppspace(buf01);
                 ls = tableprm->lpColumns;
                 while (ls) {
                    lpCol=(LPCOLUMNPARAMS)ls->lpObject; 
                    if (!x_stricmp(lpCol->szColumn,buf01)) {
                       lpCol->nSecondary=iloc1;
                       break;
                    }
                    ls=ls->lpNext;
                 }
                 exec sql end;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
          }
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
             FreeAttachedPointers(tableprm, OT_TABLE);
             return iret;
          }

       } // end of if (bMustQueryColumns || bMustQueryRemainder)

       //--- Get the table constraints ---/  
       if (bMustQueryRemainder) {

          iret=RES_SUCCESS;
          exec sql repeated select   constraint_name, text_sequence, text_segment,
                            constraint_type
                   into     :szConstraintName, :iTextSequence, :szText,
                            :uzConstraintType
                   from     iiconstraints
                   where    table_name=:szTableName
                    and     schema_name=:lpTableOwner
                   order by constraint_type, constraint_name, text_sequence;

          exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             if (iTextSequence==1) {
                lpTemp=AddListObject(tableprm->lpConstraint, 
                   sizeof(CONSTRAINTPARAMS));
                if (!lpTemp) {   // Need to free previously allocated objects.
                   FreeAttachedPointers(tableprm, OT_TABLE);
                   iret=RES_ERR;
                   exec sql endselect;
                }
                tableprm->lpConstraint=lpTemp;
                lpConst = (LPCONSTRAINTPARAMS)lpTemp->lpObject;
                freebytes=SEGMENT_TEXT_SIZE;
                lpConst->lpText= ESL_AllocMem(SEGMENT_TEXT_SIZE);
                if (!lpConst->lpText) {
                   FreeAttachedPointers(tableprm, OT_TABLE);
                   iret=RES_ERR;
                   exec sql endselect;
                }
                lpConst->cConstraintType=uzConstraintType[0];
                suppspace (szConstraintName);
                lstrcpy (lpConst->tchszConstraint, szConstraintName);
             }
             lpConst->lpText=ConcatStr(lpConst->lpText, szText, &freebytes);
             if (!lpConst->lpText) {
                FreeAttachedPointers(tableprm, OT_TABLE);
                iret=RES_ERR;
                exec sql endselect;
             }
          exec sql end;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          if (iret!=RES_SUCCESS) {
             FreeAttachedPointers(tableprm, OT_TABLE);
             return iret;
          }
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA) {
             FreeAttachedPointers(tableprm, OT_TABLE);
             return iret;
          }
          //
          // Get the index enforcement for each constraint:
          lpList = tableprm->lpConstraint;
          while (lpList) 
          {
              LPCONSTRAINTPARAMS lpCnst = (LPCONSTRAINTPARAMS)lpList->lpObject;
              VDBA2xGetIndexEnforcement (lpCnst);
              lpList = (LPOBJECTLIST)lpList->lpNext;
          }
    

          iret = VDBA2xGetTablePrimaryKeyInfo (tableprm);
          if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) 
          {
             FreeAttachedPointers (tableprm, OT_TABLE);
             return iret;
          }
          {
             int ipos = 0;
             LPCOLUMNPARAMS lpcol;
             LPSTRINGLIST ls = tableprm->lpPrimaryColumns;
             while (ls)
             {
                 lpcol = VDBA20xTableColumn_Search (tableprm->lpColumns, ls->lpszString);
                 if (lpcol)
                 {
                     ipos++;
                     lpcol->nPrimary = ipos;
                 }
                 ls = ls->next;
             }
             tableprm->lpPrimaryColumns = StringList_Done (tableprm->lpPrimaryColumns);
          }
          iret = VDBA2xGetTableForeignKeyInfo (tableprm);
          if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) 
          {
             FreeAttachedPointers (tableprm, OT_TABLE);
             return iret;
          }
          iret = VDBA2xGetTableUniqueKeyInfo (tableprm);
          if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) 
          {
             FreeAttachedPointers (tableprm, OT_TABLE);
             return iret;
          }
          iret = VDBA2xGetTableCheckInfo (tableprm);
          if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) 
          {
             FreeAttachedPointers (tableprm, OT_TABLE);
             return iret;
          }
          iret = VDBA2xGetObjectCorruptionInfo (&(tableprm->StorageParams));
          if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
          {
             FreeAttachedPointers (tableprm, OT_TABLE);
             return iret;
          }

       } // end of if (bMustQueryRemainder)

       return RES_SUCCESS;
    } // end of bGenericGateway FALSE
}

int GetInfBaseExtend(LPDATABASEPARAMS lpbaseprm)
{
   EXEC SQL BEGIN DECLARE SECTION;
   int nbCat,iret;
   char szName[MAXOBJECTNAME];
   EXEC SQL END DECLARE SECTION;
   int i;
   char pCatName [MAX_TOOL_PRODUCTS+1][20];
   BOOL bGenericGateway = FALSE;

   lstrcpy(pCatName[TOOLS_INGRES],"INGRES");
   lstrcpy(pCatName[TOOLS_INGRESDBD],"INGRES/DBD");
   lstrcpy(pCatName[TOOLS_VISION],"VISION");
   lstrcpy(pCatName[TOOLS_W4GL],"WINDOWS_4GL");
   lstrcpy(pCatName[MAX_TOOL_PRODUCTS],"");

   memset (lpbaseprm->szCatalogsOIToolsAvailables,'N',MAX_TOOL_PRODUCTS);

   if (GetOIVers() == OIVERS_NOTOI)
      bGenericGateway = TRUE;
   if (!bGenericGateway)
   {
      nbCat = 0;
      EXEC SQL repeated SELECT count(DISTINCT client_name)
               INTO :nbCat
               FROM ii_client_dep_mod;
      if (sqlca.sqlcode == -30100 || nbCat == 0) // Table not found or no catalog exist
         return RES_SUCCESS;

      EXEC SQL repeated SELECT DISTINCT client_name
               INTO     :szName
               FROM     ii_client_dep_mod;
      EXEC SQL BEGIN;
         for (i = 0;i<MAX_TOOL_PRODUCTS;i++) {
            if ( lstrcmp(pCatName[i],szName) == 0) {
               lpbaseprm->szCatalogsOIToolsAvailables[i] = 'Y';
               break;
            }
         }
         assert(i<MAX_TOOL_PRODUCTS);
      EXEC SQL END;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      iret=SQLCodeToDom(sqlca.sqlcode);

      if (iret!=RES_SUCCESS) 
         return iret;

      if (iret!=RES_SUCCESS) 
         return iret;
      else
         return RES_SUCCESS;
   }
   return RES_SUCCESS;
}

int GetCatalogPageSize(LPDATABASEPARAMS lpbaseprm)
{
   BOOL bCatWithDiffSize = FALSE;
   int iret,iFirstValue = 0;
   EXEC SQL BEGIN DECLARE SECTION;
   int iRelPgSize;
   EXEC SQL END DECLARE SECTION;

   if (lpbaseprm->DbType ==DBTYPE_DISTRIBUTED)
   {
     x_strcpy(lpbaseprm->szCatalogsPageSize ,"");
     return RES_SUCCESS;
   }
   EXEC SQL REPEATED SELECT relpgsize
            INTO :iRelPgSize
            FROM iirelation
      WHERE mod(iirelation.relstat, 2) = 1
            AND mod(iirelation.relstat/32, 2) = 0
            AND mod(iirelation.relstat/128, 2) = 0
            AND (mod(iirelation.relstat/16384, 2) = 0
            OR iirelation.relid = 'iidbcapabilities'
            OR iirelation.relid = 'iisectype'
            OR iirelation.relid = 'iiprivlist');
   EXEC SQL BEGIN;
      if (iFirstValue == 0)
          iFirstValue = iRelPgSize;
      if (iFirstValue != iRelPgSize)
      {
         bCatWithDiffSize = TRUE;
         EXEC SQL ENDSELECT;
      }
   EXEC SQL END;

   if (bCatWithDiffSize)
     x_strcpy(lpbaseprm->szCatalogsPageSize ,"");
   else
     CVla(iRelPgSize,lpbaseprm->szCatalogsPageSize);

   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   iret=SQLCodeToDom(sqlca.sqlcode);
   if (iret!=RES_SUCCESS) 
      return iret;
   else
      return RES_SUCCESS;
}

int GetInfBase(LPDATABASEPARAMS lpbaseprm)

{
   BOOL bGenericGateway = FALSE;
   int iret;
   LPOBJECTLIST obj;
   EXEC SQL BEGIN DECLARE SECTION;
   char *lpObjectName=lpbaseprm->objectname;
   char szDBName[MAXOBJECTNAME];
   char szOwner[MAXOBJECTNAME];
   char szDbDev[MAXOBJECTNAME];
   char szChkpDev[MAXOBJECTNAME];
   char szJrnlDev[MAXOBJECTNAME];
   char szDmpDev[MAXOBJECTNAME];
   char szSortDev[MAXOBJECTNAME];
   char LocName[33];
   long lAccess,Status;
   EXEC SQL END DECLARE SECTION;
   if (GetOIVers() == OIVERS_NOTOI)
      bGenericGateway = TRUE;

   if (bGenericGateway) {
       EXEC SQL repeated SELECT   own
                INTO     :szOwner
                FROM     iidatabase
                WHERE    name=:lpObjectName;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      iret=SQLCodeToDom(sqlca.sqlcode);
      ZEROINIT(lpbaseprm->szOwner);
      ZEROINIT(lpbaseprm->szDbDev);
      ZEROINIT(lpbaseprm->szChkpDev);
      ZEROINIT(lpbaseprm->szJrnlDev);
      ZEROINIT(lpbaseprm->szDmpDev);
      ZEROINIT(lpbaseprm->szSortDev);

      if (iret!=RES_SUCCESS) 
         return iret;
      suppspace(szOwner);
      fstrncpy(lpbaseprm->szOwner, szOwner, MAXOBJECTNAME);
   }
   else {
       EXEC SQL repeated SELECT   own, dbdev, ckpdev, jnldev, dmpdev, sortdev,access
                INTO     :szOwner, :szDbDev, :szChkpDev, :szJrnlDev,
                         :szDmpDev, :szSortDev, :lAccess
                FROM     iidatabase
                WHERE    name=:lpObjectName;

      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      iret=SQLCodeToDom(sqlca.sqlcode);

      if (iret!=RES_SUCCESS) 
         return iret;

      suppspace(szOwner); 
      suppspace(szDbDev); 
      suppspace(szChkpDev);
      suppspace(szJrnlDev);
      suppspace(szDmpDev);
      suppspace(szSortDev);

      fstrncpy(lpbaseprm->szOwner,    szOwner,    MAXOBJECTNAME);
      fstrncpy(lpbaseprm->szDbDev,    szDbDev,    MAXOBJECTNAME);
      fstrncpy(lpbaseprm->szChkpDev,  szChkpDev,  MAXOBJECTNAME);
      fstrncpy(lpbaseprm->szJrnlDev,  szJrnlDev,  MAXOBJECTNAME);
      fstrncpy(lpbaseprm->szDmpDev,   szDmpDev,   MAXOBJECTNAME);
      fstrncpy(lpbaseprm->szSortDev,  szSortDev,  MAXOBJECTNAME);
      lpbaseprm->bPrivate  = (lAccess&0x1)?FALSE:TRUE;
      lpbaseprm->bReadOnly = (lAccess==0x811)?TRUE:FALSE;

      if (lpbaseprm->DbType ==DBTYPE_REGULAR || lpbaseprm->DbType == DBTYPE_COORDINATOR)
      {
          lstrcpy(szDBName,lpbaseprm->objectname);
          // select the location extending for this database
          EXEC SQL repeated SELECT e.location_name,e.status
                   INTO :LocName,:Status
                   FROM iiextend_info e, iilocation_info l
                   WHERE e.status>0 AND e.database_name=:lpObjectName
                   AND e.location_name=l.location_name
                   ORDER BY e.location_name;
          EXEC SQL BEGIN;
            obj = AddListObjectTail (&lpbaseprm->lpDBExtensions, sizeof (DB_EXTENSIONS));
            if (obj)
            {
                LPDB_EXTENSIONS lpRef = (LPDB_EXTENSIONS)obj->lpObject;
                lstrcpy (lpRef->lname, LocName);
                lpRef->status=Status;
            }
            else
            {
                FreeObjectList (lpbaseprm->lpDBExtensions);
                lpbaseprm->lpDBExtensions = NULL;
                EXEC SQL ENDSELECT;
            }
          EXEC SQL END;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          iret=SQLCodeToDom(sqlca.sqlcode);
          if (iret == RES_ENDOFDATA)
            iret = RES_SUCCESS;
      }
      if (iret!=RES_SUCCESS) 
         return iret;
      else
         return RES_SUCCESS;
  }
  return RES_ERR;
}


// SQL error Management 

void DisableSqlSaveText()
{
   exec sql begin declare section;
   int iSaveQuery;
   exec sql end declare section;

   exec sql inquire_sql(:iSaveQuery=savequery);

   if (iSaveQuery) {
      bSqlErrorSaveText=FALSE;
      exec sql set_sql(savequery=0);
   }
}

void EnableSqlSaveText()
{
   exec sql begin declare section;
   int iSaveQuery;
   exec sql end declare section;

   exec sql inquire_sql(:iSaveQuery=savequery);

   if (!iSaveQuery) {
      bSqlErrorSaveText=TRUE;
      exec sql set_sql(savequery=1);
   }
}

void EnableSqlErrorManagmt()
{
   bSqlErrorManagement=TRUE;
}
void DisableSqlErrorManagmt()
{
   bSqlErrorManagement=FALSE;
}

static TCHAR tcTempFileName[_MAX_PATH]= _T("");

static int ReadTemporaryLogFile(TCHAR **lpFileTemp)
{
  int iLenFile,iretlen;

  iLenFile = Vdba_GetLenOfFileName(tcTempFileName);
  if (!iLenFile)
    return iLenFile;
  iLenFile = iLenFile * sizeof(TCHAR) + sizeof(TCHAR);
  *lpFileTemp = ESL_AllocMem(iLenFile);

  if (ESL_FillBufferFromFile(tcTempFileName, *lpFileTemp,iLenFile,
                             &iretlen, TRUE) != RES_SUCCESS)
  {
    ESL_FreeMem (lpFileTemp);
    return 0;
  }
  return iLenFile;
}

static BOOL AddSqlErrorInlist(char * statement, char * retmessage, int icode)
{
   unsigned char SQLQueryTxt[SQLERRORTXTLEN];
   unsigned char SQLErrorTxt[SQLERRORTXTLEN];
   LPSQLERRORINFO lpTemp=NULL;

   lpTemp=ESL_AllocMem(sizeof(SQLERRORINFO));
   if (!lpTemp) {
      if (lpFirstEntry)
         FreeSqlErrorPtr();
      myerror(ERR_ALLOCMEM);
      return FALSE;
   }
   if (!lpFirstEntry)
      lpFirstEntry=lpTemp;
   else
      lpNewestEntry->lpNext=lpTemp;
   lpTemp->lpPrev=lpNewestEntry;
   lpTemp->lpNext=NULL;
   lpTemp->lpSqlStm=NULL;
   lpTemp->lpSqlErrTxt=NULL;
   lpTemp->bActiveEntry=FALSE;
   lpLastEntry=lpTemp;

   lpTemp->bActiveEntry=TRUE;
   lpTemp->sqlcode=icode;
   fstrncpy(SQLQueryTxt, statement,  sizeof(SQLQueryTxt)-1);
   fstrncpy(SQLErrorTxt, retmessage, sizeof(SQLErrorTxt)-1);

   lpTemp->lpSqlErrTxt=ESL_AllocMem(sizeof(SQLErrorTxt)+1);
   if (!lpTemp->lpSqlErrTxt) { 
      FreeSqlErrorPtr(lpFirstEntry);
      myerror(ERR_ALLOCMEM);
      return FALSE;
   }
   x_strcpy(lpTemp->lpSqlErrTxt,SQLErrorTxt);

   lpTemp->lpSqlStm=ESL_AllocMem(sizeof(SQLQueryTxt)+1);
   if (!lpTemp->lpSqlStm) { 
      FreeSqlErrorPtr(lpFirstEntry);
      myerror(ERR_ALLOCMEM);
      return FALSE;
   }

   x_strcpy(lpTemp->lpSqlStm,SQLQueryTxt);

   lpNewestEntry=lpTemp;

   return TRUE;
}


BOOL FillErrorStructure()
{
  TCHAR *lpFileContents = NULL;
  TCHAR *lpCurrentError = NULL;
  TCHAR bufErrorStatement[BUFSIZE];
  TCHAR bufErrorNum[BUFSIZE];
  TCHAR bufErrorNum2[BUFSIZE];
  TCHAR bufErrorText[BUFSIZE];
  TCHAR bufSeparate[BUFSIZE];
#if defined (MAINWIN)
  TCHAR bufRetLine[] = _T("\n");
#else
  TCHAR bufRetLine[] = _T("\r\n");
#endif
  TCHAR tcSQLQueryTxt[SQLERRORTXTLEN];
  TCHAR tcSQLErrorTxt[SQLERRORTXTLEN];
  TCHAR *lpErr, *lpTxt, *lpNum, *lpSep,*lpTemp;
  long lCode;
  int iNbChar,iNbBytesAllocated,iPos,iRetlineLength,iSeperatelineLength,iErrorStatementLength,iErrorTextLength;


  if (lpFirstEntry)
    FreeSqlErrorPtr();

  iNbBytesAllocated = ReadTemporaryLogFile(&lpFileContents);

  if (!iNbBytesAllocated)
    return FALSE;

  if (!lpFileContents)
    return FALSE;
  iRetlineLength = x_strlen(bufRetLine);
  // statement that produced the error
  x_strcpy(bufErrorStatement, ResourceString(IDS_E_SQLERR_STMT));
  iErrorStatementLength = x_strlen(bufErrorStatement);
  // Sql error code
  x_strcpy(bufErrorNum, ResourceString(IDS_E_SQLERR_ERRNUM));
  x_strcpy(bufErrorNum2, bufErrorNum);
  lpErr = _tcsstr(bufErrorNum, _T(":"));
  iPos = lpErr - bufErrorNum + sizeof(TCHAR);
  bufErrorNum[iPos] = _T('\0'); // this removed the %ld format in string
  // Sql error text line after line and time after time
  x_strcpy(bufErrorText, ResourceString(IDS_E_SQLERR_ERRTXT));
  iErrorTextLength = x_strlen(bufErrorText);
  // separator for next statement
  memset(bufSeparate, '-', 70);
  bufSeparate[70] = '\0';
  iSeperatelineLength = x_strlen(bufSeparate);


  lpCurrentError = lpFileContents;
  lpErr = _tcsstr(lpCurrentError,bufErrorStatement);
  if (lpErr != lpCurrentError)
  {
    ESL_FreeMem(lpFileContents);
    return FALSE;
  }
  while (*lpCurrentError != _T('\0'))
  {
    lpErr = _tcsstr(lpCurrentError,bufErrorStatement);
    lpNum = _tcsstr(lpCurrentError,bufErrorNum);
    lpTxt = _tcsstr(lpCurrentError,bufErrorText);
    lpSep = _tcsstr(lpCurrentError,bufSeparate);
    if (!lpErr || !lpNum || !lpTxt || !lpSep) // structure error in file
    {
      ESL_FreeMem(lpFileContents);
      return FALSE;
    }
    lpTemp = lpCurrentError + iErrorStatementLength + iRetlineLength;
    iNbChar = lpNum - lpTemp - iRetlineLength+1;
    lstrcpyn(tcSQLQueryTxt,lpTemp,iNbChar);

    _stscanf(lpNum, bufErrorNum2, &lCode);

    lpTemp = lpTxt + iErrorTextLength + iRetlineLength;
    iNbChar = lpSep - lpTemp - iRetlineLength+1;
    lstrcpyn(tcSQLErrorTxt,lpTemp,iNbChar);

    if ( !AddSqlErrorInlist(tcSQLQueryTxt, tcSQLErrorTxt, lCode))
    {
      ESL_FreeMem(lpFileContents);
      return FALSE;
    }
    lpCurrentError = lpSep + iSeperatelineLength + iRetlineLength;
  }
  ESL_FreeMem(lpFileContents);
  return TRUE;
}

static void WriteInFile (LPCURRENTSQLERRINFO lpFreeEntry, char * FileName)
{
  char      buf[BUFSIZE];
  char      buf2[BUFSIZE];
  HFILE     hFile;
  OFSTRUCT  openBuff;
  char      *p, *p2;

  if (FileName[0])
  {
    // open for write with share denied,
    // preceeded by create if does not exist yet
    hFile = OpenFile(FileName, &openBuff, OF_EXIST);
    if (hFile != HFILE_ERROR)
      hFile = OpenFile(FileName, &openBuff, OF_SHARE_DENY_NONE | OF_WRITE);
    else {
      hFile = OpenFile(FileName, &openBuff, OF_CREATE);
      if (hFile != HFILE_ERROR) {
        _lclose(hFile);
        hFile = OpenFile(FileName, &openBuff, OF_SHARE_DENY_NONE | OF_WRITE);
      }
    }

    if (hFile != HFILE_ERROR) {
      if (_llseek(hFile, 0, 2) != HFILE_ERROR) {
        // statement that produced the error
        x_strcpy(buf, ResourceString(IDS_E_SQLERR_STMT));
        _lwrite(hFile, buf, x_strlen(buf));
        _lwrite(hFile, "\r\n", 2);
        _lwrite(hFile, lpFreeEntry->tcSqlStm,
                       x_strlen(lpFreeEntry->tcSqlStm));
        _lwrite(hFile, "\r\n", 2);

        // Sql error code
        x_strcpy(buf, ResourceString(IDS_E_SQLERR_ERRNUM));
        wsprintf(buf2, buf, lpFreeEntry->sqlcode);
        _lwrite(hFile, buf2, x_strlen(buf2));
        _lwrite(hFile, "\r\n", 2);

        // Sql error text line after line and time after time
        x_strcpy(buf, ResourceString(IDS_E_SQLERR_ERRTXT));
        _lwrite(hFile, buf, x_strlen(buf));
        _lwrite(hFile, "\r\n", 2);
        p = lpFreeEntry->tcSqlErrTxt;
        while (p) {
          p2 = x_strchr(p, '\n');
          if (p2) {
            _lwrite(hFile, p, p2-p);
            _lwrite(hFile, "\r\n", 2);
            p = ++p2;
          }
          else {
            _lwrite(hFile, p, x_strlen(p));
            _lwrite(hFile, "\r\n", 2);
            break;
          }
        }

        // separator for next statement
        memset(buf, '-', 70);
        buf[70] = '\0';
        _lwrite(hFile, buf, x_strlen(buf));
        _lwrite(hFile, "\r\n", 2);

      }
      _lclose(hFile);
    }
  }
}


static void ManageTemporaryLogFile(LPCURRENTSQLERRINFO lpFreeEntry)
{
  if (!tcTempFileName[0])
  {
    if ( !VdbaInitializeTemporaryFileName() )
      return;
  }
  WriteInFile (lpFreeEntry, tcTempFileName);
}

// Emb 24/10/95 : log file management
static void ManageLogFile(LPCURRENTSQLERRINFO lpFreeEntry)
{
  char      bufenv[_MAX_PATH];
  char      *penv = getenv(VDBA_GetInstallPathVar());

  if (!penv) {
    TS_MessageBox(TS_GetFocus(),
                  VDBA_ErrMsgIISystemNotSet(),
                  NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    return ;
  }
  fstrncpy(bufenv,penv,_MAX_PATH);


  if (bufenv[0]) {
#if defined (MAINWIN)
    // build full file name with path
    x_strcat(bufenv, "/");
    x_strcat(bufenv, VDBA_GetIntermPathString());
    x_strcat(bufenv, "/files/errvdba.log");
#else
    // build full file name with path
    x_strcat(bufenv, "\\");
    x_strcat(bufenv, VDBA_GetIntermPathString());
    x_strcat(bufenv, "\\files\\errvdba.log");
#endif
  }
   WriteInFile (lpFreeEntry, bufenv);
}


static BOOL VdbaManageSqlError(void  *Sqlca)
{
   IISQLCA *psqlca=(IISQLCA *) Sqlca;
   CURRENTSQLERRINFO TempSqlError;
   LPUCHAR lpPartToCopy;
   LPUCHAR *lpKeyWord, Keyword[]={  /* NULL, */  /* trick not to remove text */
                              "describe ",
                              "execute immediate ",
                              "prepare dbadynstm from ",
                              NULL};
   exec sql begin declare section;
   unsigned char SQLErrorTxt[SQLERRORTXTLEN];
   unsigned char SQLQueryTxt[SQLERRORTXTLEN];
   exec sql end declare section;


   if (!bSqlErrorManagement)
      return TRUE;

   // Check if we will fill an entry, otherwise quit

   if (psqlca->sqlcode == 0)
      return TRUE;

   if (psqlca->sqlcode == 100L || psqlca->sqlcode == 700L || psqlca->sqlcode == 710L)
      return TRUE;    // populate problem - don't report even as a warning

   memset (SQLQueryTxt, 0, sizeof(SQLQueryTxt));

   if (bSqlErrorSaveText) {
      exec sql inquire_sql (:SQLQueryTxt=querytext); 
      suppspace(SQLQueryTxt);
   }
   else
      x_strcpy(SQLQueryTxt, "Unknown statement - statistic data only");

   // do not use the error text in psqlca, it is only 70 chars. Instead
   // use the inquire_sql statement to retrieve the full text

   memset (SQLErrorTxt, 0, sizeof(SQLErrorTxt));

   exec sql inquire_sql (:SQLErrorTxt=errortext);
   suppspace(SQLErrorTxt);

   TempSqlError.sqlcode=psqlca->sqlcode;

   x_strcpy(TempSqlError.tcSqlErrTxt,SQLErrorTxt);

   lpPartToCopy = NULL;     // Emb 24/10/95 otherwise crash if no keyword found
   lpKeyWord=&Keyword[0];
   while (*lpKeyWord) {
      if (lpPartToCopy=x_strstr(SQLQueryTxt, *lpKeyWord)) {
         lpPartToCopy=SQLQueryTxt+x_strlen(*lpKeyWord);
         break;
      }
      lpKeyWord++;
   }

   if (!lpPartToCopy)
      lpPartToCopy=SQLQueryTxt;

   x_strcpy(TempSqlError.tcSqlStm,lpPartToCopy);

   // Emb 24/10/95 : log file management
   ManageLogFile(&TempSqlError);

   ManageTemporaryLogFile(&TempSqlError);

   return TRUE;
}

BOOL ManageSqlError(void  *Sqlca)
{
   IISQLCA *psqlca = (IISQLCA *) Sqlca;
   long lerrcode  = psqlca->sqlcode;
   BOOL bRes      = VdbaManageSqlError(Sqlca);
   psqlca->sqlcode = lerrcode;
   return bRes;
}

BOOL AddSqlError(char * statement, char * retmessage, int icode)
{
   CURRENTSQLERRINFO  SqlErrInfTemp;

   if (!bSqlErrorManagement)
      return TRUE;

   fstrncpy(SqlErrInfTemp.tcSqlStm, statement,  sizeof(SqlErrInfTemp.tcSqlStm)-1);
   fstrncpy(SqlErrInfTemp.tcSqlErrTxt, retmessage, sizeof(SqlErrInfTemp.tcSqlErrTxt)-1);

   SqlErrInfTemp.sqlcode=icode;

   // Emb 24/10/95 : log file management
   ManageLogFile(&SqlErrInfTemp);

   ManageTemporaryLogFile(&SqlErrInfTemp);

   return TRUE;
}

LPTSTR VdbaGetTemporaryFileName()
{
   return tcTempFileName;
}

VOID VdbaSetTemporaryFileName(LPTSTR pFileName)
{
   x_strcpy(tcTempFileName,pFileName);
}

BOOL VdbaInitializeTemporaryFileName()
{
   TCHAR szFileId  [_MAX_PATH];
   TCHAR szTempPath[_MAX_PATH];
   TCHAR* penv;
# ifdef NT_GENERIC
   NMgtAt( _T("II_TEMPORARY"), &penv );
   if( penv == NULL || *penv == EOS )
      x_strcpy(szTempPath, _T("."));
   else
   x_strcpy(szTempPath,penv);
# else
   x_strcpy(szTempPath,TEMP_DIR);
# endif

# ifdef NT_GENERIC
   x_strcat(szTempPath,_T("\\"));
#endif
   if (GetTempFileName(szTempPath, "dba", 0, szFileId) == 0)
      return FALSE;

   x_strcpy(tcTempFileName,szFileId);
   return TRUE;
}

VOID FreeSqlErrorPtr()
{
   LPSQLERRORINFO lpToFree, lpTemp=lpFirstEntry;

   while (lpTemp) {
      if (lpTemp->lpSqlStm)
         ESL_FreeMem(lpTemp->lpSqlStm);
      if (lpTemp->lpSqlErrTxt)
         ESL_FreeMem(lpTemp->lpSqlErrTxt);
      lpToFree=lpTemp;
      lpTemp=lpTemp->lpNext;
      ESL_FreeMem(lpToFree);
   }

   lpFirstEntry=NULL;
   lpLastEntry=NULL;
   lpNewestEntry=NULL;
}

LPSQLERRORINFO GetPrevSqlError(LPSQLERRORINFO lpSqlError)
{
   LPSQLERRORINFO lpReturn;

   if (!lpSqlError)
      return lpSqlError;

   lpReturn=lpSqlError->lpPrev;

   if (!lpReturn)                // We were on the first physical entry
      lpReturn=lpLastEntry;      // wrap to the last (logicaly prev)

   if (lpReturn==lpNewestEntry)  // We wrapped to the newest entry
      return NULL;               // End of the list

   if (lpReturn->bActiveEntry)
      return lpReturn;

   return NULL;

}
LPSQLERRORINFO GetNextSqlError(LPSQLERRORINFO lpSqlError)
{
   LPSQLERRORINFO lpReturn;

   if (!lpSqlError)
      return lpSqlError;

   if (lpSqlError==lpNewestEntry)   // We are already on the newest entry
      return NULL;                  // no next

   lpReturn=lpSqlError->lpNext;

   if (!lpReturn)                   // We were on the physical last entry
      lpReturn=lpFirstEntry;        // wrap to the firt (logicaly next)
// PS 08/08/97 
// This fonction needs to return a value, otherwise the "Next" button has no effect 
// when you push it down. 
   return lpReturn;

}
LPSQLERRORINFO GetOldestSqlError()
{
   LPSQLERRORINFO lpReturn;

   if (!lpNewestEntry)    // no aactive entry in the list
      return NULL;

   lpReturn=lpNewestEntry->lpNext;  // oldest physicaly follows Newest.

   if (!lpReturn)                   // newest was the last physical entry
      lpReturn=lpFirstEntry;        // take the first physical entry.

   while (!lpReturn->bActiveEntry) {  // Look for an active entry
      lpReturn=lpReturn->lpNext;
      if (!lpReturn)                // Wrap if first one
         lpReturn=lpFirstEntry;
   }

   return lpReturn;
}
LPSQLERRORINFO GetNewestSqlError()
{

   return lpNewestEntry;

}



int ReplicConnect( LPREPLCONNECTPARAMS lpaddrepl )
{
   exec sql begin declare section;
   int  server;
   int  target;
   int  number = lpaddrepl->nNumber;
   char szComment[81];
   char infnode_name[25];
   char databasename[25];
   char databaseowner[25];
   int nbRows;
   int dd_nb;
   int iret;
   exec sql end declare section;

   if ( lpaddrepl->nReplicVersion == REPLIC_V105)
      exec sql repeated select database_name, node_name    , dba_comment , server_role ,target_type,
                      database_owner  
               into   :databasename, :infnode_name, :szComment  , :server     ,:target ,
                      :databaseowner
               from   dd_connections
               where  database_no = :number;
   else
      exec sql repeated select database_name, node_name    , dba_comment , server_role ,target_type
               into   :databasename, :infnode_name, :szComment  , :server     ,:target
               from   dd_connections
               where  database_no = :number;


   iret = sqlca.sqlcode;
   if (sqlca.sqlcode == 0) {
     lpaddrepl->nServer=server;
     lpaddrepl->nType  =target;
     x_strcpy(lpaddrepl->szDescription ,szComment);
     x_strcpy(lpaddrepl->szVNode ,infnode_name);
     x_strcpy(lpaddrepl->szDBName,databasename); 
     if ( lpaddrepl->nReplicVersion == REPLIC_V105)
         x_strcpy(lpaddrepl->szDBOwner,databaseowner); 

     exec sql repeated select database_no into  :dd_nb
               from dd_installation
               where database_no=:number and
                     node_name=:infnode_name and
                     target_type=:target and
                     dba_comment=:szComment;
                    // database_no,node_name,target_type,dba_comment
     exec sql inquire_sql(:nbRows=rowcount);
     if (nbRows == 1)
        lpaddrepl->bLocalDb=TRUE;
     else
        lpaddrepl->bLocalDb=FALSE;
     }
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if ( iret==100 && lpaddrepl->bLocalDb==TRUE)  {
    // Find in table DD_INSTALLATION the local database and verify in
    // DD_CONNECTIONS if already exist.
    // TO BE FINISHED 
  exec sql repeated select database_no into  :dd_nb
               from dd_installation;
  if ( lpaddrepl->nReplicVersion == REPLIC_V105) {
     exec sql repeated select database_name, node_name    , dba_comment , server_role ,
                     target_type ,database_owner
         into   :databasename, :infnode_name, :szComment  , :server     ,
                    :target ,:databaseowner
         from   dd_connections
         where  database_no = :dd_nb;
  }
  else {
   exec sql repeated select database_name, node_name    , dba_comment , server_role , target_type
         into   :databasename, :infnode_name, :szComment  , :server     ,  :target
         from   dd_connections
         where  database_no = :dd_nb;
  }

  exec sql inquire_sql(:nbRows=rowcount);
  if (nbRows == 1)  {
    lpaddrepl->nNumber = dd_nb;
    lpaddrepl->nServer = server;
    lpaddrepl->nType   = target;
    x_strcpy(lpaddrepl->szDescription ,szComment);
    x_strcpy(lpaddrepl->szVNode ,infnode_name);
    x_strcpy(lpaddrepl->szDBName,databasename);
    if ( lpaddrepl->nReplicVersion == REPLIC_V105)
       x_strcpy(lpaddrepl->szDBOwner,databaseowner); 

    return RES_SUCCESS;
    }
  else
    return SQLCodeToDom(sqlca.sqlcode);
  }
  else
    return SQLCodeToDom(iret);
}

int ReplicConnectV11( LPREPLCONNECTPARAMS lpaddrepl )
{
   exec sql begin declare section;
      char databasename[MAXOBJECTNAME];
      char infnode_name[MAXOBJECTNAME];
      char szComment[MAX_DESCRIPTION_LEN];
      char Dbmsserver[MAXOBJECTNAME];
      int local;
      char mobdbowner[MAXOBJECTNAME];
      char shortdescription[MAXOBJECTNAME];
      int  number = lpaddrepl->nNumber;
   exec sql end declare section;

   exec sql repeated select d.database_name, d.vnode_name   , d.remark ,
                   d.dbms_type , d.local_db ,d.database_owner,t.short_description
            into   :databasename, :infnode_name, :szComment,
                   :Dbmsserver  , :local ,:mobdbowner,:shortdescription
            from   dd_databases d,dd_dbms_types t
            where  d.database_no = :number
            and    d.dbms_type = t.dbms_type;

   if (sqlca.sqlcode == 0) {
     lpaddrepl->bLocalDb=local;
     x_strcpy(lpaddrepl->szDBMStype    ,Dbmsserver  );
     x_strcpy(lpaddrepl->szDescription ,szComment   );
     x_strcpy(lpaddrepl->szVNode       ,infnode_name);
     x_strcpy(lpaddrepl->szDBName      ,databasename);
     x_strcpy(lpaddrepl->szDBOwner     ,mobdbowner);
     x_strcpy(lpaddrepl->szShortDescrDBMSType , shortdescription);
     suppspace(lpaddrepl->szDBMStype);
     suppspace(lpaddrepl->szVNode);
     suppspace(lpaddrepl->szDBName);
     suppspace(lpaddrepl->szDBOwner);
     suppspace(lpaddrepl->szShortDescrDBMSType);

     return RES_SUCCESS;
   }
   return SQLCodeToDom(sqlca.sqlcode);
}

int ReplicCDDSV11( LPREPCDDS lpcdds )
{
   exec sql begin declare section;
      char cddsname[MAXOBJECTNAME];
      int collision;
      int lastnumber;
      int errormode;
      int  number = lpcdds->cdds;
   exec sql end declare section;

   exec sql repeated select cdds_name, collision_mode , error_mode
            into   :cddsname, :collision     , :errormode
            from   dd_cdds
            where  cdds_no = :number;

   if (sqlca.sqlcode == 0) {
     lpcdds->error_mode     = errormode;
     lpcdds->collision_mode = collision;
     x_strcpy(lpcdds->szCDDSName    ,cddsname   );
     
     // find the last number for table_no 
     exec sql repeated select last_number
          into   :lastnumber
          from   dd_last_number
          where  column_name = 'table_no';
     if (sqlca.sqlcode == 0)
        lpcdds->LastNumber=lastnumber;
     else
        return SQLCodeToDom(sqlca.sqlcode);

     return RES_SUCCESS;
   }
   return SQLCodeToDom(sqlca.sqlcode);
}


int ReplicRetrieveIndexstring( LPDD_REGISTERED_TABLES lpRegTable )
{
    exec sql begin declare section;
        char Basename[MAXOBJECTNAME];
        char Owner   [MAXOBJECTNAME];
        char Name    [MAXOBJECTNAME];
        short snullind1; 
    exec sql end declare section;

    ZEROINIT(Name);

    wsprintf(Basename ,"%s",lpRegTable->tablename);
    wsprintf(Owner    ,"%s",lpRegTable->tableowner);

    exec sql repeated select min(index_name) 
                    into :Name:snullind1
                    from iiindexes 
                    where base_name = :Basename and
                    base_owner  = :Owner and unique_rule = 'U';

    if (!snullind1)
        x_strcpy(lpRegTable->IndexName,Name);

    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    return sqlca.sqlcode;
}

void ReplicDateNow( LPUCHAR dnow )
{
    EXEC SQL BEGIN DECLARE SECTION;
        char DateString[MAXOBJECTNAME];
    EXEC SQL END DECLARE SECTION;

    EXEC SQL repeated SELECT DATE_GMT('now') into :DateString; 
    
    ManageSqlError(&sqlca); // Keep track of the SQL error if any

    x_strcpy( dnow,DateString);
}

BOOL AreInstalledSupportTables ( LPUCHAR ProcName)
{
  exec sql begin declare section;
  int nbRows;
  char statement[MAXOBJECTNAME];
  exec sql end declare section;
  
  wsprintf(statement,"%s",ProcName);

  nbRows=0;
  exec sql repeated select count(*)
           into :nbRows
           from   iiprocedures
           where  procedure_name = :statement;
  
  if (nbRows >0)
    return TRUE;
  else
    return FALSE;

}

BOOL AreInstalledRules ( LPUCHAR ProcName)
{
  exec sql begin declare section;
  int nbRows;
  char statement[MAXOBJECTNAME];
  exec sql end declare section;
  
  x_strcpy(statement,ProcName);

  nbRows=0;
  exec sql repeated select count(*)
           into :nbRows
           from   iirules
           where  rule_name = :statement;
  
  if (nbRows >0)
    return TRUE;
  else
    return FALSE;

}

int GetMobileInfo(LPREPMOBILE lprepmobile)
{
   exec sql begin declare section;
      int colmode;
      int errmode;
   exec sql end declare section;

   if (!lprepmobile)
      return myerror(ERR_ADDOBJECT);

       
   exec sql repeated select collision_mode , error_mode 
            into :colmode,:errmode
            from  dd_mobile_opt;

   ManageSqlError(&sqlca); // Keep track of the SQL error if any
  
  if (sqlca.sqlcode == 0) {
	lprepmobile->collision_mode = colmode;
	lprepmobile->error_mode     = errmode;
	return TRUE;
  }
  else 	
   return FALSE;
}

/*{
** Name:  ForceFlush - force a TCB flush
**
** Description:
**     Force a TCB flush so the DBMS will be aware that replication was
**     enabled or disabled.
**
** Inputs:
**     table_name, table_owner.
** 
** Outputs:
**     none
**
** Returns:
**  
*/
int ForceFlush( char *table_name,char *table_owner )
{
   EXEC SQL BEGIN DECLARE SECTION;
     long  table_id;
     char tablename[MAXOBJECTNAME];
     char tableowner[MAXOBJECTNAME];
   EXEC SQL END DECLARE SECTION;

   x_strcpy (tablename,table_name);
   x_strcpy (tableowner,table_owner);

   EXEC SQL repeated SELECT reltid
            INTO :table_id
            FROM iirelation
            WHERE relid = :tablename
            AND relowner = :tableowner;

   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   if (sqlca.sqlcode == 0) {
     EXEC SQL SET TRACE POINT dm713 :table_id;
     //EXEC SQL COMMIT;
     return SQLCodeToDom(sqlca.sqlcode);
   }
   // "Table ID error"
   TS_MessageBox(TS_GetFocus(), ResourceString(IDS_ERR_TBL_ID_NOT_FOUND),
                 NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
   return SQLCodeToDom(sqlca.sqlcode);
}

//
// UK Sotheavut. August 13, 1996
// This following block is added for VDBA V2.0x
//

int VDBA2xGetTableColumnsInfo (LPTABLEPARAMS tableprm)
{

    return RES_SUCCESS;
}

//
// We use the extra fields of STRINGLIST as following:
// extraInfo1 -> Data type
// extraInfo2 -> Length
// or
// extraInfo2 -> Precision and extraInfo3 -> Scale, if data type is DECIMAL
//
LPSTRINGLIST 
VDBA2xGetTableKeyInfo (LPCTSTR lpszRefedTbParent, LPCTSTR lpszRefedTb, LPCTSTR lpszRefedConstr, int* m)
{
    int iret;
    LPSTRINGLIST lpStringList = NULL, lpObj;
    //
    // Host Variable Declarations
    //
    exec sql begin declare section;
        char szColName      [MAXOBJECTNAME];
        char tbName         [MAXOBJECTNAME];
        char tbCreator      [MAXOBJECTNAME];
        char szConstraint   [MAXOBJECTNAME];
        int  nColNo;
        int  iDataType;
        int  iDataLen; 
        int  iScale;

    exec sql end declare section; 

    lstrcpy (tbName,      lpszRefedTb);
    lstrcpy (tbCreator,   lpszRefedTbParent);
    lstrcpy (szConstraint,lpszRefedConstr);
    //
    // Query the key columns from TABLE IIKEYS and IICOLUMNS
    //                                  

    iret= RES_SUCCESS;

    exec sql repeated select 
                A.COLUMN_NAME,
                A.KEY_POSITION,
                B.COLUMN_INGDATATYPE,
                B.COLUMN_LENGTH,
                B.COLUMN_SCALE
        into    :szColName, 
                :nColNo,
                :iDataType,
                :iDataLen,
                :iScale
        from    IIKEYS A, IICOLUMNS B
        where   A.TABLE_NAME = :tbName and A.SCHEMA_NAME = :tbCreator and 
                B.TABLE_NAME = :tbName and B.TABLE_OWNER = :tbCreator and
                A.COLUMN_NAME= B.COLUMN_NAME                          and
                A.CONSTRAINT_NAME = :szConstraint
                order by A.KEY_POSITION;
    exec sql begin;
        ManageSqlError (&sqlca); // Keep track of the SQL error if any
        iret = SQLCodeToDom (sqlca.sqlcode);
        if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
        {
            lpStringList = StringList_Done (lpStringList);            
            exec sql endselect;
            if (m) *m = iret;
            return NULL;
        }
        suppspace (szColName);
        lpStringList = StringList_AddObject (lpStringList, szColName, &lpObj);            
        if (!lpStringList)
        {
            exec sql endselect;
            if (m) *m = RES_ERR;
            return NULL;
        }
        lpObj->extraInfo1 = IngToDbaType(iDataType, iDataLen);
        if (iDataType == IISQ_DEC_TYPE) 
        {
            lpObj->extraInfo2 = iDataLen;
            lpObj->extraInfo3 = iScale;
        }
        else
            lpObj->extraInfo2 = iDataLen;
    exec sql end;
    ManageSqlError (&sqlca); // Keep track of the SQL error if any
    iret = SQLCodeToDom(sqlca.sqlcode);
    if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
    {
        lpStringList = StringList_Done (lpStringList);            
        if (m) *m = iret;
        return NULL;
    }
    if (m) *m = RES_SUCCESS;
    return lpStringList;
}

//
// We use the extra fields of STRINGLIST as following:
// extraInfo1 -> Data type
// extraInfo2 -> Length
// or
// extraInfo2 -> Precision and extraInfo3 -> Scale, if data type is DECIMAL
//
int VDBA2xGetTablePrimaryKeyInfo (LPTABLEPARAMS lpTS)
{
    int iret;
    LPSTRINGLIST lpObj;

    //
    // Host Variable Declarations
    //
    exec sql begin declare section;
        char szTbName       [MAXOBJECTNAME];
        char szTbCreator    [MAXOBJECTNAME];
        char szColName      [MAXOBJECTNAME];
        char tbName         [MAXOBJECTNAME];
        char tbCreator      [MAXOBJECTNAME];
        char szConstraint   [MAXOBJECTNAME];
        char szConstrType   [2];
        int  nColNo;
        int  iDataType;
        int  iDataLen; 
        int  iScale;

    exec sql end declare section; 

    x_strcpy (tbName,    lpTS->objectname);
    x_strcpy (tbCreator, lpTS->szSchema);
    //
    // Query the primary key constraint from TABLE IIKEYS and IICONSTRAINTS and IICOLUMNS
    // JOIN : EQUI-JOIN (FULL JOIN)
    //                                  

    lpTS->lpPrimaryColumns = NULL;
    iret= RES_SUCCESS;

    exec sql repeated select 
                A.SCHEMA_NAME,
                A.TABLE_NAME,
                A.COLUMN_NAME,
                A.CONSTRAINT_NAME,
                A.KEY_POSITION,
                B.CONSTRAINT_TYPE,
                C.COLUMN_INGDATATYPE,
                C.COLUMN_LENGTH,
                C.COLUMN_SCALE
        into    :szTbCreator, 
                :szTbName, 
                :szColName, 
                :szConstraint,
                :nColNo,
                :szConstrType,
                :iDataType,
                :iDataLen,
                :iScale

        from    IIKEYS A, IICONSTRAINTS B, IICOLUMNS C
        where   A.TABLE_NAME = :tbName and A.SCHEMA_NAME = :tbCreator           and 
                A.TABLE_NAME = B.TABLE_NAME and A.SCHEMA_NAME = B.SCHEMA_NAME   and
                C.TABLE_NAME = :tbName and C.TABLE_OWNER = :tbCreator           and
                C.COLUMN_NAME= A.COLUMN_NAME                                    and
                A.CONSTRAINT_NAME = B.CONSTRAINT_NAME                           and
                B.CONSTRAINT_TYPE = 'P'
        order by KEY_POSITION;
    exec sql begin;
        ManageSqlError (&sqlca); // Keep track of the SQL error if any
        iret = SQLCodeToDom (sqlca.sqlcode);
        if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
        {
            lpTS->lpPrimaryColumns = StringList_Done (lpTS->lpPrimaryColumns);
            exec sql endselect;
            return iret;
        }

        suppspace (szColName);
        lpTS->lpPrimaryColumns = StringList_AddObject (lpTS->lpPrimaryColumns, szColName, &lpObj);
        if (!lpTS->lpPrimaryColumns)
        {
            exec sql endselect;
            return iret;
        }
        lpObj->extraInfo1 = IngToDbaType(iDataType, iDataLen);
        if (iDataType == IISQ_DEC_TYPE) 
        {
            lpObj->extraInfo2 = iDataLen;
            lpObj->extraInfo3 = iScale;
        }
        else
            lpObj->extraInfo2 = iDataLen;
    exec sql end;
    ManageSqlError (&sqlca); // Keep track of the SQL error if any
    iret = SQLCodeToDom(sqlca.sqlcode);
    if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
    {
        lpTS->lpPrimaryColumns = StringList_Done (lpTS->lpPrimaryColumns);
        return iret;
    }
    if (lpTS->lpPrimaryColumns)
    {
        suppspace (szConstraint);
        lstrcpy (lpTS->tchszPrimaryKeyName, szConstraint);
        lstrcpy (lpTS->primaryKey.tchszPKeyName, szConstraint);
        lpTS->primaryKey.pListKey = StringList_Copy (lpTS->lpPrimaryColumns);
        lpTS->primaryKey.bAlter   = TRUE;
        VDBA2xGetIndexConstraint (&(lpTS->primaryKey.constraintWithClause), szConstraint);
    }
    return RES_SUCCESS;
}

// ----------------------------------
// For the REFERENCEPARAMS
// Select from IICONSTRAINTS:
//      - Constraint name,
//      - Table name
//      - Table owner
// Select from IIREF_CONSTRAINTS:
//      - Parent table owner
//      - Parent table name
// Select from IIKEYS with 
// equal-join (IIREF_CONSTRAINTS.UNIQUE_CONSTRAINT_NAME = IIKEYS.CONSTRAINT_NAME)
//      - List of referencing columns
//      - List of referenced columns (due to the doc, primary key or unique keys) 
// ------------------------------------------------------------------------------
int VDBA2xGetTableForeignKeyInfo (LPTABLEPARAMS lpTS)
{
    int m, iret;
    LPOBJECTLIST lpRefList = NULL, ls,  obj, o;
    LPREFERENCEPARAMS lpRefParam;
    LPSTRINGLIST l1, l2, lpRefingColumns = NULL, lpRefedColumns = NULL;
    //
    // Host Variable Declarations
    //
    exec sql begin declare section;
        char szTbName       [MAXOBJECTNAME];
        char szTbCreator    [MAXOBJECTNAME];
        char tbName         [MAXOBJECTNAME];
        char tbCreator      [MAXOBJECTNAME];
        char szConstraint   [MAXOBJECTNAME];
        char szRefedTbParent[MAXOBJECTNAME];
        char szRefedTb      [MAXOBJECTNAME];
        char szReferedConstr[MAXOBJECTNAME];

    exec sql end declare section; 

    x_strcpy (tbName,    lpTS->objectname);
    x_strcpy (tbCreator, lpTS->szSchema);
    //
    // Query the primary key constraint from TABLE IICONSTRAINTS and IIREF_CONSTRAINTS
    // JOIN : EQUI-JOIN (FULL JOIN)
    //                                  
    iret= RES_SUCCESS;
    exec sql repeated select distinct 
                A.SCHEMA_NAME,
                A.TABLE_NAME,
                A.CONSTRAINT_NAME,
                B.UNIQUE_SCHEMA_NAME,
                B.UNIQUE_TABLE_NAME,
                B.UNIQUE_CONSTRAINT_NAME
        into    :szTbCreator, 
                :szTbName, 
                :szConstraint,
                :szRefedTbParent,
                :szRefedTb,
                :szReferedConstr
        from    IICONSTRAINTS A, IIREF_CONSTRAINTS B
        where   A.TABLE_NAME = :tbName and A.SCHEMA_NAME = :tbCreator                   and 
                A.TABLE_NAME = B.REF_TABLE_NAME and A.SCHEMA_NAME = B.REF_SCHEMA_NAME   and
                A.CONSTRAINT_NAME = B.REF_CONSTRAINT_NAME                               and
                A.CONSTRAINT_TYPE = 'R';
    exec sql begin;
        ManageSqlError (&sqlca); // Keep track of the SQL error if any
        iret = SQLCodeToDom (sqlca.sqlcode);
        if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
        {
            FreeObjectList (lpTS->lpReferences);
            lpTS->lpReferences = NULL;
            exec sql endselect;
            return iret;
        }
        suppspace (szConstraint);
        suppspace (szRefedTbParent);
        suppspace (szRefedTb);
        suppspace (szReferedConstr);

        //
        // Create a new Reference Param and add it into the list of
        // References.        
        obj = AddListObjectTail (&lpRefList, sizeof (REFERENCEPARAMS));
        if (obj)
        {
            LPREFERENCEPARAMS lpRef = (LPREFERENCEPARAMS)obj->lpObject;
            
            lpRef->lpColumns = NULL;
            lstrcpy (lpRef->szConstraintName, szConstraint);
            lstrcpy (lpRef->szRefConstraint,  szReferedConstr);
            lstrcpy (lpRef->szSchema,         szRefedTbParent);
            lstrcpy (lpRef->szTable,          szRefedTb);
            lpRef->bAlter = TRUE;
        }
        else
        {
            FreeObjectList (lpRefList);
            lpRefList = NULL;
            exec sql endselect;
            return RES_ERR;
        }
    exec sql end;
    ManageSqlError (&sqlca); // Keep track of the SQL error if any
    iret = SQLCodeToDom(sqlca.sqlcode);
    if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
    {
        LPREFERENCEPARAMS lpRefP;
        ls = lpRefList;
        while (ls)
        {
            lpRefP = (LPREFERENCEPARAMS)ls->lpObject;
            if (lpRefP->lpColumns)
                FreeObjectList (lpRefP->lpColumns);
            ls = ls->lpNext;
        }
        FreeObjectList (lpRefList);
        return iret;
    }
    ls = lpRefList;
    while (ls)
    {
        //
        // Get the List of Referencing columns
        //
        lpRefParam = (LPREFERENCEPARAMS)ls->lpObject;
        lpRefParam->lpColumns = NULL;
        lpRefingColumns = VDBA2xGetTableKeyInfo (tbCreator, tbName, lpRefParam->szConstraintName, &m); 
        if (!m)
        {
            FreeObjectList (lpRefList);
            lpRefList = NULL;
            return RES_ERR;
        }
        //
        // Get the List of Referenced columns
        //
        lpRefedColumns = VDBA2xGetTableKeyInfo (lpRefParam->szSchema, lpRefParam->szTable, lpRefParam->szRefConstraint, &m); 
        if (!m)
        {
            FreeObjectList (lpRefList);
            lpRefList = NULL;
            return RES_ERR;
        }
        if (StringList_Count (lpRefedColumns) != StringList_Count (lpRefingColumns))
        {
            FreeObjectList (lpRefList);
            lpRefList = NULL;
            return RES_ERR;
        }
        l1 = lpRefingColumns;
        l2 = lpRefedColumns;
        while (l1 && l2)
        {
            o = AddListObjectTail (&(lpRefParam->lpColumns), sizeof (REFERENCECOLS));
            if (o)
            {
                LPREFERENCECOLS lpCol = (LPREFERENCECOLS)o->lpObject;
                lstrcpy (lpCol->szRefingColumn, l1->lpszString);
                lstrcpy (lpCol->szRefedColumn,  l2->lpszString);
            }
            else
            {
                FreeObjectList (lpRefParam->lpColumns);
                FreeObjectList (lpRefList);
                lpRefList = NULL;
                return RES_ERR;
            }
            l1 = l1->next;
            l2 = l2->next;
        }
        lpRefingColumns = StringList_Done (lpRefingColumns);
        lpRefedColumns  = StringList_Done (lpRefedColumns);
        //
        // Get the Index Enforcement of the Constraint:
        VDBA2xGetIndexConstraint (&(lpRefParam->constraintWithClause), lpRefParam->szConstraintName);
        ls = ls->lpNext;
    }    
    lpTS->lpReferences = lpRefList;
    /* DEBUG TEST: TO SEE IF THE LIST IS CORRECTLY INITIALIZED !!!
    ls = lpTS->lpReferences; 
    while (ls)
    {
        LPREFERENCEPARAMS lpr;
        LPOBJECTLIST lx;
        LPREFERENCECOLS lpC;
        lpr = (LPREFERENCEPARAMS)ls->lpObject;
        lx = lpr->lpColumns;
        while (lx)
        {
            lpC = (LPREFERENCECOLS)lx->lpObject;
            lx = lx->lpNext;
        }
        ls = ls->lpNext;
    }
    */
    return RES_SUCCESS;
}


// ----------------------------------
// For the UNIQUEPARAMS
// Select from IICONSTRAINTS:
//      - Constraint name,
//      - Table name
//      - Table owner
// Select from IIKEYS with equal-join (IICONSTRAINTS.CONSTRAINT_NAME = IIKEYS.CONSTRAINT_NAME)
//      - List of columns
// ------------------------------------------------------------------------------
int VDBA2xGetTableUniqueKeyInfo  (LPTABLEPARAMS lpTS)
{
    int iret;
    LPTLCUNIQUE  lpUnique = NULL, obj;
    //
    // Host Variable Declarations
    //
    exec sql begin declare section;
        char szTbName       [MAXOBJECTNAME];
        char szTbCreator    [MAXOBJECTNAME];
        char tbName         [MAXOBJECTNAME];
        char tbCreator      [MAXOBJECTNAME];
        char szConstraint   [MAXOBJECTNAME];

    exec sql end declare section; 

    x_strcpy (tbName,    lpTS->objectname);
    x_strcpy (tbCreator, lpTS->szSchema);
    //
    // Query the unique key constraint from TABLE IIKEYS and IICONSTRAINTS
    // JOIN : EQUI-JOIN (FULL JOIN)
    //                                  
    iret= RES_SUCCESS;

    exec sql repeated select distinct
                SCHEMA_NAME,
                TABLE_NAME,
                CONSTRAINT_NAME
        into    :szTbCreator, 
                :szTbName, 
                :szConstraint
        from    IICONSTRAINTS
        where   TABLE_NAME = :tbName and SCHEMA_NAME = :tbCreator and 
                CONSTRAINT_TYPE = 'U';
    exec sql begin;
        ManageSqlError (&sqlca); // Keep track of the SQL error if any
        iret = SQLCodeToDom (sqlca.sqlcode);
        if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
        {
            lpUnique = VDBA20xTableUniqueKey_Done (lpUnique);
            exec sql endselect;
            return iret;
        }

        suppspace (szConstraint);
        obj = (LPTLCUNIQUE)ESL_AllocMem (sizeof (TLCUNIQUE));
        if (!obj)
        {
            exec sql endselect;
            lpUnique = VDBA20xTableUniqueKey_Done (lpUnique);
            return iret;
        }
        obj->bAlter = TRUE;
        lstrcpy (obj->tchszConstraint, szConstraint);
        lpUnique = VDBA20xTableUniqueKey_Add (lpUnique, obj);
        if (!lpUnique)
        {
            exec sql endselect;
            return iret;
        }
    exec sql end;
    ManageSqlError (&sqlca); // Keep track of the SQL error if any
    iret = SQLCodeToDom(sqlca.sqlcode);
    if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
    {
        lpUnique = VDBA20xTableUniqueKey_Done (lpUnique);
        return iret;
    }
    if (lpUnique)
    {
        int m;
        LPSTRINGLIST lpStringList = NULL;
        LPTLCUNIQUE  ls = lpUnique;
        while (ls)
        {
            lpStringList = VDBA2xGetTableKeyInfo (tbCreator, tbName, ls->tchszConstraint, &m);
            ls->lpcolumnlist = lpStringList;
            if (!lpStringList)
            {
                lpUnique = VDBA20xTableUniqueKey_Done (lpUnique);
                return iret;
            }
            //
            // Get the Index Enforcement of the Constraint:
            VDBA2xGetIndexConstraint (&(ls->constraintWithClause), ls->tchszConstraint);
            ls = ls->next;
        }
        lpTS->lpUnique = lpUnique;
    }

    return RES_SUCCESS;
}


// ----------------------------------
// For the CHECKPARAMS
//
// Select from IICONSTRAINTS:
//      - Constraint name,
//      - Table name
//      - Table owner
//      - The check expression.
// ------------------------------------------------------------------------------
int VDBA2xGetTableCheckInfo  (LPTABLEPARAMS lpTS)
{
    int iret, m;
    LPTLCCHECK  lpCheck = NULL, obj;
    //
    // Host Variable Declarations
    //
    exec sql begin declare section;
        char szTbName       [MAXOBJECTNAME];
        char szTbCreator    [MAXOBJECTNAME];
        char tbName         [MAXOBJECTNAME];
        char tbCreator      [MAXOBJECTNAME];
        char szConstraint   [MAXOBJECTNAME];
        char szExpr         [256];
        int  textsequence;
    exec sql end declare section; 

    x_strcpy (tbName,    lpTS->objectname);
    x_strcpy (tbCreator, lpTS->szSchema);
    //
    // Query the check condition constraint from TABLE IICONSTRAINTS
    // JOIN : EQUI-JOIN (FULL JOIN)
    //                                  
    iret= RES_SUCCESS;

    exec sql repeated select distinct
                SCHEMA_NAME,
                TABLE_NAME,
                CONSTRAINT_NAME,
                TEXT_SEGMENT,
                TEXT_SEQUENCE              
        into    :szTbCreator, 
                :szTbName, 
                :szConstraint,
                :szExpr,
                :textsequence
        from    IICONSTRAINTS
        where   TABLE_NAME = :tbName and SCHEMA_NAME = :tbCreator and 
                CONSTRAINT_TYPE = 'C';

    exec sql begin;
        ManageSqlError (&sqlca); // Keep track of the SQL error if any
        iret = SQLCodeToDom (sqlca.sqlcode);
        if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
        {
            lpCheck = VDBA20xTableCheck_Done (lpCheck);
            exec sql endselect;
            return iret;
        }

        suppspace (szConstraint);
        suppspace (szExpr);
        if (textsequence == 1)
        {
            obj = (LPTLCCHECK)ESL_AllocMem (sizeof (TLCCHECK));
            if (!obj)
            {
                lpCheck = VDBA20xTableCheck_Done (lpCheck);
                exec sql endselect;
                return iret;
            }
            lstrcpy (obj->tchszConstraint, szConstraint);
            obj->lpexpression = NULL;
            obj->bAlter = TRUE;
            m = ConcateStrings (&(obj->lpexpression), szExpr,  (LPTSTR)0);
            if (!m)
            {
                lpCheck = VDBA20xTableCheck_Done (lpCheck);
                exec sql endselect;
                return iret;
            }
            lpCheck = VDBA20xTableCheck_Add (lpCheck, obj);
            if (!lpCheck)
            {
                lpCheck = VDBA20xTableCheck_Done (lpCheck);
                exec sql endselect;
                return iret;
            }
        }
        else
        if (textsequence > 1)
        {
            m = ConcateStrings (&(obj->lpexpression), szExpr,  (LPTSTR)0);
            if (!m)
            {
                lpCheck = VDBA20xTableCheck_Done (lpCheck);
                exec sql endselect;
                return iret;
            }
        }
    exec sql end;
    ManageSqlError (&sqlca); // Keep track of the SQL error if any
    iret = SQLCodeToDom(sqlca.sqlcode);
    if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
    {
        lpCheck = VDBA20xTableCheck_Done (lpCheck);
        return iret;
    }
    lpTS->lpCheck = lpCheck;
    return RES_SUCCESS;
}







//
// Get information about table for altering
// . Need list of columns and attributes
// . Need list of foreign keys.
// . Need list of primary keys.
//
int VDBA2xAlterTableGetInfo (LPTABLEPARAMS tableprm)
{
#if 0
    LPIDXCOLS lpObj;
    LPOBJECTLIST lpList = NULL;
    LPOBJECTLIST lpTemp = NULL;              
    LPCOLUMNPARAMS lpCol;
    LPCONSTRAINTPARAMS lpConst;

    LPTABLECOLUMNS lpCol, lpColList = NULL;
    int iret;
    //
    // Host Variable Declarations
    //
    exec sql begin declare section;
        char szTbName       [MAXOBJECTNAME];
        char szTbCreator    [MAXOBJECTNAME];
        char szType         [MAXOBJECTNAME];
        char tbName         [MAXOBJECTNAME];
        char tbCreator      [MAXOBJECTNAME];
        int  nColCount;
        long nRowCount;
        int  nPercentFree;
    exec sql end declare section; 

    x_strcpy (tbName,    lpTS->name);
    x_strcpy (tbCreator, lpTS->creator);

    //
    // Query Table info from IITABLES
    //
    exec sql repeated select
                NAME, CREATOR, TYPE, COLCOUNT, ROWCOUNT, PERCENTFREE
        into    :szTbName, :szTbCreator, :szType, :nColCount,
                :nRowCount, :nPercentFree
        from    SYSADM.SYSTABLES
        where   NAME = :tbName and CREATOR= :tbCreator;

    ManageSqlError (&sqlca); // Keep track of the SQL error if any
    iret = SQLCodeToDom (sqlca.sqlcode);
    if (iret != RES_SUCCESS)
        return iret;
    //
    // Put information in our Structure.
    //
    suppspace (szTbName);
    suppspace (szTbCreator);
    lstrcpy (lpTS->name,    szTbName);
    lstrcpy (lpTS->creator, szTbCreator);
    lpTS->rowcount = nRowCount;
    lpTS->colcount = nColCount;
    //    lpTS->pctfree  = nPercentFree;

    //
    // Query The Columns of the table from IICOLUMNS
    //
    iret = VDBA2xGetTableColumnsInfo (lpTS);
    if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
        return iret;
    //
    // Query the primary key constraint from SYSADM.SYSPKCONSTRAINTS
    //
    iret = OIDTGetTablePrimaryKeyInfo (lpTS);
    if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
    {
        lpTS->lpcolumnlist = TableColumn_Done (lpTS->lpcolumnlist);
        return iret;
    }
    lpColList = lpTS->lppkcolumnlist;
    while (lpColList)
    {
        lpCol = TableColumn_Search (lpTS->lpcolumnlist, lpColList->name);
        if (lpCol)
            lpCol->nPrimary = lpColList->sorted;
        lpColList = lpColList->next;
    }
    //
    // Query the Foreign key constraint from SYSADM.SYSFKCONSTRAINTS
    //
    iret = OIDTGetTableForeignKeyInfo (lpTS);
    if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
    {
        lpTS->lpcolumnlist = TableColumn_Done (lpTS->lpcolumnlist);
        return iret;
    }
#endif
	return RES_SUCCESS;
}



//
// lpIndex : pointer to the index structure to receive information about index:
// lpszConstraint: The constraint name.
int VDBA2xGetIndexConstraint (LPINDEXOPTION lpIndex, LPCTSTR lpszConstraint)
{
	int iret;
	//
	// Host Variable Declarations
	//
	EXEC SQL BEGIN DECLARE SECTION;
		char szConstraintIn  [MAXOBJECTNAME];
		
		char szConstraint    [MAXOBJECTNAME];
		char szSchema        [MAXOBJECTNAME];
		char szIndex         [MAXOBJECTNAME];
		
		char szTable         [MAXOBJECTNAME];
		char szTableOwner    [MAXOBJECTNAME];
		char szLocation      [MAXOBJECTNAME];
		char szStructure     [MAXOBJECTNAME];
		char szMultiLocations[16];
		int  nFillfactor;
		int  nLeaffill;
		int  nNonleaffill;
		long nAllocation;
		long nExtend;
		long nMinPages;
		long nMaxPages;
		int  ilocseq;
	EXEC SQL END DECLARE SECTION; 

	x_strcpy (szConstraintIn, lpszConstraint);
	iret= RES_SUCCESS;
	
	EXEC SQL repeated SELECT DISTINCT
			A.CONSTRAINT_NAME,
			A.SCHEMA_NAME,
			A.INDEX_NAME,
			B.TABLE_NAME,
			B.TABLE_OWNER,
			B.TABLE_DFILLPCT,
			B.TABLE_LFILLPCT,
			B.TABLE_IFILLPCT,
			B.ALLOCATION_SIZE,
			B.EXTEND_SIZE,
			B.TABLE_MINPAGES,
			B.TABLE_MAXPAGES,
			B.STORAGE_STRUCTURE,
			B.LOCATION_NAME,
			B.MULTI_LOCATIONS
		INTO
			:szConstraint, 
			:szSchema, 
			:szIndex,
			:szTable,
			:szTableOwner,
			:nFillfactor,
			:nLeaffill,
			:nNonleaffill,
			:nAllocation,
			:nExtend,
			:nMinPages,
			:nMaxPages,
			:szStructure,
			:szLocation,
			:szMultiLocations
		FROM 
			IICONSTRAINT_INDEXES A, 
			IITABLES B
		WHERE   
			A.CONSTRAINT_NAME = :szConstraintIn AND 
			A.SCHEMA_NAME     = B.TABLE_OWNER   AND
			A.INDEX_NAME      = B.TABLE_NAME;

	ManageSqlError (&sqlca); // Keep track of the SQL error if any
	iret = SQLCodeToDom (sqlca.sqlcode);
	if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
	{
		memset (lpIndex, 0, sizeof (INDEXOPTION));
		return iret;
	}
	suppspace (szConstraint);
	suppspace (szSchema);
	suppspace (szIndex);
	suppspace (szTable);
	suppspace (szTableOwner);
	suppspace (szLocation);
	suppspace (szMultiLocations);
	suppspace (szStructure);

	lpIndex->bAlter = TRUE;
	lstrcpy (lpIndex->tchszIndexName, szIndex);
	lstrcpy (lpIndex->tchszStructure, szStructure);
	if (lstrcmpi (szStructure, "BTREE") == 0)
	{
		wsprintf (lpIndex->tchszLeaffill,   "%d", nLeaffill);
		wsprintf (lpIndex->tchszNonleaffill,"%d", nNonleaffill);
	}
	if (lstrcmpi (szStructure, "HASH") == 0)
	{
		wsprintf (lpIndex->tchszMinPage,    "%d", nMinPages);
		wsprintf (lpIndex->tchszMaxPage,    "%d", nMaxPages);
	}
	wsprintf (lpIndex->tchszFillfactor, "%d", nFillfactor);
	wsprintf (lpIndex->tchszAllocation, "%d", nAllocation);
	wsprintf (lpIndex->tchszExtend,     "%d", nExtend);

	if (szLocation[0])
		lpIndex->pListLocation = StringList_Add (lpIndex->pListLocation, (LPCTSTR)szLocation);
	//
	// Get the multi-locations:
	if (szMultiLocations[0] && (szMultiLocations[0] == 'Y' || szMultiLocations[0] == 'y'))
	{
		iret=RES_SUCCESS;
		EXEC SQL repeated SELECT
				LOCATION_NAME,
				LOC_SEQUENCE 
			INTO
				:szLocation,
				:ilocseq
			FROM
				IIMULTI_LOCATIONS
			WHERE
				TABLE_NAME  = :szIndex AND TABLE_OWNER = :szSchema
			ORDER BY 
				LOC_SEQUENCE DESC;
		EXEC SQL BEGIN;
			ManageSqlError(&sqlca); // Keep track of the SQL error if any
			suppspace(szLocation);
			if (szLocation[0]) 
				lpIndex->pListLocation = StringList_Add (lpIndex->pListLocation, (LPCTSTR)szLocation);
		EXEC SQL END;
	}

	ManageSqlError (&sqlca); // Keep track of the SQL error if any
	iret = SQLCodeToDom(sqlca.sqlcode);
	if (iret != RES_SUCCESS)
	{
		lpIndex->pListLocation = StringList_Done(lpIndex->pListLocation);
		memset (lpIndex, 0, sizeof (INDEXOPTION));
		return iret;
	}
	return RES_SUCCESS;
}


//
// lpszTable : Table name
// lpszTableOwner: Table owner
// pError: if not, receives "0": error. "1": success.
// RETURN: list of constraint index enforcement of a table.
LPSTRINGLIST VDBA2xGetIndexConstraints (LPCTSTR lpszTable, LPCTSTR lpszTableOwner, int* pError)
{
	int iret;
	LPSTRINGLIST lpListIndex = NULL;
	//
	// Host Variable Declarations
	//
	EXEC SQL BEGIN DECLARE SECTION;
		char szSchema        [MAXOBJECTNAME];
		char szIndex         [MAXOBJECTNAME];
		char szTableIn       [MAXOBJECTNAME];
		char szTableOwnerIn  [MAXOBJECTNAME];
	EXEC SQL END DECLARE SECTION; 
	if (pError)
		*pError = 1;
	lstrcpy (szTableIn, lpszTable);
	lstrcpy (szTableOwnerIn, lpszTableOwner);
	iret= RES_SUCCESS;
	
	EXEC SQL repeated SELECT DISTINCT
			A.SCHEMA_NAME,
			A.INDEX_NAME
		INTO
			:szSchema,
			:szIndex
		FROM 
			IICONSTRAINT_INDEXES A, 
			IICONSTRAINTS B
		WHERE
			A.CONSTRAINT_NAME = B.CONSTRAINT_NAME AND 
			B.TABLE_NAME      = :szTableIn AND
			B.SCHEMA_NAME     = :szTableOwnerIn;

	EXEC SQL BEGIN;
		ManageSqlError (&sqlca); // Keep track of the SQL error if any
		iret = SQLCodeToDom (sqlca.sqlcode);
		if (iret != RES_SUCCESS && iret != RES_ENDOFDATA)
		{
			lpListIndex = StringList_Done (lpListIndex);
			if (pError) *pError = 0;
			return NULL;
		}
		suppspace (szSchema);
		suppspace (szIndex);
		lpListIndex = StringList_Add (lpListIndex, szIndex);
	EXEC SQL END;
	ManageSqlError (&sqlca); // Keep track of the SQL error if any
	iret = SQLCodeToDom(sqlca.sqlcode);
	if (iret != RES_SUCCESS)
	{
		lpListIndex = StringList_Done (lpListIndex);
		if (pError) *pError = 0;
		return NULL;
	}
	return lpListIndex;
}


//
// Get the index enforcement of the constraint in 'lpCnst':
// Fill up the fields 'tchszIndex' and 'tchszSchema' of 'lpCnst'.
void VDBA2xGetIndexEnforcement (LPCONSTRAINTPARAMS lpCnst)
{
	int iret = RES_ERR;
	//
	// Host Variable Declarations
	//
	EXEC SQL BEGIN DECLARE SECTION;
		char szSchema        [MAXOBJECTNAME];
		char szIndex         [MAXOBJECTNAME];
		char szConstraintIn  [MAXOBJECTNAME];
	EXEC SQL END DECLARE SECTION; 
	if (!lpCnst)
		return;
	if (!lpCnst->tchszConstraint[0])
		return;
	lpCnst->tchszIndex [0] = '\0';
	lpCnst->tchszSchema[0] = '\0';
	lstrcpy (szConstraintIn, lpCnst->tchszConstraint);
	iret= RES_SUCCESS;
	
	EXEC SQL repeated SELECT DISTINCT
			SCHEMA_NAME,
			INDEX_NAME
		INTO
			:szSchema,
			:szIndex
		FROM 
			IICONSTRAINT_INDEXES
		WHERE
			CONSTRAINT_NAME = :szConstraintIn;

	ManageSqlError (&sqlca); // Keep track of the SQL error if any
	iret = SQLCodeToDom (sqlca.sqlcode);
	if (iret != RES_SUCCESS)
		return;
	suppspace (szSchema);
	suppspace (szIndex);
	lstrcpy (lpCnst->tchszIndex, szIndex);
	lstrcpy (lpCnst->tchszSchema, szSchema);

	ManageSqlError (&sqlca); // Keep track of the SQL error if any
	iret = SQLCodeToDom(sqlca.sqlcode);
	if (iret != RES_SUCCESS)
		return;
}



int MonitorGetServerInfo(LPSERVERDATAMAX pServerDataMax)
{
  exec sql begin declare section;
    long max_conn;
    char bufwhere[MAXOBJECTNAME];
  exec sql end declare section; 

  wsprintf(bufwhere,"%%%s%%",pServerDataMax->serverdatamin.listen_address);

  exec sql repeated select max_connections
           into   :max_conn
           from   ima_dbms_server_parameters
           where  server_name like :bufwhere;
  exec sql begin;
    ManageSqlError(&sqlca);
    pServerDataMax->active_max_sessions=max_conn;
  exec sql end;
  ManageSqlError(&sqlca);

  return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorGetAllServerInfo(LPSERVERDATAMAX pServerDataMax)
{
  exec sql begin declare section;
    long num_sess,max_sess,num_active,max_active;
    char bufwhere[MAXOBJECTNAME];
  exec sql end declare section; 

  wsprintf(bufwhere,"%%%s%%",pServerDataMax->serverdatamin.listen_address);

  exec sql execute procedure ima_set_server_domain;
  ManageSqlError(&sqlca);
      
  exec sql commit;
  ManageSqlError(&sqlca); 
      
  exec sql execute procedure ima_set_vnode_domain;
  ManageSqlError(&sqlca); 
      
  exec sql commit;
  ManageSqlError(&sqlca);

  exec sql repeated select blk_num_sessions, blk_max_sessions, blk_num_active, blk_max_active
           into   :num_sess       , :max_sess       , :num_active   , :max_active
           from   ima_block_info
           where  server like :bufwhere;
  exec sql begin;
    ManageSqlError(&sqlca);
    pServerDataMax->current_connections     = num_sess;
    pServerDataMax->current_max_connections = max_sess;
    pServerDataMax->active_current_sessions = num_active;
    pServerDataMax->active_max_sessions     = max_active;
  exec sql end;
  ManageSqlError(&sqlca);
  if ( sqlca.sqlcode != 0 ) { 
    pServerDataMax->current_connections     = -1;
    pServerDataMax->current_max_connections = -1;
    pServerDataMax->active_current_sessions = -1;
    pServerDataMax->active_max_sessions     = -1;
  }
return SQLCodeToDom(sqlca.sqlcode);
}

int OldMonitorGetDetailSessionInfo(LPSESSIONDATAMAX psessdta)
{
  exec sql begin declare section;
    char bufloc[1001];
    char bufnew1[MAXOBJECTNAME];
    char bufnew2[MAXOBJECTNAME];
    char bufnew3[MAXOBJECTNAME];
    char bufnew4[MAXOBJECTNAME];
    char bufnew5[MAXOBJECTNAME];
    char bufnew6[MAXOBJECTNAME];
    char bufnew7[MAXOBJECTNAME];
    char bufnew8[MAXOBJECTNAME];
    char bufnew9[MAXOBJECTNAME];
    char bufnew10[MAXOBJECTNAME];
    char bufnew11[MAXOBJECTNAME];
    char bufev1[MAXOBJECTNAME];
	int iserverpid = (int) psessdta->sessiondatamin.server_pid;
  exec sql end declare section; 
  x_strcpy(bufev1,psessdta->sessiondatamin.session_id);
  if (bufev1[0] == '\0')
    return RES_ERR;
  exec sql repeated select b.session_mask  ,a.db_owner         ,a.session_group,
                  a.session_role  ,a.session_activity ,a.activity_detail,
                  a.session_query ,a.client_host      ,a.client_pid,
                  a.client_terminal,a.client_user     ,a.client_connect_string
             into :bufnew1        ,:bufnew2           ,:bufnew3 ,
                  :bufnew4        ,:bufnew5           ,:bufnew6 ,
                  :bufloc         ,:bufnew7           ,:bufnew8 ,
                  :bufnew9        ,:bufnew10          ,:bufnew11
             from ima_server_sessions a,
                  ima_server_sessions_extra b
            where a.session_id = :bufev1 and
                  a.session_id = b.session_id and
				  a.server_pid = :iserverpid and 
				  trim(a.server) = trim(b.Server)

				  ;
  exec sql begin;
    ManageSqlError(&sqlca);

    fstrncpy(psessdta->client_host, bufnew7,
                                      sizeof(psessdta->client_host));
    fstrncpy(psessdta->client_pid , bufnew8,
                                      sizeof(psessdta->client_pid));
    fstrncpy(psessdta->client_terminal, bufnew9,
                                      sizeof(psessdta->client_terminal));
    fstrncpy(psessdta->client_user, bufnew10,
                                      sizeof(psessdta->client_user));
    fstrncpy(psessdta->client_connect_string, bufnew11,
                                      sizeof(psessdta->client_connect_string));

    fstrncpy(psessdta->session_mask, bufnew1,
                                      sizeof(psessdta->session_mask));
    suppspace(bufnew2);
    if (bufnew2[0] == '\0')
      x_strcpy(psessdta->db_owner,ResourceString(IDS_NONE_));
    else
      fstrncpy(psessdta->db_owner, bufnew2,
                                      sizeof(psessdta->db_owner));
    suppspace(bufnew3);
    if (bufnew3[0] == '\0')
      x_strcpy(psessdta->session_group,ResourceString(IDS_NONE_));
    else
      fstrncpy(psessdta->session_group, bufnew3,
                                      sizeof(psessdta->session_group));
    suppspace(bufnew4);
    if (bufnew4[0] == '\0')
      x_strcpy(psessdta->session_role,ResourceString(IDS_NONE_));
    else
      fstrncpy(psessdta->session_role, bufnew4,
                                      sizeof(psessdta->session_role));
    suppspace(bufnew5);
    if (bufnew4[0] == '\0')
      x_strcpy(psessdta->session_activity,ResourceString(IDS_NONE_));
    else
      fstrncpy(psessdta->session_activity, bufnew5,
                                      sizeof(psessdta->session_activity));
    fstrncpy(psessdta->activity_detail, bufnew6,
                                      sizeof(psessdta->activity_detail));
    fstrncpy(psessdta->session_full_query, bufloc,
                                      sizeof(psessdta->session_full_query));
  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorGetDetailSessionInfo(LPSESSIONDATAMAX psessdta)
{
  exec sql begin declare section;
    char bufloc[1001];
    char bufnew1[MAXOBJECTNAME];
    char bufnew2[MAXOBJECTNAME];
    char bufnew3[MAXOBJECTNAME];
    char bufnew4[MAXOBJECTNAME];
    char bufnew5[MAXOBJECTNAME];
    char bufnew6[MAXOBJECTNAME];
    char bufnew7[MAXOBJECTNAME];
    char bufnew8[MAXOBJECTNAME];
    char bufnew9[MAXOBJECTNAME];
    char bufnew10[MAXOBJECTNAME];
    char bufnew11[MAXOBJECTNAME];
    char bufev1[MAXOBJECTNAME];
	char bufdesc[256+1];
	short null1;
	int iserverpid = (int) psessdta->sessiondatamin.server_pid;
  exec sql end declare section; 
  x_strcpy(bufev1,psessdta->sessiondatamin.session_id);
  if (bufev1[0] == '\0')
    return RES_ERR;

  if ( GetOIVers() == OIVERS_NOTOI ||
       GetOIVers() == OIVERS_12 ||
       GetOIVers() == OIVERS_20 ) {
        x_strcpy(psessdta->session_description,"<N/A>");
        return OldMonitorGetDetailSessionInfo(psessdta);
  }

  exec sql repeated select b.session_mask  ,a.db_owner         ,a.session_group,
                  a.session_role  ,a.session_activity ,a.activity_detail,
                  a.session_query ,a.client_host      ,a.client_pid,
                  a.client_terminal,a.client_user     ,a.client_connect_string,
				  c.session_desc
             into :bufnew1        ,:bufnew2           ,:bufnew3 ,
                  :bufnew4        ,:bufnew5           ,:bufnew6 ,
                  :bufloc         ,:bufnew7           ,:bufnew8 ,
                  :bufnew9        ,:bufnew10          ,:bufnew11,
				  :bufdesc:null1
             from ima_server_sessions a,
                  ima_server_sessions_extra b,
				  ima_server_sessions_desc c
            where a.session_id = :bufev1 and
                  a.session_id = b.session_id and
				  a.server_pid = :iserverpid and
				  a.session_id = c.session_id and 
				  trim(a.server) = trim(b.server) and
				  trim(a.server) = trim(c.server);
  exec sql begin;
    ManageSqlError(&sqlca);

    fstrncpy(psessdta->client_host, bufnew7,
                                      sizeof(psessdta->client_host));
    fstrncpy(psessdta->client_pid , bufnew8,
                                      sizeof(psessdta->client_pid));
    fstrncpy(psessdta->client_terminal, bufnew9,
                                      sizeof(psessdta->client_terminal));
    fstrncpy(psessdta->client_user, bufnew10,
                                      sizeof(psessdta->client_user));
    fstrncpy(psessdta->client_connect_string, bufnew11,
                                      sizeof(psessdta->client_connect_string));

    fstrncpy(psessdta->session_mask, bufnew1,
                                      sizeof(psessdta->session_mask));
    suppspace(bufnew2);
    if (bufnew2[0] == '\0')
      x_strcpy(psessdta->db_owner,ResourceString(IDS_NONE_));
    else
      fstrncpy(psessdta->db_owner, bufnew2,
                                      sizeof(psessdta->db_owner));
    suppspace(bufnew3);
    if (bufnew3[0] == '\0')
      x_strcpy(psessdta->session_group,ResourceString(IDS_NONE_));
    else
      fstrncpy(psessdta->session_group, bufnew3,
                                      sizeof(psessdta->session_group));
    suppspace(bufnew4);
    if (bufnew4[0] == '\0')
      x_strcpy(psessdta->session_role,ResourceString(IDS_NONE_));
    else
      fstrncpy(psessdta->session_role, bufnew4,
                                      sizeof(psessdta->session_role));
    suppspace(bufnew5);
    if (bufnew4[0] == '\0')
      x_strcpy(psessdta->session_activity,ResourceString(IDS_NONE_));
    else
      fstrncpy(psessdta->session_activity, bufnew5,
                                      sizeof(psessdta->session_activity));
    fstrncpy(psessdta->activity_detail, bufnew6,
                                      sizeof(psessdta->activity_detail));
    fstrncpy(psessdta->session_full_query, bufloc,
                                      sizeof(psessdta->session_full_query));
    if (null1==-1)
       x_strcpy(psessdta->session_description,"<N/A>"); 
	   else
    fstrncpy(psessdta->session_description, bufdesc,
                                      sizeof(psessdta->session_description));

 
  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorGetDetailLockListInfo( LPLOCKLISTDATAMAX plocklistdta )
{
  exec sql begin declare section;
    long  locklst_related_llb_id_id;
    long  locklst_related_count;
    long  locklst_id;
  exec sql end declare section;
    locklst_id = plocklistdta->locklistdatamin.locklist_id;
  exec sql repeated select locklist_related_llb_id_id,locklist_related_count
             into :locklst_related_llb_id_id,:locklst_related_count
             from ima_locklists
            where locklist_id = :locklst_id;
  exec sql begin;
    ManageSqlError(&sqlca);
    plocklistdta->locklist_related_llb_id_id = locklst_related_llb_id_id;
    plocklistdta->locklist_related_count     = locklst_related_count;
  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorGetDetailLockSummary( LPLOCKSUMMARYDATAMIN pLckSummary )
{
  exec sql begin declare section;
  long sumlck1,sumlck2,sumlck3,sumlck4,sumlck5,sumlck6,sumlck7,sumlck8,
       sumlck9,sumlck10,sumlck11,sumlck12,sumlck13,sumlck14,sumlck15,
       sumlck16,sumlck17,sumlck18,sumlck19,sumlck20,sumlck21,sumlck22,
       sumlck23;
  exec sql end declare section;
  exec sql repeated select lkd_stat_create_list  ,lkd_stat_release_all ,lkd_llb_inuse        ,
                  lock_lists_remaining  ,lock_lists_available ,lkd_rsh_size         ,
                  lkd_lkh_size          ,lkd_stat_request_new ,lkd_stat_request_cvt ,
                  lkd_stat_convert      ,lkd_stat_release     ,lkd_stat_cancel      ,
                  lkd_stat_rlse_partial ,lkd_lkb_inuse        ,locks_remaining      ,
                  locks_available       ,lkd_stat_dlock_srch  ,lkd_stat_cvt_deadlock,
                  lkd_stat_deadlock     ,lkd_stat_convert_wait,lkd_stat_wait        ,
                  lkd_max_lkb           ,lkd_rsb_inuse
             into :sumlck1              ,:sumlck2             ,:sumlck3 ,
                  :sumlck4              ,:sumlck5             ,:sumlck6 ,
                  :sumlck7              ,:sumlck8             ,:sumlck9 ,
                  :sumlck10             ,:sumlck11            ,:sumlck12,
                  :sumlck13             ,:sumlck14            ,:sumlck15,
                  :sumlck16             ,:sumlck17            ,:sumlck18,
                  :sumlck19             ,:sumlck20            ,:sumlck21,
                  :sumlck22             ,:sumlck23
             from ima_lk_summary_view;
  exec sql begin;
    ManageSqlError(&sqlca);
    pLckSummary->sumlck_lklist_created_list        = sumlck1;
    pLckSummary->sumlck_lklist_released_all        = sumlck2 +1L;
    pLckSummary->sumlck_lklist_inuse               = sumlck3 -1L;
    pLckSummary->sumlck_lklist_remaining           = sumlck4 +1L;
    pLckSummary->sumlck_total_lock_lists_available = sumlck5;
    pLckSummary->sumlck_rsh_size                   = sumlck6;
    pLckSummary->sumlck_lkh_size                   = sumlck7;
    pLckSummary->sumlck_lkd_request_new            = sumlck8;
    pLckSummary->sumlck_lkd_request_cvt            = sumlck9;
    pLckSummary->sumlck_lkd_convert                = sumlck10;
    pLckSummary->sumlck_lkd_release                = sumlck11+2L;
    pLckSummary->sumlck_lkd_cancel                 = sumlck12;
    pLckSummary->sumlck_lkd_rlse_partial           = sumlck13;
    pLckSummary->sumlck_lkd_inuse                  = sumlck14-1L; /* current one normally released once request done */
    pLckSummary->sumlck_locks_remaining            = sumlck15+2L; /*    "    */
    pLckSummary->sumlck_locks_available            = sumlck16;
    pLckSummary->sumlck_lkd_dlock_srch             = sumlck17;
    pLckSummary->sumlck_lkd_cvt_deadlock           = sumlck18;
    pLckSummary->sumlck_lkd_deadlock               = sumlck19;
    pLckSummary->sumlck_lkd_convert_wait           = sumlck20;
    pLckSummary->sumlck_lkd_wait                   = sumlck21;
    pLckSummary->sumlck_lkd_max_lkb                = sumlck22;
    pLckSummary->sumlck_lkd_rsb_inuse              = sumlck23-2L; /* current one normally released once request done */
  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}
int MonitorGetDetailLogSummary( LPLOGSUMMARYDATAMIN pLogSummary )
{
  exec sql begin declare section;
  long db_adds, db_removes, tx_begins, tx_ends, log_writes,
       log_write_ios, log_read_ios, log_waits, log_split_waits,
       log_free_waits, log_stall_waits, log_forces, log_group_commit,
       log_group_count, check_commit_timer, timer_write, kbytes_written,
       inconsistent_db;
  exec sql end declare section;
  exec sql  repeated select lgd_stat_add           ,lgd_stat_remove      ,lgd_stat_begin,
                  lgd_stat_end           ,lgd_stat_write       ,lgd_stat_writeio,
                  lgd_stat_readio        ,lgd_stat_wait        ,lgd_stat_split,
                  lgd_stat_free_wait     ,lgd_stat_stall_wait  ,lgd_stat_force,
                  lgd_stat_group_force   ,lgd_stat_group_count ,lgd_stat_pgyback_check,
                  lgd_stat_pgyback_write ,lgd_stat_kbytes      ,lgd_stat_inconsist_db
             into
                  :db_adds               ,:db_removes          ,:tx_begins,
                  :tx_ends               ,:log_writes          ,:log_write_ios,
                  :log_read_ios          ,:log_waits           ,:log_split_waits,
                  :log_free_waits        ,:log_stall_waits     ,:log_forces,
                  :log_group_commit      ,:log_group_count     ,:check_commit_timer,
                  :timer_write           ,:kbytes_written      ,:inconsistent_db
             from 
                  ima_lgmo_lgd
             where
                  vnode = dbmsinfo('ima_vnode');
  exec sql begin;

    pLogSummary->lgd_add           = db_adds;
    pLogSummary->lgd_remove        = db_removes;
    pLogSummary->lgd_begin         = tx_begins;
    pLogSummary->lgd_end           = tx_ends+1; /* current tx will be ended at display time*/
    pLogSummary->lgd_write         = log_writes;
    pLogSummary->lgd_writeio       = log_write_ios;
    pLogSummary->lgd_readio        = log_read_ios;
    pLogSummary->lgd_wait          = log_waits;
    pLogSummary->lgd_split         = log_split_waits;
    pLogSummary->lgd_free_wait     = log_free_waits;
    pLogSummary->lgd_stall_wait    = log_stall_waits;
    pLogSummary->lgd_force         = log_forces;
    pLogSummary->lgd_group_force   = log_group_commit;
    pLogSummary->lgd_group_count   = log_group_count;
    pLogSummary->lgd_pgyback_check = check_commit_timer;
    pLogSummary->lgd_pgyback_write = timer_write;
    pLogSummary->lgd_kbytes        = kbytes_written /2;/*value is number of */
                                                       /*blocks of 512 bytes*/
    pLogSummary->lgd_inconsist_db  = inconsistent_db;

  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int deriveBlockNo(char *str)
{
  long  discard;
  int  block;
  if ((sscanf(str,"<%ld,%d,%d>",&discard,&block,&discard)) != 3) {
    return(-1);
  } 
  return(block);
}


int MonitorGetDetailLogHeader( LPLOGHEADERDATAMIN pLogHeader )
{
  exec sql begin declare section;
    long block_count, block_size, buf_count, logfull_interval,
         abort_interval, cp_interval, reserved_blocks;
    char begin_addr[31];
    char end_addr[31];
    char cp_addr[31];
    char status[101]; 
  exec sql end declare section;
  exec sql repeated select lgd_status      ,lfb_hdr_lgh_count     ,lfb_hdr_lgh_size,
                  lfb_buf_cnt     ,lfb_hdr_lgh_l_logfull ,lfb_hdr_lgh_l_abort,
                  lfb_hdr_lgh_l_cp,lfb_hdr_lgh_begin     ,lfb_hdr_lgh_end,
                  lfb_hdr_lgh_cp  ,lfb_reserved_space
             into :status         ,:block_count          ,:block_size,
                  :buf_count      ,:logfull_interval     ,:abort_interval,
                  :cp_interval    ,:begin_addr           ,:end_addr,
                  :cp_addr        ,:reserved_blocks
             from ima_lgmo_lgd lgd,ima_lgmo_lfb lfb
            where lgd.vnode = lfb.vnode
                  and lfb.vnode = dbmsinfo('ima_vnode');
  exec sql begin;
	ManageSqlError(&sqlca);
    x_strcpy( pLogHeader->status     , status);
    x_strcpy( pLogHeader->begin_addr , begin_addr);
    x_strcpy( pLogHeader->end_addr   , end_addr);
    x_strcpy( pLogHeader->cp_addr    , cp_addr);
    pLogHeader->block_count       = block_count;
    pLogHeader->block_size        = block_size;
    pLogHeader->buf_count         = buf_count;
    pLogHeader->logfull_interval  = logfull_interval;
    pLogHeader->abort_interval    = abort_interval;
    pLogHeader->cp_interval       = cp_interval;
    pLogHeader->reserved_blocks   = reserved_blocks / block_size;

    /*
    ** derive the rest of the required information
    */
    pLogHeader->begin_block = deriveBlockNo(pLogHeader->begin_addr);
    pLogHeader->end_block   = deriveBlockNo(pLogHeader->end_addr);
    pLogHeader->cp_block    = deriveBlockNo(pLogHeader->cp_addr);

    pLogHeader->inuse_blocks = (pLogHeader->end_block > pLogHeader->begin_block) ?
                                pLogHeader->end_block - pLogHeader->begin_block :
                                (pLogHeader->block_count - pLogHeader->begin_block) +
                                pLogHeader->end_block;
    pLogHeader->avail_blocks = pLogHeader->block_count - (pLogHeader->reserved_blocks +
                                                          pLogHeader->inuse_blocks);
    if ((pLogHeader->cp_block +
         pLogHeader->cp_interval) > pLogHeader->block_count) {
      pLogHeader->next_cp_block = (pLogHeader->cp_block +
                                   pLogHeader->cp_interval) -
                                   pLogHeader->block_count;
    } else {
      pLogHeader->next_cp_block = (pLogHeader->cp_block +
                                   pLogHeader->cp_interval);
    }
  exec sql end;
  ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int DBA_ShutdownServer ( LPSERVERDATAMIN pServer , int ShutdownType)
{
  exec sql begin declare section;
    char servername[MAXOBJECTNAME];
    char vnode[MAXOBJECTNAME];
  exec sql end declare section;

  if ((x_strcmp(pServer->server_class,"IINMSVR") == 0) ||
      (x_strcmp(pServer->server_class,"IUSVR"  ) == 0) ||
      (x_strcmp(pServer->server_class,"COMSVR" ) == 0) )  {
      TS_MessageBox(TS_GetFocus(),
                 ResourceString(IDS_ERR_ONLY_ING_OR_STAR_CANBESTPD),
                 NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      return RES_ERR;
    }
  
  exec sql repeated select dbmsinfo('ima_vnode') into :vnode;
  ManageSqlError(&sqlca); 
  
  exec sql commit;
  ManageSqlError(&sqlca); 

  wsprintf(servername,"%s::/@%s",vnode,pServer->listen_address);

  switch (ShutdownType) {
    case SHUTDOWN_HARD:
    {
      char szSqlStm[MAXOBJECTNAME*3];
      int ires;

      //ZEROINIT(szSqlStm);
      //wsprintf(szSqlStm,"set lockmode session where readlock=nolock, timeout=1");
      //ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
      

      ZEROINIT(szSqlStm);
      wsprintf(szSqlStm,"execute procedure ima_set_server_domain");
      ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
      
      ZEROINIT(szSqlStm);
      wsprintf(szSqlStm,"execute procedure ima_set_vnode_domain");
      ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);

      ZEROINIT(szSqlStm);
      wsprintf(szSqlStm,"commit");
      ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);

      ZEROINIT(szSqlStm);
      wsprintf(szSqlStm,"set autocommit off");
      ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);

      ZEROINIT(szSqlStm);
      wsprintf(szSqlStm,"execute procedure ima_stop_server (server_id='%s')",servername);
      ires=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);

      return RES_SUCCESS;
      break;
    }
    case SHUTDOWN_SOFT:
        exec sql execute procedure ima_shut_server (server_id=:servername);
      break;
    default:
      return RES_ERR;
      break;
  }
  if (sqlca.sqlcode == 700L)  { // indicates that a message in database procedure has executed.
    ManageSqlError(&sqlca);
    return RES_ERR;
  }
  ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int DBA_RemoveSession ( LPSESSIONDATAMIN pSession , char *ServerName )
{
   exec sql begin declare section;
      char srvname[MAXOBJECTNAME];
      char session[MAXOBJECTNAME];
      char extradata[MAXOBJECTNAME];
   exec sql end declare section;

   if (pSession->session_id[0] == '\0' || ServerName == NULL || ServerName[0] == '\0' )
      return RES_ERR;

   x_strcpy (session,pSession->session_id);
   exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
   wsprintf(srvname,"%s::/@%s",extradata,ServerName);

   //x_strcpy (srvname,ServerName);
   
   exec sql execute procedure ima_remove_session (session_id = :session ,server_id=:srvname);
   Sleep( 1400); // give time to be done before refresh occurs

   ManageSqlError(&sqlca);

   if (sqlca.sqlcode == 700L)  { // indicates that a message in database procedure has executed.
      ManageSqlError(&sqlca);    // no row found in the database procedure
      return RES_ERR;
   }
   if (sqlca.sqlcode == -37000L) // the current session has removed
      return RES_SUCCESS;

   
   return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorFindCurrentSessionId(char * SessId)
{
   exec sql begin declare section;
      char session[MAXOBJECTNAME];
   exec sql end declare section;

   exec sql repeated select dbmsinfo('session_id') into :session;
   ManageSqlError(&sqlca);
   exec sql commit;
   x_strcpy (SessId,session);
   return SQLCodeToDom(sqlca.sqlcode);
}  

int MonitorGetDetailTransactionInfo   ( LPLOGTRANSACTDATAMAX lpTranDataMax)
{
   exec sql begin declare section;
      char sessid[33];
      char cp_addr[33],first_addr[33],last_addr[33];
      long state_force,state_wait,internal_pid,external_pid;
   exec sql end declare section;

   x_strcpy (sessid,lpTranDataMax->logtxdatamin.tx_transaction_id);
   exec sql repeated select tx_first_log_address, tx_last_log_address ,
                   tx_cp_log_address   , tx_state_force      ,
                   tx_state_wait       , tx_pr_id_id         ,
                   tx_server_pid
            into   :first_addr         , :last_addr          ,
                   :cp_addr            , :state_force        ,
                   :state_wait         , :internal_pid       ,
                   :external_pid
	         from
		             ima_log_transactions
            where tx_transaction_id = :sessid;
  exec sql begin;
    ManageSqlError(&sqlca);
    x_strcpy (lpTranDataMax->tx_first_log_address ,  first_addr);
    x_strcpy (lpTranDataMax->tx_last_log_address  ,  last_addr);
    x_strcpy (lpTranDataMax->tx_cp_log_address    ,  cp_addr);
    lpTranDataMax->tx_state_force       =  state_force;
    lpTranDataMax->tx_state_wait        =  state_wait;
    lpTranDataMax->tx_internal_pid      =  65536L+internal_pid;
    lpTranDataMax->tx_external_pid      =  external_pid;
  exec sql end;
  ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}


void * GetFirstSQLDA(void * desc)
{
   LPOBJECTLIST lpdescr= desc;
   return (((LPSQLROWSET) (lpdescr->lpObject))->lpsqlda);
}

/*
** GetCapabilitiesInfo(LPTABLEPARAMS pTblInfo,LPGATEWAYCAPABILITIESINFO pGWInfo)
**	-	This function is used for filled the structure GATEWAYCAPABILITIESINF :
**			1) open/sql syntaxe extended(for create table) is available on this gateway.
**			2) data type DATE is suported on this gateway.
**			3) owner.table format is supported on this gateway. 
**			4) DBevents is supported on Oracle gateway.
**	- input :
**		LPTABLEPARAMS pTblInfo.
**		LPGATEWAYCAPABILITIESINFO pGWInfo.
**	- Output :
**		Filled pGWInfo.
**	- return :
**		RES_ERR : - Error in select statement
**				  - Connection failed
**		RES_SUCCESS all it's OK.
*/
int GetCapabilitiesInfo(LPTABLEPARAMS pTblInfo ,LPGATEWAYCAPABILITIESINFO pGWInfo)
{
	EXEC SQL BEGIN DECLARE SECTION;
		char capValue[MAXOBJECTNAME];
		char capcap[MAXOBJECTNAME];
	EXEC SQL END DECLARE SECTION;
	BOOL Success = TRUE;

	EXEC SQL repeated SELECT cap_capability, cap_value 
		INTO :capcap, :capValue
		FROM iidbcapabilities
		WHERE cap_capability = 'SQL92_COMPLIANCE';
	if (sqlca.sqlcode < 0) {
		Success = FALSE;
		ManageSqlError(&sqlca);
		}
	suppspace(capValue);
	// doc says "ENTRY" or "NONE" - Found with Lionel Storck that the tuple may not exist
	if (x_strcmp(capValue, "ENTRY") == 0)
		pGWInfo->bCreateTableSyntaxeExtend = TRUE;
	else
		pGWInfo->bCreateTableSyntaxeExtend = FALSE;
	if (Success)
	{
		ZEROINIT(capValue);
		EXEC SQL repeated SELECT cap_capability, cap_value 
			INTO :capcap, :capValue
			FROM iidbcapabilities
			WHERE cap_capability = 'OPEN_SQL_DATES';
		if (sqlca.sqlcode < 0) {
			Success = FALSE;
			ManageSqlError(&sqlca);
			}
		suppspace(capValue);
		if (lstrcmpi (capValue, "LEVEL 1") == 0)
			pGWInfo->bDateTypeAvailable = TRUE;
		else
			pGWInfo->bDateTypeAvailable = FALSE;
		/* noifr01 03-Jun-99 : in order to workaround the fact that iidbcapabilities     */
		/* is not uptodate for all gateways, assume all gateways accept the date type.   */
		/* This seems anyhow to be the reality. If not, the SQL error will be available  */
		/* and the resulting error message will only be different                        */
		pGWInfo->bDateTypeAvailable = TRUE;

	}
	if (Success)
	{
		ZEROINIT(capValue);
		EXEC SQL repeated SELECT cap_capability, cap_value 
			INTO :capcap, :capValue
			FROM iidbcapabilities
			WHERE cap_capability = 'OWNER_NAME';
		if (sqlca.sqlcode < 0) {
			ManageSqlError(&sqlca);
			Success = FALSE;
		}
		suppspace(capValue);
		if (capValue[0] == 'Q' || capValue[0] == 'Y' )
			pGWInfo->bOwnerTableSupported = TRUE;
		else
			pGWInfo->bOwnerTableSupported= FALSE;
	}

	if (Success)
	{
		ZEROINIT(capValue);
		EXEC SQL repeated SELECT cap_capability, cap_value 
			INTO :capcap, :capValue
			FROM iidbcapabilities
			WHERE cap_capability = 'DBEVENTS'
			  OR  cap_capability = 'dbevents';
		if (sqlca.sqlcode < 0) {
			ManageSqlError(&sqlca);
			Success = FALSE;
		}
		suppspace(capValue);
		if (capValue[0] == 'Y' || capValue[0] == 'y' )
			pGWInfo->bGWOracleWithDBevents = TRUE;
		else
			pGWInfo->bGWOracleWithDBevents = FALSE;
	}

	if (!Success)
		return RES_ERR;
	return RES_SUCCESS;

}

int DBA_OpenCloseServer (LPSERVERDATAMIN pServer ,BOOL bOpen)
{
  exec sql begin declare section;
    char servername[MAXOBJECTNAME];
    char vnode[MAXOBJECTNAME];
  exec sql end declare section;

  char szSqlStm[MAXOBJECTNAME*3];
  int ires;

  ZEROINIT(szSqlStm);
  wsprintf(szSqlStm,"set autocommit off");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca); 

  exec sql repeated select dbmsinfo('ima_vnode') into :vnode;
  if (sqlca.sqlcode != 0)
    ManageSqlError(&sqlca);


  wsprintf(servername,"%s::/@%s",vnode,pServer->listen_address);

  ZEROINIT(szSqlStm);
  wsprintf(szSqlStm,"execute procedure ima_set_server_domain");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca); 

  ZEROINIT(szSqlStm);
  wsprintf(szSqlStm,"execute procedure ima_set_vnode_domain");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca); 

  ZEROINIT(szSqlStm);
  wsprintf(szSqlStm,"commit");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca); 


  if (bOpen)
  {
    ZEROINIT(szSqlStm);
    wsprintf(szSqlStm,"execute procedure ima_open_server (server_id='%s')",servername);
    ires=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);
  }
  else
  {
    ZEROINIT(szSqlStm);
    wsprintf(szSqlStm,"execute procedure ima_close_server (server_id='%s')",servername);
    ires=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);
  }
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca);

  ZEROINIT(szSqlStm);
  wsprintf(szSqlStm,"commit");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca);

  if (sqlca.sqlcode == 700L)  { // indicates that a message in database procedure has executed.
    ManageSqlError(&sqlca);
    return RES_ERR;
  }
  return SQLCodeToDom(sqlca.sqlcode);
}

// For Modify sql statement.
// Select from iirelation Phys_[in]consistent
//                        Log_[in]consistent
//                        Table_recovery_[dis]allowed.

static int VDBA2xGetObjectCorruptionInfo  (LPSTORAGEPARAMS lpTS)
{
    EXEC SQL BEGIN DECLARE SECTION;
        long  table_Corrupt;
        char ObjName[MAXOBJECTNAME];
        char ObjOwner[MAXOBJECTNAME];
    EXEC SQL END DECLARE SECTION;

    lstrcpy(ObjName , StringWithoutOwner(lpTS->objectname) );
    if (lpTS->nObjectType == OT_INDEX)
        lstrcpy(ObjOwner, lpTS->objectowner );
    else
        lstrcpy(ObjOwner, lpTS->szSchema );

    lpTS->bPhysInconsistent   = FALSE;
    lpTS->bLogInconsistent    = FALSE;
    lpTS->bRecoveryDisallowed = FALSE;

    EXEC SQL REPEATED SELECT relstat2
             INTO :table_Corrupt
             FROM iirelation
             WHERE relid = :ObjName
             AND relowner = :ObjOwner;

    if (sqlca.sqlcode < 0) {
        ManageSqlError(&sqlca);
        return SQLCodeToDom(sqlca.sqlcode);
    }

    if (table_Corrupt&TABLE_PHYS_INCONSISTENT)
        lpTS->bPhysInconsistent   = TRUE;
    if (table_Corrupt&TABLE_LOG_INCONSISTENT)
        lpTS->bLogInconsistent    = TRUE;
    if (table_Corrupt&TABLE_RECOVERY_DISALLOWED)
        lpTS->bRecoveryDisallowed = TRUE;

    return SQLCodeToDom(sqlca.sqlcode);
}

BOOL SQLGetDbmsSecurityB1Info()
{
	BOOL bSecB1 = FALSE;
	EXEC SQL BEGIN DECLARE SECTION;
	char szB1[16];
	EXEC SQL END DECLARE SECTION;

	szB1[0] = '\0';

	EXEC SQL SELECT DBMSINFO('b1_server') INTO :szB1; 

	ManageSqlError(&sqlca);
	if (sqlca.sqlcode == 0) 
		bSecB1 = ((szB1[0] == 'Y') || (szB1[0] == 'y'));

	return bSecB1;
}

int SQLGetComment( LPTSTR lpNode, LPTSTR szDBname, LPTSTR szTableName, LPTSTR szTableOwner,
                   LPTSTR *lpObjectComment, LPOBJECTLIST lpObjColumn)
{
    EXEC SQL BEGIN DECLARE SECTION;
    char longRemark[MAX_LENGTH_COMMENT+1];
    int  numSeq;
    char szOwner[MAXOBJECTNAME];
    char szTblName[MAXOBJECTNAME];
    char subOjectName[MAXOBJECTNAME];
    EXEC SQL END DECLARE SECTION;
    int nNodeHandle,nBufSize = 0;
    int iret = RES_SUCCESS;
    LPOBJECTLIST  ls;
    LPCOMMENTCOLUMN lpCol;
    nNodeHandle = GetVirtNodeHandle    (lpNode);

    lstrcpy(szTblName, (LPCTSTR)szTableName);
    lstrcpy(szOwner, (LPCTSTR)szTableOwner);

    if (GetOIVers() == OIVERS_NOTOI)
        return RES_SUCCESS; // this a gateway comment not available
    else {
        if (nNodeHandle >= 0)
            if (IsStarDatabase(nNodeHandle, (LPUCHAR)szDBname))
                return RES_SUCCESS; // this a star database comment not available
    }

    /* Get Comment on table */
    iret=RES_SUCCESS;
    EXEC SQL REPEATED SELECT long_remark, text_sequence
        INTO :longRemark, :numSeq
        FROM iidb_comments
        WHERE object_name = :szTblName
        AND object_owner = :szOwner
        AND object_type = 'T'
        ORDER BY text_sequence;
    EXEC SQL BEGIN;
        assert (numSeq == 1);   /* Always "1" */
        assert (*lpObjectComment == NULL);
        if (!*lpObjectComment) {
            nBufSize = lstrlen (longRemark) +1;
            *lpObjectComment = ESL_AllocMem (nBufSize);
        }
        if (!*lpObjectComment) {
            iret=RES_ERR;
            exec sql endselect;
        }
        lstrcpy(*lpObjectComment,longRemark);
    EXEC SQL END;
    if (sqlca.sqlcode < 0) {
        ManageSqlError(&sqlca);
        return SQLCodeToDom(sqlca.sqlcode);
    }
    if (iret == RES_ERR)
        return RES_ERR; /* Alloc Memory error */

    if(!lpObjColumn) // if no column list it is not necessary to get the column
        return iret; // comment.

    /* Get comment on column */
    EXEC SQL REPEATED SELECT subobject_name, long_remark, text_sequence
        INTO :subOjectName, :longRemark, :numSeq
        FROM iidb_subcomments
        WHERE object_name = :szTblName
        AND object_owner = :szOwner
        AND subobject_type = 'C'
        ORDER BY text_sequence;
    EXEC SQL BEGIN;
        assert (numSeq == 1);   /* Always "1" */
        suppspace(subOjectName);
        ls = lpObjColumn;
        while (ls) {
           lpCol=(LPCOMMENTCOLUMN)ls->lpObject;
           if (!x_stricmp(lpCol->szColumnName,subOjectName)) {
              assert (lpCol->lpszComment == NULL);
              if (!lpCol->lpszComment) {
                 nBufSize = lstrlen (longRemark) +1;
                 lpCol->lpszComment = ESL_AllocMem (nBufSize);
              }
              if (!lpCol->lpszComment) {
                 iret=RES_ERR;
                 exec sql endselect;
              }
              lstrcpy(lpCol->lpszComment,longRemark);
              break;
           }
           ls=ls->lpNext;
        }
    EXEC SQL END;
    if (sqlca.sqlcode < 0) {
         ManageSqlError(&sqlca);
        return SQLCodeToDom(sqlca.sqlcode);
    }

   return iret;
}

int  SQLGetColumnList(LPTABLEPARAMS lpTbl, LPTSTR lpSqlCommand)
{
    EXEC SQL BEGIN DECLARE SECTION;
    char szTblName[MAXOBJECTNAME];
    char szColName[MAXOBJECTNAME];
    char szOwner[MAXOBJECTNAME];
    EXEC SQL END DECLARE SECTION;
    int ires;
    LPCOLUMNPARAMS lpObj;
    LPOBJECTLIST lpColTemp=NULL;

    ourString2Lower(lpTbl->objectname);
    ourString2Lower(lpTbl->szSchema);

    lstrcpy(szTblName,lpTbl->objectname);
    lstrcpy(szOwner,lpTbl->szSchema);

    ires=ExecSQLImm(lpSqlCommand,FALSE, NULL, NULL, NULL);
    if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
        return RES_ERR;

    EXEC SQL REPEATED SELECT column_name
        INTO :szColName
        FROM iicolumns
        WHERE LOWERCASE(table_name) = :szTblName
        AND LOWERCASE(table_owner) = :szOwner;
    EXEC SQL BEGIN;

      lpColTemp = AddListObject(lpTbl->lpColumns, sizeof(COLUMNPARAMS));
      if (!lpColTemp) {
          // Need to free previously allocated objects.
          FreeObjectList(lpTbl->lpColumns);
          lpTbl->lpColumns = NULL;
          exec sql endselect;
      }
      else
          lpTbl->lpColumns = lpColTemp;
      lpObj = (LPCOLUMNPARAMS)lpTbl->lpColumns->lpObject;
      suppspace(szColName);
      lstrcpy(lpObj->szColumn,szColName);

    EXEC SQL END;

    if (sqlca.sqlcode < 0)
        ManageSqlError(&sqlca);

    return SQLCodeToDom(sqlca.sqlcode);
}

LPTSTR INGRESII_llDBMSInfo(LPTSTR lpszConstName, LPTSTR lpVer)
{
   EXEC SQL BEGIN DECLARE SECTION;
   char version[MAXOBJECTNAME];
   char* szConst = lpszConstName;
   EXEC SQL END DECLARE SECTION;

   EXEC SQL REPEATED SELECT DBMSINFO(:szConst) INTO :version;
   suppspace(version);


   ManageSqlError(&sqlca);
   if (sqlca.sqlcode!=0)
   {
       lstrcpy(lpVer, _T(""));
       return lpVer;
   }
   else
   {
       lstrcpy(lpVer, version);
       return lpVer;
   }
}

int  SQLGetTableStorageStructure(LPTABLEPARAMS lpTbl, LPTSTR lpSqlStruct)
{
    EXEC SQL BEGIN DECLARE SECTION;
        char szTblName[MAXOBJECTNAME];
        char szOwner[MAXOBJECTNAME];
        char szStructure[16+1];
    EXEC SQL END DECLARE SECTION;

    ourString2Lower(lpTbl->objectname);
    ourString2Lower(lpTbl->szSchema);

    lstrcpy(szTblName,lpTbl->objectname);
    lstrcpy(szOwner,lpTbl->szSchema);

    EXEC SQL REPEATED SELECT storage_structure
        INTO :szStructure
        FROM iitables
        WHERE LOWERCASE(table_name) = :szTblName
        AND LOWERCASE(table_owner) = :szOwner;
    EXEC SQL BEGIN;
        suppspace(szStructure);
        lstrcpy(lpSqlStruct,szStructure);
    EXEC SQL END;

    ManageSqlError(&sqlca);

    return SQLCodeToDom(sqlca.sqlcode);
}

int GetInfSequence(LPSEQUENCEPARAMS seqprm)
{

   EXEC SQL BEGIN DECLARE SECTION;
    char create_date[26];
    char modify_date[26];
    char data_type[8];
    short seq_length;
    char seq_precision[50];
    char start_value[50];
    char increment_value[50];
    char next_value[50];
    char min_value [50];
    char max_value [50];
    char cache_size[50];
    char cycle_flag[2];
    char order_flag[2];
    char *SequenceName=seqprm->objectname;
    char *SequenceOwner=seqprm->objectowner;
   EXEC SQL END DECLARE SECTION;

   ourString2Lower(SequenceName);
   ourString2Lower(SequenceOwner);

   EXEC SQL REPEATED SELECT 
                 create_date,
                 modify_date,
                 data_type,
                 seq_length,
                 char(seq_precision),
                 char(start_value),
                 char(increment_value),
                 char(next_value),
                 char(min_value),
                 char(max_value),
                 char(cache_size),
                 cycle_flag,
                 order_flag
            INTO :create_date,
                 :modify_date,
                 :data_type,
                 :seq_length,
                 :seq_precision,
                 :start_value,
                 :increment_value,
                 :next_value,
                 :min_value,
                 :max_value,
                 :cache_size,
                 :cycle_flag,
                 :order_flag
            FROM     iisequences
            WHERE    LOWERCASE(seq_name)  = :SequenceName
             AND     LOWERCASE(seq_owner) = :SequenceOwner;
   EXEC SQL BEGIN;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any

      suppspace(create_date);
      suppspace(modify_date);
      suppspace(data_type);
      suppspace(start_value);
      suppspace(increment_value);
      suppspace(next_value);
      suppspace(min_value);
      suppspace(max_value);
      suppspace(seq_precision);
      suppspace(cache_size);

      x_strcpy(seqprm->szseq_create, create_date);
      x_strcpy(seqprm->szseq_modify, modify_date);
      if (lstrcmp(data_type, "integer") == 0)
         seqprm->bDecimalType = FALSE;
      else
         seqprm->bDecimalType = TRUE;

      seqprm->iseq_length      = seq_length;
      x_strcpy(seqprm->szDecimalPrecision, seq_precision);
      x_strcpy(seqprm->szCacheSize, cache_size);
      x_strcpy(seqprm->szStartWith, start_value);
      x_strcpy(seqprm->szIncrementBy, increment_value);
      x_strcpy(seqprm->szNextValue, next_value);
      x_strcpy(seqprm->szMinValue, min_value);
      x_strcpy(seqprm->szMaxValue, max_value);

      if (cycle_flag[0] == 'Y' || cycle_flag[0] == 'y' )
        seqprm->bCycle  = TRUE;
      else
        seqprm->bCycle  = FALSE;

      if (order_flag[0] == 'Y' || order_flag[0] == 'y')
        seqprm->bOrder = TRUE;
      else
        seqprm->bOrder = FALSE;

   EXEC SQL END;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   return SQLCodeToDom(sqlca.sqlcode);
}

