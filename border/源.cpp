#include "core/core.hpp"
#include "highgui/highgui.hpp"
#include "imgproc/imgproc.hpp"
#include <math.h>
#include "source.h"
#include "zbar.h"

#define PI 3.1415926

using namespace std;
using namespace cv;
using namespace zbar;

int main(int argc, char *argv[])
{
	Mat image, imageGray, imageGuussian, imageEqualize;

	/*----------------------------------------
	1. 打开图像
	----------------------------------------*/
	image = imread(argv[1]);
	myImShow("1.原图像", image,ZIP,1);

	/*----------------------------------------
	2.转化为灰度图
	----------------------------------------*/
	cvtColor(image, imageGray, CV_RGB2GRAY);
	//为了方便，对图片压缩、保存进行了封装
	myImShow("2.灰度图", imageGray,ZIP,1);

	/*----------------------------------------
	3. 高斯平滑滤波 
	----------------------------------------*/
	GaussianBlur(imageGray, imageGuussian, Size(9, 9), 0);
	myImShow("3.高斯平衡滤波", imageGuussian,ZIP,1);

	/*----------------------------------------
	4. 双峰谷底法寻找阈值二值化
	----------------------------------------*/
	//计算直方图
	MatND histG = myCalcHist(imageGuussian, 0);
	//双峰谷底法寻找阈值
	int vally = findThresholdVally(histG);
	printf("二值化阈值：%d\r\n", vally);
	//二值化
	Mat imageThreshold;
	threshold(imageGray, imageThreshold, vally, 255, CV_THRESH_BINARY);
	myImShow("二值化", imageThreshold,ZIP,1);

	/*----------------------------------------
	5. 判断条形码方向并旋转
		使用Sobel算子分别计算X、Y方向梯度
		根据两个方向上梯度余弦的计算，统计出变化最频繁方向
		根据条形码特征，条形码水平因为黑白交叉，梯度变换频繁
		根据统计信息旋转图像
	----------------------------------------*/
	//求得水平和垂直方向灰度图像的梯度,使用Sobel算子
	Mat imageX16S, imageY16S;
	Mat imageSobelX, imageSobelY;
	Mat imageDirection;
	Sobel(imageThreshold, imageX16S, CV_16S, 1, 0, 3, 1, 0, 4);
	Sobel(imageThreshold, imageY16S, CV_16S, 0, 1, 3, 1, 0, 4);
	
	//计算每个像素点梯度方向，统计峰值（除0外）
	findDirection(imageX16S, imageY16S, imageDirection);
	int max = hist16S(imageDirection);
	printf("最大值位置：%d\r\n", max);

	//梯度图像显示
	//sobel计算后每个像素是short类型，需要转换为无符号数
	convertScaleAbs(imageX16S, imageSobelX, 1, 0);
	convertScaleAbs(imageY16S, imageSobelY, 1, 0);
	convertScaleAbs(imageDirection, imageDirection, 1, 0);
	myImShow("X方向", imageSobelX,ZIP,1);
	myImShow("Y方向", imageSobelY,ZIP,1);
	myImShow("5. 方向", imageDirection,ZIP,1);

	//旋转图像
	double angle = max / 255.0 * 90;
	//计算旋转后的大小，扩充旋转
	cv::Point2f center(image.cols / 2.0f, image.rows / 2.0f);
	cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1);
	cv::Rect bbox = cv::RotatedRect(center, image.size(), angle).boundingRect();
	//中心旋转
	rot.at<double>(0, 2) += bbox.width / 2.0 - center.x;
	rot.at<double>(1, 2) += bbox.height / 2.0 - center.y;
	
	Mat imageRotate;
	Mat imageGrayRotate;
	Mat imageThresholdRotate;
	Scalar borderColor = Scalar(255,255,255);
	//因为默认旋转后填充黑色，做颜色反转
	imageThreshold = 255 - imageThreshold;
	warpAffine(image, imageRotate, rot, bbox.size(), INTER_LINEAR, BORDER_CONSTANT,borderColor);
	warpAffine(imageThreshold, imageThresholdRotate, rot, bbox.size());
	warpAffine(imageGray, imageGrayRotate, rot, bbox.size());
	myImShow("5.旋转图像", imageGrayRotate, ZIP, 1);

	/*----------------------------------------
	6. 找连通区域
		contours记录所有找到的区域
		rectVector中记录条形码区域
		判断contours中的元素是否在rectVector中
		一个条形码的多个部分不重复查找
	----------------------------------------*/
	//查找连通区域
	vector<vector<Point>> contours;
	vector<Vec4i> hiera;
	findContours(imageThresholdRotate, contours, hiera, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	printf("counters:%d\r\n", contours.size());
	int imageWidth = imageRotate.cols;
	int j = 0;
	Vector<Rect> rectVector;
	//对连通区域遍历
	for (int i = 0;i<contours.size();i++)
	{
		//简单过滤
		//区域宽度不超过图像10%，区域高是宽的3倍以上即长方形，宽大于4个像素
		Rect rect = boundingRect((Mat)contours[i]);
		if (rect.width < imageWidth / 10)
		{
			if (rect.width * 3 < rect.height && rect.width > 4)
			{
				//判断是否在已经找到的条形码内
				int xCurent = rect.tl().x;
				int yCenter = rect.tl().y + rect.height;
				int rectI;
				for (rectI = 0; rectI < rectVector.size();rectI++)
				{
					Rect rectT = rectVector[rectI];
					if ((xCurent > rectT.tl().x) && (xCurent < rectT.tl().x + rectT.width) && (yCenter > rectT.tl().y) && (yCenter < rectT.tl().y + rectT.height))
					{
						break;
					}
				}

				if (rectI == rectVector.size())
				{
					Rect rectTem;

					//横向过滤
					if (findBloak(imageGrayRotate, rect, rectTem))
					{
						//条形码识别
						ImageScanner scanner;
						scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
						Mat imageCut = Mat(imageGrayRotate, rectTem);
						Mat imageCopy = imageCut.clone();
						uchar *raw = (uchar *)imageCopy.data;
						Image imageZbar(imageCopy.cols, imageCopy.rows, "Y800", raw, imageCopy.cols * imageCopy.rows);
						scanner.scan(imageZbar);		//扫描条形码
						Image::SymbolIterator sybmol = imageZbar.symbol_begin();
						if (imageZbar.symbol_begin() == imageZbar.symbol_end())
						{
							continue;
						}
						//如果区域是可识别的条形码
						rectVector.push_back(rectTem);
						printf("height:%d;width:%d\r\n", rect.height, rect.width);
						printf("x:%d,y:%d\r\n", xCurent, yCenter);
						rectangle(imageRotate, rectTem, Scalar(255), 2);
					}
				}
			}
		}
	}
	namedWindow("6. 找出二维码矩形区域",0);
	myImShow("6. 找出二维码矩形区域", imageRotate,ZIP,1);

	waitKey();
	return 0;
}