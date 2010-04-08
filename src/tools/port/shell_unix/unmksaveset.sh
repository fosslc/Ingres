:
#
# Copyright (c) 2004 Ingres Corporation
#
## History
##	20-Oct-94 (GordonW)
##		created.
##	10-Nov-94 (GordonW)
##		make some tests on directories.
##	15-Nov-94 (GordonW0
##		test if tar worked.
##	29-Dec-94 (GordonW)
##		Missing "||"
##	23-Mar-1999 (kosma01)
##		On sgi_us5, the dot (.) in "tar xvpf - . " selects nothing.
##		However, no dot, selects everything.  
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

[ "$II_SYSTEM" = "" ] &&
{
	echo "II_SYSTEM needs to be defined..."
	exit 1
}
cd $II_SYSTEM ||
{
	echo "Can't cd to (II_SYSTEM) $II_SYSTEM..."
	exit 1
}
cd $II_SYSTEM/ingres ||
{
	echo "Can't cd to (II_SYSTEM/ingres) $II_SYSTEM/ingres..."
	exit 1
}

[ "$ING_TOOLS" = "" ] &&
{
	echo "ING_TOOLS must be defined..."
	exit 1
}
cd $ING_TOOLS ||
{
	echo "Can't cd to (ING_TOOLS) $ING_TOOLS..."
	exit 1
}

[ -f $ING_TOOLS/bin/ensure ] &&
{
	 ensure ingres || exit 1
}

echo "Getting $II_SYSTEM/ingres/install..."
cd $II_SYSTEM/ingres || mkdir ingres
tar xpvf $II_SYSTEM/ingres.tar install ||
{
	echo "Can't tar $II_SYSTEM/ingres.tar..."
	exit 1
}

echo "Getting $ING_TOOLS..."
cd $ING_TOOLS || exit 1
rm -rf files/* bin/* man1/* utility/*
TARCMD="tar xvpf - ."
[ `uname -s` = "IRIX64" ] && TARCMD="tar xvpf - "
zcat $II_SYSTEM/tools.tar.Z | $TARCMD ||
{
	echo "Can't tar $II_SYSTEM/tools.tar.Z..."
	exit 1
}
exit 0
