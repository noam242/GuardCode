#pragma once

#define MAX_PROCMON_MESSAGE_LEN	0x20000

#ifdef OPENPROCMON

#define PROCMON_PORTNAME L"\\OpenProcessMonitor24Port"
#define PROCMON_DEBUGLOGGER_DEVICE_NAME L"\\device\\OpenProcmonDebugLogger"
#define PROCMON_DEBUGLOGGER_SYMBOL_NAME L"\\DosDevices\\Global\\OpenProcmonDebugLogger"
#define PROCMON_EXTLOGGER_DEVICE_NAME L"\\device\\OpenProcmonExternalLogger"
#define PROCMON_EXTLOGGER_ENABLE_EVENT_NAME L"\\??\\OpenProcmonExternalLoggerEnabled"
#define PROCMON_DEFAULT_LOGFILE L"\\SystemRoot\\OpenProcmon.pmb"

#else
#define PROCMON_PORTNAME L"\\ProcessMonitor24Port"
#define PROCMON_DEBUGLOGGER_DEVICE_NAME L"\\device\\ProcmonDebugLogger"
#define PROCMON_DEBUGLOGGER_SYMBOL_NAME L"\\DosDevices\\Global\\ProcmonDebugLogger"
#define PROCMON_EXTLOGGER_DEVICE_NAME L"\\device\\ProcmonExternalLogger"
#define PROCMON_EXTLOGGER_ENABLE_EVENT_NAME L"\\??\\ProcmonExternalLoggerEnabled"
#define PROCMON_DEFAULT_LOGFILE L"\\SystemRoot\\Procmon.pmb"

#endif


//
// Control code definition
//

#define CTLCODE_MONITOR			0
#define CTLCODE_THREADPOFILING  1


//
// Control Flag for monitor
//

#define CTL_MONITOR_ALL_CLOSE		0x00
#define CTL_MONITOR_PROC_ON			0x01
#define CTL_MONITOR_FILE_ON			0x02
#define CTL_MONITOR_REG_ON			0x04
#define CTL_MONITOR_OLDREG_ON		0x08
#define CTL_MONITOR_EXTLOG_ON		0x10


#pragma pack(1)
typedef struct _FLTMSG_CONTROL_HEAD
{
	ULONG CtlCode;
}FLTMSG_CONTROL_HEAD, *PFLTMSG_CONTROL_HEAD;

//CtlCode == 1
typedef struct _FLTMSG_CONTROL_THREADPROFILING
{
	FLTMSG_CONTROL_HEAD Head;
	LARGE_INTEGER ThreadProfile;
}FLTMSG_CONTROL_THREADPROFILING, *PFLTMSG_CONTROL_THREADPROFILING;

//CtlCode == 0
typedef struct _FLTMSG_CONTROL_FLAGS
{
	FLTMSG_CONTROL_HEAD Head;
	ULONG Flags;
}FLTMSG_CONTROL_FLAGS, *PFLTMSG_CONTROL_FLAGS;
#pragma pack()

typedef enum _LOG_MONITOR_TYPE
{
	MONITOR_TYPE_POST = 0,
	MONITOR_TYPE_PROCESS = 1,
	MONITOR_TYPE_REG = 2,
	MONITOR_TYPE_FILE = 3,
	MONITOR_TYPE_PROFILING = 4,
}LOG_MONITOR_TYPE;

typedef enum _LOG_PROCESS_NOTIFY_TYPE
{
	NOTIFY_PROCESS_INIT = 0, //ProcmonFillProcessInfo(LOG_PROCESSCREATE_INFO)
	NOTIFY_PROCESS_CREATE = 1,   //ProcmonFillProcessInfo(LOG_PROCESSCREATE_INFO)
	NOTIFY_PROCESS_EXIT = 2,  //CreateProcessNotifyRoutineCommon
	NOTIFY_THREAD_CREATE = 3, // 
	NOTIFY_THREAD_EXIT = 4,
	NOTIFY_IMAGE_LOAD = 5, //LoadImageNotifyRoutine(LOG_LOADIMAGE_INFO)
	NOTIFY_PROCESS_THREADPERFORMANCE = 6,
	NOTIFY_PROCESS_START = 7, //ProcmonFillProcessInfo(LOG_PROCESSCREATEEXT_INFO)
	NOTIFY_PROCESS_PERFORMANCE = 8, //LOG_PROCESSBASIC_INFO
	NOTIFY_SYSTEM_PERFORMANCE = 9, //LOG_SYSTEMPERF_INFO
}LOG_PROCESS_NOTIFY_TYPE;

