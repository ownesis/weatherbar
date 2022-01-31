#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipinfo.h>
#include <waybar_json.h>
#include "utils.h"
#include "meteo.h"
#include "config_parser.h"

char *get_ip_city(void) {
    struct IPInfo *info = NULL;
    char *city = NULL;
    int res;
    char *ipinfo_err[6] = {
        [IPINFO_MEM_ERR]        = "An error occurred when allocate memory.\n",
        [IPINFO_CURL_ERR]       = "An error occurred when call CURL routines.\n",
        [IPINFO_JSON_ERR]       = "An error occured when parsing JSON of ipapi.co\n.",
        [IPINFO_ENDPOINT_ERR]   = "An error occured, the API endpoint 'apiapi.com' return a reponse code different to 200.\n",
        [IPINFO_IP_ERR]         = "An error occured, invalid IP address.\n",
    };

    res = ipinfo_get(&info, NULL);

    if (res != IPINFO_OK)
        PANIC(ipinfo_err[res]);

    city = strdup(info->city);

    ipinfo_free(info);

    return city;
}

int main(int argc, char *argv[]) {
    struct Waybar_args waybar_args = {NULL};
    struct Meteo meteo = {NULL};
    struct Config conf = {0};
    char *city = NULL;
    char *formated_text = NULL;
    char *formated_tooltip = NULL;
    char *json = NULL;

    if (argc < 2)
        PANIC("Usage: %s <config_file>\n", argv[0]);

    config_parse(&conf, argv[1]);

    /* if city is not set */
    if (strlen(conf.city) == 0)
        city = get_ip_city();
    else 
        city = conf.city;

    meteo_get(&meteo, city);

    formated_text = format_string(conf.text, &conf, &meteo);
    formated_tooltip = format_string(conf.tooltip, &conf, &meteo);

    waybar_args.text = formated_text;
    waybar_args.tooltip = formated_tooltip;
    waybar_args.class = NULL;
    waybar_args.class_len = 0;
    waybar_args.percentage = 0;

    json = waybar_json(&waybar_args);
    
    puts(json);

    free(formated_text);
    free(formated_tooltip);
    free(json);
    meteo_free(&meteo);
    config_free(&conf);

    return 0;
}
