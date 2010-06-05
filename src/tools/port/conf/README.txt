VERS Format:

VERS files contain build-time specific options that can be
adjusted based on site or platform requirements.

The build will look for a file named VERS.  If no such file exists,
it will use a file named VERS.config_string.

The VERS.config_string files are the standard configs distributed
with the Ingres source, and are under source code control.
The VERS file is not distributed and (if it exists) is entirely
under the build site's control.  Ths makes it easy for a build client
to use either standard (no VERS) or custom (site VERS) build options.
Different build clients can easily use different vers options.

VERS files have two sections, one for controlling jam directly
and one for generating shell variables and source-level defines.
Jam doesn't seem to have any good way of preprocessing its
defines/rules files before inclusion, so the jam part looks like
ordinary jam statements.
That implies that the shell/source part has to be comments to jam;
so, a jam comment starting with the exact tag #set: is understood
(by readvers) to be a build option setting that becomes a shell
variable (and perhaps indirectly, a source-level define).  A
#set: option comment can be by itself, or on the same line as
a jam variable setting.

The possibilities for the #set:  lines are:
	#set: config <config>
	#set: config32 <32 bit port config>
	#set: config64 <64 bit port config>
	#set: arch <bits>
	#set: build <num>
	#set: param <param> <value>
	#set: option <opt>
	#set: inc <build increment>

#set: config, config32, and config64 set the platform config string.
config is used for non-hybrid-capable platforms.
config32 and config64 are for hybrid-capable platforms, and BOTH
must always be defined, even if no hybrid add-on is actually
built.  Readvers produces a (non-empty) shell variable named
config (config32, config64) with the given string as its value.
For hybrid-capable platforms, readvers also produces a shell variable
named config with the value of either the 32-bit or 64-bit string,
whichever is the "primary" build architecture.  See arch, next.

#set: arch is mandatory for hybrid capable platforms, and prohibited
for non-hybrid-capables.  The arch option defines whether a
hybrid is actually built, and what kind.
<bits> is 32, 64, 32+64 (32-bit with lp64 add-on), or 64+32
(64-bit with lp32 add-on).  32+64 is a hybrid and 64+32 is a
reverse hybrid.  Readvers produces a shell variable build_arch
with the <bits> string as its value.
Any hybrid-capable platform MUST also define both BUILD_ARCH
and XERCES_ARCH as jam variables.  BUILD_ARCH should get the
same <bits> value as arch.  XERCES_ARCH specifies which build
type (32 or 64) the xerces-c library in $XERCESLOC is.  (with
the assumption being that the library in $XERCESLOC/lp32 or
$XERCESLOC/lp64 or $XERCESLOCHB is the opposite.)

#set: build gives the build number that ends up as part of the
release ID string.

#set: inc is the minor build suffix which ends up in the bldinc
member of the ingres version structure.

#set: option <opt> defines an option.  <opt> is some sort of
configuration keyword.  Readvers defines the (nonempty) shell
variable conf_<opt>, e.g. option DBL produces conf_DBL.

#set: param <param> <value> is almost the same as #set: option,
except that the generated shell variable conf_<param> is given
the value <value> instead of just "true".  The value can be pretty
much anything, although it might be unwise to have string values
with spaces in them;  quotes tend to get lost in the shellscript layers.

The selection of OPTION and PARAM keywords is somewhat platform
dependent.  The meaning of some of the options are lost in the mists
of time.  An incomplete list of options and param keywords is appended.


As for the jam part, one variable is required, PLATFORM_JAMDEFS,
with the value being the suffix for the platform-specific Jamdefs.xxx
file that jam is to include immediately after VERS.
Other variables can be defined depending on platform Jamdef
requirements.  Some generally useful optional variables:
	DBMS_UDT = "iilink UDT string" ;  (iilink override)
	BUILD_ARCH = "bits" ; (for hybrid-capable platforms, see above)
	XERCES_ARCH = "bits" ; (for hybrid-capable platforms)

Many #set: options require corresponding jam variables also.  For
instance, the #set: option SVR_AR (use server iimerge.a archive)
also requires the jam variable SVR_ARLIB to be set to TRUE.

---------

Some of the available option and param keywords are listed.  Unless
noted, all of these are for unix-like platforms ONLY.  A few apply
to VMS.  Windows doesn't seem to deal in options/params at all.

Option keywords (no value):

B64	- generally seems to mean "64-bit I/O offsets allowed".
	If set, B64 turns on any platform-specific "large file support"
	options during compiling and linking.  All VERS files should
	include B64 unless it's entirely irrelevant to the platform.
	(Ideally, B64 should go away, and mkdefault etc should assume it.)

BUILD_ICE - Enables building of the ICE component, which is otherwise
	omitted from the build. The BUILD_ICE jam variable must
	also be set.

CAS_ENABLED - says that the platform has a low level compare-and-swap
	or equivalent that implements the CScas4 and CScasptr routines.
	Without CAS_ENABLED, the CL emulates CSadjust_counter with a
	mutex, which is almost certainly slower.

CLUSTER_BUILD - (VMS also) This option enables compiling of active-active
	cluster stuff.

