#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <thread>

#include "hb_vio_log.h"
#include "hb_vpm_data_info.h"
#include "hb_vin_data_info.h"
#include "hb_vio_interface.h"

typedef void (*ImageDataCallback)( char *srcBuf, char *srcBuf1,unsigned int size, unsigned int size1,\
                                                                            unsigned int fmtid,void* userData);

typedef void(*ImageDmaDataCallback)(char* srcBuf,unsigned int size,void* userData);

typedef void(*ExterDataCallback)(pym_buffer_v3_t* pym_buf,unsigned int size,void* userData);

class CameraJ5
{
public:
    CameraJ5(int pipe_line_id,bool cim_dma);
    ~CameraJ5();
    int Start();
    int Stop();
    void SetImageCallback(ImageDataCallback callback,void *userdata);
    void SetImageDmaCallback(ImageDmaDataCallback dmacallback,void *userdata);
    void ExterCallBacktest(ExterDataCallback callback,void *userdata);
protected:
    void GrabRoutine();
private:
    int grabImage();
private:
    int pipe_line_id_;
    bool start_;
    std::thread *grab_thread_;
    int save_index_;
    ImageDataCallback m_CallBack;
    ImageDmaDataCallback m_DmaCallBack;
    ExterDataCallback m_ExterCallBack;
    void *m_UserData;
    void *m_DmaUserData;
    bool cim_dma_;
};