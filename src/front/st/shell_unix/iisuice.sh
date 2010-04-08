:
#
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#	iisuice -- set up internet communication package
#
#  Usage:
#	iisuice
#
#  Description:
#	This script should only be called by the Ingres installation
#	utility.  It sets up internet communication with Ingres.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	25-apr-96 (harpa06)
##		Created.
##	18-jun-96 (harpa06)
##		Check for Ingres/Net installation version before exiting with a
##		failure status.
##	09-jul-96 (hanch04)
##		Fix install problems
##      31-jul-96 (hanch04)
##              Move trap into do_setup.
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##		Remove the check for net since it should not be required
##		for ICE and isn't even available in the free Linux product.
##	06-apr-1999 (peeje01)
##		Update for ICE Server
##	16-apr-1999 (peeje01)
##		Add install of HTML etc files to $II_HTML_ROOT.
##		If II_HTML_ROOT not set, assume $II_SYSTEM/ingres/ice/HTTParea
##	04-May-1999 (peeje01)
##		Ensure DBMS Cache configured for 4K page size for plays demo.
##	07-May-1999 (peeje01)
##		Move DBMS Cache configuration out of sequence, to before
##		server startup, but only if Demo is selected (this is the 
##              default).
##	11-May-1999 (peeje01)
##		Ensure createdb does not hang even if the user does not exist
##	19-May-1999 (peeje01)
##		Check perms. on the HTTParea directory.
##	16-Aug-1999 (hanch04)
##	    Removed the printing of the stage of setup.
##	13-Jan-2000 (somsa01)
##	    Removed creation of 'icedb', as it is not used anymore. Also,
##	    on upgrade, add old default user into Repository.
##      27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##	14-Aug-2000 (gupsh01)
##		Added code for loading reports ( bug #102194)
##	15-Aug-2000 (gupsh01)
##		Added code for creating icedb for upgrade installations. 
##	01-Mar-2001 (musro02)
##	    Corrected msg announcing creation of icedb
##	21-Mar-2001 (hweho01)
##	    1) Added execution of proc.sql to create procedure     
##             iceprocins during the sample setup.
##          2) Removed extra "Creaing icedb..." msg and add space line   
##             after msg "Checkpointing ICE default database...".
##	06-Jun-2001 (hanje04)
##	    If startup count of icesvr is > 0 before setup has been 
##	    run, (e.g. in the case of an upgrade) set it back to it's
##	    original value afterwards.
##	15-Jun-2001 (hanje04)
##	    Added upgrade section to do_iisuice() to avoid setup failing 
##	    when upgrading from II2.5 II2.6.
##	27-aug-2001 (somsa01)
##	    If SQL92 is on, run the appropriate iceinit script.
##	18-sep-2001 (gupsh01)
##	    added -mkresponse option to create response file, without 
##	    actually doing the install
##	23-oct-2001 (gupsh01)
##	    Added code for handling -exresponse flag.
##	02-nov-2001 (gupsh01)
##	    Made changes for user defined file handling support.
##	06-jun-2002 (somsa01)
## 	    Corrected name of iceinit script for SQL92.
##	22-jul-2002 (hanch04)
##	    If ice had ever been configered, then this must be an upgrade.
##	25-jul-2002 (peeje01)
##	    Ensure that SERVER_HOST is set before using it to check if 
##          SQL 92 enabled with iigetres.
##	11-jun-2003 (hayke02)
##	    Add -rmpkg flag to remove icesvr entries from config.dat. This
##	    change fixes bug 110379.
##	20-Jan-2004 (bonro01)
##	    Remove ingbuild calls for PIF installs.
##	30-Jan-2004 (hanje04)
##	    Changes for SIR 111617 changed the default behavior of ingprenv
##	    such that it now returns an error if the symbol table doesn't
##	    exists. Unfortunately most of the iisu setup scripts depend on
##	    it failing silently if there isn't a symbol table.
##	    Added -f flag to revert to old behavior.
##      15-Jan-2004 (hanje04)
##	    Added support for running from RPM based install.
##	    Invoking with -rpm runs in batch mode and uses
##	    version.rel to check version instead of ingbuild.
##	03-feb-2004 (somsa01)
##	    Backed out symbol.tbl change for now.
##	07-Mar-2004 (bonro01)
##	    Add -rmpkg flag for PIF un-installs
##	02-Mar-204 (hanje04)
##	    Use cat and not tee to write log file when called with -exresponse
##	    to stop output being written to screen as well as log.
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##	22-Mar-2004 (hanje04)
##	    Correct -exresponse logging change by not defining INSTLOG for 
##	    exresponse at all. Just use the default value.
##	12-May-2004 (bonro01)
##	    Add -vbatch option.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##      07-Apr-2005 (hanje04)
##          Bug 114248
##          When asking users to re-run scripts it's a good idea to tell
##          them which script needs re-running.
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	18-Sep-2007 (hweho01)
##	    Need to export II_CHARSETxx, so createdb can pick up the setting.
##	18-Oct-2007 (bonro01)
##	    Correct -rmpkg code. Restore code which deletes icesvr config
##	    options and removes ingstart option for icesvr.
##      19-Oct-2007 (hweho01)
##          Amend the change on 18-Sep-2007, if II_CHARSETxx is set to null,
##          unset it.
##
#  DEST = utility
#----------------------------------------------------------------------------


