// Author: Noam 
// 
// Description: The program allows you to monitor your application and see events that happend from the program.
// The program works by interacting with procmon driver (own driver or microsoft certified) and getting and parsing the events.
// The program has 3 modes:
// (a)ctive - will allow you to monitor your application with rules and see if any events deviated from the rules, if so your program will be killed.
// (w)arning - will allow you to monitor your application with rules and see if any events deviated from the rules, if so you'll get alerts about it.
// (p)assive - will allow you to monitor your application and make rules from the events that appear.
//


#include <conio.h>
#include <atltime.h>
#include <wchar.h>
#include <string.h>
#include <fstream>
#include <regex>
#include <windows.h>
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
#include <winuser.h>
#include "../../sdk/procmonsdk/sdk.hpp"
//#include "Includes/filtermgr.h"
#include "Includes/filter.hpp"
#include "Includes/filtermgr.cpp"

/* Defines*/
#define MAX_THREADS 1000
#define FOREGROUND_GREY 7
#define FOREGROUND_YELLOW 6

/* Globals*/
HANDLE  hThreadArray[MAX_THREADS];
DWORD   dwThreadIdArray[MAX_THREADS];
HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
int threadnum = 0;
char* pathOut = (char*)"log.txt";
FILE* fptrOut;
char* pathFor = NULL;
char* pathIn = NULL;
FILE* fptrIn;
char* mode = (char*)"p";
char* exe = NULL;
wchar_t* wexe = NULL;

/* Converts unicode to ansi aka wchar_t* to char*. */
char* WUnicodeToAnsi(const wchar_t* lpszStr)
{
	char* lpAnsi;
	int nLen;

	if (NULL == lpszStr)
		return NULL;

	nLen = ::WideCharToMultiByte(CP_ACP, 0, lpszStr, -1, NULL, 0, NULL, NULL);
	if (0 == nLen)
		return NULL;

	lpAnsi = new char[nLen + 1];
	if (NULL == lpAnsi)
		return NULL;

	memset(lpAnsi, 0, nLen + 1);
	nLen = ::WideCharToMultiByte(CP_ACP, 0, lpszStr, -1, lpAnsi, nLen, NULL, NULL);
	if (0 == nLen)
	{
		delete[]lpAnsi;
		return NULL;
	}

	return lpAnsi;
}

/* Converts unicode to ansi aka wstring* to string. */
std::string UnicodeToAnsi(__in std::wstring* str)
{
	if (str == NULL) return "";

	int nRequired = WideCharToMultiByte(CP_UTF8, 0, str->c_str(), (int)str->length(), NULL, 0, NULL, NULL);
	if (nRequired == 0) return "";

	CHAR* lpWideChar = new CHAR[nRequired + 1];
	if (lpWideChar == NULL) return "";
	nRequired = WideCharToMultiByte(CP_UTF8, 0, str->c_str(), (int)str->length(), lpWideChar, nRequired, NULL, NULL);
	if (nRequired == 0) return "";
	lpWideChar[nRequired] = '\0';
	std::string wstr = lpWideChar;
	delete[]lpWideChar;
	return wstr;
}

/* Replaces all occurences of a string(toReplace) to another string(replaceWith) inside a string(s). */
void replace_all(std::string& s, std::string const& toReplace, std::string const& replaceWith)
{
	std::string buf;
	std::size_t pos = 0;
	std::size_t prevPos;

	// Reserves rough estimate of final size of string.
	buf.reserve(s.size());

	while (true) {
		prevPos = pos;
		pos = s.find(toReplace, pos);
		if (pos == std::string::npos)
			break;
		buf.append(s, prevPos, pos - prevPos);
		buf += replaceWith;
		pos += toReplace.size();
	}

	buf.append(s, prevPos, s.size() - prevPos);
	s.swap(buf);
}

/* Replaces all "special" chars in regex to a "regular" string. */
std::string makeRegex(std::string str)
{
	std::string specialchars[] = {"\\","(",")","{","}","[","]","?","*","$",".","^","+"};
	int len = sizeof(specialchars) / sizeof(*specialchars);
	for (size_t i = 0; i < len; i++)
	{
		replace_all(str, specialchars[i], "\\" + specialchars[i]);
	}
	/*replace_all(str, "\\", "\\\\");
	replace_all(str, "(", "\\(");
	replace_all(str, ")", "\\)");
	replace_all(str, ".", "\\.");
	replace_all(str, "$", "\\$");
	replace_all(str, "^", "\\^"); 
	replace_all(str, "+", "\\+"); 
	replace_all(str, "?", "\\?");
	replace_all(str, "]", "\\]");
	replace_all(str, "[", "\\[");
	replace_all(str, "}", "\\}");
	replace_all(str, "{", "\\{");
	replace_all(str, "*", "\\*");*/
	return str;
}

