#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>

typedef struct {
    int idx; 
    const char *band; 
    const char *info;
} band_t;

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

json_t* get_bands_by_index(int band_indices[], int count) {
    json_t* arr = json_array();
    for (int i = 0; i < count; ++i) {
        int idx = band_indices[i];
        if (idx >= 0 && idx < BAND_COUNT) {
            char str[100];
            snprintf(str, sizeof(str), "%s/%s", bands[idx].band, bands[idx].info);
            json_array_append_new(arr, json_string(str));
        }
    }
    return arr;
}





int main() {

    int indices[] = {N1, N28, N77, N102};
    // int count = sizeof(indices) / sizeof(indices[0]);

    json_t* supported_bands = GET_SUPPORTED_BANDS(indices);
    char* json_str = json_dumps(supported_bands, JSON_INDENT(2));
    printf("Supported bands:\n%s\n", json_str);

    free(json_str);
    json_decref(supported_bands);

    return 0;
}
