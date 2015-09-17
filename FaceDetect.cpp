// FaceDetect.cpp：基于人脸肤色进行人脸检测相关函数定义
// Copyright: cent
// 2015.9.17
// ~
//

#include "stdafx.h"
#include "resource.h"
#include <commdlg.h>
#include <direct.h>
#include <math.h>
#include "image.h"
#include "FaceDetect.h"

int cbcr[256][256];    // 人脸肤色CbCr范围统计库

/************************************************************************************
	静态函数声明

************************************************************************************/
// RGB色彩空间模型转换到YCbCr空间模型
static void RgbToYcbcr(BmpImage *pDstImage, BmpImage *pSrcImage);

// 图片膨胀处理
static void Expand(BmpImage *pImage);
// 图片腐蚀处理
static void Erode(BmpImage *pImage);
// 过滤掉小块人脸候选区域背景噪音
static void FilterNoise(BmpImage *pImage);

// 处理FaceSample.bmp人脸肤色样本库
static void FaceSampleCbcr(BmpImage *pImage);
// 根据cb,cr肤色范围得到人脸肤色对比库
static void FaceCbcr();
// 从YCbCr空间模型图像中检测出人脸
static void FaceDetect(BmpImage *pImage);
// 投影法分割人脸
static BmpImage* SplitFace(BmpImage *pImage, BmpImage *pYcbcrImage);



/****************************************************************************************************
*																								    *	
*   将位图RGB色彩空间模型转换到YCbCr色彩空间模型函数												*
*																								    *
*   输入参数：pSrcImage - RGB色彩空间模型的原始位图结构指针										    *
*   输出参数：pDstImage - YCbCr色彩空间模型的目标位图结构指针									    *
*																									*
****************************************************************************************************/
static void RgbToYcbcr(BmpImage *pDstImage, BmpImage *pSrcImage)
{
	int i, j;
	int r, g, b;
	int y, cr, cb;

	// 得到位图尺寸
	pDstImage->width = pSrcImage->width;
	pDstImage->height = pSrcImage->height;

	// 色彩空间模型转换
	for ( j=0; j<pSrcImage->height; j++ )
	{
		for ( i=0; i<pSrcImage->width; i++ )
		{
			r = (BYTE)pSrcImage->data[j*pSrcImage->width*3+i*3];
			g = (BYTE)pSrcImage->data[j*pSrcImage->width*3+i*3+1];
			b = (BYTE)pSrcImage->data[j*pSrcImage->width*3+i*3+2];
			
			/*****************************************************************\
			  RGB to YCbCr:
			| Y  |    | 16  |             |   65.738  129.057   25.06 |   | R |
			| Cb |  = | 128 | + (1/256) * |  -37.945  -74.494  112.43 | * | G |
			| Cr |    | 128 |             |  112.439  -94.154  -18.28 |   | B |
			
			\*****************************************************************/
			y  = (int)(16 + (65.738*r + 129.057*g + 25.06*b) / 256);
			cb = (int)(128 + (-37.945*r - 74.494*g + 112.43*b) / 256);
			cr = (int)(128 + (112.439*r - 94.154*g - 18.28*b) / 256);
			
			pDstImage->data[j*pDstImage->width*3+i*3] = y;
			pDstImage->data[j*pDstImage->width*3+i*3+1] = cb;
			pDstImage->data[j*pDstImage->width*3+i*3+2] = cr;
		}
	}
}

/*****************************************************************************************
*																						 *
*   由人脸样本肤色位图FaceSample.bmp获取人脸肤色cbcr[cb][cr]对比库函数					 *
*																				 	     *
*   输入参数：pImage   - YCbCr色彩空间模型的人脸样本肤色位图结构指针					 *
*   输出参数：cbcr[][] - 人脸肤色CbCr范围统计库                							 *
*																						 *		
*****************************************************************************************/
static void FaceSampleCbcr(BmpImage *pImage)
{
	int i, j;
	int y, cb, cr;

	for ( j=0; j<pImage->height; j++ )
	{
		for ( i=0; i<pImage->width; i++ )
		{
			y  = (BYTE)pImage->data[j*pImage->width*3+i*3];
			cb = (BYTE)pImage->data[j*pImage->width*3+i*3+1];
			cr = (BYTE)pImage->data[j*pImage->width*3+i*3+2];

			// 落入肤色范围，标记对应cb，cr值
			if ( cb>98 && cb<123 && cr>133 && cr<169 )
			    cbcr[cb][cr] = 1;
		}
	}
}

