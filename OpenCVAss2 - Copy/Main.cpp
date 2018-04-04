#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <string>
#include <iostream>


//#include"SkinDetector.h"
using namespace std;
using namespace cv;

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

SkinDetector::SkinDetector(void)
{
	//YCrCb threshold
	// You can change the values and see what happens
	Y_MIN = 0;
	Y_MAX = 255;
	Cr_MIN = 133;
	Cr_MAX = 173;
	Cb_MIN = 77;
	Cb_MAX = 127;
}

SkinDetector::~SkinDetector(void)
{
}

//this function will return a skin masked image
cv::Mat SkinDetector::getSkin(cv::Mat input)
{
	//cv::imread("\gestures\")
	cv::Mat skin;
	//first convert our RGB image to YCrCb
	cv::cvtColor(input, skin, cv::COLOR_BGR2YCrCb);

	//uncomment the following line to see the image in YCrCb Color Space
	//cv::imshow("YCrCb Color Space",skin);

	//filter the image in YCrCb color space
	cv::inRange(skin, cv::Scalar(Y_MIN, Cr_MIN, Cb_MIN), cv::Scalar(Y_MAX, Cr_MAX, Cb_MAX), skin);

	cv::Mat close_element = cv::getStructuringElement(MORPH_RECT, cv::Size(5, 5), cv::Point(2, 2));
	// Dilation
	cv::morphologyEx(skin, skin, MORPH_DILATE, close_element, cv::Point(-1, -1), 1);

	//Closing
	cv::morphologyEx(skin, skin, MORPH_CLOSE, close_element, cv::Point(-1, -1), 2);

	// find spatial extremes of the hull. Used code from https://docs.opencv.org/2.4/doc/tutorials/imgproc/shapedescriptors/find_contours/find_contours.html
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	RNG rng(12345);

	// Find contours
	findContours(skin, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	// Draw contours
	//Mat drawing = Mat::zeros(skin.size(), CV_8UC3);
	for (int i = 0; i< contours.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		drawContours(skin, contours, i, color, 2, 8, hierarchy, 0, Point());
	}

	return skin;
}


int main()
{
	VideoCapture capture;
	//open capture object at location zero (default location for webcam)

	capture.open(0);

	//set height and width of capture frame
	capture.set(CV_CAP_PROP_FRAME_WIDTH, 320);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	Mat cameraFeed;

	SkinDetector mySkinDetector;

	Mat skinMat;

	//start an infinite loop where webcam feed is copied to cameraFeed matrix
	//all of our operations will be performed within this loop
	while (1) {

		//store image to matrix
		capture.read(cameraFeed);

		//show the current image
		imshow("Original Image", cameraFeed);

		skinMat = mySkinDetector.getSkin(cameraFeed);

		imshow("Skin Image", skinMat);

		waitKey(30);
	}
	return 0;
}

// SkinAndGestures.cpp : Defines the entry point for the console application.
// Coert van Gemeren (2017) 
// Use this code under the following terms:
// Attribution: appropriate credit + indicate if changes were made
//
// Uses: Unistroke Recognizer by Baylor Wetzel 


/*
int main()
{
	const std::string path_images = "C:/Users/Takis/Documents/UU/Interactietech/OpenCVAss2/Gestures/Face_Dataset/Pratheepan_Dataset/FacePhoto";
	std::vector<std::string> files_images;
	Utilities::getDirectory(path_images, "jpg", files_images);

	const std::string path_masks = "C:/Users/Takis/Documents/UU/Interactietech/OpenCVAss2/Gestures/Face_Dataset/Ground_Truth/GroundT_FacePhoto";
	std::vector<std::string> files_masks;
	Utilities::getDirectory(path_masks, "png", files_masks);

	//CVLog(INFO) << files_images;
	//CVLog(INFO) << files_masks;

	// 2 histograms: one for counting skin pixel colors, the other for counting non-skin pixel colors
	cv::Mat sum_hist_skin, sum_hist_nonskin;

	double theta;  // probability threshold
	theta = 0.1;   // TODO find suitable threshold

	int bin_size;    // histogram bin size
	bin_size = 8;    // TODO find suitable size

	int hist_size[3] = { bin_size, bin_size, bin_size };      // Construct a 3D array of bin_size bin sizes
	sum_hist_skin = cv::Mat::zeros(3, hist_size, CV_32F);     // Construct the skin histogram
	sum_hist_nonskin = cv::Mat::zeros(3, hist_size, CV_32F);  // Construct the non-skin histogram

	const float color_range[2] = { 0, 256 };                            // Color range
	const float *ranges[3] = { color_range, color_range, color_range }; // Pointer to 3D array, range for every color channel
	const int channels[3] = { 0, 1, 2 };                                // Channels to work with

																		// Iterate over all training image filenames
	for (size_t i = 0; i < files_images.size(); ++i)
	{
		const auto image_file = files_images[i];

		// We need to find the correct mask for this image
		const auto ma_it = std::find_if(files_masks.begin(), files_masks.end(), [image_file](const std::string &m)
		{
			return Utilities::getBasename(m) == Utilities::getBasename(image_file);
		});

		// If we find the right mask
		if (ma_it != files_masks.end())
		{
			cv::Mat image = cv::imread(image_file);                                 // load training image
			cv::Mat mask = cv::imread(*ma_it, CV_LOAD_IMAGE_GRAYSCALE);             // load training mask
			CV_Assert(image.rows == mask.rows && image.cols == mask.cols);          // They should be equal in size

																					// TODO fill the Histograms with the correct pixel counts with cv::calcHist

																					// OPTIONAL You may want to try different color spaces than RGB
																					// OPTIONAL You may want to try to make the histograms dynamic instead of static
																					//          That means putting this routing into the video loop and updating the skin
																					//          color somehow, based on the current lighting conditions and your webcam's
																					//          color balance
		}
	}

	cv::Mat Pskin_rgb; // TODO use Bayesian statistics to calculate P(skin|rgb)

					   // Factor to scale a color number to a histogram bin
	const double factor = bin_size / 256.0;

	// Open a window
	cv::namedWindow("detection result", CV_WINDOW_FREERATIO);

	// Open the webcam
	cv::VideoCapture video_capture(0);
}*/


