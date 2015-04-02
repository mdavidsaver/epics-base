
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cassert>

#include "dbdparser.h"

void DBDParserActions::parse_command(DBDToken&, DBDToken&){}
void DBDParserActions::parse_comment(DBDToken&){}
void DBDParserActions::parse_code(DBDToken&){}
void DBDParserActions::parse_block(DBDToken&, blockarg_t&, bool){}
void DBDParserActions::parse_block_end(){}
void DBDParserActions::parse_start(){}
void DBDParserActions::parse_eoi(){}

DBDParser::DBDParser(DBDParserActions &actions)
    :parDebug(false)
    ,actions(&actions)
    ,parState(parDBD)
    ,parDepth(0)
{}

void DBDParser::reset()
{
    parState = parDBD;
    parDepth = 0;
    CoBtoken.reset();
    blockargs.clear();
    DBDLexer::reset();
}

void DBDParser::lex(std::istream& s)
{
    actions->parse_start();
    DBDLexer::lex(s);
}

const char* DBDParser::parStateName(parState_t S)
{
    switch(S) {
#define STATE(s) case s: return #s;
    STATE(parDBD);
    STATE(parCoB);
    STATE(parCom);
    STATE(parCode);
    STATE(parArg);
    STATE(parArgCont);
    STATE(parTail);
#undef STATE
    default:
        return "<invalid>";
    }
}

static void invalidToken(const DBDToken& t, DBDParser::tokState_t state)
{
    std::ostringstream strm;
    strm<<"Invalid token: "<<DBDParser::tokStateName(state)<<" "<<t;
    throw DBDSyntaxError(t, strm.str());
}

#define THROW(MSG) throw DBDSyntaxError(this->tok, MSG)

void DBDParser::token(tokState_t tokState, DBDToken &tok)
{
    if(parDebug)
        std::cerr<<"Parse "<<parDepth
                 <<" State: "<<parStateName(parState)
                 <<" Tok: "<<tokStateName(tokState)<<" "
                 <<tok<<"\n";

    switch(parState)
    {
    case parTail:
        /*
        state_tail : '{' -> reduce(block_start_short) -> state_dbd
                   | . -> reduce(block_short_start) -> jump(state_dbd)

         jump to state DBD w/o consuming token
         */
        // handle reduction of block now that we know if a body will follow.
        if(tokState==tokLit && tok.value.at(0)=='{') {
            actions->parse_block(CoBtoken, blockargs, true);
            parState = parDBD;
            CoBtoken.reset();
            blockargs.clear();
            parDepth++;
            return;
        } else {
            actions->parse_block(CoBtoken, blockargs, false);
            parState = parDBD;
            CoBtoken.reset();
            blockargs.clear();
            // handle this token as the DBD state
        }

    case parDBD:
        /*
        state_dbd : EOI -> reduce(finish) -> state_done
                  | tokBare -> shift -> state_CoB
                  | tokComment -> reduce(comment) -> state_dbd
                  | tokCode -> reduce(code) -> state_dbd
                  | '}' -> reduce(block_end) -> state_dbd
                  | . -> error
         */
        switch(tokState) {
        case tokEOI:
            if(parDepth==0) {
                actions->parse_eoi();
                return;
            } else
                THROW("EOI before }");
            break;

        case tokBare:
            // shift token
            CoBtoken.take(tok);
            parState = parCoB;
            break;

        case tokComment:
            // reduce comment
            actions->parse_comment(tok);
            parState = parDBD; break;

        case tokCode:
            // reduce code
            actions->parse_code(tok);
            parState = parDBD;break;

        case tokLit:
            assert(tok.size()==1);
            switch(tok.value.at(0))
            {
            case '}':
                // block ends
                if(parDepth==0)
                    THROW("'}' without '{'");
                parDepth--;
                actions->parse_block_end();
                parState = parDBD;
                break;
            default:
                THROW("Unexpected literal");
            }
            break;

        default:
            invalidToken(tok, tokState);
        }
        break;

    case parCoB:
        /*
        state_CoB : tokBare -> shift -> reduce(command) -> state_dbd
                  | tokQuoted -> shift -> reduce(command) -> state_dbd
                  | '(' -> state_arg
                  | . -> error
         */
        switch(tokState)
        {
        case tokBare:
        case tokQuote:
            // reduce command
            actions->parse_command(CoBtoken, tok);
            CoBtoken.reset();
            parState = parDBD;
            break;

        case tokLit:
            assert(tok.size()==1);
            if(tok.value.at(0)=='(') {
                // reduce initblock
                blockargs.clear();
                parState = parArg;
            } else
                THROW("Unexpected literal");
            break;

        default:
            invalidToken(tok, tokState);
        }
        break;

    case parArg:
        /*
        state_arg : ')' -> state_tail
                  | tokBare -> shift -> state_arg_cont
                  | tokQuoted -> shift -> state_arg_cont
                  | . -> error
         */
        switch(tokState) {
        case tokBare:
        case tokQuote:
            // shift arg
            blockargs.push_back(tok.value); //TODO: is a copy?
            parState = parArgCont;
            break;

        case tokLit:
            assert(tok.size()==1);
            if(tok.value.at(0)==')') {
                // reduce blockargs
                parState = parTail;
            } else
                THROW("Unexpected literal");
            break;

        default:
            invalidToken(tok, tokState);

        }
        break;

    case parArgCont:
        /*
        state_arg_cont : ',' -> state_arg
                       | ')' -> reduce(blockargs) -> state_tail
                       | . -> error
         */
        if(tokState!=tokLit)
            invalidToken(tok, tokState);

        assert(tok.size()==1);
        switch(tok.value.at(0))
        {
        case ',': parState = parArg; break;
        case ')': parState = parTail; break;
        default:
            THROW("Unexpected literal");
        }
        break;

    default:
        throw std::logic_error("Invalid parser state");
    }
}
