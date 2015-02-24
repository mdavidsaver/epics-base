
#include <cassert>
#include <cstdio>

#include <stdexcept>

#include "cfstream.h"

cfile_streambuf::cfile_streambuf(size_t bufsize)
    :fp(NULL)
    ,readbuf(8+bufsize)
    ,writebuf(1+bufsize)
{
    endreadbuf = &readbuf[0] + readbuf.size();
    setg(endreadbuf, endreadbuf, endreadbuf);
    endwritebuf = &writebuf[0] + writebuf.size() - 1;
    setp(&writebuf[0], endwritebuf);
}

cfile_streambuf::cfile_streambuf(FILE *fp, size_t bufsize)
    :fp(fp)
    ,readbuf(8+bufsize)
    ,writebuf(1+bufsize)
{
    endreadbuf = &readbuf[0] + readbuf.size();
    setg(endreadbuf, endreadbuf, endreadbuf);
    endwritebuf = &writebuf[0] + writebuf.size() - 1;
    setp(&writebuf[0], endwritebuf);
}

cfile_streambuf::~cfile_streambuf()
{
    if(fp)
        close();
}

void cfile_streambuf::set(FILE* fp)
{
    if(this->fp)
        close();
    this->fp = fp;
}

void cfile_streambuf::close()
{
    sync(); //TODO: what to do if sync fails?
    fclose(fp);
    fp=NULL;
}

cfile_streambuf::int_type
cfile_streambuf::underflow()
{
    char_type *front=eback(), *cur=gptr(), *back=egptr();
    if(cur<back) {
        // data in buffer
        // This case is a precaution only as streambuf tests this before calling us
        return traits_type::to_int_type(*cur);
    } else if(feof(fp) || ferror(fp))
        return traits_type::eof();

    // buffer is empty

    cur = &readbuf[8];
    size_t n = fread(cur, sizeof(char_type), endreadbuf-cur, fp);
    if(n==0) {
        return traits_type::eof();
    }

    if(sizeof(char_type)>1 && n%sizeof(char_type)!=0) {
        // mis-aligned read of multi-byte charactors
        // jump back to re-align
        size_t extra = n%sizeof(char_type);
        int ret = fseek(fp, -extra, SEEK_CUR);
        if(!ret)
            throw std::runtime_error("seek in underflow() fails");
    }

    setg(front, cur, cur+n);
    return traits_type::to_int_type(*cur);
}

cfile_streambuf::int_type
cfile_streambuf::overflow(int_type c)
{
    int_type xeof = traits_type::eof();
    if(c==xeof) {
        sync();
        return c;
    }

    char_type *cur=pptr(), *back=epptr();
    if(cur<back) {
        // buffer not full
        *cur=traits_type::to_char_type(c);
        pbump(1);
        return c;
    }
    // buffer full (we reserve one char extra)
    *cur++ = traits_type::to_char_type(c);
    back++;

    int ret = doFlush();
    if(ret!=0)
        return xeof;
    else
        return c;
}

int
cfile_streambuf::sync()
{
    int ret = doFlush();
    if(ret)
        return ret;
    ret = fflush(fp);
    if(ret!=0)
        return -1;
    return 0;
}

int
cfile_streambuf::doFlush()
{
    char_type *front=pbase(), *cur=pptr();

    size_t towrite = cur-front;
    if(towrite==0)
        return 0;

    size_t n = fwrite(front, sizeof(char_type), towrite, fp);
    if(n==0)
        return -1;

    if(n<towrite) {
        size_t unwritten = towrite-n;
        // incomplete write
        memmove(front, cur, unwritten*sizeof(char_type));
        cur = front+unwritten;
    } else {
        // buffer empty
        cur = front;
    }

    setp(cur, endwritebuf);
    return 0;
}
