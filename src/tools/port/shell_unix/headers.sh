:
#
# Copyright (c) 2004 Ingres Corporation
#
# 'headers' gives a hierarchic list of header files of specified C file(s)
# -l option gives a sorted list of all header files included anywhere
# -d option lists duplicate headers
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

(for i in $*
do
    case $i in
	-d | -D) action=d continue;;
	-l | -L) action=l continue;;
	-h | -H) action=h continue;;
	-*) continue;;
	*)  fname=`echo $i.c | sed 's/\..*/\.c/'`
	    [ -f $fname ] || echo $fname does not exist
	    [ -f $fname ] || continue
	    case $action in
	       d) echo $fname
                  cexpand $i | grep '^/\* # 1 ' | sort |
                      tee /tmp/duphdr$$ | sort -u | diff - /tmp/duphdr$$ | 
		      grep '#' | awk '{print "    " $5}'
                  rm /tmp/duphdr$$ ;;
	       l) echo $fname
		  cexpand $i | grep '^/\* # 1 ' | 
		      tail +2 | awk '{print "    " $4}'| sort;;
	       *) cexpand -c $i | awk '
                    {
                        if ($1 == "/*" && $2 == "#" && int($3) > 0)
                        {
	                    if ($3 == "1")
	                    {
	                        for (j = 0; j < i; j++) printf("    ") 
	                        print $4
	                        stack[++i] = $4
	                    }
	                    else
	                    {
	                        if (i > 0 && stack[i] != $4)
	                        i--
	                    }
                        } 
                        next
                    } ' ;;
   	   esac
    esac
done) | sed 's/"//g'
