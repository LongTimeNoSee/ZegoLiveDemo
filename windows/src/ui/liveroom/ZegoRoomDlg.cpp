// TaklDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "resource.h"
#include "ZegoRoomDlg.h"
#include "ZegoLiveDemo.h"
#include <string>
#include <tuple>
#include "ZegoUtility.h"

#include "ZegoLiveDemo.h"
#include "ZegoChatRoom.h"

#include "rapidjson.h"
#include "document.h"
#include "stringbuffer.h"
#include "writer.h"
#include "error/en.h"

#include "ZegoAVDefines.h"
using namespace ZEGO::AV;


#define VOLUME_TIMER_ID   1002

static int g_AVViews[] =
{
	IDC_AVVIEW_PRIMARY,
	IDC_AVVIEW_STUDENT1,
	IDC_AVVIEW_STUDENT2,
	IDC_AVVIEW_STUDENT3,
	IDC_AVVIEW_STUDENT4,
	IDC_AVVIEW_STUDENT5,
	IDC_AVVIEW_STUDENT6,
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CZegoRoomDlg �Ի���

IMPLEMENT_DYNAMIC(CZegoRoomDlg, CDialogEx)

CZegoRoomDlg::CZegoRoomDlg(SettingsPtr curSettings, CWnd* pParent /*=NULL*/):
	CDialogEx(IDD_DIALOG_ROOM, pParent),
	m_pAVSettings(curSettings),
	m_bCKEnableMic(TRUE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	theApp.GetBase().InitChatRoomSdk();
	GetChatRoomSignal().OnUserList().connect(this, &CZegoRoomDlg::OnChatRoomUserList);
	GetChatRoomSignal().OnStreamCreate().connect(this, &CZegoRoomDlg::OnChatRoomStreamCreate);
	GetChatRoomSignal().OnStreamList().connect(this, &CZegoRoomDlg::OnChatRoomStreamList);
	GetChatRoomSignal().OnDisconnected().connect(this, &CZegoRoomDlg::OnChatRoomDisconnected);
	GetChatRoomSignal().OnKickOut().connect(this, &CZegoRoomDlg::OnChatRoomKickout);
	GetChatRoomSignal().OnSendMessage().connect(this, &CZegoRoomDlg::OnChatRoomSendMessage);
	GetChatRoomSignal().OnReceiveMessage().connect(this, &CZegoRoomDlg::OnChatRoomRecvMessage);

	theApp.GetBase().InitAVSdk(m_pAVSettings);
	GetAVSignal().OnLoginChannel().connect(this, &CZegoRoomDlg::OnAVLoginChannel);
	GetAVSignal().OnLogoutChannel().connect(this, &CZegoRoomDlg::OnAVLogoutChannel);
	GetAVSignal().OnPublishStateUpdate().connect(this, &CZegoRoomDlg::OnAVPublishState);
	GetAVSignal().OnPlayStateUpdate().connect(this, &CZegoRoomDlg::OnAVPlayState);
	GetAVSignal().OnPublishQualityUpdate().connect(this, &CZegoRoomDlg::OnAVPublishQuality);
	GetAVSignal().OnPlayQualityUpdate().connect(this, &CZegoRoomDlg::OnAVPlayQuality);
	GetAVSignal().OnAuxInput().connect(this, &CZegoRoomDlg::OnAVAuxInput);
	GetAVSignal().OnDisconnected().connect(this, &CZegoRoomDlg::OnAVDisconnected);
	GetAVSignal().OnReconnected().connect(this, &CZegoRoomDlg::OnAVReconnected);
    GetAVSignal().OnAudioDeviceChanged().connect(this, &CZegoRoomDlg::OnAudioDeviceChanged);
    GetAVSignal().OnVideoDeviceChanged().connect(this, &CZegoRoomDlg::OnVideoDeviceChanged);

	m_bLoginChannel = false;
	m_pAuxData = NULL;
	m_nAuxDataLen = 0;
	m_nAuxDataPos = 0;
}

CZegoRoomDlg::~CZegoRoomDlg()
{
	AV::StopPreview();
	AV::SetPreviewView(NULL);
	AV::StopPublish();
	AV::LogoutChannel();

	theApp.GetBase().UninitAVSdk();

	CHATROOM::LeaveRoom();

	if (m_pChatRoom == nullptr) { return; }

	UserPtr pCurUser = m_pChatRoom->GetCurUser();
	if (pCurUser == nullptr) { return; }

	std::string strChannel = m_pChatRoom->GetChannel();
	std::string strUserId = WStringToUTF8(pCurUser->GetUserId().c_str());
	std::string strStreamId = strChannel + "_" + strUserId;

	StreamPtr pStream = m_pChatRoom->GetPlayingStreamById(pCurUser->IsPrimary(), strStreamId);
	if (pStream != nullptr)
	{
		CHATROOM::ReportStreamAction(2, pStream->GetId().c_str(), strUserId.c_str(), true);
	}
}

void CZegoRoomDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_VISITOR_LIST, m_listVisitors);
    DDX_Control(pDX, IDC_AVVIEW_PRIMARY, m_primaryAVView);
    DDX_Control(pDX, IDC_AVVIEW_STUDENT1, m_student1AVView);
    DDX_Control(pDX, IDC_AVVIEW_STUDENT2, m_student2AVView);
    DDX_Control(pDX, IDC_AVVIEW_STUDENT3, m_student3AVView);
    DDX_Control(pDX, IDC_AVVIEW_STUDENT4, m_studeng4AVView);
    DDX_Control(pDX, IDC_AVVIEW_STUDENT5, m_student5AVView);
    DDX_Control(pDX, IDC_AVVIEW_STUDENT6, m_student6AVView);
    DDX_Control(pDX, IDC_EDIT_MESSAGE, m_edMessage);
    DDX_Control(pDX, IDC_LISTBOX_CHAT, m_lbChatContent);
    DDX_Check(pDX, IDC_CHECK_MICROPHONE, m_bCKEnableMic);
    DDX_Control(pDX, IDC_EDIT_STREAMURL, m_edStreamUrl);
    DDX_Control(pDX, IDC_BUTTON_AUX, m_btnAux);
    DDX_Control(pDX, IDC_COMBO_CURAUDIO, m_cbCurrentAudio);
    DDX_Control(pDX, IDC_COMBO_CURVIDEO, m_cbCurrentVideo);
    DDX_Control(pDX, IDC_BUTTON_CAPTURE, m_btnCapture);
    DDX_Control(pDX, IDC_CAPTURE_VOLUME_PROG, m_captureVolumeProg);
}

