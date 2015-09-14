// Image.h

#ifndef IMAGE_H
#define IMAGE_H



extern HWND hMainWnd; // handle of main window
extern HDC hWinDC;

#define MAX_IMAGE_WIDTH	    320
#define MAX_IMAGE_HEIGHT	240


// 24位真彩色位图结构定义
// image[]：存放BMP真实像素RBG数据，最大可放（MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 3）字节
// width & height：存放BMP真实尺寸
typedef struct _BmpImage
{
	char image[MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT*3];
	int width;
	int height;
} BmpImage;



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

// 图像尺寸缩放归一化函数，最临近插值算法缩放线性
void NormalizeImageSize(char *pDstImageData, int dstWidth, int dstHeight,
						char *pSrcImageData, int srcWidth, int srcHeight);

// 图像归一化，缩放到指定大小
void NormalizeImageSize( BmpImage *pDstImage, BmpImage *pSrcImage, int width, int height);

// 识别人脸，库中找出目标人脸
bool RecognizeFace(BmpImage *pImage, char *facebasePath);

// 得到人脸肤色cbcr[cb][cr]对比库
bool FaceCbcrProc(char *sampleImagePath);

// 提取人脸
bool ExtractFace(BmpImage *pFaceImage, BmpImage *pImage);

// 照片录入人脸库
bool EnterFace(char *imgFileName, char *facebasePath);

// 移除人脸取样
void DeleteFace(char *facebasePath);


#endif

