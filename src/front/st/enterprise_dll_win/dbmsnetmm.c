/*
**  Copyright (c) 2001-2009 Ingres Corporation.
*/

/*
**  Name: dbmsnetmm.c
**
**  Description:
**	This file becomes the DLL for the Ingres DBMS/NET Merge Module. It
**	contains custom actions that are required to be executed.
**
**	    ingres_set_dbmsnet_reg_entries	- Set the appropriate registry
**						  entries for the post install.
**	    ingres_start_ingres_service		- Start the Ingres service.
**	    ingres_remove_dbmsnet_files		- Removes directories created
**						  during the DBMS/NET post
**						  install.
**
**  History:
**	22-apr-2001 (somsa01)
**	    Created.
**	08-may-2001 (somsa01)
**	    In ingres_remove_dbmsnet_files(), corrected execution of
**	    directory removes. Also, changed name of function to
**	    ingres_set_dbmsnet_reg_entries(). In
**	    ingres_start_ingres_service(), if this is a modify operation,
**	    make sure the service is updated to start as the SYSTEM
**	    account for the post installation run (if we are doing a
**	    post installation).
**	22-may-2001 (somsa01)
**	    In ingres_set_dbmsnet_reg_entries(), clear the service
**	    password from the MSI property table, and only re-obtain
**	    the service info from the property table only if it has not
**	    been set yet in the registry.
**	18-jun-2001 (somsa01)
**	    In ingres_start_ingres_service(), remove the InstalledFeatures
**	    and RemovedFeatures registry values.
**	14-aug-2001 (somsa01)
**	    In ingres_set_dbmsnet_reg_entries(), we now shut down IVM as
**	    well.
**	20-aug-2001 (penga03)
**	    Take away the change 14-aug-2001 (somsa01). In DBMSNET Merge Module
**	    we have created a custom action to shut down IVM.
**	22-oct-2001 (penga03)
**	    Set registry key EmbeddedRelease if DBMS or Net is selected.
**	30-oct-2001 (penga03)
**	    Added ingres_delete_file_associations() to delete file associations for
**	    VDBA and IIA of the current installation, and if there exist other Ingres
**	    installations, restore VDBA and IIA file associations to point to one of
**	    the existing installations.
**	12-nov-2001 (penga03)
**	    Added a new property INGRES_SERVICE_STARTS_IIPOST, if it is set to 0,
**	    we start post installation process directly by CreateProcess, if it is
**	    set to 1, we first start Ingres service, then Ingres servie starts the
**	    post installation process.
**	10-dec-2001 (penga03)
**	    Added IngresAlreadyRunning to check if Ingres identified by II_SYSTEM is
**	    already running; and ingres_stop_ingres to stop Ingres if it is running.
**	12-dec-2001 (penga03)
**	    Changed INGRESFOLDER to INSTALLDIR.96721983_F981_11D4_819B_00C04F189633
**	    since the dll functions defined here are specific to IngresIIDBMSNet Merge
**	    Module.
**	17-jan-2002 (penga03)
**	    If embed install, that is INGRES_EMBEDDED is set to 1, shut down Ingres
**	    silently.
**	24-jan-2002 (penga03)
**	    If install Ingres using response file, set registry entry "SilentInstall"
**	    to "Yes", post installation process will be executed in silent mode.
**	28-jan-2002 (penga03)
**	    If this is a fresh install on Windows 9X, set registry entry "RebootNeeded"
**	    to "Yes", user will be asked to reboot when post installation completes.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage Ingres\\".
**	12-feb-2002 (penga03)
**	    Took the function ingres_set_timezone, as well as related function and structure,
**	    from setupmm.c.
**	14-feb-2002 (penga03)
**	    Added embedingres_cleanup_commit.
**	06-mar-2002 (penga03)
**	    Moved the function ExecuteEx() to commonmm.c.
**	04-apr-2002 (penga03)
**	    In the function ingres_set_timezone, added code to set
**	    the default value of Ingres charset to match the OS
**	    current code page.
**	14-may-2002 (penga03)
**	    Added ingres_set_shortcuts().
**	04-jun-2002 (penga03)
**	    Before removing the Ingres registry key, check if there
**	    are other products referencing the Ingres installation
**	    being removed, if so, leave it.
**	06-jun-2002 (penga03)
**	    Added a new property, INGRES_LEAVE_RUNNING
**	    (leaveingresrunninginstallincomplete in registry), to
**	    give user an option to determine whether leave Ingres
**	    running or not when post install process completes.
**	24-jul-2002 (penga03)
**	    Changed the Target of the Monitor Ingres Cache Statistics
**	    shortcut to %windir%\system32\perfmon.exe, and the Working
**	    Directory of the Ingres Command Prompt shortcut to be
**	    the user's home directory.
**	03-oct-2002 (penga03)
**	     Changed Ingres timezone "PRCGMT6" to "GMT6".
**	17-oct-2002 (penga03)
**	     For single byte build, the OS-charset to Ingres-charset
**	     table will only include those character sets available
**	     on single byte machines.
**	     Broke ingres_set_timezone into five functions,
**	     ingres_set_timezone_auto, ingres_set_timezone_manual,
**	     ingres_set_charset_auto, ingres_set_charset_manual and
**	     ingres_set_terminal_manual.
**	21-oct-2002 (penga03)
**	     In ingres_set_charset_manual(), added code to verify the
**	     Ingres character set got from Property table. The rule is
**	     INGRES_CHARSET can NOT be set to any following character
**	     sets, THAI, SHIFTJIS, CHINESES, KOREAN, CHTBIG5, that are
**	     only available on double byte Windows machines, if DBMSNet
**	     Merge Module being used is single byte version.
**	     Changed the name of searchByValue to searchIngTimezone and
**	     searchByValue2 to searchByIngCharset.
**	29-Oct-2002 (penga03)
**	    If install Ingres on double bytes Windows machines, the
**	    default Ingres time zone is always set to be
**	    "NA-EASTERN". This is because the time zone information
**	    got from local OS is localized, but the table that
**	    translates OS time zone to Ingres time zone is not. To
**	    solve this, we added a new field to the table. Now the
**	    table has following fields:
**	    os_tz1: the OS display name of the time zone, localized
**	            by OS
**	    os_tz2: the subkey name of the time zone in the
**	            Registry, NOT localized
**	    ingres_tz: the Ingres time zone name
**	    When translating OS time zone into Ingres time zone, we
**	    use os_tz2 as the key. The reason of keeping os_tz1 is
**	    we want to use it as a reference in case of the future
**	    change on the table.
**	20-may-2003 (penga03)
**	    In ingres_stop_ingres, added code to stop OpenROAD and Portal
**	    (if SDK is defined) before stop Ingres.
**	30-may-2003 (penga03)
**	    Corrected the message that tells users Ingres is running.
**	16-jun-2003 (penga03)
**	    Corrected embedingres_cleanup_commit and ingres_set_shortcuts,
**	    added ingres_check_installed_features.
**	26-jun-2003 (penga03)
**	    Changed ingres_set_dbmsnet_reg_entries() to create Ingres
**	    registry key only if the user chooses to install certain Ingres
**	    Ingres feature/features. Also verify the Ingres instance when
**	    given its II_INSTALLATION and INSTALLDIR.
**	    Changed ingres_start_ingres_service(), ingres_rollback_dbmsnet()
**	    and ingres_unset_dbmsnet_reg_entries() so that Ingres registry
**	    key will not be deleted if:
**	    (1) There exists Ingres feature(s) still installed;
**	    (2) The Ingres instance is not installed by this product;
**	    (3) There exists other products pointing to this Ingres instance.
**	06-aug-2003 (penga03)
**	    If OpenROAD is installed independently, restore OpenROAD
**	    shortcuts.
**	20-oct-2003 (penga03)
**	    Added embedingres_set_target_path(), embedingres_verify_instance()
**	    and embedingres_prepare_upgrade().
**	20-nov-2003 (penga03)
**	    Corrected Ingres time zone ROCKOREA to KOREA.
**	10-dec-2003 (penga03)
**	    Splited embedingres_verify_instance into
**	    embedingres_verify_instance_ui and embedingres_verify_instance.
**	26-jan-2003 (penga03)
**	    Added ingres_set_date_auto(), ingres_set_date_manual(),
**	    ingres_set_money_auto(), ingres_set_money_manual(),
**	    ingres_verify_money_format().
**	30-jan-2003 (penga03)
**	    Added Ingres .NET Data Provider feature.
**	04-feb-2004 (penga03)
**	    Added shortcuts for following new vdba files: vdbasql.exe,
**	    vdbamon.exe, vcda.exe, vdda.exe and iea.exe.
**	06-feb-2004 (penga03)
**	    Added three new doc shortcuts: ingnet, ipm and rs, replaced esqlc
**	    and esqlcob with esql.
**	06-feb-2004 (penga03)
**	    Added -loop for the iea shortcut.
**	06-feb-2004 (penga03)
**	    Deregistry ActiveX controls: ipm.ocx, sqlquery.ocx, vcda.ocx,
**	    vsda.ocx and svrsqlas.dll.
**	19-feb-2004 (penga03)
**	    Added ingres_remove_3rdparty_software, removing the 3rd partys'
**	    software installed by Ingres.
**	26-feb-2004 (penga03)
**	    In ingres_remove_3rdparty_software, do remove only if the last
**	    installation is being removed.
**	01-mar-2004 (penga03)
**	    Added a doc shortcut for quelref.pdf.
**	23-mar-2004 (penga03)
**	    Added a new property INGRES_START_IVM, to support for an option
**	    in the response file to prevent the installer starting IVM.
**	22-apr-2004 (penga03)
**	    Moved AskUserYN() to commonmm.c.
**	08-jun-2004 (penga03)
**	    Added ingres_create_rspfile and ingres_browseforfile.
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage".
**	04-aug-2004 (penga03)
**	    Corrected some errors after removing the references to
**	    "Advantage".
**	07-sep-2004 (penga03)
**	   Removed all references to service password since by default
**	   Ingres service is installed using Local System account.
**	10-sep-2004 (penga03)
**	   Updated the Ingres documentation shortcuts.
**	15-sep-2004 (penga03)
**	   Backed out the change made on 07-sep-2004 (471628).
**	16-sep-2004 (penga03)
**	   Added a new shortcut "Getting Started for Linux".
**	20-sep-2004 (penga03)
**	   Fix some logical error while handling time zone. "Display"
**	   has higher priority then "Std" name.
**	22-sep-2004 (penga03)
**	    Changed licloc to cdimage.
**	07-oct-2004 (penga03)
**	    Removed readme shortcut.
**	14-oct-2004 (penga03)
**	    Modified ingres\vdba to ingres\bin.
**	25-oct-2004 (penga03)
**	    The environment variable for money format should be
**	    II_MONEY_FORMAT.
**	03-nov-2004 (penga03)
**	    Added two registry entries: ingresprestartuprunprg,
**	    ingresprestartuprunparams.
**	29-oct-2004 (penga03)
**	   Modified ingres_upgrade_ver25 so that it removes the obsolete
**	   shortcuts that were installed in the current user's folder. For
**	   those shortcuts that were installed in the allusers' folder,
**	   Ingres merge modules will handle them.
**	11-nov-2004 (penga03)
**	    Added ingres_installfinalize().
**	12-nov-2004 (penga03)
**	    In ingres_installfinalize() remove the tailing back slash
**	    of %II_SYSTEM%.
**	30-nov-2004 (penga03)
**	    In ingres_installfinalize() added code to restore Ingres
**	    environment variables.
**	01-Dec-2004 (penga03)
**	    If Ingres charset is set to Japanese, set the font of
**	    "Ingres command prompt" to Raster.
**	07-dec-2004 (penga03)
**	    Changed the formulation to generate GUIDs since the old one does
**	    not work for id from A1 to A9.
**	14-dec-2004 (penga03)
**	    Added new mappings in charsets02.
**	14-dec-2004 (penga03)
**	    Backed out the changes made for bug 113452.
**	15-dec-2004 (penga03)
**	    Backed out the change to generate ProductCode. If the installation
**	    id is between A0-A9, use the new formulation.
**	15-dec-2004 (penga03)
**	    Added a command line argument, [INGRES_IVMFLAG], to IVM shortcut.
**	21-dec-2004 (penga03)
**	    In ingres_installfinalize() use _ftprintf instead of fprintf for
**	    II_SYSTEM and PATH, since there could be local characters in the
**	    II_SYSTEM.
**	21-dec-2004 (penga03)
**	    In ingres_installfinalize(), set the code page before setting II_SYSTEM.
**	11-jan-2005 (penga03)
**	    Set the registry entries if major upgrade.
**	11-jan-2005 (penga03)
**	    Added ingres_pass_properties().
**	11-jan-2005 (penga03)
**	    Added a registry entry "decimal".
**	26-jan-2005 (penga03)
**	    Bring the iipostinst window to the top of the Z order.
**	31-jan-2005 (penga03)
**	    Updated the names of those PDF files.
**	31-jan-2005 (penga03)
**	    Added ingres_set_terminal_auto and modified
**	    ingres_set_terminal_manual.
**	01-feb-2005 (penga03)
**	    In ingres_installfinalize(), change ing30 to ing302.
**	08-feb-2005 (penga03)
**	    Always set default Ingres terminal IBMPCD.
**	10-feb-2005 (penga03)
**	    Modified ingres_stop_ingres() so that Ingres can be shutted
**	    down silently if the user issue "/qn /x".
**	03-march-2005 (penga03)
**	    Added Ingres cluster support.
**	07-march-2005 (penga03)
**	    Corrected an error.
**	14-march-2005 (penga03)
**	    Added embedded documentation shortcuts.
**	14-march-2005 (penga03)
**	    In a cluster environment, shutdown Ingres by taking the resource
**	    offline instead of run ingstop.
**	15-march-2005 (penga03)
**	    Corrected some error when creating embedded doc shortcuts.
**	24-may-2005 (penga03)
**	    updated embedded doc shortcuts.
**	27-march-2005 (penga03)
**	    Corrected the table that mapps windows code pages to Ingres
**	    charsets.
**	21-jun-2005 (penga03)
**	    Added MOSCOW timezone.
**	30-jun-2005 (penga03)
**	    Added ingres_stop_ingres_ex to stop Ingres as necessary.
**	12-jul-2005 (penga03)
**	    In ingres_set_shortcuts() and embedingre_prepare_upgrade(),
**	    replaced SHGetSpecialFolderLocation with SHGetFolderLocation.
**	27-jul-2005 (penga03)
**	    Modified ingres_installfinalize() to repair the corrupt Ingres
**	    registry entry and service.
**	29-jul-2005 (penga03)
**	    In ingres_stop_ingres_ex, corrected the way to check if Ingres
**	    cluster is installed.
**	01-aug-2005 (penga03)
**	    Fixed the error while creating the ivm shortcut.
**	09-aug-2005 (penga03)
**	    Added P003341E.pdf (Ingres RMS Gateway User Guide).
**	31-aug-2005 (penga03)
**	    Added verification for timezone, terminal, date and money format.
**	21-oct-2005 (gupsh01)
**	    Added PC737, WIN1253 and ISO 88597 to the list of character
**	    sets supported.
**	13-dec-2005 (drivi01)
**	    SIR 115597
**	    Added 11 Moscow timezones.
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**	13-Feb-2006 (drivi01)
**	    Replaced all references to Ingres r3 with Ingres 2006.
**	    Updated renamed documentation guides.
**	28-Jun-2006 (drivi01)
**	    SIR 116381
**	    iipostinst.exe is renamed to ingconfig.exe. Fix all calls to
**	    iipostinst.exe to call ingconfig.exe instead.
**    	06-Sep-2006 (drivi01)
**          SIR 116599
**          As part of the initial changes for this SIR.  Modify Ingres
**          name to "Ingres II" instead of "Ingres [ II ]", Ingres location
**          to "C:\Program Files\Ingres\IngresII" instead of
**          C:\Program Files\Ingres\Ingres [ II ]. Shortcuts to "Ingres II"
**	    instead of "Ingres [ II ]", the same with titles and any .lnk
**	    files.  Generally got rid of square brackets.
**	04-Dec-2006 (drivi01)
**	    Added new property to the installer called INGRES_DATE_TYPE.
**	    Added routines for adding this value to the registry to be used
**	    by post-installer. In the upgrade scenario this value isn't set
**	    to the registry.
**	02-Jan-2007 (drivi01)
**	    Cleanup ansidate registry value if installation fails or is aborted.
**	20-Feb-2007 (karye01)
**	    Modified ingres_create_rspfile function to re-structure ingres response
**	    file, dividing response file into 4 sections, and changing names of 
**	    each property to a new names to bring all platforms in sync.
**	06-Mar-2007 (karye01)
**	    Removed response file entry II_ENABLE_DUAL_LOG as not needed. One parameter
**	    in Ingres Locations section will control dual log - II_DUAL_LOG - if this
**	    parameter is set to some location - dual log is enabled, if not - disabled.
**	13-Mar-2007 (drivi01)
**	    Put ansidate change back.
**	13-Mar-2007 (drivi01)
**	    Updated ingres_installfinalize function to include LIB and 
**	    INCLUDE variables in setingenvs.bat.
**	25-May-2007 (drivi01)
**	    Added new property INGRES_WAIT4GUI which will force setup.exe
**	    to wait for post installer to finish if this property is set to
**	    1.  The information needs to be passed to post installer via
**	    registry entry.
**	16-Jan-2009 (whiro01) SD issue 132899
**	    Create a new "perfmonwrap.bat" file in the post install step
**	    that will be invoked by the "Monitor Ingres Cache Statistics" menu item
**	    that sets up the environment, loads the perf DLL and the counters
**	    before invoking "perfmon" with the given parameter.
**	02-Aug-2010 (drivi01)
**	    HKEY_CLASSES_ROOT\\Ingres_Database_XX where XX is an instance id
**          is not being removed on uninstall.
**          Fix the installer to remove this key.
**	27-Aug-2010 (shust01)
**	    When creating a response file, the sql92 option (II_ENABLE_SQL92)
**	    was always being set to NO, no matter what the user selected
**	    for that option.  bug 124318.
**	27-Sep-2010 (shust01)
**	    When creating a response file and a 'traditional' installation 
**	    is chosen, that information does not appear in the response  
**	    file. bug 124503.
*/

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <MsiQuery.h>
#include "commonmm.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <time.h>
#include <tchar.h>
#include <locale.h>
#include <clusapi.h>
#include <malloc.h>

typedef struct tagENTRY
{
	char key[128];
	char value[128];
}ENTRY;

typedef struct tagTIMEZONE
{
    char DisplayName[128];
    char StdName[128];
    char ingres_tz[128];
}TIMEZONE;

