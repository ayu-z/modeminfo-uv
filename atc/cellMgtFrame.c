#include "../util/log.h"
#include "cellMgtFrame.h"
#include "atchannel.h"
#include "at_tok.h"
#include <assert.h>
#include <jansson.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
// #include <signal.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <unistd.h>


static void defaultAtcRequestOpsInit(void);
static char *defaultRequestGetDeviceGMI(void);
static char *defaultRequestGetDeviceGMM(void);
static char *defaultRequestGetDeviceGMR(void);
static char *defaultRequestGetDeviceGSN(void);
static char *defaultRequestGetDeviceTemperature(void);
static char *defaultRequestGetDeviceSIMStatus(void);
static char *defaultRequestGetDeviceSIMSlot(void);
static char *defaultRequestGetDeviceSIMICCID(void);
static char *defaultRequestGetDeviceSIMIMSI(void);
static isp_t *defaultRequestGetDeviceNetISP(void);
static char *defaultRequestGetDeviceSIMNUM(void);
static sysinfo_t *defaultRequestGetDeviceSysInfo(void);
static signal_t *defaultRequestGetDeviceNetSignal(void);
static cellinfo_t *defaultRequestGetDeviceNetCellInfo(void);
static int defaultRequestGetDeviceEtherNetMode(void);
static char *defaultRequestGetDeviceAtRaw(const char *cmd);
static char *defaultRequestGetDeviceNetWorkSearchPref(void);
static char *defaultRequestSetDeviceNetWorkSearchPref(const char *pref);
static char *defaultRequestGetDeviceSupportBandList(void);

const cellMgtFrame_t default_atc_request_ops = {
    .atcRequestOpsInit = defaultAtcRequestOpsInit,
    .requestGetDeviceGMI = defaultRequestGetDeviceGMI,
    .requestGetDeviceGMM = defaultRequestGetDeviceGMM,
    .requestGetDeviceGMR = defaultRequestGetDeviceGMR,
    .requestGetDeviceGSN = defaultRequestGetDeviceGSN,
    .requestGetDeviceTemperature = defaultRequestGetDeviceTemperature,
    .requestGetDeviceSIMStatus = defaultRequestGetDeviceSIMStatus,
    .requestGetDeviceSIMSlot = defaultRequestGetDeviceSIMSlot,
    .requestGetDeviceSIMICCID = defaultRequestGetDeviceSIMICCID,
    .requestGetDeviceSIMIMSI = defaultRequestGetDeviceSIMIMSI,
    .requestGetDeviceNetISP = defaultRequestGetDeviceNetISP,
    .requestGetDeviceSIMNUM = defaultRequestGetDeviceSIMNUM,
    .requestGetDeviceSysInfo = defaultRequestGetDeviceSysInfo,
    .requestGetDeviceNetSignal = defaultRequestGetDeviceNetSignal,
    .requestGetDeviceNetCellInfo = defaultRequestGetDeviceNetCellInfo,
    .requestGetDeviceEtherNetMode = defaultRequestGetDeviceEtherNetMode,
    .requestGetDeviceNetWorkSearchPref = defaultRequestGetDeviceNetWorkSearchPref,
    .requestSetDeviceNetWorkSearchPref = defaultRequestSetDeviceNetWorkSearchPref,
    .requestGetDeviceAtRaw = defaultRequestGetDeviceAtRaw,
    .requestGetDeviceSupportBandList = defaultRequestGetDeviceSupportBandList,
};

char *defaultRequestGetDeviceGMI(void){
    char *gmi = INVALID, *tmp = NULL;
    ATResponse *p_response = NULL;

    int err = at_send_command_singleline("AT+GMI", "", &p_response);
    if (!at_response_error(err, p_response)){
        if(strncmp(p_response->p_intermediates->line, "+GMI:", strlen("+GMI:")) == 0){
            if (at_tok_scanf(p_response->p_intermediates->line, "%s", &tmp) == 1)
                gmi = strdup(tmp);
        }else {
            gmi = strdup(p_response->p_intermediates->line);
        }
    }
    safe_at_response_free(p_response);
    return gmi;
}

char *defaultRequestGetDeviceGMM(void){
    char *gmm = INVALID, *tmp = NULL;
    ATResponse *p_response = NULL;

    int err = at_send_command_singleline("AT+GMM", "", &p_response);
    if (!at_response_error(err, p_response)){
        if(strncmp(p_response->p_intermediates->line, "+GMM:", strlen("+GMM:")) == 0){
            if (at_tok_scanf(p_response->p_intermediates->line, "%s", &tmp) == 1)
                gmm = strdup(tmp);
            }else{
                gmm = strdup(p_response->p_intermediates->line);
            }
    }
    safe_at_response_free(p_response);
    return gmm;
}

char *defaultRequestGetDeviceGMR(void){
    char *gmr = INVALID, *tmp = NULL;
    ATResponse *p_response = NULL;

    int err = at_send_command_singleline("AT+GMR", "", &p_response);
    if (!at_response_error(err, p_response)){
        if(strncmp(p_response->p_intermediates->line, "+GMR:", strlen("+GMR:")) == 0){
            if (at_tok_scanf(p_response->p_intermediates->line, "%s", &tmp) == 1)
                gmr = strdup(tmp);
        }else
            gmr = strdup(p_response->p_intermediates->line);
    }
    safe_at_response_free(p_response);
    return gmr;
}

