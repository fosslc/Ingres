$! Copyright (c) 2004 Ingres Corporation
$!
$! IIJOBDEL.COM -- Deassign INGRES Job logicals.
$!
$ saved_message_format = f$environment( "MESSAGE" ) ! save message format
$ set message/notext/nofacility/noseverity/noidentification
$ DEFINE/job/super ii_config ii_system:[ingres.files]
$ set message 'saved_message_format'
$!
$ iijobdef := $ ii_system:[ingres.utility]iijobdef.exe
$ set noon
$ iijobdef 1
