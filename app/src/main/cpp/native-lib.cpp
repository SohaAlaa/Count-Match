#include <jni.h>
#include <android/log.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define TAG "NativeLib"


/**
 * Helper function to find a cosine of angle between vectors
 * from pt0->pt1 and pt0->pt2
 */




extern "C" {
    void JNICALL
        Java_com_example_nativeopencvandroidtemplate_MainActivity_adaptiveThresholdFromJNI(JNIEnv* env,
            jobject instance,
            jlong matAddr) {

        // get Mat from raw address
        Mat& src = *(Mat*)matAddr;
		Mat& dst = *(Mat*)matAddr;

        if (src.empty())
            return;

        std::vector<cv::Mat> channels;
        cv::split(src, channels);

        for (int i = 0; i < channels.size(); ++i) {
            cv::threshold(channels[i], channels[i], 127, 255, cv::THRESH_OTSU);
            cv::Canny(channels[i], channels[i], 0, 50, 5);
        }

        for (int i = 0; i < channels.size(); ++i) {
            cv::bitwise_or(channels[0], channels[i], channels[0]);
        }
        cv::Mat& bw = channels[0];

        // Find contours
        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(bw.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		std::vector<cv::Point> approx;

		for (int i = 0; i < contours.size(); i++)
		{
			// Approximate contour with accuracy proportional
			// to the contour perimeter
			cv::approxPolyDP(cv::Mat(contours[i]), approx, cv::arcLength(cv::Mat(contours[i]), true) * 0.02, true);

			// Skip small or non-convex objects 
			if (std::fabs(cv::contourArea(contours[i])) < 100 || !cv::isContourConvex(approx))
				continue;

			if (approx.size() == 3)
			{
				setLabel(dst, "TRI", contours[i]);    // Triangles
			}
			else if (approx.size() >= 4 && approx.size() <= 6)
			{
				// Number of vertices of polygonal curve
				int vtc = approx.size();

				// Get the cosines of all corners
				std::vector<double> cos;
				for (int j = 2; j < vtc + 1; j++)
					cos.push_back(angle(approx[j % vtc], approx[j - 2], approx[j - 1]));

				// Sort ascending the cosine values
				std::sort(cos.begin(), cos.end());

				// Get the lowest and the highest cosine
				double mincos = cos.front();
				double maxcos = cos.back();

				// Use the degrees obtained above and the number of vertices
				// to determine the shape of the contour
				if (vtc == 4 && mincos >= -0.1 && maxcos <= 0.3)
					setLabel(dst, "RECT", contours[i]);
				else if (vtc == 5 && mincos >= -0.34 && maxcos <= -0.27)
					setLabel(dst, "PENTA", contours[i]);
				else if (vtc == 6 && mincos >= -0.55 && maxcos <= -0.45)
					setLabel(dst, "HEXA", contours[i]);
			}
			else
			{
				// Detect and label circles
				double area = cv::contourArea(contours[i]);
				cv::Rect r = cv::boundingRect(contours[i]);
				int radius = r.width / 2;

				if (std::abs(1 - ((double)r.width / r.height)) <= 0.2 &&
					std::abs(1 - (area / (CV_PI * std::pow(radius, 2)))) <= 0.2)
					setLabel(dst, "CIR", contours[i]);
			}
		}
    }
}
