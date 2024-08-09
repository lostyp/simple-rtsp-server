#pragma once
#include "hb_media_codec.h"
#include "hb_media_error.h"
#include "hb_media_muxer.h"

#include "hb_vpm_data_info.h"
#include "hb_vin_data_info.h"
#include "hb_vio_interface.h"
#include <thread>
typedef void (*J5Codec_DataCallback)(int nChan, int nWidth, int nHeight, unsigned char *pData, int nDatalen, void *pUserData);

class CJ5Codec
{
public:
    CJ5Codec();
    ~CJ5Codec();
    int Init(int chan, int width, int height,int pixfmt);
    int Release();
    int Start();
    int Stop();
    int InputData(unsigned char *buffer, int len);
    int InputData2(pym_buffer_v3_t* pym_buf,int len);
    int SetCallback(J5Codec_DataCallback callback, void *user_data);
protected:
    static void on_encoder_input_buffer_available(hb_ptr userdata,media_codec_buffer_t* inputbuffer);
    static void on_encoder_output_buffer_available(hb_ptr userdata,media_codec_buffer_t* outputbuffer,media_codec_output_buffer_info_t* extra_info);
    static void on_encoder_media_codec_message(hb_ptr userdata,hb_s32 error);
    int get_codec_data_thread();
private:
    int init_codec();
    int start_async_encoding();

private:
    int chan_;
    int width_;
    int height_;
    int pixfmt_;
    J5Codec_DataCallback callback_;
    void *user_data_;
    bool first_;
    bool start_;
    mc_video_codec_enc_params_t *enc_params_;
    media_codec_context_t context_;
    std::thread* get_data_thread_;	
    int g_count;
};