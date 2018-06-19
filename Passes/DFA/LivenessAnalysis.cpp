//
//  LivenessAnalysis.cpp/Users/apple/Documents/CSE231_Project/Passes/DFA/LivenessAnalysis.cpp
//  
//
//  Created by hanminghao on 18/3/19.
//
//
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "231DFA.h"
#include <deque>
#include <map>
#include <utility>
#include <vector>
#include <set>
using namespace llvm;
using namespace std;
class LivenessInfo:public Info
{
private:
    set<unsigned> infolist;
public:
    LivenessInfo() {}
    LivenessInfo(unsigned index)
    {
        set<unsigned> informationlist={index};
        this->infolist = informationlist;
    }
    LivenessInfo(set<unsigned> informationlist)
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
    static bool equals(LivenessInfo * info1,  LivenessInfo* info2)
    {
        return (info1->getinfolist()==info2->getinfolist());
    }
    static LivenessInfo* join(LivenessInfo * info1, LivenessInfo * info2, LivenessInfo * result)
    {
        set<unsigned> resultinfo= info1->getinfolist();
        set<unsigned> infolist2 = info2->getinfolist();
        resultinfo.insert(infolist2.begin(),infolist2.end());
        result->setinfolist(resultinfo);
        return nullptr;
    }
     static LivenessInfo* removeinfo(LivenessInfo * info1, LivenessInfo * info2, LivenessInfo * result)
    {
        set<unsigned> resultinfo= info1->getinfolist();
        set<unsigned> infolist2 = info2->getinfolist();
        for (auto it = infolist2.begin(); it != infolist2.end(); ++it) 
        {
               resultinfo.erase(*it);
        }
        result->setinfolist(resultinfo);
        return nullptr;
    }

};
class LivenessAnalysis : public DataFlowAnalysis<LivenessInfo, false>
{
private:
    typedef pair<unsigned, unsigned> Edge;
    map<Edge,LivenessInfo *> EdgeToInfo;
public:
    LivenessAnalysis (LivenessInfo & bottom, LivenessInfo & initialState) :
    DataFlowAnalysis(bottom, initialState) {}
    void flowfunction(Instruction * I, std::vector<unsigned> & IncomingEdges,std::vector<unsigned> & OutgoingEdges,std::vector<LivenessInfo *> & Infos)
    {
        string opname = I->getOpcodeName();
        map<unsigned, Instruction *> IndexToInstr=getIndexToInstr();
        map<Instruction *, unsigned> InstrToIndex=getInstrToIndex();
        set<unsigned> operands;
        unsigned index=InstrToIndex[I];
        map<Edge,LivenessInfo *> EdgeToInfo=getEdgeToInfo();
        LivenessInfo * outInfo =new LivenessInfo();
        //caclculate the operands set
        for (unsigned i = 0; i < I->getNumOperands(); i++) 
        {
                Instruction *operandsinst= (Instruction *) I->getOperand(i);
                int countoperandinst=InstrToIndex.count(operandsinst);
                if (countoperandinst!=0)
                {
                    operands.insert(InstrToIndex[operandsinst]);
                }  
        }
        //calculate the incoming edges
        for (unsigned i=0;i<IncomingEdges.size();i++)
        {
            Edge incomingEdge = make_pair(IncomingEdges[i], index);
            LivenessInfo::join(outInfo, EdgeToInfo[incomingEdge], outInfo);

        }
        // join the information for category 1 instructions
        if (I->isBinaryOp()||opname=="alloca"||opname=="load"||opname=="select"||opname=="icmp"||opname=="fcmp"||opname=="getelementptr")
        { 
            // join the information for operands
            LivenessInfo::join(outInfo, new LivenessInfo(operands), outInfo);
            // remove the informaton  for index
            LivenessInfo::removeinfo(outInfo, new LivenessInfo(index), outInfo);
            // save the outinfo back 
        for (unsigned i = 0; i < OutgoingEdges.size(); i++)
        {
            Infos.push_back(outInfo);
        }
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
            LivenessInfo::removeinfo(outInfo, new LivenessInfo(allphi), outInfo);
            //  handle k output 
            for (unsigned k=0;k<OutgoingEdges.size();k++)
            {
                set <unsigned> operand_k;
                LivenessInfo * outInfo_k =new LivenessInfo();
                for (unsigned i = index; i < firstNonPhiIdx; i++)
                {
                    Instruction *instr = IndexToInstr[i];
                    for (unsigned j = 0; j < instr->getNumOperands(); j++) 
                    {
                        Instruction *operandsinst= (Instruction *)instr->getOperand(j);
                        int countoperandinst=InstrToIndex.count(operandsinst);
                        if (countoperandinst!=0)
                        {
                            BasicBlock *label_k=operandsinst->getParent();
                            BasicBlock *label_ij=IndexToInstr[OutgoingEdges[k]]->getParent();
                            if (label_k==label_ij)
                            {
                                operand_k.insert(InstrToIndex[operandsinst]);
                                 break;
                            }
                        }  
                    }
                    
                }

             LivenessInfo::join(outInfo, new LivenessInfo(operand_k), outInfo_k);
             Infos.push_back(outInfo_k);
            }
        }
        //   
        else 
        {
            // join the information for operands
            LivenessInfo::join(outInfo, new LivenessInfo(operands), outInfo);
            // save the outinfo back 
        for (unsigned i = 0; i < OutgoingEdges.size(); i++)
        {
            Infos.push_back(outInfo);
        }
        }

        
    }
};
namespace
{
    struct LivenessAnalysisPass : public FunctionPass
    {
        static char ID;
        LivenessAnalysisPass() : FunctionPass(ID) {}
        bool runOnFunction(Function &F) override
        {
            LivenessInfo bottom;
            LivenessInfo initialState;
            LivenessAnalysis  * myla = new LivenessAnalysis(bottom, initialState);
            myla->runWorklistAlgorithm(&F);
            myla->print();
            return false;
        }
    };
}
char LivenessAnalysisPass::ID = 0;
static RegisterPass<LivenessAnalysisPass> X ("cse231-liveness", "Liveness analysis",false ,false);