char *defaultRequestGetDeviceGSN(void){
    char *gsn = INVALID, *tmp = NULL;
    ATResponse *p_response = NULL;

    int err = at_send_command_numeric("AT+GSN", &p_response);
    if (!at_response_error(err, p_response)) 
        gsn = strdup(p_response->p_intermediates->line);
    else{
        safe_at_response_free(p_response);
        err = at_send_command_singleline("AT+GSN", "+GSN:", &p_response);
        if (!at_response_error(err, p_response)){
            if (at_tok_scanf(p_response->p_intermediates->line, "%s", &tmp) == 1)
                gsn = strdup(tmp);
        }
    }
    
    safe_at_response_free(p_response);
    return gsn;
}

char *defaultRequestGetDeviceTemperature(void){
    char* temp = "0";
    char *tmp = INVALID;
    ATResponse *p_response = NULL;
    
    int err = at_send_command_singleline("AT+TEMP", "+TEMP: \"modem-skin-usr\"", &p_response);
    if (!at_response_error(err, p_response)){
        if(2 == at_tok_scanf(p_response->p_intermediates->line, "%s%s", NULL, &tmp))
            temp = strdup(tmp);
    } else {
        safe_at_response_free(p_response);
        err = at_send_command_singleline("AT+QTEMP", "+QTEMP: \"soc-thermal\"", &p_response);
        if (!at_response_error(err, p_response)){
            if(2 == at_tok_scanf(p_response->p_intermediates->line, "%s%s", NULL, &tmp))
                temp = strdup(tmp);
        }else{
            safe_at_response_free(p_response);
            err = at_send_command_singleline("AT+QTEMP", "+QTEMP:\"cpuss-0-usr\"", &p_response);
            if (!at_response_error(err, p_response)){
                if(2 == at_tok_scanf(p_response->p_intermediates->line, "%s%s", NULL, &tmp))
                    temp = strdup(tmp);
            }
        }
    }
    safe_at_response_free(p_response);
    return temp;
}

static char *SIMStatus2String(SIM_Status status){
    char *ret = "UNKNOWN";
    switch(status){
        case SIM_ABSENT: 
            ret = "SIM_ABSENT"; 
        break;
        case SIM_NOT_READY: 
            ret = "SIM_NOT_READY"; 
        break;
        case SIM_READY: 
            ret = "SIM_READY"; 
        break;
        case SIM_PIN: 
            ret = "SIM_PIN"; 
        break;
        case SIM_PUK: 
            ret = "SIM_PUK"; 
        break;
        case SIM_NETWORK_PERSONALIZATION: 
            ret = "SIM_NETWORK_PERSONALIZATION"; 
        break;
        case SIM_BAD: 
            ret = "SIM_BAD"; 
        break;
    }

    return ret;
}

char *defaultRequestGetDeviceSIMStatus(void)
{
    int err;
    ATResponse *p_response = NULL;
    char *cpinLine;
    char *cpinResult;
    int ret = SIM_NOT_READY;

    err = at_send_command_singleline("AT+CPIN?", "+CPIN:", &p_response);
    if (at_response_error(err, p_response))
        goto done;

    switch (at_get_cme_error(p_response))
    {
    case CME_SUCCESS:
        break;

    case CME_SIM_NOT_INSERTED:
    case CME_OPERATION_NOT_ALLOWED:
    case CME_FAILURE:
        ret = SIM_ABSENT;
        goto done;

    default:
        ret = SIM_NOT_READY;
        goto done;
    }

    cpinLine = p_response->p_intermediates->line;
    err = at_tok_start (&cpinLine);

    if (err < 0)
    {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_tok_nextstr(&cpinLine, &cpinResult);

    if (err < 0)
    {
        ret = SIM_NOT_READY;
        goto done;
    }

    if (strstr(cpinResult, "SIM PIN"))
    {
        ret = SIM_PIN;
        goto done;
    }
    else if (strstr(cpinResult, "SIM PUK"))
    {
        ret = SIM_PUK;
        goto done;
    }
    else if (strstr(cpinResult, "PH-NET PIN"))
    {
        ret = SIM_NETWORK_PERSONALIZATION;
        goto done;
    }
    else if (!strstr (cpinResult, "READY"))
    {
        ret = SIM_ABSENT;
        goto done;
    }
    ret = SIM_READY;
    goto done;
done:
    safe_at_response_free(p_response);
    return SIMStatus2String(ret);
}

char *defaultRequestGetDeviceSIMSlot(void){
    char* slot = "1";
    char* tmp;
    ATResponse *p_response = NULL;
    
    int err = at_send_command_singleline("AT+QUIMSLOT?", "+QUIMSLOT:", &p_response);
    if (!at_response_error(err, p_response)){
        if(1 == at_tok_scanf(p_response->p_intermediates->line, "%s", &tmp))
            slot = tmp;
    } 
    safe_at_response_free(p_response);

    return slot;
}

char *defaultRequestGetDeviceSIMICCID(void){
    char* iccid = "0";
    char *tmp = NULL;
    ATResponse *p_response = NULL;
    ATLine *p_cur = NULL;
    int i = 0;
    
    int err = at_send_command_singleline("AT+ICCID", "", &p_response);
    if (!at_response_error(err, p_response)){
        for (i = 0, p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next, i++) {
            if(strstr(p_cur->line, "ICCID")){
                if(1 == at_tok_scanf(p_cur->line, "%s", &tmp))
                    iccid = strdup(tmp);
            }
        }
    } else {
        safe_at_response_free(p_response);
        err = at_send_command_singleline("AT+CCID", "", &p_response);
        for (i = 0, p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next, i++) {
            if(strstr(p_cur->line, "CCID")){
                if(1 == at_tok_scanf(p_cur->line, "%s", &tmp))
                    iccid = strdup(tmp);
            }
        }
    }
    safe_at_response_free(p_response);
    return iccid;
}

char *defaultRequestGetDeviceSIMIMSI(void){
    char* imsi = INVALID;
    ATResponse *p_response = NULL;
    int err = at_send_command_numeric("AT+CIMI", &p_response);
    if (!at_response_error(err, p_response)) imsi = strdup(p_response->p_intermediates->line);

    return imsi;
}

isp_t *defaultRequestGetDeviceNetISP(void){
    ATResponse *p_response = NULL;
    int err, ret;
    char cmd[128];
    char *oper="";
    char *tmp=INVALID;
    isp_t *p_isp;
    p_isp = (isp_t *)malloc(sizeof(isp_t));
    p_isp->code = p_isp->longName = p_isp->shortName = INVALID;

    for(int i = 0; i < 3; i++) {
        sprintf(cmd, "AT+COPS=3,%d", i);
        at_send_command(cmd, NULL);
        err = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);
        if (!at_response_error(err, p_response)){
            ret = at_tok_scanf(p_response->p_intermediates->line, "%d%d%s%d", NULL, NULL, &oper, NULL);
            if(ret == 4) {
                tmp = oper;
            } else {
                tmp = INVALID;
            }
        } else {
            tmp = INVALID;
        }
        
        switch (i){
            case 0: p_isp->longName = strdup(tmp); break;
            case 1: p_isp->shortName = strdup(tmp); break;
            case 2: p_isp->code =  strdup(tmp); break;
        }
    }

    safe_at_response_free(p_response);
    return p_isp;
}

