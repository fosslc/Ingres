/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : constdef.h: define constants
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Constant symbolic name
**
** History:
**
** 22-Oct-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 02-May-2002 (noifr01)
**    (sir 106648) cleaunup for the VDBA split project. removed the
**    NODE_LOCAL_BOTHINGRESxDT definition (ingres/desktop no more managed
**    specifically)
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 27-Mar-2003 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Enhance the library.
** 04-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 22-Apr-2003 (schph01)
**    SIR 107523 Add define for sequence Object
** 10-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 12-May-2004 (schph01)
**    SIR 111507 Add define for new column type bigint(int8)
** 15-Oct-2004 (penga03)
**    Redefined consttchszIngVdba to be TEXT ("\\ingres\\bin\\").
** 12-Oct-2006 (wridu01)
**    (Sir 116835) Add support for Ansi Date/Time data types.
** 24-Aug-2007 (drivi01)
**    Add consttchszUSERPROFILE constant and consttchszIngConf
**    path.
** 30-Apr-2008 (drivi01)
**    Update consttchszUSERPROFILE constant to point to ALLUSERSPROFILE.
**    The configuration files will be stored under
**    %ALLUSERSPROFILE%\Ingres\Ingres<II_INSTALLATION>\files on Vista only.
** 08-Sep-2009 (drivi01)
**    Add consttchszDBATools and consttchszIngresProd constants.
** 25-May-2010 (drivi01) Bug 123817
**    Fix the object length to allow for long IDs.
**    Remove hard coded constants, use DB_MAXNAME instead.
**/

#if !defined (CONSTANT_DEFINED_HEADER)
#define CONSTANT_DEFINED_HEADER

#define GUID_SIZE         128
#define MAX_STRING_LENGTH 256

#define DBOBJECTLEN    (DB_MAXNAME*2 + 3) 
#define MAXDATELEN     32 
#define MAXSQLERROR (1024 +1)

//
// NODE CLASSIFICATION:
#define NODE_SIMPLIFY             0x0001
#define NODE_LOCAL                0x0002
#define NODE_INSTALLATION         0x0004
//#define NODE_LOCAL_BOTHINGRESxDT  0x0008
#define NODE_WRONG_LOCALNAME      0x0010
#define NODE_TOOMUCHINSTALLATION  0x0020


//
// OBJECT_TYPE ID:
// ---------------
enum
{
	OBT_VIRTNODE = 0,
	OBT_DATABASE,
	OBT_PROFILE,
	OBT_USER,
	OBT_GROUP,
	OBT_GROUPUSER,
	OBT_ROLE,
	OBT_LOCATION,
	OBT_DBAREA,        // OI Desktop
	OBT_STOGROUP,      // OI Desktop
	OBT_TABLE,
	OBT_TABLELOCATION,
	OBT_VIEW,
	OBT_VIEWTABLE,
	OBT_INDEX,
	OBT_INTEGRITY,
	OBT_PROCEDURE,
	OBT_SEQUENCE,
	OBT_RULE,          // TRIGGERS (OIDT) to be mapped to rules
	OBT_RULEWITHPROC,
	OBT_RULEPROC,
	OBT_SCHEMAUSER,
	OBT_SYNONYM,
	OBT_SYNONYMOBJECT, // if type undefined (table/view/index) 
	OBT_DBEVENT,
	OBT_ALARM,
	OBT_TABLECOLUMN,
	OBT_VIEWCOLUMN,

	OBT_INSTALLATION,
	OBT_VNODE_OPENWIN,
	OBT_VNODE_LOGINPSW,
	OBT_VNODE_CONNECTIONDATA,
	OBT_VNODE_SERVERCLASS,
	OBT_VNODE_ATTRIBUTE,

	OBT_GRANTEE
};

#define OBT_VNODE  OBT_VIRTNODE

//
// Table Logicaly, Physically consistence and level recovery flags
#define TBL_FLAG_PHYS_INCONSISTENT         0x00000200L
#define TBL_FLAG_LOG_INCONSISTENT          0x00000400L
#define TBL_FLAG_RECOVERY_DISALLOWED       0x00000800L


//
// STAR OBJECT:
#define OBJTYPE_NOTSTAR    0
#define OBJTYPE_STARNATIVE 1
#define OBJTYPE_STARLINK   2
#define OBJTYPE_UNKNOWN    3

#define DBFLAG_NORMAL      0x00000000
#define DBFLAG_GLOBAL      0x00000001
#define DBFLAG_READONLY    0x00000811

#define DBFLAG_STARNATIVE  0x00000001 // Distribute
#define DBFLAG_STARLINK    0x00000002 // Coordinator


// Virtual Node's sub-folders:
#define FOLDER_SERVER             0x00000001
#define FOLDER_ADVANCE            0x00000002
#define FOLDER_DATABASE           0x00000004
#define FOLDER_PROFILE            0x00000008
#define FOLDER_USER               0x00000010
#define FOLDER_GROUP              0x00000020
#define FOLDER_ROLE               0x00000040
#define FOLDER_LOCATION           0x00000080

// Advance Node's sub-folders:
#define FOLDER_LOGIN              0x00000001
#define FOLDER_CONNECTION         0x00000002
#define FOLDER_ATTRIBUTE          0x00000004

// Database's sub-folders:
#define FOLDER_TABLE              0x00000001
#define FOLDER_VIEW               0x00000002
#define FOLDER_PROCEDURE          0x00000004
#define FOLDER_DBEVENT            0x00000008
#define FOLDER_SYNONYM            0x00000010
#define FOLDER_GRANTEEDATABASE    0x00000020
#define FOLDER_ALARMDATABASE      0x00000040
#define FOLDER_SEQUENCE           0x00000080

