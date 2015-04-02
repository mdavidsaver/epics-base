
#include <string.h>

#include "dbDefs.h"
#include "dbdparser.h"

#include "dbLexRoutines.c"

enum DBDLoaderState {DBDBase, DBDRecord, DBDRecordType, DBDTypeField, DBDMenu, DBDBPT};

typedef struct {
    DBDCParserActions actions;
    enum DBDLoaderState state;
} DBDLoader;

static int dbd_parse_command(DBDCParserActions* selfraw,
                             const DBDToken* cmd, const DBDToken* arg)
{
    DBDLoader* self = CONTAINER(selfraw, DBDLoader, actions);
    const char *cmdstr = DBDToken_value(cmd),
               *argstr = DBDToken_value(arg);

    if(*cmdstr=='i' && strcmp(cmdstr, "include")==0) {
        dbIncludeNew(argstr);
        return;
    }

    if(self->state==DBDBase) {
        switch(*cmdstr) {
        case 'p':
            if(strcmp(cmdstr, "path")!=0) return 1;
            dbPathCmd(argstr);
            break;
        case 'a':
            if(strcmp(cmdstr, "addpath")!=0) return 1;
            dbAddPathCmd(argstr);
            break;
        default:
            return 1;
        }
        return 0;

    } else if(self->state==DBDBPT) {
        dbBreakItem(cmdstr);
        dbBreakItem(argstr);
        return 0;

    } else return 1;
}

static int parse_comment(DBDCParserActions* selfraw, const DBDToken* t)
{
    return 0; /* ignore */
}

static int parse_code(DBDCParserActions* selfraw, const DBDToken* t)
{
    DBDLoader* self = CONTAINER(selfraw, DBDLoader, actions);
    const char *cmdstr = DBDToken_value(t);

    if(self->state!=DBDRecordType) return 1;
    dbRecordtypeCdef(cmdstr);
    return 0;
}

static int parse_block(DBDCParserActions* selfraw,
                       const DBDToken* name, const DBDBlockArgs* args, int bodytofollow)
{
    DBDLoader* self = CONTAINER(selfraw, DBDLoader, actions);
    const char *namestr = DBDToken_value(name);
    size_t nargs = DBDBlockArgs_len(args);
    int expectbody = 0;

    if(self->state==DBDBase) {
        switch(namestr[0]) {
        case 'a':
        case 'b':
        case 'd':
        case 'f':
            break;
        case 'm':
            if(strcmp(namestr,"menu")!=0) return 1;
            if(nargs!=1) return 1;
            dbMenuHead(DBDBlockArgs_value(args, 0));
            expectbody = 1;
            self->state = DBDMenu;
            break;
        case 'g':
        case 'r':
            if(strcmp(namestr,"record")==0 || strcmp(namestr,"grecord")==0) {
                if(nargs!=2) return 1;

            } else if(strcmp(namestr,"recordtype")==0) {
                if(nargs!=1) return 1;
                dbRecordtypeHead(DBDBlockArgs_value(args, 0));
            }
        case 'v':
            break;
        default:
            return 1;
        }

    } else if(self->state==DBDMenu) {
        switch(namestr[0]) {
        case 'c':
            if(strcmp(namestr,"choice")!=0) return 1;
            if(nargs!=2) return 1;
            dbMenuChoice(DBDBlockArgs_value(args,0),
                         DBDBlockArgs_value(args,1));
            break;
        default:
            return 1;
        }
    } else return 1;

    if(expectbody && !bodytofollow) {
        parse_block_end(selfraw);
    }

    return 0;
}

static int parse_block_end(DBDCParserActions* selfraw)
{

}

