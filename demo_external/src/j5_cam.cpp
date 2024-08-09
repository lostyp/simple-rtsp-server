#include "j5_cam.h"

static void print_pym_v2(pym_buffer_v2_t *pym_buf)
{
     printf("============= sensor_id: %d\n", pym_buf->pym_img_info.sensor_id);
    printf("============= pipeline_id: %d\n", pym_buf->pym_img_info.pipeline_id);
    printf("============= frame_id: %d\n", pym_buf->pym_img_info.frame_id);
    printf("============= time_stamp: %lu\n", pym_buf->pym_img_info.time_stamp);
    printf("============= get image time: %ld.%09ld\n", pym_buf->pym_img_info.tv.tv_sec, pym_buf->pym_img_info.tv.tv_usec * 1000);
    printf("============= buf_index: %d\n", pym_buf->pym_img_info.buf_index);
    printf("============= img_format: %d\n", pym_buf->pym_img_info.img_format);
    printf("============= width: %d\n", pym_buf->src_out.width);
    printf("============= height: %d\n", pym_buf->src_out.height);
    printf("============= stride_size: %d\n", pym_buf->src_out.stride_size);
    printf("============= addr[0]: %p\n", pym_buf->src_out.addr[0]);
    printf("============= addr[1]: %p\n", pym_buf->src_out.addr[1]);
    printf("============= paddr[0]: %p\n", (void*)pym_buf->src_out.paddr[0]);
    printf("============= paddr[1]: %p\n", (void*)pym_buf->src_out.paddr[1]);
    printf("*******************************************************************\n");
}

static void print_vio(hb_vio_buffer_t *vio_buf)
{
    printf("============= sensor_id: %d\n", vio_buf->img_info.sensor_id);
    printf("============= pipeline_id: %d\n", vio_buf->img_info.pipeline_id);
    printf("============= frame_id: %d\n", vio_buf->img_info.frame_id);
    printf("============= time_stamp: %lld\n", vio_buf->img_info.time_stamp);
    printf("============= buf_index: %d\n", vio_buf->img_info.buf_index);
    printf("============= img_format: %d\n", vio_buf->img_info.img_format);
    printf("============= width: %d\n", vio_buf->img_addr.width);
    printf("============= height: %d\n", vio_buf->img_addr.height);
    printf("============= stride_size: %d\n", vio_buf->img_addr.stride_size);
    printf("============= addr[0]: %p\n", vio_buf->img_addr.addr[0]);
    printf("============= addr[1]: %p\n", vio_buf->img_addr.addr[1]);
    printf("============= paddr[0]: %p\n", vio_buf->img_addr.paddr[0]);
    printf("============= paddr[1]: %p\n", vio_buf->img_addr.paddr[1]);
    printf("*******************************************************************\n");
}

static int dumpToFile2plane(char *filename, char *srcBuf, char *srcBuf1,
                            unsigned int size, unsigned int size1)
{
    FILE *yuvFd = NULL;
    char *buffer = NULL;

    yuvFd = fopen(filename, "w+");

    if (yuvFd == NULL)
    {
        printf("open(%s) fail", filename);
        return -1;
    }

    buffer = (char *)malloc(size + size1);

    if (buffer == NULL)
    {
        printf("ERR:malloc file");
        fclose(yuvFd);
        return -1;
    }

    memcpy(buffer, srcBuf, size);
    memcpy(buffer + size, srcBuf1, size1);

    fflush(stdout);

    fwrite(buffer, 1, size + size1, yuvFd);

    fflush(yuvFd);

    if (yuvFd)
        fclose(yuvFd);
    if (buffer)
        free(buffer);

    printf("filedump(%s, size(%d) is successed\n", filename, size);

    return 0;
}

int dumpToFile(char *filename, char *srcBuf, unsigned int size)
{
    FILE *yuvFd = NULL;
    char *buffer = NULL;

    yuvFd = fopen(filename, "w+");

    if (yuvFd == NULL)
    {
        printf("ERRopen(%s) fail", filename);
        return -1;
    }

    buffer = (char *)malloc(size);

    if (buffer == NULL)
    {
        printf(":malloc file");
        fclose(yuvFd);
        return -1;
    }

    memcpy(buffer, srcBuf, size);

    fflush(stdout);

    fwrite(buffer, 1, size, yuvFd);

    fflush(yuvFd);

    if (yuvFd)
        fclose(yuvFd);
    if (buffer)
        free(buffer);

    printf("filedump(%s, size(%d) is successed\n", filename, size);

    return 0;
}

CameraJ5::CameraJ5(int pipe_line_id,bool cim_dma)
{
    pipe_line_id_ = pipe_line_id;
    printf("pipe_line_id:%d\n",pipe_line_id);
    start_ = false;
    save_index_ = 20;
    cim_dma_ = cim_dma;
    if(cim_dma_)
    {
        printf("cim dma flg is true\n");
    }
    else
    {
        printf("cim dma flg is false\n");
    }
}

CameraJ5::~CameraJ5()
{
}

