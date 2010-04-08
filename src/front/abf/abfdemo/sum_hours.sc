/*
** Copyright (c) 2004 Ingres Corporation
*/
/**
 ** Sum_hours.sc: Sum the total hours in the tasktable.hours column in emptasks.
 ** 
 ** Arguments:
 **    hourly_rate: the current hourly rate;
 ** 
 ** Side Effects:
 **    Computes and displays total hours and cost in emptasks form.
 **    Changes tot_hours to blink if over 40 hours assigned.
 ** 
 ** Returns:
 **    Nothing.
 **
 ** History:
 **    10-sep-93 (kchin)
 **		hours and tot_hours should be declared as int rather than
 **		long.  int and long data types are different on 64-bit
 **		platform like Alpha OSF/1, in this case long is 64-bit, 
 **		which is no longer similar in size to 32-bit int, thus 
 **		mixing them will result in error, moreover, 64-bit integer
 **		is not supported by SQL/QUEL yet.
 **/

void
sum_hours(hourly_rate)
double		hourly_rate;
{
exec sql 	begin declare section;
		int	hours, tot_hours;
		double tot_cost;
		short	ni, overhours, state;
exec sql 	end declare section;

		tot_hours = 0; overhours= 0;
exec frs 	unloadtable emptasks tasktable(:hours:ni=hours, :state=_STATE);
exec frs 	begin;
	    		if (state != 4 && state !=0 && ni != -1) 
	        	tot_hours = tot_hours + hours;
exec frs 	end;
		tot_cost = tot_hours * hourly_rate;
exec frs 	putform emptasks (tot_hours=:tot_hours,tot_cost=:tot_cost);

		if (tot_hours > 40)  
			overhours=1; 
exec frs 	set_frs field emptasks (blink(tot_hours) = :overhours);
		return;
}
