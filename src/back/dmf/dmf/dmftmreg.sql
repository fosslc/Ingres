/*
**Copyright (c) 2004 Ingres Corporation
**
**
** REGISTER TABLE statements which register the current TM MO data structures
** for retrieval through the IMA MO Gateway.
**
**
** History:
**	26-jul-1993 (mikem)
**	    Created (modeled after the lg ima interface).
*/

remove tmmo_perf;\p\g

register table tmmo_perf (

    server		varchar(32) not null not default is 'SERVER',

    tmperf_collect  i4 not null not default 
			is 'exp.dmf.perf.tmperf_collect',
    tmperf_utime_secs  i4 not null not default 
			is 'exp.dmf.perf.tmperf_utime.TM_secs',
    tmperf_utime_msecs i4 not null not default 
			is 'exp.dmf.perf.tmperf_utime.TM_msecs',
    tmperf_stime_secs  i4 not null not default 
			is 'exp.dmf.perf.tmperf_stime.TM_secs',
    tmperf_stime_msecs i4 not null not default 
			is 'exp.dmf.perf.tmperf_stime.TM_msecs',
    tmperf_cpu_secs  i4 not null not default 
			is 'exp.dmf.perf.tmperf_cpu.TM_secs',
    tmperf_cpu_msecs i4 not null not default 
			is 'exp.dmf.perf.tmperf_cpu.TM_msecs',
    tmperf_maxrss      i4 not null not default 
			is 'exp.dmf.perf.tmperf_maxrss',
    tmperf_idrss      i4 not null not default 
			is 'exp.dmf.perf.tmperf_idrss',
    tmperf_minflt      i4 not null not default 
			is 'exp.dmf.perf.tmperf_minflt',
    tmperf_majflt      i4 not null not default 
			is 'exp.dmf.perf.tmperf_majflt',
    tmperf_nswap      i4 not null not default 
			is 'exp.dmf.perf.tmperf_nswap',
    tmperf_reads      i4 not null not default 
			is 'exp.dmf.perf.tmperf_reads',
    tmperf_writes      i4 not null not default
			is 'exp.dmf.perf.tmperf_writes',
    tmperf_dio      i4 not null not default
			is 'exp.dmf.perf.tmperf_dio',
    tmperf_msgsnd      i4 not null not default 
			is 'exp.dmf.perf.tmperf_msgsnd',
    tmperf_msgrcv      i4 not null not default 
			is 'exp.dmf.perf.tmperf_msgrcv',
    tmperf_msgtotal      i4 not null not default 
			is 'exp.dmf.perf.tmperf_msgtotal',
    tmperf_nsignals      i4 not null not default 
			is 'exp.dmf.perf.tmperf_nsignals',
    tmperf_nvcsw      i4 not null not default 
			is 'exp.dmf.perf.tmperf_nvcsw',
    tmperf_nivcsw      i4 not null not default 
			is 'exp.dmf.perf.tmperf_nivcsw'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = ( server )
\p\g
commit;
\p\g
grant all on tmmo_perf to public
\p\g

/* qry to make server update it's static buffer which contains the perf stats */
update yymib_objects set value = dbmsinfo('ima_vnode')
        where classid = 'exp.dmf.perf.tmperf_collect'
        and instance = '0'
        and server = dbmsinfo('ima_server');
\p\g
