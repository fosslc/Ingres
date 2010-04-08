:
#
# Copyright (c) 2004 Ingres Corporation
#
#
# makelocs -- set up INGRES test installation for rodv tests
#
# History:
# 	03/23/89 (boba)
#		Written.
#	05/10/89 (arana)
#		Make sure TERM_INGRES=vt100 and the default termcap file
#		is used when invoking accessdb with keystroke file.
#	04-aug-1989 (boba)
#		Moved from makelocs61.sh to mklocs.sh when added to ingresug.
#       04-aug-1989 (boba) 
#               Modify for new qa source path when added to ingresug.
#       23-jul-1990 (owen)
#		Added to ingres63p, only making location qaloc1 now.
#	19-aug-1990 (owen)
#		Move from septool to septool_unix directory.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
#
# must run as INGRES super-user, either ingres or as a porter.
#

# create directories:

umask 0
set qaloc1 # locations used in tests

cd ${ING_BUILD?"must be set to point to test installation."}
for loc in $*
do
	for area in data ckp jnl
	do
		if [ ! -d $area/$loc ]
		then
			echo "Making directory \"$area/$loc\"..."
			mkdir $area/$loc
		else	# must be set up already!
			echo "Directory \"$area/$loc\" already exists."
			continue
		fi
	done
done


# add to system catalog:

echo "Adding locations..."

TERM_INGRES=vt100
II_TERMCAP_FILE=
export TERM_INGRES II_TERMCAP_FILE
accessdb -Z$ING_SRC/testtool/sep/files_unix_win/makelocs.key
