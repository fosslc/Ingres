:
##
##  Copyright (c) 2004, 2008 Ingres Corperation. All rights reserved.
##
##  Name: mklicense.sh -- Generates Ingres licesing (click on license) script.
##
##  Usage:
##	mklicense [-gpl]|[-commercial]|[-embedded]
##
##  Description:
##	Generates LICENSE RPM and script which must be run and agreed to by 
##	users before they can install Ingres. By default the generated 
##	script contains the GPL license. There is also a commercial License
##	and an embedded license which can be chosen by passing the 
##	associated flags.
##
##	On running ingres-LICENSE script USER is presented with the apropriate
##	license and then asked if they accept it. If they do the LICENSE RPM
##	is silently installed otherwise the whole process is aborted.
##	
##  History:
##	30-Jan-2006 (hanje04)
##	    SIR 115688
##	    Created from mkcatosl.sh
##	01-Feb-2006 (hanje04)
##	    SIR 115688
##	    Correct flags to match usage in other utilities (e.g. buildrel)
##	01-Feb-2006 (hanje04)
##	    Update error message to correctly report missing file.
##	02-Feb-2006 (hanje04)
##	    Add -eval as a licensing option for evaluation license.
##	06-Feb-2006 (hanje04)
##	    Use $ING_SRC/LICENSE if LICENSE.${inglic} cannot be found.
##	23-Feb-2006 (hanje04)
##	    Replace r3 reference with Ingres 2006
##	07-May-2006 (hanje04)
##	   Bump version to ingres2007
##	16-Nov-2006 (rosan01)
##	   The use of tail +# was replaced with tail -# to comply with the
##	   POSIX standard.
##	22-Jan-2007 (bonro01)
##	    Change version back for ingres2006 release 3
##	11-Aug-2008 (hweho01)
##	    Change package name from ingres2006 to ingres for 9.2 release,
##	    use (PROD_PKGNAME) which will be substituted by jam during build.
##	10-Dec-2009 (hanje04)
##	    BUG 122944
##	    Don't actually install the RPM for now. Script is now being run
##	    before installer (again) but with RPM dependency.
INGVER=`ccpp -s ING_VER | cut -d' ' -f2`
ARCH=`uname -m`
inglic=gpl
self=`basename $0`

#
# - usage()
#
usage()
{
    cat << EOF
${self}

usage:
	${self} [-l gpl|com|enb|eval]|[-help]

	    where:
		  -l - specify license type
		    gpl - use GPL license file
	 	    com - use commercial license
		    emb - use embedded licesnse
		    eval - use evaluation licesnse
		-help - print this message

EOF
}
	    
#parse command line for license version
case $1 in
	 -help|\
	      ?)
		usage
		exit 0
		;;
	    -l)
		case $2 in
		    gpl|\
		    com|\
		   eval|\
		    emb)
			inglic=$2
			;;
		      *)
			usage
			exit 1
			;;
		esac
		shift ; shift
		;;
esac

case $ARCH in
    #IA32 Linux
    i*86) RPMARCH=i386
          ;;
    #IA64 Linux | AMD64 LINUX | S390 Linux
    *)    RPMARCH=$ARCH
          ;;
esac


. readvers
export INGLIC_EXE=ingres-LICENSE
trap 'rm -f $INGLIC_EXE' 1 2 3 4

if [ -z "$II_RPM_BUILDROOT" ] ; then
    echo II_RPM_BUILDROOT not set.
    exit 1
fi

export INGLIC_RPM=(PROD_PKGNAME)-license-${inglic}-${INGVER}-${build}.${RPMARCH}.rpm
export INGLIC_RPM_LOC=$II_RPM_BUILDROOT/${RPMARCH}/${INGLIC_RPM}

if [ -z "$II_RPM_PREFIX" ] ; then
    echo II_RPM_PREFIX not set.
    exit 2
fi

if [ -r "${ING_SRC}/LICENSE.${inglic}" ] ; then
    export INGLIC_TXT=LICENSE.${inglic}
else
    export INGLIC_TXT=LICENSE
fi

