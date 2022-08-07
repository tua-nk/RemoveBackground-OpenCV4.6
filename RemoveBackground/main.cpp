// RemoveBackground.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>

using namespace std;
using namespace cv;

Mat sobel(Mat gray);
void removeBgr(string image_before, string image_after, double thresh = 40, double maxval = 255);

int main()
{
    removeBgr("lady_before.jpeg", "lady_after.jpeg");
    waitKey(0);
}

Mat sobel(Mat gray)
{
    Mat edges;

    int scale = 1;
    int delta = 0;
    int ddepth = CV_16S;
    Mat edges_x, edges_y;
    Mat abs_edges_x, abs_edges_y;
    Sobel(gray, edges_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);
    convertScaleAbs(edges_x, abs_edges_x);
    Sobel(gray, edges_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT);
    convertScaleAbs(edges_y, abs_edges_y);
    addWeighted(abs_edges_x, 0.5, abs_edges_y, 0.5, 0, edges);

    return edges;
}

void removeBgr(string image_before, string image_after, double thresh /*= 40*/, double maxval /*= 255*/)
{
    //Load source image	
    Mat src = imread(image_before);

    //0. Source Image
    imshow("image_before", src);

    //1. Remove Shadows
    //Convert to HSV
    Mat hsvImg;
    cvtColor(src, hsvImg, COLOR_BGR2HSV);
    Mat channel[3];
    split(hsvImg, channel);
    channel[2] = Mat(hsvImg.rows, hsvImg.cols, CV_8UC1, 200);//Set V
    //Merge channels
    merge(channel, 3, hsvImg);
    Mat rgbImg;
    cvtColor(hsvImg, rgbImg, COLOR_HSV2BGR);

    //2. Convert to gray and normalize
    Mat gray(rgbImg.rows, src.cols, CV_8UC1);
    cvtColor(rgbImg, gray, COLOR_BGR2GRAY);
    normalize(gray, gray, 0, 255, NORM_MINMAX, CV_8UC1);

    //3. Edge detector
    GaussianBlur(gray, gray, Size(3, 3), 0, 0, BORDER_DEFAULT);
    //Use Sobel filter and thresholding.
    Mat edges = sobel(gray);
    threshold(edges, edges, 40, 255, cv::THRESH_BINARY);

    //4. Dilate
    Mat dilateGrad = edges;
    int dilateType = MORPH_ELLIPSE;
    int dilateSize = 3;
    Mat elementDilate = getStructuringElement(dilateType,
        Size(2 * dilateSize + 1, 2 * dilateSize + 1),
        Point(dilateSize, dilateSize));
    dilate(edges, dilateGrad, elementDilate);

    //5. Floodfill
    Mat floodFilled = cv::Mat::zeros(dilateGrad.rows + 2, dilateGrad.cols + 2, CV_8U);
    floodFill(dilateGrad, floodFilled, cv::Point(0, 0), 0, 0, cv::Scalar(), cv::Scalar(), 4 + (255 << 8) + cv::FLOODFILL_MASK_ONLY);
    floodFilled = cv::Scalar::all(255) - floodFilled;
    Mat temp;
    floodFilled(Rect(1, 1, dilateGrad.cols - 2, dilateGrad.rows - 2)).copyTo(temp);
    floodFilled = temp;

    //6. Erode
    int erosionType = MORPH_ELLIPSE;
    int erosionSize = 4;
    Mat erosionElement = getStructuringElement(erosionType,
        Size(2 * erosionSize + 1, 2 * erosionSize + 1),
        Point(erosionSize, erosionSize));
    erode(floodFilled, floodFilled, erosionElement);

    //7. Find largest contour
    int largestArea = 0;
    int largestContourIndex = 0;
    Rect boundingRectangle;
    Mat largestContour(src.rows, src.cols, CV_8UC1, Scalar::all(0));
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    findContours(floodFilled, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

    for (int i = 0; i < contours.size(); i++)
    {
        double a = contourArea(contours[i], false);
        if (a > largestArea)
        {
            largestArea = a;
            largestContourIndex = i;
            boundingRectangle = boundingRect(contours[i]);
        }
    }

    Scalar color(255, 255, 255);
    drawContours(largestContour, contours, largestContourIndex, color, cv::FILLED, 8, hierarchy); //Draw the largest contour using previously stored index.

    //8. Mask original image
    Mat maskedSrc;
    src.copyTo(maskedSrc, largestContour);

    //9. Save source image
    imshow("image_after", maskedSrc);
    imwrite(image_after, maskedSrc);
}
