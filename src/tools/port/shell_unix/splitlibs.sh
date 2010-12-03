:
#
# Copyright (c) 2004 Ingres Corporation
#
# merge INGRES libraries into libingres1.a and libingres2.a
#
# DEST = utility
#
## History:
##	26-mar-1990 (seng)
##		First created.  Based on mergelibs.  Called only from
##		mergelibs.  Can easily be modified to split libraries
##		into smaller bits, but not worth the effort for now.
##	07-Jun-90 (GordonW)
##		Provide the ability to build only 1 library, as specified
##		by the passed number.
##		Also added SHELL_DEBUG.
##	07-Jun-90 (GordonW)
##		On some systems we can't create the library than remove the
##		unwanted objects later. This causes unreferenced symbols
##		because the new library is not in order anymore. So do
##		a tedious check on unwanted objects where we are building
##		this object list.
##	08-Jun-90 (GordonW)
##		I deleted a "cd $ING_BUILD/lib" by mistake on the last
##		change...
##	14-Nov-1990 (jonb)
##		Adapted from 6.2 version.
##	30-jan-1991 (jonb)
##		Define $vers before attempting to use it.
##	15-Apr-1991 (johnst)
##		Added case stmt to explicity set hasranlib=false for
##		386_us5. This brings splitlibs.sh into sync with mkdefault
##		regarding ranlib use, and is necessary because ranlib doesn't
##		work on the box. Also fixed the following line so that 
##		$do_lorder is set to true (not false!) when hasranlib=false.
##	26-jun-1991 (jonb)
##		Integrated from ingres6302p line.  Also, changed to use
##		the HASRANLIB from default.h via iisysdep, rather than
##		a locally generated one.
##	30-dec-1991 (rudyw)
##		Make change to splitlibs corresponding to change in mergelibs
##		in which NO_CRYPT recompilation done on file gcapwd.c.
##		Add case section at end to allow platform specific actions
##		after libingres[12].a have been built. SCO must remove files
##		which attempted to support mixed case fortran but had problems
##		due to unresolved name conflicts with standard Ingres routines.
##		Name conflict only occurs for boxes with MIXED_CASE defined.
##	16-oct-1992 (lauraw)
##		Uses readvers to get config string.
##	23-jul-1993 (dianeh)
##		Removed -n300 flag from call to xargs -- latest version of xargs
##		in tools/port/others doesn't have any flags; also, change History
##		block to double pound-signs.
##	09-may-1995 (morayf)
##		Integrated OpING changes to mergelibs which I assume apply to
##		this file. In particular, the dlist and the unwanted objects
##		have changed substantially.
##	31-Mar-98 (gordy)
##		Removed gcnutil.o from wanted list.
##	10-may-1999 (walro03)
##		Remove obsolete version string odt_us5, ps2_us5, sqs_us5.
##	25-apr-2000 (somsa01)
##		Added capability for building other products.
##	11-Jun-2004 (somsa01)
##		Cleaned up code for Open Source.
##		Added capability for building EDBC.
##	15-Jul-2004 (hanje04)
##		Define DEST=utility so file is correctly pre-processed
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
##	22-Nov-2010 (kschendel) SIR 124685
##	    Drop a few more ancient / obsolete ports
##		
#

. (PROG1PRFX)sysdep

[ $SHELL_DEBUG ] && set -x

# see if they want stripped, or single library

strip=false
one_only=false
lno=1

for a in $*
do
	case $a in
	-s) strip=true;;
	1|2)  lno=$a; one_only=true;;
	*) echo "unknown option, $a" exit 1;;
	esac
done

. readvers
vers=$config

# make sure we are at the right place...
cd $ING_BUILD/lib

if [ -f libingres.a ]
then
	echo 'removing libingres.a'
	rm -f $ING_BUILD/lib/libingres.a
fi

# Tell them what we are about to do...
if $one_only
then
	if [ -f libingres$lno.a ]
	then
		echo "replacing libingres$lno.a"
		rm -f $ING_BUILD/lib/libingres$lno.a
	else
		echo "creating libingres$lno.a"
	fi
else
	if [ -f libingres1.a ]
	then
		echo 'replacing libingres[12].a'
		rm -f $ING_BUILD/lib/libingres[12].a
	else
		echo 'creating libingres[12].a'
	fi
fi

TMP=$ING_BUILD/lib/tmp
(mkdir $TMP) 2>/dev/null
objdir=$TMP/mgd$$
allobjs=$TMP/mga$$
objs=$TMP/mgb$$

lnames1="interp abfrt iaom ilrf ioi generate uf runsys runtime fd ft mt qxa oo"
lnames2="feds ui qsys q ug fmt afe qgca sqlca gcf adf compat cuf"

# directory list for object removals (unwanted)
dlist="common/gcf$noise/gcn common/gcf$noise/gcc back/dmf$noise/lg back/dmf$noise/lgk back/dmf$noise/lk cl/clf$noise/cs_unix cl/clf$noise/di_unix cl/clf$noise/ck_unix_win cl/clf$noise/jf_unix_win cl/clf$noise/sr_unix_win"

# unwanted object list within wanted directories (libcompat.a)
unwanted="mucs.o gcccl.o gcapsub.o gcctcp.o gccdecnet.o gccptbl.o bsdnet.o bsnetw.o cspipe.o cvnet.o iamiltab.o clnt_tcp.o clnt_udp.o pmap_rmt.o svc.o svc_run.o svc_tcp.o meuse.o meconsist.o medump.o mebdump.o meberror.o mexdump.o fegeterr.o pcsspawn.o pcfspawn.o "

# wanted object list within unwanted directories (libcompat.a)
wanted=""

