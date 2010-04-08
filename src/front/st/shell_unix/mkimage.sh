:
#
#  Copyright (c) 2004 Ingres Corporation
#
#  Name: mkimage.sh -- Ingres subset bundler.
#
#  Usage:
#       mkimage
#
#  Description:
#       This script forms part of the subset installer utility
#       which allows non-Ingres product groups to create
#       cut down Ingres save sets from either the base release
#       or subsequent patches.
#
#  Exit Status:
#       0       Ingres subset created successfully OR
#               Ingres feature list generated
#       1       Ingres subset creation did not complete.
#
## History:
##      12-Jan-2001 (hanal04) SIR 102895
##              Created.
##	30-Jan-2001 (hanch04)
##	    Correct typo.
##      31-Jan-2001 (hanal04) SIR 102895
##              Allow a different temporary working directory
##              to be specified with the -t flag.
##              Use pwd to determine existing dir not value of
##              $PWD. AIX does not maintain PWD.
##	10-Jun-2003 (hanje04)
##	    When checking saveset structure we want to examine the rightmost
##	    element of the tar tvf line which is not always the 8th, so 
##	    replace $8 with $n. (It's 6th on Linux)
##	17-sep-2004 (hayke02) INGINS 256, bug 113070
##	    In unpack(), run iichksum against the new install.tar.Z's and
##	    substitute in the CHECKSUM and SIZE values into the new
##	    release.dat. Also move the release.dat modification to outside
##	    the subdir 'do' loop so that the CHECKSUM/SIZE modifications
##	    do not stop when we reach the install dir.
##	21-sep-2004 (hayke02) INGINS 257 bug 113094
##	    In convert_file_list(), replace $n with $NF (number of fields) when
##	    we want to awk out the rightmost element of the tar tvf output -
##	    HP doesn't like $n.
##	23-sep-2004 (hayke02) INGINS 258 bug 113116
##	    Remove the -p (permission/ownership) flag from the tar commands.
##	    Also some minor changes to remove the -v flag from all tar
##	    commands, replace tar with $II_SYSTEM/ingres/install/gtar so that
##	    we now use gtar for both buildrel and mkimage, and redirect the tar
##	    extraction stderr (error trying to recreate the install directory)
##	    to /dev/null.
##	    We don't supply gtar in r3 so use tar instead of gtar
##	17-Mar-2005 (bonro01)
##	    Update mkimage to handle new pax filetype
##	    Allow tmp directory to be a relative path
##	03-Aug-2005 (bonro01)
##	    Use $NF instead of $n for the number of fields per line, $NF
##	    appears to be a more common syntax for most platforms.
##	    Fix for AIX which does not use pax.
##	    Fix for HP to route tar error output to the logfile.
##      08-Feb-2006 (hweho01)
##          Update the filetype for AIX platform.
##      31-May-2006 (hweho01)
##          Update the filetype for Tru64 platform.
##       6-Dec-2007 (hanal04 & hayke02) Bug 119572
##          Modify unpack()'s identification of required packages to be
##          based off of pkg list instead of the file list. The file list
##          may contain files that exist in multiple packages and a subset
##          of those packages may have been requested.
##          Added call to iisysdep to establish AWK and replaced all calls
##          to awk accordingly.
##          ls -l on Linux has file names in field 8 not 9. Modified call
##          to us $NF as the last column is consistently the file name.
##          Linux does compress tar files. It has tar files with
##          compression. .tar.Z vs .pgz. Need to skip calls to
##          compress and uncompress on Linux and build the compression
##          and decompress into the tar commands.
#
#  DEST = utility
#
. iisysdep

tmp_abs=none
oldpwd=`pwd`

fail_exit()
{
   cd $oldpwd
   if [ -d $top_unloaddir ] ; then
       echo Removing temporary working directories . . . . .
       rm -rf $top_unloaddir
   fi
   echo Ingres Subset creation failed . . . . .
   exit 1
}

