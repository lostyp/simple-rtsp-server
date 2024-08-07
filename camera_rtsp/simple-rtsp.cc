#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <asio.hpp>
#include <regex>

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

class RTSPSession : public std::enable_shared_from_this<RTSPSession>
{
public:
  RTSPSession(asio::io_context &io_context)
      : socket_(io_context), rtspSessionId_("12345678"), pts_(0)
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
                              else
                              {
                                cerr << "Error on receive: " << ec.message() << endl;
                              }
                            });
  }

  void handle_request(std::size_t length)
  {
    std::string request(data_, length);
    std::cout << "Received request: " << request << std::endl;

    // Extract CSeq from request
    std::smatch match;
    std::regex cseq_regex("CSeq: (\\d+)");
    if (std::regex_search(request, match, cseq_regex))
    {
      cseq_ = std::stoi(match[1]);
    }
    else
    {
      cseq_ = 0; // Default CSeq if not found
    }

    if (request.find("OPTIONS") != std::string::npos)
    {
      send_response("200 OK", "Public: OPTIONS, DESCRIBE, SETUP, PLAY");
    }
    else if (request.find("DESCRIBE") != std::string::npos)
    {
      send_response("200 OK", "Content-Base: rtsp://172.20.80.184:8554/live\r\nContent-Type: application/sdp",
                    "v=0\r\n"
                    "o=- 0 0 IN IP4 127.0.0.1\r\n"
                    "s=No Name\r\n"
                    "c=IN IP4 127.0.0.1\r\n"
                    "t=0 0\r\n"
                    "m=video 0 RTP/AVP 96\r\n"
                    "a=rtpmap:96 H264/90000\r\n"
                    "a=fmtp:96 packetization-mode=1\r\n"
                    "a=control:track0"
                    "\r\n");
    }
    else if (request.find("SETUP") != std::string::npos)
    {
      std::string response =
          "RTSP/1.0 200 OK\r\n"
          "CSeq: " +
          std::to_string(cseq_) + "\r\n"
                                  "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
                                  "Session: " +
          rtspSessionId_ + "\r\n"
                           "\r\n";
      socket_.send(asio::buffer(response));

      // 在此处进行会话初始化，但不开始流媒体传输
      // initialize_session();
    }
    else if (request.find("PLAY") != std::string::npos)
    {
      std::string response =
          "RTSP/1.0 200 OK\r\n"
          "CSeq: " +
          std::to_string(cseq_) + "\r\n"
                                  "Range: npt=0.000-\r\n"
                                  "Session: " +
          rtspSessionId_ + "\r\n"
                           "\r\n";
      socket_.send(asio::buffer(response));

      // 在此处开始流媒体传输
      start_streaming();
    }
  }

  void send_response(const std::string &status, const std::string &headers, const std::string &body = "")
  {
    std::string response =
        "RTSP/1.0 " + status + "\r\n"
                               "CSeq: " +
        std::to_string(cseq_) + "\r\n" + headers + "\r\n"
                                                   "Content-Length: " +
        std::to_string(body.size()) + "\r\n"
                                      "\r\n" +
        body;
    std::cout << "response:" << response << std::endl;
    socket_.send(asio::buffer(response));
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
    namedWindow("frame", WINDOW_AUTOSIZE);

    while (true)
    {
      cap >> frame;
      if (frame.empty())
        break;
      imshow("frame", frame);

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
      avFrame->pts = pts_++;
      if (avcodec_send_frame(codecCtx, avFrame) >= 0)
      {
        while (avcodec_receive_packet(codecCtx, &pkt) >= 0)
        {
          send_rtp_packet(pkt.data, pkt.size);
          av_packet_unref(&pkt);
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
      if (waitKey(1) == 27)
        break; // Wait for 'esc' key press to exit
    }

    av_frame_free(&avFrame);
    sws_freeContext(swsCtx);
    avcodec_close(codecCtx);
  }

  void send_rtp_over_tcp(const uint8_t *rtp_data, size_t rtp_size)
  {
    try
    {
      uint8_t tcp_header[4];
      tcp_header[0] = '$';
      tcp_header[1] = 0;                      // 通道号，通常是 0 或 1
      tcp_header[2] = (rtp_size >> 8) & 0xFF; // 长度高字节
      tcp_header[3] = rtp_size & 0xFF;        // 长度低字节

      std::vector<uint8_t> packet(tcp_header, tcp_header + 4);
      packet.insert(packet.end(), rtp_data, rtp_data + rtp_size);

      asio::write(socket_, asio::buffer(packet));
    }
    catch (std::exception &e)
    {
      std::cerr << "send_rtp_over_tcp exception: " << e.what() << std::endl;
    }
  }

  void send_rtp_packet(const uint8_t *data, size_t size)
  {
    try
    {
      static uint16_t sequence_number = 0;
      static uint32_t timestamp = 0;

      // 创建 RTP 包头
      uint8_t rtp_packet[12 + size];
      rtp_packet[0] = 0x80; // Version 2, no padding, no extensions, 1 contributing source
      rtp_packet[1] = 96;   // Payload type
      rtp_packet[2] = (sequence_number >> 8) & 0xFF;
      rtp_packet[3] = sequence_number & 0xFF;
      rtp_packet[4] = (timestamp >> 24) & 0xFF;
      rtp_packet[5] = (timestamp >> 16) & 0xFF;
      rtp_packet[6] = (timestamp >> 8) & 0xFF;
      rtp_packet[7] = timestamp & 0xFF;
      rtp_packet[8] = 0; // SSRC identifier (hardcoded to 0)
      rtp_packet[9] = 0;
      rtp_packet[10] = 0;
      rtp_packet[11] = 0;

      // 复制实际数据到 RTP 包
      memcpy(rtp_packet + 12, data, size);

      // 更新序列号和时间戳
      sequence_number++;
      timestamp += 90000 / 15; // 假设每秒30帧

      // 发送 RTP over TCP
      send_rtp_over_tcp(rtp_packet, sizeof(rtp_packet));
    }
    catch (std::exception &e)
    {
      std::cerr << "send_rtp_packet exception: " << e.what() << std::endl;
    }
  }

  void initialize_ffmpeg()
  {
    av_register_all();
    avformat_network_init();
  }

  tcp::socket socket_;
  char data_[1024];
  std::string rtspSessionId_;
  int cseq_;
  int64_t pts_;
};

class RTSPServer
{
public:
  RTSPServer(asio::io_context &io_context, short port)
      : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    auto new_session = std::make_shared<RTSPSession>(io_context_);
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

  asio::io_context &io_context_;
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
