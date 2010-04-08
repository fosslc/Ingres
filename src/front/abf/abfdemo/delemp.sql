/* Delete an Employee and all their tasks from the database */

drop procedure del_emp;
\p\g

create procedure del_emp (empname=c15) =
begin
	message  'Deleting employee . . .';
	delete from tasks where name = :empname;
	delete from emp where name = :empname;
	commit;
end;
\p\g
