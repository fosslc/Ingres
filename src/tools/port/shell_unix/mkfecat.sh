:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkfecat - Create the file fe.cat.msg.
#
# This file is the concatenation of all the *.msg files from the
# "front" and "gateway" directories of a source tree.
#
## History:
##	02-may-89 (russ)
##		Created.
##	07-Jul-89 (fredp)
##		Fix sh/csh bug--make the proper shell execute this script.
##	25-Aug-89 (harry)
##		Make sure fe.cat.msg gets created in $ING_SRC/common/hdr/hdr.
##		Otherwise, having a "fe.cat.msg" file anywhere else in 
##		$ING_SRC will cause lots of grief.
##	24-Oct-90 (pholman)
##		Ensure that DBMS/PE can use this, by using V envrionmental and
##		coping where $ING_SRC is a symbolic link to somewhere else.
##	15-feb-93 (dianeh)
##		Changed the logic so that mkfecat creates fe.cat.msg from both
##		the .msg files in common/hdr/hdr/and throughout the front-end
##		(this is doable now, because of the SQLSTATE_MAPPING stuff);
##		removed use of tmp files, just use pipes instead; removed calls
##		to sed, just use sort instead; changed History comment-block
##		to double pound-signs; changed check for ING_SRC set; got
##		rid of mv'ing old fe.cat.msg to fe.cat.msg.old -- couldn't
##		think of any reason to have to keep it.
##	19-feb-93 (dianeh)
##		Removed the cat'ing of the common/hdr/hdr/er*.msg files into
##		fe.cat.msg -- not only is it redundant (since the files are
##		in that directory anyway) but it interferes with the backend-only
##		build process.
##	19-feb-93 (dianeh)
##		Replace erroneously removed redirect symbol in the cat of the
##		message files.
##	22-mar-93 (sweeney)
##		Change line which generates MSGFILES so that it
##		works correctly in the baroque environment. (Thanks
##		to pholman for this fix.)
##	30-apr-93 (dianeh)
##		Add a missing closing quote.
##	21-sep-93 (peterk)
##		for baroque, collect only .msg files in .../$ING_VERS
##	23-sep-93 (vijay)
##		if ING_VERS is not set, grep "" will match nothing.
##	29-oct-93 (peterk)
##		make find follow symlinks in case any front directories
##		happen to be symlinks.
##	1-nov-93 (peterk)
##		test if find -follow is supported before using it.
##		optimize for BAROQUE;
##	12-apr-94 (swm)
##		Unfortunately, not all versions of find can follow symbolic
##		links (eg. OSF1). With the current policy for BAROQUE
##		development paths the $ING_SRC/front/*/$ING_VERS argument
##		is usually a symbolic link. Re-organised the assignment to
##		MSGFILES for BAROQUE environments to bypass the top-level
##		links using a for-loop. Note that the find search tree will
##		still be pruned if a directory symbolic link is encountered
##		lower down in the tree.
##		Also, the 'find /dev/null -follow' will not always work
##		because no action has been specified (eg. SVR4). Added -print
##		action to avoid this problem.
##	06-apr-1998 (canor01)
##		Add gateway messages.
##	01-feb-2000 (marro11)
##		Made gateway logic conditional on existence of source dir.
##		Gateway bits were added, then removed.  May be readded???
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	23-Mar-2007 (hanje04)
##	    SIR 117985
##	    Remove '+1' from sort command when it's not supported as
##	    later versions of Linux don't support it
##		
#

: {$ING_SRC?"must be set"}

CMD=`basename $0`

[ -n "$ING_VERS" ] && V="$ING_VERS"

catfile=$ING_SRC/common/hdr/$V/hdr/fe.cat.msg

# Find the front-end and gateway .msg files, sort and unique them

if [ -d "$ING_SRC/gateway" ] ; then
    if [ -n "$ING_VERS" ] ; then
	gwdirs=`echo $ING_SRC/gateway/*/$ING_VERS`
    else
	gwdirs="$ING_SRC/gateway"
    fi
    msgclause="and gateway "
else
    gwdirs=""
    msgclause=" "
fi

echo "Finding front-end ${msgclause}msg files..."
find /dev/null -follow -print >/dev/null 2>&1 && follow=-follow
export follow
plusone=+1
sort +1 /dev/null >/dev/null 2>&1 || plusone='-k 2'
export plusone

if [ "$ING_VERS" ]
then
    MSGFILES=`(
	for dir in $ING_SRC/front/*/$ING_VERS $gwdirs ; \
	do \
	    cd $dir ; \
	    find * $follow -name '*.msg' -print | \
	    sed -e "s:^:${dir}/:" ; \
	done \
	) | \
           sed -e "s:^$ING_SRC/::" -e "s:[^/]*.msg:	&:" | \
	   sort -u -t'	' $plusone | sed "s:	::"`
else
    MSGFILES=`find $ING_SRC/front $gwdirs $follow -name '*.msg' -print | \
           grep "/$V" | \
           sed -e "s:^$ING_SRC/::" -e "s:[^/]*.msg:	&:" | \
           sort -u -t'	' $plusone | sed "s:	::"`
fi

# Build $catfile
echo "Building fe.cat.msg..."
rm -f $catfile
for file in $MSGFILES
 {
   msgfile=`echo $file | sed "s:^:$ING_SRC/:"`
   cat $msgfile >> $catfile
 }

echo "$CMD: done: `date`"
exit 0
