@echo off

if NOT "%II_HTML_ROOT%" == "" goto htmlrootset

if "%1" == "" goto usage
call ipset II_HTML_ROOT  htmlroot %1
set DISPLAY=^>NUL
goto start

:htmlrootset
set DISPLAY=

:start
if not exist %II_HTML_ROOT%\ice md %II_HTML_ROOT%\ice
if not exist %II_HTML_ROOT%\ice\tmp md %II_HTML_ROOT%\ice\tmp
if not exist %II_HTML_ROOT%\ice\styles md %II_HTML_ROOT%\ice\styles
if not exist %II_HTML_ROOT%\ice\images md %II_HTML_ROOT%\ice\images
if not exist %II_HTML_ROOT%\ice\samples md %II_HTML_ROOT%\ice\samples
if not exist %II_HTML_ROOT%\ice\samples\app    md %II_HTML_ROOT%\ice\samples\app
if not exist %II_HTML_ROOT%\ice\samples\dbproc md %II_HTML_ROOT%\ice\samples\dbproc
if not exist %II_HTML_ROOT%\ice\samples\query  md %II_HTML_ROOT%\ice\samples\query
if not exist %II_HTML_ROOT%\ice\samples\report md %II_HTML_ROOT%\ice\samples\report
if not exist %II_HTML_ROOT%\ice\tutorialGuide  md %II_HTML_ROOT%\ice\tutorialGuide

copy %II_SYSTEM%\ingres\ice\icetool\login.html %II_HTML_ROOT%\ice\ialogin.html %DISPLAY%
copy %II_SYSTEM%\ingres\ice\icetutor\login.html %II_HTML_ROOT%\ice\itlogin.html %DISPLAY%
copy %II_SYSTEM%\ingres\ice\plays\play_welcome.html %II_HTML_ROOT%\ice %DISPLAY%
copy %II_SYSTEM%\ingres\ice\plays\play_public.css %II_HTML_ROOT%\ice\styles %DISPLAY%
copy %II_SYSTEM%\ingres\ice\plays\bgpaper.gif %II_HTML_ROOT%\ice\images %DISPLAY%
copy %II_SYSTEM%\ingres\ice\plays\OldGlobe.gif %II_HTML_ROOT%\ice\images %DISPLAY%
copy %II_SYSTEM%\ingres\ice\public\ice_index.html %II_HTML_ROOT% %DISPLAY%
copy %II_SYSTEM%\ingres\ice\public\exception.html %II_HTML_ROOT%\ice %DISPLAY%
copy %II_SYSTEM%\ingres\ice\public\ice.css %II_HTML_ROOT%\ice\styles %DISPLAY%
copy %II_SYSTEM%\ingres\ice\samples\app\*.* %II_HTML_ROOT%\ice\samples\app %DISPLAY%
copy %II_SYSTEM%\ingres\ice\samples\dbproc\*.* %II_HTML_ROOT%\ice\samples\dbproc %DISPLAY%
copy %II_SYSTEM%\ingres\ice\samples\query\*.* %II_HTML_ROOT%\ice\samples\query %DISPLAY%
copy %II_SYSTEM%\ingres\ice\samples\report\*.* %II_HTML_ROOT%\ice\samples\report %DISPLAY%
copy %II_SYSTEM%\ingres\ice\plays\tutor\*.* %II_HTML_ROOT%\ice\tutorialGuide %DISPLAY%

goto end

:usage
echo Usage:
echo       %0 ^<MS ^| NS ^| NS2 ^| NS3 ^| SG^>
echo          MS = Microsoft
echo          NS = Netscape
echo          NS2= Netscape 2.x
echo          NS3= Netscape 3.x
echo          SG = Spyglass

:end
