:
#
# Copyright (c) 2004 Ingres Corporation
#
# Name:	mk3glfiles
#
# This is invoked from the w4gl!specials!MINGH file to do anything
# special to create 3GL support files on a per-platform basis.
#
# mkming just threw up its arms at the idea that IT should be expected
# History:
#	11/90 (erickson) Creates for ris_us5 w4gl user3gl shared libraries.
#	Other platforms may need different flags, etc.
#	11-jun-91 (jab)
#		Converted into tool   used by w4gl!specials!MINGH
#	16-jun-91 (jab)
#		Bug fix: hp 9000/800, wasn't installing u3resproc.o. (Added this
#		to R/S 6000 code also.
# 	20-jun-91 (vijay)
#		change $(ING_BUILD) to $ING_BUILD. and single ' to ".
# 	26-jul-91 (jab)
#		Last of the DL changes for Encore support of DL.
# 	26-feb-92 (jab)
#		Adding the DL changes for the W4GLII port.
# 	13-mar-92 (jab)
#		Adding the builds of libqsys (the PIC versions for f77 support)
#		here. It's necessary to do *HERE* because otherwise we'd need
#		to build it twice from a MINGH file, which is uglier still.
#		Also, collapsing the hp8/hp3/sun cases together, because they
#		all use the same DL mechanisms (now).
#	28-apr-92 (szeng)
#		1) Put dnet_stub.o, which is used to resolve undefined entries 
#		with non-DECNET ultrix machines, into $ING_BUILD/lib for 
#		ds3_ulx. (reintegrated)
#		2) took out u3hooks.o since it is no needed any more.
#		3) Imitated hp8 to build liqsys for PIC version of f77.
#	8-jun-92 (thomasm)
#		Make machine specific changes for RS/6000:
#			1. Bump up rev# on libuser3gl.so
#			2. filter comments in .exp files 
#			3. set correct binder flags
##	20-aug-1992 (jab)
##	     Added support for the working RS/6000 dynamic linking support(!).
##	21-aug-1992 (jab)
##	     Now create the libqsys for W4GL (libw4olang.a) for RS/6000.
#
#	29-jun-92 (vijay)
#		Support for motorola dynamic linking. We need a dummy
#		shared library at link time to export symbols from a.out.
#       20-sep-92 (mwc)
#            Integrated following change:
#                 20-aug-1991 (jeromes)
#                      added DRS[36]000 support. Different flags are needed on these
#                      platforms to produce the dynamic library.
#	13-oct-92 (szeng)
#	     Temporary solution for ds3.ulx to provide special
#	     libw4olang.a to provide all the functions needed
#	     by ESQLF.
#	23-nov-92 (pghosh)
#	    	Added Dynamic linking support for NCR (nc5_us5) system. 
#		Used szeng's way of creating libw4olang.a to solve unresolved
#		external for 3gl instead of using exports.so like m88 port.
#       02-dec-92 (gmcquary)
#           pym_us5 specific code added.
#	07-jan-92 (eileenm)
#		Added 3gl support for sequent by creating a static table
#	 	of the addresses of the 3gl routines.
#	01-feb-93 (sheppard)
#		addition for odt_us5; no dynamic linking here!
#		a lot of platforms have what are termed shared libraries,
#		which are dynamically linked and loaded. SCO does not
#		support dynamic linking/loading. It does however have
#		shared libraries which are statically linked with a single
#		copy of the library 'text' being shared among the programs that
#		use it, we do not used them.  Use the 'standard' 
#		library archive.
#		
#       08-feb-93 (pghosh)
#          	Modified nc5_us5 to nc4_us5. Till date nc5_us5 & nc4_us5 were
#          	being used for the same port. This is to clean up the mess so
#          	that later on no confusion is created.
#	17-aug-93 (gillnh2o)
#		Changed the pathname to reflect the uncoupling of
#		w4gl from the front. I did this only for ports
#		owned by the Integration group.
#       05-jan-95 (chech02)
#               Added rs4_us5 for AIX 4.1.
#       13-dec-95 (morayf)
#		Added SNI RMx00 (rmx_us5) like Pyramid (pym_us5).
#	10-may-1999 (walro03)
#		Remove obsolete version string dr6_uv1, dra_us5, enc_us5,
#		odt_us5.
#       03-jul-1999 (podni01)
#               Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
#	19-jul-2001 (stephenb)
#		Add support for i64_aix
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
#
. iisysdep
ensure ingres || exit 1

