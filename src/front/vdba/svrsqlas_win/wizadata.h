/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : wizadata.h: General classes or structures used by sql assistant.
**    Project  : Meta Data / COM Server (Sql Assistant)
**    Author   : Sotheavut UK (uk$so01)
**
** History:
**
** 10-Jul-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
**/

#if ! defined (SQL_WIZARD_DATA_HEADER)
#define SQL_WIZARD_DATA_HEADER



/* function families */
#define FF_DTCONVERSION       1
#define FF_NUMFUNCTIONS       2
#define FF_STRFUNCTIONS       3
#define FF_DATEFUNCTIONS      4
#define FF_AGGRFUNCTIONS      5
#define FF_IFNULLFUNC         6
#define FF_PREDICATES         7
#define FF_PREDICCOMB         8
#define FF_DATABASEOBJECTS    9 
#define FF_SUBSELECT         10 

#define OFF_MATHFUNCTIONS     1
#define OFF_FINFUNCTIONS      2
#define OFF_STRFUNCTIONS      3
#define OFF_DATEFUNCTIONS     4
#define OFF_AGGRFUNCTIONS     5
#define OFF_SPEFUNCTIONS      6
#define OFF_IFNULLFUNC        7
#define OFF_PREDICATES        8
#define OFF_PREDICCOMB        9
#define OFF_DATABASEOBJECTS  10
#define OFF_SUBSELECT        11

/* functions */
#define F_DT_BYTE             1    /* 1+  */
#define F_DT_C                2    /* 1+  */
#define F_DT_CHAR             3    /* 1+  */
#define F_DT_DATE             4    /* 1   */
#define F_DT_DECIMAL          5    /* 1++ */
#define F_DT_DOW              6    /* 1   */
#define F_DT_FLOAT4           7    /* 1   */
#define F_DT_FLOAT8           8    /* 1   */
#define F_DT_HEX              9    /* 1   */
#define F_DT_INT1            10    /* 1   */
#define F_DT_INT2            11    /* 1   */
#define F_DT_INT4            12    /* 1   */
#define F_DT_LONGBYTE        13    /* 1+  */
#define F_DT_LONGVARC        14    /* 1   */
#define F_DT_MONEY           15    /* 1   */
#define F_DT_OBJKEY          16    /* 1   */
#define F_DT_TABKEY          17    /* 1   */
#define F_DT_TEXT            18    /* 1+  */
#define F_DT_VARBYTE         19    /* 1+  */
#define F_DT_VARCHAR         20    /* 1+  */

#define F_NUM_ABS            21    /* 1   */
#define F_NUM_ATAN           22    /* 1   */
#define F_NUM_COS            23    /* 1   */
#define F_NUM_EXP            24    /* 1   */
#define F_NUM_LOG            25    /* 1   */
#define F_NUM_MOD            26    /* 2   */
#define F_NUM_SIN            27    /* 1   */
#define F_NUM_SQRT           28    /* 1   */

#define F_STR_CHAREXT        29    /* 2   */
#define F_STR_CONCAT         30    /* 2   */
#define F_STR_LEFT           31    /* 2   */
#define F_STR_LENGTH         32    /* 1   */
#define F_STR_LOCATE         33    /* 2   */
#define F_STR_LOWERCS        34    /* 1   */
#define F_STR_PAD            35    /* 1   */
#define F_STR_RIGHT          36    /* 2   */
#define F_STR_SHIFT          37    /* 2   */
#define F_STR_SIZE           38    /* 1   */
#define F_STR_SQUEEZE        39    /* 1   */
#define F_STR_TRIM           40    /* 1   */
#define F_STR_NOTRIM         41    /* 1   */
#define F_STR_UPPERCS        42    /* 1   */

#define F_DAT_DATETRUNC      43    /* 2   */
#define F_DAT_DATEPART       44    /* 2   */
#define F_DAT_GMT            45    /* 1   */
#define F_DAT_INTERVAL       46    /* 3   */
#define F_DAT_DATE           47    /* 1   */
#define F_DAT_TIME           48    /* 1   */

#define F_AGG_ANY            49    /* 1 + checkbox */
#define F_AGG_COUNT          50    /* 1 + checkbox */
#define F_AGG_SUM            51    /* 1 + checkbox */
#define F_AGG_AVG            52    /* 1 + checkbox */
#define F_AGG_MAX            53    /* 1 + checkbox */
#define F_AGG_MIN            54    /* 1 + checkbox */

#define F_IFN_IFN            55    /* 1   */

#define F_PRED_COMP          56    /* BOX     */
#define F_PRED_LIKE          57    /* BOX     */
#define F_PRED_BETWEEN       58    /* 3       */
#define F_PRED_IN            59    /* BOX     */
#define F_PRED_ANY           60    /* BOX     */
#define F_PRED_ALL           61    /* BOXSAME */
#define F_PRED_EXIST         62    /* BOX     */
#define F_PRED_ISNULL        63    /* 1       */

