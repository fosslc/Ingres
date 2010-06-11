/*
** Copyright (c) 1998, 2001 Ingres Corporation
*/

/*
**
** Name: install.h - header file for install.cpp .
**
** History:
**	01-apr-2001 (mcgem01)
**	    Added RegisterActiveXControls().
**	10-apr-2001 (somsa01)
**	    Removed code not needed by the post install DLL.
**	07-may-2001 (somsa01)
**	    Changed m_upgrade to m_DBMSupgrade for clarity. Also,
**	    removed m_createicons. Removed EsqlcPostInstallation() and
**	    EsqlCobolPostInstallation() functions.
**	23-may-2001 (somsa01)
**	    Added InstallAcrobat() and IsCurrentAcrobatAlreadyInstalled().
**	06-jun-2001 (somsa01)
**	    Added GetESQLFORTRAN().
**	15-jun-2001 (penga03)
**	    Added RemoveTempMSIAndCab().
**	23-July-2001 (penga03)
**	    Added Local_PMget and removed AlreadyThere.
**	15-aug-2001 (somsa01)
**	    Added RemoveOldShellLinks().
**	30-oct-2001 (penga03)
**	    Added DeleteObsoleteFileAssociations().
**	06-mar-2002 (penga03)
**	    Added Create_OtherDBUsers(), InstallPortal(), StartPortal(), 
**	    OpenPortal(), InstallForestAndTrees(), InstallOpenROAD(), and 
**	    UpdateFile(). Modified StartServer() and Execute().
**	21-mar-2002 (penga03)
**	    Added member variables m_InstallSource, m_ApacheInstalled, 
**	    m_IcePortNumber, m_PortalHostPort and m_OrPortNumber.
**	    Added member functions GetHostName(), GetPortNumber(), 
**	    CheckOnePortNumber(), CheckPortNumbers(), CheckSocket(), 
**	    AddOSUser(), ReplaceString(), InstallApache(), StartApache() 
**	    and CreateOneShortcut().
**	18-may-2002 (penga03)
**	    Added UpdateIngresBatchFiles().
**	16-jul-2002 (penga03)
**	    Added m_HTTP_Server.
**	09-aug-2002 (penga03)
**	    Added m_endTime.
**	06-feb-2003 (penga03)
**	    Removed CheckValidLicense().
**	26-jan-2004 (penga03)
**	    Added m_dateformat and m_moneyformat.
**	26-jan-2004 (penga03)
**	    Added InstallJRE().
**	23-mar-2004 (penga03)
**	    Added m_StartIVM.
**  04-jun-2004 (gupsh01).
**	    Added LoadMDB().
**  21-jun-2004 (gupsh01)
**      Added Grant_mdbimaadt().
**  23-jun-2004 (penga03)
**      Added m_bInstallMDB, m_MDBName.
**	26-jul-2004 (penga03)
**	    Added RemoveEnvironmentVariables().
**	07-sep-2004 (penga03)
**	   Removed m_serviceloginid and m_servicepassword.
**	10-sep-2004 (penga03)
**	    Removed MDB.
**	19-sep-2004 (penga03)
**	   Backed out the change made on 07-sep-2004 (471628).
**	20-oct-2004 (penga03)
**	   Renamed RemoveEnvironmentVariables() to 
**	   RemoveObsoleteEnvironmentVariables().
**	03-nov-2004 (penga03)
**	    Removed m_bMDBInstall, m_MDBName and InstallMDB().
**	17-nov-2004 (penga03)
**	    Added m_completestring.
**	11-jan-2005 (penga03)
**	    Added m_decimal.
**	13-jan-2005 (penga03)
**	    Added m_MDBSize and m_MDBDebug.
**	14-jan-2005 (penga03)
**	    Added m_adminName and GetLocalizedAdminName().
**	03-march-2005 (penga03)
**	    Added Ingres cluster support.
**	07-march-2005 (penga03)
**	    Modified  RegisterClusterService().
**	08-apr-2005 (penga03)
**	    Added OnlineResource(), OfflineResource() and 
**	    RegisterCluster().
**	23-jun-2005 (penga03)
**	    Modified the return type of InstallMDB().
**	26-aug-2005 (penga03)
**	    Added m_bClusterInstallEx, set to T if this is the HAO install
**	    on the second node.
**  	15-Nov-2006 (drivi01)
**          SIR 116599
**          Enhanced post-installer in effort to improve installer usability.
**  	12-Apr-2007 (drivi01)
**	    Create variable for m_rmcmdcount to store rmcmd count in config.dat
**      03-May-2007 (drivi01)
**	    Added m_attemptedUDBupgrade, this variable will be always set to
**	    FALSE unless iipostinst step into or near UpgradeDatabase code, in
**	    which case the variable will be set to TRUE to notify that this
**	    was a full iipostinst following valid upgrade.
**  	25-May-2007 (drivi01)
**          Added new property to installer INGRES_WAIT4GUI which will force
**          setup.exe to wait for post installer to finish if this property
**          is set to 1. INGRES_WAIT4GUI which is referred to as 
**	    m_bWait4PostInstall within ingconfig.exe is set to TRUE if "/w" 
**	    flag is passed to setup.exe. If m_bbWait4PostInstall is set to 
**	    TRUE, ingconfig.exe will post messages to other windows instead 
**	    of sending them b/c setup.exe window will be frozen waiting for
**	    ingconfig.exe to return and this will result in a hang since
**	    setup.exe will be unable to process the message.
**	07-Sep-2007 (drivi01)
**	    Added new function CreateConfigDir() which will create direcotry
**	    for configuration file in vdba and vdbasql utilities.
**	10-Apr-2008 (drivi01)
**	    Added variable m_DBATools to keep track of DBA Tools installations.
**	30-Apr-2008 (drivi01)
**	    Remove CreateConfigDir function.
**	18-Sep-2008 (drivi01)
**	    Add m_vnodeCount variable to store virtual node count from the
**	    registry as well as define newly added function AddVirtualNodes().
**  14-Jul-2009 (drivi01)
**	    SIR 122309
**	    Added new dialog to specify Configuration System for ingres instance.
**	    The dialog contains 4 options which will be represented as
**	    	0 for Transactional System
**		1 for Traditional Ingres System
**		2 for Business Intelligence System
**		3 for Content Management System
**	    Post-installer will read "ingconfigtype" registry entry
**	    and depending on the configuration chosen set a custom
**	    set of parameters for ingres instance.
**          The parameters and configuration type will be reflected in install.log.
**	06-Aug-2009 (drivi01)
**	    Remove FD_SETSIZE definition b/c it is defined in winsock2.h
**	    and is not used in this application.
**	04-Jun-2010 (drivi01)
**	    Remove II_GCNapi_ModifyNode API for creating virtual nodes
**	    for DBA package b/c II_GCNapi_ModifyNode is no longer supported
**	    and stopped working due to long ids change.
**	    This change replaces the obsolete API call with a range of
**	    supported open API calls.
**	    Added 2 new functions to handle new API calls: CreateVirtualNode
**	    and CheckAPIError.
**
*/

