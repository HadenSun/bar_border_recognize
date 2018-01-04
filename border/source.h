#ifndef _SOURCE_H
#define _SOURCE_H

#include "core/core.hpp"
#include "highgui/highgui.hpp"
#include "imgproc/imgproc.hpp"
#include <math.h>

#define PI 3.1415926

using namespace std;
using namespace cv;

#define ZIP 1
#define ZIPTIME 0.17

int findThresholdAverage(MatND hist);
bool IsDimodal(double HistGram[]);
int findThresholdVally(MatND hist);
MatND myCalcHist(Mat imageGray, int isShow);
int findDirection(Mat &inputImageX, Mat &inputImageY, Mat &outputImage, int type);
int eraseBackground(Mat &inputImage, Mat &outputImage, int threshold);
void myImShow(char *imageName, Mat &image, int isZip, int isSave);
bool findBloak(Mat &image, Rect &rect, Rect & rectOut);


#endif // !_SOURCE_H
