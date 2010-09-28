/*
** Copyright (c) 2006, 2010 Ingres Corporation. All rights reserved
*/

/*
** Name: gip.h
**
** Description:
** 
**	Main header file for Linux GUI installer and Ingres package manager
**
** History:
**
**	03-Jul-2006 (hanje04)
**	    SIR 116877
**	    Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add TERMTYPE structure.
**	    Prefix misc options with GIP_OP_
**	    Make debug tracing switchable at runtime
**	    Add EXIT_STATUS macro
**	26-Oct-2006 (hanje04)
**	    Add command to remove files from under II_SYSTEM
**      21-Nov-2006 (hanje04)
**          BUG 117166
**          SD 112150
**          Set umask to 0022 in install shell to ensure directory permissions
**          are set correctly on creation by RPM.
**	29-Nov-2006 (hanje04)
**	    Add -q to iirpmrename command to surpress output from script.
**	    Define strings for labels used in upgrade/modify summary screens
**	    so the text can easily be changed when switching modes.
**	07-Dec-2006 (hanje04)
**	    Add UG_MOD_DBLOC & UG_MOD_LOGLOC to upgrade stages.
**	14-Dec-2006 (hanje04)
**	    BUG 117330
**	    Remove doc package from full install to avoid problems with
**	    (not)installing the doc package for beta. Will address problem
**	    fully for refresh.
**	22-Jan-2007 (bonro01)
**	    Set version string back to ingres2006.
**	15-Feb-2007 (hanje04)
**	    BUG 117700
**	    SD 113367
**	    Add commands to start, stop and query Ingres instance.
**	02-Feb-2007 (hanje04)
**	    SIR 116911
**	    Replace PATH_MAX with MAX_LOC.
**	27-Feb-2007 (hanje04)
**	    BUG 117330
**	    Re-enable documentation check box for non renamed installs.
**	16-Mar-2007 (hanje04)
**	    BUG 117926
**	    SD 116393
**	    Add package info for now obsolete packages is Ingres r3 so 
**	    then can be correctly removed during upgrade.
**	19-Mar-2007 (hanje04)
**	    BUG 117926
**	    SD 116393
**	    Failed to update PKGIDX index when new packages were added to 
**	    packages array.
**	18-Apr-2007 (hanje04)
**	    BUG 118087
**	    SD 115787
**	    Remove "h" from RPM_INSTALL_CMD to prevent the window "dancing" 
**	    during installation on some Linuxes. Progress now indicated
**	    with "progress bar".
**	20-Apr-2007 (hanje04)
**	    BUG 118090
**	    SD 116383
**	    Update instance structure to contain database and logfile
**	    location info so that all locations can be stored and not
**	    just II_SYSTEM.
**	31-Aug-2007 (hanje04)
**	    BUG 117330
**	    Remove PKG_DOC from FULL_INSTALL, still causing problems when
**	    Doc package isn't present.
**	23-Jul-2008 (hweho01)
**	    Changed package name from ingres2006 to ingres for 9.2 release. 
**      10-Oct-2008 (hanje04)
**          Bug 120984
**          SD 131178
**          Update CHECK_PKGNAME_VALID() to use new function GIPValidPkgName()
**	13-Jul-2009 (hanje04)
**	    SIR 122309
**	    Define new "configuration type" dialog to installer
**      15-Jan-2010 (hanje04)
**          SIR 123296
**          Config is now run post install from RC scripts. Update installation
**          commands appropriately.
**	18-May-2010 (hanje04)
**	    SIR 123791
**	    Add support for VW installs:
**	     - Make instmode values bitmask, to handle multi-mode execution
**	     - Define new dialogs to GUI (NI_LIC, UG_LIC for license 
**	       acceptance; NI_VWCFG for Vectorwise config)
**	     - Define new Vectorwise config parameters
**	     - Define vw_cfg structure for hold parameter info.
**	    Remove PKG_BASENAME, no longer used.
**	26-May-2010 (hanje04)
**	    SIR 123791
**	    Define TYPICAL_VECTORWISE set of packages to be installed.
**	14-Jul-2010 (hanje04)
**	    BUG 124081
**	    Added defns. for location to save the response file
**	22-Sep-2010 (hanje04)
**	    BUG 124480
**	    SD 126680
**	    Add PKG_SUP32 to FULL_INSTALL for 64bit platforms.
** 
*/

