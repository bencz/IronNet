#ifndef IRONNET_STRINGHELPER_H
#define IRONNET_STRINGHELPER_H

#if __OS400__
char* ascii2ebcdic_n(char* src);

#   define GETSTRING(x) (const char*)ascii2ebcdic_n(x) // Call the iconv to convert from ACS-2 to EBCDIC
#else
#   define GETSTRING(x) (const char*)x
#endif

#endif //IRONNET_STRINGHELPER_H
