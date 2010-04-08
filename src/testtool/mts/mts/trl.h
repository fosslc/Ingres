/* 
**	TRL -
**	The trl is the test routine library data structure. This is
**	an array of addresses of the test routines. The test routine
**	number retrieved from the database acts as an index into this
**	table, and the routines themselves are called like:
**			trl[N].func();
**	This is because each element in the array is a structure, with
**	a field "func" containing the address of the routine in question.
**	The TRL array and routines are located in trl.qc.
*/

struct 	trl_routine {
	int	(*func)();
};

GLOBALREF struct trl_routine trl[];   


/*
**	rout_list is the data structure for the list of routines this 
**	slave will execute. Each list member contains the routine number
**	and a pointer to the next member in the list.
*/
struct rout_list {
	int	number;
	struct	rout_list	*next;
	};
	
