:
# Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name: mkrc
#
#  Usage:
#       mkrc [-i [123456]] [-r]
#
#  Description:
#       Generates an rc style script under $II_SYSTEM/ingres/files/rcfiles 
#	called:
#
#		ingresXX 	where XX is the installation ID.
#  	
#	-i	Script is installed under $ETCRCFILES. 
#		If 1 or more run level is specified, links to the 
#		corresponding rc directories are created. 
#		If no run level is specified links will be created under
#		the levels defined by $RCDEFLNK.
#		Both ETCRCFILES and RCDEFLNK are defined in iisysdep
#
#	-r 	Remove all RC files and links from $ETCRCFILES for the current
#		installation
#       
#
#  Exit Status:
#       0       ok
#       1       failed to generate file
#	2	failed to install file
#	3	failed to create link
#	4	failed to delete rc files
#
#  PROGRAM = (PROG0PRFX)mkrc
#
#  DEST = utility
## History:
##	09-Mar-2004 (hanje04)
##	    SIR 111952
##	    Created.
##	07-Feb-2005 (hanje04)
##	    Use chkconfig to install into RC system if it's there.
##	06-Apr-2005 (hanje04)
##	    Make sure ETCRCFILES is set when we echo useage message
##	20-May-2006 (hanje04)
##	    BUG 116174
##	    Split up SuSE and RedHat config as then can conflict
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##	05-Jun-2007 (hanje04)
##	    BUG 119214
##	    Add support for Debian/Ubuntu systems.
##	04-Jan-2008 (hanje04)
##	    BUG 119699
##	    Need to use -f when checking for /etc/debconf.conf.
##	22-Feb-2008 (bonro01)
##	    Add support for Fedora systems.
##	    Fix parameter parsing bug.
##	    Use documented default RC levels 235.
##	    Create correct RC scripts for RedHat, Fedora,
##	    Debian and Ubuntu instead of creating system defaults.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##

(LSBENV)

. iisysdep
. iishlib

GENRC=true
INSTALL=false
RMRC=false
RCLNK=
USER=`whoami`
CHKCFG=/sbin/chkconfig
CHKINS=/sbin/insserv
UPDRCD=/usr/sbin/update-rc.d
self=`basename $0`
usage()
{
cat << !

Usage:
    $self [-i [123456]]|[-r]
  	
	Generates an system "init" script under: 

		\$II_SYSTEM/ingres/files/rcfiles 

	called:

		ingresXX    - 	where XX is the installation ID.

	-i	Install RC script under $ETCRCFILES. 
		If 1 or more run level is specified, links to the 
		corresponding rc directories are created. 
		If no run level is specified links will be created under
		the levels defined by $RCDEFLNK.

	-r 	Remove all RC files and links from $ETCRCFILES

!
}

# Generate the RC file from the template
genrcfile()
{
    # check we have template and output file
    [ $# -lt 2 ] || [ ! -r "$1" ] || [ -z "$2" ] && return 1

    tmplt="$1"
    outfile="$2"
    shell="/bin/bash"
    echo "#! ${shell}" > $outfile
    
    # need an extra step for Linux
    case $VERS in
	*_lnx|int_rpl)
	    if [ -f /etc/rc.status ] || [ -f /etc/debconf.conf ] ; then
		# SuSE/Debian style RC structure
		cat $tmplt | $SEDCMD | $AWK 'BEGIN{
		    prnt=1 ; skip=0 
		    }
		    $1 == "--REDHAT_CFG" { prnt=0 ; skip=1 }
		    $1 == "--REDHAT_CFG_END" { prnt=1 ; skip=1 }
		    $1 == "--SUSE_CFG" { prnt=1 ; skip=1 }
		    $1 == "--SUSE_CFG_END" { prnt=1 ; skip=1 }
		    prnt == 1 && skip == 0 { print }
		    { skip=0 }' \
			>> $outfile
	    else # RedHat style
		cat $tmplt | $SEDCMD | $AWK 'BEGIN{
		    prnt=1 ; skip=0 
		    }
		    $1 == "--REDHAT_CFG" { prnt=1 ; skip=1 }
		    $1 == "--REDHAT_CFG_END" { prnt=1 ; skip=1 }
		    $1 == "--SUSE_CFG" { prnt=0 ; skip=1 }
		    $1 == "--SUSE_CFG_END" { prnt=1 ; skip=1 }
		    prnt == 1 && skip == 0 { print }
		    { skip=0 }' \
			>> $outfile
	    fi
		;;
	*)	
		cat $tmplt | $SEDCMD >> $outfile
		;;
    esac
    rc=$?
    return $rc	
}
while [ $# != 0 ]
do
    case "$1" in 

    -r) # Remove RC files
	INSTALL=false
	GENRC=false
	RMRC=true
	break
	;;
    -i) # Install previously generated file
	INSTALL=true
	GENRC=false
  	shift
	;;
     *)
	if [ "$INSTALL" = "true" ]; then
	    INVALID=`echo $1 | tr -d "123456"`
	    if [ -z "$INVALID" ]; then
		# Run levels under which to install rc scripts
		NRCLNK=`echo $1 | sed "s/1/1 /;s/2/2 /;s/3/3 /;s/4/4 /;s/5/5 /;s/6/6 /"`
		RCLNK="$NRCLNK $RCLNK"
	    else
		usage
		exit 1
	    fi
	else
	    usage
	    exit 1
	fi
	shift
	;;
    esac
	
