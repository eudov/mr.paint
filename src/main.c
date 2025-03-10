#include <windows.h>
#include <stdio.h>
#include "lodepng.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static TCHAR szAppName[] = TEXT("Mr.Paint");
    HWND hwnd;
    MSG msg;
    WNDCLASS wndclass;

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass(&wndclass))
    {
        MessageBox(NULL, TEXT("This program Requires Windows NT!"), szAppName, MB_ICONERROR);
        return 0;
    }

    hwnd = CreateWindow(
        szAppName,
        TEXT("Mr.Paint"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

void GetLargestDisplayMode(int *pcxBitmap, int *pcyBitmap)
{
    DEVMODE devmode;
    int iModeNum = 0;

    *pcxBitmap = *pcyBitmap = 0;

    ZeroMemory(&devmode, sizeof(DEVMODE));
    devmode.dmSize = sizeof(DEVMODE);

    while (EnumDisplaySettings(NULL, iModeNum++, &devmode))
    {
        *pcxBitmap = max(*pcxBitmap, (int)devmode.dmPelsWidth);
        *pcyBitmap = max(*pcyBitmap, (int)devmode.dmPelsHeight);
    }
}

void SaveBmpAsPng(HBITMAP hBitmap, const char *fileName)
{
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;

    // Create a BITMAPINFO structure
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Negative to store in top-down format
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    // Allocate buffer for pixel data
    BYTE *pixels = (BYTE *)malloc(4 * width * height);
    if (!pixels)
        return;

    // Create a compatible DC
    HDC hdcMem = CreateCompatibleDC(NULL);
    SelectObject(hdcMem, hBitmap);

    // Retrieve pixel data
    if (!GetDIBits(hdcMem, hBitmap, 0, height, pixels, (BITMAPINFO *)&bi, DIB_RGB_COLORS))
    {
        printf("GetDIBits failed\n");
        free(pixels);
        DeleteDC(hdcMem);
        return;
    }

    // Convert from BGRA to RGBA (swap red and blue channels)
    // for (int i = 0; i < width * height; ++i)
    // {
    //     BYTE *pixel = pixels + i * 4;
    //     BYTE temp = pixel[0]; // B
    //     pixel[0] = pixel[2];  // Swap B and R
    //     pixel[2] = temp;
    // }

    // Save using LodePNG
    unsigned error = lodepng_encode32_file(fileName, pixels, width, height);
    if (error)
    {
        printf("Error saving PNG: %s\n", lodepng_error_text(error));
    }
    else
    {
        MessageBox(NULL, TEXT("Bitmap saved as PNG!"), TEXT("Save PNG"), MB_OK);
    }

    // Free resources
    free(pixels);
    DeleteDC(hdcMem);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static BOOL fLeftButtonDown, fRightButtonDown;
    static HBITMAP hBitmap;
    static HDC hdcMem;
    static int cxBitmap, cyBitmap, cxClient, cyClient, xMouse, yMouse;
    HDC hdc;
    PAINTSTRUCT ps;

    // HPEN thick = CreatePen(PS_SOLID, 10, BLACK_BRUSH);

    switch (message)
    {
    case WM_CREATE:
        PlaySound(TEXT("windowssaturated.wav"), NULL, SND_FILENAME | SND_ASYNC);
        GetLargestDisplayMode(&cxBitmap, &cyBitmap);
        hdc = GetDC(hwnd);
        hBitmap = CreateCompatibleBitmap(hdc, cxBitmap, cyBitmap);
        hdcMem = CreateCompatibleDC(hdc);
        ReleaseDC(hwnd, hdc);

        if (!hBitmap)
        {
            DeleteDC(hdcMem);
            return -1;
        }

        SelectObject(hdcMem, hBitmap);
        PatBlt(hdcMem, 0, 0, cxBitmap, cyBitmap, WHITENESS);
        return 0;

    case WM_SIZE:
        cxClient = LOWORD(lParam);
        cyClient = HIWORD(lParam);
        return 0;

    case WM_LBUTTONDOWN:
        if (!fRightButtonDown)
            SetCapture(hwnd);
        xMouse = LOWORD(lParam);
        yMouse = HIWORD(lParam);
        fLeftButtonDown = TRUE;
        return 0;

    case WM_LBUTTONUP:
        if (fLeftButtonDown)
            SetCapture(NULL);

        fLeftButtonDown = FALSE;
        return 0;

    case WM_RBUTTONDOWN:
        if (!fLeftButtonDown)
            SetCapture(hwnd);

        xMouse = LOWORD(lParam);
        yMouse = HIWORD(lParam);
        fRightButtonDown = TRUE;
        return 0;

    case WM_RBUTTONUP:
        if (fRightButtonDown)
            SetCapture(NULL);

        fRightButtonDown = FALSE;
        return 0;

    case WM_MOUSEMOVE:
        if (!fLeftButtonDown && !fRightButtonDown)
            return 0;

        hdc = GetDC(hwnd);

        SelectObject(hdc, GetStockObject(fLeftButtonDown ? BLACK_PEN : WHITE_PEN));
        SelectObject(hdcMem, GetStockObject(fLeftButtonDown ? BLACK_PEN : WHITE_PEN));

        MoveToEx(hdc, xMouse, yMouse, NULL);
        MoveToEx(hdcMem, xMouse, yMouse, NULL);

        xMouse = (short)LOWORD(lParam);
        yMouse = (short)HIWORD(lParam);

        LineTo(hdc, xMouse, yMouse);
        LineTo(hdcMem, xMouse, yMouse);

        ReleaseDC(hwnd, hdc);
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        BitBlt(hdc, 0, 0, cxClient, cyClient, hdcMem, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;

    case WM_KEYDOWN:
        if (wParam == 'S' && GetKeyState(VK_CONTROL) & 0x8000)
        {
            // Save the bitmap as a PNG when Ctrl+S is pressed
            SaveBmpAsPng(hBitmap, "output.png");
            // MessageBox(hwnd, TEXT("Bitmap saved as PNG!"), TEXT("Save PNG"), MB_OK);
        }
        break;

    case WM_DESTROY:
        DeleteDC(hdcMem);
        DeleteObject(hBitmap);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}