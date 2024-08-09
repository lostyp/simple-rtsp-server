#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "rtsps_server_api.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include "mux_common_type.h"
// #include "cjson.h"
#include "rtppay.h"
#include <errno.h>
#include <sys/wait.h>


#include "cPush.h"


#define RTSPS_LISTEN_PORT 554
#define CAM_NUM 1
#define STREAM_TYPE_VIDEO 1
#define RTSPS_VIDEO_STREAM_TYPE_H265 2

#define TEST 1

void inIt(ProgressContext *pCtx)
{
        pCtx->nFps = 30;
        pCtx->nPort = 6677;
        // 推流服务 初始化
        TRtspServerPara struPara;
        memset(&struPara, 0, sizeof(TRtspServerPara));
        struPara.byDbg = 0;
        strncpy(struPara.szUserName, "admin", sizeof(struPara.szUserName));
        strncpy(struPara.szPasswd, "tztek123456", sizeof(struPara.szPasswd));
        struPara.nServerPort = RTSPS_LISTEN_PORT;
        struPara.nMinPort = 22000;
        struPara.nMaxPort = 23000;
        RTSPServer_Init(&struPara);


        int ret = hb_vio_init("/app/cfg/vpm_config.json");

        if (ret < 0)
        {
                printf("%d hb_vio_init fail ret %d\n", __LINE__, ret);
                return;
        }
        ret = hb_cam_init(0, "/app/cfg/hb_j6dev.json");

        if (ret < 0)
        {
                printf("%d hb_cam_init fail ret %d\n", __LINE__, ret);
                return;
        }

        // 封包
        RTPPAY_Init();
}

void destory(ProgressContext *pCtx)
{
}

int main(int argc, char *argv[])
{
        ProgressContext ctx;

        inIt(&ctx);

        int ret;
        cPush *pPush = NULL;

        // config
        // FILE *fd = fopen("./test.json", "r");
        // if (NULL == fd)
        // {
        //         printf("test.json open failed.\n");
        //         return -1;
        // }
        // char aa[2048 * 2048] = {0};
        // fread(aa, 1, 2048, fd);
        // cJSON *config = cJSON_Parse(aa);
        // if (NULL != config)
        // {
        //         cJSON *pCamCfg = cJSON_GetObjectItem(config, "camera");
        //         if (NULL != pCamCfg)
        //         {
        //                 int size = cJSON_GetArraySize(pCamCfg);
        //                 for (auto i = 0; i < size; i++)
        //                 {
        //                         cJSON *pCamChan = cJSON_GetArrayItem(pCamCfg, i);
        //                         int n_enable = -1, n_height = -1, n_width = -1, n_chan = -1,n_cimdma = -1;
        //                         if (NULL != pCamChan)
        //                         {
        //                                 CJSON_ITEM_GET_INT(pCamChan, "enable", n_enable, -1);
        //                                 CJSON_ITEM_GET_INT(pCamChan, "chan", n_chan, -1);
        //                                 CJSON_ITEM_GET_INT(pCamChan, "width", n_width, -1);
        //                                 CJSON_ITEM_GET_INT(pCamChan, "height", n_height, -1);
        //                                 CJSON_ITEM_GET_INT(pCamChan,"cim_dma",n_cimdma,-1);
                                        
        //                                 if (n_enable)
        //                                 {
                                                int n_enable = 1, n_height = 2160, n_width = 3840, n_chan = 0,n_cimdma = 0;
                                                printf("chan is %d,width is %d,height is %d,n_cimdma is %d\n", n_chan, n_width, n_height,n_cimdma);
                                                TRtspServerStreamInfo stuStreamInfo;
                                                stuStreamInfo.byAudioPayload = 0;
                                                stuStreamInfo.dwAudioSSrc = 100;
                                                stuStreamInfo.byVideoPayload = 96;
                                                stuStreamInfo.dwVideoSSrc = 200;
                                                stuStreamInfo.dwVideoFormat = RTSPS_VIDEO_STREAM_TYPE_H265;
                                                RTSPServer_SetStreamInfo(n_chan, 0, &stuStreamInfo);
                                                if (RTSPServer_Start() != 0)
                                                {
                                                        printf("RTSPServer_Start is failed.\n");
                                                }
                                                pPush = new cPush(n_chan, n_width, n_height, ctx.rtppay_handle,n_cimdma);
                                                pPush->Init();
                                                pPush->StartPush();
        //                                 }
        //                         }
        //                 }
        //         }
        // }

        while (1)
        {
                usleep(1000 * 1000);
        }

        // destory
        ret = hb_cam_deinit(0);
        if (ret < 0)
        {
                printf("%d hb_cam_deinit fail ret %d\n", __LINE__, ret);
                return ret;
        }

        ret = hb_vio_deinit();
        if (ret < 0)
        {
                printf("%d hb_vio_deinit fail ret %d\n", __LINE__, ret);
                return ret;
        }

        // rtsps
        RTSPServer_Stop();
        RTSPServer_Release();

        // rtppay
        RTPPAY_DestroyHandle(ctx.rtppay_handle);
        delete ctx.rtppay_handle;

        pPush->StopPush();

        return 0;
}