BOOL CZegoRoomDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);	// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);	// ����Сͼ��

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if ( pSysMenu != NULL )
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	ModifyStyleEx(0, WS_EX_APPWINDOW);

	UpdateData(FALSE);

	CString strTile;
	strTile.Format(_T("ֱ����(%s)"), m_pChatRoom->GetRoomTitle().c_str());
	SetWindowText(strTile);

	m_edMessage.SetCueBanner(_T("�Ļ����..."));

	m_listVisitors.SetCellMargin(1.2);
	m_listVisitors.EnableVisualStyles(true);
	m_listVisitors.InsertHiddenLabelColumn();
	m_listVisitors.SetEmptyMarkupText(_T("��"));

	m_listVisitors.InsertColumnTrait(1, L"��ɫ", LVCFMT_CENTER, 80, 1, NULL);
	CRect rtList;
	m_listVisitors.GetWindowRect(&rtList);
	m_listVisitors.InsertColumnTrait(2, L"�ǳ�", LVCFMT_LEFT, rtList.Width()-80-4, 2, NULL);

	m_btnAux.EnableWindow(FALSE);
    m_btnCapture.EnableWindow(FALSE);

	RefreshVisitorList();

	m_avaliablePrimaryView.push(0);
	m_avaliableStudentView.push(6);
	m_avaliableStudentView.push(5);
	m_avaliableStudentView.push(4);
	m_avaliableStudentView.push(3);
	m_avaliableStudentView.push(2);
	m_avaliableStudentView.push(1);
	
	UserPtr pCurUser = m_pChatRoom->GetCurUser();
	if (pCurUser != nullptr)
	{
		std::string strCurUserId = WStringToUTF8(pCurUser->GetUserId().c_str());
		std::string strCurUserName = WStringToUTF8(pCurUser->GetUserName().c_str());
		std::string strChannel = m_pChatRoom->GetChannel();
		AV::LoginChannel(strCurUserId.c_str(), strCurUserName.c_str(), strChannel.c_str(), AV::ZEGO_NT_LINE);
	}

	AV::EnableMic((bool)m_bCKEnableMic);

    EnumVideoAndAudioDevice();

    m_captureVolumeProg.SetRange(0, 100);
    m_captureVolumeProg.SetPos(0);

	return TRUE;
}

