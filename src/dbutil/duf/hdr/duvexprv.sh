/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iixpriv from database iidbdb */
  EXEC SQL DECLARE iixpriv TABLE
	(i_obj_id	integer not null,
	 i_obj_idx	integer not null,
	 i_priv	integer not null,
	 i_priv_grantee	char(32) not null,
	 tidp	integer not null);

  struct xprv_tbl_ {
	i4	i_obj_id;
	i4	i_obj_idx;
	i4	i_priv;
	char	i_priv_grantee[33];
	i4	tidp;
  } xprv_tbl;
