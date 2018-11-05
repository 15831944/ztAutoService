#pragma once

#include <windows.h>
#include <vector>
#include <string>
using namespace std;

struct ProcessInfo
{
    DWORD   lPID;             //< ����ID
    TCHAR   szPath[MAX_PATH]; //< ִ���ļ�·��
    TCHAR   szCmd[MAX_PATH];  //< ���в���
    time_t  nStartTime;       //< ��������ʱ��
    int     nReStartHour;     //< ÿ��������ʱ�䣬��λʱ
    int     nRsStartDur;      //< �������
};

/**
 * ���̹������������ӽ��̵���������������ʱ����
 */
class CProcessMgr
{
public:
    CProcessMgr(void);
    ~CProcessMgr(void);

    /**
     * ����һ���ӽ���
     * @param szPath ��ִ���ļ�·��
     * @param szCmd ִ�в���
     * @param nRestartHour ÿ��������ʱ��
     * @param nRsStartDur ���о�������ʱ������
     * @return true�ɹ���falseʧ��
     */
    bool AddTask(PTCHAR szPath, PTCHAR szCmd, int nRestartHour, int nRsStartDur);

    /**
     * �����߳�
     */
    bool ProtectRun();

    /**
     * �ر������ӽ��̲��˳�
     */
    void Stop();

private:
    /**
     * �����ӽ���
     */
    bool RunChild(ProcessInfo* pro);

    /**
     * �������̣���ͨ���ܵ�д������
     * @param nNum ͬһ���������ϵĽ���������
     * @param strDevInfo �ý��̴�����豸����Ϣ
     * @return true�ɹ���falseʧ��
     */
    bool CreateChildProcess(PTCHAR szPath, PTCHAR szCmd, DWORD& lPID);

    /**
     * ����һ������
     * @param lPID ����ID
     */
    bool Find(DWORD lPID);

    /**
     * ����һ������
     * @param lPID ����ID
     */
    bool Kill(DWORD lPID);

    vector<ProcessInfo*>    m_vecProcess;       //< �ӽ�����Ϣ,���̲߳���Ҫ��
};