#include "j5_codec.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

void CJ5Codec::on_encoder_input_buffer_available(hb_ptr userdata, media_codec_buffer_t *inputbuffer)
{
}
void CJ5Codec::on_encoder_output_buffer_available(hb_ptr userdata, media_codec_buffer_t *outputbuffer, media_codec_output_buffer_info_t *extra_info)
{
    CJ5Codec *j5_codec = (CJ5Codec *)userdata;
    mc_h264_h265_output_stream_info_t info = extra_info->video_stream_info;
}
void CJ5Codec::on_encoder_media_codec_message(hb_ptr userdata, hb_s32 error)
{
}

CJ5Codec::CJ5Codec()
{
}

CJ5Codec::~CJ5Codec()
{
}

int CJ5Codec::Init(int chan, int width, int height,int pixfmt)
{
    chan_ = chan;
    width_ = width;
    height_ = height;
    pixfmt_ = pixfmt;
    g_count = 0;
    return init_codec();
}

int CJ5Codec::Release()
{
    int ret;
    do
    {
        ret = hb_mm_mc_stop(&context_);
        if (ret < 0)
        {
            printf("hb_mm_mc_stop failed,ret:%d\n", ret);
            break;
        }
        ret = hb_mm_mc_release(&context_);
        if (ret < 0)
        {
            printf("hb_mm_mc_release failed,ret:%d\n", ret);
            break;
        }
        return 0;
    } while (0);
    printf("CJ5Codec::Release failed\n");
    return -1;
}

int CJ5Codec::Start()
{
    start_ = true;
    get_data_thread_ = new (std::nothrow) std::thread(&CJ5Codec::get_codec_data_thread, this);
    return start_async_encoding();
}

int CJ5Codec::Stop()
{
    start_ = false;
    if (get_data_thread_)
    {
        get_data_thread_->join();
        delete get_data_thread_;
        get_data_thread_ = nullptr;
    }
    return 0;
}

int CJ5Codec::InputData(unsigned char *buffer, int len)
{
    int ret;
    void *buf_ptr = NULL;
    size_t buf_size = 0;
    media_codec_buffer_t inputBuffer;
    ret = hb_mm_mc_dequeue_input_buffer(&context_, &inputBuffer, 30);    // 3000
    if (ret < 0)
    {
        printf("hb_mm_mc_dequeue_input_buffer failed,ret:%d\n", ret);
        return -1;
    }

    buf_ptr = inputBuffer.vframe_buf.vir_ptr[0];
    memcpy(buf_ptr, buffer, len*2/3);
    buf_ptr = inputBuffer.vframe_buf.vir_ptr[1];
    buffer+=len*2/3;
    memcpy(buf_ptr, buffer, len/3);
    inputBuffer.vframe_buf.size = len;

    ret = hb_mm_mc_queue_input_buffer(&context_, &inputBuffer, 100);
    if (ret < 0)
    {
        printf("hb_mm_mc_queue_input_buffer failed,ret:%d\n", ret);
        return -1;
    }

    return 0;
}

int CJ5Codec::InputData2(pym_buffer_v3_t *pym_buf, int len)
{

    printf("hb_mm_mc_dequeue_input_buffer start.\n");

    int ret;
    void *buf_ptr = NULL;
    size_t buf_size = 0;
    media_codec_buffer_t inputBuffer;
    memset(&inputBuffer, 0x00, sizeof(media_codec_buffer_t));
    ret = hb_mm_mc_dequeue_input_buffer(&context_, &inputBuffer, 3000);
    if (ret < 0)
    {
        printf("hb_mm_mc_dequeue_input_buffer failed,ret:%d\n", ret);
        return -1;
    }

    inputBuffer.vframe_buf.size = len;
    // for (int j=0; j < 3; j++) {
    inputBuffer.vframe_buf.vir_ptr[0] = (hb_u8 *)pym_buf->ds_out[0].addr[0];
    inputBuffer.vframe_buf.phy_ptr[0] = (hb_u64)pym_buf->ds_out[0].paddr[0];

    inputBuffer.vframe_buf.vir_ptr[1] = (hb_u8 *)pym_buf->ds_out[0].addr[1];
    inputBuffer.vframe_buf.phy_ptr[1] = (hb_u64)pym_buf->ds_out[0].paddr[1];

    // inputBuffer.vframe_buf.vir_ptr[2] = (hb_u8 *)addr3;
    // inputBuffer.vframe_buf.phy_ptr[2] = (hb_u32)paddr3;
    //inputBuffer.vframe_buf.fd[0] = pym_buf->ds_out[0].fd[0];
    //inputBuffer.vframe_buf.fd[1] = pym_buf->ds_out[0].fd[1];

    // }
    ret = hb_mm_mc_queue_input_buffer(&context_, &inputBuffer, 3000);
    if (ret < 0)
    {
        printf("hb_mm_mc_queue_input_buffer failed,ret:%d\n", ret);
        return -1;
    }
    printf("hb_mm_mc_dequeue_input_buffer finish.\n");

    return 0;
}

