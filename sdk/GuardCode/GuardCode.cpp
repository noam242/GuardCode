// sdktest.cpp : This file contains the 'main' function. Program execution begins and ends there.
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

std::string makeRegex(std::string str)
{
	replace_all(str, "\\", "\\\\");
	replace_all(str, "(", "\\(");
	replace_all(str, ")", "\\)");
	replace_all(str, ".", "\\.");
	return str;
}

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

std::ptrdiff_t TestFormat(char* path)
{
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
						printf("Killing the monitoring process in one minute...");
						Sleep(100);
						TerminateProcess(OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, GetCurrentProcessId()), 0);
						//killProcessByName("example.exe");
					}
				}
				numline++;
				line = "";
			}
		} while (ch != EOF);
		return 1;
	}
}

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
							LogMessage(L_ERROR, TEXT("Do %s for %s\n"), lpOPt, buffer);
							_ftprintf(fptrOut, TEXT("[!]Would be killed for: Do %s for %s\n"), lpOPt, buffer);
						}
						else
						{
							LogMessage(L_ERROR, TEXT("Do %s\n"), lpOPt);
							_ftprintf(fptrOut, TEXT("[!]Would be killed for: Do %s\n"), lpOPt);
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

class CMyEvent : public IEventCallback
{
public:
	virtual BOOL DoEvent(const CRefPtr<CEventView> pEventView)
	{
		ULONGLONG Time = pEventView->GetStartTime().QuadPart;
		if (wcscmp(pEventView->GetPath().GetBuffer(), L"TODO") && wcscmp(pEventView->GetProcessName().GetBuffer(), wexe) == 0)
		{
			CString strOperator;
			LPCTSTR lpOPt = StrMapOperation(pEventView->GetPreEventEntry());
			if (!lpOPt) {
				DWORD dwClass = pEventView->GetEventClass();
				DWORD dwOperator = pEventView->GetEventOperator();
				strOperator.Format(TEXT("%d:%d"), dwClass, dwOperator);
			}
			else {
				strOperator = lpOPt;
			}
			/*FILETIME f;
			SYSTEMTIME s;
			f.dwHighDateTime = pEventView->GetStartTime().HighPart;
			f.dwLowDateTime = pEventView->GetStartTime().LowPart;
			FileTimeToSystemTime(&f,&s);
			char buffer[1024];
			SystemTimeToTzSpecificLocalTime(NULL,&s,&s);
			//strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", f);
			time_t realtime = (time_t)(Time / 1000000);
			GetTimeFormatA(LOCALE_USER_DEFAULT,0,&s, "hh':'mm':'ss:ms tt",buffer,sizeof(buffer));
			printf("%s\n", buffer);*/
			if (wcslen(pEventView->GetPath().GetBuffer()) != 0)
			{
				LogMessage(L_INFO, TEXT("%llu Process %s Do %s for %s, %s"), Time, pEventView->GetProcessName().GetBuffer(), strOperator,
					pEventView->GetPath().GetBuffer(),
					pEventView->GetOperationStrResult(emResult));
				_ftprintf(fptrOut, TEXT("[+]%llu Process %s Do %s for %s, %s\n"), Time, pEventView->GetProcessName().GetBuffer(), strOperator,
					pEventView->GetPath().GetBuffer(),
					pEventView->GetOperationStrResult(emResult));
			}
			else
			{
				LogMessage(L_INFO, TEXT("%llu Process %s Do %s, %s"), Time, pEventView->GetProcessName().GetBuffer(), strOperator,
					pEventView->GetOperationStrResult(emResult));
				_ftprintf(fptrOut, TEXT("[+]%llu Process %s Do %s, %s\n"), Time, pEventView->GetProcessName().GetBuffer(), strOperator,
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
		/*if (!strcmp(mode,"p"))
		{
			if (pathFor != NULL)
			{
				if (wcscmp(pEventView->GetPath().GetBuffer(), L"TODO") && wcscmp(pEventView->GetProcessName().GetBuffer(), wexe) == 0)
				{
					DWORD dwClass = pEventView->GetEventClass();
					DWORD dwOperator = pEventView->GetEventOperator();
					CString strOperator;
					LPCTSTR lpOPt = StrMapOperation(pEventView->GetPreEventEntry());
					if (!lpOPt) {
						strOperator.Format(TEXT("%d:%d"), dwClass, dwOperator);
					}
					else {
						strOperator = lpOPt;
					}
					if (wcslen(pEventView->GetPath().GetBuffer()) != 0)
					{
						LogMessage(L_INFO, TEXT("%llu Process %s Do %s for %s, %s"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
							pEventView->GetPath().GetBuffer(),
							pEventView->GetOperationStrResult(emResult));
						_ftprintf(fptrOut, TEXT("%llu Process %s Do %s for %s, %s\n"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
							pEventView->GetPath().GetBuffer(),
							pEventView->GetOperationStrResult(emResult));
					}
					else
					{
						LogMessage(L_INFO, TEXT("%llu Process %s Do %s, %s"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
							pEventView->GetOperationStrResult(emResult));
						_ftprintf(fptrOut, TEXT("%llu Process %s Do %s, %s\n"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
							pEventView->GetOperationStrResult(emResult));
					}
					HandleFormatPassive(lpOPt, pEventView->GetPath().GetBuffer(), pathFor);
					/*if (HandleFormat(lpOPt, pEventView->GetPath().GetBuffer(),pathFor) == 0)
					{
						FILE* fptrFor;
						fptrFor = fopen(pathFor, "a");
						_ftprintf(fptrFor, TEXT("Do %s for %s\n"), lpOPt, pEventView->GetPath().GetBuffer());
						fclose(fptrFor);
					}*/
				/*}
			}
			else
			{
				if (wcscmp(pEventView->GetPath().GetBuffer(), L"TODO") && wcscmp(pEventView->GetProcessName().GetBuffer(), wexe) == 0)
				{
					//foo();
					//threadnum++;
					DWORD dwClass = pEventView->GetEventClass();
					DWORD dwOperator = pEventView->GetEventOperator();
					CString strOperator;
					LPCTSTR lpOPt = StrMapOperation(pEventView->GetPreEventEntry());
					if (!lpOPt) {
						strOperator.Format(TEXT("%d:%d"), dwClass, dwOperator);
					}
					else {
						strOperator = lpOPt;
					}
					if (wcslen(pEventView->GetPath().GetBuffer()) != 0)
					{
						LogMessage(L_INFO, TEXT("%llu Process %s Do %s for %s, %s"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
							pEventView->GetPath().GetBuffer(),
							pEventView->GetOperationStrResult(emResult));
						_ftprintf(fptrOut, TEXT("%llu Process %s Do %s for %s, %s\n"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
							pEventView->GetPath().GetBuffer(),
							pEventView->GetOperationStrResult(emResult));
					}
					else
					{
						LogMessage(L_INFO, TEXT("%llu Process %s Do %s, %s"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
							pEventView->GetOperationStrResult(emResult));
						_ftprintf(fptrOut, TEXT("%llu Process %s Do %s, %s\n"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
							pEventView->GetOperationStrResult(emResult));
					}
					//m_viewList.push_back(pEventView);
				}
			}
		}
		else
		{
			if (wcscmp(pEventView->GetPath().GetBuffer(), L"TODO") && wcscmp(pEventView->GetProcessName().GetBuffer(), wexe) == 0)
			{
				DWORD dwClass = pEventView->GetEventClass();
				DWORD dwOperator = pEventView->GetEventOperator();
				CString strOperator;
				LPCTSTR lpOPt = StrMapOperation(pEventView->GetPreEventEntry());
				if (!lpOPt) {
					strOperator.Format(TEXT("%d:%d"), dwClass, dwOperator);
				}
				else {
					strOperator = lpOPt;
				}
				if (wcslen(pEventView->GetPath().GetBuffer()) != 0)
				{
					LogMessage(L_INFO, TEXT("%llu Process %s Do %s for %s, %s"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
						pEventView->GetPath().GetBuffer(),
						pEventView->GetOperationStrResult(emResult));
					_ftprintf(fptrOut, TEXT("%llu Process %s Do %s for %s, %s\n"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
						pEventView->GetPath().GetBuffer(),
						pEventView->GetOperationStrResult(emResult));
				}
				else
				{
					LogMessage(L_INFO, TEXT("%llu Process %s Do %s, %s"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
						pEventView->GetOperationStrResult(emResult));
					_ftprintf(fptrOut, TEXT("%llu Process %s Do %s, %s\n"), Time, pEventView->GetProcessName().GetBuffer(), lpOPt,
						pEventView->GetOperationStrResult(emResult));
				}
				HandleFormatActive(lpOPt, pEventView->GetPath().GetBuffer(), pathIn);
			}
		}*/
		return TRUE;
	}
};

int main(int argc, char** argv)
{
	const char* help = "-e <executable-name> -m <mode> -fo <output-file> -fi <input-file> -f <formatpath>\n\n-h:\t\t Shows help.\n-e:\t\t Name of the executable to monitor.\n-m:\t\t Choose a mode(Default p):\n\t\t\tp - Passive mode.\n\t\t\ta - Active mode.\n\t\t\tw - Warning mode.\n-fo:\t\t Path for output file.(Default .\\log.txt)\n-fi:\t\t Path for input file with guard-code format to monitor allowed events.\n-f:\t\t Path of file that will take events that happend and puts it in guard-code format.";
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
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("Format of file \"%s\" not correct in line:%d", pathIn, TestFormat(pathIn));
		SetConsoleTextAttribute(hConsole, FOREGROUND_GREY);
		return 1;
	}
	size_t sizexe = MultiByteToWideChar(CP_ACP, 0, exe, -1, NULL, 0); 
	wexe = new wchar_t[sizexe];
	MultiByteToWideChar(CP_ACP,0,exe,-1,wexe, sizexe);
	fptrOut = fopen(pathOut, "w");
	if(pathIn)
		fptrIn = fopen(pathIn, "r");
	CEventMgr& Optmgr = Singleton<CEventMgr>::getInstance();
	CMonitorContoller& Monitormgr = Singleton<CMonitorContoller>::getInstance();
	CDrvLoader& Drvload = Singleton<CDrvLoader>::getInstance();
	if(!Drvload.Init(TEXT("PROCMON24"), TEXT("procmon.sys"))){
		printf("Failed to init .sys");
		return -1;
	}
	Optmgr.RegisterCallback(new CMyEvent);

	//
	// Try to connect to procmon driver
	//
	if (!Monitormgr.Connect()){
		LogMessage(L_ERROR, TEXT("Cannot connect to procmon driver"));
		return -1;
	}

	//
	// try to start monitor
	//
	Monitormgr.SetMonitor(TRUE, TRUE, FALSE);
	if (!Monitormgr.Start()){
		LogMessage(L_ERROR, TEXT("Cannot start the mointor"));
		return -1;
	}
	_getch();
	
	//
	// try to stop the monitor
	//
	
	Monitormgr.Stop();

	LogMessage(L_INFO, TEXT("!!!!!monitor stop press any key to start!!!!"));
	_getch();

	Monitormgr.Start();

	_getch();
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
	if (pathFor != NULL)
	{
		FILE* fptrFor;
		fptrFor = fopen(pathFor, "a");
		fprintf(fptrFor, "#\"Drop any any\"\n");
		fprintf(fptrFor, "[-].*\n");
		fclose(fptrFor);
	}
	return 0;
}
