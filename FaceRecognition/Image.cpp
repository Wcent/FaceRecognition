// Image.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include <commdlg.h>
#include <direct.h>
#include <math.h>
#include "image.h"

HDC hWinDC;
char ImgFileName[256];
char Image[320*240*3];
char NewImage[320*240*3];
int ImageWidth;
int ImageHeight;
int FaceWidth;
int FaceHeight;
int cbcr[320][240];
int ci[320];
int cj[240];
int LBP[49*59];
int dstLBP[49*59];


/************************************************************************************************
*																								*
*	获取文件名函数OpenImageFile(char *OPDLTitle)												*
*																								*
************************************************************************************************/
void OpenImageFile(char *OPDLTitle)
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
	ofn.lpstrFile=ImgFileName;
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
	ImgFileName[0]='\0';
	GetOpenFileName(&ofn); 

	getcwd(ImgDlgFileDir, MAX_PATH);
}

/**********************************************************************************************
*																							  *	
*   丢掉用来补齐位图width*3字节数到4倍数的0像素数据，得到bmp的RGB真实图像数据				  *
*																							  *
**********************************************************************************************/
void ExtractImageData(char *image, int width, int height)
{
	int i, j;
	int cz;
	int cp;
	
	// 每行像素字节数为非4倍数，用0补齐每行像素字节数到4的倍数
	if ( !(width*3 % 4) )
		return ;
	
	// 补齐0的字节数
	cz = 4 - width * 3 % 4;
	// 像素指针
	cp = 0;
	for ( j=0; j<height; j++ )
	{
		for ( i=0; i<width; i++ )
		{
			// 位图真实像素数据保留
			image[j*width*3+i*3] = image[cp++];
			image[j*width*3+i*3+1] = image[cp++];
			image[j*width*3+i*3+2] = image[cp++];
		}
		// 每行丢掉cz个字节的补齐4倍数的无效像素数据
		for ( i=0; i<cz; i++ )
			cp++;
	}
}

/************************************************************************************************
*																								*
*   从BMP图像中，读出图像头信息，主要包括图像长度和宽度											*
*																								*
************************************************************************************************/
BOOL ReadBmpFile(LPSTR ImageFileName, char *oImage)
{
	OFSTRUCT of;
	HFILE Image_fp;
	BITMAPFILEHEADER bfImage;
	BITMAPINFOHEADER biImage;
	int i, j;
	char tmpPixel;
	char *tmpImage;

	Image_fp = OpenFile(ImageFileName, &of, OF_READ);
	if (Image_fp == HFILE_ERROR) 
	{
		MessageBox(NULL, "打开读bmp图像文件出错", "打开文件出错信息", MB_OK);
		return FALSE;
	}

	// 读取位图文件头
	_llseek(Image_fp, 0, 0);
	_lread(Image_fp, &bfImage, sizeof(BITMAPFILEHEADER));

	// "BM" (0x4d42)：表示位图BMP
	if ((bfImage.bfType != 0x4d42)) 
	{
		MessageBox(NULL, "非bmp文件", "打开bmp文件出错信息", MB_OK);
		return FALSE;
	}

	// 读取位图信息头
	_lread(Image_fp, &biImage, sizeof(BITMAPINFOHEADER));

	if (biImage.biBitCount != 24)
	{
		MessageBox(NULL, "非24位真彩色位图", "读取bmp文件出错信息", MB_OK);
		return FALSE;
	}

	ImageWidth = biImage.biWidth;
	ImageHeight= biImage.biHeight;
	tmpImage = (char *)malloc(biImage.biSizeImage);
	if( tmpImage == NULL )
		return FALSE;

	// 读取位图像素阵列数据
	_llseek(Image_fp, bfImage.bfOffBits, 0);
	_lread(Image_fp, tmpImage, biImage.biSizeImage);

	// 丢掉bmp像素数据中含多余的补齐width*3字节4倍数的0像素数据，得RGB真实像素数据
	if ( ImageWidth*3 % 4 )
		ExtractImageData(tmpImage, ImageWidth, ImageHeight);
	_lclose(Image_fp);

	ImageWidth = 320;
	ImageHeight = 240;
	//图像归一化320 240
	NormalizeImageSize(oImage, ImageWidth, ImageHeight, 
						tmpImage, biImage.biWidth, biImage.biHeight);
	free(tmpImage);

	// bmp图片像素从下到上，从左到右，倒转为从上到下，从左到右
	for (i=0; i<ImageHeight/2; i++)
	{
		for (j=0; j<ImageWidth; j++)
		{
			// R
			tmpPixel					 = oImage[i*ImageWidth*3+j*3];
			oImage[i*ImageWidth*3+j*3]	 = oImage[(ImageHeight-i-1)*ImageWidth*3+j*3+2];
			oImage[(ImageHeight-i-1)*ImageWidth*3+j*3+2] = tmpPixel;
			// G
			tmpPixel					 = oImage[i*ImageWidth*3+j*3+1];
			oImage[i*ImageWidth*3+j*3+1] = oImage[(ImageHeight-i-1)*ImageWidth*3+j*3+1];
			oImage[(ImageHeight-i-1)*ImageWidth*3+j*3+1] = tmpPixel;
			// B
			tmpPixel					 = oImage[i*ImageWidth*3+j*3+2];
			oImage[i*ImageWidth*3+j*3+2] = oImage[(ImageHeight-i-1)*ImageWidth*3+j*3];
			oImage[(ImageHeight-i-1)*ImageWidth*3+j*3] = tmpPixel;
		}
	}

	return TRUE;
}


