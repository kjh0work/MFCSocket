﻿// CClientChat.cpp: 구현 파일
//
#include <iostream>

#include "pch.h"
#include "SocketChat_improve.h"
#include "afxdialogex.h"
#include "CClientChat.h"
#include <thread>
#include "CGameover.h"

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define cell_size 40

#define M_RECV_UPDATE (WM_USER + 1) // 사용자 정의 메시지

HANDLE hServerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
bool isClientTurn = FALSE;

// CClientChat 대화 상자

IMPLEMENT_DYNAMIC(CClientChat, CDialogEx)

CClientChat::CClientChat(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent)
	, m_clientMsg(_T(""))
	, m_serverPort(_T(""))
{

}

//std::thread m_receiveThread;

CClientChat::~CClientChat()
{

}

void CClientChat::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_clientMsg);
	//  DDX_Control(pDX, IDC_Client_LIST, m_chatList);
	//  DDX_Control(pDX, IDC_Client_LIST, m_chatList_client);
	DDX_Control(pDX, IDC_LIST2, m_orderList);
	DDX_Control(pDX, IDC_CLIENT_LIST_TEST, m_client_chat_list);
	DDX_Text(pDX, IDC_SERVER_PORT, m_serverPort);
}


BEGIN_MESSAGE_MAP(CClientChat, CDialogEx)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CLIENT_MSG_BUTTON, &CClientChat::OnBnClickedClientMsgButton)
	ON_BN_CLICKED(IDC_END_BUTTON, &CClientChat::OnBnClickedEndButton)
	ON_MESSAGE(M_RECV_UPDATE, OnUpdateListbox)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void CClientChat::OnPaint()
{
	if (IsIconic())//내 프로그램이 아이콘화되었는지 확인
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
	}
	else
	{
		CPaintDC dc(this);//여기서만 CClientDC사용, Flag성변수를 1->0으로만드는 기능 포함되어있음
		CBrush brownBrush(RGB(236, 187, 90)); // 갈색 브러시 생성
		dc.SelectObject(&brownBrush); // 갈색 브러시 선택
		dc.Rectangle(cell_size - 10, cell_size - 10, cell_size * 13 + 10, cell_size * 13 + 10);

		for (int y = 0; y < 12; y++)
		{
			for (int x = 0; x < 12; x++)
			{
				dc.Rectangle(cell_size + x * cell_size, cell_size + y * cell_size,
					cell_size + x * cell_size + cell_size + 1, cell_size + y * cell_size + cell_size + 1);
			}
		}

		int radius = 5;
		CPen blackPen(PS_SOLID, 1, RGB(0, 0, 0)); // 검은색 펜 생성
		CPen* oldPen = dc.SelectObject(&blackPen); // 현재 펜 저장
		for (int y = 0; y <= 12; y++)
		{
			for (int x = 0; x <= 12; x++)
			{
				if (y % 3 == 0 && x % 3 == 0) {
					CBrush blackBrush(RGB(0, 0, 0)); // 검은색 브러시 생성
					CBrush* oldBrush = dc.SelectObject(&blackBrush); // 현재 브러시 저장

					dc.Ellipse(cell_size + x * cell_size - radius, cell_size + y * cell_size - radius, cell_size + x * cell_size + radius, cell_size + y * cell_size + radius);

					dc.SelectObject(oldBrush); // 이전 브러시로 복원
				}
			}
		}

		dc.SelectObject(oldPen); // 이전 펜으로 복원
		CBrush* p_old_brush = (CBrush*)dc.SelectStockObject(BLACK_BRUSH);
		for (int y = 1; y <= 13; y++)//저장된 바둑돌 재생성
		{
			for (int x = 1; x <= 13; x++)
			{
				if (m_dol[y - 1][x - 1] > 0)
				{

					if (m_dol[y - 1][x - 1] == 1)
					{
						dc.SelectStockObject(BLACK_BRUSH);
					}
					else
					{
						dc.SelectStockObject(WHITE_BRUSH);
					}

					dc.Ellipse(x * cell_size - 20, y * cell_size - 20, x * cell_size + 20, y * cell_size + 20);

				}
			}
		}
		dc.SelectObject(p_old_brush);
	}
}

// CClientChat 메시지 처리기