void CZegoRoomDlg::EnumVideoAndAudioDevice()
{
    int nCBCurSel(0);

    int nDeviceCount = 0;
    DeviceInfo* pDeviceList(NULL);

    nCBCurSel = 0;
    pDeviceList = ZEGO::AV::GetAudioDeviceList(AudioDevice_Input, nDeviceCount);
    for (int i = 0; i < nDeviceCount; ++i)
    {
        m_cbCurrentAudio.AddString(UTF8ToWString(pDeviceList[i].szDeviceName).c_str());
        m_vecAudioDeviceIDs.push_back(pDeviceList[i].szDeviceId);
        if (m_pAVSettings->GetMircophoneId() == std::string(pDeviceList[i].szDeviceId))
            nCBCurSel = i;
    }
    m_cbCurrentAudio.SetCurSel(nCBCurSel);
    ZEGO::AV::FreeDeviceList(pDeviceList);
    pDeviceList = NULL;

    nCBCurSel = 0;
    pDeviceList = ZEGO::AV::GetVideoDeviceList(nDeviceCount);
    for (int i = 0; i < nDeviceCount; ++i)
    {
        m_cbCurrentVideo.AddString(UTF8ToWString(pDeviceList[i].szDeviceName).c_str());
        m_vecVideoDeviceIDs.push_back(pDeviceList[i].szDeviceId);
        if (m_pAVSettings->GetCameraId() == std::string(pDeviceList[i].szDeviceId))
            nCBCurSel = i;
    }
    m_cbCurrentVideo.SetCurSel(nCBCurSel);
    ZEGO::AV::FreeDeviceList(pDeviceList);
    pDeviceList = NULL;
}

void CZegoRoomDlg::SetRoom(RoomPtr roomModel)
{
	m_pChatRoom = roomModel;
}

BEGIN_MESSAGE_MAP(CZegoRoomDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_SYSCOMMAND()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
    ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CZegoRoomDlg::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_BACK, &CZegoRoomDlg::OnBnClickedButtonBack)
	ON_COMMAND(IDOK, &CZegoRoomDlg::OnIdok)
	ON_BN_CLICKED(IDC_CHECK_MICROPHONE, &CZegoRoomDlg::OnBnClickedCheckMicrophone)
	ON_BN_CLICKED(IDC_BUTTON_AUX, &CZegoRoomDlg::OnBnClickedButtonAux)
    ON_CBN_SELCHANGE(IDC_COMBO_CURVIDEO, &CZegoRoomDlg::OnCbnSelchangeComboCurvideo)
    ON_CBN_SELCHANGE(IDC_COMBO_CURAUDIO, &CZegoRoomDlg::OnCbnSelchangeComboCuraudio)
    ON_BN_CLICKED(IDC_BUTTON_CAPTURE, &CZegoRoomDlg::OnBnClickedButtonCapture)
END_MESSAGE_MAP()


// CZegoRoomDlg ��Ϣ�������
void CZegoRoomDlg::OnGetMinMaxInfo(MINMAXINFO *lpMMI)
{
	CPoint pt(1024, 720); 
	lpMMI->ptMinTrackSize = pt;
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CZegoRoomDlg::OnClose()
{
	EndAux();
	__super::OnClose();
}

void CZegoRoomDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (::IsWindow(m_listVisitors.GetSafeHwnd()))
	{
		CRect rect;
		m_listVisitors.GetClientRect(&rect);
		m_listVisitors.SetColumnWidth(2, rect.Width() - 80);
	}
}

void CZegoRoomDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CZegoRoomDlg::OnTimer(UINT_PTR id)
{
    CDialogEx::OnTimer(id);

    if (id == VOLUME_TIMER_ID)
    {
        float fCaptureLevel = AV::GetCaptureSoundLevel();
        m_captureVolumeProg.SetPos(fCaptureLevel);
    }
}

void CZegoRoomDlg::OnBnClickedButtonBack()
{
	EndDialog(100001);
}

void CZegoRoomDlg::OnBnClickedCheckMicrophone()
{
	UpdateData();
	AV::EnableMic((bool)m_bCKEnableMic);
}

void CZegoRoomDlg::OnBnClickedButtonAux()
{
	LONG lAuxStatus = GetWindowLong(m_btnAux.GetSafeHwnd(), GWL_USERDATA);
	lAuxStatus == 0 ? BeginAux() : EndAux();
}

void CZegoRoomDlg::BeginAux(void)
{
	if (GetWindowLong(m_btnAux.GetSafeHwnd(), GWL_USERDATA) == 1)
	{
		return;
	}

	char szAppPath[MAX_PATH] = { 0 };
	::GetModuleFileNameA(NULL, szAppPath, MAX_PATH);
	CStringA strAppPath = szAppPath;
	CStringA strAppDir = strAppPath.Left(strAppPath.ReverseFind('\\') + 1);

	OPENFILENAMEA ofn = { 0 };
	char strAuxFilePath[MAX_PATH] = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "pcm�ļ�\0*.pcm;\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = strAuxFilePath;
	ofn.nMaxFile = sizeof(strAuxFilePath);
	ofn.lpstrInitialDir = strAppDir;
	ofn.lpstrTitle = "��ѡ��һ�������ļ�";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	if (GetOpenFileNameA(&ofn))
	{
		FILE* fAux;
		fopen_s(&fAux, strAuxFilePath, "rb");
		if (fAux == NULL) { return; }

		fseek(fAux, 0, SEEK_END);

		m_nAuxDataLen = ftell(fAux);
		if (m_nAuxDataLen > 0)
		{
			m_pAuxData = new unsigned char[m_nAuxDataLen];
			memset(m_pAuxData, 0, m_nAuxDataLen);
		}

		fseek(fAux, 0, 0);

		int nReadDataLen = fread(m_pAuxData, sizeof(unsigned char), m_nAuxDataLen, fAux);

		fclose(fAux);

		AV::EnableAux(TRUE);

		m_btnAux.SetWindowText(_T("�رջ���"));
		SetWindowLong(m_btnAux.GetSafeHwnd(), GWL_USERDATA, 1);
	}
}

