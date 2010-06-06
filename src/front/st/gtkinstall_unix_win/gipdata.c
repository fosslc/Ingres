/*
** Copyright 2006 Ingres Corporation. All rights reserved 
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

# include <compat.h>
# include <gl.h>

# ifdef xCL_GTK_EXISTS

# include <gtk/gtk.h>

# include <gip.h>

/*
** Name: gipdata.c
**
** Description:
**
**      Global and static data module for GTK GUI interface to both
**      the Linux GUI installer and the Ingres Package manager.
**
** History:
**
**	09-Oct-2006 (hanje04)
**	    SIR 116877
**          Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Prefix install options with GIP_OP to make them more obviously part
**	    of this section of code.
**	    Make sure "welcome" states are initialized properly.
**	    Add terminal info. 
**	06-Nov-2006 (hanje04)
**	    SIR 116877
**	    Make sure code only gets built on platforms that can build it.
**	30-Nov-2006 (hanje04)
**	    Add date_type_alias structure for storing choice from GUI.
**	    Also make existing_instances a pointer array instead of a pointer.
**	11-Dec-2006 (hanje04)
**	    SIR 116148
**	    Set date_type_alias structure to default to Ingres date not ANSI.
**	11-Dec-2006 (hanje04)
**	    BUG 117278
**	    SD 112944
**	    Add adv_nodbms_stages[] for case where DBMS package is NOT selected
**	    for installation. 
**	    Add UG_MOD_DBLOC, UG_MOD_LOGLOC and UG_MOD_MISC to upgrade_stages
**	    for DBMS config panes when adding DBMS package to existing instance.
**      02-Feb-2007 (hanje04)
**          SIR 116911
**	    Make sure starting platform is correctly set on Windows (packman).
**	    Replace PATH_MAX with MAX_LOC.
**	22-Feb-2007 (hanje04)
**	    SIR 116911
**	    Add run_platform to define the OS we're running on so we can tell
**	    if it differs from the deployment platform.
**	23-Feb-2007 (hanje04)
**	    SIR 117784
**	    On Linux we only have the option of creating a demodb. No demos
**	    are available to install at the moment.    
**	28-Mar-2007 (hanje04)
**	    Bug 117672
**	    Ingres Star is now called Ingres Star again as opposed to
**	    Federated Database Support.
**	16-Mar-2007 (hanje04)
**	    BUG 117926
**	    SD 116393
**	    Add package info for now obsolete packages is Ingres r3 so 
**	    then can be correctly removed during upgrade.
**	21-Mar-2007 (hanje04)
**	    BUG 117965
**	    SD 116490
**	    Add misc option info for custom user group.
**	25-Apr-2007 (hanje04)
**	    SIR 118202
**	    Set default Tx log size using RFAPI default.
**	23-Apr-2007 (hanje04)
**	    BUG 118090
**	    SD 116383
**	    Add "sub-directory" names for all database and log locations so
**	    they can be removed if requested.
**	21-May-2007 (hanje04)
**	    SIR 118211
**	    Add UTF8 as valid saveset. Re-order Windows character set while
**	    I'm at it too.
**	15-Aug-2007 (hanje04)
**	    SIR 118959
**	    SD 120837
**	    Use newly defined RFAPI_TXLOG_SIZE_MB_INT to initialize 
**	    txlog_info.size as previous value was a string.
**	08-Jul-2009 (hanje04)
**	    SIR 122309
**	    Add ing_config_type structure for storing configuration type
**	    selected by radio buttons.
**	31-Jul-2009 (hanje04)
**	    BUG 122359
**	    Change displayed name if "Networking" package to "Client"
**	10-Aug-2009 (hanje04)
**	    BUG 122571
**	    Remove duplicate defn. of new_pkgs_info.
**	17-May-2010 (hanje04)
**	    SIR 123791
**	    Add new license dialog to "stages" structures (NI_LIC for new
**	    install, UG_LIC for upgrade).
**	    Add new vectorwise screen to "stages" structure, NI_VWCFG and
**	    define basic_ivw_stages() and adv_ivw_stages() arrays to control
**	    execution for new IVW_INSTALL mode.
**	    Add iivwdata as new location for IVWM_INSTALL mode.
**	    Add new vw_cfg structures to define and store info relating to
**	    vectorwise configuration parameters.
**	    Add vw_cfg_info array to store new param info.
*/


/* Installation globals */
/* Instance location and ID */
GLOBALDEF char LnxDefaultII_SYSTEM[MAX_LOC];
GLOBALDEF char WinDefaultII_SYSTEM[MAX_LOC];
GLOBALDEF char *default_ii_system;
GLOBALDEF location iisystem = { 
		"Installation location",
		"II_SYSTEM",
		"ingres",
		II_RF_SYSTEM,
		RFAPI_LNX_DEFAULT_INST_LOC
		};
GLOBALDEF char dfltID[3] = RFAPI_DEFAULT_INST_ID ;
GLOBALDEF char instID[3];

/* DB locations */
static location iidatabase = {
		"Database",
		"II_DATABASE",
		"data",
		II_RF_DATABASE,
		RFAPI_LNX_DEFAULT_INST_LOC
		};
static location iicheckpoint = {
		"Backup",
		"II_CHECKPOINT",
		"ckp",
		II_RF_CHECKPOINT,
		RFAPI_LNX_DEFAULT_INST_LOC
		};
static location iijournal = {
		"Journal",
		"II_JOURNAL",
		"jnl",
		II_RF_JOURNAL,
		RFAPI_LNX_DEFAULT_INST_LOC
		};
static location iiwork = {
		"Temporary",
		"II_WORK",
		"work",
		II_RF_WORK,
		RFAPI_LNX_DEFAULT_INST_LOC
		};
static location iidump = {
		"Dump",
		"II_DUMP",
		"dmp",
		II_RF_DUMP,
		RFAPI_LNX_DEFAULT_INST_LOC
		};
static location iivwdata = {
		"VectorWise",
		"II_VWDATA",
		"vectorwise",
		II_RF_VWDATA,
		RFAPI_LNX_DEFAULT_INST_LOC
		};