#{
# Name: usage
#
# Description: Issue usage message
#
usage()
{
cat << !
iisuice [-batch] [-vbatch] [-begin Stage] [-end Stage] [-help] [-rmpkg]
        [-mkresponse [response file]]
        [-exresponse [response file]]
Where
    -batch selects batch mode
    -vbatch selects batch mode with visible output
    -begin specifies starting install stage, Stage
    -end   specifies ending install stage, Stage
    -mkresponse create a response file without installing
    -exresponse install using a response file
    -help  generates this message
    -rmpkg removes icesvr config.dat entries

Required install stages are as follows:
     Stage 1:  Create the default ICE configuration
     Stage 2:  Create the ICE admin user
     Stage 3:  Create the ICE repository, icesvr
               Stage 3 completes the ICE server install.
Further stages are optional:
     Stage 4:  Create the tutorial database, icetutor
     Stage 5:  Install the HTML ICE Administration Tool
     Stage 6:  Install the HTML ICE Tutorial and Design Tool
     Stage 7:  Install the HTML ICE Samples
     Stage 8:  Install the HTML ICE Shakespeare Demonstration
!
}
#} End usage

#{
# Name:   bloberror
#
# Description:
#     Report errors with loading of BLOBS
#
# Environment:
#     ICE_LogFile
#     Heading
#
bloberror()
{
	    cat << !

An error occurred while loading BLOBS into your ICE repository database.
A log of the attempted checkpoint can be found in:

    $ICE_LogFile
following the heading:
    $Heading
!
}
#} End bloberror

#{
# Name:   iceload
#
# Description:
#     Performs a bulk update of blobs contained in a table based upon the
#     id value.
#     Uses the data table iceload.dat in $II_SYSTEM/ingres/ice/scripts
#     Format of the table is "Entry ID BlobFileName"
#
# Environment:
#     ICE_root e.g. ICE_root = $II_SYSTEM/ingres/ice
#
Usage="Usage: iceload Session_Group Table_Name Blob_Column Database"
#
# Exactly 4 arguments are required
#
iceload()
{
if test $# -ne 4
then
    echo $Usage
    exit 1
fi # arg count

SessionGroup=$1
Tbl=$2 # Table
Blb=$3 # Blob Column Name
Db=$4  # Database Name
DataFile=$II_SYSTEM/ingres/ice/scripts/iceload.dat
status=0
    grep "^$SessionGroup" $DataFile | while read entry
    do
	ID=`echo $entry | awk '{print $2}'`
	BlbF=`echo $entry | awk '{print $3}'`
	eval blobstor -t $Tbl -b $Blb -u -w id=$ID $Db $BlbF >> $ICE_LogFile 2>&1 ||
            status=1
    done
return $status
}
#} End of iceload()

#{
# Name:   copy_files()
#
# Description:
#     Copy the required files to the II_HTML_ROOT directory, ensure
#     that the directory structure exists.
#
copy_files()
{
Component=$1
CREATE=true; export CREATE
DataFile=$II_SYSTEM/ingres/ice/scripts/icefiles.dat
status=0
    grep "^$Component" $DataFile | while read entry
    do
        RelSrc=`echo $entry | awk -F: '{print $2}'`
        RelDir=`echo $entry | awk -F: '{print $3}'`
        TGT=`echo $entry | awk -F: '{print $4}'`
	SRC=$II_SYSTEM/ingres/ice/$RelSrc
	DIR=$II_HTML_ROOT/$RelDir
	subDirList=$II_HTML_ROOT
	# Ensure directory tree exists,
	# starting at the top:
	if test ! -d $subDirList
	then
	    cat << !
Making the HTML root document directory:

    $II_HTML_ROOT
!
	    mkdir -p $subDirList || status=1
	    # ensure correct permissions on dir.
	    check_directory $subDirList 755
	fi
	echo $RelDir | tr '/' '\n' | while read subDir
	do
	    subDirList="$subDirList/$subDir"
	    CREATE=true
	    # Only need to check sub_directories
	    case "_$subDir" in
	           "_") ;;
	        "_tmp") check_directory $subDirList 777;;
		     *) check_directory $subDirList 755;;
	    esac
	done
	# Only copy if status is OK (0) and 
	# there are files to copy (RelSrc not empty)
	[ 0 -eq $status -a "_" != "_$RelSrc" ] && cp $SRC $DIR/$TGT
    done
    return $status
}
#} End of copy_files()