char *defaultRequestGetDeviceSIMNUM(void){
    char *num = INVALID, *tmp = NULL;
    ATResponse *p_response = NULL;

    int err = at_send_command_singleline("AT+CNUM", "+CNUM:", &p_response);
    if (!at_response_error(err, p_response)){
        if (at_tok_scanf(p_response->p_intermediates->line, "%s%s%d", NULL, &tmp, NULL) == 3){
            if(strlen(tmp) > 0 ) num = strdup(tmp);
        }  
    }
    safe_at_response_free(p_response);
    return num;
}

char *registerStatus2String(int status){
    char *str = INVALID;
    switch (status) {
        case 0: str = "Not registered"; break;
        case 1: str = "Registered"; break;
        case 2: str = "Searching"; break;
        case 3: str = "Registration denied"; break;
        case 4: str = "Unknown"; break;
        case 5: str = "Registered and Roaming"; break;
    }
    return str;
}

char *roamStatus2String(int status){
    return (status == 5) ? "Roaming" : "Not Roaming";
}

sysinfo_t *defaultRequestGetDeviceSysInfo(void){
    int cops_act;
    int ret;
    ATResponse *p_response = NULL;
    sysinfo_t *p_sysinfo = (sysinfo_t *)malloc(sizeof(sysinfo_t));
    if (p_sysinfo == NULL) {
        return NULL;
    }

    p_sysinfo->reg_status = p_sysinfo->roam_status = INVALID;

    int err = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);
    if (!at_response_error(err, p_response)){
        ret = at_tok_scanf(p_response->p_intermediates->line, "%d%d%s%d", NULL, NULL, NULL, &cops_act);
        if(ret == 4) {
            switch (cops_act) {
                case 2: //UTRAN
                case 4: //UTRAN W/HSDPA
                case 5: //UTRAN W/HSUPA
                case 6: //UTRAN W/HSDPA and HSUPA
                    //AT+CGREG  GPRS Network Registration Status
                    err = at_send_command_singleline("AT+CGREG?", "+CGREG:", &p_response);
                break;

                case 7: //E-UTRAN
                case 13: //E-UTRAN-NR dual connectivity
                    //AT+CEREG  EPS Network Registration Status
                    err = at_send_command_singleline("AT+CEREG?", "+CEREG:", &p_response);
                break;

                case 10: //E-UTRAN connected to a 5GCN
                case 11: //NR connected to a 5GCN
                case 12: //NG-RAN
                    //AT+C5GREG  5GS Network Registration Status
                    err = at_send_command_singleline("AT+C5GREG?", "+C5GREG:", &p_response);
                break;

                default:
                break;
            }

            if (!at_response_error(err, p_response)){
                int stat;
                ret = at_tok_scanf(p_response->p_intermediates->line, "%d%d", NULL, &stat);
                if(ret >= 2) {
                    p_sysinfo->reg_status = registerStatus2String(stat);
                    p_sysinfo->roam_status = roamStatus2String(stat);
                }
            }
        }
    }

    safe_at_response_free(p_response);
    return p_sysinfo;
}

