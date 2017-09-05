// PortScanView.cpp : implementation of the CPortScanView class
//

#include "stdafx.h"
#include "PortScan.h"
#include "TheSocket.h"
#include "PortScanDoc.h"
#include "PortScanView.h"
#include "MainFrm.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPortScanView

IMPLEMENT_DYNCREATE(CPortScanView, CFormView)

BEGIN_MESSAGE_MAP(CPortScanView, CFormView)
	//{{AFX_MSG_MAP(CPortScanView)
	ON_WM_CTLCOLOR()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_RADIO_SINGLE, OnRadioSingle)
	ON_BN_CLICKED(IDC_RADIO_RANGE, OnRadioRange)
	ON_BN_CLICKED(IDC_BUTTON_SCAN, OnButtonScan)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStop)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPortScanView construction/destruction

CPortScanView::CPortScanView()
	: CFormView(CPortScanView::IDD)
{
	//{{AFX_DATA_INIT(CPortScanView)
	//}}AFX_DATA_INIT
	// TODO: add construction code here
	m_pBrush = new CBrush;
	ASSERT(m_pBrush);
	//m_clrBk = RGB(0x00,0x00,0x77);
	m_clrText = RGB(0xff,0xff,0x00);
	//m_pBrush->CreateSolidBrush(m_clrBk);
	m_pColumns = new CStringList;
	ASSERT(m_pColumns);
	m_bSinglePort = TRUE;
	m_nMaxAttempts = 1;	//default value, This value has been set on the window
	m_pStatusList = new CPtrList;
	ASSERT(m_pStatusList);
}

CPortScanView::~CPortScanView()
{
	if (m_pStatusList)
	{
		//First Empty the port status list:
		POSITION p = m_pStatusList->GetHeadPosition();
		while (p)
		{
			POSITION temp = p;
			DATA* pNode = (DATA*)m_pStatusList->GetNext(p);
			m_pStatusList->RemoveAt(temp);
			if (pNode)
				delete pNode;
		}
		//Then remove it from heap:
		delete m_pStatusList;
	}

	if (m_pBrush)
		delete m_pBrush;
}

void CPortScanView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPortScanView)
	DDX_Control(pDX, IDC_PROGRESS, m_cProgress);
	DDX_Control(pDX, IDC_EDIT_ATTEMPTS, m_cAttempts);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_cBtnStop);
	DDX_Control(pDX, IDC_LIST_RESULT, m_cResult);
	DDX_Control(pDX, IDC_BUTTON_SCAN, m_cBtnScan);
	DDX_Control(pDX, IDC_IP_ADDRESS, m_cIP);
	DDX_Control(pDX, IDC_EDIT_SINGLE_PORT_TO, m_cPortTo);
	DDX_Control(pDX, IDC_EDIT_SINGLE_PORT_FROM, m_cPortFrom);
	DDX_Control(pDX, IDC_EDIT_SINGLE_PORT, m_cSinglePort);
	//}}AFX_DATA_MAP
}

BOOL CPortScanView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CPortScanView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit(FALSE);
	ResizeParentToFit();
	ShowHeaders();
	CheckRadioButton(IDC_RADIO_SINGLE,IDC_RADIO_RANGE,IDC_RADIO_SINGLE);
	m_cSinglePort.EnableWindow();
	m_cPortFrom.EnableWindow(FALSE);
	m_cPortTo.EnableWindow(FALSE);
	m_parent = (CMainFrame*)GetParent();
	ASSERT(m_parent);
//	m_cBtnStop.EnableWindow(FALSE);
	m_cAttempts.SetWindowText(_T("1"));
}

/////////////////////////////////////////////////////////////////////////////
// CPortScanView diagnostics

#ifdef _DEBUG
void CPortScanView::AssertValid() const
{
	CFormView::AssertValid();
}

void CPortScanView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CPortScanDoc* CPortScanView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPortScanDoc)));
	return (CPortScanDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPortScanView message handlers

HBRUSH CPortScanView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);
	//break statement must be ignored:
	switch(nCtlColor)
	{
	case CTLCOLOR_BTN:
	case CTLCOLOR_STATIC:
		pDC->SetBkColor(m_clrBk);
		pDC->SetTextColor(m_clrText);
	case CTLCOLOR_DLG:
		return static_cast<HBRUSH>(m_pBrush->GetSafeHandle());
	}
	
	// TODO: Return a different brush if the default is not desired
	return CFormView::OnCtlColor(pDC,pWnd,nCtlColor);
}


BOOL CPortScanView::AddColumn(LPCTSTR strItem,int nItem,int nSubItem,int nMask,int nFmt)
{
	LV_COLUMN lvc;
	lvc.mask = nMask;
	lvc.fmt = nFmt;
	lvc.pszText = (LPTSTR) strItem;
	lvc.cx = m_cResult.GetStringWidth(lvc.pszText) + 25;

	if(nMask & LVCF_SUBITEM)
	{
		if(nSubItem != -1)
			lvc.iSubItem = nSubItem;
		else
			lvc.iSubItem = nItem;
	}
	return m_cResult.InsertColumn(nItem,&lvc);
}

