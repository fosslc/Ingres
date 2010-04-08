/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlepsht.cpp, Implementation File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (sqlassis.chm file)
**    Activate Help Button.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "sqlepsht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CString ResourceString (UINT nIDS)
{
	CString strText;
	strText.LoadString (nIDS);
	return strText;
}

void FillFunctionParameters (LPGENFUNCPARAMS lpFparam, CaSQLComponent* lpComp)
{
	memset(lpFparam, 0,sizeof(GENFUNCPARAMS));
	//
	// Function Name:
	lstrcpy (lpFparam->FuncName, lpComp->GetFullName());
	//
	// Help String:
	lstrcpy (lpFparam->HelpText1, ResourceString(lpComp->GetHelpID1()));
	lpFparam->nbargsmin = 1;
	lpFparam->nbargsmax = 1;

	switch (lpComp->GetID())
	{
	//
	// Data type conversion functions:
	case F_DT_BYTE:
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_OPT_LENGTH));
		lstrcpy(lpFparam->resultformat1, _T("byte(%s)"));
		lstrcpy(lpFparam->resultformat2, _T("byte(%s,%s)"));
		break;
	case F_DT_C :
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_OPT_LENGTH));
		lstrcpy(lpFparam->resultformat1, _T("c(%s)"));
		lstrcpy(lpFparam->resultformat2, _T("c(%s,%s)"));
		break;
	case F_DT_CHAR:
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_OPT_LENGTH));
		lstrcpy(lpFparam->resultformat1, _T("char(%s)"));
		lstrcpy(lpFparam->resultformat2, _T("char(%s,%s)"));
		break;
	case F_DT_DATE:
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("date(%s)"));
		break;
	case F_DT_DECIMAL:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 3;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lpFparam->parmcat3 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_OPT_PREC));
		lstrcpy(lpFparam->CaptionText3, ResourceString(IDS_ASSIST_OPT_SCALE));
		lstrcpy(lpFparam->resultformat1, _T("decimal(%s)"));
		lstrcpy(lpFparam->resultformat2, _T("decimal(%s,%s)"));
		lstrcpy(lpFparam->resultformat3, _T("decimal(%s,%s,%s)"));
		break;
	case F_DT_DOW:
		lpFparam->parmcat1 = FF_DATEFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("dow(%s)"));
		break;
	case F_DT_FLOAT4:
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("float4(%s)"));
		break;
	case F_DT_FLOAT8:
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("float8(%s)"));
		break;
	case F_DT_HEX :
		lpFparam->parmcat2 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("hex(%s)"));
		break;
	case F_DT_INT1:
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("int1(%s)"));
		break;
	case F_DT_INT2:
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("int2(%s)"));
		break;
	case F_DT_INT4:
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("int2(%s)"));
		break;
	case F_DT_LONGBYTE :
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_OPT_LENGTH));
		lstrcpy(lpFparam->resultformat1, _T("long_byte(%s)"));
		lstrcpy(lpFparam->resultformat2, _T("long_byte(%s,%s)"));
		break;
	case F_DT_LONGVARC :
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("long_varchar(%s)"));
		break;
	case F_DT_MONEY:
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("money(%s)"));
		break;
	case F_DT_OBJKEY:
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("object_key(%s)"));
		break;
	case F_DT_TABKEY   :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat1, _T("table_key(%s)"));
		break;
	case F_DT_TEXT :
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_OPT_LENGTH));
		lstrcpy(lpFparam->resultformat1, _T("text(%s)"));
		lstrcpy(lpFparam->resultformat2, _T("text(%s,%s)"));
		break;
	case F_DT_VARBYTE:
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2,ResourceString(IDS_ASSIST_OPT_LENGTH));
		lstrcpy(lpFparam->resultformat1, _T("varbyte(%s)"));
		lstrcpy(lpFparam->resultformat2, _T("varbyte(%s,%s)"));
		break;
	case F_DT_VARCHAR  :
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_OPT_LENGTH));
		lstrcpy(lpFparam->resultformat1, _T("varchar(%s)"));
		lstrcpy(lpFparam->resultformat2, _T("varchar(%s,%s)"));
		break;

	//
	// Numeric functions:
	case F_NUM_ABS     :
		lpFparam->parmcat1 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ARGUMENT));
		lstrcpy(lpFparam->resultformat1, _T("abs(%s)"));
		break;
	case F_NUM_ATAN:
		lpFparam->parmcat1 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ARGUMENT));
		lstrcpy(lpFparam->resultformat1, _T("atan(%s)"));
		break;
	case F_NUM_COS :
		lpFparam->parmcat1 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ARGUMENT));
		lstrcpy(lpFparam->resultformat1, _T("cos(%s)"));
		break;
	case F_NUM_EXP:
		lpFparam->parmcat1 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ARGUMENT));
		lstrcpy(lpFparam->resultformat1, _T("exp(%s)"));
		break;
	case F_NUM_LOG :
		lpFparam->parmcat1 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ARGUMENT));
		lstrcpy(lpFparam->resultformat1, _T("log(%s)"));
		break;
	case F_NUM_MOD:
		lpFparam->nbargsmin=2;
		lpFparam->nbargsmax=2;
		lpFparam->parmcat1 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ARGUMENT));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_MODULO));
		lstrcpy(lpFparam->resultformat2, _T("mod(%s,%s)"));
		break;
	case F_NUM_SIN:
		lpFparam->parmcat1 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ARGUMENT));
		lstrcpy(lpFparam->resultformat1, _T("sin(%s)"));
		break;
	case F_NUM_SQRT :
		lpFparam->parmcat1 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ARGUMENT));
		lstrcpy(lpFparam->resultformat1, _T("sqrt(%s)"));
		break;

	//
	// String functions:
	case F_STR_CHAREXT :
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_NB_CHARS));
		lstrcpy(lpFparam->resultformat2, _T("charextract(%s,%s)"));
		break;
	case F_STR_CONCAT:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lpFparam->parmcat2 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_STRING1));
		lstrcpy(lpFparam->CaptionText2,ResourceString(IDS_ASSIST_STRING2));
		lstrcpy(lpFparam->resultformat2, _T("concat(%s,%s)"));
		break;
	case F_STR_LEFT:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_NB_CHARS));
		lstrcpy(lpFparam->resultformat2, _T("left(%s,%s)"));
		break;
	case F_STR_LENGTH  :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->resultformat1, _T("length(%s)"));
		break;
	case F_STR_LOCATE  :
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lpFparam->parmcat2 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_OCC_IN_STRING));
		lstrcpy(lpFparam->CaptionText2,  _T("of string"));
		lstrcpy(lpFparam->resultformat2, _T("locate(%s,%s)"));
		break;
		break;
	case F_STR_LOWERCS :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->resultformat1, _T("lowercase(%s)"));
		break;
	case F_STR_PAD     :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->resultformat1, _T("pad(%s)"));
		break;
	case F_STR_RIGHT   :
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_NB_CHARS));
		lstrcpy(lpFparam->resultformat2, _T("right(%s,%s)"));
		break;
	case F_STR_SHIFT   :
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lpFparam->parmcat2 = FF_NUMFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_NB_PLACES));
		lstrcpy(lpFparam->resultformat2, _T("shift(%s,%s)"));
		break;
	case F_STR_SIZE    :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->resultformat1, _T("size(%s)"));
		break;
	case F_STR_SQUEEZE :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->resultformat1, _T("squeeze(%s)"));
		break;
	case F_STR_TRIM :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->resultformat1, _T("trim(%s)"));
		break;
	case F_STR_NOTRIM  :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->resultformat1, _T("trim(%s)"));
		break;
	case F_STR_UPPERCS :
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_STRING));
		lstrcpy(lpFparam->resultformat1, _T("trim(%s)"));
		break;

	//
	// date functions:
	case F_DAT_DATETRUNC:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_DATEFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_UNIT));
		lstrcpy(lpFparam->CaptionText2,ResourceString(IDS_ASSIST_DATE));
		lstrcpy(lpFparam->resultformat2, _T("date_trunc(%s,%s)"));
		break;
	case F_DAT_DATEPART:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat2 = FF_DATEFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_UNIT));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_DATE));
		lstrcpy(lpFparam->resultformat2, _T("date_part(%s,%s)"));
		break;
	case F_DAT_GMT:
		lpFparam->parmcat1 = FF_DATEFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_DATE));
		lstrcpy(lpFparam->resultformat1, _T("date_gmt(%s)"));
		break;
	case F_DAT_INTERVAL:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_UNIT));
		lstrcpy(lpFparam->CaptionText2,ResourceString(IDS_ASSIST_DATE_INTERVAL));
		lstrcpy(lpFparam->resultformat2, _T("interval(%s,%s)"));
		break;
	case F_DAT_DATE:
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_CHAR_STRING));
		lstrcpy(lpFparam->resultformat1, _T("_date(%s)"));
		break;
	case F_DAT_TIME:
		lpFparam->parmcat1 = FF_STRFUNCTIONS;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_CHAR_STRING));
		lstrcpy(lpFparam->resultformat1, _T("_time(%s)"));
		break;

	//
	// Aggregate functions:
	// none handled here

	//
	// ifnull function:
	case F_IFN_IFN :
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_CHECKED_ARGS));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_RETVAL_IFNULL));
		lstrcpy(lpFparam->resultformat2, _T("ifnull(%s,%s)"));
		break;

	//
	// predicate functions:
	case F_PRED_LIKE:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 3;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_PATTERN));
		lstrcpy(lpFparam->CaptionText3, ResourceString(IDS_ASSIST_ESCAPE_CHAR));
		lstrcpy(lpFparam->BetweenText1Left,  _T("LIKE"));
		lstrcpy(lpFparam->resultformat2, _T("%s like %s"));
		lstrcpy(lpFparam->resultformat3, _T("%s like %s escape %s"));
		break;
	case F_PRED_BETWEEN :
		lpFparam->nbargsmin = 3;
		lpFparam->nbargsmax = 3;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText3, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat3, _T("%s between %s and %s"));
		lstrcpy(lpFparam->BetweenText1Left,  _T("BETWEEN"));
		lstrcpy(lpFparam->BetweenText2Left,  _T("AND"));
		break;
	case F_PRED_ISNULL     :
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_CHECKED4NULL));
		lstrcpy(lpFparam->resultformat1, _T("%s is null"));
		break;

	//
	// Predicate combination functions:
	case F_PREDCOMB_AND :
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_PREDICATES;
		lpFparam->parmcat2 = FF_PREDICATES;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_PREDICATE));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_PREDICATE));
		lstrcpy(lpFparam->resultformat2, _T("%s and %s"));
		lstrcpy(lpFparam->BetweenText1Left, _T("AND"));
		break;
	case F_PREDCOMB_OR     :
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_PREDICATES;
		lpFparam->parmcat2 = FF_PREDICATES;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_PREDICATE));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_PREDICATE));
		lstrcpy(lpFparam->resultformat2, _T("%s or %s"));
		lstrcpy(lpFparam->BetweenText1Left, _T("OR"));
		break;
	case F_PREDCOMB_NOT:
		lpFparam->parmcat1 = FF_PREDICATES;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_PREDICATE));
		lstrcpy(lpFparam->resultformat1, _T("not %s"));
		break;

	//
	// subqueries functions :
	case F_SUBSEL_SUBSEL:
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_SELECT_STMT));
		lstrcpy(lpFparam->resultformat1, _T("(%s)"));
		break;
	
	//
	// non standard dialogs
	//
	// aggregate functions dialog
	// distinct/all should be the first argument
	case F_AGG_ANY:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_DATABASEOBJECTS;
		lpFparam->parmcat2 = FF_DATABASEOBJECTS;
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat2, _T("any(%s %s)"));
		break;
	case F_AGG_COUNT:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_DATABASEOBJECTS;
		lpFparam->parmcat2 = FF_DATABASEOBJECTS;
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat2, _T("count(%s %s)"));
		break;
	case F_AGG_SUM:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_DATABASEOBJECTS;
		lpFparam->parmcat2 = FF_DATABASEOBJECTS;
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat2, _T("sum(%s %s)"));
		break;
	case F_AGG_AVG:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_DATABASEOBJECTS;
		lpFparam->parmcat2 = FF_DATABASEOBJECTS;
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat2, _T("avg(%s %s)"));
		break;
	case F_AGG_MAX:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_DATABASEOBJECTS;
		lpFparam->parmcat2 = FF_DATABASEOBJECTS;
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat2, _T("max(%s %s)"));
		break;
	case F_AGG_MIN:
		lpFparam->nbargsmin = 2;
		lpFparam->nbargsmax = 2;
		lpFparam->parmcat1 = FF_DATABASEOBJECTS;
		lpFparam->parmcat2 = FF_DATABASEOBJECTS;
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat2, _T("min(%s %s)"));
		break;

	//
	// predicates:
	case F_PRED_COMP:
		lpFparam->nbargsmin = 3;
		lpFparam->nbargsmax = 3;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->CaptionText2, ResourceString(IDS_ASSIST_COMP_OPERATOR));
		lstrcpy(lpFparam->CaptionText3, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat3, _T("%s %s %s"));
		break;
	case F_PRED_IN:
		lpFparam->nbargsmin = 3;
		lpFparam->nbargsmax = 3;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_EXPRESSION));
		lstrcpy(lpFparam->resultformat3, _T("%s %s (%s)"));
		break;
	case F_PRED_ANY:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1,  ResourceString(IDS_ASSIST_SUBQUERY));
		lstrcpy(lpFparam->resultformat1, _T("any (%s)"));
		break;
	case F_PRED_ALL:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_SUBQUERY));
		lstrcpy(lpFparam->resultformat1, _T("all (%s)"));
		break;
	case F_PRED_EXIST:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_SUBQUERY));
		lstrcpy(lpFparam->resultformat1, _T("exists (%s)"));
		break;

	//
	// database objects
	case F_DB_COLUMNS:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_COL));
		lstrcpy(lpFparam->resultformat1, _T("%s"));
		break;
	case F_DB_TABLES:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_TABLE));
		lstrcpy(lpFparam->resultformat1, _T("%s"));
		break;
	case F_DB_DATABASES:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_DATABASE));
		lstrcpy(lpFparam->resultformat1, _T("%s"));
		break;
	case F_DB_USERS:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1,ResourceString(IDS_ASSIST_USER));
		lstrcpy(lpFparam->resultformat1, _T("%s"));
		break;
	case F_DB_GROUPS:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_GROUP));
		lstrcpy(lpFparam->resultformat1, _T("%s"));
		break;
	case F_DB_ROLES:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_ROLE));
		lstrcpy(lpFparam->resultformat1, _T("%s"));
		break;
	case F_DB_PROFILES:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_PROFILE));
		lstrcpy(lpFparam->resultformat1, _T("%s"));
		break;
	case F_DB_LOCATIONS:
		lpFparam->nbargsmin = 1;
		lpFparam->nbargsmax = 1;
		lstrcpy(lpFparam->CaptionText1, ResourceString(IDS_ASSIST_LOCATION));
		lstrcpy(lpFparam->resultformat1, _T("%s"));
		break;

	default:
		return;     // Unknown type!
	}
}

