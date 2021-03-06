#include <string>
#include <vector>
#include <sstream>
#include <sys/time.h>

#include "LocalParameters.h"
#include "DistanceCalculator.h"
#include "Matcher.h"
#include "DBReader.h"
#include "DBWriter.h"
#include "Debug.h"
#include "Util.h"
#include "MathUtil.h"
#include <limits>
#include <cstdint>
#include <queue>
#include <mmseqs/src/commons/NucleotideMatrix.h>

#ifdef OPENMP
#include <omp.h>
#endif

class CompareResultBySeqId {
public:
    bool operator() (const Matcher::result_t & r1,const Matcher::result_t & r2) {
        if(r1.seqId < r2.seqId )
            return true;
        if(r2.seqId < r1.seqId )
            return false;
        /*  int seqLen1 = r1.qEndPos - r1.qStartPos;
          int seqLen2 = r2.qEndPos - r2.qStartPos;
          if(seqLen1 < seqLen2)
              return true;
          if(seqLen2 < seqLen1 )
              return false;*/
        return false;
    }
};

typedef std::priority_queue<Matcher::result_t, std::vector<Matcher::result_t> , CompareResultBySeqId> QueueBySeqId;
Matcher::result_t selectBestFragmentToExtend(QueueBySeqId &alignments,
                                             unsigned int queryKey) {
    // results are ordered by score
    while (alignments.empty() == false){
        Matcher::result_t res = alignments.top();
        alignments.pop();
        size_t dbKey = res.dbKey;
        const bool notRightStartAndLeftStart = !(res.dbStartPos == 0 && res.qStartPos == 0);
        const bool rightStart = res.dbStartPos == 0 && (res.dbEndPos != res.dbLen-1);
        const bool leftStart = res.qStartPos == 0   && (res.qEndPos != res.qLen-1);
        const bool isNotIdentity = (dbKey != queryKey);
        if ((rightStart || leftStart) && notRightStartAndLeftStart && isNotIdentity){
            return res;
        }
    }
    return Matcher::result_t(UINT_MAX,0,0,0,0,0,0,0,0,0,0,0,0,"");
}


