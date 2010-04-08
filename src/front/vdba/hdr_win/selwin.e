/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : selwin.e
//
//    
//
********************************************************************/
// These functions subclass standard MDI applications
// When the user Double Click the left mouse button
// insidle the mdi client area, a dialog box containing all
// MDI Childs windows is popped and allow selecting, closing, restoring etc ...

// These two functions are called After creating and befor destroying
// the MDI application
extern VOID WINAPI MdiSelWinInit(HWND hMdiClientWnd);
extern VOID WINAPI MdiSelWinEnd (HWND hMdiClientWnd);


// This function create the dialog box containing MDI childs
// it can be called when selecting the "More Windows ..." menu item
// of the main MDI menu

extern VOID WINAPI MdiSelWinBox (HWND hMdiClientWnd);