CString GetConstantAllDistinct(BOOL bAll)
{
	CString strAll = bAll? _T("all"): _T("distinct");
	return strAll;
}
/////////////////////////////////////////////////////////////////////////////
// CxDlgPropertySheetSqlExpressionWizard

IMPLEMENT_DYNAMIC(CxDlgPropertySheetSqlExpressionWizard, CPropertySheet)

CxDlgPropertySheetSqlExpressionWizard::CxDlgPropertySheetSqlExpressionWizard(CWnd* pWndParent)
	 : CPropertySheet(_T("Title to be formated"), pWndParent)
{
	m_psh.dwFlags |= PSH_HASHELP;
	m_pPointTopLeft = NULL;
	m_nCategory     = 0;
	m_nFamilyID     = -1;

	AddPage(&m_PageMain);
	SetWizardMode();
}

CxDlgPropertySheetSqlExpressionWizard::CxDlgPropertySheetSqlExpressionWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	m_psh.dwFlags |= PSH_HASHELP;
	m_pPointTopLeft = NULL;
	m_nCategory     = 0;
	m_nFamilyID     = -1;
}


CxDlgPropertySheetSqlExpressionWizard::CxDlgPropertySheetSqlExpressionWizard(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_psh.dwFlags |= PSH_HASHELP;
	m_pPointTopLeft = NULL;
	m_nCategory     = 0;
	m_nFamilyID     = -1;

	AddPage(&m_PageMain);
	SetWizardMode();
}