BOOL CPortScanView::AddItem(int nItem,int nSubItem,LPCTSTR strItem ,int nImageIndex)
{
	LV_ITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
	lvItem.iSubItem = nSubItem;
	lvItem.pszText = (LPTSTR) strItem;

	if(nImageIndex != -1)
	{
		lvItem.mask |= LVIF_IMAGE;
		lvItem.iImage |= LVIF_IMAGE;
	}

	if(nSubItem == 0)
		return m_cResult.InsertItem(&lvItem);

	return m_cResult.SetItem(&lvItem);
}

void CPortScanView::AddHeader(LPTSTR hdr)
{
	if (m_pColumns)
		m_pColumns->AddTail(hdr);
}

void CPortScanView::ShowHeaders()
{
	int nIndex = 0;
	POSITION pos = m_pColumns->GetHeadPosition();
	while (pos)
	{
		CString hdr = (CString)m_pColumns->GetNext(pos);
		AddColumn(hdr,nIndex++);
	}
}


void CPortScanView::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CFormView::OnShowWindow(bShow, nStatus);
	AddHeader(_T("IP��ַ"));
	AddHeader(_T("�˿ں�"));
	AddHeader(_T("�˿�״̬"));
	AddHeader(_T("���Ӵ���"));
	AddHeader(_T("��ע"));
}

void CPortScanView::OnRadioSingle() 
{
	m_bSinglePort = TRUE;
	m_cSinglePort.EnableWindow();
	m_cPortFrom.EnableWindow(FALSE);
	m_cPortTo.EnableWindow(FALSE);
}

void CPortScanView::OnRadioRange() 
{
	m_bSinglePort = FALSE;
	m_cSinglePort.EnableWindow(FALSE);
	m_cPortFrom.EnableWindow();
	m_cPortTo.EnableWindow();
	m_cBtnStop.EnableWindow(FALSE);
}

