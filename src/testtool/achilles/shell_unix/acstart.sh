:
#
# Copyright (c) 2004 Ingres Corporation
#
###############################################################################
# acstart.sh    Driver shell for the achilles tool.
#
## History:
##
##	26-sep-91 (DonJ)
##		Created.
##	03-oct-91 (DonJ)
##		Cleanup and improve error reporting using ingres/utility tools
##		iisysdep, echolog and echon.
##	11-dec-91 (DonJ)
##		Fixed AWK so it handles the first field in the config file
##		correctly. The first field is delimited by a semi-colon, not a
##		whitespace. Also allowed AWK to do the template file in the
##		same way. See Bug # 41392.
##	24-Jun-93 (GordonW)
##		6.5 changes-
##			echon -> iiechonn
##			echolog -> iiecholg
##			check_db = false
##
##      7-Aug-93 (Jeffr)
##		Added Changes to support Achilles Version 2 -  
##		  support for acreport by putting pid in report file
##		  support for -a (time to run feature)
##		  	
##	3-Dec-93 (jeffr)
##		Added 6.5 specific changes - removed reference to netutil
##		since its gone 
##		added user message for ACreport
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##
###############################################################################

[ $SHELL_DEBUG ] && set -x
 
check_db=false
CMD=acstart
export CMD

if [ $# -eq 0 ]
then
     cat << !

Usage: $CMD <test_case> <data_base_spec>

        -o[utput] path    Change output directory where all achilles
                          files are written.

        -r[epeat] [n]     Change repeat tests parameter in config or
                          template file. Defaults to 0. A value of
                          zero means to repeat the test endlessly.

        -u[sername] user  Change effective user parameter in config
                          or template file.

        -d[ebug]          Do all sanity checking and file creation
                          without actually starting up achilles.

        -i[nteractive]    Start up achilles in this process rather
                          than as a detached process.

	-a[actual]        Actual time for achilles to run if
                          desired (note: overrides -r[epeat] option)


!
    exit 0
fi

###############################################################################

debug_sw=false;      interactive_sw=false;   repeat_sw=false;   repeat_val=9999
output_sw=false;     output_val="";          user_sw=true;      user_val=`whoami`
test_case="";        cfg_file="";            dbspec="";         get_out=false

need_repeat=false;   need_output=false;      need_user=false;   need_cfg=true
need_db=true ; need_time=false; time_sw=false; time_val=9999

###############################################################################
#
# Check local Ingres environment
#
###############################################################################

if [ ! -n "${II_SYSTEM}" ]
then
     cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|     The local Ingres environment must be setup and the     |
|     the installation running before continuing             |
|------------------------------------------------------------|

!
   exit 1
fi

. $II_SYSTEM/ingres/utility/iisysdep
# PATH=$II_SYSTEM/ingres/utility:$PATH
# export PATH
###############################################################################

need_appended=true
tmp=`echo $PATH | sed -e 's/:/ /g'`
for i in $tmp
do
        if [ $i = "${II_SYSTEM}/ingres/utility" ]
           then
                need_appended=false
        fi
done

if $need_appended
   then
        PATH=$II_SYSTEM/ingres/utility:$PATH
        export PATH
fi



###############################################################################
#
#	Break out command line arguments.
#
###############################################################################

for i
do
    case $i in
	-* )
		if $need_output
		   then
			cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|                   Bad -o (output) flag                     |
|------------------------------------------------------------|

!
			exit 1
		fi
		if $need_user
		   then
                        cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|                  bad -u (username) flag                    |
|------------------------------------------------------------|

