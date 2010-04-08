$ set noon
$cc/debug/opt=nodisjoint/define=("STANDALONE")/diagnostics dubconfig.c
$!cc/debug/opt=nodisjoint/define=("STANDALONE")/diagnostics dubcnfmain.c
$link/debug/nosysshr/exe=jpt_duf_tst:make_config.exe sys$input/option
symbol=INcleanup, 0
jpt_clf_lib:frontmain
jpt_duf_src:[dub]dubcnfmain.obj
jpt_duf_src:[dub]dubconfig.obj
jpt_duf_lib:dub.olb/lib
jpt_duf_lib:duu.olb/lib
develop4:[jupiter.cl.rplus.lib]cl/lib
$copy jpt_duf_tst:make_config.exe jpt_dmf_bin:make_config.exe
$del jpt_duf_src:[dub]dubconfig.obj.
