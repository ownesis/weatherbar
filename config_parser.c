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
#include "utils.h"
#include "meteo.h"
#include "config_parser.h"

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

struct Dict {
    char *key;
    char *value;
};

static _Bool startswith(const char *str, const char *start) {
    return strncmp(start, str, strlen(start)) == 0;
}

static char *fmt_keyvalue(const char *key, const struct Config *config, const struct Meteo *meteo) {
    struct Dict fmt_array[] = {
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
    };

    if (startswith(key, "weather.")) {
        char *weather_key = NULL;
        json_object *tmp[2] = {NULL};
        
        if (!json_object_object_get_ex(config->weather, meteo->condition_key, &tmp[0])) {
            PANIC("Error no key '%s' in weather object.\n", 
                    meteo->condition_key);
        }

        if (json_object_get_type(tmp[0]) != json_type_object) {
            PANIC("Error the key weather.%s is not of type object/dict.\n",
                    meteo->condition_key);
        }

        weather_key = strchr(key, '.')+1;

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

    for (size_t i = 0; i < ARRAY_LEN(fmt_array); i++) {
        if (strcmp(fmt_array[i].key, key) == 0)
            return fmt_array[i].value;
    }

    return NULL;
}

char *braces_parser(const char *format, const struct Config *config, const struct Meteo *meteo) {
    char *value = NULL;
    char *buf = calloc(1, DEFAULT_MAX_LINE+1);
    char key[DEFAULT_MAX_LINE+1] = {0};
    size_t key_index = 0;
    size_t buf_index = 0;
    size_t format_len = 0;
    int i = 0;

    if (!buf || !format)
        return NULL;

    format_len = strlen(format);

    for (; i < format_len && buf_index < DEFAULT_MAX_LINE; i++, buf_index++) {
        memset(key, 0, DEFAULT_MAX_LINE);
        key_index = 0;

        if (format[i] == '{') {
            /* if not escaped brace */
            if (i > 0 && format[i-1] == '\\') {
                /* erase backslash with brace */
                buf_index--;
            } else {
                i++;
                while (format[i] != '}') {
                    if (key_index == DEFAULT_MAX_LINE)
                        return buf;
                    key[key_index++] = format[i++];
                }
                i++;

                value = fmt_keyvalue(key, config, meteo);

                if (value) {
                    strncat(buf, value, (DEFAULT_MAX_LINE - buf_index));
                    buf_index += strlen(value);
                } else {
                    strncat(buf, "", (DEFAULT_MAX_LINE - buf_index));
                    buf_index++;
                }
            } 
        } 
        
        if (format[i] == '}' && format[i-1] == '\\') {
            /* replace backslash with brace */
            buf_index--;
        }

        buf[buf_index] = format[i];
    }

    return buf;
}

static size_t get_file_size(int fd) {
    struct stat stat = {0};
    
    fstat(fd, &stat);

    return stat.st_size;
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
