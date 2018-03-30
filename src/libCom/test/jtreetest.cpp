/*************************************************************************\
* Copyright (c) 2018 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <string.h>
#include <stdlib.h>

#include <epicsUnitTest.h>
#include <testMain.h>
#include <dbDefs.h>

#include <epicsjtree.h>


namespace {

namespace ejt = epics::jtree;

static
void errfn(void *errpvt, size_t pos, const char *msg)
{
    testDiag("Error at %zu : %s", pos, msg);
}

static void testEmpty()
{
    static const char value[] = "";
    testDiag("testEmpty()");

    const epicsJTree *tree = ejtParse(value, strlen(value), errfn, NULL, 0);
    testOk1(!tree);
    ejtFree(tree);
}

static void testComment()
{
    static const char value[] = "  // foo!";
    testDiag("testComment()");

    const epicsJTree *tree = ejtParse(value, strlen(value), errfn, NULL, 0);
    testOk1(!tree);
    ejtFree(tree);
}

static void testJunk()
{
    static const char value[] = "hello";
    testDiag("testJunk()");

    const epicsJTree *tree = ejtParse(value, strlen(value), errfn, NULL, 0);
    testOk1(!tree);
    ejtFree(tree);
}

static void testIncomplete()
{
    static const char value[] = "{";
    testDiag("testIncomplete()");

    const epicsJTree *tree = ejtParse(value, strlen(value), errfn, NULL, 0);
    testOk1(!tree);
    ejtFree(tree);
}

static void testNULL()
{
    testDiag("testNULL()");

    testOk1(ejtRoot(NULL)==NULL);
    testOk1(ejtType(NULL)==epicsJNodeTypeNull);
    testOk1(ejtCount(NULL)==(size_t)-1);
    testOk1(ejtLookup(NULL, NULL, 0)==NULL);
    testOk1(ejtIndex(NULL, 0)==NULL);
    testOk1(!ejtAsBool(NULL, 0, NULL));
    testOk1(!ejtAsInteger(NULL, 0, NULL));
    testOk1(!ejtAsReal(NULL, 0, NULL));
    testOk1(!ejtAsString(NULL, 0, NULL));
    testOk1(ejtAsStringAlloc(NULL, 0)==NULL);
    testOk1(!ejtIterate(NULL, NULL, NULL, NULL));
    ejtFree(NULL);
}

static void testPlainNumber()
{
    static const char value[] = "42";
    testDiag("testPlainNumber()");

    const epicsJTree *tree = ejtParse(value, strlen(value), errfn, NULL, 0);
    const epicsJNode *node = ejtRoot(tree);
    long long val;

    testOk1(!!tree);
    testOk1(!!node);
    testOk1(ejtType(node) == epicsJNodeTypeInteger);
    testOk1(ejtAsInteger(node, 0, &val));
    testOk  (val==42, "val = %lld", val);

    ejtFree(tree);
}

static void testString()
{
    static const char value[] = " \"this is a \\\"test \"";
    static const char raw_expect[] = "this is a \"test ";

    testDiag("testString()");

    const epicsJTree *tree = ejtParse(value, strlen(value), errfn, NULL, 0);
    const epicsJNode *node = ejtRoot(tree);
    const char* raw_actual = NULL;

    testOk1(!!tree);
    testOk1(!!node);
    testOk1(ejtType(node) == epicsJNodeTypeString);
    testOk1(ejtAsString(node, 0, &raw_actual));
    testOk1(strcmp(raw_expect, raw_actual)==0);
    testDiag("actual = '%s'", raw_actual);

    ejtFree(tree);
}

static void testBig()
{
    const char value[] ="{"
                            "\"one\": 1,"
                            "\"two\": \"other\","
                            "\"abc\": {"
                                "\"xyz\": [1, \"two\", {\"q\": 5}]"
                            "},"
                            "\"a\": true,"
                        "}";

    const epicsJTree * const tree = ejtParse(value, strlen(value), errfn, NULL, 0);
    const epicsJNode * const node = ejtRoot(tree);
    int bval;
    long long ival;
    const char *sval;
    testDiag("testBig()");

    testOk1(ejtLookup(node, "nonexistant", 0)==NULL);
    testOk1(ejtLookupAsInteger(node, "one", 0, &ival) && ival==1);
    testOk1(ejtLookupAsString(node, "two", 0, &sval) && strcmp(sval, "other")==0);
    testOk1(ejtLookupAsBool(node, "a", 0, &bval) && bval==1);

    {
        const epicsJNode *abc = ejtLookup(node, "abc", 0),
                         *xyz = ejtLookup(abc, "xyz", 0),
                         *temp;

        testOk1(ejtType(xyz)==epicsJNodeTypeArray);

        testOk1((temp = ejtIndex(xyz, 0))!=NULL);
        testOk1(ejtAsInteger(temp, 0, &ival) && ival==1);

        testOk1((temp = ejtIndex(xyz, 1))!=NULL);
        testOk1(ejtAsString(temp, 0, &sval) && strcmp(sval, "two")==0);
    }

    {
        epicsJNodeIterator iter = NULL;
        const char *key;
        const epicsJNode *child;
        size_t i = 0;

        while(ejtIterate(node, &iter, &key, &child)) {
            switch(i++) {
            case 0:
                testOk1(strcmp(key, "one")==0);
                break;
            case 1:
                testOk1(strcmp(key, "two")==0);
                break;
            case 2:
                testOk1(strcmp(key, "abc")==0);
                break;
            case 3:
                testOk1(strcmp(key, "a")==0);
                break;
            default:
                testFail("Larger than expected");
            }
        }
    }

    ejtFree(tree);
}

void testcxx_err()
{
    const char value[] ="// this is line 1\n {\"blah\":}\n// parser doesn't get here";

    testDiag("testcxx_err()");
    try{
        ejt::Tree tree;
        tree.parse(value, strlen(value));
        testFail("Missed expected exception");
    }catch(ejt::ParseError& e) {
        testDiag("expected error %s", e.what());
        testOk(e.pos==28 && e.line==2 && e.col==11, "pos=%zu line=%zu col=%zu", e.pos, e.line, e.col);
    }
}

void testcxx()
{
    const char value[] ="{"
                            "\"one\": 1,"
                            "\"two\": \"other\","
                            "\"abc\": {"
                                "\"xyz\": [1, \"two\", {\"q\": 5}]"
                            "},"
                            "\"a\": true,"
                        "}";

    testDiag("testcxx()");

    ejt::Tree tree;
    tree.parse(value, strlen(value));
    testOk1(!!tree);

    ejt::Node root(tree.root());
    testOk1(!!root);

    testOk1(root["one"].as<long long>()==1);
    testOk1(root["two"].as<std::string>()=="other");
    testOk1(root["a"].as<bool>()==true);

    testOk1(root.begin()!=root.end());

    size_t i=0;
    for(ejt::Node::iterator it(root.begin()), end(root.end()); it!=end; ++it)
    {
        switch(i++) {
        case 0: testOk1(strcmp(it->first, "one")==0); break;
        case 1: testOk1(strcmp(it->first, "two")==0); break;
        case 2: testOk1(strcmp(it->first, "abc")==0); break;
        case 3: testOk1(strcmp(it->first, "a")==0); break;
        default: testFail("Unexpected key %s", it->first); break;
        }
    }

#if __cplusplus>=201103L
    i=0;
    for(auto& pair : root) {
        switch(i++) {
        case 0: testOk1(strcmp(pair.first, "one")==0); break;
        case 1: testOk1(strcmp(pair.first, "two")==0); break;
        case 2: testOk1(strcmp(pair.first, "abc")==0); break;
        case 3: testOk1(strcmp(pair.first, "a")==0); break;
        default: testFail("Unexpected key %s", pair.first); break;
        }
    }
#else
    testSkip(4, "Not c++11");
#endif // c++11
}

void testarr()
{
    const char value[] = " [1, null, 3]";

    testDiag("testarr()");

    ejt::Tree tree;
    tree.parse(value, strlen(value));
    testOk1(!!tree);

    ejt::Node root(tree.root());
    testOk1(!!root);

    size_t i=0;
    for(ejt::Node::iterator it(root.begin()), end(root.end()); it!=end; ++it)
    {
        testOk1(it->first==NULL);
        switch(i++) {
        case 0:  testOk1(it->second.as<long long>()==1); break;
        case 1:  testOk1(it->second.type()==epicsJNodeTypeNull); break;
        case 2:  testOk1(it->second.as<long long>()==3); break;
        default: testFail("Unexpected key %s", it->first); break;
        }
    }
}

} // namespace

MAIN(jtreetest)
{
    testPlan(61);
    try {
        testEmpty();
        testComment();
        testJunk();
        testIncomplete();
        testNULL();
        testPlainNumber();
        testString();
        testBig();
        testcxx_err();
        testcxx();
        testarr();
    }catch(std::exception& e){
        testFail("Unhandled C++ exception : %s", e.what());
    }
    return testDone();
}
