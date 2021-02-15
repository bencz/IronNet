#include "stringHelper.h"

#include <string.h>
#include <stdlib.h>

#if __OS400__
#include <iconv.h>
#include <QTQICONV.h>

char* ascii2ebcdic_n(char* src)
{
    iconv_t conv;
    char* out, * out_orig;
    int strSize;

    QtqCode_T ebcdic, utf;
    memset(&ebcdic, 0, sizeof(ebcdic));
    ebcdic.shift_alternative = 1; // so the iconv object doesnrequire resetting
    memset(&utf, 0, sizeof(utf));
    utf.shift_alternative = 1;
    ebcdic.CCSID = 37;
    utf.CCSID = 437;
    conv = QtqIconvOpen(&ebcdic, &utf); // to, from
    if (conv.return_value == -1) {
        printf("iconv open");
        exit(-1);
    }

    strSize = strlen(src) + 1;
    out = (char*)malloc(strSize);
    out_orig = out;
    memset(out, 0, strSize);

    size_t inleft = strSize, outleft = strSize;
    size_t ret = iconv(conv, &src, &inleft, &out, &outleft);
    out = out_orig;
    if (ret == -1) {
        printf("iconv");
        exit(-2);
    }
    iconv_close(conv);

    return out;
}
#else
char* ascii2ebcdic_n(char* src)
{
    return src;
}
#endif