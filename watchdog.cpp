// watchdog.cpp : �������̨Ӧ�ó������ڵ㡣
// Create by zyao @20190527 for  watchdog  linux&windowsӦ���û��ػ�����
// QQ: 1041367859
// ������վ��www.kingstargold.com
#include "stdafx.h"
#include "inifile.h"
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <assert.h>  
#include <ctype.h>  
#include <errno.h> 
#include <string>
#include <iostream>

#ifdef WIN32
#include <direct.h>
#include <windows.h>
#include <tlhelp32.h>
#include <process.h> 
#include <tchar.h> 
#else
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#endif


using namespace std;


//��λ��
#ifdef WIN32
#define mysleep(a) Sleep(a*1000)
#else
#define mysleep(a) sleep(a)
#endif


typedef struct _unit_
{
	_unit_()
	{
		memset(proc,0,sizeof(proc));
		memset(startcmd, 0, sizeof(startcmd));
		memset(stopcmd, 0, sizeof(stopcmd));
		memset(work_path, 0, sizeof(work_path));
		memset(bin_name, 0, sizeof(bin_name));
	}
	char proc[256];
	char startcmd[32];
	char stopcmd[32];
	char work_path[256];
	char bin_name[256];
}_UNIT;


string GetPathOrURLShortName(string strFullName)
{
	if (strFullName.empty())
	{
		return "";
	}
#ifdef WIN32
	string::size_type iPos = strFullName.find_last_of('\\') + 1;
#else
	string::size_type iPos = strFullName.find_last_of('/') + 1;
#endif
	return strFullName.substr(iPos, strFullName.length() - iPos);
}

string GetPathFromFullName(string strFullName)
{
	if (strFullName.empty())
	{
		return "";
	}
#ifdef WIN32
	string::size_type iPos = strFullName.find_last_of('\\') + 1;
#else
	string::size_type iPos = strFullName.find_last_of('/') + 1;
#endif
	return strFullName.substr(0, iPos);
}

#ifdef WIN32
int isProcessExist(_UNIT *watch)
{
	int iRet = 0;
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	DWORD dwPid = 0;
	int nProcCount = 0;

	// ��ȡϵͳ�����н��̵Ŀ���
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);          // clean the snapshot object
		return -1;
	}

	// ��ȡ�Լ�����Ľ����������ڱȶ�
	TCHAR szFileFullPath[_MAX_PATH] = { 0 };

	::GetModuleFileName(NULL, static_cast<LPTSTR>(szFileFullPath), _MAX_PATH);

	string szProcName = watch->bin_name;
	
	char szPeProcName[_MAX_PATH] = { 0 };
	do
	{
		// �������̿��գ��ȶ��Ƿ���ͬ������
		memset(szPeProcName, 0, _MAX_PATH);

		// wchar ת char*
		sprintf(szPeProcName, "%s", pe32.szExeFile);

		if (0 == strcmp(szProcName.c_str(), szPeProcName))
		{
			dwPid = pe32.th32ProcessID;

			++nProcCount;
			if (nProcCount > 0)
			{
				// ���̴���
				iRet = 1;
				break;
			}
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);

	return iRet;
}
#endif



/**************************************************
��������:	�������Ƿ�������
�������:	watch	�����̲���
�������:	char *pRetMsg			������Ϣ
����ֵ:��������
***************************************************/
int is_alive(_UNIT *watch, char *pRetMsg)
{
	int	iNum = 0;
	
	char cmd[100];
	

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "ps -C %s | wc -l", watch->bin_name);
#ifdef WIN32

	iNum = isProcessExist(watch);
#else
	char sBuf[100];
	FILE *pPipe;
	if ((pPipe = popen(cmd, "r")) == NULL)
	{
		strcpy(pRetMsg, "popen error!");
		return -1;
	}

	memset(sBuf, 0, sizeof(sBuf));
	fgets(sBuf, 99, pPipe);
	pclose(pPipe);
	iNum = atoi(sBuf);
#endif

	if (iNum > 0)
	{
		sprintf(pRetMsg, "program %s is already running, stop it first!", watch->bin_name);
	}
	else if (iNum < 1)
	{
		sprintf(pRetMsg, "program %s is not running, start it first!", watch->bin_name);
	}
#ifdef WIN32
	return iNum;
#else
	return iNum -1;
#endif
	
}

// void check_proc(_UNIT *watch)
// {
// 	int num = 0;
// 
// }



/*****************************
��������:	�ػ����̳�ʼ��
�������:	��
�������:	��
����ֵ:		��
*****************************/
void InitDaemon()
{
#ifdef WIN32
	//�����һ�仰�����ؿ���̨����
	#pragma comment(linker,"/subsystem:\"windows\"  /entry:\"mainCRTStartup\"" ) 
#else

	int pid = 0;
	if((pid = fork()) > 0)
	{
		exit(0);
	}
	setsid();
	if((pid = fork()) > 0)
	{
		exit(0);
	}
	
#endif
	return;
}


#define WIN_DELIMITER '\\'
#define NIX_DELIMITER '/'
char *ExtractFilePath(char *path, const char *fname)
{
	const char *p = 0;
	for (const char *q = fname; *q; q++)
	{
		switch (*q)
		{
		case WIN_DELIMITER:
		case NIX_DELIMITER:
			p = q;
		}
	}
	if (p)
	{
		int len = p - fname + 1;
		memcpy(path, fname, len);
		path[len] = 0;
	}
	else
	{
		*path = 0;
	}
	return path;
}