#{
# Name:   do_iisuice()
#
# Description:
#     Perform a staged install of the Ingres ICE server and Demos
#     The following list is repeated in the usage message:
#     There are 8 stages:
#     Stage 1:  Create the default ICE configuration
#     Ensure Ingres started (terminate on failure)
#     Stage 2:  Create the ICE admin user
#     Stage 3:  Create the ICE repository, icesvr
#               Stage 3, $RequiredMax completes the ICE server install.
#               Further stages are optional.
#     Stage 4:  Create the tutorial database, icetutor
#     Stage 5:  Install the HTML ICE Administration Tool
#     Stage 6:  Install the HTML ICE Tutorial and Design Tool
#     Stage 7:  Install the HTML ICE Samples
#     Stage 8:  Install the HTML ICE Shakespeare Demonstration
#
# Environment:
#     ICE_root e.g. ICE_root = $II_SYSTEM/ingres/ice
#
## History
## 04-Mar-1999 (peeje01)
##    Created
## 13-Jan-2000 (somsa01)
##    Removed 'icedb' creation.
##      14-Aug-2000 (gupsh01)
##              Added code for loading reports ( bug #102194)
##      15-Aug-2000 (gupsh01)
##              Added code for creating icedb for upgrade installations.
##
#
#
do_iisuice()
{
#{
# Initial
umask 022
RequiredMax=3
trap "rm -f $II_SYSTEM/ingres/files/config.lck 1>/dev/null \
2>/dev/null" 0 1 2 3 15
if test $ICE_begin -gt $ICE_end
then
    usage
    cat << !
Start stage ($ICE_begin) is higher than end stage ($ICE_end).

Please check values and re-start
!
    exit 1
fi # begin, end range check
echo "Setting up Ingres/ICE..."

. iisysdep

. iishlib

check_response_file

if [ "$WRITE_RESPONSE" = "true" ] ; then #mkresponse mode
    mkresponse_msg
else # skip rest in response mode

# Check for lockout
iisulock "Ingres/ICE setup" || exit 1

# make sure symbol.tbl and config.dat exist
touch_files

if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
   VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=ice` ||
   {
       cat << !

$VERSION

!
      exit 1
   }
else
   VERSION=`head -1 $II_SYSTEM/ingres/version.rel` ||
   {
       cat << !

Missing file $II_SYSTEM/ingres/version.rel

!
      exit 1
   }
fi

RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`iigetres ii.$CONFIG_HOST.config.icesvr.$RELEASE_ID` || exit 1

[ "$SETUP" = "complete" ] && [ $ICE_begin -le $RequiredMax ] &&
{
   cat << !

Ingres/ICE has already been set up on "$HOSTNAME".

!
   $BATCH || pause
   trap : 0
   clean_exit
}

# Check that the Ingres Intelligent RDBMS setup is complete:

DBMS_SETUP=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID`

UPGRADE=`grep 'config.icesvr.ii' $II_CONFIG/config.dat \
 		 | awk '{print $2}'`

if [ "$DBMS_SETUP" != "complete" ]
then
   cat << !

The setup procedures for the following version of Ingres Intelligent
DBMS:

        $VERSION

must be completed up before Ingres/ICE can be set up on host:

        $HOSTNAME

!
   $BATCH || pause
   exit 1
fi # DBMS setup not complete

if [ -z "$UPGRADE" ]
then

# Announce we are setting up ICE

cat << !
This procedure will set up the following version of Ingres/ICE:

        $VERSION

to run on local host:

        $HOSTNAME

!
SERVER_HOST=`iigetres ii."*".config.server_host` || exit 1
# initial }

# If upgrading from ii2.5  bypass setup and just upgrade databases

#
# Out of sequence stage:
# If we are doing stage 8, ensure 4K page size is enabled
# BEFORE we start the server.
# The rest of stage 8 will be done at the appropriate time.
#
ICE_stage=8
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    # The DBMS parameters for DMF cache size should have 4096 byte pages enabled
    Heading="Ensure 4K page size enabled (for Tutorial Demonstration)"
    ICE_LogFile=$II_SYSTEM/ingres/files/ice_plays.log
    echo $Heading >> $ICE_LogFile
    echo $Heading
    Status4k=`iigetres ii.$CONFIG_HOST.dbms.private.*.cache.p4k_status`
    case "$Status4k" in
        "ON") ;;
	"OFF") iisetres ii.$CONFIG_HOST.dbms.private.*.cache.p4k_status ON
	     cat << !

