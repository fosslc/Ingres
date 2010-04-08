
	rpclean: Alternative for Replicator arcclean
	--------------------------------------------

This SIG directory contains the rpclean executable. This is an
alternative for the arcclean utility and was developed by an
Ingres user. The principal advantage of rpclean over arcclean is
that rpclean can process multiple tables concurrently (up to 24).
In addition, the remodification of shadow, archive and queue
tables at end of processing is optional. However, rpclean has the
following known limitations:

1. rpclean does not support the -u flag of arcclean which allows
   someone other than the DBA to run the utility.
2. Tables with spaces and other characters requiring delimited
   identifier handling in the key column names are not supported.


Syntax
------

    rpclean -d dbname [-b before_time] [-n concurrency] [-f] [-v]

The parameters of the rpclean command are explained in the
following table:

    -----------------------------------------------------------
     Parameter      Description
    -----------------------------------------------------------
     dbname         The name of the database to be cleaned.
                    This must be a full peer node of a
                    replication scheme.
    -----------------------------------------------------------
     before_time    Delete all old shadow table entries before
                    this time. This time should be after the
                    last checkpoint. If this not specified, the
                    current date and time are used as default.
    -----------------------------------------------------------
     concurrency    The number of tables to clean concurrently.
                    The default is 10. The maximum is 24.
    -----------------------------------------------------------
     -f             Run in full mode. This remodifies the
                    Replicator tables after they have been
                    cleaned. This may slow down applications
                    connected to the database while running,
                    but will result in a faster database after
                    the clean.
    -----------------------------------------------------------
     -v             Print verbose information during the clean.
    -----------------------------------------------------------
