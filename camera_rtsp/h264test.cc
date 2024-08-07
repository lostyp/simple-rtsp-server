#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;
using namespace cv;

int main()
{
    // 打开摄像头
    VideoCapture cap(0, CAP_V4L2);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));

    if (!cap.isOpened()) {
        cerr << "Couldn't open capture." << endl;
        return -1;
    }

    // 获取摄像头的帧宽度和高度
    int frameWidth = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
    int frameHeight = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));

    // 创建 VideoWriter 对象
    VideoWriter writer;
    writer.open("output.mp4", VideoWriter::fourcc('H', '2', '6', '4'), 30, Size(frameWidth, frameHeight), true);

    if (!writer.isOpened()) {
        cerr << "Couldn't open video writer." << endl;
        return -1;
    }

    Mat frame;
    namedWindow("frame", WINDOW_AUTOSIZE);

    while (true) {
        cap >> frame;

        if (frame.empty()) break; // 检查是否读取到帧

        imshow("frame", frame);

        // 将帧写入文件
        writer.write(frame);

        if (waitKey(1) == 27) break; // 等待 'esc' 键按下退出
    }

    // 释放摄像头和视频写入对象
    cap.release();
    writer.release();

    return 0;
}
