/*
** Copyright (c) 2008 Ingres Corporation.
*/

/*
** adgmo.sql - Define lookup tables for ADG MO access.
**
** History:
**	18-Jun-2008 (kiria01) b120519
**	    Created to match adgmo.c structure.
*/

/*
** adg_dt - Datatype definitions
*/
DROP adg_dt;
REGISTER TABLE adg_dt (
	dt_name		varchar(32) not null not default is 'exp.adf.adg.dt_name',
	dt_id		i2 not null not default is 'exp.adf.adg.dt_id',
	dt_stat		i4 not null not default is 'exp.adf.adg.dt_stat'
)
AS IMPORT FROM 'tables' WITH DBMS = IMA, STRUCTURE = unique SORTKEYED, KEY = (dt_name);

/*
** adg_op - operator definitions
*/
DROP adg_op;
REGISTER TABLE adg_op (
	op_name		varchar(32) not null not default is 'exp.adf.adg.op_name',
	op_id		i2 not null not default is 'exp.adf.adg.op_id',
	op_type		varchar(16) not null not default is 'exp.adf.adg.op_type',
	op_use		varchar(8) not null not default is 'exp.adf.adg.op_use',
	op_qlangs	i4 not null not default is 'exp.adf.adg.op_qlangs',
	op_systems	i4 not null not default is 'exp.adf.adg.op_systems',
	op_cntfi	i4 not null not default is 'exp.adf.adg.op_cntfi'
)
AS IMPORT FROM 'tables' WITH DBMS = IMA, STRUCTURE = unique SORTKEYED, KEY = (op_name);

/*
** adg_fi - Function instance definitions
*/
DROP adg_fi;
REGISTER TABLE adg_fi (
	fi_id		i2 not null not default is 'exp.adf.adg.fi_id',
	fi_cmplmnt	i2 not null not default is 'exp.adf.adg.fi_cmplmnt',
	fi_type		varchar(16) not null not default is 'exp.adf.adg.fi_type',
	fi_flags	i2 not null not default is 'exp.adf.adg.fi_flags',
	fi_opid		i2 not null not default is 'exp.adf.adg.fi_opid',
	fi_numargs	i2 not null not default is 'exp.adf.adg.fi_numargs',
	fi_dtresult	i2 not null not default is 'exp.adf.adg.fi_dtresult',
	fi_dtarg1	i2 not null not default is 'exp.adf.adg.fi_dtarg1',
	fi_dtarg2	i2 not null not default is 'exp.adf.adg.fi_dtarg2',
	fi_dtarg3	i2 not null not default is 'exp.adf.adg.fi_dtarg3',
	fi_dtarg4	i2 not null not default is 'exp.adf.adg.fi_dtarg4'
)
AS IMPORT FROM 'tables' WITH DBMS = IMA, STRUCTURE = unique SORTKEYED, KEY = (fi_id);

