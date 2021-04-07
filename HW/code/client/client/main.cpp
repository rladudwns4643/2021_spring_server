#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <WS2tcpip.h>
#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <conio.h>
#ifdef UNICODE
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#else
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

using namespace std;
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSVCRTD.lib")

constexpr int BUF_SIZE = 1024;
constexpr int PORT = 3500;
constexpr int MAX_BUFFER = 1024;
constexpr int MAX_USER = 10;
constexpr int init_X = 5;
constexpr int init_Y = 5;
enum class e_KEY { LEFT = 1, RIGHT = 2, UP = 3, DOWN = 4, K_NULL = 5 };

char SERVER_ADDR[] = "127.0.0.1";
constexpr int WINSIZE_X = 500;
constexpr int WINSIZE_Y = 500;

struct User {
    int id;

    int x;
    int y;
    bool connect;
};

struct KEY {
    int id;
    e_KEY k;
};

int r[10];
int g[10];
int b[10];

// 전역 변수:
HINSTANCE g_hInst;                                // 현재 인스턴스입니다.

LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Client";

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    cout << "input ip" << endl;
    cin >> SERVER_ADDR;


    for (int i = 0; i < 10; ++i) {
        r[i] = rand() % 25 * 10;
        g[i] = rand() % 25 * 10;
        b[i] = rand() % 25 * 10;
    }

    // 전역 문자열을 초기화합니다.
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style =
        CS_HREDRAW 	    // 클라이언트의 너비를 변경하면, 전체 윈도우를 다시 그리게 한다.
        | CS_VREDRAW	// 클라이언트의 높이를 변경하면, 전체 윈도우를 다시 그리게 한다.
    //	| CS_DBLCLKS	// 해당 윈도우에서 더블클릭을 사용해야 한다면 추가해야 한다.
        ;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;

    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = lpszClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    DWORD dwStyle =
        WS_OVERLAPPED 	    // 디폴트 윈도우. 타이틀 바와 크기 조절이 안되는 경계선을 가진다. 아무런 스타일도 주지 않으면 이 스타일이 적용된다.
        | WS_CAPTION 		// 타이틀 바를 가진 윈도우를 만들며 WS_BORDER 스타일을 포함한다.
        | WS_SYSMENU		    // 시스템 메뉴를 가진 윈도우를 만든다.
        | WS_MINIMIZEBOX	    // 최소화 버튼을 만든다.
        | WS_BORDER			// 단선으로 된 경계선을 만들며 크기 조정은 할 수 없다.
        ;					// 추가로 필요한 윈도우 스타일은 http://www.soen.kr/lecture/win32api/reference/Function/CreateWindow.htm 참고.

    HWND hWnd = CreateWindowW(
        lpszClass,
        lpszWindowName,
        dwStyle,
        0,
        0,
        WINSIZE_X,
        WINSIZE_Y,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);


    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc, mdc;
    static HDC BackBuffer;
    static HBITMAP hBitmap, BackBit, BackoBit;
    PAINTSTRUCT ps;
    COLORREF text_color;
    COLORREF bk_color;
    HBRUSH hBrush, oldBrush;
    RECT rcClient;
    static RECT table[8][8];

    static int x, y;
    static bool chessRect = false;
    static bool isConnect = false;
    
    static WSADATA wsa;
    static SOCKET s;
    static SOCKADDR_IN s_a;
    int retval;
    ZeroMemory(&s_a, sizeof(s_a));

    static User userlist[MAX_USER];
    static KEY k;
    static char msgBuffer[MAX_BUFFER];


    switch (message)
    {
    case WM_CREATE: {
        retval = WSAStartup(MAKEWORD(2, 0), &wsa);
        s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
        s_a.sin_family = AF_INET;
        s_a.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_ADDR, &s_a.sin_addr);
        connect(s, (sockaddr*)&s_a, sizeof(s_a));
        recv(s, msgBuffer, sizeof(User), 0);
        User temp = *(User*)msgBuffer;
        userlist[temp.id] = temp;
        k.id = userlist[temp.id].id;
        cout << "id: " << k.id << endl;
        isConnect = true;
        chessRect = true;


        SetTimer(hWnd, 1, 50, NULL); //for update table
        int d = WSAGetLastError();
        break;
    }
    case WM_TIMER: {
        switch (wParam) {
        case 1:
            if (isConnect) {
                send(s, (char*)&k, sizeof(KEY), 0);
                recv(s, (char*)&userlist, sizeof(userlist), 0);
                k.k = e_KEY::K_NULL;
            }
            break;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_LEFT:
            k.k = e_KEY::LEFT;
            break;
        case VK_RIGHT:
            k.k = e_KEY::RIGHT;
            break;
        case VK_DOWN:
            k.k = e_KEY::DOWN;
            break;
        case VK_UP:
            k.k = e_KEY::UP;
            break;
        }
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }
    case WM_KEYUP: {
        k.k = e_KEY::K_NULL;
        break;
    }
    case WM_PAINT: {
        // Server Client
        PAINTSTRUCT ps;
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rcClient);

        BackBuffer = CreateCompatibleDC(hdc);

        BackoBit = CreateBitmap(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, 1, 32, NULL);
        hBitmap = (HBITMAP)SelectObject(BackBuffer, BackoBit);


        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                table[i][j].left = 0 + i * 50;
                table[i][j].right = 50 + i * 50;
                table[i][j].top = 0 + j * 50;
                table[i][j].bottom = 50 + j * 50;
                if (i % 2 == 1 && j % 2 == 0) {
                    hBrush = CreateSolidBrush(RGB(0, 0, 0));
                    oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);

                    Rectangle(BackBuffer, table[i][j].left, table[i][j].top, table[i][j].right, table[i][j].bottom);
                    SelectObject(BackBuffer, oldBrush);
                    DeleteObject(hBrush);
                }

                else if (i % 2 == 0 && j % 2 == 1) {
                    hBrush = CreateSolidBrush(RGB(0, 0, 0));
                    oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);

                    Rectangle(BackBuffer, table[i][j].left, table[i][j].top, table[i][j].right, table[i][j].bottom);
                    SelectObject(BackBuffer, oldBrush);
                    DeleteObject(hBrush);
                }

                else {
                    Rectangle(BackBuffer, table[i][j].left, table[i][j].top, table[i][j].right, table[i][j].bottom);
                }
            }
        }

        // 체스말 Rect
        if (true == chessRect)
        {
            for (int i = 0; i < 10; ++i)
            {
                if (true == userlist[i].connect)
                {
                    //if (k.id == i)   // 자기자신 체스 말
                    //{
                    //    hBrush = CreateSolidBrush(RGB(255, 0, 0));
                    //    oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
                    //    Ellipse(BackBuffer, userlist[i].x, userlist[i].y, userlist[i].x + 50, userlist[i].y + 50);
                    //    SelectObject(BackBuffer, oldBrush);
                    //    DeleteObject(hBrush);

                    //    TCHAR str[10];
                    //    wsprintf(str, TEXT("ME"));
                    //    TextOut(BackBuffer, userlist[i].x + 10, userlist[i].y + 10, str, lstrlen(str));
                    //}
                    //else
                    //{
                    //    // 다른 클라이언트 체스 말

                        hBrush = CreateSolidBrush(RGB(r[i], g[i], b[i]));
                        oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
                        Ellipse(BackBuffer, userlist[i].x, userlist[i].y, userlist[i].x + 50, userlist[i].y + 50);
                        SelectObject(BackBuffer, oldBrush);
                        DeleteObject(hBrush);

                        TCHAR str[10];
                        wsprintf(str, TEXT("%dPlayer"), i + 1);
                        TextOut(BackBuffer, userlist[i].x + 10, userlist[i].y + 10, str, lstrlen(str));
                    //}

                }
            }
        }

        BitBlt(hdc, 0, 0, 400, 400, BackBuffer, 0, 0, SRCCOPY);
        DeleteDC(BackBuffer);
        DeleteObject(hBitmap);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY: {
        SelectObject(BackBuffer, BackoBit);
        DeleteObject(BackBit);
        DeleteDC(BackBuffer);
        closesocket(s);
        WSACleanup();
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