static TIMEZONE timezones[]=
{
    {"(GMT) Casablanca, Monrovia", "GMT Standard Time", "GMT"},
    {"(GMT) Casablanca, Monrovia", "Greenwich Standard Time", "GMT"},
    {"(GMT) Casablanca, Monrovia", "Greenwich", "GMT"},
    {"(GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London", "GMT Standard Time", "UNITED-KINGDOM"},
    {"(GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London", "GMT", "UNITED-KINGDOM"},
    {"(GMT+01:00)  Amsterdam, Copenhagen, Madrid, Paris, Vilnius", "Romance Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna", "W. Europe Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna", "W. Europe", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague", "Central Europe Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague", "Central Europe", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Belgrade, Sarajevo, Skopje, Sofija, Zagreb", "Central European Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Bratislava, Budapest, Ljubljana, Prague, Warsaw", "Central Europe Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Brussels, Berlin, Bern, Rome, Stockholm, Vienna", "W. Europe Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris", "Romance Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris", "Romance", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris, Vilnius", "Romance", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Sarajevo, Skopje, Sofija, Vilnius, Warsaw, Zagreb", "Central European", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Sarajevo, Skopje, Sofija, Warsaw, Zagreb", "Central European", "EUROPE-CENTRAL"},
    {"(GMT+01:00) West Central Africa", "W. Central Africa Standard Time", "GMT1"},
    {"(GMT+01:00) West Central Africa", "W. Central Africa", "GMT1"},
    {"(GMT+02:00) Athens, Istanbul, Minsk", "GFT Standard Time", "EUROPE-EASTERN"},
    {"(GMT+02:00) Athens, Istanbul, Minsk", "GTB Standard Time", "EUROPE-EASTERN"},
    {"(GMT+02:00) Athens, Istanbul, Minsk", "GTB", "EUROPE-EASTERN"},
    {"(GMT+02:00) Bucharest", "E. Europe Standard Time", "EUROPE-EASTERN"},
    {"(GMT+02:00) Bucharest", "E. Europe", "EUROPE-EASTERN"},
    {"(GMT+02:00) Cairo", "Egypt Standard Time", "EGYPT"},
    {"(GMT+02:00) Cairo", "Egypt", "EGYPT"},
    {"(GMT+02:00) Harare, Pretoria", "South Africa Standard Time", "GMT2"},
    {"(GMT+02:00) Harare, Pretoria", "South Africa", "GMT2"},
    {"(GMT+02:00) Helsinki, Riga, Tallinn", "FLE Standard Time", "EUROPE-EASTERN"},
    {"(GMT+02:00) Helsinki, Riga, Tallinn", "FLE", "EUROPE-EASTERN"},
    {"(GMT+02:00) Israel", "Israel Standard Time", "ISRAEL"},
    {"(GMT+02:00) Jerusalem", "Israel Standard Time", "ISRAEL"},
    {"(GMT+02:00) Jerusalem", "Israel", "ISRAEL"},
    {"(GMT+02:00) Kaliningrad", "Kaliningrad Standard Time", "MOSCOW-1"},
    {"(GMT+02:00) Kaliningrad", "Kaliningrad", "MOSCOW-1"},
    {"(GMT+03:00) Baghdad", "Arabic Standard Time", "GMT3"},
    {"(GMT+03:00) Baghdad", "Arabic", "GMT3"},
    {"(GMT+03:00) Baghdad, Kuwait, Riyadh", "Arab", "GMT3"},
    {"(GMT+03:00) Baghdad, Kuwait, Riyadh", "Saudi Arabia Standard Time", "GMT3"},
    {"(GMT+03:00) Kuwait, Riyadh", "Arab Standard Time", "SAUDI-ARABIA"},
    {"(GMT+03:00) Kuwait, Riyadh", "Arab", "SAUDI-ARABIA"},
    {"(GMT+03:00) Moscow, St. Petersburg, Volgograd", "Russian Standard Time", "MOSCOW"},
    {"(GMT+03:00) Moscow, St. Petersburg, Volgograd", "Russian", "MOSCOW"},
    {"(GMT+03:00) Nairobi", "E. Africa Standard Time", "GMT3"},
    {"(GMT+03:00) Nairobi", "E. Africa", "GMT3"},
    {"(GMT+03:30) Tehran", "Iran Standard Time", "IRAN"},
    {"(GMT+03:30) Tehran", "Iran", "IRAN"},
    {"(GMT+04:00) Abu Dhabi, Muscat", "Arabian Standard Time", "GMT4"},
    {"(GMT+04:00) Abu Dhabi, Muscat", "Arabian", "GMT4"},
    {"(GMT+04:00) Baku, Tbilisi", "Caucasus Standard Time", "GMT4"},
    {"(GMT+04:00) Baku, Tbilisi", "Caucasus", "GMT4"},
    {"(GMT+04:00) Baku, Tbilisi, Yerevan", "Caucasus Standard Time", "GMT4"},
    {"(GMT+04:00) Baku, Tbilisi, Yerevan", "Caucasus", "GMT4"},
    {"(GMT+04:00) Samara, Saratov, Ul'yanovsk", "Samara Standard Time", "MOSCOW1"},
    {"(GMT+04:00) Samara, Saratov, Ul'yanovsk", "Samara", "MOSCOW1"},
    {"(GMT+04:30) Kabul", "Afghanistan Standard Time", ""},
    {"(GMT+04:30) Kabul", "Afghanistan", ""},
    {"(GMT+05:00) Ekaterinburg", "Ekaterinburg Standard Time", "GMT5"},
    {"(GMT+05:00) Ekaterinburg", "Ekaterinburg", "GMT5"},
    {"(GMT+05:00) Islamabad, Karachi, Tashkent", "West Asia Standard Time", "PAKISTAN"},
    {"(GMT+05:00) Islamabad, Karachi, Tashkent", "West Asia", "PAKISTAN"},
    {"(GMT+05:00) Yekaterinburg, Sverdlovsk, Chelyabinsk", "Yekaterinburg Standard Time", "MOSCOW2"},
    {"(GMT+05:00) Yekaterinburg, Sverdlovsk, Chelyabinsk", "Yekaterinburg", "MOSCOW2"},
    {"(GMT+05:30) Bombay, Calcutta, Madras, New Delhi", "India Standard Time", "INDIA"},
    {"(GMT+05:30) Bombay, Calcutta, Madras, New Delhi", "India", "INDIA"},
    {"(GMT+05:30) Calcutta, Chennai, Mumbai, New Delhi", "India", "INDIA"},
    {"(GMT+05:45) Kathmandu", "Nepal Standard Time", ""},
    {"(GMT+05:45) Kathmandu", "Nepal", ""},
    {"(GMT+06:00) Almaty, Dhaka", "Central Asia Standard Time", "GMT6"},
    {"(GMT+06:00) Almaty, Novosibirsk", "N. Central Asia Standard Time", "GMT6"},
    {"(GMT+06:00) Almaty, Novosibirsk", "N. Central Asia", "GMT6"},
    {"(GMT+06:00) Astana, Almaty, Dhaka", "Central Asia", "GMT6"},
    {"(GMT+06:00) Astana, Dhaka", "Central Asia Standard Time", "GMT6"},
    {"(GMT+06:00) Astana, Dhaka", "Central Asia", "GMT6"},
    {"(GMT+06:00) Colombo", "Sri Lanka Standard Time", "GMT6"},
    {"(GMT+06:00) Colombo", "Sri Lanka", "GMT6"},
    {"(GMT+06:00) Omsk, Novosibirsk", "Omsk/Novosibirsk Standard Time", "MOSCOW3"},
    {"(GMT+06:00) Omsk, Novosibirsk", "Omsk/Novosibirsk", "MOSCOW3"},
    {"(GMT+06:00) Sri Jayawardenepura", "Sri Lanka Standard Time", "GMT6"},
    {"(GMT+06:00) Sri Jayawardenepura", "Sri Lanka", "GMT6"},
    {"(GMT+06:30) Rangoon", "Myanmar Standard Time", ""},
    {"(GMT+06:30) Rangoon", "Myanmar", ""},
    {"(GMT+07:00) Bangkok, Hanoi, Jakarta", "Bangkok Standard Time", "INDONESIA-WEST"},
    {"(GMT+07:00) Bangkok, Hanoi, Jakarta", "SE Asia Standard Time", "INDONESIA-WEST"},
    {"(GMT+07:00) Bangkok, Hanoi, Jakarta", "SE Asia", "INDONESIA-WEST"},
    {"(GMT+07:00) Krasnoyarsk", "North Asia Standard Time", "GMT7"},
    {"(GMT+07:00) Krasnoyarsk", "North Asia", "GMT7"},
    {"(GMT+07:00) Krasnoyarsk", "Krasnoyarsk Standard Time", "MOSCOW4"},
    {"(GMT+07:00) Krasnoyarsk", "Krasnoyarsk", "MOSCOW4"},
    {"(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi", "China Standard Time", "HONG-KONG"},
    {"(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi", "China", "HONG-KONG"},
    {"(GMT+08:00) Irkutsk, Ulaan Bataar", "North Asia East Standard Time", "GMT8"},
    {"(GMT+08:00) Irkutsk, Ulaan Bataar", "North Asia East", "GMT8"},
    {"(GMT+08:00) Irkutsk, Ulaan Bataar", "Irkutsk Standard Time", "MOSCOW5"},
    {"(GMT+08:00) Irkutsk, Ulaan Bataar", "Irkutsk", "MOSCOW5"},
    {"(GMT+08:00) Kuala Lumpur, Singapore", "Singapore Standard Time", "MALAYSIA"},
    {"(GMT+08:00) Kuala Lumpur, Singapore", "Singapore", "MALAYSIA"},
    {"(GMT+08:00) Perth", "W. Australia Standard Time", "AUSTRALIA-WEST"},
    {"(GMT+08:00) Perth", "W. Australia", "AUSTRALIA-WEST"},
    {"(GMT+08:00) Singapore", "Singapore Standard Time", "SINGAPORE"},
    {"(GMT+08:00) Singapore", "Singapore", "SINGAPORE"},
    {"(GMT+08:00) Taipei", "Taipei Standard Time", "GMT8"},
    {"(GMT+08:00) Taipei", "Taipei", "GMT8"},
    {"(GMT+09:00) Osaka, Sapporo, Tokyo", "Tokyo Standard Time", "JAPAN"},
    {"(GMT+09:00) Osaka, Sapporo, Tokyo", "Tokyo", "JAPAN"},
    {"(GMT+09:00) Seoul", "Korea Standard Time", "KOREA"},
    {"(GMT+09:00) Seoul", "Korea", "KOREA"},
    {"(GMT+09:00) Yakutsk", "Yakutsk Standard Time", "GMT9"},
    {"(GMT+09:00) Yakutsk", "Yakutsk", "GMT9"},
    {"(GMT+09:00) Yakutsk", "Yakutsk Standard Time", "MOSCOW6"},
    {"(GMT+09:00) Yakutsk", "Yakutsk", "MOSCOW6"},
    {"(GMT+09:30) Adelaide", "Cen. Australia Standard Time", "AUSTRALIA-SOUTH"},
    {"(GMT+09:30) Adelaide", "Cen. Australia", "AUSTRALIA-SOUTH"},
    {"(GMT+09:30) Darwin", "AUS Central Standard Time", "AUSTRALIA-NORTH"},
    {"(GMT+09:30) Darwin", "AUS Central", "AUSTRALIA-NORTH"},
    {"(GMT+10:00) Brisbane", "E. Australia Standard Time", "AUSTRALIA-QUEENSLAND"},
    {"(GMT+10:00) Brisbane", "E. Australia", "AUSTRALIA-QUEENSLAND"},
    {"(GMT+10:00) Canberra, Melbourne, Sydney (Year 2000 only)", "Syd2000", "AUSTRALIA-NSW"},
    {"(GMT+10:00) Canberra, Melbourne, Sydney", "AUS Eastern Standard Time", "AUSTRALIA-NSW"},
    {"(GMT+10:00) Canberra, Melbourne, Sydney", "AUS Eastern", "AUSTRALIA-NSW"},
    {"(GMT+10:00) Canberra, Melbourne, Sydney", "Sydney Standard Time", "AUSTRALIA-NSW"},
    {"(GMT+10:00) Guam, Port Moresby", "West Pacific Standard Time", "GMT10"},
    {"(GMT+10:00) Guam, Port Moresby", "West Pacific", "GMT10"},
    {"(GMT+10:00) Hobart", "Tasmania Standard Time", "AUSTRALIA-TASMANIA"},
    {"(GMT+10:00) Hobart", "Tasmania", "AUSTRALIA-TASMANIA"},
    {"(GMT+10:00) Vladivostok", "Vladivostok Standard Time", "GMT10"},
    {"(GMT+10:00) Vladivostok", "Vladivostok", "GMT10"},
    {"(GMT+10:00) Vladivostok", "Vladivostok Standard Time", "MOSCOW7"},
    {"(GMT+10:00) Vladivostok", "Vladivostok", "MOSCOW7"},
    {"(GMT+11:00) Magadan, Solomon Is., New Caledonia", "Central Pacific Standard Time", "GMT11"},
    {"(GMT+11:00) Magadan, Solomon Is., New Caledonia", "Central Pacific", "GMT11"},
    {"(GMT+11:00) Magadan, Sakha, Solomon Is.", "Magadan Standard Time", "MOSCOW8"},
    {"(GMT+11:00) Magadan, Sakha, Solomon Is.", "Magadan", "MOSCOW8"},
    {"(GMT+12:00) Auckland, Wellington", "New Zealand Standard Time", "NEW-ZEALAND"},
    {"(GMT+12:00) Auckland, Wellington", "New Zealand", "NEW-ZEALAND"},
    {"(GMT+12:00) Fiji, Kamchatka, Marshall Is.", "Fiji Standard Time", "GMT12"},
    {"(GMT+12:00) Fiji, Kamchatka, Marshall Is.", "Fiji", "GMT12"},
    {"(GMT+12:00) Kamchatka, Chukot", "Anadyr Standard Time", "MOSCOW9"},
    {"(GMT+12:00) Kamchatka, Chukot", "Anadyr", "MOSCOW9"},
    {"(GMT+13:00) Nuku'alofa", "Tonga Standard Time", "GMT13"},
    {"(GMT+13:00) Nuku'alofa", "Tonga", "GMT13"},
    {"(GMT-01:00) Azores", "Azores Standard Time", "GMT-1"},
    {"(GMT-01:00) Azores", "Azores", "GMT-1"},
    {"(GMT-01:00) Azores, Cape Verde Is.", "Azores Standard Time", "GMT-1"},
    {"(GMT-01:00) Azores, Cape Verde Is.", "Azores", "GMT-1"},
    {"(GMT-01:00) Cape Verde Is.", "Cape Verde Standard Time", "GMT-1"},
    {"(GMT-01:00) Cape Verde Is.", "Cape Verde", "GMT-1"},
    {"(GMT-02:00) Mid-Atlantic", "Mid-Atlantic Standard Time", "GMT-2"},
    {"(GMT-02:00) Mid-Atlantic", "Mid-Atlantic", "GMT-2"},
    {"(GMT-03:00) Brasilia", "E. South America Standard Time", "BRAZIL-EAST"},
    {"(GMT-03:00) Brasilia", "E. South America", "BRAZIL-EAST"},
    {"(GMT-03:00) Buenos Aires, Georgetown", "SA Eastern Standard Time", "GMT-3"},
    {"(GMT-03:00) Buenos Aires, Georgetown", "SA Eastern", "GMT-3"},
    {"(GMT-03:00) Greenland", "Greenland Standard Time", "GMT-3"},
    {"(GMT-03:00) Greenland", "Greenland", "GMT-3"},
    {"(GMT-03:30) Newfoundland", "Newfoundland Standard Time", "CANADA-NEWFOUNDLAND"},
    {"(GMT-03:30) Newfoundland", "Newfoundland", "CANADA-NEWFOUNDLAND"},
    {"(GMT-04:00) Atlantic Time (Canada)", "Atlantic Standard Time", "CANADA-ATLANTIC"},
    {"(GMT-04:00) Atlantic Time (Canada)", "Atlantic", "CANADA-ATLANTIC"},
    {"(GMT-04:00) Caracas, La Paz", "SA Western Standard Time", "GMT-4"},
    {"(GMT-04:00) Caracas, La Paz", "SA Western", "GMT-4"},
    {"(GMT-04:00) Santiago", "Pacific SA Standard Time", "CHILE-CONTINENTAL"},
    {"(GMT-04:00) Santiago", "Pacific SA", "CHILE-CONTINENTAL"},
    {"(GMT-05:00) Bogota, Lima, Quito", "SA Pacific Standard Time", "GMT-5"},
    {"(GMT-05:00) Bogota, Lima, Quito", "SA Pacific", "GMT-5"},
    {"(GMT-05:00) Eastern Time (US & Canada)", "Eastern Standard Time", "NA-EASTERN"},
    {"(GMT-05:00) Eastern Time (US & Canada)", "Eastern", "NA-EASTERN"},
    {"(GMT-05:00) Indiana (East)", "US Eastern Standard Time", "NA-EASTERN"},
    {"(GMT-05:00) Indiana (East)", "US Eastern", "NA-EASTERN"},
    {"(GMT-06:00) Central America", "Central America Standard Time", "GMT-6"},
    {"(GMT-06:00) Central America", "Central America", "GMT-6"},
    {"(GMT-06:00) Central Time (US & Canada)", "Central Standard Time", "NA-CENTRAL"},
    {"(GMT-06:00) Central Time (US & Canada)", "Central", "NA-CENTRAL"},
    {"(GMT-06:00) Mexico City", "Mexico", "MEXICO-GENERAL"},
    {"(GMT-06:00) Mexico City, Tegucigalpa", "Mexico Standard Time", "GMT-6"},
    {"(GMT-06:00) Mexico City, Tegucigalpa", "Mexico", "GMT-6"},
    {"(GMT-06:00) Saskatchewan", "Canada Central Standard Time", "NA-CENTRAL"},
    {"(GMT-06:00) Saskatchewan", "Canada Central", "NA-CENTRAL"},
    {"(GMT-07:00) Arizona", "US Mountain Standard Time", "NA-MOUNTAIN"},
    {"(GMT-07:00) Arizona", "US Mountain", "NA-MOUNTAIN"},
    {"(GMT-07:00) Mountain Time (US & Canada)", "Mountain Standard Time", "NA-MOUNTAIN"},
    {"(GMT-07:00) Mountain Time (US & Canada)", "Mountain", "NA-MOUNTAIN"},
    {"(GMT-08:00) Pacific Time (US & Canada); Tijuana", "Pacific Standard Time", "NA-PACIFIC"},
    {"(GMT-08:00) Pacific Time (US & Canada); Tijuana", "Pacific", "NA-PACIFIC"},
    {"(GMT-09:00) Alaska", "Alaskan Standard Time", "US-ALASKA"},
    {"(GMT-09:00) Alaska", "Alaskan", "US-ALASKA"},
    {"(GMT-10:00) Hawaii", "Hawaiian Standard Time", "US-HAWAII"},
    {"(GMT-10:00) Hawaii", "Hawaiian", "US-HAWAII"},
    {"(GMT-11:00) Midway Island, Samoa", "Samoa Standard Time", "GMT-11"},
    {"(GMT-11:00) Midway Island, Samoa", "Samoa", "GMT-11"},
    {"(GMT-12:00) Eniwetok, Kwajalein", "Dateline Standard Time", "GMT-12"},
    {"(GMT-12:00) Eniwetok, Kwajalein", "Dateline", "GMT-12"},
    {0, 0, 0}
};

static ENTRY charsets[]=
{
    {"737", "PC737"},
    {"874", "WTHAI"},
    {"932", "SHIFTJIS"},
    {"936", "CSGBK"},
    {"949", "KOREAN"},
    {"950", "CHTBIG5"},
    {"1200", "WIN1252"},
    {"1250", "WIN1250"},
    {"1251", "CW"},
    {"1252", "WIN1252"},
    {"1253", "WIN1253"},
    {"1254", "ISO88599"},
    {"1255", "WHEBREW"},
    {"1256", "WARABIC"},
    {"1257", "WIN1252"},
    {"1258", "WIN1252"},
    {0,0}
};

static ENTRY charsets02[]=
{
    {"THAI", "874"},
    {"WTHAI", "874"},
    {"SHIFTJIS", "932"},
    {"KANJIEUC", "932"},
    {"CHINESES", "936"},
    {"CSGBK", "936"},
    {"CSGB2312", "936"},
    {"KOREAN", "949"},
    {"CHTBIG5", "950"},
    {"CHTEUC", "950"},
    {"CHTHP", "950"},
    {"UCS2", "1200"},
    {"UCS4", "1200"},
    {"UTF7", "1200"},
    {"UTF8", "1200"},
    {"IBMPC850", "850"},
    {"WIN1250", "1250"},
    {"IBMPC866", "1251"},
    {"ALT", "1251"},
    {"CW", "1251"},
    {"WIN1252", "1252"},
    {"GREEK", "1253"},
    {"ELOT437", "1253"},
    {"PC857", "1254"},
    {"HEBREW", "1255"},
    {"WHEBREW", "1255"},
    {"PCHEBREW", "1255"},
    {"ARABIC", "1256"},
    {"WARABIC", "1256"},
    {"DOSASMO", "1256"},
    {"WIN1252", "1257"},
    {"WIN1253", "1253"},
    {"IS885915", "28605"},
    {"ISO88591", "28591"},
    {"ISO88592", "28592"},
    {"ISO88595", "28595"},
    {"ISO88597", "28597"},
    {"ISO88599", "28599"},
    {"KOI8", "20866"},
    {"SLAV852", "852"},
    {"IBMPC437", "437"},
    {0,0}
};

static ENTRY dateformats[]=
{
    {"yyyy/MM/dd", "SEWDEN"},
    {"yy/MM/dd", "ISO"},
    {"yyyy-MM-dd", "SEWDEN"},
    {"yy-MM-dd", "ISO"},
    {"dd/MM/yyyy", "MULTINATIONAL"},
    {"dd/MM/yy", "MULTINATIONAL"},
    {"M/d/yyyy", "MDY"},
    {"M/d/yy", "MDY"},
    {"MM/dd/yy", "US"},
    {"MM/dd/yyyy", "MDY"},
    {"dd-MM-yyyy", "MULTINATIONAL"},
    {"dd-MM-yy", "MULTINATIONAL"},
    {"dd.MM.yyyy", "GERMAN"},
    {"dd.MM.yy", "GERMAN"},
    {"d/MM/yyyy", "MULTINATIONAL"},
    {"d/MMM/yyyy", "DMY"},
    {"dd/MMM/yyyy", "DMY"},
    {"d.M.yy", "GERMAN"},
    {"yyyy/M/d", "SWEDEN"},
    {"yyyy-M-d", "SWEDEN"},
    {"yy-M-d", "SWEDEN"},
    {"d/M/yyyy", "DMY"},
    {"d/M/yy", "MULTINATIONAL"},
    {"yy/M/d", "ISO"},
    {"d.M.yyyy", "GERMAN"},
    {"d. M. yyyy", "GERMAN"},
    {"d. M. yy", "GERMAN"},
    {"dd. MM. yy", "GERMAN"},
    {"d-M-yy", "MULTINATIONAL"},
    {"yyyy MM dd", "SWEDEN"},
    {"d-M-yyyy", "MULTINATIONAL"},
    {"d/MM/yy", "MULTINATIONAL"},
    {"dd-MMM-yy", "DMY"},
    {"dd-MMMM-yyyy", "DMY"},
    {"M/dd/yy", "MDY"},
    {"d.MM.yy", "GERMAN"},
    {"d.MM.yyyy", "GERMAN"},
    {"yy MM dd", "ISO"},
    {"dd. M. yy", "GERMAN"},
    {"dd.M.yyyy", "GERMAN"},
    {"dd/MMMM/yyyy", "DMY"},
    {"dd/MMM/yy", "DMY"},
    {"yyyy. MM. dd.",	"SWEDEN"},
    {"dd.M.yy", "GERMAN"},
    {"yyyy.MM.dd.", "SWEDEN"},
    {"yy.MM.dd.", "ISO"},
    {"yyyy.MM.dd", "SWEDEN"},
    {"yy.MM.dd", "ISO"},
    {"MM-dd-yyyy", "MDY"},
    {"d MMM yyyy", "DMY"},
    {"dd MMM yyyy", "DMY"},
    {"dd/MM yyyy", "DMY"},
    {0,0}
};

BOOL
searchIngTimezone(char *os_tz, char *ing_tz)
{
    int total, i;

    total=sizeof(timezones)/sizeof(TIMEZONE);
    for(i=0; i<total; i++)
    {
	if (!_stricmp(os_tz, timezones[i].DisplayName) && _stricmp(timezones[i].ingres_tz, ""))
	{
	    strcpy(ing_tz, timezones[i].ingres_tz);
	    return (TRUE);
	}
	if (!_stricmp(os_tz, timezones[i].StdName) && _stricmp(timezones[i].ingres_tz, ""))
	{
	    strcpy(ing_tz, timezones[i].ingres_tz);
	    return (TRUE);
	}
    }
    return (FALSE);
}

BOOL
searchIngCharset(char *k, char *v)
{
    BOOL bret=FALSE;
    int total, i;

    total=sizeof(charsets)/sizeof(ENTRY);
    for(i=0; i<total; i++)
    {
	if (!strcmp(k, charsets[i].key))
	{
	    strcpy(v, charsets[i].value);
	    bret=TRUE;
	    return bret;
	}
    }
    return bret;
}

BOOL
searchIngDateFormat(char *k, char *v)
{
    BOOL bret=FALSE;
    int total, i;

    total=sizeof(dateformats)/sizeof(ENTRY);
    for(i=0; i<total; i++)
    {
	if (!strcmp(k, dateformats[i].key))
	{
	    strcpy(v, dateformats[i].value);
	    bret=TRUE;
	    return bret;
	}
    }
    return bret;
}

/*
**  Name: IngresAlreadyRunning
**
**  Description:
**	Check if Ingres identified by II_SYSTEM is already running.
**
**  Returns:
**	 0	Ingres is not running
**	 1	Ingres is running
**
**  History:
**	10-dec-2001 (penga03)
**	    Created.
*/
int
IngresAlreadyRunning(char *installpath)
{
	char file2check[MAX_PATH], temp[MAX_PATH];

	sprintf(file2check, "%s\\ingres\\files\\config.dat", installpath);
	if (GetFileAttributes(file2check) == -1)
		return 0;

	sprintf(file2check, "%s\\ingres\\bin\\iigcn.exe", installpath);
	if (GetFileAttributes(file2check) != -1)
	{
		sprintf(temp, "%s\\ingres\\bin\\temp.exe", installpath);
		CopyFile(file2check, temp, FALSE);
		if (DeleteFile(file2check))
		{
			/* iigcn is not running */
			CopyFile(temp, file2check, FALSE);
			DeleteFile(temp);
			return 0;
		}
		else
		{
			/* iigcn is running */
			DeleteFile(temp);
			return 1;
		}
	}
	else
	{
		/* iigcn.exe does not exist.*/
		return 0;
	}
}


/*{
**  Name: ingres_set_dbmsnet_reg_entries
**
**  Description:
**	Sets the appropriate registry entries for the DBMS/NET Merge Module.
**	These entries will drive he post installation for the DBMS/NET.
**
**  Inputs:
**	ihnd		MSI environment handle.
**
**  Outputs:
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**	22-apr-2001 (somsa01)
**	    Created.
**	22-may-2001 (somsa01)
**	    Clear the service password from the MSI property table, and
**	    only re-obtain the service info from the property table only
**	    if it has not been set yet in the registry.
**	23-July-2001 (penga03)
**	    If the date of installation installed by older installer is
**	    later than the last-write time of iiname.all, during upgrade,
**	    window installer won't overwrite iiname.all, so changed its
**	    last-write time the same as its creation time.
**	07-Aug-2001 (penga03)
**	    If made some changes on protocols, set dbmsnet registry entries.
**	14-aug-2001 (somsa01)
**	    We now shut down IVM as well.
**	12-sep-2001 (penga03)
**	    Set the two keys installationcode and II_SYSTEM no matter what.
**	12-dec-2001 (penga03)
**	    Set INGRES_II_SYSTEM (i.e. INSTALLDIR without tailing back slash)
**	    for IngreIIDBMSNet Merge Module to set proper environment variables.
**	14-jan-2002 (penga03)
**	    If install Ingres using response file, set registry entry "SilentInstall"
**	    to "Yes", post installation process will be executed in silent mode.
**	14-jan-2002 (penga03)
**	    Set property INGRES_POSTINSTALLATIONNEEDED to "1" if post installation
**	    is needed.
**	26-jun-2003 (penga03)
**	    Made following changes:
**	    Create Ingres registry key only if the user chooses to install some
**	    Ingres feature/features.
**	    Verify the Ingres instance when given its II_INSTALLATION and INSTALLDIR
**	    (i.e. II_SYSTEM).
**	16-Nov-2006 (drivi01)
**	    Added new registry entries for post-installer (ingconfig) to use.
**	    Most of these entries represent newly added functionlity to installer.
**	    1) includeinpath tells configuration utility if ingres will be
**	       added to the path.
**	    2) installdemodb tells configuration utility if demodb needs to be
**	       created and populated.
**	    3) expressinstall tells configuration utility what mode installer is
**	       running in. This is relevant for summary information.
**	    4) upgradedatabases tells configuration utility if user databases need
**	       to be upgraded. Not new functionality but we never asked this so
**	       early in install.
**	    5) On upgrade serviceauto is set to whatever it was set during
**	       original install.  By default in new installs serviceauto is always
**	       set.
**	    Added new function ingres_remove_from_path.
**	13-Mar-2007 (drivi01)
**	    Fixed check for existing service.  The code was always falling though
**	    and trying to perform upgrade on service.
**	10-May-2007 (drivi01)
**	    Bug 118303
**	    Do not install demodb on upgrade.
**	    If upgrade is detected set installdemodb registry entry to 0.
**	04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    Update routines to be able to handle DBA Tools install.
**	    Add a dbatools registry entry to distinguish regular
**	    Ingres install from DBA Tools install and compensate for
**	    the renaming of the service in DBA Tools.
**	02-Jul-2009 (drivi01)
**	    Add ingconfigtype registry value that is retrieved from
**	    CONFIG_TYPE property.
*/

UINT __stdcall
ingres_set_dbmsnet_reg_entries (MSIHANDLE ihnd)
{
    HKEY		hKey;
    char		KeyName[256], oldKeyName[256];
    int			status = 0;
    DWORD		dwSize, dwType, dwDisposition;
    char		tchValue[MAX_PATH], tchII_INSTALLATION[3];
    INSTALLSTATE	iInstalledDBMS, iActionDBMS;
    INSTALLSTATE	iInstalledNET, iActionNET;
    char		buf[MAX_PATH+1];
    DWORD		nret;
    HANDLE		hFile;
    FILETIME		ftCreate;
    INSTALLSTATE	iInstalledTCPIP, iActionTCPIP, iInstalledBIOS;
    INSTALLSTATE	iActionBIOS, iInstalledSPX, iActionSPX;
    INSTALLSTATE	iInstalledDECNET, iActionDECNET;
    BOOL		protocolsChanged;
    BOOL		bInstalled, bVer25, bVersion9X;
    INSTALLSTATE iInstalled, iAction;
    BOOL bCreate=FALSE;
    BOOL bSpecialUpgrade=FALSE;
    BOOL bIngresUpgrade=FALSE;
    BOOL isDBATools=FALSE;
    protocolsChanged=FALSE;
    bInstalled=bVer25=bVersion9X=FALSE;

	nret=sizeof(buf);
	if (!MsiGetProperty(ihnd, "DBATools", buf, &nret) && strcmp(buf, "1")==0)
		isDBATools=TRUE;

    nret=sizeof(buf);
    if(MsiGetTargetPath(ihnd, "INSTALLDIR.96721983_F981_11D4_819B_00C04F189633", buf, &nret)==ERROR_SUCCESS ||
		(isDBATools && MsiGetTargetPath(ihnd, "INSTALLDIR", buf, &nret)==ERROR_SUCCESS ))
    {
	if(buf[strlen(buf)-1]=='\\')
	    buf[strlen(buf)-1]='\0';

	MsiSetProperty(ihnd, "INGRES_II_SYSTEM", buf);

	strcat(buf, "\\ingres\\files\\name\\iiname.all");
	hFile=CreateFile(
			buf,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	if(hFile!=INVALID_HANDLE_VALUE)
	{
	    GetFileTime(hFile, &ftCreate, NULL, NULL);
	    SetFileTime(hFile, &ftCreate, &ftCreate, &ftCreate);
	    CloseHandle(hFile);
	}
    }

    bCreate=FALSE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresDBMS", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresIce", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresStar", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresReplicator", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresNet", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresEsqlC", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresEsqlCobol", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresEsqlFortran", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresTools", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresVision", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresDotNETDataProvider", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;
    if (!bCreate
    && !MsiGetFeatureState(ihnd, "IngresDoc", &iInstalled, &iAction)
    && (iAction==3)) bCreate=TRUE;

    dwSize = sizeof(tchValue);
    if (MsiGetProperty( ihnd, "II_INSTALLATION", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	strcpy(tchII_INSTALLATION, tchValue);
	sprintf(KeyName,
		"Software\\IngresCorporation\\Ingres\\%s_Installation",
		tchValue);
	if (bCreate)
	{
	    status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, NULL,
	             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
				 &hKey, &dwDisposition);
	}
	else
	{
	    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS, &hKey))
		return ERROR_SUCCESS;
	}
	status = RegSetValueEx( hKey, "installationcode", 0, REG_SZ,
	         tchValue, strlen(tchValue) + 1 );
	}

    /** This is special case for Upgrade from Ingres r3 to Ingres 2006 where registry is being moved */
    if (!MsiGetProperty(ihnd, "INGRES_UPGRADE", tchValue, &dwSize) && (!_stricmp(tchValue, "1") || !_stricmp(tchValue, "2")))
    {
	    bIngresUpgrade = TRUE;
	    if (RegQueryValueEx(hKey, "II_SYSTEM", NULL, &dwType, tchValue, &dwSize))
	    {
		    RegCloseKey(hKey);
		    sprintf(oldKeyName, "Software\\ComputerAssociates\\Ingres\\%s_Installation",
		    tchII_INSTALLATION);
		    if( !RegOpenKeyEx(HKEY_LOCAL_MACHINE, oldKeyName, 0, KEY_ALL_ACCESS, &hKey)
				&& !RegQueryValueEx(hKey, "II_SYSTEM", NULL, NULL, NULL, NULL) )
			    bSpecialUpgrade = TRUE;
		    RegCloseKey(hKey);
		    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
		        return (ERROR_INSTALL_FAILURE);
	    }

    }

    dwSize = sizeof(tchValue);
	if (isDBATools && MsiGetTargetPath(ihnd, "INSTALLDIR", tchValue, &dwSize) == ERROR_SUCCESS )
	{
		if (tchValue[strlen(tchValue)-1] == '\\')
			tchValue[strlen(tchValue)-1] = '\0';
		status = RegSetValueEx( hKey, "II_SYSTEM", 0, REG_SZ,
			tchValue, strlen(tchValue) + 1 );
	}
	if (!isDBATools)
	{
    if (status || 
	MsiGetTargetPath(ihnd, "INSTALLDIR.96721983_F981_11D4_819B_00C04F189633", tchValue,
			 &dwSize) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else if (bCreate || bSpecialUpgrade)
    {
	if (tchValue[strlen(tchValue)-1] == '\\')
	    tchValue[strlen(tchValue)-1] = '\0';

	/*if (!VerifyInstance(ihnd, tchII_INSTALLATION, tchValue))
	{
	    return (ERROR_INSTALL_FAILURE);
	}*/

	status = RegSetValueEx( hKey, "II_SYSTEM", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
	}
	}
	

    MsiGetFeatureState(ihnd, "IngresDBMS", &iInstalledDBMS, &iActionDBMS);
    MsiGetFeatureState(ihnd, "IngresNet", &iInstalledNET, &iActionNET);
    MsiGetFeatureState(ihnd, "IngresTCPIP", &iInstalledTCPIP, &iActionTCPIP);
    MsiGetFeatureState(ihnd, "IngresNetBIOS", &iInstalledBIOS, &iActionBIOS);
    MsiGetFeatureState(ihnd, "IngresNovellSPXIPX", &iInstalledSPX, &iActionSPX);
    MsiGetFeatureState(ihnd, "IngresDECNet", &iInstalledDECNET, &iActionDECNET);

    if ((iInstalledTCPIP != INSTALLSTATE_LOCAL &&
	 iActionTCPIP == INSTALLSTATE_LOCAL) ||
	(iInstalledTCPIP == INSTALLSTATE_LOCAL &&
	 iActionTCPIP == INSTALLSTATE_ABSENT) ||
	(iInstalledBIOS != INSTALLSTATE_LOCAL &&
	 iActionBIOS == INSTALLSTATE_LOCAL) ||
	(iInstalledBIOS == INSTALLSTATE_LOCAL &&
	 iActionBIOS == INSTALLSTATE_ABSENT) ||
	(iInstalledSPX != INSTALLSTATE_LOCAL &&
	 iActionSPX == INSTALLSTATE_LOCAL) ||
	(iInstalledSPX == INSTALLSTATE_LOCAL &&
	 iActionSPX == INSTALLSTATE_ABSENT) ||
	(iInstalledDECNET != INSTALLSTATE_LOCAL &&
	 iActionDECNET == INSTALLSTATE_LOCAL) ||
	(iInstalledDECNET == INSTALLSTATE_LOCAL &&
	 iActionDECNET == INSTALLSTATE_ABSENT))
	{
	    protocolsChanged=TRUE;
	}


    if ((iInstalledDBMS == INSTALLSTATE_LOCAL ||
	 iActionDBMS != INSTALLSTATE_LOCAL) &&
	((iActionNET == INSTALLSTATE_ABSENT) ||
	(iInstalledNET == INSTALLSTATE_LOCAL && !protocolsChanged) ||
	(iInstalledNET != INSTALLSTATE_LOCAL &&
	 iActionNET != INSTALLSTATE_LOCAL)) )
    {
	/* Neither DBMS or NET are not being installed. */
	dwSize = sizeof(tchValue);
	if (!MsiGetProperty(ihnd, "INGRES_UPGRADE", tchValue, &dwSize) && strcmp(tchValue, "2"))
	{
	    return (ERROR_SUCCESS);
	}
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_LANGUAGE", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "language", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_TIMEZONE", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "timezone", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_TERMINAL", tchValue,
			&dwSize) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "terminal", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_CHARSET", tchValue,
			&dwSize) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "charset", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_INCLUDEINPATH", tchValue,
	   	        &dwSize) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (bIngresUpgrade)
		sprintf(tchValue, "%s", "0");
	status = RegSetValueEx( hKey, "includeinpath", 0, REG_SZ,
			        tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
        MsiGetProperty( ihnd, "INGRES_INSTALLDEMODB", tchValue,
	   	        &dwSize) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (!bIngresUpgrade)
		status = RegSetValueEx( hKey, "installdemodb", 0, REG_SZ,
			        tchValue, strlen(tchValue) + 1 );
	else
    		status = RegSetValueEx(hKey, "installdemodb", 0, REG_SZ, 
				"0", strlen(tchValue) + 1 );
    }
    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_INSTALL_MODE", tchValue,
			&dwSize) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "expressinstall", 0, REG_SZ,
				tchValue, strlen(tchValue)+1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_UPGRADE_USERDB", tchValue,
			&dwSize) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
		status = RegSetValueEx( hKey, "upgradeuserdatabases", 0, REG_SZ,
				tchValue, strlen(tchValue)+1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_DATE_FORMAT", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "dateformat", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
 	MsiGetProperty( ihnd, "INGRES_DATE_TYPE", tchValue, 
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (!bIngresUpgrade)
	{
	if (strcmp(tchValue, "ANSI") == 0)
	     status = RegSetValueEx( hKey, "ansidate", 0, REG_SZ,
				"1", strlen(tchValue) + 1 );
	else
	     status = RegSetValueEx( hKey, "ansidate", 0, REG_SZ,
				"0", strlen(tchValue) + 1 );	
	}	
    }

    dwSize = sizeof(tchValue);
    if (status || 
	MsiGetProperty( ihnd, "INGRES_WAIT4GUI", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey,
				"waitforpostinstall", 
				0, REG_SZ, (CONST BYTE *)&tchValue, 
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_MONEY_FORMAT", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "moneyformat", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_DECIMAL", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
    }
    else
    {
	status = RegSetValueEx( hKey, "decimal", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_EMBEDDED", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "EmbeddedRelease", 0, REG_SZ,
				(CONST BYTE *)&tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_LEAVE_RUNNING", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
    }
    else
    {
	status = RegSetValueEx( hKey,
				"leaveingresrunninginstallincomplete",
				0, REG_SZ, (CONST BYTE *)&tchValue,
				strlen(tchValue) + 1 );

    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_START_IVM", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey,
				"startivm",
				0, REG_SZ, (CONST BYTE *)&tchValue,
				strlen(tchValue) + 1 );

    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_PRESTARTUPRUNPRG", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey,
				"ingresprestartuprunprg",
				0, REG_SZ, (CONST BYTE *)&tchValue,
				strlen(tchValue) + 1 );

    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_PRESTARTUPRUNPARAMS", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey,
				"ingresprestartuprunparams",
				0, REG_SZ, (CONST BYTE *)&tchValue,
				strlen(tchValue) + 1 );

    }
    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "DBATools", tchValue, 
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey, "dbatools", 0, REG_SZ, 
				(CONST BYTE *)&tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "CONFIG_TYPE", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey, "ingconfigtype", 0, REG_SZ, 
				(CONST BYTE *)&tchValue,
				strlen(tchValue) + 1 );
    }

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "PostInstallationNeeded", NULL, &dwType,
			tchValue, &dwSize) != ERROR_SUCCESS)
    {
	dwSize = sizeof(tchValue);
	if (status ||
	    MsiGetProperty( ihnd, "INGRES_SERVICEAUTO", tchValue,
			    &dwSize ) != ERROR_SUCCESS)
	{
	    return (ERROR_INSTALL_FAILURE);
	}
	else
	{
	    char szBuf[MAX_PATH+1];
	    if (isDBATools)
	    sprintf(szBuf, "Ingres_DBATools_%s", tchII_INSTALLATION);
	    else
	    sprintf(szBuf, "Ingres_Database_%s", tchII_INSTALLATION);
	    if (CheckServiceExists(szBuf))
	    {
	         if (IsServiceStartupTypeAuto(szBuf))
		    sprintf(tchValue, "1");
		 else
		    sprintf(tchValue, "0");

		 status = RegSetValueEx( hKey, "serviceauto", 0, REG_SZ,
				    tchValue, strlen(tchValue) + 1 );

	    }
	    else
	    	 status = RegSetValueEx( hKey, "serviceauto", 0, REG_SZ,
				    tchValue, strlen(tchValue) + 1 );
	}

	dwSize = sizeof(tchValue);
	if (status ||
	    MsiGetProperty( ihnd, "INGRES_SERVICELOGINID", tchValue,
			    &dwSize ) != ERROR_SUCCESS)
	{
	    return (ERROR_INSTALL_FAILURE);
	}
	else
	{
	    status = RegSetValueEx( hKey, "serviceloginid", 0, REG_SZ,
				    tchValue, strlen(tchValue) + 1 );
	}

	dwSize = sizeof(tchValue);
	if (status ||
	    MsiGetProperty( ihnd, "INGRES_SERVICEPASSWORD", tchValue,
			    &dwSize ) != ERROR_SUCCESS)
	{
	    return (ERROR_INSTALL_FAILURE);
	}
	else
	{
	    /* Clear the password from the property table. */
	    MsiSetProperty(ihnd, "INGRES_SERVICEPASSWORD", "");
	    MsiSetProperty(ihnd, "INGRES_SERVICEPASSWORD2", "");

	    status = RegSetValueEx( hKey, "servicepassword", 0, REG_SZ,
				    tchValue, strlen(tchValue) + 1 );
	}

	strcpy(tchValue, "YES");
	status = RegSetValueEx( hKey, "PostInstallationNeeded", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
	MsiSetProperty(ihnd, "INGRES_POSTINSTALLATIONNEEDED", "1");

	dwSize = sizeof(tchValue);
	MsiGetProperty(ihnd, "INGRES_RSP_LOC", tchValue, &dwSize);
	if (tchValue[0] && strcmp(tchValue, "0"))
	{
	    strcpy(tchValue, "YES");
	    RegSetValueEx( hKey, "SilentInstall", 0, REG_SZ, (CONST BYTE *)&tchValue, strlen(tchValue) + 1 );
	}

	dwSize = sizeof(tchValue);
	MsiGetProperty(ihnd, "Installed", tchValue, &dwSize);
	if (tchValue[0])
	    bInstalled=TRUE;
	dwSize = sizeof(tchValue);
	MsiGetProperty(ihnd, "INGRES_VER25", tchValue, &dwSize);
	if (tchValue[0] && !strcmp(tchValue, "1"))
	    bVer25=0;
	dwSize=sizeof(tchValue);
	MsiGetProperty(ihnd, "Version9X", tchValue, &dwSize);
	if (tchValue[0])
	    bVersion9X=TRUE;
	if (!bInstalled && !bVer25 && bVersion9X)
	{
	    strcpy(tchValue, "YES");
	    RegSetValueEx( hKey, "RebootNeeded", 0, REG_SZ, (CONST BYTE *)&tchValue, strlen(tchValue) + 1 );
	}
	}
    return (status == ERROR_SUCCESS ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE);
}