/****************************************************************************************
*																						*
*	直接由肤色cb,cr范围得到人脸肤色的cbcr[cb][cr]对比库函数								*
*																						*
*   输出参数：cbcr[][] - 人脸肤色CbCr范围统计库                							*
*																						*
****************************************************************************************/
static void FaceCbcr()
{
	int i, j;

	// 肤色范围：98<cb<123 && 133<cr<169
	// 标记人脸样本肤色对应cb，cr值
	for ( i=98; i<123; i++ )
		for ( j=133; j<169; j++ )
			cbcr[i][j] = 1;
}

/****************************************************************************************
*																						*
*	获取人脸肤色的cbcr[cb][cr]对比库函数												*
*   人脸样本肤色位图FaceSample.bmp存在时，由样本获取，不存在时，直接由肤色范围取得      *
*																						*
*   输出参数：cbcr[][] - 人脸肤色CbCr范围统计库                							*
*																						*
****************************************************************************************/
bool FaceCbcrProc(char *sampleImagePath)
{
	OFSTRUCT of;
	BmpImage *image;		

	// 尝试打开文件，判断文件是否存在
	// 返回负数文件不存在，存在时，关闭文件，返回已无效句柄
	if ( OpenFile(sampleImagePath, &of, OF_EXIST) < 0 )
	{
		// 未找到肤色样本图时，直接由Cb，Cr肤色范围得到人脸肤色对比库
		FaceCbcr();
		return true;
	}

	// 打开FaceSample.bmp肤色样本图
	image = ReadBmpFile(sampleImagePath)
	if ( image == NULL )
		return false;

	RgbToYcbcr(image, image);
	// 由样本肤色Cb，Cr得到人脸肤色对比库
	FaceSampleCbcr(image);
	
	// 释放ReadBmpFile动态生成的位图空间
	FreeBmpImage(image);

	return true;
}

/*****************************************************************************************
*																						 *
*	YCbCr空间检测出人脸候选区域函数，位图中肤色区域灰度值标记255（白色）	    		 *
*																						 *
*   输入输出参数：pImage - YCbCr色彩空间模型的位图结构指针     							 *
*																						 *
*****************************************************************************************/
void FaceDetect(BmpImage *pImage)
{
	int i, j;
	int y, cb, cr;

	for ( j=0; j<pImage->height; j++ )
	{
		for ( i=0; i<pImage->width; i++ )
		{
			y = (BYTE)pImage->data[j*pImage->width*3+i*3];
			cb = (BYTE)pImage->data[j*pImage->width*3+i*3+1];
			cr = (BYTE)pImage->data[j*pImage->width*3+i*3+2];

			// 肤色范围
		//	if ( cb>98 && cb<123 && cr>133 && cr<169 )

			// 标记肤色像素
			if ( cbcr[cb][cr] == 1 )
				y = 255; //white
			else
				y = 0; //black

			pImage->data[j*pImage->width*3+i*3] = y;
			pImage->data[j*pImage->width*3+i*3+1] = cb;
			pImage->data[j*pImage->width*3+i*3+2] = cr;
		}
	}
}