DBL	- enables double-byte builds.  A double-byte build has the
	capability of running single or double byte character sets,
	while a non-double-byte build cannot run double-byte character
	sets (which includes UTF8).  Most platforms are double-byte
	these days, with the exception of some of the classic unix
	platforms (Sun Sparc, HP PA, IBM AIX).

DELIM_TEST - doesn't seem to be used by anything??

MAINWIN_VDBA - causes MAINWIN and Visual DBA to be included in the release
	packaging.  Since these are not part of the community distribution,
	this option is not set for community builds.

SVR_SHL - calls for server shared libraries.  iimerge will be just main.
SVR_AR  - calls for server objects to be stuff into an iimerge.a archive
	library, with iimerge (statically) linked against iimerge.a.
	Both of these two also require the corresponding jam variable,
	SVR_SHLIBS or SVR_ARLIB respectively.  If neither option is set,
	server objects are linked into one big iimerge.o, which is the
	traditional (old-fashioned) way.

libc6	- A linux option, probably dating from early Linux ports.
	Apparently obsolete.

(Added by Datallegro:)
DATALLEGRO (*) - enable Datallegro site specific features, such as
	defaulting the elapsed-time display in the Terminal Monitor to on.
GWIS_IB	(*) - Compile and build with IngresStream over Infiniband support.
	Specifically, include the "gwis" code in the gateway facility GWF.
	Without this, IngresStream declarations parse but don't execute.
XFS	- compile in XFS reservation code for linux's XFS filesystem.
DISSL_ENABLED (*) - compile in table level encryption code in DI

(*) option also requires a corresponding jam variable.

----------

Param keywords (options taking a value):

ADI_MAX_OPERANDS - (integer) Override the build default for maximum
	number of operands to a function.  If you have UDF's with lots
	of parameters, you'll need this.  (Note that a UDF with more
	then 5 parameters including the result had better be a
	"countvec" style udf, unless you extend the call switch in
	adeexecute.c)

RELID	- (string) Override the release ID string built by genrelid.
	Nothing uses this at present;  it's included in case there is
	some need to apply a special ID string for a particular build.

MAX_COLS - (number) Define the maximum number of columns allowed in a table.
	If not defined, a default in iicommon.h is used.  None of the
	standard VERS files define MAX_COLS, but a site may wish to
	change the maximum number of columns for special builds.
	Increasing MAX_COLS beyond 32K (!) is probably not going to work.

(Datallegro)
LZO	- (directory path) Link with the LZO Professional compression
	library.  The parameter value is the path to the LZO Pro .a
	library (directory path only, without the filename).  Without
	LZO, the page-compression code is there, but it doesn't compress
	anything!  (or, at best, uses a simple/stupid inline compressor.)
	The LZO jam variable must be defined the same way.

-------------------------------------------------------------------

Notes on the hybrid building scheme:

Hybrid capable platforms must define config32, config64, arch, and
the BUILD_ARCH jam variable.  The platform Jamdefs will define
a bunch of xxx32 and xxx64 variables, and Jamdefs will select the
proper one based on BUILD_ARCH.  Jamdefs also defines the xxxHB
add-on variables if BUILD_ARCH says to build a hybrid.

readvers returns the primary config string as "config" when
building a hybrid.

mkdefault follows a scheme similar to Jamdefs for generating
the CCMACH etc variables into default.h:  it defines xxx32 and
xxx64 variables into default.h, and at the end selects the
primary arch for the xxx variable.  Note that default.h is
included as part of building iisysdep as well.

mkbzarch arranges for bzarch.h to #define either the config32 or
config64 string, depending on which arch is being compiled. It
also #defines conf_BUILD_ARCH_xx_yy if the platform is hybrid capable.
If arch is 32 or 64, it defines conf_BUILD_ARCH_32 or _64.
If arch is 32+64 or 64+32, it defines conf_BUILD_ARCH_32_64
or conf_BUILD_ARCH_64_32 respectively.  (The conf_BUILD_ARCH_xx_yy
symbols are defined for ccpp and yypp preprocessing as well,
as well as in default.h and therefore in iisysdep.)

For hybrid capable platforms, both jam and mkdefault include
-DBUILD_ARCH32 or -DBUILD_ARCH64 on the cc command line.  The
config string is no longer defined on the command line.

mkbzarch used to define additional platform strings so that
new platforms could piggyback on existing conditional compilation.
For example, i64_hpu would also define hp2_us5, hpb_us4, and hp8_us5;
making it a bit hard to figure out exactly what the real config
string was in the first place.
This is no longer done.  Instead, some generic OS symbols are
defined into bzarch.h:
    su4_us5, su9_us5 --> sparc_sol
    hp8_us5, hpb_us5, hp2_us5, i64_hpu --> any_hpux
    hpb_us5, hp2_us5, i64_hpu --> thr_hpux ("os-threaded hpux")
    rs4_us5, r64_us5 --> any_aix

----------------------------------------------------------------

VMS limitations:

VMS does not support #set: param or #set: arch.  Nor does it support
#set: on the same line as a jam-statement.  (This is as of June 2009.)


Windows limitations:

Windows does not support any #set: option other than config, build, and inc.
It appears that Windows has never supported configurable options in
the VERS file in any case.  (June 2009.)
