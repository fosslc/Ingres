/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : parse.h, Header File 
**    Project  : Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Function prototype for accessing low level data 
**               (Implemented by francois noirot-nerin)
**
** History:
**
** 24-Jul-2001 (uk$so01)
**    Created. Split the old code from VDBA (mainmfc\parse.cpp)
** 03-Sep-2001 (uk$so01)
**    Transform code to be an ActiveX Control
** 02-Oct-2001 (noifr01)
**    (bug 105933) the number of rows, in a QEP, could be truncated for "big"
**    tables because of the assumption of node widths always being 24
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 16-Oct_2002 (noifr01)
**    (bug 108059) commented some lines for indicating there was no incidence
**    of the locale
** 13-Jan-2003 (schph01)
**    BUG 99242,  cleanup for DBCS compliance
** 05-Mar-2004 (lazro01) (Problem 2755 - Bug 111956)
**    Parsing of the QEP output should also take into account that
**    some nodes always have two immediate child nodes.
** 09-Sept-2004 (schph01)
**    BUG #112993 Add the try catch statements implement exception handling
**    in QEPParse() function.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "parse.h"
#include "qframe.h"
#include "ingobdml.h"
#include "sqltrace.h"
#include "tlsfunct.h"
extern "C"
{
#include <compat.h>
#include <cm.h>
#include <st.h>
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void MSGBOXQepSystemError()
{
	CString strMsg;
	AfxFormatString1 (strMsg, IDS_MSG_SYSTEM_ERROR, _T("33"));
	AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
}

#define MAXNODESPERLEVEL     40
#define MAXTRACELINELENGTH 2048
#define MAXLINESINNODE       16 // increased from 8 to 16 for bug 101620)

typedef struct tagLevelNodesdata 
{
	BOOL bIsDistributed;
	BOOL bLastNodes;

	int  nbNodes;
	int  nbNonStarLines;
	TCHAR nodeslines   [MAXLINESINNODE][MAXTRACELINELENGTH]; // first one (0) reserved for distributed

	int  NodeId       [MAXNODESPERLEVEL];
	int  ParentNodeId [MAXNODESPERLEVEL];      // 0 means "root node"
	int  StartPos     [MAXNODESPERLEVEL];
	BOOL bIsRight     [MAXNODESPERLEVEL];
	BOOL bHasImmRightChild[MAXNODESPERLEVEL]; 
        // True if the node must have immediate right child.
} LEVELNODES, FAR * LPLEVELNODES;


// do not change following hardcoded strings that are taken from optcotree.c,
// unless they are changed in optcotree
static TCHAR *szQueryPlan=_T("QUERY PLAN ");
static TCHAR *szTimedOut =_T(", timed out,");
static TCHAR *szLargeTmp =_T(", large temporaries,");
static TCHAR *szFloatExp =_T(", float/integer exceptions,");
static TCHAR *szOf       =_T(" of ");
static TCHAR *szGenTmpTbl=_T("    producing temporary table ");
static TCHAR *szGenTmpInTiTle=_T("producing temporary table ");
static TCHAR *szAggrExpr =_T("    aggregate expression -> ");
static TCHAR *szByExprAtt=_T("    by expression attribute -> ");
static TCHAR *szSiteID   =_T("Site ID ");
static TCHAR *szDstStInfo=_T("Distributed Site information");

// List of QEP Node Types having two immediate children, 
// last Node is empty to indicate the end of list.
char QEPNodeType[][10] = {"Cart-Prod", "FSM Join", "K Join", "PSM Join", "T Join", "Hash Join", "SE Join", ""}; 
	
static  BOOL HasAlwaysTwoChildren(char *pString) {
	int index=0;
	// Search the list of QEP Node types, to check whether the current node 
	// has any immediate right child.
	while (QEPNodeType[index][0]!='\0') {
		if (STbcompare(pString,
			             STlength(pString),QEPNodeType[index], 
			             STlength(QEPNodeType[index]), TRUE)==0)
			return TRUE;
		index++;
	}
	return FALSE;
}   
static int AvailableNodeId;

static  BOOL GetOneLevelNodes(TCHAR ** pCurLine, LPLEVELNODES lp0,LPLEVELNODES lp1, CaSqlTrace& aTrace)
{
	TCHAR * CurLine   = *pCurLine;
	TCHAR * pc;

	if (!lp0->nbNodes) {  // root Node
		lp1->nbNodes=1;
		lp1->NodeId[0]=1;
		lp1->ParentNodeId[0]=0;
		AvailableNodeId=2;
		while (CurLine[0]!=_T(' ')) { // Current Line should start with spaces and contain 1rst line of root node
			MSGBOXQepSystemError();
			CurLine = aTrace.GetNextSignificantTraceLine();
			if (!CurLine)
				return FALSE;
		}

		lp1->StartPos[0]=0; // find hor. position of root node
		pc=CurLine;
		while (*pc==_T(' ')) {
			lp1->StartPos[0]++;
			pc=_tcsinc(pc);
		}
	}
	else { // not the root branch
		int pos = 0;
		lp1->nbNodes=0;
		pc=CurLine;
		int ParentNodePos = -1; // Initialise to a invalid value
		while (*pc) {
			int i1;
			lp1->ParentNodeId [lp1->nbNodes] = -1;
			if (*pc==_T('/')){
				lp1->NodeId  [lp1->nbNodes]      = AvailableNodeId++;
				for (i1=0;i1<lp0->nbNodes;i1++) { 
					if (lp0->StartPos[i1]>= (pos-1) +12){
						lp1->ParentNodeId [lp1->nbNodes] = lp0->NodeId[i1];
						// Save the position of ParentNode, this will useful if the
                        // parent node has immediate right child.
                        ParentNodePos = i1;
						break;
					}
				}
				if (lp1->ParentNodeId [lp1->nbNodes]<0) 
				{
					MSGBOXQepSystemError();
				}
				lp1->StartPos[lp1->nbNodes] = pos-1;
				lp1->bIsRight[lp1->nbNodes] = FALSE;
				lp1->nbNodes++;

			}
			else {
				if (*pc==_T('\\')) {
					lp1->NodeId  [lp1->nbNodes]    = AvailableNodeId++;
					// Check if the parent has immediate right child,
					// if true then this node is the right child.
					if ((ParentNodePos != -1 ) && (TRUE == lp0->bHasImmRightChild[ParentNodePos]))
					{
						lp1->ParentNodeId[lp1->nbNodes] = lp0->NodeId[ParentNodePos];
						// Make the ParentNodePos now as invalid, as the 
						// right child for the parent node has been found. 
						ParentNodePos = -1;
					}
					else 
					{
						for (i1=lp0->nbNodes-1;i1>=0;i1--)
						{ 
							if (lp0->StartPos[i1]<=pos-12)
							{
								lp1->ParentNodeId [lp1->nbNodes] = lp0->NodeId[i1];
								break;
							}
						}
					}
					if (lp1->ParentNodeId [lp1->nbNodes]<0)
					{
						MSGBOXQepSystemError();
					}
					lp1->StartPos[lp1->nbNodes]    = pos;
					lp1->bIsRight[lp1->nbNodes]    = TRUE;
					lp1->nbNodes++;
				}
			}

			pc=_tcsinc(pc);
			pos++;
		} 
		if (!lp1->nbNodes)
			return FALSE;

		CurLine = aTrace.GetNextSignificantTraceLine();
		if (!CurLine)
			return FALSE;
		pc=CurLine;
	}

	lp1->bIsDistributed=FALSE;
	while (*pc==_T(' '))
		pc=_tcsinc(pc);
	if (!_tcsncmp(pc,szSiteID,_tcslen(szSiteID))) {
		lp1->bIsDistributed=TRUE;
		lstrcpy(lp1->nodeslines[0],CurLine);
		CurLine = aTrace.GetNextSignificantTraceLine();
		if (!CurLine)
			return FALSE;
	}

	lp1-> bLastNodes     = FALSE;
	lp1-> nbNonStarLines = MAXLINESINNODE-1;
	for (int i=1;/*i<MAXLINESINNODE*/;i++) { 
		if (!CurLine ||
		    !_tcsncmp(CurLine,szQueryPlan,_tcslen(szQueryPlan)) ||
		    !_tcsncmp(CurLine,szDstStInfo,_tcslen(szDstStInfo)) ||
		    !_tcsncmp(CurLine,_T("----"),     4)) {
			lp1->bLastNodes=TRUE;
			lp1-> nbNonStarLines =i-1;
			if (i<2)
				return FALSE;
			break;
		}
		if (i>=MAXLINESINNODE) {
			MSGBOXQepSystemError();
			break;
		}
		pc=CurLine;
		while (*pc) {
			if (*pc!=_T(' ')) {
				if (*pc==_T('/') || *pc==_T('\\')) {
					lp1-> nbNonStarLines = i -1;
					i=10000;
					break;
				}
			}
			pc=_tcsinc(pc);
		}
		if (i>=MAXLINESINNODE)
			break;
			lstrcpy(lp1->nodeslines[i],CurLine);
			CurLine = aTrace.GetNextSignificantTraceLine();
	}

	*pCurLine=CurLine;
	if (!CurLine)
		lp1-> bLastNodes = TRUE;
	return TRUE;
}


//
//
// pMainDoc    : Contains more information about the mdi.
//               pMainDoc->m_hNode (Node handle).
//               pMainDoc->m_strDatabase (Current database that is being connected).
//               ...
// strStatement: The statement to execute.
// bSelect     : TRUE if strStatement is a Select Statement.
// pDoc        : Document that will contain the list of Qep's trees.
extern void INGRESII_llPrepareQEP(LPCTSTR lpszStatement, int nIngresVersion, int nCursorSequence);
BOOL QEPParse (CdSqlQueryRichEditDoc* pMainDoc, LPCTSTR strStatement, BOOL bSelect, CdQueryExecutionPlanDoc* pDoc)
{
	//
	// This is an example how to initialize the Qep's trees.
	int nLeft    = 1;
	int nRight   = 2;
	int nParentID= 0;
	int nNodeID  = 0;
	LEVELNODES nodes0,nodes1;
	int ires;
	TCHAR * CurLine, *pc, *pc1;
	BOOL bResult    = TRUE;

	TCHAR szOptm[]   =_T("set optimizeonly"); 
	TCHAR szNoOptm[] =_T("set nooptimizeonly"); 
	TCHAR szQEP[]    =_T("set qep"); 
	TCHAR szNoQEP[]  =_T("set noqep"); 

	int nIngresVersion = -1;
	CaSqlTrace aTrace;
	CaLLAddAlterDrop execStatement (pMainDoc->GetNode(), pMainDoc->GetDatabase(), strStatement);
	try 
	{
		execStatement.SetUser(pMainDoc->GetUser());
		execStatement.SetServerClass(pMainDoc->GetServerClass());
		execStatement.SetOptions(pMainDoc->GetConnectionOption());
		execStatement.SetCommitInfo(FALSE);
		CaSession* pCurrentSession = pMainDoc->GetCurrentSession();
		if (pCurrentSession)
		{
			pCurrentSession->Activate();
			nIngresVersion = pCurrentSession->GetVersion();
		}

		execStatement.SetStatement(szOptm);
		INGRESII_llExecuteImmediate (&execStatement, NULL);
		execStatement.SetStatement(szQEP);
		INGRESII_llExecuteImmediate (&execStatement, NULL);

		aTrace.Start();
		//
		// The 'INGRESII_llPrepareQEP' does:
		// 1) EXEC SQL PREPARE :dyn_stm FROM :strStatement;
		// 2) EXEC SQL DESCRIBE :dyn_stm INTO :pSqlda;
		// 3) EXEC SQL EXECUTE IMMEDIATE :strStatement USING DESCRIPTOR pSqlda;
		INGRESII_llPrepareQEP(strStatement, nIngresVersion, theApp.GetCursorSequence());
		bResult=TRUE;

		aTrace.Stop();

		execStatement.SetStatement(szNoOptm);
		INGRESII_llExecuteImmediate (&execStatement, NULL);
		execStatement.SetStatement(szNoQEP);
		INGRESII_llExecuteImmediate (&execStatement, NULL);

		execStatement.SetStatement(_T("commit"));
		INGRESII_llExecuteImmediate (&execStatement, NULL);
		if (!bResult || !aTrace.GetTraceResult())
			return FALSE;
	}
	catch (CeSqlException e)
	{
		aTrace.Stop();

		execStatement.SetStatement(szNoOptm);
		INGRESII_llExecuteImmediate (&execStatement, NULL);
		execStatement.SetStatement(szNoQEP);
		INGRESII_llExecuteImmediate (&execStatement, NULL);
		throw CeSqlException (e.GetReason(), e.GetErrorCode());
	}
	CurLine = aTrace.GetFirstTraceLine();

	while (TRUE) { 
		while (CurLine) {     // Look for "QUERY PLAN" string (start of a QEP)
			if (!_tcsncmp(CurLine,szQueryPlan,_tcslen(szQueryPlan)))
				break;
			CurLine = aTrace.GetNextSignificantTraceLine();
		}
		if (!CurLine)
			return TRUE;  // no more Query Plan
		pc = _tcschr(CurLine, _T(','));
		if (!pc)
			return FALSE;
		pc=_tcschr(pc+1, _T(','));  // second comma (normally followed by " no timeout," or " timed out" 
		if (!pc)
			return FALSE;
		CaSqlQueryExecutionPlanData* pTree = new CaSqlQueryExecutionPlanData(QEP_NORMAL);
		*pc=_T('\0');
		pTree->m_strHeader =  CurLine+_tcslen(szQueryPlan); // Main header of the tree.
		*pc=_T(',');

		if (!_tcsncmp(pc,szTimedOut,_tcslen(szTimedOut)))
			pTree->m_bTimeOut = TRUE;                         // Timed out
		else
			pTree->m_bTimeOut = FALSE;
		pc= _tcschr(pc+1, _T(','));
		if (!pc)
			return FALSE;
		pc++; /* DBCS checked */
		if (!_tcsncmp(pc,szLargeTmp,_tcslen(szLargeTmp))) {
			pTree->m_bLargeTemporaries = TRUE;                         // Large temporaries
			pc+=_tcslen(szLargeTmp);
		}
		else
			pTree->m_bLargeTemporaries = FALSE;    
		if (!_tcsncmp(pc,szFloatExp,_tcslen(szFloatExp))) {
			pTree->m_bFloatIntegerException = TRUE;                         // Float / Integer Exceptions
			pc+=_tcslen(szFloatExp);
		}
		else
			pTree->m_bFloatIntegerException = FALSE;

		pTree->m_strGenerateTable= _T("");                               // Generated table.
	
		pTree->m_strHeader += _T("   ");
		if (!_tcsncmp(pc,szOf,_tcslen(szOf)))
			pc+=_tcslen(szOf);
		pTree->m_strHeader += pc;
		pc=_tcsstr(pc,szGenTmpInTiTle);
		if (pc) 
			pTree->m_strGenerateTable =  pc+_tcslen(szGenTmpInTiTle);   // Generated table in title.


		CurLine = aTrace.GetNextSignificantTraceLine();
		if (!CurLine)
			return FALSE;
		if (!_tcsncmp(CurLine,szGenTmpTbl,_tcslen(szGenTmpTbl))) {
			pTree->m_strGenerateTable      = CurLine+_tcslen(szGenTmpTbl);   // Generated table in separate line
			CurLine = aTrace.GetNextSignificantTraceLine();
			if (!CurLine)
				return FALSE;
		}

		while (!_tcsncmp(CurLine,szAggrExpr,_tcslen(szAggrExpr)-1)) {          // aggregate expressions
			TCHAR * pc2=CurLine+_tcslen(szAggrExpr);
			if (pc2[0]==_T('\0')) {    // workaround of bug under NT for 0D 0A return ...
				CurLine = aTrace.GetNextSignificantTraceLine();
				if (!CurLine)
					return FALSE;
				if (CurLine[0])
					pc2=CurLine;
				else
					break;
			}
			pTree->m_strlistAggregate.AddTail (pc2);
			CurLine = aTrace.GetNextSignificantTraceLine();
			if (!CurLine)
				return FALSE;
		}
		while (!_tcsncmp(CurLine,szByExprAtt,_tcslen(szByExprAtt))) {        // By expressions
			pTree->m_strlistExpression.AddTail (CurLine+_tcslen(szByExprAtt));
			CurLine = aTrace.GetNextSignificantTraceLine();
			if (!CurLine)
				return FALSE;
		}

		// => TO BE FINISHED CHECK IF NOTHING BEFORE NODES
		nodes1.nbNodes=0;
		nodes1.bLastNodes=FALSE;
		while (!nodes1.bLastNodes) {
			nodes0=nodes1;
			bResult=GetOneLevelNodes(&CurLine,&nodes0,&nodes1, aTrace);
			if (!bResult)
				return FALSE;
			if (nodes1.bIsDistributed){         // TO BE FINISHED => DISTRIBUTED
				pTree->m_qepType =QEP_STAR;
				for (int i=0;i<nodes1.nbNodes;i++) {
					BOOL bRoot=FALSE;
					BOOL bPagesReached =FALSE;
					int iLinePages =-1;
					if (nodes1.ParentNodeId[i]==0) 
						bRoot=TRUE;
					CaQepNodeInformationStar* pNodeInfo = NULL;
					if (bRoot)
						pNodeInfo = new CaQepNodeInformationStar(QEP_NODE_ROOT);
					else
						pNodeInfo = new CaQepNodeInformationStar ();

					pNodeInfo->m_nIcon        = IDR_QEP_PROJ_REST;        // Icon
					pNodeInfo->m_strNodeHeader=_T("");
					pNodeInfo->m_iPosX=nodes1.StartPos[i]/12;
					pNodeInfo->m_strNode=_T("");

					for (int j=0;j<=nodes1.nbNonStarLines;j++) {
						TCHAR buf[200];
						static TCHAR* szPagesTupsStart = _T("Pages ");
						static TCHAR* szPagesTups      = _T("Pages %d Tups %d");
						static TCHAR* sz3Costs         = _T("D%d C%d N%d");

						if ((int)_tcslen(nodes1.nodeslines[j])>=nodes1.StartPos[i]) {
							int ilen = sizeof(buf) -1;
							if ( (i+1) <nodes1.nbNodes)
								ilen = 1 + nodes1.StartPos[i+1] - nodes1.StartPos[i];
							fstrncpy(buf, nodes1.nodeslines[j]+nodes1.StartPos[i],ilen);
							if (j==0) {
								pc1=_tcsstr(buf,_T("->"));
								if (pc1) {
									pNodeInfo->m_strNode = pc1+2;
									*pc1=_T('\0');
								}
								pNodeInfo->m_strDatabase = buf;
							}
							else
							if (j==1)
							{
								pNodeInfo->m_strNodeType  = buf;
								// Check whether this node has immediate right child.
								nodes1.bHasImmRightChild[i] = HasAlwaysTwoChildren(buf);
							}
							else {
								if (!_tcsncmp(buf,szPagesTupsStart,_tcslen(szPagesTupsStart))) {
									int pgs,tps;
									bPagesReached=TRUE;
									iLinePages=j;
									ires =_stscanf(buf,szPagesTups,&pgs,&tps); /* integers. ok for locale */
									if (ires==2) {
										pNodeInfo->m_strPage.Format (_T("%d"), pgs);  // Number of pages
										pNodeInfo->m_strTuple.Format(_T("%d"), tps);  // Number of tuples.            
									}
								}
								else {
									if (!bPagesReached) {
										if (!pNodeInfo->m_strNodeHeader.Compare(_T("")))
											pNodeInfo->m_strNodeHeader = buf;
										else {
											TCHAR buf1[10];
											lstrcpy(buf1,_T("\r\n"));
											pNodeInfo->m_strNodeHeader+=buf1;
											pNodeInfo->m_strNodeHeader+=buf;
										}
									}
									else {
										if (j == iLinePages+1) { //get costs;
											int d1,c1,n1;
											ires =_stscanf(buf,sz3Costs,&d1,&c1,&n1); /* integers. ok for locale */
											if (ires==3) {
												pNodeInfo->m_nDiskCost=d1;
												pNodeInfo->m_nCPUCost =c1;
												pNodeInfo->m_nNetCost =n1;
											}
										}
									}
								}
							}
						}
					}
					if (bRoot)
						pTree->AddQEPRootNode (nodes1.NodeId[i], pNodeInfo);
					else{
						int ileftright = nLeft;
						if (nodes1.bIsRight[i])
							ileftright = nRight;
						pTree->AddQEPNode     (nodes1.NodeId[i], nodes1.ParentNodeId[i], ileftright, pNodeInfo);
					}
				}
				continue;
			}
			for (int i=0;i<nodes1.nbNodes;i++) {
				BOOL bRoot=FALSE;
				BOOL bPagesReached =FALSE;
				int iLinePages =-1;
				if (nodes1.ParentNodeId[i]==0) 
					bRoot=TRUE;
					CaQepNodeInformation* pNodeInfo = NULL;

				if (bRoot)
					pNodeInfo = new CaQepNodeInformation(QEP_NODE_ROOT);
				else
					pNodeInfo = new CaQepNodeInformation ();

				pNodeInfo->m_nIcon        = IDR_QEP_PROJ_REST;        // Icon
				pNodeInfo->m_strNodeHeader=_T("");
				pNodeInfo->m_iPosX=nodes1.StartPos[i]/12;

				for (int j=1;j<=nodes1.nbNonStarLines;j++) {
					TCHAR buf[200];
					static TCHAR* szPagesTupsStart = _T("Pages ");
					static TCHAR* szPagesTups      = _T("Pages %d Tups %d");
					static TCHAR* sz2Costs         = _T("D%d C%d");
					static TCHAR* sz3Costs         = _T("D%d C%d N%d");

					if ((int)_tcslen(nodes1.nodeslines[j])>=nodes1.StartPos[i]) {
						int ilen = sizeof(buf) -1;
						if ( (i+1) <nodes1.nbNodes)
							ilen = 1 + nodes1.StartPos[i+1] - nodes1.StartPos[i];
						fstrncpy(buf, nodes1.nodeslines[j]+nodes1.StartPos[i],ilen);
						if (j==1)
						{
							pNodeInfo->m_strNodeType  = buf;
							// Check whether this node has immediate right child.
							nodes1.bHasImmRightChild[i] = HasAlwaysTwoChildren(buf);
						}
						else {
							if (!_tcsncmp(buf,szPagesTupsStart,_tcslen(szPagesTupsStart))) {
								int pgs,tps;
								bPagesReached=TRUE;
								iLinePages=j;
								ires =_stscanf(buf,szPagesTups,&pgs,&tps); /* integers. ok for locale */
								if (ires==2) {
									pNodeInfo->m_strPage.Format (_T("%d"), pgs);  // Number of pages
									pNodeInfo->m_strTuple.Format(_T("%d"), tps);  // Number of tuples.            
								}
							}
							else {
								if (!bPagesReached) {
									if (!pNodeInfo->m_strNodeHeader.Compare(_T("")))
										pNodeInfo->m_strNodeHeader = buf;
									else {
										TCHAR buf1[10];
										lstrcpy(buf1,_T("\r\n"));
										pNodeInfo->m_strNodeHeader+=buf1;
										pNodeInfo->m_strNodeHeader+=buf;
									}
								}
								else {
									if (j == iLinePages+1) { //get costs;
										int d1,c1,n1;
										if (!nodes1.bIsDistributed) {
											ires =_stscanf(buf,sz2Costs,&d1,&c1); /* integers. ok for locale */
											if (ires==2) {
												pNodeInfo->m_nDiskCost=d1;
												pNodeInfo->m_nCPUCost =c1;
											}
										}
										else {
											ires =_stscanf(buf,sz3Costs,&d1,&c1,&n1); /* integers. ok for locale */
											if (ires==3) {
												pNodeInfo->m_nDiskCost=d1;
												pNodeInfo->m_nCPUCost =c1;
												pNodeInfo->m_nNetCost =n1;
											}
										}
									}
								}
							}
						}
					}
				}
	
				if (bRoot)
					pTree->AddQEPRootNode (nodes1.NodeId[i], pNodeInfo);
				else{
					int ileftright = nLeft;
					if (nodes1.bIsRight[i])
					ileftright = nRight;
					pTree->AddQEPNode     (nodes1.NodeId[i], nodes1.ParentNodeId[i], ileftright, pNodeInfo);
				}
			}
		}

		pTree->m_pQepBinaryTree->CalcChildrenPercentages(NULL);
		if (pTree->m_qepType ==QEP_STAR) 
			CalcSTARChildrenPercentages(pTree->m_pQepBinaryTree,NULL);

		pDoc->m_listQepData.AddTail (pTree);

		if (CurLine) {
			if (!_tcsncmp(CurLine,szDstStInfo,_tcslen(szDstStInfo))) {
				ASSERT(pTree->m_qepType ==QEP_STAR);
				CurLine = aTrace.GetNextSignificantTraceLine();
				while (CurLine) {
					int nodeno;
					TCHAR buf1[200],buf2[100];
					static TCHAR *szDstNames =_T("    Site %d has node name= %s, LDB name= %s");
					static TCHAR *s1=_T("    Site ");
					static TCHAR *s2=_T("has node name= ");
					static TCHAR *s3=_T(", LDB name= ");
					if (_tcsncicmp(CurLine,s1,_tcslen(s1)))
						break;
					pc=CurLine+_tcslen(s1);
					pc1=_tcschr(pc,_T(' '));
					if (!pc1)
						break;
					*pc1=_T('\0');
					nodeno=_ttoi(pc);
					pc=pc1+1;
					if (_tcsncicmp(pc,s2,_tcslen(s2)))
						break;
					pc += _tcslen(s2);
					pc1 = _tcsstr(pc,s3);
					if (!pc1)
						break;
					*pc1=_T('\0');
					lstrcpy(buf1,pc);
					pc=pc1+_tcslen(s3);
					lstrcpy(buf2,pc);
					if (_tcscmp(buf2,_T("not applicable"))) {
						_tcscat(buf1,_T("::"));
						_tcscat(buf1,buf2);
					}
					CaSqlQueryExecutionPlanData* pData = NULL;
					POSITION pos = pDoc->m_listQepData.GetHeadPosition();
					while (pos != NULL)  {
						pData = pDoc->m_listQepData.GetNext(pos);
						UpdateSTARSiteStrings(pData->m_pQepBinaryTree,nodeno, buf1);
					}
					CurLine = aTrace.GetNextSignificantTraceLine();
				}
				if (CurLine) 
				{
					MSGBOXQepSystemError();
				}
			}
		}
	}
	return TRUE;
}



