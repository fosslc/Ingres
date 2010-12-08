:
#
# Copyright (c) 2004 Ingres Corporation
#
# Tar the ingres files for ingbuild
#
# DEST = utility
#
# History:
# --------
#	24-Oct-94 (GordonW)
#		created.
#	10-Nov-94 (GordonW)
#		exit if tar failes.
#	7-feb-94 (mnorton)
#		Create a packing list because not all tar commands accespt 'u'.
#	16-June-95 (chimi03)
#		For hp8_us5, use the gtar command with the -T option instead
#		of the tar command with the -I option.  On hp, the tar
#		command does not support the -I functionality.
#	5-July-95 (chimi03)
#		Fix a typo in the above fix.  Added the 'esac' statement 
#		to terminate the 'case'.
#	28-Sep-95 (thoro01)
#		For ris_us5, use the gtar command with the -T option instead
#               of the tar command with the -I option.  See 16 June update.
#	17-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
#		Added axp_osf to use gtar.
#       21-may-1997 (musro02)
#               Add sqs_ptx to not use gtar -I
#       29-aug-1997 (mosjo01)
#               Add sos_us5 to not use gtar -I
#	24-sep-1997 (mosjo01)
#		For rs4_us5, use gtar command with the -T option. Also use
#		echo "install" > /tmp/packlist so that gtar will properly 
#		create an ingres.tar that is acceptable to the tar xpf cmd.
#		Seems that tar xpf behaves differently between AIX 3.2 & 4.1.
#	17-feb-1998 (fucch01)
#		Add sgi_us5 to not use gtar -I
#	26-feb-1998 (toumi01)
#		Add Linux (lnx_us5) to use gtar.
#	26-Mar-1998 (muhpa01)
#		Adde new HP config, hpb_us5, to platforms using gtar.
#	13-May-99 (GordonW)
#		Use real tar filename, not ingres.tar
#	06-oct-1999 (toumi01)
#		Change Linux config string from lnx_us5 to int_lnx.
#	28-apr-2000 (somsa01)
#		Enabled for multiple product builds.
#	23-May-2000 (hweho01)
#		Add support for AIX 64-bit platform (ris_u64).
#	15-Jun-2000 (hanje04)
#		Add ibm_lnx to use gtar
#	11-Sep-2000 (hanje04)
#		Alpha Linux to use gtar.
#	20-jul-2001 (stephenb)
#		Add support for i64_aix
#	03-Dec-2001 (hanje04)
#		Added support for IA64 Linux (i64_lnx)
#	11-dec-2001 (somsa01)
#		Changes for hp2_us5.
#	06-Nov-2002 (hanje04)
#		Added support for AMD x86-64 Linux and genericise Linux defines
#		where apropriate.
#	07-Feb-2003 (bonro01)
#		Added support for IA64 HP (i64_hpu)
#	15-Jul-2004 (hanje04)
#		Define DEST=utility so file is correctly pre-processed
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#	27-Oct-2004 (bonro01)
#	    Remove use of gtar and qasetuser.
#	16-May-2004 (hanje04)
#	    Don't use -I on Mac OS X
#        6-Nov-2006 (hanal04) SIR 117044
#           Add int.rpl for Intel Rpath Linux build.
#	08-Feb-2008 (hanje04)
#	    SIR S119978
#	    Add suport for Intel OS X
#	22-Jun-2009 (kschendel) SIR 122138
#	    Config is now primary (new hybrid scheme), fix here.
#	18-Mar-2010 (frima01) SIR 122138
#	    Try to make filename for packlist unique to avoid
#	    mutual interferences.
##	23-Nov-2010 (kschendel)
##	    drop obsolete ports.
#
 
[ $SHELL_DEBUG ] && set -x

if [ "$(PROG3PRFX)_RELEASE_DIR" = "" ]
then
	echo "taringres: Must define (PROG3PRFX)_RELEASE_DIR..."
	exit 1
fi

tarfile=""
compress_it=false
while [ ! "$1" = "" ]
do
	if [ "$1" = "-compress" ]
	then
		compress_it=true
	elif [ "$1" = "-nocompress" ]
	then
		compress_it=false
	else
		tarfile=$1
	fi
	[ "$2" = "" ] && break
	shift
done

[ "$tarfile" = "" ] && 
{	
	echo "usage: taringres tarfile"
	exit 1
}

# get the version string for later use
. readvers
vers=$config

fname=`basename $tarfile`
fdir=`dirname $tarfile`
[ "$fdir" = "." ] && fdir=`pwd`
tarfile=$fdir/$fname

echo "Creating packing list for $fnam ..."

# Make sure packlist doesn't exist
packlistfile=/tmp/packlist.$$
rm -f $packlistfile
trap "rm -f $packlistfile; exit 1" 1 2 3 15

# Put install tools in first 
cd $(PROG3PRFX)_RELEASE_DIR
case $vers in
   rs4_us5|r64_us5)
            echo "install"  > $packlistfile 
            ;;
   *)       ls -1 install/* > $packlistfile 
            ;;
esac


# Put other files in packlist
files="`ls 2>/dev/null`"
for d in $files
do
	[ ! -d $d ] && continue
	[ "$d" = "install" ] && continue
	ls -1 $d/* >> $packlistfile
done

# Create the tar file
echo "Creating $fname ..."
case $vers in
   hp8_us5|rs4_us5|r64_us5|axp_osf|sgi_us5|hpb_us5|\
   hp2_us5|*_lnx|int_rpl|i64_hpu|*_osx)
	    tar -cvf $tarfile `cat $packlistfile` ||
            {
		echo "Tar command failed."
		exit 1
            } 
	    ;;
   *)       tar cvf $tarfile -I $packlistfile ||
            {
	       echo "Tar command failed."
	       exit 1
            } 
            ;;
esac

rm -rf $packlistfile

# Compress is required
if $compress_it 
then
        echo "Compressing $tarfile..."
        compress $tarfile ||
        {
                echo "Compress failed..."
                exit 1
        }
fi

exit 0