void CZegoRoomDlg::EndAux(void)
{
	if (GetWindowLong(m_btnAux.GetSafeHwnd(), GWL_USERDATA) == 0)
	{
		return;
	}

	AV::EnableAux(FALSE);

	m_btnAux.SetWindowText(_T("��������"));
	SetWindowLong(m_btnAux.GetSafeHwnd(), GWL_USERDATA, 0);

	delete[] m_pAuxData;
	m_nAuxDataLen = 0;
	m_nAuxDataPos = 0;
}

void CZegoRoomDlg::WakeupPendingStream(void)
{
	if (m_pChatRoom == nullptr || !m_bLoginChannel) { return; }

	while (m_pChatRoom->GetPlayingStreamCount(true) < 1 && 
		m_avaliablePrimaryView.size() > 0)
	{
		StreamPtr pPrimaryStream = m_pChatRoom->PopPendingStream(true);
		if (pPrimaryStream == nullptr)
		{
			break;
		}

		unsigned int uIndex = m_avaliablePrimaryView.top();
		m_avaliablePrimaryView.pop();
		pPrimaryStream->SetPlayView(uIndex);
		m_pChatRoom->AddStream(pPrimaryStream);

		PlayPendingStream(pPrimaryStream);
	}
	
	while (m_pChatRoom->GetPlayingStreamCount(false) < 6 &&
		m_avaliableStudentView.size() > 0)
	{
		StreamPtr pStudentStream = m_pChatRoom->PopPendingStream(false);
		if (pStudentStream == nullptr)
		{
			break;
		}

		unsigned int uIndex = m_avaliableStudentView.top();
		m_avaliableStudentView.pop();
		pStudentStream->SetPlayView(uIndex);
		m_pChatRoom->AddStream(pStudentStream);

		PlayPendingStream(pStudentStream);
	}
}

void CZegoRoomDlg::PlayPendingStream(StreamPtr stream)
{
	if (stream == nullptr) { return; }

	int nIndex = stream->GetPlayView();
	if (nIndex<0 || nIndex > 6) { return; }

	CZegoAVView* pAVView = (CZegoAVView*)GetDlgItem(g_AVViews[nIndex]);
	if (pAVView == nullptr || !IsWindow(pAVView->GetDisplaySafeWnd()))
	{
		return;
	}

	if (stream->IsCurUserCreated())
	{
		if (m_pAVSettings == nullptr || m_pAVSettings->GetCameraId().empty())
		{
			MessageBox(L"��ⲻ������ͷ", L"��ʾ");
			return;
		}

		AV::SetPreviewView(pAVView->GetDisplaySafeWnd());
		AV::SetPreviewViewMode(AV::ZegoVideoViewModeScaleToFill);
		AV::StartPreview();
		AV::StartPublish( WStringToUTF8(m_pChatRoom->GetRoomTitle().c_str()).c_str(), stream->GetId().c_str());
	}
	else
	{
		AV::SetView(pAVView->GetDisplaySafeWnd(), nIndex);
		AV::SetViewMode(AV::ZegoVideoViewModeScaleToFill, nIndex);
		AV::PlayStream(stream->GetId().c_str(), nIndex);
	}
}

void CZegoRoomDlg::ShutdownPlayingStream(const std::string& streamId)
{
	if (m_pChatRoom == nullptr)
	{
		return;
	}

	StreamPtr pStream = m_pChatRoom->RemoveStream(streamId.c_str());
	if (pStream == nullptr || !pStream->IsPlaying())
	{
		return;
	}

	if (pStream->IsCurUserCreated())
	{
		AV::StopPublish();
		AV::StopPreview();
		AV::SetPreviewView(NULL);
	}
	else
	{
		AV::StopPlayStream(streamId.c_str());
	}

	FreeAVView(pStream);
}

void CZegoRoomDlg::FreeAVView(StreamPtr stream)
{
	if (stream == nullptr)
	{
		return;
	}

	int nIndex = stream->GetPlayView();
	if (stream->IsPrimary())
	{
		m_avaliablePrimaryView.push(nIndex);
	}
	else
	{
		m_avaliableStudentView.push(nIndex);
	}

	CZegoAVView* pAVView = (CZegoAVView*)GetDlgItem(g_AVViews[nIndex]);
	if (pAVView != nullptr)
	{
		pAVView->ShowQuailtyTips(-1);
	}
}

