
#include <stdexcept>
#include <sstream>
#include <vector>

#include "macstream.h"

#include "epicsUnitTest.h"
#include "testMain.h"

#define TOK(X) #X

namespace {

class TestMessageGenerator
{
    int pass;
    std::ostringstream testmsg;
public:
    TestMessageGenerator(bool pass)
        :pass(pass), testmsg()
    {}
    ~TestMessageGenerator()
    {
        testOk(pass, testmsg.str().c_str());
    }
    template<typename T>
    TestMessageGenerator&
    operator<<(T m)
    {
        testmsg<<m;
        return *this;
    }
};

#define testEqual(a, b) TestMessageGenerator(a==b) << #a " ("<<a<<") == " #b " ("<<b<<") "

}

static
void readLineTooShort(size_t i)
{
    size_t j=i;
    if(j>0)
        j--;

    testDiag("Test buf %u",(unsigned)i);
    std::string expect("this is a string with\ntwo lines");
    std::istringstream strm(expect);
    std::vector<char> buf(i);

    strm.getline(&buf[0], buf.size());
    testEqual(strm.gcount(), (unsigned)j);
    testOk1(strm.fail());
    testOk1(!strm.bad());
    testOk1(!strm.eof());

    strm.clear();
    testOk1(strm.good());
    std::string result(&buf[0], j);
    testEqual(result, expect.substr(0,j));
}

static
void readLine()
{
    testDiag("Check the behavour of std::istream::getline()");

    // we should never actually do this, but the spec.
    // says this will set the fail bit
    readLineTooShort(0);

    // should set fail bit to indicate that the line buffer is too short
    // but only "removes" a nill, which isn't counted
    readLineTooShort(1);

    // will have space to remove the first charactor
    readLineTooShort(2);

    readLineTooShort(7);

    {
        testDiag("Read whole line");
        std::istringstream strm("this is a string with\ntwo lines");
        std::vector<char> buf(30);
        strm.getline(&buf[0], buf.size());
        testOk1(strm.good());
        testEqual(strm.gcount(), 22);

        // gcount reports on count for the '\n' which isn't stored.
        std::string result(&buf[0], 21);
        testEqual(result, "this is a string with");
    }

    {
        testDiag("read whole string w/o newline");
        std::istringstream strm("this is one line");
        std::vector<char> buf(30);
        strm.getline(&buf[0], buf.size());
        testOk1(!strm.fail());
        testOk1(!strm.bad());
        testOk1(strm.eof());
        testEqual(strm.gcount(), 16);

        std::string result(&buf[0], 16);
        testEqual(result, "this is one line");
    }

}

namespace {
struct macro_context
{
    MAC_HANDLE *ctxt;
    explicit macro_context(const char* defs)
    {
        char **pairs = 0;
        if(macCreateHandle(&ctxt, NULL))
            throw std::runtime_error("Failed to create context");
        long ndefs = macParseDefns(ctxt, defs, &pairs);
        if(ndefs<0)
            throw std::runtime_error("Failed to parse macro definitions");
        ndefs = macInstallMacros(ctxt, pairs);
        if(ndefs<0)
            throw std::runtime_error("Failed to install macro definitions");

    }
    ~macro_context() {
        macDeleteHandle(ctxt);
    }
};
}

static
void macStream()
{
    testDiag("Test imacrostream");
    {
        testDiag("No macro context");
        std::istringstream bstrm("this is a string with\ntwo lines");
        imacrostream ostrm(bstrm);

        std::string line;
        std::getline(ostrm, line);
        testEqual(line, "this is a string with");
        std::getline(ostrm, line);
        testEqual(line, "two lines");
    }
    {
        testDiag("With macro context");
        macro_context ctxt("A=hello,B=world");
        std::istringstream bstrm("this is a ${A} with\ntwo ${B}s");
        imacrostream ostrm(bstrm, ctxt.ctxt);

        std::string line;
        std::getline(ostrm, line);
        testEqual(line, "this is a hello with");
        std::getline(ostrm, line);
        testEqual(line, "two worlds");
    }

}

MAIN(streamTest)
{
    testPlan(36);
    readLine();
    macStream();
    return testDone();
}
