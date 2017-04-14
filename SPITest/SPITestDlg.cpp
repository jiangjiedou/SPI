
// SPITestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SPITest.h"
#include "SPITestDlg.h"
#include "afxdialogex.h"

#include "libMPSSE_spi.h"
#include "ftd2xx.h"
#include "windows.h"
#include <iostream>
#include <fstream>
#include <ostream>
#include <tchar.h>

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

uint32 channels;
uint8  m_txt = 0x00;
FT_HANDLE ftHandle;
ChannelConfig channelConf;
uint8 buffer[SPI_DEVICE_BUFFER_SIZE];


ofstream logfile;
/*ofstream logfile;
logfile.open(".F:\.demo.txt");
if (!logfile.is_open())
cout<< "Failed to openlog file " << endl;

logfile << "ca1=[";
for (int k = 0; k < 1023; k++)
{

	if (k % 80 == 0)
		logfile << "..." << endl;

	logfile << ca[k] << " ";

}
logfile << "];" << endl << endl;
logfile.close();
*/


FT_STATUS write_byte(uint8 * buffer,int length)

{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered = 0;
	bool writeComplete = 0;
	uint32 retry = 0;
	FT_STATUS status;


	/* Write Data to 23S08's OLAT Register */
	sizeToTransfer = 8 *length ;  // 3 Bytes Opcodes + Data
	sizeTransfered = 0;
	//buffer[0] = 0x40;  //  Opcode to select device
	//buffer[1] = 0x0A;  //  Opcode for OLAT Register
	
	

	status = p_SPI_Write(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
		SPI_TRANSFER_OPTIONS_SIZE_IN_BITS |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);

	return status;
}

FT_STATUS read_byte(uint8 * buffer)
{
	uint32 sizeToTransfer = 0;
	uint32 sizeTransfered = 0;
	bool readComplete = 0;
	uint32 retry = 0;
	bool state;
	FT_STATUS status;

	
	sizeToTransfer = 8;  //3 Bytes Opcodes and Data
	sizeTransfered = 0;
	//buffer[0] = 0x00; // Opcode to select device 
	//buffer[1] = 0x00; // Opcode for IODIR register
	//buffer[2] = 0x00; // Data Packet - Make GPIO pins outputs
	status = p_SPI_Read(ftHandle, buffer, sizeToTransfer, &sizeTransfered,
		SPI_TRANSFER_OPTIONS_SIZE_IN_BITS |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE |
		SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);


	return status;
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
	DDX_Control(pDX, IDC_BUTTON1, m_time);
	DDX_Control(pDX, IDC_EDIT1, m_txt);
	DDX_Control(pDX, IDC_EDIT2, m_txt2);
}

BEGIN_MESSAGE_MAP(CSPITestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CSPITestDlg::OnBnClickedButton1)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT1, &CSPITestDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDOK, &CSPITestDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSPITestDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON2, &CSPITestDlg::OnBnClickedButton2)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CSPITestDlg::OnCbnSelchangeCombo2)
	ON_BN_CLICKED(IDC_CHECK2, &CSPITestDlg::OnBnClickedCheck2)
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSPITestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSPITestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CSPITestDlg::OnBnClickedButton1()
{
	// TODO:  在此添加控件通知处理程序代码
	SetTimer(1, 1000, NULL);

	logfile << "SetTimer 1ms" << endl << endl;
	
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
	p_SPI_CloseChannel = (pfunc_SPI_CloseChannel)GET_FUN_POINTER(h_libMPSSE, "SPI_CloseChannel");


	status = p_SPI_GetNumChannels(&channels);
//	printf("Number of available SPI channels = %d\n", channels);


	status = p_SPI_OpenChannel(CHANNEL_TO_OPEN, &ftHandle); // Open the first available channel

	status = p_SPI_InitChannel(ftHandle, &channelConf);

//	printf("Enter a hex value to display in binary -  ");

	//read in a hex value from standard input
//	scanf_s("%x", &LEDS);
	logfile << "OpenSPI" << endl << endl;

//printf("End of SPI Demo");
//	status = p_SPI_CloseChannel(ftHandle);
	Sleep(2000);
}


void CSPITestDlg::OnTimer(UINT_PTR nIDEvent)
{
	uint8 buffer[8];
	
	CString strText;
	m_txt.GetWindowTextW(strText);
	memcpy(buffer, (LPCSTR)strText.GetBuffer(0), strText.GetAllocLength());
	buffer[0] = 'a';
	buffer[1] = 'b';
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	/*switch (nIDEvent)

	{
	case 1:*/
	logfile << "TimerWrite" << endl << endl;
		write_byte(buffer,2);
		read_byte(buffer);
		write_byte(buffer,2);
		read_byte(buffer);
		write_byte(buffer,2);
		read_byte(buffer);
		//TCHAR A;
	//	wstring str;
		//A = read_byte();
	//	str = uint8 *buffer.read_byte();
		m_txt2.SetWindowTextW((LPCTSTR)buffer);
		//printf("1");
	
		//break;
	    //default:
		//break;
	//}
	

	CDialogEx::OnTimer(nIDEvent);
}


void CSPITestDlg::OnEnChangeEdit1()
{
	CString txt;
	m_txt.GetWindowTextW(txt);
	
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}




void CSPITestDlg::OnBnClickedOk()
{
	// TODO:  在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


void CSPITestDlg::OnBnClickedCancel()
{
	// TODO:  在此添加控件通知处理程序代码
	CDialogEx::OnCancel();

}


void CSPITestDlg::OnBnClickedButton2()
{

	KillTimer(1);
	p_SPI_CloseChannel(ftHandle);

	logfile << "Close SPI" << endl << endl;


	logfile.close();
	// TODO:  在此添加控件通知处理程序代码
}


void CSPITestDlg::OnCbnSelchangeCombo2()
{
	// TODO:  在此添加控件通知处理程序代码


}


void CSPITestDlg::OnBnClickedCheck2()
{
	// TODO:  在此添加控件通知处理程序代码
}