void CZegoRoomDlg::OnChatRoomUserList(std::vector<UserPtr> userAddList, std::vector<UserPtr> userDelList, CHATROOM::ZegoUserUpdateFlag flag, bool isPublic)
{
	if (m_pChatRoom == nullptr) { return; }

	bool bUserUpdated(false);
	// �����û��б�,�ֱ����������뿪���û�
	for (auto& elem : userAddList)
	{
		if (m_pChatRoom->AddVisitor(elem))
		{
			bUserUpdated = true;
		}
	}

	for (auto& elem : userDelList)
	{
		if (m_pChatRoom->RemoveVisitor(elem->GetUserId()))
		{
			bUserUpdated = true;
		}
	}

	if ( bUserUpdated )
	{
		RefreshVisitorList();
	}
}

void CZegoRoomDlg::RefreshVisitorList(void)
{
	if (m_pChatRoom == nullptr) { return; }

	m_listVisitors.DeleteAllItems();

	UserPtr pCurUser = m_pChatRoom->GetCurUser();
	m_listVisitors.InsertItem(0, _T("1"));
	m_listVisitors.SetItemText(0, 1, pCurUser->IsPrimary() ? _T("����") : _T("����"));
	m_listVisitors.SetItemText(0, 2, pCurUser->GetUserName().c_str());
	m_listVisitors.SetItemData(1, 1);

	for (size_t index = 0; index < m_pChatRoom->GetVisitorCount(); index++)
	{
		UserPtr pUser = m_pChatRoom->GetVisitor(index);
		if (pUser == nullptr) { continue; }

		CString strIndex;
		strIndex.Format(_T("%u"), index + 2);
		m_listVisitors.InsertItem(index + 1, strIndex);
		m_listVisitors.SetItemText(index + 1, 1, pUser->IsPrimary() ? _T("����") : _T("����"));
		m_listVisitors.SetItemText(index + 1, 2, pUser->GetUserName().c_str());

		m_listVisitors.SetItemData(index + 1, index + 1);
	}
}

void CZegoRoomDlg::OnChatRoomStreamCreate(std::string streamId, std::string url, bool isPublic)
{
	if (streamId.empty()) { return; }

	if (m_pChatRoom == nullptr) { return; }

	UserPtr pCurUser = m_pChatRoom->GetCurUser();
	if (pCurUser == nullptr) { return; }

	std::string strUserName = WStringToUTF8(pCurUser->GetUserName().c_str());
	std::string strRoomTitle = WStringToUTF8(m_pChatRoom->GetRoomTitle().c_str());
	StreamPtr pCurUserStrame(new CZegoStreamModel(strRoomTitle, streamId, strUserName, true));
	m_pChatRoom->AddStream(pCurUserStrame);

	WakeupPendingStream();
}

void CZegoRoomDlg::OnChatRoomStreamList(std::vector<StreamPtr> streamList, CHATROOM::ZegoUpdateFlag flag, bool isPublic)
{
	for (auto& stream : streamList)
	{
		if (flag == CHATROOM::UPDATE_ADDED && m_pChatRoom)
		{
			m_pChatRoom->AddStream(stream);
		}
		else if (flag == CHATROOM::UPDATE_DELETED && m_pChatRoom)
		{
			ShutdownPlayingStream(stream->GetId());
		}
	}

	WakeupPendingStream();
}

void CZegoRoomDlg::OnChatRoomDisconnected(int nErrorCode, unsigned int dwRoomKey, unsigned int dwServerKey, bool isPublic)
{
	if (m_pChatRoom != nullptr &&
		m_pChatRoom->GetRoomKey() != dwRoomKey &&	m_pChatRoom->GetServerKey() != dwServerKey)
	{
		MessageBox(L"ʧȥ����", L"��ʾ");

		EndDialog(100001);
	}
}

void CZegoRoomDlg::OnChatRoomKickout(int nErrorCode, std::string msg, bool isPublic)
{
	if (nErrorCode != 0)
	{
		MessageBox(UTF8ToWString(msg.c_str()).c_str(), L"��ʾ");

		EndDialog(100001);
	}
}

void CZegoRoomDlg::OnAVLoginChannel(std::string userId, std::string channelId, unsigned int uErrorCode)
{
	if (uErrorCode == 0 && channelId == m_pChatRoom->GetChannel())
	{
		m_bLoginChannel = true;
		std::string strStreamId = channelId + "_" + userId;
		CHATROOM::CreateStreamInRoom(WStringToUTF8(m_pChatRoom->GetRoomTitle().c_str()).c_str(), strStreamId.c_str(), true);
	}
}

void CZegoRoomDlg::OnAVLogoutChannel(std::string userId, std::string channelId, unsigned int uErrorCode)
{
	m_bLoginChannel = false;
}