int CJ5Codec::SetCallback(J5Codec_DataCallback callback, void *user_data)
{
    callback_ = callback;
    user_data_ = user_data;
    return 0;
}

int CJ5Codec::init_codec()
{
    int ret;

    memset(&context_, 0x00, sizeof(media_codec_context_t));
    context_.codec_id = MEDIA_CODEC_ID_H265;
    context_.encoder = 1;

    enc_params_ = &context_.video_enc_params;
    enc_params_->width = width_;
    enc_params_->height = height_;
    if(pixfmt_ == 0)
    {
        enc_params_->pix_fmt = MC_PIXEL_FORMAT_NV12;
    }
    else
    {
        enc_params_->pix_fmt = MC_PIXEL_FORMAT_YUV420P;
    }
    enc_params_->frame_buf_count = 5;   
    enc_params_->external_frame_buf = 1;   
    enc_params_->bitstream_buf_count = 5;  
	enc_params_->h265_enc_config.lossless_mode = 0;
	enc_params_->h265_enc_config.tmvp_enable =1;
    enc_params_->rot_degree = MC_CCW_0;
    enc_params_->rc_params.mode = MC_AV_RC_MODE_H265CBR;
    ret = hb_mm_mc_get_rate_control_config(&context_, &enc_params_->rc_params);
    if (ret < 0)
    {
        printf("hb_mm_mc_get_rate_control_config failed,ret:%d", ret);
        return -1;
    }
    enc_params_->rc_params.h265_cbr_params.bit_rate = 5000;

    enc_params_->rc_params.h265_cbr_params.frame_rate = 30;
    enc_params_->rc_params.h265_cbr_params.intra_period = 30;
    enc_params_->gop_params.gop_preset_idx = 2;
    enc_params_->gop_params.decoding_refresh_type = 2;
    enc_params_->mir_direction = MC_DIRECTION_NONE;
    enc_params_->frame_cropping_flag = false;
    return 0;   
}

int CJ5Codec::start_async_encoding()
{
    int ret;
    ret = hb_mm_mc_initialize(&context_);
    if (ret < 0)
    {
        printf("hb_mm_mc_initialize failed,ret:%d\n", ret);
        return -1;
    }
    
    if (ret < 0)
    {
        printf("hb_mm_mc_set_callback failed,ret:%d\n", ret);
        return -1;
    }
    ret = hb_mm_mc_configure(&context_);
    if (ret < 0)
    {
        printf("hb_mm_mc_configure failed,ret:%d\n", ret);
        return -1;
    }
    mc_av_codec_startup_params_t startup_params;
    startup_params.video_enc_startup_params.receive_frame_number = 0;
    ret = hb_mm_mc_start(&context_, &startup_params);
    if (ret < 0)
    {
        printf("hb_mm_mc_start failed,ret:%d\n", ret);
        return -1;
    }
    return 0;
}

int CJ5Codec::get_codec_data_thread()
{
    int ret;
    media_codec_buffer_t outputBuffer;
    media_codec_output_buffer_info_t info;
    while (start_)
    {
        memset(&outputBuffer, 0x00, sizeof(media_codec_buffer_t));
        memset(&info, 0x00, sizeof(media_codec_output_buffer_info_t));
        ret = hb_mm_mc_dequeue_output_buffer(&context_, &outputBuffer, &info, 3000);   // 3000
        if (ret < 0)
        {
            printf("hb_mm_mc_dequeue_output_buffer failed,ret:%d\n", ret);
            usleep(30 * 1000);
            continue;
        }
        // printf("output bufferviraddr %p phy addr %x, size = %d\n",
        //        outputBuffer.vstream_buf.vir_ptr, outputBuffer.vstream_buf.phy_ptr,
        //        outputBuffer.vstream_buf.size);

        callback_(chan_, width_, height_, outputBuffer.vstream_buf.vir_ptr, outputBuffer.vstream_buf.size, user_data_);

        ret = hb_mm_mc_queue_output_buffer(&context_, &outputBuffer, 100);
        if (ret < 0)
        {
            printf("hb_mm_mc_queue_output_buffer failed,ret:%d\n", ret);
            continue;
        }
    }
    return 0;
}
