#include "CHidIO.h"
#include <tchar.h>
#include <vector>
//#include "dbt.h"

extern "C" {
#include "setupapi.h"
#include "hidsdi.h"
}

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "Setupapi.lib")

HANDLE CHidIO::GetActiveHandle() const
{
    return m_hWriteHandle[m_szActiveDeviceIndex];
}

CHidIO::CHidIO()
    : m_bUseTwoHandle(TRUE)
    , m_hAbordEvent(CreateEvent(NULL,TRUE,FALSE,NULL))
    , m_hReadEvent(CreateEvent(NULL,TRUE,FALSE,NULL))
    , m_hWriteEvent(CreateEvent(NULL,TRUE,FALSE,NULL))
    , m_szActiveDeviceIndex(0)
    , m_hReadHandle()
    , m_hWriteHandle()
{
}
CHidIO::~CHidIO()
{
    CloseDevice();
    CloseHandle(m_hWriteEvent);
    CloseHandle(m_hReadEvent);
    CloseHandle(m_hAbordEvent);
}

size_t CHidIO::GetDeviceLength() const
{
    return m_hReadHandle.size();
}

size_t CHidIO::SetActiveDevice(size_t szIndex)
{
    m_DeviceData.Close();

    size_t szOldIndex = m_szActiveDeviceIndex;
    m_szActiveDeviceIndex = szIndex;

    if(m_szActiveDeviceIndex < m_hReadHandle.size()) {
        std::basic_string<TCHAR> path = m_sNames[m_szActiveDeviceIndex].c_str();
        for(size_t i = 0; i < path.size(); ++i) {
            if(!::isalnum(path[i]))
                path[i] = _T('_');
        }
        m_DeviceData.SetName(path.c_str());
        m_DeviceData.Open();
    }
    return szOldIndex;
}

size_t CHidIO::GetActiveDevice() const
{
    return m_szActiveDeviceIndex;
}

ShareArea<CHidShare> &CHidIO::GetActiveDeviceData()
{
    return m_DeviceData;
}

void CHidIO::CloseDevice()
{
    if(m_bUseTwoHandle) {
        size_t i;

        for(i = 0; i < m_hReadHandle.size(); ++i) {
            if(m_hReadHandle[i] != INVALID_HANDLE_VALUE)
                CancelIo(m_hReadHandle[i]);
        }

        for(i = 0; i < m_hWriteHandle.size(); ++i) {
            if(m_hWriteHandle[i] != INVALID_HANDLE_VALUE)
                CancelIo(m_hWriteHandle[i]);
        }

        for(i = 0; i < m_hReadHandle.size(); ++i) {
            if(m_hReadHandle[i] != INVALID_HANDLE_VALUE) {
                CloseHandle(m_hReadHandle[i]);
                m_hReadHandle[i] = INVALID_HANDLE_VALUE;
            }
        }

        for(i = 0; i < m_hWriteHandle.size(); ++i) {
            if(m_hWriteHandle[i] != INVALID_HANDLE_VALUE) {
                CloseHandle(m_hWriteHandle[i]);
                m_hWriteHandle[i] = INVALID_HANDLE_VALUE;
            }
        }
    } else {
        size_t i;
        for(i = 0; i < m_hReadHandle.size(); ++i) {
            if(m_hReadHandle[i] != INVALID_HANDLE_VALUE)
                CancelIo(m_hReadHandle[i]);
        }

        for(i = 0; i < m_hReadHandle.size(); ++i) {
            if(m_hReadHandle[i] != INVALID_HANDLE_VALUE) {
                CloseHandle(m_hReadHandle[i]);
                m_hReadHandle[i] = INVALID_HANDLE_VALUE;
            }
        }
    }
    m_hReadHandle.clear();
    m_hWriteHandle.clear();
    m_sNames.clear();
}

