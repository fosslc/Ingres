/*
**  IDMSOVER.H -- Ingres ODBC Driver Version Info
**
** Note: You'll need to update caiiodbc.rc manually
*/


/* ----- VS_VERSION.dwFileFlags ----- */
#define VS_FF_DEBUG             0x00000001L
#define VS_FF_PRERELEASE        0x00000002L

/* ----- VS_VERSION.dwFileOS ----- */
#define VOS__WINDOWS32          0x00000004L
#define VOS_DOS_WINDOWS16       0x00010001L

#define VER_FILEVERSION     03,50,1010,122
#define VER_FILEVERSION_STR " 3.50.1010.0122\0"

#define VER_PRODUCTVERSION      VER_FILEVERSION
#define VER_PRODUCTVERSION_STR  VER_FILEVERSION_STR

#define VER_FILEFLAGSMASK (VS_FF_DEBUG | VS_FF_PRERELEASE)
#ifdef DEBUG
#define VER_FILEFLAGS (VS_FF_DEBUG)
#else
#define VER_FILEFLAGS (0)
#endif

#ifdef WIN16
#define VER_FILEOS  VOS_DOS_WINDOWS16
#else
#ifdef _WIN32
#define VER_FILEOS  VOS__WINDOWS32
#endif
#endif
