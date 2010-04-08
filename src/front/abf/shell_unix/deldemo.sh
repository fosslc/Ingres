:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
# Deldemo	09/13/88 (trina)
#
# Destroys the demo by first checking for the appropriate directories.
#
# History;
#	12-may-1989 (boba)
#		Add check for appropriate user prompt as in abfdemo.
#	23-may-1989 (boba)
#		Remove unnecessary, unportable -i grep option.
#	10-apr-1990 (rog)
#		Add support for using a database through INGRES/Net.
#       01-Aug-2004 (hanch04)
#           First line of a shell script needs to be a ":" and
#           then the copyright should be next.
#	16-Feb-2010 (hanje04)
#	    SIR 123296
#	    Add suppor for LSB builds
#

(LSBENV)
# Checks what echo style is on the system, for echo -n or echo ... \c 

if [ "`echo '\tt'`" = "	t" ]
then
	PROMPT="> \c"
else
	PROMPT="-n > "
fi

# Defines the variables for reverse, bold, and normal video for the VT100,
# VT220, or VT320.

case $TERM_INGRES in
    vt[123][02]0* | sun*)
	BOLD=[1m
	NORMAL=[0m
	;;
    *)
	BOLD=
	NORMAL=
esac

cd
if [ -d abfdemo -a -f abfdemo/iidb_name.txt ]
then
    if [ ! -w abfdemo/iidb_name.txt ]
    then
        echo $BOLD
        echo "
		    The file '$HOME/abfdemo/iidb_name.txt' must be 
		    	readable in order to continue."
        echo ""
        echo ""
        echo ""
        echo "Good-Bye."
        echo $NORMAL
        echo ""
        exit 1
    fi

    DBNAME=`cat abfdemo/iidb_name.txt`
    echo $BOLD
    echo "WARNING! This program deletes all files in the ./abfdemo     
  	 subdirectory of your login directory.  As well as destroying
	 the database '$DBNAME'."
    echo $NORMAL
    echo ""
    echo " You should be certain that you have no important files in this" 
    echo " directory before you run this program."
    echo ""
    echo $BOLD
    echo "Do you wish to continue?"
    echo $NORMAL
    echo "$PROMPT"
    
    read resp
    if [ "$resp" = "n" -o "$resp" = "no" ]
    then
        echo ""
        echo $BOLD
        echo "Good-Bye."
        echo $NORMAL
        echo ""
        exit 0
    fi
    
    if echo $DBNAME | grep '::' > /dev/null 2>&1
    then
	VNODE=`expr $DBNAME ':' '\([^:]*\).*'`
	echo $BOLD
	echo ""
	echo "You must manually destroy '$DBNAME' on vnode '$VNODE'."
	echo $NORMAL
    else
	echo $BOLD
	echo ""
	echo "Destroying the database '$DBNAME'."
	echo $NORMAL
	echo ""
	destroydb $DBNAME | grep -s successfully 2>&1

	if [ $? -ne 0 ]
	then
	    echo $BOLD
	    echo ""
	    echo  "     Because the database you specified was not found, the files
	       in the abf subdirectory have not been destroyed."
	    echo ""
	    echo ""
	    echo "Good-Bye."
	    echo $NORMAL
	    echo ""
	    exit 1
	fi
    fi
    echo "Deleting all files in $HOME/abfdemo"
    rm -rf abfdemo
    echo ""
    echo $BOLD
    echo "Good-Bye."
    echo $NORMAL
    echo ""
    exit 0
else 
    echo ""
    echo $BOLD
    echo "The INGRES ABFDEMO is not installed in this account. "
    echo ""
    echo $NORMAL
    exit 1
fi