void CZegoRoomDlg::OnAVPublishState(std::string userId, std::string channelId, AV::ZegoAVAPIState estate, std::string streamId, StreamPtr stream)
{
	if (m_pChatRoom == nullptr || m_pChatRoom->GetChannel() != channelId) { return; }

	if ( estate == AV::AVStateBegin )
	{
		CHATROOM::ReportStreamAction(1, streamId.c_str(), userId.c_str(), true);

		m_btnAux.EnableWindow(TRUE);
        m_btnCapture.EnableWindow(TRUE);
        SetTimer(VOLUME_TIMER_ID, 100, NULL);

		if (stream != nullptr)
		{
			CString strUrl;

			CString strRtmpUrl = (stream->m_vecRtmpUrls.size() > 0) ? 
				UTF8ToWString(stream->m_vecRtmpUrls[0].c_str()).c_str() : _T("");
			if (!strRtmpUrl.IsEmpty())
			{
				strUrl.Append(_T("1. "));
				strUrl.Append(strRtmpUrl);
				strUrl.Append(_T("\r\n"));
			}

			CString strFlvUrl = (stream->m_vecFlvUrls.size() > 0) ? 
				UTF8ToWString(stream->m_vecFlvUrls[0].c_str()).c_str() : _T("");
			if (!strFlvUrl.IsEmpty())
			{
				strUrl.Append(_T("2. "));
				strUrl.Append(strFlvUrl);
				strUrl.Append(_T("\r\n"));
			}

			CString strHlsUrl = (stream->m_vecHlsUrls.size() > 0) ? 
				UTF8ToWString(stream->m_vecHlsUrls[0].c_str()).c_str() : _T("");
			if (!strHlsUrl.IsEmpty())
			{
				strUrl.Append(_T("3. "));
				strUrl.Append(strHlsUrl);
				strUrl.Append(_T("\r\n"));
			}

			m_edStreamUrl.SetWindowText(strUrl);
		}
	}
	else if (estate == AV::AVStateEnd || estate == AV::TempBroken || estate == AV::FatalError)
	{
		CHATROOM::ReportStreamAction(2, streamId.c_str(), userId.c_str(), true);

		EndAux();
		m_btnAux.EnableWindow(FALSE);
        m_btnCapture.EnableWindow(FALSE);
        KillTimer(VOLUME_TIMER_ID);

		ShutdownPlayingStream(streamId);

		WakeupPendingStream();

		m_edStreamUrl.SetWindowText(_T(""));
	}
}

void CZegoRoomDlg::OnAVPlayState(std::string userId, std::string channelId, AV::ZegoAVAPIState estate, std::string streamId)
{
	if (m_pChatRoom == nullptr || m_pChatRoom->GetChannel() != channelId) { return; }
	
	if (estate == AV::AVStateEnd || estate == AV::TempBroken || estate == AV::FatalError)
	{
		ShutdownPlayingStream(streamId);

		WakeupPendingStream();
	}
}

void CZegoRoomDlg::OnAVPublishQuality(std::string streamId, int quality)
{
	StreamPtr pStream = m_pChatRoom->GetPlayingStreamById(true, streamId);
	if (pStream == nullptr) 
	{ 
		return; 
	}

	int nIndex = pStream->GetPlayView();
	if (nIndex < 0 || nIndex > 1)
	{
		return;
	}

	CZegoAVView* pAVView = (CZegoAVView*)GetDlgItem(g_AVViews[nIndex]);
	if (pAVView != nullptr)
	{
		pAVView->ShowQuailtyTips(quality);
	}
}

void CZegoRoomDlg::OnAVPlayQuality(std::string streamId, int quality)
{
	StreamPtr pStream = m_pChatRoom->GetPlayingStreamById(false, streamId);
	if (pStream == nullptr) 
	{ 
		return; 
	}

	int nIndex = pStream->GetPlayView();
	if (nIndex < 0 || nIndex > 6)
	{
		return;
	}

	CZegoAVView* pAVView = (CZegoAVView*)GetDlgItem(g_AVViews[nIndex]);
	if (pAVView != nullptr)
	{
		pAVView->ShowQuailtyTips(quality);
	}

    //float fRemoteLevel = AV::GetRemoteSoundLevel(nIndex);
    //TCHAR buffer[MAX_PATH] = { 0 };
    //_stprintf(buffer, _T("current remote index %d level %f"), nIndex, fRemoteLevel);
    //OutputDebugString(buffer);
}

void CZegoRoomDlg::OnAVAuxInput(unsigned char *pData, int *pDataLen, int *pSampleRate, int *pNumChannels)
{
	if (m_pAuxData != nullptr && (*pDataLen < m_nAuxDataLen))
	{
		*pSampleRate = 44100;
		*pNumChannels = 2;

		if (m_nAuxDataPos + *pDataLen > m_nAuxDataLen)
		{
			m_nAuxDataPos = 0;
		}

		int nCopyLen = *pDataLen;
		memcpy(pData, m_pAuxData + m_nAuxDataPos, nCopyLen);

		m_nAuxDataPos += nCopyLen;

		*pDataLen = nCopyLen;
	}
	else
	{
		*pDataLen = 0;
	}
}

