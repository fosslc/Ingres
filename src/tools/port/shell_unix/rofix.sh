:
#
# Copyright (c) 2004 Ingres Corporation
#
# rofix - modify assembly code to put constant data into text segment
#
# History:
#	31-jul-1989 (boba)
#		Change conf to tools/port$noise/conf.
#	03-oct-89 (rexl)
#		/bin/ed will not function with the cmd <<! syntax.
#		Instead cat commands to a file, cat file | ed $i
#	04-jan-1990 (boba)
#		Fix backwards comment at top of file.
#	18-aug-1991 (ketan)
#		Added entry for nc5_us5
#       01-Nov-1991 (dchan)
#		Ds3_ulx now has it's very own entry because
#  		the decstation compiler doesn't generate the correct
#		assembler code.  We have to append an "extra" blank
#		line to the resultant file so that it will correctly
#		assemble.
#	16-oct-1992 (lauraw)
#		Uses readvers to get config string.
#	24-dec-93 (swm)
#		Bug #58888
#		On axp_osf there is an rdata section within text, weird
#		side effects occur if we try to use the text directive
#		rather than the rdata directive.
#	17-jun-94 (swm)
#		On axp_osf roc code does not always get assembled correctly
#		causing the dbms server to SIGSEGV during a copy from table.
#		This problem is *not* a consequence of the rofix process itself
#		but of incorrect assembly of the .s file produced by the C
#		compiler. The problem with the assembly is unclear but it
#		goes away when the ".set noreorder" directive is specified, so
#		add the directive after each ".text" directive.
#	31-jul-1997 (walro03)
#		Tandem NonStop (ts2_us5) assembly errors on .roc files caused
#		by .text.  Changed to .rdata.
#	10-may-1999 (walro03)
#		Remove obsolete version string nc5_us5, pyr_u42, 3bx_us5.
#	12-Sep-2000 (hanje04)
#		Alpha Linux (axp_lnx) assembler needs output as generated
#		by axp_osf case. Added |axp_lnx to axp_osf.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

. readvers
vers=$config
for i in $*
do
	case $vers in

		elx_us5)
ed $i <<!
g/^[ 	]*\.var/s// .inst/
w
q
!
		;;

		sun_u42)
ed $i <<!
g/^[ 	]*\.data$/s// .text/
g/^[ 	]*\.zero[ 	]*\([0-9]*\)/s// .set	.,.+\1/
w
q
!
		;;

		ds3_ulx)
echo " " >> $i
ed $i <<!
g/^\(.*:\)\.data/s//\1.text/
g/^[ 	]*\.data/s// .text/
g/^[ 	]*\.zero[ 	]*\([0-9]*\)/s// .set	.,.+\1/
w
q
!
		;;
		axp_osf|axp_lnx)
ed $i <<!
g/^\(.*:\)\.data/s//\1.rdata/
g/^\([ 	]*\)\.data/s//\1.rdata/
g/\.rdata/s//.data/
g/\.text.*/s//&\\
	.set	noreorder/
w
q
!
		;;
		ts2_us5)
ed $i <<!
g/^\(.*:\)\.data/s//\1.rdata/
g/^[ 	]*\.data/s// .rdata/
g/^[ 	]*\.zero[ 	]*\([0-9]*\)/s// .set	.,.+\1/
w
q
!
		;;
		*)
ed $i <<!
g/^\(.*:\)\.data/s//\1.text/
g/^[ 	]*\.data/s// .text/
g/^[ 	]*\.zero[ 	]*\([0-9]*\)/s// .set	.,.+\1/
w
q
!
		;;
	esac
done