/*{
**  Name: ingres_start_ingres_service
**
**  Description:
**	Starts the Ingres service.
**
**  Inputs:
**	ihnd		MSI environment handle.
**
**  Outputs:
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**	22-apr-2001 (somsa01)
**	    Created.
**	15-may-2001 (somsa01)
**	    If this is a modify operation, make sure the service
**	    is updated to start as the SYSTEM account for the post
**	    installation run (if we are doing a post installation).
**	18-jun-2001 (somsa01)
**	    Remove the InstalledFeatures and RemovedFeatures registry
**	    values.
**	03-Aug-2001 (penga03)
**	    Remove the remove_ingresodbc, remove_ingresodbcreadonly,
**	    setup_ingresodbc, and setup_ingresodbcreadonly registry
**	    value.
**	03-Aug-2001 (penga03)
**	    Remove the registry key if we are removing the installation.
**	22-Aug-2001 (penga03)
**	   Don't remove the remove_ingresodbc, remove_ingresodbcreadonly,
**	   setup_ingresodbc, and setup_ingresodbcreadonly registry
**	   value if this is a fresh install.
**	11-Sep-2001 (penga03)
**	   Start post-installation proccess directly.
**	16-nov-2001 (penga03)
**	   Donnot have to remove odbc related registry values since we
**	   donnot set them any more.
**	28-jan-2002 (penga03)
**	   Moved the code to delete the InstalledFeatures and RemovedFeatures
**	   registry entries to dll functions called ingres_cleanup_commit, and
**	   ingres_cleanup_immediate in setupmm.c.
**	26-jun-2003 (penga03)
**	   Do not delete Ingres registry key if:
**	   There exists Ingres feature(s) still installed;
**	   The Ingres instance is not installed by this product;
**	   There exists other products pointing to this Ingres instance.
**	04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    Update routines to be able to handle DBA Tools install.
**	    Compensate for the renaming of the service in DBA Tools
**	    and recognize the change in title in ingconfig when installing
**	    DBA Tools.
**	04-Apr-2010 (drivi01)
**	    Add additional check to make sure DBATools property query returned
**          something.
*/

UINT __stdcall
ingres_start_ingres_service (MSIHANDLE ihnd)
{
    char		KeyName[256];
    HKEY		hKey;
    DWORD		dwSize, dwType;
    char		tchValue[MAX_PATH+1], tchII_INSTALLATION[3];
    char		ServiceName[255], szBuf[MAX_PATH];
    SERVICE_STATUS	ssServiceStatus;
    LPSERVICE_STATUS	lpssServiceStatus = &ssServiceStatus;
    SC_HANDLE		schSCManager, OpIngSvcHandle;
    BOOL		bServiceStartsIipost, bAdminUser, bVersionNT;
    PROCESS_INFORMATION	pi;
    STARTUPINFO		si;
    HWND hWnd;
	BOOL isDBATools=FALSE;

	bServiceStartsIipost=bAdminUser=bVersionNT=FALSE;
	dwSize = sizeof(tchValue);
	if ((MsiGetProperty( ihnd, "DBATools", tchValue, &dwSize)==ERROR_SUCCESS) && tchValue[0])
		isDBATools=TRUE;

    dwSize = sizeof(tchII_INSTALLATION);
    if (MsiGetProperty( ihnd, "II_INSTALLATION", tchII_INSTALLATION,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }

    sprintf(KeyName,
	    "Software\\IngresCorporation\\Ingres\\%s_Installation",
	    tchII_INSTALLATION);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS,
		     &hKey)) return ERROR_SUCCESS;

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "PostInstallationNeeded", NULL, &dwType,
			tchValue, &dwSize) != ERROR_SUCCESS)
    {
	char szComponentGUID[39], szProductGUID[39];
	INSTALLSTATE iInstalled, iAction;
	BOOL bCreate, bInstanceExist;
	char ii_system[MAX_PATH];

	RegDeleteValue(hKey, "InstalledFeatures");
	RegDeleteValue(hKey, "RemovedFeatures");
	RegCloseKey(hKey);

	bCreate=FALSE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresDBMS", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresIce", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresStar", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresReplicator", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresNet", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresEsqlC", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresEsqlCobol", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresEsqlFortran", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresTools", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresVision", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresDotNETDataProvider", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(ihnd, "IngresDoc", &iInstalled, &iAction)
	&& (iInstalled==3 && iAction!=2 || iAction==3)) bCreate=TRUE;

	bInstanceExist=FALSE;
	if (GetInstallPath(tchII_INSTALLATION, ii_system))
	{
	    sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
	    if (GetFileAttributes(szBuf)!=-1) bInstanceExist=TRUE;
	}

	if (!_stricmp(tchII_INSTALLATION, "II"))
	    strcpy(szComponentGUID, "{844FEE64-249D-11D5-BDF5-00B0D0AD4485}");
	else
	{
	    int idx;

	    idx = (toupper(tchII_INSTALLATION[0]) - 'A') * 26 + toupper(tchII_INSTALLATION[1]) - 'A';
	    if (idx <= 0)
		idx = (toupper(tchII_INSTALLATION[0]) - 'A') * 26 + toupper(tchII_INSTALLATION[1]) - '0';
	    sprintf(szComponentGUID, "{844FEE64-249D-11D5-%04X-%012X}", idx, idx*idx);
	}

	dwSize=sizeof(szProductGUID);
	MsiGetProperty(ihnd, "ProductCode", szProductGUID, &dwSize);

	if (GetRefCount(szComponentGUID, szProductGUID)==0
	&& !bCreate && !bInstanceExist)
	{
	    RegDeleteKey(HKEY_LOCAL_MACHINE, KeyName);
	    sprintf(KeyName, "SOFTWARE\\IngresCorporation\\Ingres");
	    SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, KeyName);
	    sprintf(KeyName, "SOFTWARE\\IngresCorporation");
	    SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, KeyName);
	}

	return (ERROR_SUCCESS);
    }
    RegCloseKey(hKey);

	dwSize=sizeof(tchValue);
	if (MsiGetProperty(ihnd, "AdminUser", tchValue, &dwSize)==ERROR_SUCCESS
		&& !strcmp(tchValue, "1"))
	{
		bAdminUser=TRUE;
	}

	dwSize=sizeof(tchValue);
	if (MsiGetProperty(ihnd, "VersionNT", tchValue, &dwSize)==ERROR_SUCCESS
		&& tchValue[0])
	{
		bVersionNT=TRUE;
	}

	dwSize=sizeof(tchValue);
	if (MsiGetProperty(ihnd, "INGRES_SERVICE_STARTS_IIPOST", tchValue, &dwSize)==ERROR_SUCCESS
		&& !strcmp(tchValue, "1") && bVersionNT && bAdminUser)
	{
		bServiceStartsIipost=TRUE;
	}

	dwSize=sizeof(tchValue);
	if(!isDBATools && MsiGetTargetPath(ihnd, "INSTALLDIR.96721983_F981_11D4_819B_00C04F189633", tchValue, &dwSize)!=ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;
	dwSize=sizeof(tchValue);
	if (isDBATools && MsiGetTargetPath(ihnd, "INSTALLDIR", tchValue, &dwSize)!=ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;
	if(tchValue[strlen(tchValue)-1]=='\\')
		tchValue[strlen(tchValue)-1]='\0';

	if(!bServiceStartsIipost)
	{
		/* Start post-install directly */
		memset((char*)&pi, 0, sizeof(pi));
		memset((char*)&si, 0, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWNORMAL;
		sprintf(szBuf, "\"%s\\ingres\\bin\\ingconfig.exe\"", tchValue);
		if (CreateProcess(NULL, szBuf, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
		{
			Sleep(1000);
			sprintf(szBuf, "Ingres %s Configuration", tchII_INSTALLATION);
			if (isDBATools)
				sprintf(szBuf, "Ingres DBA Tools %s Configuration", tchII_INSTALLATION);
			hWnd=FindWindow(NULL, szBuf);
			if (hWnd)
				BringWindowToTop(hWnd);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return (ERROR_SUCCESS);
		}
		else
		{
			sprintf(szBuf, "Failed to execute \"%s\\ingres\\bin\\ingconfig.exe\"", tchValue);
			MyMessageBox(ihnd, szBuf);
			return (ERROR_INSTALL_FAILURE);
		}
	}

	/* Start post-install from Ingres service */
	if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT)) != NULL)
	{
		if (isDBATools)
		     sprintf(ServiceName, "Ingres_DBATools_%s", tchII_INSTALLATION);
		else
		     sprintf(ServiceName, "Ingres_Database_%s", tchII_INSTALLATION);
		sprintf(szBuf, "\"%s\\ingres\\bin\\servproc.exe\"", tchValue);

		if ((OpIngSvcHandle = OpenService(schSCManager, ServiceName,
			SERVICE_CHANGE_CONFIG |
			SERVICE_START)) != NULL)
		{
			/* Make sure we're set to start as the SYSTEM account.*/
			if (!ChangeServiceConfig(OpIngSvcHandle,
				SERVICE_WIN32_OWN_PROCESS |
				SERVICE_INTERACTIVE_PROCESS,
				SERVICE_DEMAND_START,
				SERVICE_NO_CHANGE,
				szBuf,
				NULL,
				NULL,
				NULL,
				"LocalSystem",
				"",
				NULL))
			{
				CloseServiceHandle(OpIngSvcHandle);
				CloseServiceHandle(schSCManager);
				return (ERROR_INSTALL_FAILURE);
			}

			if (!StartService(OpIngSvcHandle, 0, NULL))
			{
				CloseServiceHandle(OpIngSvcHandle);
				CloseServiceHandle(schSCManager);
				return (ERROR_INSTALL_FAILURE);
			}

			CloseServiceHandle(OpIngSvcHandle);
		}
		else
		{
			CloseServiceHandle(schSCManager);
			return (ERROR_INSTALL_FAILURE);
		}

		CloseServiceHandle(schSCManager);
    }
    else
		return (ERROR_INSTALL_FAILURE);

    return (ERROR_SUCCESS);

}

/*{
**  Name: ingres_remove_dbmsnet_files
**
**  Description:
**	Removes any files/directories created by the DBMS/NET Merge Module
**	when it was installed, as well as its specific registry entries.
**
**  Inputs:
**	ihnd		MSI environment handle.
**
**  Outputs:
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**	22-apr-2001 (somsa01)
**	    Created.
**	08-may-2001 (somsa01)
**	    Corrected execution of directory removes. Also, grab
**	    II_INSTALLATION via the CustomActionData property.
**	23-July-2001 (penga03)
**	    Clean up those files created during post installation.
**	03-Aug-2001 (penga03)
**	    Let the DBMDNET Merge Module delete the files/directories created
**	    by the post installation process.
**	03-Aug-2001 (penga03)
**	    Don't delete the registry entries for this installation, in case
**	    of rollback.
**	01-nov-2001 (penga03)
**	    If both DBMS and NET are being removed, deregistry ActiveX controls.
*/

UINT __stdcall
ingres_remove_dbmsnet_files (MSIHANDLE ihnd)
{
    char		KeyName[256];
    HKEY		hKey;
    DWORD		status = 0;
    DWORD		dwSize, dwType;
    TCHAR		tchValue[2048], tchInstalled[2048];
    BOOL		NETInstalled=FALSE, DBMSInstalled=FALSE;
	BOOL		DeRegActiveX=FALSE;
	char		path[MAX_PATH], szBuf[MAX_PATH];

    dwSize = sizeof(tchValue);
    if (MsiGetProperty( ihnd, "CustomActionData", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }

    sprintf(KeyName,
	    "Software\\IngresCorporation\\Ingres\\%s_Installation",
	    tchValue);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS,
			  &hKey)) return ERROR_SUCCESS;

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    RegQueryValueEx(hKey, "InstalledFeatures", NULL, &dwType,
		    tchInstalled, &dwSize);

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "Ingres DBMS Server", NULL, &dwType,
			tchValue, &dwSize) == ERROR_SUCCESS ||
	strstr(tchInstalled, "DBMS"))
    {
	DBMSInstalled = TRUE;
    }

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "Ingres/Net", NULL, &dwType,
			tchValue, &dwSize) == ERROR_SUCCESS ||
	strstr(tchInstalled, "NET"))
    {
	NETInstalled = TRUE;
    }

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "RemovedFeatures", NULL, &dwType,
			tchValue, &dwSize) != ERROR_SUCCESS)
    {
	/* Nothing is being removed. */
	RegCloseKey(hKey);
	return (ERROR_SUCCESS);
    }

    if ((strstr(tchValue, "NET") && !DBMSInstalled) ||
	(!NETInstalled && strstr(tchValue, "DBMS")) ||
	(strstr(tchValue, "NET") && strstr(tchValue, "DBMS")))
    {
	/*
	** This is one of three things:
	**  1. NET is being removed and DBMS is not installed.
	**  2. NET is not installed and DBMS is being removed.
	**  3. NET and DBMS are being removed.
	**
	** In either of these cases, we want to go through here
	** because we will be doing a final cleanup.
	*/
	DeRegActiveX=TRUE;
	}
    else
    {
	RegCloseKey(hKey);
	return(ERROR_SUCCESS);
    }

    /*
    ** First, let's delete all directories that would have been created
    ** outside of InstallShield.
    */
    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "II_SYSTEM", NULL, &dwType, tchValue, &dwSize) !=
	ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }

    SetEnvironmentVariable("II_SYSTEM", tchValue);
	GetEnvironmentVariable("path", path, sizeof(path));
	sprintf(szBuf, "%s\\ingres\\bin;%s\\ingres\\utility;%s", tchValue, tchValue, path);
	SetEnvironmentVariable("PATH", szBuf);

    /* Deregistry ActiveX controls */
    if(DeRegActiveX)
    {
	sprintf(szBuf, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\IJACTRL.OCX\"", tchValue);
	Execute(szBuf, TRUE);
	sprintf(szBuf, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\SVRIIA.DLL\"", tchValue);
	Execute(szBuf, TRUE);
	sprintf(szBuf, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\ipm.ocx\"", tchValue);
	Execute(szBuf, TRUE);
	sprintf(szBuf, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\sqlquery.ocx\"", tchValue);
	Execute(szBuf, TRUE);
	sprintf(szBuf, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\vcda.ocx\"", tchValue);
	Execute(szBuf, TRUE);
	sprintf(szBuf, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\vsda.ocx\"", tchValue);
	Execute(szBuf, TRUE);
	sprintf(szBuf, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\svrsqlas.dll\"", tchValue);
	Execute(szBuf, TRUE);
    }

    /*
    ** Finally, remove the registry entries for this installation.
    */
    RegCloseKey(hKey);

    return ERROR_SUCCESS;
}

