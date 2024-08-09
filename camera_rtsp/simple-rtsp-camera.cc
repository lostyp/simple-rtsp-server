#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <asio.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using asio::ip::tcp;
using namespace std;
using namespace cv;

class RTSPSession;

class RTSPSession : public std::enable_shared_from_this<RTSPSession>
{
public:
    RTSPSession(asio::io_context &io_context)
        : socket_(io_context), rtspSessionId_("12345678")
    {
        initialize_ffmpeg();
    }

    tcp::socket &socket()
    {
        return socket_;
    }

    void start()
    {
        do_read();
    }

private:
    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(data_),
                                [this, self](std::error_code ec, std::size_t length)
                                {
                                    if (!ec)
                                    {
                                        handle_request(length);
                                        do_read();
                                    }
                                });
    }

    void handle_request(std::size_t length)
    {
        std::string request(data_, length);
        std::cout << "Received request: " << request << std::endl;

        std::string cseq;
        std::string::size_type cseq_pos = request.find("CSeq:");
        if (cseq_pos != std::string::npos)
        {
            std::string::size_type start = cseq_pos + 5;
            std::string::size_type end = request.find("\r\n", start);
            cseq = request.substr(start, end - start);
            std::cout << "CSeq:" << cseq << std::endl;
        }

        if (request.find("OPTIONS") != std::string::npos)
        {
            std::string response =
                "RTSP/1.0 200 OK\r\n"
                "CSeq: 2\r\n"
                "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n";
            socket_.send(asio::buffer(response));
        }
        else if (request.find("DESCRIBE") != std::string::npos)
        {
            send_response("200 OK", "application/sdp",
                          "v=0\r\n"
                          "o=- 0 0 IN IP4 127.0.0.1\r\n"
                          "s=No Name\r\n"
                          "c=IN IP4 127.0.0.1\r\n"
                          "t=0 0\r\n"
                          "a=tool:libavformat 58.29.100\r\n"
                          "m=video 0 RTP/AVP 96\r\n"
                          "a=rtpmap:96 H264/90000\r\n"
                          "a=control:streamid=0\r\n"
                          "a=fmtp:96 packetization-mode=1;profile-level-id=42E01F;sprop-parameter-sets=Z0LgKdoBQBbpUgAAAwABAAADACQAAAMABAAAAwDoIAAA0eKZPgAAAwAAADGAAM0fLCwgA0vA=,aM4B4g==\r\n"
                          "\r\n",
                          cseq);
        }
        else if (request.find("SETUP") != std::string::npos)
        {
            std::string response =
                "RTSP/1.0 200 OK\r\n"
                "CSeq: " +
                cseq + "\r\n"
                       "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
                       "Session: " +
                rtspSessionId_ + "\r\n"
                                 "\r\n";
            socket_.send(asio::buffer(response));
            start_streaming();
        }
        else if (request.find("PLAY") != std::string::npos)
        {
            std::string response =
                "RTSP/1.0 200 OK\r\n"
                "CSeq: " +
                cseq + "\r\n"
                       "Range: npt=0.000-\r\n"
                       "Session: " +
                rtspSessionId_ + "\r\n"
                                 "\r\n";
            socket_.send(asio::buffer(response));
        }
    }

    void send_response(const std::string &status, const std::string &headers, const std::string &body = "", const std::string &cseq = "")
    {
        std::string response =
            "RTSP/1.0 " + status + "\r\n"
                                   "CSeq: " +
            (cseq.empty() ? std::to_string(++cseq_) : cseq) + "\r\n"
                                                              "Content-Type: " +
            headers + "\r\n"
                      "Content-Length: " +
            std::to_string(body.size()) + "\r\n"
                                          "\r\n" +
            body;
        socket_.send(asio::buffer(response));
        std::cout << "----S->C----" << std::endl;
        std::cout << response << std::endl;
    }

    void start_streaming()
    {
        VideoCapture cap(0);
        if (!cap.isOpened())
        {
            cerr << "Couldn't open capture." << endl;
            return;
        }

        int frameWidth = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
        int frameHeight = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
        int fps = 15;

        SwsContext *swsCtx = sws_getContext(
            frameWidth, frameHeight, AV_PIX_FMT_BGR24,
            frameWidth, frameHeight, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        AVCodecContext *codecCtx = avcodec_alloc_context3(codec);

        codecCtx->bit_rate = 4000000;
        codecCtx->width = frameWidth;
        codecCtx->height = frameHeight;
        codecCtx->time_base = (AVRational){1, fps};
        codecCtx->gop_size = fps;
        codecCtx->max_b_frames = 0;
        codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

        if (avcodec_open2(codecCtx, codec, nullptr) < 0)
        {
            cerr << "Could not open codec" << endl;
            return;
        }

        Mat frame;
        AVFrame *avFrame = av_frame_alloc();
        avFrame->format = codecCtx->pix_fmt;
        avFrame->width = codecCtx->width;
        avFrame->height = codecCtx->height;
        av_frame_get_buffer(avFrame, 32);

        while (true)
        {
            cap >> frame;
            if (frame.empty())
                break;

            uint8_t *data[4];
            int linesize[4];
            av_frame_get_buffer(avFrame, 32);

            data[0] = frame.data;
            linesize[0] = static_cast<int>(frame.step[0]);
            avFrame->data[0] = data[0];
            avFrame->linesize[0] = linesize[0];

            sws_scale(swsCtx, (const uint8_t *const *)data, linesize, 0, frame.rows, avFrame->data, avFrame->linesize);

            AVPacket pkt;
            av_init_packet(&pkt);
            pkt.data = nullptr;
            pkt.size = 0;

            if (avcodec_send_frame(codecCtx, avFrame) >= 0)
            {
                while (avcodec_receive_packet(codecCtx, &pkt) >= 0)
                {
                    send_rtp_packet(pkt.data, pkt.size);
                    av_packet_unref(&pkt);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
        }

        av_frame_free(&avFrame);
        sws_freeContext(swsCtx);
        avcodec_close(codecCtx);
    }

    void send_rtp_packet(uint8_t *data, int size)
    {
        std::string rtpPacket(reinterpret_cast<char *>(data), size);
        // Send RTP packet via TCP
        asio::write(socket_, asio::buffer(rtpPacket));
    }

    void initialize_ffmpeg()
    {
        av_register_all();
        avcodec_register_all();
        avformat_network_init();
    }

    tcp::socket socket_;
    char data_[1024];
    std::string rtspSessionId_;
    int cseq_;
};

class RTSPServer
{
public:
    RTSPServer(asio::io_context &io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        auto new_session = std::make_shared<RTSPSession>(acceptor_.get_executor().context());
        acceptor_.async_accept(new_session->socket(),
                               [this, new_session](std::error_code ec)
                               {
                                   if (!ec)
                                   {
                                       new_session->start();
                                   }
                                   start_accept();
                               });
    }

    tcp::acceptor acceptor_;
};

int main(int argc, char *argv[])
{
    try
    {
        asio::io_context io_context;
        RTSPServer server(io_context, 8554);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
