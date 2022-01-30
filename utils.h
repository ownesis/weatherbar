#ifndef __UTILS_H__
#define __UTILS_H__

#define PANIC(f, ...) \
    do {                                    \
        fprintf(stderr, f, ## __VA_ARGS__); \
        exit(EXIT_FAILURE);                 \
    } while (0)

#define ARRAY_LEN(a) (sizeof((a))/(sizeof((*(a)))))

#define FREE_IF_NONNULL(p) if ((p)) free(p)

char *double_to_str(double d, char *buf, size_t buf_size);
char *int_to_str(int i, char *buf, size_t buf_size);

#endif /* __UTILS_H__ */
