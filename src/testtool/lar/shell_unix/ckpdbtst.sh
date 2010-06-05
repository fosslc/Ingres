:
#
# Copyright (c) 2004 Ingres Corporation
#
#  Online Checkpoint Test
#
#  This is called by SEP to test online checkpoint.
#
#  Outline:
#
#	  - Reload data in the database
#	  - Turn off journaling for database.
#	  - Start test programs as subprocesses.
#	  - Start Checkpoint process.
#	  - Wait for subprocesses to complete.
#	  - Check test results for correctness.
#	  - Roll database back to checkpoint.
#	  - Check data for consistency.
#
#  Usage:
#	
#	ckpdbtst dbname journaled id
#    or
#	ckpdbtst dbname notjournaled id
#
#       where:
#		- dbname is the name of a database
#
#		- "journaled" or "notjournaled" is a required keyword
#
#		- id is a unique character or short string that will 
#		  be used to create distinct filenames
#
##  History:
##
##	sep-27-1990 - owen
##		Update path for ckpdbmasks file from ckpdb to lar
##	oct-08-1990 - owen
##		Fixing typo in comment causing syntax error
##	13-aug-1991 (lauraw)
##		Combined old ckpdbtst1.sh and ckpdbtst2.sh, since
##		they were practically identical. Corrected comments.
##		Added usage description. Changed colons to hashmarks.
##		Added $df to distinguish filenames, for two reasons:
##		1) so this script can be run by more than one test
##		concurrently, and 2) to match existing sep canons in
##		tests bea24.sep and bea25.sep. 
##
##	05-oct-1992 (barbh)
##		Changed path of ckpdbmasks to ing_tst/be/lar/data. The
##		file ckpdbmasks was moved from ing_src/testtool/lar/lar. 
##		
##	22-oct-1992 (gillnh2o)
##		Added -l flag to ckpdb if $journaled = false. This will
##		have no adverse [e|a]ffect on what we are trying to test.
##		On SunOS, a spurious diff (tar : aaaaaa*.* : file
##		changed size) occurs if the db is not locked.
##
##	27-Dec-1999 (vande02)
##		added sleep command before ckpdb_load1 and ckpdb +j
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	24-Nov-2008 (wanfr01)
##	    Performance improvement - use ckpdb +w rather than an 
##	    arbitrary sleep.
##       4-May-2010 (hanal04) SIR 123608
##          ingres.sh now starts an sql session not a quel session. Update
##          this script to call quel directly.
#
# Get commandline args 
#
: ${1?} ${2?} ${3?}

dbname=$1

case $2 in

	journaled) journaled="TRUE" 
	  	   ;;

	notjournaled) journaled="FALSE" 
		      ;;

        *) echo "Second argument must be \"journaled\" or \"notjournaled\""
           exit
esac

# $df is used to build distinct filenames, so this script can be run
# by more than one concurrent process. Each SEP script that calls the
# "ckpdbtst" command should pass a unique string as the 3rd arg.

df=$3

#
# Set up run variables
#
abort_loops=25
delapp_loops=600
append_loops=300
test_success="TRUE"
ckploc=`ingprsym II_CHECKPOINT`
cpdbmask=$ING_TST/be/lar/data/ckpdbmasks

#
# Number of seconds to delay while doing checkpoint.  This stalls the
# checkpoint to allow the system to do work which will need to be copied
# to the DUMP directory.
# THIS IS NOT CURRENTLY USED BY CHECKPOINT - when the stall flag is passed
# to checkpoint it always stalls for 2 minutes, this cannot currently be
# changed other than by recompiling the checkpoint code.
#
    ckpdb_stall=120

#
# Turn off Journaling
# 
if [ $journaled = "FALSE" ]
then
    echo "Turning off journaling on $dbname"
    ckpdb -l -j $dbname | sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
fi
#
# Reload database
# 
    echo "Reloading $dbname database"
    ckpdb1_load $dbname 
#
# Turn on journaling 
#
if [ $journaled = "TRUE" ]
then
    echo "Turning on journaling on $dbname"
    ckpdb +w +j $dbname |sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
fi

