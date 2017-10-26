#include <stdlib.h>
#include <string.h>

#include "sam.h"

#define BUFFER_MIN 1024
#define GAP_MIN    2048

struct Buffer{
    size_t size;
    Posn gs;
    Posn ge;
    wchar_t *buf;
};

static inline size_t
gapsize(const Buffer *b)
{
    return b->ge - b->gs;
}

static inline void
movegap(Buffer *b, Posn p)
{
    if (p == b->gs)
        return;
    else if (p < b->gs){
        size_t d = b->gs - p;
        b->gs -= d;
        b->ge -= d;
        wmemmove(b->buf + b->ge, b->buf + b->gs, d);
    } else{
        size_t d = p - b->gs;
        b->gs += d;
        b->ge += d;
        wmemmove(b->buf + b->gs - d, b->buf + b->ge - d, d);
    }
}

static inline void
ensuregap(Buffer *b, size_t l)
{
    size_t ns = b->size + l + GAP_MIN;
    size_t es = b->size - b->ge;

    if (gapsize(b) >= l)
        return;

    b->buf = realloc(b->buf, ns * sizeof(wchar_t));
    wmemmove(b->buf + (ns - es), b->buf + b->ge, es);
    b->ge = ns - es;
    b->size = ns;
}

void
deletebuffer(Buffer *b, Posn p, size_t l)
{
    if (p > bufferlength(b))
        return;

    if (p + l > bufferlength(b))
        l = bufferlength(b) - p;

    movegap(b, p);
    b->ge += l;
}

inline size_t
readbuffer(const Buffer *b, Posn p, size_t l, wchar_t *s)
{
    size_t r = 0;

    if (p > bufferlength(b))
        return 0;

    if (p + l > bufferlength(b))
        l = bufferlength(b) - p;

    if (p < b->gs){
        size_t d = b->gs - p;
        size_t t = l > d? d : l;

        wmemcpy(s, b->buf + p, t);
        s += t;
        l -= t;
        r += t;

        wmemcpy(s, b->buf + b->ge, l);
        r += l;
    } else{
        p += gapsize(b);
        wmemcpy(s, b->buf + p, l);
        r = l;
    }

    return r;
}

inline wint_t
getbufferchar(const Buffer *b, Posn p)
{
    wchar_t w = 0;
    if (!readbuffer(b, p, 1, &w))
        return WEOF;

    return w;
}

void
insertbuffer(Buffer *b, Posn p, const wchar_t *s, size_t l)
{
    if (l == 0)
        l = wcslen(s);

    if (p > bufferlength(b))
        return;

    ensuregap(b, l);
    movegap(b, p);
    wmemcpy(b->buf + b->gs, s, l);
    b->gs += l;
}

inline size_t
bufferlength(const Buffer *b)
{
    return b->size - gapsize(b);
}

Buffer *
openbuffer(void)
{
    Buffer *b = calloc(1, sizeof(Buffer));
    b->buf = calloc(1, BUFFER_MIN * sizeof(wchar_t));

    b->size = BUFFER_MIN;
    b->gs = 0;
    b->ge = BUFFER_MIN;

    return b;
}

void
closebuffer(Buffer *b)
{
    free(b->buf);
    free(b);
}
