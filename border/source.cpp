#include "source.h"

//平均值法找二值化阈值
int findThresholdAverage(MatND hist)
{
	double histMaxValue;
	Point histMaxLoc;
	minMaxLoc(hist, 0, &histMaxValue, 0, &histMaxLoc);

	double avr = 0;
	double sum = 0;
	for (int i = 0;i < 255;i++)
	{
		sum += hist.at<float>(i);
		avr += (double)hist.at<float>(i) * i;
	}

	return (int)(avr / sum);
}

bool IsDimodal(double HistGram[])       // 检测直方图是否为双峰的  
{
	// 对直方图的峰进行计数，只有峰数位2才为双峰   
	int Count = 0;
	for (int Y = 1; Y < 255; Y++)
	{
		if (HistGram[Y - 1] < HistGram[Y] && HistGram[Y + 1] < HistGram[Y])
		{
			Count++;
			if (Count > 2) return false;
		}
	}
	if (Count == 2)
		return true;
	else
		return false;
}

//谷底最小值二值化阈值
int findThresholdVally(MatND hist)
{
	int Y, Iter = 0;
	double HistGramC[256];           // 基于精度问题，一定要用浮点数来处理，否则得不到正确的结果  
	double HistGramCC[256];          // 求均值的过程会破坏前面的数据，因此需要两份数据  
	for (Y = 0; Y < 256; Y++)
	{
		HistGramC[Y] = hist.at<float>(Y);
		HistGramCC[Y] = hist.at<float>(Y);
	}

	// 通过三点求均值来平滑直方图  
	while (IsDimodal(HistGramCC) == false)                                        // 判断是否已经是双峰的图像了        
	{
		HistGramCC[0] = (HistGramC[0] + HistGramC[0] + HistGramC[1]) / 3;                 // 第一点  
		for (Y = 1; Y < 255; Y++)
			HistGramCC[Y] = (HistGramC[Y - 1] + HistGramC[Y] + HistGramC[Y + 1]) / 3;     // 中间的点  
		HistGramCC[255] = (HistGramC[254] + HistGramC[255] + HistGramC[255]) / 3;         // 最后一点 
		memcpy(HistGramC, HistGramCC, sizeof(HistGramCC));
		Iter++;
		if (Iter >= 1000)
			return -1;                                                   // 直方图无法平滑为双峰的，返回错误代码  
	}
	// 阈值极为两峰之间的最小值   
	bool Peakfound = false;
	for (Y = 1; Y < 255; Y++)
	{
		if (HistGramCC[Y - 1] < HistGramCC[Y] && HistGramCC[Y + 1] < HistGramCC[Y]) Peakfound = true;
		if (Peakfound == true && HistGramCC[Y - 1] >= HistGramCC[Y] && HistGramCC[Y + 1] >= HistGramCC[Y])
			return Y - 1;
	}
	return -1;
}




//计算直方图
//isShow	-0 不绘制
//			-1 绘制
MatND myCalcHist(Mat imageGray, int isShow)
{
	//计算直方图
	int channels = 0;
	MatND dstHist;
	int histSize[] = { 256 };
	float midRanges[] = { 0,256 };
	const float *ranges[] = { midRanges };
	calcHist(&imageGray, 1, &channels, Mat(), dstHist, 1, histSize, ranges, true, false);
	//images(&imageGray):输入图像指针，可以多幅，要求必须同样深度
	//nimages(1):输入图像个数
	//channels(&channels):计算直方图的channels的数组
	//mask(Mat()):掩码，如果mask不为空，必须是一个8位（CV_8U）数组
	//hist(hstHist):每一维上直方图的元素个数
	//dims（1）：直方图维数
	//histSize（histSize)：直方图每维元素个数
	//ranges（ranges）：直方图每维范围
	//uniform（true）：ture说明需要计算的直方图的每一维按照它的范围和尺寸取均值
	//accumulate（false）：是否对hist清零，不清零计算多幅直方图累加

	if (isShow)
	{
		//绘制直方图,首先先创建一个黑底的图像，为了可以显示彩色，所以该绘制图像是一个8位的3通道图像    
		Mat drawImage = Mat::zeros(Size(256, 256), CV_8UC3);
		//任何一个图像的某个像素的总个数有可能会很多，甚至超出所定义的图像的尺寸，  
		//所以需要先对个数进行范围的限制，用minMaxLoc函数来得到计算直方图后的像素的最大个数    
		double g_dHistMaxValue;
		minMaxLoc(dstHist, 0, &g_dHistMaxValue, 0, 0);
		//将像素的个数整合到图像的最大范围内    
		for (int i = 1; i < 256; i++)
		{
			int value = cvRound(dstHist.at<float>(i) * 256 * 0.9 / g_dHistMaxValue);
			line(drawImage, Point(i, drawImage.rows - 1), Point(i, drawImage.rows - 1 - value), Scalar(0, 0, 255));
		}
		line(drawImage, Point(0, drawImage.rows - 1), Point(0, drawImage.rows - 1 - 0), Scalar(0, 0, 255));
		imshow("hist", drawImage);
	}


	return dstHist;
}

