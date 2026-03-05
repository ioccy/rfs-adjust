#include "framework.h"
#include "resource.h"
#include "utils.h"
#include "fecomm.cpp"

#define APP_HEADER  L"RFS Adjust"
#define APP_VERSION L"v1.0 (" __DATE__ L")"
const std::wstring APP_LINK = L"https://github.com/ioccy/vlp-2000";

HINSTANCE hInst;
DWORD currentOffset;                    // Current device frequency offset
DeviceConnect* rubidium;                // Serial connect session with device
HFONT hFontBase, hFontBold, hFontHex;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);                                // Main window: handles top menu and DPI change
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);                                  // Dialog: About logic
INT_PTR CALLBACK MainDlg(HWND, UINT, WPARAM, LPARAM);                                // Dialog: Main logic
INT_PTR CALLBACK Terminal(HWND, UINT, WPARAM, LPARAM);                               // Dialog: Terminal logic
LRESULT CALLBACK AdjustInput(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);       // Formatter: Hex offset adjustment
LRESULT CALLBACK TerminalInput(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);     // Formatter: Terminal input
LRESULT CALLBACK TerminalOutput(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);    // Popup menu: Terminal output

void RedrawBold(HWND, int, bool);       // Make Hz text bold or normal
void RedrawOffset(HWND, DWORD);         // Update Hz value
void PutToClipboard(HWND, std::string); // Save string to clipboard
void AddOutputLine(HWND, std::string);  // Add a line to terminal output box

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,_In_ LPWSTR    lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    hInst = hInstance;

    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MAINMENU);
    wcex.lpszClassName = L"mainClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"mainClass", APP_HEADER,
        WS_OVERLAPPEDWINDOW - WS_MINIMIZEBOX - WS_MAXIMIZEBOX - WS_THICKFRAME,
        0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;

    UINT dpi = GetDpiForWindow(hWnd);
    int windowW = MulDiv(170, dpi, 96);
    int windowH = MulDiv(280, dpi, 96);
    int windowX = (GetSystemMetrics(SM_CXSCREEN) - windowW) / 2;
    int windowY = (GetSystemMetrics(SM_CYSCREEN) - windowH) / 2;

    SetWindowPos(hWnd, nullptr, windowX, windowY, windowW, windowH, SWP_NOZORDER);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAINMENU));
    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

        case WM_CREATE: {
                HWND myDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN), hWnd, MainDlg);
                ShowWindow(myDialog, SW_SHOW);
            }
            break;

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);

            switch (wmId) {
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;

                case IDM_RESET:
                    currentOffset = 0;
                    RedrawOffset(GetWindow(hWnd, GW_CHILD), currentOffset);
                    RedrawBold(GetWindow(hWnd, GW_CHILD), IDC_CURRENT, FALSE);
                    break;

                case IDM_TERMINAL:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_TERMINAL), hWnd, Terminal);
                    break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;

                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
            break;

        case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                // TODO: Add any drawing code that uses hdc here...
                EndPaint(hWnd, &ps);
            }
            break;

        case WM_DESTROY:
            if (rubidium) delete rubidium;
            DeleteObject(hFontBold); DeleteObject(hFontHex);
            PostQuitMessage(0);
            break;

        case WM_DPICHANGED: {
            RECT* suggested = (RECT*)lParam;
            SetWindowPos(hWnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER);
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {

        case WM_INITDIALOG: {
            std::wstring linkText;
            linkText = L"Github: <a href=\"" + APP_LINK + L"\">" + APP_LINK + L"</a>";
            SetDlgItemText(hDlg, IDC_LINK, linkText.data());
            SetDlgItemText(hDlg, IDC_ABT_NAME, APP_HEADER);
            SetDlgItemText(hDlg, IDC_ABT_VER, APP_VERSION);
            return (INT_PTR)TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;

        case WM_NOTIFY:
            NMHDR* pHdr = (NMHDR*)lParam;
            if (pHdr->code == NM_CLICK && pHdr->idFrom == IDC_LINK) {
                NMLINK* pLink = (NMLINK*)lParam;
                ShellExecute(nullptr, L"open", pLink->item.szUrl, nullptr, nullptr, SW_SHOW);
            }
            break;
        }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Terminal(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {

        case WM_INITDIALOG: {
            SetWindowSubclass(GetDlgItem(hDlg, IDC_TER_INPUT), TerminalInput, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_TER_OUTPUT), TerminalOutput, 0, 0);

            SendMessage(GetDlgItem(hDlg, IDC_TER_OUTPUT), WM_SETFONT, (WPARAM)hFontHex, TRUE);
            SendMessage(GetDlgItem(hDlg, IDC_TER_INPUT), WM_SETFONT, (WPARAM)hFontHex, TRUE);

            RedrawBold(hDlg, IDC_STATIC, TRUE);

            return (INT_PTR)FALSE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_TER_CONSENT && HIWORD(wParam) == BN_CLICKED) {
                if (IsDlgButtonChecked(hDlg, IDC_TER_CONSENT) == BST_CHECKED) {
                    EnableWindow(GetDlgItem(hDlg, IDC_TER_INPUT), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_TER_SEND), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_TER_CONSENT), FALSE);
                    SetFocus(GetDlgItem(hDlg, IDC_TER_INPUT));
                }
            }
            if (LOWORD(wParam) == IDC_TER_SEND) {
                std::string buffer(MAXWORD, '\0');
                buffer.resize(GetDlgItemTextA(hDlg, IDC_TER_INPUT, buffer.data(), buffer.size()));
                SetDlgItemTextA(hDlg, IDC_TER_INPUT, "");

                // Command length < 2 or data length is odd
                if (buffer.size() < 2 || (buffer.size() > 3 && buffer.size() % 2 != 1)) {
                    AddOutputLine(hDlg, "[Bad command]");
                    break;
                }

                BYTE commandPart = std::stoi(buffer.substr(0, 2), NULL, 16);
                std::string dataPart = buffer.size() > 3 ? HexToBytes(buffer.substr(3)) : "";

                DeviceCommand command(commandPart, dataPart);
                DeviceCommand response = rubidium->CustomCommand(command);

                std::string dump = buffer;
                dump += " -> " + (response.commandData.size() > 0 ? BytesToHex(response.commandData) : "[OK]");

                AddOutputLine(hDlg, dump);
                SetFocus(GetDlgItem(hDlg, IDC_TER_INPUT));
            }
            break;

        case WM_NOTIFY:
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return TRUE;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK TerminalOutput(HWND hList, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    if (msg == WM_CONTEXTMENU) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, IDM_TER_COPY, L"Copy");
        AppendMenu(hMenu, MF_STRING, IDM_TER_CLEAR, L"Clear");
        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, (short)LOWORD(lParam), (short)HIWORD(lParam), 0, hList, NULL);
        DestroyMenu(hMenu);

        switch (cmd) {
            case IDM_TER_COPY: {
                int count = SendMessage(hList, LB_GETCOUNT, 0, 0);
                std::string combined;
                for (int i = 0; i < count; i++) {
                    int len = SendMessage(hList, LB_GETTEXTLEN, i, 0);
                    std::string line(len + 1, '\0');
                    SendMessageA(hList, LB_GETTEXT, i, (LPARAM)line.data());
                    line.resize(len);
                    combined += line + "\r\n";
                }

                PutToClipboard(hList, combined);
                break;
            }

            case IDM_TER_CLEAR:
                SendMessage(hList, LB_RESETCONTENT, 0, 0);
                SendMessage(hList, LB_SETHORIZONTALEXTENT, 0, 0);
                break;
        }
        return 0;
    }
    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) return 0;

    return DefSubclassProc(hList, msg, wParam, lParam);
}