/************************************************************************************************
*																								*
*	在屏幕上显示图像函数ShowBmpImage(char *Image, int wImage, int hImage, int xPos, int yPos)	*
*																								*
*   	Image	－存放图像点阵的向量															*
*		wImage	－图像宽度																		*
*		hImage	－图像高度																		*
*		xPos	－图像显示开始位置x坐标															*
*		yPos	－图像显示开始位置y坐标															*
*																								*
************************************************************************************************/
void ShowBmpImage(char *Image, int wImage, int hImage, int xPos, int yPos)	
{
	int i,j;
	int r, g, b;
	
	for (i=0; i<hImage; i++) 
	{
		for (j=0; j<wImage; j++) 
		{
			r = (BYTE) Image[i*wImage*3+j*3]; 
			g = (BYTE) Image[i*wImage*3+j*3+1];
			b = (BYTE) Image[i*wImage*3+j*3+2];
			
			SetPixel(hWinDC, j+xPos, i+yPos, RGB(r, g, b));
		}
	}
}

/*****************************************************************************************
*																						 *
*   显示bmp图片的在YCbCr空间下的灰度图													 *
*																						 *
*****************************************************************************************/
void ShowBmpGreyImage(char *Image, int wImage, int hImage, int xPos, int yPos)
{
	int i,j;
	int y;
	
	for (i=0; i<hImage; i++) 
	{
		for (j=0; j<wImage; j++) 
		{
			y = (BYTE)Image[i*wImage*3+j*3];
			SetPixel(hWinDC, j+xPos, i+yPos, RGB(y, y, y));
		}
	}
}

/***************************************************************************************
*																					   *
*   清屏，把显示的图像清为白色														   *	
*																					   *
***************************************************************************************/
void CleanUpShowImage(int width, int height, int x, int y)
{
	int i, j;

	for ( j=0; j<height/2+1; j++ )
	{
		for ( i=0; i<width; i++ )
		{
			SetPixel(hWinDC, x+i, y+j, RGB(255, 255, 255));
			SetPixel(hWinDC, x+i, y+height-j, RGB(255, 255, 255));
		}
	}
}


