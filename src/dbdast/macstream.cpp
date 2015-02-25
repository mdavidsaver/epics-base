
#include "epicsAssert.h"

#include "macstream.h"

#define NPUTBACK 4
#define BASESIZE 128

macro_streambuf::macro_streambuf()
    :strm(0)
    ,macctxt(0)
    ,linebuf(BASESIZE)
    ,outbuf(BASESIZE)
{
    setg(&outbuf[0],&outbuf[0],&outbuf[0]);
}

macro_streambuf::macro_streambuf(std::istream &backing, MAC_HANDLE* ctxt)
    :strm(&backing)
    ,macctxt(ctxt)
    ,linebuf(BASESIZE)
    ,outbuf(BASESIZE)
{
    setg(&outbuf[0],&outbuf[0],&outbuf[0]);
}

macro_streambuf::~macro_streambuf() {}

void
macro_streambuf::setBackingStream(std::istream &backing)
{
    strm = &backing;
    setg(&outbuf[0],&outbuf[0],&outbuf[0]);
}

void
macro_streambuf::setMacContext(MAC_HANDLE* c)
{
    macctxt = c;
    // don't clear the buffer
}

macro_streambuf::int_type
macro_streambuf::underflow()
{
    char_type *cur=gptr(), *back=egptr();
    if(cur<back)
        return traits_type::to_int_type(*cur);

    else if(!strm->good())
        return traits_type::eof();

    // output buf is empty
    // need to read in one line
    size_t i=NPUTBACK;

    linebuf.resize(BASESIZE);

    while(strm->good()) {
        assert(i<linebuf.size()-1);
        strm->getline(&linebuf[i], linebuf.size()-i-1);
        i += strm->gcount();

        // if only failbit set
        if(strm->fail() && !strm->bad() && !strm->eof()) {
            // buffer too short, retry
            linebuf.resize(linebuf.size()+BASESIZE);
            strm->clear();
            continue;
        } else if(strm->good())
            linebuf[i-1] = '\n'; // include the delimiter in the output

        break;
    }
    linebuf.resize(i+1);
    linebuf[i]='\0';

    if(!macctxt) {
        /* no macro expansion */
        setg(&linebuf[0], &linebuf[NPUTBACK], &linebuf[linebuf.size()-1]); // exclude nil
        return traits_type::to_int_type(linebuf[NPUTBACK]);
    }

    /* assume output will be approx. the same size as input */
    outbuf.resize(linebuf.size()+16);

    long ret=0;
    while(true){
        size_t capacity = outbuf.size()-NPUTBACK;
        ret = macExpandString(macctxt, &linebuf[NPUTBACK], &outbuf[NPUTBACK], capacity);
        if((unsigned long)ret<(capacity-1)) {
            outbuf.resize(ret+NPUTBACK); // exclude nil
            break;
        }
        outbuf.resize(outbuf.size()+BASESIZE);
    }

    setg(&outbuf[0], &outbuf[NPUTBACK], &outbuf[outbuf.size()]);
    return traits_type::to_int_type(outbuf[NPUTBACK]);
}
