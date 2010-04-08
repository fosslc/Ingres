/*
** Copyright (c) 2007, 2009 Ingres Corporation
**
*/

/*
** Name: astjacket.h
**
** Description:
**    redefines of asynchronous VMS system services
**    i.e. those that may issue ASTs
**
**
** History:
**  18-Jun-2007 (stegr01)
**      Created.
**  22-dec-2008 (stegr01)
**      Added definitions for ii_useScb, to reduce the need
**      for #ifdef VMS in generic code.
**	07-jul-2009 (joea)
**	    Remove ii_sys$hiber, ii_sys$wake and ii_sys$schdwk.
*/

#ifndef ASTJACKET_H
#define ASTJACKET_H

#if defined(VMS) && defined(OS_THREADS_USED)

#ifndef __STARLET_LOADED
#error <starlet.h> must be included BEFORE including <astjacket.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#undef sys$brkthru
#undef sys$check_privilege
#undef sys$dclast
#undef sys$enq
#undef sys$enqw
#undef sys$getdvi
#undef sys$getlki
#undef sys$getjpi
#undef sys$getqui
#undef sys$getrmi
#undef sys$getsyi
#undef sys$qio
#undef sys$setast
#undef sys$setimr
#undef sys$cantim
#undef sys$sndjbc
#undef sys$acm
#undef sys$audit_event
#undef sys$cpu_transition
#undef sys$dns
#undef sys$files_64
#undef sys$getdti
#undef sys$io_fastpath
#undef sys$io_setup
#undef sys$ipc
#undef sys$lfs
#undef sys$setcluevt
#undef sys$set_system_event
#undef sys$setevtast
#undef sys$setpfm
#undef sys$setpra
#undef sys$setdti
#undef sys$setuai
#undef sys$updsec
#undef sys$xfs_client
#undef sys$xfs_server

#undef SYS$BRKTHRU
#undef SYS$CHECK_PRIVILEGE
#undef SYS$DCLAST
#undef SYS$ENQ
#undef SYS$ENQW
#undef SYS$GETDVI
#undef SYS$GETLKI
#undef SYS$GETJPI
#undef SYS$GETQUI
#undef SYS$GETRMI
#undef SYS$GETSYI
#undef SYS$QIO
#undef SYS$SETAST
#undef SYS$SETIMR
#undef SYS$CANTIM
#undef SYS$SNDJBC
#undef SYS$ACM
#undef SYS$AUDIT_EVENT
#undef SYS$CPU_TRANSITION
#undef SYS$DNS
#undef SYS$FILES_64
#undef SYS$GETDTI
#undef SYS$IO_FASTPATH
#undef SYS$IO_SETUP
#undef SYS$IPC
#undef SYS$LFS
#undef SYS$SETCLUEVT
#undef SYS$SET_SYSTEM_EVENT
#undef SYS$SETEVTAST
#undef SYS$SETPFM
#undef SYS$SETPRA
#undef SYS$SETDTI
#undef SYS$SETUAI
#undef SYS$UPDSEC
#undef SYS$XFS_CLIENT
#undef SYS$XFS_SERVER



#define sys$brkthru          ii_sys$brkthru
#define sys$check_privilege  ii_sys$check_privilege
#define sys$dclast           ii_sys$dclast
#define sys$enq              ii_sys$enq
#define sys$enqw             ii_sys$enqw
#define sys$getdvi           ii_sys$getdvi
#define sys$getlki           ii_sys$getlki
#define sys$getjpi           ii_sys$getjpi
#define sys$getqui           ii_sys$getqui
#define sys$getrmi           ii_sys$getrmi
#define sys$getsyi           ii_sys$getsyi
#define sys$qio              ii_sys$qio
#define sys$setast           ii_sys$setast
#define sys$setimr           ii_sys$setimr
#define sys$cantim           ii_sys$cantim
#define sys$sndjbc           ii_sys$sndjbc
#define sys$acm              ii_sys$acm
#define sys$audit_event      ii_sys$audit_event
#define sys$cpu_transition   ii_sys$cpu_transition
#define sys$dns              ii_sys$dns
#define sys$files_64         ii_sys$files_64
#define sys$getdti           ii_sys$getdti
#define sys$io_fastpath      ii_sys$io_fastpath
#define sys$io_setup         ii_sys$io_setup
#define sys$ipc              ii_sys$ipc
#define sys$lfs              ii_sys$lfs
#define sys$setcluevt        ii_sys$setcluevt
#define sys$set_system_event ii_sys$set_system_event
#define sys$setevtast        ii_sys$setevtast
#define sys$setpfm           ii_sys$setpfm
#define sys$setpra           ii_sys$setpra
#define sys$setdti           ii_sys$setdti
#define sys$setuai           ii_sys$setuai
#define sys$updsec           ii_sys$updsec
#define sys$xfs_client       ii_sys$xfs_client
#define sys$xfs_server       ii_sys$xfs_server