TMP=/tmp/x$$
mkdir $TMP && cd $TMP

case $VERS in
ds3_ulx)
	ar xv $ING_BUILD/lib/libuser3gl.a
	ld -o $ING_BUILD/lib/u3resproc.o -r -x u3resproc.o
	cd $ING_SRC/w4gl/fleas/specials
	cc -c -o $ING_BUILD/lib/dnet_stub.o dnet_stub.c
        cd $ING_SRC/bin
        ./mergeolang
	;;
rs4_us5|ris_u64|\
ris_us5|i64_aix)
	 # Binder flags for shared library
    files=$ING_BUILD/files/sph_to_3gl.exp
	grep -v '^\*'  $ING_SRC/w4gl/fleas/specials/sph_to_3gl.exp > $files
	chmod 644 $files
	(cd $ING_SRC/w4gl/fleas/sphrexes && ming CCLDMACH="-x -bE:$files" EXEMODE=755)
	OBJECTS='u3resproc.o'
    ar xv  $ING_BUILD/lib/libuser3gl.a $OBJECTS
    cp $OBJECTS $ING_BUILD/lib
	cd  $ING_BUILD/lib && chmod 644 $OBJECTS
	cd $ING_SRC/front/frontcl/libqsys
	ming -F 'LIBQSYSLIB=$(W4OLANGLIB)' 'CCDEFS=$(CCPICFLAG)'
	;;

su4_u42|hp3_us5|hp8_us5|sqs_ptx)
	cd $ING_SRC/w4gl/fleas/user3gl
	ming -F CCDEFS='$(CCPICFLAG)' u3resproc.o && cp u3resproc.o $ING_BUILD/lib
	cd $ING_SRC/front/frontcl/libqsys
	ming -F 'LIBQSYSLIB=$(W4OLANGLIB)' 'CCDEFS=$(CCPICFLAG)'
	;;
m88_us5)
	cd $ING_SRC/w4gl/fleas/user3gl
	ming -F CCDEFS='$(CCPICFLAG)' u3resproc.o && cp u3resproc.o $ING_BUILD/lib
	cd $ING_SRC/front/frontcl/libqsys
	ming -F 'LIBQSYSLIB=$(W4OLANGLIB)' 'CCDEFS=$(CCPICFLAG)'
	# make the dummy shared library which helps in exporting symbols from
	# sphmerge
	cp $ING_SRC/w4gl/fleas/specials/exports.c /$TMP/exports.c
	cc -K PIC -G -o $ING_BUILD/lib/exports.so /$TMP/exports.c
	;;
dr6_us5)
        OBJECTS='u3resproc.o IIu3gltab.o'
        cd $ING_SRC/w4gl/fleas/user3gl
        ming -F $OBJECTS CCFLAGS='-K PIC'
        cc -G -o $ING_BUILD/lib/libuser3gl.so $OBJECTS
        chmod 755 $ING_BUILD/lib/libuser3gl.so
        ld -r -o $ING_BUILD/lib/u3resproc.o u3resproc.o
        ;;
nc4_us5)
	cd $ING_SRC/w4gl/fleas/user3gl
	ming -F CCDEFS='$(CCPICFLAG)' u3resproc.o && cp u3resproc.o $ING_BUILD/lib
	cd $ING_SRC/bin
        ./mergeolang
	;;
pym_us5|rmx_us5|rux_us5)
        cd $ING_SRC/w4gl/fleas/user3gl
        ming IIu3gltab.o
        ming u3resproc.o
        ld -G -o libuser3gl.so.2.0 IIu3gltab.o u3resproc.o
        mv libuser3gl.so.2.0 $ING_BUILD/lib/libuser3gl.so.2.0
        cd $ING_SRC/front/frontcl/libqsys
        ming -F 'LIBQSYSLIB=$(W4OLANGLIB)' 'CCDEFS=$(CCPICFLAG)'
        ;;
esac
rm -rf $TMP
exit 0
