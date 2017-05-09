
// SPITestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SPITest.h"
#include "SPITestDlg.h"
#include "afxdialogex.h"
#include "stdio.h"

#include "../SPITest/include/libMPSSE_spi.h"
#include "../SPITest/include/windows/ftd2xx.h"
#include "windows.h"
#include <iostream>
#include <fstream>
#include <ostream>
#include <tchar.h>
#include <sstream>  

using namespace std;
#ifdef _WIN32
#define GET_FUN_POINTER	GetProcAddress
#endif

#define SPI_DEVICE_BUFFER_SIZE		256
#define SPI_WRITE_COMPLETION_RETRY	10
#define CHANNEL_TO_OPEN			0	/*0 for first available channel, 1 for next... */
#define SPI_SLAVE_0				0
#define SPI_SLAVE_1				1
#define SPI_SLAVE_2				2

/* Options-Bit0: If this bit is 1 then it means that the transfer size provided is in bytes */
#define	SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES			0x00000001
/* Options-Bit0: If this bit is 1 then it means that the transfer size provided is in bytes */
#define	SPI_TRANSFER_OPTIONS_SIZE_IN_BITS			0x00000001
/* Options-Bit1: if BIT1 is 1 then CHIP_SELECT line will be enables at start of transfer */
#define	SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE		0x00000002
/* Options-Bit2: if BIT2 is 1 then CHIP_SELECT line will be disabled at end of transfer */
#define SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE		0x00000004

typedef FT_STATUS(*pfunc_SPI_GetNumChannels)(uint32 *numChannels);
pfunc_SPI_GetNumChannels p_SPI_GetNumChannels;
typedef FT_STATUS(*pfunc_SPI_GetChannelInfo)(uint32 index, FT_DEVICE_LIST_INFO_NODE *chanInfo);
pfunc_SPI_GetChannelInfo p_SPI_GetChannelInfo;
typedef FT_STATUS(*pfunc_SPI_OpenChannel)(uint32 index, FT_HANDLE *handle);
pfunc_SPI_OpenChannel p_SPI_OpenChannel;
typedef FT_STATUS(*pfunc_SPI_InitChannel)(FT_HANDLE handle, ChannelConfig *config);
pfunc_SPI_InitChannel p_SPI_InitChannel;
typedef FT_STATUS(*pfunc_SPI_CloseChannel)(FT_HANDLE handle);
pfunc_SPI_CloseChannel p_SPI_CloseChannel;
typedef FT_STATUS(*pfunc_SPI_Read)(FT_HANDLE handle, uint8 *buffer, uint32 sizeToTransfer, uint32 *sizeTransfered, uint32 options);
pfunc_SPI_Read p_SPI_Read;
typedef FT_STATUS(*pfunc_SPI_Write)(FT_HANDLE handle, uint8 *buffer, uint32 sizeToTransfer, uint32 *sizeTransfered, uint32 options);
pfunc_SPI_Write p_SPI_Write;
typedef FT_STATUS(*pfunc_SPI_IsBusy)(FT_HANDLE handle, bool *state);
pfunc_SPI_IsBusy p_SPI_IsBusy;
typedef FT_STATUS (*pfunc_SPI_ReadWrite)(FT_HANDLE handle, uint8 *inBuffer, uint8 *outBuffer, uint32 sizeToTransfer, uint32 *sizeTransferred, uint32 transferOptions);
pfunc_SPI_ReadWrite p_SPI_ReadWrite;
uint32 channels;
uint8  m_txt = 0x00;
FT_HANDLE ftHandle;
ChannelConfig channelConf;
bool spiOpened;
uint8 buffer[SPI_DEVICE_BUFFER_SIZE];


ofstream logfile;


FT_STATUS writeByte(uint8 * buffer,int length)

{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered = 0;
	bool writeComplete = 0;
	uint32 retry = 0;
	FT_STATUS status;

	sizeToTransfer = 8 *length ;
	sizeTransfered = 0;
	if (spiOpened)
	{

		status = p_SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
			SPI_TRANSFER_OPTIONS_SIZE_IN_BITS |
			SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE |
			SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
		return status;
	}

	return false;

}

