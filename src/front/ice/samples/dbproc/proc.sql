drop table iceproctbl;
create table iceproctbl (col1 integer, col2 integer, col3 integer, col4 integer);\p\g

drop procedure iceprocins;
create procedure iceprocins (val1 integer, val2 integer,
                             val3 integer, val4 integer) as
declare
err integer not null;
begin
    insert into iceproctbl values (:val1, :val2, :val3, :val4);
    if iierrornumber > 0 then
        err = iierrornumber;
        return err;
    else
        return 0;
    endif
end;
\p\g