/* Kills a process by its name. */
void killProcessByName(const char* filename)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	while (hRes)
	{
		if (strcmp(WUnicodeToAnsi(pEntry.szExeFile), filename) == 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
				(DWORD)pEntry.th32ProcessID);
			if (hProcess != NULL)
			{
				TerminateProcess(hProcess, 9);
				CloseHandle(hProcess);
			}
		}
		hRes = Process32Next(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);
}

/* Gets local date and time from a Large integer. */
CString GetDateTime(LARGE_INTEGER systime)
{
	FILETIME fit;
	SYSTEMTIME syt;
	CString time;
	fit.dwHighDateTime = systime.HighPart;
	fit.dwLowDateTime = systime.LowPart;
	if (FileTimeToSystemTime(&fit, &syt) && SystemTimeToTzSpecificLocalTime(NULL, &syt, &syt))
	{
		time.Format(TEXT("%llu"),systime.QuadPart);
		return time;
	}
		
	time.Format(TEXT("%02d.%02d.%d %02d:%02d:%02d:%d"), syt.wDay, syt.wMonth, syt.wYear, syt.wHour, syt.wMinute, syt.wSecond, syt.wMilliseconds);
	return time;
}

/* Tests if the file has the correct format for GuardCode code. */
std::ptrdiff_t TestFormat(char* path)
{
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(path) && GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		return -1;
	}
	char ch;
	std::string line = "";
	int numline = 1;
	FILE* fptrFor = fopen(path, "r");
	if (fptrFor != NULL)
	{
		do {
			ch = fgetc(fptrFor);
			line = line + ch;
			if (ch == '\n')
			{
				std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) { return std::tolower(c); });
				if (line.rfind("[+]", 0) != 0 && line.rfind("[-]", 0) != 0 && line.rfind("#", 0) != 0)
				{
					return numline;
				}
				numline++;
				line = "";
			}
		} while (ch != EOF);
	}
		return 0;
}

/* Handles the format in an active way that means that it is looking for a deviation from the rules and if so then kills the process. */
std::ptrdiff_t HandleFormatActive(LPCTSTR lpOPt, LPWSTR buffer, char* path)
{
	char ch;
	int numline = 1;
	std::string line = "";
	FILE* fptrFor = fopen(path, "r");
	wchar_t conv[1028];
	if (wcslen(buffer) != 0)
		_stprintf(conv, TEXT("Do %s for %s\n"), lpOPt, buffer);
	else
		_stprintf(conv, TEXT("Do %s\n"), lpOPt);
	std::string tmpstr(WUnicodeToAnsi(_wcslwr(conv)));
	if (fptrFor != NULL)
	{
		do {
			ch = fgetc(fptrFor);
			line = line + ch;
			if (ch == '\n')
			{
				std::transform(line.begin(), line.end(), line.begin(),[](unsigned char c) { return std::tolower(c); });
				if (line.rfind("[+]", 0) == 0)
				{
					line = line.substr(line.find("[+]") + strlen("[+]"), line.length());
					std::regex rx(line.c_str());
					std::ptrdiff_t result = std::distance(std::sregex_iterator(tmpstr.begin(), tmpstr.end(), rx), std::sregex_iterator());
					if (result > 0)
					{
						fclose(fptrFor);
						return 0;
					}
				}
				else if (line.rfind("[-]", 0) == 0)
				{
					line = line.substr(line.find("[-]") + strlen("[-]"), line.length());
					std::regex rx(line.c_str());
					std::ptrdiff_t result = std::distance(std::sregex_iterator(tmpstr.begin(), tmpstr.end(), rx), std::sregex_iterator());
					if (result > 0)
					{
						fclose(fptrFor);
						killProcessByName(exe);
						SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
						printf("Killed by line %d for:\n", numline);
						if (wcslen(buffer) != 0)
						{
							LogMessage(L_ERROR, TEXT("Do %s for %s\n"), lpOPt, buffer);
							_ftprintf(fptrOut, TEXT("[!]Killed by line %d for: Do %s for %s\n"), numline, lpOPt, buffer);
						}
						else
						{
							LogMessage(L_ERROR, TEXT("Do %s\n"), lpOPt);
							_ftprintf(fptrOut, TEXT("[!]Killed by line %d for: Do %s\n"), numline, lpOPt);
						}
						printf("\n\n");
						SetConsoleTextAttribute(hConsole, FOREGROUND_GREY);
						fclose(fptrOut);
						printf("Killing the monitoring process in one minute...");
						Sleep(1000);
						TerminateProcess(OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, GetCurrentProcessId()), 0);
					}
				}
				numline++;
				line = "";
			}
		} while (ch != EOF);
		return 1;
	}
}

