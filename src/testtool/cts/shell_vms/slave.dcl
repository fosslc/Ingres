$ slave :== "$TOOLS_DIR:[bin]ctsslave.exe"
$!
$!	define result file and save the slave's output.
$!
$ define sys$output ing_tst:[output.output]'p4'_'p3'.res
$ slave 'p1 'p2 'p3 'p5
$ deassign sys$output 
$ exit
