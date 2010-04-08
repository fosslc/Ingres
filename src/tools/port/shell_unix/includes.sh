:
#
# Copyright (c) 2004 Ingres Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	10-Dec-2004 (hanje04)
##	    Make includes work with jam
##	14-Sep-2004 (hanje04)
##	    Add support for jam builds.
##      29-May-2007 (hanal04) Bug 118414
##          Double quote ^SubDir in grep to avoid grep failure on su9.us5
##      10-Dec-2008 (horda03)
##          Indent the headers (if request -i flag)
##      23-Mar-2010 (horda03)
##          Handle different directory levels not just 3.
##          Also exclude directories from the list of files.

if [ "$1" == "-i" ] ; then

   indent=1
   shift
else
   indent=0
fi

if [ "$ING_SRCRULES" ] ; then
    fgrist=`grep "^SubDir" Jamfile | awk '{ printf("<%s", $3) ; for(i=4; i<NF; i++) printf("!%s", $i) ;  print ">"}'`
    fname=`echo $1 | cut -d. -f1`
    jam -an -d5 ${fgrist}${fname}.o  | awk -v indent=$indent '
 	$1 == "bind" { 
                pad="";

                if (indent==1)
                {
                   n=split($0, parts, "-");

                   file=parts [3];

                   # The first char is a TAB, so count the spaces so that
                   # the header file can be indented. Thus easy to determine
                   # where the file gets included.

                   for(i=3; substr(file,i,1) == " "; i++)
                   {
                      pad= pad "#";
                   }
                }
                printf( "%s%s\n", pad, $4)
 	}' | while read line
             do
                inc_file=`echo $line | sed -e "s/#/ /g"`

                if [ ! -d $inc_file ]
                then
                   echo "$inc_file"
                fi
             done
else
    ming -o $* | awk '
	$1 == "make" { lev = lev + 1 }
	$1 == "done" { lev = lev - 1 }
	$1 == "file" {
		for( i = 1; i < lev; i++ )
			printf "    ";
		printf "%s\n", $2;
	}'
fi