/* Handles the format in a warning way that means that it is looking for a deviation from the rules and if it will show you what deviated without killing the process. */
std::ptrdiff_t HandleFormatWarning(LPCTSTR lpOPt, LPWSTR buffer, char* path)
{
	char ch;
	int numline = 1;
	std::string line = "";
	FILE* fptrFor = fopen(path, "r");
	wchar_t conv[1028];
	if (wcslen(buffer) != 0)
		_stprintf(conv, TEXT("Do %s for %s\n"), lpOPt, buffer);
	else
		_stprintf(conv, TEXT("Do %s\n"), lpOPt);
	std::string tmpstr(WUnicodeToAnsi(_wcslwr(conv)));
	if (fptrFor != NULL)
	{
		do {
			ch = fgetc(fptrFor);
			line = line + ch;
			if (ch == '\n')
			{
				std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) { return std::tolower(c); });
				if (line.rfind("[+]", 0) == 0)
				{
					line = line.substr(line.find("[+]") + strlen("[+]"), line.length());
					std::regex rx(line.c_str());
					std::ptrdiff_t result = std::distance(std::sregex_iterator(tmpstr.begin(), tmpstr.end(), rx), std::sregex_iterator());
					if (result > 0)
					{
						fclose(fptrFor);
						return 0;
					}
				}
				else if (line.rfind("[-]", 0) == 0)
				{
					line = line.substr(line.find("[-]") + strlen("[-]"), line.length());
					std::regex rx(line.c_str());
					std::ptrdiff_t result = std::distance(std::sregex_iterator(tmpstr.begin(), tmpstr.end(), rx), std::sregex_iterator());
					if (result > 0)
					{
						fclose(fptrFor);
						SetConsoleTextAttribute(hConsole, FOREGROUND_YELLOW);
						printf("Would be killed by line %d for:\n", numline);
						if (wcslen(buffer) != 0)
						{
							LogMessage(L_DEBUG, TEXT("Do %s for %s\n"), lpOPt, buffer);
							_ftprintf(fptrOut, TEXT("[?]Would be killed for: Do %s for %s\n"), lpOPt, buffer);
						}
						else
						{
							LogMessage(L_DEBUG, TEXT("Do %s\n"), lpOPt);
							_ftprintf(fptrOut, TEXT("[?]Would be killed for: Do %s\n"), lpOPt);
						}
						SetConsoleTextAttribute(hConsole, FOREGROUND_GREY);
						return 0;
					}
				}
				numline++;
				line = "";
			}
		} while (ch != EOF);
		return 1;
	}
}

/* Handles the format in a passive way that means that it takes the events that happends and translate them into GuardCode format for later use as rules. */
std::ptrdiff_t HandleFormatPassive(LPCTSTR lpOPt, LPWSTR buffer, char* path)
{
	char ch;
	std::string line = "";
	FILE* fptrFor = fopen(path, "r");
	if (fptrFor != NULL)
	{
		do {
			ch = fgetc(fptrFor);
			line = line + ch;
		} while (ch != EOF);
		fclose(fptrFor);
	}
	wchar_t conv[1028];
	if(wcslen(buffer) != 0)
		_stprintf(conv, TEXT("Do %s for %s\n"), lpOPt, buffer);
	else
		_stprintf(conv, TEXT("Do %s\n"), lpOPt);
	std::string condi = makeRegex(WUnicodeToAnsi(_wcslwr(conv)));
	std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) { return std::tolower(c); });
	std::regex rx(condi.c_str());
	std::ptrdiff_t result =  std::distance(std::sregex_iterator(line.begin(), line.end(), rx), std::sregex_iterator());
	if (result == 0)
	{
		FILE* fptrFor;
		fptrFor = fopen(path, "a");
		fprintf(fptrFor, "[+]%s", condi.c_str());
		fclose(fptrFor);
	}
}

