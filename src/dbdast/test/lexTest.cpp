
#include <sstream>
#include <map> // for std::pair
#include <list>

#include "dbdlexer.h"

#include "epicsUnitTest.h"
#include "testMain.h"

namespace {
struct LexnStore : public DBDLexer
{
    typedef std::list<std::pair<DBDLexer::tokState_t, DBDToken> > list_t;
    list_t tokens;

    virtual void token(tokState_t s, DBDToken &t)
    {
        tokens.push_back(std::make_pair(s,t));
    }

    virtual void reset()
    {
        DBDLexer::reset();
        tokens.clear();
    }
};
}

struct Pair {
    DBDLexer::tokState_t state;
    const char *value;
    unsigned line, col;
};
#define TOKEN(STATE, VALUE, LINE, COL) {DBDLexer::tok ## STATE, VALUE, LINE, COL}
#define EOI(LINE, COL) {DBDLexer::tokEOI, "", LINE, COL}, {DBDLexer::tokInit, NULL, 0, 0}

static const char test1_inp[] = " tag ( 1, \"hello \" )";

static const Pair test1_out[] = {
    TOKEN(Bare, "tag", 1, 2),
    TOKEN(Lit, "(", 1, 6),
    TOKEN(Bare, "1", 1, 8),
    TOKEN(Lit, ",", 1, 9),
    TOKEN(Quote, "hello ", 1, 11),
    TOKEN(Lit, ")", 1, 20),
    EOI(1, 20)
};

static const char test2_inp[] = "test, # comment\n  #another ";

static const Pair test2_out[] = {
    TOKEN(Bare, "test", 1, 1),
    TOKEN(Lit, ",", 1, 5),
    TOKEN(Comment, " comment", 1, 7),
    TOKEN(Comment, "another ", 2, 3),
    EOI(2, 11)
};

static const char test3_inp[] = "+_( \"hello \\\"world\"";

static const Pair test3_out[] = {
    TOKEN(Bare, "+_", 1, 1),
    TOKEN(Lit, "(", 1, 3),
    TOKEN(Quote, "hello \"world", 1, 5),
    EOI(1, 19)
};

static const char test4_inp[] = "\"1\"\"3\"";

static const Pair test4_out[] = {
    TOKEN(Quote, "1", 1, 1),
    TOKEN(Quote, "3", 1, 4),
    EOI(1, 6)
};

static const char test5_inp[] = "hello\"\x02\x7f\" test";

static const Pair test5_out[] = {
    TOKEN(Bare, "hello", 1, 1),
    TOKEN(Quote, "\x02\x7f", 1, 6),
    TOKEN(Bare, "test", 1, 11),
    EOI(1, 14)
};

static void doTest(const char *inp,
                   const Pair *out,
                   const char *name)
{
    LexnStore lexer;
    std::istringstream strm(inp);
    try{
        lexer.lex(strm);
    }catch(std::exception& e){
        testFail("Exception %s while lexer %s", e.what(), inp);
        return;
    }
    const Pair *expect = out;
    LexnStore::list_t::const_iterator it=lexer.tokens.begin(),
                                     end=lexer.tokens.end();
    unsigned i;
    bool pass = true;
    for(i=0; it!=end && expect->value; ++it, ++expect, ++i)
    {
        if(it->first!=expect->state) {
            testDiag("Token %u states don't match %s != %s",
                     i, DBDLexer::tokStateName(it->first),
                     DBDLexer::tokStateName(expect->state));
            pass = false;

        } else if(it->second.value!=expect->value) {
            testDiag("Token %u values don't match %s != %s",
                     i, it->second.value.c_str(), expect->value);
            pass = false;

        } else if(it->second.line!=expect->line ||
                  it->second.col!=expect->col) {
            testDiag("Token %u line:col don't match %u:%u != %u:%u",
                     i, it->second.line, it->second.col,
                     expect->line, expect->col);
            pass = false;
        }
    }
    if(it!=end) {
        testDiag("More tokens than expected %u > %u",
                 (unsigned)lexer.tokens.size(), i);
        pass = false;
    } else if(expect->value) {
        testDiag("Fewer tokens than expected %u < %u",
                 (unsigned)lexer.tokens.size(), i);
        pass = false;
    }

    testOk(pass, "%s", name);
}

MAIN(lexTest)
{
    testPlan(0);
    testDiag("Parse test1 %s", test1_inp);
    doTest(test1_inp, test1_out, "test1");
    testDiag("Parse test2 (multiline)");
    doTest(test2_inp, test2_out, "test2");
    testDiag("Parse test3 %s", test3_inp);
    doTest(test3_inp, test3_out, "test3");
    testDiag("Parse test4 %s", test4_inp);
    doTest(test4_inp, test4_out, "test4");
    testDiag("Parse test5 %s", test5_inp);
    doTest(test5_inp, test5_out, "test5");
    return testDone();
}