#define MAX_COMP_LEN 256
#define MAX_INGVER_SIZE 81

#include <winsock2.h>
#include "process.h"
#include <iiapi.h>

#define LOGNTFILE               "\\LogWatNT.exe"
#define SERVICENAME             "LogWatch"
#define DEFUALTNTPATH           "C:\\WinNT"
#define EVENTLOGNAME            "CA_LIC"
#define DISPLAYNAME             "Event Log Watch"
#define SMALL                   10

class CComponent : public CObject
{
public:
    CString m_name;
    double  m_size;
    BOOL    m_selected;
    BOOL    m_visible;
    CString m_installPath;
    BOOL    m_enabled;
public:
    CComponent(LPCSTR name,double size, BOOL selected,BOOL visible,LPCSTR path=0);
};

class CInstallation
{
public:
    CObArray	m_components;		
    CString	m_language;		
    CString	m_timezone;		
    CString	m_terminal;		
    CString	m_installationcode;
    CString	m_charset; 	
    CString	m_dateformat;
    CString	m_moneyformat;
    CString	m_decimal;
    CString	m_logname; 		
    CString	m_duallogname; 		
    CString	m_use_256_colors;			
    CTime	m_startTime;
    BOOL	m_serviceAuto;
    BOOL	m_configureIngres;
    BOOL	m_installDoc;
    CString	m_defaultInstallPath;
    CStatic	*m_output;
    HANDLE	m_process;
    HANDLE	m_logFile;
    HANDLE	m_hChildStdoutRd;
    BOOL	m_enableTCPIP;
    BOOL	m_enableNETBIOS;
    BOOL	m_enableSPX;
    BOOL	m_enableDECNET;
    CString	m_iidatabase; 		
    CString	m_iicheckpoint; 		
    CString	m_iijournal; 		
    CString	m_iidump; 		
    CString	m_iiwork; 		
    CString	m_iilogfilesize; 		
    CString	m_iilogfile; 		
    CString	m_iiduallogfile;
    CString	m_serviceloginid;
    CString	m_servicepassword; 		
    BOOL	m_enableduallogging; 		
    BOOL	m_win95;
    int 	m_systemPages;
    int 	m_nbPages;
    int 	m_pageSize;
    int 	m_charCount;
	int		m_vnodeCount;
	int		m_configType;
    HANDLE	m_postInstThread;
    BOOL	m_ingresAlreadyInstalled;
    BOOL	m_halted;
    CString	m_installPath;
    CString	m_InstallSource;
    CString	m_browsePath;
    CString	m_modulePath;
    CString	m_temp;
    CString	m_dataStoreName;
    BOOL	m_reboot;
    BOOL	m_postInstRet;
    CString	m_computerName;
	CString m_adminName;
    CString	m_userName;
    BOOL	m_DBMSupgrade;
    BOOL	m_sql92;
    BOOL	m_LeaveIngresRunningInstallIncomplete;
    BOOL	m_StartIVM;
    CString	m_IngresPreStartupRunPrg;
    CString	m_IngresPreStartupRunParams;
    BOOL	m_remove_IngresODBC;
    BOOL	m_remove_IngresODBCReadOnly;
    BOOL	m_setup_IngresODBC;
    BOOL	m_setup_IngresODBCReadOnly;
    BOOL	m_hidewindows;
    BOOL	m_upgradedatabases;
    BOOL	m_attemptedUDBupgrade; /*  This variable will be FALSE. 
					** If post installer gets near upgrade 
					** code for user databases, this variable
					** will be set to TRUE */
    BOOL	m_destroytxlogfile;
    BOOL	m_ApacheInstalled;
	CString m_HTTP_Server;
    CString	m_HTTP_ServerPath;
    BOOL	m_bInstallColdFusion;
    CString	m_ColdFusionPath;
    CString	m_ResponseFile;
    BOOL	m_bClusterInstall;
    BOOL	m_bClusterInstallEx;
    CString m_ClusterResource;
    BOOL	m_bMDBInstall;
    CString	m_MDBName;
    CString	m_MDBSize;
    BOOL	m_bMDBDebug;
    char	m_completestring[MAX_INGVER_SIZE];
	BOOL	m_express;
	BOOL	m_includeinpath;
	BOOL	m_installdemodb;
	BOOL	m_ansidate;
	BOOL	m_CompletedSuccessfully;
	BOOL	m_bWait4PostInstall;
	int	m_rmcmdcount;
	BOOL	m_DBATools;

public:
    BOOL OnlineResource();
    BOOL OfflineResource();
    BOOL RegisterCluster();
    BOOL AddVirtualNodes();
    int  CreateVirtualNode(CString vnodename, CString hostname, CString protocol, CString listenaddress, CString username, CString password);
    void CheckAPIError( IIAPI_GENPARM	*genParm);
    BOOL GetLocalizedAdminName();
    int InstallMDB();
    unsigned int RegisterClusterService(LPSTR *sharedDisk, INT sharedDiskCount, LPSTR lpszResourceName, LPSTR lpszInstallCode);
    BOOL RemoveObsoleteEnvironmentVariables();
	CTime m_endTime;
	void UpdateIngresBatchFiles();
    int	CreateOneShortcut(CString Folder, CString DisplayName, CString ExeFile, CString Params, CString WorkDir);
    u_short	m_OrPortNumber;
    u_short	m_IcePortNumber;
    u_short	m_PortalHostPort;
    BOOL	GetHostName(char *Hostname);
    VOID	CheckPortNumbers();
    BOOL	GetPortNumber(CString KeyName, CString &PortNumber, CString FileName);
    VOID	CheckOnePortNumber(CString KeyName, CString FileName, CString TempFile);
    VOID	CheckSocket(u_short *port);
    DWORD	AddOSUser(char *username, char *password, char *comment);
    BOOL	RegisterActiveXControls();
    BOOL	IsCurrentAcrobatAlreadyInstalled();
    void	InstallAcrobat();
	void	OpenPageForAcrobat();
    CComponent	*GetODBC();
    void	DeleteRegValue(CString strKey);
    void	SetODBCEnvironment();
    HANDLE	m_phToken;
    BOOL	InstallIngresService();
    CString	GetRegValue(CString strKey);
    BOOL	CheckpointDatabases(BOOL jnl_flag);
    BOOL	m_checkpointdatabases;
    HKEY	m_SoftwareKey;
    CComponent	*GetDBMS();
    CComponent	*GetNet();
    CComponent	*GetStar();
    CComponent	*GetESQLC();
    CComponent	*GetESQLCOB();
    CComponent	*GetESQLFORTRAN();
    CComponent	*GetTools();
    CComponent	*GetSQLServerGateway();
    CComponent	*GetOracleGateway();
    CComponent	*GetInformixGateway();
    CComponent	*GetSybaseGateway();
    CComponent	*GetVision();
    CComponent	*GetReplicat();
    CComponent	*GetICE();
    CComponent	*GetOpenROADDev();
    CComponent	*GetOpenROADRun();
    CComponent	*GetOnLineDoc();
    CComponent	*GetVDBA();
    CComponent	*GetEmbeddedComponents();
    CComponent	*GetJDBC();

