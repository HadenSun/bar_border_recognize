#include "source.h"

//平均值法找二值化阈值
//参数：	hist：直方图计算结果
//返回值：	灰度均值
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

//检测直方图是否为双峰的  
//参数：	HistGram[] 直方图数组
//返回值：	是否为双峰
bool IsDimodal(double HistGram[])       
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
//参数：	hist 直方图
//返回值：	谷底灰度值
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
//参数：	imageGray 灰度图像
//参数：	isShow	-0 不绘制
//					-1 绘制
//返回值：	灰度直方图数组
MatND myCalcHist(Mat imageGray, int isShow)
{
	//计算直方图
	int channels = 0;
	MatND dstHist;
	int histSize[] = { 256 };
	float midRanges[] = { 0,256 };
	const float *ranges[] = { midRanges };
	calcHist(&imageGray, 1, &channels, Mat(), dstHist, 1, histSize, ranges, true, false);

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
//参数：	inputImageX x方向梯度图像
//参数：	inputImageY y方向梯度图像
//参数：	outputImage 输出结果图像
//返回值：	0  - 正常
//			-1 - 异常
int findDirection(Mat &inputImageX, Mat &inputImageY, Mat &outputImage)
{
	if (inputImageX.cols != inputImageY.cols)
		return -1;
	if (inputImageX.rows != inputImageY.rows)
		return -1;

	outputImage.create(inputImageX.size(), inputImageX.type());

	short* dataX = inputImageX.ptr<short>(0);
	short* dataY = inputImageY.ptr<short>(0);
	short* data = outputImage.ptr<short>(0);

	int i, j;
	for (i = 0; i < inputImageX.rows;i++)
	{
		for (j = 0; j < inputImageX.cols;j++)
		{
			if (*dataY < 20 && *dataY > -20 && *dataX > -20 && *dataX < 20)
			{
				//梯度变化过小的剔除
				*data = 0;
			}
			else if (*dataX == 0)
			{
				if (*dataY != 0)
				{
					*data = 255;
				}
				else
				{	
					*data = 0;
				}
			}
			else
			{				
				*data = atan((float)*dataY / (float)*dataX) / PI * 2 * 254;
				//无意义数据/两个方向梯度都是0的数据，放在0里
				//结果小于1的取整为0，存为1
				if (*data == 0)
					(*data)++;
			}
			data++;
			dataX++;
			dataY++;
		}

	}

	return 0;
}

//背景分离
//背景摸为全黑0，其他不变
//参数：	inputImage 输入图像
//参数：	outputImage 输出图像
//参数：	threshold 阈值
//返回值：	0 - 正常
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
//参数：	imageName 图像名称
//参数：	iamge 图像
//参数：	isZip 是否压缩显示 1-压缩 0-不压缩
//参数：	isSave 是否保存图片（不受上一参数影响，全分辨率保存） 1-保存 0-不保存
void myImShow(char *imageName, Mat &image, int isZip, int isSave)
{
	Mat imagZip;
	if (isZip)
	{
		resize(image, imagZip, Size(), ZIPTIME, ZIPTIME);
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


//验证是否是条形码区域
//参数：	image 图像
//参数：	rect 感兴趣区域
//参数：	rectOut 条形码区域
//返回值：	是否是条形码
bool findBloak(Mat & image, Rect & rect,Rect & rectOut)
{
	int rectX, rectY, rectWidth, rectHeight;
	int rectEndX;

	int x = rect.tl().x;
	int y = rect.tl().y + rect.height / 2;

	int y0_0, y0_1, y0_2, y0_3;
	int y1_1, y1_2;
	int y2_1, y2_2;

	int i = 0;

	if (x < rect.width || x > image.cols - rect.width)
		return false;

	//x -= rect.width / 2;		//左移部分，保证监测到边沿

	//x增大方向判断
	int edge_last = x;
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
			//黑框后沿
			else if ((y1_sign < 0) && (y1_1 > 0))
			{
				edge_cur = x;
				edge_last = edge_cur;
				y1_sign = y1_1;
				i++;
			}
			//黑框前沿
			else if ((y1_sign > 0) && (y1_1 < 0))
			{
				edge_last = x;
				y1_sign = y1_1;
			}
		}


		x++;
		//黑框不超过感兴趣区域1.5倍宽
		//白色部分不超过感兴趣区域3倍宽
		if ((y1_sign > 0)?(x - edge_last > rect.width * 3):(x - edge_last > rect.width * 1.5) || (x == image.cols))
		{
			if (i > 9)
			{
				//连续9个符合区域，是条形区域
				rectEndX = x;
				break;
			}
			else
				return false;
		}
	}

	//x减小方向判断
	x = rect.tl().x;
	edge_last = x;
	edge_cur = 0;
	y1_sign = 0;			//一阶导方向
	while (1)
	{
		//零阶
		y0_0 = image.at<uchar>(y, x);
		y0_1 = image.at<uchar>(y, x - 1);
		y0_2 = image.at<uchar>(y, x - 2);
		y0_3 = image.at<uchar>(y, x - 3);

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
			//黑框前沿
			else if ((y1_sign > 0) && (y1_1 < 0))
			{
				edge_cur = x;
				edge_last = edge_cur;
				y1_sign = y1_1;
			}
			//黑框后沿
			else if ((y1_sign < 0) && (y1_1 > 0))
			{
				edge_last = x;
				y1_sign = y1_1;
			}
		}

		x--;
		if ((y1_sign < 0) ? (edge_last - x > rect.width * 3) : (edge_last - x  > rect.width * 1.5) || (x == 5))
		{
			rectX = x;
			rectY = rect.tl().y;
			rectHeight = rect.height;
			rectWidth = rectEndX - rectX;
			rectOut.height = rectHeight;
			rectOut.width = rectWidth;
			rectOut.x = rectX;
			rectOut.y = rectY;
			return true;
		}
	}
	

	return false;
}

//16位图像找直方图最大值
//输入数据范围-255 -- +254
//0为无效数据
//参数：	image 输入图像
//返回值：	直方图最大值
int hist16S(Mat &image)
{
	int maxLoc = 0;
	int maxValue = 0;
	double hist[512] = { 0 };

	short *data = image.ptr<short>(0);
	
	for (int i = 0; i < image.rows;i++)
	{
		for (int j = 0;j < image.cols;j++)
		{
			hist[*data + 255]++;
			if (hist[*data + 255] > maxValue && *data != 0)
			{
				maxLoc = *data;
				maxValue = hist[*data + 255];
			}
			data++;
		}
	}
	return maxLoc;
}