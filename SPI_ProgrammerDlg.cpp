/*******************************************************************************
  * @file    SPI_ProgrammerDlg.cpp
  * $Author: wl wang
  * $Revision: Ver1.0.0.1
  * $Date:: 2022-05-19 11:13:14 +0800 GuangDong ShenZhen
  * @brief    spi read/write/erase...operater function;  SPI config
  ******************************************************************************
  * @attention
  *All Rights Reserved @ 2022
  *
  ******************************************************************************
*/
#include "pch.h"
#include "framework.h"
#include "SPI_Programmer.h"
#include "SPI_ProgrammerDlg.h"
#include "afxdialogex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "usb_device.h"
#include "usb2spi.h"
#include "usb2gpio.h"
#include "CABOUTDLG.h"

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DEVICE_INFO DevInfo;
int DevHandle[10];
int SPIIndex = 0x00;
bool open_state;
int ret, i;
int Start_Address_dec; //start address for erase write

SPI_FLASH_CONFIG SPIFlashConfig;
#define GPIO_SUCCESS             (0)   //函数执行成功  
#define GPIO_ERR_NOT_SUPPORT     (-1)  //适配器不支持该函数  
#define GPIO_ERR_USB_WRITE_FAIL  (-2)  //USB写数据失败  
#define GPIO_ERR_USB_READ_FAIL   (-3)  //USB读数据失败  
#define GPIO_ERR_CMD_FAIL        (-4)  //命令执行失败 
//上下拉电阻配置
#define GPIO_PUPD_NOPULL 0X00
#define GPIO_PUPD_UP 0X01
#define GPIO_PUPD_DOWN 0X02
#define CMD_SECTOR_ERASE_4KB 0X20
#define CMD_BLOCK_ERSE_64KB 0XD8

unsigned char Status_Unprotect[] = { 0x01, 0x00 };  //解保护指令0x01 0x00
unsigned char Status_Protect[] = { 0x01, 0x7C };  //写保护指令0x01 0xFC
int WINAPI GPIO_SetOutput(int DevHandle, unsigned int PinMask, unsigned char PuPd);
int WINAPI GPIO_Write(int DevHandle, unsigned int PinMask, unsigned int PinValue);
int WINAPI SPI_FlashRead(int DevHandle, int SPIIndex, int StartAddr, unsigned char* pReadData, int ReadLen);
int WINAPI SPI_FlashInit(int DevHandle, int SPIIndex, int ClockSpeed, PSPI_FLASH_CONFIG pConfig);
int WINAPI SPI_FlashReadFast(int DevHandle, int SPIIndex, int StartAddr, unsigned char* pReadData, int ReadLen);
int WINAPI SPI_FlashErase(int DevHandle, int SPIIndex, int StartAddr, unsigned char AddrBytes, unsigned char EraseCmd, int TimeOutMs);
int WINAPI SPI_FlashEraseSector(int DevHandle, int SPIIndex, int StartSector, int NumSector, int TimeOutMs);
int WINAPI SPI_FlashEraseChip(int DevHandle, int SPIIndex, int TimeOutMs);
int WINAPI SPI_FlashWrite(int DevHandle, int SPIIndex, int StartAddr, unsigned char* pWriteData, int WriteLen);
int WINAPI SPI_WriteBytes(int DevHandle, int SPIIndex, unsigned char* pWriteData, int WriteLen);
int WINAPI SPI_ReadBytes(int DevHandle, int SPIIndex, unsigned char* pReadData, int ReadLen);
int WINAPI SPI_WriteReadBytes(int DevHandle, int SPIIndex, unsigned char* pWriteData, int WriteLen, unsigned char* pReadData, int ReadLen, int IntervalTimeUs);
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnVersionVersionabout();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_COMMAND(ID_VERSION_VERSIONABOUT, &CSPI_ProgrammerDlg::OnVersionVersionabout)
END_MESSAGE_MAP()

// CSPI_ProgrammerDlg 对话框

CSPI_ProgrammerDlg::CSPI_ProgrammerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SPI_Programmer_DIALOG, pParent)
	, Edit_SpiRate(_T(""))
	, m_Start_Address(_T(""))
{
	//m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME); //SYSTEM ICON
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_LOGO);
}

void CSPI_ProgrammerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_LOG, EDIT_LOG);
	DDX_Control(pDX, IDC_STATUS_CONNECT, m_Status_Connect);
	DDX_Text(pDX, IDC_EDIT_SPIRATE, Edit_SpiRate);
	DDX_Control(pDX, IDC_EDIT_FLASHID, Edit_FlashID);
	DDX_Control(pDX, IDC_PROGRESS_READ, m_ctrlProgress);
	DDX_Text(pDX, IDC_EDIT_START_ADDRESS, m_Start_Address);
}

