/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : aspen.h
//
//     
// History:
// uk$so01 (20-Jan-2000, Bug #100063)
//         Eliminate the undesired compiler's warning
//	14-Feb-2000 (somsa01)
//	    Removed typedefs of system types.
//	07-May-2009 (drivi01)
//	    Undefine MAXUINT because it's already
//	    defined in Visual Studio 2008.
//      02-Jun-2010 (drivi01)
//	    Redefine INT if mecl.h is included b/c we want
//          the definition in this file to persist in Visual tools.
********************************************************************/
//
//
// ASPEN.H
//
// 07/28/92 GM
// 03/31/93 UH
//
//

#ifndef ASPEN_INCLUDED
#define ASPEN_INCLUDED

#if defined(SYS_WIN16)
    #include <windows.h>
    #define SYS_16BIT
#elif defined(SYS_WIN32)
    #include <windows.h>
    #define SYS_32BIT
#else
    #define SYS_32BIT
#endif



//
// Modifiers
//


#ifdef WIN32
#define EXPORT
#define HUGE
#else
#define HUGE                huge
#define EXPORT              PASCAL FAR _export
#endif
#define APIFUNC             PASCAL FAR
#define CAPIFUNC            _cdecl FAR
#define EXTERN              extern
#define STATIC              static


//
// basic types
//

#define CHAR                char            /* ch           */
#ifdef INT
#undef INT
#define INT                 int             /* i or nothing */
#endif
#define SHORT               short           /* s            */

typedef WORD                SEL;            /* sel          */
typedef double              DOUBLE;         /* d            */
typedef UINT                RETCODE;        /* rc           */
typedef DWORD               XHANDLE;        /* xh           */

typedef unsigned long       TIME;           /* t, portable? */

//
// pointers
//

typedef LPSTR               LPSZ;           /* lpsz         */
typedef USHORT FAR          *LPUSHORT;      /* lpus         */
typedef SEL FAR             *LPSEL;         /* lpsel        */
typedef ULONG FAR           *LPULONG;       /* lpul         */
typedef CHAR FAR            *LPCHAR;        /* lpch         */
typedef CHAR HUGE           *HPCHAR;        /* hpch         */
typedef UCHAR NEAR          *LPUCHAR;       /* lpuch        */
typedef HFILE FAR           *LPHFILE;       /* lphf         */
typedef HWND FAR            *LPHWND;        /* lphw         */
typedef XHANDLE FAR         *LPXHANDLE;     /* lpxh         */
typedef VOID                (FAR* LPFUNC)();/* lpfn         */
typedef TIME FAR            *LPTIME;        /* lpTime       */

typedef LPCHAR FAR          *LPLPCHAR;              // lplpch
typedef LPBYTE FAR          *LPLPBYTE;              // lplpb
typedef LPWORD FAR          *LPLPWORD;              // lplpw
typedef LPUSHORT FAR        *LPLPUSHORT;            // lplpus
typedef PSZ FAR             *PPSZ;                  // ppsz
typedef LPSZ FAR            *LPLPSZ;                // lplpsz
typedef LPSTR FAR           *LPLPSTR;               // lplpstr
typedef LPVOID FAR          *LPLPVOID;              // lplp


//
// Force old style pointers to conform or fail
//

#ifdef OS2_CONVENTIONS

    #define PBYTE       LPBYTE
    #define PWORD       LPWORD
    #define PDWORD      LPDWORD
    #define PUINT       LPUINT

    #define PINT        LPINT
    #define PLONG       LPLONG
    #define PBOOL       LPBOOL
    #define PSEL        LPSEL
    #define PVOID       LPVOID
    #define PUSHORT     LPUSHORT
    #define PULONG      LPULONG
    #define PCHAR       LPCHAR

    #define PUCHAR      LPUCHAR
    #define PHANDLE     LPHANDLE
    #define PHFILE      LPHFILE
    #define PHWND       LPHWND
    #define PXHANDLE    LPXHANDLE
    #define PFUNC       LPFUNC

    #define PPCHAR      LPLPCHAR
    #define PPBYTE      LPLPBYTE
    #define PPWORD      LPLPWORD
    #define PPUSHORT    LPLPUSHORT
    #define PPVOID      LPLPVOID

