#include "cPush.h"

#define ENABLE 1
// #define SUPPORT_SAVE_YUV

FILE *fd_in;

#if ENABLE
void ImageGrabDataCallback(char *srcBuf, char *srcBuf1, unsigned int size, unsigned int size1,
                           unsigned int fmtid, void *userData)
{
        // if (0 != (fmtid % 3))
        // {
        //         return;
        // }

        CJ5Codec *pEnc = (CJ5Codec *)userData;
        int n_datalen = size1 + size;
        if (NULL == pEnc)
        {
                printf("%d,pEnc is NULL.\n", __LINE__);
                return;
        }
        
        void *g_buffer = (char *)malloc(size + size1);
        if (g_buffer == NULL)
        {
                printf("ERR:malloc file.%d\n", __LINE__);
                return;
        }
        memset(g_buffer, 0, n_datalen);
        
        memcpy(g_buffer, srcBuf, size);
        memcpy(g_buffer + size, srcBuf1, size1);

        pEnc->InputData((unsigned char *)g_buffer, n_datalen);

#ifdef SUPPORT_SAVE_YUV
        FILE *pFile = NULL;
        char filename[128] = {0};

        snprintf(filename, sizeof(filename) - 1, "./yuv_%d", fmtid);
        pFile = fopen(filename, "w");
        if (pFile == NULL)
        {
                printf("open file %s failed.\n", filename);
                return;
        }
        fwrite(g_buffer, 1, n_datalen, pFile);
        fclose(pFile);
#endif
        free(g_buffer);
        return;
}

void ImageGrabDmaDataCallback(char* srcBuf,unsigned int size,void* userData)
{
        CJ5Codec *pEnc = (CJ5Codec *)userData;
        int n_datalen = size;
        if (NULL == pEnc)
        {
                printf("%d,pEnc is NULL.\n", __LINE__);
                return;
        }
        void *g_buffer = (char *)malloc(size);
        if (g_buffer == NULL)
        {
                printf("ERR:malloc file.%d\n", __LINE__);
                return;
        }
        memset(g_buffer, 0, n_datalen);
        memcpy(g_buffer, srcBuf, size);
        pEnc->InputData((unsigned char *)g_buffer, n_datalen);

        free(g_buffer);
}

void ExterCodeCallBack(pym_buffer_v3_t* pym_buf,unsigned int size,void* userData)
{
        CJ5Codec *pEnc = (CJ5Codec *)userData;
        int n_datalen = size;
        if (NULL == pEnc)
        {
                printf("%d,pEnc is NULL.\n", __LINE__);
                return;
        } 
        pEnc->InputData2(pym_buf,size); 
}

void VideoenCodeCallback(int nChan, int nWidth, int nHeight, unsigned char *pData, int nDatalen, void *pUserData)
{
        void *pHandle = (ProgressContext *)pUserData;
        if (NULL == pHandle)
        {
                printf("%d,RtppayHandle is NULL.\n", __LINE__);
        }

        RTPPAY_PROCESS_PARAM process_param;
        memset(&process_param, 0, sizeof(process_param));

        process_param.pInData = pData;
        process_param.dwInDataLen = nDatalen;
        process_param.byStreamType = STREAM_TYPE_VIDEO;
        RTPPAY_Process(pHandle, &process_param);

        unsigned char *pOut_data = process_param.pOutData;
        unsigned int current_rtppacket_size = 0;
        int sumsize = 0;

        for (int i = 1; i <= process_param.dwPacketNum; i++)
        {
                current_rtppacket_size = ntohl(*(int *)pOut_data);

                pOut_data[0] = 0x24;
                pOut_data[1] = 0;
                pOut_data[2] = current_rtppacket_size >> 8;
                pOut_data[3] = current_rtppacket_size & 0xff;

                sumsize += (current_rtppacket_size + 4);

                pOut_data += current_rtppacket_size + 4;
        }

        RTSPServer_SendRealSteam(nChan, 0, process_param.pOutData, process_param.dwOutDataLen);
        printf("------------RTSPServer_SendRealSteam-----------\n");
}
#endif

cPush::cPush(int chan, int width, int height, void *rtppayhandle,int cimdma)
{
        m_chan = chan;
        m_width = width;
        m_height = height;
        m_rtppay = rtppayhandle;
        m_cimdma = cimdma;

        pCodec = new CJ5Codec();
        pCam = new CameraJ5(m_chan,cimdma);
}

cPush::~cPush()
{
        hb_cam_deinit(0);
        hb_vio_deinit();
}

void cPush::Init()
{
        RTPPAY_PARAM param;
        memset(&param, 0, sizeof(param));
        param.dwMtu = 1400;
        param.byVideoFps = 30; // 30;
        param.byStreamType = STREAM_TYPE_VIDEO;
        param.byVideoPayloadType = 96;
        param.byVideoStreamType = VIDEO_STREAM_TYPE_H265;
        param.dwAudioSsrc = 100;
        param.dwVideoSsrc = 200; // 200
        param.dwPrivateSsrc = 300;
        m_handle = RTPPAY_CreateHandle(&param);
        if (NULL == m_handle)
        {
                printf("----%d---------rtppay_handle isfailed.-------\n", __LINE__);
        }
}

// test
void cPush::InputData()
{

        fd_in = fopen("./output.yuv", "rb+");
        while (1)
        {
                memset(buffer,0,sizeof(buffer));
                if(fread(buffer, 1, sizeof(buffer), fd_in) < 0)
                {
                        printf("-00000000000000000000000-\n");
                }
                if(feof(fd_in) ==  1)
                {
                        fd_in = fopen("./output.yuv", "rb+");
                        printf("----------------file is end.---------------\n");
                }
                pCodec->InputData(buffer, sizeof(buffer));
                usleep(30 * 1000);
        }
}

void cPush::StartPush()
{
#if ENABLE
        // test
        // m_thread = new std::thread(&cPush::InputData,this);
          
        if(false == m_cimdma)
        {    
                // pCam->SetImageCallback(ImageGrabDataCallback, pCodec);
                pCam->ExterCallBacktest(ExterCodeCallBack,pCodec);
         }
        else
        {
                pCam->SetImageDmaCallback(ImageGrabDmaDataCallback,pCodec);
        }
        pCam->Start();

        pCodec->Init(m_chan, m_width, m_height,0);
        pCodec->SetCallback(VideoenCodeCallback, m_handle); 
        pCodec->Start();
#endif
}

void cPush::StopPush()
{
        pCam->Stop();
        pCodec->Stop();
        pCodec->Release();
}
