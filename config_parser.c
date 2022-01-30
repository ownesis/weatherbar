#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include <ssfmt.h>
#include "utils.h"
#include "meteo.h"
#include "config_parser.h"

#define MAX_BUF 2048

#define JSON_GET_STR(j, k, s)                                                       \
    do {                                                                            \
        json_object *_tmp = NULL;                                                   \
        if (!json_object_object_get_ex((j), #k, &_tmp)) {                           \
            PANIC("Error key '%s' not found in configuration file\n", #k);          \
        }                                                                           \
        if (json_object_get_type(_tmp) != json_type_string)                         \
            PANIC("Error the key '%s' is not of type String.\n", #k);               \
        (s)->k = strdup(json_object_get_string(_tmp));                              \
        if (!(s)->k)                                                                \
            PANIC("Error occured when duplicate the value of the key '%s'.\n", #k); \
    } while (0)

#define JSON_GET_OBJ(j, k, s)                                                       \
    do {                                                                            \
        json_object *_tmp = NULL;                                                   \
        if (!json_object_object_get_ex((j), #k, &_tmp)) {                           \
            PANIC("Error key '%s' not found in configuration file\n", #k);          \
        }                                                                           \
        if (json_object_get_type(_tmp) != json_type_object)                         \
            PANIC("Error the key '%s' is not of type object/dict.\n", #k);          \
            json_object_deep_copy(_tmp, &(s)->k, NULL);                             \
    } while (0)

static _Bool startswith(const char *str, const char *start) {
    return strncmp(start, str, strlen(start)) == 0;
}

static size_t get_file_size(int fd) {
    struct stat stat = {0};
    
    fstat(fd, &stat);

    return stat.st_size;
}

static char *_get_weather_subkey(const char *weather_key, const struct Config *config, const struct Meteo *meteo) {
    json_object *tmp[2] = {NULL};
    
    if (!json_object_object_get_ex(config->weather, meteo->condition_key, &tmp[0])) {
        PANIC("Error no key '%s' in weather object.\n", 
                meteo->condition_key);
    }

    if (json_object_get_type(tmp[0]) != json_type_object) {
        PANIC("Error the key weather.%s is not of type object/dict.\n",
                meteo->condition_key);
    }

    if (!json_object_object_get_ex(tmp[0], weather_key, &tmp[1])) {
        PANIC("Error no key '%s' found in object weather.%s in configuration file.\n", 
                weather_key,
                meteo->condition_key);
    }

    if (json_object_get_type(tmp[1]) != json_type_string) {
        PANIC("Error the key weather.%s.%s is not of type string.\n",
                meteo->condition_key,
                weather_key);
    }
    
    return (char *)json_object_get_string(tmp[1]);
}


char *format_string(const char *format, const struct Config *conf, const struct Meteo *meteo) {
    char *buf = calloc(1, MAX_BUF);
    struct SSFMTDict array[] = {
        {"name", meteo->name},
        {"country", meteo->country},
        {"latitude", meteo->latitude},   
        {"longitude", meteo->longitude},   
        {"elevation", meteo->elevation},   
        {"sunrise", meteo->sunrise},   
        {"sunset", meteo->sunset},   
        {"date", meteo->date},   
        {"hour", meteo->hour},   
        {"wind_dir", meteo->wnd_dir},   
        {"condition", meteo->condition},   
        {"condition_key", meteo->condition_key},
        {"temp", meteo->tmp},
        {"wind_speed", meteo->wnd_spd},
        {"wind_gust", meteo->wnd_gust},
        {"humidity", meteo->humidity},
        {"pressure", meteo->pressure},
        {"weather.icon", _get_weather_subkey("icon", conf, meteo)},
        {"weather.text", _get_weather_subkey("text", conf, meteo)},
    };

    ssfmt_ctx_t ctx = SSFMT_INIT(array, MAX_BUF, 0);

    if (!buf)
        PANIC("Error occurred when allocate buffer.\n");  
    
    return ssfmt_parser(&ctx, format, buf, MAX_BUF);
}

void config_free(struct Config *conf) {
    FREE_IF_NONNULL(conf->city);
    FREE_IF_NONNULL(conf->text);
    FREE_IF_NONNULL(conf->tooltip);

    json_object_put(conf->weather);
}

int config_parse(struct Config *conf, const char *path) {
    int fd;
    char *buf = NULL;
    size_t file_size = 0;
    json_object *jobj = NULL;

    if (!path)
        return 0;
    
    fd = open(path, O_RDONLY);

    if (fd == -1) {
        PANIC("Can't parse config file %s: %s\n", 
                path, 
                strerror(errno));
    }

    file_size = get_file_size(fd);

    buf = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    
    if (!buf) {
        PANIC("Error occurred when project configuration file in memory: %s\n", 
                strerror(errno));
    }

    close(fd);

    jobj = json_tokener_parse(buf);

    if (!jobj) {
        PANIC("Error when parsing configuration file: %s\n", 
                path);
    }

    munmap(buf, file_size);

    JSON_GET_STR(jobj, city, conf);
    JSON_GET_STR(jobj, text, conf);
    JSON_GET_STR(jobj, tooltip, conf);

    JSON_GET_OBJ(jobj, weather, conf);

    json_object_put(jobj);

    return 1;
}