typedef enum _LOG_PROFILING_NOTIFY_TYPE
{
	NOTIFY_THREAD_PROFILING = 0, //MONITOR_TYPE_THREAD_PROFILING == 4
	NOTIFY_PROCESS_PROFILING,    //MONITOR_TYPE_THREAD_PROFILING == 4
	NOTIFY_PROFILING_DEBUG,
}LOG_PROFILING_NOTIFY_TYPE;


typedef enum _LOG_REG_NOTIFY_TYPE
{
	NOTIFY_REG_OPENKEYEX = 0,
	NOTIFY_REG_CREATEKEYEX,
	NOTIFY_REG_KEYHANDLECLOSE,
	NOTIFY_REG_QUERYKEY,
	NOTIFY_REG_SETVALUEKEY,
	NOTIFY_REG_QUERYVALUEKEY,
	NOTIFY_REG_ENUMERATEVALUEKEY,
	NOTIFY_REG_ENUMERATEKEY,
	NOTIFY_REG_SETINFORMATIONKEY,
	NOTIFY_REG_DELETEKEY,
	NOTIFY_REG_DELETEVALUEKEY,
	NOTIFY_REG_FLUSHKEY,
	NOTIFY_REG_LOADKEY,
	NOTIFY_REG_UNLOADKEY,
	NOTIFY_REG_RENAMEKEY,
	NOTIFY_REG_QUERYMULTIPLEVALUEKEY,
	NOTIFY_REG_SETKEYSECURITY,
	NOTIFY_REG_QUERYKEYSECURITY,
}LOG_REG_NOTIFY_TYPE;


//
// MAJORFUNCTION + 20, //for file
//


typedef struct _LOG_BUFFER
{
	LARGE_INTEGER DataTime;
	LIST_ENTRY List;
	ULONG Length;
	UCHAR Text[1];
}LOG_BUFFER, *PLOG_BUFFER;

#pragma pack(1)
typedef struct _LOG_ENTRY
{
	/*00*/LONG ProcessSeq;
	/*04*/ULONG ThreadId;
	/*08*/USHORT MonitorType;
	/*0A*/USHORT field_A;
	/*0c*/USHORT NotifyType;
	/*0E*/USHORT field_E;
	/*10*/LONG Sequence;
	/*14*/ULONG field_14;
	/*18*/ULONG field_18;
	/*1C*/LARGE_INTEGER Time;
	/*24*/NTSTATUS Status;
	/*28*/USHORT nFrameChainCounts;
	/*2A*/USHORT field_2A;
	/*2C*/ULONG DataLength;
	/*30*/ULONG field_30;
}LOG_ENTRY, *PLOG_ENTRY;


#define TO_EVENT_DATA(_type, _entry) (_type)((PUCHAR)((PLOG_ENTRY)_entry + 1) + ((PLOG_ENTRY)_entry)->nFrameChainCounts * sizeof(PVOID))
#define CALC_ENTRY_SIZE(_entry) (((PLOG_ENTRY)_entry)->DataLength + \
								(sizeof(PVOID) * ((PLOG_ENTRY)_entry)->nFrameChainCounts) + \
								sizeof(LOG_ENTRY));

typedef struct _LOG_PROCESSCREATE_INFO
{
	ULONG Seq;
	ULONG ProcessId;
	ULONG ParentProcSeq;
	ULONG ParentId;
	ULONG SessionId;
	ULONG IsWow64;
	LARGE_INTEGER CreateTime;
	LUID AuthenticationId;
	ULONG TokenVirtualizationEnabled;
	UCHAR SidLength;
	UCHAR IntegrityLevelSidLength;
	USHORT ProcNameLength;
	USHORT CommandLineLength;
	USHORT UnKnown1;
}LOG_PROCESSCREATE_INFO, *PLOG_PROCESSCREATE_INFO;

