#include "lib.h"

// a very basic uart putc for qemu virt machine
// this assumes a pl011 uart at 0x09000000
#define UART_BASE   0x09000000
#define UART_DR     ((volatile uint32_t*)(UART_BASE + 0x00))
#define UART_FR     ((volatile uint32_t*)(UART_BASE + 0x18))
#define UART_IBRD   ((volatile uint32_t*)(UART_BASE + 0x24))
#define UART_FBRD   ((volatile uint32_t*)(UART_BASE + 0x28))
#define UART_LCRH   ((volatile uint32_t*)(UART_BASE + 0x2C))
#define UART_CR     ((volatile uint32_t*)(UART_BASE + 0x30))
#define UART_IMSC   ((volatile uint32_t*)(UART_BASE + 0x38))

#define UART_FR_TXFF (1 << 5)
#define UART_CR_UARTEN (1 << 0)
#define UART_CR_TXE (1 << 8)
#define UART_CR_RXE (1 << 9)
#define UART_LCRH_FEN (1 << 4)
#define UART_LCRH_WLEN_8BIT (0x3 << 5)

void uart_init() {
    // disable uart
    *UART_CR = 0;

    // set baud rate (115200 for 24mhz clock)
    *UART_IBRD = 13; // integer portion
    *UART_FBRD = 1;  // fractional portion

    // enable fifo, 8-bit word length
    *UART_LCRH = UART_LCRH_FEN | UART_LCRH_WLEN_8BIT;

    // enable tx, rx, and uart
    *UART_CR = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
}

void uart_putc(char c) {
    while (*UART_FR & UART_FR_TXFF);
    *UART_DR = c;
}

void* memset(void* s, int c, uint64_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

void* memcpy(void* dest, const void* src, uint64_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, uint64_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strncpy(char* dest, const char* src, uint64_t n) {
    uint64_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (!*s++) {
            return 0;
        }
    }
    return (char*)s;
}

uint64_t strlen(const char* s) {
    uint64_t len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

static void itoa(long long num, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_val;

    if (num == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return;
    }

    int is_negative = 0;
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    while (num != 0) {
        tmp_val = num % base;
        *ptr++ = (tmp_val > 9) ? (char)(tmp_val - 10 + 'a') : (char)(tmp_val + '0');
        num /= base;
    }

    if (is_negative) {
        *ptr++ = '-';
    }
    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

int vsnprintf(char* str, uint64_t size, const char* format, va_list ap) {
    char* str_ptr = str;
    uint64_t written = 0;

    while (*format && written < size) {
        if (*format == '%') {
            format++;
            if (*format == 's') {
                char* s = va_arg(ap, char*);
                uint64_t s_len = strlen(s);
                if (written + s_len >= size) s_len = size - written - 1;
                memcpy(str_ptr, s, s_len);
                str_ptr += s_len;
                written += s_len;
            } else if (*format == 'd' || *format == 'i') {
                int val = va_arg(ap, int);
                char num_buf[32];
                itoa(val, num_buf, 10);
                uint64_t num_len = strlen(num_buf);
                if (written + num_len >= size) num_len = size - written - 1;
                memcpy(str_ptr, num_buf, num_len);
                str_ptr += num_len;
                written += num_len;
            } else if (*format == 'x' || *format == 'p') {
                uint64_t val = va_arg(ap, uint64_t);
                char num_buf[32];
                itoa(val, num_buf, 16);
                uint64_t num_len = strlen(num_buf);
                if (written + num_len >= size) num_len = size - written - 1;
                memcpy(str_ptr, num_buf, num_len);
                str_ptr += num_len;
                written += num_len;
            } else if (*format == 'c') {
                char c = (char)va_arg(ap, int);
                if (written + 1 < size) {
                    *str_ptr++ = c;
                    written++;
                }
            } else if (*format == '%') {
                if (written + 1 < size) {
                    *str_ptr++ = '%';
                    written++;
                }
            }
        } else {
            *str_ptr++ = *format;
            written++;
        }
        format++;
    }
    if (written < size) {
        *str_ptr = '\0';
    } else if (size > 0) {
        str[size - 1] = '\0';
    }
    return written;
}

int snprintf(char* str, uint64_t size, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}