BEGIN_MESSAGE_MAP(CSPI_ProgrammerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONNECTSPI, &CSPI_ProgrammerDlg::OnBnClickedButtonConnectspi)
	ON_BN_CLICKED(IDCANCEL, &CSPI_ProgrammerDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_FLASHID, &CSPI_ProgrammerDlg::OnBnClickedButtonFlashid)
	ON_BN_CLICKED(IDC_BUTTON_SET_SPI_RATE, &CSPI_ProgrammerDlg::OnBnClickedButtonSetSpiRate)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CSPI_ProgrammerDlg::OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_READFLASH, &CSPI_ProgrammerDlg::OnBnClickedButtonReadflash)
	ON_BN_CLICKED(IDC_BUTTON_LOGOPEN, &CSPI_ProgrammerDlg::OnBnClickedButtonLogopen)
	ON_BN_CLICKED(IDC_BUTTON_CHIPERASE, &CSPI_ProgrammerDlg::OnBnClickedButtonChiperase)
	ON_BN_CLICKED(IDC_BUTTON_SPIWRITE, &CSPI_ProgrammerDlg::OnBnClickedButtonSpiwrite)
	ON_BN_CLICKED(IDC_BUTTON_SPI_UNPROTECT, &CSPI_ProgrammerDlg::OnBnClickedButtonSpiUnprotect)
	ON_BN_CLICKED(IDC_BUTTON_SPI_PROTECT, &CSPI_ProgrammerDlg::OnBnClickedButtonSpiProtect)
	ON_BN_CLICKED(IDC_BUTTON_SPI_SR0, &CSPI_ProgrammerDlg::OnBnClickedButtonSpiSr0)
	ON_BN_CLICKED(IDC_BUTTON_SPI_SR1, &CSPI_ProgrammerDlg::OnBnClickedButtonSpiSr1)
	ON_BN_CLICKED(IDC_BUTTON_SPI_ISOLATION, &CSPI_ProgrammerDlg::OnBnClickedButtonSpiIsolation)
	ON_BN_CLICKED(IDC_BUTTON_SET_START_ADDRESS, &CSPI_ProgrammerDlg::OnBnClickedButtonSetStartAddress)
	ON_COMMAND(ID_VERSION_VERSIONABOUT, &CSPI_ProgrammerDlg::OnVersionVersionabout)
	ON_BN_CLICKED(IDC_BUTTON_EMPTY_CHECK, &CSPI_ProgrammerDlg::OnBnClickedButtonEmptyCheck)
END_MESSAGE_MAP()

// CSPI_ProgrammerDlg 消息处理程序

BOOL CSPI_ProgrammerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// 将“关于...”菜单项添加到系统菜单中。
	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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
	m_Menu.LoadMenu(IDR_MENU1);     //标题栏
	SetMenu(&m_Menu);
	// TODO: 在此添加额外的初始化代码
	//初始化log文本
	el::Configurations conf("log.conf");
	el::Loggers::reconfigureAllLoggers(conf);
	LOG(INFO) << "SPI Programmmer Start!";
	//load icon
	m_hIconLEDGREEN = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	m_hIconLEDRED = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON1));
	m_hIconLOGO = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_LOGO));
	m_Status_Connect.SetIcon(m_hIconLEDRED); //初始化红色，连接OK后绿色
	
	UpdateData(TRUE);

	//命令
	SPIFlashConfig.CMD_EraseSector = 0x20;
	SPIFlashConfig.CMD_ReadData = 0x03;
	SPIFlashConfig.CMD_ReadFast = 0x0B;
	SPIFlashConfig.CMD_ReadID = 0x9F;
	SPIFlashConfig.CMD_ReadStatus = 0x05;
	SPIFlashConfig.CMD_WriteStatus = 0X01;
	SPIFlashConfig.CMD_WriteEnable = 0x06;
	SPIFlashConfig.CMD_WritePage = 0x02;
	SPIFlashConfig.CMD_EraseChip = 0xC7;
	SPIFlashConfig.EraseSectorAddressBytes = 3;
	SPIFlashConfig.ID_Length = 3;
	SPIFlashConfig.NumPages = 32768;
	SPIFlashConfig.PageSize = 256;
	SPIFlashConfig.ReadDataAddressBytes = 3;
	SPIFlashConfig.ReadFastAddressBytes = 3;
	SPIFlashConfig.SectorSize = 4096;
	SPIFlashConfig.WritePageAddressBytes = 3;
	
	//spi rate
	CString spi_rate;
	spi_rate.Format(_T("%d"), 12);
	SetDlgItemText(IDC_EDIT_SPIRATE, spi_rate);
	//起始地址默认值设置为0
	Start_Address_dec = 0;
	CString address_str;
	address_str.Format(_T("%d"), Start_Address_dec);
	SetDlgItemText(IDC_EDIT_START_ADDRESS, address_str);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSPI_ProgrammerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CSPI_ProgrammerDlg::OnPaint()
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
HCURSOR CSPI_ProgrammerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSPI_ProgrammerDlg::OnBnClickedButtonConnectspi()
{
	// TODO:连接并初始化设备
	//连接USB 
	UpdateData(TRUE);
	int DevHandle_temp[10];
	int SPIIndex = 0x00;
	bool state;
	int ret, i;
	//扫描查找设备
	ret = USB_ScanDevice(DevHandle_temp);
	if (ret <= 0)
	{
		LOG(ERROR) << "No device connected!";
		m_Status_Connect.SetIcon(m_hIconLEDRED);
		//CDialogEx::OnCancel();
	}
	else
	{
		DevHandle[0] = DevHandle_temp[0];
		SetDlgItemInt(IDC_EDIT_LOG, DevHandle[0]);
		LOG(INFO) << "Device Connect Successfully!";
		LOG(INFO) << "Device No: " << DevHandle[0];

	}
	//打开设备
	open_state = USB_OpenDevice(DevHandle[0]);
	if (!open_state)
	{
		LOG(ERROR) << "Open device error!";
	}
	else
	{
		LOG(INFO) << "Device Open Successfully!";
		m_Status_Connect.SetIcon(m_hIconLEDGREEN);
	}
	ret = SPI_FlashInit(DevHandle[0], SPIIndex, 12500000, &SPIFlashConfig);
	if (ret != SPI_SUCCESS) {
		LOG(ERROR) << "Initialize Device Error!";
	}
	else
	{
		m_Status_Connect.SetIcon(m_hIconLEDGREEN); //初始化连接OK后绿色
		CString spi_rate;
		spi_rate.Format(_T("%d"), 12);
		SetDlgItemText(IDC_EDIT_SPIRATE, spi_rate);
		LOG(INFO) << "Set Device SPI_RATE:" << 12500000 << "Hz";
	}
}