//查找梯度最多方向
//type：0-灰度图像
//		1-彩色图像
int findDirection(Mat &inputImageX, Mat &inputImageY, Mat &outputImage, int type)
{
	if (inputImageX.cols != inputImageY.cols)
		return -1;
	if (inputImageX.rows != inputImageY.rows)
		return -1;

	if (type)
		outputImage = Mat::zeros(inputImageX.rows, inputImageX.cols, CV_8UC3);
	else
		outputImage.create(inputImageX.size(), inputImageX.type());

	uchar* dataX = inputImageX.ptr<unsigned char>(0);
	uchar* dataY = inputImageY.ptr<unsigned char>(0);
	uchar* data = outputImage.ptr<unsigned char>(0);

	int i, j;
	for (i = 0; i < inputImageX.rows;i++)
	{
		for (j = 0; j < inputImageX.cols;j++)
		{
			if (*dataY < 50 && *dataX < 50)
			{
				if (type)
				{
					outputImage.at<Vec3b>(i, j)[0] = 0;
					outputImage.at<Vec3b>(i, j)[1] = 0;
				}
				else
				{
					*data = 0;
				}
			}
			else if (*dataX == 0)
			{
				if (type)
				{
					outputImage.at<Vec3b>(i, j)[0] = 255;
					outputImage.at<Vec3b>(i, j)[1] = 255;
				}
				else
				{
					*data = 255;
				}
			}
			else
			{
				if (type)
				{
					outputImage.at<Vec3b>(i, j)[0] = 255;
					outputImage.at<Vec3b>(i, j)[1] = atan(*dataY / *dataX) / PI * 2 * 254 + 1;
				}
				else
				{
					*data = atan(*dataY / *dataX) / PI * 2 * 254 + 1;
				}

			}
			if (type)
			{
				outputImage.at<Vec3b>(i, j)[2] = 0;
				if (outputImage.at<Vec3b>(i, j)[1] == 1)
					outputImage.at<Vec3b>(i, j)[2] = 255;
			}
			data++;
			dataX++;
			dataY++;
		}

	}

	return 0;
}

//背景分离
int eraseBackground(Mat &inputImage, Mat &outputImage, int threshold)
{
	outputImage.create(inputImage.size(), inputImage.type());

	uchar* dataIn = inputImage.ptr<unsigned char>(0);
	uchar* dataOut = outputImage.ptr<unsigned char>(0);

	for (int i = 0;i < inputImage.rows;i++)
	{
		for (int j = 0;j < inputImage.cols;j++)
		{
			if (*dataIn < threshold)
				*dataOut = *dataIn;
			else
				*dataOut = 255;

			dataIn++;
			dataOut++;
		}
	}

	return 0;
}

//图像显示，附带压缩显示和保存
void myImShow(char *imageName, Mat &image, int isZip, int isSave)
{
	Mat imagZip;
	if (isZip)
	{
		resize(image, imagZip, Size(), 0.2, 0.2);
	}
	else
	{
		imagZip = image.clone();
	}

	if (isSave)
	{
		char * name = new char[strlen(imageName) + sizeof(char) * 4];
		memcpy(name, imageName, strlen(imageName));
		*(name + strlen(imageName)) = '.';
		*(name + strlen(imageName) + 1) = 'j';
		*(name + strlen(imageName) + 2) = 'p';
		*(name + strlen(imageName) + 3) = 'g';
		*(name + strlen(imageName) + 4) = 0;
		imwrite(name, image);
	}
	imshow(imageName, imagZip);
}


//验证是否是方块
bool findBloak(Mat & image, Rect & rect)
{
	int x = rect.tl().x;
	int y = rect.tl().y + rect.height / 2;

	int y0_0, y0_1, y0_2, y0_3;
	int y1_1, y1_2;
	int y2_1, y2_2;

	int i = 0;
	int width[10];

	if (x < rect.width || x > image.cols - rect.width)
		return false;

	//x -= rect.width / 2;		//左移部分，保证监测到边沿
	int edge_last = 0;
	int edge_cur = 0;
	int y1_sign = 0;			//一阶导方向
	while (1)
	{
		//零阶
		y0_0 = image.at<uchar>(y,x);
		y0_1 = image.at<uchar>(y,x-1 );
		y0_2 = image.at<uchar>(y,x-2);
		y0_3 = image.at<uchar>(y,x-3);

		//一阶导
		y1_1 = y0_1 - y0_2;
		y1_2 = y0_2 - y0_3;
		{
			if ((abs(y1_1) < abs(y1_2)) && ((y1_1 >= 0) == (y1_2 >= 0)))
				y1_1 = y1_2;
		}

		//二阶导
		y2_1 = y0_0 - (y0_1 * 2) + y0_2;
		y2_2 = y0_1 - (y0_2 * 2) + y0_3;

		//二阶导为0点，一阶导极大/极小值，可能是边沿
		if (!y2_1 || ((y2_1 > 0) ? y2_2 < 0 : y2_2>0))
		{
			if (!y1_sign && y1_1)
			{
				edge_last = edge_cur = x;
				y1_sign = y1_1;
			}
			else if ((y1_sign < 0) && (y1_1 > 0))
			{
				edge_cur = x;
				width[i] = edge_cur - edge_last;
				edge_last = edge_cur;
				y1_sign = y1_1;
				i++;
			}
			else if ((y1_sign > 0) && (y1_1 < 0))
			{
				edge_last = x;
				y1_sign = y1_1;
			}
		}


		x++;
		if ((x - edge_last > rect.width * 3) || (x == image.cols))
		{
			return false;
		}
		if (i > 5)
			return true;
	}
	

	return false;
}
