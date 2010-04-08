/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:    icesample.sql
**
** Description:
**      Script for setting up the samples as public documents readable from
**      the HTTP directory structure.
**
## History:
##      23-Oct-98 (fanra01)
##          Add history.
##          Removed dumplicate application name.
*/
/*
** Create a report location for tutorial and name it
*/
delete from ice_locations
    where id=4 and name='icereport' and type=3 and path='ice/samples/report';\p\g
insert into ice_locations values (4, 'icereport', 3, 'ice/samples/report', '');\p\g

/*
** Create a procedure location for tutorial and name it
*/
delete from ice_locations
    where id=5 and name='icedbproc' and type=3 and path='ice/samples/dbproc';\p\g
insert into ice_locations values (5, 'icedbproc', 3, 'ice/samples/dbproc', '');\p\g

/*
** Create an application location for tutorial and name it
*/
delete from ice_locations
    where id=6 and name='iceapp' and type=3 and path='ice/samples/app';\p\g
insert into ice_locations values (6, 'iceapp', 3, 'ice/samples/app', '');\p\g

/*
** Create a query location for tutorial and name it
*/
delete from ice_locations
    where id=7 and name='icequery' and type=3 and path='ice/samples/query';\p\g
insert into ice_locations values (7, 'icequery', 3, 'ice/samples/query', '');\p\g