#else

    #define PBYTE       old_style_pointer_decl
    #define PWORD       old_style_pointer_decl
    #define PDWORD      old_style_pointer_decl
    #define PUINT       old_style_pointer_decl

    #define PINT        old_style_pointer_decl
    #define PLONG       old_style_pointer_decl
    #define PBOOL       old_style_pointer_decl
    #define PSEL        old_style_pointer_decl
    #define PVOID       old_style_pointer_decl
    #define PUSHORT     old_style_pointer_decl
    #define PULONG      old_style_pointer_decl
    #define PCHAR       old_style_pointer_decl

    #define PUCHAR      old_style_pointer_decl
    #define PHANDLE     old_style_pointer_decl
    #define PHFILE      old_style_pointer_decl
    #define PHWND       old_style_pointer_decl
    #define PXHANDLE    old_style_pointer_decl
    #define PFUNC       old_style_pointer_decl

    #define PPCHAR      old_style_pointer_decl
    #define PPBYTE      old_style_pointer_decl
    #define PPWORD      old_style_pointer_decl
    #define PPUSHORT    old_style_pointer_decl
    #define PPVOID      old_style_pointer_decl

#endif

//
// Macros
//

#define MAKEP(sel, off)     ((LPVOID)MAKEULONG(off,sel))
#define MAKEULONG(l, h)     ((ULONG)(((WORD)(l))|((DWORD)((WORD)(h)))<<16))
#define MAKEDWORD(l, h)     ((DWORD)(((WORD)(l))|((DWORD)((WORD)(h)))<<16))
#define MAKEUSHORT(l, h)    ((USHORT)((WORD)(l))|((WORD)(h))<<8)
#if !defined (_WINDEF_)
#define MAKEWORD(l, h)      ((WORD)((WORD)(l))|((WORD)(h))<<8)
#endif
#define MAKESHORT(l, h)     ((SHORT)MAKEUSHORT(l,h))
#define MAKETYPE(v, t)      (*((t FAR *)&v))

#define NEG(u)              (0-u)

#define LOUCHAR(w)          ((UCHAR)(USHORT)(w))
#define HIUCHAR(w)          ((UCHAR)(((USHORT)(w)>>8)&0xff))
#define LOUSHORT(l)         ((USHORT)(ULONG)(l))
#define HIUSHORT(l)         ((USHORT)(((ULONG)(l)>>16)&0xffff))

#define LOUCHAROF(w)        (((LPUCHAR)&(w))[0])
#define HIUCHAROF(w)        (((LPUCHAR)&(w))[1])
#define LOUSHORTOF(l)       (((LPUSHORT)&(l))[0])
#define HIUSHORTOF(l)       (((LPUSHORT)&(l))[1])
#define LOWORDOF(l)         (((LPWORD)&(l))[0])
#define HIWORDOF(l)         (((LPWORD)&(l))[1])
#define LOBYTEOF(w)         (((LPBYTE)&(w))[0])
#define HIBYTEOF(w)         (((LPBYTE)&(w))[1])

#undef OFFSETOF
#undef SELECTOROF

#define OFFSETOF(lp)        (((LPWORD)&(lp))[0])
#define SELECTOROF(lp)      (((LPWORD)&(lp))[1])




//
// constants
//

#undef  TRUE
#undef  FALSE
#define TRUE            1
#define FALSE           0

#if !defined (_WINNT_)
#define MAXDWORD        ((DWORD)0xFFFFFFFFL)
#define MAXWORD         ((WORD)0xFFFF)
#endif

#ifdef SYS_WIN16
    #define MAXFILENAME     (260)
    #define MAXDRIVERNAME   (12)
    #define MAX_KEY_LEN     (256)
    #define DB_MAXAREAS     (256)

