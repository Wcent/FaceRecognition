// Face Recognition.cpp : Defines the entry point for the application.
// Copyright: cent
// 2015.9.17
// ~

#include "stdafx.h"
#include "resource.h"

/*****************************************************************************************/
#include <commdlg.h> // common dialogs
#include <vfw.h> // vedio for window library
#pragma comment(lib, "vfw32.lib") // 解决link时project加入lib问题
#include "Image.h"
#include "FaceDetect.h"
#include "FaceRecognize.h"
/*****************************************************************************************/

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text

/*****************************************************************************************
我的全局变量定义

*****************************************************************************************/
HWND hMainWnd; // handle of main window
HWND hwndCap; // handle of capture window
CAPDRIVERCAPS capDrvCaps; // driver capabilities
bool isRecognition = false; // flag set if is recognizing face
bool isThreadEnd = true;
char curDir[256];   


// face recognition thread procedure
DWORD WINAPI RecognitionThreadProc(LPVOID lParam);
//声明frame callback函数，frame available调用
LRESULT PASCAL FrameCallbackProc(HWND hwnd, LPVIDEOHDR lpVHdr);
//选择目标图像进行人脸识别
void ChooseImageToRecognizeFace();


// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FACERECOGNITION, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_FACERECOGNITION);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_FACERECOGNITION);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_FACERECOGNITION;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	
	// 自定义变量
	char path[256];

	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_FACERECOGNITION:
			isRecognition = true;

			strcpy(path, curDir);
			strcat(path, "\\FaceSample.bmp");
			// 得到人脸肤色Cb，Cr对比库cbcr[cb][cr]
			if ( !FaceCbcrProc(path) )
				break;

			// Register a frame callback function with the capture window
			capSetCallbackOnFrame(hwndCap, FrameCallbackProc);
			break;

		case IDM_ENTERINGFACE:
			isRecognition = false;
			strcpy(path, curDir);
			strcat(path, "\\FaceSample.bmp");
			// 得到人脸肤色Cb，Cr对比库cbcr[cb][cr]
			if ( !FaceCbcrProc(path) )
				break;

			// Register a frame callback function with the capture window
			capSetCallbackOnFrame(hwndCap, FrameCallbackProc);
			break;

		case IDM_DELETE:
			strcpy(path, curDir);
			strcat(path, "\\FaceSample.bmp");
			// 得到人脸肤色Cb，Cr对比库cbcr[cb][cr]
			if ( !FaceCbcrProc(path) )
				break;

			// 构造人脸库路径
			strcpy(path, curDir);
			strcat(path, "\\Facebase");
			
			DeleteFace(path);
			break;

		case IDM_CHOOSE:
			ChooseImageToRecognizeFace();
			break;

		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
		
	case WM_CREATE:
		hWinDC = GetDC(hWnd);
		hMainWnd = hWnd;

		// 得到当前目录
		GetCurrentDirectory(256, curDir);

		// 创建capture窗口，捕获视频
		hwndCap = capCreateCaptureWindow(
								(LPSTR)"My Capture Window",
								WS_CHILD | WS_VISIBLE,
								0, 0, 640, 480,
								(HWND)hWnd,
								(int)1);

		// Register a frame callback function with the capture window
	//	capSetCallbackOnFrame(hwndCap, FrameCallbackProc);
			
		// connect to the driver 0(NO.1)
		SendMessage(hwndCap, WM_CAP_DRIVER_CONNECT, 0, 0L);
		// update the driver capabilities
		SendMessage(hwndCap, WM_CAP_DRIVER_GET_CAPS,
							sizeof(CAPDRIVERCAPS), (LONG)(LPVOID)&capDrvCaps);
		// set preview rate to 66 milliseconds
		capPreviewRate(hwndCap, 66);
		// start preview vedio
		capPreview(hwndCap, TRUE);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...
			EndPaint(hWnd, &ps);
			break;

	case WM_DESTROY:
			// disable frame callback function
			capSetCallbackOnFrame(hwndCap, NULL);
			// end preview
			capPreview(hwndCap, FALSE);
			// disconnect from driver
			capDriverDisconnect(hwndCap);
			PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