GLOBALDEF location *dblocations[] = {
	&iisystem,
	&iidatabase,
	&iicheckpoint,
	&iijournal,
	&iiwork,
	&iidump,
	&iivwdata,
	NULL
};

/* Transaction log  */
GLOBALDEF log_info txlog_info = {
	{
		"Trasaction log location",
		"II_LOG_FILE",
		"log",
		II_RF_LOG_FILE,
		RFAPI_LNX_DEFAULT_INST_LOC
	},
	{
		"Dual log location",
		"II_DUAL_LOG",
		"log",
		II_RF_DUAL_LOG,
		RFAPI_LNX_DEFAULT_INST_LOC
	},
	(SIZE_TYPE)RFAPI_DEFAULT_TXLOG_SIZE_MB_INT,
	FALSE
} ;

/*
**  Vectorwise config parameters
**
**  vw_cfg structure defined in gip.h
**
**   typedef struct _vw_cfg {
**         VWCFG bit;
**         II_RFAPI_PNAME rfapi_name;
**         const char *descrip;
**         const char *field_prefix;
**         SIZE_TYPE value;
**         SIZE_TYPE dfval;
**         VWUNIT unit;
**         VWUNIT dfunit;
**   } vw_cfg ;
**  
*/
/* lookup for VWUNIT */
GLOBALDEF char vw_cfg_units[] = {'K', 'M', 'G', 'T', '\0'};

static vw_cfg ivw_max_memory = {
	GIP_VWCFG_MAX_MEMORY,
	II_VWCFG_MAX_MEMORY,
	"Processing memory",
	"ivw_cfg_procmem",
	RFAPI_DEFAULT_VWCFG_MAX_MEMORY_GB_INT,
	RFAPI_DEFAULT_VWCFG_MAX_MEMORY_GB_INT,
	VWGB,
	VWGB,
};

static vw_cfg ivw_bufferpool = {
	GIP_VWCFG_BUFFERPOOL,
	II_VWCFG_BUFFERPOOL,
	"Buffer pool memory",
	"ivw_cfg_buffpoolmem",
	RFAPI_DEFAULT_VWCFG_BUFFERPOOL_GB_INT,
	RFAPI_DEFAULT_VWCFG_BUFFERPOOL_GB_INT,
	VWGB,
	VWGB,
};

static vw_cfg ivw_columnspace = {
	GIP_VWCFG_COLUMNSPACE,
	II_VWCFG_COLUMNSPACE,
	"Maximum database size",
	"ivw_cfg_maxdata",
	RFAPI_DEFAULT_VWCFG_COLUMNSPACE_GB_INT,
	RFAPI_DEFAULT_VWCFG_COLUMNSPACE_GB_INT,
	VWGB,
	VWGB,
};

static vw_cfg ivw_block_size = {
	GIP_VWCFG_BLOCK_SIZE,
	II_VWCFG_BLOCK_SIZE,
	"Block size",
	"ivw_cfg_blksz",
	RFAPI_DEFAULT_VWCFG_BLOCK_SIZE_MB_INT,
	RFAPI_DEFAULT_VWCFG_BLOCK_SIZE_MB_INT,
	VWMB,
	VWMB,
};

static vw_cfg ivw_group_size = {
	GIP_VWCFG_GROUP_SIZE,
	II_VWCFG_GROUP_SIZE,
	"Block group size",
	"ivw_cfg_blkgrpsz",
	RFAPI_DEFAULT_VWCFG_GROUP_SIZE_INT,
	RFAPI_DEFAULT_VWCFG_GROUP_SIZE_INT,
	VWNOUNIT, /* no unit for group size */
	VWNOUNIT, /* no unit for group size */
};

GLOBALDEF vw_cfg *vw_cfg_info[] = {
	&ivw_max_memory,
	&ivw_bufferpool,
	&ivw_columnspace,
	&ivw_block_size,
	&ivw_group_size,
	NULL
};


/* Misc options  */
static misc_op_info sql92 = {
	GIP_OP_SQL92,
	II_ENABLE_SQL92,
};

static misc_op_info inst_cust_usr = {
	GIP_OP_INST_CUST_USR,
	II_USERID,
};
    
static misc_op_info inst_cust_grp = {
	GIP_OP_INST_CUST_GRP,
	II_GROUPID,
};
    
static misc_op_info start_on_boot = {
	GIP_OP_START_ON_BOOT,
	II_START_ON_BOOT,
};
    
static misc_op_info install_demo = {
	GIP_OP_INSTALL_DEMO,
	II_DEMODB_CREATE,
};
    
static misc_op_info win_install_demo = {
	GIP_OP_WIN_INSTALL_DEMO,
	II_INSTALL_DEMO,
};
    
static misc_op_info desktop_folder = {
	GIP_OP_DESKTOP_FOLDER,
	II_CREATE_FOLDER,
};
    
static misc_op_info win_start_as_service = {
	GIP_OP_WIN_START_AS_SERVICE,
	II_SERVICE_START_AUTO,
};
    
static misc_op_info win_start_cust_usr = {
	GIP_OP_WIN_START_CUST_USR,
	II_SERVICE_START_USER,
};
    
static misc_op_info win_add_to_path = {
	GIP_OP_WIN_ADD_TO_PATH,
	II_ADD_TO_PATH,
};

static misc_op_info remove_all_files = {
	GIP_OP_REMOVE_ALL_FILES,
	II_REMOVE_ALL_FILES,
};

static misc_op_info upgrade_user_db = {
	GIP_OP_UPGRADE_USER_DB,
	II_UPGRADE_USER_DB,
};

GLOBALDEF misc_op_info *misc_ops_info[] = {
    &sql92,
    &inst_cust_usr,
    &inst_cust_grp,
    &start_on_boot,
    &install_demo,
    &win_install_demo,
    &desktop_folder,
    &win_start_as_service,
    &win_start_cust_usr,
    &win_add_to_path,
    &remove_all_files,
    &upgrade_user_db,
    NULL
} ;
GLOBALDEF MISCOPS misc_ops = GIP_OP_START_ON_BOOT|\
				GIP_OP_INSTALL_DEMO|\
				GIP_OP_DESKTOP_FOLDER|\
				GIP_OP_UPGRADE_USER_DB;
