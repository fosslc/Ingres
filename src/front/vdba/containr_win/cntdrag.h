/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTDRAG.H
 *
 * Contains drag drop prototypes
 ****************************************************************************/

#ifndef CATODRAG_H   /* wrapper */
#define CATODRAG_H 

#ifdef  __cplusplus
extern "C" {
#endif

// prefix used for generating tempfile names
#define TEMPNAME "DRG"
#define DRAGINFO "DragInfo"

LRESULT WINAPI VerifyAllowDrop(HWND hWnd);
BOOL WINAPI DoDragTracking(HWND hWnd);
VOID FreeDragInfo(HWND hWnd);
LPCDRAGINFO GetDragInfo(HWND hWndCnt, WPARAM wParam, LPARAM lParam);
VOID SetDragDropOverRec(HWND hWnd, int nOperation);
LPDRGINPROG FindDrgInfoByIndex(HWND hWnd, int nIndex);
LRESULT Process_DefaultMsg( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam );


#ifdef  __cplusplus
}
#endif

#endif   /* end wrapper */
