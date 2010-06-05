/*
**Copyright (c) 2004 Ingres Corporation
**
**	27-Apr-2010 (kschendel) SIR 123639
**	    Byte-ize the bitmap columns.
*/
/* Description of table iipriv from database iidbdb
  EXEC SQL DECLARE iipriv TABLE
	(d_obj_id	integer not null,
	 d_obj_idx	integer not null,
	 d_priv_number	smallint not null,
	 d_obj_type	smallint not null,
	 i_obj_id	integer not null,
	 i_obj_idx	integer not null,
	 i_priv	integer not null,
	 i_priv_grantee	char(32) not null,
	 prv_flags	integer not null,
	 i_priv_map	byte(DB_COL_BYTES) not null
);
*/

  struct prv_tbl_ {
	i4	d_obj_id;
	i4	d_obj_idx;
	short	d_priv_number;
	short	d_obj_type;
	i4	i_obj_id;
	i4	i_obj_idx;
	i4	i_priv;
	char	i_priv_grantee[33];
	i4	prv_flags;
	char	i_priv_map[DB_COL_BYTES];
  } prv_tbl;