GLOBALDEF MISCOPS WinMiscOps = GIP_OP_WIN_START_AS_SERVICE|\
				GIP_OP_INSTALL_DEMO|\
				GIP_OP_WIN_ADD_TO_PATH;

/* User IDs */
GLOBALDEF const char *defaultuser = "ingres" ;
GLOBALDEF char *userid;
GLOBALDEF char *groupid;

GLOBALDEF const char *WinServiceID = NULL ;
GLOBALDEF const char *WinServicePwd = NULL ;

/* locale settings */
GLOBALDEF ing_locale locale_info = {
		RFAPI_LNX_DEFAULT_TIMEZONE,
		RFAPI_LNX_DEFAULT_CHARSET,
		} ;

/* 
** generated in an Ingres installation with the following command
** for z in `iivalres -v ii."*".setup.region \
**		${II_SYSTEM}/ingres/files/net.rfm | \
**		grep [[:upper:]][[:upper:]]` ; 
**		do printf "\t{ \"$z\", NULL },\n" ; 
**			iivalres -v ii."*".setup.$z.tz BOGUS_TIMEZONE \
**			${II_SYSTEM}/ingres/files/net.rfm | \
**			grep [[:upper:]][[:upper:]] | \
**			awk '{
**				printf"\t{ NULL, \"%s\" },\n", $1} 
**				$2 != "" {printf"\t{ NULL, \"%s\" },\n", $2
**			}'
**		done
*/
GLOBALDEF TIMEZONE timezones[] = {
    { "AFRICA", NULL },
    { NULL, "GMT" },
    { NULL, "GMT1" },
    { NULL, "GMT2" },
    { NULL, "GMT3" },
    { NULL, "GMT4" },
    { "ASIA", NULL },
    { NULL, "MOSCOW9" },
    { NULL, "MOSCOW8" },
    { NULL, "MOSCOW7" },
    { NULL, "MOSCOW6" },
    { NULL, "MOSCOW5" },
    { NULL, "MOSCOW4" },
    { NULL, "MOSCOW3" },
    { NULL, "MOSCOW2" },
    { NULL, "ROC" },
    { NULL, "JAPAN" },
    { NULL, "KOREA" },
    { NULL, "INDIA" },
    { NULL, "HONG-KONG" },
    { NULL, "PAKISTAN" },
    { NULL, "PRC" },
    { NULL, "GMT5" },
    { NULL, "GMT6" },
    { NULL, "GMT7" },
    { NULL, "GMT8" },
    { NULL, "GMT9" },
    { NULL, "GMT10" },
    { NULL, "GMT11" },
    { "AUSTRALIA", NULL },
    { NULL, "AUSTRALIA-LHI" },
    { NULL, "AUSTRALIA-YANCO" },
    { NULL, "AUSTRALIA-NORTH" },
    { NULL, "AUSTRALIA-WEST" },
    { NULL, "AUSTRALIA-SOUTH" },
    { NULL, "AUSTRALIA-TASMANIA" },
    { NULL, "AUSTRALIA-QUEENSLAND" },
    { NULL, "AUSTRALIA-VICTORIA" },
    { NULL, "AUSTRALIA-NSW" },
    { "MIDDLE-EAST", NULL },
    { NULL, "EGYPT" },
    { NULL, "IRAN" },
    { NULL, "ISRAEL" },
    { NULL, "KUWAIT" },
    { NULL, "SAUDI-ARABIA" },
    { NULL, "GMT2" },
    { NULL, "GMT3" },
    { NULL, "GMT4" },
    { "NORTH-AMERICA", NULL },
    { NULL, "NA-PACIFIC" },
    { NULL, "NA-MOUNTAIN" },
    { NULL, "NA-CENTRAL" },
    { NULL, "NA-EASTERN" },
    { NULL, "CANADA-ATLANTIC" },
    { NULL, "CANADA-NEWFOUNDLAND" },
    { NULL, "CANADA-YUKON" },
    { NULL, "MEXICO-GENERAL" },
    { NULL, "MEXICO-BAJANORTE" },
    { NULL, "MEXICO-BAJASUR" },
    { NULL, "US-ALASKA" },
    { "NORTH-ATLANTIC", NULL },
    { NULL, "EUROPE-WESTERN" },
    { NULL, "EUROPE-CENTRAL" },
    { NULL, "EUROPE-EASTERN" },
    { NULL, "IRELAND" },
    { NULL, "MOSCOW1" },
    { NULL, "MOSCOW" },
    { NULL, "MOSCOW-1" },
    { NULL, "POLAND" },
    { NULL, "TURKEY" },
    { NULL, "UNITED-KINGDOM" },
    { NULL, "GMT" },
    { NULL, "GMT1" },
    { NULL, "GMT2" },
    { NULL, "GMT3" },
    { "SOUTH-AMERICA", NULL },
    { NULL, "BRAZIL-EAST" },
    { NULL, "BRAZIL-WEST" },
    { NULL, "BRAZIL-ACRE" },
    { NULL, "BRAZIL-DENORONHA" },
    { NULL, "CHILE-CONTINENTAL" },
    { NULL, "CHILE-EASTER-ISLAND" },
    { NULL, "GMT-6" },
    { NULL, "GMT-5" },
    { NULL, "GMT-4" },
    { NULL, "GMT-3" },
    { "SOUTH-PACIFIC", NULL },
    { NULL, "NEW-ZEALAND-CHATHAM" },
    { NULL, "NEW-ZEALAND" },
    { NULL, "US-HAWAII" },
    { NULL, "GMT10" },
    { NULL, "GMT11" },
    { NULL, "GMT12" },
    { NULL, "GMT-12" },
    { NULL, "GMT-11" },
    { NULL, "GMT-10" },
    { "SOUTHEAST-ASIA", NULL },
    { NULL, "INDONESIA-WEST" },
    { NULL, "INDONESIA-CENTRAL" },
    { NULL, "INDONESIA-EAST" },
    { NULL, "MALAYSIA" },
    { NULL, "PHILIPPINES" },
    { NULL, "SINGAPORE" },
    { NULL, "THAILAND" },
    { NULL, "VIETNAM" },
    { NULL, "GMT7" },
    { NULL, "GMT8" },
    { NULL, "GMT9" },
    { "GMT-OFFSET", NULL },
    { NULL, "GMT-12" },
    { NULL, "GMT-11" },
    { NULL, "GMT-10" },
    { NULL, "GMT-9" },
    { NULL, "GMT-8" },
    { NULL, "GMT-7" },
    { NULL, "GMT-6" },
    { NULL, "GMT-5" },
    { NULL, "GMT-4" },
    { NULL, "GMT-3" },
    { NULL, "GMT-3-and-half" },
    { NULL, "GMT-2" },
    { NULL, "GMT-2-and-half" },
    { NULL, "GMT-1" },
    { NULL, "GMT" },
    { NULL, "GMT1" },
    { NULL, "GMT2" },
    { NULL, "GMT3" },
    { NULL, "GMT3-and-half" },
    { NULL, "GMT4" },
    { NULL, "GMT5" },
    { NULL, "GMT5-and-half" },
    { NULL, "GMT6" },
    { NULL, "GMT7" },
    { NULL, "GMT8" },
    { NULL, "GMT9" },
    { NULL, "GMT9-and-half" },
    { NULL, "GMT10" },
    { NULL, "GMT10-and-half" },
    { NULL, "GMT11" },
    { NULL, "GMT12" },
    { NULL, "GMT13" },
    { NULL, NULL }
};

