#include "utils.h"

#include <stdio.h>

double min(double a, double b) {
    if (a > b) return b;
    return a;
}

void fmt_bytes(size_t size, char *buf, size_t n) {
    static const char *names[] = {"B", "KB", "MB", "GB", "TB"};
    int order = 0;
    while (size >= 1024) {
        order++;
        size /= 1024;
    }
    const char *name = order < 5 ? names[order] : "?";
    snprintf(buf, n, "%ld%s", size, name);
}

void clear_lines(size_t count) {
    for (size_t i = 0; i < count; i++) {
        printf("\033[A\033[2K");
    }
}