/*************************************************************************************************
	显示捕获样本人脸处理状态

*************************************************************************************************/
void ShowCaptureState()
{
	static int countFrame=0;
	countFrame++;
	if ( !isRecognition )
	{
		if ( countFrame < 5 )
			TextOut(hWinDC, 750, 150, "正在捕获人脸     ", strlen("正在捕获人脸     "));
		else if ( countFrame < 10 )
			TextOut(hWinDC, 750, 150, "正在捕获人脸.    ", strlen("正在捕获人脸.    "));
		else if ( countFrame < 15 )
			TextOut(hWinDC, 750, 150, "正在捕获人脸..   ", strlen("正在捕获人脸..   "));
		else if ( countFrame < 20 )
			TextOut(hWinDC, 750, 150, "正在捕获人脸...  ", strlen("正在捕获人脸...  "));
		else if ( countFrame < 25 )
			TextOut(hWinDC, 750, 150, "正在捕获人脸.... ", strlen("正在捕获人脸.... "));
		else if ( countFrame < 30 )
			TextOut(hWinDC, 750, 150, "正在捕获人脸.....", strlen("正在捕获人脸....."));
		else
			countFrame = 0;
	}
}

/************************************************************************************************
	callback procedure: FrameCallbackProc
						frame callback function
				  hwnd: capture window handle
				lpVHdr: pointer to structure containing captured frame information

************************************************************************************************/
LRESULT PASCAL FrameCallbackProc(HWND hwnd, LPVIDEOHDR lpVHdr)
{
	char imagePath[256];
	HANDLE hRecognitionThread; //handle of face recognition thread

	if ( !hwnd )
		return FALSE;

	// 输出捕获处理状态
	if ( !isRecognition )
		ShowCaptureState();

// 旧线程结束标记后，再新开线程新处理
	if ( isThreadEnd )
	{
		strcpy(imagePath, curDir);
		strcat(imagePath, "\\FaceImage.bmp");
		// 保存当前帧图像到ImagePath的bmp文件中
		capFileSaveDIB(hwnd, imagePath);

		// disable frame callback function to end entering face
		//	capSetCallbackOnFrame(hwndCap, NULL);

		// create the video capture thread
		DWORD id;
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;
		
		hRecognitionThread = CreateThread(&sa, (ULONG)0,
									RecognitionThreadProc, //线程函数
									(LPVOID)(ULONG)0, (ULONG)0, &id);
		if ( hRecognitionThread == NULL )
			MessageBox(hwnd, "Creation of face recognition thread failed!",
						"Thread", MB_OK | MB_ICONEXCLAMATION);
	}
	
	return (LRESULT)TRUE;
}