int dohybridassembleresult(LocalParameters &par) {
    DBReader<unsigned int> *nuclSequenceDbr = new DBReader<unsigned int>(par.db1.c_str(), par.db1Index.c_str());
    nuclSequenceDbr->open(DBReader<unsigned int>::NOSORT);

    DBReader<unsigned int> *aaSequenceDbr = new DBReader<unsigned int>(par.db2.c_str(), par.db2Index.c_str());
    aaSequenceDbr->open(DBReader<unsigned int>::NOSORT);

    DBReader<unsigned int> * nuclAlnReader = new DBReader<unsigned int>(par.db3.c_str(), par.db3Index.c_str());
    nuclAlnReader->open(DBReader<unsigned int>::NOSORT);

    DBWriter nuclResultWriter(par.db4.c_str(), par.db4Index.c_str(), par.threads);
    nuclResultWriter.open();

    DBWriter aaResultWriter(par.db5.c_str(), par.db5Index.c_str(), par.threads);
    aaResultWriter.open();

    NucleotideMatrix subMat(par.scoringMatrixFile.c_str(), 1.0f, 0.0f);
    SubstitutionMatrix::FastMatrix fastMatrix = SubstitutionMatrix::createAsciiSubMat(subMat);

    unsigned char * wasExtended = new unsigned char[nuclSequenceDbr->getSize()];
    std::fill(wasExtended, wasExtended+nuclSequenceDbr->getSize(), 0);

#pragma omp parallel
    {
        unsigned int thread_idx = 0;
#ifdef OPENMP
        thread_idx = (unsigned int) omp_get_thread_num();
#endif

        #pragma omp for schedule(dynamic, 100)
        for (size_t id = 0; id < nuclSequenceDbr->getSize(); id++) {
            Debug::printProgress(id);

            unsigned int queryId = nuclSequenceDbr->getDbKey(id);

            char *nuclQuerySeq = nuclSequenceDbr->getData(id);
            unsigned int nuclQuerySeqLen = nuclSequenceDbr->getSeqLens(id) - 2;
            char *aaQuerySeq = aaSequenceDbr->getData(id);
            unsigned int aaQuerySeqLen = aaSequenceDbr->getSeqLens(id) - 2;

            unsigned int nuclLeftQueryOffset = 0;
            unsigned int nuclRightQueryOffset = 0;
            std::string nuclQuery(nuclQuerySeq, nuclQuerySeqLen); // no /n/0
            std::string aaQuery(aaQuerySeq, aaQuerySeqLen); // no /n/0

            char *nuclAlnData = nuclAlnReader->getDataByDBKey(queryId);

            std::vector<Matcher::result_t> nuclAlignments = Matcher::readAlignmentResults(nuclAlnData, true);

            QueueBySeqId alnQueue;
            bool queryCouldBeExtended = false;
            while(nuclAlignments.size() > 1){
                bool queryCouldBeExtendedLeft = false;
                bool queryCouldBeExtendedRight = false;
                for (size_t alnIdx = 0; alnIdx < nuclAlignments.size(); alnIdx++) {
                    alnQueue.push(nuclAlignments[alnIdx]);
                    if (nuclAlignments.size() > 1) {
                        size_t id = nuclSequenceDbr->getId(nuclAlignments[alnIdx].dbKey);
                        __sync_or_and_fetch(&wasExtended[id],
                                            static_cast<unsigned char>(0x40));
                    }
                }
                std::vector<Matcher::result_t> tmpNuclAlignments;

                Matcher::result_t nuclBesttHitToExtend;

                while ((nuclBesttHitToExtend = selectBestFragmentToExtend(alnQueue, queryId)).dbKey != UINT_MAX) {
                    nuclQuerySeqLen = nuclQuery.size();
                    nuclQuerySeq = (char *) nuclQuery.c_str();

//                nuclQuerySeq.mapSequence(id, queryKey, nuclQuery.c_str());
                    unsigned int targetId = nuclSequenceDbr->getId(nuclBesttHitToExtend.dbKey);
                    if (targetId == UINT_MAX) {
                        Debug(Debug::ERROR) << "Could not find targetId  " << nuclBesttHitToExtend.dbKey
                                            << " in database " << nuclSequenceDbr->getDataFileName() << "\n";
                        EXIT(EXIT_FAILURE);
                    }

                    char *nuclTargetSeq = nuclSequenceDbr->getData(targetId);
                    unsigned int nuclTargetSeqLen = nuclSequenceDbr->getSeqLens(targetId) - 2;
                    //TODO is this right?
                    char *aaTargetSeq = aaSequenceDbr->getData(targetId);

                    // check if alignment still make sense (can extend the nuclQuery)
                    if (nuclBesttHitToExtend.dbStartPos == 0) {
                        if ((nuclTargetSeqLen - (nuclBesttHitToExtend.dbEndPos + 1)) <= nuclRightQueryOffset) {
                            continue;
                        }
                    } else if (nuclBesttHitToExtend.qStartPos == 0) {
                        if (nuclBesttHitToExtend.dbStartPos <= nuclLeftQueryOffset) {
                            continue;
                        }
                    }
                    __sync_or_and_fetch(&wasExtended[targetId], static_cast<unsigned char>(0x10));
                    int qStartPos, qEndPos, nuclDbStartPos, nuclDbEndPos;
                    int diagonal = (nuclLeftQueryOffset + nuclBesttHitToExtend.qStartPos) - nuclBesttHitToExtend.dbStartPos;
                    int dist = std::max(abs(diagonal), 0);
                    if (diagonal >= 0) {
//                    nuclTargetSeq.mapSequence(targetId, nuclBesttHitToExtend.dbKey, dbSeq);
                        size_t diagonalLen = std::min(nuclTargetSeqLen, nuclQuerySeqLen - abs(diagonal));
                        DistanceCalculator::LocalAlignment alignment = DistanceCalculator::computeSubstitutionStartEndDistance(
                                nuclQuerySeq + abs(diagonal),
                                nuclTargetSeq, diagonalLen, fastMatrix.matrix);
                        qStartPos = alignment.startPos + dist;
                        qEndPos = alignment.endPos + dist;
                        nuclDbStartPos = alignment.startPos;
                        nuclDbEndPos = alignment.endPos;
                    } else {
                        size_t diagonalLen = std::min(nuclTargetSeqLen - abs(diagonal), nuclQuerySeqLen);
                        DistanceCalculator::LocalAlignment alignment = DistanceCalculator::computeSubstitutionStartEndDistance(
                                nuclQuerySeq,
                                nuclTargetSeq + abs(diagonal),
                                diagonalLen, fastMatrix.matrix);
                        qStartPos = alignment.startPos;
                        qEndPos = alignment.endPos;
                        nuclDbStartPos = alignment.startPos + dist;
                        nuclDbEndPos = alignment.endPos + dist;
                    }

                    if (nuclDbStartPos == 0 && qEndPos == (nuclQuerySeqLen - 1) ) {
                        if(queryCouldBeExtendedRight == true) {
                            tmpNuclAlignments.push_back(nuclBesttHitToExtend);
                            continue;
                        }
                        size_t nuclDbFragLen = (nuclTargetSeqLen - nuclDbEndPos) - 1; // -1 get not aligned element
                        size_t aaDbFragLen = (nuclTargetSeqLen/3 - nuclDbEndPos/3) - 1; // -1 get not aligned element

                        std::string fragment = std::string(nuclTargetSeq + nuclDbEndPos + 1, nuclDbFragLen);
                        std::string aaFragment = std::string(aaTargetSeq + nuclDbEndPos/3 + 1, aaDbFragLen);

                        if (fragment.size() + nuclQuery.size() >= par.maxSeqLen) {
                            Debug(Debug::WARNING) << "Sequence too long in nuclQuery id: " << queryId << ". "
                                    "Max length allowed would is " << par.maxSeqLen << "\n";
                            break;
                        }
                        //update that dbKey was used in assembly
                        __sync_or_and_fetch(&wasExtended[targetId], static_cast<unsigned char>(0x80));
                        queryCouldBeExtendedRight = true;
                        nuclQuery += fragment;
                        aaQuery += aaFragment;

                        nuclRightQueryOffset += nuclDbFragLen;

                    } else if (qStartPos == 0 && nuclDbEndPos == (nuclTargetSeqLen - 1)) {
                        if (queryCouldBeExtendedLeft == true) {
                            tmpNuclAlignments.push_back(nuclBesttHitToExtend);
                            continue;
                        }
                        std::string fragment = std::string(nuclTargetSeq, nuclDbStartPos); // +1 get not aligned element
                        std::string aaFragment = std::string(aaTargetSeq, nuclDbStartPos/3); // +1 get not aligned element

                        if (fragment.size() + nuclQuery.size() >= par.maxSeqLen) {
                            Debug(Debug::WARNING) << "Sequence too long in nuclQuery id: " << queryId << ". "
                                    "Max length allowed would is " << par.maxSeqLen << "\n";
                            break;
                        }
                        // update that dbKey was used in assembly
                        __sync_or_and_fetch(&wasExtended[targetId], static_cast<unsigned char>(0x80));
                        queryCouldBeExtendedLeft = true;
                        nuclQuery = fragment + nuclQuery;
                        aaQuery = aaFragment + aaQuery;
                        nuclLeftQueryOffset += nuclDbStartPos;
                    }

                }
                if (queryCouldBeExtendedRight || queryCouldBeExtendedLeft){
                    queryCouldBeExtended = true;
                }
                nuclAlignments.clear();
                nuclQuerySeq = (char *) nuclQuery.c_str();
                break;
                for(size_t alnIdx = 0; alnIdx < tmpNuclAlignments.size(); alnIdx++){
                    int idCnt = 0;
                    int qStartPos = tmpNuclAlignments[alnIdx].qStartPos;
                    int qEndPos = tmpNuclAlignments[alnIdx].qEndPos;
                    int dbStartPos = tmpNuclAlignments[alnIdx].dbStartPos;

                    int diagonal = (nuclLeftQueryOffset + nuclBesttHitToExtend.qStartPos) - nuclBesttHitToExtend.dbStartPos;
                    int dist = std::max(abs(diagonal), 0);
                    if (diagonal >= 0) {
                        qStartPos+=dist;
                        qEndPos+=dist;
                    }else{
                        dbStartPos+=dist;
                    }
                    unsigned int targetId = nuclSequenceDbr->getId(tmpNuclAlignments[alnIdx].dbKey);
                    char *nuclTargetSeq = nuclSequenceDbr->getData(targetId);
                    for(int i = qStartPos; i < qEndPos; i++){
                        idCnt += (nuclQuerySeq[i] == nuclTargetSeq[dbStartPos+(i-qStartPos)]) ? 1 : 0;
                    }
                    float seqId =  static_cast<float>(idCnt) / (static_cast<float>(qEndPos) - static_cast<float>(qStartPos));
                    tmpNuclAlignments[alnIdx].seqId = seqId;
                    if(seqId >= par.seqIdThr){
                        nuclAlignments.push_back(tmpNuclAlignments[alnIdx]);
                    }
                }
            }
            if (queryCouldBeExtended == true) {
                nuclQuery.push_back('\n');
                aaQuery.push_back('\n');
                __sync_or_and_fetch(&wasExtended[id], static_cast<unsigned char>(0x20));
                nuclResultWriter.writeData(nuclQuery.c_str(), nuclQuery.size(), queryId, thread_idx);
                aaResultWriter.writeData(aaQuery.c_str(), aaQuery.size(), queryId, thread_idx);
            }
        }
    } // end parallel

// add sequences that are not yet assembled
#pragma omp parallel for schedule(dynamic, 10000)
    for (size_t id = 0; id < nuclSequenceDbr->getSize(); id++) {
        unsigned int thread_idx = 0;
#ifdef OPENMP
        thread_idx = (unsigned int) omp_get_thread_num();
#endif
        //   bool couldExtend =  (wasExtended[id] & 0x10);
        bool isNotContig =  !(wasExtended[id] & 0x20);
//        bool wasNotUsed =  !(wasExtended[id] & 0x40);
//        bool wasNotExtended =  !(wasExtended[id] & 0x80);
        //    bool wasUsed    =  (wasExtended[id] & 0x40);
        //if(isNotContig && wasNotExtended ){
        if (isNotContig){
            char *querySeqData = nuclSequenceDbr->getData(id);
            unsigned int queryLen = nuclSequenceDbr->getSeqLens(id) - 1; //skip null byte
            nuclResultWriter.writeData(querySeqData, queryLen, nuclSequenceDbr->getDbKey(id), thread_idx);
            char *queryAASeqData = aaSequenceDbr->getData(id);
            unsigned int queryAALen = aaSequenceDbr->getSeqLens(id) - 1; //skip null byte
            aaResultWriter.writeData(queryAASeqData, queryAALen, aaSequenceDbr->getDbKey(id), thread_idx);
        }
    }

    // cleanup
    aaResultWriter.close(aaSequenceDbr->getDbtype());
    nuclResultWriter.close(nuclSequenceDbr->getDbtype());
    nuclAlnReader->close();
    delete [] wasExtended;
    delete nuclAlnReader;

    delete [] fastMatrix.matrix;
    delete [] fastMatrix.matrixData;
    aaSequenceDbr->close();
    delete aaSequenceDbr;
    nuclSequenceDbr->close();
    delete nuclSequenceDbr;
    Debug(Debug::INFO) << "\nDone.\n";

    return EXIT_SUCCESS;
}

int hybridassembleresults(int argc, const char **argv, const Command& command) {
    LocalParameters& par = LocalParameters::getLocalInstance();
    par.parseParameters(argc, argv, command, 5);

    MMseqsMPI::init(argc, argv);

    // never allow deletions
    par.allowDeletion = false;
    Debug(Debug::INFO) << "Compute assembly.\n";
    struct timeval start, end;
    gettimeofday(&start, NULL);

    int retCode = dohybridassembleresult(par);

    gettimeofday(&end, NULL);
    time_t sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "Time for processing: " << (sec / 3600) << " h " << (sec % 3600 / 60) << " m " << (sec % 60) << "s\n";

    return retCode;
}

