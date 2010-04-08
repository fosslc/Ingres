Rules for the platform jamdefs, Jamdefs.$(PLATFORM_JAMDEFS):

Variables that can be set include:

VERS		- config string (required)
AS		- assembler command
LD		- link command
CCLD		- link via cc command
ASMACH		- flags for as command
CCMACH		- flags for compiling via cc
C++FLAGS	- flags for compiling with c++
CCLDMACH	- flags for linking via cc
LDLIBPATH	- system link search path (apparently not used??)
LDLIBMACH	- system libraries when linking via cc
LDORIGIN	- cc link flag addition for supplying $ORIGIN based rpath
		  for distributed binaries, NOT including the dbms server
CCLDSERVER	- added CCLDMACH for linking iimerge.o (with -r)
CCSHLDMACH	- added CCLDMACH for linking shared libraries
ASCPPFLAGS	- flags for preprocessing assembly files before m4/as
ASM4FLAGS	- flags for m4-processing assembly files after cpp, before as
		  (only applies to AIX, currently)

This is not a complete list, other variables can be and are set.
See below.

AS, LD, and CCLD are not normally set, as the defaults normally work fine.

CCLDSERVER and CCSHLDMACH are more important on the mkdefault / shlibinfo
side of things, although they are used in the jam world as well.
CCLDSERVER is passed to the ld -r command that makes iimerge.o;
for server-archive or server-shared-libraries, the only thing going
into iimerge.o is dmfmerge.o, but IILIBEXE still does the ld -r.
CCSHLDMACH is only used for UDT demo library linking, other shared
libs are linked by mkshlibs / mksvrshlibs etc which get their flag
strings from mkdefault.

ASCPPFLAGS is normally not set;  Jamdefs will supply -Dconfig-string.
(The config-string is normally defined by bzarch.h, or for things
that are processed by ccpp / yypp, in the ccpp / yypp wrappers.
Assembly is one case where we need the -Dconfig-string in the flags.)

Keep in mind that we don't (yet) have a linkage between jam variables
and shell / conditional compiling, the bzarch / default / shlibinfo world.
So, things still have to be defined in two places.  This must and
will get fixed eventually, but as of July 2009 no obvious mechanism
for transmitting jam variables into the shell side has presented itself.

-----------

Hybrid-capable platforms DO NOT set the above variables in the platform
jamdefs;  instead, if some variable xxx is to be set, the platform jamdefs
will INSTEAD define xxx32 and xxx64 variations.  The hybrid-capable
VERS file must also supply the appropriate BUILD_ARCH setting.
Jamdefs will use BUILD_ARCH and the 32/64 bit variables to figure
out the base and xxxHB (add-on, hybrid) variables.  VERS32 and VERS64
are mandatory;  the rest are optional.  It would be prudent for a
hybrid-capable platform jamdefs to include:
	AS32 = $(AS) ;
	AS64 = $(AS) ;
and similarly for LD32/64, CCLD32/64;  the reason being that Jamdefs
will do
	AS = $(AS32) ;  # or $(AS64)
and setting AS to a defined but empty value might be a bad idea.
(Question: is this really necessary? probably not?)

Special rules for the hybrid 32 or 64-bit symbols:

ASMACH32/64	- include flags for forcing 32 vs 64-bit
CCMACH32/64	- must include -DBUILD_ARCH32/64;
		  include flags for forcing 32 vs 64-bit
C++FLAGS32/64	- must include -DBUILD_ARCH32/64;
		  include flags for forcing 32 vs 64-bit
CCLDMACH32/64	- include flags needed for forcing 32 vs 64-bit
LDORIGINLP32/64	- In addition to LDORIGIN, see below
		  for distributed binaries, NOT including the dbms server
CCLDSERVER32/64	- include flags needed for forcing 32 vs 64-bit
CCSHLDMACH32/64	- include flags needed for forcing 32 vs 64-bit
ASCPPFLAGS32/64	- do NOT include -DBUILD_ARCH32/64, jamdefs adds it since
		  this variable is unix-only and -D can be assumed

It's especially important to force the proper architecture on platforms
where the compiler or linker default depends on the architecture of
the build machine!  Hybrid-capable platforms must be able to build
either 32-bit or 64-bit binaries regardless of the build machine.

One special case for a hybrid-capable platform is LDORIGIN.  The
$ORIGIN-based path for the primary (as opposed to add-on) binaries
is usually something like '$ORIGIN/../lib'.  The add-on's look
like '$ORIGIN/../../lib/lp32' and .../lp64.  Since LDORIGIN is
tied to hybrid-ness more than bit-ness, the hybrid capable Jamdefs
should supply all three: LDORIGIN, LDORIGINLP32, and LDORIGINLP64.
Once again, Jamdefs will figure out which add-on origin to assign
to the ultimate add-on link flags, CCLDMACHHB.

---------

Other things that the platform Jamdefs should define (possibly not a
complete list!)

JAMDEFS_INCLUDED - Mandatory, just a signal to Jamdefs that the platform
		  Jamdefs file was found and processed.
CCPICFLAG	- cc flag for producing position-independent code
LEVEL1_OPTIM	- Level 1 or limited / safe optimization level
OPTIM		- usually blank?
IIOPTIM		- Standard cc optimization level plus any additional
		  flags needed to make that level safe.  (eg on linux
		  with gcc, it's -O2 -fno-strict-aliasing -fwrapv.)
NOOPTIM		- Explicitly non-optimizing flags, eg blank or -O0
SLSFX		- Shared library suffix, eg so
LIBSFX		- archive library suffix, eg a
OBJSFX		- object file suffix, eg o
DBMS_STACKSIZE	- standard default stack size
PAX_CHECK	- True if Jamdefs is to require PAXLOC to be defined,
		  generally as a shell environment variable
NO_KRB5HDR	- TRUE if Jamdefs is NOT to check that KRB5HDR is defined
platform_string	- A platform string substituted into the generic readme
extension_string - more of the same, generally set to the config string


Some things generally NOT defined in the platform Jamdefs, mostly because
they require an associated #set: option and thus must be defined in VERS:

BUILD_ARCH	- needs arch specification too, set in VERS
BUILD_ICE	- needs option BUILD_ICE too, set in VERS
SVR_ARLIB	- needs option SVR_AR too, set in VERS
SVR_SHLIBS	- needs option SVR_SHL too, set in VERS
XERCES_ARCH	- is build machine sensitive and is better set in either
		  VERS, or as an exported shell variable.
