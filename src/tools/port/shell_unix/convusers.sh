:
#
# Copyright (c) 2004 Ingres Corporation
#
# Convert a specified 6.4-style users file to 6.5 sql CREATE USER statements
# to feed to iidbdb via 'sql -s...'
#
# Output is to "users.sql" in the local directory.
#
## History
##	28-apr-1993 (dianeh)
##		Created.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

CMD=`basename $0`

usage=' -f <users_file>'

[ $# -gt 1 ] ||
{ echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }

while [ $# -gt 0 ]
do
  case $1 in
    -f) [ $# -gt 1 ] ||
        { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
        users_file="$2"
        shift ; shift
        ;;
    -*) echo "$CMD: Illegal flag: $1"
        echo "Usage:" ; echo "$CMD $usage" ; exit 1
        ;;
     *) echo "$CMD: Illegal argument: $1"
        echo "Usage:" ; echo "$CMD $usage" ; exit 1
        ;;
  esac
done

[ "$users_file" ] ||
{ echo "$CMD: No users file specified." ; echo "Exiting..." ; exit 1 ; }
[ -r "$users_file" ] ||
{ echo "$CMD: Cannot read $users_file." ; echo "Exiting..." ; exit 1 ; }

# Substitution table:
# 0000001 CREATEDB
# 0000004 MAINTAIN_LOCATIONS
# 0000020 TRACE
# 0100000 OPERATOR
##NOTE: Add SECURITY as well to 100025 users

echo "set autocommit on\\g" > users.sql
sed -e "s:^:CREATE USER :" \
    -e "s:!0!0![0]*1$: WITH PRIVILEGES = (CREATEDB)\\\p\\\g:" \
    -e "s:!0!0![0]*4$: WITH PRIVILEGES = (MAINTAIN_LOCATIONS)\\\p\\\g:" \
    -e "s:!0!0![0]*5$: WITH PRIVILEGES = (CREATEDB,MAINTAIN_LOCATIONS)\\\p\\\g:" \
    -e "s:!0!0![0]*20$: WITH PRIVILEGES = (TRACE)\\\p\\\g:" \
    -e "s:!0!0![0]*21$: WITH PRIVILEGES = (CREATEDB,TRACE)\\\p\\\g:" \
    -e "s:!0!0![0]*24$: WITH PRIVILEGES = (MAINTAIN_LOCATIONS,TRACE)\\\p\\\g:" \
    -e "s:!0!0![0]*25$: WITH PRIVILEGES = (CREATEDB,MAINTAIN_LOCATIONS,TRACE)\\\p\\\g:" \
    -e "s:!0!0![0]*100001$: WITH PRIVILEGES = (CREATEDB,OPERATOR)\\\p\\\g:" \
    -e "s:!0!0![0]*100004$: WITH PRIVILEGES = (MAINTAIN_LOCATIONS,OPERATOR)\\\p\\\g:" \
    -e "s:!0!0![0]*100005$: WITH PRIVILEGES = (CREATEDB,MAINTAIN_LOCATIONS,OPERATOR)\\\p\\\g:" \
    -e "s:!0!0![0]*100020$: WITH PRIVILEGES = (TRACE,OPERATOR)\\\p\\\g:" \
    -e "s:!0!0![0]*100021$: WITH PRIVILEGES = (CREATEDB,TRACE,OPERATOR)\\\p\\\g:" \
    -e "s:!0!0![0]*100024$: WITH PRIVILEGES = (MAINTAIN_LOCATIONS,TRACE,OPERATOR)\\\p\\\g:" \
    -e "s:!0!0![0]*100025$: WITH PRIVILEGES = (CREATEDB,MAINTAIN_LOCATIONS,SECURITY,TRACE,OPERATOR)\\\p\\\g:" $users_file >> users.sql

exit 0