//��ʼɨ�谴ť�������
void CPortScanView::OnButtonScan() 
{
	CString btnTxt,IP;
	UINT nSinglePort;
	BYTE f1,f2,f3,f4;//���ʶIP��ַ��4���ֽ�
	TCHAR temp[10] = "\0";

	m_cProgress.SetPos(0);
	m_cResult.DeleteAllItems();

	//����б��
	POSITION p = m_pStatusList->GetHeadPosition();
	while (p)
	{
		POSITION temp = p;
		DATA* pNode = (DATA*)m_pStatusList->GetNext(p);
		m_pStatusList->RemoveAt(temp);
		//ѭ��ɾ��
		if (pNode)
			delete pNode;
	}

	//��֤IP��ַ�Ƿ�Ϊ��
	if (m_cIP.IsBlank())
	{
		MessageBox(_T("������IP��ַ."),
			_T("Error"),
			MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	
	//��֤IP��ַ�Ƿ���ȷ
	if (m_cIP.GetAddress(f1,f2,f3,f4) < 4)
	{
		MessageBox(_T("��ȷ��IP��ַ."),
			_T("Invalid IP address"),MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	//����IP��ַת��
	IP = _itoa(f1,temp,10);
	IP += _T('.');
	IP += _itoa(f2,temp,10);
	IP += _T('.');
	IP += _itoa(f3,temp,10);
	IP += _T('.');
	IP += _itoa(f4,temp,10);

	m_cBtnStop.EnableWindow();
	m_cBtnScan.EnableWindow(FALSE);

	//�Ƿ��ǵ����˿�ɨ��
	if (m_bSinglePort)
	{
		CString port;
		m_cSinglePort.GetWindowText(port);
		m_minPort = m_maxPort = nSinglePort = atoi(port);
	}
	else//����˿ڽ���ɨ��
	{
		CString port1,port2;
		m_cPortFrom.GetWindowText(port1);
		m_cPortTo.GetWindowText(port2);
		m_minPort = atoi(port1);
		m_maxPort = atoi(port2);
		m_cProgress.SetRange32(0,m_maxPort-m_minPort+1);
		m_cProgress.SetStep(1);
	}

	
	if (!m_bSinglePort && m_maxPort < m_minPort)
	{
		MessageBox(_T("���˿�Ҫ������С�˿�."),
			_T("Caution"),
			MB_OK | MB_ICONINFORMATION);
		return;
	}

	UINT m_nMaxAttempts = GetDlgItemInt(IDC_EDIT_ATTEMPTS);

	for (m_nCounter = m_minPort; m_nCounter <= m_maxPort; m_nCounter++)
	{
		BOOL bIsOpen = FALSE;
		UINT nAttempt = 1;
		
		while(nAttempt <= m_nMaxAttempts && !bIsOpen)
		{
			TCHAR temp[10]="\0";
			CString str = _T("���Ӷ˿�# ");
			#ifdef _UNICODE
			str += _itow(m_nCounter,temp,10);
			#else
			str += itoa(m_nCounter,temp,10);
			#endif
			str += _T(", IP��ַ=");
			str += IP;
			str += _T(", Attempt=");
			#ifdef _UNICODE
			str += _itow(nAttempt,temp,10);
			#else
			str += itoa(nAttempt,temp,10);
			#endif
			//�趨״̬��
			m_parent->SetStatusBarText(str);
			str.Empty();
			//������
			bIsOpen = TestConnection(IP,m_nCounter);
			//�ж��Ƿ��
			if (bIsOpen)
			{
				DATA* pNode = new DATA;
				ASSERT(pNode);
				strcpy(pNode->IPAddress,IP.GetBuffer(IP.GetLength()));
				strcpy(pNode->port,_itoa(m_nCounter,temp,10));
				pNode->bStatus = 1; //open
				pNode->nAttempts = nAttempt;
				m_pStatusList->AddTail(pNode);
			}
			//��ͼ���Ӵ���
			nAttempt++;
		}
		//��������޷�ɨ��ɹ�
		if (!bIsOpen)
		{
			DATA* pNode = new DATA;
			ASSERT(pNode);
			strcpy(pNode->IPAddress,IP.GetBuffer(IP.GetLength()));
			strcpy(pNode->port,_itoa(m_nCounter,temp,10));
			pNode->bStatus = 0; //�ر�״̬
			pNode->nAttempts = nAttempt-1;
			m_pStatusList->AddTail(pNode);
		}
		
		MSG message;
		if (::PeekMessage(&message,NULL,0,0,PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}
		m_cProgress.StepIt();
	}
	//�趨״̬��
	m_parent->SetStatusBarText((CString)_T("���"));
	m_cBtnScan.EnableWindow();
	m_cBtnStop.EnableWindow(FALSE);

	//��䱨����ͼ
	UINT nIndex = 0;
	POSITION pos = m_pStatusList->GetHeadPosition();
	//ѭ������ɨ����
	while (pos)
	{
		DATA* pNode = (DATA*)m_pStatusList->GetNext(pos);
		AddItem(nIndex,0,pNode->IPAddress);
		AddItem(nIndex,1,pNode->port);
		if (pNode->bStatus)
		{
			AddItem(nIndex,2,_T("��"));
			AddItem(nIndex,4,_T("*"));
		}
		else
		{
			AddItem(nIndex,2,_T("�ر�"));
			AddItem(nIndex,4,_T(" "));
		}
		AddItem(nIndex++,3,_itoa(pNode->nAttempts,temp,10));
	}
}

//������
BOOL CPortScanView::TestConnection(CString IP, UINT nPort)
{
	CTheSocket* pSocket;
	pSocket = new CTheSocket;
	ASSERT(pSocket);

	//����socket
	if (!pSocket->Create())
	{
		//�������ʧ�ܣ���ɾ�������� false
		delete pSocket;
		pSocket = NULL;
		return FALSE;
	}

	//���ӱ����ӵ�������ַ��ָ���˿�
	while (!pSocket->Connect(IP , nPort))
	{
		//���ʧ�ܷ���false
		delete pSocket;
		pSocket = NULL;
		return FALSE;
	}

	//�ر�socket
	pSocket->Close();
	delete pSocket;
	return TRUE;
}

void CPortScanView::OnButtonStop() 
{
	m_nCounter = m_maxPort+1;
	m_cBtnStop.EnableWindow(FALSE);
	m_cBtnScan.EnableWindow();
	m_parent->SetStatusBarText((CString)_T("Ready"));
}

//������ļ�
void CPortScanView::OnFileSave() 
{
	CFileDialog* pDlg = new CFileDialog(FALSE,
		_T("txt"),NULL,
		OFN_OVERWRITEPROMPT | 
		OFN_EXPLORER | OFN_LONGNAMES,
		_T("Scanned ports files (port.txt)"),this);

	ASSERT(pDlg);
	if (pDlg->DoModal() == IDOK)
	{
		int nHandle,retVal;
		nHandle = _open(pDlg->GetPathName(),_O_BINARY | _O_CREAT | _O_TRUNC | _O_RDWR);
		if (nHandle == -1)
		{
			MessageBox(_T("Unable to open output file to write."),
				_T("Error"),
				MB_OK | MB_ICONEXCLAMATION);
			delete pDlg;
			return;
		}

		POSITION pos = m_pStatusList->GetHeadPosition();
		while (pos)
		{
			CString buffer;
			DATA* pNode = (DATA*)m_pStatusList->GetNext(pos);
			buffer = pNode->IPAddress;
			buffer += _T(',');
			buffer += pNode->port;
			buffer += _T(',');

			if (pNode->bStatus)
				buffer += _T("Open");
			else
				buffer += _T("Close");

			buffer += _T("\r\n\0");
			retVal = _write(nHandle,
				(void*)buffer.GetBuffer(buffer.GetLength()), 
				buffer.GetLength());
			
			if (retVal != buffer.GetLength())
			{
				MessageBox(_T("An error occured while writing records."),
					_T("Error"),MB_OK | MB_ICONEXCLAMATION);
				delete pDlg;
				return;
			}
			buffer.Empty();
		}
		_close(nHandle);
	}
	delete pDlg;
}
