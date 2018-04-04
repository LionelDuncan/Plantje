#include <C:\opencv\build\include\opencv2\opencv.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "Helper.h"
#include "Video.h"

using namespace cv;
using namespace std;
/*
int main() 
{
	imgs = std::get_fil
	for (int i = 0; i < imgs; i++) {
		Mat img = imread()
	}
	Mat faceSrc = imread("\Gestures\Face_Dataset\Ground_Truth\GroundT_FacePhoto\*");
	Mat faceMask = imread("\Gestures\Face_Dataset\Pratheepan_Dataset\FacePhoto\*");
}
/*
int main()
{
	Mat src = imread("hand.jpg");
	if (src.empty())
		return -1;
	//calcHist(src);
	blur(src, src, Size(3, 3));
	Mat hsv;
	cvtColor(src, hsv, CV_BGR2HSV);
	Mat bw;
	inRange(hsv, Scalar(0, 10, 60), Scalar(20, 150, 255), bw);
	imshow("src", src);
	imshow("dst", bw);
	imshow("hsv", hsv);
	waitKey(0);
	return 0;
}
*/
