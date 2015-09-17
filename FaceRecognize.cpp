// FaceRecognize.cpp：基于人脸图像纹理的LBP（局部二值模式）特征进行人脸识别相关函数定义
// Copyright: cent
// 2015.9.17
// ~

#include "stdafx.h"
#include "resource.h"
#include <commdlg.h>
#include <direct.h>
#include <math.h>
#include "image.h"
#include "FaceDetect.h"
#include "FaceRecognize.h"



/************************************************************************************
	静态函数声明

************************************************************************************/
// 提取灰度图像纹理的LBP特征，统计LBP特征直方图表征人脸
static void ExtractImageLbp(BmpImage *pImage, int *LBP );
// 卡方统计两图像LBP相似度
static int ChiSquareStatistic(int *dstLBP, int *baseLBP, int len);
// 查找人脸库中是否存在目标人脸
static bool SearchFace(char *facebaseDir, int *dstLBP);



/***********************************************************************************
*																				   *
*	提取位图中第(m,n)block中第(i,j)像素点在领域内LBP值函数         			  	   *
*   取半径为r的环形领域上，圆周上P像素点数计算其Uniform LBP                        *
*   LBP模式二值序列中0->1或1->0变化次数小于等于2次的为Uniform LBP                  *
*																				   *
*   输入参数：image      - 位图像素数据		                            		   *
*             width      - 位图宽度                                                *
*             height     - 位图高度                                                *
*             m          - 块所在行号                                              *
*             n          - 块所在列号                                              *
*             blkWidth   - 位图分块宽度                                            *
*             blkHeight  - 位图分块高度                                            *
*             i          - 当前块像素行号                                          *
*             j          - 当前块像素列号                                          *
*   返回值：  LBP                                                				   *
*                        														   *
*																				   *
***********************************************************************************/
BYTE PixelLbp(char *image, int width, int height, 
              int m, int n, int blkWidth, int blkHeight, int i, int j)
{
	int k;
	int r, p;
	double pi;
	BYTE bit, lbp;
	int coff, tmpCoff;
	int x, y;

	r  = 2;
	p  = 8;
	pi = 3.1415927;
	lbp = 0;
	// 中心像素作阈值
	coff = (BYTE)image[(blkHeight*n+j)*width*3+(blkWidth*m+i)*3];
	// 与环形圆周上的8个领域像素点比较，得二值序列（LBP模式）
	for ( k=0; k<p; k++ )
	{
		// 均匀分布在环形圆周上的像素点在block中的下标
		x = (int)(i + r*sin(2*pi*k/p) + 0.5);
		y = (int)(j - r*cos(2*pi*k/p) + 0.5);
		// 像素下标从block坐标系转换到image坐标系
		x += blkWidth * m;
		y += blkHeight * n;
		if ( x < 0 )
			x = 0;
		else if ( x >= width )
			x = width-1;
		if ( y < 0 )
			y = 0;
		else if ( y >= height )
			y = height-1;
		tmpCoff = (BYTE)image[y*width*3+x*3];
		if ( tmpCoff < coff )
			bit = 0;
		else
			bit = 1;
		// 按顺序分配LBP二值权值，最后得LBP
		lbp |= bit << (p-1-k);
	} // end for (k)
	return lbp;
}

/***********************************************************************************
*																				   *	
*   取旋转不变性LBP值函数            		                            		   *
*   十进制数值最小LBP即为旋转不变性LBP，循环移位得当前LBP各个模式，取最小值LBP	   *
*                                                                                  *
*   输入参数：lbp  - LBP模式		                            		           *
*             n    - 模式二进制位数位宽                                            *
*   返回值：  旋转不变性lbp                                                        *
*												     							   *
***********************************************************************************/
BYTE MinLbp(BYTE lbp, int n)
{
	int i;
	BYTE bit;
	BYTE min;

	// 循环移位得当前LBP各个模式，取最小值LBP
	min = lbp;
	for ( i=0; i<n; i++ )
	{
		//循环移1位
		bit = lbp & (BYTE)(1 << (n-1));
		lbp = (lbp << 1) | bit;
		if ( lbp < min )
			min = lbp;
	}
	return min;
}

