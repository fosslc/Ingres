/* psqmonth.c */
FUNC_EXTERN DB_STATUS psq_month(
	char *monthname,
	i4 *monthnum,
	DB_ERROR *err_blk);
FUNC_EXTERN i4 psq_monsize(
	i4 month,
	i4 year);