/*
**  Name:ingres_rollback_dbmsnet
**
**  Description:
**	Reverse the changes to the system made by ingres_set_dbmsnet_reg_entries and
**	ingres_remove_dbmsnet_files. Executed only during an installation rollback.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS			The function succeeds.
**	    ERROR_INSTALL_FAILURE	The function fails.
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	03-Aug-2001 (penga03)
**	    Created.
**	21-Aug-2001 (penga03)
**	    Use the property Installed to determine if we are doing upgrade or
**	    fresh install.
**	01-nov-2001 (penga03)
**	    Reregistry ActiveX controls.
**	26-jun-2003 (penga03)
**	   Do not delete Ingres registry key if:
**	   There exists Ingres feature(s) still installed;
**	   The Ingres instance is not installed by this product;
**	   There exists other products pointing to this Ingres instance.
**
*/
UINT __stdcall
ingres_rollback_dbmsnet(MSIHANDLE hInstall)
{
	char ii_code[3], installed[256], cadata[1024], temp[1024], *p, *q;
	DWORD ret;
	char RegKey[256];
	DWORD type, size;
	HKEY hkRegKey;
	char  ii_system[MAX_PATH], path[MAX_PATH], szBuf[MAX_PATH];
	char szComponentGUID[39], szProductGUID[39];
	INSTALLSTATE iInstalled, iAction;
	BOOL bCreate, bInstanceExist;

	ret=sizeof(cadata);
	if(MsiGetProperty(hInstall, "CustomActionData", cadata, &ret)!=ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;

	strcpy(temp, cadata);
	p=strchr(temp, ';');
	q=p+1;
	*p=0;
	strcpy(ii_code, temp);
	while(*q!=0 && !isalnum(*q))
		q=q+1;
	strcpy(installed, q);

	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", ii_code);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
		return ERROR_SUCCESS;

	RegDeleteValue(hkRegKey, "language");
	RegDeleteValue(hkRegKey, "timezone");
	RegDeleteValue(hkRegKey, "terminal");
	RegDeleteValue(hkRegKey, "charset");
	RegDeleteValue(hkRegKey, "dateformat");
	RegDeleteValue(hkRegKey, "moneyformat");
	RegDeleteValue(hkRegKey, "decimal");
	RegDeleteValue(hkRegKey, "EmbeddedRelease");
	RegDeleteValue(hkRegKey, "leaveingresrunninginstallincomplete");
	RegDeleteValue(hkRegKey, "ingresprestartuprunprg");
	RegDeleteValue(hkRegKey, "ingresprestartuprunparams");
	RegDeleteValue(hkRegKey, "startivm");
	RegDeleteValue(hkRegKey, "serviceauto");
	RegDeleteValue(hkRegKey, "serviceloginid");
	RegDeleteValue(hkRegKey, "servicepassword");
	RegDeleteValue(hkRegKey, "PostInstallationNeeded");
	RegDeleteValue(hkRegKey, "SilentInstall");
	RegDeleteValue(hkRegKey, "RebootNeeded");
	RegDeleteValue(hkRegKey, "includeinpath");
	RegDeleteValue(hkRegKey, "installdemodb");
	RegDeleteValue(hkRegKey, "expressinstall");
	RegDeleteValue(hkRegKey, "upgradeuserdatabases");
	RegDeleteValue(hkRegKey, "ansidate");
	RegDeleteValue(hkRegKey, "waitforpostinstall");

	/* Reregistry ActiveX controls */
	size=sizeof(ii_system);

	if(RegQueryValueEx(hkRegKey, "II_SYSTEM", NULL, &type, (BYTE *)ii_system, &size)==ERROR_SUCCESS)
	{
		SetEnvironmentVariable("II_SYSTEM", ii_system);
		GetEnvironmentVariable("path", path, sizeof(path));
		sprintf(szBuf, "%s\\ingres\\bin;%s\\ingres\\utility;%s", ii_system, ii_system, path);
		SetEnvironmentVariable("PATH", szBuf);

		sprintf(szBuf,  "%s\\ingres\\bin\\vdba.exe", ii_system);
		if(GetFileAttributes(szBuf) != -1)
		{
			sprintf(szBuf, "REGSVR32.EXE /s \"%s\\ingres\\bin\\IJACTRL.OCX\"", ii_system);
			Execute(szBuf, TRUE);
			sprintf(szBuf, "REGSVR32.EXE /s \"%s\\ingres\\bin\\SVRIIA.DLL\"", ii_system);
			Execute(szBuf, TRUE);
			sprintf(szBuf, "REGSVR32.EXE /s \"%s\\ingres\\bin\\ipm.ocx\"", ii_system);
			Execute(szBuf, TRUE);
			sprintf(szBuf, "REGSVR32.EXE /s \"%s\\ingres\\bin\\sqlquery.ocx\"", ii_system);
			Execute(szBuf, TRUE);
			sprintf(szBuf, "REGSVR32.EXE /s \"%s\\ingres\\bin\\vcda.ocx\"", ii_system);
			Execute(szBuf, TRUE);
			sprintf(szBuf, "REGSVR32.EXE /s \"%s\\ingres\\bin\\vsda.ocx\"", ii_system);
			Execute(szBuf, TRUE);
			sprintf(szBuf, "REGSVR32.EXE /s \"%s\\ingres\\bin\\svrsqlas.dll\"", ii_system);
			Execute(szBuf, TRUE);
		}
	}

	if(installed[0])
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}

	bCreate=FALSE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresDBMS", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresIce", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresStar", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresReplicator", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresNet", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresEsqlC", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresEsqlCobol", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresEsqlFortran", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresTools", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresVision", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresDotNETDataProvider", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresDoc", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;

	bInstanceExist=FALSE;
	if (GetInstallPath(ii_code, ii_system))
	{
	    sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
	    if (GetFileAttributes(szBuf)!=-1) bInstanceExist=TRUE;
	}

	if (!_stricmp(ii_code, "II"))
	    strcpy(szComponentGUID, "{844FEE64-249D-11D5-BDF5-00B0D0AD4485}");
	else
	{
	    int idx;

	    idx = (toupper(ii_code[0]) - 'A') * 26 + toupper(ii_code[1]) - 'A';
	    if (idx <= 0)
		idx = (toupper(ii_code[0]) - 'A') * 26 + toupper(ii_code[1]) - '0';

	    sprintf(szComponentGUID, "{844FEE64-249D-11D5-%04X-%012X}", idx, idx*idx);
	}

	ret=sizeof(szProductGUID);
	MsiGetProperty(hInstall, "ProductCode", szProductGUID, &ret);

	if(GetRefCount(szComponentGUID, szProductGUID)>=1
	|| bCreate || bInstanceExist)
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}

	RegDeleteValue(hkRegKey, "installationcode");
	RegDeleteValue(hkRegKey, "II_SYSTEM");
	RegCloseKey(hkRegKey);
	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", ii_code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\IngresCorporation\\Ingres");
	SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\IngresCorporation");
	SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, RegKey);
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_unset_dbmsnet_reg_entries
**
**  Description:
**	Reverse the changes to the system made by ingres_set_dbmsnet_reg_entries
**	during windows installer is creating script.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS			The function succeeds.
**	    ERROR_INSTALL_FAILURE	The function fails.
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	21-Aug-2001 (penga03)
**	    Created.
**	21-mar-2002 (penga03)
**	    Changed the function always return ERROR_SUCCESS since
**	    installation already completes at this point.
**	26-jun-2003 (penga03)
**	   Do not delete Ingres registry key if:
**	   There exists Ingres feature(s) still installed;
**	   The Ingres instance is not installed by this product;
**	   There exists other products pointing to this Ingres instance.
*/
UINT __stdcall
ingres_unset_dbmsnet_reg_entries(MSIHANDLE hInstall)
{
	char ii_code[3], buf[256];
	DWORD ret;
	char RegKey[256];
	HKEY hkRegKey;
	char szComponentGUID[39], szProductGUID[39];
	char ii_system[MAX_PATH], szBuf[MAX_PATH];
	INSTALLSTATE iInstalled, iAction;
	BOOL bCreate, bInstanceExist;

	ret=sizeof(ii_code);
	if(MsiGetProperty(hInstall, "II_INSTALLATION", ii_code, &ret)!=ERROR_SUCCESS)
		return ERROR_SUCCESS;

	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", ii_code);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
		return ERROR_SUCCESS;

	RegDeleteValue(hkRegKey, "language");
	RegDeleteValue(hkRegKey, "timezone");
	RegDeleteValue(hkRegKey, "terminal");
	RegDeleteValue(hkRegKey, "charset");
	RegDeleteValue(hkRegKey, "dateformat");
	RegDeleteValue(hkRegKey, "moneyformat");
	RegDeleteValue(hkRegKey, "decimal");
	RegDeleteValue(hkRegKey, "EmbeddedRelease");
	RegDeleteValue(hkRegKey, "leaveingresrunninginstallincomplete");
	RegDeleteValue(hkRegKey, "ingresprestartuprunprg");
	RegDeleteValue(hkRegKey, "ingresprestartuprunparams");
	RegDeleteValue(hkRegKey, "startivm");
	RegDeleteValue(hkRegKey, "serviceauto");
	RegDeleteValue(hkRegKey, "serviceloginid");
	RegDeleteValue(hkRegKey, "servicepassword");
	RegDeleteValue(hkRegKey, "PostInstallationNeeded");
	RegDeleteValue(hkRegKey, "SilentInstall");
	RegDeleteValue(hkRegKey, "RebootNeeded");
	RegDeleteValue(hkRegKey, "includeinpath");
	RegDeleteValue(hkRegKey, "installdemodb");
	RegDeleteValue(hkRegKey, "expressinstall");
	RegDeleteValue(hkRegKey, "upgradeuserdatabases");
	RegDeleteValue(hkRegKey, "ansidate");
	RegDeleteValue(hkRegKey, "waitforpostinstall");

	ret=sizeof(buf);
	MsiGetProperty(hInstall, "Installed", buf, &ret);
	if(buf[0])
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}

	bCreate=FALSE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresDBMS", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresIce", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresStar", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresReplicator", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresNet", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresEsqlC", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresEsqlCobol", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresEsqlFortran", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresTools", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresVision", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresDotNETDataProvider", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;
	if (!bCreate
	&& !MsiGetFeatureState(hInstall, "IngresDoc", &iInstalled, &iAction)
	&& (iInstalled==3)) bCreate=TRUE;

	bInstanceExist=FALSE;
	if (GetInstallPath(ii_code, ii_system))
	{
	    sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
	    if (GetFileAttributes(szBuf)!=-1) bInstanceExist=TRUE;
	}

	if (!_stricmp(ii_code, "II"))
	    strcpy(szComponentGUID, "{844FEE64-249D-11D5-BDF5-00B0D0AD4485}");
	else
	{
	    int idx;

	    idx = (toupper(ii_code[0]) - 'A') * 26 + toupper(ii_code[1]) - 'A';
	    if (idx <= 0)
		idx = (toupper(ii_code[0]) - 'A') * 26 + toupper(ii_code[1]) - '0';
	    sprintf(szComponentGUID, "{844FEE64-249D-11D5-%04X-%012X}", idx, idx*idx);
	}

	ret=sizeof(szProductGUID);
	MsiGetProperty(hInstall, "ProductCode", szProductGUID, &ret);

	if(GetRefCount(szComponentGUID, szProductGUID)>=1
	|| bCreate || bInstanceExist)
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}

	RegDeleteValue(hkRegKey, "installationcode");
	RegDeleteValue(hkRegKey, "II_SYSTEM");
	RegCloseKey(hkRegKey);
	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", ii_code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\IngresCorporation\\Ingres");
	SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\IngresCorporation");
	SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, RegKey);
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_delete_file_associations
**
**  Description:
**	Delete file associations for VDBA and IIA of the current installation,
**	and if there exist other Ingres installations, restore VDBA and IIA file
**	associations to point to one of the existing installations. Run as a commit
**	custom action.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	30-oct-2001 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_delete_file_associations(MSIHANDLE hInstall)
{
	char cadata[255], cur_code[3], cur_path[MAX_PATH+1], temp[1024], *p, *q;
	DWORD dwSize;
	char file2check[MAX_PATH+1], code[3], szKey[255];
	int isDBATools=0;

	dwSize=sizeof(cadata);
	MsiGetProperty(hInstall, "CustomActionData", cadata, &dwSize);

	strcpy(temp, cadata);
	p=strchr(temp, ';');
	q=p+1;
	*p=0;
	strcpy(cur_code, temp);
	while(*q!=0 && !isalnum(*q))
		q=q+1;
	strcpy(cur_path, q);

	if (cur_path[strlen(cur_path)-1] == '\\')
		cur_path[strlen(cur_path)-1] = '\0';


        dwSize = sizeof(temp);
        if (MsiGetProperty( hInstall, "DBATools", temp, &dwSize)==ERROR_SUCCESS && temp[0])
	    isDBATools=TRUE;

	/* Delete VDBA and IIA file associations set in registry*/
	sprintf(file2check, "%s\\ingres\\bin\\vdba.exe", cur_path);
	if (GetFileAttributes(file2check) != -1)
		return ERROR_SUCCESS;

	sprintf(szKey, "Ingres.VDBA.%s\\shell\\open\\command", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
	sprintf(szKey, "Ingres.VDBA.%s\\shell\\open", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
	sprintf(szKey, "Ingres.VDBA.%s\\shell", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
	sprintf(szKey, "Ingres.VDBA.%s", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

	sprintf(szKey, "Ingres.IIA.%s\\shell\\open\\command", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
	sprintf(szKey, "Ingres.IIA.%s\\shell\\open", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
	sprintf(szKey, "Ingres.IIA.%s\\shell", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
	sprintf(szKey, "Ingres.IIA.%s", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

	sprintf(szKey, "Ingres_Database_%s\\shell", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
	if (isDBATools)
	sprintf(szKey, "Ingres_DBATools_%s", cur_code);
	else
	sprintf(szKey, "Ingres_Database_%s", cur_code);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

	if(OtherInstallationsExist(cur_code, code))
	{
		HKEY hKey=0;
		DWORD dwType;
		char szData[255];
		DWORD dwSize=sizeof(szData);

		*szData=0;
		sprintf(szKey, "Ingres.VDBA.%s", cur_code);
		RegOpenKeyEx(HKEY_CLASSES_ROOT, ".vdbacfg", 0, KEY_QUERY_VALUE, &hKey);
		RegQueryValueEx(hKey, NULL, NULL, &dwType,(BYTE *)szData, &dwSize);
		if(szData[0] && !_stricmp(szData, szKey))
		{
			// restore association for VDBA
			sprintf(szKey, "Ingres.VDBA.%s", code);
			SetRegistryEntry(HKEY_CLASSES_ROOT, ".vdbacfg", "", REG_SZ, szKey);

			// restore association for IIA
			sprintf(szKey, "Ingres.IIA.%s", code);
			SetRegistryEntry(HKEY_CLASSES_ROOT, ".ii_imp", "", REG_SZ, szKey);
		}
	}
	else
	{
		RegDeleteKey(HKEY_CLASSES_ROOT, ".vdbacfg");
		RegDeleteKey(HKEY_CLASSES_ROOT, ".ii_imp");
	}

	return ERROR_SUCCESS;
}

/*
**  Name:ingres_stop_ingres
**
**  Description:
**	Stop Ingres if it is running.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	10-dec-2001 (penga03)
**	    Created.
**	17-jan-2002 (penga03)
**	    If embed install, shut down Ingres silently.
**	21-mar-2002 (penga03)
**	    Shut down Apache HTTP Server first if this is
**	    Advantage Ingres SDK.
**	05-jun-2002 (penga03)
**	    Shut down ivm.
**	20-may-2003 (penga03)
**	    Added code to stop OpenROAD and Portal (if SDK is defined)
**	    before stop Ingres.
*/
UINT __stdcall
ingres_stop_ingres(MSIHANDLE hInstall)
{
	char ii_id[3], ii_system[MAX_PATH];
	DWORD dwSize;
	HKEY hRegKey;
	char szBuf[MAX_PATH], szBuf2[MAX_PATH];
	BOOL bSilent, bCluster;
	HCLUSTER hCluster = NULL;
	HRESOURCE hResource = NULL;
	WCHAR lpwResourceName[256];

    /* get II_INSTALLATION and II_SYSTEM */
    dwSize=sizeof(ii_id); *ii_id=0;
    MsiGetProperty(hInstall, "II_INSTALLATION", ii_id, &dwSize);

    dwSize=sizeof(ii_system); *ii_system=0;
    MsiGetTargetPath(hInstall, "INGRESFOLDER", ii_system, &dwSize);

    if (!ii_system[0])
    {
	if (!Local_NMgtIngAt("II_INSTALLATION", szBuf) && !_stricmp(szBuf, ii_id) )
	    GetEnvironmentVariable("II_SYSTEM", ii_system, sizeof(ii_system));
    }
    if (!ii_system[0])
    {
	sprintf(szBuf, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", ii_id);
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	    dwSize=sizeof(ii_system);
	    RegQueryValueEx(hRegKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwSize);
	    RegCloseKey(hRegKey);
	}
    }
    if (!ii_system[0])
    {
	sprintf(szBuf, "SOFTWARE\\IngresCorporation\\IngresII\\%s_Installation", ii_id);
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	    dwSize=sizeof(ii_system);
	    RegQueryValueEx(hRegKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwSize);
	    RegCloseKey(hRegKey);
	}
    }

    if (ii_system[strlen(ii_system)-1] == '\\')
	ii_system[strlen(ii_system)-1] = '\0';

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "INGRES_SDK", szBuf, &dwSize);
    if(!strcmp(szBuf, "1"))
    {
	/* stop OpenROAD AppServer */
	ExecuteEx("net stop orsposvc");

	/* stop portal */
	GetCurrentDirectory(sizeof(szBuf), szBuf);
	sprintf(szBuf2, "%s\\PortalSDK", ii_system);
	SetCurrentDirectory(szBuf2);
	ExecuteEx("stopportal.bat");
	SetCurrentDirectory(szBuf);

	/* stop apache */
	if(RegOpenKeyEx(HKEY_CURRENT_USER,
	  "SOFTWARE\\Apache Group\\Apache\\1.3.23",
	  0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
	{
	    char szData[256];
	    DWORD dwSize=sizeof(szData);

	    if(RegQueryValueEx(hRegKey, "ServerRoot",
	       NULL, NULL, (BYTE *)szData, &dwSize)==ERROR_SUCCESS)
	    {
		if (szData[strlen(szData)-1]=='\\')
		    szData[strlen(szData)-1]='\0';
		sprintf(szBuf, "\"%s\\Apache.exe\" -k stop", szData);
		ExecuteEx(szBuf);
	    }
	    RegCloseKey(hRegKey);
	}
    }

	/* shutdown Ingres */
	MsiSetProperty(hInstall, "INGRES_RUNNING", "0");

	/* shut down ivm */
	sprintf(szBuf, "%s\\ingres\\bin\\ivm.exe", ii_system);
	if (GetFileAttributes(szBuf) != -1)
	{
		sprintf(szBuf, "\"%s\\ingres\\bin\\ivm.exe\" -stop", ii_system);
		Execute(szBuf, FALSE);
	}

	if (!IngresAlreadyRunning(ii_system))
		return ERROR_SUCCESS;

	MessageBeep(MB_ICONEXCLAMATION);
	MsiSetProperty(hInstall, "INGRES_RUNNING", "1");

	bSilent=FALSE;
	dwSize=sizeof(szBuf);
	MsiGetProperty(hInstall, "UILevel", szBuf, &dwSize);
	if (!strcmp(szBuf, "2"))
		bSilent=TRUE;

	bCluster=FALSE;
	dwSize=sizeof(szBuf);
	MsiGetProperty(hInstall, "INGRES_CLUSTER_INSTALL", szBuf, &dwSize);
	if (!strcmp(szBuf, "1"))
		bCluster=TRUE;
	if (bSilent ||
		AskUserYN(hInstall, "Your Ingres instance is currently running. Would you like to shut it down now?"))
	{
		SetEnvironmentVariable("II_SYSTEM", ii_system);
		GetSystemDirectory(szBuf, sizeof(szBuf));
		sprintf(szBuf2, "%s\\ingres\\bin;%s\\ingres\\utility;%s", ii_system, ii_system, szBuf);
		SetEnvironmentVariable("PATH", szBuf2);

		if (!bCluster)
		{
			if (bSilent)
			{
				sprintf(szBuf, "\"%s\\ingres\\utility\\ingstop.exe\"", ii_system);
				ExecuteEx(szBuf);
			}
			else
			{
				sprintf(szBuf, "\"%s\\ingres\\bin\\winstart.exe\" /stop", ii_system);
				Execute(szBuf, TRUE);

				sprintf(szBuf, "Ingres - Service Manager [%s]", ii_id);
				while (FindWindow(NULL, szBuf))
					Sleep(1000);
			}
		}
		else
		{//take the cluster resource offline
			dwSize=sizeof(szBuf); *szBuf=0;
			MsiGetProperty(hInstall, "INGRES_CLUSTER_RESOURCE", szBuf, &dwSize);

			hCluster = OpenCluster(NULL);
			if (hCluster)
			{
				mbstowcs(lpwResourceName, szBuf, 256);
				hResource = OpenClusterResource(hCluster, lpwResourceName);
				if (hResource)
				{
					OfflineClusterResource(hResource);
					CloseClusterResource(hResource);
				}
			}
			while (IngresAlreadyRunning(ii_system))
				Sleep(1000);
		}

		if(!IngresAlreadyRunning(ii_system))
			MsiSetProperty(hInstall, "INGRES_RUNNING", "0");
	}

	return ERROR_SUCCESS;
}

/*
**  Name: ingres_set_timezone
**
**  Description:
**	When installing Ingres, the installer querys the operating system for
**	the timezone being used and set the Ingres timezone to be the system
**	default timezone.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	07-May-2001 (penga03)
**	    Created.
**	22-Jun-2001 (penga03)
**	    If this is a modify or upgrade, leave the timezone, as well as
**	    charset and terminal, as was.
**	10-Aug-2001 (penga03)
**	    During upgrade, if can not get timezone, charset or terminal from
**	    config.dat, set them to their default values.
**	04-apr-2002 (penga03)
**	    Set the default value of Ingres charset to match the OS
**	    current code page gotten by GetACP().
**	17-oct-2002 (penga03)
**	    Rename ingres_set_timezone to ingres_set_timezone_auto,
**	    and narrowed down its function to: if this is an upgrade,
**	    get time zone from %II_SYSTEM%\ingres\files\symbol.tbl,
**	    otherwise get the OS default time zone and map it to Ingres
**	    time zone.
**	13-Feb-2007 (drivi01)
**	    Timezone registry slightly changed on Vista.
**	    Added routine that looks at "TimeZoneKeyName" in registry
**	    on Vista machines to determine the timezone currently set.
*/
UINT __stdcall
ingres_set_timezone_auto(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1];
    DWORD dwSize;
    char *szKey;
    HKEY hKey;
    TCHAR default_tz[128], DisplayName[128], StdName[128], ing_tz[128];

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);

	Local_NMgtIngAt("II_TIMEZONE_NAME", szBuf);
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_TIMEZONE", szBuf);
	return (ERROR_SUCCESS);
    }

    /* Get OS default time zone */
    if(IsWindows95())
	szKey="SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Time Zones";
    else
	szKey="SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones";

    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
                     0, KEY_QUERY_VALUE, &hKey))
    {
	char szData[512];
	DWORD dwType, dwSize=sizeof(szData);
	
	if (IsVista())
	{
	    if(!RegQueryValueEx(hKey, "TimeZoneKeyName", NULL, &dwType, (BYTE *)szData, &dwSize))
	    	lstrcpy(default_tz, szData);
	}
	else
	{
	    if(!RegQueryValueEx(hKey, "StandardName", NULL, &dwType, (BYTE *)szData, &dwSize))
	    	lstrcpy(default_tz, szData);
	}
    }

    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_ENUMERATE_SUB_KEYS, &hKey))
    {
	LONG retCode;
	int i;

	for (i = 0, retCode = ERROR_SUCCESS; retCode == ERROR_SUCCESS; i++)
	{
	    char SubKey[512];
	    DWORD dwSize=sizeof(SubKey);
	    HKEY hSubKey=0;

	    retCode=RegEnumKeyEx(hKey, i, SubKey, &dwSize, NULL, NULL, NULL, NULL);

	    if(!RegOpenKeyEx(hKey, SubKey, 0, KEY_QUERY_VALUE, &hSubKey))
	    {
		DWORD dwType, dwSize=sizeof(StdName);
		DWORD dwType02, dwSize02=sizeof(DisplayName);

		if(!RegQueryValueEx(hSubKey, "Std", NULL, &dwType, (BYTE *)StdName, &dwSize))
		{
		    if(!lstrcmpi(default_tz, StdName))
		    {
			strcpy(StdName, SubKey);
			RegQueryValueEx(hSubKey, "Display", NULL, &dwType02, (BYTE *)DisplayName, &dwSize02);
			break;
		    }
		}

		RegCloseKey(hSubKey);
	    }
	}

	RegCloseKey(hKey);
    }

    /* Query the table to get corresponding Ingres time zone */
    if (searchIngTimezone(DisplayName, ing_tz))
	MsiSetProperty(hInstall, "INGRES_TIMEZONE", ing_tz);
    else if (searchIngTimezone(StdName, ing_tz))
	MsiSetProperty(hInstall, "INGRES_TIMEZONE", ing_tz);
    else
	MsiSetProperty(hInstall, "INGRES_TIMEZONE", "NA-EASTERN");

    return (ERROR_SUCCESS);
}

/*
**  Name: ingres_set_timezone_manual
**
**  Description:
**	If this is an upgrade, get time zone from
**	%II_SYSTEM%\ingres\files\symbol.tbl, otherwise use the value
**	of INGRES_TIMEZONE that is set in the Property table.
**
**  History:
**	17-oct-2002 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_set_timezone_manual(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1];
    DWORD dwSize;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);

	Local_NMgtIngAt("II_TIMEZONE_NAME", szBuf);
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_TIMEZONE", szBuf);
	return (ERROR_SUCCESS);
    }

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "INGRES_TIMEZONE", szBuf, &dwSize);
    if (!szBuf[0] || !VerifyIngTimezone(szBuf))
	MsiSetProperty(hInstall, "INGRES_TIMEZONE", "NA-EASTERN");

    return (ERROR_SUCCESS);
}

/*
**  Name: ingres_set_charset_auto
**
**  Description:
**	If this is an upgrade, get character set from
**	%II_SYSTEM%\ingres\files\symbol.tbl, otherwise get the OS
**	default character set and map it to Ingres character set.
**
**  History:
**	17-oct-2002 (penga03)
**	    Created.
**	26-Jun-2008 (drivi01)
**	    Update this function to append "(default)" to the
**	    default character set.
*/
UINT __stdcall
ingres_set_charset_auto(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1], Symbol[128], cmd[MAX_PATH+1];
    DWORD dwSize;
    UINT acp;
    char os_acp[32];
    MSIHANDLE msih, hView, hRec;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);

	sprintf(Symbol, "II_CHARSET%s", szCode);
	if (!Local_NMgtIngAt(Symbol, szBuf))
	{
	    strcpy(Symbol, "II_CHARSET");
	    Local_NMgtIngAt(Symbol, szBuf);
	}
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_CHARSET", szBuf);
	return (ERROR_SUCCESS);
    }

    /* Get OS default charset */
    acp=GetACP();
    sprintf(os_acp, "%d", acp);
    if (searchIngCharset(os_acp, szBuf))
	MsiSetProperty(hInstall, "INGRES_CHARSET",szBuf);
    else
	{
	sprintf(szBuf, "WIN1252");
	MsiSetProperty(hInstall, "INGRES_CHARSET","WIN1252");
	}

    /* append "(default)" string to the default character set. */
    msih = MsiGetActiveDatabase(hInstall);
    if (!msih)
	return FALSE;
    sprintf(cmd, "SELECT * FROM ComboBox WHERE Property='INGRES_CHARSET' AND Text='%s'", szBuf);
    if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
	return FALSE;

    if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
	return FALSE;

    if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
	return FALSE;

    sprintf(cmd, "%s %s", szBuf, "(default)");
    if (!(MsiRecordSetString(hRec, 4, cmd) == ERROR_SUCCESS))
	return FALSE;

    if (!(MsiViewModify(hView, MSIMODIFY_DELETE, hRec) == ERROR_SUCCESS))
	return FALSE;

    if (!(MsiViewModify(hView, MSIMODIFY_INSERT_TEMPORARY, hRec) == ERROR_SUCCESS))
	return FALSE;

    if (!(MsiCloseHandle(hRec) == ERROR_SUCCESS))
		return FALSE;

    if (!(MsiViewClose( hView ) == ERROR_SUCCESS))
	return FALSE;

    if (!(MsiCloseHandle(hView) == ERROR_SUCCESS))
	return FALSE;
	/* Commit changes to the MSI database */
    if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	return FALSE;

    if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	return FALSE;

    return (ERROR_SUCCESS);
}

