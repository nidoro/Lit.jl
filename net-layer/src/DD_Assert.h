
#ifndef DD_ASSERT_H
#define DD_ASSERT_H

#undef DD_Assert
#undef DD_Assert1
#undef DD_Assert2

#define DD_Assert(expression) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d)\n", __FILE__, __LINE__);\
        *((int*) 0) = 0;\
    }

#define DD_Assert1(expression, msg) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d): ", __FILE__, __LINE__);\
        fprintf(stderr, msg);\
        fprintf(stderr, "\n");\
        *((int*) 0) = 0;\
    }

#define DD_Assert2(expression, format, ...) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d): ", __FILE__, __LINE__);\
        fprintf(stderr, format, __VA_ARGS__);\
        fprintf(stderr, "\n");\
        *((int*) 0) = 0;\
    }

#endif // DD_ASSERT_H

