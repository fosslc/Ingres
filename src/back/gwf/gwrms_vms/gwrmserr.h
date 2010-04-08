/*
**  GWRMSERR.H
**
**  RMS Gateway errors related to datatype conversion and mapping errors.
**
**  History:
**	15-jun-90 (linda)
**	    Created, brought out from other headers to resolve compilation/
**	    linking problems.
**      18-may-92 (schang)
**          merge in this message.
**	    4-feb-91 (linda)
**	        Add error message for g_float & h_float conversion errors.
**      20-apr-92 (schang)
**          add  #define E_GW702C_DATA_INCOMPAT (E_GWF_MASK + 0x702c)
*/

#define		E_GWF_MASK			(DB_GWF_ID << 16)
#define		E_GW7000_INTEGER_OVF		(E_GWF_MASK + 0x7000)
#define		E_GW7001_FLOAT_OVF		(E_GWF_MASK + 0x7001)
#define		E_GW7002_FLOAT_UND		(E_GWF_MASK + 0x7002)
#define		E_GW7003_DATE_OUT_OF_RANGE	(E_GWF_MASK + 0x7003)
#define		E_GW7004_MONEY_OVF		(E_GWF_MASK + 0x7004)
#define		E_GW7005_MONEY_UND		(E_GWF_MASK + 0x7005)
#define		E_GW7006_ING_TO_VAX_DATE	(E_GWF_MASK + 0x7006)
#define		E_GW7010_BAD_FLOAT_VAL		(E_GWF_MASK + 0x7010)
#define		E_GW7011_BAD_VAXDATE_VAL	(E_GWF_MASK + 0x7011)
#define		E_GW7012_BAD_STR_TO_INT		(E_GWF_MASK + 0x7012)
#define		E_GW7013_BAD_NUM_TO_INT		(E_GWF_MASK + 0x7013)
#define		E_GW7014_BAD_STR_TO_VDATE	(E_GWF_MASK + 0x7014)
#define		E_GW7015_DECIMAL_OVF		(E_GWF_MASK + 0x7015)
#define		E_GW7016_BAD_STR_TO_DEC		(E_GWF_MASK + 0x7016)
#define		E_GW7017_BAD_NUM_TO_STR		(E_GWF_MASK + 0x7017)
#define		E_GW7020_BAD_STR_TO_FLT		(E_GWF_MASK + 0x7020)
#define		E_GW7021_BAD_NUM_TO_FLT		(E_GWF_MASK + 0x7021)
#define		E_GW7022_BAD_INT_TO_DATE	(E_GWF_MASK + 0x7022)
#define		E_GW7023_BAD_STR_TO_DATE	(E_GWF_MASK + 0x7023)
#define		E_GW7024_BAD_STR_TO_MNY		(E_GWF_MASK + 0x7024)
#define		E_GW7025_BAD_NUM_TO_MNY		(E_GWF_MASK + 0x7025)
#define		E_GW7026_BAD_FLT_TO_MNY		(E_GWF_MASK + 0x7026)
#define		E_GW7027_MNY_STR_TOO_LONG	(E_GWF_MASK + 0x7027)
#define		E_GW7028_DATE_IS_INTERVAL	(E_GWF_MASK + 0x7028)
#define		E_GW7029_YEAR_NOT_1900		(E_GWF_MASK + 0x7029)
#define		E_GW702A_FLT_STR_TOO_LONG	(E_GWF_MASK + 0x702a)
#define		E_GW702B_CANT_DO_G_OR_H		(E_GWF_MASK + 0x702b)
#define         E_GW702C_DATA_INCOMPAT          (E_GWF_MASK + 0x702c)
#define		E_GW7050_BAD_ERLOOKUP		(E_GWF_MASK + 0x7050)
#define		E_GW7100_NOCVT_AT_STARTUP	(E_GWF_MASK + 0x7100)
#define		E_GW7999_INTERNAL_ERROR		(E_GWF_MASK + 0x7999)
