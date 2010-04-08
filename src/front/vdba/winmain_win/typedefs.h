/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : CA/OpenIngres Visual DBA
**
**  Source : typedefs.h
**  application typedefs
**  Sub-section taken from cavo.e 8/3/95, adapted by Emb
**
**  History:
**	08-feb-2000 (somsa01)
**	    Removed redefines of system type definitions.
*/

//
// Modifiers
//
#ifdef WIN32
    #define HUGE
    #define EXPORT
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
//#define INT                 int             /* i or nothing */
#define SHORT               short           /* s            */

typedef WORD                SEL;            /* sel          */
typedef UINT                RETCODE;        /* rc           */
typedef DWORD               XHANDLE;        /* xh           */
typedef long double         XDOUBLE;        /* xd           */

typedef unsigned long       TIME;           /* t, portable? */

//
// pointers
//

typedef LPSTR               LPSZ;           /* lpsz         */
typedef SHORT FAR           *LPSHORT;       /* lpsi         */
typedef USHORT FAR          *LPUSHORT;      /* lpus         */
typedef SEL FAR             *LPSEL;         /* lpsel        */
typedef ULONG FAR           *LPULONG;       /* lpul         */
typedef CHAR FAR            *LPCHAR;        /* lpch         */
typedef CHAR HUGE           *HPCHAR;        /* hpch         */
//???typedef UCHAR NEAR          *LPUCHAR;       /* lpuch        */
typedef HFILE FAR           *LPHFILE;       /* lphf         */
typedef HWND FAR            *LPHWND;        /* lphw         */
typedef XHANDLE FAR         *LPXHANDLE;     /* lpxh         */
typedef XDOUBLE FAR         *LPXDOUBLE;     /* lpxd         */
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

