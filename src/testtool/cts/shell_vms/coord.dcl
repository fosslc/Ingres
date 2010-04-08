$ coord :== "$TOOLS_DIR:[bin]ctscoord.exe"
$!
$!	define the result file and save the coordinator's output
$!
$ define sys$output ing_tst:[output.output]'p4'_c.res
$ coord 'p1 'p2 'p3 'p5
$ deassign sys$output
$ exit
