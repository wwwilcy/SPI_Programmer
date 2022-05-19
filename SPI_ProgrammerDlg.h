
// SPI_ProgrammerDlg.h: 头文件
//

#pragma once


// CSPI_ProgrammerDlg 对话框
class CSPI_ProgrammerDlg : public CDialogEx
{
// 构造
public:
	CSPI_ProgrammerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SPI_Programmer_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	HICON m_hIconLEDGREEN;  //设置绿灯
	HICON m_hIconLEDRED;	//设置红灯
	HICON m_hIconLOGO;
	CMenu m_Menu;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButtonConnectspi();
	afx_msg void OnBnClickedCancel();
	CEdit EDIT_LOG;
	CStatic m_Status_Connect;
	afx_msg void OnBnClickedButtonFlashid();
	afx_msg void OnBnClickedButtonSetSpiRate();
	CString Edit_SpiRate;
	afx_msg void OnBnClickedButtonReset();
	CEdit Edit_FlashID;
	afx_msg void OnBnClickedButtonReadflash();
	afx_msg void OnBnClickedButtonLogopen();
	CProgressCtrl m_ctrlProgress;
	afx_msg void OnBnClickedButtonChiperase();
	afx_msg void OnBnClickedButtonSpiwrite();
	afx_msg void OnBnClickedButtonSpiUnprotect();
	afx_msg void OnBnClickedButtonSpiProtect();
	afx_msg void OnBnClickedButtonSpiSr0();
	afx_msg void OnBnClickedButtonSpiSr1();
	afx_msg void OnBnClickedButtonSpiIsolation();
	CString m_Start_Address;
	afx_msg void OnBnClickedButtonSetStartAddress();
	afx_msg void OnVersionVersionabout();
	afx_msg void OnBnClickedButtonEmptyCheck();
};