void CZegoRoomDlg::OnAVDisconnected(std::string userId, std::string channelId, unsigned int errCode)
{
	if (m_pChatRoom == nullptr || m_pChatRoom->GetChannel() != channelId) { return; }

	if (errCode != 0)
	{
		MessageBox(L"ֱ��ʧȥ����", L"��ʾ", MB_OK);
	}
}

void CZegoRoomDlg::OnAVReconnected(std::string userId, std::string channelId)
{

}

void CZegoRoomDlg::OnAudioDeviceChanged(AudioDeviceType deviceType, std::string strDeviceId, std::string strDeviceName, DeviceState state)
{
    if (deviceType == AudioDevice_Output)
        return;

    if (state == Device_Added)
    {
        m_cbCurrentAudio.AddString(UTF8ToWString(strDeviceName.c_str()).c_str());
        m_vecAudioDeviceIDs.push_back(strDeviceId);
    }
    else if (state == Device_Deleted)
    {
        for (int i = 0; i < m_vecAudioDeviceIDs.size(); i++)
        {
            if (m_vecAudioDeviceIDs[i] != strDeviceId)
                continue;

            m_vecAudioDeviceIDs.erase(m_vecAudioDeviceIDs.begin() + i);

            int currentCurl = m_cbCurrentAudio.GetCurSel();
            if (currentCurl == i)
            {
                //MessageBox(L"��ǰ��Ƶ�ɼ��豸���Ƴ�", L"��ʾ", MB_OK);

                //Ĭ�ϲɼ���һ����Ƶ�豸
                if (m_vecAudioDeviceIDs.size() > 0)
                {
                    AV::SetAudioDevice(AV::AudioDevice_Input, m_vecAudioDeviceIDs[0].c_str());
                    m_pAVSettings->SetMicrophoneId(m_vecAudioDeviceIDs[0]);
                }
            }

            m_cbCurrentAudio.DeleteString(i);
            m_cbCurrentAudio.SetCurSel(0);
            break;
        }
    }
}

void CZegoRoomDlg::OnVideoDeviceChanged(std::string strDeviceId, std::string strDeviceName, DeviceState state)
{
    if (state == Device_Added)
    {
        m_cbCurrentVideo.AddString(UTF8ToWString(strDeviceName.c_str()).c_str());
        m_vecVideoDeviceIDs.push_back(strDeviceId);
    }
    else if (state == Device_Deleted)
    {
        for (int i = 0; i < m_vecVideoDeviceIDs.size(); i++)
        {
            if (m_vecVideoDeviceIDs[i] != strDeviceId)
                continue;

            m_vecVideoDeviceIDs.erase(m_vecVideoDeviceIDs.begin() + i);

            int currentCurl = m_cbCurrentVideo.GetCurSel();
            if (currentCurl == i)
            {
                //MessageBox(L"��ǰ��Ƶ�豸���Ƴ�", L"��ʾ", MB_OK);
                //Ĭ�ϲɼ���һ����Ƶ�豸
                if (m_vecVideoDeviceIDs.size() > 0)
                {
                    AV::SetVideoDevice(m_vecVideoDeviceIDs[0].c_str());
                    m_pAVSettings->SetCameraId(m_vecVideoDeviceIDs[0]);
                }
            }

            m_cbCurrentVideo.DeleteString(i);
            m_cbCurrentVideo.SetCurSel(0);
            break;
        }
    }
}
//////////////////////////////////////////////////////////////////////////
// ��Ϣ��ش���
void CZegoRoomDlg::OnIdok()
{
	if (GetFocus()->GetSafeHwnd() == m_edMessage.GetSafeHwnd())
	{
		OnBnClickedButtonSend();
	}
}

void CZegoRoomDlg::OnBnClickedButtonSend()
{
	CString strChat;
	m_edMessage.GetWindowText(strChat);
	if (strChat.IsEmpty())
	{
		return;
	}

	UserPtr pCurUser = m_pChatRoom->GetCurUser();
	if (pCurUser == nullptr)
	{
		return;
	}

	std::string strUserName = WStringToUTF8(pCurUser->GetUserName().c_str());
	m_strLastSendMsg = strUserName + "&amp;" + WStringToUTF8(strChat);
	CHATROOM::SendBroadcastTextMsg(m_strLastSendMsg.c_str(), true);

	m_edMessage.SetWindowText(L"");
}

