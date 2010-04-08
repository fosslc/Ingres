:
#
# Usage : mkapirel
#
# Description
#
#    This script will make the deliverable for remote API on Unix.    
#
#    Before running this utility, an environment variable API_RELEASE_DIR 
#    must be defined, it points to the location where the package will be
#    made, also the file relremoteapi.dat must exists in $II_MANIFEST_DIR.
#
#
# History
#	29-Apr-2004 (hweho01)
# 	   Created for remote API package.
#	03-Dec-2004 (hanje04)
#	   Basic package is now a pax archive not tar. So it needs to be 
#	   repacked. Remove verbose flag from tar command also.
#	06-Feb-2007 (hweho01)
# 	   Updated the packaging process and API package name. 
#
. readvers
. iisysdep

       if [ "$VERS64" ]; then
         vers=$VERS64
       else
         vers=$VERS
       fi

       set - `cat $ING_BUILD/version.rel`
       release=$2


# Set up the API package environment

       [ "$API_RELEASE_DIR" = "" ] &&
       {
         API_RELEASE_DIR=$ING_ROOT/api_release/b${build}
         export API_RELEASE_DIR
       }

       if [ ! -d "$API_RELEASE_DIR" ]; then
          mkdir -p $API_RELEASE_DIR
       fi

       if [ ! -d "$API_RELEASE_DIR" ]; then
          echo "mkapirel: Could not create $API_RELEASE_DIR."
          exit 1
       fi

       if [ ! -f $II_MANIFEST_DIR/relremoteapi.dat ] ; then
          echo "mkapirel: $II_MANIFEST_DIR/relremoteapi.dat not found."
          exit 1
       fi


gen_full_release_manifest()
{
       echo "mkapirel: Generate a full release manifest file... "
       mkdir -p $API_RELEASE_DIR/temp_bld 
       II_RELEASE_DIR=$API_RELEASE_DIR/temp_bld
       export II_RELEASE_DIR
       buildrel || 
        {
	    echo "mkapirel: Can't do a buildrel for generating manifest file."
	    exit 1
        }
       if [ ! -f $II_RELEASE_DIR/install/release.dat ] ; then
         echo "mkapirel: Unable to generate full release manifest file..."
         exit 1
       fi
       cp $II_RELEASE_DIR/install/release.dat  $API_RELEASE_DIR/basic/ingres/install/. 
       rm -rf $II_RELEASE_DIR
}

#
#-[ Main - main body of script ]----------------------------------------------
#

       pwd_dir=`pwd`

       cd $API_RELEASE_DIR
       echo "mkapirel: Clean up release area..."
       rm -rf *   

# Select all the components according to the API manifest file 

       echo "mkapirel: Filling API release area..."
       buildrel -b relremoteapi.dat ||
       {
          echo "mkapirel: Can't do a buildrel for API release."
          exit 1
       }

# unpack archive file

       cd basic

       case $vers in
         int_lnx)
             pax -pp -rzf install.pgz
             desc="pc-linux-i386"
             ;;
         hp2_us5)
             zcat install.pax.Z | pax -pp -r 
             desc="hp-hpux-pa-risc"
             ;;
         su9_us5)
             zcat install.pax.Z | pax -pp -r 
             desc="sun-solaris-sparc"
             ;;
         r64_us5)
             zcat install.tar.Z | tar xpf -  
             desc="ibm-aix-powerpc"
             ;;
         *)
         echo "mkapirel: No api release available on this platform currently."
         exit 1
            ;;
       esac

# remove archive file 
       rm install.*

# Generate and include the full release manifest in the package.
# The full release manifest file will be used in the API functions 
# to determine the disk size.  
       mkdir -p $API_RELEASE_DIR/basic/ingres/install 
       gen_full_release_manifest	
       if [ ! -f $API_RELEASE_DIR/basic/ingres/install/release.dat ] ; then
          echo "mkapirel: Unable to generate full release manifest file..."
          exit 1
       fi

# repack archive using tar

       tar cf install.tar ingres
       compress install.tar

       api_tarfile=ingres-rmtapi-${release}-${build}-${desc}.tar.Z
       mv install.tar.Z $api_tarfile
       cp $api_tarfile  $pwd_dir/$api_tarfile  
       cd $pwd_dir
       
       exit 0

