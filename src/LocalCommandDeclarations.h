#ifndef LOCALCOMMANDDECLARATIONS_H
#define LOCALCOMMANDDECLARATIONS_H

#include "Command.h"

extern int assembler(int argc, const char** argv, const Command &command);
extern int nuclassembler(int argc, const char** argv, const Command &command);
extern int hybridassembler(int argc, const char** argv, const Command &command);
extern int assembleresult(int argc, const char** argv, const Command &command);
extern int hybridassembleresults(int argc, const char** argv, const Command &command);
extern int filternoncoding(int argc, const char** argv, const Command &command);
extern int mergereads(int argc, const char** argv, const Command &command);
extern int findassemblystart(int argc, const char** argv, const Command &command);

#endif
