/*
** Copyright (c) 2004 Ingres Corporation
*/
grant select,insert,update,delete on remotecmdinview to infopump;
\p\g

grant select,insert,update,delete on remotecmdoutview to infopump;
\p\g

grant select,insert,update,delete on remotecmdview to infopump;
\p\g

grant execute on procedure launchremotecmd to infopump;
\p\g

grant execute on procedure sendrmcmdinput to infopump;
\p\g

grant register, raise on dbevent rmcmdcmdend to infopump;
\p\g

grant register, raise on dbevent rmcmdnewcmd to infopump;
\p\g

grant register, raise on dbevent rmcmdnewinputline to infopump;
\p\g

grant register, raise on dbevent rmcmdnewoutputline to infopump;
\p\g

grant register, raise on dbevent rmcmdstp to infopump;
\p\g