/*
** Character set lists generated (partially) with:
**
** iivalres -v ii."*".setup.ii_charset BOGUS_CHARSET | \
**    grep [[:upper:]][[:upper:]] | \
**    awk '{printf \
**    "    { NULL, \"%s\" },\n    { NULL, \"%s\" },\n    { NULL, \"%s\" },\n", \
**    $1,$2,$3}' 
*/
GLOBALDEF CHARSET linux1_charsets[] = {
    { "Western European", NULL },
    { NULL, "Arabic (ARABIC)" },
    { NULL, "Arabic (DOSASMO)" },
    { NULL, "Arabic (WARABIC)" },
    { NULL, "Cyrillic (ALT)" },
    { NULL, "Cyrillic (CW)" },
    { NULL, "Cyrillic (IBMPC866)" },
    { NULL, "English (IBMPC437)" },
    { NULL, "Greek (ELOT437)" },
    { NULL, "Greek (GREEK)" },
    { NULL, "Greek (ISO88597)" },
    { NULL, "Greek (WIN1253)" },
    { NULL, "Hebrew (HEBREW)" },
    { NULL, "Hebrew (WHEBREW)" },
    { NULL, "Hebrew (PCHEBREW)" },
    { NULL, "Japanese (KANJIEUC)" },
    { NULL, "Japanese (SHIFTJIS)" },
    { NULL, "Korean (KOREAN)" },
    { NULL, "Latin (IS885915)" },
    { NULL, "Latin (ISO88591)" },
    { NULL, "Latin (ISO88592)" },
    { NULL, "Latin (ISO88595)" },
    { NULL, "Multilingual (IBMPC850)" },
    { NULL, "Roman (HPROMAN8)" },
    { NULL, "Russian (KOI8)" },
    { NULL, "Simplified Chinese (CSGB2312)" },
    { NULL, "Simplified Chinese (CSGBK)" },
    { NULL, "Simplified Chinese (CHINESES)" },
    { NULL, "Slavic (SLAV852)" },
    { NULL, "Thai (CHTHP)" },
    { NULL, "Thai (CHTBIG5)" },
    { NULL, "Thai (CHTEUC)" },
    { NULL, "Thai (THAI)" },
    { NULL, "Thai (WTHAI)" },
    { NULL, "Turkish (PC857)" },
    { NULL, "Turkish (ISO88599)" },
    { NULL, "Eastern Europe (WIN1250)" },
    { NULL, "Western Europe (WIN1252)" },
    { "Eastern European", NULL },
    { "East Asian", NULL },
    { "South Asian", NULL },
    { "Middle Eastern", NULL },
    { "Unicode", NULL },
    { NULL, NULL }
};

GLOBALDEF CHARSET linux_charsets[] = {
    { "Arabic", NULL },
    { NULL, "ARABIC" },
    { NULL, "DOSASMO" },
    { NULL, "WARABIC" },
    { "Cyrillic", NULL },
    { NULL, "ALT" },
    { NULL, "CW" },
    { NULL, "IBMPC866" },
    { "Greek", NULL },
    { NULL, "ELOT437" },
    { NULL, "GREEK" },
    { NULL, "ISO88597" },
    { NULL, "WIN1253" },
    { "Hebrew", NULL },
    { NULL, "HEBREW" },
    { NULL, "WHEBREW" },
    { NULL, "PCHEBREW" },
    { "Japanese", NULL },
    { NULL, "KANJIEUC" },
    { NULL, "SHIFTJIS" },
    { "Korean", NULL },
    { NULL, "KOREAN" },
    { "Multilingual", NULL },
    { NULL, "IBMPC850" },
    { "Simplified Chinese", NULL },
    { NULL, "CSGB2312" },
    { NULL, "CSGBK" },
    { NULL, "CHINESES" },
    { "Thai", NULL },
    { NULL, "CHTHP" },
    { NULL, "CHTBIG5" },
    { NULL, "CHTEUC" },
    { NULL, "THAI" },
    { NULL, "WTHAI" },
    { "Turkish", NULL },
    { NULL, "PC857" },
    { NULL, "ISO88599" },
    { "Western European", NULL },
    { NULL, "IBMPC437" },
    { NULL, "IS885915" },
    { NULL, "ISO88591" },
    { NULL, "ISO88592" },
    { NULL, "ISO88595" },
    { NULL, "HPROMAN8" },
    { NULL, "WIN1252" },
    { "Eastern European", NULL },
    { NULL, "WIN1250" },
    { NULL, "KOI8" },
    { NULL, "SLAV852" },
    { "Unicode", NULL },
    { NULL, "UTF8" },
    { NULL, NULL }
};