signal_t *defaultRequestGetDeviceNetSignal(void){

    int i=0;
    char *rat = NULL;
    ATLine *p_cur = NULL;
    ATResponse *p_response = NULL;
    signal_t *p_signal = (signal_t *)malloc(sizeof(signal_t));
    int rssi, rsrp, rsrq, sinr;
    if (p_signal == NULL) {
        return NULL;
    }

    p_signal->netType = "NO SERVER";
    // p_signal->rssi.raw = -95;
    // p_signal->rsrp.raw = 0;
    // p_signal->rsrq.raw = 0;
    // p_signal->sinr.raw = 0;

    int err = at_send_command_multiline("at+qeng=\"servingcell\"", "+QENG:", &p_response);
    if (!at_response_error(err, p_response)){
        for (i = 0, p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next, i++) {
        char *type, *state;

        err = at_tok_scanf(p_cur->line, "%s%s", &type, &state);
        if (err != 2 || strcmp(type, "servingcell"))
            continue;

        if (!strcmp(state, "SEARCH") || !strcmp(state, "LIMSRV"))
            continue;

        if (!strcmp(state, "NOCONN") || !strcmp(state, "CONNECT")) {
            err = at_tok_scanf(p_cur->line, "%s%s%s", &type, &state, &rat);
            if (err != 3)
                continue;        
        }
        else {
            rat = state;
        }

        p_signal->netType = strdup(rat);
       
        if (!strcmp(rat, "NR5G-SA"))
        {
            //+QENG: "servingcell",<state>,"NR5G-SA",<duplex_mode>,<MCC>,<MNC>,<cellID>,<PCID>,<TAC>,<ARFCN>,<band>,<NR_DL_bandwidth>,<RSRP>,<RSRQ>,<SINR>,<tx_power>,<srxlev> 
            //+QENG: "servingcell","NOCONN","NR5G-SA","TDD", 454,12,0,21,4ED,636576,78,3,-85,-11,32,0,5184

            err = at_tok_scanf(p_cur->line, "%s,%s,%s,%s,%d,%d,%s,%d,%x,%d,%d,%d,%d,%d,%d,%d",
                NULL, NULL, NULL, NULL,
                NULL, NULL, NULL, NULL, 
                NULL, NULL, NULL, NULL, 
                &rsrp, &rsrq, &sinr, NULL, NULL);
            if (err == 16) {
                p_signal->nr5g_rsrp.raw = rsrp;
                p_signal->nr5g_rsrp.pct = ((float)(p_signal->nr5g_rsrp.raw + 140) / 96) * 100;
                
                p_signal->nr5g_rsrq.raw = rsrq;
                p_signal->nr5g_rsrq.pct = ((float)(p_signal->nr5g_rsrq.raw + 20) / 17) * 100;

                p_signal->nr5g_sinr.raw = sinr;
                p_signal->nr5g_sinr.pct = ((float)(p_signal->nr5g_sinr.raw + 20) / 50) * 100;
                
            }
        }
        else if (!strcmp(rat, "NR5G-NSA"))
        {
            //+QENG: "NR5G-NSA",<MCC>,<MNC>,<PCID>,<RSRP>,< SINR>,<RSRQ>,<ARFCN>,<band>,<bandwidth>,<scs>

            err = at_tok_scanf(p_cur->line, "%s%s%s%s%d%d%d%d",
                NULL, NULL, NULL, NULL, &rsrp, &sinr, &rsrq);
            if (err >= 7){
                p_signal->nr5g_rsrp.raw = rsrp;
                p_signal->nr5g_rsrp.pct = ((float)(p_signal->nr5g_rsrp.raw + 140) / 96) * 100;
                p_signal->nr5g_rsrq.raw = rsrq;
#ifdef RM500U
                p_signal->rsrq.pct = ((float)(p_signal->rsrq.raw + 43) / 63) * 100;
#else      
                p_signal->nr5g_rsrq.pct = ((float)(p_signal->nr5g_rsrq.raw + 20) / 17) * 100;
#endif
                p_signal->nr5g_sinr.raw = sinr;
                p_signal->netType = rat;
                p_signal->nr5g_sinr.pct = ((float)(p_signal->nr5g_sinr.raw + 20) / 50) * 100;
            }
        }
        else if (!strcmp(rat, "LTE"))
        {
            //+QENG: "LTE",<is_tdd>,<MCC>,<MNC>,<cellID>,<PCID>,<earfcn>,<freq_band_ind>,<UL_bandwidth>,<DL_bandwidth>,<TAC>,<RSRP>,<RSRQ>,<RSSI>,<SINR>,<CQI>,<tx_power>,<srxlev>
            if (!strcmp(rat, state)){
                err = at_tok_scanf(p_cur->line, "%s%s%s%s%s%x%d%d%d%d%d%x%d%d%d%d%d%d%d",
                    NULL, NULL, NULL, NULL, NULL, NULL, 
                    NULL, NULL, NULL, NULL, 
                    NULL, NULL, NULL, NULL, 
                    &rssi, NULL, NULL, NULL, NULL);
                
                if (err >= 18) {
                    if(rssi <= 0) p_signal->lte_rssi.raw = -113;
                    if (rssi == 1) p_signal->lte_rssi.raw = -111;
                    if (rssi >= 2 && rssi <= 30) p_signal->lte_rssi.raw = -109 + (rssi - 2) * 2;
                    if (rssi == 31) p_signal->lte_rssi.raw = -51;
                    if (rssi == 99) p_signal->lte_rssi.raw = 99;
                    p_signal->lte_rssi.pct = ((float)(p_signal->lte_rssi.raw + 113) / 62) * 100;
                }
            }
            else{
                 err = at_tok_scanf(p_cur->line, "%s%s%s%s%s%s%x%d%d%d%d%d%x%d%d%d%d%d%d%d",
                    NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
                    NULL, NULL, NULL, NULL, 
                    NULL, NULL, &rsrp, &rsrq, 
                    &rssi, &sinr, NULL, NULL, NULL);
            
                if (err >= 18) {
                    if(rssi <= 0) p_signal->lte_rssi.raw = -113;
                    if (rssi == 1) p_signal->lte_rssi.raw = -111;
                    if (rssi >= 2 && rssi <= 30) p_signal->lte_rssi.raw = -109 + (rssi - 2) * 2;
                    if (rssi == 31) p_signal->lte_rssi.raw = -51;
                    if (rssi == 99) p_signal->lte_rssi.raw = 99;
                    p_signal->lte_rssi.pct = ((float)(p_signal->lte_rssi.raw + 113) / 62) * 100;

                    p_signal->lte_rsrp.raw = rsrp;
                    p_signal->lte_rsrp.pct = ((float)(p_signal->lte_rsrp.raw + 140) / 96) * 100;
                    
                    p_signal->lte_rsrq.raw = rsrq;
                    p_signal->lte_rsrq.pct = ((float)(p_signal->lte_rsrq.raw + 20) / 17) * 100;

                    p_signal->lte_sinr.raw = 2 * sinr - 20;
                    p_signal->lte_sinr.pct = ((float)(p_signal->lte_sinr.raw + 20) / 50) * 100;
            }
            
            }
            
        }
    }
    }
    
    if(p_signal->lte_rssi.raw > -65) p_signal->lte_rssi.level = "Excellent";
    if(p_signal->lte_rssi.raw > -75 && p_signal->lte_rssi.raw <= -65) p_signal->lte_rssi.level = "Good";
    if(p_signal->lte_rssi.raw > -85 && p_signal->lte_rssi.raw <= -75) p_signal->lte_rssi.level = "Fair";
    if(p_signal->lte_rssi.raw > -95 && p_signal->lte_rssi.raw <= -85) p_signal->lte_rssi.level = "Poor";
    if(p_signal->lte_rssi.raw <= -95) p_signal->lte_rssi.level = "No signal";
    if (p_signal->lte_rssi.pct < 0) p_signal->lte_rssi.pct = 0;
    if (p_signal->lte_rssi.pct > 100) p_signal->lte_rssi.pct = 100;

    if (p_signal->nr5g_rsrp.raw >= -80) p_signal->nr5g_rsrp.level = "Excellent";
    if (p_signal->nr5g_rsrp.raw >= -90 && p_signal->nr5g_rsrp.raw < -80) p_signal->nr5g_rsrp.level = "Good";
    if (p_signal->nr5g_rsrp.raw > -100 && p_signal->nr5g_rsrp.raw < -90) p_signal->nr5g_rsrp.level = "Fair";
    if (p_signal->nr5g_rsrp.raw <= -100) p_signal->nr5g_rsrp.level = "Poor";
    if (p_signal->nr5g_rsrp.pct < 0) p_signal->nr5g_rsrp.pct = 0;
    if (p_signal->nr5g_rsrp.pct > 100) p_signal->nr5g_rsrp.pct = 100;

    if (p_signal->nr5g_rsrq.raw >= -10) p_signal->nr5g_rsrq.level = "Excellent";
    if (p_signal->nr5g_rsrq.raw >= -15 && p_signal->nr5g_rsrq.raw < -10) p_signal->nr5g_rsrq.level = "Good";
    if (p_signal->nr5g_rsrq.raw > -20 && p_signal->nr5g_rsrq.raw < -15) p_signal->nr5g_rsrq.level = "Fair";
    if (p_signal->nr5g_rsrq.raw <= -20) p_signal->nr5g_rsrq.level = "Poor";
    if (p_signal->nr5g_rsrq.pct < 0) p_signal->nr5g_rsrq.pct = 0;
    if (p_signal->nr5g_rsrq.pct > 100) p_signal->nr5g_rsrq.pct = 100;

    if (p_signal->nr5g_sinr.raw >= 20) p_signal->nr5g_sinr.level = "Excellent";
    if (p_signal->nr5g_sinr.raw >= 13 && p_signal->nr5g_sinr.raw < 20) p_signal->nr5g_sinr.level = "Good";
    if (p_signal->nr5g_sinr.raw > 0 && p_signal->nr5g_sinr.raw < 13) p_signal->nr5g_sinr.level = "Fair to poor";
    if (p_signal->nr5g_sinr.raw <= 0) p_signal->nr5g_sinr.level = "No signal";
    if (p_signal->nr5g_sinr.pct < 0) p_signal->nr5g_sinr.pct = 0;
    if (p_signal->nr5g_sinr.pct > 100) p_signal->nr5g_sinr.pct = 100;

    at_response_free(p_response);
    return p_signal;
}