done

# Check II_SYSTEM is set
[ -z "$II_SYSTEM" ] && {
echo '$II_SYSTEM must be set to run this script'
exit 1
}


if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    INST_ID=""
    II_SYSTEM=/usr/libexec
    RCTMPLT=$II_CONFIG/rcfiles/$RCTMPLT
    RCFILE=$II_ADMIN/rcfiles/ingres${INST_ID}
else
    INST_ID=`ingprenv II_INSTALLATION`
    RCTMPLT=$II_CONFIG/rcfiles/$RCTMPLT
    RCFILE=$II_CONFIG/rcfiles/ingres${INST_ID}
fi
SEDCMD="sed -e s,INST_ID,${INST_ID},g -e s,INST_LOC,$II_SYSTEM,g"

if $RMRC # Remove all RC files
then
    if [ -x ${CHKCFG} ] # Use chkconfig if we can
    then
	${CHKCFG} --del ingres${INST_ID} > /dev/null 2>&1
    elif [ -x ${UPDRCD} ] # Use update-rc.d
    then
	${UPDRCD} -f ingres${INST_ID} remove > /dev/null 2>&1
    else
	for dir in $RC1 $RC2 $RC3 $RC4 $RC5 
	do
	    # Remove startup links
	    [ -L $dir/${RCSTART}ingres${INST_ID} ] && {
	       rm -f $dir/${RCSTART}ingres${INST_ID}	
	       [ $? != 0 ] && {
	        cat << !

Could not remove:

    $dir/${RCSTART}ingres${INST_ID}
!
	        }

	    }
	    # Remove shutdown links
	    [ -L $dir/${RCSTOP}ingres${INST_ID} ] && {
	        rm $dir/${RCSTOP}ingres${INST_ID}
	        [ $? != 0 ] && {
	            cat << !

Could not remove:

    $dir/${RCSTOP}ingres${INST_ID}
!
	        }

	    }
        done
    fi

    # Remove RC file
    [ -f $ETCRCFILES/ingres${INST_ID} ]  && {
	rm -f $ETCRCFILES/ingres${INST_ID}  
        [ $? != 0 ] && {
	    cat << !

Could not remove:

    $ETCRCFILES/ingres${INST_ID}
!
        }
	exit 4

    }
    exit 0

elif $GENRC
then
# Check template file exists
    [ ! -f $RCTMPLT ] && {
cat << !

Cannot locate RC template file:
	$RCTMPLT
Aborting...
!
	usage
	exit 1
    }

# Generate file

    genrcfile $RCTMPLT $RCFILE
    [ $? != 0 ] && {
	echo "Failed to create $RCFILE"
	exit 1
    }

    chmod 755 $RCFILE
    exit 0

elif $INSTALL
then
	
    #Check RC file exists
       [ ! -f $RCFILE ] && {
    cat << !

Cannot locate RC file:
 $RCFILE
It can be created by running:
 $0
!
            exit 2
    }
    
    #Check Install location is writeable
    [ ! -w $ETCRCFILES ] && {
    cat << !

$ETCRCFILES is not writeable by user "$USER"
Re-run as root.
!
    exit 2
    }
    
    # Install RC file
    cp $RCFILE $ETCRCFILES
    if [ $? = 0 ] ; then
    [ ! -x $ETCRCFILES/ingres${INST_ID} ] && {
	chmod 755 $ETCRCFILES/ingres${INST_ID}
}
    else
	cat << !

Failed to copy

    $RCFILE
    
to

    $ETCRCFILES