GLOBALDEF CHARSET windows_charsets[] = {
    { "Western European", NULL },
    { NULL, "English (IBMPC437)" },
    { NULL, "Latin (IS885915)" },
    { NULL, "Latin (ISO88591)" },
    { NULL, "Latin (ISO88592)" },
    { NULL, "Latin (ISO88595)" },
    { NULL, "Multilingual (IBMPC850)" },
    { NULL, "Roman (HPROMAN8)" },
    { NULL, "Western Europe (WIN1252)" },
    { "Eastern European", NULL },
    { NULL, "Cyrillic (ALT)" },
    { NULL, "Cyrillic (CW)" },
    { NULL, "Cyrillic (IBMPC866)" },
    { NULL, "Eastern Europe (WIN1250)" },
    { NULL, "Russian (KOI8)" },
    { NULL, "Slavic (SLAV852)" },
    { "East Asian", NULL },
    { NULL, "Japanese (KANJIEUC)" },
    { NULL, "Japanese (SHIFTJIS)" },
    { NULL, "Korean (KOREAN)" },
    { NULL, "Simplified Chinese (CSGB2312)" },
    { NULL, "Simplified Chinese (CSGBK)" },
    { NULL, "Simplified Chinese (CHINESES)" },
    { NULL, "Thai (CHTHP)" },
    { NULL, "Thai (CHTBIG5)" },
    { NULL, "Thai (CHTEUC)" },
    { NULL, "Thai (THAI)" },
    { NULL, "Thai (WTHAI)" },
    { "South Asian", NULL },
    { "Middle Eastern", NULL },
    { NULL, "Arabic (ARABIC)" },
    { NULL, "Arabic (DOSASMO)" },
    { NULL, "Arabic (WARABIC)" },
    { NULL, "Greek (ELOT437)" },
    { NULL, "Greek (GREEK)" },
    { NULL, "Greek (ISO88597)" },
    { NULL, "Greek (WIN1253)" },
    { NULL, "Hebrew (HEBREW)" },
    { NULL, "Hebrew (WHEBREW)" },
    { NULL, "Hebrew (PCHEBREW)" },
    { NULL, "Turkish (PC857)" },
    { NULL, "Turkish (ISO88599)" },
    { "Unicode", NULL },
    { NULL, "UTF8" },
    { NULL, NULL }
};

GLOBALDEF TERMTYPE terminals[] = {
    { "ANSI", NULL },
    { NULL, "ansif" },
    { NULL, "ansinf" },
    { NULL, "iris-ansi" },
    { "BULL", NULL },
    { NULL, "bull12" },
    { NULL, "bull24" },
    { NULL, "bullbds1" },
    { NULL, "bullvtu10" },
    { NULL, "bullwv" },
    { "DG", NULL },
    { NULL, "dg100em" },
    { NULL, "dg220em" },
    { NULL, "dgxterm" },
    { "SUN", NULL },
    { NULL, "dtterm" },
    { NULL, "gnome-sun4" },
    { NULL, "gnome-sun" },
    { NULL, "suncmdf" },
    { NULL, "sunf" },
    { NULL, "sunk" },
    { NULL, "sunm" },
    { NULL, "suntype5" },
    { NULL, "xsun" },
    { "HP", NULL },
    { NULL, "hp2392" },
    { NULL, "hp70092" },
    { NULL, "hpterm" },
    { "PC", NULL },
    { NULL, "ibm5151f" },
    { NULL, "ibmpc" },
    { NULL, "pc-220" },
    { NULL, "pc-305" },
    { NULL, "pc-386" },
    { NULL, "pckermit" },
    { NULL, "winpcalt" },
    { "ICL", NULL },
    { NULL, "icl12" },
    { NULL, "icl34" },
    { NULL, "icl5" },
    { NULL, "icldrs" },
    { "Linux", NULL },
    { NULL, "konsole" },
    { "MWS", NULL },
    { NULL, "mws00" },
    { NULL, "mws01" },
    { NULL, "mws02" },
    { NULL, "mws03" },
    { NULL, "mws04" },
    { NULL, "mws05" },
    { NULL, "mws06" },
    { "VT", NULL },
    { NULL, "vt100f" },
    { NULL, "vt100hp" },
    { NULL, "vt100i" },
    { NULL, "vt100nk" },
    { NULL, "vt200i" },
    { NULL, "vt220ak" },
    { NULL, "vt220" },
    { "Other", NULL },
    { NULL, "io8256" },
    { NULL, "h19f" },
    { NULL, "h19nk" },
    { NULL, "wview" },
    { NULL, "wy60at" },
    { NULL, "xmlname" },
    { NULL, "97801f" },
    { NULL, "at386" },
    { NULL, "pt35" },
    { NULL, "pt35t" },
    { NULL, "tk4105" },
    { NULL, "m30n" },
    { NULL, "m91e" },
    { NULL, "mac2" },
    { NULL, "cbf" },
    { NULL, "frs" },
    { NULL, NULL }
};

/* date type alias */
GLOBALDEF struct _date_type_alias {
		II_RFAPI_PNAME	rfapi_name;	
		const char	*values[2];
		enum {
		    ANSI,
		    INGRES,
		    }		val_idx;
	} date_type_alias = {
		II_DATE_TYPE_ALIAS,
		{ "ansidate", "ingresdate" },
		INGRES
	};

/* Ingres config type */
GLOBALDEF struct _ing_config_type {
		II_RFAPI_PNAME	rfapi_name;	
		const char	*values[4];
		const char	*descrip[4];
		enum {
		    TXN,
		    BI,
		    CM,
		    TRAD
		    }		val_idx;
	} ing_config_type = {
		II_CONFIG_TYPE,
		{ "TXN", "BI", "CM", "TRAD" },
		{ "Transactional System",
		    "Business Intelligence System",
		    "Content Management System",
		    "Traditional Ingres Configuration" },
		TXN
	};
