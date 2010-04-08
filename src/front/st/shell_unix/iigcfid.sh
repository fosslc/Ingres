:
#  Copyright (c) 1992, 2010 Ingres Corporation
#
#  Name: iigcfid -- displays all GCF identifiers for a process type
#
#  Usage:
#	iigcfid process_type	
#
#  Description:
#	A crude cover for iinamu which used to identify and shut down
#	INGRES processes.
#
## History:
##	22-feb-93 (tyler)
##		Created.	
##	08-Feb-96 (rajus01)
##		Added process type "iigcb"(Protocol Bridge).
##	21-Jan-98 (GordonW)
##	        Added history section.
##	        Added albase.
##	25-mar-1998 (canor01)
##		Added future gateways.
##	11-Mar-1999 (peeje01)
##		Add ICE server (icesvr)
##	24-Feb-2000 (rajus01)
##		Add JDBC server (iijdbc)
##	26-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##	18-oct-2002 (mofja02)
##		Added support for db2udb.
##	19-Feb-2003 (wansh01)
##		Add DAS server (iigcd).   
##	11-Jun-2004 (somsa01)
##		Cleaned up code for Open Source.
##	12-Feb-2007 (bonro01)
##		Remove JDBC package.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG1PRFX)gcfid
#
#  DEST = utility
#----------------------------------------------------------------------------

(LSBENV)

# temp files
tmp=/tmp/ii$$
res=/tmp/ir$$

# Make sure iinamu is found
if ( (PROG1PRFX)namu </dev/null >/dev/null 2>&1 ) >/dev/null 2>&1
then
	:
else
	exit 1
fi

# now test if any argument passed in
if [ "$1" = "" ]
then
	exit 2
fi

# OK now switch on incoming type

case "$1" in
"(PROG1PRFX)gcn"|"(PROG3PRFX)GCN") nm="NMSVR" ;;
"(PROG1PRFX)gcc"|"(PROG3PRFX)GCC") nm="COMSVR" ;;
"(PROG1PRFX)gcb"|"(PROG3PRFX)GCB") nm="BRIDGE" ;;
"(PROG1PRFX)gcd"|"(PROG3PRFX)gcd") nm="DASVR" ;;
"iidbms"|"IIDBMS") nm="INGRES" ;;
"iistar"|"IISTAR"|"star"|"STAR") nm="STAR" ;;
"informix"|"INFORMIX") nm="INFORMIX" ;;
"oracle"|"ORACLE") nm="ORACLE" ;;
"sybase"|"SYBASE") nm="SYBASE" ;;
"albase"|"ALBASE") nm="ALB" ;;
"odbc"|"ODBC") nm="ODBC" ;;
"informix"|"INFORMIX") nm="INFORMIX" ;;
"mssql"|"MSSQL") nm="MSSQL" ;;
"rms"|"RMS") nm="RMS" ;;
"rdb"|"RDB") nm="RDB" ;;
"db2"|"DB2") nm="DB2" ;;
"db2udb"|"DB2UDB") nm="DB2UDB" ;;
"dcom"|"DCOM") nm="DCOM" ;;
"idms"|"IDMS") nm="IDMS" ;;
"ims"|"IMS") nm="IMS" ;;
"vsam"|"VSAM") nm="VSAM" ;;
"icesvr"|"ICESVR") nm="ICESVR" ;;
*) exit 1 ;;
esac

echo show $nm | (PROG1PRFX)namu | sed -n '
	1d
	s/^IINAMU> //
	s/^'$nm'[ 	]*[^ 	]*[ 	]*\([^ 	]*\).*/\1/p
'