/***********************************************************************************
*																				   *
*	提取灰度图像纹理的LBP特征并统计LBP特征直方图表征人脸函数					   *
*																				   *
*   输入参数：pImage  - 图像结构指针     		                            	   *
*   输出参数：LBP     - LBP特征直方图                           		           *
*																				   *
***********************************************************************************/
static void ExtractImageLbp(BmpImage *pImage, int *LBP )
{
	int i, j, k;
	int n, m, blkCount;
	int blkWidth, blkHeight;
	BYTE lbp;
	int ULBPtable[256];    // uniform LBP 索引映射表
	char *tmpImage;

	memset(ULBPtable, -1, sizeof(ULBPtable));
	for ( i=0,k=0; i<256; i++ )
	{
		int count = 0;
		for ( j=0; j<7; j++ )
		{
			// LBP模式循环变化0->1 or 1->0次数，count<=2: Uniform LBP
			if ( ((i&(1<<j))<<1 ) != (i&(1<<(j+1))) )
				count++;
		}
		// 建立映射关系
		if ( count <= 2 )
			ULBPtable[i] = k++;
	}

	tmpImage = (char *)malloc(pImage->width*pImage->height*3);
	// copy image信息到临时空间
	memcpy(tmpImage, pImage->data, pImage->width*pImage->height*3);
	
	// 图像分块提取LBP，7*& blocks
	blkCount = 7;
	blkWidth = pImage->width / blkCount;
	blkHeight = pImage->height / blkCount;
	memset(LBP, 0, 59*blkCount*blkCount*sizeof(int));
	for ( n=0; n<blkCount; n++ )
	{
		for ( m=0; m<blkCount; m++ )
		{
			// 分块提取LBP
			for ( j=0; j<blkHeight; j++ )
			{
				for ( i=0; i<blkWidth; i++ )
				{
					// 提取image(blkCount*blkCount)分块中block(m,n)的pixel(i,j)的LBP
					lbp = PixelLbp(tmpImage, pImage->width, pImage->height, m, n, blkWidth, blkHeight, i, j);
					
					// 旋转不变性LBP
			//		lbp = MinLbp(lbp, 8);
			
					// LBP二值序列（LBP模式）对应的10进制数作LBP值
					pImage->data[(blkHeight*n+j)*pImage->width*3+(blkWidth*m+i)*3] = lbp;

					// 分类统计Uniform LBP直方图，58( p*(p-1)+2 )种ULBP
					if ( ULBPtable[lbp] != -1 )
						LBP[(n*blkCount+m)*59+ULBPtable[lbp]]++;
					else // 非ULBP归为1种混合LBP，混合LBP放最后
						LBP[(n*blkCount+m)*59+58]++;
				} // end for (i)
			} // end for (j)

		} // end for (m)
	} // end for (n)

	// 释放临时image空间
	free(tmpImage);
}

/***********************************************************************************
*																				   *
*	卡方统计两个图像LBP直方图特征的相似度函数									   *
*	χ^2 (S,M) = ∑ { (Si-Mi)^2 / (Si+Mi) }										   *
*	     		  																   *
*   输入参数：dstLBP  - 目标人脸位图LBP特征直方图                            	   *
*             baseLBP - 人脸库位图LBP特征直方图                                    *
*             len     - LBP特征直方图维度                                          *
*   返回值：  卡方值                                            		           *
*	     		 																   *
***********************************************************************************/
static int ChiSquareStatistic(int *dstLBP, int *baseLBP, int len)
{
	int i, j;
	int w[49];  // 人脸位图分块：7 * 7，每块权重值
	double x2;

	// 初始化为LBP特征直方图卡方统计时分配的权重
	for ( i=0; i<49; i++ )
		w[i] = 1;
		
	// 眼睛携带人脸重要信息，分配权重为4
	w[7*1+1] = w[7*1+2] = w[7*2+1] = w[7*2+2] = 4;
	w[7*1+4] = w[7*1+5] = w[7*2+4] = w[7*2+5] = 4;
	
	// 嘴和额头两侧权重为2
	w[7*4+3] = 2;
	w[0] = w[7*1+0] = 2;
	w[6] = w[7*1+6] = 2;
	
	// 鼻子和脸下巴两侧权重为0
	w[7*2+3] = w[7*3+3] = 0;
	w[7*3+0] = w[7*4+0] = w[7*5+0] = w[7*6+0] = 0;
	w[7*3+6] = w[7*4+6] = w[7*5+6] = w[7*6+6] = 0;

	for ( x2=0,i=0; i<49; i++ )
	{
		for ( j=0; j<59; j++ )
		{
			if ( dstLBP[i*59+j] || baseLBP[i*59+j] )
				x2 += 1.0*w[i]*(dstLBP[i*59+j]-baseLBP[i*59+j]) *
							   (dstLBP[i*59+j]-baseLBP[i*59+j]) / 
							   (dstLBP[i*59+j]+baseLBP[i*59+j]);
		}
	}
	return (int)x2;
}

