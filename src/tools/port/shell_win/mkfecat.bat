@echo off
REM A rather crude attempt at duplicating the functionality provided
REM by the shell script mkfecat.sh   This needs to be redone when time 
REM permits, so that it goes through the front source, searching for 
REM msg files, rather than hardcoding them as I have done for now.
REM
REM History:
REM 19-may-95 (emmag)
REM     Created.
REM 13-jul-95 (canor01)
REM     Removed duplicate ervf.msg
REM 07-dec-95 (harpa06)
REM	Added erwb.msg
REM 29-oct-1996 (canor01)
REM     ermf.msg moved.
REM 06-jun-1997 (harpa06)
REM     Added erwa.msg
REM 06-jun-1997 (harpa06)
REM     Added erwa.msg
REM 25-mar-98 (mcgem01)
REM	Added erge.msg, ergu.msg and ergx.msg for the gateways.
REM 08-jun-1999 (somsa01)
REM	Added errm.msg for RMCMD.
REM 01-jul-1999 (somsa01)
REM	Changed errm.msg to erre.msg for RMCMD.
REM 17-Jan-2000 (fanra01)
REM     Add erwu.msg for ice utilities.
REM 12-jun-2001 (somsa01)
REM	Added erxml.msg .
REM 24-jun-2003 (somsa01)
REM	Removed ere5.msg.
REM 21-jul-2004 (somsa01)
REM	Removed front!graphics for Windows.
REM 15-Nov-2004 (drivi01)
REM	Removed gateway and front!abf!oldosl.
REM 28-Jun-2007 (hanal04) Bug 118589
REM     Added erctou.msg for convtouni messages.
REM 28-Jan-2010 (horda03) Bug 121811
REM     Added erdg,msg
REM	
REM
REM NOTE: Any new message files added to %ING_SRC%\FRONT will need to
REM be added here.

if exist %ING_SRC%\common\hdr\hdr\fe.cat.msg del %ING_SRC%\common\hdr\hdr\fe.cat.msg

type %ING_SRC%\front\abf\abf\erab.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\abf\erib.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\abfimage\erai.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\abfrt\eras.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\cautil\erac.msg  >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\codegen\ercg.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\copyapp\erca.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\hdr\erar.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\hdr\erfg.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\hdr\erit.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\hdr\eros.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\iaom\eram.msg    >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\ilrf\eror.msg    >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\impexp\erie.msg  >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
REM type %ING_SRC%\front\abf\oldosl\ero2.msg  >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\abf\vq\ervq.msg      >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
REM type %ING_SRC%\front\dbiembed\hdr\ere7.msg>> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\dict\dictutil\erdu.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\dclgen\erdc.msg  >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\hdr\erco.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\hdr\ere0.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\hdr\ere1.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\hdr\ere2.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\hdr\ere3.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\hdr\ere4.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\hdr\ere6.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\embed\hdr\erlc.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\compfrm\erfc.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\copyform\ercf.msg>> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\frame\erfd.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\hdr\erfi.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\hdr\erm1.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\printform\erpf.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\runtime\erfr.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\tbacc\ertb.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frame\valid\erfv.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frontcl\gt\ergt.msg      >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frontcl\hdr\erft.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frontcl\libqsys\erls.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frontcl\mt\ermt.msg      >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frontcl\termdr\ertd.msg  >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\frontcl\vt\ervt.msg      >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\eraf.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\ercx.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erde.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erdg.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\ereq.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erfe.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erfm.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erg4.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\ergr.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erlq.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erqg.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erug.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\hdr\hdr\erui.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\convtouni\erctou.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\copydb\ercd.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\hdr\ermc.msg        >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\genxml\erxml.msg    >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\ingcntrl\eric.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\ingmenu\erim.msg    >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\tables\ertl.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\unloaddb\erud.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\misc\xferdb\erxf.msg     >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\qbf\qbf\erqf.msg         >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\report\copyrep\errc.msg  >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\report\hdr\errw.msg      >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\report\rbf\errf.msg      >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
REM This is a duplicate of the one below. Why?
REM type %ING_SRC%\front\report\rfvifred\ervf.msg >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\report\sreport\ersr.msg  >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\st\hdr\erst.msg          >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\tm\hdr\ermf.msg          >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\tm\monitor\ermo.msg      >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\tm\qr\erqr.msg           >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\utils\copyutil\ercu.msg  >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\utils\oo\eroo.msg        >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\utils\tblutil\ertu.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\utils\uf\eruf.msg        >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\vifred\vifred\ervf.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\web\hdr\erwb.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\web\hdr\erwa.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\rep\repmgr\errm.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
REM type %ING_SRC%\gateway\gwf\hdr\erge.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
REM type %ING_SRC%\gateway\gwf\hdr\ergu.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
REM type %ING_SRC%\gateway\gwf\hdr\ergx.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\vdba\rmcmd\erre.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
type %ING_SRC%\front\ice\hdr\erwu.msg   >> %ING_SRC%\common\hdr\hdr\fe.cat.msg