#ifndef _INC_STDLIB
    #define _MAX_PATH       (260)   /* max. length of full pathname       */
    #define _MAX_DRIVE      (3)     /* max. length of drive component     */
    #define _MAX_DIR        (256)   /* max. length of path component      */
    #define _MAX_FNAME      (256)   /* max. length of file name component */
    #define _MAX_EXT        (256)   /* max. length of extension component */
#endif // _INC_STDLIB

#else
    #define MAXFILENAME     (260)
    #define MAXDRIVERNAME   (260)
    #define MAX_KEY_LEN     (256)
    #define DB_MAXAREAS     (256)

#ifndef _INC_STDLIB
    #define _MAX_PATH       (260)   /* max. length of full pathname       */
    #define _MAX_DRIVE      (3)     /* max. length of drive component     */
    #define _MAX_DIR        (256)   /* max. length of path component      */
    #define _MAX_FNAME      (256)   /* max. length of file name component */
    #define _MAX_EXT        (256)   /* max. length of extension component */
#endif // _INC_STDLIB

#endif


#undef  NULL
#define NULL            ((LPVOID)0L)

#undef  ZERO
#define ZERO            ((UCHAR)'\0')

#undef  NOTHING
#define NOTHING         ((UINT)0)

#undef SUCCESS
#undef FAILURE
#undef NOSUCCESS
#define SUCCESS         ((UINT)0)
#define FAILURE         ((UINT)0xFFFF)
#define NOSUCCESS       (FAILURE)



//
// USUAL support
//

typedef struct _USUAL                       // polymorphic value
    {                                       //
    union                                   //
        {                                   //
        INT     i;                          //
        WORD    w;                          //
        BOOL    f;                          //
        DWORD   dw;                         //
        LONG    l;                          //
        LPVOID  lp;                         //
        struct                              //
            {                               //
            WORD    wLow;                   //
            WORD    wHigh;                  //
            }ww;                            //
        }value;                             //
    WORD    wTag;                           //
    } USUAL, FAR *LPUSUAL;                  //



//
// Macros to get typed values from polymorphes
//

#define USUAL_TAG(u)    ((u).wTag)
#define USUAL_PTR(u)    ((u).value.lp)
#define USUAL_PSZ(u)    ((PSZ)((u).value.dw))
#define USUAL_INT(u)    ((u).value.i)
#define USUAL_WORD(u)   ((u).value.w)
#define USUAL_LONG(u)   ((u).value.l)
#define USUAL_DWORD(u)  ((u).value.dw)
#define USUAL_STRING(u) ((STRING)((u).value.dw))
#define USUAL_VOREAL(u) ((VOREAL)((u).value.dw))
#define USUAL_LOGIC(u)  ((LOGIC)(u).value.f)
#define USUAL_CODE(u)   ((CODEBLOCK)((u).value.dw))

#define USUAL_AS(u,t)   MAKE_TYPE(u,t)

#define USUAL_LOW(u)    ((u).value.ww.wLow)
#define USUAL_HIGH(u)   ((u).value.ww.wHigh)



#define BT_VOID         0       // == NIL
#define BT_LONG         1       // signed long
#define BT_DATE         2       //
#define BT_FLOAT        3       // internal real format
#define BT_FIXED        4       // internal fixed format
#define BT_ARRAY        5       // array (ptr)
#define BT_OP           6       // object pointer
#define BT_STR          7       // string
#define BT_LOGIC        8
#define BT_CODE         9       // code block
#define BT_SYMBOL       10      // atom nr in symbol atom table

#define BT_BYTE         11      // byte
#define BT_INT          12      // signed int
#define BT_WORD         13      // word
#define BT_DWORD        14      // dword
#define BT_REAL4        15      // C float
#define BT_REAL8        16      // C double
#define BT_PSZ          17      // C double
#define BT_PTR          18      // raw ptr
#define BT_POLY         19      // polymorphic


//------------------------------------------------------------------------
//
// Common VO/C error structure
//

