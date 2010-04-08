/*
**Copyright (c) 2004 Ingres Corporation
*/
/*************************************************
	Add new system catalogs
	iiproc_params, iiproc_access.

	sql -U'$ingres' <database> < iiproc.sql
	
History:
        22-Aug-2007 (kiria01) b118999
            Move charextract to byteextract
	05-Mar-2009 (kiria01) b121520
	    Put CHAR cast to results of BYTEEXTRACT to cover
	    its return type change.
*************************************************/

drop view iiproc_params
\p\g
create view iiproc_params
   as select procedure_name = p1.dbp_name
			 , procedure_owner = p1.dbp_owner 
		     , param_name = p2.pp_name
	 	     , param_sequence = p2.pp_number
			 , param_datatype = uppercase(iitypename(ii_ext_type(p2.pp_datatype, p2.pp_length)))
		     , param_datatype_code = p2.pp_datatype
		     , param_length = iiuserlen(ii_ext_type(p2.pp_datatype, p2.pp_length), (ii_ext_length(p2.pp_datatype, p2.pp_length)*65536) +p2.pp_precision)
		     , param_scale =int4(mod( p2.pp_precision, (256)))
			 , param_nulls=char(byteextract('YN',  (mod(p2.pp_flags,2))+1))
			 , param_defaults=char(byteextract('YN', (mod((p2.pp_flags/2),(2)))+1))
			 , param_default_val=squeeze(d.defvalue)
from iiprocedure p1 
, iiprocedure_parameter p2
, iidefault d
where (p2.pp_procid1 = p1.dbp_id and p2.pp_procid2 = p1.dbp_idx)
and (p2.pp_defid1 = d.defid1 and p2.pp_defid2 = d.defid2)
\p\g
drop view iiproc_access
\p\g
create view iiproc_access
as
select object_name=p.probjname,             
object_owner=p.probjowner, permit_grantor=p.prograntor,               
object_type=p.probjtype, create_date=gmt_timestamp(p.procrtime1),     
permit_user=p.prouser, permit_depth=p.prodepth,                       
permit_number=int2(p.propermid), text_sequence=int2(q.seq) +1,        
text_segment=q.txt from "$ingres". iiqrytext q, "$ingres". iiprotect p
where p.proqryid1=q.txtid1 and p.proqryid2=q.txtid2 and q.mode=19 and 
mod((p.proopset/128), (2))=0  and p.probjtype = 'P'                                         
\p\g
drop view iiaccess
\p\g
EXEC SQL CREATE VIEW  iiaccess AS
SELECT
table_name  = r.relid,
table_owner = r.relowner,
table_type  =
char(byteextract('TVI',(mod((r.relstat/32), (2))
+(2*(mod((r.relstat/128), (2)))))+1)),
system_use = char(byteextract('USSG',
(mod(r.relstat,(2)))+(mod((r.relstat/16384), (2)))
+(3*mod((r.relstat2/16), (2)))+1)),
permit_user = p.prouser,
permit_type = iipermittype(p.proopset,0)
FROM iirelation r, iiprotect p
WHERE r.reltid = p.protabbase
AND (mod((p.proopset/128), 2) = 0)
AND iimacaccess(r.relsecid,1)=1;
\p\g
\q
