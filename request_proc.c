#include "util/log.h"
#include "atc/at_tok.h"
#include "atc/atchannel.h"
#include "atc/cellMgtFrame.h"
#include "http_server.h"
#include <jansson.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static cellMgtFrame_t cmf;

static json_t *pack_isp_json(isp_t *isp) {
    json_t *root = json_object();
    json_t *isp_obj = json_object();
    json_object_set_new(isp_obj, "code", json_string(isp->code));
    json_object_set_new(isp_obj, "lcr", json_string(isp->longName));
    json_object_set_new(isp_obj, "scr", json_string(isp->shortName));
    json_object_set_new(root, "operator", isp_obj);
    return root;
}

static json_t *pack_sysinfo_json(sysinfo_t *sys) {
    json_t *root = json_object();
    json_t *sys_obj = json_object();
    json_object_set_new(sys_obj, "registered", json_string(sys->reg_status));
    json_object_set_new(sys_obj, "roaming", json_string(sys->roam_status));
    json_object_set_new(root, "sysinfo", sys_obj);
    return root;
}

static json_t *pack_signal_json(signal_t *sig) {
    json_t *root = json_object();
    json_t *sig_obj = json_object();
    json_object_set_new(root, "nettype", json_string(sig->netType));
    if(strstr(sig->netType, "LTE")){
        json_object_set_new(sig_obj, "rssi raw", json_integer(sig->lte_rssi.raw));
        json_object_set_new(sig_obj, "rssi pct", json_real(sig->lte_rssi.pct));
        json_object_set_new(sig_obj, "rssi lv", json_string(sig->lte_rssi.level));
        json_object_set_new(sig_obj, "rsrq raw", json_integer(sig->lte_rsrq.raw));
        json_object_set_new(sig_obj, "rsrq pct", json_real(sig->lte_rsrq.pct));
        json_object_set_new(sig_obj, "rsrq lv", json_string(sig->lte_rsrq.level));
        json_object_set_new(sig_obj, "rsrp raw", json_integer(sig->lte_rsrp.raw));
        json_object_set_new(sig_obj, "rsrp pct", json_real(sig->lte_rsrp.pct));
        json_object_set_new(sig_obj, "rsrp lv", json_string(sig->lte_rsrp.level));
        json_object_set_new(sig_obj, "sinr raw", json_integer(sig->lte_sinr.raw));
        json_object_set_new(sig_obj, "sinr pct", json_real(sig->lte_sinr.pct));
        json_object_set_new(sig_obj, "sinr lv", json_string(sig->lte_sinr.level));
        
    }
    else if (strstr(sig->netType, "NSA")) {
        json_object_set_new(sig_obj, "nr5g rsrq raw", json_integer(sig->nr5g_rsrq.raw));
        json_object_set_new(sig_obj, "nr5g rsrq pct", json_real(sig->nr5g_rsrq.pct));
        json_object_set_new(sig_obj, "nr5g rsrq lv", json_string(sig->nr5g_rsrq.level));
        json_object_set_new(sig_obj, "nr5g rsrp raw", json_integer(sig->nr5g_rsrp.raw));
        json_object_set_new(sig_obj, "nr5g rsrp pct", json_real(sig->nr5g_rsrp.pct));
        json_object_set_new(sig_obj, "nr5g rsrp lv", json_string(sig->nr5g_rsrp.level));
        json_object_set_new(sig_obj, "nr5g sinr raw", json_integer(sig->nr5g_sinr.raw));
        json_object_set_new(sig_obj, "nr5g sinr pct", json_real(sig->nr5g_sinr.pct));
        json_object_set_new(sig_obj, "nr5g sinr lv", json_string(sig->nr5g_sinr.level));
        
    }
    else if (strstr(sig->netType, "SA")) {
        json_object_set_new(sig_obj, "lte rssi raw", json_integer(sig->lte_rssi.raw));
        json_object_set_new(sig_obj, "lte rssi pct", json_real(sig->lte_rssi.pct));
        json_object_set_new(sig_obj, "lte rssi lv", json_string(sig->lte_rssi.level));
        json_object_set_new(sig_obj, "lte rsrq raw", json_integer(sig->lte_rsrq.raw));
        json_object_set_new(sig_obj, "lte rsrq pct", json_real(sig->lte_rsrq.pct));
        json_object_set_new(sig_obj, "lte rsrq lv", json_string(sig->lte_rsrq.level));
        json_object_set_new(sig_obj, "lte rsrp raw", json_integer(sig->lte_rsrp.raw));
        json_object_set_new(sig_obj, "lte rsrp pct", json_real(sig->lte_rsrp.pct));
        json_object_set_new(sig_obj, "lte rsrp lv", json_string(sig->lte_rsrp.level));
        json_object_set_new(sig_obj, "lte sinr raw", json_integer(sig->lte_sinr.raw));
        json_object_set_new(sig_obj, "lte sinr pct", json_real(sig->lte_sinr.pct));
        json_object_set_new(sig_obj, "lte sinr lv", json_string(sig->lte_sinr.level));
        json_object_set_new(sig_obj, "nr5g rsrq raw", json_integer(sig->nr5g_rsrq.raw));
        json_object_set_new(sig_obj, "nr5g rsrq pct", json_real(sig->nr5g_rsrq.pct));
        json_object_set_new(sig_obj, "nr5g rsrq lv", json_string(sig->nr5g_rsrq.level));
        json_object_set_new(sig_obj, "nr5g rsrp raw", json_integer(sig->nr5g_rsrp.raw));
        json_object_set_new(sig_obj, "nr5g rsrp pct", json_real(sig->nr5g_rsrp.pct));
        json_object_set_new(sig_obj, "nr5g rsrp lv", json_string(sig->nr5g_rsrp.level));
        json_object_set_new(sig_obj, "nr5g sinr raw", json_integer(sig->nr5g_sinr.raw));
        json_object_set_new(sig_obj, "nr5g sinr pct", json_real(sig->nr5g_sinr.pct));
        json_object_set_new(sig_obj, "nr5g sinr lv", json_string(sig->nr5g_sinr.level));
    }else {
        json_object_set_new(sig_obj, "status", json_string("invalid data"));
    }
    json_object_set_new(root, "signal", sig_obj);
    char *json_string = json_dumps(root, JSON_INDENT(0));
    return root;
}

