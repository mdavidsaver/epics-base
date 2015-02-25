#ifndef DBDPARSER_H
#define DBDPARSER_H

#include <vector>
#include <list>

#include "dbdlexer.h"

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
    DBDParser();
    virtual ~DBDParser(){};

    virtual void reset();

    enum parState_t {
        parDBD, parCoB, parCom, parCode, parArg, parArgCont, parTail
    };
    static const char* parStateName(parState_t S);

    virtual void lex(std::istream&);

    inline unsigned depth() const {return parDepth;}

    bool parDebug;

    typedef std::vector<std::string> blockarg_t;
protected:

    //! Command name from CoBtoken and arg from tok
    virtual void parse_command(DBDToken& cmd, DBDToken& arg)=0;

    //! comment from tok
    virtual void parse_comment(DBDToken&)=0;
    //! code from tok
    virtual void parse_code(DBDToken&)=0;

    //! Command name from CoBtoken and args from blockargs
    virtual void parse_block(DBDToken& name, blockarg_t&)=0;

    //! Mark start of block body
    virtual void parse_block_body_start()=0;
    //! Mark start of block body
    virtual void parse_block_body_end()=0;

    virtual void parse_start();
    virtual void parse_eoi();

private:
    virtual void token(tokState_t, DBDToken&);

    parState_t parState;

    unsigned parDepth;

    DBDToken CoBtoken;
    blockarg_t blockargs;
};

#endif // DBDPARSER_H
