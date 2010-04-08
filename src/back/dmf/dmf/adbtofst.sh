:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
# Name: adbtofst.sh: pipes auditdb output direcly to fastload.
#
# Description:
# This is the wrapper to enable output from the auditdb 
# to be able to be piped to the fastload utility for 
# copying into ingres tables. 
# this is equivalent to the following command. 
#  auditdb -table=<db table> -file=- <audited dbname> | 
#			fastload <fload dbname> -table=<fst table> -file=-
#  usage : adbtofst [-uusername] <fload_table> <fload_dbname> <db_table> 
#			<audited_database> [other_auditdb_parameters]
#	
#     [-uusername]: optional "-uusername" is specified for user id. 
#		     should be same for both fastload and audited database.
#     <fload_table>: non journalled table into which fastloading.	
#    <fload_dbname>: fastload database name
#        <db_table>: journaled table name on which we are running auditdb.
#  <audited_dbname>: journaled database name
#  [other_auditdb_parameters] : optional other auditdb parameters like
#				-abort_transactions, -internal_data.. etc.
#
## History:
## 	1-may-2000 (gupsh01)
##	   Created.
##	04-may-2000 (hanch04)
##	   Updated to conform with Ingres coding standards.
##	28-jun-2000 (gupsh01)
##	   Modified the syntax for specifying user name in -uusername form. 
##      04-Jan-2000 (hanje04)
##         Removed paren' suffux from call of 'usage' routine. Syntax was
##         incorrect.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB build.

(LSBENV)

# Get the important parameters 
usage()
{
echo adbtofst:: error! incorrect parameters
echo  usage : adbtofst [-uusername] fload_table fload_dbname db_table
echo  			audited_database [other_auditdb_parameters]
echo	[-uusername]: optional "-uusername" is specified for user id. 
echo		       should be same for both fastload and audited database.
echo     fastld_table: non journalled table into which fastloading.
echo     fastld_db: name of the fastload database.
echo     audited_table: journaled table name on which we are running auditdb.
echo     audited_db: journaled database name.
echo     [auditdb parameters]: other auditdb parameters if any.
exit 1
}

 userset=false
 case "$1" in
        -u*) username=`echo $1 | sed -e "s/^-u//"`; userset=true; shift
             ;;
 esac

 if [ "$1" = "" ] || [ "$2" = "" ] || [ "$3" = "" ] || [ "$4" = "" ] ;  then
     usage;
 else 
      fastldtab=$1
      shift
      dbname=$1
      shift
      audittab=$1
      shift
      auditeddb=$1
      shift
 fi
 
 temp="auditdb -table=$audittab -file=- $@"

 if [ "$userset" = "true" ] ; then
 eval $temp -u$username $auditeddb | fastload -u$username $dbname -table=$fastldtab -file=-
 else
 eval $temp $auditeddb | fastload $dbname -table=$fastldtab -file=-
 fi

exit 0