static json_t *pack_cellinfo_json(cellinfo_t *cell) {
    json_t *root = json_object();
    json_t *cell_obj = json_object();
    json_object_set_new(cell_obj, "band", json_string(cell->band));
    json_object_set_new(cell_obj, "lac", json_integer(cell->lac));
    json_object_set_new(cell_obj, "cellid", json_string(cell->cellID));
    json_object_set_new(cell_obj, "pcid", json_integer(cell->pcid));
    json_object_set_new(cell_obj, "ul_bandwidth", json_string(cell->UL_bandwidth));
    json_object_set_new(cell_obj, "dl_bandwidth", json_string(cell->DL_bandwidth));
    json_object_set_new(root, "cellinfo", cell_obj);
    return root;
}

json_t* fmt_raw2json(const char *input) {
    if(input == NULL) 
        return NULL;
    char *cp = strdup(input);
    json_t *arr = json_array();

    char *token = strtok(cp, "\n");
    while (token != NULL) {
        json_array_append_new(arr, json_string(token));
        token = strtok(NULL, "\n");
    }
    free(cp);
    return arr;
}

static json_t *pack_band_json(const char *input) {
    json_t *s_root = json_object();
    json_object_set_new(s_root, "supportband", fmt_raw2json(input));
    return s_root;
}

#if 0
static json_t *pack_atraw_json(const char *input) {
    return fmt_raw2json(input);;
}
#endif

static json_t *pack_atraw_json(const char *input) {
    if (input == NULL) {
        LOGE("Input is NULL\n");
        return NULL;
    }
    json_error_t error;
    json_t* root = json_loads(input, 0, &error);
    if (!root) {
        LOGE("JSON parsing error: %s\n", error.text);
        return NULL;
    }
    json_t *s_root = json_object();
    json_t* value = json_object_get(root, "cmd");
    if (json_is_string(value)) {  
        char *raw = cmf.requestGetDeviceAtRaw(json_string_value(value));
        json_t *arr = fmt_raw2json(raw);
        json_object_set_new(s_root, "cmd", value);
        json_object_set_new(s_root, "response", arr);
    }
    return s_root;
}

