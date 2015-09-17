// FaceDetect.h：基于人脸肤色进行人脸检测相关函数声明
// Copyright: cent
// 2015.9.17
// ~

#ifndef FACEDETECT_H
#define FACEDETECT_H



// 得到人脸肤色cbcr[cb][cr]对比库
bool FaceCbcrProc(char *sampleImagePath);

// RGB色彩空间模型转换到YCbCr空间模型
void RgbToYcbcr(BmpImage *pDstImage, BmpImage *pSrcImage);

// 提取人脸
BmpImage* ExtractFace(BmpImage *pImage);



#endif