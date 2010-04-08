The program in this directory serves as an example of using Ingres and 
the Ingres Management Architecture (IMA) to provide an ima equivalent
of the ipm utility.

This code is provided as an example only. It is an unsupported example 
provided by Ingres Corporation to help customers who wish to use the IMA 
facilities in Ingres. The code in this directory is 
(c) 2002 Ingres Corporation. 

Ingres Corporation accepts no responsibility for any damage or action 
arising from the use of this code.

For a discussion of IMA - see the Ingres System Reference Manual.

The program is written in embedded SQL with FRS calls. 

Files in this directory:

makefile        Makefile for the utility - this file prepares the environment
                for compiling and linking the utility.
                    Creates the database for the forms definitions.
                    Loads the schema for the utility.
                N.B.    The initiation of the nmake command must be
                        from a window where the user is ingres or where
                        the user has been assigned the Ingres security user
                        privilege.
                        Ingres must also be started.

                nmake               create database and populate
                nmake compile       create database, populate, generate
                                    forms code, compile and link executable.

imp.mak         Makefile called from makefile to compile and link the
                executable.

imp.forms	Copyform output of the forms used in the application

imp.h
imp_dmf.incl
impcommon.sc
impdbcon.sc
impdmf.sc
impfrs.sc
impinter.sc
impio.sc
implocks.sc
implog.sc
impmain.sc
impqsf.sc
imprdf.sc
impserver.sc
impslave.sc
impstart.sc
impstatic.sc 	The source code files for the utility

imp.sql 	SQL script to register the required tables

To setup the utility, as privileged INGRES user, register the 
imp IMA tables and procedures.

> nmake  register

Copy the form in, then get a compiled version:

> nmake  copyform

Build the application:

> nmake  compile

To run the application, use

> imp [database] [-rTIMEOUT_SECONDS]

Where:

database		is the name of the database containing the IMA tables.
TIMEOUT_SECONDS		is the number of seconds for the refresh interval.

The screen output is similar to ipm.
