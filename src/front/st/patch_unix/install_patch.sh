:
#
# Copyright (c) 2004 Ingres Corporation
# Standard Ingres patch installation script for Unix.
#
##
## This script addresses some of the commonly encountered issues when 
## installing a patch:
##	Is Ingres running
##	Am I the correct user
##	Is there enough disk space
##	I don't want to copy the original files elsewhere
##	The patch files listed and what I've got don't match up
##	Setuid bits have been lost
##
## It also provides more information when each step is taking place.
##
## History:
##
##	18-nov-97 (ocoro02)
##		Added
##	19-nov-97 (ocoro02)
##		Added version.rel history information (SIR 87301)
##	08-dec-97 (ocoro02)
##		version.rel not being updated properly on HP & AIX. Escaped
##		backslashes in awk statement so echo evaluates result 
##		properly
##	11-jul-1998 (ocoro02)
##		Removed disk calculation figures. Make backup plist from
##		comparison between installation files and patch files. 
##		Removed a lot of unnecessary verbosity.
##		Check whether Ingres is running before making backups.
##	24-jul-1998 (ocoro02)
##		Calculate free/required space in bytes. Avoids out of
##		range errors with expr.
##	27-jul-1998 (ocoro02)
##		Added to ingres!oping20 codeline.
##	28-apr-2000 (somsa01)
##		Enabled for different product builds.
##	24-Nov-2004 (bonro01)
##		$ECHO_CHAR can only be used with $ECHO_CMD
###############################################################################

bail_out()
{
  return_code=$1
  shift
  description="$*"
  echo "Error: $description"
  exit $return_code
}

# Need (PRODLOC) variable set. Check for it.
check_iisystem_set()
{
  [ -z "$(PRODLOC)" ] && bail_out 1 (PRODLOC) variable is not set
}

# Set up platform specific variables
setup_vars()
{
  [ -r $(PRODLOC)/(PROD2NAME)/utility/(PROG1PRFX)sysdep ] && \
    . $(PRODLOC)/(PROD2NAME)/utility/(PROG1PRFX)sysdep || \
      bail_out 1 Cannot read $(PRODLOC)/(PROD2NAME)/utility/(PROG1PRFX)sysdep
}

CMD=`echo $0 | sed "$BPATHSED"`
SOURCE=`pwd`
DESTINATION="$(PRODLOC)/(PROD2NAME)"

# Print usage message
usage()
{
cat <<- EOF
Usage: $CMD [ -h | -n ]

no args	: Install patch from current directory
-h	: Print this help message
-n	: Don't backup existing installation files.
EOF
exit 0
}

# Check plist file is in current directory
check_plist()
{
  if [ ! -r plist ]; then
    bail_out 1 plist file not readable in current directory
  else
    PLIST=plist
  fi
}

BACKUP=true

# Get some arguments. Program takes zero or one argument.
get_args()
{
  if [ $# -gt 0 ]; then
    case "$1" in
    -h)	# Print usage
  	  usage
  	  ;;
    -n)	# Don't copy current installation files elsewhere
  	  BACKUP=false
  	  ;;
    esac
  fi
}

# Need to figure out version level
# Expected form of first two lines of version.rel (e.g):
# 1.2/01 (su4.us5/00)
#         1234
# 
# or
# version: 1.2/01 (su4.us5/00)
# patch:   1234
#
# If version.rel doesn't exist, try getting version from ingbuild.
get_version_level()
{
  destination=$1
  VERSION=""

  if [ -r $destination/version.rel ]; then
    # Get the version number
    # Grab first line using 'head'. This could be platform dependent.
    VERSION=`head -1 $destination/version.rel`
    if [ -n "`echo $VERSION | grep -i version`" ]; then
      # Has the string 'version:' in front
      VERSION=`echo $VERSION | \
	sed -e 's/version//' \
	    -e 's/Version//' \
	    -e 's/VERSION//' \
	    -e 's/^\://' \
	    -e 's/^	//g' \
	    -e 's/^ //g'`
    else
      VERSION=`echo $VERSION | \
	sed -e 's/^	//g' \
	    -e 's/^ //g'`
    fi
  elif [ -x $DESTINATION/install/ingbuild ]; then
    # Try get the version number from ingbuild if it exists
      VERSION=`$DESTINATION/install/ingbuild -version | \
	sed -e 's/^OI//' -e 's/^ //g'`
  else
    # Need to prompt for version
    echo "Cannot determine (PROD1NAME) version number"
    echo "Please enter current version, e.g:  1.2/01 (su4.us5/00)"
    read VERSION junk
    echo ""
  fi
  [ "$VERSION" = "" ] && \
    bail_out 1 Cannot determine (PROD1NAME) version
  MAJOR_VERSION=`echo "$VERSION" | sed -e 's/\/.*$//'`
}