DWORD WINAPI test(LPVOID parmas) {
	MessageBox(
		NULL,
		(LPCWSTR)L"Resource not available\nDo you want to try again?",
		(LPCWSTR)L"Account Details",
		MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
	);
	Sleep(10000);
	printf("threadhere");
	return 0;
}

void foo()
{
	if (threadnum < MAX_THREADS)
	{
		printf("thread");
		hThreadArray[threadnum] = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			test,       // thread function name
			0,          // argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArray[threadnum]);
		if (hThreadArray[threadnum] == NULL)
		{
			printf("CreateThreadFailed");
			ExitProcess(3);
		}
		printf("endthread");
		printf(" %d\n",threadnum);
	}
}

/* A class to handle the events that are coming and have all of the attributes of IEventCallback. */
class CMyEvent : public IEventCallback
{
public:
	/* Handles the events that coming from the driver. */
	virtual BOOL DoEvent(const CRefPtr<CEventView> pEventView)
	{
		
		if (wcscmp(pEventView->GetPath().GetBuffer(), L"TODO") && wcscmp(pEventView->GetProcessName().GetBuffer(), wexe) == 0)
		{
			CString strOperator;
			CString Time = GetDateTime(pEventView->GetStartTime());
			LPCTSTR lpOPt = StrMapOperation(pEventView->GetPreEventEntry());
			if (!lpOPt) {
				DWORD dwClass = pEventView->GetEventClass();
				DWORD dwOperator = pEventView->GetEventOperator();
				strOperator.Format(TEXT("%d:%d"), dwClass, dwOperator);
			}
			else {
				strOperator = lpOPt;
			}

			if (wcslen(pEventView->GetPath().GetBuffer()) != 0)
			{
				LogMessage(L_INFO, TEXT("%s Process %s Do %s for %s, %s"), Time.GetString(), pEventView->GetProcessName().GetBuffer(), strOperator,
					pEventView->GetPath().GetBuffer(),
					pEventView->GetOperationStrResult(emResult));
				_ftprintf(fptrOut, TEXT("[+]%s Process %s Do %s for %s, %s\n"), Time.GetString(), pEventView->GetProcessName().GetBuffer(), strOperator,
					pEventView->GetPath().GetBuffer(),
					pEventView->GetOperationStrResult(emResult));
			}
			else
			{
				LogMessage(L_INFO, TEXT("%s Process %s Do %s, %s"), Time.GetString(), pEventView->GetProcessName().GetBuffer(), strOperator,
					pEventView->GetOperationStrResult(emResult));
				_ftprintf(fptrOut, TEXT("[+]%s Process %s Do %s, %s\n"), Time.GetString(), pEventView->GetProcessName().GetBuffer(), strOperator,
					pEventView->GetOperationStrResult(emResult));
			}
			if (!strcmp(mode, "p"))
			{
				if (pathFor != NULL)
				{
					HandleFormatPassive(strOperator, pEventView->GetPath().GetBuffer(), pathFor);
				}
			}
			else if(!strcmp(mode, "a"))
			{
				HandleFormatActive(strOperator, pEventView->GetPath().GetBuffer(), pathIn);
			}
			else
			{
				HandleFormatWarning(strOperator, pEventView->GetPath().GetBuffer(), pathIn);
			}
		}
		return TRUE;
	}
};