/**************************************************************************************************
*																								  *	
*   图像尺寸大小归一化：最临近插值算法缩放，线性												  *
*   nImage: new image																			  *
*   nWidth: new width																			  *
*	nHeight: new height																			  *
*																								  *
**************************************************************************************************/
void NormalizeImageSize( char *dstImage, int nWidth, int nHeight, 
						char *image, int width, int height)
{
	int i, j;
	int x, y;
	double rx, ry;

	// 缩放比例
	rx = (double)width/nWidth;
	ry = (double)height/nHeight;
	for ( j=0; j<nHeight; j++ )
	{
		// 映射到原height下标
		y = (int)(j*ry+0.5);
		if ( y<0 )
			y = 0;
		else if ( y>=height )
			y = height-1;

		for ( i=0; i<nWidth; i++ )
		{
			// 映射到原width下标
			x = (int)(i*rx+0.5);
			if ( x<0 )
				x = 0;
			else if ( x>=width )
				x = width-1;

			dstImage[j*3*nWidth+i*3+0] = image[y*3*width+x*3+0];
			dstImage[j*3*nWidth+i*3+1] = image[y*3*width+x*3+1];
			dstImage[j*3*nWidth+i*3+2] = image[y*3*width+x*3+2];
		}
	}
}

/****************************************************************************************************
*																								    *	
*   RGB色彩空间模型转换到YCbCr色彩空间模型															*
*																									*
****************************************************************************************************/
void RgbToYcbcr(char *image, char *nImage, int width, int height)
{
	int i, j;
	int r, g, b;
	int y, cr, cb;

	for ( j=0; j<height; j++ )
	{
		for ( i=0; i<width; i++ )
		{
			r = (BYTE)image[j*width*3+i*3];
			g = (BYTE)image[j*width*3+i*3+1];
			b = (BYTE)image[j*width*3+i*3+2];
			
			/*****************************************************************\
			RGB to YCbCr:
			| Y  |    | 16  |             |   65.738  129.057   25.06 |   | R |
			| Cb |  = | 128 | + (1/256) * |  -37.945  -74.494  112.43 | * | G |
			| Cr |    | 128 |             |  112.439  -94.154  -18.28 |   | B |
			
			\*****************************************************************/
			y  = (int)(16 + (65.738*r + 129.057*g + 25.06*b) / 256);
			cb = (int)(128 + (-37.945*r - 74.494*g + 112.43*b) / 256);
			cr = (int)(128 + (112.439*r - 94.154*g - 18.28*b) / 256);
			
			nImage[j*width*3+i*3] = y;
			nImage[j*width*3+i*3+1] = cb;
			nImage[j*width*3+i*3+2] = cr;
		}
	}
}

/*****************************************************************************************
*																						 *
*   处理FaceSample.bmp得到样本人脸肤色的cbcr[cb][cr]库									 *
*																						 *		
*****************************************************************************************/
void FaceSampleCbcr(char *image, int width, int height)
{
	int i, j;
	int y, cb, cr;

	for ( j=0; j<height; j++ )
	{
		for ( i=0; i<width; i++ )
		{
			y  = (BYTE)image[j*width*3+i*3];
			cb = (BYTE)image[j*width*3+i*3+1];
			cr = (BYTE)image[j*width*3+i*3+2];

			// 肤色范围...标记人脸样本肤色对应cb，cr值
			if ( cb>98 && cb<123 && cr>133 && cr<169 )
			    cbcr[cb][cr] = 1;
		}
	}
}

/****************************************************************************************
*																						*
*	根据cb,cr肤色范围得到人脸肤色的cbcr[cb][cr]对比库									*
*																						*
****************************************************************************************/
void FaceCbcr()
{
	int i, j;

	// cb>98 && cb<123 && cr>133 && cr<169
	// 肤色范围...标记人脸样本肤色对应cb，cr值
	for ( i=98; i<123; i++ )
		for ( j=133; j<169; j++ )
			cbcr[i][j] = 1;
}

bool FaceCbcrProc()
{
	OFSTRUCT of;
	char sampleImagePath[1024];

	strcpy(sampleImagePath, curDir);
	strcat(sampleImagePath, "\\FaceSample.bmp");		
	// 尝试打开文件，判断文件是否存在
	// 返回负数文件不存在，存在时，关闭文件，返回已无效句柄
	if ( OpenFile(sampleImagePath, &of, OF_EXIST) < 0 )
	{
		// 未找到肤色样本图时，直接由Cb，Cr肤色范围得到人脸肤色对比库
		FaceCbcr();
		return true;
	}

	// 打开FaceSample.bmp肤色样本图
	if ( !ReadBmpFile(sampleImagePath, Image) )
		return false;
	RgbToYcbcr(Image, NewImage, ImageWidth, ImageHeight);
	// 由样本肤色Cb，Cr得到人脸肤色对比库
	FaceSampleCbcr(NewImage, ImageWidth, ImageHeight);

	return true;
}

