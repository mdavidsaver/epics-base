#ifndef DBDPARSER_H
#define DBDPARSER_H

#ifdef __cplusplus

#include <vector>
#include <list>

#include "dbdlexer.h"

class DBDParserActions
{
public:
    typedef std::vector<std::string> blockarg_t;

    //! Command name from CoBtoken and arg from tok
    virtual void parse_command(DBDToken& cmd, DBDToken& arg)=0;

    //! comment from tok
    virtual void parse_comment(DBDToken&)=0;
    //! code from tok
    virtual void parse_code(DBDToken&)=0;

    //! Block name and args, flag indicates whether this block has a body
    virtual void parse_block(DBDToken& name, blockarg_t&, bool bodytofollow)=0;

    //! Mark end of block body
    virtual void parse_block_end()=0;

    virtual void parse_start();
    virtual void parse_eoi();
};

/** @brief Parser for DB/DBD grammar
 *
 @code
    value : tokBare | tokQuote

    entry : command
          | code
          | comment
          | block

    command : tokBare value

    code : tokCode
    comment : tokComment

    block : tokeBare block_head
          | tokeBare block_head bock_body

    block_head : '(' ')'
               | '(' value *(',' value) ')'

    block_body : '{' dbd '}'

    dbd :
        | entry
        | entry dbd
 @endcode
 */
class DBDParser : public DBDLexer
{
public:
    DBDParser(DBDParserActions& actions);
    virtual ~DBDParser(){};

    virtual void reset();

    enum parState_t {
        parDBD, parCoB, parCom, parCode, parArg, parArgCont, parTail
    };
    static const char* parStateName(parState_t S);

    virtual void lex(std::istream&);

    inline unsigned depth() const {return parDepth;}

    bool parDebug;

    typedef DBDParserActions::blockarg_t blockarg_t;

private:
    virtual void token(tokState_t, DBDToken&);

    DBDParserActions* actions;

    parState_t parState;

    unsigned parDepth;

    DBDToken CoBtoken;
    blockarg_t blockargs;
};

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
typedef DBDParserActions::blockarg_t DBDBlockArgs;
#else
typedef struct DBDBlockArgs DBDBlockArgs;
#endif

const char* DBDToken_value(const DBDToken*);
unsigned DBDToken_line(const DBDToken*);
unsigned DBDToken_col(const DBDToken*);
size_t DBDBlockArgs_len(const DBDBlockArgs*);
const char* DBDBlockArgs_value(const DBDBlockArgs*, size_t);

typedef struct DBDCParserActions {
    int (*parse_command)(struct DBDCParserActions *self,
                         const DBDToken* cmd, const DBDToken* arg);
    //! comment from tok
    int (*parse_comment)(struct DBDCParserActions *self,
                         const DBDToken*);
    //! code from tok
    int (*parse_code)(struct DBDCParserActions *self,
                      const DBDToken*);

    //! Block name and args, flag indicates whether this block has a body
    int (*parse_block)(struct DBDCParserActions *self,
                       const DBDToken* name, const DBDBlockArgs*, int bodytofollow);

    //! Mark end of block body
    int (*parse_block_end)(struct DBDCParserActions *self);

    int (*parse_start)(struct DBDCParserActions *self);
    int (*parse_eoi)(struct DBDCParserActions *self);

} DBDCParserActions;

#ifdef __cplusplus
}

class DBDCParserActionsWrapper
{
    DBDCParserActions *act;
    typedef DBDParserActions::blockarg_t blockarg_t;
public:
    DBDCParserActionsWrapper(DBDCParserActions *a) : act(a) {}
#define DDTHROW() throw std::runtime_error("User abort")

    virtual void parse_command(DBDToken& cmd, DBDToken& arg)
    { if(act->parse_command && (*act->parse_command)(act, &cmd, &arg)) DDTHROW();}

    virtual void parse_comment(DBDToken& t)
    { if(act->parse_comment && (*act->parse_comment)(act, &t)) DDTHROW();}

    virtual void parse_code(DBDToken& t)
    { if(act->parse_code && (*act->parse_code)(act, &t)) DDTHROW();}

    virtual void parse_block(DBDToken& name, blockarg_t& a, bool bodytofollow)
    { if(act->parse_block && (*act->parse_block)(act, &name, &a, bodytofollow)) DDTHROW();}

    virtual void parse_block_end()
    { if(act->parse_block_end && (*act->parse_block_end)(act)) DDTHROW();}

    virtual void parse_start()
    { if(act->parse_start && (*act->parse_start)(act)) DDTHROW();}

    virtual void parse_eoi()
    { if(act->parse_eoi && (*act->parse_eoi)(act)) DDTHROW();}
#undef DDTHROW
};

#endif /* __cplusplus */

#endif /* DBDPARSER_H */
