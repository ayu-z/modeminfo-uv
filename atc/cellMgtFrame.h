#ifndef __MM_FRAME_H__
#define __MM_FRAME_H__

#include <jansson.h>

#define INVALID "N/A"
#define INVALID_IPV4 "0.0.0.0"
#define INVALID_IPV6 "0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0"
#define COUNT(indices) sizeof(indices) / sizeof(indices[0])
#define GET_SUPPORTED_BANDS(indices) get_bands_by_index(indices, COUNT(indices))

typedef struct {
    int idx; 
    const char *band; 
    const char *info;
} band_t;

typedef enum {
    SIM_ABSENT = 0,
    SIM_NOT_READY = 1,
    SIM_READY = 2,
    SIM_PIN = 3,
    SIM_PUK = 4,
    SIM_NETWORK_PERSONALIZATION = 5,
    SIM_BAD = 6,
} SIM_Status;

typedef struct isp{
    char *longName;
    char *shortName;
    char *code;
} isp_t;

typedef enum netmode{
NET_GSM,
NET_WCDMA,
NET_LTE,
NET_NSA_NR5G,
NET_SA_NR5G,
}netmode_t; 

typedef struct {
    int raw;
    float pct;
    char *level;
}signal_info_t;

typedef struct signal{
    // "GSM"   rssi ber
    // "WCDMA" rssi ecio rscp ber
    // "LTE"   rssi rsrq rsrp snr
    // "NR5G"  lte_rssi lte_rsrq lte_rsrp lte_snr nr5g_rsrq nr5g_rsrp nr5g_snr /* NSA */
    // "NR5G"  nr5g_rsrq nr5g_rsrp nr5g_snr /* SA */
    char *netType;
    signal_info_t lte_rssi;
    signal_info_t lte_rsrq;
    signal_info_t lte_rsrp;
    signal_info_t lte_sinr;
    signal_info_t nr5g_rsrq;
    signal_info_t nr5g_rsrp;
    signal_info_t nr5g_sinr;
    
}signal_t;

typedef struct {
    char *reg_status;   //注册状态
    char *roam_status;  //漫游状态
}sysinfo_t;

typedef struct {
    char *band; //频段
    int lac;    //位置区域码
    char *cellID; //小区ID
    int pcid;   //物理小区ID
    char *UL_bandwidth;     //上行带宽
    char *DL_bandwidth;     //下行带宽
}cellinfo_t;

typedef struct {
    void (*atcRequestOpsInit)(void);
    char *(*requestGetDeviceGMI)(void);
    char *(*requestGetDeviceGMM)(void);
    char *(*requestGetDeviceGMR)(void);
    char *(*requestGetDeviceGSN)(void);
    char *(*requestGetDeviceTemperature)(void);
    char *(*requestGetDeviceSIMStatus)(void);
    char *(*requestGetDeviceSIMSlot)(void);
    char *(*requestGetDeviceSIMICCID)(void);
    char *(*requestGetDeviceSIMIMSI)(void);
    isp_t *(*requestGetDeviceNetISP)(void);
    char *(*requestGetDeviceSIMNUM)(void);
    sysinfo_t *(*requestGetDeviceSysInfo)(void);
    signal_t *(*requestGetDeviceNetSignal)(void);
    cellinfo_t *(*requestGetDeviceNetCellInfo)(void);
    int (*requestGetDeviceEtherNetMode)(void);
    char *(*requestGetDeviceAtRaw)(const char *cmd);

    char *(*requestGetDeviceNetWorkSearchPref)(void);
    char *(*requestSetDeviceNetWorkSearchPref)(const char *pref);

    char *(*requestGetDeviceSupportBandList)(void);
    char *(*requestGetDeviceLockBand)(void);
    char *(*requestSetDeviceLockBand)(const char *band);
}cellMgtFrame_t;
cellMgtFrame_t createAtcRequestOps();
char* get_bands_by_index(int band_indices[], int count);

#endif // !__MM_FRAME_H__