/*
**  Name: ingres_set_charset_manual
**
**  Description:
**	If this is an upgrade, get character set from
**	%II_SYSTEM%\ingres\files\symbol.tbl, otherwise use the value
**	of INGRES_CHARSET that is set in the Property table.
**
**  History:
**	17-oct-2002 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_set_charset_manual(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1], Symbol[128];
    DWORD dwSize;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);

	sprintf(Symbol, "II_CHARSET%s", szCode);
	if (!Local_NMgtIngAt(Symbol, szBuf))
	{
	    strcpy(Symbol, "II_CHARSET");
	    Local_NMgtIngAt(Symbol, szBuf);
	}
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_CHARSET", szBuf);
	return (ERROR_SUCCESS);
    }

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "INGRES_CHARSET", szBuf, &dwSize);
    if (!szBuf[0] || !VerifyIngCharset(szBuf))
	MsiSetProperty(hInstall, "INGRES_CHARSET", "WIN1252");

    return (ERROR_SUCCESS);
}

/*
**  Name: ingres_set_terminal_auto
**
**  History:
**	31-Jan-2005 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_set_terminal_auto(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1];
    DWORD dwSize;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);

	Local_NMgtIngAt("TERM_INGRES", szBuf);
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_TERMINAL", szBuf);
	return (ERROR_SUCCESS);
    }

    MsiSetProperty(hInstall, "INGRES_TERMINAL", "IBMPCD");

    return (ERROR_SUCCESS);
}

/*
**  Name: ingres_set_terminal_manual
**
**  Description:
**	If this is an upgrade, get character set from
**	%II_SYSTEM%\ingres\files\symbol.tbl, otherwise use the value
**	of INGRES_TERMINAL that is set in the Property table.
**
**  History:
**	17-oct-2002 (penga03)
**	    Created.
**	31-jan-2005 (penga03)
**	    If Ingres charset is set to WIN1252, the default terminal
**	    will be IBMPC, otherwise IBMPCD.
*/
UINT __stdcall
ingres_set_terminal_manual(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1];
    DWORD dwSize;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);

	Local_NMgtIngAt("TERM_INGRES", szBuf);
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_TERMINAL", szBuf);
	return (ERROR_SUCCESS);
    }

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "INGRES_TERMINAL", szBuf, &dwSize);
    if (!szBuf[0] || !VerifyIngTerminal(szBuf))
	MsiSetProperty(hInstall, "INGRES_CHARSET", "IBMPCD");

    return (ERROR_SUCCESS);
}

/*
**  Name:embedingres_cleanup_commit
**
**  Description:
**	Clean up the registry enteries and directories. Run in commit execution
**	mode. ONLY used by the product that embeds Ingres merge moudles.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	14-feb-2002 (penga03)
**	    Created.
**	26-feb-2002 (penga03)
**	    Check the property REMOVE, if all Ingres features are being removed,
**	    clean up the Ingres registry and directory.
**	10-apr-2002 (penga03)
**	    When clean up the Ingres registry and directory, besides checking
**	    REMOVE, also checking PATCH and UPGRADINGPRODUCTCODE.
**	11-apr-2002 (penga03)
**	    Delete parent folders of the Ingres registry if they are empty.
**	24-mar-2003 (penga03)
**	    Initialize the char arrays that will store the properties chopped
**	    from CustomActionData.
**	16-jun-2003 (penga03)
**	    Do not use strtok to parse CustomActionData, since strtok will exit
**	    once it encounters a NULL token, even there could exit a non-NULL
**	    token later.
**	    Instead of using MsiEnumFeatures, use INGRES_INSTALLEDFEATURES. If
**	    there exists an Ingres feature in INGRES_INSTALLEDFEATURES, but not
**	    in REMOVE, we won't do clean up. There are couple of reasons we no
**	    longer use MsiEnumFeatures:
**	    This function enumerates all "published" features, no matter of which
**	    is installed or not. There could have the case that an published
**	    Ingres feature is not listed in the REMOVE, as a result, Ingres is
**	    not cleaned up, which is supposed to be.
**	    Another reason is MsiEnumFeatures will return FALSE if called in a
**	    commit custom action.
**	20-oct-2003 (penga03)
**	    Remove %ii_system% only if it is empty.
*/
UINT __stdcall
embedingres_cleanup_commit(MSIHANDLE hInstall)
{
    char szBuf[MAX_PATH];
    DWORD dwSize;
    char code[3], szProduct[39], remove[1024];
    char patch[1024], upgradingproduct[1024], installedfeatures[1024];
    char szComponentGUID[39], ii_system[MAX_PATH];
    char RegKey[256];
    HKEY hkRegKey;
    char *p, *q, *tokens[6], *token;
    int count;
    BOOL bCleanup, bInstanceExist;

    bCleanup=TRUE;
    /*
    **CustomActionData:[II_INSTALLATION];[ProductCode];[REMOVE];
    **[PATCH];[UPGRADINGPRODUCTCODE];[INGRES_INSTALLEDFEATURES]
    */
    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "CustomActionData", szBuf, &dwSize);

    *code=*szProduct=*remove=*patch=*upgradingproduct=*installedfeatures=0;
    p=szBuf;
    for (count=0; count<=5; count++)
    {
	q=strchr(p, ';');
	if (q) *q='\0';
	tokens[count]=p;
	p=q+1;
    }
    strcpy(code,tokens[0]);
    strcpy(szProduct, tokens[1]);
    strcpy(remove, tokens[2]);
    strcpy(patch, tokens[3]);
    strcpy(upgradingproduct, tokens[4]);
    strcpy(installedfeatures, tokens[5]);

    sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", code);
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
    {
	RegDeleteValue(hkRegKey, "InstalledFeatures");
	RegDeleteValue(hkRegKey, "RemovedFeatures");
	RegCloseKey(hkRegKey);
    }

    if (installedfeatures[0] && remove[0] && !patch[0] && !upgradingproduct[0])
    {
	token=strtok(installedfeatures, ",");
	while(token != NULL)
	{
	    if (!strstr(remove, token))
	    {
		bCleanup=FALSE;
		break;
	    }
	    token=strtok(NULL, "," );
	}

	/* if remove from Add/Remove, REMOVE is set to ALL; if remove from UI,
	REMOVE is set to the features to be remove */
	if (bCleanup || !_stricmp(remove, "ALL"))
	{
	    if (!_stricmp(code, "II"))
		strcpy(szComponentGUID, "{844FEE64-249D-11D5-BDF5-00B0D0AD4485}");
	    else
	    {
		int idx;

		idx = (toupper(code[0]) - 'A') * 26 + toupper(code[1]) - 'A';
		if (idx <= 0)
		    idx = (toupper(code[0]) - 'A') * 26 + toupper(code[1]) - '0';
		sprintf(szComponentGUID, "{844FEE64-249D-11D5-%04X-%012X}", idx, idx*idx);
	    }

	    if (GetRefCount(szComponentGUID, szProduct)==0)
	    {
		if (GetInstallPath(code, ii_system))
		{
		    sprintf(szBuf, "%s\\ingres", ii_system);
		    RemoveDir(szBuf, TRUE);
		    RemoveDirectory(ii_system);
		}

		sprintf(RegKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", code);
		RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
		sprintf(RegKey, "SOFTWARE\\IngresCorporation\\Ingres");
		SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, RegKey);
		sprintf(RegKey, "SOFTWARE\\IngresCorporation");
		SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, RegKey);
	    }

	    sprintf(RegKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", szProduct);
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "EmbeddedRelease");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "IngresAllUsers");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "IngresCreateIcons");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "IngresInstallationID");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "SilentInstalled");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "JDBCDriverSilentInstalled");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "JDBCServerSilentInstalled");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "VDBASilentInstalled");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "Cdimage");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "IngresODBCInstalled");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "IngresODBCReadOnlyInstalled");
	    SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "IngresClusterResource");
		SHDeleteValue(HKEY_LOCAL_MACHINE, RegKey, "IngresClusterInstall");

	    SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, RegKey);
	}
    }

    bInstanceExist=FALSE;
    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Advantage Ingres\\%s_Installation", code);
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
    {
	dwSize=sizeof(ii_system);
	if(!RegQueryValueEx(hkRegKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system,&dwSize))
	{
	    sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
	    if(GetFileAttributes(szBuf)!=-1) bInstanceExist=TRUE;
	}
	RegCloseKey(hkRegKey);
    }
    if(!bInstanceExist)
    {
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Advantage Ingres\\%s_Installation", code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Advantage Ingres");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
    }

    bInstanceExist=FALSE;
    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII\\%s_Installation", code);
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
    {
	dwSize=sizeof(ii_system);
	if(!RegQueryValueEx(hkRegKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system,&dwSize))
	{
	    sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
	    if(GetFileAttributes(szBuf)!=-1) bInstanceExist=TRUE;
	}
	RegCloseKey(hkRegKey);
    }
    if(!bInstanceExist)
    {
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII\\%s_Installation", code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
    }

    return ERROR_SUCCESS;
}