char * NR_DL_bandwidth2String(int bandwidth){
#ifdef RM500U
    int len = snprintf(NULL, 0, "%dMHz", bandwidth);
    char* res = (char*)malloc(len + 1);
    if (res == NULL) {
        return INVALID;
    }
    sprintf(res, "%dMHz", bandwidth);
    return res; 
#else
       switch (bandwidth){
            case 0: return "5MHz";
            case 1: return "10MHz";
            case 2: return "15MHz";
            case 3: return "20MHz";
            case 4: return "25MHz";
            case 5: return "30MHz";
            case 6: return "40MHz";
            case 7: return "50MHz";
            case 8: return "60MHz";
            case 9: return "70MHz";
            case 10: return "80MHz";
            case 11: return "90MHz";
            case 12: return "100MHz";
            case 13: return "200MHz";
            case 14: return "400MHz";
       }
#endif       
       return INVALID;
}

char *DLUL_bandwidth2String(int bandwidth){
       switch (bandwidth){
            case 0: return "1.4MHz";
            case 1: return "3MHz";
            case 2: return "5MHz";
            case 3: return "10MHz";
            case 4: return "15MHz";
            case 5: return "20MHz";
       }
       return INVALID;
}

cellinfo_t *defaultRequestGetDeviceNetCellInfo(void){
    char *rat = NULL;
    ATLine *p_cur = NULL;
    ATResponse *p_response = NULL;
    cellinfo_t *p_cellinfo = (cellinfo_t *)malloc(sizeof(cellinfo_t));
    if (!p_cellinfo) return NULL;

    p_cellinfo->band = INVALID;
    p_cellinfo->cellID = INVALID;
    p_cellinfo->pcid = -1;
    p_cellinfo->UL_bandwidth = INVALID;
    p_cellinfo->DL_bandwidth = INVALID;

    char *band; //频段
    char *cellID; //小区ID
    int pcid;   //物理小区ID
    int ul_bandwidth;     //上行带宽
    int dl_bandwidth;

    int i = 0;
    int err = at_send_command_multiline("at+qeng=\"servingcell\"", "+QENG:", &p_response);
    if (!at_response_error(err, p_response)){
        for (i = 0, p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next, i++) {
        char *type, *state;

        err = at_tok_scanf(p_cur->line, "%s%s", &type, &state);
        if (err != 2 || strcmp(type, "servingcell"))
            continue;

        if (!strcmp(state, "SEARCH") || !strcmp(state, "LIMSRV"))
            continue;

        if (!strcmp(state, "NOCONN") || !strcmp(state, "CONNECT")) {
            err = at_tok_scanf(p_cur->line, "%s%s%s", &type, &state, &rat);
            if (err != 3)
                continue;        
        }
        else {
            rat = state;
        }
       
        if (!strcmp(rat, "NR5G-SA"))
        {
            //+QENG: "servingcell",<state>,"NR5G-SA",<duplex_mode>,<MCC>,<MNC>,<cellID>,<PCID>,<TAC>,<ARFCN>,<band>,<NR_DL_bandwidth>,<RSRP>,<RSRQ>,<SINR>,<tx_power>,<srxlev> 
            //+QENG: "servingcell","NOCONN","NR5G-SA","TDD", 454,12,0,21,4ED,636576,78,3,-85,-11,32,0,5184

            err = at_tok_scanf(p_cur->line, "%s,%s,%s,%s,%d,%d,%s,%d,%x,%d,%s,%d,%d,%d,%d,%d",
                NULL, NULL, NULL, NULL,
                NULL, NULL, &cellID, &pcid, 
                NULL, NULL, &band, &dl_bandwidth, 
                NULL, NULL, NULL, NULL, NULL);
            if (err >= 16) {
                p_cellinfo->band = strdup(band);
                p_cellinfo->cellID = strdup(cellID);
                p_cellinfo->pcid = pcid;

                p_cellinfo->DL_bandwidth = NR_DL_bandwidth2String(dl_bandwidth);
                p_cellinfo->UL_bandwidth = NR_DL_bandwidth2String(dl_bandwidth);        
            }
        }
        else if (!strcmp(rat, "NR5G-NSA"))
        {
            //+QENG: "NR5G-NSA",<MCC>,<MNC>,<PCID>,<RSRP>,< SINR>,<RSRQ>,<ARFCN>,<band>,<bandwidth>,<scs>

            err = at_tok_scanf(p_cur->line, "%s%s%s%s%d%d%d%d%s",
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &band);
            if (err >= 7){
                p_cellinfo->band = band;
            }
        }
        else if (!strcmp(rat, "LTE"))
        {
            //+QENG: "LTE",<is_tdd>,<MCC>,<MNC>,<cellID>,<PCID>,<earfcn>,<freq_band_ind>,<UL_bandwidth>,<DL_bandwidth>,<TAC>,<RSRP>,<RSRQ>,<RSSI>,<SINR>,<CQI>,<tx_power>,<srxlev>
            // if (!strcmp(rat, state)){
            //     err = at_tok_scanf(p_cur->line, "%s%s%s%s%s%d%d%d%d%d%d%x%d%d%d%d%d%d%d",
            //         NULL, NULL, NULL, NULL, &cellID, &pcid, 
            //         NULL, NULL, &ul_bandwidth, &dl_bandwidth, 
            //         NULL, NULL, NULL, NULL, 
            //         NULL, NULL, NULL, NULL, NULL);

            //     if (err >= 18) {
            //         p_cellinfo->cellID = cellID;
            //         p_cellinfo->pcid = pcid;
            //         p_cellinfo->DL_bandwidth = DLUL_bandwidth2String(dl_bandwidth);
            //         p_cellinfo->UL_bandwidth = DLUL_bandwidth2String(ul_bandwidth);
            //     }

            // }else{
            //      err = at_tok_scanf(p_cur->line, "%s%s%s%s%s%s%d%d%d%d%d%d%x%d%d%d%d%d%d%d",
            //         NULL, NULL, NULL, NULL, NULL, NULL, &cellID, 
            //         &pcid, NULL, &band, &ul_bandwidth, 
            //         &dl_bandwidth, NULL, NULL, NULL, 
            //         NULL, NULL, NULL, NULL, NULL);

            //     if (err >= 18) {
            //         p_cellinfo->band = band;
            //         p_cellinfo->cellID = cellID;
            //         p_cellinfo->pcid = pcid;
            //         p_cellinfo->DL_bandwidth = DLUL_bandwidth2String(dl_bandwidth);
            //         p_cellinfo->UL_bandwidth = DLUL_bandwidth2String(ul_bandwidth);
            //     }
            // }
            
        }
    }
    }

    return p_cellinfo;
}