/* Response File API header */
# include <rfapi.h>

/* Default Install Information */
#define DEFAULT_CHAR_SET 17 /* iso88591 */
#define RESPONSE_FILE_NAME "iirfinstall"
#define RESPONSE_FILE_SAVE_NAME "install.rsp"
#define RESPONSE_FILE_SAVE_DIR "/var/lib/ingres/%s"
#define STDOUT_LOGFILE_NAME "/tmp/iiinstall_stdout"
#define STDERR_LOGFILE_NAME "/tmp/iiinstall_stderr"
#define MAX_FNAME 256
#define MAX_LOGFILE_NAME 25
#define RPM_INSTALL_CMD "rpm -iv "
#define RPM_FORCE_INSTALL "--replacefiles --replacepkgs "
#define RPM_UPGRMV_CMD "rpm -e --justdb "
#define RPM_REMOVE_CMD "rpm -e "
#define RPM_RENAME_CMD "iirpmrename -q "
#define RPM_RENAME_DIR "iiinstall_rpmrename"
#define RPM_PREFIX "--prefix"
#define RM_CMD "rm -rf"
#define UMASK_CMD "umask 0022"
#define INGSTART_CMD "/etc/init.d/ingres%s start"
#define INGSTOP_CMD "/etc/init.d/ingres%s stop > /dev/null 2>&1"
#define INGSTATUS_CMD "/etc/init.d/ingres%s status > /dev/null 2>&1"
#define INGCONFIG_CMD "/etc/init.d/ingres%s configure %s"
#define OUTPUT_READSIZE 5

enum {
	BASIC_INSTALL=0x01,
	ADVANCED_INSTALL=0x02,
	IVW_INSTALL=0x4,
	RFGEN_LNX=0x08,
	RFGEN_WIN=0x10,
};

/* screen names */
#define START_SCREEN 0
#define LIC_SCREEN (START_SCREEN + 1)
enum {
	RF_START,
	RF_PLATFORM,
	NI_START,
	/* install frames */
	NI_LIC,
	NI_CFG,
	NI_EXP,
	NI_INSTID,
	NI_PKGSEL,	
	NI_DBLOC,
	NI_VWCFG,
	NI_LOGLOC,
	NI_LOCALE,
	NI_MISC,
	RF_WINMISC,
	NI_SUMMARY,
	NI_INSTALL,
	NI_DONE,
	NI_FAIL,
	RF_SUMMARY,
	RF_DONE,
	NUM_NI_FRAMES
};

enum {
	/* upgrade frames */
	UG_START,
	UG_LIC,
	UG_SSAME,
	UG_SOLD,
	UG_MULTI,
	UG_SELECT,
	UG_MOD,
	UG_MOD_DBLOC,
	UG_MOD_LOGLOC,
	UG_MOD_MISC,
	UG_SUMMARY,
	UG_DO_INST,
	UG_DONE,
	UG_FAIL,
	NUM_UG_FRAMES
};
/* installer options  */
typedef enum {
	UM_FALSE = 0x00,
	UM_TRUE = 0x01,
	UM_INST = 0x02,
	UM_MOD = 0x04,
	UM_RMV = 0x08,
	UM_UPG = 0x10,
	UM_MULTI = 0x20,
	UM_RENAME = 0x40,
} UGMODE;

enum {
	/* master frames */
	MA_UPGRADE,
	MA_INSTALL,
	NUM_MA_FRAMES
};

enum {
	/* Tx log locs */
	TX_LOG,
	DUAL_LOG,
	NUM_LOG_LOCS
};

typedef struct _stage_info {
	int stage;
	char stage_name[20];
	char status;
	/* stage status */
#define	ST_INIT 0x1
#define	ST_VISITED 0x2
#define	ST_COMPLETE 0x4
#define	ST_FAILED 0x8
	} stage_info;
				
