
/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*::
** Name:	ii_sequence_value -	ABF RunTime Sequence Value Table.
*/
create table ii_sequence_value (
	sequence_key char(32) not null not default,
	sequence_value integer not null with default
	) with noduplicates;
modify ii_sequence_value to hash on sequence_key;
drop permit on ii_sequence_value all;

/*{
** Name:	_ii_sequence_value() -	Return Sequence Value for Table.
**
** Description:
**	Returns a sequence value (or range) for an input key given an increment
**	range.  This value is an integer in the range of 1 to 2^31 - 1, and if
**	it is to be a range, is the highest integer in that range.  A zero value
**	is an error and will be returned on error or when no more sequential
**	values can be allocated.
**
**	The UPDATE detects that the row exists and that another sequential value
**	(or range) can be allocated.  If the row for the key does not exist,
**	then the row will be added to the sequence value table.
**
** Input:
**	key	{char(32)}  The unique key identifying the sequence value.
**	inc	{integer}  The increment range.
**
** Returns:
**	{integer}	0 on error, otherwise, the sequence value.
**
** History:
**	06/89 (jhw) -- Written.
*/

create procedure _ii_sequence_value (
	key = char(32) not null,
	inc = integer not null
) = 
declare
	value = integer not null;
{
	if inc <= 0 then
		return 0
	endif;

	update ii_sequence_value set sequence_value = sequence_value + :inc
		where sequence_value > 0 and sequence_key = :key;
	if iierrornumber != 0 then
		return 0
	endif;
	if iirowcount = 0 then
		select value = sequence_value from ii_sequence_value
			where sequence_key = :key;
		if iierrornumber != 0 or iirowcount > 0 then
			return 0
		endif;
		insert into ii_sequence_value values ( :key, :inc );
		if iierrornumber != 0 or iirowcount = 0 then
			return 0
		endif
	endif;
	select value = sequence_value from ii_sequence_value
		where sequence_key = :key;
	if iierrornumber != 0 or iirowcount = 0 then
		return 0
	endif;
	return :value
}
grant execute on procedure _ii_sequence_value to public;
