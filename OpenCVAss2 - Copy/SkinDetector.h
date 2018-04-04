// https://razibdeb.wordpress.com/2013/09/10/skin-detection-in-c-using-opencv/
#pragma once
#include<opencv\cv.h>
using namespace std;
class SkinDetector
{
public:
	SkinDetector(void);
	~SkinDetector(void);

	cv::Mat getSkin(cv::Mat input);

private:
	int Y_MIN;
	int Y_MAX;
	int Cr_MIN;
	int Cr_MAX;
	int Cb_MIN;
	int Cb_MAX;
};