UINT CClientChat::ClientOwnThread(LPVOID aParam)
{
	CClientChat* pThis = reinterpret_cast<CClientChat*>(aParam);

	char buf[BUFSIZE+1];
	int len;

	//포트 번호 보내기
	CString portNum;
	portNum.Format(_T("%d"), pThis->myPortNum);
	CStringA strAnsi(portNum); // 유니코드 CString을 ANSI 문자열로 변환
	len = strAnsi.GetLength() + 1;
	
	// 버퍼 크기를 확인하고 널 종료 문자를 포함하여 복사
	buf[0] = 3;
	strncpy_s(buf + 1, BUFSIZE, strAnsi, len);


	// 데이터 보내기(고정 길이)
	int retval = send(pThis->m_sock, (char*)&len, sizeof(int), 0);
	// 데이터 보내기(가변 길이)
	retval = send(pThis->m_sock, buf, len, 0);

	while (true)
	{
		int retval = recv(pThis->m_sock, (char*)&len, sizeof(int), 0);
		if (retval <= 0) break;

		if (len >= BUFSIZE) break;

		retval = recv(pThis->m_sock, buf, len, 0);
		if (retval <= 0) break;

		buf[retval] = '\0';
		if (buf[0] == 1) {
			// PostMessage를 사용하여 메시지를 UI 스레드로 전달
			CString str(buf + 1);
			pThis->PostMessage(M_RECV_UPDATE, 0, (LPARAM)new CString(str));
		}
		else if (buf[0] == 2) {
			//SetEvent(hServerEvent);
			isClientTurn = TRUE;

			int x = buf[1];
			int y = buf[2];

			pThis->recivePoint(x, y, aParam);

			//TODO: 이 x y 좌표로 그리기
			CString test;
			test.Format(_T("x : %2d y : %2d         black"), x, y);
			pThis->PostMessageW(M_RECV_UPDATE, 2, (LPARAM)new CString(test));
		}
		else if (buf[0] == 3) {
			//상대방 포트 번호 받기
			CString str(buf + 1);
			pThis->PostMessage(M_RECV_UPDATE, 1, (LPARAM)new CString(str));
		}
	}

	// 소켓 닫기
	closesocket(pThis->m_sock);
	pThis->m_sock = INVALID_SOCKET;
	return 0;
}

void CClientChat::recivePoint(int x, int y, LPVOID aParam) {
	CClientChat* pThis = reinterpret_cast<CClientChat*>(aParam);
	CClientDC dc(this);
	int m_dol_state_ = (pThis->m_dol_state == 1 ? 0 : 1);
	pThis->m_dol[y - 1][x - 1] =  m_dol_state_+ 1;
	printf("%d %d", x, y);
	if (x > 0 && x <= 13 && y > 0 && y <= 13) {
		int rx_ = x;
		int ry_ = y;
		x *= 40;
		y *= 40;
		
		CBrush* p_old_brush;
		//client blck돌 수신
		p_old_brush = (CBrush*)dc.SelectStockObject(BLACK_BRUSH);

		dc.Ellipse(x - 20, y - 20, x + 20, y + 20);
		dc.SelectObject(p_old_brush);
		if (CheckWin(rx_ - 1, ry_ - 1, m_dol_state_)) {
			// 게임 이겼을 경우
			//일단 이기면 좌표 창에 우승자 표시
			CString winString;
			winString = _T("Winner is ");
			m_orderList.AddString(winString + (!m_dol_state_ ? "black" : "white"));

			//게임 종료 화면 띄우기
			CGameover gameover;
			gameover.SetWinner(!m_dol_state_ ? 1 : 2); // 흑돌이 이겼으면 1, 백돌이 이겼으면 2
			gameover.DoModal();
			SendMessage(WM_CLOSE, 0, 0);
		}

	}
}


LRESULT CClientChat::OnUpdateListbox(WPARAM wParam, LPARAM lParam) {
	if (wParam == 0) {
		CString* pStr = reinterpret_cast<CString*>(lParam);
		m_client_chat_list.AddString(pStr->GetString());
		delete pStr; // 동적으로 할당된 CString 객체를 삭제
	}
	else if (wParam == 1) {
		CFont m_font;
		m_font.CreatePointFont(110, _T("Arial")); // 16포인트 크기의 Arial 폰트
		GetDlgItem(IDC_SERVER_PORT)->SetFont(&m_font); // ID가 IDC_CLIENT_PORT인 컨트롤에 폰트 설정

		CString* pStr = reinterpret_cast<CString*>(lParam);
		CString str;
		str.Format(_T("상대편 포트번호 : %s"), pStr->GetString());

		m_serverPort.SetString(str);

		UpdateData(FALSE);

		delete pStr; // 동적으로 할당된 CString 객체를 삭제
	}
	else if (wParam == 2) {
		CString* pStr = reinterpret_cast<CString*>(lParam);
		m_orderList.AddString(pStr->GetString());
		delete pStr; // 동적으로 할당된 CString 객체를 삭제
	}
	return 0;
}