# Generate the absolute file name
absolute()
{
    first_char=`echo $1 | cut -c 1`
    first_two_chars=`echo $1 | cut -c 1-2`
    if [ "${first_char}" = "/" ] ; then
        tmp_abs=$1
    else
        # fake the absolute path with pwd + relative path
        if [ "${first_two_chars}" = "./" ] ; then
            shorted=`echo $1 | sed -e 's/\.\///'` 
            tmp_abs=${oldpwd}/${shorted}
        else
            tmp_abs=${oldpwd}/$1
        fi
    fi
}

# Define globals
subbase=iisubset
exedir=`dirname $0`
iisub=$exedir/$subbase
absolute $iisub
exedir=`dirname $tmp_abs`
log_dir=$exedir/mkimage_log
# Create the log directory if it does not exist
if [ ! -d "$log_dir" ] ; then
    mkdir $log_dir
else
    rm -rf $log_dir
    mkdir $log_dir
fi

top_unloaddir=/tmp/iisubsetunload
unloaddir=${top_unloaddir}/ingres
source_saveset=ingres.tar
source_default=true
source_tar=true
source_compressed=false
dest_saveset=subset.tar
sdest_saveset=subset.tar
dest_default=true
dest_tar=true
dest_compressed=true
dest_tar='.tar'
dest_Z='.Z'
ext_Z='.Z'
list_req=false
list_pkgs=false
list_cust=false
list_desc=false
subset_req=false
subset_ids=none
source_base_format=false
source_patch_format=false
pdir=patchXXXX
unames=`uname -s`

# Default use compress and uncompress. Override if needed (Linux)
uncompcmd="uncompress"
compcmd="compress"
tarzxf='tar -xf'
tarzcf='tar -cf'
iflag='-I'
cflag='-F'

if [ "$unames" = "AIX" -o "$unames" = "OSF1" ] ; then
    ext_tar=.gtar
else
    if [ "$unames" = "Linux" ] ; then
        ext_tar=.pgz
        ext_Z=''
        uncompcmd=''
        tarzxf='tar -Zxf'
        compcmd=''
        tarzcf='tar -Zcf'
        iflag='-T'
        cflag=''
    else
        ext_tar=.pax
    fi
fi

subinstfile=install${ext_tar}
filelist=output.log
pkg_list=pkg.log
subfiles=suboutput.log
tmpfile=tmp.txt

releasedat=release.dat
packagedat=package.dat
newreleasedat=new.dat

# Check mkimage lives in $II_SYSTEM/ingres/install
tmp_basename=`basename $exedir`
if [ "$tmp_basename" != "install" ] ; then
    echo mkimage must reside in \$II_SYSTEM/ingres/install
    echo Saveset creation terminated . . . . .
    exit 1
fi
II_SYSTEM=`dirname $exedir`
tmp_basename=`basename $II_SYSTEM`
if [ "$tmp_basename" != "ingres" ] ; then
    echo mkimage must reside in \$II_SYSTEM/ingres/install
    echo Saveset creation terminated . . . . .
    exit 1
fi
II_SYSTEM=`dirname $II_SYSTEM`
echo II_SYSTEM set to $II_SYSTEM
export II_SYSTEM



# Usage function
usage()
{
echo
echo 'Usage: mkimage [flags]'
echo
echo ' Flags;'
echo
echo '  -r <list>          Build a subset distribution file which contains'
echo '                     the packages in the list.'
echo '  -d dest_saveset    Optionally used with -r to specify the name of'
echo '                     the subset distribution file. If not used the'
echo '                     default file name ./subset.tar is used.'
echo '  -s src_saveset     Optionally used with -r to specify the name of'
echo '                     the original distribution file. If not used the'
echo '                     default file name ./ingres.tar is used.'
echo '  -t dir_name        Optionally used with -r to specify the name of'
echo '                     an alternative temporary work location. Default'
echo '                     is /tmp'
echo '  -l                 Lists all of the Ingres product feature names'
echo '  -c                 List Custom Packages.'
echo '  -p                 List Express Packages.'
echo '  -v <list>          Display descriptions of the packages in the list.'
echo
echo 'In the above <list> represents a comma-separated list of product'
echo 'feature names.'
echo
exit 1
}