/*****************************************************************************************
*																						 *
*	在YCbCr空间中分割出人脸候选区域														 *
*																						 *
*****************************************************************************************/
void FaceDetect(char *image, int width, int height)
{
	int i, j;
	int y, cb, cr;

	for ( j=0; j<height; j++ )
	{
		for ( i=0; i<width; i++ )
		{
			y = (BYTE)image[j*width*3+i*3];
			cb = (BYTE)image[j*width*3+i*3+1];
			cr = (BYTE)image[j*width*3+i*3+2];

			// 肤色分割...肤色范围
		//	if ( cb>98 && cb<123 && cr>133 && cr<169 )
			if ( cbcr[cb][cr] == 1 )
				y = 255; //white
			else
				y = 0; //black

			image[j*width*3+i*3] = y;
			image[j*width*3+i*3+1] = cb;
			image[j*width*3+i*3+2] = cr;
		}
	}
}

/*****************************************************************************************
*																						 *
*	投影法分割人脸位置，投影肤色计算下标像素数											 *
*																						 *
*****************************************************************************************/
bool CountFacePixel(char *image, int width, int height, 
					char *fImage, int &fWidth, int &fHeight)
{
	int i, j;
	int y;
	int ws, we, hs, he;

	memset(ci, 0, sizeof(ci));
	memset(cj, 0, sizeof(cj));

	for ( i=0; i<height; i++ )
	{
		for ( j=0; j<width; j++ )
		{
			y = (BYTE)image[i*width*3+j*3];
			// 人脸肤色像素投影到width、height两坐标上
			if ( y == 255 )
			{
				ci[j]++;
				cj[i]++;
			}
		}
	}

	ws = -1, we = -1;
	hs = -1, he = -1;
	for ( i=0; i<width; i++ )
	{
		// 人脸width start下标
		if ( ci[i]>0 && ws<0 )
			ws = i;
		// 人脸width end下标
		else if ( ws>=0 )
		{
			if ( ci[i]<=0 || i==width-1 )
			{
				we = i;
				break;
			}
		}
	}
	for ( i=0; i<height; i++ )
	{
		// 人脸height start下标
		if ( cj[i]>0 && hs<0 )
			hs = i;
		// 人脸height end下标
		else if ( hs>=0 )
		{
			if ( cj[i]<=0 || i==height-1 )
			{
				double percent = 1.0*(i-hs)/(we-ws);
				if ( percent > 0.5 && percent < 1.3 )
					he = i;
				else
					he = hs+(int)(1.2*(we-ws));
				break;
			}
		}
	}

	// 分割出的人脸
	if ( ws>0 && ws<we && hs>0 && hs<he )
	{
		/*
		// 画出分割的人脸top/bottom width
		for ( i=ws; i<we; i++ )
		{
			SetPixel(hWinDC, 680+i, 20+hs, RGB(255, 0, 0));
			SetPixel(hWinDC, 680+i, 20+he, RGB(255, 0, 0));
		}
		// 画出分割的人脸left/right height
		for ( i=hs; i<he; i++ )
		{
			SetPixel(hWinDC, 680+ws, 20+i, RGB(255, 0, 0));
			SetPixel(hWinDC, 680+we, 20+i, RGB(255, 0, 0));
		}
		*/
		// copy face to Image
		fWidth = we - ws;
		fHeight = he - hs;
		for ( j=hs; j<he; j++ )
		{
			for ( i=ws; i<we; i++ )
			{
				fImage[(j-hs)*(we-ws)*3+(i-ws)*3] = fImage[j*width*3+i*3];
				fImage[(j-hs)*(we-ws)*3+(i-ws)*3+1] = fImage[j*width*3+i*3+1];
				fImage[(j-hs)*(we-ws)*3+(i-ws)*3+2] = fImage[j*width*3+i*3+2];
			}
		}
		return true;
	} // end if ()
	else
		return false;
}

