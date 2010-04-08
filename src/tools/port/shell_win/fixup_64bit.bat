@echo off
setlocal

set ING_SRC=Z:\RMAINNT64
rm %II_SYSTEM%\ingres\files\config.dat %II_SYSTEM%\ingres\files\symbol.tbl
call setenv.bat

REM
REM First, re-run chinst
REM
chinst alt %ING_SRC%\front\misc\chsfiles\alt.chs
chinst arabic %ING_SRC%\front\misc\chsfiles\arabic.chs
chinst chineses %ING_SRC%\front\misc\chsfiles\chineses.chs
chinst csgb2312 %ING_SRC%\front\misc\chsfiles\csgb2312.chs
chinst csgbk %ING_SRC%\front\misc\chsfiles\csgbk.chs
chinst chineset %ING_SRC%\front\misc\chsfiles\chineset.chs
chinst chtbig5 %ING_SRC%\front\misc\chsfiles\chtbig5.chs
chinst chteuc %ING_SRC%\front\misc\chsfiles\chteuc.chs
chinst chthp %ING_SRC%\front\misc\chsfiles\chthp.chs
chinst cw %ING_SRC%\front\misc\chsfiles\cw.chs
chinst decmulti %ING_SRC%\front\misc\chsfiles\decmulti.chs
chinst dosasmo %ING_SRC%\front\misc\chsfiles\dosasmo.chs
chinst elot437 %ING_SRC%\front\misc\chsfiles\elot437.chs
chinst greek %ING_SRC%\front\misc\chsfiles\greek.chs
chinst hebrew %ING_SRC%\front\misc\chsfiles\hebrew.chs
chinst hproman8 %ING_SRC%\front\misc\chsfiles\hproman8.chs
chinst ibmpc437 %ING_SRC%\front\misc\chsfiles\ibmpc437.chs
chinst ibmpc850 %ING_SRC%\front\misc\chsfiles\ibmpc850.chs
chinst ibmpc866 %ING_SRC%\front\misc\chsfiles\ibmpc866.chs
chinst iso88591 %ING_SRC%\front\misc\chsfiles\iso88591.chs
chinst iso88592 %ING_SRC%\front\misc\chsfiles\iso88592.chs
chinst iso88595 %ING_SRC%\front\misc\chsfiles\iso88595.chs
chinst iso88599 %ING_SRC%\front\misc\chsfiles\iso88599.chs
chinst kanjieuc %ING_SRC%\front\misc\chsfiles\kanjieuc.chs
chinst korean %ING_SRC%\front\misc\chsfiles\korean.chs
chinst pc857 %ING_SRC%\front\misc\chsfiles\pc857.chs
chinst pchebrew %ING_SRC%\front\misc\chsfiles\pchebrew.chs
chinst shiftjis %ING_SRC%\front\misc\chsfiles\shiftjis.chs
chinst slav852 %ING_SRC%\front\misc\chsfiles\slav852.chs
chinst thai %ING_SRC%\front\misc\chsfiles\thai.chs
chinst utf8 %ING_SRC%\front\misc\chsfiles\utf8.chs
chinst warabic %ING_SRC%\front\misc\chsfiles\warabic.chs
chinst whebrew %ING_SRC%\front\misc\chsfiles\whebrew.chs
chinst win1250 %ING_SRC%\front\misc\chsfiles\win1250.chs
chinst wthai %ING_SRC%\front\misc\chsfiles\wthai.chs

REM
REM Now, re-make the .mnx files
REM
set MESSAGE_LIST=eradf.msg eraif.msg erclf.msg erdmf.msg erduf.msg ergcf.msg erglf.msg ergwf.msg eropf.msg erpsf.msg erqef.msg erqsf.msg errdf.msg errqf.msg erscf.msg ersxf.msg ertpf.msg erulf.msg erurs.msg erusf.msg erddf.msg erwsf.msg erodbc.msg erjdbc.msg erdo.msg ereo.msg erom.msg erpw.msg erse.msg ersm.msg erw4.msg erwc.msg erwn.msg erwr.msg fecat.msg

cd /d %ING_SRC%\common\hdr\hdr
ercomp -qsqlstate.h -f %MESSAGE_LIST%
cp fast_v4.mnx %II_SYSTEM%\ingres\files\english
ercomp -qsqlstate.h -s %MESSAGE_LIST%
cp slow_v4.mnx %II_SYSTEM%\ingres\files\english
c:

REM
REM Now, make rtiforms.fnx again
REM
rm -f rtiforms.fnx
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abaddtop.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abcompcr.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abcompsx.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abdeflts.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abedlock.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfcnsed.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfconf.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfconst.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfcpcat.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfcpdet.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfcreat.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfgcnsf.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfglobs.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfgrafd.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfimage.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfprocd.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfqbfd.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfrec.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfrecd.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfrepwd.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abfuserd.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abglobsx.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\ablnkdsp.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\ablokadm.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\ablokdet.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abloktr.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abpartcr.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abpaswrd.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abreccr.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abf\abtyppik.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abfrt\afrblank.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abfrt\afrnproc.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abfrt\afrtsdmp.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\abfrt\afrunfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\interp\itieffo.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\interp\ititffo.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\newfr.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\newlup.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\newmi.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\newvq.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqapprep.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqcmper2.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqcmperr.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqedectf.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqhlparm.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqlcreat.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqlisfil.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqlocals.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqlocks.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqmarkfm.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqparms.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqpcreat.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqprocs.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqqparts.frm
formindex rtiforms.fnx %ING_SRC%\front\abf\vq\vqrecapp.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\catamenu.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\ctrlmenu.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\dbacc.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\dbfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\dbtblfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\extdb.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\extenddb.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\extens.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\icdbpacc.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\icinsacc.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\location.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\loclist.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\locnew.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\lpick.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\nlocatio.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\prvfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\pwdfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\usrfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingcntrl\usrtbfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\misc\ingmenu\imtopfr.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqbfjoin.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqbfops.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqbftbls.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqchange.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqcrjoin.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqexec.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqjdup.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqnewjoi.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqqfnms.frm
formindex rtiforms.fnx %ING_SRC%\front\qbf\qbf\mqstart.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\break.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\coptions.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rbfabf.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfadcols.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfagg.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfarchv.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfbrkopt.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfchcols.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfcols.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rffile.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfgetnm.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfiidet.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfiisave.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfindop.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfnwpag.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfprint.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfprompt.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rfvar.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\rlayout.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rbf\roptions.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\dupfld.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\tfcreate.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\valerr.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfabcfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfboxatr.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfcat.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfcfdup.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfcfjdef.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfcfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfcftbl.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfdetail.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vffrmatr.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfqbfcat.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfrulfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfsave.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vftblprm.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vftrmatr.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vfvalder.frm
formindex rtiforms.fnx %ING_SRC%\front\report\rfvifred\vifattr.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\bridgeprots.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\browser.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\c2audit.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\cfgcache.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\dblist.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\dmfdep.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\dmfind.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\gendep.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\genind.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\lgraph.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\netprots.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\startup.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\txlogs.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\txlog2.frm
formindex rtiforms.fnx %ING_SRC%\front\st\config\secmechs.frm
formindex rtiforms.fnx %ING_SRC%\front\st\netutil\connedit.frm
formindex rtiforms.fnx %ING_SRC%\front\st\netutil\nusvrcntrl.frm
formindex rtiforms.fnx %ING_SRC%\front\st\netutil\attribform.frm
formindex rtiforms.fnx %ING_SRC%\front\st\netutil\attribedit.frm
formindex rtiforms.fnx %ING_SRC%\front\st\netutil\loginform.frm
formindex rtiforms.fnx %ING_SRC%\front\st\netutil\logpassedit.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_f1.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_f2.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_f3.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p11.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p12.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p13.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p14.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p15.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p16.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p2.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p3.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p6.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p7.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p7_1.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p8.frm
formindex rtiforms.fnx %ING_SRC%\front\st\starview\dut_p9.frm
formindex rtiforms.fnx %ING_SRC%\front\tm\fstm\fserrfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\tm\fstm\quelcmds.frm
formindex rtiforms.fnx %ING_SRC%\front\tools\erconvert\mcconv.frm
formindex rtiforms.fnx %ING_SRC%\front\tools\erconvert\mcmsg.frm
formindex rtiforms.fnx %ING_SRC%\front\tools\erconvert\mctbl.frm
formindex rtiforms.fnx %ING_SRC%\front\tools\erconvert\mctop.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\oo\iicat.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\oo\iidetail.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\oo\iisave.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\oo\oolngrem.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\tblutil\tucreate.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\tblutil\tudefprm.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\tblutil\tuexam.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\tblutil\tutblprm.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\tblutil\tutopfr.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\uf\help2frm.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\uf\helpform.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\uf\helptype.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\uf\helpvmax.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\uf\keysform.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\uf\uffile.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\uf\uflstfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\utils\uf\ufprint.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\dupfld.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\tfcreate.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\valerr.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfabcfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfboxatr.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfcat.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfcfdup.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfcfjdef.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfcfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfcftbl.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfdetail.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vffrmatr.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfrulfrm.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfsave.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vftblprm.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vftrmatr.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vfvalder.frm
formindex rtiforms.fnx %ING_SRC%\front\vifred\vifred\vifattr.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmcddact.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmcdddbs.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmcdddet.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmcddpth.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmcddsum.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmcddtbl.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmcfgmnu.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmdbdetl.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmdbsumm.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmdisply.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmedtcfg.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmevents.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmlocldb.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmmailct.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmmainmn.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmmonitr.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmmovcfg.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmreport.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmshwasg.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmsndevt.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmtbldet.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmtblint.frm
formindex rtiforms.fnx %ING_SRC%\front\rep\repmgr\rmtblsum.frm
copy rtiforms.fnx "%II_SYSTEM%\ingres\files\english"

REM
REM Re-make the collation sequences
REM
aducompile %ING_SRC%\common\adf\adm\multi.dsc multi
aducompile %ING_SRC%\common\adf\adm\spanish.dsc spanish
aducompile %ING_SRC%\common\adf\adm\udefault.uce udefault -u

REM
REM Finally, re-make the timezone files
REM
cd /d %ING_SRC%\gl\glf\tmgl
iizic -d "%II_SYSTEM%\ingres\files\zoneinfo" %ING_SRC%\gl\glf\tmgl\iiworld.tz
c:

endlocal