/*
**  Name:ingres_set_shortcuts
**
**  Description:
**	Create Ingres shortcuts for current user.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	14-may-2002 (penga03)
**	    Created.
**	01-aug-2002 (penga03)
**	    Corrected some error. There is no "Ingres 2.6 Documentation"
**	    subfolder under the "Computer Associates\\Advantage\\".
**	12-aug-2002 (penga03)
**	    Made the corresponding change on Ingres installer for SIR
**	    108421, two embedded specific documentation shortcuts has
**	    been added.
**	16-jun-2003 (penga03)
**	    Do not use strtok to parse CustomActionData, since strtok will exit
**	    once it encounters a NULL token, even there could exit a non-NULL
**	    token later.
**	06-Mar-2007 (drivi01)
**	    Add a shortcut for readme.html.
**	16-Jun-2008 (drivi01)
**	    Remove any references to "Ingres 2006", the name of the product
**	    is being changed to Ingres.
**	27-Jun-2008 (drivi01)
**	    Remove DebugBreak.
**	16-Jan-2009 (whiro01) SD issue 132899
**	    Create a new "perfmonwrap.bat" file in the post install step
**	    that will be invoked by the "Monitor Ingres Cache Statistics" menu item
**	    that sets up the environment, loads the perf DLL and the counters
**	    before invoking "perfmon" with the given parameter.
**	19-Feb-2009 (drivi01)
**	    Rename Bookshelf.pdf to Ing_Bookshelf.pdf.
*/
UINT __stdcall
ingres_set_shortcuts(MSIHANDLE hInstall)
{
    char szBuf[1024], *tokens[6], *p, *q;
    DWORD dwSize;
    char szIngAllUsers[2], szIngShortcuts[2], szId[3], szII_SYSTEM[MAX_PATH];
    LPITEMIDLIST pidlProg, pidlStart;
    char szPath[MAX_PATH];
    BOOL DBMSInstalled, NetInstalled, VdbaInstalled, DocInstalled, EmbedDocInstalled;
    char szFolder[MAX_PATH], szDisplayName[125], szTarget[MAX_PATH], szArgs[1024];
    char szIconFile[MAX_PATH], szWorkDir[MAX_PATH];
    int count;
    char szORFolder[MAX_PATH], szLang[256];
    HKEY hKey;
    BOOL bAllUsers, bOpenROAD=0;
    LPMALLOC pMalloc = NULL;
    char szIVMFlag[256];

    /*
    ** CustomActionData:
    ** [INGRES_ALLUSERS];[INGRES_SHORTCUTS];[II_INSTALLATION]
    ** ;[INGRES_ORFOLDER];[INGRES_LANGUAGE];[INGRES_IVMFLAG]
    */
    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "CustomActionData", szBuf, &dwSize);

    *szIngAllUsers=*szIngShortcuts=*szId=*szORFolder=*szLang=0;
    p=szBuf;
    for (count=0; count<=5; count++)
    {
	q=strchr(p, ';');
	if (q) *q='\0';
	tokens[count]=p;
	p=q+1;
    }
    strcpy(szIngAllUsers, tokens[0]);
    strcpy(szIngShortcuts, tokens[1]);
    strcpy(szId, tokens[2]);
    strcpy(szORFolder, tokens[3]);
    strcpy(szLang, tokens[4]);
	strcpy(szIVMFlag, tokens[5]);

    GetInstallPath(szId, szII_SYSTEM);
    if (!strcmp(szIngAllUsers, "1")) bAllUsers=1;
    else bAllUsers=0;

    /* Restore OpenROAD shortcuts. */
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
       "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{8568E000-AE70-11D4-8EE7-00C04F81B484}",
       0, KEY_READ, &hKey))
    {
	bOpenROAD=1;
	RegCloseKey(hKey);
    }

    if (szORFolder[0] && bOpenROAD)
    {
	sprintf(szTarget, "%s\\ingres\\bin\\ingwrap.exe", szII_SYSTEM);
	sprintf(szArgs, "\"%s\\ingres\\bin\\w4glrunu.exe\" runimage", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\w4glrunu.exe", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\bin\\", szII_SYSTEM);
	CreateOneShortcut(szORFolder, "OpenROAD RunImage", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "OpenROAD Runtime Startup Utility.",
	                  0, bAllUsers);

	sprintf(szTarget, "%s\\ingres\\bin\\ingwrap.exe", szII_SYSTEM);
	sprintf(szArgs, "\"%s\\ingres\\bin\\w4gldev.exe\" runimage workbnch.img", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\w4gldev.exe", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\bin\\", szII_SYSTEM);
	CreateOneShortcut(szORFolder, "OpenROAD Workbench", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "OpenROAD Development.",
	                  0, bAllUsers);
    }

    if(IsWindows95() || !strcmp(szIngShortcuts, "0") || !strcmp(szIngAllUsers, "1"))
	return ERROR_SUCCESS;

    CoInitialize(NULL);
    SHGetMalloc(&pMalloc);

    SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlProg);
    SHGetFolderLocation(NULL, CSIDL_STARTUP, NULL, 0, &pidlStart);

    SHGetPathFromIDList(pidlProg, szPath);

    sprintf(szBuf, "%s\\Ingres\\Ingres %s\\Ingres Documentation", szPath, szId);
    if (RemoveOneDir(szBuf))
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
    sprintf(szBuf, "%s\\Ingres\\Ingres %s\\Ingres Embedded Documentation", szPath, szId);
    if (RemoveOneDir(szBuf))
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
    sprintf(szBuf, "%s\\Ingres\\Ingres %s", szPath, szId);
    if (RemoveOneDir(szBuf))
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
    sprintf(szBuf, "%s\\Ingres", szPath);
    if (RemoveDirectory(szBuf))
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);

    SHGetPathFromIDList(pidlStart, szPath);

    sprintf(szBuf, "%s\\Ingres Visual Manager %s.lnk", szPath, szId);
    if (DeleteFile(szBuf))
	SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szBuf, 0);

    SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlProg, NULL);
    SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlStart, NULL);

    if (pidlProg)
	pMalloc->lpVtbl->Free(pMalloc, pidlProg);
    if (pidlStart)
	pMalloc->lpVtbl->Free(pMalloc, pidlStart);

    pMalloc->lpVtbl->Release(pMalloc);
    CoUninitialize();

    DBMSInstalled=NetInstalled=VdbaInstalled=DocInstalled=EmbedDocInstalled=FALSE;
    sprintf(szBuf, "%s\\ingres\\bin\\iidbms.exe", szII_SYSTEM);
    if (GetFileAttributes(szBuf)!=-1)
	DBMSInstalled=TRUE;
    sprintf(szBuf, "%s\\ingres\\bin\\iigcc.exe", szII_SYSTEM);
    if (GetFileAttributes(szBuf)!=-1)
	NetInstalled=TRUE;
    sprintf(szBuf, "%s\\ingres\\bin\\ivm.exe", szII_SYSTEM);
    if (GetFileAttributes(szBuf)!=-1)
	VdbaInstalled=TRUE;
    sprintf(szBuf, "%s\\ingres\\files\\english\\Ing_Bookshelf.pdf", szII_SYSTEM);
    if (GetFileAttributes(szBuf)!=-1)
	DocInstalled=TRUE;
    sprintf(szBuf, "%s\\ingres\\files\\english\\BookshelfEIE.pdf", szII_SYSTEM);
    if (GetFileAttributes(szBuf)!=-1)
	EmbedDocInstalled=TRUE;

    /* creating shortcuts... */
    if(DBMSInstalled || NetInstalled)
    {
	char szSystemFolder[MAX_PATH], szProgramFilesFolder[MAX_PATH];

	GetSystemDirectory(szSystemFolder, sizeof(szSystemFolder));
	GetEnvironmentVariable("programfiles", szProgramFilesFolder, sizeof(szProgramFilesFolder));

	sprintf(szFolder, "Ingres\\Ingres %s", szId);

	sprintf(szTarget, "%s\\ingres\\bin\\ingwrap.exe", szII_SYSTEM);
	sprintf(szArgs, "\"%s\\ingres\\bin\\winstart.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\winstart.exe", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\bin\\", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Service Manager", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "Controls the starting and stopping of the Ingres installation.",
	                  0, 0);

	sprintf(szTarget, "%s\\notepad.exe", szSystemFolder);
	sprintf(szArgs, "\"%s\\ingres\\files\\errlog.log\"", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\files\\", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Error Log", szTarget, szArgs, "", szWorkDir,
	                  "errlog.log for Ingres installation.",
	                  0, 0);

	sprintf(szTarget, "%s\\ingres\\bin\\ingwrap.exe", szII_SYSTEM);
	sprintf(szArgs, "\"%s\\ingres\\bin\\perfwiz.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\perfwiz.exe", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\bin\\", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Performance Monitor Wizard", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "Wizard used to customize the usage of the Performance Monitor with the Ingres installation.",
	                  0, 0);

	sprintf(szTarget, "%s\\CMD.EXE", szSystemFolder);
	sprintf(szArgs, "/K \"%s\\ingres\\bin\\setingenvs.bat\"", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Command Prompt", szTarget, szArgs,
	                  "", "%HOMEDRIVE%%HOMEPATH%",
	                  "DOS command window with Ingres environment settings.",
	                  0, 0);

	sprintf(szTarget, "%s\\readme.html", szII_SYSTEM);
	sprintf(szArgs, "");
	CreateOneShortcut(szFolder, "Readme", szTarget, szArgs,
	                  "", "",
	                  "Ingres readme.",
	                  0, 0);

    }

    if(DBMSInstalled)
    {
	char szSystemFolder[MAX_PATH];

	GetSystemDirectory(szSystemFolder, sizeof(szSystemFolder));
	sprintf(szFolder, "Ingres\\Ingres %s", szId);

	sprintf(szTarget, "%s\\ingres\\bin\\ingwrap.exe", szII_SYSTEM);
	sprintf(szArgs, "\"%s\\ingres\\bin\\iilink.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\iilink.exe", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\bin\\", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres User Defined Data Type Linker", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "Wizard which aids in adding user defined data types to the Ingres DBMS server.",
	                  0, 0);

	sprintf(szTarget, "\"%s\\ingres\\bin\\perfmonwrap.bat\"", szII_SYSTEM);
	sprintf(szArgs, "\"%s\\ingres\\files\\ingcache.pmw\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\files\\ingcache.ico", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\files\\", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Monitor Ingres Cache Statistics", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "A performance monitor work file to monitor Ingres cache statistics.",
	                  0, 0);
    }

    if(VdbaInstalled)
    {
	sprintf(szFolder, "Ingres\\Ingres %s", szId);
	sprintf(szTarget, "%s\\ingres\\bin\\ingwrap.exe", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\bin\\", szII_SYSTEM);

	sprintf(szArgs, "\"%s\\ingres\\bin\\vcbf.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\vcbf.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Configuration Manager", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI version of the CBF utility.",
	                  0, 0);

	sprintf(szArgs, "\"%s\\ingres\\bin\\ingnet.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\ingnet.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Network Utility", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to manage Ingres networking.",
	                  0, 0);
	sprintf(szArgs, "\"%s\\ingres\\bin\\vdba.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\vdba.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Visual DBA", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to administer the Ingres installation.",
	                  0, 0);
	sprintf(szArgs, "\"%s\\ingres\\bin\\vdbasql.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\vdbasql.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Visual SQL", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to execute SQL queries.",
	                  0, 0);
	sprintf(szArgs, "\"%s\\ingres\\bin\\vdbamon.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\vdbamon.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Visual Performance Monitor", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to monitor an Ingres installation.",
	                  0, 0);

	sprintf(szArgs, "\"%s\\ingres\\bin\\iia.exe\" -loop", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\iia.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Import Assistant", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to aid in importing delimited value format files (such as .csv or .dbf files) into an Ingres database.",
	                  0, 0);
	sprintf(szArgs, "\"%s\\ingres\\bin\\iea.exe\" -loop", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\iea.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Export Assistant", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to export data from an Ingres database.",
	                  0, 0);

	sprintf(szArgs, "\"%s\\ingres\\bin\\vcda.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\vcda.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Visual Configuration Differences Analyzer", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to visualize configuration differences between Ingres installations.",
	                  0, 0);
	sprintf(szArgs, "\"%s\\ingres\\bin\\vdda.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\vdda.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Visual Database Objects Differences Analyzer", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to visualize differences between two Ingres installations or schemas.",
	                  0, 0);

	sprintf(szArgs, "\"%s\\ingres\\bin\\ija.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\ija.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Journal Analyzer", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to analyze Ingres journal files.",
	                  0, 0);

	if (szIVMFlag[0])
	    sprintf(szArgs, "\"%s\\ingres\\bin\\ivm.exe\" %s", szII_SYSTEM, szIVMFlag);
	else
	    sprintf(szArgs, "\"%s\\ingres\\bin\\ivm.exe\"", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\ivm.exe", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Visual Manager", szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to manage the Ingres installation.",
	                  0, 0);

	/* create ivm startup item */
	sprintf(szDisplayName, "Ingres Visual Manager %s", szId);
	sprintf(szTarget, "%s\\ingres\\bin\\ingwrap.exe", szII_SYSTEM);
	if (szIVMFlag[0])
	    sprintf(szArgs, "\"%s\\ingres\\bin\\ivm.exe\" %s", szII_SYSTEM, szIVMFlag);
	else
	    sprintf(szArgs, "\"%s\\ingres\\bin\\ivm.exe\"", szII_SYSTEM);
	sprintf(szWorkDir, "%s\\ingres\\bin\\", szII_SYSTEM);
	sprintf(szIconFile, "%s\\ingres\\bin\\ivm.exe", szII_SYSTEM);
	CreateOneShortcut("", szDisplayName, szTarget, szArgs,
	                  szIconFile, szWorkDir,
	                  "GUI used to manage the Ingres installation.",
	                  1, 0);
    }

    if(DocInstalled)
    {

	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\gs.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Getting Started", szTarget, "",
	                  "", szWorkDir, "Getting Started", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\gs-linux.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Getting Started for Linux", szTarget, "",
	                  "", szWorkDir, "Getting Started for Linux", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\mg.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Migration Guide", szTarget, "",
	                  "", szWorkDir, "Migration Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ome.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Object Management Extension User Guide", szTarget, "",
	                  "", szWorkDir, "Object Management Extension User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\cmdref.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Command Reference Guide", szTarget, "",
	                  "", szWorkDir, "Command Reference Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\dba.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Database Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Database Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\dtp.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Distributed Transaction Processing User Guide", szTarget, "",
	                  "", szWorkDir, "Distributed Transaction Processing User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\esql.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded SQL Companion Guide", szTarget, "",
		              "", szWorkDir, "Embedded SQL Companion Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ice.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Web Deployment Option User Guide", szTarget, "",
	                  "", szWorkDir, "Web Deployment Option User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\oapi.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "OpenAPI User Guide", szTarget, "",
	                  "", szWorkDir, "OpenAPI User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\osql.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "OpenSQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "OpenSQL Reference Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\equel.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded QUEL Companion Guide", szTarget, "",
	                  "", szWorkDir, "Embedded QUEL Companion Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\quel.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "QUEL Reference Guide", szTarget, "",
	                  "", szWorkDir, "QUEL Reference Guide", 0, 0);


	sprintf(szTarget, "%s\\ingres\\files\\english\\rep.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Replicator Option User Guide", szTarget, "",
	                  "", szWorkDir, "Replicator Option User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\sql.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "SQL Reference Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\sysadm.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "System Administrator Guide", szTarget, "",
	                  "", szWorkDir, "System Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\star.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Distributed Option User Guide", szTarget, "",
	                  "", szWorkDir, "Distributed Option User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\qry.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Character-based Querying and Reporting Tools User Guide", szTarget, "",
	                  "", szWorkDir, "Character-based Querying and Reporting Tools User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\fadt.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Forms-based Application Development Tools User Guide", szTarget, "",
	                  "", szWorkDir, "Forms-based Application Development Tools User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\connect.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Connectivity Guide", szTarget, "",
	                  "", szWorkDir, "Connectivity Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ipm.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Interactive Performance Monitor User Guide", szTarget, "",
	                  "", szWorkDir, "Interactive Performance Monitor User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\rs.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Release Summary", szTarget, "",
	                  "", szWorkDir, "Release Summary", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\rms.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres RMS Gateway User Guide", szTarget, "",
		              "", szWorkDir, "Ingres RMS Gateway User Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\Ing_Bookshelf.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);
    }

    if(EmbedDocInstalled && !_stricmp(szLang, "english"))
    {
	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Embedded Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\BookshelfEIE.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ing_GetStart_E.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Getting Started", szTarget, "",
	                  "", szWorkDir, "Ingres Getting Started", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ing_Linux_GetStart_E.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Getting Started for Linux", szTarget, "",
	                  "", szWorkDir, "Ingres Getting Started for Linux", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ing_Connectivity_E.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Connectivity Guide", szTarget, "",
	                  "", szWorkDir, "Ingres Connectivity Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ing_Admin_E.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres System Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Ingres System Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ing_DB_Admin_E.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Database Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Ingres Database Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ing_SQL_Ref_E.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "Ingres SQL Reference Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ing_Command_E.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Command Reference Guide", szTarget, "",
	                  "", szWorkDir, "Ingres Command Reference Guide", 0, 0);
    }

    else if(!_stricmp(szLang, "deu"))
    {
	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Embedded Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\deu\\BookshelfEIG.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\deu\\P002761G.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded Edition Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Embedded Edition Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\deu\\P003001G.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "SQL Reference Guide", 0, 0);
    }

    else if(!_stricmp(szLang, "esn"))
    {
	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Embedded Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\esn\\BookshelfEIS.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\esn\\P002761S.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded Edition Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Embedded Edition Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\esn\\P003001S.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "SQL Reference Guide", 0, 0);
    }

    else if(!_stricmp(szLang, "fra"))
    {
	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Embedded Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\fra\\BookshelfEIF.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\fra\\P002761F.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded Edition Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Embedded Edition Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\fra\\P003001F.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "SQL Reference Guide", 0, 0);
    }

    else if(!_stricmp(szLang, "ita"))
    {
	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Embedded Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ita\\BookshelfEII.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ita\\P002761I.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded Edition Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Embedded Edition Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ita\\P003001I.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "SQL Reference Guide", 0, 0);
    }

    else if(!_stricmp(szLang, "jpn"))
    {
	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Embedded Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\jpn\\BookshelfEIJ.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\jpn\\P002761J.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded Edition Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Embedded Edition Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\jpn\\P003001J.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "SQL Reference Guide", 0, 0);
    }

    else if(!_stricmp(szLang, "ptb"))
    {
	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Embedded Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ptd\\BookshelfEIP.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ptd\\P002761P.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded Edition Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Embedded Edition Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\ptd\\P003001P.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "SQL Reference Guide", 0, 0);
    }

    else if(!_stricmp(szLang, "sch"))
    {
	sprintf(szFolder, "Ingres\\Ingres %s\\Ingres Embedded Documentation", szId);
	sprintf(szWorkDir, "%s\\ingres\\files\\english\\", szII_SYSTEM);

	sprintf(szTarget, "%s\\ingres\\files\\english\\sch\\BookshelfEIC.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Ingres Bookshelf", szTarget, "",
	                  "", szWorkDir, "Ingres Bookshelf", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\sch\\P002761C.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "Embedded Edition Administrator Guide", szTarget, "",
	                  "", szWorkDir, "Embedded Edition Administrator Guide", 0, 0);

	sprintf(szTarget, "%s\\ingres\\files\\english\\sch\\P003001C.pdf", szII_SYSTEM);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "",
	                  "", szWorkDir, "SQL Reference Guide", 0, 0);
    }

    return ERROR_SUCCESS;
}

/* Name: ingres_create_vista_shortcut
**
** Description:
**
**     Create the Vista elevated Privilege command prompt
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**      08-Feb-2007 (horda03)
**          Created.
**	04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    Update routines to be able to handle DBA Tools install.
**	    Add Vista shortcut for VDBA tools.
**	12-Feb-2009 (drivi01)
**	    Fix "Windows Administrator Command Prompt" for DBA Tools package
**	    to be installed under "All Users" Start Menu, otherwise
**          it doesn't get removed on uninstall.
**	
*/
UINT __stdcall
ingres_create_vista_shortcut(MSIHANDLE hInstall)
{
    DWORD dwSize;
    char szIngAllUsers[2], szId[3], szII_SYSTEM[MAX_PATH], szBuf[MAX_PATH];
    char szFolder[MAX_PATH], szTarget[MAX_PATH], szArgs[1024];
    char szSystemFolder[MAX_PATH];
    int isDBATools = FALSE;

    if (!IsVista()) return ERROR_SUCCESS;

    dwSize = sizeof(szBuf);
    if (MsiGetProperty(hInstall, "DBATools", szBuf, &dwSize ) != ERROR_SUCCESS && szBuf[0])
    {
	return (ERROR_INSTALL_FAILURE);
    }
    if(!strcmp(szBuf, "1"))
	isDBATools = TRUE;

    dwSize = sizeof(szId);
    if (MsiGetProperty(hInstall, "II_INSTALLATION", szId, &dwSize ) != ERROR_SUCCESS)
    {
        return (ERROR_INSTALL_FAILURE);
    }

    dwSize = sizeof(szIngAllUsers);
    if (isDBATools)
    {
	if (MsiGetProperty(hInstall, "ALLUSERS", szIngAllUsers, &dwSize ) != ERROR_SUCCESS)
	{
	     return (ERROR_INSTALL_FAILURE);
	}
    }
    else
    {
	if (MsiGetProperty(hInstall, "INGRES_ALLUSERS", szIngAllUsers, &dwSize ) != ERROR_SUCCESS)
	{
             return (ERROR_INSTALL_FAILURE);
	}
    }
    if (isDBATools && !strcmp(szId, "VT"))
    sprintf(szFolder, "Ingres\\DBA Tools");
    else if (isDBATools && strcmp(szId, "VT"))
    sprintf(szFolder, "Ingres\\DBA Tools %s", szId);
    else
    sprintf(szFolder, "Ingres\\Ingres %s", szId);

    if (!isDBATools)
    {
    	dwSize = sizeof(szII_SYSTEM);
	if (MsiGetProperty(hInstall, "INGRES_II_SYSTEM", szII_SYSTEM, &dwSize ) != ERROR_SUCCESS)
	{
        	return (ERROR_INSTALL_FAILURE);
    	}
    }
    else
    {
	dwSize=sizeof(szII_SYSTEM);
	if (MsiGetTargetPath(hInstall, "INSTALLDIR", szII_SYSTEM, &dwSize) != ERROR_SUCCESS)
		return (ERROR_INSTALL_FAILURE);
    }

    /* On Vista, need to create a second command prompt to start with
    ** administrator privileges.
    */

    GetSystemDirectory(szSystemFolder, sizeof(szSystemFolder));

    sprintf(szTarget, "%s\\CMD.EXE", szSystemFolder);
    sprintf(szArgs, "/K \"%s\\ingres\\bin\\setingenvs.bat\"", 
            szII_SYSTEM);

#define VISTA_CMD_PROMPT "Ingres Administrator Command Prompt"
#define DBA_VISTA_CMD_PROMPT "Windows Administrator Command Prompt"
    if (isDBATools)
	    CreateOneAdminShortcut(szFolder, DBA_VISTA_CMD_PROMPT, szTarget, szArgs,
                      "", "%HOMEDRIVE%%HOMEPATH%",
              "DOS command window with elevated privileges and Ingres environment settings.",
                      0, (!strcmp(szIngAllUsers, "1")) ? 1 : 0);	
    else
	    CreateOneAdminShortcut(szFolder, VISTA_CMD_PROMPT, szTarget, szArgs,
                      "", "%HOMEDRIVE%%HOMEPATH%",
              "DOS command window with elevated privileges and Ingres environment settings.",
                      0, (!strcmp(szIngAllUsers, "1")) ? 1 : 0);

    return ERROR_SUCCESS;
}

/*
**  Name:ingres_check_installed_features
**
**  Description:
**	List all installed Ingres features and save them in
**	the property INGRES_INSTALLEDFEATURES.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	16-jun-2003 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_check_installed_features(MSIHANDLE hInstall)
{
    char szBuf[1024];
    DWORD dwSize;
    INSTALLSTATE iInstalled, iAction;
    int i;
    char szProduct[39], FeatureID[MAX_FEATURE_CHARS];
    UINT uiRet;

    dwSize=sizeof(szProduct);
    MsiGetProperty(hInstall, "ProductCode", szProduct, &dwSize);

    i=0; *szBuf=0;
    uiRet=ERROR_SUCCESS;
    while(uiRet==ERROR_SUCCESS)
    {
	*FeatureID=0;
	uiRet=MsiEnumFeatures(szProduct, i, FeatureID, NULL);
	if (uiRet==ERROR_SUCCESS )
	{
	    if (strstr(FeatureID, "Ingres"))
	    {
		MsiGetFeatureState(hInstall, FeatureID, &iInstalled, &iAction);
		if(iInstalled==INSTALLSTATE_LOCAL)
		{
		    if(!szBuf[0]) strcpy(szBuf, FeatureID);
		    else {strcat(szBuf, ","); strcat(szBuf, FeatureID);}
		}
	    }
	    i++;
	}
    }//while

    if (szBuf[0])
	MsiSetProperty(hInstall, "INGRES_INSTALLEDFEATURES", szBuf);

    return ERROR_SUCCESS;
}

/*
**  Name: embedingres_set_target_path
**
**  Description:
**	Set the destination folder of Ingres, INGRESFOLDER. If this is
**	an upgrade, keep its old value.
**	ONLY used by the product that embeds Ingres merge moudles.
**
**  History:
**	16-Oct-2003 (penga03)
**	    Created.
*/
UINT __stdcall
embedingres_set_target_path(MSIHANDLE hInstall)
{
    char id[3], path[MAX_PATH], cur_id[3], cur_path[MAX_PATH];
    DWORD dwSize;
    char RegKey[255];
    HKEY hKey;

    dwSize=sizeof(id); *id=0;
    if (MsiGetProperty(hInstall, "II_INSTALLATION", id, &dwSize))
	return ERROR_SUCCESS;

    dwSize=sizeof(path); *path=0;
    MsiGetTargetPath(hInstall, "INGRESFOLDER", path, &dwSize);
    if (path[strlen(path)-1] == '\\')
	path[strlen(path)-1] = '\0';

    if (!Local_NMgtIngAt("II_INSTALLATION", cur_id)
		&& !_stricmp(cur_id, id)
		&& GetEnvironmentVariable("II_SYSTEM", cur_path, sizeof(cur_path))
		&& _stricmp(cur_path, path))
    {
	MsiSetTargetPath(hInstall, "INGRESFOLDER", cur_path);
	return ERROR_SUCCESS;
    }

    sprintf(RegKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", id);
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_QUERY_VALUE, &hKey))
    {
	dwSize=sizeof(cur_path);
	if (!RegQueryValueEx(hKey,"II_SYSTEM", NULL, NULL, (BYTE *)cur_path, &dwSize)
		&& _stricmp(cur_path, path))
	{
	    MsiSetTargetPath(hInstall, "INGRESFOLDER", cur_path);
	    RegCloseKey(hKey);
	    return ERROR_SUCCESS;
	}
	RegCloseKey(hKey);
    }

    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation", id);
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_QUERY_VALUE, &hKey))
    {
	dwSize=sizeof(cur_path);
	if (!RegQueryValueEx(hKey,"II_SYSTEM", NULL, NULL, (BYTE *)cur_path, &dwSize)
		&& _stricmp(cur_path, path))
	{
	    MsiSetTargetPath(hInstall, "INGRESFOLDER", cur_path);
	    RegCloseKey(hKey);
	    return ERROR_SUCCESS;
	}
	RegCloseKey(hKey);
    }

    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Advantage Ingres\\%s_Installation", id);
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_QUERY_VALUE, &hKey))
    {
	dwSize=sizeof(cur_path);
	if (!RegQueryValueEx(hKey, "II_SYSTEM", NULL, NULL, (BYTE *)cur_path, &dwSize)
		&& _stricmp(cur_path, path))
	{
	    MsiSetTargetPath(hInstall, "INGRESFOLDER", cur_path);
	    RegCloseKey(hKey);
	    return ERROR_SUCCESS;
	}
	RegCloseKey(hKey);
    }

    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII\\%s_Installation", id);
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_QUERY_VALUE, &hKey))
    {
	dwSize=sizeof(cur_path);
	if (!RegQueryValueEx(hKey, "II_SYSTEM", NULL, NULL, (BYTE *)cur_path, &dwSize)
		&& _stricmp(cur_path, path))
	{
	    MsiSetTargetPath(hInstall, "INGRESFOLDER", cur_path);
	    RegCloseKey(hKey);
	    return ERROR_SUCCESS;
	}
	RegCloseKey(hKey);
    }
    return ERROR_SUCCESS;
}

/*
**  Name: embedingres_verify_instance
**
**  Description:
**	Given II_INSTALLATION and II_SYSTEM (INGRESFOLDER), verify if
**	it is eligible to install/upgrade.
**	Its custom action can ONLY used in the Execute sequence.
**
**  History:
**	16-Oct-2003 (penga03)
**	    Created.
*/
UINT __stdcall
embedingres_verify_instance(MSIHANDLE hInstall)
{
    char id[3], path[MAX_PATH];
    DWORD dwSize;
    int nRet=0;

    MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");

    dwSize=sizeof(id); *id=0;
    MsiGetProperty(hInstall, "II_INSTALLATION", id, &dwSize);

    dwSize=sizeof(path); *path=0;
    MsiGetTargetPath(hInstall, "INGRESFOLDER", path, &dwSize);
    if (path[strlen(path)-1] == '\\')
	path[strlen(path)-1] = '\0';

    nRet=VerifyInstance(hInstall, id, path);
    if (nRet) //nRet==-1 or nRet==1
	return (ERROR_INSTALL_FAILURE);

    return (ERROR_SUCCESS);
}

