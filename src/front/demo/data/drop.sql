/*
** Drop tables and other database objects from demodb database so then
** can be drop without error.
**
## History:
##	26-Feb-2007 (hanje04)
##	    SIR 117784
##	    created.
##	10-May-2007 (drivi01)
##	    BUG 118304
##	    Added procedures to the list of objects
##	    to be dropped from demodb.
*/

\sql
set autocommit on
\p\g
set nojournaling
\p\g
\sql
set session with privileges=all
\p\g
on error continue
\p\g

/* tables */
drop table airline
\p\g
drop table airport
\p\g
drop table country
\p\g
drop table flight_day
\p\g
drop table route
\p\g
drop table tz
\p\g
drop table user_profile
\p\g
drop table version
\p\g

/* sequences */
drop sequence al_id
\p\g
drop sequence al_tmp
\p\g
drop sequence ap_id
\p\g
drop sequence ct_id
\p\g
drop sequence fl_id
\p\g
drop sequence ia_id
\p\g
drop sequence rt_id
\p\g
drop sequence tz_id
\p\g
drop sequence up_id
\p\g
drop sequence ver_id
\p\g

/* views */
drop view  full_route
\p\g

/* procedures */
drop procedure get_airlines
\p\g
drop procedure get_airports
\p\g
drop procedure get_my_airlines
\p\g
drop procedure get_my_airports
\p\g
drop procedure search_route
\p\g