# Convert the file list into one usable by tar
# Strip out package names.
convert_file_list()
{
   echo Removing duplicates and extracting package names . . . . .
   # Remove any duplicates
   sort -u $log_dir/$filelist > $log_dir/$tmpfile
   mv $log_dir/$tmpfile $log_dir/$filelist
   # Place PACKAGE entries into pkg_list
   grep "^PACKAGE" $log_dir/$filelist > $log_dir/$pkg_list
   # Remove PACKAGE entries from filelist
   grep -v "^PACKAGE" $log_dir/$filelist > $log_dir/$tmpfile
   mv $log_dir/$tmpfile $log_dir/$filelist

   echo Concatenating directory structures and file names . . . . .
   sed -e 's/ /\//' -e 's/ //g' $log_dir/$filelist > $log_dir/$tmpfile
   mv $log_dir/$tmpfile $log_dir/$filelist

   echo Determining the source save set type . . . . .
   if [ "$source_compressed" = "true" ] ; then
       pdir=`zcat $source_saveset | tar -tf - | grep "\/bin\/" | head -1 | $AWK '{print $NF}'`
   else
       pdir=`tar -tf $source_saveset | grep "\/bin\/" | head -1 | $AWK '{print $NF}'`
   fi
   if [ "$pdir" = "" ] ; then
       if [ "$source_compressed" = "true" ] ; then
           pdir=`zcat $source_saveset | tar -tf - | grep "install\/" | head -1`
       else
           pdir=`tar -tf $source_saveset | grep "install\/" | head -1`
       fi
       if [ "$pdir" = "" ] ; then
           echo Cannot determine directory structure within source save set
           echo Terminating save set production . . .
           rm $log_dir/$filelist
           fail_exit
       else
           source_base_format=true
       fi
   else
       source_patch_format=true
       pdir=`expr $pdir : '\(.*\)\/bin\/'`
   fi

   # If we have base release format then ingres/.... wil be the existing
   # diectory structure so only find/replace in patch format..
   if [ "$source_patch_format" = "true" ] ; then
       sed -e s/'^ingres\/'/"$pdir\/"/ $log_dir/$filelist > $log_dir/$tmpfile
       mv $log_dir/$tmpfile $log_dir/$filelist

       # Manually add the plist, version.rel and iiinstaller to the filelist
       echo $pdir\/utility\/plist >> $log_dir/$filelist
       echo $pdir\/utility\/iiinstaller >> $log_dir/$filelist
       echo $pdir\/version.rel >> $log_dir/$filelist
       echo $pdir\/patch.doc >> $log_dir/$filelist
   fi

}

check_source_file()
{
   # We assume the source set is a tar file. If this does not hold
   # true additional checks will be needed to set source_tar accordingly.
   if [ -f $source_saveset ] ; then
       filespec=`file $source_saveset | grep -i compress`
       if [ "$filespec" != "" ] ; then
           source_compressed=true;
       fi
       if [ $source_default = "false" ] ; then

           absolute $source_saveset
           source_saveset=$tmp_abs
       fi
   else
       echo The src_saveset file $source_saveset can not be found.
       fail_exit
   fi
}