typedef struct _location {
	char name[30]; 
	char symbol[15];
	char dirname[10];
	II_RFAPI_PNAME rfapi_name;
	char path[MAX_LOC] ;
	} location ;
	
enum {
    INST_II_SYSTEM,
    INST_II_DATABASE,
    INST_II_CHECKPOINT,
    INST_II_JOURNAL,
    INST_II_WORK,
    INST_II_DUMP,
    INST_II_VWDATA,
    NUM_LOCS
} location_index;

typedef struct _log_info {
	location log_loc;
	location dual_loc;
	size_t size;
	bool duallog;
	} log_info;
				
typedef struct _TIMEZONE {
    char *region;
    char *timezone;
  } TIMEZONE ;

typedef struct _CHARSET {
    char *region;
    char *charset;
    char *description;
  } CHARSET ;

typedef struct _TERMTYPE {
    char *class;
    char *terminal;
    } TERMTYPE ;

typedef struct _locale_info {
	char *timezone ;
	char *charset ;
	} ing_locale ;

typedef enum {
	GIP_VWCFG_MAX_MEMORY = 0x01,
	GIP_VWCFG_BUFFERPOOL = 0x02,
	GIP_VWCFG_COLUMNSPACE = 0x04,
	GIP_VWCFG_BLOCK_SIZE = 0x08,
	GIP_VWCFG_GROUP_SIZE = 0x10,
	} VWCFG ;

typedef enum {
	VWKB,
	VWMB,
	VWGB,
	VWTB,
	VWNOUNIT,
	} VWUNIT ;

typedef struct _vw_cfg {
	VWCFG bit;
	II_RFAPI_PNAME rfapi_name;
	const char *descrip;
	const char *field_prefix;
	SIZE_TYPE value;
	SIZE_TYPE dfval;
	VWUNIT unit;
	VWUNIT dfunit;
} vw_cfg ;

typedef enum {
	GIP_OP_SQL92 = 0x001,
	GIP_OP_INST_CUST_USR = 0x002,
	GIP_OP_INST_CUST_GRP = 0x004,
	GIP_OP_START_ON_BOOT = 0x008,
	GIP_OP_INSTALL_DEMO = 0x010,
	GIP_OP_WIN_INSTALL_DEMO = 0x020,
	GIP_OP_DESKTOP_FOLDER = 0x040,
	GIP_OP_WIN_START_AS_SERVICE = 0x080,
	GIP_OP_WIN_START_CUST_USR = 0x100,
	GIP_OP_WIN_ADD_TO_PATH = 0x200,
	GIP_OP_REMOVE_ALL_FILES = 0x400,
	GIP_OP_UPGRADE_USER_DB = 0x800,
	} MISCOPS ;

typedef struct _misc_op_info {
	MISCOPS bit;
	II_RFAPI_PNAME rfapi_name;
} misc_op_info ;
    
#define DBG_ON
/* Misc MACROs */
#define OK 0
#define FAIL 1
#define BITS_PER_BYTE 8
#define DBG_PRINT	if ( debug_trace ) printf
#define UG_MOD_UPG_TXT "The following instances have been detected, please select the instance\nyou wish to %s"
#define UG_SUM_LABEL1_TXT "<b>Ready to %s</b>"
#define UG_SUM_LABEL2_TXT "Click <b>%s</b> to begin the installation"
#define UG_SUM_LABEL3_TXT "You are now ready to %s your Ingres instance."
#define UG_SUM_LABEL4_TXT "<b>Please wait while the Ingres instance is %s</b>"
#define UG_SUM_LABEL5_TXT "Ingres has been successfully %s"
#define UG_SUM_LABEL6_TXT "<b>%s Complete</b>"
#define UG_SUM_LABEL7_TXT "<b>%s Failed</b>"
#define CHECK_PKGNAME_VALID( s, f ) GIPValidatePkgName( s, f )
#define EXIT_STATUS stage_names[current_stage]->status == ST_FAILED ? \
								FAIL : OK

	