int CameraJ5::Start()
{

    int ret = 1;
    // ret = hb_vio_start_pipeline(pipe_line_id_);
    // if (ret < 0)
    // {
    //     printf("%d hb_vio_start_pipeline pipeid:%d  fail ret %d\n", __LINE__, pipe_line_id_, ret);
    //     return ret;
    // }
    // ret = hb_cam_start(pipe_line_id_);
    // if (ret < 0)
    // {
    //     printf("%d hb_cam_start pipe_line_id_:%d  fail ret %d\n", __LINE__, pipe_line_id_, ret);
    //     return ret;
    // }
    start_ = true;
    grab_thread_ = new std::thread(&CameraJ5::GrabRoutine, this);
    return ret;
}

int CameraJ5::Stop()
{
    start_ = false;
    int ret = -1;
    ret = hb_cam_stop(pipe_line_id_);
    if (ret < 0)
    {
        printf("%d hb_cam_stop pipe_line_id_:%d ret %d\n", __LINE__, pipe_line_id_, ret);
        return ret;
    }
    if (cim_dma_ == false)
    {
        ret = hb_vio_stop_pipeline(pipe_line_id_);
        if (ret < 0)
        {
            printf("%d hb_vio_stop_pipeline pipe_line_id_:%d ret %d\n", __LINE__, pipe_line_id_, ret);
            return ret;
        }
    }
    return ret;
}

void CameraJ5::GrabRoutine()
{
    int ret;
    if(cim_dma_ == false)
    {
        ret = hb_vio_start_pipeline(pipe_line_id_);
        if (ret < 0)
        {
            printf("%d hb_vio_start_pipeline pipeid:%d  fail ret %d\n", __LINE__, pipe_line_id_, ret);
        }
    }
    ret = hb_cam_start(pipe_line_id_);
    if (ret < 0)
    {
        printf("%d hb_cam_start pipe_line_id_:%d  fail ret %d\n", __LINE__, pipe_line_id_, ret);
    }
    while (start_)
    {
        int ret = grabImage();
        if (ret < 0)
        {
            printf("grabImage %d fail,ret %d\n", __LINE__, ret);
            continue;
        }
    }
}

void CameraJ5::SetImageCallback(ImageDataCallback callback,void* userdata)
{
        m_CallBack = callback;
        m_UserData = userdata;
        return;
}

int CameraJ5::grabImage()
{
    int ret = -1;
    if(cim_dma_ == false)
    {
        pym_buffer_v3_t pym_buf;
        ret = hb_vio_get_data(pipe_line_id_, HB_VIO_PYM_DATA_V3, &pym_buf);
        if (ret < 0)
        {
            printf("%d hb_vio_get_data , pipe_line_id_:%d ret %d\n", __LINE__, pipe_line_id_, ret);
            return ret;
        }

        int aa = 0;                  
        for (int j=0; j < pym_buf.pym_img_info.planeCount; j++) {
            printf("%d %d %d\n", j, pym_buf.ds_out[0].width, pym_buf.ds_out[0].height);
            aa += pym_buf.ds_out[0].width * pym_buf.ds_out[0].height;
            printf("aa size = %d \n", aa); 
        }
        aa = pym_buf.ds_out[0].width * pym_buf.ds_out[0].height*1.5;

        m_ExterCallBack(&pym_buf,aa,m_DmaUserData);

        ret = hb_vio_free_pymbuf(pipe_line_id_, HB_VIO_PYM_DATA_V3, &pym_buf);
        if (ret < 0)
        {
            printf("%d,pipe_line_id_:%d,hb_vio_free_pymbuf ret %d\n", __LINE__, pipe_line_id_, ret);
            return ret;
        }
    }
    else
    {
        hb_vio_buffer_t cim_dma_buf;
        ret = hb_cam_get_data(pipe_line_id_, HB_CAM_RAW_DATA, &cim_dma_buf);
        if (ret < 0)
        {
            printf("%d hb_cam_get_data , pipe_line_id_:%d ret %d\n", __LINE__, pipe_line_id_, ret);
            return ret;
        }

        m_DmaCallBack(cim_dma_buf.img_addr.addr[0],
                      cim_dma_buf.img_info.size[0],
                      m_DmaUserData);    // 3840 * 2160  

        ret = hb_cam_free_data(pipe_line_id_, HB_CAM_RAW_DATA, &cim_dma_buf);
        if (ret < 0)
        {
            printf("%d,pipe_line_id_:%d,hb_cam_free_data ret %d\n", __LINE__, pipe_line_id_, ret);
            return ret;
        }
    }
    return ret;
}

void CameraJ5::SetImageDmaCallback(ImageDmaDataCallback dmacallback,void *userdata)
{
    m_DmaCallBack = dmacallback;
    m_DmaUserData = userdata;
}

void CameraJ5::ExterCallBacktest(ExterDataCallback callback,void *userdata)
{
    m_ExterCallBack = callback;
    m_DmaUserData  = userdata;
}