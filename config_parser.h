#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

#include <stddef.h>
#include <json-c/json.h>
#include "utils.h"
#include "meteo.h"

#define DEFAULT_MAX_LINE 1024

struct Config {
    char *city;
    char *text;
    char *tooltip;
    json_object *weather;
};

char *braces_parser(const char *format, const struct Config *config, const struct Meteo *meteo);
void config_free(struct Config *conf);
int config_parse(struct Config *conf, const char *path);

#endif /* __CONFIG_PARSER_H__ */