    BOOL CopyOneFile(CString &src, CString &dst);
    BOOL RemoveFile(LPCSTR lpFileName);
    BOOL SetRegistryEntry(HKEY hRootKey, LPCSTR lpKey, LPCSTR lpValue, DWORD dwType, void *lpData);
    
    void DeleteComponents();
    void Halted();
    BOOL CreateIIDATABASE();
    BOOL CreateIICHECKPOINT();
    BOOL CreateIIJOURNAL();
    BOOL CreateIIDUMP();
    BOOL CreateIIWORK();
    BOOL CreateIILOGFILE();
    BOOL CreateRepDirectories();
    void SetDate();
    void ReadInstallLog();
    void ReadRNS();
    BOOL LoadIMA();
	BOOL LoadDemodb();
    BOOL LoadPerfmon();
    BOOL LoadICE();
    BOOL Create_icedbuser();
	BOOL Create_OtherDBUsers();
    BOOL DatabaseExists(LPCSTR lpName);
    BOOL CreateOneDatabase(LPCSTR lpName);
    BOOL CheckpointOneDatabase(LPCSTR lpName);
    BOOL CreateDatabases();
    BOOL StopServer(BOOL echo = TRUE);
    BOOL UpgradeDatabases();
    BOOL RemoveOneDirectory(char *szDirectoryPath);
    void Init();
    BOOL Execute(LPCSTR lpCmdLine,BOOL bWait=TRUE, BOOL bWindow=FALSE);
    