!
                        exit 1
		fi
		$need_repeat && repeat_val=9999 && repeat_sw=true && need_repeat=false
		$need_time && time_val=9999 && time_sw=true && need_time=false
		;;

	*.* | * )
		if $need_time
		  then
		time_val="$i"; time_sw=true; need_time=false
               
		elif $need_repeat	
		   then
			repeat_val="$i"; repeat_sw=true; need_repeat=false
		elif $need_output
		   then
			output_val="$i"; output_sw=true; need_output=false
		elif $need_user
		   then
			user_val="$i";   user_sw=true;   need_user=false
		else
			if $need_cfg
			   then
				need_cfg=false; test_case="$i"
			elif $need_db
			   then
				need_db=false;  dbspec="$i"
			fi
		fi
		;;

    esac
    case $i in

	-actual | -actua | -actu | -act | -ac | -a )
               need_time=true
               ;;

	-actual=* | -actua=* | -actu=* | -act=* | -ac=* | -a=* )
               time_sw = true  ;       time_val=`echo $i | sed -e 's/-actual=//
' -e 's/-actua=//' -e 's/-actu=//' -e 's/-act//' -e 's/-ac=//' -e 's/-a=//'`
               ;;

	-debug | -debu | -deb | -de | -d )			
		debug_sw=true
		;;

	-interactive | -interactiv | -interacti | -interact | -interac | -intera | -inter | -inte | -int | -in | -i )
		interactive_sw=true
		;;

	-repeat | -repea | -repe | -rep | -re | -r )
		need_repeat=true
		;;

	-repeat=* | -repea=* | -repe=* | -rep=* | -re=* | -r=* )
		repeat_sw=true
		repeat_val=`echo $i | sed -e 's/-repeat=//' -e 's/-repea=//' -e 's/-repe=//' -e 's/-rep=//' -e 's/-re=//' -e 's/-r=//'`
		;;

	-output | -outpu | -outp | -out | -ou | -o )
		need_output=true
		;;

	-output=* | -outpu=* | -outp=* | -out=* | -ou=* | -o=* )
		output_sw=true
		output_val=`echo $i | sed -e 's/-output=//' -e 's/-outpu=//' -e 's/-outp=//' -e 's/-out=//' -e 's/-ou=//' -e 's/-o=//'`
		;;

	-username | -usernam | -userna | -usern | -user | -use | -us | -u )
		need_user=true
		;;

	-username=* | -usernam=* | -userna=* | -usern=* | -user=* | -use=* | -us=* | -u=* )
		user_sw=true
		user_val=`echo $i | sed -e 's/-username=//' -e 's/-usernam=//' -e 's/-userna=//' -e 's/-usern=//' -e 's/-user=//' -e 's/-use=//' -e 's/-us=//' -e 's/-u=//'`
		echo "switch $user_sw"
		echo "user val is $user_val"
		;;

	-* )	cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|                      bad flag $i
|------------------------------------------------------------|

!
		exit 1
		;;

	*.* | * ) ;;
    esac
done

###############################################################################

 if $need_time
    then
       cat << !

 |------------------------------------------------------------|
 |                      E  R  R  O  R                         |
 |------------------------------------------------------------|
 |                   Bad -a (actual-time) flag                |
 |------------------------------------------------------------|

!

       exit 1

fi

if $need_output
   then
	cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|                   Bad -o (output) flag                     |
|------------------------------------------------------------|

!
	exit 1
fi
if $need_user
   then
        cat << !
 
|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|                  bad -u (username) flag                    |
|------------------------------------------------------------|
 
!
	exit 1
fi
if $need_cfg
   then
        cat << ! 
 
|------------------------------------------------------------| 
|                      E  R  R  O  R                         | 
|------------------------------------------------------------| 
|                  no test case given                        |
|------------------------------------------------------------|
 
!
	exit 1
fi
if $need_db
   then
        cat << ! 
  
|------------------------------------------------------------|  
|                      E  R  R  O  R                         |  
|------------------------------------------------------------|
|            no data base specification given                |
|------------------------------------------------------------| 
 
!
	exit 1
fi

$need_repeat && repeat_val=9999 && need_repeat=false && repeat_sw=true
$need_time && time_val=9999 && need_time=false && time_sw=true
###############################################################################

test ! -n "${achilles_result}" && ach_result_exists=false || ach_result_exists=true 
test ! -n "${achilles_config}" && ach_config_exists=false || ach_config_exists=true  

if [ ! -f "$test_case" ]
   then
        get_out=true 
        $ach_config_exists && test -f "${achilles_config}/$test_case" && get_out=false && test_case="${achilles_config}/$test_case" 
        if $get_out
	   then
		cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |  
|------------------------------------------------------------|
|                    Can't find $test_case
|------------------------------------------------------------|

!
		exit 1
	fi
fi
if [ ! -r "$test_case" ]
   then
        cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|                 Can't read $test_case
|------------------------------------------------------------| 

!
	exit 1
fi

###############################################################################

test $output_sw = false && $ach_result_exists && output_val="${achilles_result}" && output_sw=true

if $output_sw
   then
	if [ ! -d "$output_val" ]
	   then
		cat << !

|------------------------------------------------------------|
|                   W  A  R  N  I  N  G                      |
|------------------------------------------------------------|
|  output dir <$output_val> does not exist, creating it.
|------------------------------------------------------------|

!
		mkdir $output_val
	   else
		if [ ! -w "$output_val" ]
		   then
			cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|        Can't write to output <$output_val>
|------------------------------------------------------------|

!
			exit 1
		fi
	fi
   else
	pwd0=`pwd`
	cat << !

|------------------------------------------------------------|
|                          W  H  A  T                        |
|------------------------------------------------------------|
!
	iiechonn "| No output path was given, please enter one <$pwd0> "
	read  tmp
	cat << !