The ICE Demonstration makes use of 4096 byte pages.  Your system has
been configured to enable 4096 byte pages.

!
        ;;
        *) cat << !

An error occurred while modifying your DBMS server cache for the ICE
Demonstration.  A log of the attempt can be found in:

    $ICE_LogFile
following the heading:
    $Heading

!
    esac
fi
#}

#{
# Stage 1:
#     Create default config.dat entries
#
ICE_stage=1
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    cat << !
Generating default internet communication configuration...
!
    iigenres $CONFIG_HOST $II_SYSTEM/ingres/files/ice.rfm ||
    {
       cat << !
An error occurred while generating the default configuration.

!
       $BATCH || pause
       exit 1
    }
# Make sure that the icesvr doesn't start up
ICE_SERVER=`iigetres ii.$CONFIG_HOST.ingstart."*".icesvr`
[ -z "$ICE_SERVER" ] && ICE_SERVER=0
if [ $ICE_SERVER -ne 0 ]
then
   iisetres ii.$CONFIG_HOST.ingstart."*".icesvr 0
fi

fi # ICE_stage
#}

#{
# Start Ingres to create the repository
# Must occur after update of config file (iigenres) and
# BEFORE ANY RDBMS activity
#
cat << !

Starting the Ingres server to initialize ICE catalogs...
!
   ingstart ||
   {
      ingstop >/dev/null 2>&1
      cat << !
The Ingres server could not be started.  See the server error log
(\$II_SYSTEM/ingres/files/errlog.log) for a full description of the
problem. Once the problem has been corrected, you must re-run $SELF
before attempting to use ICE.

!
      exit 1
   }
#} Start Ingres

#{
# Stage 2:
#     Setup ICE admin. user
#

ICE_stage=2
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    ICE_LogFile=$II_SYSTEM/ingres/files/ice_user.log
    sql iidbdb <$ICEscripts/icedbusr.sql>$ICE_LogFile 2>&1 ||
	    {
		cat << !
An error occured while creating your ICE system user. A log of
the attempted user creation can be found in:
	$ICE_LogFile
!
	    }
fi # ICE_stage
#}

#{
# Stage 3:
#     Create repository database
#     Populate repository
#     On upgrade, add old default user to repository.
#     Checkpoint populated repository
#

ICE_stage=3
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    ICE_LogFile=$II_SYSTEM/ingres/files/ice_repository.log
    cat << !

Creating the ICE repository database, icesvr...

!

# If II_CHARSETxx is set to null, need to unset it.
if env | grep  "^II_CHARSET$II_INSTALLATION" > /dev/null ; then
eval "ii_charsetxx_value"="\$II_CHARSET$II_INSTALLATION"
[ -z "$ii_charsetxx_value" ] && eval unset "II_CHARSET$II_INSTALLATION"
fi

    if echo | createdb icesvr > $ICE_LogFile 2>&1; then
	# Create repository schema
	Heading="Create repository schema"
	echo $Heading >> $ICE_LogFile
	ICE_SCRIPT=iceinit.sql
	SQL_92=`iigetres ii.$SERVER_HOST.fixed_prefs.iso_entry_sql-92`
	if [ "$SQL_92" = "ON" ] ; then
	    ICE_SCRIPT=icinit92.sql
	fi
	sql icesvr < $ICEscripts/$ICE_SCRIPT >> $ICE_LogFile 2>&1 ||
		{
		 cat << !

An error occurred while creating your ICE repository schema.  A log of
the attempt can be found in:

	$ICE_LogFile
following the heading:
	$Heading

!
		}
	old_defusr=`iigetres ii.$CONFIG_HOST.ice.default_userid`
	if [ $old_defusr ]
	then
	    iisetres ii.$CONFIG_HOST.icesvr.*.default_dbuser $old_defusr
	    echo "insert into ice_dbusers values (2, '$old_defusr', '$old_defusr', '', '');\p\g" | sql icesvr > /dev/null 2>&1
	fi
	echo "Checkpointing ICE repository database..."
	ckpdb icesvr -l +w >>$ICE_LogFile 2>&1 ||
	    {
	    cat << !

An error occurred while checkpointing your ICE repository database.  A
log of the attempted checkpoint can be found in:

$ICE_LogFile

You should contact Ingres Corporation Technical Support to resolve
this problem.
!
	}
    else
	{
	    cat << !

An error occurred while creating your ICE repository database.  A log
of the attempted checkpoint can be found in:

    $ICE_LogFile

Once the problem has been corrected, you must re-run
$SELF before attempting to use ICE.
!
    exit 1
	}
    fi # createdb icesvr

