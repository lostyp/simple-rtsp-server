#include "j5_cam.h"
#include "j5_codec.h"
#include "mux_common_type.h"
#include "rtppay.h"
#include "rtsps_server_api.h"
#include <arpa/inet.h>
#include <thread>

#define STREAM_TYPE_VIDEO 1

typedef struct tagProgressContext
{
        FILE* fd;
        int nFps;
        int nPort;
        void* rtppay_handle;
        TRtspServerStreamInfo stuStreamInfo;
}ProgressContext;

class cPush
{
// protected:
    // void ImageGrabDataCallback( char *srcBuf, char *srcBuf1,unsigned int size, unsigned int size1,\
                                                                unsigned int fmtid,void* userData);
    // void VideoenCodeCallback(int nChan, int nWidth, int nHeight, unsigned char *pData, int nDatalen, void *pUserData);
public:
    cPush(int chan,int width,int height,void* rtppayhandle,int cimdma);    
    ~cPush();
    void StartPush();
    void StopPush();
    void InputData();
    void Init();
private:
    int m_height;
    int m_width;
    void* m_rtppay;
    int m_chan;
    bool m_cimdma;
    CJ5Codec *pCodec;
    CameraJ5 *pCam;
    std::thread* m_thread;
    void * m_handle;
      unsigned  char buffer[3840*2160*3/2];

    // char *g_buffer;
};