CxDlgPropertySheetSqlExpressionWizard::~CxDlgPropertySheetSqlExpressionWizard()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}


void CxDlgPropertySheetSqlExpressionWizard::ResetCategory()
{
	int i, nPages = GetPageCount();
	for (i=1; i<nPages; i++)
	{
		RemovePage (1);
	}
}

void CxDlgPropertySheetSqlExpressionWizard::SetStepAggregate()
{
	ResetCategory();
	AddPage(&m_PageAggregateParam);
}

void CxDlgPropertySheetSqlExpressionWizard::SetStepFunctionParam()
{
	ResetCategory();
	AddPage(&m_PageFuntionParam);
}

void CxDlgPropertySheetSqlExpressionWizard::SetStepComparaison()
{
	ResetCategory();
	AddPage(&m_PageComparaison);
}

void CxDlgPropertySheetSqlExpressionWizard::SetStepAnyAllExist()
{
	ResetCategory();
	AddPage(&m_PageAnyAllExistParam);
}

void CxDlgPropertySheetSqlExpressionWizard::SetStepIn()
{
	ResetCategory();
	AddPage(&m_PageInParam);
}

void CxDlgPropertySheetSqlExpressionWizard::SetStepDBObject()
{
	ResetCategory();
	AddPage(&m_PageDBObject);
}

