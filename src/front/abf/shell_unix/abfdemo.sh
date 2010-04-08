:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
#	Abfdemo		09/13/88 (trina)
#	
#	
#  Determines whether or not the demo has already been installed by checking
#  for the existence of the file :
#		$HOME/abfdemo/iidb_name.txt
#
#	
#  If the file exists, start up of abf on the database name found in 
#		iidb_name.txt
#  Otherwise,
#	-- get a database name from the user.  Will create the database
#	   if the user so desires.
#	-- create the directory $HOME/abfdemo along with the appropriate sub
#	   directories.
#	-- fill the database with the appropriate data
#	-- start abf on the database with the sample application (projmgmt)
#
#  History:
#	12-may-1989 (boba)
#		Fix path name of frame61.h in .c files.  Also cleaned up
#		similar transformation of .osq files and activated sysmod.
#	23-may-1989 (boba)
#		Remove unnecessary, unportable -i grep option.
#	07-June-1989 (fredv)
#		Changed the default source to $HOME/abfdemo.
#	09-oct-1989 (boba)
#		Remove logic that fixes path of frame61.h files now that
#		no .c files are present with 6.2 version of demo files.
#	17-oct-1989 (gautam/kimman)
#		Adding sreport and copyform so that abfdemo can find the
#		reports and forms.
#	25-jan-1990 (rog)
#		Allow user to use an already existing database.  Also, change
#		grep of createdb output to check exit status of createdb.
#	10-apr-1990 (rog)
#		Add support for using a database through INGRES/Net.
#	24-apr-1990 (gautam)
#		Fixed mkdir directory name
#	08-may-1990 (mgw)
#		Fixed typo in Welcome to INGRES ABF DEMO message.
#       01-Aug-2004 (hanch04)
#           First line of a shell script needs to be a ":" and
#           then the copyright should be next.
#	16-Feb-2010 (hanje04)
#	    SIR 123296
#	    Add suppor for LSB builds

(LSBENV)

ABFDEMODIR=$HOME/abfdemo
FNAME=$ABFDEMODIR/iidb_name.txt
ING_ABFDIR=$ABFDEMODIR/abf
TMP_DEMODIR=$II_SYSTEM/ingres/files/abfdemo
II_PATTERN_MATCH=SQL 
II_DML_DEF=SQL 
export ING_ABFDIR
export II_DML_DEF
export II_PATTERN_MATCH

# Checks what echo style is on the system, for echo -n or echo ... \c 

if [ "`echo '\tt'`" = "	t" ]
then
	PROMPT="> \c"
else
	PROMPT="-n > "
fi

# Checks to see if the terminal type is defined.  If neither TERM_INGRES
# nor TERM is defined, asks the user for the terminal type.

if [ -z "$TERM_INGRES" ]
then
    TERM_INGRES=$TERM
    if [ -z "$TERM" ]
    then
	echo ""
	echo "No terminal type is defined in your environment.  INGRES needs"
	echo "to know what type of terminal you are using in order to work"
	echo "properly."
	echo ""
	while [ -z "$TERM_INGRES" ]
	do
	    echo "Please enter the type of terminal you are using.  Ex: vt100"
	    echo " 	Or type 'quit' to quit"
	    echo $PROMPT
	    read TERM_INGRES
	    if [ "$TERM_INGRES" = "quit" ]
	    then
		echo ""
		echo $BOLD
		echo "Good-Bye"
		echo $NORMAL
		echo ""
		exit 0
	    fi
	done
    fi
    export TERM_INGRES
fi