int main(int argc, char** argv)
{

	/* Parsing the argument from the commandline and checking if valid. */
#pragma region ArgParse
	const char* help = "-e <executable-name> -m <mode> -fo <output-file> -fi <input-file> -f <formatpath>\n\n"
		"-h:\t\t Shows help.\n-e:\t\t Name of the executable to monitor.\n-m:\t\t Choose a mode(Default p):\n"
		"\t\t\tp - Passive mode.\n\t\t\ta - Active mode.\n\t\t\tw - Warning mode.\n"
		"-fo:\t\t Path for output file.(Default .\\log.txt)\n"
		"-fi:\t\t Path for input file with GuardCode format to monitor allowed events.\n"
		"-f:\t\t Path of file that will take events that happend and puts it in GuardCode format.\n\n"
		"Examples:\n\tTo activate active mode it should be like this:\n\t\tGuardCode.exe -e <your_executable_name> -m a -fi Format.gcf\n"
		"\tTo activate warning mode it should be like this:\n\t\tGuardCode.exe -e <your_executable_name> -m w -fi Format.gcf\n"
		"\tTo activate passive mode it should be like this:\n\t\tGuardCode.exe -e <your_executable_name> -m p\n"
		"\tTo activate passive mode and make format it should be like this:\n\t\tGuardCode.exe -e <your_executable_name> -m p -f Format.gcf\n";
	if (argc > 1)
	{
		for (size_t i = 1; i < argc; i++)
		{
			if (!strcmp(argv[i], "-h"))
			{
				printf("Usage:\n%s %s", argv[0], help);
				return 1;
			}
			else if (!strcmp(argv[i], "-e"))
				exe = argv[i + 1];
			else if (!strcmp(argv[i], "-m"))
				mode = argv[i + 1];
			else if (!strcmp(argv[i], "-fo"))
				pathOut = argv[i + 1];
			else if (!strcmp(argv[i], "-fi"))
				pathIn = argv[i + 1];
			else if (!strcmp(argv[i], "-f"))
				pathFor = argv[i + 1];
		}
		if (((!strcmp(mode, "a") || !strcmp(mode, "w")) && pathIn == NULL) || exe == NULL || (strcmp(mode, "a") && strcmp(mode, "p") && strcmp(mode, "w")))
		{
			SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
			printf("input file or exe are NULL or mode not compatible.\n");
			SetConsoleTextAttribute(hConsole, FOREGROUND_GREY);
			printf("Usage:\n%s %s", argv[0], help);
			return 1;
		}
		printf("executable to monitor: %s mode: %s in: %s out: %s formated-file: %s\n", exe, mode, pathIn, pathOut, pathFor);
	}
	else
	{
		printf("Usage:\n%s %s", argv[0], help);
		return 1;
	}
	if (pathIn != NULL && TestFormat(pathIn))
	{
		if (TestFormat(pathIn) == -1)
		{
			SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
			printf("File doesn't exists");
			SetConsoleTextAttribute(hConsole, FOREGROUND_GREY);
			return 1;
		}
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("Format of file \"%s\" is not valid in line:%d", pathIn, TestFormat(pathIn));
		SetConsoleTextAttribute(hConsole, FOREGROUND_GREY);
		return 1;
	}
#pragma endregion
	size_t sizexe = MultiByteToWideChar(CP_ACP, 0, exe, -1, NULL, 0); 
	wexe = new wchar_t[sizexe];
	MultiByteToWideChar(CP_ACP,0,exe,-1,wexe, sizexe);
	fptrOut = fopen(pathOut, "w");
	if(pathIn)
		fptrIn = fopen(pathIn, "r");
	CEventMgr& Optmgr = Singleton<CEventMgr>::getInstance();
	CMonitorContoller& Monitormgr = Singleton<CMonitorContoller>::getInstance();
	CDrvLoader& Drvload = Singleton<CDrvLoader>::getInstance();

	// Try to initialize the driver to interact with.
	if(!Drvload.Init(TEXT("PROCMON24"), TEXT("procmon.sys"))){
		printf("Failed to init .sys");
		return -1;
	}
	Optmgr.RegisterCallback(new CMyEvent);

	// Try to connect to procmon driver.
	if (!Monitormgr.Connect()){
		LogMessage(L_ERROR, TEXT("Cannot connect to procmon driver"));
		return -1;
	}

	// try to start monitor.
	Monitormgr.SetMonitor(TRUE, TRUE, FALSE);
	if (!Monitormgr.Start()){
		LogMessage(L_ERROR, TEXT("Cannot start the mointor"));
		return -1;
	}
	printf("Press any key to end monitoring...\n");
	_getch();
	
	// try to stop the monitor.
	Monitormgr.Stop();
	Monitormgr.Destory();
	for (int i = 0; i < MAX_THREADS; i++)
	{
		WaitForSingleObject(hThreadArray[i], INFINITE);
		CloseHandle(hThreadArray[i]);
		//printf("threandnum:%d\n", i);
		/*if (pDataArray[i] != NULL)
		{
			HeapFree(GetProcessHeap(), 0, pDataArray[i]);
			pDataArray[i] = NULL;    // Ensure address is not reused.
		}*/
	}
	fclose(fptrOut);
	if (pathFor != NULL)
	{
		FILE* fptrFor;
		fptrFor = fopen(pathFor, "a");
		fprintf(fptrFor, "#\"Drop any any\"\n");
		fprintf(fptrFor, "[-].*\n");
		fclose(fptrFor);
	}
	LogMessage(L_INFO, TEXT("!!!!!Monitor stop process will die in one minute!!!!"));
	Sleep(1000);
	return 0;
}
