// Image.cpp：位图相关处理函数定义
// Copyright: cent
// 2015.9.17
// ~

#include "stdafx.h"
#include "resource.h"
#include <commdlg.h>
#include <direct.h>
#include <math.h>
#include "image.h"

HDC hWinDC;



/************************************************************************************************
*																								*
*	动态分配目标位图内存函数                                           							*
*																								*
*   输入参数：width  - 动态分配目标位图宽度                                                     *
*             height - 动态分配目标位图高度                                                     *
*   返回会  ：         指向动态分配目标位图内存指针                                             *
*																								*
************************************************************************************************/
BmpImage * MallocBmpImage(int width, int height)
{
	BmpImage *pImage;
	
	pImage = (BmpImage *)malloc(sizeof(BmpImage));
	if( pImage == NULL)
		return NULL;
		
	pImage->width  = width;
	pImage->height = height;
	
	// 3倍width*height，用来像素RGB数据
	pImage->data = (char *)malloc(width*height*3);
	if( pImage->data == NULL )
	{
		free(pImage);
		return NULL;
	}
	
	return pImage;
}

/************************************************************************************************
*																								*
*	回收动态分配位图内存函数                                           							*
*																								*
*   输入参数：pImage  - 指向动态分配内存目标位图指针                                            *
*																								*
************************************************************************************************/
void FreeBmpImage(BmpImage *pImage)
{
	pImage->width  = 0;
	pImage->height = 0;
	
	// 先释放像素数据内存，再释放位图结构内存
	free(pImage->data);
	free(pImage);
}

/************************************************************************************************
*																								*
*	获取文件名函数OpenImageFile(char *OPDLTitle, char *pImgFileName)							*
*																								*
*   输入参数：OPDLTitle    - 文件选择对话框标题                                                 *
*   输出参数：pImgFileName - 选择目标的文件名                                                   *
*																								*
************************************************************************************************/
void OpenImageFile(char *OPDLTitle, char *pImgFileName)
{
	char FileTitle[100], ImgDlgFileDir[256];
	OPENFILENAME ofn;

	getcwd(ImgDlgFileDir, MAX_PATH);

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=NULL;
	ofn.hInstance=NULL;
	ofn.lpstrFilter=TEXT("bmp files\0*.bmp\0All File\0*.*\0\0");
	ofn.lpstrCustomFilter=NULL;
	ofn.nMaxCustFilter=0;
	ofn.nFilterIndex=1;
	ofn.lpstrFile=pImgFileName;
	ofn.nMaxFile=MAX_PATH;
	ofn.lpstrFileTitle=FileTitle;
	ofn.nMaxFileTitle=99;
	ofn.lpstrInitialDir=ImgDlgFileDir;
	ofn.lpstrTitle=OPDLTitle;
	ofn.Flags=OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt="raw";
	ofn.lCustData=NULL;
	ofn.lpfnHook=NULL;
	ofn.lpTemplateName=NULL;
	pImgFileName[0]='\0';
	GetOpenFileName(&ofn); 

	getcwd(ImgDlgFileDir, MAX_PATH);
}

/**********************************************************************************************
*																							  *	
*   提取位图文件原始像素数据24位真彩色位图像素RGB数据函数									  *	
*   丢掉用来补齐位图width*3字节数到4倍数的0像素数据，得到bmp的RGB真实图像数据				  *
*																							  *	
*   输入输出参数：pImage - 包含24位真彩色原始位图文件中分解出来像素数据的位图                 *
*																							  *
**********************************************************************************************/
void ExtractImageData(BmpImage *pImage)
{
	int i, j;
	int cntZeros;
	int cntPixels;
	
	// 每行像素字节数为非4倍数，用0补齐每行像素字节数到4的倍数
	if ( !(pImage->width*3 % 4) )
		return ;
	
	// 补齐0的字节数
	cntZeros = 4 - pImage->width * 3 % 4;
	
	// 像素数据指示器
	cntPixels = 0;
	for ( j=0; j<pImage->height; j++ )
	{
		for ( i=0; i<pImage->width; i++ )
		{
			// 位图真实像素数据保留
			pImage->data[j*pImage->width*3+i*3]   = pImage->data[cntPixels++];
			pImage->data[j*pImage->width*3+i*3+1] = pImage->data[cntPixels++];
			pImage->data[j*pImage->width*3+i*3+2] = pImage->data[cntPixels++];
		}
		// 每行丢掉cntZeros个字节的补齐4倍数的无效像素数据
		for ( i=0; i<cntZeros; i++ )
			cntPixels++;
	}
}