void CxDlgPropertySheetSqlExpressionWizard::GetStatement(CString& strStatement)
{
	strStatement = m_strStatement;
}

void CxDlgPropertySheetSqlExpressionWizard::SetStatement(LPCTSTR lpszStatement)
{
	m_strStatement = lpszStatement? lpszStatement: _T("");
}

BEGIN_MESSAGE_MAP(CxDlgPropertySheetSqlExpressionWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CxDlgPropertySheetSqlExpressionWizard)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgPropertySheetSqlExpressionWizard message handlers


void CxDlgPropertySheetSqlExpressionWizard::OnDestroy() 
{
	CPropertySheet::OnDestroy();
}

BOOL CxDlgPropertySheetSqlExpressionWizard::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	if (m_pPointTopLeft)
	{
		SetWindowPos(NULL, m_pPointTopLeft->x, m_pPointTopLeft->y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
	}

	return bResult;
}

LRESULT CxDlgPropertySheetSqlExpressionWizard::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	//
	// The HELP Button is always disable. This is the reason why I overwrite this
	// function.
	// Due to the documentation, if PSH_HASHELP is set for the page (in property page,
	// the member m_psp.dwFlags |= PSH_HASHELP) then the help button is enable. But it
	// does not work.

	CWnd* pHelpButton = GetDlgItem (ID_W_HELP);
	if (pHelpButton)
		pHelpButton->EnableWindow (TRUE);
	
	return CPropertySheet::WindowProc(message, wParam, lParam);
}

void CxDlgPropertySheetSqlExpressionWizard::OnHelp()
{
	CTabCtrl* pTab = GetTabControl();
	ASSERT(pTab);
	if (pTab)
	{
		int nActivePage = pTab->GetCurSel();
		CPropertyPage* pPage = GetPage(nActivePage);
		if (pPage)
			pPage->SendMessage (WM_HELP);
	}
}