fi # ICE_stage
#} End 3

#{
# Stage 4:
#     Create tutor database
#     Checkpoint tutor database
#     Create tutor users in icesvr
#     Copy the public files ('global' component)
#

ICE_stage=4
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    ICE_LogFile=$II_SYSTEM/ingres/files/ice_tutor.log
    Heading="Create tutorial database"
    echo $Heading >> $ICE_LogFile
    ingunset ING_SET_ICETUTOR
    cat << !

Creating the ICE tutor database, icetutor...

!
    if echo | createdb -u$icedbuser icetutor >> $ICE_LogFile 2>&1; then
	echo "Checkpointing ICE tutor database..."
	ckpdb icetutor -l +w >>$ICE_LogFile 2>&1 ||
	    {
		cat << !

An error occurred while checkpointing your ICE tutor database.  A log
of the attempted checkpoint can be found in:

    $ICE_LogFile

You should contact Ingres Corporation Technical Support to resolve
this problem.
!
	    }
    else
	{
	    cat << !

An error occurred while creating your ICE tutor database.  A log of the
attempted create can be found in:

    $ICE_LogFile
following the heading:
    $Heading

Once the problem has been corrected, you must re-run
$SELF.
!
	}
	return 1
    fi # createdb icetutor
    # Setup tutorial users
    sql icesvr < $ICEroot/scripts/iceuser.sql>>$ICE_LogFile ||
	    {
	     cat << !

An error occurred while creating your ICE tutorial users.  A log of
the attempt can be found in:

	$ICE_LogFile

!
	    }

II_DATABASE=`ingprenv II_DATABASE`
if [ ! -d $II_DATABASE/ingres/data/default/icedb ]
then
# now create the icedb for new installs.
{
             cat << !

Creating the ICE default database, icedb...

!
            }
   if echo | createdb -u$icedbuser icedb > $ICE_LogFile 2>&1; then
        echo "Checkpointing ICE default database..."
        echo ""
        ckpdb icedb -l +w >$ICE_LogFile 2>&1 ||
            {
                cat << !

An error occurred while checkpointing your ICE default database.  A
log of the attempted checkpoint can be found in:

    $ICE_LogFile

You should contact Ingres Corporation Technical Support to resolve
this problem.
!
            }
# Signal successful completion of ICE Required components

    else
        {
            cat << !

An error occurred while creating your ICE default database.  A log
of the attempted checkpoint can be found in:

    $ICE_LogFile

Once the problem has been corrected, you must re-run
$SELF.
!
        }
     return 1
     fi # createdb icedb
fi
    # Copy globally required files
    copy_files global
fi # ICE_stage
#} End 4

#{
# Stage 5:
#     Install the Admin tool
#     Checkpoint server database
#

ICE_stage=5
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    Heading="Installing ICE Administration Tool"
    ICE_LogFile=$II_SYSTEM/ingres/files/ice_admin.log
    echo $Heading
    echo $Heading >> $ICE_LogFile
    sed "s,\[II_WEB_SYSTEM\],$II_SYSTEM,g" $ICEscripts/icetool.sql |
	sql icesvr  >> $ICE_LogFile 2>&1 ||
	    {
	     cat << !

An error occurred while creating your ICE Administration Tool.  A log of
the attempt can be found in:

    $ICE_LogFile
following the heading:
    $Heading

!
	    }
    # load the Admin tool Blobs
## FixMe: IDs should come from the data file, & move makeid to iceload()
    $II_SYSTEM/ingres/bin/makeid 1095 1146 id ice_files icesvr 
    iceload icetool ice_files doc icesvr || bloberror
    # Checkpoint the icesvr database
    echo "Checkpointing ICE repository database..."
    ckpdb icesvr -l +w >>$ICE_LogFile 2>&1 ||
	{
	    cat << !

An error occurred while checkpointing your ICE repository database.  A log
of the attempted checkpoint can be found in:

    $ICE_LogFile
following the heading:
    $Heading

You should contact Ingres Corporation Technical Support to resolve
this problem.
!
	}
    # Copy admin tool files
    copy_files icetool