void CZegoRoomDlg::OnChatRoomSendMessage(int nErrorCode, std::string msg, bool isPublic)
{
	if (nErrorCode != 0)
	{
		return;
	}

	if (m_strLastSendMsg.empty())
	{
		return;
	}

	size_t offset = m_strLastSendMsg.find("&amp;");
	if (offset == std::string::npos)
	{
		return;
	}

	std::string strSendContent = m_strLastSendMsg.substr(offset + 5);

	CString strTmpContent;
	strTmpContent.Format(_T("��: %s"), UTF8ToWString(strSendContent.c_str()).c_str());
	m_lbChatContent.InsertString(-1, strTmpContent);

	m_strLastSendMsg.clear();
}

void CZegoRoomDlg::OnChatRoomRecvMessage(std::string msg, bool isPublic)
{
	if (msg.empty())
	{
		return;
	}

	size_t offset = msg.find("&amp;");
	if (offset != std::string::npos)
	{
		HandleTextMessage(msg, offset);
		return;
	}

	HandlePublishRequest(msg);
}

void CZegoRoomDlg::HandleTextMessage(const std::string& msg, size_t split)
{
	std::string strSenderName = msg.substr(0, split);
	std::string strContent = msg.substr(split + 5);

	CString strTmpContent;
	strTmpContent.Format(_T("%s: %s"), UTF8ToWString(strSenderName.c_str()).c_str(), UTF8ToWString(strContent.c_str()).c_str());
	m_lbChatContent.InsertString(-1, strTmpContent);
}

void CZegoRoomDlg::HandlePublishRequest(const std::string& msg)
{
	rapidjson::Document doc;
	doc.Parse<rapidjson::kParseStopWhenDoneFlag>(msg.c_str());
	if (doc.HasParseError())
	{
		rapidjson::ParseErrorCode errorCode = doc.GetParseError();
		return;
	}

	if (!doc.HasMember("command"))
	{
		return;
	}

	std::string strCommand;
	rapidjson::Value& val = doc["command"];
	if (val.IsString())
	{
		strCommand = val.GetString();
	}

	if (strCommand != "requestPublish")
	{
		return;
	}

	MessageBeep(MB_OK);

	rapidjson::Document rspdoc;
	rspdoc.SetObject();
	rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

	UserPtr pCurUser = m_pChatRoom->GetCurUser();
	if (pCurUser == nullptr || !pCurUser->IsPrimary())
	{
		return;
	}

	std::string strUserId = WStringToUTF8(pCurUser->GetUserId().c_str());
	std::string strUserName = WStringToUTF8(pCurUser->GetUserName().c_str());

	rspdoc.AddMember("magic", doc["magic"], allocator);
	rspdoc.AddMember("command", "requestPublishRespond", allocator);
	rspdoc.AddMember("fromUserId", rapidjson::StringRef(strUserId.c_str()), allocator);
	rspdoc.AddMember("fromUserName", rapidjson::StringRef(strUserName.c_str()), allocator);
	rspdoc.AddMember("content", "YES", allocator);

	rapidjson::Value arrDestUsers(rapidjson::kArrayType);
	rapidjson::Value destUser(rapidjson::kObjectType);
	destUser.AddMember("toUserId", doc["fromUserId"], allocator);
	destUser.AddMember("toUserName", doc["fromUserName"], allocator);
	arrDestUsers.PushBack(destUser, allocator);
	rspdoc.AddMember("toUser", arrDestUsers, allocator);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	rspdoc.Accept(writer);

	std::string strBuffer = buffer.GetString();
	CHATROOM::SendRelayBroadcastCustomMsg(strBuffer.c_str(), strBuffer.size(), true);
}

void CZegoRoomDlg::OnCbnSelchangeComboCurvideo()
{
    // TODO: Add your control notification handler code here
    int currentCurl = m_cbCurrentVideo.GetCurSel();
    if (currentCurl < m_vecVideoDeviceIDs.size())
    {
        AV::SetVideoDevice(m_vecVideoDeviceIDs[currentCurl].c_str());
        m_pAVSettings->SetCameraId(m_vecVideoDeviceIDs[currentCurl]);
    }
}


void CZegoRoomDlg::OnCbnSelchangeComboCuraudio()
{
    // TODO: Add your control notification handler code here
    int currentCurl = m_cbCurrentAudio.GetCurSel();
    if (currentCurl < m_vecAudioDeviceIDs.size())
    {
        AV::SetAudioDevice(AV::AudioDevice_Input, m_vecAudioDeviceIDs[currentCurl].c_str());
        m_pAVSettings->SetMicrophoneId(m_vecAudioDeviceIDs[currentCurl]);
    }
}


void CZegoRoomDlg::OnBnClickedButtonCapture()
{
    // TODO: Add your control notification handler code here
    if (m_bSystemCapture)
    {
        m_bSystemCapture = false;
        m_btnCapture.SetWindowText(_T("�����ɼ�"));
    }
    else
    {
        m_bSystemCapture = true;
        m_btnCapture.SetWindowText(_T("�رղɼ�"));
    }

    AV::EnableMixSystemPlayout(m_bSystemCapture);
}
