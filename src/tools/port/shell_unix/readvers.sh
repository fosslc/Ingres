:
#
# Copyright (c) 2004 Ingres Corporation
#
# readvers.sh: reads the VERS file and set some variables
#
#   VERS is of the following (relatively free) format:
#
#       # comment line
#       #set: config <config>
#       #set: config32 <32 bit port config>
#       #set: config64 <64 bit port config>
#	#set: arch <bits>
#       #set: build <num>
#	#set: param <param> <num>
#       #set: option <opt>
#	#set: inc <build increment>
#	jamvariable = jamvalue ;
#
#   and as a convenience combination:
#
#	jamvariable = jamvalue ; #set: <one of the above>
#
#
#   The VERS file needs to be able to control jam decisions, and it
#   also needs to be able to set shell variables (via this readvers
#   shellscript).  VERS gets included early in the jam process, and
#   jam doesn't have any reasonable way to preprocess its rules file
#   input first.  So, VERS has to look like valid jam input.
#   VERS also has to be intelligible to readvers in order to define
#   shell variables (and indirectly, compilation level #define macros).
#   The simplest way to meet both needs is to make all of the traditional
#   VERS option stuff look like Jam comments.  The tag "#set:" thus
#   operates as both a jam comment and as a tag for readvers parsing.
#
#   config, config32, and config64 set the platform config string, and
#   they simply generate shell variables of the same name.
#   config should be used ONLY for non-hybrid-capable platforms.
#
#   config32 and config64 should (both!) be defined for hybrid-capable
#   platforms.  The arch option defines whether a hybrid is actually
#   built:
#   <bits> is 32, 64, 32+64 (32-bit with lp64 add-on), or 64+32
#   (64-bit with lp32 add-on).
#
#   <opt> is a configuration keyword, such as W4GL, DBL, B64, etc.  There
#   may be multiple option lines.
#   <param> is an option with a value (rather then just having existence).
#   build <num> is the build level number.
#   <build increment> is the minor build version for any on build number.
#   i.e. if the build is r89e then the build increment is e.
#
#   Use with
#
#	. readvers
#
#   and then the following variables will be set:
#
#       $config         value of config line
#			For a hybrid, config is set to config32 or config64
#			depending on the first (or only) bit-ness specified
#			by the arch line.
#       $config32       value of config32 line
#       $config64       value of config64 line
#	$build_arch	value of arch line if any
#       $build          value of build line
#       $optdef         -Dconf_<opt> ... (for all options)
#       $conf_<opt>     set to "true" for each option
#	$conf_<param>	set to param's value for each param
#	$opts		list of option names
#	$conf_params	list of param names
#
#   Example:
#
#	VERS contains:		readvers.sh sets:
#	-------------		----------------
#	config32 su4_us5	config32=su4_us5
#				if arch does not include 64, also sets
#				config=su4_us5
#	config64 su9_us5	config64=su9_us5
#				if arch includes 64, also sets config=su9_us5
#	arch 32+64		build_arch='32+64'
#	build 02		build=02
#	option W4GL		optdef=" -Dconf_W4GL -Dconf_DBL"
#	option DBL		conf_W4GL=true
#				conf_DBL=true
#
#	option B64		conf_B64=true
#	option NO_SHL		conf_NO_SHL=true
#		If this option is present, the tools will not build FE
#		shared libraries.
#	option SVR_SHL		conf_SVR_SHL=true
#		If present Ingres server (iimerge) will be built from 
#		shared libraries and not from just a iimerge.o containing
#		all the libraries
#	option INGRESII		conf_INGRESII=true
#	param MAX_COLS 4096	conf_MAX_COLS=4096
#
##   History:
##	28-Sep-92 (seiwald)
##		Written.
##	15-oct-92 (lauraw)
##		Changed "patch" to "build" (as per SMUG).
##		Added exit for bad VERS file.	 
##		Added port$noise (i.e., $ING_VERS) support.
##		Added support for MPE build environment use of $CL.
##	10-nov-92 (lauraw)
##		Default "build" changed to 00. (01 would be PUR1.)
##	15-mar-1993 (dianeh)
##		Changed first line to a colon (":").
##	13-jul-1993 (lauraw)
##		VERS may now live in $ING_SRC, too (as per SMUG).
##	13-jul-93 (vijay)
##		Add comment about NO_SHL.
##	23-jul-93 (gillnh2o)
##		Integrating Jab's changes made in ingres63p 'readvers'.... 
##		We were collecting "opts" but never outputting it, and
##		there's a case for setting it in the output. Added that
##		print statement in END.
##		We were setting $noise as a shell variable that was left in
##		the "environment" (for lack of a better term) and thus anything
##		source'ing this would have $noise set. ("mkming.sh" broke as a
##		result.) Changing the name to make it a little more local...
##	04-nov-1997 (hanch04)
##	    Added comment for B64 Large File Support option
##	09-jun-1998 (walro03)
##		Corrected comments from previous change (INGRESII doc).
##	07-jan-1999 (canor01)
##		Added TNG option.
##	26-apr-2000 (hayke02)
##		Removed TNG (embedded Ingres). This change fixes bug 101325.
##	04-Feb-2001 (hanje04)
##		As part of fix for SIR 103876, added comment about SRV_SHL
##		for building iimerge with shared libraries.
##	22-Feb-2002 (hanje04)
##		If build increment <inc> is defined in VERS file then 
##		display it in version.rel
##	20-aug-2002 (hanch04)
##		Added config64 for 32/64 dual build.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	15-Aug-2006 (kschendel)
##	    Spiff up vers to allow limited build configuration in
##	    the VERS, e.g. number of columns max, etc.
##	19-Oct-2007 (kschendel)
##	    Change VERS format so that jamdefs can read VERS as well.
##	17-Jun-2009 (kschendel) b122138
##	    Add more schmaltz to VERS format: #set: on jam line; arch.
##	    Return config as the primary in a hybrid.
##	24-Aug-2009 (bonro01)
##	    Make the VERS override file optional.

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"