FT_STATUS readByte(uint8 * pAddr, uint8 * inBuf, int length)
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered = 0;
	bool readComplete = 0;
	uint32 retry = 0;
	bool state;
	FT_STATUS status;



	uint8* outBuf = new uint8(length);


	sizeToTransfer = 8 * length;
	sizeTransfered = 0;
	if (spiOpened)
	{

		p_SPI_Write(ftHandle, pAddr, 8, &sizeTransfered,
			SPI_TRANSFER_OPTIONS_SIZE_IN_BITS |
			SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);

		status = p_SPI_ReadWrite(ftHandle, inBuf, outBuf, sizeToTransfer, &sizeTransfered,
			SPI_TRANSFER_OPTIONS_SIZE_IN_BITS |
			SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
		return status;
	}

	return false;
}


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSPITestDlg 对话框



CSPITestDlg::CSPITestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSPITestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSPITestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MODE_SEL, modeSel);
	//  DDX_Control(pDX, IDC_READCNT, sendCnt);
	DDX_Control(pDX, IDC_RECVCONTENT, recvContent);
	DDX_Control(pDX, IDC_SENDCONTENT, sendContent);
	DDX_Control(pDX, IDC_READCNT, rdCount);
}

BEGIN_MESSAGE_MAP(CSPITestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_MODE_SEL, &CSPITestDlg::OnCbnSelchangeModeSel)
	ON_WM_CHAR()
	ON_BN_CLICKED(IDC_SEND, &CSPITestDlg::OnBnClickedSend)
	ON_BN_CLICKED(IDC_CLEARSEND, &CSPITestDlg::OnBnClickedClearsend)
	ON_BN_CLICKED(IDC_CLEARRECV, &CSPITestDlg::OnBnClickedClearrecv)
	ON_BN_CLICKED(IDC_RECEIVE, &CSPITestDlg::OnBnClickedReceive)
	ON_CBN_SELCHANGE(IDC_READCNT, &CSPITestDlg::OnCbnSelchangeReadcnt)
	ON_EN_CHANGE(IDC_SENDCONTENT, &CSPITestDlg::OnEnChangeSendcontent)
END_MESSAGE_MAP()


// CSPITestDlg 消息处理程序

BOOL CSPITestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码


	logfile.open("./debug.txt");
	if (!logfile.is_open())
		cerr << "Failed to openlog file " << endl;

	modeSel.SetCurSel(0);
	rdCount.SetCurSel(0);
	//try open the SPI

#ifdef _WIN32
#ifdef _MSC_VER
	HMODULE h_libMPSSE;
#else
	HANDLE h_libMPSSE;
#endif
#endif

	FT_STATUS status;
	//FT_DEVICE_LIST_INFO_NODE devList;
	uint8 address = 0;
	channelConf.ClockRate = 50000;
	channelConf.LatencyTimer = 255;
	channelConf.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_DBUS3 | SPI_CONFIG_OPTION_CS_ACTIVELOW;
	channelConf.Pin = 0x00000000;
	// Load libMPSSE
#ifdef _WIN32
#ifdef _MSC_VER
	h_libMPSSE = LoadLibrary(L"libMPSSE.dll");

#endif
#endif

	// init function pointers
	p_SPI_GetNumChannels = (pfunc_SPI_GetNumChannels)GET_FUN_POINTER(h_libMPSSE, "SPI_GetNumChannels");
	p_SPI_GetChannelInfo = (pfunc_SPI_GetChannelInfo)GET_FUN_POINTER(h_libMPSSE, "SPI_GetChannelInfo");
	p_SPI_OpenChannel = (pfunc_SPI_OpenChannel)GET_FUN_POINTER(h_libMPSSE, "SPI_OpenChannel");
	p_SPI_InitChannel = (pfunc_SPI_InitChannel)GET_FUN_POINTER(h_libMPSSE, "SPI_InitChannel");
	p_SPI_Read = (pfunc_SPI_Read)GET_FUN_POINTER(h_libMPSSE, "SPI_Read");
	p_SPI_Write = (pfunc_SPI_Write)GET_FUN_POINTER(h_libMPSSE, "SPI_Write");
	p_SPI_ReadWrite = (pfunc_SPI_ReadWrite)GET_FUN_POINTER(h_libMPSSE, "SPI_ReadWrite");
	p_SPI_CloseChannel = (pfunc_SPI_CloseChannel)GET_FUN_POINTER(h_libMPSSE, "SPI_CloseChannel");


	status = p_SPI_GetNumChannels(&channels);
	//	printf("Number of available SPI channels = %d\n", channels);


	status = p_SPI_OpenChannel(CHANNEL_TO_OPEN, &ftHandle); // Open the first available channel

	status = p_SPI_InitChannel(ftHandle, &channelConf);

	if (status != 0)
	{
		logfile << "SPI Open error" << endl << endl;
		spiOpened = false;
	}
	else
	{

		logfile << "OpenSPI" << endl << endl;
		spiOpened = true;
	}


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSPITestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSPITestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CSPITestDlg::OnCbnSelchangeModeSel()
{
	// TODO:  在此添加控件通知处理程序代码
	if (modeSel.GetCurSel() != 1)
	{
		if (ftHandle != NULL)
		p_SPI_CloseChannel(ftHandle); //close the SPI
	}
}