if [ ! -f ${ING_SRC}/$INGLIC_TXT ] ; then
    echo Cannot locate license text file: ${INGLIC_TXT}
    echo "Aborting.."
    exit 3
fi

if [ -f $INGLIC_RPM_LOC ] ; then
    export INGLIC_RPM_MD5=`md5sum $INGLIC_RPM_LOC | cut -d' ' -f1`
else
    echo Cannot locate (PROD_PKGNAME)-license-${inglic} RPM package
    echo "Aborting.."
    exit 4
fi


function do_gen_eula()
{
exec 1> $INGLIC_EXE

# Write header 

cat << EOF
#!/bin/sh
#
#  Copyright (c) 2006 Ingres Corporation. All rights reserved.
#
#  Name: $INGLIC_EXE -- setup script for Ingres Licensing.
#
#  Usage:
#       $INGLIC_EXE
#
#  Description:
#       User is presented with INGLIC and asked to accept it before Ingres can
#	be installed.
#	Must be run as root user.
#
#  Exit Status:
#       0       INGLIC accepted
#       1       Not run as root
#       2       INGLIC rejected
#       3       Failed to install INGLIC_RPM
#       4       INGLIC_RPM failed the md5sum

USER=\`whoami\`
INGLIC_RPM=`echo $INGLIC_RPM | cut -d- -f1,2,3,4`
INGLIC_RPM_LOC=/tmp/inglic_\$$/\$INGLIC_RPM
INGLIC_RPM_MD5=$INGLIC_RPM_MD5
trap 'rm -rf /tmp/inglic_\$$' 0 1 2 3 4 INT HUP

# Check if INGLIC has already been accepted
rpm -q \$INGLIC_RPM >& /dev/null 
[ \$? = 0 ] && accepted=y


if [ "\$USER" != "root" ] ; then
    echo You must be root to run this script
    exit 1
fi

mkdir -p /tmp/inglic_\$$

#Display License agreement
more  << !
EOF

#Cat eula into script

if [ -f ${ING_SRC}/${INGLIC_TXT} ] ;then
    cat ${ING_SRC}/$INGLIC_TXT
    echo "!"
else
    exit 1
fi

# Do they accept?

cat << EOF
#If it's already been accepted exit here
[ "\$accepted" ] && {
    echo "The Ingres ${inglic} license agreement has already been accepted."
    exit 0
}

while [ -z "\$accepted" ]
do
read -p "Do you accept this license agreement? (y or n)" ans
    case "\$ans" in
        y|Y|\
      yes|YES)
                accepted=\$ans
                ;;
        n|N|\
       NO|no)
                echo "You must answer 'yes' to continue with the Ingres product install"
                exit 1
                ;;
           *)
                echo "Please answer 'yes' or 'no'"
                ;;
    esac
done

EOF

# Verify it matches it's MD5 sum and install it
cat << EOF
tail -RPMLENGTH \$0 > \${INGLIC_RPM_LOC}
inglic_rpm_md5=\`md5sum \$INGLIC_RPM_LOC | cut -d' ' -f1\`
if [ "\$inglic_rpm_md5" = "\$INGLIC_RPM_MD5" ] 
then
# Install it
    # rpm -U \$INGLIC_RPM_LOC
    # fake out the install for now
    true
    if [ \$? -gt 0 ] ; then
	echo Failed to install \$INGLIC_RPM_LOC
	exit 3
    fi
else
    echo Failed md5 checksum, possibly corrupted during download
    echo Aborting...
    exit 4
fi


    exit 0
EOF

# Get the length of the file (+1 for the start of the RPM)
# at this point and replace RPMLENGTH  with the length of the inline RPM
    flen=`wc -l $INGLIC_RPM_LOC|awk '{print $1}'`
    flen=`eval expr $flen + 1 `
    vi -e -s +:%s,RPMLENGTH,$flen,g +:x $INGLIC_EXE

# Make the file executable
    chmod 755 $INGLIC_EXE
}

# Generate the script
do_gen_eula

# Add RPM to the end
cat $INGLIC_RPM_LOC >> $INGLIC_EXE