BOOL CClientChat::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0);

	// 소켓 생성
	m_sock = socket(AF_INET, SOCK_STREAM, 0);

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(m_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) { AfxMessageBox(_T("connect 오류")); }
	AfxMessageBox(_T("서버 연결 성공"));


	//자신의 포트 번호 알아내기
	struct sockaddr_in localAddr;
	int localAddressLength = sizeof(localAddr);
	
	if (getsockname(m_sock, (struct sockaddr*)&localAddr, &localAddressLength) == 0) {
		myPortNum = localAddr.sin_port;
	}
	else
	{
		// getsockname이 실패한 경우 에러 처리를 추가할 수 있습니다.
		int errorCode = WSAGetLastError();
		CString errorMsg;
		errorMsg.Format(_T("Error getting port number: %d"), errorCode);
		AfxMessageBox(errorMsg, MB_OK | MB_ICONERROR);
	}


	AfxBeginThread(ClientOwnThread, this);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CClientChat::OnBnClickedClientMsgButton()
{

	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData(TRUE);
	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE+1];
	int len;

	// 서버와 데이터 통신
	CString msg = m_clientMsg;
	CStringA strAnsi(msg); // 유니코드 CString을 ANSI 문자열로 변환
	len = strAnsi.GetLength() + 1;

	// 버퍼 크기를 확인하고 널 종료 문자를 포함하여 복사
	strncpy_s(buf+1, BUFSIZE, strAnsi, len);
	buf[0] = 1;
	// 데이터 보내기(고정 길이)
	int retval = send(m_sock, (char*)&len, sizeof(int), 0);
	// 데이터 보내기(가변 길이)
	retval = send(m_sock, buf, len, 0);

	m_client_chat_list.AddString(msg);

}


void CClientChat::OnBnClickedEndButton()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	// 소켓 닫기
	closesocket(m_sock);

	// 윈속 종료
	WSACleanup();
}

void CClientChat::OnSendPosition(int x, int y) {
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData(TRUE);
	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE];
	int len = 4;


	// 버퍼 크기를 확인하고 널 종료 문자를 포함하여 복사
	buf[0] = 2;
	buf[1] = x;
	buf[2] = y;


	// 데이터 보내기(고정 길이)
	int retval = send(m_sock, (char*)&len, sizeof(int), 0);
	// 데이터 보내기(가변 길이)
	retval = send(m_sock, buf, len, 0);
}

void CClientChat::OnSendMessage(char buf[], int len, int i) {
	buf[0] = i;
	int retval = send(m_sock, (char*)&len, sizeof(int), 0);
	retval = send(m_sock, buf, len, 0);
}


//헤더로 뺄것들
bool CClientChat::CheckFive(int x, int y, int dx, int dy, int m_dol_state_) {
	int count = 0;
	int dx_ = dx;
	int dy_ = dy;
	bool che = false;
	for (int i = 0; i < 5; ++i) {
		int newX = x + i * dx_;
		int newY = y + i * dy_;
		if (newX < 0 || newX >= 14 || newY < 0 || newY >= 14)
			break;
		if (m_dol[newY][newX] != m_dol_state_ + 1)
			break;
		count += 1;
	}
	if (count >= 5) {
		return true;
	}
	dx_ *= -1;
	dy_ *= -1;
	for (int i = 1; i < 5; ++i) {
		int newX = x + i * dx_;
		int newY = y + i * dy_;
		if (newX < 0 || newX >= 14 || newY < 0 || newY >= 14)
			return false;
		if (m_dol[newY][newX] != m_dol_state_ + 1)
			return false;
		count += 1;
		if (count >= 5) {
			return true;
		}
	}
	return false;
}
bool CClientChat::CheckWin(int x, int y, int m_dol_state_) {
	if (CheckFive(x, y, 1, 0, m_dol_state_) //가로
		|| CheckFive(x, y, 0, 1, m_dol_state_) //세로
		|| CheckFive(x, y, 1, 1, m_dol_state_) //대각선 
		|| CheckFive(x, y, 1, -1, m_dol_state_))//대각선
		return true;
	return false;
}

void CClientChat::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (isClientTurn == FALSE) return;
	//WaitForSingleObject(hServerEvent, INFINITE);

	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CClientDC dc(this);

	int x = (point.x + 20) / 40;
	int y = (point.y + 20) / 40;
	if (m_dol[y - 1][x - 1] > 0) return;    //중복 체크 

	if (x > 0 && x <= 13 && y > 0 && y <= 13) {

		m_dol[y - 1][x - 1] = m_dol_state + 1;
		OnSendPosition(x, y);
		int rx_ = x;
		int ry_ = y;
		x *= 40;
		y *= 40;

		CBrush* p_old_brush;
		
		// client는 백돌
		p_old_brush = (CBrush*)dc.SelectStockObject(WHITE_BRUSH);
		
		dc.Ellipse(x - 20, y - 20, x + 20, y + 20);
		dc.SelectObject(p_old_brush);
		if (CheckWin(rx_ - 1, ry_ - 1, m_dol_state)) {
			// 게임 이겼을 경우
			//일단 이기면 좌표 창에 우승자 표시
			CString winString;
			winString = _T("Winner is ");
			m_orderList.AddString(winString + (!m_dol_state ? "black" : "white"));

			//게임 종료 화면 띄우기
			CGameover gameover;
			gameover.SetWinner(!m_dol_state ? 1 : 2); // 흑돌이 이겼으면 1, 백돌이 이겼으면 2
			gameover.DoModal();
			SendMessage(WM_CLOSE, 0, 0);
		}

		CString str;
		str.Format(_T("x : %2d y : %2d"), x / 40, y / 40);

		m_orderList.AddString(str + " white");
		isClientTurn = FALSE;
	}


	CDialogEx::OnLButtonDown(nFlags, point);
}

