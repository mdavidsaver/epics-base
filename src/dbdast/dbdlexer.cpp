#include <stdexcept>
#include <iostream>
#include <sstream>
#include <assert.h>

#include "dbdlexer.h"

DBDLexer::DBDLexer()
    :lexDebug(false)
    ,tokState(tokInit)
    ,line(1)
    ,col(0)
{}

void DBDLexer::token(tokState_t, DBDToken &) {}

void DBDLexer::reset()
{
    tokState = tokInit;
    tok.reset();
    line = 1;
    col = 0;
}

std::ostream& operator<<(std::ostream& strm, const DBDToken& t)
{
    strm<<t.line<<":"<<t.col<<"\t'"<<t.value<<"'";
    return strm;
}

const char* DBDLexer::tokStateName(tokState_t S)
{
    switch(S) {
#define STATE(S) case DBDLexer::S: return #S
    STATE(tokInit);
    STATE(tokLit);
    STATE(tokQuote);
    STATE(tokEsc);
    STATE(tokBare);
    STATE(tokCode);
    STATE(tokComment);
    STATE(tokEOI);
#undef STATE
    default:
        return "<invalid>";
    }
}

static
std::string LexError(const DBDLexer& L, const char *msg)
{
    std::ostringstream strm;
    strm<<"Lexer error at "<<L.tok.line<<":"<<L.tok.col<<" : "<<msg;
    return strm.str();
}
#define THROW(msg) throw std::runtime_error(LexError(*this, msg))

std::string DBDLexer::InvalidChar(const DBDLexer& L, DBDLexer::tokState_t state, char c)
{
    std::ostringstream strm;
    strm<<"Invalid charactor at "<<L.line<<":"<<L.col
        <<" : '"<<c<<"' ("<<int(c)<<") State "
        <<DBDLexer::tokStateName(state);
    return strm.str();
}
#define INVALID(c) throw std::runtime_error(InvalidChar(*this, this->tokState, c))

static bool iswordchar(char c)
{
    if(c>='a' && c<='z') return true;
    else if(c>='A' && c<='Z') return true;
    else if(c>='0' && c<='9') return true;
    else switch(c) {
    case '_':
    case '-':
    case '+':
    case ':':
    case ';':
    case '.':
    case '[':
    case ']':
    case '<':
    case '>': return true;
    default: return false;
    }
}

void DBDLexer::doToken(tokState_t next)
{
    if(lexDebug)
        std::cerr<<"Lex finish token\n";
    token(tokState, tok);
    tok.reset();
    tokState = next;
}

void DBDLexer::setLine()
{
    if(lexDebug)
        std::cerr<<"Lex start token\n";
    tok.line=line;
    tok.col=col;
}

void DBDLexer::lex(std::istream &strm)
{
    char c;
    while(strm.get(c)) {
        if(c=='\n') {
            line++;
            col=0;
        } else
            col++;

        if(lexDebug)
            std::cerr<<"Lex "<<line<<":"<<col<<" in "<<DBDLexer::tokStateName(tokState)
                    <<" '"<<c<<"' ("<<int(c)<<")\n";

new_state:
        switch(tokState) {
        case tokInit:
            assert(tok.value.empty());
            /*
            tokInit : '"' -> tokQuote
                    | [a-zA-Z0-9_\-+:.\[\]<>;] -> yymore -> tokBare
                    | '%' -> tokCode
                    | '#' -> tokComment
                    | [ \t\n\r] -> tokInit
                    | [(){},] -> tok(literal) -> tokInit
                    | EOI -> done
                    | . -> error
             */
            switch(c) {
            case '"': setLine(); tokState = tokQuote; break;
            case '%': setLine(); tokState = tokCode; break;
            case '#': setLine(); tokState = tokComment; break;
            case '(':
            case ')':
            case '{':
            case '}':
            case ',': setLine(); tokState = tokLit;
                tok.push_back(c);
                doToken(tokInit);
                break;
            case ' ':
            case '\t':
            case '\r':
            case '\n': tokState = tokInit; break;
            default:
                if(iswordchar(c)){
                    setLine(); tokState = tokBare;
                    tok.push_back(c);
                } else {
                    INVALID(c);
                }
            }
            break;

        case tokQuote:
            /*
            tokQuote : '\\' -> tokEsc
                     | [\n\r] -> error
                     | '"' -> tok(quote) -> tokInit
                     | . -> yymore -> tokQuote
                     | EOI -> error
             */
            switch(c) {
            case '\\': tokState = tokEsc; break;
            case '\n':
            case '\r':
                THROW("Missing closing quote");
            case '"':
                doToken(tokInit);
                break;
            default:
                tok.push_back(c);
                tokState = tokQuote;
            }
            break;

        case tokEsc:
            /*
            st_esc : 't' -> push '\t'
                   | 'r' -> push '\r'
                   | 'n' -> push '\n'
                   | . -> yymore -> st_quote
                   | EOI -> error
             */
            switch(c) {
            case 't': c='\t'; break;
            case 'r': c='\r'; break;
            case 'n': c='\n'; break;
            }
            tok.push_back(c);
            tokState = tokQuote;
            break;

        case tokBare:
            /*
            tokBare : [a-zA-Z0-9_\-+:.\[\]<>;] -> yymore -> tokBare
                    | . tok(bareword) -> jump tokInit
             */
            if(iswordchar(c)) {
                tok.push_back(c);
            } else {
                doToken(tokInit);
                goto new_state; // process this char in the initial state
            }

            break;

        case tokCode:
        case tokComment:
            /*
            tokCode : [\r\n]EOI -> tok(code) -> tokInit
                    | . -> yymore -> tokCode
            tokComment : [\r\n]EOI -> tok(comment) -> tokInit
                       | . -> yymore -> tokComment
             */
            switch(c) {
            case '\r':
            case '\n':
                doToken(tokInit);
                break;
            default:
                tok.push_back(c);
            }

            break;

        default:
            THROW("Invalid state");
        }
    }

    // EOI
    switch(tokState) {
    case tokInit:
        break;
    case tokCode:
    case tokComment:
    case tokBare:
        doToken(tokEOI);
        break;
    default:
        THROW("Unexpected end of input");
    }
    setLine();
    tokState = tokEOI;
    doToken(tokEOI);
}
