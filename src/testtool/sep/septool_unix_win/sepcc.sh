:
#
# Copyright (c) 2004 Ingres Corporation
#
# sepcc -- driver to allow sep to compile .c files.
#
## History:
##
##	05-Jul-91 (DonJ)
##		Added this file to Ingres38p and pasted in this
##		history header.
##	19-Aug-91 (DonJ)
##		fixed problem with compiling files with no file extensions.
##	20-Aug-91 (DonJ)
##		Simplified conditional statements. Made other changes
##		suggested by Roger Taranto.
##	27-Aug-91 (DonJ)
##		More improvements.
##	29-jan-93 (donj)
##		Added Ingres environment check and call to iisysdep. Will
##		make compiler calls more compatible to porting.
##	05-feb-1993 (donj)
##		Give user a chance to override CCMACH with CCFLAGS Env Var.
##      17-mar-1993 (pghosh)
##              For ncr platform nc4_us5. we remove the compiler messages to
##              minimize the canon diffs.
##	08-apr-1993 (donj)
##		Allow the use of the $CC Env Var. to pick which compiler.
##	21-jun-1993 (donj)
##		Put glhdr area into include_dir variable.
##	10-aug-1993 (donj)
##		Plug in (rcs)'s ingres63p change #270130:
##		09-jul-1993 (rcs)
##			rearrange if as case statement, and provide usl_us5 + dr6_us5
##		        cases because -w flag is not recognised by the compiler on these
##			machines.
##	28-mar-94 (vijay)
##		Add no optim case.
##	12-jan-95 (wadag01)
##		Added SCO (sos_us5) to the list of platforms on which the
##		compiler does not recognise '-w' flag.
##	22-may-1996 (popri01)
##		Modified previous change to ignore case distinction ("WARNING").
##	07-jan-1998 (kosma01)
##		Remove warnings, "(E)", from AIX C compiler output.
##	12-mar-1999 (walro03)
##		Remove warnings from Sequent (sqs_ptx) compiler output.
##	12-mar-1999 (walro03)
##		sepcc loops if any flags supplied.
##	12-mar-1999 (walro03)
##		STARTRAK issue 6654705: Remove $ING_SRC directories from
##		include list; if you want them, use the -SRC flag.
##	02-Aug-1999 (hweho01)
##              Remove warnings from AIX 64-bit (ris_u64) compiler output. 
##	03-may-2000 (hayke02)
##		Add PWD and II_CONFIG to include_dirs. This change fixes bug
##		101417.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
##	13-Apr-2005 (shaha03)
##		Added case statement for i64_hpu compilation.
##	28-Oct-2008 (wanfr01)
##              SIR 121146
##		Add $II_SYSTEM/ingres/files to include directories to simplify 
##		api sep tests
##	10-Apr-2009 (frima01)
##		Add flags to compile 32 respectively 64 bit object code.
##		Therefore removed separate i64.hpu and usl.us5 compile commands.
##      20-Apr-2010 (hanal04) Bug 123603
##              Add CCPICFLAG to the compiler flags to avoid errors
##              if the target is subsequently included in a shared library.
##

CMD=sepcc
export CMD

if [ $# -eq 0 ]
then
     cat << !

Usage: sepcc <file1> [<file2> ... <filen>]

        -c[onditional]   Conditional Compilation. Compile only if
                         object file doesn't exist.

        -d[ebug]         Debug code included.

        -I<directory>    Directory to search for include files.

        -SRC             Include ING_SRC directories

        -<ocode type>    Determine type of object code to compile
			 where <type> can be one of the following:
			 native (default), hybrid, 32 or 64.
			 If set ocode type overrides the value of
			 SEPPARAM_SEPCC_OCODE which can also be used
			 to specify the object code type.
			 Be aware that CCFLAGS invalidates ocode type.
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

###############################################################################

ocode=""
cflag=false
nodbg_flag=true
include_dirs="-I$II_SYSTEM/ingres/files"
src_dirs="-I$PWD -I$II_CONFIG -I$ING_SRC/gl/hdr/hdr -I$ING_SRC/cl/hdr/hdr_unix -I$ING_SRC/cl/hdr/hdr_unix_win -I$ING_SRC/cl/clf/hdr_unix_win -I$ING_SRC/front/hdr/hdr -I$ING_SRC/front/frontcl/hdr -I$ING_SRC/common/hdr/hdr"

if [ ! -n "${CC}" ]
then
	CC=cc
fi

for i
do
    case $i in
	-conditional | -conditiona | -condition | -conditio |	\
	-conditi | -condit | -condi | -cond | -con | -co | -c )	\
		cflag=true
		shift
		;;

	-debug | -debu | -deb | -de | -d )			\
		nodbg_flag=false
		shift
		;;

	-SRC)   include_dirs="$include_dirs $src_dirs"
		shift
		;;

        -32 | -32bit) ocode=32
                shift
                ;;

        -64 | -64bit) ocode=64
                shift
                ;;

        -hybrid) ocode=hybrid
                shift
                ;;

        -native) ocode=native
                shift
                ;;

	-I* )	include_dirs="$include_dirs $i"
		shift
		;;

	-* )	echo "bad flag $i"
		exit 1
		;;

	*.* | * ) ;;
    esac
