
// PCM-DSD_Converter.cpp : �A�v���P�[�V�����̃N���X������`���܂��B
//

#include "stdafx.h"
#include "PCM-DSD_Converter.h"
#include "PCM-DSD_ConverterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPCMDSD_ConverterApp

BEGIN_MESSAGE_MAP(CPCMDSD_ConverterApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPCMDSD_ConverterApp �R���X�g���N�V����

CPCMDSD_ConverterApp::CPCMDSD_ConverterApp()
{
	// TODO: ���̈ʒu�ɍ\�z�p�R�[�h��ǉ����Ă��������B
	// ������ InitInstance ���̏d�v�ȏ��������������ׂċL�q���Ă��������B
}


// �B��� CPCMDSD_ConverterApp �I�u�W�F�N�g�ł��B

CPCMDSD_ConverterApp theApp;


// CPCMDSD_ConverterApp ������

BOOL CPCMDSD_ConverterApp::InitInstance()
{
	CWinApp::InitInstance();


	// �_�C�A���O�ɃV�F�� �c���[ �r���[�܂��̓V�F�� ���X�g �r���[ �R���g���[����
	// �܂܂�Ă���ꍇ�ɃV�F�� �}�l�[�W���[���쐬���܂��B
	CShellManager *pShellManager = new CShellManager;

	// MFC �R���g���[���Ńe�[�}��L���ɂ��邽�߂ɁA"Windows �l�C�e�B�u" �̃r�W���A�� �}�l�[�W���[���A�N�e�B�u��
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// �W��������
	// �����̋@�\���g�킸�ɍŏI�I�Ȏ��s�\�t�@�C����
	// �T�C�Y���k���������ꍇ�́A�ȉ�����s�v�ȏ�����
	// ���[�`�����폜���Ă��������B
	// �ݒ肪�i�[����Ă��郌�W�X�g�� �L�[��ύX���܂��B
	// TODO: ��Ж��܂��͑g�D���Ȃǂ̓K�؂ȕ������
	// ���̕������ύX���Ă��������B
	SetRegistryKey(_T("�A�v���P�[�V���� �E�B�U�[�h�Ő������ꂽ���[�J�� �A�v���P�[�V����"));

	CPCMDSD_ConverterDlg dlg;
	m_pMainWnd = &dlg;

	// �R�}���h���C�������̎擾
	int i;
	int nArgs;
	LPTSTR* lplpszArgs;
	CString param;
#if 1	// GetCommandLine()�� �������̃A�v�������擾�����̂ōŏ��͎̂̂Ă�
	lplpszArgs = CommandLineToArgvW(GetCommandLine(), &nArgs);
	for (i = 1; i < nArgs; i++) {
		param = lplpszArgs[i];
		dlg.m_strCmdLineArray.Add(param.Trim());
	}
#else	// m_lpCmdLine��
	lplpszArgs = CommandLineToArgvW(m_lpCmdLine, &nArgs);
	for (i = 0; i < nArgs; i++) {
		param = lplpszArgs[i];
		dlg.m_strCmdLineArray.Add(param.Trim());
	}
#endif

	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �_�C�A���O�� <OK> �ŏ����ꂽ���̃R�[�h��
		//  �L�q���Ă��������B
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �_�C�A���O�� <�L�����Z��> �ŏ����ꂽ���̃R�[�h��
		//  �L�q���Ă��������B
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "�x��: �_�C�A���O�̍쐬�Ɏ��s���܂����B�A�v���P�[�V�����͗\�������ɏI�����܂��B\n");
		TRACE(traceAppMsg, 0, "�x��: �_�C�A���O�� MFC �R���g���[�����g�p���Ă���ꍇ�A#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS ���w��ł��܂���B\n");
	}

	// ��ō쐬���ꂽ�V�F�� �}�l�[�W���[���폜���܂��B
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// �_�C�A���O�͕����܂����B�A�v���P�[�V�����̃��b�Z�[�W �|���v���J�n���Ȃ���
	//  �A�v���P�[�V�������I�����邽�߂� FALSE ��Ԃ��Ă��������B
	return FALSE;
}

