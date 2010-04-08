/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Monitor internal memory pools for the specified server
**
**
## History:
##	22-May-95 (johna)
##		created from a number of standalone programs.
##
##      15-mar-2002     (mdf150302)
##              Break display after IOMON selection.
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
##      04-Sep-03       (mdf040903)                     
##              Commit after calling imp_complete_vnode 
##              Commit after calling imp_reset_domain
##      06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "imp.h"

exec sql include sqlca;
exec sql include 'impcommon.sc';

exec sql begin declare section;

extern 	int 	imp_server_diag;
extern 	int 	inFormsMode;
extern 	int 	imaSession;
extern 	int 	iidbdbSession;
extern 	int 	userSession;
extern 	char	*currentVnode;

exec sql end declare section;

int
impInternalMenu(server)
GcnEntry	*server;
{
	exec sql begin declare section;
	int	i;
	char	buf[60];
	exec sql end declare section;

	exec frs display imp_server_diag read;
	exec frs initialize;
	exec frs begin;

		exec sql repeated select dbmsinfo('ima_vnode') into :buf;
		sqlerr_check();
		if (strcmp(buf,currentVnode) != 0) {
			exec sql execute procedure imp_reset_domain;
			exec sql execute procedure 
				imp_set_domain(entry=:currentVnode);
		} else {
			exec sql execute procedure imp_reset_domain;
		}
		exec sql commit;  
/*
##mdf040903
*/

		strcpy(buf,"GCA");
		exec frs putform ( gca_field = :buf);
		strcpy(buf,"SCF");
		exec frs putform ( scf_field = :buf);
		strcpy(buf,"PSF");
		exec frs putform ( psf_field = :buf);
		strcpy(buf,"OPF");
		exec frs putform ( opf_field = :buf);
		strcpy(buf,"QEF");
		exec frs putform ( qef_field = :buf);
		strcpy(buf,"RDF");
		exec frs putform ( rdf_field = :buf);
		strcpy(buf,"QSF");
		exec frs putform ( qsf_field = :buf);
		strcpy(buf,"ADF");
		exec frs putform ( adf_field = :buf);
		strcpy(buf,"SXF");
		exec frs putform ( sxf_field = :buf);
		strcpy(buf,"GWF");
		exec frs putform ( gwf_field = :buf);
		strcpy(buf,"DMF");
		exec frs putform ( dmf_field = :buf);
		strcpy(buf,"DI (io)");
		exec frs putform ( di_field = :buf);

	exec frs end;

	exec frs activate menuitem 'Select', frskey4;
	exec frs begin;
		exec frs inquire_frs field imp_server_diag (:buf = 'name');
		if (strcmp(buf,"scf_field") == 0) {
			displayServerSessions(server);
		} else if (strcmp(buf,"qsf_field") == 0) {
			processQsfSummary(server);
		} else if (strcmp(buf,"rdf_field") == 0) {
/* debugging stuff
		strcpy(buf,server);
 exec frs message :buf with style = popup;
*/
			processRdfSummary(server);
		} else if (strcmp(buf,"di_field") == 0) {
			exec frs display submenu;

				exec frs activate menuitem 'I/O by session';
				exec frs begin;
					processIoMonitor(server);
					exec frs breakdisplay; 
/*
##mdf150302
*/
				exec frs end;

				exec frs activate menuitem 'Slave Info';
				exec frs begin;
					processSlaveInfo(server);
					exec frs breakdisplay; 
/*
##mdf150302
*/
				exec frs end;

				exec frs activate menuitem 'End', frskey3;
				exec frs begin;
					exec frs breakdisplay;
				exec frs end;

		} else if (strcmp(buf,"dmf_field") == 0) {
			processDmfSummary(server);
		} else {
			exec frs message 'Sorry - no module written (yet)'
				with style = popup;
		}

	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs breakdisplay;
	exec frs end;
	exec sql commit;   
/*
##mdf040903
*/
	exec sql execute procedure imp_complete_vnode;
	return(TRUE);
}