done


# evaluate object code type to compile

cc_sw="$CCMACH $CCPICFLAG"

if [ -n "${CCFLAGS}" ]
then
     cc_sw="$CCFLAGS"
     ocode=""
elif [ ! -n "$ocode" ] && [ -n "$SEPPARAM_SEPCC_OCODE" ]
then
    case $SEPPARAM_SEPCC_OCODE in
	32 | 64 | hybrid | native ) 
		ocode=$SEPPARAM_SEPCC_OCODE
		;;
	* ) echo  Wrong value in SEPPARAM_SEPCC_OCODE. Defaulting to native.
	    ocode=native
	    ;;
    esac
fi

[ -n "$ocode" ] && {
  case $ocode in
    64 | 64bit ) [ -n "$CCMACH64" ] && cc_sw="$CCMACH64"
		;;
    32 | 32bit ) [ -n "$CCMACH32" ] && cc_sw="$CCMACH32"
		;;
	hybrid ) [ -n "$CCMACH32" ] && cc_sw="$CCMACH32" || 
		{
		  [ -n "$CCMACH64" ] && cc_sw="$CCMACH64" 
		} || {
		  echo " The object code type hybrid is not available."
		  echo " Defaulting to native."
		}
		;;
  esac
}

new_cc_sw=""
cc_sw_del=""

for i in $cc_sw
do
	add_to_sw=true

	for j in $new_cc_sw
	do
		if [ $i = $j ]
		then
			add_to_sw=false
		fi
	done

	if $add_to_sw
	then
		new_cc_sw="$new_cc_sw$cc_sw_del$i"
	fi
	cc_sw_del=" "
done
cc_sw="$new_cc_sw"

copy_it=false
cp_str=""
for i
do
    do_it=false
    case $i in
	-* )	;;

	*.c )	cfilnam="$i"
		filnam=`basename $i .c`
		do_it=true
		;;

	* )	if [ ! -f "$i.c" -a -f "$i" ]
		then
		    copy_it=true
		    cp $i $i.c
		    cp_str="$cp_str $i.c"
		fi
		cfilnam="$i.c"
		filnam="$i"
		do_it=true
		;;
    esac
    if $nodbg_flag
    then
     	if egrep "NO_OPTIM.*=.*all$|NO_OPTIM.*=.*${VERS}|NO_OPTIM.*=.*${CC}" $cfilnam > /dev/null
    	then
	   :
    	else
	   cc_sw="$cc_sw $OPTIM"
    	fi
    else 
   	 cc_sw="$cc_sw -g"
    fi

    $do_it && $cflag && [ -f $filnam.o ] && do_it=false
    if $do_it
    then
        case $VERS in
	   nc4_us5 ) $CC $cfilnam -c -w $cc_sw $include_dirs 2>&1 | grep -v Meta
		     ;;
	   dr6_us5 | sos_us5 | sqs_ptx)
		     $CC $cfilnam -c $cc_sw $include_dirs 2>&1 | grep -v warning:
                     ;;  
           rs4_us5 | ris_u64 )
	             $CC $cfilnam -c -w $cc_sw $include_dirs 2>&1 | grep -v '(E)'
		     ;;
           * )       $CC $cfilnam -c -w $cc_sw $include_dirs 2>&1
		     ;;
       esac
    fi
done

$copy_it && rm $cp_str

