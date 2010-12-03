:
#
# Copyright (c) 2004 Ingres Corporation
#
# yypp.sh - Prepend $ING_SRC/tools/port$noise/conf/{VERS,CONFIG} to $1 and run
#           through yapp.
#
## History:
##    10-aug-93 (dianeh)
##        Created (from ccpp.sh).
##    09-nov-93 (dianeh)
##        yapp now understands C-style comments; allow for standard-in.
##    09-nov-93 (dianeh)
##        Arghh -- fix the old mouse-doesn't-pick-up-tabs trick...
##    29-nov-93 (dianeh)
##        Forget to allow for yapp's -H flag.
##    01-dec-93 (dianeh)
##        Make sure yapp is available; if not, exit 1.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##    27-Oct-2004 (drivi01)
##     	  shell directories on unix were replaced with shell_unix,
##        this script was updated to reflect that change.
##    11-Jun-2007 (bonro01)
##        Fix for Linux reverse Hybrid builds.
##	19-Jun-2009 (kschendel) SIR 122138
##	    readvers figures out config for hybrids now.
##	    Compute and pass conf_BUILD_ARCH_xx_yy symbol.
##	23-Nov-2010 (kschendel)
##	    -Z $READVERS apparently can't happen, because it was broken.
##	    Fix it anyway, more or less.
#

CMD="`basename $0`"

usage='[ -c <config> ] [ <files>... ] [ -s <sym> ] [ -D<def> ] [ -U<undef> ]'

[ -n "$ING_VERS" ] && { noise="/$ING_VERS/src" ; vers="/$ING_VERS" ; }

[ -r $ING_SRC/tools/port$noise/conf/CONFIG ] || exit 1

#Make sure we have yapp available to us
yapp </dev/null || exit 1

while [ $# -gt 0 ]
do
  case "$1" in
    -c*) if [ -z "$conf" -a `expr $1 : '.*'` -gt 2 ] ; then
           conf="`expr $1 : '-c\(.*\)'`" ; shift
         else
           [ -z "$conf" -a $# -gt 1 ] ||
           { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
           conf="$2" ; shift ; shift
         fi
         ;;
    -s*) if [ -z "$sym" -a `expr $1 : '.*'` -gt 2 ] ; then
           sym="`expr $1 : '-s\(.*\)'`" ; shift
         else
           [ -z "$sym" -o $# -gt 1 ] ||
           { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
           sym="$2" ; shift ; shift
         fi
         ;;
    -D*) [ `expr $1 : '.*'` -eq 2 ] &&
         { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
         defs="$defs $1" ; shift
         ;;
    -H*) [ `expr $1 : '.*'` -eq 2 ] &&
         { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
         hist="$hist $1" ; shift
         ;;
    -U*) [ `expr $1 : '.*'` -eq 2 ] &&
         { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
         undefs="$undefs $1" ; shift
         ;;
     -*) echo "Usage:" ; echo "$CMD $usage"
         exit 1
         ;;
      *) files="$files $1" ; shift
         ;;
  esac
done


if [ -x $ING_SRC/bin/readvers ] ; then
   READVERS=$ING_SRC/bin/readvers
elif [ -r $ING_SRC/tools/port$noise/shell_unix/readvers.sh ] ; then
   READVERS=$ING_SRC/tools/port$noise/shell_unix/readvers.sh
fi

. $READVERS

if [ -z "$conf" -a -z "$config" ] ; then
   echo "ERROR: Config string not given and could not execute readvers."
   echo "Usage:" ; echo " $CMD $usage" ; exit 1
# Since readvers wasn't used, check the config string 
elif [ -z "$READVERS" ] ; then
  case $conf in
    [a-z0-9][a-z0-9][a-z0-9]_us5) ;;
    [a-z0-9][a-z0-9][a-z0-9]_osf) ;;
    [a-z0-9][a-z0-9][a-z0-9]_osx) ;;
    [a-z0-9][a-z0-9][a-z0-9]_lnx) ;;
    [a-z0-9][a-z0-9][a-z0-9]_sol) ;;
                           *_kit) ;;
                               *) echo "$CMD: Bad config string: $conf"
                                  echo "Usage:" ; echo " $CMD $usage"
                                  ;;
  esac
# If $conf was passed, but readvers reported a different string in
# VERS, don't use anything from readvers; it doesn't apply to $conf.
# Print a warning.
elif [ -n "$conf" -a "$conf" != "$config" ] ; then
  optdef="" ; opts=""
  echo "Warning: $config defined in VERS but $conf will be used." >&2
# Just use config and everything we got from readvers.
else
  conf=$config
fi

# Build up arguments to yapp
dconf="-D$conf"
dvers="-DVERS=$conf"
dopts="$optdef"

if [ -n "$build_arch" ] ; then
    # Hybrid capable platform, define VERS32 and VERS64 as well as VERS,
    # define conf_BUILD_ARCH_xx_yy.
    dvers="-DVERS32=$config32 -DVERS64=$config64 $dvers"
    x=`echo $build_arch | tr '+' '_'`
    dvers="-Dconf_BUILD_ARCH_$x $dvers"
fi

if [ "$sym" ] ; then
  res=`echo $sym | cat $ING_SRC/tools/port$noise/conf/CONFIG - |
       yapp $dvers $dconf $dopts $defs $hist $undefs | sed -e '/^[ 	]*$/d'`
  if [ "$res" = "$sym" ] ; then
    echo "$sym undefined in CONFIG or VERS for $conf" >&2; exit 1
  else
    echo $res
  fi
else
  cat $ING_SRC/tools/port$noise/conf/CONFIG ${files-"-"} |
      yapp $dvers $dconf $dopts $defs $hist $undefs | sed -e '/^[ 	]*$/d'
fi

exit 0
