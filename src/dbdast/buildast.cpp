#include <cstring>
#include <cassert>

#include <fstream>
#include <sstream>

#include <errlog.h>

#include "dbdast.h"
#include "dbdparser.h"
#include "cfstream.h"

namespace {
class DBDASTActions : public DBDParserActions
{
public:
    DBDASTActions(DBDContext* ctxt)
        :totalalloc(0)
        ,ctxt(ctxt)
    {
        fakeroot = (DBDBlock*)calloc(1, sizeof(*fakeroot));
        if(!fakeroot)
            throw std::bad_alloc();
    }

    virtual ~DBDASTActions()
    {
        DBDFreeNode(&fakeroot->common);
    }

    //virtual void reset();

    virtual void parse_command(DBDToken& cmd, DBDToken& arg)
    {
        size_t clen = cmd.value.size(),
               alen =arg.value.size();
        if(clen>0xffffff||alen>0xffffff)
            throw std::bad_alloc();
        DBDStatement *stmt=(DBDStatement*)calloc(1, sizeof(*stmt)+alen+clen);
        totalalloc += sizeof(*stmt)+alen+clen;
        if(!stmt)
            throw std::bad_alloc();
        stmt->common.type = DBDNodeStatement;

        stmt->cmd = stmt->_alloc;
        stmt->arg = stmt->cmd+clen+1;
        stmt->common.line = cmd.line;
        stmt->common.col = cmd.col;
        memcpy(stmt->cmd, cmd.value.c_str(), clen);
        memcpy(stmt->arg, arg.value.c_str(), alen);

        ellAdd(&stack.back()->children, &stmt->common.node);
    }

    void nest(char type, DBDToken& C)
    {
        size_t len = C.value.size();
        if(len>0xffffff)
            throw std::bad_alloc();

        DBDNest *stmt=(DBDNest*)calloc(1, sizeof(*stmt)+len+1);
        totalalloc += sizeof(*stmt)+len+1;
        if(!stmt)
            throw std::bad_alloc();

        stmt->common.type = DBDNodeNest;
        stmt->common.line = C.line;
        stmt->common.col = C.col;
        stmt->line[0] = type;
        memcpy(stmt->line+1, C.value.c_str(), len);

        ellAdd(&stack.back()->children, &stmt->common.node);
    }

    virtual void parse_comment(DBDToken& C){nest('#',C);}
    virtual void parse_code(DBDToken& C){nest('%',C);}

    virtual void parse_block(DBDToken& name, blockarg_t& args, bool hasbody)
    {
        size_t nlen = name.value.size(), alen=nlen,
               bcnt = args.size();
        if(nlen>0xff || bcnt>DBDBlockMaxArgs)
            throw std::bad_alloc();

        for(unsigned i=0; i<bcnt; i++) {
            size_t esize = args[i].size()+1;
            if(size_t(-1)-alen<esize)
                throw std::bad_alloc();
            alen += esize;
        }
        DBDBlock *stmt=(DBDBlock*)calloc(1, sizeof(*stmt)+alen);
        totalalloc += sizeof(*stmt)+alen;
        if(!stmt)
            throw std::bad_alloc();
#ifndef NDEBUG
        const char * const buflim = (char*)stmt + sizeof(*stmt)+alen;
#endif

        stmt->common.type = DBDNodeBlock;
        stmt->common.line = name.line;
        stmt->common.col = name.col;
        stmt->name = stmt->_alloc;
        stmt->nargs = bcnt;
        memcpy(stmt->name, name.value.c_str(), nlen);

        char *buf = stmt->_alloc+nlen+1;
        for(size_t i = 0;  i<bcnt; i++) {
            assert(buf<buflim);
            size_t esize = args[i].size();
            stmt->args[i] = buf;
            memcpy(buf, args[i].c_str(), esize);
            buf += esize+1;
        }

        ellAdd(&stack.back()->children, &stmt->common.node);

        if(hasbody) {
            stack.push_back(stmt);
        }
    }

    virtual void parse_block_end()
    {
        stack.pop_back();
    }

    virtual void parse_start()
    {
        stack.push_back(fakeroot);
    }

    virtual void parse_eoi()
    {
        assert(stack.size()==1);
        assert(stack.at(0)==fakeroot);
        stack.pop_back();
    }

    std::vector<DBDBlock*> stack;

    DBDBlock *fakeroot;
    size_t totalalloc;
    DBDContext *ctxt;
};

}//namespace

DBDFile *DBDParseStream(DBDContext *ctxt, std::istream& istrm, const char *fname)
{
    DBDFile *file = (DBDFile*)calloc(1, sizeof(*file)+strlen(fname));
    if(!file) {
        return NULL;
    }
    strcpy(file->name, fname);

    try{
        DBDASTActions actions(ctxt);
        DBDParser P(actions);
        P.lex(istrm);
        ellConcat(&file->entries, &actions.fakeroot->children);
    } catch(std::exception& e) {
        free(file);
        throw;
    }

    return file;

}

DBDFile *DBDParseFileP(DBDContext *ctxt, FILE *fp, const char* fname)
{
    try{
        // fp closed by cfile_streambuf dtor
        cfile_streambuf isb(fp);
        std::istream istrm(&isb);
        return DBDParseStream(ctxt, istrm, fname);
    } catch(std::exception& e) {
        errlogPrintf("Parsing error: %s", e.what());
        return NULL;
    }
}

DBDFile *DBDParseFile(DBDContext *ctxt, const char* fname)
{
    DBDFile *ret;
    FILE *fp=fopen(fname, "r");
    ret = DBDParseFileP(ctxt, fp, fname);
    // fp closed by cfile_streambuf dtor
    return ret;
}

DBDFile *DBDParseMemory(DBDContext *ctxt, const char *buf, const char *fname)
{
    try{
        std::istringstream istrm(buf);
        return DBDParseStream(ctxt, istrm, fname);
    } catch(std::exception& e) {
        errlogPrintf("Parsing error: %s", e.what());
        return NULL;
    }
}