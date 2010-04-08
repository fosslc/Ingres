/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTBRW.H
 *
 * Typedefs and data structures needed to interface the DLL custom control
 * to Borland's Resource Workshop dialog editor.
 ****************************************************************************/

typedef HANDLE (CALLBACK* LPFNINFO)(void);
typedef BOOL   (CALLBACK* LPFNSTYLE)(HWND, HANDLE, LPFNSTRTOID, LPFNIDTOSTR);
typedef UINT   (CALLBACK* LPFNFLAGS)(DWORD, LPSTR, UINT);

typedef HANDLE (CALLBACK* LPFNLOADRES)(LPCSTR, LPSTR);
typedef VOID   (CALLBACK* LPFNEDITRES)(LPCSTR, LPSTR);

typedef struct tagRWCTLCLASS
    {
    LPFNINFO  fnRWInfo;             // Info function
    LPFNSTYLE fnRWStyle;            // Style function
    LPFNFLAGS fnFlags;              // Flags function
    char      szClass[CTLCLASS];    // Class name
    } RWCTLCLASS;

typedef RWCTLCLASS FAR *LPRWCTLCLASS;

typedef struct tagCTLCLASSLIST
    {
    short      nClasses;            // Number of classes in list
    RWCTLCLASS Classes[];           // Class list
    } CTLCLASSLIST;

typedef CTLCLASSLIST FAR *LPCTLCLASSLIST;

typedef struct tagRWCTLTYPE
    {
    UINT       wType;               // type style
    UINT       wWidth;              // suggested width
    UINT       wHeight;             // suggested height
    DWORD      dwStyle;             // default style
    char       szDescr[CTLDESCR];   // menu name
    HBITMAP    hToolBit;            // Toolbox bitmap
    HCURSOR    hDropCurs;           // Drag and drop cursor
    } RWCTLTYPE;

typedef RWCTLTYPE FAR *LPRWCTLTYPE;

typedef struct tagRWCTLINFO
    {
    UINT       wVersion;            // control version
    UINT       wCtlTypes;           // control types
    char       szClass[CTLCLASS];   // control class name
    char       szTitle[CTLTITLE];   // control title
    char       szReserved[10];      // reserved for future use
    RWCTLTYPE  Type[CTLTYPES];      // control type list
    } RWCTLINFO;

typedef RWCTLINFO     *RWPCTLINFO;
typedef RWCTLINFO FAR *LPRWCTLINFO;

#define CTLDATALENGTH      100      // ?????

typedef struct
    {                            
    UINT   wX;                      // x origin of control
    UINT   wY;                      // y origin of control
    UINT   wCx;                     // width of control
    UINT   wCy;                     // height of control
    UINT   wId;                     // control child id
    DWORD  dwStyle;                 // control style
    char   szClass[CTLCLASS];       // name of control class
    char   szTitle[CTLTITLE];       // control text
    BYTE   CtlDataSize;             // control data size
    BYTE   CtlData[CTLDATALENGTH];  // control data
    } RWCTLSTYLE;

typedef RWCTLSTYLE     *PRWCTLSTYLE;
typedef RWCTLSTYLE FAR *LPRWCTLSTYLE;