LRESULT CALLBACK TerminalInput(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    DWORD pos;
    if (msg == WM_CHAR) {
        char ch = toupper((char)wParam);

        if (ch == VK_BACK) {
            SendMessage(hEdit, EM_GETSEL, (WPARAM)&pos, NULL);

            if (pos == 0) return 0;

            if (pos == 3) {
                SendMessage(hEdit, EM_SETSEL, 1, 3);
                SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)"");
                return 0;
            }

            if (pos == 4 && GetWindowTextLengthA(hEdit) == 4) {
                SendMessage(hEdit, EM_SETSEL, 2, 4);
                SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)"");
                return 0;
            }

            return DefSubclassProc(hEdit, msg, wParam, lParam);
        }

        if ((ch < '0' || ch > '9') && (ch < 'A' || ch > 'F')) return 0;

        SendMessage(hEdit, EM_GETSEL, (WPARAM)&pos, NULL);

        if (pos == 2) {
            SendMessage(hEdit, EM_SETSEL, 2, 2);
            SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)":");
        }

        return DefSubclassProc(hEdit, msg, ch, lParam);
    }

    if (msg == WM_PASTE) return 0;

    return DefSubclassProc(hEdit, msg, wParam, lParam);
}

LRESULT CALLBACK AdjustInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    
    if (msg == WM_RBUTTONDOWN) return 0;                    // block context menu
    if (msg == WM_KEYDOWN && wParam == VK_DELETE) return 0; // block del
    if (msg == WM_CHAR) {

        if (GetKeyState(VK_CONTROL) & 0x8000) return 0;     // block ctrl shortcuts
        if (wParam == VK_BACK) return 0;                    // block backspace

        if (wParam >= 32) {            
            DWORD pos;
            char c = toupper((char)wParam);
            char s[2] = { c, 0 };

            if (!isxdigit(c)) return 0;                     // block non-hex

            SendMessage(hWnd, EM_GETSEL, (WPARAM)&pos, NULL);
            
            if (pos % 3 == 2) pos++; // skip over space
            if (pos >= 11) return 0; // block over 11 letters

            // select character at pos to replace it
            SendMessage(hWnd, EM_SETSEL, pos, pos + 1);
            SendMessageA(hWnd, EM_REPLACESEL, FALSE, (LPARAM)s);

            pos++; if (pos % 3 == 2) pos++; // move to next hex pos, skip space
            SendMessage(hWnd, EM_SETSEL, pos, pos);

            std::string hexString(12, '\0');
            GetWindowTextA(hWnd, hexString.data(), 12);
            currentOffset = HexToDword(hexString);
            SetDlgItemTextA(GetParent(hWnd), IDC_CURRENT, FormatFrequency(DwordToFreq(currentOffset)).c_str());
            RedrawBold(GetParent(hWnd), IDC_CURRENT, FALSE);

            return 0;
        }
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

INT_PTR CALLBACK MainDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {

        case WM_INITDIALOG: {
            char targetPath[50];
            char comPort[5] = "COM ";
            for (int i = 1; i < 10; i++) {
                comPort[3] = (i + '0');
                if (QueryDosDeviceA(comPort, targetPath, 50))
                    SendMessageA(GetDlgItem(hDlg, IDC_PORT), CB_ADDSTRING, 0, (LPARAM)comPort);
            }

            // Normal font
            hFontBase = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_CURRENT), WM_GETFONT, 0, 0);
        
            LOGFONT lf{};
            GetObject(hFontBase, sizeof(lf), &lf);

            // Bold font
            lf.lfWeight = FW_BOLD;
            hFontBold = CreateFontIndirect(&lf);

            // Monospace font
            lf.lfHeight = MulDiv(14, GetDpiForWindow(hDlg), 96);
            lf.lfWeight = FW_NORMAL;
            lstrcpy(lf.lfFaceName, L"Consolas");
            hFontHex = CreateFontIndirect(&lf);

            SendMessage(GetDlgItem(hDlg, IDC_EDIT), WM_SETFONT, (WPARAM)hFontHex, TRUE);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_EDIT), AdjustInput, 0, 0);

            return (INT_PTR)TRUE;
        }   

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_APPLY:
                    if (rubidium) {
                        rubidium->FESet(currentOffset);
                        Sleep(500); // The device needs to time to change the offset
                        currentOffset = rubidium->FEPoll();
                        RedrawOffset(hDlg, currentOffset);
                        RedrawBold(hDlg, IDC_CURRENT, TRUE);
                    }
                    break;

                case IDC_SAVE:
                    if (rubidium) {
                        rubidium->FESave(currentOffset);
                        Sleep(500); // The device needs to time to change the offset
                        currentOffset = rubidium->FEPoll();
                        RedrawOffset(hDlg, currentOffset);
                        RedrawBold(hDlg, IDC_CURRENT, TRUE);
                    }
                    break;

                case IDC_PORT:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        char comPort[256];

                        HWND hCombo = GetDlgItem(hDlg, IDC_PORT);
                        int selectedIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                        SendMessageA(hCombo, CB_GETLBTEXT, selectedIndex, (LPARAM)comPort);

                        try { rubidium = new DeviceConnect(comPort); }
                        catch (const std::exception& e) { delete rubidium; rubidium = NULL; SetDlgItemTextA(hDlg, IDC_CURRENT, e.what()); break; }

                        try { currentOffset = rubidium->FEPoll(); }
                        catch (const std::exception& e) { delete rubidium; rubidium = NULL; SetDlgItemTextA(hDlg, IDC_CURRENT, e.what()); break; }

                        RedrawOffset(hDlg, currentOffset);
                        RedrawBold(hDlg, IDC_CURRENT, TRUE);

                        // Enable all controls in the dialog, disable COM port selection
                        EnumChildWindows(hDlg,
                            [](HWND hChild, LPARAM enable) -> BOOL {
                                EnableWindow(hChild, (BOOL)enable);
                                return TRUE;
                            }, TRUE);
                        EnableWindow(hCombo, FALSE);
                        EnableMenuItem(GetMenu(GetParent(hDlg)), IDM_RESET, MF_ENABLED);
                        EnableMenuItem(GetMenu(GetParent(hDlg)), IDM_TERMINAL, MF_ENABLED);
                    }
                    break;

                case IDC_DN0: case IDC_DN1: case IDC_DN2: case IDC_DN3:
                case IDC_UP0: case IDC_UP1: case IDC_UP2: case IDC_UP3:

                    static const struct { int id; DWORD value; } buttons[] = {
                        { IDC_DN0, 1 }, { IDC_DN1, 1 << 8 }, { IDC_DN2, 1 << 16 }, { IDC_DN3, 1 << 24 },
                        { IDC_UP0, 1 }, { IDC_UP1, 1 << 8 }, { IDC_UP2, 1 << 16 }, { IDC_UP3, 1 << 24 }
                    };

                    for (auto& btn : buttons) {
                        if (btn.id == LOWORD(wParam)) {
                            currentOffset += LOWORD(wParam) >= IDC_DN0 ? -(signed int)btn.value : btn.value;
                            break;
                        }
                    }

                    RedrawOffset(hDlg, currentOffset);
                    RedrawBold(hDlg, IDC_CURRENT, FALSE);

                    break;
            }
            break;
    }
    return (INT_PTR)FALSE;
}

