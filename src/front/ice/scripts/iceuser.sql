/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Add a default database user.  The icedbuser should be an Ingres user.
*/
delete from ice_dbusers where id=1 and name='icedbuser';\p\g
insert into ice_dbusers values (1, 'icedefdb', 'icedbuser', '', '');\p\g
