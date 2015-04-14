// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

#include <atlbase.h>
#include <atlstr.h>

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <iterator>
#include <algorithm>
#include <functional>

#define MAX_CORE 16

typedef struct _WorkerProcess {
	std::string *instancePath, *csvPath, *studyUid; // command level
	bool integrityStudy;  // Is study integrity?
    HANDLE hProcess, hThread, mutexIdle, mutexRec, hChildStdInWrite; // process level
	HANDLE hLogFile; std::string *logFilePath; // slot level
} WorkerProcess, *PWorkerProcess, *LPWorkerProcess;

bool RedirectMessageLabelEqualWith(const char *equalWith, const char *queueName);
bool SendCommonMessageToQueue(const char *label, const char *body, const long priority, const char *queueName);
HRESULT QLetEveryoneFullControl(LPCWSTR wszFormatNameBuffer);
int commandDispatcher(const char *queueName, int processorNumber);
std::ostream& time_header_out(std::ostream &os);
void autoCleanPacsDiskByStudyDate();
bool deleteDayStudy(const char *dayxml);
bool deleteTree(const char *dirpath);
void resetStatus(const char *queueName);
bool __stdcall captureStdoutToLogStream(std::ostream &flog);
void __stdcall releaseStdout(std::ostream &flog);