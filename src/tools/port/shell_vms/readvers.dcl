$! Copyright (c) 2004 Ingres Corporation
$!
$!
$! readvers.dcl: reads the VERS file and set some variables
$!
$!   VERS is of the following (relatively free) format:
$!
$!	# comment line
$!	#set: config <config>
$!	#set: config64 <64 bit port config>
$!	#set: build <num>
$!	#set: param <param> <num>
$!	#set: option <opt>
$!	#set: inc <build increment>
$!	jamvariable = jamvalue ;
$!
$!   Because jam doesn't have any reasonable way to preprocess its rules
$!   file input before including it, it's simplest to make all the
$!   other config-ly stuff in VERS look like Jam comments.  Hence the
$!   string "#set:" which precedes all of the non-jam VERS lines.
$!
$!!  Beware!  VMS VERS files DO NOT support all the variations that
$!!  Unix VERS files do.  In particular, VMS readvers DOES NOT support:
$!!	jamstatement ; #set: vers-thing  (ie all on one line)
$!!	#set: param nnn
$!!	#set: arch bits
$!
$!   <config> is the familiar config string. <num> is the build level.
$!   <config64> is the familiar config string for the 64 bit port.
$!	If a port is a hybrid, config64 is the 64 bit port regardless
$!	of whether 64 bits is the primary or the hybrid.
$!	If a platform is 64 bits without hybrid, use config, not config64.
$!   <opt> is a configuration keyword, such as W4GL, DBL, B64, etc.  There
$!   may be multiple option lines.
$!   <param> is an option with a value (rather then just having existence).
$!   <build increment> is the minor build version for any on build number.
$!   i.e. if the build is r89e then the build increment is e.
$!
$!   VMS NOTES: On VMS, readvers should be invoked as @ING_TOOLS:[bin]readvers
$!	 It sets up global symbols in the form IIVERS_xxx.
$!
$!   and then the following variables will be set:
$!
$!       $config         value of config line
$!       $config64       value of config64 line
$!       $build          value of build line
$!       $optdef         -Dconf_<opt> ... (for all options)
$!       $conf_<opt>     set to "true" for each option
$!
$!   Example:
$!
$!	VERS contains:		readvers.sh sets:
$!	-------------		----------------
$!	config=su4_us5		config=su4_us5
$!	config=su9_us5		config64=su9_us5
$!	build=02		build=02
$!	option=W4GL		optdef=" -Dconf_W4GL -Dconf_DBL"
$!	option=DBL		conf_W4GL=true
$!				conf_DBL=true
$!	option=B64		conf_B64=true
$!	option=NO_SHL		conf_NO_SHL=true
$!		If this option is present, the tools will not build FE
$!		shared libraries.
$!	option=SVR_SHL		conf_SVR_SHL=true
$!		If present Ingres server (iimerge) will be built from 
$!		shared libraries and not from just a iimerge.o containing
$!		all the libraries
$!	option=INGRESII		conf_INGRESII=true
$!
$!!  History:
$!!	26-aug-2004 (abbjo03)
$!!	    Created from readvers.sh.
$!!	19-Oct-2007 (kschendel)
$!!	    Adapt to changed format VERS: old-style lines start with #set:
$!!	    so that jam can read VERS as well for variable defns.
$!!	    SIR 122138 note Jun 2009:  The VMS readvers doesn't need to
$!!	    deal with hybrid builds, or many of the other goofy things
$!!	    that Unix has to contend with.  Therefore, don't make VMS readvers
$!!	    understand #set: arch, nor param, nor #set: on jam lines.
$!!	    Someone with better DCL skills (and a machine to test on)
$!!	    can take the next step.
$!!	24-Aug-2009 (bonro01)
$!!	    Make the VERS override file optional.
$!!
$ echo := write sys$output
$ if f$trnl("ING_VERS") .nes. "" then noise = ".''f$trnl("ING_VERS").src"
$ ing_src = f$trnl("ING_SRC") - ".]"
$ if f$sear("''ing_src.tools.port''noise.conf]VERS.") .nes. ""
$ then VERS = "''ing_src.tools.port''noise.conf]VERS."
$ else
$    VERS = "''ing_src.tools.port''noise.conf]VERS.''config_string'" 
$    if f$sear("''VERS'") .eqs. ""
$    then
$       VERS = "''ing_src']VERS."
$    endif
$ endif
$ IIVERS_build="00"	! default if not set in VERS
$ IIVERS_inc=""		! If not set don't print
$ validopts="/config/config64/build/option/inc/"
$ l=f$len(validopts)
$ open /read /err=errend2 versfile 'VERS'
$readloop:
$	read /end=okend /err=errend versfile line
$	if f$extr(0, 5, line) .nes. "#set:" then goto readloop
$	line = f$extr(6, f$len(line), line)
$	line = f$edit(line, "TRIM")
$	symb = f$extr(0, f$loc(" ", line), line)
$	val = f$extr(f$loc(" ", line) + 1, f$len(line), line)
$	symb = f$edit(symb, "TRIM")
$	val = f$edit(val, "TRIM")
$	if f$loc("/''symb'/", validopts) .eq. l then goto readloop
$!	At some point we'll want to support param name parmval as well.
$	if symb .eqs. "option"
$	then	opts="''opts' "+val
$		IIVERS_conf_'val'=="true"
$		if "''dopts'" .nes. ""
$		then	dopts=dopts+",conf_"+val
$		else	dopts="conf_"+val
$		endif
$	else	IIVERS_'symb'=="''val'"
$	endif
$	goto readloop
$okend:
$ close versfile
$ if "''IIVERS_config'" .eqs. "" then goto errend2
$ IIVERS_optdef=="/DEF=(''dopts')"
$ IIVERS_opts=="''opts'"
$ exit
$errend:
$ close versfile
$errend2:
$ echo "config: ''VERS' file is missing or uses obsolete format."
$ exit %x10000012