/***********************************************************************************
*																				   *
*	提取image(width, height)第(m,n)block第(i,j)pixel点在领域内LBP值				   *
*	count：LBP模式二值序列中0->1或1->0变化次数，								   *
*	count<=2，ULBP模式															   *
*	r：半径为r的环形领域														   *
*	p：环形领域圆周上的像素点数													   *
*																				   *
***********************************************************************************/
BYTE PixelLbp(char *image, int width, int height, 
			  int m, int n, int bw, int bh, int i, int j)
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
	coff = (BYTE)image[(bh*n+j)*width*3+(bw*m+i)*3];
	// 与环形圆周上的8个领域像素点比较，得二值序列（LBP模式）
	for ( k=0; k<p; k++ )
	{
		// 均匀分布在环形圆周上的像素点在block中的下标
		x = (int)(i + r*sin(2*pi*k/p) + 0.5);
		y = (int)(j - r*cos(2*pi*k/p) + 0.5);
		// 像素下标从block坐标系转换到image坐标系
		x += bw * m;
		y += bh * n;
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

/********************************************************************************************
*																							*
*    旋转不变性LBP，循环移位得当前LBP各个模式，取最小值LBP									*
*    n : lbp模式二进制位数，模式位宽														*
*																							*
********************************************************************************************/
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

/******************************************************************************************
*																						  *
*	提取灰度图像纹理的LBP特征，统计LBP特征直方图表征人脸								  *
*																						  *
******************************************************************************************/
void ExtractImageLbp(char *image, int width, int height, int *LBP )
{
	int i, j, k;
	int n, m, blkCount;
	int bw, bh;
	BYTE lbp;
	int ULBPtable[256];
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
		// uniform LBP 索引映射表
		if ( count <= 2 )
			ULBPtable[i] = k++;
	}

	// keep原image临时空间
	tmpImage = (char *)malloc(width*height*3);
	// copy image信息到临时空间
	memcpy(tmpImage, image, width*height*3);
	
	// image分块提取LBP，blkCount*blkCount blocks
	blkCount = 7;
	bw = width / blkCount;
	bh = height / blkCount;
	memset(LBP, 0, 59*blkCount*blkCount*sizeof(int));
	for ( n=0; n<blkCount; n++ )
	{
		for ( m=0; m<blkCount; m++ )
		{
			// 分块提取LBP
			for ( j=0; j<bh; j++ )
			{
				for ( i=0; i<bw; i++ )
				{
					// 提取image(blkCount*blkCount)分块中block(m,n)的pixel(i,j)的LBP
					lbp = PixelLbp(tmpImage, width, height, m, n, bw, bh, i, j);
					// 旋转不变性LBP
			//		lbp = MinLbp(lbp, 8);
					// LBP二值序列（LBP模式）对应的10进制数作LBP值
					image[(bh*n+j)*width*3+(bw*m+i)*3] = lbp;

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

/****************************************************************************************
*																						*
*	卡方统计两个图像LBP直方图特征的相似度												*
*	χ^2 (S,M) = ∑ { (Si-Mi)^2 / (Si+Mi) }												*
*	     		 i																		*
****************************************************************************************/
int ChiSquareStatistic(int *dstLBP, int *baseLBP, int len)
{
	int i, j;
	int w[49];
	double x2;

	// allocate weights for chi square statistic of LBPs
	for ( i=0; i<49; i++ )
		w[i] = 1;
	// weight 4 for eyes
	w[7*1+1] = w[7*1+2] = w[7*2+1] = w[7*2+2] = 4;
	w[7*1+4] = w[7*1+5] = w[7*2+4] = w[7*2+5] = 4;
	// weight 2 for mouth and 额头两侧
	w[7*4+3] = 2;
	w[0] = w[7*1+0] = 2;
	w[6] = w[7*1+6] = 2;
	// weight 0 for nose and 脸下巴两侧
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

/********************************************************************************************
*																							*
*	遍历目录facebaseDir所有文件，包括目录													*
*	处理获取到的.bmp图像文件，得到LBP，与													*
*	目标查找face的dstLBp处理得相似度量差，													*
*	根据给定阈值，判断是否为同一人脸，返回													*
*	判断结果																				*
*																							*
********************************************************************************************/
bool SearchFace(char *facebaseDir, int *dstLBP)
{
	char findFileName[100];
	char strFilter[1024];
	char findFilePath[1024];
	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile;

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
		ReadBmpFile(findFilePath, Image);
		
		if ( !ExtractFace(Image, ImageWidth, ImageHeight, FaceWidth, FaceHeight) )
			continue;
		// normalize face image to 70 70 size
		NormalizeImageSize(NewImage, 70, 70, Image, FaceWidth, FaceHeight);
		ShowBmpImage(NewImage, 70, 70, 750, 20);
		RgbToYcbcr(NewImage, NewImage, 70, 70);
		ExtractImageLbp(NewImage, 70, 70, LBP);
		ShowBmpGreyImage(NewImage, 70, 70, 750, 100);

		int x2;
		char strx2[50];
		// LBP卡方统计相似度量值与设定阈值对比，小于默认阈值时，图像相同
		if ( (x2=ChiSquareStatistic(dstLBP, LBP, 49*59)) < 3000 )
		{
			wsprintf(strx2, "%s%d", "x^2 = ", x2);
			TextOut(hWinDC, 700, 180, strx2, strlen(strx2));
			ReadBmpFile(findFilePath, Image);
			ShowBmpImage(Image, ImageWidth, ImageHeight, 660, 200);
			
			FindClose(hFindFile);
			return true;
		}

	} while ( FindNextFile(hFindFile, &findFileData) );
	FindClose(hFindFile);

	// Update Show Rect()
	RECT rt;
	rt.left = 750, rt.top = 20;
	rt.right = 1000, rt.bottom = 200;
	InvalidateRect(hMainWnd, &rt, true);

	return false;
}


/******************************************************************************************************
*																									  *
*	在人脸库中查找出相似度量比较接近目标人脸的图片													  *
*																									  *
******************************************************************************************************/
bool RecognizeFace(char *Image, int width, int height, char *facebasePath)
{
	if ( !ExtractFace(Image, width, height, FaceWidth, FaceHeight) )
		return false;

	// normalize face image to 70 70 size
	NormalizeImageSize(NewImage, 70, 70, Image, FaceWidth, FaceHeight);
	ShowBmpImage(NewImage, 70, 70, 660, 20);
	RgbToYcbcr(NewImage, NewImage, 70, 70);
	ExtractImageLbp(NewImage, 70, 70, dstLBP);
	ShowBmpGreyImage(NewImage, 70, 70, 660, 100);

	return SearchFace(facebasePath, dstLBP);
}

/*****************************************************************************************************
*																									 *	
*	对图像处理后，提取人脸																			 *
*																									 *	
*****************************************************************************************************/
bool ExtractFace(char *image, int width, int height, int &fWidth, int &fHeight)
{
	// RGB色彩空间 --> YCbCr空间转换
	RgbToYcbcr(image, NewImage, width, height);
	// 预处理YCbCr空间图像得到人脸候选区域的二值化图像
	FaceDetect(NewImage, width, height);
	
	// 开运算处理，先腐蚀后膨胀
	Erode(NewImage, width, height);
	Expand(NewImage, width, height);
	// 去掉非人脸噪音，默认像素数小于阈值时去掉
	FilterNoise(NewImage, width, height);

	// 投影法，计算人脸肤色在width，height的位置投影下标，分割人脸
	return CountFacePixel(NewImage, width, height, image, fWidth, fHeight);
}

/************************************************************************************************
*																								*
*	将捕获到的人脸图像及录入人脸库																*
*																								*
************************************************************************************************/
bool EnterFace(char *imgFileName, char *facebasePath)
{
	char fileName[1024];
	char strFilter[1024];
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

	int t=0;
	while (1)
	{
		// 失败次数超过1000后，照片录入失败
		if ( t++ >= 1000 )
			return false;

		wsprintf(fileName, "%s%s%d%s", facebasePath, "\\face", count++, ".bmp");
		if ( CopyFile(imgFileName, fileName, 1) )
			break;
	}

	return true;
}

void DeleteFace()
{
	char facePath[256];
	char FileTitle[100], ImgDlgFileDir[256];
	char SelImgFileDir[256];
	OPENFILENAME ofn;

	// 指定选择对话框打开人脸库目录
	strcpy(ImgDlgFileDir, curDir);
	strcat(ImgDlgFileDir, "\\Facebase");

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
	ofn.lpstrFileTitle=FileTitle;
	ofn.nMaxFileTitle=99;
	ofn.lpstrInitialDir=ImgDlgFileDir;
	ofn.lpstrTitle="选择要移除的人脸样本图像";
	ofn.Flags=OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt="raw";
	ofn.lCustData=NULL;
	ofn.lpfnHook=NULL;
	ofn.lpTemplateName=NULL;
	facePath[0]='\0';
	GetOpenFileName(&ofn); 

	getcwd(SelImgFileDir, MAX_PATH);

	// 路径空时，取消
	if ( !strlen(facePath) )
		return ;

	TextOut(hWinDC, 660, 20, "已选择待移除的人脸样本：", strlen("已选择待移除的人脸样本："));
	ReadBmpFile(facePath, Image);
	ShowBmpImage(Image, ImageWidth, ImageHeight, 660, 50);
	ExtractFace(Image, ImageWidth, ImageHeight, FaceWidth, FaceHeight);
	// normalize face image to 70 70 size
	NormalizeImageSize(NewImage, 70, 70, Image, FaceWidth, FaceHeight);
	ShowBmpImage(NewImage, 70, 70, 730, ImageHeight+80);
	RgbToYcbcr(NewImage, NewImage, 70, 70);
	ExtractImageLbp(NewImage, 70, 70, dstLBP);
	ShowBmpGreyImage(NewImage, 70, 70, 850, ImageHeight+80);

	if ( strcmp(SelImgFileDir, ImgDlgFileDir) )
	{
		MessageBox(hMainWnd, "移除失败，选择的不是人脸取样库目录...\\Facebase下的图像", "移除人脸样本", 0);
		return ;
	}

	if ( MessageBox(hMainWnd, "确定移除所选人脸样本图像？", "移除人脸样本", MB_OKCANCEL) == IDCANCEL )
	{
		// Update Show Rect()
		RECT rt;
		rt.left = 650, rt.top = 20;
		rt.right = 1000, rt.bottom = 500;
		InvalidateRect(hMainWnd, &rt, true);
		return ;
	}

	// Update Show Rect()
	RECT rt;
	rt.left = 650, rt.top = 20;
	rt.right = 1000, rt.bottom = 500;
	InvalidateRect(hMainWnd, &rt, true);

	if ( !DeleteFile(facePath) )
	{
		MessageBox(hMainWnd, "移除目标人脸样本图像失败！", "移除人脸样本", 0);
		return ;
	}
	MessageBox(hMainWnd, "移除目标人脸样本成功！", "移除人脸样本", 0);
}

/***********************************************************************************************
*																							   *
*	图片膨胀处理																			   *
*																							   *
***********************************************************************************************/
void Expand(char *image, int width, int height)
{
	int i, j, m, n;
	int coff;
	int B[9] = {1, 0, 1,
			    0, 0, 0,
		        1, 0, 1};
	char *tmpImage;

	tmpImage = (char *)malloc(width*height*3);
	memcpy(tmpImage, image, width*height*3);

	for ( j=1; j<height-1; j++ )
	{
		for ( i=1; i<width-1; i++ )
		{
			for ( m=0; m<3; m++ )
			{
				for ( n=0; n<3; n++ )
				{
					if ( B[m+n] == 1 )
						continue;
					// top/bottom/left/right exist same pixel to target face, expand
					coff = (BYTE)image[(j+m-1)*width*3+(i+n-1)*3];
					if ( coff == 255 )
					{
						tmpImage[j*width*3+i*3] = (char)255;
						goto EXPAND_BREAK_I_LOOP;
					}
				}
			}
EXPAND_BREAK_I_LOOP: ;
		}
	}
	memcpy(image, tmpImage, width*height*3);
	free(tmpImage);
}

/**********************************************************************************************
*																							  *
*	图片腐蚀处理																			  *
*																							  *	
**********************************************************************************************/
void Erode(char *image, int width, int height)
{
	
	int i, j, m, n;
	int coff;
	int B[9] = {1, 0, 1,
			    0, 0, 0,
				1, 0, 1};
	char *tmpImage;

	tmpImage = (char *)malloc(width*height*3);
	memcpy(tmpImage, image, width*height*3);

	for ( j=1; j<height-1; j++ )
	{
		for ( i=1; i<width-1; i++ )
		{
			for ( m=0; m<3; m++ )
			{
				for ( n=0; n<3; n++ )
				{
					if ( B[m+n] == 1 )
						continue;
					// top/bottom/left/right exist same pixel to  background, erode
					coff = (BYTE)image[(j+m-1)*width*3+(i+n-1)*3];
					if ( coff == 0 )
					{
						tmpImage[j*width*3+i*3] = (char)0;
						goto ERODE__BREAK_I_LOOP;
					}
				}
			}
ERODE__BREAK_I_LOOP: ;
		}
	}
	memcpy(image, tmpImage, width*height*3);
	free(tmpImage);
}

/*******************************************************************************************
*	过滤掉小块人脸候选区域背景噪音														   *
*	肤色像素点数小于5000时，假定为非人脸区域											   *	
*	用队列广度优先搜索的方式来统计肤色像素点数											   *	 	
*																						   *	
*******************************************************************************************/
void FilterNoise(char *image, int width, int height)
{
	int i, j, k;
	int y;
	// 肤色像素点计数变量
	int count;
	int m, n;
	// 小块非人脸肤色区域最大范围
	int iMin, iMax, jMin, jMax;
	// 队列首尾下标指针
	int tail, head;
	// 肤色像素点访问队列
	int iQue[320*240];
	int jQue[320*20];
	// 像素点访问标志
	int flagVisited[320][240] = {0}; 
	// 上下左右四周8个像素点的下标差值关系
	int a[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	int b[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

	for ( j=0; j<height; j++ )
	{
		for ( i=0; i<width; i++ )
		{
			y = (BYTE)image[j*width*3+i*3];
			// 找第一个肤色像素点，开始广搜计数
			if ( y == 255 && flagVisited[i][j] == 0 )
			{
				count = 0;
				iMin = width;
				jMin = height;
				iMax = 0;
				jMax = 0;
				// 队列初始化
				head = 0;
				tail = 0;
				// 当前第一个肤色像素点入队
				iQue[tail] = i;
				jQue[tail++] = j;
				// 标记访问记录
				flagVisited[i][j] = 1;
				// 队列空？
				while ( head < tail )
				{
					count++;
					// 队首出列
					m = iQue[head];
					n = jQue[head++];
					if ( m > iMax )
						iMax = m;
					if ( m < iMin )
						iMin = m;
					if ( n > jMax )
						jMax = n;
					if ( n < jMin )
						jMin = n;
					// 往四周8个像素点广搜
					for ( k=0; k<8; k++ )
					{
						// 四周8个点下标位置
						m = iQue[head-1] + a[k];
						n = jQue[head-1] + b[k];
						// 防止越界
						if ( m<0 || m>=width || n<0 || n>=height )
							continue;
						// 新访问点？
						if ( flagVisited[m][n] == 1 )
							continue;
						// 肤色像素点？
						y = (BYTE)image[n*width*3+m*3];
						if ( y != 255 )
							continue;
						// 队尾入列
						iQue[tail] = m;
						jQue[tail++] = n;
						// 标记访问记录
						flagVisited[m][n] = 1;
					}
				} // end while ( head < tail )

				// 过滤小块非人脸肤色区域
				if ( count < 1000 )
				{
					for ( n=jMin; n<=jMax; n++ )
						for ( m=iMin; m<=iMax; m++ )
							image[n*width*3+m*3] = 0;
				}
			} // end if ( y == 255 && flagVisited[i][j] == 0 )
			// 访问记录
			else
				flagVisited[i][j] = 1;
		}
	}
}