# Attempt to figure out current patch level
get_patch_level()
{
  destination=$1
  PATCH_ID=""

  if [ -r $destination/version.rel ]; then
    # Grab the patch id if it exists
    # If more than one line in version.rel, it must have a patch id
    if [ `wc -l $destination/version.rel | awk '{print $1}'` -gt 1 ]; then
      PATCH_ID=`head -2 $destination/version.rel | tail -1`
    else
      return
    fi
    
    # Strip off the string 'patch:' if it exists
    if [ -n "`echo "$PATCH_ID" | grep -i patch`" ]; then
      PATCH_ID=`echo $PATCH_ID | \
	sed -e 's/patch//' \
	    -e 's/Patch//' \
	    -e 's/PATCH//' \
	    -e 's/^\://' \
	    -e 's/^	//g' \
	    -e 's/^ //g'`
    else
      PATCH_ID=`echo $PATCH_ID | \
	sed -e 's/^	//g' \
	-e 's/^ //g'`
    fi
  fi
}

# Check current user is ingres
check_user()
{
  [ "$WHOAMI" != "(PROD2NAME)" ] && \
    bail_out 1 Must run $CMD as user (PROD2NAME)
}

# Check whether Ingres installation already running. Just do a simple name
# server count.
check_ingres_running()
{
  IINAMU_FILTER="sed 's/ *(.*)//'"
  if [ "$MAJOR_VERSION" = "6.4" ]; then
    NS_CHECK=runiinamu
  else
    NS_CHECK=iigcfid
  fi

  # Get name server status
  gcn_count=`eval "$NS_CHECK iigcn | $IINAMU_FILTER | wc -w"`

  if [ $gcn_count -ge 1 ] ; then
    bail_out 1 Must shutdown (PROD1NAME) before installing patch
  fi
}

# Check disk space required in destination directory. 
# Extra disk space with backup = total size of patch + scripts to be copied.
# Extra disk space without backup = size of patch minus size of destination
# files.
# This function assumes we're copying from the current directory.
check_disk_space()
{
  destination=$1

  if [ "$MAJOR_VERSION" = "6.4" ]; then
    DISKFREE=$destination/utility/diskfree
  else
    DISKFREE=$destination/utility/iidsfree
  fi

  # Strip df header and take "avail" column
  free_space=`$DISKFREE $destination`

  # Need to check size of source & destination files
  dest_total=0
  curr_dest_file=0
  src_total=0
  curr_src_file=0
  for file in `cat $PLIST`; do
    if [ -f $destination/$file ]; then
      curr_dest_file=`ls -l $destination/$file | eval $AWKBYTES`
      dest_total=`expr $curr_dest_file + $dest_total`
    fi
    if [ -f $file ]; then
      curr_src_file=`ls -l $file | eval $AWKBYTES`
      src_total=`expr $curr_src_file + $src_total`
    else
      bail_out 1 File $file mentioned in plist but does not exist
    fi
  done

  if [ "$BACKUP" = "true" ]; then
  # We're copying install script & plist to backup directory. Need to
  # take account of these.
  extra_total=0
  curr_extra_file=0
  for extra_file in $CMD $PLIST; do
    curr_extra_file=`ls -l $extra_file | eval $AWKBYTES`
    extra_total=`expr $curr_extra_file + $extra_total`
  done
    
  # Need to calculate extra space for patch files
    required_bytes=`expr $src_total + $extra_total`
  else
  # No backups
  # Calculate patch files minus installation files
    required_bytes=`expr $src_total - $dest_total`
  fi

  required_space=`expr $required_bytes \/ 1024`

  if [ $free_space -lt $required_space ]; then
    bail_out 1 Not enough disk space to install patch
  fi
}

# Set setuid bits on known setuid programs.
set_perms()
{
  destination=$1

  if [ "$MAJOR_VERSION" = "6.4" ]; then
    SETUID="bin/accessdb bin/iistar bin/wakeup bin/createdb bin/destroydb bin/finddbs bin/upgradedb bin/conv60ph bin/verifydb bin/iimerge utility/ciutil utility/csspintest utility/csshmemtest bin/kill_ing bin/lkmemtest bin/lktest bin/iifsgw"
    MODE="4711"
  else
  # Assume this is 1.2 or higher. This will need reviewing.
    SETUID="bin/verifydb bin/upgradedb bin/iimerge bin/starview utility/csreport sig/star/wakeup"
    MODE="4755"
  fi

  for file in $SETUID; do
    [ `grep "^$file$" $PLIST` ] && \
      [ -w $destination/$file ] && \
      chmod $MODE $destination/$file
  done
}

# Copy files from source to destination using cpio in pass mode.
copy_files()
{
  source=$1
  destination=$2
  

  # Copy install script & plist to backup area
  # There may be files in the patch which aren't in the installation.
  # Need to build up a temporary list of files which exist in the installation.
  if [ "$BACKUP" = "true" ]; then
    cp $CMD $destination

    touch /tmp/plist.$$
    for file in `cat $PLIST`; do
      if [ -r $source/$file ]; then
	echo $file >> /tmp/plist.$$
      fi
    done
    LIST=/tmp/plist.$$
    cp $LIST $destination/plist
  else
    LIST=$PLIST
  fi

  cat $LIST | (cd $source; cpio -pdmu $destination 2>/dev/null) || \
    bail_out 1 Problem reported by cpio

  [ "$BACKUP" = "true" ] && rm -f /tmp/plist.$$
}