// Table's sub-folders:
#define FOLDER_COLUMN             0x00000001
#define FOLDER_INDEX              0x00000002
#define FOLDER_RULE               0x00000004
#define FOLDER_INTEGRITY          0x00000008
#define FOLDER_GRANTEETABLE       0x00000010
#define FOLDER_ALARMTABLE         0x00000020

// View's sub-folders:
//#define FOLDER_COLUMN             0x00000001
#define FOLDER_GRANTEEVIEW        0x00000002

// Procedure's sub-folders:
#define FOLDER_GRANTEEPROCEDURE   0x00000001

// DBEvent's sub-folders:
#define FOLDER_GRANTEEDBEVENT     0x00000001

// Sequence's sub-folders:
#define FOLDER_GRANTEESEQUENCE    0x00000001


#define INGTYPE_ERROR           0
#define INGTYPE_C               1
#define INGTYPE_CHAR            2
#define INGTYPE_TEXT            3
#define INGTYPE_VARCHAR         4
#define INGTYPE_LONGVARCHAR     5
#define INGTYPE_INT4            6
#define INGTYPE_INT2            7
#define INGTYPE_INT1            8
#define INGTYPE_DECIMAL         9 
#define INGTYPE_FLOAT8         10
#define INGTYPE_FLOAT4         11
#define INGTYPE_DATE           12
#define INGTYPE_MONEY          13
#define INGTYPE_BYTE           14
#define INGTYPE_BYTEVAR        15
#define INGTYPE_LONGBYTE       16
#define INGTYPE_OBJKEY         17
#define INGTYPE_TABLEKEY       18
#define INGTYPE_SECURITYLBL    19
#define INGTYPE_SHORTSECLBL    20
#define INGTYPE_FLOAT          21
#define INGTYPE_INTEGER        22     // equivalent to INGTYPE_INT4
#define INGTYPE_UNICODE_NCHR   23     // unicode nchar
#define INGTYPE_UNICODE_NVCHR  24     // unicode nvarchar
#define INGTYPE_UNICODE_LNVCHR 25     // unicode long nvarchar
#define INGTYPE_INT8           26
#define INGTYPE_BIGINT         27
#define INGTYPE_ADTE           28     // ansidate
#define INGTYPE_TMWO           29     // time without time zone
#define INGTYPE_TMW            30     // time with time zone
#define INGTYPE_TME            31     // time with local time zone
#define INGTYPE_TSWO           32     // timestamp without time zone
#define INGTYPE_TSW            33     // timestamp with time zone
#define INGTYPE_TSTMP          34     // timestamp with local time zone
#define INGTYPE_INYM           35     // interval year to month
#define INGTYPE_INDS           36     // interval day to second
#define INGTYPE_IDTE           37     // ingresdate

#if defined (EDBC)
#define consttchszII_SYSTEM TEXT ("EDBC_ROOT")
#define consttchszUSERPROFILE	TEXT ("ALLUSERSPROFILE")
#define consttchszIngres    TEXT ("edbc")
#define consttchszIngDTD    TEXT ("ingres.dtd")
#else
#define consttchszII_SYSTEM TEXT ("II_SYSTEM")
#define consttchszUSERPROFILE	TEXT ("ALLUSERSPROFILE")
#define consttchszIngres    TEXT ("ingres")
#define consttchszIngDTD    TEXT ("ingres.dtd")
#define consttchszDBATools	TEXT("DBA Tools")
#define consttchszIngresProd	TEXT("Ingres")
#endif


#if defined (MAINWIN)
#define consttchszReturn   TEXT ("\n\0")
#define consttchszPathSep  TEXT ("/")

#if defined (EDBC)
#define consttchszIngFiles TEXT ("/edbc/files/")
#define consttchszIngBin   TEXT ("/edbc/bin/")
#define consttchszIngVdba  TEXT ("/edbc/vdba/")
#define consttchszWinstart TEXT ("edbcwstart /start");
#else  // NOT EDBC 
#define consttchszIngFiles TEXT ("/ingres/files/")
#define consttchszIngBin   TEXT ("/ingres/bin/")
#define consttchszIngVdba  TEXT ("/ingres/vdba/")
#define consttchszIngConf  TEXT ("/ingres/files/")
#define consttchszWinstart TEXT ("winstart /start");
#endif // #if defined (EDBC)

#else  // NOT MAINWIN
#define consttchszReturn   TEXT ("\r\n\0")
#define consttchszPathSep  TEXT ("\\")

#if defined (EDBC)
#define consttchszIngFiles TEXT ("\\edbc\\files\\")
#define consttchszIngBin   TEXT ("\\edbc\\bin\\")
#define consttchszIngVdba  TEXT ("\\edbc\\vdba\\")
#define consttchszWinstart TEXT ("edbcwstart.exe /start");
#else  // NOT EDBC
#define consttchszIngFiles TEXT ("\\ingres\\files\\")
#define consttchszIngBin   TEXT ("\\ingres\\bin\\")
#define consttchszIngVdba  TEXT ("\\ingres\\bin\\")
#define consttchszIngConf  TEXT ("\\Ingres\\")
#define consttchszIngConfFiles	TEXT ("\\files\\")
#define consttchszWinstart TEXT ("winstart.exe /start");
#endif // #if defined (EDBC)

#endif // #if defined (MAINWIN)


#endif // CONSTANT_DEFINED_HEADER
