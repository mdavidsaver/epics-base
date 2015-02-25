
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cassert>

#include "dbdparser.h"

DBDParser::DBDParser()
    :parDebug(false)
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
    parse_start();
    DBDLexer::lex(s);
}

void DBDParser::parse_command(DBDToken&, DBDToken&){}
void DBDParser::parse_comment(DBDToken&){}
void DBDParser::parse_code(DBDToken&){}
void DBDParser::parse_block(DBDToken&, blockarg_t&){}
void DBDParser::parse_block_body_start(){}
void DBDParser::parse_block_body_end(){}
void DBDParser::parse_start(){}
void DBDParser::parse_eoi(){}

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
    throw std::runtime_error(strm.str());
}

void DBDParser::token(tokState_t tokState, DBDToken &tok)
{
    if(parDebug)
        std::cerr<<"Parse "<<parDepth
                 <<" State: "<<parStateName(parState)
                 <<" Tok: "<<tokStateName(tokState)<<" "
                 <<tok<<"\n";

    switch(parState)
    {
    case parDBD:
    case parTail:
        /*
        state_dbd : EOI -> reduce(finish) -> state_done
                  | tokBare -> shift -> state_CoB
                  | tokComment -> reduce(comment) -> state_dbd
                  | tokCode -> reduce(code) -> state_dbd
                  | '}' -> reduce(block) -> state_dbd
                  | . -> error

        state_tail : '{' -> reduce(initbody) -> state_dbd
                   | EOI -> reduce(finish) -> state_done
                   | tokBare -> shift -> state_CoB
                   | tokComment -> reduce(comment) -> state_dbd
                   | tokCode -> reduce(code) -> state_dbd
                   | '}' -> reduce(block) -> state_dbd
                   | . -> error
         */
        switch(tokState) {
        case tokEOI:
            if(parDepth==0) {
                parse_eoi();
                return;
            } else
                throw std::runtime_error("EOI before }");
            break;

        case tokBare:
            // shift token
            CoBtoken.take(tok);
            parState = parCoB;
            break;

        case tokComment:
            // reduce comment
            parse_comment(tok);
            parState = parDBD; break;

        case tokCode:
            // reduce code
            parse_code(tok);
            parState = parDBD;break;

        case tokLit:
            assert(tok.size()==1);
            switch(tok.value.at(0))
            {
            case '}':
                // reduce block
                if(parDepth==0)
                    throw std::runtime_error("'}' without '{'");
                parDepth--;
                parse_block_body_end();
                parState = parDBD;
                break;
            case '{':
                if(parState==parTail) {
                    // reduce initbody
                    if(parDepth==0xffff)
                        throw std::runtime_error("Block depth limit exceed");
                    parse_block_body_start();
                    parDepth++;
                    parState = parDBD;
                    break;
                }
            default:
                throw std::runtime_error("Unexpected literal");
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
                  | '(' -> reduce(initblock) -> state_arg
                  | . -> error
         */
        switch(tokState)
        {
        case tokBare:
        case tokQuote:
            // reduce command
            parse_command(CoBtoken, tok);
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
                throw std::runtime_error("Unexpected literal");
            break;

        default:
            invalidToken(tok, tokState);
        }
        break;

    case parArg:
        /*
        state_arg : ')' -> reduce(blockargs) -> state_tail
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
                parse_block(CoBtoken, blockargs);
                parState = parTail;
            } else
                throw std::runtime_error("Unexpected literal");
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
        case ')':
            // reduce blockargs
            parse_block(CoBtoken, blockargs);
            parState = parTail;
            break;
        default:
            throw std::runtime_error("Unexpected literal");
        }
        break;

    default:
        throw std::logic_error("Invalid parser state");
    }
}