if [ -r $ING_SRC/tools/port$noise/conf/VERS ]
  then VERS=$ING_SRC/tools/port$noise/conf/VERS
elif [ -r $ING_SRC/tools/port$noise/conf/VERS.${config_string} ]
  then VERS=$ING_SRC/tools/port$noise/conf/VERS.${config_string}
elif [ -r $CL/tools/port$noise/conf/VERS ]
  # This is where MPE development keeps their VERS file
  then VERS=$CL/tools/port$noise/conf/VERS
else VERS=$ING_SRC/VERS
fi

build=00	# default if not set in VERS
inc=""		# If not set don't print
unset config	# ensure not set if not in VERS

eval `awk '
	/^[ 	]*$/{next}
	/^[ 	]*#/ && $1 != "#set:" {next}
	/^[^#]*#set: / {
	    setline = substr($0,index($0,"#set: ")+6);
	    split(setline, toks);
	    if (toks[1] == "config") {
		print "config=" toks[2] ;
	    } else if (toks[1] == "config32") {
		print "config32=" toks[2] ;
	    } else if (toks[1] == "config64") {
		print "config64=" toks[2] ;
	    } else if (toks[1] == "arch") {
		print "build_arch=\"" toks[2] "\"" ;
	    } else if (toks[1] == "build") {
		print "build=" toks[2] ;
	    } else if (toks[1] == "inc") {
		print "inc=" toks[2] ;
	    } else if (toks[1] == "option") {
		opts=opts " " toks[2]; print "conf_" toks[2] "=true";
		dopts=dopts " -Dconf_" toks[2];
	    } else if (toks[1] == "param") {
		conf_params=conf_params " " toks[2];
		print "conf_" toks[2] "=" toks[3];
	    }
	}
	END { print "optdef=\"" dopts "\"" 
		print "conf_params=\"" conf_params "\""
		print "opts=\"" opts "\"" }
' $VERS` 

if [ -n "$build_arch" ] ; then
    if [ -n "$config" ] ; then
	echo "arch setting should only be used with hybrid-capable platforms."
	echo "Set config32 and config64, not config."
	unset build_arch
	unset config
    elif [ "$build_arch" != '32' -a "$build_arch" != '64' -a "$build_arch" != '32+64' -a "$build_arch" != '64+32' ] ; then
	echo "Improper arch setting: $build_arch"
	unset build_arch
	unset config
    fi
fi
if [ -z "$config" -a -n "$build_arch" ] ; then
    if [ -z "$config32" -o -z "$config64" ] ; then
	echo 'Hybrid-capable platforms must define both config32 and config64'
    elif expr "$build_arch" : '64.*' >/dev/null 2>/dev/null ; then
	config=$config64
    else
	config=$config32
    fi
fi
: ${config?"$VERS file is missing or uses obsolete format."}
