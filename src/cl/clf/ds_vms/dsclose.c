/*
** static char Sccsid[] = "%W%	%G%";
*/

# include	<compat.h>
#include    <gl.h>
# include	<si.h>
# include	<ds.h>

DSclose(sh_desc)
SH_DESC	*sh_desc;
{
	if (sh_desc->sh_type == IS_FILE) {
		SIclose(sh_desc->sh_reg.file.fp);
		sh_desc->sh_reg.file.fp = NULL;
	}
}