/*****************************************************************************************
*																						 *
*	投影法分割人脸位置函数                                                               *
*   肤色像素投影到x，y轴上，统计到对应下标肤色像素点数									 *
*																						 *
*   输入参数：pSrcImage   - 原始位图结构指针						                     *
*   输入参数：pYcbcrImage - 肤色区域灰度值标记255的YCbCr位图结构指针					 *
*   返回值  ：              指针人脸位图结构指针                   						 *
*																						 *
*****************************************************************************************/
static BmpImage* SplitFace(BmpImage *pSrcImage, BmpImage *pYcbcrImage)
{
	int i, j;
	int y;
	int widthStart, widthEnd, heightStart, heightEnd;
	int *ci;
	int *cj;

	ci = (int *)malloc(pYcbcrImage->width*sizeof(int));
	if( ci == NULL )
		return NULL;
		
	cj = (int *)malloc(pYcbcrImage->height*sizeof(int));
	if( cj == NULL )
	{
		free(ci);
		return NULL;
	}
	
	// 初始化两坐标轴上肤色像素统计值
	memset(ci, 0, sizeof(ci));
	memset(cj, 0, sizeof(cj));

	for ( j=0; j<pYcbcrImage->height; j++ )
	{
		for ( i=0; i<pYcbcrImage->width; i++ )
		{
			// 取YCbCr位图灰度值
			y = (BYTE)pYcbcrImage->data[j*pYcbcrImage->width*3+i*3];

			// 人脸肤色像素投影到两坐标轴上统计
			if ( y == 255 )
			{
				ci[i]++;
				cj[j]++;
			}
		}
	}

	// 得到x轴人脸开始下标
	for ( widthStart=0; widthStart < pYcbcrImage->width; widthStart++ )
	{
		if ( ci[widthStart] > 0 )
			break;
	}
	// 得到x轴人脸结束下标
	for ( widthEnd=pYcbcrImage->width-1; widthEnd>=0; widthEnd-- )
	{
		if ( ci[widthEnd] > 0 )
			break;
	}

	// 得到y轴人脸开始下标
	for ( heightStart=0; heightStart < pYcbcrImage->height; heightStart++ )
	{
		if ( cj[heightStart] > 0 )
			break;
	}

	// 得到y轴人脸结束下标
	for ( heightEnd=pYcbcrImage->height-1; heightEnd>=0; heightEnd-- )
	{
		if ( cj[heightEnd] > 0 )
		{
			double rate;
		
			// 人脸长宽比例
			if ( widthEnd - widthStart > 0 )
				rate = 1.0*(heightEnd-heightStart)/(widthEnd-widthStart);
			else
			{
				free(ci);
				free(cj);
				return NULL;
			}

			// 人脸长宽比例不在普遍范围内，作适当调整
			if ( rate < 0.5 || rate > 1.3 )
				heightEnd = heightStart+(int)(1.2*(widthEnd-widthStart));
			break;
		}
	}

	// 人脸范围是否非法
	if ( !(widthStart>=0 && widthStart<widthEnd && 
	      heightStart>=0 && heightStart<heightEnd && heightEnd<pYcbcrImage->height) )
	{
		free(ci);
		free(cj);
		return NULL;
	}
/*
	// 画出分割的人脸top/bottom width
	for ( i=widthStart; i<widthEnd; i++ )
	{
		SetPixel(hWinDC, 680+i, 20+heightStart, RGB(255, 0, 0));
		SetPixel(hWinDC, 680+i, 20+heightEnd, RGB(255, 0, 0));
	}
	// 画出分割的人脸left/right height
	for ( i=heightStart; i<heightEnd; i++ )
	{
		SetPixel(hWinDC, 680+widthStart, 20+i, RGB(255, 0, 0));
		SetPixel(hWinDC, 680+widthEnd, 20+i, RGB(255, 0, 0));
	}
*/
	// 分配人脸位图结构，尺寸为：(widthEnd-widthStart) * (heightEnd-heightStart)
	pFaceImage = MallocBmpImage(widthEnd-widthStart, heightEnd-heightStart);
	if(pFaceImage == NULL)
	{
		free(ci);
		free(cj);
		return NULL;
	}
		
	for ( j=heightStart; j<heightEnd; j++ )
	{
		for ( i=widthStart; i<widthEnd; i++ )
		{
			// 分割出的人脸
			pFaceImage->data[(j-heightStart)*(widthEnd-widthStart)*3+(i-widthStart)*3] 
				= pSrcImage->data[j*pSrcImage->width*3+i*3];
			pFaceImage->data[(j-heightStart)*(widthEnd-widthStart)*3+(i-widthStart)*3+1]
				= pSrcImage->data[j*pSrcImage->width*3+i*3+1];
			pFaceImage->data[(j-heightStart)*(widthEnd-widthStart)*3+(i-widthStart)*3+2]
				= pSrcImage->data[j*pSrcImage->width*3+i*3+2];
		}
	}
	
	free(ci);
	free(cj);
	return pFaceImage;
}