# build "unwanted" list from directories
echo "building unwanted object list ..."
here=`pwd`
echo "" >$allobjs
echo "" >$objs
for i in $dlist
do
	# legal directory?
	if [ ! -d $ING_SRC/$i ]
	then
		echo "Can't find $ING_SRC/$i...aborting..."
		exit 1
	fi
	cd $ING_SRC/$i

	# yes.. get directory source file list
	ls *.c *.s *.roc 2>/dev/null | \
		sed -e 's/\.c/\.o/g' -e 's/\.s/\.o/g' -e 's/\.roc/\.o/g' \
			>>$allobjs 2>/dev/null

	# get source directory (main.c) file list
	ls *.o >>$objs 2>/dev/null
done
unwanted="$unwanted `cat $allobjs | tr '\012' ' '`"
wanted="$wanted `cat $objs | tr '\012' ' '`"
cd $here

# now remove any "wanted" objects from unwanted list
echo "building wanted object list ..."
ufiles=""
echo "$wanted" >$allobjs
for i in $unwanted
do
	if grep -s $i $allobjs >/dev/null 2>/dev/null
	then
		continue
	else
		ufiles="$ufiles $i"
	fi
done
unwanted="$ufiles"


# now do them
while [ $lno != 3 ]
do
	case $lno in
	1) lnames=$lnames1;;
	2) lnames=$lnames2;;
	*) echo "Bad library number (libingres$lno.a). Aborting..." >&2 exit1;;
	esac

	echo "processing libingres$lno.a ..."

	cd $ING_BUILD/lib
	missing=false
	libs=""
	for i in $lnames
	do
		if [ -f lib$i.a ]
		then
			libs="$libs lib$i.a"
		else
			if [ -f $i.a ]
			then
				libs="$libs $i.a"
			else
				echo "lib$i.a not found" >&2
				missing=true
			fi
		fi
	done

	if $missing
	then
		echo "aborting" >&2
		exit 2
	fi

	trap "rm -rf $objs $allobjs $objdir" 0 1 2 3 15

	echo "fetching library object list ..."
	cp /dev/null $allobjs
	for lib in $libs
	do
		ar t $lib | grep -v $SYMDEFNAME >> $allobjs
	done

	echo "checking for duplicates ..."
	sort $allobjs | uniq -d > $objs
	if [ -s $objs ]
	then
		echo "repeated objects - aborting" >&2
		allobjs=`cat $objs`
		for obj in $allobjs
		do
			for lib in $libs
			do
				if ar t $lib $obj 1>/dev/null 2>&1
				then
					echo "$lib: $obj" >&2
				fi
			done
		done
		exit 3
	fi

	echo "OK for merge"

	[ $HASRANLIB ] && do_lorder=false || do_lorder=true

	# now do slow or fast object list generation...
	if $do_lorder
	then
		echo "lorder ..."
		lorder $libs | grep -v '\.a' > $allobjs 2>/dev/null

		echo "tsort ..."
		# clean up output to screen of tsort errors for mx3_us5
	        tsort $allobjs 2> /dev/null | grep -v tsort > $objs
	else
		sort $allobjs >$objs 2>/dev/null
	fi

	# now remove the unwanted objects
	echo "removing unwanted objects..."
	objlist=`cat $objs`
	cp /dev/null $objs
	for x in $objlist
	do
		found=false
		for y in $unwanted
		do
			if [ "$x" = "$y" ]
			then
				found=true
				break
			fi
		done
		$found || echo $x >> $objs
	done

	echo "extracting objects from libraries ..."
	rm -rf $objdir
	mkdir $objdir
	cd $objdir
	for lib in $libs
	do
		echo "	$lib"
		if [ -f $ING_BUILD/lib/$lib ]
		then
			ar xl $ING_BUILD/lib/$lib
		else
			ar xl $lib
		fi
		rm -f $SYMDEFNAME
	done

	# do stripping
	if $strip
	then
		echo "stripping symbols ..."
		olist=`cat $objs`
		for i in $olist
		do
			strip $i
		done
	fi
	echo "building libingres$lno.a ..."
	if $do_lorder
	then
		xargs ar cql $ING_BUILD/lib/libingres$lno.a < $objs
	else
		ar cql $ING_BUILD/lib/libingres$lno.a `cat $objs`
	fi

	if [ $lno = 2 ]
	then
		echo "removing crypt from gcapwd.c"
		cd $ING_SRC/cl/clf$noise/gc_unix
		if [ `grep NO_CRYPT gcapwd.c | wc -l` -ge 1 ]
		then
        		ming OPTIM="-DNO_CRYPT" gcapwd.o
        		if [ ! -f gcapwd.o ]
        		then
                		echo "gcapwd.o didn't compile!"
        		else
                		ar r $ING_BUILD/lib/libingres$lno.a gcapwd.o
                		rm -f gcapwd.o
        		fi
		else
        		echo "gcapwd.c didn't have NO_CRYPT symbol!"
		fi
	fi

	echo "concurrent cleanup on libingres$lno.a ..."
	cd $ING_BUILD/lib
	rm -rf $objs $allobjs $objdir &

	if [ $HASRANLIB ] 
	then
	    echo "polishing libingres$lno.a ..."
	    ranlib libingres$lno.a
	fi
	chmod 644 libingres$lno.a

	wait
	$one_only && break
	lno=`expr $lno + 1`
	echo ""
done

case $vers in
    	sco_us5)
	     echo "Extracting mixed case routines from libingres1.a ..."
	     ar d $ING_BUILD/lib/libingres1.a iiuftbM.o iiufrunM.o
	     echo "Extracting mixed case routines from libingres2.a ..."
	     ar d $ING_BUILD/lib/libingres2.a iiuflbqM.o iiufsqlM.o iiufutlM.o;;
    	*) ;;
esac

echo "done"
exit 0