/* package info */
static package pkg_core = {
		PKG_CORE,
		PKG_NONE,
		"",
		"",
		II_COMPONENT_CORE,
		TRUE };
static package pkg_dbms = {
		PKG_DBMS,
		PKG_CORE,
		"Ingres Database Server",
		"dbms",
		II_COMPONENT_DBMS,
		FALSE };
static package pkg_net = {
		PKG_NET,
		PKG_CORE,
		"Ingres Client",
		"net",
		II_COMPONENT_NET,
		FALSE };
static package pkg_odbc = {
		PKG_ODBC,
		PKG_CORE,
		"Ingres ODBC Connectivity",
		"odbc",
		II_COMPONENT_ODBC,
		FALSE };
static package pkg_rep = {
		PKG_REP,
		PKG_DBMS|PKG_NET,
		"Ingres Replicator",
		"rep",
		II_COMPONENT_REPLICATOR,
		FALSE };
static package pkg_abf = {
		PKG_ABF,
		PKG_CORE,
		"Ingres ABF/Vision",
		"abf",
		II_COMPONENT_FRONTTOOLS,
		FALSE };
static package pkg_star = {
		PKG_STAR,
		PKG_DBMS|PKG_NET,
		"Ingres Star Distributed Database Engine",
		"star",
		II_COMPONENT_STAR,
		FALSE }; 
static package pkg_jdbc = {
		PKG_JDBC,
		PKG_CORE,
		"Ingres JDBC Driver",
		"jdbc",
		II_COMPONENT_JDBC_CLIENT,
		TRUE };
static package pkg_ice = {
		PKG_ICE,
		PKG_DBMS,
		"Ingres Web Deployment Option",
		"ice",
		TRUE };
static package pkg_sup32 = {
		PKG_SUP32,
		PKG_CORE,
		"Ingres 32bit Support",
		"32bit",
		TRUE };
static package pkg_lgpl = {
		PKG_LGPL,
		PKG_NONE,
		"Ingres GPL Licence",
		"license-gpl",
		TRUE };
static package pkg_lcom = {
		PKG_LCOM,
		PKG_NONE,
		"Ingres Commercial Licence",
		"license-com",
		TRUE };
static package pkg_leval = {
		PKG_LEVAL,
		PKG_NONE,
		"Ingres Evaluation Licence",
		"license-eval",
		TRUE };
static package pkg_doc = {
		PKG_DOC,
		PKG_NONE,
		"Ingres Documentation",
		"documentation",
		II_COMPONENT_DOCUMENTATION,
		TRUE };

/* obsolete packages */
static package pkg_bridge = {
		PKG_BRIDGE,
		PKG_CORE,
		"Ingres Bridge Server (Obsolete)",
		"bridge",
		GIP_OBS_PKG,
		FALSE };
static package pkg_c2audit = {
		PKG_C2,
		PKG_DBMS,
		"Ingres C2 Security (Obsolete)",
		"c2audit",
		GIP_OBS_PKG,
		FALSE };
static package pkg_das = {
		PKG_DAS,
		PKG_CORE,
		"Ingres Data Access Server (Obsolete)",
		"das",
		GIP_OBS_PKG,
		FALSE };
static package pkg_esql = {
		PKG_ESQL,
		PKG_CORE,
		"Ingres Embedded SQL Precomilers (Obsolete)",
		"esql",
		GIP_OBS_PKG,
		FALSE };
static package pkg_i18n = {
		PKG_I18N,
		PKG_CORE,
		"Ingres Internationalization Support (Obsolete)",
		"i18n",
		GIP_OBS_PKG,
		FALSE };
static package pkg_ome = {
		PKG_OME,
		PKG_DBMS,
		"Ingres Object Management Extensions (Obsolete)",
		"ome",
		GIP_OBS_PKG,
		FALSE };
static package pkg_qrrun = {
		PKG_QRRUN,
		PKG_CORE,
		"Ingres Query/Reportng Runtime (Obsolete)",
		"qr_run",
		GIP_OBS_PKG,
		FALSE };
static package pkg_tuxedo = {
		PKG_TUX,
		PKG_DBMS,
		"Ingres Tuxedo Support (Obsolete)",
		"tuxedo",
		GIP_OBS_PKG,
		FALSE };
static package pkg_vision = {
		PKG_VIS,
		PKG_CORE,
		"Ingres Vision (Obsolete)",
		"vision",
		GIP_OBS_PKG,
		FALSE };

GLOBALDEF package *packages[] = {
	&pkg_core,
	&pkg_sup32,
        &pkg_abf,
        &pkg_bridge,
        &pkg_c2audit,
        &pkg_das,
        &pkg_dbms,
        &pkg_esql,
        &pkg_i18n,
        &pkg_ice,
        &pkg_jdbc,
        &pkg_net,
        &pkg_odbc,
        &pkg_ome,
        &pkg_qrrun,
        &pkg_rep,
        &pkg_star,
        &pkg_tuxedo,
        &pkg_vision,
	&pkg_doc,
	NULL
	};

GLOBALDEF package *licpkgs[] = {
	&pkg_lgpl,
	&pkg_lcom,
	&pkg_leval,
	NULL
	};

/* Installer GLOBALs */
/* GTK globals */
GLOBALDEF GtkWidget *IngresInstall;
GLOBALDEF GtkWidget *CharsetDialog;
GLOBALDEF GtkWidget *TimezoneDialog;
GLOBALDEF GtkWidget *InstanceTreeView;
GLOBALDEF GtkTreeModel *CharsetTreeModel;
GLOBALDEF GtkTreeModel *TimezoneTreeModel;
GLOBALDEF GtkTreeIter *timezone_iter;
GLOBALDEF GtkTreeIter *charset_iter;