check_dest_file()
{
   absolute $dest_saveset
   dest_saveset=$tmp_abs
   if [ -f $dest_saveset ] ; then
       echo The dest_saveset file $dest_saveset already exists.
       echo Terminating save set creation . . .
       fail_exit
   fi

   name=`basename $dest_saveset`
   ext=`expr $name : "\(.*\)${dest_Z}"`
   if [ "$ext" = "" ] ; then
       dest_compressed=false
   else
       dest_compressed=true
       # Strip the .Z - compress will add it back later
       dir=`dirname $dest_saveset`
       if [ "X$compcmd" != "X" ] ; then
          dest_saveset=$dir/$ext
       fi
   fi

   if [ -f $dest_saveset ] ; then
       echo The tar file $dest_saveset already exists.
       echo Terminating save set creation . . .
       fail_exit
   fi
   
   ext=`expr $name : "\(.*\)${dest_tar}${dest_Z}"`
   if [ "$ext" = "" ] ; then
       if [ "$dest_compressed" = "true" ] ; then
           echo Invalid dest_saveset file name.
           echo A .Z extension may only be used in conjunction with tar
           echo E.g. filename.tar.Z
           fail_exit
       fi
   fi

   # The destination will always be a tar file.
   ext=`expr $name : "\(.*\)${dest_tar}"`
   if [ "$ext" = "" ] ; then
       dest_saveset=${dest_saveset}${dest_tar}
       dest_tar=true
   else
       dest_tar=true
   fi

}

