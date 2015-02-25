
#include "epicsAssert.h"

#include "macstream.h"

#define NPUTBACK 4

macro_streambuf::macro_streambuf()
    :strm(0)
    ,macctxt(0)
    ,linebuf(128)
    ,outbuf(128)
{
    setg(&outbuf[0],&outbuf[0],&outbuf[0]);
}

macro_streambuf::macro_streambuf(std::istream &backing, MAC_HANDLE* ctxt)
    :strm(&backing)
    ,macctxt(ctxt)
    ,linebuf(128)
    ,outbuf(128)
{
    setg(&outbuf[0],&outbuf[0],&outbuf[0]);
}

void
macro_streambuf::setBackingStream(std::istream &backing)
{
    strm = &backing;
    setg(&outbuf[0],&outbuf[0],&outbuf[0]);
}

void
macro_streambuf::setMacContext(MAC_HANDLE& c)
{
    ctxt = c;
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

    linebuf.resize(128);

    while(strm.good()) {
        assert(i<buf.size());
        strm.getline(&linebuf[i], linebuf.size()-i);
        i += strm.gcount();

        // if only failbit set
        if(strm.fail() && !strm.bad() && !strm.eof()) {
            // buffer too short, retry
            linebuf.resize(buf.size()+128);
            strm.clear();
            continue;
        }
        /* extra count for nil, normal EOF gives a count for the terminator
         * which isn't stored.
         */
        if(strm.eof())
            i++;
        break;
    }
    buf.resize(i);// buf size includes trailing nil

    if(!macctxt) {
        /* no macro expansion */
        setg(&buf[0], &buf[NPUTBACK], &buf[buf.size()]);
        return traits_type::to_int_type(buf[NPUTBACK]);
    }

    outbuf.resize(buf.size()+128);

    long ret=0;
    while(true){
        ret = macExpandString(macctxt, &buf[0], &outbuf[0], outbuf.size());
        if(ret<(outbuf.size()-1)) {
            outbuf.resize(ret); // exclude nil
            break;
        }
        outbuf.resize(buf.size()+128);
    }

    setg(&outbuf[0], &outbuf[NPUTBACK], &outbuf[outbuf.size()]);
    return traits_type::to_int_type(outbuf[NPUTBACK]);
}
