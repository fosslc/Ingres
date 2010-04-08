/*
**  Copyright (c) 2008 Ingres Corporation.
*/

/*
**	Convert the current Windows OS-specific timezone value
**	to a valid Ingres timezone name.
**
**	Copied from code in dbmsnetmm.c
**
**  History:
**	03-Jun-2008 (whiro01)
**	    Created.
**	25-Aug-2008 (whiro01)
**	    Recoded to use registry so that auto daylight adjust
**	    can be correctly determined (GetTimeZoneInformation
**	    seems to be broken).
*/

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <malloc.h>

static void ErrorOut()
{
	DWORD err;
	LPVOID lpMsgBuf;

	err = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	fprintf(stderr, "Error %d: %s\n", err, lpMsgBuf);
	LocalFree(lpMsgBuf);
	exit(err);
}

//
// Copied from Ingres installer post-processing (dbmsnetmm.c)
//
typedef struct tagTIMEZONE
{
    char DisplayName[128];
    char StdName[128];
    char ingres_tz[128];
}TIMEZONE;

static TIMEZONE timezones[]=
{
    {"(GMT) Casablanca, Monrovia", "GMT Standard Time", "GMT"},
    {"(GMT) Casablanca, Monrovia", "Greenwich Standard Time", "GMT"},
    {"(GMT) Casablanca, Monrovia", "Greenwich", "GMT"},
    {"(GMT) Casablana", "Morocco Standard Time", "GMT"},
    {"(GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London", "GMT Standard Time", "UNITED-KINGDOM"},
    {"(GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London", "GMT", "UNITED-KINGDOM"},
    {"(GMT+01:00)  Amsterdam, Copenhagen, Madrid, Paris, Vilnius", "Romance Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna", "W. Europe Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna", "W. Europe", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague", "Central Europe Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague", "Central Europe", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Belgrade, Sarajevo, Skopje, Sofija, Zagreb", "Central European Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Bratislava, Budapest, Ljubljana, Prague, Warsaw", "Central Europe Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Brussels, Berlin, Bern, Rome, Stockholm, Vienna", "W. Europe Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris", "Romance Standard Time", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris", "Romance", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris, Vilnius", "Romance", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Sarajevo, Skopje, Sofija, Vilnius, Warsaw, Zagreb", "Central European", "EUROPE-CENTRAL"},
    {"(GMT+01:00) Sarajevo, Skopje, Sofija, Warsaw, Zagreb", "Central European", "EUROPE-CENTRAL"},
    {"(GMT+01:00) West Central Africa", "W. Central Africa Standard Time", "GMT1"},
    {"(GMT+01:00) West Central Africa", "W. Central Africa", "GMT1"},
    {"(GMT+02:00) Athens, Istanbul, Minsk", "GFT Standard Time", "EUROPE-EASTERN"},
    {"(GMT+02:00) Athens, Istanbul, Minsk", "GTB Standard Time", "EUROPE-EASTERN"},
    {"(GMT+02:00) Athens, Istanbul, Minsk", "GTB", "EUROPE-EASTERN"},
    {"(GMT+02:00) Bucharest", "E. Europe Standard Time", "EUROPE-EASTERN"},
    {"(GMT+02:00) Bucharest", "E. Europe", "EUROPE-EASTERN"},
    {"(GMT+02:00) Cairo", "Egypt Standard Time", "EGYPT"},
    {"(GMT+02:00) Cairo", "Egypt", "EGYPT"},
    {"(GMT+02:00) Harare, Pretoria", "South Africa Standard Time", "GMT2"},
    {"(GMT+02:00) Harare, Pretoria", "South Africa", "GMT2"},
    {"(GMT+02:00) Helsinki, Riga, Tallinn", "FLE Standard Time", "EUROPE-EASTERN"},
    {"(GMT+02:00) Helsinki, Riga, Tallinn", "FLE", "EUROPE-EASTERN"},
    {"(GMT+02:00) Israel", "Israel Standard Time", "ISRAEL"},
    {"(GMT+02:00) Jerusalem", "Israel Standard Time", "ISRAEL"},
    {"(GMT+02:00) Jerusalem", "Israel", "ISRAEL"},
    {"(GMT+02:00) Kaliningrad", "Kaliningrad Standard Time", "MOSCOW-1"},
    {"(GMT+02:00) Kaliningrad", "Kaliningrad", "MOSCOW-1"},
    {"(GMT+03:00) Baghdad", "Arabic Standard Time", "GMT3"},
    {"(GMT+03:00) Baghdad", "Arabic", "GMT3"},
    {"(GMT+03:00) Baghdad, Kuwait, Riyadh", "Arab", "GMT3"},
    {"(GMT+03:00) Baghdad, Kuwait, Riyadh", "Saudi Arabia Standard Time", "GMT3"},
    {"(GMT+03:00) Kuwait, Riyadh", "Arab Standard Time", "SAUDI-ARABIA"},
    {"(GMT+03:00) Kuwait, Riyadh", "Arab", "SAUDI-ARABIA"},
    {"(GMT+03:00) Moscow, St. Petersburg, Volgograd", "Russian Standard Time", "MOSCOW"},
    {"(GMT+03:00) Moscow, St. Petersburg, Volgograd", "Russian", "MOSCOW"},
    {"(GMT+03:00) Nairobi", "E. Africa Standard Time", "GMT3"},
    {"(GMT+03:00) Nairobi", "E. Africa", "GMT3"},
    {"(GMT+03:30) Tehran", "Iran Standard Time", "IRAN"},
    {"(GMT+03:30) Tehran", "Iran", "IRAN"},
    {"(GMT+04:00) Abu Dhabi, Muscat", "Arabian Standard Time", "GMT4"},
    {"(GMT+04:00) Abu Dhabi, Muscat", "Arabian", "GMT4"},
    {"(GMT+04:00) Baku, Tbilisi", "Caucasus Standard Time", "GMT4"},
    {"(GMT+04:00) Baku, Tbilisi", "Caucasus", "GMT4"},
    {"(GMT+04:00) Baku, Tbilisi, Yerevan", "Caucasus Standard Time", "GMT4"},
    {"(GMT+04:00) Baku, Tbilisi, Yerevan", "Caucasus", "GMT4"},
    {"(GMT+04:00) Samara, Saratov, Ul'yanovsk", "Samara Standard Time", "MOSCOW1"},
    {"(GMT+04:00) Samara, Saratov, Ul'yanovsk", "Samara", "MOSCOW1"},
    {"(GMT+04:30) Kabul", "Afghanistan Standard Time", ""},
    {"(GMT+04:30) Kabul", "Afghanistan", ""},
    {"(GMT+05:00) Ekaterinburg", "Ekaterinburg Standard Time", "GMT5"},
    {"(GMT+05:00) Ekaterinburg", "Ekaterinburg", "GMT5"},
    {"(GMT+05:00) Islamabad, Karachi, Tashkent", "West Asia Standard Time", "PAKISTAN"},
    {"(GMT+05:00) Islamabad, Karachi, Tashkent", "West Asia", "PAKISTAN"},
    {"(GMT+05:00) Yekaterinburg, Sverdlovsk, Chelyabinsk", "Yekaterinburg Standard Time", "MOSCOW2"},
    {"(GMT+05:00) Yekaterinburg, Sverdlovsk, Chelyabinsk", "Yekaterinburg", "MOSCOW2"},
    {"(GMT+05:30) Bombay, Calcutta, Madras, New Delhi", "India Standard Time", "INDIA"},
    {"(GMT+05:30) Bombay, Calcutta, Madras, New Delhi", "India", "INDIA"},
    {"(GMT+05:30) Calcutta, Chennai, Mumbai, New Delhi", "India", "INDIA"},
    {"(GMT+05:45) Kathmandu", "Nepal Standard Time", ""},
    {"(GMT+05:45) Kathmandu", "Nepal", ""},
    {"(GMT+06:00) Almaty, Dhaka", "Central Asia Standard Time", "GMT6"},
    {"(GMT+06:00) Almaty, Novosibirsk", "N. Central Asia Standard Time", "GMT6"},
    {"(GMT+06:00) Almaty, Novosibirsk", "N. Central Asia", "GMT6"},
    {"(GMT+06:00) Astana, Almaty, Dhaka", "Central Asia", "GMT6"},
    {"(GMT+06:00) Astana, Dhaka", "Central Asia Standard Time", "GMT6"},
    {"(GMT+06:00) Astana, Dhaka", "Central Asia", "GMT6"},
    {"(GMT+06:00) Colombo", "Sri Lanka Standard Time", "GMT6"},
    {"(GMT+06:00) Colombo", "Sri Lanka", "GMT6"},
    {"(GMT+06:00) Omsk, Novosibirsk", "Omsk/Novosibirsk Standard Time", "MOSCOW3"},
    {"(GMT+06:00) Omsk, Novosibirsk", "Omsk/Novosibirsk", "MOSCOW3"},
    {"(GMT+06:00) Sri Jayawardenepura", "Sri Lanka Standard Time", "GMT6"},
    {"(GMT+06:00) Sri Jayawardenepura", "Sri Lanka", "GMT6"},
    {"(GMT+06:30) Rangoon", "Myanmar Standard Time", ""},
    {"(GMT+06:30) Rangoon", "Myanmar", ""},
    {"(GMT+07:00) Bangkok, Hanoi, Jakarta", "Bangkok Standard Time", "INDONESIA-WEST"},
    {"(GMT+07:00) Bangkok, Hanoi, Jakarta", "SE Asia Standard Time", "INDONESIA-WEST"},
    {"(GMT+07:00) Bangkok, Hanoi, Jakarta", "SE Asia", "INDONESIA-WEST"},
    {"(GMT+07:00) Krasnoyarsk", "North Asia Standard Time", "GMT7"},
    {"(GMT+07:00) Krasnoyarsk", "North Asia", "GMT7"},
    {"(GMT+07:00) Krasnoyarsk", "Krasnoyarsk Standard Time", "MOSCOW4"},
    {"(GMT+07:00) Krasnoyarsk", "Krasnoyarsk", "MOSCOW4"},
    {"(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi", "China Standard Time", "HONG-KONG"},
    {"(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi", "China", "HONG-KONG"},
    {"(GMT+08:00) Irkutsk, Ulaan Bataar", "North Asia East Standard Time", "GMT8"},
    {"(GMT+08:00) Irkutsk, Ulaan Bataar", "North Asia East", "GMT8"},
    {"(GMT+08:00) Irkutsk, Ulaan Bataar", "Irkutsk Standard Time", "MOSCOW5"},
    {"(GMT+08:00) Irkutsk, Ulaan Bataar", "Irkutsk", "MOSCOW5"},
    {"(GMT+08:00) Kuala Lumpur, Singapore", "Singapore Standard Time", "MALAYSIA"},
    {"(GMT+08:00) Kuala Lumpur, Singapore", "Singapore", "MALAYSIA"},
    {"(GMT+08:00) Perth", "W. Australia Standard Time", "AUSTRALIA-WEST"},
    {"(GMT+08:00) Perth", "W. Australia", "AUSTRALIA-WEST"},
    {"(GMT+08:00) Singapore", "Singapore Standard Time", "SINGAPORE"},
    {"(GMT+08:00) Singapore", "Singapore", "SINGAPORE"},
    {"(GMT+08:00) Taipei", "Taipei Standard Time", "GMT8"},
    {"(GMT+08:00) Taipei", "Taipei", "GMT8"},
    {"(GMT+09:00) Osaka, Sapporo, Tokyo", "Tokyo Standard Time", "JAPAN"},
    {"(GMT+09:00) Osaka, Sapporo, Tokyo", "Tokyo", "JAPAN"},
    {"(GMT+09:00) Seoul", "Korea Standard Time", "KOREA"},
    {"(GMT+09:00) Seoul", "Korea", "KOREA"},
    {"(GMT+09:00) Yakutsk", "Yakutsk Standard Time", "GMT9"},
    {"(GMT+09:00) Yakutsk", "Yakutsk", "GMT9"},
    {"(GMT+09:00) Yakutsk", "Yakutsk Standard Time", "MOSCOW6"},
    {"(GMT+09:00) Yakutsk", "Yakutsk", "MOSCOW6"},
    {"(GMT+09:30) Adelaide", "Cen. Australia Standard Time", "AUSTRALIA-SOUTH"},
    {"(GMT+09:30) Adelaide", "Cen. Australia", "AUSTRALIA-SOUTH"},
    {"(GMT+09:30) Darwin", "AUS Central Standard Time", "AUSTRALIA-NORTH"},
    {"(GMT+09:30) Darwin", "AUS Central", "AUSTRALIA-NORTH"},
    {"(GMT+10:00) Brisbane", "E. Australia Standard Time", "AUSTRALIA-QUEENSLAND"},
    {"(GMT+10:00) Brisbane", "E. Australia", "AUSTRALIA-QUEENSLAND"},
    {"(GMT+10:00) Canberra, Melbourne, Sydney (Year 2000 only)", "Syd2000", "AUSTRALIA-NSW"},
    {"(GMT+10:00) Canberra, Melbourne, Sydney", "AUS Eastern Standard Time", "AUSTRALIA-NSW"},
    {"(GMT+10:00) Canberra, Melbourne, Sydney", "AUS Eastern", "AUSTRALIA-NSW"},
    {"(GMT+10:00) Canberra, Melbourne, Sydney", "Sydney Standard Time", "AUSTRALIA-NSW"},
    {"(GMT+10:00) Guam, Port Moresby", "West Pacific Standard Time", "GMT10"},
    {"(GMT+10:00) Guam, Port Moresby", "West Pacific", "GMT10"},
    {"(GMT+10:00) Hobart", "Tasmania Standard Time", "AUSTRALIA-TASMANIA"},
    {"(GMT+10:00) Hobart", "Tasmania", "AUSTRALIA-TASMANIA"},
    {"(GMT+10:00) Vladivostok", "Vladivostok Standard Time", "GMT10"},
    {"(GMT+10:00) Vladivostok", "Vladivostok", "GMT10"},
    {"(GMT+10:00) Vladivostok", "Vladivostok Standard Time", "MOSCOW7"},
    {"(GMT+10:00) Vladivostok", "Vladivostok", "MOSCOW7"},
    {"(GMT+11:00) Magadan, Solomon Is., New Caledonia", "Central Pacific Standard Time", "GMT11"},
    {"(GMT+11:00) Magadan, Solomon Is., New Caledonia", "Central Pacific", "GMT11"},
    {"(GMT+11:00) Magadan, Sakha, Solomon Is.", "Magadan Standard Time", "MOSCOW8"},
    {"(GMT+11:00) Magadan, Sakha, Solomon Is.", "Magadan", "MOSCOW8"},
    {"(GMT+12:00) Auckland, Wellington", "New Zealand Standard Time", "NEW-ZEALAND"},
    {"(GMT+12:00) Auckland, Wellington", "New Zealand", "NEW-ZEALAND"},
    {"(GMT+12:00) Fiji, Kamchatka, Marshall Is.", "Fiji Standard Time", "GMT12"},
    {"(GMT+12:00) Fiji, Kamchatka, Marshall Is.", "Fiji", "GMT12"},
    {"(GMT+12:00) Kamchatka, Chukot", "Anadyr Standard Time", "MOSCOW9"},
    {"(GMT+12:00) Kamchatka, Chukot", "Anadyr", "MOSCOW9"},
    {"(GMT+13:00) Nuku'alofa", "Tonga Standard Time", "GMT13"},
    {"(GMT+13:00) Nuku'alofa", "Tonga", "GMT13"},
    {"(GMT-01:00) Azores", "Azores Standard Time", "GMT-1"},
    {"(GMT-01:00) Azores", "Azores", "GMT-1"},
    {"(GMT-01:00) Azores, Cape Verde Is.", "Azores Standard Time", "GMT-1"},
    {"(GMT-01:00) Azores, Cape Verde Is.", "Azores", "GMT-1"},
    {"(GMT-01:00) Cape Verde Is.", "Cape Verde Standard Time", "GMT-1"},
    {"(GMT-01:00) Cape Verde Is.", "Cape Verde", "GMT-1"},
    {"(GMT-02:00) Mid-Atlantic", "Mid-Atlantic Standard Time", "GMT-2"},
    {"(GMT-02:00) Mid-Atlantic", "Mid-Atlantic", "GMT-2"},
    {"(GMT-03:00) Brasilia", "E. South America Standard Time", "BRAZIL-EAST"},
    {"(GMT-03:00) Brasilia", "E. South America", "BRAZIL-EAST"},
    {"(GMT-03:00) Buenos Aires, Georgetown", "SA Eastern Standard Time", "GMT-3"},
    {"(GMT-03:00) Buenos Aires, Georgetown", "SA Eastern", "GMT-3"},
    {"(GMT-03:00) Greenland", "Greenland Standard Time", "GMT-3"},
    {"(GMT-03:00) Greenland", "Greenland", "GMT-3"},
    {"(GMT-03:30) Newfoundland", "Newfoundland Standard Time", "CANADA-NEWFOUNDLAND"},
    {"(GMT-03:30) Newfoundland", "Newfoundland", "CANADA-NEWFOUNDLAND"},
    {"(GMT-04:00) Atlantic Time (Canada)", "Atlantic Standard Time", "CANADA-ATLANTIC"},
    {"(GMT-04:00) Atlantic Time (Canada)", "Atlantic", "CANADA-ATLANTIC"},
    {"(GMT-04:00) Caracas, La Paz", "SA Western Standard Time", "GMT-4"},
    {"(GMT-04:00) Caracas, La Paz", "SA Western", "GMT-4"},
    {"(GMT-04:00) Santiago", "Pacific SA Standard Time", "CHILE-CONTINENTAL"},
    {"(GMT-04:00) Santiago", "Pacific SA", "CHILE-CONTINENTAL"},
    {"(GMT-05:00) Bogota, Lima, Quito", "SA Pacific Standard Time", "GMT-5"},
    {"(GMT-05:00) Bogota, Lima, Quito", "SA Pacific", "GMT-5"},
    {"(GMT-05:00) Eastern Time (US & Canada)", "Eastern Standard Time", "NA-EASTERN"},
    {"(GMT-05:00) Eastern Time (US & Canada)", "Eastern", "NA-EASTERN"},
    {"(GMT-05:00) Indiana (East)", "US Eastern Standard Time", "NA-EASTERN"},
    {"(GMT-05:00) Indiana (East)", "US Eastern", "NA-EASTERN"},
    {"(GMT-06:00) Central America", "Central America Standard Time", "GMT-6"},
    {"(GMT-06:00) Central America", "Central America", "GMT-6"},
    {"(GMT-06:00) Central Time (US & Canada)", "Central Standard Time", "NA-CENTRAL"},
    {"(GMT-06:00) Central Time (US & Canada)", "Central", "NA-CENTRAL"},
    {"(GMT-06:00) Mexico City", "Mexico", "MEXICO-GENERAL"},
    {"(GMT-06:00) Mexico City, Tegucigalpa", "Mexico Standard Time", "GMT-6"},
    {"(GMT-06:00) Mexico City, Tegucigalpa", "Mexico", "GMT-6"},
    {"(GMT-06:00) Saskatchewan", "Canada Central Standard Time", "NA-CENTRAL"},
    {"(GMT-06:00) Saskatchewan", "Canada Central", "NA-CENTRAL"},
    {"(GMT-07:00) Arizona", "US Mountain Standard Time", "GMT-7"},
    {"(GMT-07:00) Arizona", "US Mountain", "GMT-7"},
    {"(GMT-07:00) Mountain Time (US & Canada)", "Mountain Standard Time", "NA-MOUNTAIN"},
    {"(GMT-07:00) Mountain Time (US & Canada)", "Mountain", "NA-MOUNTAIN"},
    {"(GMT-08:00) Pacific Time (US & Canada); Tijuana", "Pacific Standard Time", "NA-PACIFIC"},
    {"(GMT-08:00) Pacific Time (US & Canada); Tijuana", "Pacific", "NA-PACIFIC"},
    {"(GMT-08:00) Tijuana, Baja California", "Pacific Standard Time (Mexico)", "MEXICO-BAJANORTE"},
    {"(GMT-08:00) Tijuana, Baja California", "Pacific (Mexico)", "MEXICO-BAJANORTE"},
    {"(GMT-09:00) Alaska", "Alaskan Standard Time", "US-ALASKA"},
    {"(GMT-09:00) Alaska", "Alaskan", "US-ALASKA"},
    {"(GMT-10:00) Hawaii", "Hawaiian Standard Time", "US-HAWAII"},
    {"(GMT-10:00) Hawaii", "Hawaiian", "US-HAWAII"},
    {"(GMT-11:00) Midway Island, Samoa", "Samoa Standard Time", "GMT-11"},
    {"(GMT-11:00) Midway Island, Samoa", "Samoa", "GMT-11"},
    {"(GMT-12:00) Eniwetok, Kwajalein", "Dateline Standard Time", "GMT-12"},
    {"(GMT-12:00) Eniwetok, Kwajalein", "Dateline", "GMT-12"},
    {0, 0, 0}
};

BOOL
searchIngTimezone(char *os_tz, char *ing_tz)
{
    int total, i;

    total=sizeof(timezones)/sizeof(TIMEZONE);
    for(i=0; i<total; i++)
    {
	if (!_stricmp(os_tz, timezones[i].DisplayName) && _stricmp(timezones[i].ingres_tz, ""))
	{
	    strcpy(ing_tz, timezones[i].ingres_tz);
	    return (TRUE);
	}
	if (!_stricmp(os_tz, timezones[i].StdName) && _stricmp(timezones[i].ingres_tz, ""))
	{
	    strcpy(ing_tz, timezones[i].ingres_tz);
	    return (TRUE);
	}
    }
    return (FALSE);
}

int main(int argc, char **argv)
{
	char os_tz[200], ing_tz[100];
	static WCHAR *REG_LOC = L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation";
	static WCHAR *ACTIVE_TIME_BIAS = L"ActiveTimeBias";
	static WCHAR *BIAS = L"Bias";
	static WCHAR *DAYLIGHT_BIAS = L"DaylightBias";
	static WCHAR *DISABLE_AUTO_DAYLIGHT = L"DisableAutoDaylightTimeSet";
	static WCHAR *STANDARD_NAME = L"StandardName";
	HKEY hTZI;
	LONG lRet;
	WCHAR valuename[256];
	DWORD len, datalen;
	DWORD dwType;
	BYTE data[256];
	DWORD value;
	DWORD activebias, bias, daylightbias;
	bool autodisable = false;
	bool couldnotconvert = false;

	// Open the registry key for the current timezone information
	if ((lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_LOC, 0, KEY_READ, &hTZI)) != ERROR_SUCCESS)
		ErrorOut();

	// Query all the values of interest
	for (DWORD i = 0; lRet == ERROR_SUCCESS; i++)
	{
		len = sizeof(valuename);
		lRet = RegEnumValue(hTZI, i, valuename, &len, NULL, &dwType, NULL, NULL);
  		if (lRet == ERROR_SUCCESS)
		{
			datalen = 0;
			lRet = RegQueryValueEx(hTZI, valuename, NULL, &dwType, NULL, &datalen);
			switch (dwType)
			{
			    case REG_DWORD:
				datalen = sizeof(value);
				RegQueryValueEx(hTZI, valuename, NULL, NULL, (LPBYTE)&value, &datalen);
				break;
			    case REG_SZ:
			    case REG_BINARY:
				datalen = sizeof(data);
				RegQueryValueEx(hTZI, valuename, NULL, NULL, data, &datalen);
				break;
			    default:
				break;
			}
			if (wcscmp(valuename, ACTIVE_TIME_BIAS) == 0)
				activebias = value;
			else if (wcscmp(valuename, BIAS) == 0)
				bias = value;
			else if (wcscmp(valuename, DAYLIGHT_BIAS) == 0)
				daylightbias = value;
			else if (wcscmp(valuename, DISABLE_AUTO_DAYLIGHT) == 0)
			{
				if (value == 1)
					autodisable = true;
			}
			else if (wcscmp(valuename, STANDARD_NAME) == 0)
			{
				if (WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)data, -1, os_tz, sizeof(os_tz), NULL, NULL) == 0)
					ErrorOut();
			}
		}
	}

	RegCloseKey(hTZI);

	// If auto adjust is disabled, then just use the GMT bias as the zone name
	if (autodisable)
	{
		couldnotconvert = true;
	}
	// Else just get the Ingres equivalent of the regular timezone
	else
	{
		if (!searchIngTimezone(os_tz, ing_tz))
		{
			fprintf(stderr, "Windows timezone '%s' could not be converted to Ingres timezone.\n", os_tz);
			couldnotconvert = true;
		}
		
	}
	if (couldnotconvert)
	{
		long tzbias = (long)bias;
		sprintf(ing_tz, "GMT%ld", (-tzbias) / 60);
		if ((tzbias % 60) == 30)
			strcat(ing_tz, "-and-half");
	}
	printf("%s\n", ing_tz);
	return 0;
}