/************************************************************************************************
	Face Recognition Thread 线程处理函数

************************************************************************************************/
DWORD WINAPI RecognitionThreadProc(LPVOID lParam)
{
	char imgFileName[256];
	char facebasePath[256];
	BmpImage *image, *faceImage;

	strcpy(imgFileName, curDir);
	strcat(imgFileName, "\\FaceImage.bmp");

	isThreadEnd = false;
	image = ReadBmpFile(imgFileName);
	if ( image == NULL )
		// Register a frame callback function with the capture window
		capSetCallbackOnFrame(hwndCap, FrameCallbackProc);
//	ShowBmpImage(Image, 665, 20);

	if ( isRecognition )
	{
		strcpy(facebasePath, curDir);
		strcat(facebasePath, "\\Facebase");
		
		// 人脸库中查找目标人脸
		if ( RecognizeFace(image, facebasePath) )
		{
			// disable frame callback function to end entering face
			capSetCallbackOnFrame(hwndCap, NULL);
			MessageBox(hMainWnd, "匹配成功！", "人脸识别", 0);
		}
		else if ( MessageBox(hMainWnd, "匹配失败！快去录入人脸样本，或继续识别几次？", "人脸识别", MB_YESNO) == IDYES )
			// Register a frame callback function with the capture window
			capSetCallbackOnFrame(hwndCap, FrameCallbackProc);
		else // disable frame callback function to end entering face
			capSetCallbackOnFrame(hwndCap, NULL);

		// Update Show Rect()
		RECT rt;
		rt.left = 660, rt.top = 20;
		rt.right = 1100, rt.bottom = 500;
		InvalidateRect(hMainWnd, &rt, true);
	}
	else // if ( is Recognition == false )
	{
		faceImage = ExtractFace(image);
		if ( faceImage == NULL )
		{
			// Register a frame callback function with the capture window
			capSetCallbackOnFrame(hwndCap, FrameCallbackProc);
			isThreadEnd = true;
			FreeBmpImage(image);
			return 0;
		}
		ShowBmpImage(faceImage, 770, 200);

		int rtn = MessageBox(hMainWnd, "是，则录入人脸样本？否，则捕获新人脸", "Entering Face", MB_YESNOCANCEL);
		if (  rtn == IDNO )
			// Register a frame callback function with the capture window
			capSetCallbackOnFrame(hwndCap, FrameCallbackProc);
		else if ( rtn == IDYES )
		{
			// disable frame callback function to end entering face
			capSetCallbackOnFrame(hwndCap, NULL);

			// 人脸样本库目录
			strcpy(facebasePath, curDir);
			strcat(facebasePath, "\\Facebase");
			
			// 人脸图像保存入库
			if ( EnterFace(imgFileName, facebasePath) )
				MessageBox(hMainWnd, "录入成功！", "Entering Face", 0);
		}
		else // disable frame callback function to end entering face
			capSetCallbackOnFrame(hwndCap, NULL);
	
		// Update Show Rect()
		RECT rt;
		rt.left = 750, rt.top = 150;
		rt.right = 900, rt.bottom = 270;
		InvalidateRect(hMainWnd, &rt, true);	
	}

	FreeBmpImage(image);
	if( !isRecognition )
		FreeBmpImage(faceImage);
	isThreadEnd = true;
//	MessageBox(NULL, "Leaving Face Recognition Thread", "Thread", NULL);
	return 0;
}


/*************************************************************************************************
	根据选择的目标图像，进行人脸识别

*************************************************************************************************/
void ChooseImageToRecognizeFace()
{
	char imgFileName[256];
	char path[256];
	BmpImage *image;
	
	//隐藏视频捕获窗口
	ShowWindow(hwndCap, SW_HIDE);
	//设置帧图像捕获回调函数为空
	capSetCallbackOnFrame(hwndCap, NULL);
	
	strcpy(path, curDir);
	strcat(path, "\\FaceSample.bmp");
	// 得到人脸肤色Cb，Cr对比库cbcr[cb][cr]
	if ( !FaceCbcrProc(path) )
		goto STOP_RECOGNIZING;
	
	OpenImageFile("选择搜索目标人脸图像", imgFileName);
	if ( !strlen(imgFileName) || (image=ReadBmpFile(imgFileName)) == NULL )
		goto STOP_RECOGNIZING;
	ShowBmpImage(image, 300, 200);

	strcpy(path, curDir);
	strcat(path, "\\Facebase");
	
	// 在人脸库中查找目标人脸
	if ( RecognizeFace(image, path) )
		MessageBox(hMainWnd, "匹配成功！", "人脸识别", 0);
	else 
		MessageBox(hMainWnd, "匹配失败！", "人脸识别", 0);
		
	// 释放ReadBmpFile动态生成的位图结构
	FreeBmpImage(image);

STOP_RECOGNIZING:
	// Update Show window rect
	InvalidateRect(hMainWnd, NULL, true);
	ShowWindow(hwndCap, SW_SHOW);
}


// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}