    BOOL CreateLogFile();
    BOOL UpdateLogFile();
    
    BOOL ServerPostInstallation();
    BOOL NetPostInstallation();
    BOOL StarPostInstallation();
    BOOL ToolsPostInstallation();
    BOOL VisionPostInstallation();
    BOOL ReplicatPostInstallation();
    BOOL IcePostInstallation();
    BOOL OpenROADPostInstallation();
    BOOL JDBCPostInstallation();
    
    BOOL SetConfigDat();
    BOOL SetSymbolTbl();
    BOOL SetServerSymbolTbl();
    BOOL CreateServerDirectories();
    BOOL SetEnvironmentFile();
    BOOL CleanSharedMemory();
    BOOL StartServer(BOOL Comment=TRUE);
    BOOL IsBusy();
    VOID RetrieveHostname(CHAR *HostName);
    
    int Local_NMgtIngAt(CString strKey, CString &strRetValue);
	BOOL Local_PMget(CString strKey, CString &strRetValue);
	    
    /* Unattended Installation Support */
    BOOL m_UseResponseFile;
    BOOL GetRegValueBOOL(CString strKey, BOOL DefaultValue);
    BOOL WriteRSPContentsToLog();
    BOOL WriteUserEnvironmentToLog();
    void GetNTErrorText(int code, CString &strErrText);
    int RemoveOldShellLinks();
    
    BOOL RunPreStartupPrg();
    BOOL CheckAdminUser();
    BOOL m_bSkipLicenseCheck;
    BOOL m_bEmbeddedRelease;
    CString m_commandline;
    
    CInstallation();
    ~CInstallation();
    
    void  PostInstallation(CStatic *output);
    DWORD ThreadPostInstallation();
    DWORD ThreadAppendToLog();
    void AppendToLog(LPCSTR p);
    void AppendToLog(UINT uiStrID);
    int  Exec(LPCSTR cmdLine, LPCSTR param, BOOL appendToLog = TRUE, BOOL changeDir = TRUE);
    int CreateDirectoryRecursively(char *szDirectory);
    void RemoveOldDirectories();
    BOOL InstallPortal();
    BOOL StartPortal();
    void OpenPortal();
    BOOL InstallForestAndTrees();
    BOOL InstallOpenROAD();
    BOOL UpdateFile(CString KeyName, CString Value, CString FileName, CString TempFile, CString Symbol="", BOOL Append=FALSE);
    BOOL ReplaceString(CString KeyName, CString Value, CString FileName, CString TempFile);
    BOOL InstallApache();
    BOOL StartApache();
    int InstallJRE(void);
	int SetupBIConfig();
	int SetupCMConfig();
	int SetupTXNConfig();

private:
	void RemoveTempMSIAndCab();
    void AddComponent(UINT uiStrID, double size, BOOL selected, BOOL visible, LPCSTR path = 0);
    void SetTemp();
    void SetFileAssociations();
	void DeleteObsoleteFileAssociations();
    void SetEnvironmentVariables();
    void SetOneEnvironmentVariable(LPCSTR lpEnv, LPCSTR lpValue, BOOL bAdd = FALSE);
    BOOL AppendToFile(CString &file, LPSTR p);
    BOOL AppendToFile(CString &s, CString &value);
    void AppendComment(LPCSTR p);
    void AppendComment(UINT uiStrID);
};
