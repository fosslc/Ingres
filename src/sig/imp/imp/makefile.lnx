.SUFFIXES: .o  .c  .c~	.sc  .y	 .y~  .l  .l~  .s  .s~	.h  .h~	 .sh  .sh~  .f	.f~  .asm  .asm~  .src	.src~  .inc  .inc~

.sc.c:
	esqlc $<

.c.o:
	cc -g -c $(CFLAGS) $(INCLS) $*.c

DATABASE=   imadb

CFLAGS = -g -DDATABASE=\"$(DATABASE)\"

# Take the LDFLAGS from the $II_SYSTEM/ingres/utility/iisysdep's LDLIBMACH
#LDFLAGS = -lpthread -lm -lc -lcrypt -ldl
# Try these flags instead if the make fails
#LDFLAGS = -lm -lc -lcrypt -ldl

#LDFLAGS = -lcurses -lgen -lm -lnsl -lsocket -lelf -mt -ldl
#LDFLAGS = -ldnet_stub -lrt -lm -lbsd -laio -lpthread -lmach -lexc -lmld
LDFLAGS =  -ldl -lm  -lpthread -lcrypt

#INCLS=  $(ING_SRC)/cl/clf/hdr $(ING_SRC)/cl/hdr/hdr

#CMPLIB= $(ING_BUILD)/lib/libcompat.a

all:	register copyform compform imp

implog.c : implog.sc $(FORMS) $(HDRS) Makefile
implocks.c : implocks.sc $(FORMS) $(HDRS) Makefile
impserver.c : impserver.sc $(FORMS) $(HDRS) Makefile
impstart.c : impstart.sc $(FORMS) $(HDRS) Makefile
impstatic.c : impstatic.sc $(FORMS) $(HDRS) Makefile
impdbcon.c : impdbcon.sc $(FORMS) $(HDRS) Makefile
impfrs.c : impfrs.sc $(FORMS) $(HDRS) Makefile
impmain.c : impmain.sc $(HDRS) Makefile
impcommon.c : impcommon.sc $(HDRS) Makefile
implkmain.c : implkmain.sc $(HDRS) Makefile
impinter.c : impinter.sc $(HDRS) Makefile
impqsf.c : impqsf.sc $(HDRS) Makefile
imprdf.c : imprdf.sc $(HDRS) Makefile
impio.c : impio.sc $(HDRS) Makefile
impdmf.c : impdmf.sc $(HDRS) Makefile
impslave.c : impslave.sc $(HDRS) Makefile
impsvrlist.c : $(HDRS) Makefile
impseslst.c : $(HDRS) Makefile
impsvrpop.c : $(HDRS) Makefile
impsesdtl.c : $(HDRS) Makefile
impmenu.c : $(HDRS) Makefile
impsvrprm.c : $(HDRS) Makefile
implksum.c : $(HDRS) Makefile
implkmenu.c : $(HDRS) Makefile
implklst.c : $(HDRS) Makefile
imp_lk_tx.c : $(HDRS) Makefile
imp_lk_locks.c : $(HDRS) Makefile
impreslocks.c : $(HDRS) Makefile
imp_res_list.c : $(HDRS) Makefile
imp_lg_summary.c : $(HDRS) Makefile
imp_lg_menu.c : $(HDRS) Makefile
imp_lg_header.c : $(HDRS) Makefile
imp_lg_address.c : $(HDRS) Makefile
imp_lg_processes.c : $(HDRS) Makefile
imp_lg_databases.c : $(HDRS) Makefile
imp_lg_transactions.c : $(HDRS) Makefile
imp_client_info.c : $(HDRS) Makefile
imp_server_diag.c : $(HDRS) Makefile
imp_dmf_cache_stats.c : $(HDRS) Makefile
imp_dmfcache.c : $(HDRS) Makefile
ima_qsf_cache_stats.c : $(HDRS) Makefile
ima_qsf_dbp.c : $(HDRS) Makefile
ima_qsf_rqp.c : $(HDRS) Makefile
ima_rdf_cache_info.c : $(HDRS) Makefile
ima_dio.c : $(HDRS) Makefile
ima_di_slave_info.c : $(HDRS) Makefile
implklocks.c : $(HDRS) Makefile
imp_query_info.c : $(HDRS) Makefile
imp_transaction_detail.c : $(HDRS) Makefile
imp_transaction_hexdump.c : $(HDRS) Makefile
imp_gcc_info.c : $(HDRS) Makefile

FILES=	 impserver.o impmain.o impcommon.o impfrs.o impdbcon.o \
	impstart.o impstatic.o implocks.o implog.o impinter.o \
	impqsf.o imprdf.o impio.o impdmf.o impslave.o

