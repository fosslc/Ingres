/*
**	Copyright (c) 2004 Ingres Corporation
*/
extern II_STATUS usop_lenchk();
extern II_STATUS usop_compare();
extern II_STATUS usop_keybld();
extern II_STATUS usop_getempty();
extern II_STATUS usop_valchk();
extern II_STATUS usop_hashprep();
extern II_STATUS usop_helem();
extern II_STATUS usop_hmin();
extern II_STATUS usop_hmax();
extern II_STATUS usop_dhmin();
extern II_STATUS usop_dhmax();
extern II_STATUS usop_hg_dtln();
extern II_STATUS usop_minmaxdv();
extern II_STATUS usop_tmlen();
extern II_STATUS usop_tmcvt();
extern II_STATUS usop_dbtoev();
extern II_STATUS usop_add();
extern II_STATUS usop_convert();
extern II_STATUS usop_distance();
extern II_STATUS usop_slope();
extern II_STATUS usop_midpoint();
extern II_STATUS usop_xcoord();
extern II_STATUS usop_ycoord();
extern II_STATUS usop_ordpair();
extern II_STATUS usop_trace(II_SCB          *scb ,
            		    II_DATA_VALUE         *dispose_mask ,
            		    II_DATA_VALUE         *trace_string ,
            		    II_DATA_VALUE         *result ); 
extern II_STATUS (*Ingres_trace_function)();