fi # ICE_stage
#} End 5


#{
# Stage 6:
#     Install the Tutorial and Design tool
#     Checkpoint server database
#

ICE_stage=6
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    Heading="Installing ICE Tutorial and Design Tool"
    ICE_LogFile=$II_SYSTEM/ingres/files/ice_design.log
    echo $Heading
    echo $Heading >> $ICE_LogFile
    sed "s,\[II_WEB_SYSTEM\],$II_SYSTEM,g" $ICEscripts/icetutor.sql |
	sql icesvr >> $ICE_LogFile 2>&1 ||
	{
	     cat << !

An error occurred while creating your ICE Tutorial and Design Tool.  A log of
the attempt can be found in:

    $ICE_LogFile
following the heading:
    $Heading

!
	}
    # load the Design and Tutorial Tool Blobs
## FixMe: IDs should come from the data file, & move makeid to iceload()
    $II_SYSTEM/ingres/bin/makeid 2079 2098 id ice_files icesvr
	iceload icetutor ice_files doc icesvr || bloberror
    # Checkpoint the icesvr database
    echo "Checkpointing ICE repository database..."
    ckpdb icesvr -l +w >>$ICE_LogFile 2>&1 ||
	{
	    cat << !

An error occurred while checkpointing your ICE repository database.  A log
of the attempted checkpoint can be found in:

    $ICE_LogFile
following the heading:
    $Heading

You should contact Ingres Corporation Technical Support to resolve
this problem.
!
	}
    # Copy ICE tutor files
    copy_files icetutor
fi # ICE_stage
#} End 6

#{
# Stage 7:
#     Install the Samples
#     Checkpoint server database
#

ICE_stage=7
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    Heading="Installing ICE Samples"
    ICE_LogFile=$II_SYSTEM/ingres/files/ice_samples.log
    echo $Heading
    echo $Heading >> $ICE_LogFile
    sql icesvr < $ICEscripts/icesample.sql>>$ICE_LogFile ||
	    {
	     cat << !

An error occurred while creating your ICE Samples.  A log of
the attempt can be found in:

	$ICE_LogFile
following the heading:
	$Heading

!
	    }

sreport -s -uicedbuser icetutor "$II_SYSTEM/ingres/ice/samples/report/existing.rw">>$ICE_LogFile ||
            {
             cat << !

An error occurred while creating your ICE Samples.  A log of
the attempt can be found in:

        $ICE_LogFile
following the heading:
        $Heading

!
            }

sreport -s -uicedbuser icetutor "$II_SYSTEM/ingres/ice/samples/report/htmlrbf.rw">>$ICE_LogFile ||
            {
             cat << !

An error occurred while creating your ICE Samples.  A log of
the attempt can be found in:

        $ICE_LogFile
following the heading:
        $Heading

!
            }

sreport -s -uicedbuser icetutor "$II_SYSTEM/ingres/ice/samples/report/htmlwrite.rw">>$ICE_LogFile ||
            {
             cat << !

An error occurred while creating your ICE Samples.  A log of
the attempt can be found in:

        $ICE_LogFile
following the heading:
        $Heading

!
            }

sreport -s -uicedbuser icetutor "$II_SYSTEM/ingres/ice/samples/report/htmlwvar.rw">>$ICE_LogFile ||
            {
             cat << !

An error occurred while creating your ICE Samples.  A log of
the attempt can be found in:

        $ICE_LogFile
following the heading:
        $Heading

!
            }

sql -uicedbuser icetutor < $II_SYSTEM/ingres/ice/samples/dbproc/proc.sql >> $ICE_LogFile || 

             {
              cat << !

An error occurred while creating your ICE Samples.  A log of
the attempt can be found in:

         $ICE_LogFile
following the heading:
         $Heading

!
             }

    echo "Checkpointing ICE repository database..."
    ckpdb icesvr -l +w >>$II_SYSTEM/ingres/files/ICE_LogFile 2>&1 ||
	{
	    cat << !

An error occurred while checkpointing your ICE repository database.  A log
of the attempted checkpoint can be found in:

$ICE_LogFile

You should contact Ingres Corporation Technical Support to resolve
this problem.
!
	}
    # Copy sample files
    copy_files samples
fi # ICE_stage
#} Stage 7

#{
# Stage 8:
#     Install the Shakespear Play Demonstration
#     Checkpoint server database
#     Checkpoint tutor database
#