int start_process(_UNIT *watch)
{
#ifdef WIN32
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	//si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	//si.wShowWindow = SW_HIDE;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	//CREATE_NEW_CONSOLE �����µ�cmd��������
	if (!CreateProcess(NULL, watch->proc, NULL, NULL, true, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		printf("Create err:file[%s] not exist or permission denied \n", watch->proc);
		return 0;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
#else
	pid_t pid;
	if ((pid = fork()) < 0)	//�����ӽ���ʧ��
	{
		return -1;
	}
	else if (pid == 0) //�ӽ���
	{
		//�ӽ���������ҵ�������
		int iResult;
		iResult = execl(watch->proc, watch->bin_name, NULL);
		if (iResult < 0)								//����ʧ��
		{
			printf("��������ʧ��[%s]\n",watch->proc);
			return -1;
		}
	}
	else //������
	{
		signal(SIGCHLD, SIG_IGN);
		return 0;
	}
#endif
	return 0;
}



int main(int argc, char* argv[])
{
	//��ʱ����������
	//if (argc )
	//{
	//}
	InitDaemon();

	int proc_count = 0;
	int i = 0;
	char err_msg[1024] = {0};
	_UNIT watch[10];
	//char proc[256] = {0};
	char temp[1024] = {0};
	IniFile cfg;
	if (cfg.Load("watchdog.cfg"))
	{
		char szBuf[128] = { 0 };
		sprintf(szBuf, "���ܴ������ļ�<%s>,err=%s\n", "watchdog.cfg",cfg.GetErrMsg().c_str());
		printf(szBuf);
		return -1;
	}
	//��ȡ����
	//proc_count = cfg.ReadInt("watch", "count", 0);
	string str;
	cfg.GetIntValue("watch", "count", &proc_count);
	for (i = 0; i < proc_count; i++)
	{
		//ȡ��һ������
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "proc%d", i + 1);
		//cfg.ReadString("watch", temp, "", watch[i].proc, sizeof(watch[i].proc) - 1);
		str = "";
		cfg.GetStringValue("watch", temp, &str);
		if (str.empty())
		{
			printf("[watch]��%s�ڵ��ǿ�ֵ\n", temp);
			//return 0;
		}
		strcpy(watch[i].proc, str.c_str());

		//ȡ�ڶ�������
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "startcmd%d", i + 1);
		//cfg.ReadString("watch", temp, "", watch[i].startcmd, sizeof(watch[i].startcmd) - 1);
		str = "";
		cfg.GetStringValue("watch", temp, &str);
		if (str.empty())
		{
			printf("[watch]��%s�ڵ��ǿ�ֵ\n", temp);
			//return 0;
		}
		strcpy(watch[i].startcmd, str.c_str());
		//ȡ����������
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "stopcmd%d", i + 1);
		//cfg.ReadString("watch", temp, "", watch[i].stopcmd, sizeof(watch[i].stopcmd) - 1);
		str = "";
		cfg.GetStringValue("watch", temp, &str);
		if (str.empty())
		{
			printf("[watch]��%s�ڵ��ǿ�ֵ\n", temp);
			//return 0;
		}
		strcpy(watch[i].stopcmd, str.c_str());
	}
	while (1)
	{
		printf("Loop!!!\n");
		mysleep(2);
		for (i = 0; i < proc_count; i++)
		{
			string work_path;
			string bin_name;
			work_path = GetPathFromFullName(watch[i].proc);
			bin_name = GetPathOrURLShortName(watch[i].proc);
			strcpy(watch[i].work_path,work_path.c_str());
			strcpy(watch[i].bin_name,bin_name.c_str());
			memset(err_msg,0,sizeof(err_msg));
			int ret = is_alive(&watch[i], err_msg);
			if (ret == 0) //���򲻴�����
			{
				int iRet = 0;
				//���ù���Ŀ¼
#ifdef WIN32
				
				ExtractFilePath(watch[i].work_path, watch[i].proc);          //��ȫ·���е��ļ���ȥ����ֻ����֮ǰ��Ŀ¼
				//SetCurrentDirectory(watch[i].work_path);
				iRet = _chdir(watch[i].work_path);//���ʧ��˵�������ļ���utf-8��ʽ����Ҫתansi��ʽ
#else
				iRet = chdir(watch[i].work_path);
#endif
				if (iRet != 0)
				{
					printf("�л�����Ŀ¼ʧ��[%s]\n", watch[i].work_path);
					printf("·�������ڻ������ļ�һ��Ҫ��ANSI���룬����·�������Ļ����ù���·��ʧ��\n");
					continue;
				}
				printf("workpath:%s\n",watch[i].work_path);
				//��������
//#ifdef WIN32
				start_process(&watch[i]);
// #else
// 				printf("start1");
// 				system(watch[i].proc);
// 				printf("start2");
// #endif
				
			}
			else if (ret > 0) //������
			{
				printf("Program is [%s] alive\n",watch[i].bin_name);
			}
			else
			{
				printf("failure\n");
			}
		}
	}
	return 0;
}

