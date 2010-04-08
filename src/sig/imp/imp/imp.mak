#
#   NMAKE makefile for imp
#
## History:
##      28-Jun-2002 (fanra01)
##          Sir 108159
##          Created.
##      12-Nov-2002 (fanra01)
##          Bug 109104
##          Changed the default target database to imadb.
##          Also allow change of database name from environment.
##	28-oct-2004 (somsa01 for drivi01)
##	    Updated for building with JAM.
##		20-Sep-2005 (fanra01)
##			Quote delimit II_SYSTEM as it may contain spaces.
#

# Pre-requisites
#
# Database should be loaded with the front-end forms.  The loadfrm section
# copies the forms into the database.

!if "$(DATABASE)" == ""
DATABASE    = imadb
!endif

!MESSAGE TARGET DATABASE: $(DATABASE)

# Build Commands
CC          = CL -nologo -c                         # -c compile only
LINKCMD     = LINK /NOLOGO
LIBCMD      = LIB /MACHINE:IX86 

# Compile Flags
CDEBUG       =  -Z7 -Od                             # -Z7 include debug info
                                                    # -Od disable optimization
CFLAGS      =   $(CDEBUG) -G5 -Di386=1 -D_X86_ -W3  # -G5 code generation for
                                                    #     pentium class
                                                    #     processors
                                                    # -W3 warning level 3

# Link flags
LDEBUG       =   /DEBUG:mapped,full /DEBUGTYPE:both # link debug information
LFLAGS      =   /MAP $(LDEBUG) /MACHINE:IX86

.SUFFIXES: .sc .obj .c .lfm

# processing rules changed fo esqlc compile so that include files are
# unchanged from default.  .sc -> .c
.sc.obj:
        esqlc -n.sh -o -f$*.c $<
        $(CC) $(CFLAGS) $*.c

.sc.c:
        esqlc -n.sh -o -f$*.c $<

# Target executable
IMP     = imp.exe

# Source files generated from form definitions loaded previously by loadfrm.
FORMSRCS = \
ima_dio.c ima_di_slave_info.c ima_qsf_cache_stats.c ima_qsf_dbp.c \
ima_qsf_rqp.c ima_rdf_cache_info.c imp_client_info.c \
imp_dmfcache.c imp_dmf_cache_stats.c imp_lg_address.c \
imp_lg_databases.c imp_lg_header.c imp_lg_menu.c imp_lg_processes.c \
imp_lg_summary.c imp_lg_transactions.c imp_lk_list.c imp_lk_locks.c \
imp_lk_menu.c imp_lk_tx.c imp_lock_summary.c imp_main_menu.c \
imp_res_list.c imp_res_locks.c imp_server_diag.c imp_server_list.c \
imp_server_parms.c imp_server_popup.c imp_session_detail.c \
imp_session_list.c imp_query_info.c \
imp_transaction_detail.c imp_transaction_hexdump.c imp_gcc_info.c

# Archive of object files.
LIBSRCS = \
impdbcon.sc impdmf.sc impfrs.sc impinter.sc impio.sc \
implocks.sc implog.sc impqsf.sc imprdf.sc impserver.sc impslave.sc \
impstart.sc impstatic.sc

# Included files.
HEADERS  = impcommon.sc imp_dmf.incl

PREPMAIN = impmain.sc
MAINSRCS = impmain.c

LIBRARY     = IMP.LIB
LIBOBJS     = $(LIBSRCS:.c=.obj) $(FORMSRCS:.c=.obj)
LIBOBJS     = $(LIBOBJS:.sc=.obj)
LIBOBJS     = $(LIBOBJS:.qsc=.obj)
LIBOBJS     = $(LIBOBJS:.roc=.obj)
PREPMAIN    = $(PREPMAIN:.sc=.c)
MAINOBJS    = $(MAINSRCS:.c=.obj)
HDRS        = $(HEADERS:.sc=.c)
HDRS        = $(HDRS:.incl=.c)

.PRECIOUS:  $(LIBRARY) $(LIBOBJS)

all:        compform $(HDRS) $(PREPMAIN) $(MAINOBJS) $(LIBRARY) $(IMP)

compform: $(FORMSRCS)

$(FORMSRCS):
    !compform $(DATABASE) $* $*.c

!IF "$(MAINSRCS)" != ""
$(MAINOBJS):
    !$(CC) $(CFLAGS) $(INCLUDES) -Gd -Ge -Fo$@ $? > %|pfF.err
    !COPY $@ ..\..\lib
!ENDIF

!IF "$(LIBSRCS)" != ""
$(LIBRARY): $(LIBOBJS)
    !$(LIBCMD) /OUT:$@ $(LIBOBJS)
!ENDIF

imp_dmf.c: imp_dmf.incl
    esqlc -c -o.h -n.sh -f$@ $**

$(IMP): impmain.obj $*.lib
    $(LINKCMD) $(LFLAGS) /OUT:$@ \
    $** \
    "$(II_SYSTEM)\ingres\lib\libingres.lib"