ICE_stage=8
if [ $ICE_stage -ge $ICE_begin ] && [ $ICE_stage -le $ICE_end ]
then
    Heading="Installing ICE demonstration"
    ICE_LogFile=$II_SYSTEM/ingres/files/ice_plays.log
    echo $Heading >> $ICE_LogFile
    echo $Heading
    # The icetutor database contains the tables for the plays demo.

    ingunset ING_SET_ICETUTOR

    sed "s,\[II_WEB_SYSTEM\],$II_SYSTEM,g" $ICEscripts/iceplays.sql |
	sql icesvr >> $ICE_LogFile 2>&1 ||
	{
         cat << !

An error occurred while creating your ICE Demonstration.  A log of
the attempt can be found in:

    $ICE_LogFile
following the heading:
    $Heading

!
	}
    # load the Plays Blobs
## FixMe: IDs should come from the data file, & move makeid to iceload()
    $II_SYSTEM/ingres/bin/makeid 24 31 id ice_files icesvr
	iceload plays ice_files doc icesvr || bloberror

    # Checkpoint the ICE server database
    echo "Checkpointing ICE repository database..."
    ckpdb icesvr -l +w >>$II_SYSTEM/ingres/files/ICE_LogFile 2>&1 ||
	{
	    cat << !

An error occurred while checkpointing your ICE repository database.  A log
of the attempted checkpoint can be found in:

$ICE_LogFile

You should contact Ingres Corporation Technical Support to resolve
this problem.
!
	}

    # Load the plays schema and data
    sql -uicedbuser icetutor < $ICEscripts/icedemo.sql >> $ICE_LogFile ||
	    {
	     cat << !

An error occurred while creating your ICE plays demonstration.  A log of
the attempt can be found in:

	$ICE_LogFile
following the heading:
	$Heading

!
	    }

    # Set locking mode on application table
    ingsetenv ING_SET_ICETUTOR "set lockmode on play_order where level=row, readlock=nolock"
    # Checkpoint the ICE tutor database
    echo "Checkpointing ICE tutor database..."
    ckpdb icetutor -l +w >>$ICE_LogFile 2>&1 ||
	{
	    cat << !

An error occurred while checkpointing your icetutor database.  A log
of the attempted checkpoint can be found in:

$ICE_LogFile

You should contact Ingres Corporation Technical Support to resolve
this problem.
!
	}
    # Copy Play files
    copy_files plays
fi # ICE_stage
#} End 8

#{
# Cleanup
cat << !

Shutting down the Ingres server...

!
ingstop >/dev/null 2>&1 &&

cat << !

Ingres/ICE stages

    $ICE_begin to $ICE_end

have been set up in this installation.

To use ICE
  You must enable your ICE server (cbf)
  You must configure your html home directory for your Web server
  You must configure your Web server for ICE, please see the ICE documentation

!

else # UPGRADE

# Anounce we are performing an upgrade

cat << !
A previous version Ingres/ICE has been detected on local host:

        $HOSTNAME 

This procedure will upgrade it to the following version:

        $VERSION

The ICE repository database has been upgraded...

!

# Make sure that the icesvr doesn't start up
ICE_SERVER=`iigetres ii.$CONFIG_HOST.ingstart."*".icesvr`
[ -z "$ICE_SERVER" ] && ICE_SERVER=0
if [ $ICE_SERVER -ne 0 ]
then
   iisetres ii.$CONFIG_HOST.ingstart."*".icesvr 0
fi

# Start Ingres so that databases can be upgraded.

cat << !

Starting the Ingres server...
!
   ingstart ||
   {
      ingstop >/dev/null 2>&1
      cat << !
The Ingres server could not be started.  See the server error log
(\$II_SYSTEM/ingres/files/errlog.log) for a full description of the
problem. Once the problem has been corrected, you must re-run $SELF
before attempting to use ICE.

!
      exit 1
   }

if [ -d $II_SYSTEM/ingres/data/default/icetutor ]
then

    cat << !
Upgrading tutorial database...

!

    upgradedb icetutor >> $II_SYSTEM/ingres/files/upgradedb.log 2>&1 ||
    {
    cat << !
An error occurred while upgrading your system catalogs or one of the
databases in this installation.	 A log of the attempted upgrade can
be found in:

        $II_SYSTEM/ingres/files/upgradedb.log

You should contact Ingres Corporation Technical Support to resolve
this problem.

    !
    return 1
    }

   echo "Checkpointing tutorial database..."
   ckpdb icetutor -l +w >>$ICE_LogFile 2>&1 ||
        {
            cat << !
An error occurred while checkpointing your ICE tutor database.  A log
of the attempted checkpoint can be found in:

    $ICE_LogFile

You should contact Ingres Corporation Technical Support to resolve
this problem.

!
    return 1
        }
	