void proc_get_request(app_sched_t* work) {
    url_handler_t handlers[] = {
        {"netisp", (void* (*)())cmf.requestGetDeviceNetISP, (json_t* (*)(void *))pack_isp_json},
        {"sysinfo", (void* (*)())cmf.requestGetDeviceSysInfo, (json_t* (*)(void *))pack_sysinfo_json},
        {"signal", (void* (*)())cmf.requestGetDeviceNetSignal, (json_t* (*)(void *))pack_signal_json},
        {"cellinfo", (void* (*)())cmf.requestGetDeviceNetCellInfo, (json_t* (*)(void *))pack_cellinfo_json},
        {"ethernet", (void* (*)())cmf.requestGetDeviceEtherNetMode, NULL},
        {"gmi", (void* (*)())cmf.requestGetDeviceGMI, NULL},
        {"gmm", (void* (*)())cmf.requestGetDeviceGMM, NULL},
        {"gmr", (void* (*)())cmf.requestGetDeviceGMR, NULL},
        {"gsn", (void* (*)())cmf.requestGetDeviceGSN, NULL},
        {"temperature", (void* (*)())cmf.requestGetDeviceTemperature, NULL},
        {"simstatus", (void* (*)())cmf.requestGetDeviceSIMStatus, NULL},
        {"simslot", (void* (*)())cmf.requestGetDeviceSIMSlot, NULL},
        {"iccid", (void* (*)())cmf.requestGetDeviceSIMICCID, NULL},
        {"imsi", (void* (*)())cmf.requestGetDeviceSIMIMSI, NULL},
        {"simnum", (void* (*)())cmf.requestGetDeviceSIMNUM, NULL},
        {"supportband", (void* (*)())cmf.requestGetDeviceSupportBandList, (json_t* (*)(void *))pack_band_json},
        {NULL, NULL, NULL}
    };

    char *url_prefix;
    json_t *s_json = json_object();
    at_tok_by_index_scanf(work->url_path, "/", -1, "%s", &url_prefix);
    if ((strncasecmp(work->url_path, "/api/get/", 8) == 0) && (url_prefix != NULL)) {
        int found = 0;
        for (int i = 0; handlers[i].url != NULL; i++) {
            if (strcasecmp(handlers[i].url, url_prefix) == 0) {
                void *data = handlers[i].request_handler();
                if (handlers[i].pack_handler) {
                    s_json = handlers[i].pack_handler(data);
                } else {
                    if (strcasecmp(url_prefix, "ethernet") == 0) {
                        json_object_set_new(s_json, url_prefix, json_boolean((data != NULL)));
                    } else {
                        json_object_set_new(s_json, url_prefix, json_string(data ? (char *)data : "invalid url"));
                    }
                }
                found = 1;
                break;
            }
        }

        if (!found) {
            work->response_data = strdup(URL_ERR_JSON);
        } else {
            work->response_data = json_dumps(s_json, JSON_INDENT(0));
            if (s_json) json_decref(s_json);
        }
    } else {
        work->response_data = strdup(URL_ERR_JSON);
    }
    work->response_length = strlen(work->response_data);
}

#if 0
void proc_post_request(app_sched_t* work) {
    
    url_handler_t post_handlers[] = {
        {"atraw", (void* (*)(void *))cmf.requestGetDeviceAtRaw, (json_t* (*)(void *))pack_atraw_json},
        {NULL, NULL, NULL}
    };

    char *url_prefix;
    at_tok_by_index_scanf(work->url_path, "/", -1, "%s", &url_prefix);
    if ((strncasecmp(work->url_path, "/api/set/", 8) == 0) && (url_prefix != NULL)) {
        json_t *s_json = json_object();
        int found = 0;
        json_error_t error;
        json_t* root = json_loads(work->data, 0, &error);
        json_t* cmd = json_object_get(root, "cmd");
        json_t* s_tmp = json_object();
        if (json_is_string(cmd)) {  
            for (int i = 0; post_handlers[i].url != NULL; i++) {
                if (strcasecmp(post_handlers[i].url, url_prefix) == 0) {
                    void *data = post_handlers[i].request_handler((void *)json_string_value(cmd));
                    if (post_handlers[i].pack_handler) {
                        s_tmp = post_handlers[i].pack_handler(data);
                    }else {
                        s_tmp = json_string(data ? (char *)data : "invalid data");
                    }
                    found = 1;
                    break;
                }
            }
        }
        json_object_set_new(s_json, "cmd", cmd);
        if (!found) {
            work->response_data = strdup(URL_ERR_JSON);
        } else {
            json_object_set_new(s_json, "response", s_tmp);
            work->response_data = json_dumps(s_json, JSON_INDENT(0));
            if (s_json) json_decref(s_json);
            if (s_tmp) json_decref(s_tmp);
        }
    } else {
        work->response_data = strdup(URL_ERR_JSON);
    }
    work->response_length = strlen(work->response_data);
}
#endif
void proc_post_request(app_sched_t* work) {
    
    url_handler_t post_handlers[] = {
        {"atraw", (void* (*)(void))cmf.requestGetDeviceAtRaw, (json_t* (*)(void *))pack_atraw_json},
        {NULL, NULL, NULL}
    };

    char *url_prefix;
    at_tok_by_index_scanf(work->url_path, "/", -1, "%s", &url_prefix);
    if ((strncmp(work->url_path, "/api/set/", 8) == 0) && (url_prefix != NULL)) {
        json_t *s_json;
        int found = 0;
        for (int i = 0; post_handlers[i].url != NULL; i++) {
            if (strcmp(post_handlers[i].url, url_prefix) == 0) {
                // void *data = post_handlers[i].request_handler();
                s_json = post_handlers[i].pack_handler(work->data);
                found = 1;
                break;
            }
        }

        if (!found) {
            work->response_data = strdup("{\"err\": \"invalid url\"}");
        } else {
            work->response_data = json_dumps(s_json, JSON_INDENT(0));
            if (s_json) json_decref(s_json);
        }
    } else {
        work->response_data = strdup("{\"err\": \"invalid url\"}");
    }
    work->response_length = strlen(work->response_data);
}


int prco_request_init(char *dev){
    int ret = at_open(dev, NULL, 0);
    if(ret < 0) return ret;
    if(at_handshake() != 0 ) {
        at_close();
        return -1;
    }
    cmf = createAtcRequestOps();
    cmf.atcRequestOpsInit();
    return 0;
}