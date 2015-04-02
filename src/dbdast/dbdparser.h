#ifndef DBDPARSER_H
#define DBDPARSER_H

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

#endif // DBDPARSER_H