# ###################################################################
#  unpack() unpacks the source saveset and makes any changes required
#           prior to repack()
# ###################################################################
unpack()
{
   echo Unpacking source save set . . . . .
   if [ "$source_base_format" = "true" ] ; then
       # Must determine which .pax.Z's we need in an ingres.tar
       # dump the content into a tmp subdir so we can rebuild
       # the .pax.Z with only the files we want
       # Clear down the files we don't want
       # must mimick the ingres files structure

       #unpack the whole lot
       tar -xf $source_saveset > $log_dir/xtar_src.log 2>&1
       
       # Assumptions: ingres.tar not contain files in the top directory
       #             the file name on ls -l is the last entry of output.
       #             All directories other than install contain a
       #             files called install.pax.Z
       subdir_list=`ls -l | grep "^d" | $AWK '{print $NF}' | grep -v "^\."`

       echo Removing unwanted files . . . . .
       for subdir in $subdir_list
       do
           need_pkg=false
           # Assume we will always bundle install 
           if [ "$subdir" != "install" ] ; then
               cd $subdir > /dev/null
               if [ "X$uncompcmd" != "X" ] ; then
                  $uncompcmd ${subinstfile}${ext_Z}
               fi
               $tarzxf $subinstfile 1> $log_dir/xtar_$subdir.log 2>&1
               for pkg in `cat $log_dir/$pkg_list | $AWK '{print $2}'`
               do
                   if [ "$pkg" = "$subdir" ] ; then
                       need_pkg=true
                   fi
               done
               if [ "$need_pkg" = "true" ] ; then
                   rm -rf ingres
                   if [ "X$compcmd" != "X" ] ; then
                       $compcmd $subinstfile
                   fi
               else
                   cd $oldpwd
                   cd $unloaddir
                   rm -rf $subdir 
               fi

               # Move back to unloaddir for next iteration of the loop
               cd $oldpwd
               cd $unloaddir
           fi 
       done

       # Deal with install after all other dirs.
       subdir='install'
       cd $subdir

       # Strip down the release.dat
       # Copy the RELEASE line at the top of release.dat
       head -1 $releasedat > $newreleasedat 
       # Copy the package details for all requested packages
       for pkg in `cat $log_dir/$pkg_list | $AWK '{print $2}'`
       do
	   if [ -f $top_unloaddir/ingres/$pkg/$subinstfile.Z ] ; then
		chksum=`$exedir/iichksum $top_unloaddir/ingres/$pkg/$subinstfile.Z | grep -v FILE`
		checksum=`echo $chksum | $AWK '{print $2}'`
		size=`echo $chksum | $AWK '{print $4}'`
	   fi
           sed -n -e "/^PACKAGE        $pkg$/,/PACKAGE/p" release.dat | $AWK \
                         'BEGIN { i=0 }
           	{
                    if  (( $1 == "PACKAGE" ) && ( i < 1 ) )
                        { i = 1; print $0 }
                    else if ( $1  != "PACKAGE")
                        print $0
                }' > $packagedat
	   cat $packagedat |\
		   sed -e "1,4 s/CHECKSUM  *[0-9]*/CHECKSUM $checksum/"\
		   -e "1,4 s/SIZE  *[0-9]*/SIZE $size/"\
		   >> $newreleasedat
       done

       # Swap out the old release.dat and replace with new version
       chmod 777 $releasedat
       rm $releasedat
       mv $newreleasedat $releasedat

       # Now remove PREFER entries where the PACKAGE is not defined
       # install/ingbuild will not work if they are left in place.
       # First we build the remove list.
       prefer_list=`cat $releasedat | grep PREFER | $AWK '{print $2}'`
       pkg_list=`cat $releasedat | grep PACKAGE | $AWK '{print $2}'`
       remove_list=''
       for prefer in $prefer_list
       do
           pkg_defined=false
           for pkg in $pkg_list
           do
               if [ "$prefer" = "$pkg" ] ; then
                   pkg_defined=true
               fi
           done
           if [ "$pkg_defined" = "false" ] ; then
               remove_list=$remove_list" $prefer"
           fi
       done
       # Now rip out the PREFER entries that need to be stripped.
       for remove in $remove_list
       do
           grep -v "PREFER.*$remove" $releasedat > $newreleasedat
           rm $releasedat
           mv $newreleasedat $releasedat
       done

       # Restore the permissions on the release.dat
       chmod 644 $releasedat

       # Move back to unloaddir for next iteration of the loop
       cd $unloaddir
   else
       # We must have a patch format
       # Extract the required files from the source save set
       echo Extracting required files from source save set . . . . .
       if [ "$source_compressed" = "true" ] ; then
           zcat $source_saveset | tar -xf - $iflag $log_dir/$filelist \
                > $log_dir/xtar_src.log 2>&1
       else
           tar -xf $source_saveset $iflag $log_dir/$filelist  \
                > $log_dir/xtar_src.log 2>&1
       fi
     
       # Rebuild a new plist which only has the required files
       # We only unpack the required files so if the file in the
       # plist entry does not exit don't write it to the new plist
       echo Stripping redundant entries from the Packing list file . . . . .
       cd $pdir
       today=`date`
       echo "## Contents: REGENERATED Packing list" > utility/plist.new
       echo "##" >> utility/plist.new
       echo "## Performed:        $today" >> utility/plist.new
       echo "## Original Saveset: $source_saveset" >> utility/plist.new
       echo "## Rebuilt Saveset:  $sdest_saveset" >> utility/plist.new
       echo "## Feature List:     $subset_ids" >> utility/plist.new
       echo "##" >> utility/plist.new
       echo "## ORIGINAL Packing list details below." >> utility/plist.new
       echo "##" >> utility/plist.new

       cat utility/plist|$AWK '{ if ($1 == "##") 
               {print $0}
           else 
               {file=$1; 
                if (system("test -f " file) == 0) 
                    { print $0 }
               }}' >> utility/plist.new

       chmod 777 utility/plist
       rm utility/plist
       mv utility/plist.new utility/plist
       chmod 444 utility/plist
       cd ..
   fi

}

repack()
{
   # Add extracted files to destination save set
   echo Bundling the destination save set . . . . .
   if [ "$dest_compressed" = "true" ] ; then
       $tarzcf $dest_saveset * > $log_dir/ctar_dest.log 2>&1
       # Clear down the top_unloaddir early to make space for compress
       cd $oldpwd
       if [ -d $top_unloaddir ] ; then
           rm -rf $top_unloaddir
       fi
       if [ "X$compcmd" != "X" ] ; then
           $compcmd $cflag $dest_saveset
       fi
       # Add the .Z back to the variable dest_saveset
       dest_saveset=${dest_saveset}${ext_Z}
   else
       tar -cf $dest_saveset * > $log_dir/ctar_dest.log 2>&1
   fi
}