# Perform final checksum comparing patch & destination files.
check_file_sums()
{
  destination=$1

  error_count=0
  for file in `cat $PLIST`; do
    source_sum=`sum -r $file | awk '{printf "%s\t%s", $1, $2}'`
    dest_sum=`sum -r $destination/$file | awk '{printf "%s\t%s", $1, $2}'`
    if [ "$source_sum" != "$dest_sum" ]; then
      error_count=`expr $error_count + 1`
      echo "Checksum mismatch"
      echo "	$source_sum	$file"
      echo "	$dest_sum	$destination/$file"
    fi
  done
  [ $error_count -gt 0 ] && \
    bail_out 1 Checksum mismatch
}

# Use version.rel to keep a history of patch installations.
# If it doesn't exist, create it. File will be of form:
# Version:	<version string>
# Patch:	<patch number>
#
# Installed:	<date>
#
# patchid	filename	checksum
# .
# .
#
# Subsequent patch info will be placed at the top, and the previous info	# will be shifted down.
update_version_rel()
{
  upd_destination=$1

  # Re-get the version & patch info from version.rel supplied with the patch
  get_version_level $SOURCE
  get_patch_level $SOURCE
  # get_version_level will prompt if it can't figure out the version level
  # get_patch_level won't however, so prompt if PATCH_ID=""
  while [ "$PATCH_ID" = "" ]; do
    echo "Please enter the number of the patch about to be installed, e.g 1234:"
    read PATCH_ID junk
  done
  # Create a temporary file with new information in
  cat <<-EOF > /tmp/version.rel$$
Version:	$VERSION
Patch:		$PATCH_ID
Installed:	`date`

EOF
  for file in `cat $PLIST`; do
    echo "$PATCH_ID	`sum -r $upd_destination/$file | awk '{printf "%s\\t%s\\t%s\\n", $1, $2, $3}'`" >> /tmp/version.rel$$
  done
  # Cat old version.rel onto new, if it exists
  if [ -r /tmp/version.rel_backup$$ ]; then
    cat <<-EOF >> /tmp/version.rel$$

-------------------------------------------------------------------------------
Previous version:

EOF
    cat /tmp/version.rel_backup$$ >> /tmp/version.rel$$
  fi
  # Copy new version.rel into place
  cp /tmp/version.rel$$ $upd_destination/version.rel
  rm -f /tmp/version.rel$$
}

###############################################################################t
# Main program

get_args "$*"

echo "Starting (PROD1NAME) patch installation"
echo ""

check_iisystem_set
setup_vars
check_user
check_plist
get_version_level $DESTINATION
check_ingres_running
check_disk_space $DESTINATION

# Backup files if necessary
if [ "$BACKUP" = "true" ]; then
  echo "Backing up installation files..."
  echo ""
  BACKUP_DIR=$DESTINATION/install/backup

  get_patch_level $DESTINATION
  # Need to get patch id to name backup directory
  if [ "$PATCH_ID" = "" ]; then
    $ECHO_CMD "Does the existing installation already have a patch installed? (y/n) [y]: $ECHO_CHAR"
    read reply junk
    if [ "$reply" = "n" ] || [ "$reply" = "no" ]; then 
      PATCH_ID=base_release
    else
      # Need to prompt for current patch level
      echo "Please enter the number (i.e 1234) of existing patch level:"
      read PATCH_ID junk
      [ "$PATCH_ID" = "" ] && \
	bail_out 1 Must supply existing patch number
      PATCH_ID="patch$PATCH_ID"
    fi
  else
    PATCH_ID="patch$PATCH_ID"
  fi
  BACKUP_DIR="$BACKUP_DIR/$PATCH_ID"
  # Create backup directory structure
  echo "Default backup directory:"
  echo "	$BACKUP_DIR"
  $ECHO_CMD "Is this correct? (y/n) [y]: $ECHO_CHAR"
  read reply junk
  if [ "$reply" = "n" ] || [ "$reply" = "no" ]; then
    echo "Please enter alternate backup directory:"
    read BACKUP_DIR junk
  fi
  echo ""
  [ -d "$BACKUP_DIR" ] && \
    bail_out 1 Directory $BACKUP_DIR already exists
  mkdir -p $BACKUP_DIR || \
    bail_out 1 User $WHOAMI cannot create directory $BACKUP_DIR
  # Assume disk space OK
  # Copy files mentioned in plist from installation to backup directory
  copy_files $DESTINATION $BACKUP_DIR
fi

# Copy the files to the installation

echo "Installing patch..."
echo ""

BACKUP=false
[ -r $DESTINATION/version.rel ] && \
  cp $DESTINATION/version.rel /tmp/version.rel_backup$$
copy_files $SOURCE $DESTINATION
set_perms $DESTINATION
check_file_sums $DESTINATION
update_version_rel $DESTINATION
[ -f /tmp/version.rel_backup$$ ] && \
  rm -rf /tmp/version.rel_backup$$

echo "(PROD1NAME) patch installation was successful"
echo "Please read patch documentation for further instructions"
