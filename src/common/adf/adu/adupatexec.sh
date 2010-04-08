#!/bin/sh
#
# adupatexec.sh - process the graph source file adupatexec.dotty
#
# From this standard DOT(TY) file input we can produce several outputs.
# Most important is that we will produce the include files with the definitions
# for all the operators and their states.
# We can also produce a reformatted version which produces a prettier graph plus
# the sub-graphs used in the DDS if required.
#
# Usage:
#  adupatexec.sh [-p [-o OUT]]
#   -p		Produce the 'prettied' output with sub-graphs. The subgraphs
#		actually written to will be listed in a file adupatexec.lis
#
#  If no qualifiers or parameters are passed then the outputs
#  will be the include files for the state engine.
#
fmt=eps
base=`basename $0 .sh`
pretty=""
out=""
if [ "$1" = "-p" ];then
  pretty="-vpretty=1"
  shift
  out=""
  if [ "$1" = "-o" ];then
    out=$2
    shift
    shift
  fi
  INF=${1:-$base.dotty}
  IN=`basename $INF .dotty`
  if [ "_$out" = "_" ];then
    out=$IN-rev.dotty
  fi
  vout="-vout=$out"
  awk $pretty $vout -v list="$IN.lis" -f adupatexec.awk $INF
  dot -T$fmt -o $IN.$fmt $INF
  dot -T$fmt -o `basename $out .dotty`.$fmt $out
  cat $IN.lis|while read f;do
    b=`basename $f .dotty`.$fmt
    echo "Formatting: $b"
    dot -T$fmt -o $b $f
  done
else
  INF=${1:-$base.dotty}
  if awk -v header=${base}_ops.h.new \
		 -v body=${base}_inc.i.new -f adupatexec.awk $INF ;then
	mv --backup ${base}_ops.h.new ${base}_ops.h
	mv --backup ${base}_inc.i.new ${base}_inc.i
  else
    echo "Error processing awk command"
    exit -1
  fi
fi