unpack_repack()
{
   cd $oldpwd
   rm -rf $top_unloaddir > /dev/null
   mkdir -p $unloaddir
   if [ ! -d $unloaddir ] ; then
       echo Creation of temporary working directory $unloaddir failed.
       echo Terminating save set production . . .
       fail_exit
   else
       echo Created temporary work location $unloaddir
   fi

   cd $unloaddir

   unpack
   repack

   echo Removing temporary working directories . . . . .
   cd $oldpwd
   if [ -d $top_unloaddir ] ; then
       rm -rf $top_unloaddir
   fi
   echo Destination save set complete . . . . .
   exit 0
}

rebundle()
{
   convert_file_list;
   unpack_repack;	# Will exit
}


# ###################
#  Main body
# ###################
 
# resolve parms
num_args=$#

if [ $num_args = 0 ] ; then
        usage;
fi
while [ $# -gt 0 ]
do
        case "$1" in
                -l)
                        if [ $num_args -gt 1 ] ; then
                                usage;
                        fi;
                        list_req=true;
                        shift;;
                -c)
                        if [ $num_args -gt 1 ] ; then
                                usage;
                        fi;
                        list_cust=true;
                        shift;;
                -p)
                        if [ $num_args -gt 1 ] ; then
                                usage;
                        fi;
                        list_pkgs=true;
                        shift;;
                -d)
                        # -d must have a following parameter.
                        if [ "$2" = "" ] ; then
                                usage;
                        fi;
                        if [ $num_args -lt 4 ] ; then
                                usage;
                        fi;
                        shift;
                        dest_default=false;
                        dest_saveset=$1;
                        sdest_saveset=$1
                        shift;;
                -s)
                        # -s must have a following parameter.
                        if [ "$2" = "" ] ; then
                                usage;
                        fi;
                        if [ $num_args -lt 4 ] ; then
                                usage;
                        fi;
                        shift;
                        source_default=false;
                        source_saveset=$1;
                        shift;;
                -t)
                        # -t must have a following parameter.
                        if [ "$2" = "" ] ; then
                                usage;
                        fi;
                        if [ $num_args -lt 4 ] ; then
                                usage;
                        fi;
                        shift;
                        if [ -d $1 ] ; then
                                top_unloaddir=$1/iisubsetunload
                                unloaddir=${top_unloaddir}/ingres
                        else
                                usage;
                        fi;
                        shift;;
                -v)
                        if [ $num_args != 2 ] ; then
                                usage;
                        fi;
                        if [ "$2" = "" ] ; then
                                usage;
                        fi;
                        list_desc=true;
                        shift;
                        subset_ids=$1;
                        shift;;
                -r)
                        if [ "$2" = "" ] ; then
                                usage;
                        fi;
                        subset_req=true;
                        shift;
                        subset_ids=$1;
                        shift;;
                *)
                        usage;  # Will cause script to exit.
                        shift;;
                esac
done
 
# User request for a list of Ingres components
   if [ "$list_req" = "true" ] ; then
       $exedir/$subbase -all
       exit 0
   fi

# User request for a list of Express Packages
   if [ "$list_pkgs" = "true" ] ; then
       $exedir/$subbase -packages
       exit 0
   fi

# User request for a list of Custom Packages
   if [ "$list_cust" = "true" ] ; then
       $exedir/$subbase -custom
       exit 0
   fi

# User request for a description of specified Packages
   if [ "$list_desc" = "true" ] ; then
       $exedir/$subbase -describe=$subset_ids
       exit 0
   fi

# Try to Get the file list for the supplied list of elements.
# Before we start let's check the source and destination files
# won't cause us a problem.
   if [ "$subset_req" = "true" ] ; then
       check_source_file
       check_dest_file
       if $exedir/$subbase -resolve=$subset_ids > $log_dir/$filelist ; then
           echo  Generating file lists . . .
           rebundle;
       else
# Error in iisubset's attempt to get file list
           cat $log_dir/$filelist
           exit 1
       fi
   fi
