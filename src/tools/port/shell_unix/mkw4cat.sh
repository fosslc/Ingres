:
#
# Copyright (c) 2004 Ingres Corporation
#
# mksphcat - Create the file sph.msg.
#
# This file is the concatonation of all the *.msg files from the
# "w4gl" code tree.
#
# History:
#	03-jul-90 (brett)
#	    Created from mkfecat.sh.
#	23-jul-90 (brett)
#	    Renamed the msg file, name was too long for the old copy.
#	17-aug-90 (brett)
#	    Renamed sapphire to w4gl, leave files with sph in the name alone.
#	08-oct-90 (brett)
#	    General clean up and check of functionality.
#	14-jan-91 (brett)
#	    Add chmod to set the correct modes on the message files.
#	18-mar-91 (jab)
#	    Copied from w4gl03 library, changed the "sed" to "awk"
#           invocations, and changed it so that it would run all of the
#           *.msg files at once. This saves us from having to concatenate
#           it into one giant ".msg" file, which doesn't work if only a
#           handful use "GENERIC_ERROR_MAPPING".
#	16-apr-91 (jab)
#	    It now copies all the ".msg" files to a directory in /tmp,
#	    and just runs "ercomp *.msg". There's a problem in that
#	    ercompile insists on "generr.h" to be in the local directory
#	    (long story, not worth the wait), and so we just copy it in
#	    from $ING_SRC/common/hdr/hdr.
#	08-may-91 (kimman)
#	    Changing ${TMPDIR:-"/tmp"} to be more portable ${TMPDIR-"/tmp"}
#	30-oct-91 (jab)
#	    Put in support for the 6.4 error file names.
#	26-feb-92 (jab)
#	    Moved to support W4GLII mechanisms. Really doesn't change a
#		lot, but there are no more sparse trees. At all.
#	11-jan-93 (lauraw)
#		Modified for new VERS/CONFIG scheme:
#		Release id comes from genrelid now.
#	14-sep-93 (dwilson)
#		Added a line copying $ING_SRC/common/hdr/hdr/sqlstate.h,
#		for same reason we copied generr.h to the temporary
#		directory (see above)
#	15-sep-93 (dwilson)
#		Needed to change fastname and slowname from *w4gv3.mnx
#		to *w4gv4.mnx for W4GL 3.0
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

TMP1=${TMPDIR-"/tmp"}/w4gl.$$
LONGSPHVER=`genrelid W4GL`				# long version name
IS64PORT=`expr "$LONGSPHVER" : 6.4`
if [ $IS64PORT ]
then
	fastname=$ING_BUILD/files/english/fw4gv4.mnx
	slowname=$ING_BUILD/files/english/sw4gv4.mnx
else
	fastname=$ING_BUILD/files/english/fastsph.mnx
	slowname=$ING_BUILD/files/english/slowsph.mnx
fi

trap "rm -rf $TMP1" 0
mkdir $TMP1

if [ -z "$ING_SRC" ]
then
	echo $0: \$ING_SRC must be set
	exit 1
fi

ensure ingres rtingres || exit 1

#
# find the files, sort by the last component of the filename,
# and save the output in $TMP1.


cd $ING_SRC

find . -name '*.msg' -print  | egrep -v '/oldmsg/|fe.cat.msg' |  while read file
do
cp $file $TMP1
done

#
# Create .mnx files
#
(
cd $TMP1
cp $ING_SRC/common/hdr/hdr/generr.h $TMP1
cp $ING_SRC/common/hdr/hdr/sqlstate.h $TMP1
ercompile -f$fastname -s$slowname  *.msg

chmod 644 $fastname $slowname
)

exit 0
