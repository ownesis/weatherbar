/* https://www.prevision-meteo.ch/uploads/pdf/recuperation-donnees-meteo.pdf */
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define array_size(a) (sizeof((a))/(sizeof((*(a)))))

#define METEO_URL "https://www.prevision-meteo.ch/services/json/"

#define CAUTION "⚠️"
#define LIGHT "⚡"
#define SUN "🌣"
#define RAIN "🌧"
#define CLOUD "☁"
#define TORNADO "🌪"
#define FOG "🌫"
#define STARS "✨"
#define SNOW "❄️"
#define WIND "🌬"
#define UMBRELLA "☂"
#define CLOUD_SUN1 "🌤"
#define CLOUD_SUN2 "🌥"
#define CLOUD_SUN3 "⛅"
#define CLOUD_SUN_RAIN "🌦"
#define CLOUD_SNOW "🌨"
#define CLOUD_LIGHT_RAIN "⛈"
#define CLOUD_LIGHT "🌩"
 
struct Weather_conditions {
    char *cond;
    char *icon;
};

struct Mem {
    char *ptr;
    size_t size;
};

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
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

char *get_city(CURL *curl_handle) {
    CURLcode res;
    struct Mem chunk;
    struct json_object *jobj;
    struct json_object *loc_obj;
    _Bool test;
    char *city = NULL;

    chunk.ptr = malloc(1);
    chunk.size = 0;

    res = curl_easy_setopt(curl_handle, CURLOPT_URL, "https://wtfismyip.com/json");
    res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    chunk.ptr[chunk.size] = 0;

    jobj = json_tokener_parse(chunk.ptr);
    test = json_object_object_get_ex(jobj, "YourFuckingLocation", &loc_obj);

    free(chunk.ptr);

    city = strdup(json_object_get_string(loc_obj));

    json_object_put(jobj);
        
    return city;
}

int main(void) {
    CURL *curl_handle;
    CURLcode res;
    struct Mem chunk;
    struct json_object *jobj;
    struct json_object *current_jobj;
    struct json_object *temp_jobj;
    struct json_object *key_jobj;
    _Bool test;

    struct Weather_conditions weather[] = {
        {"ensoleille", SUN},
        {"nuit-claire", STARS},
        {"ciel-voile", CLOUD_SUN1},
        {"nuit-legerement-voilee", STARS CLOUD_SUN1},
        {"faibles-passages-nuageux", CLOUD},
        {"nuit-bien-degagee", STARS},
        {"brouillard", FOG},
        {"stratus", CLOUD},
        {"stratus-se-dissipant", CLOUD},
        {"nuit-claire-et-stratus", STARS CLOUD},
        {"eclaircies", CLOUD_SUN1},
        {"nuit-nuageuse", STARS CLOUD},
        {"faiblement-nuageux", CLOUD},
        {"fortement-nuageux", CLOUD CAUTION},
        {"averses-de-pluie-faible", UMBRELLA},
        {"nuit-avec-averses", STARS RAIN},
        {"averses-de-pluie-moderee", RAIN},
        {"averses-de-pluie-forte", RAIN CAUTION},
        {"couvert-avec-averses", CLOUD_SUN_RAIN},
        {"pluie-faible", UMBRELLA},
        {"pluie-forte", RAIN},
        {"pluie-moderee", RAIN},
        {"developpement-nuageux", CLOUD},
        {"nuit-avec-developpement-nuageux", STARS CLOUD},
        {"faiblement-orageux", LIGHT},
        {"nuit-faiblement-orageuse", STARS LIGHT},
        {"orage-modere", CLOUD_LIGHT},
        {"fortement-orageux", CLOUD_LIGHT CAUTION},
        {"averses-de-neige-faible", SNOW},
        {"nuit-avec-averses-de-neige-faible", STARS SNOW},
        {"neige-faible", SNOW},
        {"neige-moderee", CLOUD_SNOW},
        {"neige-forte", CLOUD_SNOW CAUTION},
        {"pluie-et-neige-melee-faible", RAIN SNOW},
        {"pluie-et-neige-melee-modeereee", RAIN CLOUD_SNOW},
        {"pluie-et-neige-melee-forte", RAIN CLOUD CAUTION},
    };

    size_t meteo_url_len = strlen(METEO_URL);

    char *buff = malloc(meteo_url_len+255);
    if (!buff)
        return -1;

    memset(buff, 0, meteo_url_len+255);

    curl_handle = curl_easy_init();

    res = curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
 
    char *loc = get_city(curl_handle);
    char *end_city = strchr(loc, ',');
    loc[end_city - loc] = '\0';

    snprintf(buff, meteo_url_len+255, "%s%s", METEO_URL, loc);

    chunk.ptr = malloc(1);
    chunk.size = 0;

    res = curl_easy_setopt(curl_handle, CURLOPT_URL, buff);
    res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    curl_easy_cleanup(curl_handle);
    chunk.ptr[chunk.size] = 0;

    jobj = json_tokener_parse(chunk.ptr);
    test = json_object_object_get_ex(jobj, "current_condition", &current_jobj);
    
    if (!test)
        return -1;

    test = json_object_object_get_ex(current_jobj, "tmp", &temp_jobj);
    test = json_object_object_get_ex(current_jobj, "condition_key", &key_jobj);
    
    int temp =  json_object_get_int(temp_jobj);
    const char *key = json_object_get_string(key_jobj);
    const char *icon = NULL;

    for (int i = 0; i < array_size(weather); i++) {
        if (strstr(key, weather[i].cond)) {
            icon = weather[i].icon;
            break;
        }
    }

    if (!icon) icon = "";

    printf("%d°C %s\n", temp, icon);

    json_object_put(jobj);
    free(chunk.ptr);
    free(buff);
    free(loc);

    return 0;
}
