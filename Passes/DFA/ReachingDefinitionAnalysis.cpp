//
//  ReachingDefinitionAnalysis.cpp
//  
//
//  Created by hanminghao on 18/2/20.
//
//
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "231DFA.h"
#include <deque>
#include <map>
#include <utility>
#include <vector>
#include <set>
using namespace llvm;
using namespace std;
class ReachingInfo:public Info
{
private:
    set<unsigned> infolist;
public:
    ReachingInfo() {}
    ReachingInfo(unsigned index)
    {
        set<unsigned> informationlist={index};
        this->infolist = informationlist;
    }
    ReachingInfo(set<unsigned> informationlist)
    {
        this->infolist = informationlist;
    }
    set<unsigned> & getinfolist()
    {
        return this->infolist;
    }
    void setinfolist(set<unsigned> information)
    {
        this->infolist = information;
    }
     void print()
    {
        for (auto it = infolist.begin(); it != infolist.end(); ++it) 
        {
            errs() << *it << '|';
        }
        errs()<< '\n';
    }
    static bool equals(ReachingInfo * info1,  ReachingInfo* info2)
    {
        return (info1->getinfolist()==info2->getinfolist());
    }
    static ReachingInfo* join(ReachingInfo * info1, ReachingInfo * info2, ReachingInfo * result)
    {
        set<unsigned> resultinfo= info1->getinfolist();
        set<unsigned> infolist2 = info2->getinfolist();
        resultinfo.insert(infolist2.begin(),infolist2.end());
        result->setinfolist(resultinfo);
        return nullptr;
    }
};
class ReachingDefinitionAnalysis: public DataFlowAnalysis<ReachingInfo, true>
{
private:
    typedef pair<unsigned, unsigned> Edge;
    map<Edge,ReachingInfo *> EdgeToInfo;
public:
    ReachingDefinitionAnalysis(ReachingInfo & bottom, ReachingInfo & initialState) :
    DataFlowAnalysis(bottom, initialState) {}
    void flowfunction(Instruction * I, std::vector<unsigned> & IncomingEdges,std::vector<unsigned> & OutgoingEdges,std::vector<ReachingInfo *> & Infos)
    {
        string opname = I->getOpcodeName();
        map<unsigned, Instruction *> IndexToInstr=getIndexToInstr();
        map<Instruction *, unsigned> InstrToIndex=getInstrToIndex();
        unsigned index=InstrToIndex[I];
        map<Edge,ReachingInfo *> EdgeToInfo=getEdgeToInfo();
        ReachingInfo * outInfo =new ReachingInfo();
        //calculate the incoming edges
        for (unsigned i=0;i<IncomingEdges.size();i++)
        {
            Edge incomingEdge = make_pair(IncomingEdges[i], index);
            ReachingInfo::join(outInfo, EdgeToInfo[incomingEdge], outInfo);
        }
        // join the information for category 1 instructions
        if (I->isBinaryOp()||opname=="alloca"||opname=="load"||opname=="select"||opname=="icmp"||opname=="fcmp"||opname=="getelementptr")
        {
             ReachingInfo::join(outInfo, new ReachingInfo(index), outInfo);
        }
        // process the phi nodes together
        else if (opname=="phi")
        {
            Instruction * firstNonPhi = I->getParent()->getFirstNonPHI();
            unsigned firstNonPhiIdx =InstrToIndex[firstNonPhi];
            set<unsigned> allphi;
            for (unsigned i = index; i < firstNonPhiIdx; i++)
            {
                allphi.insert(i);
            }
            ReachingInfo::join(outInfo, new ReachingInfo(allphi), outInfo);
        }
        // save the outinfo back 
        for (unsigned i = 0; i < OutgoingEdges.size(); i++)
        {
            Infos.push_back(outInfo);
        }
    }
};
namespace
{
    struct ReachingDefinitionAnalysisPass : public FunctionPass
    {
        static char ID;
        ReachingDefinitionAnalysisPass() : FunctionPass(ID) {}
        bool runOnFunction(Function &F) override
        {
            ReachingInfo bottom;
            ReachingInfo initialState;
            ReachingDefinitionAnalysis * myrda = new ReachingDefinitionAnalysis(bottom, initialState);
            myrda->runWorklistAlgorithm(&F);
            myrda->print();
            return false;
        }
    };
}
char ReachingDefinitionAnalysisPass::ID = 0;
static RegisterPass<ReachingDefinitionAnalysisPass> X ("cse231-reaching", "reaching definition analysis",false ,false);