/*****************************************************************************************
*																						 *	
*	提取人脸位图函数 																	 *
*																						 *
*   输入参数：pImage       - 目标位图结构指针   	               						 *
*   返回值  ：               指向尺寸为70*70的人脸位图指针               				 *
*																						 *	
*****************************************************************************************/
BmpImage* ExtractFace(BmpImage *pImage)
{
	BmpImage *ycbcrImage, *pFaceImage;
		
	ycbcrImage = MallocBmpImage(pImage->width, pImage->height);
	if (ycbcrImage == NULL)
		return NULL;

	// RGB色彩空间 --> YCbCr空间转换
	RgbToYcbcr(ycbcrImage, pImage);
	// 预处理YCbCr空间图像得到人脸候选区域的二值化图像
	FaceDetect(ycbcrImage);
	
	// 开运算处理，先腐蚀后膨胀
	Erode(ycbcrImage);
	Expand(ycbcrImage);

	// 去掉非人脸噪音，默认像素数小于阈值时去掉
	FilterNoise(ycbcrImage);

	// 分割人脸，投影法：人脸投影到width，height坐标上，得出下标位置
	pFaceImage = SplitFace(pImage, ycbcrImage);
	if( pFaceImage == NULL )
		return NULL;
	
	// 人脸限定缩放尺寸为：70 * 70
	NormalizeImageSize(pFaceImage, 70, 70);
	
	FreeBmpImage(ycbcrImage);
	return pFaceImage;
}

/*****************************************************************************************
*																						 *
*	图片膨胀处理函数																	 *
*																						 *
*   输入输出参数：pImage  - 位图结构指针                               					 *
*																						 *
*****************************************************************************************/
static void Expand(BmpImage *pImage)
{
	int i, j, m, n;
	int coff;
	int B[9] = {1, 0, 1,
			    0, 0, 0,
		        1, 0, 1};
	char *tmpImage;

	tmpImage = (char *)malloc(pImage->width*pImage->height*3);
	memcpy(tmpImage, pImage->data, pImage->width*pImage->height*3);

	for ( j=1; j<pImage->height-1; j++ )
	{
		for ( i=1; i<pImage->width-1; i++ )
		{
			// 上下左右中存在与目标灰度值255一致像素，同化当前像素，即膨胀
			for ( m=0; m<3; m++ )
			{
				for ( n=0; n<3; n++ )
				{
					if ( B[m+n] == 1 )
						continue;
						
					coff = (BYTE)pImage->data[(j+m-1)*pImage->width*3+(i+n-1)*3];
					if ( coff == 255 )
					{
						tmpImage[j*pImage->width*3+i*3] = (char)255;
						// 继续处理下一个像素
						goto EXPAND_BREAK_I_LOOP;
					}
				}
			}
EXPAND_BREAK_I_LOOP: ;
		}
	}
	memcpy(pImage->data, tmpImage, pImage->width*pImage->height*3);
	free(tmpImage);
}

/*****************************************************************************************
*																						 *
*	图片腐蚀处理函数																	 *
*																						 *
*   输入输出参数：pImage  - 位图结构指针                               					 *
*																						 *
*****************************************************************************************/
static void Erode(BmpImage *pImage)
{
	
	int i, j, m, n;
	int coff;
	int B[9] = {1, 0, 1,
			    0, 0, 0,
				1, 0, 1};
	char *tmpImage;

	tmpImage = (char *)malloc(pImage->width*pImage->height*3);
	memcpy(tmpImage, pImage->data, pImage->width*pImage->height*3);

	for ( j=1; j<pImage->height-1; j++ )
	{
		for ( i=1; i<pImage->width-1; i++ )
		{
			// 上下左右中存在与背景灰度值0一致像素，同化当前像素，即腐蚀
			for ( m=0; m<3; m++ )
			{
				for ( n=0; n<3; n++ )
				{
					if ( B[m+n] == 1 )
						continue;
					
					coff = (BYTE)pImage->data[(j+m-1)*pImage->width*3+(i+n-1)*3];
					if ( coff == 0 )
					{
						tmpImage[j*pImage->width*3+i*3] = (char)0;
						// 继续处理下一个像素
						goto ERODE__BREAK_I_LOOP;
					}
				}
			}
ERODE__BREAK_I_LOOP: ;
		}
	}
	memcpy(pImage->data, tmpImage, pImage->width*pImage->height*3);
	free(tmpImage);
}

