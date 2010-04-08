:
#
# Copyright (c) 2004 Ingres Corporation
#
# ferun : brainlessly pass things through to the os from sep
#
## mar 26/91 (sgp) - added loop for multiple args (used to be
##                   this file was just "$1").  added comments.
##	14-Aug-92 (GordonW)
##		Use a better way.
##	29-jan-1993 (donj)
##	    Fix problem running an implied "run ./file.exe" when expressed
##	    as "run file.exe". The test, [ ! -f $1 ], found the file, but the
##	    "exec $cmdline" couldn't. Also practice adding Ingres environment
##	    checking and including the call to iisysdep.
##	 2-jun-1993 (donj)
##	    Changed echolog to iiecholg.
##      24-aug-1993 (donj)
##          ferun wasn't finding images that were in PATH. The if [ ! -f $1]
##          test was failing on basic unix functions; i.e. chmod.
##	10-may-1994 (donj)
##	    Added a "debug" option to help in debugging.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##      17-Feb-2009 (horda03) Bug 121667
##          Use 'type' to determine location of the tool to be executed.
#

CMD=ferun
export CMD

if [ $# -eq 0 ]
then
     cat << !

Usage: $CMD <executable_filespec> [args ...]

!
    exit 0
fi

###############################################################################
#
# Check local Ingres environment
#
###############################################################################

if [ ! -n "${II_SYSTEM}" ]
then
     cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|     The local Ingres environment must be setup and the     |
|     the installation running before continuing             |
|------------------------------------------------------------|

!
   exit 1
fi

if [ ! -n "${DEBUG}" ]
then
     debugit=false
else
     debugit=true
fi

. $II_SYSTEM/ingres/utility/iisysdep

need_appended=true
tmp=`echo $PATH | sed -e 's/:/ /g'`
for i in $tmp
do
        if [ $i = "${II_SYSTEM}/ingres/utility" ]
           then
                need_appended=false
        fi
done

if $need_appended
   then
        PATH=$II_SYSTEM/ingres/utility:$PATH
        export PATH
fi

cmdline=$*
prog_name=$1
dir_name=`dirname $prog_name`

if $debugit
then
	iiecholg << !

|------------------------------------------------------------|
|	cmdline   = $cmdline	
|	prog_name = $prog_name
|	dir_name  = $dir_name
|------------------------------------------------------------|

!
fi
###############################################################################

if [ ! -f $prog_name ]
then

     which_prog=`type $prog_name 2>&1`

     if [ $? -eq 0 ]
     then
          shift
          prog_name=`echo $which_prog | cut -f3- -d" "`
          cmdline="$prog_name $*"
          dir_name=`dirname $prog_name`
     else
          iiecholg << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|                    Can't find $prog_name
|------------------------------------------------------------|

!
	  exit 1
     fi
fi

if [ $dir_name = "." ]
then
	case $prog_name in
	    \.* | \$* | /* )
		;;
	    * )
		cmdline="./$cmdline"
		;;
	esac

fi

exec $cmdline