typedef struct _ERRORINFO
    {
    LPBYTE          pszSubSystem    ; // PSZ
    UINT            wSubCode        ; // WORD
    UINT            wGenCode        ; // WORD   _EG
    UINT            wOsCode         ; // WORD
    UINT            wSeverity       ; // WORD
    BOOL            lCanDefault     ; // LOGIC
    BOOL            lCanRetry       ; // LOGIC
    BOOL            lCanSubstitute  ; // LOGIC
    LPBYTE          pszOperation    ; // PSZ
    LPBYTE          pszDescription  ; // PSZ
    LPBYTE          pszFileName     ; // PSZ
    LPVOID          strucArgs       ; // _ARGS
    UINT            wTries          ; // WORD

    ATOM            symFuncSym      ; // SYMBOL
    HFILE           wFileHandle     ; // WORD
    UINT            wArgNum         ; // WORD
    LPVOID          ptrFuncPtr      ; // PTR
    LPBYTE          pszSubCodeText  ; // PSZ
    LPBYTE          pszArg          ; // PSZ
    UINT            wArgType        ; // WORD
    UINT            wArgTypeReq     ; // WORD
    UINT            wMaxSize        ; // WORD
    UINT            wSubstituteType ; // WORD
    UINT            wChoice         ; // WORD
    UINT            symCallFuncSym  ; // SYMBOL

    } ERRORINFO, FAR *LPERRORINFO;



//------------------------------------------------------------------------
//
// Workarea status (debugger support)
//

typedef struct _WORKAREASTATUS
    {

    //
    //  Workarea related informations
    //

    UINT            uiSelect;                   // Workarea number
    ATOM            atomAlias;                  // Alias name as symbol
    CHAR            achDriver[MAXDRIVERNAME];   // RDD name
    LONG            lRecno;                     // Record number
    LONG            lLastRec;                   // Number of records
    UINT            uiRecSize;                  // Record size
    UINT            uiHeader;                   // Header size
    UINT            uiFieldCount;               // Number of fields
    LONG            lLastUpdate;                // Date of last update
    UINT            uiIndexOrd;                 // Current activ order
    BOOL            fFound;                     // Found flag
    BOOL            fBOF;                       // Begin of file flag
    BOOL            fEOF;                       // End of file flag
    BOOL            fFlocked;                   // File locked ?
    BOOL            fAnsi;                      // Ansi file ?
    UINT            uiRecLocks;                 // Number of locked records
    UINT            uiIndexCount;               // Number of activ orders
    UINT            uiRelations;                // Number of activ relations
    HFILE           hfDbf;                      // File handle
    CHAR            achFileName[MAXFILENAME];   // File name

    //
    //  Current record based informations
    //

    BOOL            fDeleted;                   // Current record deleted ?
    BOOL            fRLocked;                   // Current record locked  ?

    }   WORKAREASTATUS, FAR *LPWORKAREASTATUS;



//------------------------------------------------------------------------
//
// Order status (debugger support)
//

typedef struct _ORDERSTATUS
    {

    //
    //  Order related informations  (index status)
    //

    UINT            uiSelect;                   // Workarea number
    UINT            uiOrder;                    // Order number
    CHAR            achBagName[MAXFILENAME];    // Name of order bag

    HFILE           hfTag;                      // Handle number

    BOOL            fConditional;               // Is conditional ?
    BOOL            fDescending;                // Is descending ?
    BOOL            fHasFocus;                  // Is it the current activ ?

    UINT            uiKeyType;                  // Key type
    UINT            uiKeySize;                  // Key size

    LONG            lPos;                       // Position of current record

    CHAR            achKey[MAX_KEY_LEN];        // Key expression
    CHAR            achFor[MAX_KEY_LEN];        // Condition expression
    CHAR            achOrder[MAXFILENAME];      // Name of order
    CHAR            achFileName[MAXFILENAME];   // File name

    }   ORDERSTATUS, FAR *LPORDERSTATUS;



//------------------------------------------------------------------------
//
// Define name of INI file
//


#define INI_FILE    "CACI.INI"

#endif // ASPEN_INCLUDED


/*
** eof
*/
