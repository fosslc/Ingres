#ifndef OSLHDR_H_INCLUDED
#define OSLHDR_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

# define	DB_MAXNAME	24

/*
** The following definitions and typedefs should be changed for each
** installation to match what is in compat.h, targetsys.h and dbms.h.
*/

# define	i1		char
# define	u_i1		unsigned char
# define	i2		short
# define	i4		long
# define	f4		float
# define	f8		double
# define	nat		int
# define	longnat		long
# define	u_longnat	unsigned long

# define	ALIGN_RESTRICT  long

# define	DB_CNTSIZE	2

typedef		char *		PTR;
typedef		unsigned short	u_i2;
typedef		i2		DB_DT_ID;

#ifdef WSC
#define _PTR_
#else
#define _PTR_	(PTR)
#endif

typedef struct
{
	PTR		db_data;
	i4		db_length;
	DB_DT_ID	db_datatype;
	i2		db_prec;
	i2		db_collID;
} DB_DATA_VALUE;

typedef struct
{
	longnat		db_offset;
	i4		db_length;
	DB_DT_ID	db_datatype;
	i2		db_prec;
} IL_DB_VDESC;

typedef struct
{
	i2	fe_version;
	i2	fe_i1_align;
	i2	fe_i2_align;
	i2	fe_i4_align;
	i2	fe_f4_align;
	i2	fe_f8_align;
	i2	fe_char_align;
	i2	fe_max_align;
	i2	adf_hi_version;
	i2	adf_lo_version;
	i2	adf_max_align;
} FRMALIGN;

typedef struct
{
	char		*fdv2_name;
	char		*fdv2_tblname;
	char		fdv2_visible;
	i2		fdv2_order;
	nat		fdv2_dbdvoff;
	i2		fdv2_flags;
	char		*fdv2_typename;
} FDESCV2;

typedef struct
{
	nat	fdsc_zero;
	nat	fdsc_version;
	nat	fdsc_cnt;
	FDESCV2	*fdsc_ofdesc;
} FDESC;

typedef struct {
    PTR	    qs_var;
    u_i2    qs_index;
    u_i1    qs_base;
    u_i1    qs_type;
} QRY_SPEC;

typedef struct
{
    PTR		qsg_value;
    nat		(*qsg_gen)();
} QSGEN;

typedef struct {
    DB_DATA_VALUE   *qv_dbval;
    u_i2	    qv_index;
    u_i1	    qv_base;
    char	    *qv_name;
} QRY_ARGV;

typedef struct _qdesc {
    char	*qg_new;
    char	*qg_name;
    u_i1	qg_fill[2];
    u_i1	qg_version;
    u_i1	qg_dml;

    u_i1	qg_mode;

    u_longnat	qg_num[2];

    QRY_ARGV    *qg_argv;

    QRY_SPEC    *qg_tlist;
    QRY_SPEC    *qg_from;
    QRY_SPEC    *qg_where;
    QRY_SPEC    *qg_order;

    struct _qdesc   *qg_child;
    struct qgint    *qg_internal;

    nat             qg_bcnt;
    DB_DATA_VALUE   *qg_base[2];
} QDESC;

 # define	QI_BREAK	0101
 # define	QI_GET		0102
 # define	QI_START	0103
 # define	QI_NMASTER	0104

typedef struct qrytype
{
	nat		qr_zero;
	nat		qr_version;
	QRY_ARGV	*qr_argv;
	char		*qr_putparam;
	char		*qr_form;
	char		*qr_table;
	struct qrytype	*qr_child;
	QDESC		*qr_qdesc;
} QRY;

typedef struct
{
	char	*tf_name;
	char	*tf_column;
	i4	*tf_expr;
	i4	*tf_value;
} ABRTSTFLD;
typedef struct
{
	char		*pr_tlist;
	char		**pr_argv;
	ABRTSTFLD	*pr_tfld;
	QRY		*pr_qry;
} ABRTSOPRM;
typedef struct
{
	i4		abpvtype;
	char		*abpvvalue;
} OABRTSPV;
typedef struct
{
	i4		abpvtype;
	char		*abpvvalue;
	i4		abpvsize;
} ABRTSPV;
typedef struct
{
	nat		pr_zero;
	nat		pr_version;
	ABRTSOPRM	*pr_oldprm;
	ABRTSPV		*pr_actuals;
	char		**pr_formals;
	nat		pr_argcnt;
	nat		pr_rettype;
	i4		*pr_retvalue;
	nat		pr_flags;
} ABRTSPRM;

typedef struct
{
	char	*abprtable;
	char	*abprfld;
	char	*abprexpr;
	i4	abprtype;
} ABRTSPR;

typedef struct
{
    char	*qf_field;
    char	*qf_expr;
} ABRT_QFLD;

typedef struct
{
    nat		qu_type;
    char	*qu_form;
    nat		qu_count;
    ABRT_QFLD	*qu_elms;
} ABRT_QUAL;

char	*IIARtocToCstr();

#endif /* OSLHDR_H_INCLUDED */
