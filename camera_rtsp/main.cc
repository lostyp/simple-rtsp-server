#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;
using namespace cv;

int main()
{
    VideoCapture cap;
    cap.open(0, CAP_V4L2);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));

    if( !cap.isOpened() ) {

	    cerr << "couldn't open capture."<<endl;
	    return -1;
    }

    Mat frame;
    namedWindow("frame", WINDOW_AUTOSIZE);

    while(true)
    {
       cap >> frame;
       
       imshow("frame", frame);

        if(waitKey(1) == 27) break; // Wait for 'esc' key press to exit
    }

    cap.release();

    return 0;
}