void RedrawOffset(HWND hDlg, DWORD offset) {
    SetDlgItemTextA(hDlg, IDC_EDIT, DwordToHex(offset).c_str());
    SetDlgItemTextA(hDlg, IDC_CURRENT, FormatFrequency(DwordToFreq(offset)).c_str());
}

void RedrawBold(HWND hDlg, int control, bool bold) {
    SendMessage(GetDlgItem(hDlg, control), WM_SETFONT, (WPARAM)(bold ? hFontBold : hFontBase), TRUE);
}

void PutToClipboard(HWND hWnd, std::string str) {
    if (OpenClipboard(hWnd)) {
        EmptyClipboard();
        size_t bytes = (str.size() + 1) * sizeof(char);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            void* ptr = GlobalLock(hMem);
            if (ptr) {
                memcpy(ptr, str.c_str(), bytes);
                GlobalUnlock(hMem);

                // On success, clipboard owns the handle; don't free it.
                if (SetClipboardData(CF_TEXT, hMem)) hMem = NULL;
            } else {
                GlobalFree(hMem);
                hMem = NULL;
            }
        }
        CloseClipboard();
        if (hMem) GlobalFree(hMem);
    }
}

void AddOutputLine(HWND hDlg, std::string str) {
    HWND hList = GetDlgItem(hDlg, IDC_TER_OUTPUT);
    SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)str.c_str());

    // Enable horizontal scroll if needed
    HDC hdc = GetDC(hList);
    SIZE textSize;
    GetTextExtentPoint32A(hdc, str.c_str(), str.size(), &textSize);
    ReleaseDC(hList, hdc);

    int listBoxWidth = SendMessage(hList, LB_GETHORIZONTALEXTENT, 0, 0);
    if (textSize.cx > listBoxWidth) SendMessage(hList, LB_SETHORIZONTALEXTENT, textSize.cx, 0);
}