void CHidIO::CloseInactiveDevice()
{
    if(m_bUseTwoHandle) {
        size_t i;

        for(i = 0; i < m_hReadHandle.size(); ++i) {
            if(m_hReadHandle[i] != INVALID_HANDLE_VALUE
                    && m_szActiveDeviceIndex != i)
                CancelIo(m_hReadHandle[i]);
        }

        for(i = 0; i < m_hWriteHandle.size(); ++i) {
            if(m_hWriteHandle[i] != INVALID_HANDLE_VALUE
                    && m_szActiveDeviceIndex != i)
                CancelIo(m_hWriteHandle[i]);
        }

        for(i = 0; i < m_hReadHandle.size(); ++i) {
            if(m_hReadHandle[i] != INVALID_HANDLE_VALUE
                    && m_szActiveDeviceIndex != i) {
                CloseHandle(m_hReadHandle[i]);
                m_hReadHandle[i] = INVALID_HANDLE_VALUE;
            }
        }

        for(i = 0; i < m_hWriteHandle.size(); ++i) {
            if(m_hWriteHandle[i] != INVALID_HANDLE_VALUE
                    && m_szActiveDeviceIndex != i) {
                CloseHandle(m_hWriteHandle[i]);
                m_hWriteHandle[i] = INVALID_HANDLE_VALUE;
            }
        }
    } else {
        size_t i;
        for(i = 0; i < m_hReadHandle.size(); ++i
                && m_szActiveDeviceIndex != i) {
            if(m_hReadHandle[i] != INVALID_HANDLE_VALUE)
                CancelIo(m_hReadHandle[i]);
        }

        for(i = 0; i < m_hReadHandle.size(); ++i
                && m_szActiveDeviceIndex != i) {
            if(m_hReadHandle[i] != INVALID_HANDLE_VALUE) {
                CloseHandle(m_hReadHandle[i]);
                m_hReadHandle[i] = INVALID_HANDLE_VALUE;
            }
        }
    }
}