FORMS=	impsvrlist.o impseslst.o impsvrpop.o impsesdtl.o impmenu.o \
	impsvrprm.o implksum.o implkmenu.o implklst.o imp_lk_tx.o \
	implklocks.o impreslocks.o imp_res_list.o imp_lg_summary.o \
	imp_lg_menu.o imp_lg_header.o imp_lg_address.o imp_lg_processes.o \
	imp_lg_databases.o imp_lg_transactions.o imp_client_info.o \
	imp_server_diag.o imp_dmf_cache_stats.o ima_qsf_cache_stats.o \
	ima_qsf_dbp.o ima_qsf_rqp.o ima_rdf_cache_info.o ima_dio.o \
	ima_di_slave_info.o imp_dmfcache.o imp_query_info.o \
	imp_transaction_detail.o imp_transaction_hexdump.o imp_gcc_info.o 

HDRS=	imp.h

DEFN=	imp.sql


imp: $(FILES) $(FORMS) Makefile
	cc $(FILES) $(FORMS) $(II_SYSTEM)/ingres/lib/libingres.a \
	$(LDFLAGS) -o imp

compform:
	compform $(DATABASE) imp_lk_list implklst.c
	compform $(DATABASE) imp_lk_locks implklocks.c
	compform $(DATABASE) imp_lk_menu implkmenu.c
	compform $(DATABASE) imp_lock_summary implksum.c
	compform $(DATABASE) imp_main_menu impmenu.c
	compform $(DATABASE) imp_server_list impsvrlist.c
	compform $(DATABASE) imp_server_parms impsvrprm.c
	compform $(DATABASE) imp_server_popup impsvrpop.c
	compform $(DATABASE) imp_session_detail impsesdtl.c
	compform $(DATABASE) imp_session_list impseslst.c
	compform $(DATABASE) imp_lk_tx imp_lk_tx.c
	compform $(DATABASE) imp_res_locks impreslocks.c
	compform $(DATABASE) imp_res_list imp_res_list.c
	compform $(DATABASE) imp_lg_summary imp_lg_summary.c
	compform $(DATABASE) imp_lg_menu imp_lg_menu.c
	compform $(DATABASE) imp_lg_header imp_lg_header.c
	compform $(DATABASE) imp_lg_address imp_lg_address.c
	compform $(DATABASE) imp_lg_processes imp_lg_processes.c
	compform $(DATABASE) imp_lg_databases imp_lg_databases.c
	compform $(DATABASE) imp_lg_transactions imp_lg_transactions.c
	compform $(DATABASE) imp_client_info imp_client_info.c
	compform $(DATABASE) imp_server_diag imp_server_diag.c
	compform $(DATABASE) imp_dmf_cache_stats imp_dmf_cache_stats.c
	compform $(DATABASE) imp_dmfcache imp_dmfcache.c
	compform $(DATABASE) ima_qsf_cache_stats ima_qsf_cache_stats.c
	compform $(DATABASE) ima_qsf_dbp ima_qsf_dbp.c
	compform $(DATABASE) ima_qsf_rqp ima_qsf_rqp.c
	compform $(DATABASE) ima_rdf_cache_info ima_rdf_cache_info.c
	compform $(DATABASE) ima_dio ima_dio.c
	compform $(DATABASE) ima_di_slave_info ima_di_slave_info.c
	compform $(DATABASE) imp_query_info imp_query_info.c
	compform $(DATABASE) imp_transaction_detail imp_transaction_detail.c
	compform $(DATABASE) imp_transaction_hexdump imp_transaction_hexdump.c
	compform $(DATABASE) imp_gcc_info imp_gcc_info.c

catalogs: $(DEFN)
	sql $(DATABASE) "-u\$$ingres" < imp.reg

register:
	upgradefe $(DATABASE) "-u\$$ingres" INGRES  
	sql $(DATABASE) "-u\$$ingres" < imp.sql

copyform:
	copyform -i $(DATABASE) imp.forms

saveforms:
	copyform $(DATABASE) imp.forms imp_lk_list imp_lk_menu \
		imp_lock_summary imp_main_menu imp_server_list \
		imp_server_parms imp_server_popup \
		imp_session_detail imp_session_list imp_lk_tx \
		imp_lk_locks imp_res_locks imp_res_list \
		imp_lg_summary imp_lg_menu imp_lg_header \
		imp_lg_address imp_lg_processes \
		imp_lg_databases imp_lg_transactions \
		imp_client_info imp_server_diag \
		imp_dmf_cache_stats \
		imp_dmfcache \
		ima_qsf_cache_stats \
		ima_qsf_dbp ima_qsf_rqp ima_rdf_cache_info ima_dio \
		ima_di_slave_info imp_query_info \
                imp_transaction_detail imp_transaction_hexdump \
                imp_gcc_info
