#include <stdio.h>
#include "utils.h"

char *double_to_str(double d, char *buf, size_t buf_size) {
    if (!buf || buf_size <= 0)
        return NULL;

    if (snprintf(buf, buf_size, "%lf", d) <= 0)
        return NULL;

    return buf;
}

char *int_to_str(int i, char *buf, size_t buf_size) {
    if (!buf || buf_size <= 0)
        return NULL;

    if (snprintf(buf, buf_size, "%d", i) <= 0)
        return NULL;

    return buf;
}