void CSPI_ProgrammerDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	if (open_state) 
	{
		bool state;
		state = USB_CloseDevice(DevHandle[0]);
		if (state)
		{
			LOG(INFO) << "关闭设备成功!";
		}
		else
		{
			LOG(ERROR) << "关闭设备失败！";
		}
	}
	LOG(INFO) << "Quit!";
	CDialogEx::OnCancel();
}


void CSPI_ProgrammerDlg::OnBnClickedButtonFlashid()
{
	// TODO: 在此添加控件通知处理程序代码
	CString FlashID_str;

	//读取芯片ID
	ret = SPI_FlashReadID(DevHandle[0], SPIIndex, SPIFlashConfig.ID);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "Get Flash ID Error!";
	}
	else
	{
		LOG(INFO) << "Flash ID = " << (int)SPIFlashConfig.ID[0] << ":" << (int)SPIFlashConfig.ID[1] << ":" << (int)SPIFlashConfig.ID[2];
		FlashID_str.Format(_T("%X, %X, %X"), SPIFlashConfig.ID[0], SPIFlashConfig.ID[1], SPIFlashConfig.ID[2]); //int->CString 其他的基本类型转化类似
		SetDlgItemText( IDC_EDIT_FLASHID, FlashID_str );
		USES_CONVERSION;
		std::string FlashID_str(W2A(FlashID_str));
		LOG(INFO) << FlashID_str;
		if ( (SPIFlashConfig.ID[0] == 0xFF) && (SPIFlashConfig.ID[1] == 0xFF) && (SPIFlashConfig.ID[2] == 0xFF) || (SPIFlashConfig.ID[0] == 0x00) && (SPIFlashConfig.ID[1] == 0x00) && (SPIFlashConfig.ID[2] == 0x00))
		{
			LOG(ERROR) << "Unknown Flash ID!";
		}
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonSetSpiRate()
{
	// TODO: 在此添加控件通知处理程序代码
	CString rate;
	double rate_spi;
	GetDlgItemText(IDC_EDIT_SPIRATE, rate);
	//rate.Format(Edit_SpiRate);
	//MessageBox(rate, _T("Spi_Rate  Mhz"));
	rate_spi = _wtof(LPCTSTR(rate));
	ret = SPI_FlashInit(DevHandle[0], SPIIndex, rate_spi * 1000000, &SPIFlashConfig);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "Initialize Device Error!";
	}
	else
	{
		LOG(INFO) << "Set Device SPI_RATE:" << (double)rate_spi << "MHZ";
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonReset()
{
	// TODO: 在此添加控件通知处理程序代码
	//erase/write Reset TCON
	int ret, ret_L,ret_H;
	//将P8 配置为下拉输出模式  reset pin 
	ret = GPIO_SetOutput(DevHandle[0], 0x0100, GPIO_PUPD_NOPULL);
	ret_L = GPIO_Write(DevHandle[0], 0x0100, 0x0000);    //reset L
	if (ret_L != GPIO_SUCCESS)
	{
		LOG(ERROR) << "配置Reset引脚L失败";
	}
	else
	{
		LOG(INFO) << "配置Reset引脚L成功";
	}
	Sleep(1000);
	//ret = GPIO_Write(DevHandle[0], 0x0200, 0x0000);    //Control pin L
	//Sleep(100);
	ret_H = GPIO_Write(DevHandle[0], 0x0100, 0x0100);	//reset H
	if (ret_H != GPIO_SUCCESS)
	{
		LOG(ERROR) << "配置Reset H失败";
	}
	else
	{
		LOG(INFO) << "配置Reset H成功";
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonReadflash()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret;
	int SPIIndex = SPI1_CS0;
	unsigned char *ReadBinALL; //4MB Flash

	ret = SPI_FlashReadID(DevHandle[0], SPIIndex, SPIFlashConfig.ID); //通过ID判断Flash 容量大小
	int Flash_Size_MB = pow(2, ((int)SPIFlashConfig.ID[2] - 20));  // 14hex-20dec 1MB  15hex-21dec 2MB  16hex-22dec 3MB 17hex-23dec 4MB

	ReadBinALL = (unsigned char*)malloc(Flash_Size_MB *1024*1024);
	unsigned char ReadBuffer[20*1024];  //单次Max 20K Byte
	//地址 
	//unsigned char Start_Address = 0;
	//int Start_Address_dec_temp;
	//Start_Address_dec_temp = Start_Address_dec;
	LOG(INFO) << "Read Bin Start";
	//进度条控制
	m_ctrlProgress.SetRange32(0, Flash_Size_MB * 1024 * 1024 - Start_Address_dec);
	int nProgram_pos = m_ctrlProgress.GetPos();
	nProgram_pos = 0;

	//read data
	for ( int i = 0; i < ( Flash_Size_MB * 1024 * 1024 - Start_Address_dec ); i += (20 * 1024) )
	{
		if ( (Flash_Size_MB  * 1024 * 1024 - i) >= (20 *1024) )
		{
			ret = SPI_FlashReadFast( DevHandle[0], SPIIndex, Start_Address_dec + i, ReadBuffer, sizeof(ReadBuffer) );
			memcpy(ReadBinALL+i, ReadBuffer, (20 * 1024) );
			nProgram_pos = nProgram_pos + i;
			m_ctrlProgress.SetPos(nProgram_pos);
		}
		else //少于20KByte
		{
			ret = SPI_FlashReadFast( DevHandle[0], SPIIndex, Start_Address_dec + i, ReadBuffer, (Flash_Size_MB * 1024 * 1024 - Start_Address_dec - i) );
			memcpy(ReadBinALL + i, ReadBuffer, (Flash_Size_MB * 1024 * 1024 - Start_Address_dec - i));
			nProgram_pos = nProgram_pos + (Flash_Size_MB * 1024 * 1024 - Start_Address_dec - i);
			m_ctrlProgress.SetPos(nProgram_pos);
		}
	}

	LOG(INFO) << "Save Bin Start";

	//对话框交互式 保存bin文件
	char szFilters[] = "Binary file(*.bin)";
	CStdioFile file;
	CString filepath,filename;
	TCHAR szFilter[] = _T("Binary file(*.bin)|*.bin|所有文件(*.*)|*.*||");
	CFileDialog fileDlg(FALSE, _T("bin"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, this);

	if (IDOK == fileDlg.DoModal())
	{		
		filepath = fileDlg.GetPathName();
		file.Open(filepath, CFile::modeCreate | CFile::modeWrite| CFile::typeBinary);
		filepath = file.GetFilePath();
		filename = file.GetFileName();
		USES_CONVERSION;
		std::string filename_str(W2A(filename));
		std::string pathname_str(W2A(filepath));
		LOG(INFO) << "Save Bin  " << pathname_str << "\\" << filename_str;  //cstring 转string，存log
		file.Write(ReadBinALL, Flash_Size_MB * 1024 * 1024);
		file.Close();
	}
	LOG(INFO) << "Save Bin End";

	free(ReadBinALL);
	
}


void CSPI_ProgrammerDlg::OnBnClickedButtonLogopen()
{
	// TODO: 在此添加控件通知处理程序代码
	//打开当前log 方便调试, 设置要打开的log文件路径和名称 与存储的路径 名称一致
	CTime tm;
	tm = CTime::GetCurrentTime();
	CString str1,logpath;
	str1 = tm.Format("%Y%m%d");
	logpath = _T(".\\log\\log_") + str1 + _T(".log");		//这里添加需要查看的文件路径 或者exe文件 	
	ShellExecute(NULL, _T("open"), logpath, NULL, NULL, SW_SHOWNORMAL);
}


void CSPI_ProgrammerDlg::OnBnClickedButtonChiperase()
{
	// TODO: 在此添加控件通知处理程序代码
	//chip erase,实现Flash芯片整块区域擦除,timeout 30s
	int SPIIndex = SPI1_CS0;
	unsigned char WriteEnable[] = { 0x06 };
	
	unsigned char StatusReg_buff[] = { 0x00 };
	unsigned char StatusReg_Read[] = { 0x05 };
	ret = SPI_WriteBytes(DevHandle[0], SPIIndex, WriteEnable, 1);
	ret = SPI_WriteReadBytes(DevHandle[0], SPIIndex, StatusReg_Read, 1, StatusReg_buff, 1, 500);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片寄存器读取出错!";
	}
	LOG(INFO) << "Status Register is: " << (int)( StatusReg_buff[0] & 0xFC);
	ret = SPI_WriteBytes( DevHandle[0], SPIIndex, Status_Unprotect, 2);  //解除保护
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片寄存器解除出错!";
	}
	ret = SPI_WriteReadBytes(DevHandle[0], SPIIndex, StatusReg_Read, 1, StatusReg_buff, 1, 500);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片寄存器读取出错!";
	}
	LOG(INFO) << "Protect Release:Status Register is: " << (int)(StatusReg_buff[0] & 0xFC);
	ret = SPI_FlashEraseChip( DevHandle[0], SPIIndex, 30*1000 );  //timeout 时间30s  
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片擦出出错!";
	}
	else
	{
		MessageBox(_T("chip erase OK!"), _T("Chip Erase"), MB_ICONINFORMATION);
		LOG(INFO) << "chip erase successed!";
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonSpiwrite()
{
	// TODO: 在此添加控件通知处理程序代码
	int SPIIndex = SPI1_CS0;
	//对话框交互式 打开bin文件
	unsigned char* ReadBinALL; // Flash

	unsigned char WriteEnable[] = { 0x06 };
	unsigned char EraseBlock64KB[] = { 0xD8 };
	ret = SPI_WriteBytes(DevHandle[0], SPIIndex, WriteEnable, 1);
	ret = SPI_WriteBytes(DevHandle[0], SPIIndex, Status_Unprotect, sizeof(Status_Unprotect)); //unprotect 
	Sleep(50);
	CString FlashID_str;
	//读取芯片ID
	ret = SPI_FlashReadID(DevHandle[0], SPIIndex, SPIFlashConfig.ID);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "Get Flash ID Error!";
	}
	else
	{
		LOG(INFO) << "Flash ID = " << (int)SPIFlashConfig.ID[0] << ":" << (int)SPIFlashConfig.ID[1] << ":" << (int)SPIFlashConfig.ID[2];
		FlashID_str.Format(_T("%X, %X, %X"), SPIFlashConfig.ID[0], SPIFlashConfig.ID[1], SPIFlashConfig.ID[2]); //int->CString 其他的基本类型转化类似
		SetDlgItemText(IDC_EDIT_FLASHID, FlashID_str);
		USES_CONVERSION;
		std::string FlashID_str(W2A(FlashID_str));
		LOG(INFO) << FlashID_str;
		if ((SPIFlashConfig.ID[0] == 0xFF) && (SPIFlashConfig.ID[1] == 0xFF) && (SPIFlashConfig.ID[2] == 0xFF) || (SPIFlashConfig.ID[0] == 0x00) && (SPIFlashConfig.ID[1] == 0x00) && (SPIFlashConfig.ID[2] == 0x00))
		{
			LOG(ERROR) << "Unknown Flash ID!";
		}
	}
	//ret = SPI_FlashReadID(DevHandle[0], SPIIndex, SPIFlashConfig.ID); //通过ID判断Flash 容量大小
	int Flash_Size_MB = pow( 2, ( (int)SPIFlashConfig.ID[2] - 20 ) );  // 14hex-20dec 1MB  15hex-21dec 2MB  16hex-22dec 3MB 17hex-23dec 4MB
	ReadBinALL = (unsigned char*)malloc(Flash_Size_MB * 1024 * 1024);

	int BinFile_Size;
	char szFilters[] = "Binary file(*.bin)";
	CStdioFile file;
	CString filepath, filename;

	TCHAR szFilter[] = _T("Binary file(*.bin)|*.bin|所有文件(*.*)|*.*||");
	CFileDialog fileDlg(TRUE, _T("bin"), NULL, OFN_HIDEREADONLY | OFN_READONLY, szFilter, this);
	LOG(INFO) << "bin文件读取";
	if (IDOK == fileDlg.DoModal())
	{
		filepath = fileDlg.GetPathName();
		file.Open(filepath, CFile::modeRead | CFile::typeBinary);
		file.Read(ReadBinALL, Flash_Size_MB * 1024 * 1024);
		BinFile_Size = file.GetLength();
		filename = file.GetFileName();
		USES_CONVERSION;
		std::string filename_str(W2A(filename));
		std::string pathname_str(W2A(filepath));
		LOG(INFO) << pathname_str << "\\" << filename_str;
		file.Close();
	}
	LOG(INFO) << "芯片 Erase 开始";
	//jindu tiao
	unsigned char* Bin_temp; // Flash write temp
	Bin_temp = (unsigned char*)malloc(10 * 1024);
	int write_result = -1;  //写入结果，=0是成功
	
	int nProgram_pos = m_ctrlProgress.GetPos();	
	// erase, size = binsize
	ret = SPI_WriteBytes( DevHandle[0], SPIIndex, WriteEnable, 1);
	int start_address_erase = Start_Address_dec;
	//4kb address start,Toomoss only 4KB erase or chiperase
	if (BinFile_Size <= ((Flash_Size_MB * 1024 * 1024) - Start_Address_dec))  //load binsize <= 当前Flash容量
	{

		//start address is 4KB address
		if ((start_address_erase % 4096 == 0) && (start_address_erase % 65536 != 0))
		{
			int start_secotr4KB = (start_address_erase / 4096);   //start 64kbsector  number

			int Num1_Sector4KB = (ceil)((double)BinFile_Size / 4096);  //4kb erase until BINsize
			m_ctrlProgress.SetRange32(0, Num1_Sector4KB);
			nProgram_pos = 0;
			for (int i = 0;i <= Num1_Sector4KB;i++)
			{
				ret = SPI_FlashEraseSector(DevHandle[0], SPIIndex, start_secotr4KB + i, 1, 20000);		//4KB erase
				nProgram_pos = nProgram_pos + i;
				m_ctrlProgress.SetPos(nProgram_pos);
				if (ret != SPI_SUCCESS)
				{
					LOG(ERROR) << "erase error@ sector4kb " << start_secotr4KB + i;
				}
			}
			LOG(INFO) << "4KB Erase start sector4KB: " << start_secotr4KB << "  4kb block number is: " << Num1_Sector4KB;
		}

		//start address is 64KB address
		if (start_address_erase % 65536 == 0)    
		{
			m_ctrlProgress.SetRange32(0, BinFile_Size);
			nProgram_pos = 0;
			for (int i = start_address_erase; i < ( BinFile_Size - start_address_erase); i += 65536 )
			{
				/// <64KB Erase>
				unsigned char data[4];
				unsigned char WriteEnable[] = { 0x06 }; //wel
				unsigned char Status_Read[] = { 0x05 }; //read register
				unsigned char Status_Buff[] = { 0x00 };
				double time_use;

				LARGE_INTEGER litmp;
				LONGLONG StartTime, EndTime;
				double dfFreq;
				QueryPerformanceFrequency(&litmp);// Get the performance counter frequency, in n/s
				dfFreq = (double)litmp.QuadPart;
				QueryPerformanceCounter(&litmp);  // Get the current value of the performance counter
				StartTime = litmp.QuadPart;       // Start time
				int ret = SPI_WriteBytes( DevHandle[0], SPIIndex, WriteEnable, 1); //WEL

				data[0] = 0xD8;  //block64kb CMD
				data[1] = ( i >> 16 ) & 0xFF;
				data[2] = ( i >> 8 ) & 0xFF;
				data[3] = i >> 0 & 0xFF;
				ret = SPI_WriteBytes( DevHandle[0], SPIIndex, data, sizeof(data)); //Block64 Erase

				ret = SPI_WriteReadBytes( DevHandle[0], SPIIndex, Status_Read, 1, Status_Buff, 1, 500);
				int WIP_temp = (Status_Buff[0] & 0x01);
				while (WIP_temp)
				{
					//Sleep(10);
					ret = SPI_WriteReadBytes( DevHandle[0], SPIIndex, Status_Read, 1, Status_Buff, 1, 500);
					WIP_temp = (Status_Buff[0] & 0x01);
					QueryPerformanceCounter(&litmp);// Get the current value of the performance counter
					EndTime = litmp.QuadPart; // Stop time
					time_use = (EndTime - StartTime) / dfFreq;
					if ((time_use * 1000) > 5000)
					{
						LOG(ERROR) << "ERASE Timeout!";
						break;
					}
				}
				/// <64KB Erase>
				nProgram_pos = nProgram_pos + i - start_address_erase;
				m_ctrlProgress.SetPos(nProgram_pos);
				LOG(INFO) << "erase @ sector64kb " << (i >> 16);
			}
			LOG(INFO) << "64KB Erase start sector4KB: " << 0 << "  64kb block end address is: " << i;
		}
		// write
		LOG(INFO) << "芯片 Write 开始";
		m_ctrlProgress.SetRange32(0, BinFile_Size);
		nProgram_pos = 0;
		for (int i = 0; i < (BinFile_Size); i += (10 * 1024))
		{
			if (BinFile_Size <= ((Flash_Size_MB * 1024 * 1024) - Start_Address_dec))  //load binsize <= 当前Flash容量
			{
				if ((BinFile_Size - i) >= (10 * 1024))
				{
					memcpy(Bin_temp, ReadBinALL + i, (10 * 1024));
					ret = SPI_FlashWrite(DevHandle[0], SPIIndex, Start_Address_dec + i, Bin_temp, 10 * 1024);
					if (ret != SPI_SUCCESS)
					{
						LOG(ERROR) << "芯片写入出错!";
						break;
					}
					else
					{
						//LOG(INFO) << "芯片 写入 successed!";
					}
					nProgram_pos = nProgram_pos + i;
					m_ctrlProgress.SetPos(nProgram_pos);
				}
				else //少于10KByte
				{
					memcpy(Bin_temp, ReadBinALL + i, (BinFile_Size - i));
					ret = SPI_FlashWrite(DevHandle[0], SPIIndex, Start_Address_dec + i, Bin_temp, (BinFile_Size - i));
					if (ret != SPI_SUCCESS)
					{
						LOG(ERROR) << "芯片写入出错!";
						break;
					}

					nProgram_pos = nProgram_pos + (BinFile_Size - i);
					m_ctrlProgress.SetPos(nProgram_pos);
				}
				if (ret == SPI_SUCCESS)
				{
					write_result = 0;
					//LOG(INFO) << "芯片 写入 successed!";
				}
			}
			else
			{
				MessageBox(_T("BIN Size is Bigger than Available Memory Size"), MB_OK, MB_ICONERROR);
				LOG(ERROR) << "Write Error: BINSize is :" << BinFile_Size << ", Where FlashMemory is Only:" << (Flash_Size_MB * 1024 * 1024);
				break;
			}
		}

		//回读和校验功能
		//read data
		nProgram_pos = 0;
		unsigned char* ReadBinALL_Varify; // Flash
		ReadBinALL_Varify = (unsigned char*)malloc(BinFile_Size);
		unsigned char* Read_Buff;
		Read_Buff = (unsigned char*)malloc(20 * 1024);

		if (BinFile_Size <= (Flash_Size_MB * 1024 * 1024))
		{
			for (int i = 0; i < (BinFile_Size); i += (20 * 1024))
			{
				if ((BinFile_Size - i) >= (20 * 1024))
				{
					ret = SPI_FlashReadFast(DevHandle[0], SPIIndex, Start_Address_dec + i, Read_Buff, (20 * 1024));
					if (ret != SPI_SUCCESS)
					{
						write_result = -1;
						break;
					}
					memcpy(ReadBinALL_Varify + i, Read_Buff, (20 * 1024));
					nProgram_pos = nProgram_pos + i;
					m_ctrlProgress.SetPos(nProgram_pos);
				}
				else //少于20KByte
				{
					ret = SPI_FlashReadFast(DevHandle[0], SPIIndex, Start_Address_dec + i, Read_Buff, (BinFile_Size - i));
					if (ret != SPI_SUCCESS)
					{
						write_result = -1;
						break;
					}
					memcpy(ReadBinALL_Varify + i, Read_Buff, (BinFile_Size - i));
					nProgram_pos = nProgram_pos + (BinFile_Size - i);
					m_ctrlProgress.SetPos(nProgram_pos);
				}
			}
		}
		if (write_result == 0)
		{
			LOG(INFO) << "Write Data OK!";
			//varify
			int varify_result = 0;
			for (int i = 0; i < BinFile_Size; i++)
			{
				if (ReadBinALL_Varify[i] != ReadBinALL[i])
				{
					varify_result = -1;
					LOG(ERROR) << "Varify Error @ " << i << ", Original is :" << (int)ReadBinALL[i] << ", actual is : " << (int)ReadBinALL_Varify[i];
					MessageBox(_T("Varify Failed!"), _T("Varify"), MB_ICONERROR);
					break;
				}
			}
			if (varify_result == 0)
			{
				LOG(INFO) << "Varify OK";
				MessageBox(_T("Write & Varify OK!"), _T("Write & Varify"),MB_ICONINFORMATION);
			}
			else
			{
				LOG(INFO) << "Varify Failed!";
				MessageBox(_T("Varify Failed!"), _T("Varify"),MB_ICONERROR);
			}
		}
		else
		{
			LOG(ERROR) << "Write Data Failed!";
			MessageBox(_T("Write Failed!"), _T("Write"));
		}

		// 配置保护
		//unsigned char WriteEnable[] = { 0x06 };
		unsigned char StatusReg_buff[] = { 0x00 };
		unsigned char StatusReg_Read[] = { 0x05 };
		ret = SPI_WriteBytes(DevHandle[0], SPIIndex, WriteEnable, sizeof(WriteEnable));
		ret = SPI_WriteBytes(DevHandle[0], SPIIndex, Status_Protect, sizeof(Status_Protect));
		if (ret != SPI_SUCCESS)
		{
			LOG(ERROR) << "芯片寄存器-0解除出错!";
		}
		//SPI write read ,读写寄存器，write 05  读取sr1
		ret = SPI_WriteReadBytes(DevHandle[0], SPIIndex, StatusReg_Read, 1, StatusReg_buff, 1, 500);
		if (ret != SPI_SUCCESS)
		{
			LOG(ERROR) << "芯片寄存器-0读取出错!";
		}
		LOG(INFO) << "Protect set:Status Register-0 is: " << (int)(StatusReg_buff[0] & 0xFC);
		//UpdateData(false);

		free(ReadBinALL);
		free(Bin_temp);
		free(Read_Buff);
		free(ReadBinALL_Varify);
	}
	else
	{
		LOG(ERROR) << "BIN SIZE :" << BinFile_Size << "Where Avaliable Memory Size is:" << ((Flash_Size_MB * 1024 * 1024) - Start_Address_dec);
		MessageBox(_T("BIN Size is Bigger than Available Memory Size"), MB_OK, MB_ICONERROR);
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonSpiUnprotect()
{
	// TODO: 在此添加控件通知处理程序代码
	unsigned char WriteEnable[] = { 0x06 };
	unsigned char Status_Read[] = { 0x05 };
	unsigned char Status_Buff[] = { 0x00 };

	ret = SPI_WriteBytes(DevHandle[0], SPIIndex, WriteEnable, 1);
	ret = SPI_WriteBytes(DevHandle[0], SPIIndex, Status_Unprotect, 2);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片寄存器-0解除出错!";
	}
	Sleep(100);
	ret = SPI_WriteReadBytes(DevHandle[0], SPIIndex, Status_Read, 1, Status_Buff, 1, 500);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片寄存器-0读取出错!";
	}
	LOG(INFO) << "Protect Release:Status Register-0 is: " << (int)(Status_Buff[0] & 0xFC);
	CString Status_str;
	Status_str.Format(_T("%X"), (Status_Buff[0] & 0xFC)); //int->CString 其他的基本类型转化类似
	
	MessageBox( Status_str, _T("Status Register-0"),MB_ICONINFORMATION);
}


void CSPI_ProgrammerDlg::OnBnClickedButtonSpiProtect()
{
	// TODO: 在此添加控件通知处理程序代码
	unsigned char WriteEnable[] = { 0x06 };
	unsigned char Status_Read[] = { 0x05 };
	unsigned char Status_Buff[] = { 0x00 };

	ret = SPI_WriteBytes(DevHandle[0], SPIIndex, WriteEnable, 1);
	ret = SPI_WriteBytes(DevHandle[0], SPIIndex, Status_Protect, 2);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片寄存器-0解除出错!";
	}
	Sleep(100);
	ret = SPI_WriteReadBytes(DevHandle[0], SPIIndex, Status_Read, 1, Status_Buff, 1, 500);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片寄存器-0读取出错!";
		MessageBox( _T("Status Register-0 Read Erroe!"),MB_OK, MB_ICONERROR);
	}
	else
	{
		LOG(INFO) << "Protect set:Status Register-0 is: " << (int)(Status_Buff[0] & 0xFC);
		CString Status_str;
		Status_str.Format(_T("%X"), (Status_Buff[0] & 0xFC)); //int->CString 其他的基本类型转化类似
		MessageBox(Status_str, _T("Status Register-0"), MB_ICONINFORMATION);
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonSpiSr0()
{
	// TODO: 在此添加控件通知处理程序代码
	unsigned char WriteEnable[] = { 0x06 };
	unsigned char Status_Read[] = { 0x05 };
	unsigned char Status_Buff[] = { 0x00 };
	ret = SPI_WriteReadBytes(DevHandle[0], SPIIndex, Status_Read, 1, Status_Buff, 1, 500);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "芯片寄存器-0读取出错!";
		MessageBox(_T("Status Register Read Error!"),MB_OK, MB_ICONERROR);
	}
	else
	{
		LOG(INFO) << "Protect set:Status Register-0 is: " << (int)(Status_Buff[0] & 0xFC);
		CString Status_str;
		Status_str.Format(_T("%X"), (Status_Buff[0] & 0xFC)); //int->CString 其他的基本类型转化类似
		MessageBox(Status_str, _T("Status Register-0"),MB_ICONINFORMATION);
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonSpiSr1()
{
	// TODO: 在此添加控件通知处理程序代码
	unsigned char WriteEnable[] = { 0x06 };
	unsigned char Status_Read[] = { 0x35 };
	unsigned char Status_Buff[] = { 0x00 };
	ret = SPI_WriteReadBytes(DevHandle[0], SPIIndex, Status_Read, 1, Status_Buff, 1, 500);
	if (ret != SPI_SUCCESS)
	{
		MessageBox(_T("Status Register-1 Read Error!"));
		LOG(ERROR) << "芯片寄存器-1读取出错!";
	}
	else
	{
		LOG(INFO) << "Protect set:Status Register-1 is: " << (int)(Status_Buff[0] & 0xFF);
		CString Status_str;
		Status_str.Format(_T("%X"), (Status_Buff[0] & 0xFF)); //int->CString 其他的基本类型转化类似
		MessageBox( Status_str, _T("Status Register-1"),MB_ICONINFORMATION);
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonSpiIsolation()
{
	//隔离所有pin
	ret = GPIO_SetOutput(DevHandle[0], 0xFFFF, GPIO_PUPD_NOPULL);
	//ret = GPIO_Write(DevHandle[0], 0x0100, 0x0100);
	if (ret != SPI_SUCCESS)
	{
		LOG(ERROR) << "SPI Programmer isolation ERROR!";
	}
	else
	{
		LOG(INFO) << "SPI Programmer isolation OK! ";
	}
}


void CSPI_ProgrammerDlg::OnBnClickedButtonSetStartAddress()
{
	// TODO: Set Start Address for write erase read.
	CString Start_Address;
	GetDlgItemText(IDC_EDIT_START_ADDRESS, Start_Address);
	Start_Address_dec = _tcstoul( Start_Address,0,16 ); //字符串转化为10进制整数
	LOG(INFO) << "Set address at: " << _wtof(Start_Address) << ", hex   " << Start_Address_dec << " dec";
}


void CSPI_ProgrammerDlg::OnVersionVersionabout()
{
	// TODO: goto -> vidsion dialog 
	CABOUTDLG Dlg;
	Dlg.DoModal();
}


void CSPI_ProgrammerDlg::OnBnClickedButtonEmptyCheck()
{
	// TODO: 在此添加控件通知处理程序代码
	// 0 - endaddress data FF check
	int ret;
	int SPIIndex = SPI1_CS0;
	unsigned char* ReadBinALL; //  Flash Memory

	ret = SPI_FlashReadID(DevHandle[0], SPIIndex, SPIFlashConfig.ID); //通过ID判断Flash 容量大小
	int Flash_Size_MB = pow(2, ((int)SPIFlashConfig.ID[2] - 20));  // 14hex-20dec 1MB  15hex-21dec 2MB  16hex-22dec 3MB 17hex-23dec 4MB

	ReadBinALL = (unsigned char*)malloc(Flash_Size_MB * 1024 * 1024);
	unsigned char ReadBuffer[20 * 1024];  //单次Max 20K Byte
	LOG(INFO) << "BinData empty Check Start";
	//进度条控制
	m_ctrlProgress.SetRange32(0, Flash_Size_MB * 1024 * 1024 );
	int nProgram_pos = m_ctrlProgress.GetPos();
	nProgram_pos = 0;

	//read data
	for (int i = 0; i <  Flash_Size_MB * 1024 * 1024 ; i += (20 * 1024))
	{
		if ((Flash_Size_MB * 1024 * 1024 - i) >= (20 * 1024))
		{
			ret = SPI_FlashReadFast(DevHandle[0], SPIIndex, i, ReadBuffer, sizeof(ReadBuffer));
			memcpy(ReadBinALL + i, ReadBuffer, (20 * 1024));
			nProgram_pos = nProgram_pos + i;
			m_ctrlProgress.SetPos(nProgram_pos);
		}
		else //少于20KByte
		{
			ret = SPI_FlashReadFast(DevHandle[0], SPIIndex, i, ReadBuffer, (Flash_Size_MB * 1024 * 1024 - i));
			memcpy(ReadBinALL + i, ReadBuffer, (Flash_Size_MB * 1024 * 1024 - i));
			nProgram_pos = nProgram_pos + (Flash_Size_MB * 1024 * 1024 - i);
			m_ctrlProgress.SetPos(nProgram_pos);
		}
	}
	//check FF
	bool Varify_Empty = 1;
	for (int i = 0; i < Flash_Size_MB * 1024 * 1024; i++ )
	{
		if (ReadBinALL[i] != 255)
		{
			Varify_Empty = 0;  //empty check error, bool = 0
			LOG(ERROR) << "Flash Data Empty Check error, data is:  " << (int)ReadBinALL[i] << ", @ address_dec " << i;
			MessageBox(_T("Flash Data Empty Check Error, details see log!"), MB_OK, MB_ICONERROR);
			break;
		}
	}

	if ( Varify_Empty )  //status = 1 ->successed
	{
		LOG(INFO) << "Flash Data Empty Check Successed!";
		MessageBox(_T("Flash Data Empty Check Successed!"),MB_OK,MB_ICONINFORMATION);
	}
	// memory free
	free(ReadBinALL);
}