#define F_PREDCOMB_AND       64    /* 2       */
#define F_PREDCOMB_OR        65    /* 2       */
#define F_PREDCOMB_NOT       66    /* 1       */

#define F_DB_COLUMNS         67
#define F_DB_TABLES          68
#define F_DB_DATABASES       69
#define F_DB_USERS           70
#define F_DB_GROUPS          71
#define F_DB_ROLES           72
#define F_DB_PROFILES        73
#define F_DB_LOCATIONS       74
#define F_SUBSEL_SUBSEL      75

// Elb Dec 2, 96: dbareas and stogroup had been mapped on roles and profiles:
// need custom values for help
#define OF_DB_DBAREA        178
#define OF_DB_STOGROUP      179


typedef struct tagGenfuncParams 
{
	TCHAR FuncName [50];
	TCHAR HelpText1[256];
	TCHAR HelpText2[65];
	TCHAR HelpText3[65];

	int nbargsmin;
	int nbargsmax;

	TCHAR CaptionText1[40];
	TCHAR CaptionText2[40];
	TCHAR CaptionText3[40];

	TCHAR BetweenText1Left [40];
	TCHAR BetweenText1Right[40];
	TCHAR BetweenText2Left [40];
	TCHAR BetweenText2Right[40];

	int parmcat1; // category of parm 1
	int parmcat2;
	int parmcat3;

	TCHAR resultformat1[256];
	TCHAR resultformat2[50];
	TCHAR resultformat3[50];

} GENFUNCPARAMS, FAR * LPGENFUNCPARAMS;


class CaSQLComponent: public CObject 
{
public:
	CaSQLComponent(BOOL bArg, int nID, LPCTSTR lpszFName,  LPCTSTR lpszFullName, UINT nHelpID1, UINT nHelpID2 = -1)
	{
		m_bArgument = bArg;
		m_nFuncId = nID;
		m_strFuncName = lpszFName;
		m_strFullName = lpszFullName;
		m_nFhelp1IDS  = nHelpID1;
		m_nFhelp2IDS  = nHelpID2;
	}
	~CaSQLComponent(){}

	CString GetFunctionName(){return m_strFuncName;}
	CString GetFullName(){return m_strFullName;}
	UINT GetHelpID1(){return m_nFhelp1IDS;}
	BOOL HasArgument(){return m_bArgument;}
	int  GetID(){return m_nFuncId;}

protected:
	BOOL    m_bArgument;   // TRUE: Function requires argument
	int     m_nFuncId;     // Function Identifier
	CString m_strFuncName;   // Name of the function. EX: ABS
	CString m_strFullName;   // Full name (with arguments). EX: ABS(n)
	UINT    m_nFhelp1IDS;
	UINT    m_nFhelp2IDS;
};

class CaSQLFamily: public CObject
{
public:
	CaSQLFamily(LPCTSTR lpszFamily, int nID, CTypedPtrList<CObList, CaSQLComponent*>* pListComp)
	{
		m_strFamilyName = lpszFamily;
		m_nFamilyId = nID;
		m_pSqlComp = pListComp;
	}
	~CaSQLFamily(){}

	CString GetName(){return m_strFamilyName;}
	int GetID(){return m_nFamilyId;}
	CTypedPtrList<CObList, CaSQLComponent*>* GetListComponent(){return m_pSqlComp;}

protected:
	CString m_strFamilyName;
	int     m_nFamilyId;
	CTypedPtrList<CObList, CaSQLComponent*>* m_pSqlComp;

};



//
// HELP IDs for MANAGING THE CONTEXT HELP (the constants 2001, 2002 are from VDBA):
// ************************************************************************************************
#define IDH_ASSISTANT_MAIN      2000          // Main wizard
#define IDH_ASSISTANT_INS1      2001          // Insert Step 1
#define IDH_ASSISTANT_INS2      2002          // Insert Step 2
#define IDH_ASSISTANT_DEL1      2003          // Delete Step 1
#define IDH_ASSISTANT_UPD1      2004          // Update Step 1
#define IDH_ASSISTANT_UPD2      2005          // Update Step 2
#define IDH_ASSISTANT_UPD3      2006          // Update Step 3
#define IDH_ASSISTANT_SEL1      2007          // Select Step 1
#define IDH_ASSISTANT_SEL2      2008          // Select Step 2
#define IDH_ASSISTANT_SEL3      2009          // Select Step 3
//
// Expression Wizard:
// The main step has the HELP ID = 6015, and
// the other steps of the expression wizard has the Help ID = CaSQLComponent::GetID()
#define IDH_ASSISTANT_EXPR_MAIN 6015 // Main Expression Wizard Step 1




#endif // SQL_WIZARD_DATA_HEADER