/***********************************************************************************
*																				   *
*	在人脸库中查找与目标人脸位图LBP特征直方图相近人脸函数						   *
*	设定阀值，小于阀值可认为找到同一人脸										   *
*	  																			   *
*   输入参数：dstLBP      - 目标人脸位图LBP特征直方图                          	   *
*             facebaseDir - 人脸库路径                                             *
*																				   *
***********************************************************************************/
static bool SearchFace(char *facebaseDir, int *dstLBP)
{
	char findFileName[256];
	char strFilter[256];
	char findFilePath[256];
	BmpImage *image, *faceImage;
	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile;
	int LBP[49*59];    // 人脸位图分块：7 * 7

	// 通配符遍历facebaseDir目录下的所以文件，包括目录
	strcpy(strFilter, facebaseDir);
	strcat(strFilter, "\\*.*");
	
	// 获取目录查找句柄，及第一个文件数据
	hFindFile = FindFirstFile(strFilter, &findFileData);
	// 当前目录是否存在
	if ( hFindFile == INVALID_HANDLE_VALUE )
		return false;

	do
	{
		// 忽略目录
		if ( findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			continue;

		strcpy(findFileName, findFileData.cFileName);
		// 忽略非.bmp文件
		if ( strcmp(findFileName+strlen(findFileName)-4, ".bmp" ) != 0 )
			continue;

		// .bmp文件的绝对路径
		strcpy(findFilePath, facebaseDir);
		strcat(findFilePath, "\\");
		strcat(findFilePath, findFileName);
		image = ReadBmpFile(findFilePath);
		if( image == NULL)
			continue;
		
		faceImage = ExtractFace(image);
		if ( faceImage == NULL )
		{
			FreeBmpImage(image);
			continue;
		}
		ShowBmpImage(faceImage, 750, 20);
		
		// 色彩空间模型转换
		RgbToYcbcr(faceImage, faceImage);
		ExtractImageLbp(faceImage, LBP);
		ShowBmpGreyImage(faceImage, 750, 100);

		int x2;
		// LBP卡方统计相似度量值与设定阈值对比，小于默认阈值时，图像相同
		if ( (x2=ChiSquareStatistic(dstLBP, LBP, 49*59)) < 3000 )
		{
			char strx2[50];
			
			wsprintf(strx2, "%s%d", "x^2 = ", x2);
			TextOut(hWinDC, 700, 180, strx2, strlen(strx2));
			ShowBmpImage(image, 660, 200);
			
			FindClose(hFindFile);
			FreeBmpImage(image);
			FreeBmpImage(faceImage);
			return true;
		}
		
		// 释放ReadBmpFile、ExtractFace动态生成的位图空间
		FreeBmpImage(image);
		FreeBmpImage(faceImage);

	} while ( FindNextFile(hFindFile, &findFileData) );
	FindClose(hFindFile);
	
	// Update Show Rect()
	RECT rt;
	rt.left = 750, rt.top = 20;
	rt.right = 1000, rt.bottom = 200;
	InvalidateRect(hMainWnd, &rt, true);

	return false;
}


/*****************************************************************************************
*																						 *
*	在人脸库中查找出相似度量比较接近目标人脸的图片函数									 *
*																						 *
*   输入参数：pImage       - 目标位图结构指针  	                						 *
*             facebasePath - 人脸库路径                          						 *
*																						 *
*****************************************************************************************/
bool RecognizeFace(BmpImage *pImage, char *facebasePath)
{
	int dstLBP[49*59];    // 人脸位图分块：7 * 7
	BmpImage *faceImage;
	
	faceImage = ExtractFace(pImage);
	if ( faceImage == NULL )
		return false;
	ShowBmpImage(faceImage, 660, 20);
	
	RgbToYcbcr(faceImage, faceImage);
	ShowBmpGreyImage(faceImage, 660, 100);
	
	ExtractImageLbp(faceImage, dstLBP);
	
	// 释放ExtractFace动态生成的位图空间
	FreeBmpImage(faceImage);

	return SearchFace(facebasePath, dstLBP);
}

/*****************************************************************************************
*																						 *
*	捕获人脸图像入库函数         														 *
*																						 *
*   输入参数：imgFileName   - 人脸位图文件名    	               						 *
*             facebasePath  - 人脸库位图位置路径                       					 *
*																						 *
*****************************************************************************************/
bool EnterFace(char *imgFileName, char *facebasePath)
{
	char fileName[256];
	char strFilter[256];
	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile;
	int count;

	// 创建人脸样本库目录
	CreateDirectory(facebasePath, NULL);

	// 通配符遍历facebaseDir目录下的所有文件，包括目录
	strcpy(strFilter, facebasePath);
	strcat(strFilter, "\\*.*");
	
	// 获取目录查找句柄，及第一个文件数据
	hFindFile = FindFirstFile(strFilter, &findFileData);
	// 当前目录是否存在
	if ( hFindFile == INVALID_HANDLE_VALUE )
		return false;
	count = 0;
	do
	{
		// 忽略目录
		if ( findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			continue;

		strcpy(fileName, findFileData.cFileName);
		// 忽略非.bmp文件
		if ( strcmp(fileName+strlen(fileName)-4, ".bmp" ) != 0 )
			continue;
		
		count++;
	} while ( FindNextFile(hFindFile, &findFileData) );
	FindClose(hFindFile);

	// 以序号构造录库人脸图像文件名
	wsprintf(fileName, "%s%s%d%s", facebasePath, "\\face", count++, ".bmp");
	if ( CopyFile(imgFileName, fileName, 1) == NULL )
		return false;
	return true;
}

/*****************************************************************************************
*																						 *
*	选择删除人脸库中的人脸样本函数 														 *
*																						 *
*   输入参数：facebasePath  - 人脸库位图位置路径                       					 *
*																						 *
*****************************************************************************************/
void DeleteFace(char *facebasePath)
{
	char facePath[256];
	char fileTitle[256];
	char selImgFileDir[256];
	OPENFILENAME ofn;
	BmpImage *image, *faceImage;
	int dstLBP[49*59];    // 人脸位图分块：7 * 7

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=NULL;
	ofn.hInstance=NULL;
	ofn.lpstrFilter=TEXT("bmp files\0*.bmp\0All File\0*.*\0\0");
	ofn.lpstrCustomFilter=NULL;
	ofn.nMaxCustFilter=0;
	ofn.nFilterIndex=1;
	ofn.lpstrFile=facePath;
	ofn.nMaxFile=MAX_PATH;
	ofn.lpstrFileTitle=fileTitle;
	ofn.nMaxFileTitle=99;
	ofn.lpstrInitialDir=facebasePath;
	ofn.lpstrTitle="选择要移除的人脸样本图像";
	ofn.Flags=OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt="raw";
	ofn.lCustData=NULL;
	ofn.lpfnHook=NULL;
	ofn.lpTemplateName=NULL;
	facePath[0]='\0';
	GetOpenFileName(&ofn); 

	// 取得选择目标文件名
	getcwd(selImgFileDir, MAX_PATH);

	// 路径空时，取消
	if ( !strlen(facePath) )
		return ;

	image = ReadBmpFile(facePath);
	if( image == NULL )
		return ;
		
	TextOut(hWinDC, 660, 20, "已选择待移除的人脸样本：", strlen("已选择待移除的人脸样本："));
	ShowBmpImage(image, 660, 50);
	
	faceImage = ExtractFace(image);
	if( faceImage == NULL )
	{
		FreeBmpImage(image);
		return ;
	}
	ShowBmpImage(faceImage, 730, faceImage->height+80);
	
	RgbToYcbcr(faceImage, faceImage);
	ExtractImageLbp(faceImage, dstLBP);
	ShowBmpGreyImage(faceImage, 850, faceImage->height+80);

	if ( strcmp(selImgFileDir, facebasePath) )
	{
		MessageBox(hMainWnd, "移除失败，选择的不是人脸取样库目录...\\Facebase下的图像", "移除人脸样本", 0);
		
		FreeBmpImage(image);
		FreeBmpImage(faceImage);
		return ;
	}

	if ( MessageBox(hMainWnd, "确定移除所选人脸样本图像？", "移除人脸样本", MB_OKCANCEL) == IDCANCEL )
	{
		if ( !DeleteFile(facePath) )
			MessageBox(hMainWnd, "移除目标人脸样本图像失败！", "移除人脸样本", 0);
		else
			MessageBox(hMainWnd, "移除目标人脸样本成功！", "移除人脸样本", 0);
	}
	
	FreeBmpImage(image);
	FreeBmpImage(faceImage);

	// Update Show Rect()
	RECT rt;
	rt.left = 650, rt.top = 20;
	rt.right = 1000, rt.bottom = 500;
	InvalidateRect(hMainWnd, &rt, true);
}
