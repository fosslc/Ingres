:
#
# Copyright (c) 2004 Ingres Corporation
#
# This script pipes input to NETU to show, add, or delete 
# user or node authorizations. 
#
# 	$1 = s(how), a(dd) or d(elete)
# 	$2 = p(rivate) or g(lobal)
# 	$3 = n(node) or u(user)
#
# If $1 is 'a' (add) or 'd' (delete), the next arg is required:
#
# 	$4 = v-node name
#
# When $3 is 'n' (node), these args also apply:
#
#	$5 = software type  (defaults to 'tcp_ip')
#	$6 = node name      (defaults to v-node name)
#	$7 = listen address (defaults to II)
#
# When $3 is 'u' (user), these args also apply:
#
#	$5 = user name (required unless $1 is 's' (show) )
#	$6 = user pw   (defaults to user name)
#	
#
# Examples:
#
#   To show global user entries:
#
#	authnetu s g u
#
#   To add v-node 'hydra' with defaults for software, node, and listen
#   address:
# 
#	authnetu a g n hydra
#
#   To add v-node 'hydraXX' for node 'hydra' with listen address 'XX':
#
#	authnetu a g n hydraXX tcp_ip hydra XX
#
#	
## History:
##
##	19-dec-1991 (lauraw)
##		Created.
##	11-feb-1992 (lauraw)
##		Added vnode arg so you can have vnode names that are different
##		the node names. Switched order of args.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##

netu_escape=""

junk_out="/tmp/junk$$.out"

case $1 in

   [ads]) action=$1 ;;

   *) echo "First arg must be 'a' (add), 'd' (delete), or 's' (show)"
      exit
      ;;
esac

case $2 in

   [gp]) auth_type=$2 ;;
   *) echo "Second arg must be 'g' or 'p' (global or private)"
      exit ;;
esac

if [ $action = "s" ]
then
   vnode=${4-"*"}
else
   : ${4?"must be set to vnode name"}
   vnode=$4
fi

case $3 in
   n) what='n' 

      if [ $action = "a" ]
      then
      	software=${5-"tcp_ip"}
	node=${6-$vnode}
      	listen=${7-"II"}
      else
      	software=${5-"*"}
	node=${6-$vnode}
      	listen=${7-"*"}
      fi
      ;;

   u) what='u' 

      if [ $action = "s" ]
      then
	user=${5-"*"}
      else
      	: ${5?"5th arg must be username"}
      	user=$5
      fi

      if [ $action = "a" ]
      then
      	: ${6?"6th arg must be password"}
      	pw=$6
      fi
      ;;

   *) echo "Third arg must be 'n' (node) or 'u' (user)"
      exit
      ;;
esac
   
case $action$what in

   su)  netu >$junk_out <<EOF_sa
a
s
$auth_type
*
*
$netu_escape
e
EOF_sa

	echo " "
	sed -e '/V_NODE/,/^$/p' -n $junk_out

	;;

   au) 	echo "Using NETU to add $auth_type user $user to v-node $vnode"

	netu >$junk_out <<EOF_aa
a
a
$auth_type
$vnode
$user
$pw
$pw
$netu_escape
EOF_aa

	grep "Server Registry" $junk_out

	;;
 
   du) 	echo "Using NETU to delete $auth_type user $user from v-node $vnode"

        netu >$junk_out <<EOF_da
a
d
$auth_type
$vnode
$user
$netu_escape
EOF_da

	grep "Server Registry" $junk_out

        ;;

   an)	echo "Using NETU to add vnode $vnode for node $node (with software $software and listen address $listen)"

	netu >$junk_out <<EOF_an
n
a
$auth_type
$vnode
$software
$node
$listen
EOF_an

        grep "Server Registry" $junk_out

	;;

   sn)	
	netu >$junk_out <<EOF_sn
n
s
$auth_type
$vnode
$software
$node
$listen
EOF_sn

        echo " "
        sed -e '/V_NODE/,/^$/p' -n $junk_out


	;;

   dn)	echo "Using NETU to delete vnode $vnode (node $node)"

	netu >$junk_out <<EOF_dn
n
d
$auth_type
$vnode
$software
$node
$listen
EOF_dn

        grep "Server Registry" $junk_out

	;;


   esac

rm -f $junk_out
