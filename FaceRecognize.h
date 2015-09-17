// FaceRecognize.h：基于人脸图像纹理的LBP（局部二值模式）特征进行人脸识别相关函数声明
// Copyright: cent
// 2015.9.17
// ~

#ifndef FACERECOGNIZE_H
#define FACERECOGNIZE_H



// 识别人脸，库中找出目标人脸
bool RecognizeFace(BmpImage *pImage, char *facebasePath);

// 照片录入人脸库
bool EnterFace(char *imgFileName, char *facebasePath);

// 移除人脸取样
void DeleteFace(char *facebasePath);



#endif