fi

if [ -d $II_SYSTEM/ingres/data/default/icedb ]
then

    cat << !
Upgrading the ICE default database, icedb...

!

    upgradedb icedb >> $II_SYSTEM/ingres/files/upgradedb.log 2>&1 ||
    {
    cat << !
An error occurred while upgrading your system catalogs or one of the
databases in this installation.  A log of the attempted upgrade can
be found in:

        $II_SYSTEM/ingres/files/upgradedb.log

You should contact Ingres Corporation Technical Support to resolve
this problem.

    !
    return 1
    }

   echo "Checkpointing ICE default database..."
   ckpdb icetutor -l +w >>$ICE_LogFile 2>&1 ||
        {
            cat << !
An error occurred while checkpointing your ICE tutor database.  A log
of the attempted checkpoint can be found in:

    $ICE_LogFile

You should contact Ingres Corporation Technical Support to resolve
this problem.

!
    return 1
        }



fi

# Cleanup
cat << !

Shutting down the Ingres server...

!
ingstop >/dev/null 2>&1 &&

cat << !

Ingres/ICE has been successfully upgraded to

	$VERSION

!

fi # UPGRADE

if [ "$ICE_SERVER" -ne 0 ] ; then
  iisetres  ii.$CONFIG_HOST.ingstart."*".icesvr $ICE_SERVER
fi

iisetres ii.$CONFIG_HOST.config.icesvr.$RELEASE_ID complete

fi #end $WRITE_RESPONSE mode

$BATCH || pause
trap : 0
clean_exit
#} Cleanup

}
#} End of do_iisuice()

#{
# main processing starts here
# Initialize variables
BATCH=false
INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
ICEroot="$II_SYSTEM/ingres/ice"
ICEscripts="$ICEroot/scripts"
[ "_" = "_$II_HTML_ROOT" ] && II_HTML_ROOT=$ICEroot/HTTParea
ICE_begin=1
ICE_end=8
icedbuser=icedbuser
WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false
SELF=`basename $0`

while [ $# != 0 ]
do
    case "$1" in
        -batch)
            BATCH=true ; export BATCH ;
	    INSTLOG="2>&1 | cat >> $II_SYSTEM/ingres/files/install.log"
	    export INSTLOG;
            shift ;;
        -vbatch)
            BATCH=true ; export BATCH ;
	    export INSTLOG;
            shift ;;
	-begin)
	    ICE_begin=$2
	    shift; shift ;;
	-end)
	    ICE_end=$2
	    shift; shift ;;
	-mkresponse)
	    WRITE_RESPONSE=true 
   	    INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
	    export INSTLOG;
	    if [ "$2" ] ; then
		RESOUTFILE="$2"
		shift;
	    fi
	    shift ;;
	-exresponse)
            BATCH=true ; export BATCH ;
	    READ_RESPONSE=true
	    export INSTLOG;
	    if [ "$2" ] ; then
		RESINFILE="$2"
		shift;
	    fi 
	    shift ;;
        -rpm)
	    RPM=true
            BATCH=true ; export BATCH ;
	    INSTLOG="2>&1 | cat >> $II_SYSTEM/ingres/files/install.log"
	    export INSTLOG;
            shift ;;
	-help)
	    usage
	    exit 0 ;;
	-rmpkg)
	    II_CONF_DIR=$II_SYSTEM/ingres/files
 
	    cp -p $II_CONF_DIR/config.dat $II_CONF_DIR/config.tmp
     
	    trap "cp -p $II_CONF_DIR/config.tmp $II_CONF_DIR/config.dat; \
	    rm -f $II_CONF_DIR/config.tmp; exit 1" 1 2 3 15
		  
	    cat $II_CONF_DIR/config.dat | grep -v 'icesvr' | \
	    grep -v '\.net'  >$II_CONF_DIR/config.new
			     
	    rm -f $II_CONF_DIR/config.dat
				 
	    mv $II_CONF_DIR/config.new $II_CONF_DIR/config.dat
				     
	    rm -f $II_CONF_DIR/config.tmp

   cat << !
  Web Deployment Option has been removed

!
	    exit 0 ;;
        *)
            usage
            exit 1 ;;
    esac
done

export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

trap "rm -f $II_SYSTEM/ingres/files/config.lck 1>/dev/null \
2>/dev/null" 0 1 2 3 15

eval do_iisuice $INSTLOG
trap : 0
exit
#} End main