/* Installer mode and stage globals */
GLOBALDEF i4 instmode=BASIC_INSTALL;
GLOBALDEF i4 runmode=MA_INSTALL;
GLOBALDEF PKGLST pkgs_to_install=TYPICAL_SERVER;
GLOBALDEF PKGLST mod_pkgs_to_install=PKG_NONE;
GLOBALDEF PKGLST mod_pkgs_to_remove=PKG_NONE;
GLOBALDEF const char install_notebook[] = { "install_notebook" };
GLOBALDEF const char upgrade_notebook[] = { "upgrade_notebook" };
GLOBALDEF const char *current_notebook = install_notebook;
GLOBALDEF UGMODE ug_mode = UM_FALSE; /* user chosen upgrade mode */
GLOBALDEF UGMODE inst_state = UM_FALSE; /* upgrade mode dictated by existing instances */
GLOBALDEF i4 inst_count=0;
GLOBALDEF i4 current_stage = NI_START;
GLOBALDEF i4 debug_trace = 0;

/* screen names and progress bar stages */				
static stage_info install_stages[] = {
	/* install frame detail */
	{ RF_START, "prog_welcome", ST_COMPLETE },
	{ RF_PLATFORM, "rf_platform", ST_INIT },
	{ NI_START, "prog_welcome", ST_COMPLETE },
	{ NI_LIC, "prog_lic", ST_INIT },
	{ NI_CFG, "prog_cfg", ST_INIT },
	{ NI_EXP, "prog_exp", ST_INIT },
	{ NI_INSTID, "prog_instid", ST_INIT },
	{ NI_PKGSEL, "prog_pkgsel", ST_INIT },
	{ NI_DBLOC, "prog_dbloc", ST_INIT },
	{ NI_VWCFG, "prog_vwcfg", ST_INIT },
	{ NI_LOGLOC, "prog_logloc", ST_INIT },		
	{ NI_LOCALE, "prog_locale", ST_INIT },
	{ NI_MISC,	"prog_misc", ST_INIT },
	{ RF_WINMISC,	"prog_misc", ST_INIT },
	{ NI_SUMMARY, "prog_summary", ST_INIT },
	{ NI_INSTALL, "prog_inst", ST_INIT },
	{ NI_DONE, "prog_done", ST_INIT },
	{ NI_FAIL, "prog_done", ST_INIT },
	{ RF_SUMMARY, "rf_preview", ST_INIT },
	{ RF_DONE, "rf_done", ST_INIT },
	};
			
static stage_info upgrade_stages[] = {
	/* install frame detail */
	{ UG_START, "upgrade_welcome", ST_COMPLETE },
	{ UG_LIC, "upgrade_license", ST_INIT },
	{ UG_SSAME, "upgrade_inst_mod", ST_INIT },
	{ UG_SOLD,"upgrade_inst_ug", ST_INIT },
	{ UG_MULTI,"upgrade_inst_multi", ST_INIT },
	{ UG_SELECT, "upgrade_select", ST_INIT },
	{ UG_MOD, "upgrade_modify", ST_INIT },
	{ UG_MOD_DBLOC, "modify_dbloc", ST_INIT },
	{ UG_MOD_LOGLOC, "modify_logloc", ST_INIT },
	{ UG_MOD_MISC, "modify_misc", ST_INIT },
	{ UG_SUMMARY, "upgrade_summary", ST_INIT },
	{ UG_DO_INST, "upgrade_do_inst", ST_INIT },
	{ UG_DONE, "upgrade_done", ST_INIT },
	{ UG_FAIL, "upgrade_done", ST_INIT }
	};


GLOBALDEF stage_info *adv_stages[] = { 
	&install_stages[NI_START], /* start screen (WELCOME_TO_INSTALL) */
	&install_stages[NI_LIC], /* license agreement (WELCOME_TO_INSTALL) */
	&install_stages[NI_CFG], /* config type (CHOOSE_CONFIG_TYPE)*/
	&install_stages[NI_EXP], /* experience level (CHOOSE_INSTALL_TYPE)*/
	&install_stages[NI_INSTID], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_PKGSEL], /* basic config (BASIC_INSTALL) */
	&install_stages[NI_DBLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_LOGLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_LOCALE],
	&install_stages[NI_MISC], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_SUMMARY], /* pre installation summary (SUMMARY_SCREEN) */
	&install_stages[NI_INSTALL], /* actual installation (INSTALL_SCREEN) */
	&install_stages[NI_DONE], /* finish screen (FINISH_INSTALL) */
	&install_stages[NI_FAIL], /* failure screen (FINISH_INSTALL) */
	NULL /* end of array */
	};
					
GLOBALDEF stage_info *adv_ivw_stages[] = { 
	&install_stages[NI_START], /* start screen (WELCOME_TO_INSTALL) */
	&install_stages[NI_LIC], /* license agreement (WELCOME_TO_INSTALL) */
	&install_stages[NI_EXP], /* experience level (CHOOSE_INSTALL_TYPE)*/
	&install_stages[NI_INSTID], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_PKGSEL], /* basic config (BASIC_INSTALL) */
	&install_stages[NI_DBLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_VWCFG],  /* vectorwise config */
	&install_stages[NI_LOGLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_MISC], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_SUMMARY], /* pre installation summary (SUMMARY_SCREEN) */
	&install_stages[NI_INSTALL], /* actual installation (INSTALL_SCREEN) */
	&install_stages[NI_DONE], /* finish screen (FINISH_INSTALL) */
	&install_stages[NI_FAIL], /* failure screen (FINISH_INSTALL) */
	NULL /* end of array */
	};
					
GLOBALDEF stage_info *adv_nodbms_stages[] = { 
	&install_stages[NI_START], /* start screen (WELCOME_TO_INSTALL) */
	&install_stages[NI_LIC], /* license agreement (WELCOME_TO_INSTALL) */
	&install_stages[NI_EXP], /* experience level (CHOOSE_INSTALL_TYPE)*/
	&install_stages[NI_INSTID], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_PKGSEL], /* basic config (BASIC_INSTALL) */
	&install_stages[NI_DBLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_LOCALE],
	&install_stages[NI_MISC], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_SUMMARY], /* pre installation summary (SUMMARY_SCREEN) */
	&install_stages[NI_INSTALL], /* actual installation (INSTALL_SCREEN) */
	&install_stages[NI_DONE], /* finish screen (FINISH_INSTALL) */
	&install_stages[NI_FAIL], /* failure screen (FINISH_INSTALL) */
	NULL /* end of array */
	};
					