int defaultRequestGetDeviceEtherNetMode(void){
    int en = 0, tmp = 0;
    ATResponse *p_response = NULL;

    int err = at_send_command_singleline("AT+QCFG=\"ethernet\"", "+QCFG: \"ethernet\"", &p_response);
    if (!at_response_error(err, p_response)){
        if (at_tok_scanf(p_response->p_intermediates->line, "%s%d", NULL, &tmp) == 2){
            en = tmp;
        }  
    }
    safe_at_response_free(p_response);
    return en;
}

char *defaultRequestGetDeviceNetWorkSearchPref(void){
    char *tmp = "AUTO";
    ATResponse *p_response = NULL;

    int err = at_send_command_singleline("AT+QNWPREFCFG=\"mode_pref\"", "+QNWPREFCFG: \"mode_pref\"", &p_response);
    if (!at_response_error(err, p_response)){
        if(at_tok_scanf(p_response->p_intermediates->line, "%s%s", NULL, &tmp) == 2){
            safe_at_response_free(p_response);
            return tmp;
        }
    }
    return tmp;
}

char *defaultRequestSetDeviceNetWorkSearchPref(const char *pref){
    char *result = "FAILURE";
    
    if (pref == NULL || strlen(pref) == 0) {
        return result;
    }
    int err = 0;
    char cmd[32] = {0};
    ATResponse *p_response = NULL;
    if((strcmp(pref, "AUTO") == 0) || (strcmp(pref, "LTE") == 0) || (strcmp(pref, "NR5G") == 0)){
        at_send_command("AT+CFUN=0",NULL);
        sprintf(cmd, "AT+QNWPREFCFG=\"mode_pref\",%s", pref);
        err = at_send_command(cmd, &p_response);
        if (!at_response_error(err, p_response)){
            result = "SUCCESS";
        }
        at_send_command("AT+CFUN=1",NULL);
    }
    safe_at_response_free(p_response);
    LOGD("Set network search preference %s: %s", pref, result);
    return result;
}

