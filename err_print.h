#ifndef ERR_PRINT
#define ERR_PRINT(x) fprintf(stderr, "%s in %s, %s: %d\n", x, __func__,__FILE__,__LINE__)
#endif