# Defines the variables for reverse, bold, and normal video for the VT100,
# VT220, or VT320.
case $TERM_INGRES in
    vt[123][02]0* | sun*)
	REVERSE=[7m
	BOLD=[1m
	NORMAL=[0m
	;;
    *)
	REVERSE=
	BOLD=
	NORMAL=
esac

# Checks to see if there is II_SYSTEM defined.  If not, exits.

if [ ! "$II_SYSTEM" ]
then
    echo $BOLD
    echo "Your environment isn't set up correctly.  Please see your"
    echo "system administrator."
    echo $NORMAL
    exit 1
fi

# If abfdemo does not find $HOME/abfdemo, then print the initial greeting
# and create the database if requested.  Allow the user to exit after he has
# been told about the side effects. 

cd
if [ ! -d "$ABFDEMODIR" ] 
then
    echo $REVERSE
    echo "


               Welcome to the INGRES ABF DEMONSTRATION!                    
                                                                           
        To use the INGRES ABF Demonstration, a database and some           
        files must be created for your use. The demonstration              
        creates a subdirectory called abfdemo in your login                
        (home) directory as well as a database using a name you            
        supply. When you run this command for the first time, it will      
        take several minutes while the necessary files are set up,         
        but after the installation process is completed, you can use       
        this command to start ABF and pick up where you left off.          
        The deldemo command can be used to de-install the demo and         
        remove your database.  If you use an existing database, some       
        tables, i.e., emp, projects, tasks, dependents, and some forms,    
        i.e., emp, projects, tasks, will be overwritten.                   

"
    echo $NORMAL
    echo $BOLD
    echo "Do you wish to continue? "
    echo $NORMAL
    echo $PROMPT
    while :
    do
        read resp
        if [ "$resp" = "n" -o "$resp" = "no" ]
        then
	    echo ""
	    echo $BOLD
	    echo "Good-Bye"
	    echo $NORMAL
	    echo ""
	    exit 0
        fi

        if [ "$resp" = "y" -o "$resp" = "ye" -o "$resp" = "yes" ]
        then
	    break
        else
	    echo $BOLD
	    echo " 'yes' or 'no' please"
	    echo $NORMAL
	    echo $PROMPT
        fi
    done

# Gets the name of the database.  If the name is good, then the database
# is created.  It will check to see if the name is good by checking to see
# if there is any errors.  The test is on the return status of 'grep'.

echo "Would you like to create a new database for ABFDEMO?"
echo $PROMPT
while :
do
	read resp
	if [ "$resp" = "n" -o "$resp" = "no" ]
	then
	    echo "Enter the name of an existing database."
	    echo $PROMPT
	    read dbname
	    break
	elif [ "$resp" = "y" -o "$resp" = "ye" -o "$resp" = "yes" ]
	then
	    while :
	    do
		echo ""
		echo $BOLD
		echo "What name do you wish to give your ABFDEMO database? "
		echo $NORMAL
		echo ""
		echo "     Or you may type 'quit' to quit. "
		echo ""
		echo $PROMPT 
		read dbname
		if [ "$dbname" = "quit" ]
		then
		    echo ""
		    echo $BOLD
		    echo "Good-Bye"
		    echo $NORMAL
		    echo ""
		    exit 0
		fi
		if [ ! -z "$dbname" ]
		then
		    echo "Creating database '$dbname'..."
		    createdb $dbname 2>&1 > iitmp
		    if [ $? -ne 0 ]
		    then
			grep -s exists iitmp
			if [ $? -eq 0 ]
			then
			    echo "Database '$dbname' already exists, try another name."
			 else
			    echo ""
			    echo $BOLD
			    echo "There is a problem executing that request.  "
			    echo "Please check with your INGRES system manager."
			    echo $NORMAL
			    echo ""
			    exit 1
			fi
		    else
			break
		    fi
		fi
	    done
	    rm iitmp
	    break
	else
	    echo $BOLD
	    echo " 'yes' or 'no' please"
	    echo $NORMAL
	    echo $PROMPT
	fi
done

# Creates needed directories under the user's login directory.
    if echo $dbname | grep '::' > /dev/null 2>&1
    then
	dbdirname=`expr $dbname ':' '[^:]*:*\(.*\)'`
	dosysmod=false
    else
	dbdirname=$dbname
	dosysmod=true
    fi

    echo "Creating the directory '$ABFDEMODIR'..."
    echo ""

    mkdir $ABFDEMODIR

    echo "Creating the directory '$ABFDEMODIR/abf'..."
    echo ""

    mkdir $ABFDEMODIR/abf

    echo "Creating the directory '$ABFDEMODIR/abf/$dbdirname'..."
    echo ""

    mkdir $ABFDEMODIR/abf/$dbdirname

    echo "Creating the directory '$ABFDEMODIR/abf/$dbdirname/projmgmt'..."
    echo ""

    mkdir $ABFDEMODIR/abf/$dbdirname/projmgmt

# Stores the database name in the file iidb_name.txt.

    cd $ABFDEMODIR
    echo $dbname > iidb_name.txt

# Copies all files from $TMP_DEMODIR to the user's abfdemo directory.

    cd $TMP_DEMODIR
    cp * $ABFDEMODIR

# Converts the paths in the .osq files to the user's abfdemo dir

    for file in *.osq 
    do
	rm -f $ABFDEMODIR/$file
	sed "s^II_SYSTEM\:\[INGRES\.FILES\.ABFDEMO\]^$ABFDEMODIR/^g" $file > $ABFDEMODIR/$file
    done

# Converts the source file directory to the user's abfdemo dir

	rm -f $ABFDEMODIR/iicopyapp.tmp
	sed "s^II_SYSTEM\:\[INGRES\.FILES\.ABFDEMO\]^$ABFDEMODIR^g" iicopyapp.tmp > $ABFDEMODIR/iicopyapp.tmp

# Copies the tables into the database.

    echo "Copying data into files..."
    sql -s $dbname < sqlcopy.in

# Setting up the application.  Using copyapp, and sysmod.  

    cd $ABFDEMODIR
    echo "Setting up the Sample Application..."
    copyapp in -r $dbname iicopyapp.tmp

    copyform -i -s $dbname $TMP_DEMODIR/qbf_forms.frm

    sreport -s $dbname $TMP_DEMODIR/experience.rw

    $dosysmod && {
	echo "Modifying system catalogs..."
	sysmod $dbname
    }

    echo ""
    echo ""
    echo $REVERSE
    echo "             The ABFDEMO has been installed.            " 
    echo $NORMAL
    echo ""
    echo ""
    echo "The name of the directory is '$ABFDEMODIR'"
    echo "The name of the database is '$dbname'"
    echo ""
    echo $BOLD
    echo "Would you like to start the ABFDEMO now? "
    echo $NORMAL
    echo $PROMPT
    read resp
    if [ "$resp" = "n" -o "$resp" = "no" ]
    then
	echo ""
	echo $BOLD
        echo "Good-bye"
	echo $NORMAL
	echo ""
        exit 0
    fi
fi

if [ -d $ABFDEMODIR ]
then 
    if [ -f $ABFDEMODIR/dbname.txt ]
    then 
	echo "It seems that you already have the ABFDEMO ver. 5.0 installed."
	echo ""
       	echo "In order to install the 6.0 version, you must run 'deldemo'."
        exit 1
    fi
    if [ ! -f $FNAME ]
    then 
        echo "A directory called '$ABFDEMODIR' already exists. You must rename "
        echo "the directory in order for the ABFDEMO to be installed."
	exit 1
    fi
fi

if [ ! -d $ABFDEMODIR/abf ]
then 
	    mkdir $ABFDEMODIR/abf
	    mkdir $ABFDEMODIR/abf/$dbdirname
	    mkdir $ABFDEMODIR/abf/$dbdirname/projmgmt
fi

dbname=`cat $ABFDEMODIR/iidb_name.txt`
echo  "Starting up the ABFDEMO Application on '$dbname'..."
echo  "Your files are in '$ABFDEMODIR'."
cd $ABFDEMODIR
echo ""
echo "abf '$dbname' projmgmt" 
echo ""
abf $dbname projmgmt

if [ ! $? -eq 0 ]
then 
    echo "Error starting up ABF on '$dbname'. "
    echo "Ask your system manager for help."
fi

echo "Exiting the INGRES ABFDEMO."