void defaultAtcRequestOpsInit(void){
    // at_send_command_multiline("AT+CREG=0;AT+CEREG=0;AT+C5GREG=0;","", NULL);
    at_send_command("AT+CREG=0", NULL);
    at_send_command("AT+CEREG=0", NULL);
    at_send_command("AT+C5GREG=0", NULL);
}


static char *defaultRequestGetDeviceAtRaw(const char *cmd){
    if( cmd == NULL) 
        return "ERROR";

    int i = 0;
    ATLine *p_cur = NULL;
    ATResponse *p_response = NULL;
    char response[1024] = "\0";
    int err = at_send_command_multiline(cmd, "", &p_response);
    if (!at_response_error(err, p_response)){
        for (i = 0, p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next, i++) {
            strcat(response, p_cur->line);
            strcat(response, "\n");
        }
    }else {
        sprintf(response, "%s(%d)", "ERROR", err);
    }
    return strdup(response);
}


typedef enum {
    N1 = 1,
    N2,
    N3 = 3,
    N5 = 5,
    N7 = 7,
    N8,
    N12 = 12,
    N13,
    N14,
    N18 = 18,
    N20 = 20,
    N24 = 24,
    N25,
    N26,
    N28 = 28,
    N29,
    N30,
    N34 = 34,
    N38 = 38,
    N39,
    N40,
    N41,
    N46 = 46,
    N47,
    N48,
    N50 = 50,
    N51,
    N53 = 53,
    N65 = 65,
    N66,
    N67,
    N70 = 70,
    N71,
    N74 = 74,
    N75,
    N76,
    N77,
    N78,
    N79,
    N80,
    N81,
    N82,
    N83,
    N84,
    N85,
    N86,
    N89 = 89,
    N90,
    N91,
    N92,
    N93,
    N94,
    N95,
    N96,
    N97,
    N98,
    N99,
    N100 = 100,
    N101,
    N102,
    N104 = 104,
    N105,
    BAND_COUNT
} bandIdx_t;

const band_t bands[BAND_COUNT] = {
    {N1, "N1", "FDD/2100MHz"},
    {N2, "N2", "FDD/1900MHz"},
    {N3, "N3", "FDD/1800MHz"},
    {N5, "N5", "FDD/850MHz"},
    {N7, "N7", "FDD/2600MHz"},
    {N8, "N8", "FDD/900MHz"},
    {N12, "N12", "FDD/700MHz"},
    {N13, "N13", "FDD/700MHz"},
    {N14, "N14", "FDD/700MHz"},
    {N18, "N18", "FDD/850MHz"},
    {N20, "N20", "FDD/800MHz"},
    {N24, "N24", "FDD/1600MHz"},
    {N25, "N25", "FDD/1900MHz"},
    {N26, "N26", "FDD/850MHz"},
    {N28, "N28", "FDD/700MHz"},
    {N29, "N29", "SDL/700MHz"},
    {N30, "N30", "FDD/2300MHz"},
    {N34, "N34", "TDD/2100MHz"},
    {N38, "N38", "TDD/2600MHz"},
    {N39, "N39", "TDD/1900MHz"},
    {N40, "N40", "TDD/2300MHz"},
    {N41, "N41", "TDD/2500MHz"},
    {N46, "N46", "TDD/5200MHz"},
    {N47, "N47", "TDD/5900MHz"},
    {N48, "N48", "TDD/3500MHz"},
    {N50, "N50", "TDD/1500MHz"},
    {N51, "N51", "TDD/1500MHz"},
    {N53, "N53", "TDD/2500MHz"},
    {N65, "N65", "FDD/2100MHz"},
    {N66, "N66", "FDD/1700MHz"},
    {N67, "N67", "SDL/700MHz"},
    {N70, "N70", "FDD/2000MHz"},
    {N71, "N71", "FDD/600MHz"},
    {N74, "N74", "FDD/1500MHz"},
    {N75, "N75", "SDL/1500MHz"},
    {N76, "N76", "SDL/1500MHz"},
    {N77, "N77", "TDD/3700MHz"},
    {N78, "N78", "TDD/3500MHz"},
    {N79, "N79", "TDD/4700MHz"},
    {N80, "N80", "SUL/1800MHz"},
    {N81, "N81", "SUL/900MHz"},
    {N82, "N82", "SUL/800MHz"},
    {N83, "N83", "SUL/700MHz"},
    {N84, "N84", "SUL/2100MHz"},
    {N85, "N85", "FDD/700MHz"},
    {N86, "N86", "SUL/1700MHz"},
    {N89, "N89", "SUL/850MHz"},
    {N90, "N90", "TDD/2500MHz"},
    {N91, "N91", "FDD/80MHz"},
    {N92, "N92", "FDD/800MHz"},
    {N93, "N93", "FDD/900MHz"},
    {N94, "N94", "FDD/900MHz"},
    {N95, "N95", "SUL/2100MHz"},
    {N96, "N96", "TDD/6000MHz"},
    {N97, "N97", "SUL/2300MHz"},
    {N98, "N98", "SUL/1900MHz"},
    {N99, "N99", "SUL/1600MHz"},
    {N100, "N100", "FDD/900MHz"},
    {N101, "N101", "TDD/1900MHz"},
    {N102, "N102", "TDD/6200MHz"},
    {N104, "N104", "TDD/6700MHz"},
    {N105, "N105", "FDD/600MHz"}
};

