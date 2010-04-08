; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CSplashWnd
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "enterprise.h"

ClassCount=7
Class1=CEnterpriseApp
Class2=CSplashWnd

ResourceCount=4
Resource1=IDR_MAINFRAME
Class3=C256bmp
Resource2=IDD_WELCOMEDLG
Resource3=IDD_INSTALLCODE
Class4=CInstallCode
Class5=CWelcome
Class6=CPropSheet
Class7=CWaitDlg
Resource4=IDD_WAIT_DIALOG

[CLS:CEnterpriseApp]
Type=0
HeaderFile=enterprise.h
ImplementationFile=enterprise.cpp
Filter=N
LastObject=CEnterpriseApp

[CLS:C256bmp]
Type=0
HeaderFile=256bmp.h
ImplementationFile=256bmp.cpp
BaseClass=CStatic
Filter=W
VirtualFilter=WC
LastObject=C256bmp

[DLG:IDD_WELCOMEDLG]
Type=1
Class=CWelcome
ControlCount=2
Control1=IDC_STATIC,static,1342308352
Control2=IDC_IMAGE,static,1342308352

[DLG:IDD_INSTALLCODE]
Type=1
Class=CInstallCode
ControlCount=6
Control1=IDC_STATIC,static,1342308864
Control2=IDC_INSTALLATIONCODE,edit,1350631560
Control3=IDC_STATIC,button,1342177287
Control4=IDC_INSTALLEDLIST,SysListView32,1350631629
Control5=IDC_IMAGE,static,1342308352
Control6=IDC_STATIC,static,1342308352

[CLS:CInstallCode]
Type=0
HeaderFile=InstallCode.h
ImplementationFile=InstallCode.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CInstallCode

[CLS:CWelcome]
Type=0
HeaderFile=Welcome.h
ImplementationFile=Welcome.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CWelcome
VirtualFilter=idWC

[CLS:CPropSheet]
Type=0
HeaderFile=PropSheet.h
ImplementationFile=PropSheet.cpp
BaseClass=CPropertySheet
Filter=W
LastObject=CPropSheet
VirtualFilter=hWC

[DLG:IDD_WAIT_DIALOG]
Type=1
Class=CWaitDlg
ControlCount=2
Control1=IDC_AVI,SysAnimate32,1342242823
Control2=IDC_TEXT,static,1342308352

[CLS:CWaitDlg]
Type=0
HeaderFile=WaitDlg.h
ImplementationFile=WaitDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CWaitDlg
VirtualFilter=dWC

[CLS:CSplashWnd]
Type=0
HeaderFile=splash.h
ImplementationFile=splash.cpp
BaseClass=CWnd
LastObject=CSplashWnd

