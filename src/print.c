static bool print(const char *fmt, ...) {
    unsigned count = 0;
    char ascii[BUF_MAX];
    va_list va;

    va_start(va, fmt);
    vsnprintf(ascii, BUF_MAX, fmt, va);
    va_end(va);

    tud_cdc_write(ascii, strlen(ascii));
    tud_cdc_write_flush();
}