GLOBALDEF stage_info *rfgen_lnx_stages[] = { 
	&install_stages[RF_START], /* start screen  */
	&install_stages[RF_PLATFORM], /* deployment platform  */
	&install_stages[NI_LIC], /* config type (CHOOSE_CONFIG_TYPE)*/
	&install_stages[NI_CFG], /* config type (CHOOSE_CONFIG_TYPE)*/
	&install_stages[NI_EXP], /* experience level (CHOOSE_INSTALL_TYPE)*/
	&install_stages[NI_INSTID], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_PKGSEL], /* basic config (BASIC_INSTALL) */
	&install_stages[NI_DBLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_VWCFG],  /* vectorwise config */
	&install_stages[NI_LOGLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_LOCALE],
	&install_stages[NI_MISC], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[RF_SUMMARY], /* pre installation summary (SUMMARY_SCREEN) */
	&install_stages[RF_DONE], /* finish screen (FINISH_INSTALL) */
	NULL /* end of array */
	};
					
GLOBALDEF stage_info *rfgen_win_stages[] = { 
	&install_stages[RF_START], /* start screen  */
	&install_stages[RF_PLATFORM], /* deployment platform  */
	&install_stages[NI_CFG], /* config type (CHOOSE_CONFIG_TYPE)*/
	&install_stages[NI_EXP], /* experience level (CHOOSE_INSTALL_TYPE)*/
	&install_stages[NI_INSTID], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_PKGSEL], /* basic config (BASIC_INSTALL) */
	&install_stages[NI_DBLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_LOGLOC],  /* advanced config (ADVANCED_INSTALL) */
	&install_stages[NI_LOCALE],
	&install_stages[RF_WINMISC], /* advanced config (ADVANCED_INSTALL) */
	&install_stages[RF_SUMMARY], /* pre installation summary (SUMMARY_SCREEN) */
	&install_stages[RF_DONE], /* finish screen (FINISH_INSTALL) */
	NULL /* end of array */
	};
					
GLOBALDEF stage_info *basic_stages[] = { 
	&install_stages[NI_START], /* start screen (WELCOME_TO_INSTALL) */
	&install_stages[NI_LIC], /* license agreement (WELCOME_TO_INSTALL) */
	&install_stages[NI_CFG], /* config type (CHOOSE_CONFIG_TYPE)*/
	&install_stages[NI_EXP], /* experience level (CHOOSE_INSTALL_TYPE)*/
	&install_stages[NI_PKGSEL], /* basic config (BASIC_INSTALL) */
	&install_stages[NI_SUMMARY], /* pre installation summary (SUMMARY_SCREEN) */
	&install_stages[NI_INSTALL], /* actual installation (INSTALL_SCREEN) */
	&install_stages[NI_DONE], /* finish screen (FINISH_INSTALL) */
	&install_stages[NI_FAIL], /* failure screen (FINISH_INSTALL) */
	NULL /* end of array */
	};	

GLOBALDEF stage_info *basic_ivw_stages[] = { 
	&install_stages[NI_START], /* start screen (WELCOME_TO_INSTALL) */
	&install_stages[NI_LIC], /* license agreement (WELCOME_TO_INSTALL) */
	&install_stages[NI_EXP], /* experience level (CHOOSE_INSTALL_TYPE)*/
	&install_stages[NI_PKGSEL], /* basic config (BASIC_INSTALL) */
	&install_stages[NI_SUMMARY], /* pre installation summary (SUMMARY_SCREEN) */
	&install_stages[NI_INSTALL], /* actual installation (INSTALL_SCREEN) */
	&install_stages[NI_DONE], /* finish screen (FINISH_INSTALL) */
	&install_stages[NI_FAIL], /* failure screen (FINISH_INSTALL) */
	NULL /* end of array */
	};	

GLOBALDEF stage_info **stage_names = basic_stages;

GLOBALDEF stage_info *ug_stages[] = {
			&upgrade_stages[UG_START],
			&upgrade_stages[UG_LIC],
			&upgrade_stages[UG_SSAME],
			&upgrade_stages[UG_SOLD],
			&upgrade_stages[UG_MULTI],
			&upgrade_stages[UG_SELECT],
			&upgrade_stages[UG_MOD],
			&upgrade_stages[UG_MOD_DBLOC],
			&upgrade_stages[UG_MOD_LOGLOC],
			&upgrade_stages[UG_MOD_MISC],
			&upgrade_stages[UG_SUMMARY],
			&upgrade_stages[UG_DO_INST],
			&upgrade_stages[UG_DONE],
			&upgrade_stages[UG_FAIL],
			NULL
			};

/* existing instances */
GLOBALDEF instance *existing_instances[256] = { NULL };
GLOBALDEF instance *selected_instance=NULL;
GLOBALDEF saveset new_pkgs_info;

/* Response file and installation globals */
GLOBALDEF II_RFAPI_PLATFORMS dep_platform = II_RF_DP_LINUX;
# ifdef NT_GENERIC
GLOBALDEF II_RFAPI_PLATFORMS run_platform = II_RF_DP_WINDOWS;
# else
GLOBALDEF II_RFAPI_PLATFORMS run_platform = II_RF_DP_LINUX;
# endif
GLOBALDEF LOCATION	rfnameloc ; /* response file LOCATION */
GLOBALDEF char		rfnamestr[MAX_FNAME] ; /* response file string */
GLOBALDEF char		rfdirstr[MAX_LOC] ; /* response file dir string */
GLOBALDEF LOCATION	rndirloc ; /* rename temporary LOCATION */
GLOBALDEF char		rndirstr[MAX_LOC] ; /* rename temporary dir string */
GLOBALDEF _childinfo	childinfo = { FAIL, FALSE };
GLOBALDEF char	inst_stdout[MAX_LOGFILE_NAME]; /* STDOUT from install process */
GLOBALDEF char	inst_stderr[MAX_LOGFILE_NAME]; /* STDERR from install process */

# endif /* xCL_GTK_EXISTS */
