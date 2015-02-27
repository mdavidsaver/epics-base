
#include <sstream>
#include <map> // for std::pair
#include <list>

#include "dbDefs.h"
#include "errlog.h"

#include "dbdast.h"

#include "epicsUnitTest.h"
#include "testMain.h"

struct Node {
    const char * name;
    DBDNodeType type;
    const char * value; // for statements/nest
    const char * args[DBDBlockMaxArgs]; // block
    const Node *nodes; // block
};
#define BLOCK_HEAD(NAME) {NAME, DBDNodeBlock, NULL
#define BLOCK_TAIL() }
#define END() {NULL}
#define VALUE(TYPE, NAME, VALUE) {NAME, DBDNode ## TYPE, VALUE, NULL, NULL}

static const char test1_inp[] = "menu(waveformPOST) {\n"
        "choice(waveformPOST_Always,\"Always\")\n"
        "choice(waveformPOST_OnChange,\"On Change\")}";

static const Node test1_out_inner[] = {
    BLOCK_HEAD("choice"), {"waveformPOST_Always","Always", NULL}, NULL},
    BLOCK_HEAD("choice"), {"waveformPOST_OnChange","On Change", NULL}, NULL},
    END()
};

static const Node test1_out[] = {
    BLOCK_HEAD("menu"), {"waveformPOST", NULL}, test1_out_inner},
    END()
};

static
bool doCompare(const Node *expect,
               const ELLLIST *actualnodes,
               std::list<const char*>& stack)
{
    bool pass = true;
    ELLNODE *cur;
    unsigned i;
    for(i=0, cur=ellFirst(actualnodes); cur && expect->name; cur=ellNext(cur), ++expect, ++i)
    {
        DBDNode *actual = CONTAINER(cur, DBDNode, node);
        if(expect->type!=actual->type) {
            testDiag("Node %u type doesn't match %d != %d",
                     i, expect->type, actual->type);
            return false;

        } else switch(actual->type) {
        case DBDNodeBlock: {
            DBDBlock *actblk = CONTAINER(actual, DBDBlock, common);
            if(std::string(actblk->name)!=expect->name) {
                testDiag("Node %u name doesn't match %s != %s",
                         i, actblk->name, expect->name);
                pass = false;
            }
            stack.push_back(actblk->name);
            // compare arguments
            unsigned n;
            for(n=0; n<actblk->nargs && expect->args[n]; ++n)
            {
                if(std::string(expect->args[n])!=actblk->args[n]) {
                    testDiag("Node %u Argument %u does not match %s != %s",
                             i, n, expect->args[n], actblk->args[n]);
                    pass = false;
                }
            }
            if(expect->args[n]) {
                testDiag("Node %u Fewer arguments than expected %u != %u",
                         i, n, actblk->nargs);
                pass = false;
            } else if(n<actblk->nargs) {
                testDiag("Node %u More arguments than expected %u != %u",
                         i, n, actblk->nargs);
                pass = false;
            }
            if(expect->nodes) {
                // compare children
                pass &= doCompare(expect->nodes, &actblk->children, stack);

            } else if(ellCount(&actblk->children)>0) {
                testDiag("Node %u Children when not expected", i);
                pass = false;
            }
            stack.pop_back();
            break;
        }
        case DBDNodeNest: {
            DBDNest *actnest = CONTAINER(actual, DBDNest, common);
            if(std::string(actnest->line)!=expect->name) {
                testDiag("Node %u line doesn't match %s != %s",
                         i, actnest->line, expect->name);
                pass = false;
            }

            break;
        }
        case DBDNodeStatement: {
            DBDStatement *actcmd = CONTAINER(actual, DBDStatement, common);
            if(std::string(actcmd->cmd)!=expect->name) {
                testDiag("Node %u name doesn't match %s != %s",
                         i, actcmd->cmd, expect->name);
                pass = false;
            }
            if(std::string(actcmd->arg)!=expect->value) {
                testDiag("Node %u value doesn't match %s != %s",
                         i, actcmd->arg, expect->value);
                pass = false;
            }
            break;
        }
        default: {
            testDiag("Node %u Unexpected type %d", i, (int)actual->type);
            pass = false;
            break;
        }
        }
    }
    if(cur) {
        testDiag("More children than expected %u", i);
        pass = false;
    } else if(expect->name)  {
        testDiag("Fewer children than expected %u", i);
        pass = false;
    }
    if(!pass && stack.size()) {
        std::ostringstream strm;
        std::list<const char*>::const_iterator it=stack.begin();
        for(;it!=stack.end(); ++it)
            strm << *it <<" ";
        testDiag("At %s", strm.str().c_str());
    }
    return pass;
}

static
void doTest(const char *inp,
            const Node *out,
            const char *msg)
{
    DBDFile *file = DBDParseMemory(inp, "testfile");
    if(!file) {
        testFail("Failed to parse %s", msg);
        return;
    }
    std::list<const char*> stack;
    bool pass = doCompare(out, &file->entries, stack);
    testOk(pass, "%s", msg);
}

MAIN(parseTest)
{
    testPlan(0);
    doTest(test1_inp, test1_out, "test1");
    //doTest(test2_inp, NULL, "test2");
    errlogFlush();
    return testDone();
}