/*****************************************************************************************
*                                                                                        *
*	过滤背景噪音函数								                					 *
*	肤色像素点数小于5000时，假定为非人脸区域											 *	
*	用队列广度优先搜索的方式来统计肤色像素点数											 *
*																						 *
*   输入输出参数：pImage  - 位图结构指针                               					 *
*																						 *	
*****************************************************************************************/
static void FilterNoise(BmpImage *pImage)
{
	int i, j, k;
	int y;
	int count;                    // 肤色像素点计数变量
	int m, n;
	int iMin, iMax, jMin, jMax;   // 小块非人脸肤色区域最大范围
	int tail, head;               // 队列首尾下标指针
	
	int *iQue;            // 肤色像素点访问队列
	int *jQue;
	char *flagVisited;  // 像素点访问标志
	
	// 上下左右四周8个像素点的下标差值关系
	int a[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	int b[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

	iQue = (int *)malloc(pImage->width*pImage->height*sizeof(int));
	if ( iQue == NULL )
		return ;

	jQue = (int *)malloc(pImage->width*pImage->height*sizeof(int));
	if ( jQue == NULL )
	{
		free(iQue);
		return ;
	}

	flagVisited = (char *)malloc(pImage->width*pImage->height);
	if ( flagVisited == NULL )
	{
		free(iQue);
		free(jQue);
		return ;
	}

	memset(flagVisited, '0', pImage->width*pImage->height);
	for ( j=0; j<pImage->height; j++ )
	{
		for ( i=0; i<pImage->width; i++ )
		{
			y = (BYTE)pImage->data[j*pImage->width*3+i*3];
			
			// 找第一个肤色像素点，开始广搜计数
			if ( y == 255 && flagVisited[j*pImage->width+i] == '0' )
			{
				count = 0;
				iMin = pImage->width;
				jMin = pImage->height;
				iMax = 0;
				jMax = 0;
				
				// 队列初始化
				head = 0;
				tail = 0;
				
				// 当前第一个肤色像素点入队
				iQue[tail] = i;
				jQue[tail++] = j;
				// 标记访问记录
				flagVisited[j*pImage->width+i] = '1';
				
				// 循环条件队列非空
				while ( head < tail )
				{
					count++;
					// 队首出列
					m = iQue[head];
					n = jQue[head++];
					
					// 更新最大范围
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
						if ( m<0 || m>=pImage->width || n<0 || n>=pImage->height )
							continue;
							
						// 是否已访问过
						if ( flagVisited[n*pImage->width+m] == '1' )
							continue;
							
						// 是否为肤色像素
						y = (BYTE)pImage->data[n*pImage->width*3+m*3];
						if ( y != 255 )
							continue;
							
						// 队尾入列
						iQue[tail] = m;
						jQue[tail++] = n;
						// 标记访问记录
						flagVisited[n*pImage->width+m] = '1';
					}
				} // end while ( head < tail )

				// 过滤小块非人脸肤色区域，假定小于1000个像素点
				if ( count < 1000 )
				{
					for ( n=jMin; n<=jMax; n++ )
						for ( m=iMin; m<=iMax; m++ )
							pImage->data[n*pImage->width*3+m*3] = 0;
				}
			} // end if ( y == 255 && flagVisited[i][j] == '0' )
		} // end for i
	} // end for j

	free(iQue);
	free(jQue);
	free(flagVisited);
}
