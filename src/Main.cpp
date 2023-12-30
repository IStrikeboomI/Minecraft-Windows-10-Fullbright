#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>
#include <set>
#include <sstream>
#include <vector>
#include <Psapi.h> 

//Minecraft's process
HANDLE hProcess = {0};
unsigned long long baseAddress;

//These two are used for the window's dimensions
static const int WIDTH = 300, HEIGHT = 200;

//offsets were found through pointer scans
const static int BRIGHTNESS_OFFSET = 0x057302E0;
const static std::vector<unsigned int> offsets = { 0x10, 0x118, 0x8, 0x88, 0x128, 0x1B8, 0x18 };
const static float MAX_BRIGHTNESS = 1000000;


//this method gets the handle for Minecraft
void getMinecraftHandle(std::wstring name) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            //test if both strings are the same
            if (!std::wstring(entry.szExeFile).compare(name)) {
                hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, entry.th32ProcessID);
            }
        }
    }
    //close out of snapshot to prevent a leak
    CloseHandle(snapshot);
    
    HMODULE modules[1024];
    unsigned long cbNeeded;
    if (EnumProcessModules(hProcess, modules, sizeof(modules), &cbNeeded)) {
        baseAddress = (DWORDLONG)modules[0];
    }
}
void setToMaxBrightness() {
    unsigned long long ptr = baseAddress + 0x057302E0;
    for (unsigned int i = 0; i < offsets.size(); ++i)
    {
        ReadProcessMemory(hProcess, (BYTE*)ptr, &ptr, sizeof(ptr), 0);
        ptr += offsets[i];
    }
    WriteProcessMemory(hProcess, (void*)ptr, &MAX_BRIGHTNESS, sizeof(MAX_BRIGHTNESS), nullptr);
}
//Everything below here is for the GUI

//This just matches the text to the system font
void changeFont(const HWND& hwnd) {
    NONCLIENTMETRICS metrics = {};
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
    HFONT font = CreateFontIndirect(&metrics.lfCaptionFont);
    SendMessage(hwnd, WM_SETFONT, (WPARAM)font, true);
}
//The next functions are basically the same where it creates objects like buttons and labels (text)
void createFoundObjects(const HWND& hwnd) {
    HWND foundLabel = CreateWindowW(L"static", L"Enjoy Fullbright!", WS_VISIBLE | WS_CHILD, 100, 10, 100, 50, hwnd, nullptr, nullptr, nullptr);
    HWND closeWindowLabel = CreateWindowW(L"static", L"You can freely close out this window", WS_VISIBLE | WS_CHILD, 45, 35, 200, 20, hwnd, nullptr, nullptr, nullptr);

    changeFont(foundLabel);
    changeFont(closeWindowLabel);
}
void createCantFindObjects(const HWND& hwnd) {
    HWND cantFindLabel = CreateWindowW(L"static", L"Can't find Minecraft", WS_VISIBLE | WS_CHILD, 80, 10, 140, 20, hwnd, nullptr, nullptr, nullptr);
    HWND restartLabel = CreateWindowW(L"static", L"Restart both programs and try again", WS_VISIBLE | WS_CHILD, 45, 35, 190, 20, hwnd, nullptr, nullptr, nullptr);

    changeFont(cantFindLabel);
    changeFont(restartLabel);
}
LRESULT CALLBACK windowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_DESTROY:
        {
            CloseHandle(hProcess);
            //default destroy message
            PostQuitMessage(0);
            break;
        }
        //called on start
        case WM_CREATE:
        {
            //get the process for Minecraft
            getMinecraftHandle(L"Minecraft.Windows.exe");

            //if the process doesn't exist then show user corresponding message
            if (hProcess) {
                createFoundObjects(hwnd);
                setToMaxBrightness();
            } else {
                createCantFindObjects(hwnd);
            }
            break;
        }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
int main() {
    //initizalie wc
    WNDCLASSW wc = { 0 };
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"fullbright";
    wc.lpfnWndProc = windowProcedure;
    wc.lpszMenuName = L"Full Bright";
    //Adds icon in corner (117 is icon)
    wc.hIcon = (HICON)LoadImageW(wc.hInstance, MAKEINTRESOURCEW(117), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    if (!RegisterClassW(&wc)) {
        return -1;
    }

    //Gets screen to center window
    RECT screen;
    GetWindowRect(GetDesktopWindow(), &screen);

    CreateWindowW(wc.lpszClassName, wc.lpszMenuName, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE, screen.right / 2 - WIDTH / 2, screen.bottom / 2 - HEIGHT / 2, WIDTH, HEIGHT, nullptr, nullptr, wc.hInstance, nullptr);
        
    MSG msg = { nullptr };

    //loop over messages
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}