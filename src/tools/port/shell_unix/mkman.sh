:
#
# Copyright (c) 2004 Ingres Corporation
#
# make man page for $1
#
# History:
#	31-jul-1989 (boba)
#		Change to new tools directory structure for ingresug.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	27-Oct-2004 (drivi01)
##	    Updated references to shell directories with shell_unix b/c
##	    all shell directories on unix were moved to shell_unix.

if [ $# != 1 ] 
then
	echo "Usage:	mkman program-name"
	exit 1
fi

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"

program=$1
if [ -r $program.1 ]
then
	echo Man page already exists for $program
else
	echo Making man page for $program
	p=`echo $program | sed "s/\(.\).*/\1/"`
	P=`echo $p | tr "a-z" "A-Z"`
	Program=`echo $program | sed "s/$p/$P/"`
	PROGRAM=`echo $program | tr "a-z" "A-Z"`
	cat $ING_SRC/tools/port$noise/shell_unix/mantmplt.1 | \
		sed "s/program/$program/g
			s/Program/$Program/g
			s/PROGRAM/$PROGRAM/g" > $program.1
	chmod 666 $program.1
fi

# edit it until it looks right.
while :
do
	${EDITOR-vi} $program.1
	nroff -man $program.1 | more -s
	echov "e)dit  q)uit : \c"
	read answer
	case $answer in
	q*|Q*)	
		break
		;;
	esac
done		
