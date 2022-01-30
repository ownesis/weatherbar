#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "meteo.h"
#include "utils.h"

#define CURL_ERR(f) if ((f) != CURLE_OK)

#define GET_PRESSURE(j, k, s)                                                   \
    do {                                                                        \
        char _buf[10] = {0};                                                    \
        json_object *_tmp = NULL;                                               \
        if (!json_object_object_get_ex((j), #k, &_tmp)) {                       \
            PANIC("Error key '%s' not found in body\n", #k);                    \
        }                                                                       \
        if (json_object_get_type(_tmp) == json_type_int) {                      \
            if (!int_to_str(json_object_get_int(_tmp), _buf, 10))               \
                PANIC("Error occured when convert %s to string\n", "int");      \
        } else if (json_object_get_type(_tmp) == json_type_double) {            \
             if (!double_to_str(json_object_get_double(_tmp), _buf, 10))        \
                PANIC("Error occured when convert %s to string\n", "double");   \
        }                                                                       \
        (s)->k = strdup(_buf);                                                  \
    } while (0)

#define JSON_GET_TYPE(t, j, k, s)                                           \
    do {                                                                    \
        char _buf[10] = {0};                                                \
        json_object *_tmp = NULL;                                           \
        if (!json_object_object_get_ex((j), #k, &_tmp)) {                   \
            PANIC("Error key '%s' not found in body\n", #k);                \
        }                                                                   \
        if (json_object_get_type(_tmp) != json_type_##t)                    \
            PANIC("Error the key '%s' is not of type %s.\n", #k, #t);       \
            if (!t##_to_str(json_object_get_##t(_tmp), _buf, 10))           \
                PANIC("Error occured when convert %s to string\n", #t);     \
            (s)->k = strdup(_buf);                                          \
    } while (0)

#define JSON_GET_STR(j, k, s)                                                       \
    do {                                                                            \
        json_object *_tmp = NULL;                                                   \
        if (!json_object_object_get_ex((j), #k, &_tmp)) {                           \
            PANIC("Error key '%s' not found in body\n", #k);                        \
        }                                                                           \
        if (json_object_get_type(_tmp) != json_type_string)                         \
            PANIC("Error the key '%s' is not of type string.\n", #k);               \
        (s)->k = strdup(json_object_get_string(_tmp));                              \
        if (!(s)->k)                                                                \
            PANIC("Error occured when duplicate the value of the key '%s'.\n", #k); \
    } while (0)

struct Mem {
    char *ptr;
    size_t size;
};

static size_t _write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = (size * nmemb);
    struct Mem *chunk = (struct Mem *)userdata;
    char *tmp = realloc(chunk->ptr, chunk->size+total_size+1);

    if (!tmp)
        return 0;

    chunk->ptr = tmp;
    memcpy(chunk->ptr+chunk->size, ptr, total_size);
    chunk->size += total_size;

    return total_size;
}

void meteo_free(struct Meteo *meteo) {
    if (!meteo)
        return;

    FREE_IF_NONNULL(meteo->name);
    FREE_IF_NONNULL(meteo->country);
    FREE_IF_NONNULL(meteo->latitude);
    FREE_IF_NONNULL(meteo->longitude);
    FREE_IF_NONNULL(meteo->elevation);
    FREE_IF_NONNULL(meteo->sunrise);
    FREE_IF_NONNULL(meteo->sunset);
    FREE_IF_NONNULL(meteo->date);
    FREE_IF_NONNULL(meteo->hour);
    FREE_IF_NONNULL(meteo->wnd_dir);
    FREE_IF_NONNULL(meteo->condition);
    FREE_IF_NONNULL(meteo->condition_key);
    FREE_IF_NONNULL(meteo->tmp);
    FREE_IF_NONNULL(meteo->wnd_spd);
    FREE_IF_NONNULL(meteo->wnd_gust);
    FREE_IF_NONNULL(meteo->pressure);
    FREE_IF_NONNULL(meteo->humidity);
}

int meteo_get(struct Meteo *meteo, const char *city) {
    CURL *curl_handle;
    struct json_object *jobj;
    struct json_object *temp_jobj;
    struct Mem chunk = {NULL};
    size_t city_len = 0;
    char buf[10] = {0};
    char *url = NULL; 
    long code = 0;

    chunk.ptr = calloc(1, 1);

    if (!chunk.ptr)
        PANIC("Error occurred when alloc chunk\n");

    if (!city)
        PANIC("Error no city set.");
    
    city_len = strlen(city);

    url = calloc(1, (sizeof(METEO_URL) 
                + city_len
                + 1));

    if (!url)
        PANIC("Error occurred when allocate memory: %s\n", strerror(errno));

    if (!strcat(url, METEO_URL))
        PANIC("Error occurred when concatenate url\n");

    if (!strcat(url, city))
        PANIC("Error occurred when concatenate url\n");

    curl_handle = curl_easy_init();

    CURL_ERR(curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L)) {
        PANIC("Error occurred when set option to curl: %s: %ld\n", 
                "CURLOPT_FOLLOWLOCATION",
                1L);
    }
    
    CURL_ERR(curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _write_callback)) {
        PANIC("Error occurred when set option to curl: %s: %p\n",
               "CURLOPT_WRITEFUNCTION",
               _write_callback);    
    }

    CURL_ERR(curl_easy_setopt(curl_handle, CURLOPT_URL, url)) {
        PANIC("Error occurred when set option to curl: %s: %s\n",
                "CURLOPT_URL",
                url);
    }
    
    CURL_ERR(curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk)) {
        PANIC("Error occurred when set option to curl: %s %p\n",
                "CURLOPT_WRITEDATA",
                &chunk);
    }
 
    CURL_ERR(curl_easy_perform(curl_handle)) {
        PANIC("Error occurred when perform request");
    }

    CURL_ERR(curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &code)) {
        PANIC("Error occurred when get info of request performed: %s %p\n",
                "CURLINFO_RESPONSE_CODE",
                &code);
    }

    if (code != 200) {
        PANIC("Error the response code of %s: %ld\n",
                url,
                code);
    }

    curl_easy_cleanup(curl_handle);

    chunk.ptr[chunk.size] = 0;

    jobj = json_tokener_parse(chunk.ptr);

    if (!jobj)
        PANIC("Error occurred when parsing json body\n");

    free(chunk.ptr);

    if (!json_object_object_get_ex(jobj, "city_info", &temp_jobj))
        PANIC("Error the key '%s' doesn't exist\n", "city_info");

    JSON_GET_STR(temp_jobj, name, meteo);
    JSON_GET_STR(temp_jobj, country, meteo);
    JSON_GET_STR(temp_jobj, latitude, meteo);
    JSON_GET_STR(temp_jobj, longitude, meteo);
    JSON_GET_STR(temp_jobj, elevation, meteo);
    JSON_GET_STR(temp_jobj, sunrise, meteo);
    JSON_GET_STR(temp_jobj, sunset, meteo);

    if (!json_object_object_get_ex(jobj, "current_condition", &temp_jobj))
        PANIC("Error the key '%s' doesn't exist\n", "current_condition");

    JSON_GET_STR(temp_jobj, date, meteo);
    JSON_GET_STR(temp_jobj, hour, meteo);
    JSON_GET_STR(temp_jobj, wnd_dir, meteo);
    JSON_GET_STR(temp_jobj, condition, meteo);
    JSON_GET_STR(temp_jobj, condition_key, meteo);

    JSON_GET_TYPE(int, temp_jobj, tmp, meteo);
    JSON_GET_TYPE(int, temp_jobj, wnd_spd, meteo);
    JSON_GET_TYPE(int, temp_jobj, wnd_gust, meteo);
    JSON_GET_TYPE(int, temp_jobj, humidity, meteo);
    
    GET_PRESSURE(temp_jobj, pressure, meteo);

    free(url);
    json_object_put(jobj);

    return 1;
}