BOOL CHidIO::OpenDevice(BOOL bUseTwoHandle, USHORT usVID, USHORT usPID0, USHORT usPID1, USHORT usPID2)
{
    m_bUseTwoHandle = bUseTwoHandle;

    //CString MyDevPathName="";
    TCHAR MyDevPathName[MAX_PATH];

    //�w�q�@��GUID�����c��HidGuid�ӫO�sHID�]�ƪ����f��GUID�C
    GUID HidGuid;
    //�w�q�@��DEVINFO���y�`hDevInfoSet�ӫO�s����쪺�]�ƫH�����X�y�`�C
    HDEVINFO hDevInfoSet;
    //�w�qMemberIndex�A��ܷ�e�j����ĴX�ӳ]�ơA0��ܲĤ@�ӳ]�ơC
    DWORD MemberIndex;
    //DevInterfaceData�A�ΨӫO�s�]�ƪ��X�ʱ��f�H��
    SP_DEVICE_INTERFACE_DATA DevInterfaceData;
    //�w�q�@��BOOL�ܶq�A�O�s��ƽեάO�_��^���\
    BOOL Result;
    //�w�q�@��RequiredSize���ܶq�A�Ψӱ����ݭn�O�s�ԲӫH�����w�Ī��סC
    DWORD RequiredSize;
    //�w�q�@�ӫ��V�]�ƸԲӫH�������c����w�C
    PSP_DEVICE_INTERFACE_DETAIL_DATA	pDevDetailData;
    //�w�q�@�ӥΨӫO�s���}�]�ƪ��y�`�C
    HANDLE hDevHandle;
    HANDLE hReadHandle;
    HANDLE hWriteHandle;
    //�w�q�@��HIDD_ATTRIBUTES�����c���ܶq�A�O�s�]�ƪ��ݩʡC
    HIDD_ATTRIBUTES DevAttributes;

    //��l�Ƴ]�ƥ����
    BOOL MyDevFound=FALSE;

    //��l��Ū�B�g�y�`���L�ĥy�`�C
    m_hReadHandle.clear();
    m_hWriteHandle.clear();
    m_sNames.clear();

    //��DevInterfaceData���c�骺cbSize��l�Ƭ����c��j�p
    DevInterfaceData.cbSize=sizeof(DevInterfaceData);
    //��DevAttributes���c�骺Size��l�Ƭ����c��j�p
    DevAttributes.Size=sizeof(DevAttributes);

    //�ե�HidD_GetHidGuid������HID�]�ƪ�GUID�A�ëO�s�bHidGuid���C
    HidD_GetHidGuid(&HidGuid);

    //�ھ�HidGuid������]�ƫH�����X�C�䤤Flags�ѼƳ]�m��
    //DIGCF_DEVICEINTERFACE|DIGCF_PRESENT�A�e�̪�ܨϥΪ�GUID��
    //���f��GUID�A��̪�ܥu�C�|���b�ϥΪ��]�ơA�]���ڭ̳o�̥u
    //�d��w�g�s���W���]�ơC��^���y�`�O�s�bhDevinfo���C�`�N�]��
    //�H�����X�b�ϥΧ�����A�n�ϥΨ��SetupDiDestroyDeviceInfoList
    //�P���A���M�|�y�����s���|�C
    hDevInfoSet=SetupDiGetClassDevs(&HidGuid,
                                    NULL,
                                    NULL,
                                    DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);

    //AddToInfOut("�}�l�d��]��");
    //�M���]�ƶ��X���C�ӳ]�ƶi��C�|�A�ˬd�O�_�O�ڭ̭n�䪺�]��
    //����ڭ̫��w���]�ơA�Ϊ̳]�Ƥw�g�d�䧹���ɡA�N�h�X�d��C
    //�������V�Ĥ@�ӳ]�ơA�Y�NMemberIndex�m��0�C
    MemberIndex=0;
    while(1) {
        //�ե�SetupDiEnumDeviceInterfaces�b�]�ƫH�����X������s����
        //MemberIndex���]�ƫH���C
        Result=SetupDiEnumDeviceInterfaces(hDevInfoSet,
                                           NULL,
                                           &HidGuid,
                                           MemberIndex,
                                           &DevInterfaceData);

        //�p�G����H�����ѡA�h�����]�Ƥw�g�d�䧹���A�h�X�`���C
        if(Result==FALSE) break;

        //�NMemberIndex���V�U�@�ӳ]��
        MemberIndex++;

        //�p�G����H�����\�A�h�~������ӳ]�ƪ��ԲӫH���C�b����]��
        //�ԲӫH���ɡA�ݭn�����D�O�s�ԲӫH���ݭn�h�j���w�İϡA�o�q�L
        //�Ĥ@���եΨ��SetupDiGetDeviceInterfaceDetail������C�o��
        //���ѽw�İϩM���׳���NULL���ѼơA�ô��Ѥ@�ӥΨӫO�s�ݭn�h�j
        //�w�İϪ��ܶqRequiredSize�C
        Result=SetupDiGetDeviceInterfaceDetail(hDevInfoSet,
                                               &DevInterfaceData,
                                               NULL,
                                               NULL,
                                               &RequiredSize,
                                               NULL);

        //�M��A���t�@�Ӥj�p��RequiredSize�w�İϡA�ΨӫO�s�]�ƸԲӫH���C
        pDevDetailData=(PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(RequiredSize);
        if(pDevDetailData==NULL) { //�p�G���s�����A�h������^�C
            //MessageBox("���s����!");
            SetupDiDestroyDeviceInfoList(hDevInfoSet);
            return FALSE;
        }

        //�ó]�mpDevDetailData��cbSize�����c�骺�j�p�]�`�N�u�O���c��j�p�A
        //���]�A�᭱�w�İϡ^�C
        pDevDetailData->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        //�M��A���ե�SetupDiGetDeviceInterfaceDetail��ƨ�����]�ƪ�
        //�ԲӫH���C�o���եγ]�m�ϥΪ��w�İϥH�νw�İϤj�p�C
        Result=SetupDiGetDeviceInterfaceDetail(hDevInfoSet,
                                               &DevInterfaceData,
                                               pDevDetailData,
                                               RequiredSize,
                                               NULL,
                                               NULL);

        //�N�]�Ƹ��|�ƻs�X�ӡA�M��P�����ӽЪ����s�C
        //MyDevPathName=pDevDetailData->DevicePath;
        _tcscpy_s(MyDevPathName, pDevDetailData->DevicePath);
        free(pDevDetailData);

        //�p�G�եΥ��ѡA�h�d��U�@�ӳ]�ơC
        if(Result==FALSE) continue;

        //�p�G�եΦ��\�A�h�ϥΤ��aŪ�g�X�ݪ�CreateFile���
        //������]�ƪ��ݩʡA�]�AVID�BPID�B���������C
        //���@�ǿW���]�ơ]�ҦpUSB��L�^�A�ϥ�Ū�X�ݤ覡�O�L�k���}���A
        //�ӨϥΤ��aŪ�g�X�ݪ��榡�~�i�H���}�o�ǳ]�ơA�q������]�ƪ��ݩʡC
        hDevHandle=CreateFile(MyDevPathName,
                              NULL,
                              FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

        //�p�G���}���\�A�h����]���ݩʡC
        if(hDevHandle!=INVALID_HANDLE_VALUE) {
            //����]�ƪ��ݩʨëO�s�bDevAttributes���c�餤
            Result=HidD_GetAttributes(hDevHandle,
                                      &DevAttributes);

            //������襴�}���]��
            CloseHandle(hDevHandle);

            //������ѡA�d��U�@��
            if(Result==FALSE) continue;

            //�p�G������\�A�h�N�ݩʤ���VID�BPID�H�γ]�ƪ������P�ڭ̻ݭn��
            //�i�����A�p�G���@�P���ܡA�h�������N�O�ڭ̭n�䪺�]�ơC
            if(DevAttributes.VendorID == usVID
                    && ((DevAttributes.ProductID == usPID0)
                        || (DevAttributes.ProductID == usPID1)
                        || (DevAttributes.ProductID == usPID2))) {
                MyDevFound=TRUE; //�]�m�]�Ƥw�g���
                //AddToInfOut("�]�Ƥw�g���");

                if(bUseTwoHandle) {
                    //��ô��������Ҫ�ҵ��豸���ֱ�ʹ�ö�д��ʽ��֮������������
                    //����ѡ��Ϊ�첽���ʷ�ʽ��
                    //����ʽ���豸
                    hReadHandle=CreateFile(MyDevPathName,
                                           GENERIC_READ,
                                           FILE_SHARE_READ|FILE_SHARE_WRITE,
                                           NULL,
                                           OPEN_EXISTING,
                                           //FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
                                           FILE_ATTRIBUTE_NORMAL,
                                           NULL);
                    //if(hReadHandle!=INVALID_HANDLE_VALUE)AddToInfOut("�����ʴ��豸�ɹ�");
                    //else AddToInfOut("�����ʴ��豸ʧ��");

                    //д��ʽ���豸
                    hWriteHandle=CreateFile(MyDevPathName,
                                            GENERIC_WRITE,
                                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                                            NULL,
                                            OPEN_EXISTING,
                                            //FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL);
                    //if(hWriteHandle!=INVALID_HANDLE_VALUE)AddToInfOut("д���ʴ��豸�ɹ�");
                    //else AddToInfOut("д���ʴ��豸ʧ��");
                } else {
                    hWriteHandle =
                        hReadHandle = CreateFile(MyDevPathName,
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 FILE_SHARE_READ|FILE_SHARE_WRITE,
                                                 NULL,
                                                 OPEN_EXISTING,
                                                 //FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 NULL);
                }

                m_hReadHandle.push_back(hReadHandle);
                m_hWriteHandle.push_back(hWriteHandle);
                m_sNames.push_back(MyDevPathName);
                //�ֶ������¼����ö������ָ̻߳����С���Ϊ����֮ǰ��û�е���
                //�����ݵĺ�����Ҳ�Ͳ��������¼��Ĳ�����������Ҫ���ֶ�����һ
                //���¼����ö������ָ̻߳����С�
                //	SetEvent(ReadOverlapped.hEvent);

                //��ʾ�豸��״̬��
                //SetDlgItemText(IDC_DS,"�豸�Ѵ�");

                //�ҵ��豸���˳�ѭ����������ֻ���һ��Ŀ���豸�����ҵ�����˳�
                //�����ˡ��������Ҫ�����е�Ŀ���豸���г����Ļ�����������һ��
                //���飬�ҵ���ͱ����������У�ֱ�������豸��������ϲ��˳�����
                //break;
                //��������
            }
        }
        //�p�G���}���ѡA�h�d��U�@�ӳ]��
        else continue;
    }

    //�ե�SetupDiDestroyDeviceInfoList��ƾP���]�ƫH�����X
    SetupDiDestroyDeviceInfoList(hDevInfoSet);

    this->SetActiveDevice(0);
    //�p�G�]�Ƥw�g���A�������Өϯ�U�ާ@���s�A�æP�ɸT��}�]�ƫ��s
    return MyDevFound;
}


BOOL CHidIO::ReadFile(char *pcBuffer, size_t szMaxLen, DWORD *pdwLength, DWORD dwMilliseconds)
{
    if(m_szActiveDeviceIndex >= m_hReadHandle.size())
        return FALSE;
    HANDLE hReadHandle = m_hReadHandle[m_szActiveDeviceIndex];
    HANDLE events[2] = {m_hAbordEvent, m_hReadEvent};

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = m_hReadEvent;

    if(pdwLength != NULL)
        *pdwLength = 0;

    if(!::ReadFile(hReadHandle, pcBuffer, szMaxLen, NULL, &overlapped)) {
        DWORD dwError = ::GetLastError();
        if(dwError != ERROR_IO_PENDING) {
            return FALSE;
        }

        /* Wait for pending IO event */
        DWORD dwIndex = WaitForMultipleObjects(2, events, FALSE, dwMilliseconds);
        if(dwIndex == WAIT_OBJECT_0
                || dwIndex == WAIT_OBJECT_0 + 1) {
            ResetEvent(events[dwIndex - WAIT_OBJECT_0]);
            if(dwIndex == WAIT_OBJECT_0) {
                return FALSE;	//Abort event
            }
        } else {
            return FALSE;
        }
    }


    DWORD dwLength = 0;
    //Read OK
    GetOverlappedResult(hReadHandle, &overlapped, &dwLength, TRUE);
    if(pdwLength != NULL)
        *pdwLength = dwLength;

    return TRUE;
}

BOOL CHidIO::WriteFile(const char *pcBuffer, size_t szLen, DWORD *pdwLength, DWORD dwMilliseconds)
{
    if(m_szActiveDeviceIndex >= m_hWriteHandle.size())
        return FALSE;
    HANDLE hWriteHandle = m_hWriteHandle[m_szActiveDeviceIndex];
    HANDLE events[2] = {m_hAbordEvent, m_hWriteEvent};

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = m_hWriteEvent;

    if(pdwLength != NULL)
        *pdwLength = 0;

    if(!::WriteFile(hWriteHandle, pcBuffer, szLen, NULL, &overlapped)) {
        if(::GetLastError() != ERROR_IO_PENDING)
            return FALSE;

        DWORD dwIndex = WaitForMultipleObjects(2, events, FALSE, dwMilliseconds);
        if(dwIndex == WAIT_OBJECT_0
                || dwIndex == WAIT_OBJECT_0 + 1) {
            ResetEvent(events[dwIndex - WAIT_OBJECT_0]);

            if(dwIndex == WAIT_OBJECT_0)
                return FALSE;	//Abort event
        } else
            return FALSE;
    }

    DWORD dwLength = 0;
    //Write OK
    GetOverlappedResult(hWriteHandle, &overlapped, &dwLength, TRUE);
    if(pdwLength != NULL)
        *pdwLength = dwLength;

    return TRUE;
}