typedef struct _LOG_PROCESSSTART_INFO
{
	ULONG ParentId;
	USHORT CommandLineLength;
	USHORT CurrentDirectoryLength;
	ULONG EnvironmentLength;
}LOG_PROCESSSTART_INFO, *PLOG_PROCESSSTART_INFO;

typedef struct _LOG_THREADEXIT_INFO
{
	ULONG ExitStatus;
	LARGE_INTEGER KenrnelTime;
	LARGE_INTEGER UserTime;
}LOG_THREADEXIT_INFO, *PLOG_THREADEXIT_INFO;

typedef struct _LOG_PROCESSBASIC_INFO
{
	ULONG ExitStatus;
	LARGE_INTEGER KenrnelTime;
	LARGE_INTEGER UserTime;
	SIZE_T WorkingSetSize;
	SIZE_T PeakWorkingSetSize;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
}LOG_PROCESSBASIC_INFO, *PLOG_PROCESSBASIC_INFO;

typedef struct _LOG_SYSTEMPERF_INFO
{
	ULONG UnKnown;
	SYSTEM_PERFORMANCE_INFORMATION SystemPerfInfo;
}LOG_SYSTEMPERF_INFO, *PLOG_SYSTEMPERF_INFO;

typedef struct _LOG_PROCESS_PROFILING_INFO
{
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	SIZE_T WorkingSetSize;
	SIZE_T PagefileUsage;

}LOG_PROCESS_PROFILING_INFO, *PLOG_PROCESS_PROFILING_INFO;

typedef struct _LOG_LOADIMAGE_INFO
{
	/*00*/PVOID ImageBase;
	/*08*/ULONG ImageSize;
	/*0C*/USHORT ImageNameLength;
	/*0E*/WCHAR Fill0E[1];
	/*10*/
}LOG_LOADIMAGE_INFO, *PLOG_LOADIMAGE_INFO;

typedef struct _LOG_THREAD_PROFILING_INFO
{
	ULONG UserTimeChange;
	ULONG KernelTimeChange;
	ULONG ContextSwitchesChange;
}LOG_THREAD_PROFILING_INFO, *PLOG_THREAD_PROFILING_INFO;

typedef struct _LOG_FILE_OPT
{
	UCHAR MinorFunction;
	UCHAR Fill1[7];
	ULONG IopbFlag;
	ULONG Flags;
#if 0
	PVOID Argument1;
	PVOID Argument2;
	PVOID Argument3;
	PVOID Argument4;
	PVOID Argument5;
	LARGE_INTEGER Argument6;
#endif
	FLT_PARAMETERS FltParameter;
	USHORT NameLength;
	UCHAR Fill42[2];
	WCHAR Name[1];
}LOG_FILE_OPT, *PLOG_FILE_OPT;

typedef struct _LOG_REG_OPT
{
	UCHAR MinorFunction;
	UCHAR Fill1[7];
	ULONG IopbFlag;
	ULONG Flags;
	WCHAR Name[1];
#if 0
	PVOID Argument1;
	PVOID Argument2;
	PVOID Argument3;
	PVOID Argument4;
	PVOID Argument5;
	LARGE_INTEGER Argument6;
#endif
	FLT_PARAMETERS FltParameter;
	USHORT NameLength;
	UCHAR Fill42[2];
}LOG_REG_OPT, * PLOG_REG_OPT;

typedef struct _LOG_FILE_NAME_COMMON
{
	USHORT FileNameLength;
}LOG_FILE_NAME_COMMON, *PLOG_FILE_NAME_COMMON;

typedef struct _LOG_FILE_LOCKCONTROL
{
	LONGLONG Length;
}LOG_FILE_LOCKCONTROL, *PLOG_FILE_LOCKCONTROL;

typedef struct _LOG_FILE_ACQUIREFORMODIFIEDPAGEWRITER
{
	LARGE_INTEGER EndingOffset;
}LOG_FILE_ACQUIREFORMODIFIEDPAGEWRITER, *PLOG_FILE_ACQUIREFORMODIFIEDPAGEWRITER;