#define SYS$BRKTHRU          ii_sys$brkthru
#define SYS$CHECK_PRIVILEGE  ii_sys$check_privilege
#define SYS$DCLAST           ii_sys$dclast
#define SYS$ENQ              ii_sys$enq
#define SYS$ENQW             ii_sys$enqw
#define SYS$GETDVI           ii_sys$getdvi
#define SYS$GETLKI           ii_sys$getlki
#define SYS$GETJPI           ii_sys$getjpi
#define SYS$GETQUI           ii_sys$getqui
#define SYS$GETRMI           ii_sys$getrmi
#define SYS$GETSYI           ii_sys$getsyi
#define SYS$QIO              ii_sys$qio
#define SYS$SETAST           ii_sys$setast
#define SYS$SETIMR           ii_sys$setimr
#define SYS$CANTIM           ii_sys$cantim
#define SYS$SNDJBC           ii_sys$sndjbc
#define SYS$ACM              ii_sys$acm
#define SYS$AUDIT_EVENT      ii_sys$audit_event
#define SYS$CPU_TRANSITION   ii_sys$cpu_transition
#define SYS$DNS              ii_sys$dns
#define SYS$FILES_64         ii_sys$files_64
#define SYS$GETDTI           ii_sys$getdti
#define SYS$IO_FASTPATH      ii_sys$io_fastpath
#define SYS$IO_SETUP         ii_sys$io_setup
#define SYS$IPC              ii_sys$ipc
#define SYS$LFS              ii_sys$lfs
#define SYS$SETCLUEVT        ii_sys$setcluevt
#define SYS$SET_SYSTEM_EVENT ii_sys$set_system_event
#define SYS$SETEVTAST        ii_sys$setevtast
#define SYS$SETPFM           ii_sys$setpfm
#define SYS$SETPRA           ii_sys$setpra
#define SYS$SETDTI           ii_sys$setdti
#define SYS$SETUAI           ii_sys$setuai
#define SYS$UPDSEC           ii_sys$updsec
#define SYS$XFS_CLIENT       ii_sys$xfs_client
#define SYS$XFS_SERVER       ii_sys$xfs_server

extern void ii_useScb(i4 stat);
extern void ii_useAst(void);

extern i4 ii_sys$acm();
extern i4 ii_sys$audit_event();
extern i4 ii_sys$brkthru();
extern i4 ii_sys$cantim();
extern i4 ii_sys$check_privilege();
extern i4 ii_sys$cpu_transition();
extern i4 ii_sys$dclast();
extern i4 ii_sys$dns();
extern i4 ii_sys$enq();
extern i4 ii_sys$enqw();
extern i4 ii_sys$files_64();
extern i4 ii_sys$getdti();
extern i4 ii_sys$getdvi();
extern i4 ii_sys$getlki();
extern i4 ii_sys$getjpi();
extern i4 ii_sys$getqui();
extern i4 ii_sys$getrmi();
extern i4 ii_sys$getsyi();
extern i4 ii_sys$io_fastpath();
extern i4 ii_sys$io_setup();
extern i4 ii_sys$ipc();
extern i4 ii_sys$lfs();
extern i4 ii_sys$qio();
extern i4 ii_sys$set_system_event();
extern i4 ii_sys$setast();
extern i4 ii_sys$setcluevt();
extern i4 ii_sys$setdti();
extern i4 ii_sys$setevtast();
extern i4 ii_sys$setimr();
extern i4 ii_sys$setpfm();
extern i4 ii_sys$setpra();
extern i4 ii_sys$setuai();
extern i4 ii_sys$sndjbc();
extern i4 ii_sys$updsec();
extern i4 ii_sys$xfs_client();
extern i4 ii_sys$xfs_server();

#define II_USESCB(stat) ii_useScb(stat)
#define II_USEAST()     ii_useAst()

#ifdef  __cplusplus
}
#endif

#endif /* VMS && OS_THREADS_USED */

#ifndef II_USESCB

#define II_USESCB(stat)
#define II_USEAST()

#endif


#endif  /* ASTJACKET_H */

