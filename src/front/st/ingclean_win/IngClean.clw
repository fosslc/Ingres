; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CIngCleanDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "IngClean.h"

ClassCount=3
Class1=CIngCleanApp
Class2=CIngCleanDlg

ResourceCount=3
Resource1=IDR_MAINFRAME
Resource2=IDD_INGCLEAN_DIALOG
Class3=CWaitDlg
Resource3=IDD_WAIT_DIALOG

[CLS:CIngCleanApp]
Type=0
HeaderFile=IngClean.h
ImplementationFile=IngClean.cpp
Filter=N
LastObject=CIngCleanApp

[CLS:CIngCleanDlg]
Type=0
HeaderFile=IngCleanDlg.h
ImplementationFile=IngCleanDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=CIngCleanDlg



[DLG:IDD_INGCLEAN_DIALOG]
Type=1
Class=CIngCleanDlg
ControlCount=7
Control1=IDUNINSTALL,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_TEXT,static,1342177280
Control4=IDC_STATIC,button,1342177287
Control5=IDC_COPYRIGHT,static,1342177280
Control6=IDC_OUTPUT,static,1342177280
Control7=IDC_STATIC,static,1342181383

[DLG:IDD_WAIT_DIALOG]
Type=1
Class=CWaitDlg
ControlCount=2
Control1=IDC_AVI,SysAnimate32,1342177287
Control2=IDC_TEXT,static,1342177280

[CLS:CWaitDlg]
Type=0
HeaderFile=WaitDlg.h
ImplementationFile=WaitDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CWaitDlg