typedef struct _LOG_FILE_CREATE
{
	ACCESS_MASK DesiredAccess;
	ULONG UserTokenLength;
}LOG_FILE_CREATE, *PLOG_FILE_CREATE;

typedef struct _LOG_REG_SETVALUEKEY
{
	USHORT KeyNameLength;
	USHORT Fill02;
	ULONG Type;
	ULONG DataSize;
	USHORT CopySize;
	USHORT Fill0E;
}LOG_REG_SETVALUEKEY, *PLOG_REG_SETVALUEKEY;

typedef struct _LOG_REG_DELETEVALUEKEY
{
	USHORT KeyNameLength;
}LOG_REG_DELETEVALUEKEY, *PLOG_REG_DELETEVALUEKEY;

typedef struct _LOG_REG_SETINFORMATIONKEY
{
	USHORT KeyNameLength;
	USHORT Fill02;
	KEY_SET_INFORMATION_CLASS KeySetInformationClass;
	ULONG KeySetInformationLength;
	USHORT CopySize;
	USHORT Fill0E;
}LOG_REG_SETINFORMATIONKEY, *PLOG_REG_SETINFORMATIONKEY;

typedef struct _LOG_REG_RENAMEKEY
{
	USHORT KeyNameLength;
	USHORT NewNameLength;
}LOG_REG_RENAMEKEY, *PLOG_REG_RENAMEKEY;


typedef struct _LOG_REG_ENUMERATEKEY
{
	USHORT KeyNameLength;
	USHORT Fill02;
	ULONG Length;
	ULONG Index;
	KEY_INFORMATION_CLASS KeyInformationClass;
}LOG_REG_ENUMERATEKEY, *PLOG_REG_ENUMERATEKEY;

typedef struct _LOG_REG_ENUMERATEVALUEKEY
{
	USHORT KeyNameLength;
	USHORT Fill02;
	ULONG Length;
	ULONG Index;
	KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass;
}LOG_REG_ENUMERATEVALUEKEY, *PLOG_REG_ENUMERATEVALUEKEY;

typedef struct _LOG_REG_QUERYKEY
{
	USHORT KeyNameLength;
	USHORT Fill02;
	ULONG Length;
	KEY_INFORMATION_CLASS KeyInformationClass;
}LOG_REG_QUERYKEY, *PLOG_REG_QUERYKEY;

typedef struct _LOG_REG_QUERYVALUEKEY
{
	USHORT KeyNameLength;
	USHORT Fill02;
	ULONG Length;
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
}LOG_REG_QUERYVALUEKEY, *PLOG_REG_QUERYVALUEKEY;

typedef struct _LOG_REG_CONNMON
{
	USHORT KeyNameLength;
}LOG_REG_CONNMON, *PLOG_REG_CONNMON;

typedef struct _LOG_REG_LOADKEY
{
	USHORT KeyNameLength;
	USHORT SourceFileLength;
}LOG_REG_LOADKEY, *PLOG_REG_LOADKEY;

typedef LOG_REG_CONNMON LOG_REG_UNLOADKEY;
typedef LOG_REG_CONNMON* PLOG_REG_UNLOADKEY;

typedef struct _LOG_REG_CREATEOPENKEY
{
	USHORT KeyNameLength;
	USHORT Fill02;
	ACCESS_MASK DesiredAccess;
}LOG_REG_CREATEOPENKEY, *PLOG_REG_CREATEOPENKEY;

typedef struct _LOG_REG_POSTCREATEOPENKEY
{
	ACCESS_MASK GrantedAccess;
	ULONG Disposition;
}LOG_REG_POSTCREATEOPENKEY, *PLOG_REG_POSTCREATEOPENKEY;

#pragma pack()

#pragma pack(4)
typedef struct _PROCMON_MESSAGE_HEADER
{
	FILTER_MESSAGE_HEADER Header;
	ULONG Length;
}PROCMON_MESSAGE_HEADER, * PPROCMON_MESSAGE_HEADER;
#pragma pack()