/* package info */
typedef u_i4 PKGLST;
#define PKG_NONE	0x0 /* used to clear the list  */
#define PKG_CORE	0x0000001
#define PKG_DBMS	0x0000002
#define PKG_NET		0x0000004
#define PKG_ODBC	0x0000008
#define PKG_REP		0x0000010
#define PKG_ABF		0x0000020
#define PKG_STAR	0x0000040
#define PKG_JDBC	0x0000080
#define PKG_ICE		0x0000100
#define PKG_SUP32	0x0000200
#define PKG_BRIDGE	0x0000400
#define PKG_C2		0x0000800
#define PKG_DAS		0x0001000
#define PKG_ESQL	0x0002000
#define PKG_I18N	0x0004000
#define PKG_OME		0x0008000
#define PKG_QRRUN	0x0010000
#define PKG_TUX		0x0020000
#define PKG_VIS		0x0040000
#define PKG_LGPL	0x0080000
#define PKG_LCOM	0x0100000
#define PKG_LEVAL	0x0200000
#define PKG_DOC		0x1000000 /* always the last package */
#define TYPICAL_SERVER (PKG_CORE|PKG_DBMS|PKG_NET|PKG_ODBC|PKG_ABF)
#define TYPICAL_VECTORWISE (PKG_CORE|PKG_DBMS|PKG_NET|PKG_ODBC)
#define TYPICAL_CLIENT (PKG_CORE|PKG_NET|PKG_ODBC)
#ifdef LP64
#define FULL_INSTALL (PKG_CORE|PKG_DBMS|PKG_NET|PKG_ODBC|PKG_REP|PKG_ABF| \
					 PKG_STAR|PKG_SUP32)	
#else
#define FULL_INSTALL (PKG_CORE|PKG_DBMS|PKG_NET|PKG_ODBC|PKG_REP|PKG_ABF| \
					 PKG_STAR)	
#endif


/*
** Obsolete packages are NOT defined to RFAPI
** so represent them with GIP_OBS_PKG
*/
#define GIP_OBS_PKG (NUM_PKGS + 1)

typedef struct _package {
		PKGLST	bit;
		PKGLST	prereqs;
		char display_name[50];		
# define MAX_PKG_NAME_LEN 15
		char pkg_name[MAX_PKG_NAME_LEN];
		II_RFAPI_PNAME rfapi_name;
		bool hidden;
	} package;
	
/* packages array index */
typedef enum _PKGIDX {
	CORE,
	SUP32,
	ABF,
	BRIDGE,
	C2AUD,
	DAS,
	DBMS,
	ESQL,
	I18N,
	ICE,
	JDBC,
	NET,
	ODBC,
	OME,
	QRRUN,
	REP,
	STAR,
	TUX,
	VIS,
	DOC,
	NUM_PKGS
} PKGIDX ;

typedef struct _auxloc auxloc;
struct _auxloc {
		i4 idx;
		char loc[MAX_LOC];
		auxloc *next_loc;
		}  ;

typedef struct _instance {
# define MAX_REL_LEN 15
		char	pkg_basename[MAX_REL_LEN];
# define INST_NAME_LEN 15
		char	instance_name[INST_NAME_LEN];
		char	*instance_ID;
# define MAX_VERS_LEN 25
		char	version[MAX_VERS_LEN];
		char	inst_loc[MAX_LOC];
		auxloc 	*datalocs;
		i4	numdlocs ;
		auxloc	*loglocs;
		i4	numllocs ;
		PKGLST	installed_pkgs;
		UGMODE	action; /* action to be taken by installer */
		u_i2	memtag;
	} instance ;
	
typedef struct _saveset {
		char	product[MAX_REL_LEN];
		char	pkg_basename[MAX_REL_LEN];
		char	version[MAX_VERS_LEN];
# define MAX_ARCH_LEN 8
		char	arch[MAX_ARCH_LEN];
# define MAX_FORMAT_LEN 4
		char	format[MAX_FORMAT_LEN];
		char	file_loc[MAX_LOC];
		PKGLST	pkgs;
	} saveset ;
# define MAX_PKG_FILE_LEN 50	
	
typedef struct {
	STATUS exit_status;
	bool alive;
	} _childinfo;
enum
{
	COL_NAME,
	COL_VERSION,
	COL_LOCATION,
	NUM_COLS
};