!
	exit 2
    fi
    
    if [ -x ${CHKCFG} ] # Use chkconfig if we can
    then
	# SuSE, RedHat and Fedora style RC
	${CHKCFG} --add ingres${INST_ID} > /dev/null 2>&1
        [ -z "$RCLNK" ] && RCLNK="$RCDEFLNK"
	[ "$RCLNK" ] &&
	{
	    #strip spaces
	    RCLNK=`echo $RCLNK | sed -e 's, ,,g'`
	    if [ -x ${CHKINS} ]; then
		# SuSE style RC
		${CHKCFG} ingres${INST_ID} $RCLNK
	    else
		# Redhat and Fedora style RC
		${CHKCFG} ingres${INST_ID} off
		${CHKCFG} --level ${RCLNK} ingres${INST_ID} on
	    fi
	}
    elif [ -x ${UPDRCD} ] # Use update-rc.d
    then
	# ubuntu and debian style RC
        [ -z "$RCLNK" ] && RCLNK="$RCDEFLNK"
	RCLNK2=`echo $RCLNK | tr -d " "`
	RCLNK_STOP=`echo "0 1 2 3 4 5 6" | tr -d "$RCLNK2"`
	RCSTART_NUM=`echo $RCSTART | tr -d "S"`
	RCSTOP_NUM=`echo $RCSTOP | tr -d "K"`
	${UPDRCD} -f ingres${INST_ID} remove > /dev/null 2>&1
	${UPDRCD} ingres${INST_ID} start $RCSTART_NUM $RCLNK . stop $RCSTOP_NUM $RCLNK_STOP . > /dev/null 2>&1
    elif [ -f /etc/rc.status ] # SuSE style RC structure
    then
        #If no run levels are specified create the default links
        [ -z "$RCLNK" ] && RCLNK="$RCDEFLNK"
        for run_level in $RCLNK
        do
            LNK_OK=true
            case "$run_level" in
            1) #If RC1 defined create links
	        if [ "$RC1" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		    ${RC1}/${RCSTART}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		    ${RC1}/${RCSTOP}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;
    
            2) #If RC2 defined create links
	        if [ "$RC2" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		        ${RC2}/${RCSTART}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		        ${RC2}/${RCSTOP}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
    	        fi
	        ;;	
    
            3) #If RC3 defined create links
	        if [ "$RC3" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		        ${RC3}/${RCSTART}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		        ${RC3}/${RCSTOP}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		    cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	
    
            4) #If RC4 defined create links
	        if [ "$RC4" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		        ${RC4}/${RCSTART}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		        ${RC4}/${RCSTOP}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	
    
            5) #If RC5 defined create links
	        if [ "$RC5" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		        ${RC5}/${RCSTART}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		    ${RC5}/${RCSTOP}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	
    
            *) # Any other value
 	        cat << !

$run_level is not a valid RC run level
Please check and try again
!
	        exit 3
	        ;;
    
            esac
        done
    else # Redhat/Solaris style RC structure
	#If no run levels are specified create the default links
        [ -z "$RCLNK" ] && RCLNK="$RCDEFLNK"
	
	#Create all shutdown links appart from the levels want to
	#start Ingres from

	RC1PRFX=$RCSTOP
	RC2PRFX=$RCSTOP
	RC3PRFX=$RCSTOP
	RC4PRFX=$RCSTOP
	RC5PRFX=$RCSTOP

	for run_level in $RCLNK
	do 
	    eval RC${run_level}PRFX=$RCSTART
	done
	
	for run_level in 1 2 3 4 5
	do
	    LNK_OK=true
	    case $run_level in
	    1) #If RC1 defined create links
	 	if [ "$RC1" ] ; then
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		    ${RC1}/${RC1PRFX}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    2) #If RC2 defined create links
	 	if [ "$RC2" ] ; then
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		    ${RC2}/${RC2PRFX}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    3) #If RC3 defined create links
	 	if [ "$RC3" ] ; then
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		    ${RC3}/${RC3PRFX}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    4) #If RC4 defined create links
	 	if [ "$RC4" ] ; then
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		    ${RC4}/${RC4PRFX}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    5) #If RC5 defined create links
	 	if [ "$RC5" ] ; then
	            ln -s ${ETCRCFILES}/ingres${INST_ID} \
		    ${RC5}/${RC5PRFX}ingres${INST_ID}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    *) # Any other value
 	        cat << !

$run_level is not a valid RC run level
Please check and try again
!
	        exit 3
	        ;;
    
            esac
        done

    fi # [ -f /etc/rc.status ]

fi

exit 0
