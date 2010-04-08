/*
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <raat.h>
exec sql include sqlca;

/*
** Name: raattest.sc - to build testbed for RAAT 
**
** Description:
**	This routine is currently used to test development of the Ingres
**	Row At A Time interface. It will eventually form the genesis
**	of the run_all test for RAAT
**
**	This test must be run from the 'ingres' account, failure to do
**	so may result in GPF's.
**
** History:
**	23-may-95 (stephenb)
**	    First commented, code was originally written when RAAT was
**	    developed, along with lewda02.
**	18-jul-95 (emmag)
**	    Changed user name to ingres from lewda02.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-sep-2000 (somsa01)
**	    We need to include compat.h .
*/

/*
PROGRAM = raattest

NEEDLIBS = LIBINGRES
*/
int
main()
{
    exec sql begin declare section;
	int		count;
    	int		high_key = 4000; 
    	int		low_key = 3990;
    exec sql end declare section;

    RAAT_CB		raat_cb;
    int			status;
    RAAT_TABLE_HANDLE	*table1, *index1;
    int			i, j;
    struct {
	int		c1val;
	int		c2val;
	char		c3val[41];
    } values = {0, 0, "1234567890123456789012345678901234567890"};
    int			low_base = 200;
    int			high_base = 211;
    int			new_low_key = low_key + 5000;
    int			new_high_key = high_key + 5000;
    int			new_low_base = low_base + 5000;
    int			new_high_base = high_base + 5000;
    RAAT_KEY_ATTR	key_attr[2];
    RAAT_KEY		key;

    /*
    ** Start session using RAAT, database is hard coded for now
    */
    raat_cb.dbname = "lewdadb";
    status = IIraat_call(RAAT_SESS_START, &raat_cb);

    /*
    ** setup table t1 using sql
    */
    exec sql drop table t1;

    exec sql commit;

    exec sql whenever sqlerror stop;

    exec sql create table t1 (c1 integer not null not default,
	c2 integer not null not default,
	c3 char(40) not null not default);

    exec sql modify t1 to btree on c1;

    exec sql create index i1 on t1(c2);

    exec sql commit;

    /*
    ** Insert some data in t1 using RAAT, test out locking options
    ** at this time. Data in collumn 2 is descending making the secondary
    ** index work in reverse scanning order.
    */
    status = IIraat_call(RAAT_TX_BEGIN, &raat_cb);

    raat_cb.tabname = "ingres.t1";
    raat_cb.flag = RAAT_LK_NL;
    status = IIraat_call(RAAT_TABLE_OPEN, &raat_cb);

    raat_cb.flag = RAAT_LK_XP;
    status = IIraat_call(RAAT_TABLE_LOCK, &raat_cb);

    raat_cb.record = (char *)&values;
    for (values.c1val = 0, values.c2val = 4999;
	 values.c1val < 5000; values.c1val++, values.c2val--)
    {
	status = IIraat_call(RAAT_RECORD_PUT, &raat_cb);
    }

    status = IIraat_call(RAAT_TABLE_CLOSE, &raat_cb);

    status = IIraat_call(RAAT_TX_COMMIT, &raat_cb);

    /*
    ** now retrieve some data from t1 by key, using the secondary index
    */
    status = IIraat_call(RAAT_TX_BEGIN, &raat_cb);

    raat_cb.tabname = "t1";
    raat_cb.flag = RAAT_LK_XP;
    status = IIraat_call(RAAT_TABLE_OPEN, &raat_cb);
    table1 = raat_cb.table_handle;

    raat_cb.tabname = "i1";
    status = IIraat_call(RAAT_TABLE_OPEN, &raat_cb);
    index1 = raat_cb.table_handle;

    /*
    ** fill out key fields, and then position on secondary index,
    ** key values are hard coded for now, we will retrieve all index
    ** rows between low_key and high_key.
    */
    key_attr[0].attr_number = 1;
    key_attr[0].attr_operator = RAAT_GTE;
    key_attr[0].attr_value = (char *)&low_key;
    key_attr[1].attr_number = 1;
    key_attr[1].attr_operator = RAAT_LTE;
    key_attr[1].attr_value = (char *)&high_key;
    key.attr_count = 2;
    key.key_attr = key_attr;
    raat_cb.key = &key;
    status = IIraat_call(RAAT_RECORD_POSITION, &raat_cb);

    /*
    ** allocate space for records
    */
    raat_cb.record = (char *)malloc(table1->table_info.tbl_width);
    /*
    ** loop through the retrieve until we have no more rows to get
    ** (error 196693)
    */
    for (; ;)
    {
	/*
	** first get index record, ask for tidp in recnum field so that we
	** can use it to retrieve base table record, for now we'll print out
	** the index record so we can see what we retrieved
	*/
        raat_cb.flag = (RAAT_REC_NEXT | RAAT_BAS_RECNUM);
	raat_cb.table_handle = index1;
	status = IIraat_call(RAAT_RECORD_GET, &raat_cb);
	if (status && raat_cb.err_code == 196693)
	    break;
	printf("\n%d %d", *((int *)raat_cb.record),
	    *((int *)((char *)raat_cb.record + 4)));
	/*
	** now use the record number to retrieve the corresponding base table
	** record, then print it out
	*/
	raat_cb.table_handle = table1;
	raat_cb.flag = RAAT_REC_BYNUM;
	status = IIraat_call(RAAT_RECORD_GET, &raat_cb);
	printf("\n%d %d %.*s\n", *((int *)raat_cb.record), 
	    *((int *)((char *)raat_cb.record + 4)), 
	    raat_cb.table_handle->table_info.tbl_width - 8, raat_cb.record + 8);
	/*
	** after retrieveing the record we will add 5000 to the value in
	** collumn 2 and then replace it in the database
	** NOTE: this does not affect the current get stream, which will
	** still get all records that qualified the original key value
	*/
	*((int *)((char *)raat_cb.record + 4)) += 5000;
	status = IIraat_call(RAAT_RECORD_REPLACE, &raat_cb);
    }
    /*
    ** close the tables and commit
    */
    status = IIraat_call(RAAT_TABLE_CLOSE, &raat_cb);
    raat_cb.table_handle = table1;
    status = IIraat_call(RAAT_TABLE_CLOSE, &raat_cb);
    status = IIraat_call(RAAT_TX_COMMIT, &raat_cb);


    /*
    ** perform the same retrieve and replace using the base table
    ** (primary) index
    */
    raat_cb.tabname = "t1";
    status = IIraat_call(RAAT_TX_BEGIN, &raat_cb);
    status = IIraat_call(RAAT_TABLE_OPEN, &raat_cb);

    key_attr[0].attr_number = 1;
    key_attr[0].attr_operator = RAAT_GTE;
    key_attr[0].attr_value = (char *)&low_base;
    key_attr[1].attr_number = 1;
    key_attr[1].attr_operator = RAAT_LTE;
    key_attr[1].attr_value = (char *)&high_base;
    key.attr_count = 2;
    key.key_attr = key_attr;
    raat_cb.key = &key;
    status = IIraat_call(RAAT_RECORD_POSITION, &raat_cb);
    raat_cb.flag = RAAT_REC_NEXT;
    for (;;)
    {
        status = IIraat_call(RAAT_RECORD_GET, &raat_cb);	
	if (status && raat_cb.err_code == 196693)
	    break;
	printf("\n%d %d %.*s\n", *((int *)raat_cb.record), 
	    *((int *)((char *)raat_cb.record + 4)), 
	    raat_cb.table_handle->table_info.tbl_width - 8, raat_cb.record + 8);
	*((int *)raat_cb.record) += 5000;
        status = IIraat_call(RAAT_RECORD_REPLACE, &raat_cb);
    }

    /*
    ** Now check that the records were replaced by getting using the new
    ** key value
    */
    key_attr[0].attr_number = 1;
    key_attr[0].attr_operator = RAAT_GTE;
    key_attr[0].attr_value = (char *)&new_low_base;
    key_attr[1].attr_number = 1;
    key_attr[1].attr_operator = RAAT_LTE;
    key_attr[1].attr_value = (char *)&new_high_base;
    key.attr_count = 2;
    key.key_attr = key_attr;
    raat_cb.key = &key;
    status = IIraat_call(RAAT_RECORD_POSITION, &raat_cb);
    raat_cb.flag = RAAT_REC_NEXT;
    for (;;)
    {
        status = IIraat_call(RAAT_RECORD_GET, &raat_cb);	
	if (status && raat_cb.err_code == 196693)
	    break;
	printf("\n%d %d %.*s\n", *((int *)raat_cb.record), 
	    *((int *)((char *)raat_cb.record + 4)), 
	    raat_cb.table_handle->table_info.tbl_width - 8, raat_cb.record + 8);
    }

    /*
    ** now re-position to the beggining of the current range and get the first
    ** record in the key range again
    */
    raat_cb.flag = RAAT_REC_REPOS;
    status = IIraat_call(RAAT_RECORD_POSITION, &raat_cb);
    raat_cb.flag = RAAT_REC_NEXT;
    status = IIraat_call(RAAT_RECORD_GET, &raat_cb);	
    printf("\n%d %d %.*s\n", *((int *)raat_cb.record), 
	*((int *)((char *)raat_cb.record + 4)), 
	raat_cb.table_handle->table_info.tbl_width - 8, raat_cb.record + 8);

    /*
    ** now re-position to the begining of the table and get the first 
    ** record in the table
    */
    raat_cb.flag = RAAT_REC_FIRST;
    status = IIraat_call(RAAT_RECORD_POSITION, &raat_cb);
    raat_cb.flag = RAAT_REC_NEXT;
    status = IIraat_call(RAAT_RECORD_GET, &raat_cb);	
    printf("\n%d %d %.*s\n", *((int *)raat_cb.record), 
	*((int *)((char *)raat_cb.record + 4)), 
	raat_cb.table_handle->table_info.tbl_width - 8, raat_cb.record + 8);

    /*
    ** now re-position to the end of the table and get the last two records
    ** in the table, this will also test the get previous function
    */
    raat_cb.flag = RAAT_REC_LAST;
    status = IIraat_call(RAAT_RECORD_POSITION, &raat_cb);
    raat_cb.flag = RAAT_REC_PREV;
    status = IIraat_call(RAAT_RECORD_GET, &raat_cb);	
    printf("\n%d %d %.*s\n", *((int *)raat_cb.record), 
	*((int *)((char *)raat_cb.record + 4)), 
	raat_cb.table_handle->table_info.tbl_width - 8, raat_cb.record + 8);
    status = IIraat_call(RAAT_RECORD_GET, &raat_cb);	
    printf("\n%d %d %.*s\n", *((int *)raat_cb.record), 
	*((int *)((char *)raat_cb.record + 4)), 
	raat_cb.table_handle->table_info.tbl_width - 8, raat_cb.record + 8);

    /*
    ** finally close up and commit
    */
    status = IIraat_call(RAAT_TABLE_CLOSE, &raat_cb);

    status = IIraat_call(RAAT_TX_COMMIT, &raat_cb);

    status = IIraat_call(RAAT_SESS_END, &raat_cb);
}