/**********************************************************************************************
*																							  *	
*   倒转原始24位真彩色位图像素RGB数据函数													  *	
*   原始位图文件像素从下到上，从左到右，倒转为从上到下，从左到右                              *
*																							  *	
*   输入输出参数：pImage - 包含24位真彩色原始位图文件中分解出来像素数据位图指针               *
*																							  *
**********************************************************************************************/
void ReverseImageData(BmpImage *pImage)
{
	int i, j;
	char tmpPixel;
	
	// 24位真彩色位图每个像素用3个字节R，G，B来表示
	for (i=0; i<pImage->height/2; i++)
	{
		for (j=0; j<pImage->width; j++)
		{
			// R
			tmpPixel = pImage->data[i*pImage->width*3+j*3];
			pImage->data[i*pImage->width*3+j*3] = pImage->data[(pImage->height-i-1)*pImage->width*3+j*3+2];
			pImage->data[(pImage->height-i-1)*pImage->width*3+j*3+2] = tmpPixel;
			// G
			tmpPixel = pImage->data[i*pImage->width*3+j*3+1];
			pImage->data[i*pImage->width*3+j*3+1] = pImage->data[(pImage->height-i-1)*pImage->width*3+j*3+1];
			pImage->data[(pImage->height-i-1)*pImage->width*3+j*3+1] = tmpPixel;
			// B
			tmpPixel = pImage->data[i*pImage->width*3+j*3+2];
			pImage->data[i*pImage->width*3+j*3+2] = pImage->data[(pImage->height-i-1)*pImage->width*3+j*3];
			pImage->data[(pImage->height-i-1)*pImage->width*3+j*3] = tmpPixel;
		}
	}
}

/************************************************************************************************
*																								*
*   读取24位真彩色位图像素数据函数，每个像素用3个字节RGB表示                                    *
*   打开图像文件，解析位图文件结构，分解出位图文件头、位图信息头，像素数据						*
*   输入参数：imgFileName - 文件名                                                              *
*   返回值  ：              存放24位真彩色位图像素RGB数据及尺寸为320*240位图结构指针            *
*																								*
************************************************************************************************/
BmpImage* ReadBmpFile(LPSTR imgFileName)
{
	OFSTRUCT of;
	HFILE Image_fp;
	BITMAPFILEHEADER bfImage;
	BITMAPINFOHEADER biImage;
	BmpImage *pImage;


	if( strlen(imgFileName) == 0 )
		return NULL;
	
	Image_fp = OpenFile(imgFileName, &of, OF_READ);
	if (Image_fp == HFILE_ERROR) 
	{
		MessageBox(NULL, "打开读bmp图像文件出错", "打开文件出错信息", MB_OK);
		return NULL;
	}

	// 定位及读取位图文件头
	_llseek(Image_fp, 0, 0);
	_lread(Image_fp, &bfImage, sizeof(BITMAPFILEHEADER));

	// "BM" (0x4d42)：表示位图BMP
	if ((bfImage.bfType != 0x4d42)) 
	{
		MessageBox(NULL, "非bmp文件", "打开bmp文件出错信息", MB_OK);
		_lclose(Image_fp);
		return NULL;
	}

	// 读取位图信息头
	_lread(Image_fp, &biImage, sizeof(BITMAPINFOHEADER));

	if (biImage.biBitCount != 24)
	{
		MessageBox(NULL, "非24位真彩色位图", "读取bmp文件出错信息", MB_OK);
		_lclose(Image_fp);
		return NULL;
	}

	// 分配临时存储像素数据空间
	pImage = MallocBmpImage(biImage.biWidth, biImage.biHeight);
	if( pImage == NULL )
	{
		MessageBox(NULL, "系统没有足够内存空间以分配存储位图", "读取bmp文件出错信息", MB_OK);
		_lclose(Image_fp);
		return NULL;
	}

	// 定位及读取位图像素阵列数据
	_llseek(Image_fp, bfImage.bfOffBits, 0);
	_lread(Image_fp, pImage->data, biImage.biSizeImage);

	// 丢掉bmp像素数据中补齐每行像素字节数4倍的无效数据0，得RGB真实像素数据
	ExtractImageData(pImage);
	
	// bmp图片像素从下到上，从左到右，倒转为从上到下，从左到右
	ReverseImageData(pImage);

	//图像缩放归一化，限定输出图像尺寸：320*240
	NormalizeImageSize(pImage, 320, 240);
						
	_lclose(Image_fp);

	return pImage;
}