char* get_bands_by_index(int band_indices[], int count) {
    char arr[512] = "\0";
    char tmp[64] = "\0";
    int idx = -1;
    for (int i = 0; i < count; i++) {
        for(int j=0; j<BAND_COUNT; j++) {
            if(bands[j].idx == band_indices[i]) {
                idx = j;
                break;
            }
        }
        if (idx >= 0 && idx < BAND_COUNT) {
            snprintf(tmp, sizeof(tmp), "%s/%s\n", bands[idx].band, bands[idx].info);
            strcat(arr, tmp);
        }
    }
    return strdup(arr);
}

static char *defaultRequestGetDeviceSupportBandList(){
    int indices[] = { N1, N2, N3, N5, N7, N8, N12, N13, N14, N18, N20, 
                    N25, N26, N28, N29, N30, N38, N40, N41, N48, 
                    N66, N70, N71, N75, N76, N77, N78, N79};
    
    return GET_SUPPORTED_BANDS(indices);
}

cellMgtFrame_t createAtcRequestOps(){
    return  default_atc_request_ops;
}



#ifdef TEST
int main(int argc, char **argv){

    int ret = at_open(argv[1], NULL, 0);
    if(ret < 0) return ret;

    at_handshake();
    default_atc_request_ops.atcRequestOpsInit();
    usleep(100);
    char *gmi = default_atc_request_ops.requestGetDeviceGMI();
    usleep(100);
    char *gmm = default_atc_request_ops.requestGetDeviceGMM();
    usleep(100);
    char *gmr = default_atc_request_ops.requestGetDeviceGMR();
    usleep(100);
    char *gsn = default_atc_request_ops.requestGetDeviceGSN();
    usleep(100);
    char* temp = default_atc_request_ops.requestGetDeviceTemperature();
    usleep(100);
    char* simStatus = default_atc_request_ops.requestGetDeviceSIMStatus();
    usleep(100);
    int simslot = default_atc_request_ops.requestGetDeviceSIMSlot();
    usleep(100);
    char* iccid = default_atc_request_ops.requestGetDeviceSIMICCID();
    usleep(100);
    char* imsi = default_atc_request_ops.requestGetDeviceSIMIMSI();
    usleep(100);
    isp_t *isp = default_atc_request_ops.requestGetDeviceNetISP();
    usleep(100);
    char* num = default_atc_request_ops.requestGetDeviceSIMNUM();
    usleep(100);
    sysinfo_t *sysinfo = default_atc_request_ops.requestGetDeviceSysInfo();
    usleep(100);
    signal_t *signal = default_atc_request_ops.requestGetDeviceNetSignal();
    usleep(100);
    cellinfo_t *cellinfo = default_atc_request_ops.requestGetDeviceNetCellInfo();
    usleep(100);
    int eth = default_atc_request_ops.requestGetDeviceEtherNetMode();
    usleep(100);
    at_close();

    printf("Manufacturer:%s\n", gmi);
    printf("Model:%s\n", gmm);
    printf("Revision:%s\n", gmr);
    printf("IMEI:%s\n", gsn);
    printf("Temperature:%s\n", temp);
    printf("SIM STATUS:%s\n", simStatus);
    printf("SIM SLOT:%d\n", simslot);
    printf("ICCID:%s\n", iccid);
    printf("IMSI:%s\n", imsi);
    printf("OperatorCode:%s\n", isp->code);
    printf("OperatorLcr:%s\n", isp->longName);
    printf("OperatorScr:%s\n", isp->shortName);
    printf("NUM:%s\n", num);
    printf("Registered Status:%s\n", sysinfo->reg_status);
    printf("Roaming Status:%s\n", sysinfo->roam_status);
    printf("netType:%s\n", signal->netType);
    printf("RSSI RAW:%d\n", signal->rssi.raw);
    printf("RSSI PERCENT:%.2f\n", signal->rssi.pct);
    printf("RSSI LEVEL:%s\n", signal->rssi.level);
    printf("RSRQ RAW:%d\n", signal->rsrq.raw);
    printf("RSRQ PERCENT:%f\n", signal->rsrq.pct);
    printf("RSRQ LEVEL:%s\n", signal->rsrq.level);
    printf("RSRP RAW:%d\n", signal->rsrp.raw);
    printf("RSRP PERCENT:%.2f\n", signal->rsrp.pct);
    printf("RSRP LEVEL:%s\n", signal->rsrp.level);
    printf("SINR RAW:%d\n", signal->sinr.raw);
    printf("SINR PERCENT:%.2f\n", signal->sinr.pct);
    printf("SINR LEVEL:%s\n", signal->sinr.level);
    printf("Cell ID:%s\n", cellinfo->cellID);
    printf("PCID:%d\n", cellinfo->pcid);
    printf("BAND:%s\n", cellinfo->band);
    printf("UL Bandwidth:%s\n", cellinfo->UL_bandwidth);
    printf("DL Bandwidth:%s\n", cellinfo->DL_bandwidth);
    printf("ETHERNET MODE:%d\n", eth);

    return 0;
}
#endif