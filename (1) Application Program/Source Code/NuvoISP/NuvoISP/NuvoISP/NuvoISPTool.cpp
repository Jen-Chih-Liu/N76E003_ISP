
// ISPTool.cpp : �w�q���ε{�������O�欰�C
//

#include "stdafx.h"
#include "NuvoISPTool.h"
#include "Lang.h"
#include "DlgInterfaceSel.h"


#include "DlgNuvoISP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CISPToolApp

BEGIN_MESSAGE_MAP(CISPToolApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CISPToolApp �غc

CISPToolApp::CISPToolApp()
{
	// TODO: �b���[�J�غc�{���X�A
	// �N�Ҧ����n����l�]�w�[�J InitInstance ��
}


// �Ȧ����@�� CISPToolApp ����

CISPToolApp theApp;


// CISPToolApp ��l�]�w

BOOL CISPToolApp::InitInstance()
{
	CWinApp::InitInstance();

	AfxInitRichEdit();
	
	//AllocConsole();
	//SetConsoleTitle(_T("[Clientside dumper]: Console"));
	//freopen("CONOUT$", "w+t", stdout);
	//freopen("CONOUT$", "w+t", stderr);
	//freopen("CONIN$", "r+t", stdin);	

	// �إߴ߼h�޲z���A�H����ܤ���]�t
	// ����߼h���˵��δ߼h�M���˵�����C
	CShellManager *pShellManager = new CShellManager;

	// �зǪ�l�]�w
	// �p�G�z���ϥγo�ǥ\��åB�Q���
	// �̫᧹�����i�����ɤj�p�A�z�i�H
	// �q�U�C�{���X�������ݭn����l�Ʊ`���A
	// �ܧ��x�s�]�w�Ȫ��n�����X
	// TODO: �z���ӾA�׭ק惡�r��
	// (�Ҧp�A���q�W�٩β�´�W��)
	SetRegistryKey(_T("���� AppWizard �Ҳ��ͪ����ε{��"));

	//CISPToolDlg dlg;
	//m_pMainWnd = &dlg;
	//INT_PTR nResponse = dlg.DoModal();
	//CDlg_NUC4xx dlg = CDlg_NUC4xx();
	//CNuvoISPDlg isp;
	//isp.DoModal();

	DlgInterfaceSel InterfaceDlg;
	INT_PTR nResponse = InterfaceDlg.DoModal();

	if (nResponse == IDOK)
	{
		unsigned int it = InterfaceDlg.m_Interface;
		CString str = InterfaceDlg.m_ComNum;

		//SetLangID(0x0404);
		AfxSetResourceHandle(m_hInstance);

		CNuvoISPDlg MainDlg;

		MainDlg.SetInterface(it, str);
		nResponse = MainDlg.DoModal();
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �b����m��ϥ� [����] �Ӱ���ϥι�ܤ����
		// �B�z���{���X
	}

	// �R���W���ҫإߪ��߼h�޲z���C
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}
	//FreeConsole();
	// �]���w�g������ܤ���A�Ǧ^ FALSE�A�ҥH�ڭ̷|�������ε{���A
	// �ӫD���ܶ}�l���ε{�����T���C
	return FALSE;
}

void CISPToolApp::SetLangID(LANGID langID)
{
	//Free language dll;
	if(m_hLangResouce != NULL)
		FreeLibrary(m_hLangResouce);
	m_hLangResouce = NULL;

	if(langID == 0)
		langID = GetSystemDefaultLangID();

	//File name for language dll;
	TCHAR *pszLangFile = NULL;

	//Check the dll name 
	size_t szLangCount;
	const LANG_DEF_T *pLang = GetLangDefs(&szLangCount);
	for(size_t i = 0; i < szLangCount; ++i)
	{
		if(pLang[i].m_langID == langID)
		{
			pszLangFile = pLang[i].m_pszLangFile;
			break;
		}
	}

	//Load the language dll
	if(pszLangFile != NULL)
		m_hLangResouce = ::LoadLibrary(pszLangFile);

	if (m_hLangResouce != NULL)
		AfxSetResourceHandle(m_hLangResouce); // get resources from the DLL 
	else
		AfxSetResourceHandle(m_hInstance); // get resources from the DLL 
}