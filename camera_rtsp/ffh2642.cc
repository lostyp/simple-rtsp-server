#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus 
}
#endif



using namespace std;
using namespace cv;

int main()
{
    avformat_network_init();

    VideoCapture cap(0, CAP_V4L2);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));

    if (!cap.isOpened()) {
        cerr << "Couldn't open capture." << endl;
        return -1;
    }

    int frameWidth = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
    int frameHeight = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));

    AVFormatContext *formatCtx = avformat_alloc_context();
    AVOutputFormat *outputFormat = av_guess_format(NULL, "output.mp4", NULL);
    formatCtx->oformat = outputFormat;
    
    AVStream *videoStream = avformat_new_stream(formatCtx, NULL);
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);

    codecCtx->bit_rate = 16000000; // 16 Mbps
    codecCtx->width = frameWidth;
    codecCtx->height = frameHeight;
    codecCtx->time_base = (AVRational){1, 30}; // 25 fps
    codecCtx->gop_size = 1; // 每帧都是关键帧
    codecCtx->max_b_frames = 0; // 不使用 B 帧
    codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        cerr << "Couldn't open codec." << endl;
        return -1;
    }

    avcodec_parameters_from_context(videoStream->codecpar, codecCtx);

    if (avio_open(&formatCtx->pb, "output.mp4", AVIO_FLAG_WRITE) < 0) {
        cerr << "Couldn't open output file." << endl;
        return -1;
    }

    avformat_write_header(formatCtx, NULL);

    SwsContext *swsCtx = sws_getContext(
        frameWidth, frameHeight, AV_PIX_FMT_BGR24,
        frameWidth, frameHeight, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);

    Mat frame;
    AVFrame *avFrame = av_frame_alloc();
    avFrame->format = codecCtx->pix_fmt;
    avFrame->width = codecCtx->width;
    avFrame->height = codecCtx->height;
    av_frame_get_buffer(avFrame, 32);

    namedWindow("frame", WINDOW_AUTOSIZE);

    int64_t pts = 0;
    int64_t dts = 0;

    while (true) {
        cap >> frame;

        if (frame.empty()) break; // 检查是否读取到帧

        imshow("frame", frame);

        uint8_t *data[4];
        int linesize[4];
        av_frame_get_buffer(avFrame, 32);

        data[0] = frame.data;
        linesize[0] = static_cast<int>(frame.step[0]);
        avFrame->data[0] = data[0];
        avFrame->linesize[0] = linesize[0];

        sws_scale(swsCtx, (const uint8_t * const *)data, linesize, 0, frame.rows, avFrame->data, avFrame->linesize);

        // 设置 PTS 和 DTS
        avFrame->pts = pts++;
        avFrame->pkt_dts = dts++;

        // 编码并写入文件
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        if (avcodec_send_frame(codecCtx, avFrame) >= 0) {
            while (avcodec_receive_packet(codecCtx, &pkt) >= 0) {
                pkt.pts = av_rescale_q(pkt.pts, codecCtx->time_base, videoStream->time_base);
                pkt.dts = av_rescale_q(pkt.dts, codecCtx->time_base, videoStream->time_base);
                av_write_frame(formatCtx, &pkt);
                av_packet_unref(&pkt);
            }
        }
        // std::this_thread::sleep_for(330ms);
        if (waitKey(1) == 27) break; // Wait for 'esc' key press to exit
    }

    av_write_trailer(formatCtx);

    av_frame_free(&avFrame);
    sws_freeContext(swsCtx);
    avcodec_close(codecCtx);
    avformat_close_input(&formatCtx);
    avformat_free_context(formatCtx);

    cap.release();

    return 0;
}