#
# Start Tests in background
# 
    echo "Starting Stress Tests"
    (date > CKPDB${df}_LCK1 ; ckpdb_abort $dbname $abort_loops   |sed -f $cpdbmask >ckpdb_ab${df}.out1 2>&1 ; rm -f CKPDB${df}_LCK1) &
    (date > CKPDB${df}_LCK2 ; ckpdb_append $dbname $append_loops |sed -f $cpdbmask >ckpdb_ap${df}.out1 2>&1 ; rm -f CKPDB${df}_LCK2) &
    (date > CKPDB${df}_LCK3 ; ckpdb_delapp $dbname $delapp_loops |sed -f $cpdbmask >ckpdb_da${df}.out1 2>&1 ; rm -f CKPDB${df}_LCK3) &
# 
# Wait for tests to get going - instead of building in handshaking between
# driver and test scripts, we just wait for a few seconds.
# 
    echo "Waiting to start Checkpoint"
# 
# Start Checkpoint
# Pass checkpoint flag to slow down the checkpoint to get better testing.
# 
if [ $journaled = "TRUE" ]
then
    echo "Starting Online Checkpoint of $dbname"
    ckpdb +w -d '#w' $dbname |sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
else
    echo "Starting Online Checkpoint of $dbname"
    ckpdb +w '#w' $dbname |sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
fi
# 
# Wait for subprocesses to complete
# 
    echo "Waiting for Stress Tests to complete"
    while ( [ -f CKPDB${df}_LCK1 -o -f CKPDB${df}_LCK2 -o -f CKPDB${df}_LCK3 ] )
    do
      sleep 15
    done
# 
# Copy result tables out.
# 
    echo "Checking results of stress tests"
    quel -s $dbname <ckp${df}qry1 |sed -f $cpdbmask >$dbname.out1
    quel -s $dbname <ckp${df}qry2 |sed -f $cpdbmask >$dbname.out2
    quel -s $dbname <ckp${df}qry3 |sed -f $cpdbmask >$dbname.out3
    quel -s $dbname <ckp${df}qry4 |sed -f $cpdbmask >$dbname.out4
    quel -s $dbname <ckp${df}qry5 |sed -f $cpdbmask >$dbname.out5
    quel -s $dbname <ckp${df}qry6 |sed -f $cpdbmask >$dbname.out6

if [ $journaled = "TRUE" ]
then
#
# Roll back database to consistency point and check data for consistency.
#
    echo "Restoring database $dbname to checkpoint"
    rollforwarddb -j $dbname |sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
#
    echo "Checking results of checkpoint"
    if ( ckpdb1_check $dbname ) then
        echo " "
        echo "Online Checkpoint Test - Phase 2 Part 1 Successful"
        echo " "
    else
        echo " "
        echo "Online Checkpoint Test Check Failure"
        echo "Phase2 Part 1, rollforward journaled DB back to CP"
        echo "Online Checkpoint Test - Phase 2 Part 1 Failed"
        echo " "
    fi

else
#
# Roll back database to consistency point and check data.
# Try both with '-j' flag and without.  Both should actually be the
# same since the db is not journaled.
#
    echo "Restoring database $dbname to checkpoint"
    rollforwarddb -j $dbname    |sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
    rollforwarddb -c +j $dbname |sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
    rollforwarddb $dbname       |sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
#
    echo "Checking results of checkpoint"
    if ( ckpdb1_check $dbname ) then
        echo "Online Checkpoint Test - Phase 1 Successfull"
    else
        echo " "
        echo "Online Checkpoint Test Check Failure"
        echo "Phase1, rollforward non-journaled DB"
        echo "Online Checkpoint Test - Phase 1 Failed"
        echo " "
    fi
fi

if [ $journaled = "TRUE" ]
then
#
# Roll forward and check again that the data is correct.
#
    echo "Restoring database $dbname using journals"
    rollforwarddb $dbname |sed -e "s#$ckploc#CKPDBLOC#g" -f $cpdbmask
#
# Copy result tables out.
#
    echo "Checking results of rollforwarddb"
    quel -s $dbname <ckp${df}qry1 |sed -f $cpdbmask >$dbname.out7
    quel -s $dbname <ckp${df}qry2 |sed -f $cpdbmask >$dbname.out8
    quel -s $dbname <ckp${df}qry3 |sed -f $cpdbmask >$dbname.out9
    quel -s $dbname <ckp${df}qry4 |sed -f $cpdbmask >$dbname.out10
    quel -s $dbname <ckp${df}qry5 |sed -f $cpdbmask >$dbname.out11
    quel -s $dbname <ckp${df}qry6 |sed -f $cpdbmask >$dbname.out12
#
# Clean up space used up by test.
#
#    ckpdb -l -d $dbname

fi

exit
