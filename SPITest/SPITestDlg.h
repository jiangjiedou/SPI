
// SPITestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CSPITestDlg 对话框
class CSPITestDlg : public CDialogEx
{
// 构造
public:
	CSPITestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SPITEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
//	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	CComboBox modeSel;
	afx_msg void OnCbnSelchangeModeSel();
	HICON m_hIcon;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedSend();
	CEdit recvContent;
	CEdit sendContent;
	afx_msg void OnBnClickedClearsend();
	afx_msg void OnBnClickedClearrecv();
	afx_msg void OnBnClickedReceive();
	CComboBox rdCount;
	afx_msg void OnCbnSelchangeReadcnt();
	afx_msg void OnEnChangeSendcontent();
};
