/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTDRAG.C
 *
 * Contains drag drop handler for all views.
 ****************************************************************************/

/*
**  History:
**  02-Jan-2002 (noifr01)
**      (SIR 99596) removed usage of unused or obsolete resources
*/

#include <windows.h>
#include <windowsx.h>   // Need this for the memory macros
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "catocntr.h"
#include "cntdll.h"
#include "cntdrag.h"

static BOOL InitDragInfoFile(HWND hWndCnt,LPDRGINPROG lpDrgInProg);
static LPDRGINPROG AllocDrgInfo(HWND hWnd, LPPOINT ptStartPos);
static  LPDRGINPROG FindDrgInfoByName(HWND hWnd,LPCDRAGINFO lpDragInfo);
static void FreeDrgInfo(HWND hWnd, int nIndex);
static LPCDRAGINFO GetDropInfo(HWND hWndCnt, WPARAM wParam, LPARAM lParam);
static LPCNTDRAGTRANSFER GetDragTransferInfo(HWND hWndCnt,WPARAM wParam, LPARAM lParam);
static void FreeDragTransferInfo(HWND hWnd, int nIndex);
static void DisplayCursor(LPCONTAINER lpCnt,LPDRGINPROG lpDrgInProg,UINT uOp);

LRESULT Process_OnDragOver(HWND hWnd,WPARAM wParam,LPARAM lParam);
VOID Process_OnDragLeave(HWND hWnd,WPARAM wParam,LPARAM lParam);
VOID Process_OnDrop(HWND hWnd, WPARAM wParam, LPARAM lParam);
VOID Process_OnDropComplete(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT Process_OnRenderData(HWND hWnd, WPARAM wParam, LPARAM lParam);
VOID Process_OnRenderComplete(HWND hWnd, WPARAM wParam, LPARAM lParam);

/****************************************************************************
 * Function: Process_DefaultMsg
 *
 * Purpose: Default Message handler for all views
 *
 * Parameters   hWnd - handle of container
 *              iMsg - Message identifier
 *              wParam - wParam of Message
 *              lParam - lParam of Message
 *
 *
 * Return Value: determined by what type of message is processed
 * 
 ****************************************************************************/
LRESULT Process_DefaultMsg( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
  LRESULT lResult = 0L;
  LPCONTAINER lpCnt = GETCNT(hWnd);  

  if(lpCnt != NULL)
    {
      // check our styles before processing messages
      if(FALSE /*lpCnt->dwStyle & CTS_DRAGDROP  || lpCnt->dwStyle & CTS_DRAGONLY ||
	 lpCnt->dwStyle & CTS_DROPONLY*/) /* drag/drop disabled for this control */ 
	{

	  if(iMsg == lpCnt->uWmDragOver)
	    lResult  =  Process_OnDragOver(hWnd,wParam,lParam);
	  else if(iMsg == lpCnt->uWmDrop)
	    Process_OnDrop(hWnd,wParam,lParam);
	  else if(iMsg  == lpCnt->uWmDragLeave)
	    Process_OnDragLeave(hWnd,wParam,lParam);
	  else if(iMsg == lpCnt->uWmDragComplete)
	    Process_OnDropComplete(hWnd,wParam,lParam);
	  else if(iMsg == lpCnt->uWmRenderData)
	    lResult = Process_OnRenderData(hWnd,wParam,lParam);
	  else if(iMsg == lpCnt->uWmRenderComplete)
	    Process_OnRenderComplete(hWnd,wParam,lParam);
	  else
	    lResult =  CNT_DEFWNDPROC( hWnd, iMsg, wParam, lParam );

	}
      else
	lResult =  CNT_DEFWNDPROC( hWnd, iMsg, wParam, lParam );
    }
  else
    lResult = CNT_DEFWNDPROC( hWnd, iMsg, wParam, lParam );
  
return(lResult);

}
/****************************************************************************
 * Function: Process_OnRenderData
 *
 * Purpose: Message handler for custom render data message
 *
 * Parameters   hWnd - handle of container
 *              wParam - wParam of Drop Message
 *              lParam - lParam of Drop Message
 *
 *
 * Return Value: None
 * 
 ****************************************************************************/
LRESULT Process_OnRenderData(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  LRESULT lResult = 0L;

  LPCONTAINER lpCnt = GETCNT(hWnd);  

  lpCnt->lpDragTransfer = GetDragTransferInfo(hWnd,wParam,lParam);

  lResult = NotifyAssocWndEx( hWnd, lpCnt, CN_RENDERDATA, 0, NULL, NULL, 0, 0, 0, NULL );           

  FreeDragTransferInfo(hWnd,lpCnt->nRenderIndex);
  if(lResult > 0)
    {

    }

return(lResult);
}

/****************************************************************************
 * Function: Process_OnRenderComplete
 *
 * Purpose: Message handler for custom render complete message
 *
 * Parameters   hWnd - handle of container
 *              wParam - wParam of Drop Message
 *              lParam - lParam of Drop Message
 *
 *
 * Return Value: None
 * 
 ****************************************************************************/
VOID Process_OnRenderComplete(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  LPCONTAINER lpCnt = GETCNT(hWnd);  

  lpCnt->lpRenderComplete = GetDragTransferInfo(hWnd,wParam,lParam);

  NotifyAssocWndEx( hWnd, lpCnt, CN_RENDERCOMPLETE, 0, NULL, NULL, 0, 0, 0, NULL );           

  FreeDragTransferInfo(hWnd,lpCnt->nRenderIndex);
}

/****************************************************************************
 * Function: Process_OnDropComplete
 *
 * Purpose: Message handler for custom drop complete
 *
 * Parameters   hWnd - handle of container
 *              wParam - wParam of Drop Message
 *              lParam - lParam of Drop Message
 *
 *
 * Return Value: None
 * 
 ****************************************************************************/
VOID Process_OnDropComplete(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  LPDRGINPROG lpDrgInfo = NULL;
  LPCDRAGINFO lpTargetInfo = NULL;
  int nIndex;
  LPCONTAINER lpCnt = GETCNT(hWnd);  
  

  nIndex = (int) wParam;
  lpCnt->nDropComplete = nIndex;
  lpDrgInfo = FindDrgInfoByIndex(hWnd, nIndex);

  NotifyAssocWndEx( hWnd, lpCnt, CN_DROPCOMPLETE, 0, NULL, NULL, 0, 0, 0, NULL );           

  FreeDrgInfo(hWnd,nIndex);

#ifdef WIN32  
  // file attributes tell system to delete file when
  // all handles are closed so this should cause the file
  // to be deleted
  CloseHandle(lpDrgInfo->hMapFileHandle);
#endif


}
/****************************************************************************
 * Function: Process_OnDrop
 *
 * Purpose: Message handler for custom drop handling
 *
 * Parameters   hWnd - handle of container
 *              wParam - wParam of Drop Message
 *              lParam - lParam of Drop Message
 *
 *
 * Return Value: None
 * 
 ****************************************************************************/
VOID Process_OnDrop(HWND hWnd, WPARAM wParam, LPARAM lParam)
{

  LPCONTAINER lpCnt = GETCNT(hWnd);

  // get shared memory or pointer to drag info and store it away
  lpCnt->lpTargetInfo = GetDropInfo(hWnd,wParam,lParam);

  SetDragDropOverRec(hWnd,CN_DROP);
  NotifyAssocWndEx( hWnd, lpCnt, CN_DROP, 0, NULL, NULL, 0, 0, 0, NULL );                  
  FreeDragInfo(hWnd);

}

/****************************************************************************
 * Function: Process_OnDragLeave
 *
 * Purpose: Message handler for custom drag/drop dragleave handling
 *
 * Parameters   hWnd - handle of container
 *              wParam - wParam of DragLeave Message
 *              lParam - lParam of DragLeave Message
 *
 *
 * Return Value: None
 * 
 ****************************************************************************/
VOID Process_OnDragLeave(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
  LPCONTAINER lpCnt = GETCNT(hWnd);
  
  lpCnt->lpTargetInfo = GetDragInfo(hWnd,wParam,lParam);
  NotifyAssocWndEx( hWnd, lpCnt, CN_DRAGLEAVE, 0, NULL, NULL, 0, 0, 0, NULL );                  
  FreeDragInfo(hWnd);
}

/****************************************************************************
 * Function: Dtl_OnDragOver
 *
 * Purpose: Message handler for custom drag/drop dragover handling
 *
 * Parameters   hWnd - handle of container
 *              wParam - wParam of QueryDragover Message
 *              lParam - lParam of QueryDragover Message
 *
 *
 * Return Value: LRESULT - HIWORD LOWORD combination describing if object
 *               being dragged can be accepted.
 *               
 * 
 ****************************************************************************/
LRESULT Process_OnDragOver(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
 
  LRESULT lRet = MAKELRESULT(DOR_NEVERDROP,DO_COPY);                              
  LPCONTAINER lpCnt = GETCNT(hWnd);
 

  // first check to see if we should honor the drag over at all
  if( FALSE /*lpCnt->dwStyle & CTS_DROPONLY  || lpCnt->dwStyle & CTS_DRAGDROP */)
    {
      lpCnt->lpTargetInfo = GetDragInfo(hWnd,wParam,lParam);
      SetDragDropOverRec(hWnd,CN_DRAGOVER);
      lRet = VerifyAllowDrop(hWnd);
      FreeDragInfo(hWnd);
    }

	return(lRet);

}

/*
***************************************************************************
* Function: FreeDragTransferInfo
*
* Purpose:  This function will free the RENDERDATA structure referenced
*           by nIndex;
*           
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  int           nIndex      - index of drginprog structure to free
*
* Return Value:  None
*                
*
***************************************************************************
*/
static void FreeDragTransferInfo(HWND hWnd, int nIndex)
{
  BOOL bDone = FALSE;
  LPCONTAINER lpCnt = GETCNT(hWnd);
  LPRENDERDATA lpRenderTemp = NULL;  
  LPRENDERDATA lpRenderCurr = NULL;  

  lpRenderTemp = lpCnt->lpRenderData;
  lpRenderCurr = lpCnt->lpRenderData;

  while(lpRenderTemp != NULL && !bDone)
    {

      if(nIndex == lpRenderTemp->nRenderIndex)
	bDone = TRUE;
      else
	{
	  lpRenderCurr = lpRenderTemp;
	  lpRenderTemp = lpRenderTemp->lpNext;
	}
    }

  if(lpRenderTemp != NULL)
    {


#ifdef WIN32
      // clean up shared memory 
      if(lpRenderTemp->hDragHandle != NULL)
	{
	  // first unmap view of shared memory
	  if(lpRenderTemp->lpDragTransfer != NULL)
	    UnmapViewOfFile(lpRenderTemp->lpDragTransfer);

	  // close handle to shared memory and reset pointers
	  CloseHandle(lpRenderTemp->hDragHandle);
	}
#endif

      // check and see if were the first link
      if(lpCnt->lpRenderData != lpRenderTemp)
	lpRenderCurr->lpNext = lpRenderTemp->lpNext;
      else
	lpCnt->lpRenderData = lpRenderTemp->lpNext;

      free(lpRenderTemp);
    }

}


/*
***************************************************************************
* Function: GetDragTransferInfo
*
* Purpose:  Will unpack the WPARAM and LPARAM values and do the correct
*           memory access to get the dragtransferinfo structure.
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  WPARAM        wParam      - message wParam. Only used in win16
*  LPARAM        lParam      - message lParam.
*                              For win16 contains ptr to dragtransferinfo struct
*                              For win32 LOWORD = processid HIWORD is index to struct
*
* Return Value:  pointer to dragtransferinfo struct or NULL
*                
*
***************************************************************************
*/
LPCNTDRAGTRANSFER GetDragTransferInfo(HWND hWndCnt,WPARAM wParam, LPARAM lParam)
{

  LPRENDERDATA lpRenderData = NULL;
  LPCONTAINER lpCnt = GETCNT(hWndCnt);


  lpCnt->nRenderIndex++;
  lpRenderData = calloc(1,LEN_CNTRENDERDATA);
  lpRenderData->lpNext = NULL;
  lpRenderData->nRenderIndex = lpCnt->nRenderIndex;
  lpRenderData->lpDragTransfer = NULL;

#ifdef WIN32
  // build key to shared memory where dragtransfer info is
  sprintf(lpRenderData->szKey,"%ul.%d",lParam,wParam );

  // now open that name
  lpRenderData->hDragHandle = OpenFileMapping(FILE_MAP_READ,TRUE,lpRenderData->szKey);

  // if the handle is not null get data
  if(lpRenderData->hDragHandle != NULL)
  	lpRenderData->lpDragTransfer = MapViewOfFile(lpRenderData->hDragHandle,FILE_MAP_READ,0,0,0);
  
#else
// FOR 16 BIT OPERATION THE DRAGINFO IS PASSED IN wParam

	lpRenderData->lpDragTransfer = (LPCNTDRAGTRANSFER) lParam;
#endif


  // add drop info to list
  if(lpCnt->lpRenderData == NULL)
  	lpCnt->lpRenderData = lpRenderData;
  else
	{
	  // scan list till end and add 
	  LPRENDERDATA lpRenderTemp = lpCnt->lpRenderData;
	  LPRENDERDATA lpRenderCurr = lpCnt->lpRenderData;
	  while(lpRenderTemp != NULL)
	    {
	      lpRenderTemp = lpRenderTemp->lpNext;
	      // if were not null move current pointer
	      if(lpRenderTemp != NULL)
		lpRenderCurr = lpRenderTemp;              
	    }

	  lpRenderCurr->lpNext = lpRenderData;
	}
      
  

return(lpRenderData->lpDragTransfer);

}
/*
***************************************************************************
* Function: GetDropInfo
*
* Purpose:  Will unpack the WPARAM and LPARAM values and do the correct
*           memory access to get the dragtargetinfo structure.
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  WPARAM        wParam      - message wParam. Only used in win16
*  LPARAM        lParam      - message lParam.
*                              For win16 contains ptr to draginfo struct
*                              For win32 LOWORD = processid HIWORD is index to struct
*
* Return Value:  ptr to draginfo structure or NULL
*                
*
***************************************************************************
*/
static LPCDRAGINFO GetDropInfo(HWND hWndCnt, WPARAM wParam, LPARAM lParam)
{
#ifdef WIN32
  DWORD nPid;
#endif
  PFDROPINPROG lpDropInfo = NULL;
  LPCONTAINER lpCnt = GETCNT(hWndCnt);


  lpCnt->nDropop++;
  lpDropInfo = calloc(1,LEN_CNTDROPINPROG);

#ifdef WIN32
// FOR 32 BIT OPERATION THE DRAGINFO INFORMATION IS PASSED VIA 
// SHARED MEMORY 
      
      // get amount of data present in memory mapped file
      nPid = (DWORD) lParam;
      lpDropInfo->nIndex = wParam;

      sprintf(lpDropInfo->szKey,"%ul.%d",nPid,lpDropInfo->nIndex);

      // set current drop key
      lstrcpy(lpCnt->szDropKey,lpDropInfo->szKey);

      // now open that name
      lpDropInfo->hDragHandle = OpenFileMapping(FILE_MAP_READ,TRUE,lpDropInfo->szKey);

      // if the handle is not null get data
      if(lpDropInfo->hDragHandle != NULL)
	{
	  lpDropInfo->lpDragInfo = MapViewOfFile(lpDropInfo->hDragHandle,FILE_MAP_READ,0,0,0);

	}
  
#else
// FOR 16 BIT OPERATION THE DRAGINFO IS PASSED IN wParam

	lpDropInfo->lpDragInfo = (LPCDRAGINFO) lParam;
	lpDropInfo->nIndex = (int) wParam;
	
#endif

      // add drop info to list
      if(lpCnt->lpDropInProg == NULL)
	lpCnt->lpDropInProg = lpDropInfo;
      else
	{
	  // scan list till end and add 
	  LPDROPINPROG lpDropTemp = lpCnt->lpDropInProg;
	  LPDROPINPROG lpDropCurr = lpCnt->lpDropInProg;
	  while(lpDropTemp != NULL)
	    {
	      lpDropTemp = lpDropTemp->lpNext;
	      // if were not null move current pointer
	      if(lpDropTemp != NULL)
		lpDropCurr =lpDropTemp;              
	    }

	  lpDropCurr->lpNext = lpDropInfo;
	}

return(lpDropInfo->lpDragInfo);
}
/*
***************************************************************************
* Function: GetDragInfo
*
* Purpose:  Will unpack the WPARAM and LPARAM values and do the correct
*           memory access to get the dragtargetinfo structure.
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  WPARAM        wParam      - wparam message value
*  LPARAM        lParam      - lparam message value
*
* Return Value:  pointer to drag info structure or NULL
*                
*
***************************************************************************
*/
LPCDRAGINFO GetDragInfo(HWND hWndCnt, WPARAM wParam, LPARAM lParam)
{
 LPCDRAGINFO lpTargetInfo = NULL;

#ifdef WIN32
// FOR 32 BIT OPERATION THE DRAGINFO INFORMATION IS PASSED VIA 
// SHARED MEMORY 
      HANDLE hDragHandle = NULL;
      char lpName[20];

      // get amount of data present in memory mapped file
      DWORD nPid = lParam;
      int nIndex = wParam;

      sprintf(lpName,"%ul.%d",nPid,nIndex);

      // now open that name
      hDragHandle = OpenFileMapping(FILE_MAP_READ,TRUE,lpName);


      // if the handle is not null get data
      if(hDragHandle != NULL)
	lpTargetInfo = MapViewOfFile(hDragHandle,FILE_MAP_READ,0,0,0);
  
#else
// FOR 16 BIT OPERATION THE DRAGINFO IS PASSED IN wParam

	lpTargetInfo = (LPCDRAGINFO) lParam;
	
#endif

return(lpTargetInfo);

}

/*
***************************************************************************
* Function: FreeDrgInfo
*
* Purpose:  This function will free the DRGINPRROG structure referenced
*           by nIndex;
*           
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  int           nIndex      - index of drginprog structure to free
*
* Return Value:  None
*                
*
***************************************************************************
*/
static void FreeDrgInfo(HWND hWnd, int nIndex)
{
  BOOL bDone = FALSE;
  LPCONTAINER lpCnt = GETCNT(hWnd);  
  LPDRGINPROG lpDrgTemp = NULL;
  LPDRGINPROG lpDrgCurr = NULL;

  lpDrgTemp = lpCnt->lpDrgInProg;
  lpDrgCurr = lpCnt->lpDrgInProg;

  while(lpDrgTemp != NULL && !bDone)
    {

      if(nIndex == lpDrgTemp->nIndex)
	bDone = TRUE;
      else
	{
	  lpDrgCurr = lpDrgTemp;
	  lpDrgTemp = lpDrgTemp->lpNext;
	}
    }

  if(lpDrgTemp != NULL)
    {
      if(lpDrgTemp->lpDragInit != NULL)
	free(lpDrgTemp->lpDragInit);


#ifdef WIN32
      // clean up shared memory 
      if(lpDrgTemp->hDragMem != NULL)
	{
	  // first unmap view of shared memory
	  if(lpDrgTemp->lpDragView != NULL)
	    UnmapViewOfFile(lpDrgTemp->lpDragView);

	  // close handle to shared memory and reset pointers
	  CloseHandle(lpDrgTemp->hDragMem);
	}
#endif

      // check and see if were the first link
      if(lpCnt->lpDrgInProg != lpDrgTemp)
	lpDrgCurr->lpNext = lpDrgTemp->lpNext;
      else
	lpCnt->lpDrgInProg = lpDrgTemp->lpNext;

      free(lpDrgTemp);
    }

}

/*
***************************************************************************
* Function: AllocDrgInfo
*
* Purpose:  This function will allocate and initialize a DRGINPRROG structure
*           
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  LPPOINT       ptStartPos  - Pointer to point structure containing
*                              start position of drag operation
*
* Return Value:  NULL or pointer to draginfo structure
*                
*
***************************************************************************
*/
static LPDRGINPROG AllocDrgInfo(HWND hWnd, LPPOINT ptStartPos)
{

  LPCONTAINER lpCnt = GETCNT(hWnd);

  LPDRGINPROG lpDrgInProg = (LPDRGINPROG) calloc(1,LEN_CNTDRGINPROG);

  // increment our drag drop index number
  lpCnt->nDrgop++;

  // allocate memory for the draginit structure
  lpDrgInProg->lpDragInit = (LPCNTDRAGINIT) calloc(1,LEN_CNTDRAGINIT);
  lpDrgInProg->lpNext = NULL;

  // set index number for this drag drop operation
  lpDrgInProg->nIndex = lpCnt->nDrgop;

  // set current mouse pos  (screen coords)
  lpDrgInProg->lpDragInit->x = ptStartPos->x;
  lpDrgInProg->lpDragInit->y = ptStartPos->y;
  lpDrgInProg->lpDragInit->nIndex = lpCnt->nDrgop;

  // did drag start over whitespace
  if(ptStartPos->y > lpCnt->yLastRec)
    lpDrgInProg->lpDragInit->lpRecord = NULL;
  else
    {
      // get record where drag and drop started
      lpDrgInProg->lpDragInit->lpRecord = CntScrollRecList( hWnd, lpCnt->lpCI->lpTopRec, lpCnt->nCurRec );
      CntFocusExtExGet(hWnd, &lpDrgInProg->lpDragInit->cx,&lpDrgInProg->lpDragInit->cy );
    }

   // add link to drag and drops in progress
   if(lpCnt->lpDrgInProg == NULL)
      lpCnt->lpDrgInProg = lpDrgInProg;
   else
     {
	// scan list till end and add 
	LPDRGINPROG lpDrgTemp = lpCnt->lpDrgInProg;
	LPDRGINPROG lpDrgCurr = lpCnt->lpDrgInProg;
	while(lpDrgTemp != NULL)
	  {
	    lpDrgTemp = lpDrgTemp->lpNext;
	    // if were not null move current pointer
	    if(lpDrgTemp != NULL)
	      lpDrgCurr =lpDrgTemp;              
	  }

	lpDrgCurr->lpNext = lpDrgInProg;
     }

return(lpDrgInProg);
}

/*
***************************************************************************
* Function: FindDrgInfoByName
*
* Purpose:  This function will scan the list of draginfo structures and
*           either return a  pointer to a drag info structure identified
*           by name or NULL.
*           
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  LPCDRAGINFO   lpDragInfo  - index identifier of draginfo struct
*
* Return Value:  NULL or pointer to draginfo structure
*                
*
***************************************************************************
*/
static  LPDRGINPROG FindDrgInfoByName(HWND hWnd,LPCDRAGINFO lpDragInfo)
{
  LPDRGINPROG lpDrgTemp = NULL;
  LPDRGINPROG lpDrgInProg = NULL;
  BOOL bDone = FALSE;
  LPCONTAINER lpCnt = GETCNT(hWnd);
 

  lpDrgTemp = lpCnt->lpDrgInProg;
  while(lpDrgTemp != NULL && !bDone)
    {
      if(lpDrgTemp->lpDragInfo != NULL)
	{
	  if((lstrcmpi(lpDragInfo->fName,lpDrgTemp->lpDragInfo->fName))== 0)
	    {
	      bDone = TRUE;
	      lpDrgInProg = lpDrgTemp;
	    }
	  else
	    lpDrgTemp =lpDrgTemp->lpNext;
	}
      else
	bDone = TRUE;  
      
    }

return(lpDrgInProg);
}
/*
***************************************************************************
* Function: FindDrgInfoByIndex
*
* Purpose:  This function will scan the list of draginfo structures and
*           either return a  pointer to a drag info structure identified
*           by nIndex or NULL.
*           
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  int           nIndex      - index identifier of draginfo struct
*
* Return Value:  NULL or pointer to draginfo structure
*                
*
***************************************************************************
*/
LPDRGINPROG FindDrgInfoByIndex(HWND hWnd, int nIndex)
{
  LPDRGINPROG lpDrgTemp = NULL;
  LPCONTAINER lpCnt = GETCNT(hWnd);

  lpDrgTemp = lpCnt->lpDrgInProg;
   while(lpDrgTemp != NULL && nIndex != lpDrgTemp->nIndex)
     lpDrgTemp =lpDrgTemp->lpNext;


return(lpDrgTemp);

}
/*
***************************************************************************
* Function: FreeDragInfo
*
* Purpose:  Will reset the lpTargetInfo pointer and if were on NT or
*           95 we will unmap our view of shared memory
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*
* Return Value:  None
*                
*
***************************************************************************
*/
VOID FreeDragInfo(HWND hWnd)
{
  LPCONTAINER lpCnt = GETCNT(hWnd);

#ifdef DAN
  #ifdef WIN32
    if(lpCnt->lpTargetInfo != NULL)
      UnmapViewOfFile(lpCnt->lpTargetInfo);  
  #endif

  lpCnt->lpTargetInfo = NULL;

#endif
}

/*
***************************************************************************
* Function: SetDragDropOverRec
*
* Purpose:  Will set the current record where the nOperation occurred.
*           It will also set the xDrop and yDrop values to the screen
*           coordinates of where the operation occurred.
*
*           if nOperation = CN_DRAGOVER lpCnt->lpDragOverRec is set
*           if nOperation = CN_DROP     lpCnt->lpDropOverRec is set
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*  int           nOperation  - Specifies what notification is occurring
*
* Return Value:  None
*                
*
***************************************************************************
*/
VOID SetDragDropOverRec(HWND hWnd, int nOperation)
{
  POINT ptMousePos;
  LPCONTAINER lpCnt = GETCNT(hWnd);

  ptMousePos.x = lpCnt->lpTargetInfo->xDrop;
  ptMousePos.y = lpCnt->lpTargetInfo->yDrop;

  // convert coordinates to client since the xDrop and yDrop
  // are in screen coordinates
  ScreenToClient(hWnd,&ptMousePos);

  // based on the operation get the record where the event
  // is taking place.
  if(nOperation == CN_DRAGOVER)
    lpCnt->lpDragOverRec = CntRecAtPtGet(hWnd, &ptMousePos );
  else if(nOperation == CN_DROP)
    lpCnt->lpDropOverRec = CntRecAtPtGet(hWnd, &ptMousePos );    

}
/*
***************************************************************************
* Function: VerifyAllowDrop
*
* Purpose:  Will contact parent to determine if drop can be handled or not
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*
* Return Value:  TRUE - Drop is allowed
*                FALSE - Drop not allowed
*
***************************************************************************
*/
LRESULT WINAPI VerifyAllowDrop(HWND hWnd)
{
  LRESULT lAllowDrop;
  POINT ptMousePos;
  LPCONTAINER lpCnt = GETCNT(hWnd);
  short sDefaultop;
  short sDrop;
  RECT rect;
 
  GetCursorPos(&ptMousePos);
  ScreenToClient(hWnd,&ptMousePos);
  GetClientRect(hWnd,&rect);

  // the actual area were records can be dropped is anywhere
  // after the title area and the column title area
  rect.top = lpCnt->lpCI->cyTitleArea + lpCnt->lpCI->cyColTitleArea;
  
  // ask owner if we can accept the drop 
  lAllowDrop = NotifyAssocWndEx( hWnd, lpCnt, CN_DRAGOVER, 0, NULL, NULL, 0, 0, 0, NULL );                  
  
  sDrop = LOWORD(lAllowDrop);

  //if the record can be dropped check to see if we are in an ok area
  if(sDrop == DOR_DROP)
    {
  
      // now check if were in the client record area that can handle the drop
      if(!PtInRect( &rect, ptMousePos ) )
	{
	  sDefaultop = HIWORD(lAllowDrop);
	  // if were not in the record area return no drop
	  lAllowDrop = MAKELRESULT(DOR_NODROP, sDefaultop);
	}
    }  

return (lAllowDrop);
}

/*
***************************************************************************
* Function: InitDragInfoFile
*
* Purpose:  This function will create the shared memory file mapping and
*           copy the draginfo struct to the shared memory
*           
*
* Parameters:
*  HWND          hWndCnt        - Container child window handle 
*
* Return Value:  TRUE -  Error in create of file mapping
*                FALSE - file was created
*
***************************************************************************
*/
static BOOL InitDragInfoFile(HWND hWndCnt,LPDRGINPROG lpDrgInProg)
{
  BOOL bRet = TRUE;

#ifdef WIN32
  char   fName[_MAX_FNAME];
  char   fExt [4];
  char   szTempPath[_MAX_PATH];
  char   szTempFile[_MAX_FNAME];
  DWORD  dwFileSize;
  SYSTEM_INFO sysInfo;
#endif

  LPCONTAINER lpCnt = GETCNT(hWndCnt);

  
#ifdef WIN32   
  GetSystemInfo(&sysInfo);
  // get temp path and generate unique file name
  GetTempPath(_MAX_PATH,szTempPath);
  GetTempFileName(szTempPath,TEMPNAME,0,szTempFile);
  _splitpath(szTempFile,NULL,NULL,fName,fExt);
  _makepath(lpDrgInProg->lpDragInfo->fName,NULL,NULL,fName,fExt);

  lpDrgInProg->hMapFileHandle =  CreateFile(szTempFile,
					   GENERIC_READ | GENERIC_WRITE,
					   FILE_SHARE_READ | FILE_SHARE_WRITE,
					   NULL,
					   CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL |FILE_FLAG_DELETE_ON_CLOSE /* | STANDARD_RIGHTS_REQUIRED |
					   FILE_MAP_WRITE | FILE_MAP_READ */,
					   NULL);


  if(lpDrgInProg->hMapFileHandle != INVALID_HANDLE_VALUE)  
    {
      // these calculations are to make the file allocation on a page allocation 
      // granularity.  This is required by 95 and not NT so we will just do for both
      if(lpDrgInProg->lpDragInfo->nDragInfo < sysInfo.dwPageSize)
      	dwFileSize = sysInfo.dwPageSize;
      else
	      {
	        dwFileSize = lpDrgInProg->lpDragInfo->nDragInfo % sysInfo.dwPageSize;
	        dwFileSize += lpDrgInProg->lpDragInfo->nDragInfo;
	      }

      sprintf(lpDrgInProg->szKey,"%ul.%d",lpCnt->dwPid,lpDrgInProg->nIndex);
      // create file mapping using generated file name
      lpDrgInProg->hDragMem = CreateFileMapping(lpDrgInProg->hMapFileHandle, NULL,PAGE_READWRITE,0,
			                                          dwFileSize,    // size of file to be created
			                                          lpDrgInProg->szKey);  // name of file to be create

  
	
      if(lpDrgInProg->hDragMem != NULL)
	      {
	        lpDrgInProg->lpDragInfo->nDropIndex = lpDrgInProg->nIndex;
	        lpDrgInProg->lpDragView = MapViewOfFile(lpDrgInProg->hDragMem,FILE_MAP_READ|FILE_MAP_WRITE,0,0,0);
	        _fmemcpy( lpDrgInProg->lpDragView, lpDrgInProg->lpDragInfo, (size_t) lpDrgInProg->lpDragInfo->nDragInfo);
	        bRet = FALSE;

	      }
    }
#else

  if(lpDrgInProg->lpDragInfo != NULL)
    {
      lpDrgInProg->lpDragInfo->nDropIndex = lpDrgInProg->nIndex;
      bRet = FALSE;
    }


#endif
  return(bRet);
}

static void DisplayCursor(LPCONTAINER lpCnt,LPDRGINPROG lpDrgInProg,UINT uOp)
{
	// otherwise set drop allowed cursor
  
	if(lpDrgInProg->lpDragInfo->nItems == 1 && uOp == DO_COPY)
    SetCursor(lpCnt->lpCI->hDropCopySingle);
  else if(lpDrgInProg->lpDragInfo->nItems == 1 && uOp == DO_MOVE)
    SetCursor(lpCnt->lpCI->hDropMoveSingle);
  else if(lpDrgInProg->lpDragInfo->nItems > 1 && uOp == DO_COPY)
    SetCursor(lpCnt->lpCI->hDropCopyMulti);
  else if(lpDrgInProg->lpDragInfo->nItems > 1 && uOp == DO_MOVE)
    SetCursor(lpCnt->lpCI->hDropMoveMulti);
  else
    SetCursor(lpCnt->lpCI->hDropNotAllow);

}

/*
***************************************************************************
* Function: DoDragTracking
*
* Purpose:  This function will determine and track the drag selection of 
*           of mouse movements
*
* Parameters:
*  HWND          hWnd        - Container child window handle 
*
* Return Value:  TRUE - Dragging was done
*                FALSE - Dragging was not done
*
***************************************************************************
*/
BOOL WINAPI DoDragTracking(HWND hWnd)
{
  BOOL    fOkToDrop;
  BOOL bShiftKey = FALSE;
  BOOL bCtrlKey = FALSE;
  BOOL bDragStarted = FALSE;
  POINT   ptMousePos;
  POINT   ptStartPos;
  HWND    hWndSubject = NULL;
  HWND    hWndPrevious = NULL;
  BOOL    bFirstTime = TRUE;
  MSG     msgModal;
  BOOL    bTracking = TRUE;
  BOOL    bDragged = FALSE;
  BOOL    bNeverDrop = FALSE;
  BOOL bFreeNotAllowed = FALSE;
  BOOL bFreeCopySingle = FALSE;
  BOOL bFreeCopyMulti = FALSE;
  BOOL bFreeMoveSingle = FALSE;
  BOOL bFreeMoveMulti = FALSE;
  LRESULT lResult = 0;
  LRESULT lOperation;
  LPARAM  lParam;
  WPARAM  wParam;
  HANDLE  hMapHandle = NULL;
  LPDRGINPROG lpDrgInProg = NULL;
  LPCONTAINER lpCnt = GETCNT(hWnd);
  HCURSOR hOrigCursor = NULL;

  
  // get mouse position when we start
  GetCursorPos(&ptStartPos);

  /* next cursors are in fact unused, since the drag/drop functionality is
     now disabled in this DLL (unused anyhow in VDBA) */

  // if drag cursor handles are not set load them
  if(!lpCnt->lpCI->hDropNotAllow)
    {
      lpCnt->lpCI->hDropNotAllow = LoadCursor(NULL, IDC_NO);
      bFreeNotAllowed = TRUE;
    }
  
  if(!lpCnt->lpCI->hDropCopySingle)
    {
      lpCnt->lpCI->hDropCopySingle = LoadCursor(NULL,IDC_UPARROW);
      bFreeCopySingle = TRUE;
    }

  if(!lpCnt->lpCI->hDropCopyMulti)
    {
      lpCnt->lpCI->hDropCopyMulti = LoadCursor(NULL,IDC_UPARROW);
      bFreeCopyMulti = TRUE;
    }

  if(!lpCnt->lpCI->hDropMoveMulti)
    {
      lpCnt->lpCI->hDropMoveMulti = LoadCursor(NULL,IDC_UPARROW);
      bFreeMoveMulti = TRUE;
    }

  if(!lpCnt->lpCI->hDropMoveSingle)
    {
      lpCnt->lpCI->hDropMoveSingle = LoadCursor(NULL,IDC_UPARROW);
      bFreeMoveSingle = TRUE;
    }


  // *** Loop while determining the drop site
  
  SetCapture(hWnd);
  while( bTracking )
  {
    while( !PeekMessage( &msgModal, NULL, 0, 0, PM_REMOVE ) )
      WaitMessage();

    switch( msgModal.message )
      {
        case WM_KEYDOWN:
            // we only process this if were already
            // dragging
            if(bDragStarted)
              {
                UINT uPrevOp;
                int nVirtKey = msgModal.wParam;

                if(nVirtKey == VK_CONTROL)
                  {
                    // the key is down is it the first time
                    if(!bCtrlKey)
                      {
                        bCtrlKey = TRUE;
                        // notify app of control key state change
                        uPrevOp = lpDrgInProg->lpDragInfo->uDragOp;
    	                  lOperation = NotifyAssocWndEx( hWnd, lpCnt, CN_DRAGCTRLDOWN, 0, NULL,
    	                                                 NULL, 0, bShiftKey, bCtrlKey, NULL );
                        
                        // set correct cursor if it has changed
                        // drag operations (copy/move)
                        if((UINT) lOperation != uPrevOp)
                          {
                            lpDrgInProg->lpDragInfo->uDragOp = (UINT)lOperation;
                            DisplayCursor(lpCnt,lpDrgInProg,(UINT)lOperation);
                          }
                        
                        
                      }
                  }
                else if (nVirtKey == VK_SHIFT)
                  {
                    // the key is down is it the first time
                    if(!bShiftKey)
                      {
                        bShiftKey = TRUE;
                        uPrevOp = lpDrgInProg->lpDragInfo->uDragOp;
    	                  lOperation = NotifyAssocWndEx( hWnd, lpCnt, CN_DRAGSHFTDOWN, 0, NULL,
    	                                                 NULL, 0, bShiftKey, bCtrlKey, NULL );
                        
                        // set correct cursor if it has changed
                        // drag operations (copy/move)
                        if((UINT)lOperation != uPrevOp)
                          {
                            lpDrgInProg->lpDragInfo->uDragOp = (UINT)lOperation;
                            DisplayCursor(lpCnt,lpDrgInProg,(UINT)lOperation);
                          }

                      }

                  }
              }
        break;

        case WM_KEYUP:
            // we only process this if were already
            // dragging
            if(bDragStarted)
              {
                UINT uPrevOp;
                int nVirtKey = msgModal.wParam;

                if(nVirtKey == VK_CONTROL)
                  {
                    // was the key down
                    if( bCtrlKey )
                      {
                        bCtrlKey = FALSE;
                        // notify app of shift key state change
                        uPrevOp = lpDrgInProg->lpDragInfo->uDragOp;
    	                  lOperation = NotifyAssocWndEx( hWnd, lpCnt, CN_DRAGCTRLUP, 0, NULL,
    	                                                 NULL, 0, bShiftKey, bCtrlKey, NULL );

                        // set correct cursor if it has changed
                        // drag operations (copy/move)
                        if((UINT)lOperation != uPrevOp)
                          {
                            lpDrgInProg->lpDragInfo->uDragOp = (UINT)lOperation;
                            DisplayCursor(lpCnt,lpDrgInProg,(UINT)lOperation);
                          }

                      }
                  }
                else if (nVirtKey == VK_SHIFT)
                  {
                    // was the key down
                    if( bShiftKey )
                      {
                        bShiftKey = FALSE;
                        // notify app of shift key state change
                        uPrevOp = lpDrgInProg->lpDragInfo->uDragOp;

    	                  lOperation = NotifyAssocWndEx( hWnd, lpCnt, CN_DRAGSHFTUP, 0, NULL,
    	                                                 NULL, 0, bShiftKey, bCtrlKey, NULL );

                        // set correct cursor if it has changed
                        // drag operations (copy/move)
                        if((UINT)lOperation != uPrevOp)
                          {
                            lpDrgInProg->lpDragInfo->uDragOp = (UINT)lOperation;
                            DisplayCursor(lpCnt,lpDrgInProg,(UINT)lOperation);
                          }


                      }

                  }

              }

        break;

	case WM_MOUSEMOVE:

	    // Get cursor pos. & window under the cursor
	    GetCursorPos(&ptMousePos);
	    // dont flip cursor till we move out of delta
	    if(ptStartPos.x + 6 < ptMousePos.x || 
	       ptStartPos.y + 6 < ptMousePos.y ||
	       ptStartPos.x - 6 > ptMousePos.x ||
	       ptStartPos.y - 6 > ptMousePos.y)
	      {
        	// if we are just starting our drag operation
        	// allocate cntinitdrag struct and notify our parent
        	if(bFirstTime)
        	  {
              bDragStarted = TRUE;
        	    hOrigCursor = GetCursor();
        	    bFirstTime = FALSE;

        	    lpDrgInProg = AllocDrgInfo(hWnd,&ptStartPos);
        	    // Get the state of the shift and control keys.
        	    bShiftKey = GetAsyncKeyState( VK_SHIFT ) & ASYNC_KEY_DOWN;
        	    bCtrlKey  = GetAsyncKeyState( VK_CONTROL ) & ASYNC_KEY_DOWN;


        	    // if our parent responds non zero set cursor to never drop regardless
        	    if(NotifyAssocWndEx( hWnd, lpCnt, CN_INITDRAG, 0, NULL, NULL, 0, bShiftKey, bCtrlKey, NULL ))
        	      bNeverDrop = TRUE;
        	    else
        	      {
              		// initialize shared memory file and if we cant display nodropallowed
            	  	if(InitDragInfoFile(hWnd,lpDrgInProg))
        		        bNeverDrop = TRUE;
        		      else
        		        {
                      // set initial value of drag operation
                      lOperation = lpDrgInProg->lpDragInfo->uDragOp;
#ifdef WIN32                          
      			          // set lparam to processid of share memory area
      			          lParam = (LPARAM) lpCnt->dwPid;


      			          // Set wParam to index number  of share memory area
      			          wParam = (WPARAM) lpDrgInProg->nIndex;
			    
#else       
      			          lParam = (LPARAM) lpDrgInProg->lpDragInfo;
      			          wParam = lpDrgInProg->nIndex;
#endif                                       

      			        }
		            }
		   
		        }
		
		      hWndSubject = WindowFromPoint(ptMousePos);

      		// if what were over is a window and the CN_INITDRAG says its ok to continue
      		// the drag operation then check its style bits
      		if (IsWindow(hWndSubject) && !bNeverDrop)
      		  { 
#ifdef WIN32                    
      		    LPCDRAGINFO lpTemp = (LPCDRAGINFO)lpDrgInProg->lpDragView;
#else
		          LPCDRAGINFO lpTemp = lpDrgInProg->lpDragInfo;
#endif

      		    // update where the mouse is now for the dragover event
      		    lpTemp->xDrop = ptMousePos.x;
      		    lpTemp->yDrop = ptMousePos.y;          

              // if the type of drag operation has changed
              // update the drag info struct for the user
              if(lpTemp->uDragOp != (UINT)lOperation)
                lpTemp->uDragOp = (UINT)lOperation;
#ifdef WIN32
      		    // flush the memory to our memory mapped file
      		    // only flush the first part (ie DRAGINFO information)
      		    FlushViewOfFile(lpDrgInProg->lpDragView,LEN_DRAGINFO);

#endif
      		    // now see if it can handle what were going to drop
      		    lResult = SendMessage(hWndSubject,lpCnt->uWmDragOver,wParam,lParam);

      		    // did the window respond that it can accept the drop
      		    if(LOWORD(lResult) != DOR_DROP)
      		      {
      			      // if not set no drop allowed cursor
      			      fOkToDrop = FALSE;
      			      SetCursor(lpCnt->lpCI->hDropNotAllow);
			
      		      }
      		    else
      		      {
            			fOkToDrop = TRUE;
            			// otherwise set drop allowed cursor
                  DisplayCursor(lpCnt,lpDrgInProg,lpDrgInProg->lpDragInfo->uDragOp);

      		      }


      		    if(hWndPrevious == NULL)
      		      hWndPrevious = hWndSubject;
      		    else if (hWndPrevious != hWndSubject)
      		      {
            			//notify previous window we left if during our drag
            			SendMessage(hWndSubject,lpCnt->uWmDragLeave,wParam,lParam);    
            			// update previous handle
            			hWndPrevious = hWndSubject;
      		      }

		        }
      		else
      		  {
      		    // is not a window set no drop allowed cursor
      		    fOkToDrop = FALSE;
      		    SetCursor(lpCnt->lpCI->hDropNotAllow);
      		  }

	      }
	continue;
	break;

	case WM_LBUTTONUP:              // End of selection drag          
	   bTracking = FALSE;
	   GetCursorPos(&ptMousePos);

	   // check to see if we actually dragged or user had a shakey hand
	   if(bDragStarted && !bNeverDrop)
	      {

		      bDragged = TRUE;
		 
      		// check last response from our target
      		// if the reply was DOR_DROP then send the drop message
      		if(LOWORD(lResult) == DOR_DROP)
      		  {
#ifdef WIN32                    
      		    LPCDRAGINFO lpTemp = (LPCDRAGINFO)lpDrgInProg->lpDragView;
#else
		          LPCDRAGINFO lpTemp = lpDrgInProg->lpDragInfo;
#endif
      		    // update where the mouse is now for the drop event
      		    lpTemp->xDrop = ptMousePos.x;
      		    lpTemp->yDrop = ptMousePos.y;          

              if(lpTemp->uDragOp != (UINT)lOperation)
                lpTemp->uDragOp = (UINT)lOperation;


#ifdef WIN32
      		    // flush the memory to our memory mapped file
      		    // only flush the first part (ie DRAGINFO information)
      		    FlushViewOfFile(lpDrgInProg->lpDragView,LEN_DRAGINFO);

#endif
      		    // post wm_cntdrop message to target window to notify
      		    // it that the drop occurred
      		    PostMessage(hWndSubject,lpCnt->uWmDrop,wParam,lParam);    
      		  }
      		 else
      		  {
      		    if(lpDrgInProg != NULL)
      		      FreeDrgInfo(hWnd,lpDrgInProg->nIndex);
      		  }  
		  
    		  SetCursor(hOrigCursor);
		      NotifyAssocWndEx( hWnd, lpCnt, CN_DRAGCOMPLETE, 0, NULL, NULL, 0, 0, 0, NULL );           
	      }
	   
  	   // since we ate this message we need to post it to ourself so as proper
  	   // clean up can be done
  	   PostMessage(msgModal.hwnd,WM_LBUTTONUP,msgModal.wParam,msgModal.lParam);
	      
	continue;
	break;
     }
    TranslateMessage( &msgModal );
    DispatchMessage( &msgModal );
  }

  ReleaseCapture();

  // Free the cursors we loaded memory
  if(bFreeNotAllowed)
    DestroyCursor(lpCnt->lpCI->hDropNotAllow);

  if(bFreeCopySingle)
    DestroyCursor(lpCnt->lpCI->hDropCopySingle);

  if(bFreeCopyMulti)
    DestroyCursor(lpCnt->lpCI->hDropCopyMulti);
    
  if(bFreeMoveSingle)
    DestroyCursor(lpCnt->lpCI->hDropMoveSingle);

  if(bFreeMoveMulti)
    DestroyCursor(lpCnt->lpCI->hDropMoveMulti);

return(bDragged);
}



