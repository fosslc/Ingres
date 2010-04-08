:
#
#  Copyright (c) 2004, 2005 Ingres Corporation
#
#  Name: iijam -- Wrapper for jam executable to enhance functionality
#	        of jam, it will be able to create .i, .s, .asm, and 
#		.cod files.
#
#  Usage:
#       iijam 'grist'file.suffix   
#
#  Description:
#       This wrapper will scan the parameters, specifically the last
#	parameter and try to figure out if it has .i, .s, .asm, 
#	or .cod suffix, if it does then this script will set
#	some variables to trigger appropriate actions in iijam and
#	pass those variables to jam executable, however if the
#	last parameter doesn't contain above mentioned suffix
#	this script will pass all parameters it recieved onto
#	jam executable without changing anything.
#	
#
#  Exit Status:
#       0       Jamfile successfully created
#       1       Creation of Jamfile failed
#
#
## History: 
##	22-Apr-2005 (drivi01)
##	    Created.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.



#
#define some variables
#
if [ "$config_string" == "int_lnx" ] || [ "$config_string" == "i64_lnx" ] || [ "$config_string" == "int_rpl" ] 

then
        SUF=.o
        IFLAG=-E
        SFLAG=-S
elif [ "$config_string" == "int_w32" ] || [ "$config_string" == "i64_win" ] || [ "$config_string" == "a64_win" ]
then
        SUF=.obj
        IFLAG=-P
        SFLAG1=-FAs
        SFLAG2=-FAcs

fi

#
#figure out what full path to jam executable is (on unix), on windows just set it to jam.exe
#b/c on windows iijam.bat and iijam.exe can be distinguished by calling them with suffix.
#
if [ "$config_string" == "int_w32" ] || [ "$config_string" == "i64_win" ] || [ "$config_string" == "a64_win" ]
then
	jampath=jam.exe
else
	#replaces : in a path with space
	sedpath=`echo $PATH | sed -e 's/:/ /g'`

	#for each path in the $PATH variable, check if it's a path to jam executable
	#verify that jam executable is not jam shell which would be in $ING_SRC/bin
	for d in $sedpath
	do

	        if [ -x $d/jam ] && [ "$d" != "$ING_SRC/bin" ] && [ "$d" != "$ING_ROOT/tools/bin" ]
	        then
        	        jampath=$d/jam
                	break
	        fi


	done
fi

#
#if jam executable wasn't found, quit.
#
if [ $jampath == "" ]
then
        echo Could not find jam executable. Add it to the path and try again.
	exit 1 
	
fi

#
# find last argument, figure out if it's .i, .asm, .cod, or .s file
#
while [ $# -gt 0 ]
do
    case $1 in
        *.i) lastarg=`echo $1| sed  -e "s/\.i/$SUF/g"`
           customflag=$IFLAG
           NEWSUF=.i
           shift
            ;;
        *.asm) lastarg=`echo $1| sed  -e "s/\.asm/$SUF/g"`
           customflag=$SFLAG1
           NEWSUF=.asm
           shift
            ;;
        *.cod) lastarg=`echo $1| sed  -e "s/\.cod/$SUF/g"`
           customflag=$SFLAG2
           NEWSUF=.cod
           shift
            ;;
        *.s) lastarg=`echo $1| sed  -e "s/\.s/$SUF/g"`
           customflag=$SFLAG
           NEWSUF=.s
           shift
            ;;
        *) [ "$restarg" ] && restarg="$restarg $1"
	   [ -z "$restarg" ] && restarg=$1
           shift
            ;;
    esac
done

#
# if lastarg variable is empty, we should just run a regular jam
# special case isn't required then.
# if lastarg contains a file name then user is trying to generate
# .ism, .i, .cod, or .s file.
#
if [ -z "$lastarg" ]
then
	$jampath $restarg
else
        if [ -z "SUF" ] || [ -z "IFLAG" ]
        then
                echo To generate .i, .s, .asm, or .cod files on your platform
                echo you must specify:
                echo SUF - suffix of object files specific to your OS.
                echo IFLAG - a flag that will be passed to compiler for
                echo         generating .i files.
                echo SFLAG - a flag that will be passed to compiler for
                echo         generating .s files.
                echo SFLAG1 - a flag that will be passed to compiler for
                echo         generating .asm files.
                echo SFLAG2 - a flag that will be passed to compiler for
                echo         generating .cod files.
                echo PLEASE edit this script and specify variables specific
                echo to your platform.
                echo to your platform.
        else
	lastarg1=`echo $lastarg|sed "s/>/> /g"|awk '{print $1}'`
	lastarg2=`echo $lastarg|sed "s/>/> /g"|awk '{print $2}'`
        $jampath -sCUSTOMFLAG=\"$customflag\" -sSUFFIX=$NEWSUF $restarg "$lastarg1"$lastarg2
        fi     
fi

exit 0