/*
**  Name: embedingres_verify_instance_ui
**
**  Description:
**	The same at the custom action embedingres_verify_instance,
**	except it can only be called as a control event.
**
**  History:
**	24-Nov-2003 (penga03)
**	    Created.
*/
UINT __stdcall
embedingres_verify_instance_ui(MSIHANDLE hInstall)
{
    char id[3], path[MAX_PATH];
    DWORD dwSize;
    int nRet=0;

    MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");

    dwSize=sizeof(id); *id=0;
    MsiGetProperty(hInstall, "II_INSTALLATION", id, &dwSize);

    dwSize=sizeof(path); *path=0;
    MsiGetTargetPath(hInstall, "INGRESFOLDER", path, &dwSize);
    if (path[strlen(path)-1] == '\\')
	path[strlen(path)-1] = '\0';

    nRet=VerifyInstance(hInstall, id, path);
    if (nRet==-1)
	return (ERROR_INSTALL_FAILURE);
    if (nRet==1)
	MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");

    return (ERROR_SUCCESS);
}

/*
**  Name: embedingres_prepare_upgrade
**
**  Description:
**	Set values for those properties that need to keep their old
**	values.
**	ONLY used by the product that embeds Ingres merge moudles.
**
**  History:
**	16-Oct-2003 (penga03)
**	    Created.
*/
UINT __stdcall
embedingres_prepare_upgrade(MSIHANDLE hInstall)
{
    char id[3], ii_system[MAX_PATH];
    DWORD dwSize;
    LPITEMIDLIST pidlProg, pidlComProg;
    char szPath[MAX_PATH], szPath2[MAX_PATH];
    char szTemp1[MAX_PATH], szTemp2[MAX_PATH];
    char szTemp3[MAX_PATH], szTemp4[MAX_PATH];
    INSTALLSTATE	iInstalled, iAction;
    BOOL bDBMS=0, bNet=0;
    LPMALLOC pMalloc = NULL;

    if (!MsiGetFeatureState(hInstall, "IngresDBMS", &iInstalled, &iAction) &&
		iAction==INSTALLSTATE_LOCAL)
	bDBMS=1;
    if (!MsiGetFeatureState(hInstall, "IngresNet", &iInstalled, &iAction) &&
		iAction==INSTALLSTATE_LOCAL)
	bNet=1;

    if (!bDBMS && !bNet)
	return ERROR_SUCCESS;

    dwSize=sizeof(id); *id=0;
    MsiGetProperty(hInstall, "II_INSTALLATION", id, &dwSize);

    dwSize=sizeof(ii_system); *ii_system=0;
    MsiGetTargetPath(hInstall, "INGRESFOLDER", ii_system, &dwSize);
    if (ii_system[strlen(ii_system)-1] == '\\')
	ii_system[strlen(ii_system)-1] = '\0';

    /* Set INGRES_II_SYSTEM.*/
    MsiSetProperty(hInstall, "INGRES_II_SYSTEM", ii_system);

    /* Set INGRES_VER25.*/
    MsiSetProperty(hInstall, "INGRES_VER25", "0");
    if (CheckVer25(id, ii_system))
	MsiSetProperty(hInstall, "INGRES_VER25", "1");

    /* Set INGRES_SHORTCUT and INGRES_ALLUSERS. */
    _tcslwr(ii_system);
    SHGetMalloc(&pMalloc);

    SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlProg);
    SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlComProg);

    MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "0");
    MsiSetProperty(hInstall, "INGRES_ALLUSERS", "1");

    SHGetPathFromIDList(pidlProg, szPath);
	SHGetPathFromIDList(pidlComProg, szPath2);
    sprintf(szTemp1, "%s\\Ingres\\Ingres %s", szPath, id);
    sprintf(szTemp2, "%s\\Ingres\\Ingres %s", szPath2, id);
    if (GetFileAttributes(szTemp1)!=-1 || GetFileAttributes(szTemp2)!=-1)
	MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
    else
    {
    sprintf(szTemp1, "%s\\Computer Associates\\Ingres [ %s ]", szPath, id);
    sprintf(szTemp2, "%s\\Computer Associates\\Ingres [ %s ]", szPath2, id);
    if (GetFileAttributes(szTemp1)!=-1 || GetFileAttributes(szTemp2)!=-1)
	MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
    else
    {
	sprintf(szTemp1, "%s\\Ingres\\Ingres [ %s ]", szPath, id);
	sprintf(szTemp2, "%s\\Ingres\\Ingres [ %s ]", szPath2, id);
	if (GetFileAttributes(szTemp1)!=-1 || GetFileAttributes(szTemp2)!=-1)
	    MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
	else
	{
	    sprintf(szTemp1, "%s\\Computer Associates\\Advantage\\Ingres [ %s ]", szPath, id);
	    sprintf(szTemp2, "%s\\Computer Associates\\Advantage\\Ingres [ %s ]", szPath2, id);
	    if (GetFileAttributes(szTemp1)!=-1 || GetFileAttributes(szTemp2)!=-1)
	        MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
	    else
	    {
	        sprintf(szTemp1, "%s\\Ingres II [ %s ]", szPath, id);
	        sprintf(szTemp2, "%s\\Ingres II [ %s ]", szPath2, id);
	        if (GetFileAttributes(szTemp1)!=-1 || GetFileAttributes(szTemp2)!=-1)
		    MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
	        else
	        {
		    sprintf(szTemp1, "%s\\Ingres II\\Ingres Service Manager.lnk", szPath);
		    sprintf(szTemp2, "%s\\Ingres II\\Ingres Service Manager.lnk", szPath2);
		    if (GetLinkInfo(szTemp1, szTemp3) && _tcsstr(szTemp3, ii_system) || GetLinkInfo(szTemp2, szTemp4) && _tcsstr(szTemp4, ii_system))
		        MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
		    else
		    {
		       sprintf(szTemp1, "%s\\Ingres II\\Start Ingres.lnk", szPath);
		       sprintf(szTemp2, "%s\\Ingres II\\Start Ingres.lnk", szPath2);
		       if (GetLinkInfo(szTemp1, szTemp3) && _tcsstr(szTemp3, ii_system) || GetLinkInfo(szTemp2, szTemp4) && _tcsstr(szTemp4, ii_system))
			   MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
		     }
		}
	    }
	}
	}
    }

    /* Setting INGRES_ALLUSERS */
    SHGetPathFromIDList(pidlProg, szPath);
    sprintf(szTemp1, "%s\\Ingres\\Ingres %s", szPath, id);
    if (GetFileAttributes(szTemp1)!=-1)
	MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
    else
    {
    sprintf(szTemp1, "%s\\Ingres\\Ingres [ %s ]", szPath, id);
    if (GetFileAttributes(szTemp1)!=-1)
	MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
    else
    {
    SHGetPathFromIDList(pidlProg, szPath);
    sprintf(szTemp1, "%s\\Computer Associates\\Ingres [ %s ]", szPath, id);
    if (GetFileAttributes(szTemp1)!=-1)
	MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
    else
    {
	sprintf(szTemp1, "%s\\Computer Associates\\Advantage\\Ingres [ %s ]", szPath, id);
	if (GetFileAttributes(szTemp1)!=-1)
	    MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
	else
	{
	    sprintf(szTemp1, "%s\\Ingres II [ %s ]", szPath, id);
	    if (GetFileAttributes(szTemp1)!=-1)
		MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
	    else
	    {
		sprintf(szTemp1, "%s\\Ingres II\\Ingres Service Manager.lnk", szPath);
		if (GetLinkInfo(szTemp1, szTemp3) && _tcsstr(szTemp3, ii_system))
		    MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
		else
		{
		    sprintf(szTemp1, "%s\\Ingres II\\Start Ingres.lnk", szPath);
		    if (GetLinkInfo(szTemp1, szTemp3) && _tcsstr(szTemp3, ii_system))
			MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
		}
	    }
	}
    }
    }
    }

    if (pidlComProg)
	pMalloc->lpVtbl->Free(pMalloc, pidlComProg);
    if (pidlProg)
	pMalloc->lpVtbl->Free(pMalloc, pidlProg);

    pMalloc->lpVtbl->Release(pMalloc);
    return ERROR_SUCCESS;
}

/*
**  Name: ingres_set_date_manual
**
**  Description:
**	If this is an upgrade, get time zone from
**	%II_SYSTEM%\ingres\files\symbol.tbl, otherwise use the value
**	of II_DATE_FORMAT that is set in the Property table.
**
**  History:
**	26-jan-2004 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_set_date_manual(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1];
    DWORD dwSize;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);
	Local_NMgtIngAt("II_DATE_FORMAT", szBuf);
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_DATE_FORMAT", szBuf);
	return (ERROR_SUCCESS);
    }

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "INGRES_DATE_FORMAT", szBuf, &dwSize);
    if (!szBuf[0] || !VerifyIngDateFormat(szBuf))
	MsiSetProperty(hInstall, "INGRES_DATE_FORMAT", "US");
    return (ERROR_SUCCESS);
}

/*
**  Name: ingres_set_date_auto
**
**  Description:
**	If this is an upgrade, get time zone from
**	%II_SYSTEM%\ingres\files\symbol.tbl, otherwise get the
**	OS default date format and map it to the Ingres date
**	format.
**
**  History:
**	26-jan-2004 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_set_date_auto(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1];
    DWORD dwSize;
    HKEY hKey;
    char os_date_format[128];

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);
	Local_NMgtIngAt("II_DATE_FORMAT", szBuf);
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_DATE_FORMAT", szBuf);
	return (ERROR_SUCCESS);
    }

    /* get OS default date format */
    if(!RegOpenKeyEx(HKEY_CURRENT_USER,
                     "Control Panel\\International",
                     0, KEY_QUERY_VALUE, &hKey))
    {
	char szData[128];
	DWORD dwType, dwSize=sizeof(szData);

	if(!RegQueryValueEx(hKey, "sShortDate", NULL, &dwType, (BYTE *)szData, &dwSize))
	    lstrcpy(os_date_format, szData);
    }

    if (searchIngDateFormat(os_date_format, szBuf))
	MsiSetProperty(hInstall, "INGRES_DATE_FORMAT",szBuf);
    else
	MsiSetProperty(hInstall, "INGRES_DATE_FORMAT","US");
    return (ERROR_SUCCESS);
}

UINT __stdcall
ingres_set_money_manual(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1];
    DWORD dwSize;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);
	Local_NMgtIngAt("II_MONEY_FORMAT", szBuf);
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_MONEY_FORMAT", szBuf);
	return (ERROR_SUCCESS);
    }

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "INGRES_MONEY_FORMAT", szBuf, &dwSize);
    if (!szBuf[0] || !VerifyIngMoneyFormat(szBuf))
	MsiSetProperty(hInstall, "INGRES_MONEY_FORMAT", "L:$");

    return (ERROR_SUCCESS);
}

UINT __stdcall
ingres_set_money_auto(MSIHANDLE hInstall)
{
    char szCode[3], szBuf[MAX_PATH+1];
    DWORD dwSize;
    HKEY hKey;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);
    if (GetInstallPath(szCode, szBuf))
    {
	SetEnvironmentVariable("II_SYSTEM", szBuf);
	Local_NMgtIngAt("II_MONEY_FORMAT", szBuf);
    }
    if (szBuf[0])
    {
	MsiSetProperty(hInstall, "INGRES_MONEY_FORMAT", szBuf);
	return (ERROR_SUCCESS);
    }

    /* get OS default currency symbol */
    if(!RegOpenKeyEx(HKEY_CURRENT_USER,
                     "Control Panel\\International",
                     0, KEY_QUERY_VALUE, &hKey))
    {
	char szData[8];
	DWORD dwType, dwSize=sizeof(szData);

	if(!RegQueryValueEx(hKey, "sCurrency", NULL, &dwType, (BYTE *)szData, &dwSize))
	{
	    sprintf(szBuf, "L:%s", szData);
	    MsiSetProperty(hInstall, "INGRES_MONEY_FORMAT",szBuf);
	    return (ERROR_SUCCESS);
	}
    }

    MsiSetProperty(hInstall, "INGRES_MONEY_FORMAT", "L:$");
    return (ERROR_SUCCESS);
}

UINT __stdcall
ingres_verify_money_format(MSIHANDLE hInstall)
{
    char szMoneyFormat[7];
    DWORD dwSize;

    MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");

    dwSize=sizeof(szMoneyFormat);
    MsiGetProperty(hInstall, "INGRES_MONEY_FORMAT", szMoneyFormat, &dwSize);
    if (!VerifyIngMoneyFormat(szMoneyFormat))
    {
	MyMessageBox(hInstall, "Ingres money format must be a string with two symbols separated by a colon. The symbol to the left of the colon must be 'L' or 'T'. To the right of the colon put the currency symbol you want displayed. Currency symbols may contain up to 4 physical characters.");
	MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
    }
    return (ERROR_SUCCESS);
}

/*
**  Name: ingres_remove_3rdparty_software
**
**  Description:
**	Remove the 3rd partys' software installed by Ingres.
**
**  History:
**	19-Feb-2004 (penga03)
**	    Created. Remove JRE.
**	05-May-2004 (penga03)
**	    Reversed the previous change.
*/
UINT __stdcall
ingres_remove_3rdparty_software(MSIHANDLE hInstall)
{
    return (ERROR_SUCCESS);
}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
    TCHAR buf[100];

    GetClassName(hwnd, buf, sizeof(buf));
    if (_tcscmp(buf, (_T("RichEdit20W"))) == 0)
    {
	*(HWND*)lParam = hwnd;
	return FALSE;
    }
    return TRUE;
}

/*
**  Name: ingres_browseforfile
**
**  Description:
**	Creates a "Save As" dialog box that lets the user specify the
**	name of a response file.
**
**  History:
**	08-jun-2004 (penga03)
**	    Created.
**	13-Mar-2007 (drivi01)
**	    Update the Window Title to reflect current installer title.
**	    Added routine to append .rsp at the end of the response file if
**	    the suffix got lost.
*/
UINT __stdcall
ingres_browseforfile(MSIHANDLE hInstall)
{
    TCHAR szRSPFile[MAX_PATH], szProdName[256], szBuf[1024];
    DWORD cchValue;
    OPENFILENAME ofn;
    TCHAR szFilters[] = _T("Ingres Response File (*.rsp)\0*.rsp\0");

    cchValue = sizeof(szRSPFile)/sizeof(TCHAR);
    ZeroMemory(szRSPFile, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_RSP_FILE"), szRSPFile, &cchValue);

    cchValue = sizeof(szProdName)/sizeof(TCHAR);
    ZeroMemory(szProdName, sizeof(TCHAR)*256);
    MsiGetProperty(hInstall, TEXT("ProductName"), szProdName, &cchValue);

    /* Initialize OPENFILENAME structure. */
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetForegroundWindow();
    ofn.lpstrFile = szRSPFile;
    ofn.nMaxFile = sizeof(szRSPFile);
    ofn.lpstrFilter = szFilters;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn))
    {
	HWND hWnd;
	HWND hWndChild=NULL;

	if (strstr(szRSPFile, ".rsp") == NULL)
		strcat(szRSPFile, ".rsp");
	MsiSetProperty(hInstall, TEXT("INGRES_RSP_FILE"), szRSPFile);

	_stprintf(szBuf, "%s - Installation Wizard", szProdName);
	hWnd = FindWindow(NULL, szBuf);
	if (hWnd)
	{
	    EnumChildWindows( hWnd, EnumChildProc, (LPARAM)&hWndChild );
	    if ( hWndChild ) {
		_tcscpy(szBuf, szRSPFile);
		SendMessage(hWndChild, WM_SETTEXT, 0, (LPARAM)szRSPFile);
	    }
	}
    }
    return ERROR_SUCCESS;
}

/*
**  Name: ingres_create_rspfile
**
**  Description:
**	Create a response file.
**
**  History:
**	08-jun-2004 (penga03)
**	    Created.
**	20-Feb-2007 (karye01)
**	    Modified to re-structure response file into 4 sub-parts, and
**	    also changing names of each property to a new names to bring
**	    it inline with all platforms.
**	28-May-2008 (drivi01)
**	    Added ODBC read-only option to the response file.
**	14-Jul-2009 (drivi01)
**	    Add the new II_CONFIG_TYPE parameter to the response file
**	    generation.  Supports 3 options TXN, CM, BI.  
**	    II_CONFIG_TYPE=NULL or the absence of II_CONFIG_TYPE parameter
**          will revert back to Classic/Traditional Ingres configuration.
**	11-Aug-2009 (drivi01)
**	    Fix II_LOCATION_DOCUMENTATION to put together Documentation 
**	    location instead of retrieving it from InresII_DBL.ism.
**	    The routine will now retrieve INGRESCORPFOLDER and then
**	    will append documentation folder on it and use variable
**	    to reflect version.  This is done to eliminate the need
**	    to update documentation path in IngresII_DBL.ism every
**	    time the version of the product increases. 
**	12-Aug-2009 (drivi01)
**	    Add II_COMPONENT_SPATIAL to the routines that generate
**	    response file.
**	27-Aug-2010 (drivi01)
**	    Remove II_COMPONENT_ICE.  We don't want it added anymore
**	    to generated response file.
*/
UINT __stdcall
ingres_create_rspfile(MSIHANDLE hInstall)
{
    TCHAR szRSPFile[MAX_PATH], szBuf[MAX_PATH];
    DWORD cchValue;
    FILE *fp;
    time_t ltime;
    INSTALLSTATE iInstalled, iAction;
    int bDBMSInstalled, bNetInstalled;

    cchValue = sizeof(szRSPFile)/sizeof(TCHAR);
    ZeroMemory(szRSPFile, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_RSP_FILE"), szRSPFile, &cchValue);

    if ((fp=fopen(szRSPFile, "w")) == NULL)
    {
	MyMessageBox(hInstall, "Error: Could not open the response file!");
	return ERROR_SUCCESS;
    }

    time(&ltime);

    fprintf(fp, "; Ingres Response File \n");
    fprintf(fp, "; Generated by the Ingres Response File API \n");
    fprintf(fp, "; Created: %s\n\n", ctime(&ltime) );
    fprintf(fp, "[Ingres Components]\n\n" );

    bDBMSInstalled=bNetInstalled=0;
    if (!MsiGetFeatureState(hInstall, "IngresDBMS", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
    {
	bDBMSInstalled=1;
	fprintf(fp, "II_COMPONENT_DBMS=\"YES\"\n" );
    }
    else
	fprintf(fp, "II_COMPONENT_DBMS=\"NO\"\n" );

	if (!MsiGetFeatureState(hInstall, "IngresNet", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	{
	bNetInstalled=1;
	fprintf(fp, "II_COMPONENT_NET=\"YES\"\n" );
	}
	else
	fprintf(fp, "II_COMPONENT_NET=\"NO\"\n" );

    if (!MsiGetFeatureState(hInstall, "IngresDoc", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_COMPONENT_DOCUMENTATION=\"YES\"\n" );
    else
	fprintf(fp, "II_COMPONENT_DOCUMENTATION=\"NO\"\n" );

    if (!MsiGetFeatureState(hInstall, "IngresDotNETDataProvider", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_COMPONENT_DOTNET=\"YES\"\n" );
    else
	fprintf(fp, "II_COMPONENT_DOTNET=\"NO\"\n" );

    if (!MsiGetFeatureState(hInstall, "IngresTools", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_COMPONENT_FRONTTOOLS=\"YES\"\n" );
    else
	fprintf(fp, "II_COMPONENT_FRONTTOOLS=\"NO\"\n" );

    if (!MsiGetFeatureState(hInstall, "IngresVision", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_COMPONENT_VISION=\"YES\"\n" );
    else
	fprintf(fp, "II_COMPONENT_VISION=\"NO\"\n" );

    if (!MsiGetFeatureState(hInstall, "IngresReplicator", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_COMPONENT_REPLICATOR=\"YES\"\n" );
    else
	fprintf(fp, "II_COMPONENT_REPLICATOR=\"NO\"\n" );

    if (!MsiGetFeatureState(hInstall, "IngresStar", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_COMPONENT_STAR=\"YES\"\n" );
    else
	fprintf(fp, "II_COMPONENT_STAR=\"NO\"\n" );

    if (!MsiGetFeatureState(hInstall, "IngresSpat", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_COMPONENT_SPATIAL=\"YES\"\n");
    else
	fprintf(fp, "II_COMPONENT_SPATIAL=\"NO\"\n");


    if (bDBMSInstalled || bNetInstalled)
    {
	fprintf(fp, "II_COMPONENT_JDBC_CLIENT=\"YES\"\n\n" );
    }
    else
    {
	fprintf(fp, "II_COMPONENT_JDBC_CLIENT=\"NO\"\n\n" );
    }

	fprintf(fp, "[Ingres Locations]\n\n" );


    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetTargetPath(hInstall, TEXT("INGRESFOLDER"), szBuf, &cchValue);
    if (szBuf[strlen(szBuf)-1] == '\\')
	szBuf[strlen(szBuf)-1] = '\0';
    fprintf(fp, "II_SYSTEM=\"%s\"\n", szBuf);


    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_DATADIR"), szBuf, &cchValue);
    fprintf(fp, "II_DATABASE=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_CKPDIR"), szBuf, &cchValue);
    fprintf(fp, "II_CHECKPOINT=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_JNLDIR"), szBuf, &cchValue);
    fprintf(fp, "II_JOURNAL=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_WORKDIR"), szBuf, &cchValue);
    fprintf(fp, "II_WORK=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_DMPDIR"), szBuf, &cchValue);
    fprintf(fp, "II_DUMP=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_PRIMLOGDIR"), szBuf, &cchValue);
    fprintf(fp, "II_LOG_FILE=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    if (!MsiGetProperty(hInstall, TEXT("INGRES_ENABLEDUALLOG"), szBuf, &cchValue)
	&& !_stricmp(szBuf, "1"))
	{
	    cchValue = sizeof(szBuf)/sizeof(TCHAR);
	    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
	    MsiGetProperty(hInstall, TEXT("INGRES_DUALLOGDIR"), szBuf, &cchValue);
	    fprintf(fp, "II_DUAL_LOG=\"%s\"\n", szBuf);
	}
	else	    
		fprintf(fp, "II_DUAL_LOG=\"\"\n");

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
	ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
	MsiGetProperty(hInstall, TEXT("INGRES_DOTNET"), szBuf, &cchValue);
    fprintf(fp, "II_LOCATION_DOTNET=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
	ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
	MsiGetProperty(hInstall, TEXT("INGRESCORPFOLDER"), szBuf, &cchValue);
    fprintf(fp, "II_LOCATION_DOCUMENTATION=\"%s\Ingres Documentation %s\\\"\n\n", szBuf, ING_VERS);


	fprintf(fp, "[Ingres Configuration]\n\n" );

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("II_INSTALLATION"), szBuf, &cchValue);
    fprintf(fp, "II_INSTALLATION=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_CHARSET"), szBuf, &cchValue);
    fprintf(fp, "II_CHARSET=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_TIMEZONE"), szBuf, &cchValue);
    fprintf(fp, "II_TIMEZONE_NAME=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_LANGUAGE"), szBuf, &cchValue);
    fprintf(fp, "II_LANGUAGE=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_TERMINAL"), szBuf, &cchValue);
    fprintf(fp, "II_TERMINAL=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_DATE_FORMAT"), szBuf, &cchValue);
    fprintf(fp, "II_DATE_FORMAT=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_DATE_TYPE"), szBuf, &cchValue);
    fprintf(fp, "II_DATE_TYPE_ALIAS=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_MONEY_FORMAT"), szBuf, &cchValue);
    fprintf(fp, "II_MONEY_FORMAT=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_LOGFILESIZE"), szBuf, &cchValue);
    fprintf(fp, "II_LOG_FILE_SIZE_MB=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    if (!MsiGetProperty(hInstall, TEXT("INGRES_ENABLESQL92"), szBuf, &cchValue)
	&& !_stricmp(szBuf, "1"))
	fprintf(fp, "II_ENABLE_SQL92=\"YES\"\n", szBuf);
    else
	fprintf(fp, "II_ENABLE_SQL92=\"NO\"\n", szBuf);


    fprintf(fp, "II_ADD_REMOVE_PROGRAMS=\"YES\"\n" );

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    if (!MsiGetProperty(hInstall, TEXT("INGRES_INCLUDEINPATH"), szBuf, &cchValue)
    && !_stricmp(szBuf, "1"))
    fprintf(fp, "II_ADD_TO_PATH=\"YES\"\n", szBuf);
    else
    fprintf(fp, "II_ADD_TO_PATH=\"NO\"\n", szBuf);


    fprintf(fp,"II_INSTALL_ALL_ICONS=\"YES\"\n" );


    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    if (!MsiGetProperty(hInstall, TEXT("INGRES_LEAVE_RUNNING"), szBuf, &cchValue)
    && !_stricmp(szBuf, "1"))
    fprintf(fp, "II_START_INGRES_ON_COMPLETE=\"YES\"\n", szBuf);
    else
    fprintf(fp, "II_START_INGRES_ON_COMPLETE=\"NO\"\n", szBuf);

    
    fprintf(fp, "II_START_IVM_ON_COMPLETE=\"YES\"\n");


    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    if (!MsiGetProperty(hInstall, TEXT("INGRES_SERVICEAUTO"), szBuf, &cchValue)
	&& !_stricmp(szBuf, "1"))
	fprintf(fp, "II_SERVICE_START_AUTO=\"YES\"\n", szBuf);
    else
	fprintf(fp, "II_SERVICE_START_AUTO=\"NO\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_SERVICELOGINID"), szBuf, &cchValue);
    fprintf(fp, "II_SERVICE_START_USER=\"%s\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    MsiGetProperty(hInstall, TEXT("INGRES_SERVICEPASSWORD"), szBuf, &cchValue);
    fprintf(fp, "II_SERVICE_START_USERPASSWORD=\"%s\"\n", szBuf);


    if (!MsiGetFeatureState(hInstall, "IngresTCPIP", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_ENABLE_WINTCP=\"YES\"\n" );
    else
	fprintf(fp, "II_ENABLE_WINTCP=\"NO\"\n" );

    if (!MsiGetFeatureState(hInstall, "IngresNetBIOS", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_ENABLE_NETBIOS=\"YES\"\n" );
    else
	fprintf(fp, "II_ENABLE_NETBIOS=\"NO\"\n" );


    if (!MsiGetFeatureState(hInstall, "IngresNovellSPXIPX", &iInstalled, &iAction)
	&& (iInstalled == INSTALLSTATE_LOCAL || iAction == INSTALLSTATE_LOCAL))
	fprintf(fp, "II_ENABLE_SPX=\"YES\"\n" );
    else
	fprintf(fp, "II_ENABLE_SPX=\"NO\"\n" );


    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    if (!MsiGetProperty(hInstall, TEXT("INGRES_INSTALLDEMODB"), szBuf, &cchValue)
	&& !_stricmp(szBuf, "1"))
	fprintf(fp, "II_DEMODB_CREATE=\"YES\"\n", szBuf);
    else
	fprintf(fp, "II_DEMODB_CREATE=\"NO\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    if (!MsiGetProperty(hInstall, TEXT("INGRES_ODBC_READONLY"), szBuf, &cchValue)
	&& !_stricmp(szBuf, "1"))
	fprintf(fp, "II_SETUP_ODBC_READONLY=\"YES\"\n", szBuf);
    else
	fprintf(fp, "II_SETUP_ODBC_READONLY=\"NO\"\n", szBuf);

    cchValue = sizeof(szBuf)/sizeof(TCHAR);
    ZeroMemory(szBuf, sizeof(TCHAR)*MAX_PATH);
    if (!MsiGetProperty(hInstall, TEXT("CONFIG_TYPE"), szBuf, &cchValue))
    {
	if (!_stricmp(szBuf, "0"))
	     fprintf(fp, "II_CONFIG_TYPE=\"TXN\"\n", szBuf);
	if (!_stricmp(szBuf, "1"))
	     fprintf(fp, "II_CONFIG_TYPE=\"TRAD\"\n", szBuf);
	if (!_stricmp(szBuf, "2"))
	     fprintf(fp, "II_CONFIG_TYPE=\"BI\"\n", szBuf);
	if (!_stricmp(szBuf, "3"))
	     fprintf(fp, "II_CONFIG_TYPE=\"CM\"\n", szBuf);
    }

    fprintf(fp, "\n[User Defined Properties]\n\n" );


    fclose(fp);

    sprintf(szBuf, "The response file '%s' has been successfully created.  Do you want to view it now?", szRSPFile);
    if (AskUserYN(hInstall, szBuf))
    {
	sprintf(szBuf, "notepad.exe \"%s\"", szRSPFile);
	Execute(szBuf, TRUE);
    }

    return ERROR_SUCCESS;
}


void
SetOneEnvironmentVariable(char *szEnv, char *szValue)
{
    char szBuf[4086], szTemp[4086];
    HKEY hKey;
    DWORD dwSize, dwType;

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
		0, KEY_ALL_ACCESS, &hKey))
    {
	dwSize=sizeof(szTemp);
	if (!RegQueryValueEx(hKey, szEnv, NULL, &dwType,(BYTE *)szTemp, &dwSize))
	{
	    if (!strstr(szTemp, szValue))
	    {
		strcpy(szBuf, szValue);
		strcat(szBuf, ";");
		strcat(szBuf, szTemp);
		RegSetValueEx(hKey, szEnv, 0, dwType, (BYTE *)szBuf, strlen(szBuf)+1);
	    }
	}
	else
	{
	    RegSetValueEx(hKey, szEnv, 0, REG_EXPAND_SZ, (BYTE *)szValue, strlen(szValue)+1);
	}
	RegCloseKey(hKey);
    }
}

