#include "Command.h"
#include "CommandDeclarations.h"
#include "LocalCommandDeclarations.h"
#include "LocalParameters.h"

const char* binary_name = "plass";
const char* tool_name = "Plass";
const char* tool_introduction = "Protein Level Assembler.";
const char* main_author = "Martin Steinegger (martin.steinegger@mpibpc.mpg.de)";
const char* show_extended_help = NULL;
const char* show_bash_info = NULL;
LocalParameters& par = LocalParameters::getLocalInstance();

std::vector<struct Command> commands = {
        // Main tools  (for non-experts)
        {"createdb",             createdb,             &par.createdb,             COMMAND_HIDDEN,
                "Convert protein sequence set in a FASTA file to MMseqs sequence DB format",
                "converts a protein sequence flat/gzipped FASTA or FASTQ file to the MMseqs sequence DB format. This format is needed as input to mmseqs search, cluster and many other tools.",
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:fastaFile1[.gz]> ... <i:fastaFileN[.gz]> <o:sequenceDB>",
                CITATION_MMSEQS2},
        {"assemble",             assembler,            &par.assemblerworkflow,    COMMAND_MAIN,
                "Assemble protein sequences by iterative greedy overlap assembly.",
                "Extends sequence to the left and right using ungapped alignments.",
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de> ",
                "<i:fast(a|q)File[.gz]> | <i:fastqFile1_1[.gz] ... <i:fastqFileN_1[.gz] <i:fastqFile1_2[.gz] ... <i:fastqFileN_2[.gz]> <o:fastaFile> <tmpDir>",
                CITATION_PLASS },
        {"nuclassemble",             nuclassembler,            &par.assemblerworkflow,    COMMAND_MAIN,
                "Assemble nucleotide sequences  by iterative greedy overlap assembly. (experimental)",
                "Extends sequence to the left and right using ungapped alignments.",
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de> ",
                "<i:fast(a|q)File[.gz]> | <i:fastqFile1_1[.gz] ... <i:fastqFileN_1[.gz] <i:fastqFile1_2[.gz] ... <i:fastqFileN_2[.gz]> <o:fastaFile> <tmpDir>",
                CITATION_PLASS},
        {"hybridassemble",             hybridassembler,            &par.assemblerworkflow,    COMMAND_HIDDEN,
                "Assemble protein sequences in linear time using protein and nucleotide information.",
                "Assemble protein sequences in linear time using protein and nucleotide information.",
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de> ",
                "<i:sequenceDB> <o:repSeqDb> <tmpDir>",
                CITATION_MMSEQS2 | CITATION_LINCLUST},
        {"proteinaln2nucl",          proteinaln2nucl,          &par.onlythreads,            COMMAND_HIDDEN,
                "Map protein alignment to nucleotide alignment",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de> ",
                "<i:queryDB> <i:targetDB> <i:alnDB> <o:alnDB>",
                CITATION_MMSEQS2},
        {"kmermatcher",          kmermatcher,          &par.kmermatcher,          COMMAND_HIDDEN,
                "Finds exact $k$-mers matches between sequences",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de> ",
                "<i:sequenceDB> <o:prefDB>",
                CITATION_MMSEQS2},
        {"extractorfs",          extractorfs,          &par.extractorfs,          COMMAND_HIDDEN,
                "Extract open reading frames from all six frames from nucleotide sequence DB",
                NULL,
                "Milot Mirdita <milot@mirdita.de>",
                "<i:sequenceDB> <o:sequenceDB>",
                CITATION_MMSEQS2},
        {"translatenucs",        translatenucs,        &par.translatenucs,        COMMAND_HIDDEN,
                "Translate nucleotide sequence DB into protein sequence DB",
                NULL,
                "Milot Mirdita <milot@mirdita.de>",
                "<i:sequenceDB> <o:sequenceDB>",
                CITATION_MMSEQS2},
        {"swapresults",          swapresults,          &par.swapresult,           COMMAND_HIDDEN,
                "Reformat prefilter/alignment/cluster DB as if target DB had been searched through query DB",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de> & Clovis Galiez",
                "<i:queryDB> <i:targetDB> <i:resultDB> <o:resultDB>",
                CITATION_MMSEQS2},
        {"mergedbs",             mergedbs,             &par.mergedbs,             COMMAND_HIDDEN,
                "Merge multiple DBs into a single DB, based on IDs (names) of entries",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:sequenceDB> <o:resultDB> <i:resultDB1> ... <i:resultDBn>",
                CITATION_MMSEQS2},
        {"splitdb",              splitdb,              &par.splitdb,              COMMAND_HIDDEN,
                "Split a mmseqs DB into multiple DBs",
                NULL,
                "Milot Mirdita <milot@mirdita.de>",
                "<i:sequenceDB> <o:sequenceDB_1..N>",
                CITATION_MMSEQS2},
        {"subtractdbs",          subtractdbs,          &par.subtractdbs,          COMMAND_HIDDEN,
                "Generate a DB with entries of first DB not occurring in second DB",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:resultDBLeft> <i:resultDBRight> <o:resultDB>",
                CITATION_MMSEQS2},
        {"filterdb",             filterdb,             &par.filterDb,             COMMAND_HIDDEN,
                "Filter a DB by conditioning (regex, numerical, ...) on one of its whitespace-separated columns",
                NULL,
                "Clovis Galiez & Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:resultDB> <o:resultDB>",
                CITATION_MMSEQS2},
        {"createsubdb",          createsubdb,          &par.onlyverbosity,        COMMAND_HIDDEN,
                "Create a subset of a DB from a file of IDs of entries",
                NULL,
                "Milot Mirdita <milot@mirdita.de>",
                "<i:subsetFile or DB> <i:resultDB> <o:resultDB>",
                CITATION_MMSEQS2},
        {"offsetalignment",      offsetalignment,      &par.onlythreads,          COMMAND_HIDDEN,
                "Offset alignemnt by orf start position.",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:queryDB> <i:targetDB> <i:alnDB> <o:alnDB>",
                CITATION_MMSEQS2},
        {"tsv2db",               tsv2db,               &par.tsv2db,               COMMAND_HIDDEN,
                "Turns a TSV file into a MMseqs database",
                NULL,
                "Milot Mirdita <milot@mirdita.de>",
                "<i:tsvFile> <o:resultDB>",
                CITATION_MMSEQS2
        },
        {"assembleresults",      assembleresult,       &par.assembleresults,      COMMAND_HIDDEN,
                "Extending representative sequence to the left and right side using ungapped alignments.",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:sequenceDB> <i:alnResult> <o:reprSeqDB>",
                CITATION_MMSEQS2},
        {"hybridassembleresults",      hybridassembleresults,       &par.assembleresults,      COMMAND_HIDDEN,
                "Extending representative sequence to the left and right side using ungapped alignments.",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:nuclSequenceDB> <i:aaSequenceDB> <i:nuclAlnResult> <o:nuclAssembly> <o:aaAssembly>",
                CITATION_MMSEQS2},
        // Special-purpose utilities
        {"rescorediagonal",      rescorediagonal,      &par.rescorediagonal,      COMMAND_HIDDEN,
                "Compute sequence identity for diagonal",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:queryDB> <i:targetDB> <i:prefilterDB> <o:resultDB>",
                CITATION_MMSEQS2},
        {"concatdbs",            concatdbs,            &par.concatdbs,            COMMAND_HIDDEN,
                "Concatenate two DBs, giving new IDs to entries from second input DB",
                NULL,
                "Clovis Galiez & Martin Steinegger (martin.steinegger@mpibpc.mpg.de)",
                "<i:resultDB> <i:resultDB> <o:resultDB>",
                CITATION_MMSEQS2},
        {"convert2fasta",        convert2fasta,        &par.convert2fasta,        COMMAND_HIDDEN,
                "Convert sequence DB to FASTA format",
                NULL,
                "Milot Mirdita <milot@mirdita.de>",
                "<i:sequenceDB> <o:fastaFile>",
                CITATION_MMSEQS2},
        {"prefixid",             prefixid,             &par.prefixid,             COMMAND_HIDDEN,
                "For each entry in a DB prepend the entry ID to the entry itself",
                NULL,
                "Milot Mirdita <milot@mirdita.de>",
                "<i:resultDB> <o:resultDB>",
                CITATION_MMSEQS2},

        {"findassemblystart",    findassemblystart,    &par.onlythreads,          COMMAND_HIDDEN,
                "Compute consensus based new * stop before M amino acid",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:sequenceDB> <i:alignmentDB> <o:sequenceDB>",
                CITATION_MMSEQS2},
        {"filternoncoding",      filternoncoding,      &par.onlythreads,          COMMAND_HIDDEN,
                "Filter non-coding protein sequences",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:sequenceDB> <o:sequenceDB>",
                CITATION_MMSEQS2},

        {"mergereads",      mergereads,      &par.onlythreads,          COMMAND_HIDDEN,
                "Merge paired-end reads from FASTQ file (powered by FLASH)",
                NULL,
                "Martin Steinegger <martin.steinegger@mpibpc.mpg.de>",
                "<i:fastq> <o:sequenceDB>",
                CITATION_MMSEQS2},
        {"shellcompletion",      shellcompletion,      &par.empty,                COMMAND_HIDDEN,
                "",
                NULL,
                "",
                "",
                CITATION_MMSEQS2},
};