|------------------------------------------------------------|

!
	test -z "$tmp" && tmp=`pwd`
	output_val=$tmp
	output_sw=true
fi
test "$output_val" = "./" && output_val=`pwd`

pid=$$
host=`hostname`
fname="$host$pid"
fn_root="$output_val/$fname"
cfg_file=`echo "$fn_root.config"         | sed -e 's#//#/#'`
log_file=`echo "$output_val/${fname}.log" | sed -e 's#//#/#'`
#log_file=`echo "$output_val/${fname}.log" | sed -e 's#//#/#'`
rpt_file=`echo "$output_val/${fname}.rpt" | sed -e 's#//#/#'`
awk_file=`echo "$output_val/$fname.awk"  | sed -e 's#//#/#'`
awk_fil2=`echo "$output_val/$fname.awk2" | sed -e 's#//#/#'`
awk_fil3=`echo "$output_val/$fname.awk3" | sed -e 's#//#/#'`
awk_fil4=`echo "$output_val/$fname.awk4" | sed -e 's#//#/#'`

childlog="$fname"

###############################################################################
 
need_vnd=true;  need_dbn=true;  need_svr=true
have_vnd=false; have_dbn=false; have_svr=false
tmp=`echo $dbspec | sed -e 's/ //' -e 's/::/ /'`
for i in $tmp
do
        if $need_vnd
           then
                vnode="$i";  need_vnd=false; have_vnd=true
        elif $need_dbn
           then
                dname="$i";  need_dbn=false; have_dbn=true
        fi
done
$need_dbn && dname="$vnode" && have_dbn=true && need_dbn=false && have_vnd=false && need_vnd=true && vnode=""
 
tmp=`echo $dname | sed -e 's#/# #'`; need_dbn=true; have_dbn=false
for i in $tmp
do
        if $need_dbn
           then
                dname="$i";  need_dbn=false; have_dbn=true
        elif $need_svr
           then
                server="$i"; need_svr=false; have_svr=true
        fi
done
if $need_dbn
   then
        cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|      Something's wrong with data base spec=$dbspec
|------------------------------------------------------------|

!
	exit 1
fi
$have_svr && test $server = "ingres" && need_svr=true && have_svr=false && server=""

tmp="$dname"
$have_vnd && tmp="$vnode::$tmp"
$have_svr && tmp="$tmp/$server"
dbspec="$tmp"
 
############################################################################### 

cat << !

|------------------------------------------------------------|
|                P  A  R  A  M  E  T  E  R  S                |
|------------------------------------------------------------|
!
if $time_sw
    then
       cat << !
| time value = $time_val
!
fi
if $debug_sw
   then
	cat << !
| debug is on
!
fi
if $interactive_sw
   then
	cat << !
| interactive is on
!
fi
if $repeat_sw
   then
	cat << !
| repeat value = $repeat_val
!
fi
if $output_sw
   then
	cat << !
| output path  = $output_val
!
fi
if $user_sw
   then
	cat << !
| username     = $user_val
!
fi
cat << !
| test case    = $test_case
| config file  = $cfg_file
| rpt file     = $rpt_file
| log file     = $log_file
| db spec      = $dbspec
|------------------------------------------------------------|

!

###############################################################################
###############################################################################

if $have_vnd
   then
	echo "show comsvr"  | iinamu >tmp.tmp
	tmp=`cat tmp.tmp | egrep 'IINAMU>' | egrep 'COMSVR'`
	rm tmp.tmp
	if [ -z "$tmp" ]
	   then
                cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|                 No comsvrs are running                     |
|------------------------------------------------------------|

!
                exit 1
	fi
elif $need_svr
   then
	echo "show ingres
quit
"       | iinamu >tmp.tmp
        tmp=`cat tmp.tmp | egrep 'IINAMU>' | egrep 'INGRES'`
	rm tmp.tmp
	if [ -z "$tmp" ]
	   then
                cat << !
 
|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|              No dbms servers are running                   |
|------------------------------------------------------------|

!
                exit 1   
	fi
elif [ $server = "star" ]
   then
	echo "show star
quit
"	| iinamu >tmp.tmp
        tmp=`cat tmp.tmp | egrep 'IINAMU>' | egrep 'STAR'`
	rm tmp.tmp
	if [ -z "$tmp" ] 
           then 
                cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|              No star servers are running                   |
|------------------------------------------------------------|

!
                exit 1
        fi
fi

