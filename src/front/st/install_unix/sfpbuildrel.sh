:
#|
#|  sfpbuildrel.sh
#|
#|	Bundles a distribution file from a private path. This
#|      script minimises the changes required in the buildrel
#|      executable, in order to successfully generate an
#|      SFP distribution file (ingres.tar)
#|
#|	Parameters: None.
#|
relfile=release.dat
versfile=version.rel

#! Log messages to the terminal
unset warn
warn()
{
    echo "$*" 1>&2
}

if [ X$ING_BUILD = "X" ] ; then
    warn "ERROR: ING_BUILD must be set"
    exit 1
fi 

if [ X$ING_VERS = "X" ] ; then
    warn "ERROR: ING_VERS must be set"
    exit 1
fi 

if [ X$BASEPATH = "X" ] ; then
    warn "ERROR: BASEPATH must be set"
    exit 1
fi 

if [ X$ING_VERS = X$BASEPATH ] ; then
    warn "WARN: Private path ($ING_VERS) equal to Base Path ($BASEPATH)"
    warn "      $ING_BUILD/$versfile will be overwritten."
    warn " "
    warn "      OK to continue [y/n]"
    read ans
    if [ X$ans != "Xy" ] ; then
	warn "Exiting . . ."
        exit 1
    fi
fi

if [ ! -d $ING_BUILD ] ; then
    warn "ERROR: $ING_BUILD/$ING_VERS does not exist."
    exit 1
fi

if [ ! -d $ING_BUILD/../$BASEPATH ] ; then
    warn "ERROR: $ING_BUILD/$BASEPATH does not exist."
    exit 1
fi

if [ X$DEEDXID = "X" ] ; then
    warn "ERROR: DEEDXID must be set"
    exit 1
else
    patchid=`echo $DEEDXID | sed -e s/.//`
    ingbuild -v > $ING_BUILD/$versfile
    echo $patchid >> $ING_BUILD/$versfile
fi

if [ X$PATCHTMP = "X" ] ; then
    warn "ERROR: PATCHTMP must be set"
    exit 1
fi

if [ X$PKID = "X" ] ; then
    warn "ERROR: PKID must be set"
    exit 1
fi

if [ ! -d $PATCHTMP/$PKID ] ; then
    warn "Directory $PATCHTMP/$PKID does not exist"
    exit 1
fi

II_TAR_DIR=$PATCHTMP/$PKID/p${patchid}
if [ -d $II_TAR_DIR ] ; then
    warn "Directory $II_TAR_DIR already exists."
    exit 1
else
    II_RELEASE_DIR=$II_TAR_DIR/unpacked
    mkdir -p $II_RELEASE_DIR
fi

if [ ! -f $II_MANIFEST_DIR/$relfile ] ; then
    II_OLD_MANIFEST_DIR=$II_MANIFEST_DIR
    II_MANIFEST_DIR=$ING_SRC/front/st/$BASEPATH/src/specials_unix
    if [ ! -f $II_MANIFEST_DIR/$relfile ] ; then
	warn "Could not find $relfile in either"
	warn "\t$II_OLD_MANIFEST_DIR"
	warn "or"
	warn "\t $II_MANIFEST_DIR"
	exit 1
    fi
fi

buildrel 

cd $II_RELEASE_DIR
tar -cvf $II_TAR_DIR/ingres.tar *
if [ $? -ne 0 ] ; then
    warn "ERROR: creating tar file $II_TAR_DIR/ingres.tar"
    exit 1
fi
cd $II_TAR_DIR
rm -rf unpacked
