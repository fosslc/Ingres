:
#
# Copyright (c) 2004 Ingres Corporation
#
## History
##	20-Oct-94 (GordonW)
##		created.
##	21-Oct-94 (GordonW)
##		add $ING_SRC/admin and $ING_SRC/sig
##	21-Oct-94 (GordonW)
##		convert to 6.5 style
##	21-Oct-94 (GordonW)
##		missing 'cd ../'
##	24-Oct-94 (GordonW)
##		There is a sig directory for 6.5
##	07-Dec-94 (GordonW)
##		Added ING_VERS for development.
##		Added Shared library support
##	09-Dec-94 (GordonW)
##		It should be $( not ($.
##	15-Dec-94 (GordonW)
##		Add makeing fe.cat.msg.
##	16-Dec-94 (GordonW)
##		add ING_VERS on fe.cat.msg section.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.

[ $SHELL_DEBUG ] && set -x

[ "$ING_SRC" = "" ] &&
{
	echo "Must define $ING_SRC to continue..."
	exit 1
}

[ -f $ING_SRC/bin/ensure ] && ensure ingres || exit 1

do_mkmings=false
do_ming=false
while [ ! "$1" = "" ]
do
	if [ "$1" = "-mkmings" ]
	then
		do_mkmings=true
	elif [ "$1" = "-ming" ]
	then
		do_ming=true
	fi
	[ "$2" = "" ] && break
	shift
done

# need to mkming?
if $do_mkmings
then
	echo ""
	echo "Doing mkmings..."
	mkmings
fi

if [ "$ING_VERS" = "" ]
then
	V=
else
	V="${ING_VERS}"
fi

# need to rebuild MING?
if $do_ming 
then
	echo ""
	echo "Building tools/port${V}/ming (ming)..."
	cd $ING_SRC/tools/port${V}/ming
	sh -x BOOT | tee OUT
fi

# need to build $ING_SRC/tools?
echo ""
echo "Building tools/port${V}..."
cd $ING_SRC/tools/port${V}
mk | tee OUT

# need to build all specials?
for x in tools/port${V}/specials cl/clf${V}/specials_unix  dbutil/duf${V}/specials_unix
do
	echo ""
	echo "Building $x..."
	cd $ING_SRC/$x
	mk | tee OUT
done

# need to build GL/CL libs?
for f in gl/glf${V} cl/clf${V}
do
	echo ""
	echo "Building $x lib..."
	cd $ING_SRC/$f
	mk lib | tee OUT
done

echo ""
echo "Building cl/clf${V}/er_unix_win..."
cd $ING_SRC/cl/clf${V}/er_unix_win
mk | tee OUT

echo ""
echo "Building common/hdr{$V}/hdr..."
cd $ING_SRC/common/hdr${V}/hdr
mk | tee OUT

# need to build yacc's and eqmerge?
for f in cl back/psf${V}/yacc front/tools${V}/yacc front/tools${V}/yycase front/tools${V}/eqmerge 
do
	echo ""
	echo "Building $f..."
	cd $ING_SRC/$f
	mk | tee OUT
done

cd $ING_SRC/common/hdr${V}/hdr
if [ ! -f fe.cat.msg ] 
then
	echo ""
	echo "Building fe.cat.msg..."
	cd $ING_SRC/common/hdr${V}/hdr
	mkfecat | tee OUT
fi

# need to build all HDRS?
for d in cl back common front dbutil
do
	echo ""
	echo "Building $d HDRS..."
	cd $ING_SRC/$d${V}
	files="`ls 2>/dev/null`"
	for f in $files
	do
		if [ -d $f ]
		then
			cd $f
			ffiles="`ls 2>/dev/null`"
			for ff in $ffiles
			do
				if [ -d $ff -a "$ff" = "hdr" ]
				then
					cd $ff
					echo "Doing $f/$ff..."
					mk hdrs | tee OUT
					cd ../
				fi
			done
			cd ../
		fi
	done
	cd ../
done

echo ""
echo "Building common/adf${V} lib..."
cd $ING_SRC/common/adf${V}
mk lib | tee OUT

echo ""
echo "Building front/utils${V} lib..."
cd $ING_SRC/front/utils${V}
mk lib | tee OUT

# need to build esqlc eqc?
for f in c csq
do
	echo ""
	echo "Building $f..."
	cd $ING_SRC/front/embed${V}/$f
	mk | tee OUT
done

echo ""
echo "Building front/utils${V} lib..."
cd $ING_SRC/front/utils${V}
mk lib | tee OUT

# need to build all HDRS?
for d in cl back common front dbutil
do
	echo ""
	echo "Building $d hdrs..."
	cd $ING_SRC/$d${V}
	files="`ls 2>/dev/null`"
	for f in $files
	do
		if [ -d $f ]
		then
			cd $f
			ffiles="`ls 2>/dev/null`"
			for ff in $ffiles
			do
				if [ -d $ff -a "$ff" = "hdr" ]
				then
					cd $ff
					echo "Doing $f/$ff..."
					mk hdrs | tee OUT
					cd ../
				fi
			done
			cd ../
		fi
	done
	cd ../
done

# need to build all LIBS?
for x in back common dbutil testtool front admin sig
do
	echo ""
	echo "Building $x lib..."
	cd $ING_SRC/$x${V}
	mk lib | tee OUT
done

# need to build libingres.a?
echo ""
echo "Building libingres.a..."
mergelibs 

if [ ! "$LD_LIBRARY_PATH" = "" -o ! "$SHLIB_PATH" = "" ]
then
	echo ""
	echo "Building shared libraries..."
	mkshlibs | tee OUT
fi

# need to build all EXE?
for x in tools back common dbutil testtool vec31 front admin testtool sig
do
	echo ""
	echo "Building $x..."
	cd $ING_SRC/$x${V}
	mk | tee OUT
done

# need to build all Testing programs?
for x in stress suites/shell be/msfc
do
	echo ""
	echo "Building $x..."
	cd $ING_TST/$x${V}
	mk | tee OUT
done

# need to build IIMERGE.O?
echo ""
echo "Building back/dmf${V}/specials_unix (iimerge.o)..."
cd $ING_SRC/back/dmf${V}/specials_unix
mk | tee OUT

# need to build IIMERGE?
echo ""
echo "Building iimerge..."
iilink -noadt

exit 0