BOOL CSPITestDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO:  在此添加专用代码和/或调用基类
	if (WM_CHAR == pMsg->message && ((GetFocus() == GetDlgItem(IDC_SENDCONTENT)) ))
	{
		if (((pMsg->wParam >= 0x30) && (pMsg->wParam <= 0x39)) || ((pMsg->wParam >= 'a') && (pMsg->wParam <= 'f') || (pMsg->wParam >= 'a')) 
			|| ((pMsg->wParam >= 'A') && (pMsg->wParam <= 'F')) || (pMsg->wParam <= 'BS'))
		{
			return CDialogEx::PreTranslateMessage(pMsg);
		}
		else
		{
			MessageBeep(-1);
			pMsg->wParam = NULL;
			return false;
		}
	}
	else
	{
		return CDialogEx::PreTranslateMessage(pMsg);
	}


}


void CSPITestDlg::OnBnClickedSend()
{
	// TODO:  在此添加控件通知处理程序代码
	CString strText;
	sendContent.GetWindowText(strText);

	const size_t strsize = (strText.GetLength() + 1) * 2; // 宽字符的长度;
	char * pstr = new char[strsize]; //分配空间;
	size_t sz = 0;
	wcstombs_s(&sz, pstr, strsize, strText, _TRUNCATE);

//	strcpy(pstr, (LPCTSTR)strText);
	char * leftover;
	int num = strtoul(pstr, &leftover, 16);


	int length = strText.GetLength();
	uint8* buffer = new uint8(length);
	uint8* hexValue = new uint8(length/2);
	memcpy(buffer, (LPCSTR)strText.GetBuffer(), length);
	hexValue[0] = buffer[0] << 4 + buffer[1];
	
	writeByte((uint8 * )buffer, length);
}


void CSPITestDlg::OnBnClickedClearsend()
{
	// TODO:  在此添加控件通知处理程序代码
	sendContent.SetWindowTextW(_T(""));
}


void CSPITestDlg::OnBnClickedClearrecv()
{
	// TODO:  
	recvContent.SetWindowTextW(_T(""));
	
	recvContent.Clear();
}



CString toCString(string str) {
#ifdef _UNICODE  
	//如果是unicode工程  
	USES_CONVERSION; CString s(str.c_str());
	CString ans(str.c_str());
	return ans;
#else  
	//如果是多字节工程   
	//string 转 CString  
	CString ans;
	ans.Format("%s", str.c_str());
	return ans;
#endif // _UNICODE    
}

void CSPITestDlg::OnBnClickedReceive()
{
	// TODO:  在此添加控件通知处理程序代码
    CString rdCntStr;
	rdCount.GetLBText(rdCount.GetCurSel(), rdCntStr);
	int rdCnt = _ttoi(rdCntStr);

	rdCnt = 1;
	uint8* inBuf = new uint8(rdCnt+1);
	uint8 addr = 0xF0;
	uint32 sizeToTransfer = 8;
	uint32 sizeTransfered = 0;


	readByte(&addr, (uint8 *)inBuf, rdCnt);
	inBuf[rdCnt] = '\0';

	bool writeComplete = 0;
	uint32 retry = 0;
	bool state;

	char* strBuf = new char(rdCnt*2 + 1);

	sprintf(strBuf, "%X", *inBuf);

	CString cstr(strBuf);

	CString oldStr;
	recvContent.GetWindowTextW(oldStr);

	recvContent.SetWindowTextW(LPCTSTR(oldStr+cstr));


}


void CSPITestDlg::OnCbnSelchangeReadcnt()
{
	// TODO:  在此添加控件通知处理程序代码
}


void CSPITestDlg::OnEnChangeSendcontent()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