/*
**  Name: ingres_installfinalize
**
**  Description:
**	This function contains all operations needed before launching
**	iipostinst. This is a type 1 custom action.
**
**  History:
**	11-nov-2004 (penga03)
**	    Created. Update %ii_system%\ingres\bin\setingenvs.bat.
**	30-nov-2004 (penga03)
**	    Restore Ingres environment variables.
**	24-Jul-2007 (drivi01)
**	    Added additional registry entry for Vista only.
**	    It will force Administrator command prompt shortcut to use
**	    Lucida Console font.
**	04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    Update routines to be able to handle DBA Tools install.
**	    Account for service name change in DBA Tools install.
**	16-Jan-2009 (whiro01) SD issue 132899
**	    Create a new "perfmonwrap.bat" file in the post install step
**	    that will be invoked by the "Monitor Ingres Cache Statistics" menu item
**	    that sets up the environment, loads the perf DLL and the counters
**	    before invoking "perfmon" with the given parameter.
*/
UINT __stdcall
ingres_installfinalize(MSIHANDLE hInstall)
{
    char szBuf[1024], szII_SYSTEM[1024], szBuf2[1024];
    char szSystemFolder[MAX_PATH];
    FILE *fp;
    int total, i;
    HKEY hKey;
    DWORD dwSize, dwDisposition;
    BOOL bJapanese=0;
    BOOL bRepair, bUpgrade, DBMSInstalled;
    char szId[3], szServiceName[512];
    SC_HANDLE schSCManager, schService;
    int isDBATools = TRUE;

    dwSize = sizeof(szBuf);
    if (MsiGetProperty( hInstall, "DBATools", szBuf, &dwSize)==ERROR_SUCCESS && szBuf[0])
	isDBATools=TRUE;

    dwSize=sizeof(szII_SYSTEM);
    if (MsiGetTargetPath(hInstall, "INGRESFOLDER", szII_SYSTEM, &dwSize))
	return (ERROR_SUCCESS);

    dwSize=sizeof(szId);
    if (MsiGetProperty(hInstall, "II_INSTALLATION", szId, &dwSize))
	return (ERROR_SUCCESS);

    if (szII_SYSTEM[strlen(szII_SYSTEM)-1] == '\\')
	szII_SYSTEM[strlen(szII_SYSTEM)-1] = '\0';

    /* repair Ingres registry entry and service */
    bRepair=0;
    dwSize=sizeof(szBuf);
    if (!MsiGetProperty(hInstall, "REINSTALL", szBuf, &dwSize)
	   && (!_stricmp(szBuf, "ALL") || strstr(szBuf, "IngresDBMS") || strstr(szBuf, "IngresNet")))
	bRepair=1;

    bUpgrade=0;
    dwSize=sizeof(szBuf);
    if (!MsiGetProperty(hInstall, "INGRES_UPGRADE", szBuf, &dwSize) && (!_stricmp(szBuf, "1") || !_stricmp(szBuf, "2")))
	bUpgrade=1;

    if (bRepair && !bUpgrade)
    {
	sprintf(szBuf, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", szId);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hKey)
		|| RegQueryValueEx(hKey, "installationcode", NULL, NULL, NULL, NULL)
		|| RegQueryValueEx(hKey, "II_SYSTEM", NULL, NULL, NULL, NULL))
	{
	    if (!RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition))
	    {
		RegSetValueEx(hKey, "installationcode", 0, REG_SZ, szId, strlen(szId) + 1 );
		RegSetValueEx(hKey, "II_SYSTEM", 0, REG_SZ, szII_SYSTEM, strlen(szII_SYSTEM) + 1 );
	    }
	}

	if (isDBATools)
	     sprintf(szServiceName, "Ingres_DBATools_%s", szId);
	else
	     sprintf(szServiceName, "Ingres_Database_%s", szId);

	if (!CheckServiceExists(szServiceName))
	{
	    DBMSInstalled=0;
	    sprintf(szBuf, "%s\\ingres\\bin\\iidbms.exe", szII_SYSTEM);
	    if (GetFileAttributes(szBuf) != -1)
		DBMSInstalled=1;

	    SetEnvironmentVariable("II_SYSTEM", szII_SYSTEM);
	    GetSystemDirectory(szBuf, sizeof(szBuf));
	    dwSize = sizeof(szBuf);
      	    sprintf(szBuf2, "%s\\ingres\\bin;%s\\ingres\\utility;%s", szII_SYSTEM, szII_SYSTEM, szBuf);
	    SetEnvironmentVariable("PATH", szBuf2);

	    if (DBMSInstalled)
		sprintf(szBuf, "opingsvc.exe install", szII_SYSTEM);
	    else
		sprintf(szBuf, "opingsvc.exe install client", szII_SYSTEM);

	    ExecuteEx(szBuf);

	    dwSize=sizeof(szBuf);
	    if (!MsiGetProperty(hInstall, "INGRES_CLUSTER_INSTALL", szBuf, &dwSize) && !_stricmp(szBuf, "1"))
	    {
		schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (schSCManager)
		{
		    schService=OpenService(schSCManager, szServiceName, SERVICE_CHANGE_CONFIG);
		    if (schService)
		    {
			ChangeServiceConfig(schService, SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, 0, 0, 0, 0, "LocalSystem", "", 0);
			CloseServiceHandle(schService);
		    }
		    CloseServiceHandle(schSCManager);
		}
	    }
	    else
	    {
		if (DBMSInstalled)
		    MyMessageBox(hInstall, "The Ingres service has been repaired. The service properties, including login user, password and startup type should be verified in the Windows Service Control Manager before restarting the service.");
	    }
	}//end of if (!CheckServiceExists(szServiceName))
    }

    sprintf(szBuf, "%s\\ingres\\files\\english\\ing30", szII_SYSTEM);
    sprintf(szBuf2, "%s\\ingres\\files\\english\\ing302", szII_SYSTEM);
    MoveFileEx(szBuf, szBuf2, MOVEFILE_REPLACE_EXISTING);

    sprintf(szBuf, "%s\\ingres\\bin\\setingenvs.bat", szII_SYSTEM);
    if ((fp=fopen(szBuf, "w")) == NULL)
	return (ERROR_SUCCESS);

    fprintf(fp, "@echo off\n" );

    dwSize=sizeof(szBuf);
    if (!MsiGetProperty(hInstall, "INGRES_CHARSET", szBuf, &dwSize))
    {
	if (!_stricmp(szBuf, "SHIFTJIS"))	bJapanese=1;
	total=sizeof(charsets02)/sizeof(ENTRY);
	for (i=0; i<total; ++i)
	{
	    if (!_stricmp(charsets02[i].key, szBuf))
	    {
		fprintf(fp, "MODE CON CP SELECT=%s > C:\\NUL\n", charsets02[i].value);
		break;
	    }
	}
    }
    else
	fprintf(fp, "MODE CON CP SELECT=1252 > C:\\NUL\n");

    _ftprintf(fp, "SET II_SYSTEM=%s\n", szII_SYSTEM);
    _ftprintf(fp, "SET PATH=%%II_SYSTEM%%\\ingres\\bin;%%II_SYSTEM%%\\ingres\\utility;%%PATH%%\n");
    _ftprintf(fp, "SET LIB=%%II_SYSTEM%%\\ingres\\lib;%%LIB%%\n");
    _ftprintf(fp, "SET INCLUDE=%%II_SYSTEM%%\\ingres\\files;%%INCLUDE%%\n");

    dwSize=sizeof(szBuf);
    if (!MsiGetProperty(hInstall, "INGRES_TERMINAL", szBuf, &dwSize))
	fprintf(fp, "SET TERM_INGRES=%s\n", szBuf);
    else
	fprintf(fp, "SET TERM_INGRES=IBMPCD\n");

    dwSize=sizeof(szBuf);
    if (!MsiGetProperty(hInstall, "II_INSTALLATION", szBuf, &dwSize))
	fprintf(fp, "TITLE Ingres %s\n", szBuf);
    else
	fprintf(fp, "TITLE Ingres\n");

    fclose(fp);

    if(!RegCreateKeyEx (HKEY_CURRENT_USER,
                       "Console\\Ingres Command Prompt",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL))
    {
	if (!bJapanese)
	    strcpy(szBuf, "Lucida Console");
	else
	    strcpy(szBuf, "");
	RegSetValueEx(hKey, "FaceName", 0, REG_SZ, (CONST BYTE *)szBuf, strlen(szBuf)+1);
	RegCloseKey(hKey);
    }

    if (IsVista())
    {
    if(!RegCreateKeyEx (HKEY_CURRENT_USER,
                       "Console\\Ingres Administrator Command Prompt",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL))
    {
	if (!bJapanese)
	    strcpy(szBuf, "Lucida Console");
	else
	    strcpy(szBuf, "");
	RegSetValueEx(hKey, "FaceName", 0, REG_SZ, (CONST BYTE *)szBuf, strlen(szBuf)+1);
	RegCloseKey(hKey);
    }
    }

    // Create the needed wrapper to setup the environment and load the counter stuff
    // before invoking "perfmon" to show the cache statistics
    sprintf(szBuf, "%s\\ingres\\bin\\perfmonwrap.bat", szII_SYSTEM);
    if ((fp=fopen(szBuf, "w")) == NULL)
	return (ERROR_SUCCESS);

    GetSystemDirectory(szSystemFolder, sizeof(szSystemFolder));

    fprintf(fp, "@echo off\n" );
    fprintf(fp, "call \"%s\\ingres\\bin\\setingenvs.bat\"\n", szII_SYSTEM);
    fprintf(fp, "unlodctr oipfctrs\n");
    fprintf(fp, "regedit /s \"%%II_SYSTEM%%\\ingres\\files\\OIPFCTRS.REG\"\n");
    fprintf(fp, "lodctr \"%%II_SYSTEM%%\\ingres\\files\\OIPFCTRS.INI\"\n");
    fprintf(fp, "%s\\perfmon %%1\n", szSystemFolder);

    fclose(fp);

    return (ERROR_SUCCESS);
}

/*
**  Name: ingres_remove_from_path
**
**  Description:
**	Removes PATH variable from Environment table in MSI file
**	if INGRES_INCLUDEINPATH is not set.
**
**  History:
**	30-Oct-2006 (penga03)
**	    Created.
*/
ingres_remove_from_path(MSIHANDLE hInstall)
{
	MSIHANDLE msih, hView, hRec;
	char cmd[MAX_PATH+1];

	msih = MsiGetActiveDatabase(hInstall);
	if (!msih)
		return FALSE;
	sprintf(cmd, "SELECT * FROM Environment WHERE Name = '*=-PATH'");
	if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
		return FALSE;



	if (!(MsiViewModify(hView, MSIMODIFY_DELETE, hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiCloseHandle(hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewClose( hView ) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiCloseHandle(hView) == ERROR_SUCCESS))
		return FALSE;
		/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	    return FALSE;

	if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	    return FALSE;

	return ERROR_SUCCESS;


}

/*
**  Name: ingres_pass_properties
**
**  Description:
**	Pass the properties set by Execute sequence to UI sequence.
**
**  History:
**	11-jan-2005 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_pass_properties(MSIHANDLE hInstall)
{
    char szBuf[1024], szID[3];
    HKEY hKey;
    DWORD dwSize;

    dwSize=sizeof(szID);
    if (!MsiGetProperty(hInstall, "II_INSTALLATION", szID, &dwSize))
    {
	sprintf(szBuf, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", szID);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hKey))
	{
	    dwSize=sizeof(szBuf);
	    if (!RegQueryValueEx(hKey, "PostInstallationNeeded", NULL, NULL,
		   szBuf, &dwSize) && !_stricmp(szBuf, "YES"))
		MsiSetProperty(hInstall, "INGRES_POSTINSTALLATIONNEEDED", "1");

	    RegCloseKey(hKey);
	}
    }

    return (ERROR_SUCCESS);
}

/*
**  Name:ingres_stop_ingres_ex
**
**  Description:
**	Stop Ingres if it is running.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	28-jun-2005 (penga03)
**	    Created.
**	29-jul-2005 (penga03)
**	    Corrected the way to check if Ingres cluster is installed.
**	18-Mar-2007 (drivi01)
**	    Fixed an error in sentence:
**	    "You have to shut down Ingres before you can continue!".
**	19-Feb-2009 (drivi01)
**	    Change the title in FindWindow to the correct winstart
**	    dialog title.
*/
UINT __stdcall
ingres_stop_ingres_ex(MSIHANDLE hInstall)
{
	char ii_id[3], ii_system[MAX_PATH];
	DWORD dwSize;
	char szBuf[MAX_PATH], szBuf2[MAX_PATH];
	BOOL bSilent, bCluster;
	HCLUSTER hCluster = NULL;
	HRESOURCE hResource = NULL;
	WCHAR lpwResourceName[256];
	BOOL bStop, bInstalled;
	char szResourceName[256];

	/* check if we need to stop ingres */
	bStop=0;
	//case 1: already installed by "third party" software
	bInstalled=0;
	dwSize=sizeof(szBuf); *szBuf=0;
	if (!MsiGetProperty(hInstall, "Installed", szBuf, &dwSize) && szBuf[0])
		bInstalled=1;

	dwSize=sizeof(ii_id); *ii_id=0;
	MsiGetProperty(hInstall, "II_INSTALLATION", ii_id, &dwSize);
	if (GetInstallPath(ii_id, ii_system))
	{
		sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
		if (!bInstalled && GetFileAttributes(szBuf) != -1)
			bStop=1;
	}

	//case 2: upgrade 26 or earlier versions
	dwSize=sizeof(szBuf); *szBuf=0;
	if (!MsiGetProperty(hInstall, "INGRES_VER25", szBuf, &dwSize) && !_stricmp(szBuf, "1"))
		bStop=1;

	//case 3: upgrade
	dwSize=sizeof(szBuf); *szBuf=0;
	if (!MsiGetProperty(hInstall, "INGRES_UPGRADE", szBuf, &dwSize)
		&& (!_stricmp(szBuf, "1") || !_stricmp(szBuf, "2")))
		bStop=1;

	//case 4: remove
	dwSize=sizeof(szBuf); *szBuf=0;
	if (!MsiGetProperty(hInstall, "REMOVE", szBuf, &dwSize) && szBuf[0])
	{
		if (strstr(szBuf, "ALL")
		   || strstr(szBuf, "IngresDBMS") || strstr(szBuf, "IngresIce")
		   || strstr(szBuf, "IngresReplicator") || strstr(szBuf, "IngresStar")
		   || strstr(szBuf, "IngresNet") || strstr(szBuf, "IngresTCPIP")
		   || strstr(szBuf, "IngresNetBIOS") || strstr(szBuf, "IngresNovellSPXIPX")
		   || strstr(szBuf, "IngresVision") || strstr(szBuf, "IngresDECNet"))
		bStop=1;
	}

	//case 5: modify
	dwSize=sizeof(szBuf); *szBuf=0;
	if (!MsiGetProperty(hInstall, "ADDLOCAL", szBuf, &dwSize)
		&& szBuf[0] && bInstalled)
	{
		if (strstr(szBuf, "IngresDBMS") || strstr(szBuf, "IngresIce")
		   || strstr(szBuf, "IngresReplicator") || strstr(szBuf, "IngresStar")
		   || strstr(szBuf, "IngresNet") || strstr(szBuf, "IngresTCPIP")
		   || strstr(szBuf, "IngresNetBIOS") || strstr(szBuf, "IngresNovellSPXIPX")
		   || strstr(szBuf, "IngresVision") || strstr(szBuf, "IngresDECNet"))
		bStop=1;
	}

	//case 6: repair
	dwSize=sizeof(szBuf); *szBuf=0;
	if (!MsiGetProperty(hInstall, "REINSTALL", szBuf, &dwSize)
		&& szBuf[0] && bInstalled)
		bStop=1;

	if (!bStop)
		return ERROR_SUCCESS;

	/* shut down ivm */
	sprintf(szBuf, "%s\\ingres\\bin\\ivm.exe", ii_system);
	if (GetFileAttributes(szBuf) != -1)
	{
		sprintf(szBuf, "\"%s\\ingres\\bin\\ivm.exe\" -stop", ii_system);
		Execute(szBuf, FALSE);
	}

	if (!IngresAlreadyRunning(ii_system))
		return ERROR_SUCCESS;

	bSilent=0;
	dwSize=sizeof(szBuf); *szBuf=0;
	if (!MsiGetProperty(hInstall, "UILevel", szBuf, &dwSize) && !_stricmp(szBuf, "2"))
		bSilent=1;

	bCluster=0;
	dwSize=sizeof(szResourceName); *szResourceName=0;
	if (!MsiGetProperty(hInstall, "INGRES_CLUSTER_RESOURCE", szResourceName, &dwSize))
	{
		mbstowcs(lpwResourceName, szResourceName, sizeof(szResourceName));

		hCluster = OpenCluster(NULL);
		if(hCluster)
		{
			hResource = OpenClusterResource(hCluster, lpwResourceName);
			if(hResource)
			{
				bCluster=1;
				CloseClusterResource(hResource);
			}
			CloseCluster(hCluster);
		}
	}

	/* shutdown Ingres */
	if (bSilent ||
		AskUserYN(hInstall, "Your Ingres instance is currently running. Would you like to shut it down now?"))
	{
		SetEnvironmentVariable("II_SYSTEM", ii_system);
		GetSystemDirectory(szBuf, sizeof(szBuf));
		sprintf(szBuf2, "%s\\ingres\\bin;%s\\ingres\\utility;%s", ii_system, ii_system, szBuf);
		SetEnvironmentVariable("PATH", szBuf2);

		if (!bCluster)
		{
			if (bSilent)
			{
				sprintf(szBuf, "\"%s\\ingres\\utility\\ingstop.exe\"", ii_system);
				ExecuteEx(szBuf);
			}
			else
			{
				sprintf(szBuf, "\"%s\\ingres\\bin\\winstart.exe\" /stop", ii_system);
				Execute(szBuf, TRUE);

				sprintf(szBuf, "Ingres Service Manager %s", ii_id);
				while (FindWindow(NULL, szBuf))
					Sleep(1000);
			}
		}
		else
		{//take the cluster resource offline
			hCluster = OpenCluster(NULL);
			if (hCluster)
			{
				hResource = OpenClusterResource(hCluster, lpwResourceName);
				if (hResource)
				{
					OfflineClusterResource(hResource);
					CloseClusterResource(hResource);
				}
				CloseCluster(hCluster);
			}
			while (IngresAlreadyRunning(ii_system))
				Sleep(1000);
		}
	}

	if (IngresAlreadyRunning(ii_system))
	{
		if (!bSilent)
			MyMessageBox(hInstall, "You have to shut down Ingres before you can continue.");
		return ERROR_INSTALL_FAILURE;
	}

	return ERROR_SUCCESS;
}

