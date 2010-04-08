// Warning: this file is NOT TO BE COMPILED but included in Winmain
// due to "scope" issues

	char *   argv[12+1];
	ARGCTYPE argc;
	short    wQueueNumber = PREFERRED_QUEUE_SIZE;
	unsigned char   qchar;
	unsigned char * p;
	char *   fn;
	FILE *   fp;

	/*
	**  Increase the application message queue size.  Ingres apps may not
	**  follows the 1/10th second rule... Therefore, the queue may be
	**  overflowing.
	*/
	while (!SetMessageQueue(wQueueNumber)
	    && wQueueNumber > NORMAL_QUEUE_SIZE)
		wQueueNumber--;

	/*
	** Get the real name of this program; store the full path in argv0 
	** and store the progname in IIpnProgName.
	*/
	GetModuleFileName(hInstance, argv0, sizeof(argv0));
	for (p = argv0 + strlen(argv0); --p > argv0; )
	{
		if ((*p == '\\') || (*p == ':'))
		{
			++p;
			break;
		}
	}
	strcpy(IIpnProgName, p);
	for (p = IIpnProgName-1; *++p; )
	{
		if (*p == '.')
		{
			*p = '\0';
			break;
		}
	}

	/*
	** Initalize INGRES.
	*/
	IIStartIngres(hInstance, hPrevInstance, nCmdShow, IIpnProgName, 
					lpszCmdLine, IIshShowHistory, &status);
	if (status == 0)
	{
		MessageBox((HWND)NULL, "Unable to Initalize for WINDOWS",
			"Application Execution Error",
			MB_OK | MB_APPLMODAL | MB_ICONEXCLAMATION);
		exit(-1);
	}


