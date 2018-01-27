/*************************************************************************\
* Copyright (c) 2018 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <string.h>

#include <epicsStdio.h>
#define epicsExportSharedSymbols
#include <dbAccess.h>
#include <dbStaticLib.h>

void dbDumpDot(void)
{
    DBENTRY entry;
    long status;

    dbInitEntry(pdbbase, &entry);

    printf("digraph database {\n"
           "  node [shape=record];\n\n");

    for(status = dbFirstRecordType(&entry); !status; status = dbNextRecordType(&entry))
    {
        for(status = dbFirstRecord(&entry); !status; status = dbNextRecord(&entry))
        {
            printf("  \"%s\" [label = \"{<HEAD> [%s] \\\"%s\\\"", entry.precnode->recordname, entry.precordType->name, entry.precnode->recordname);

            // first pass through field list to fill in node
            for(status = dbFirstField(&entry, 1); !status; status = dbNextField(&entry, 1))
            {
                switch(entry.pflddes->field_type) {
                case DBF_INLINK:
                case DBF_OUTLINK:
                case DBF_FWDLINK: {
                    DBLINK *plink = entry.pfield;
                    if(plink->type == CONSTANT && !plink->value.constantStr)
                        continue;
                    printf("|<%s> %s = \\\"%s\\\"", entry.pflddes->name, entry.pflddes->name, dbGetString(&entry));
                }
                    break;
                default:
                    if(dbIsDefaultValue(&entry))
                        continue;
                    printf("|<%s> %s = \\\"%s\\\"", entry.pflddes->name, entry.pflddes->name, dbGetString(&entry));
                }
            }

            printf("}\"];\n");

            // second pass through field list to emit edges for links
            for(status = dbFirstField(&entry, 1); !status; status = dbNextField(&entry, 1))
            {
                DBLINK *plink;
                unsigned wrote = 0;

                switch(entry.pflddes->field_type) {
                case DBF_INLINK:
                case DBF_OUTLINK:
                case DBF_FWDLINK:
                    plink = entry.pfield;
                    if(plink->type == CONSTANT && !plink->value.constantStr)
                        continue;
                    break;
                default:
                    continue;
                }

                if(plink->type == DB_LINK) {
                    DBADDR *target = plink->value.pv_link.pvt;
                    DBENTRY tentry;

                    dbInitEntryFromAddr(target, &tentry);

                    wrote = 1;
                    printf("  \"%s\":%s -> \"%s\":HEAD [",
                           entry.precnode->recordname, entry.pflddes->name,
                           tentry.precnode->recordname);

                    dbFinishEntry(&tentry);

                } else {
                    printf("  # skip %s.%s type=%u\n", entry.precnode->recordname, entry.pflddes->name, plink->type);
                }

                if(!wrote)
                    continue;

                switch(entry.pflddes->field_type) {
                case DBF_INLINK:
                    if(plink->value.pv_link.pvlMask&(pvlOptCP|pvlOptCPP|pvlOptPP)) {
                        printf("arrowhead=invinv");
                    } else {
                        printf("arrowhead=invoinv");
                    }
                    break;
                case DBF_OUTLINK:
                    if(plink->value.pv_link.pvlMask&(pvlOptCP|pvlOptCPP|pvlOptPP)) {
                        printf("arrowhead=normalnormal");
                    } else {
                        printf("arrowhead=normalonormal");
                    }
                    break;
                case DBF_FWDLINK:
                    printf("arrowhead=empty,arrowtail=normal");
                    break;
                default:
                    break;
                }

                printf("];\n");
            }
        }
    }

    printf("}\n");

    dbFinishEntry(&entry);
}
