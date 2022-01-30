#ifndef __METEO_H__
#define __METEO_H__

/* DOC https://www.prevision-meteo.ch/uploads/pdf/recuperation-donnees-meteo.pdf */
#define METEO_URL "https://www.prevision-meteo.ch/services/json/"

struct Meteo {
    char *name;
    char *country;
    char *latitude;
    char *longitude;
    char *elevation;
    char *sunrise;
    char *sunset;
    char *date;
    char *hour;
    char *wnd_dir;
    char *condition;
    char *condition_key;
    char *tmp;
    char *wnd_spd;
    char *wnd_gust;
    char *humidity;
    char *pressure;
};

int meteo_get(struct Meteo *meteo, const char *city);
void meteo_free(struct Meteo *meteo);

#endif /* __METEO_H__ */