/************************************************************************************************
*																								*
*	在屏幕上显示位图函数ShowBmpImage(BmpImage *pImage, int xPos, int yPos)                    	*
*																								*
*   输入参数：pImage - 存放位图像素数据点阵的向量及尺寸结构的指针								*
*		      xPos   - 图像显示开始位置x坐标													*
*		      yPos   - 图像显示开始位置y坐标													*
*																								*
************************************************************************************************/
void ShowBmpImage(BmpImage *pImage, int xPos, int yPos)	
{
	int i,j;
	int r, g, b;
	
	for (i=0; i<pImage->height; i++) 
	{
		for (j=0; j<pImage->width; j++) 
		{
			r = (BYTE) pImage->data[i*pImage->width*3+j*3]; 
			g = (BYTE) pImage->data[i*pImage->width*3+j*3+1];
			b = (BYTE) pImage->data[i*pImage->width*3+j*3+2];
			
			SetPixel(hWinDC, j+xPos, i+yPos, RGB(r, g, b));
		}
	}
}

/************************************************************************************************
*																						        *
*   显示位图的YCbCr空间灰度图函数   													        *
*                                                                                               *
*   输入参数：pImage - 存放位图像素YCbCr空间灰度数据及尺寸结构的指针							*
*		      xPos   - 图像显示开始位置x坐标													*
*		      yPos   - 图像显示开始位置y坐标													*
*																						        *
************************************************************************************************/
void ShowBmpGreyImage(BmpImage *pImage, int xPos, int yPos)
{
	int i,j;
	int y;
	
	for (i=0; i<pImage->height; i++) 
	{
		for (j=0; j<pImage->width; j++) 
		{
			y = (BYTE)pImage->data[i*pImage->width*3+j*3];
			SetPixel(hWinDC, j+xPos, i+yPos, RGB(y, y, y));
		}
	}
}

/************************************************************************************************
*																					   			*
*   清屏函数，把显示的图像清为白色													   			*
*                                                                                               *
*   输入参数：width  - 清屏区域宽度 															*
*             height - 清屏区域高度 															*
*		      xPos   - 图像显示开始位置x坐标													*
*		      yPos   - 图像显示开始位置y坐标													*
*																					   			*
************************************************************************************************/
void CleanUpShowImage(int width, int height, int xPos, int yPos)
{
	int i, j;

	for ( j=0; j<height/2+1; j++ )
	{
		for ( i=0; i<width; i++ )
		{
			SetPixel(hWinDC, xPos+i, yPos+j, RGB(255, 255, 255));
			SetPixel(hWinDC, xPos+i, yPos+height-j, RGB(255, 255, 255));
		}
	}
}

/************************************************************************************************
*																								*	
*   图像尺寸缩放归一化函数，最临近插值算法缩放线性												*
*                                                                                               *
*   输入输出参数：pImage  - 位图结构指针														*
*                 width   - 缩放后宽度															*
*	              height  - 缩放后高度															*
*																								*
************************************************************************************************/
void NormalizeImageSize( BmpImage *pImage, int width, int height)
{
	int i, j;
	int x, y;
	double xRate, yRate;
	char *tmpData;

	tmpData = (char *)malloc(width*height*3);
	if (tmpData == NULL)
		return ;

	// 缩放比例
	xRate = (double)pImage->width/width;
	yRate = (double)pImage->height/height;
	
	for ( j=0; j<height; j++ )
	{
		// 映射到原pImage->height下标
		y = (int)(j*yRate+0.5);
		if ( y<0 )
			y = 0;
		else if ( y>=pImage->height )
			y = pImage->height-1;

		for ( i=0; i<width; i++ )
		{
			// 映射到原pImage->width下标
			x = (int)(i*xRate+0.5);
			if ( x<0 )
				x = 0;
			else if ( x>=pImage->width )
				x = pImage->width-1;

			tmpData[j*3*width+i*3+0] = pImage->data[y*3*pImage->width+x*3+0];  // R
			tmpData[j*3*width+i*3+1] = pImage->data[y*3*pImage->width+x*3+1];  // G
			tmpData[j*3*width+i*3+2] = pImage->data[y*3*pImage->width+x*3+2];  // B
		}
	}
	// 得到位图新尺寸
	pImage->width = width;
	pImage->height = height;

	// 释放原位图数据，并指向缩放后新的位图数据
	free(pImage->data);
	pImage->data = tmpData;
}
