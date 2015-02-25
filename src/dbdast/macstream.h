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
    void setMacContext(MAC_HANDLE *ctxt);

    virtual int_type underflow();
};

/** An I/O stream backed by another std::istream.
 * The backing stream is read line by line.
 * Lines are expanded with macExpandString()
 */
class imacrostream : public std::istream
{
    macro_streambuf macbuf;
public:
    explicit imacrostream()
        :std::istream(&macbuf), macbuf() {}
    explicit imacrostream(std::istream& backing, MAC_HANDLE* ctxt=0)
        :std::istream(&macbuf), macbuf(backing, ctxt) {}

    inline void setBackingStream(std::istream& backing)
    { macbuf.setBackingStream(backing); }
    inline void setMacContext(MAC_HANDLE *ctxt)
    { macbuf.setMacContext(ctxt); }
};

#endif // MACSTREAM_H
