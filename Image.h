// Image.h：位图结构定义，及位图相关处理函数声明
// Copyright: cent
// 2015.9.17
// ~

#ifndef IMAGE_H
#define IMAGE_H



extern HWND hMainWnd; // handle of main window
extern HDC hWinDC;


// 24位真彩色位图结构定义
// data：动态分配内存存放BMP真实像素RBG数据
// width & height：存放BMP真实尺寸
typedef struct _BmpImage
{
	char *data;
	int width;
	int height;
} BmpImage;


// 动态分配目标位图内存函数
BmpImage * MallocBmpImage(int width, int height);

// 回收动态分配位图内存函数
void FreeBmpImage(BmpImage *pImage);

// 获取选择目标文件名
void OpenImageFile(char *OPDLTitle, char *pImgFileName);

// 读取bmp图像文件
BOOL ReadBmpFile(LPSTR imgFileName, BmpImage *oImage);

// 显示bmp图像
void ShowBmpImage(BmpImage *pImage, int xPos, int yPos);

// 显示bmp灰度图像
void ShowBmpGreyImage(BmpImage *pImage, int xPos, int yPos);

// 清除图像显示
void CleanUpShowImage(int width, int height, int xPos, int yPos);

// 图像归一化，缩放到指定大小
void NormalizeImageSize( BmpImage *pImage, int width, int height);



#endif

