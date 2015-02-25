#ifndef MACSTREAM_H
#define MACSTREAM_H

#include <istream>
#include <ostream>
#include <vector>

#include "macLib.h"

/* A readonly stream buffer which is backed by an istream.
 * The streambuf read the istream line-by-line and passes
 * lines through macExpandString()
 */
class macro_streambuf : public std::streambuf
{
    std::istream *strm;
    MAC_HANDLE* macctxt;
    std::vector<char> linebuf, outbuf;
public:
    macro_streambuf();
    macro_streambuf(std::istream& backing, MAC_HANDLE* ctxt=0);
    ~macro_streambuf();

    void setBackingStream(std::istream& backing);
    void setMacContext(MAC_HANDLE& ctxt);

    virtual int_type underflow();
};

#endif // MACSTREAM_H
