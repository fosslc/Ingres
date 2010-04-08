/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : reptbdef.h, header file
**
**  Project  : Ingres II/ VDBA.
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : created from the old file repltbl.sh to be used by
**             collisio.sc and tblinteg.sc.
**             reptbdef.h is now controlled by the source control.
**
** History :
**
** 13-Feb-2002 (uk$so01)
**    Created
**    SIR #106648, remove the use of repltbl.sh
*/


#if !defined (REPTBLDEF_HEADER)
#define REPTBLDEF_HEADER

typedef struct _coldef
{
	char column_name[DB_MAXNAME+1];
	char dlm_column_name[DLMNAME_LENGTH];
	char column_datatype[DB_MAXNAME+1];
	long column_length;
	long column_scale;
	long column_sequence;
	char column_nulls[2];
	char column_defaults[2];
	long key_sequence;
} COLDEF;

typedef struct _tbldef
{
	char table_name[DB_MAXNAME+1];
	char dlm_table_name[DLMNAME_LENGTH];
	char table_owner[DB_MAXNAME+1];
	char dlm_table_owner[DLMNAME_LENGTH];
	i4 table_no;
	i4 page_size;
	i2 cdds_no;
	i2 target_type;
	i4 ncols; // Number of registered columns 
	char columns_registered[26];
	char supp_objs_created[26];
	char rules_created[26];
	char cdds_lookup_table[DB_MAXNAME+1];
	char prio_lookup_table[DB_MAXNAME+1];
	char index_used[DB_MAXNAME+1];
	char sha_name[DB_MAXNAME+1];
	char arc_name[DB_MAXNAME+1];
	char rem_ins_proc[DB_MAXNAME+1];
	char rem_upd_proc[DB_MAXNAME+1];
	char rem_del_proc[DB_MAXNAME+1];
	COLDEF *col_p;
} TBLDEF;

#endif // REPTBLDEF_HEADER