entry_type=""; skip_vn=true;        need_prot=true
need_remnode=true;   need_listen=true

	tmp=`echo $tmp | sed -e 's/	/ /g' -e 's/  / /g'`
	for i in $tmp
	do
		if $skip_vn
		   then
			skip_vn=false
		elif $need_prot
		   then
			net_protocol="$i"
			need_prot=false
		elif $need_remnode
		   then
			rem_node="$i"
			need_remnode=false
		elif $need_listen
		   then
			list_addr="$i"
			need_listen=false
		fi
	done
###############################################################################
# Check that remote db is available
###############################################################################

if $check_db
then
	echo "\q" | sql ${dbspec} 2>&1 >/dev/null
	ret_val=$?
	if [ $ret_val -ne 0 ]
   	then
		cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|     Could not access $dbspec -- tests not submitted
|------------------------------------------------------------|

!
		exit 1
	fi
fi 
###############################################################################
#
#	Build AWK file to translate template file into config file. This is
#	dependent on input switches, repeat_sw and user_sw, that override cer-
#	tain defaults..
#
###############################################################################

case ${test_case} in
        *.config)
		cat >${awk_fil2} << !
                        /^#/ { continue }
                        /^$/ { continue }
                        {
                                n = split( \$0, array1, ";" )
                                printf( "%s;", array1[1] )
                                n = split( array1[2], array2 )
                                for ( i = 1; i <= n; i++ )
                                        \$(i+8) = array2[i]
!
		if $repeat_sw
		   then
			cat >${awk_fil3} << !
				printf( " %s %s %s %s %s", \$9, repeat_tests, \$11, \$12, \$13 )
!
		   else
			cat >${awk_fil3} << !
				printf( " %s %s %s %s %s", \$9, \$10, \$11, \$12, \$13 )
!
		fi
		if $user_sw
		   then
			cat >${awk_fil4} << !
				printf( " %s %s %s %s\n", \$14, \$15, achilles_log, achilles_user )
			}
!
		   else
			cat >${awk_fil4} << !
				printf( " %s %s %s %s\n", \$14, \$15, achilles_log, \$17 )
!
		fi
		cat ${awk_fil2} ${awk_fil3} ${awk_fil4} > ${awk_file}
                rm  ${awk_fil2} ${awk_fil3} ${awk_fil4}
                break
                ;;

        * | *.*)
		cat >${awk_file} << !
		/^#/ { continue }
		/^$/ { continue }
		{
                        is_semi = index(\$0,";")
                        i = 1
                        if (is_semi == 0) {
                                n = split( \$0, array2 )
                                cmdline = array2[i++]
                        }
                        else {   
                                n = split( \$0, array1, ";" )
                                cmdline = array1[1]
                                n = split( array1[2], array2 )
                        }
                        iters    = array2[i++]
                        isleep   = array2[i++]
                        cdsleep  = array2[i++]
                        dcsleep  = array2[i++]
                        nr_ch    = array2[i++]
                        int_cnt  = array2[i++]
                        kill_cnt = array2[i++]
                        int_int  = array2[i++]
                        kill_int = array2[i++]
                        printf( "%s %s %s", cmdline, "<DBNAME>", iters )
                        printf( " %s %s %s %s %s", isleep, "<CHILDNUM>", cdsleep, dcsleep, ";")
                        printf( " %s %s %s %s %s", nr_ch, repeat_tests, int_cnt, kill_cnt, int_int )
                        printf( " %s %s %s %s\n", kill_int, "<NONE>", achilles_log, achilles_user )
		}
!
                break
                ;;   
esac

###############################################################################
#
#	Now use the AWK file to do the translation.
#
###############################################################################

awk -f ${awk_file} \
   achilles_log=${childlog} \
   achilles_user=${user_val} repeat_tests=${repeat_val} \
   ${test_case} > ${cfg_file}

rm ${awk_file}

if $debug_sw
then
	cat << !

|------------------------------------------------------------|
|        Here's the command line we would have used:         |
|------------------------------------------------------------|
  achilles -l -s -d $dbspec 
           -f $cfg_file 
           -o $output_val 
            > $log_file
|------------------------------------------------------------|

!
else
	if $interactive_sw
	then
achilles -l -s -d ${dbspec}  -f ${cfg_file}  -o ${output_val} -a ${time_val} -i
#pass interactive switch to achilles in this mode
	   else

#set default time to 9999 - (i.e infinite)
achilles  -l -s -d ${dbspec} -f ${cfg_file} -o ${output_val} -a ${time_val}  > ${log_file} &

#create report file for the user and let him know where it is
 cat  << ! 

	   please enter the file - "${cfg_file}"
           when generating a Report File with "acreport"

!
 echo "ACHILLES  $!" > ${rpt_file}
	fi
fi
exit 0

