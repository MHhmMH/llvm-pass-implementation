//
//  MayPointToInfoAnalysis.cpp/Users/apple/Documents/CSE231_Project/Passes/DFA/MayPointToInfoAnalysis.cpp
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
class MayPointToInfo:public Info
{
private:
    map<pair<char,unsigned>,set<unsigned>> maypointinfo;
public:
    MayPointToInfo() {}
    MayPointToInfo(map<pair<char,unsigned>,set<unsigned>> maypointinfo)
    {
        this->maypointinfo = maypointinfo;
    }
    map<pair<char,unsigned>,set<unsigned>> &getmaypointinfo()
    {
        return this->maypointinfo;
    }
    void setmaypointinfo(map<pair<char,unsigned>,set<unsigned>> maypointinfo)
    {
        this->maypointinfo = maypointinfo;
    }
    set<unsigned> getmaypointset(pair<char,unsigned> mapkey)
    {
     return this->maypointinfo[mapkey];
    }

     void print()
    {
        for (auto it = maypointinfo.begin(); it!= maypointinfo.end(); ++it) 
        {
                if (!it->second.empty())
                {
                    char R_M=it->first.first;
                    unsigned DFA_identifier=it->first.second;
                    errs() <<  R_M << DFA_identifier << "->(";

                for (auto mi = it->second.begin(); mi != it->second.end(); ++mi) 
                {
                    errs() << "M" << *mi << "/";
                }
                errs() << ")|";
                }
                else 
                {
                	continue;
                }
        }
            errs() << "\n";
    }
    static bool equals(MayPointToInfo * info1,  MayPointToInfo* info2)
    {
        return (info1->getmaypointinfo()==info2->getmaypointinfo());
    }
    static MayPointToInfo* join(MayPointToInfo * info1, MayPointToInfo * info2, MayPointToInfo * result)
    {
        map<pair<char,unsigned>,set<unsigned>> resultmaypointinfo= info1->getmaypointinfo();
        map<pair<char,unsigned>,set<unsigned>> maypointinfo2 = info2->getmaypointinfo();
        for (auto it = maypointinfo2.begin(); it!= maypointinfo2.end(); ++it)
        {
          char R_M=it->first.first;
          unsigned DFA_identifier=it->first.second;
          set<unsigned> maypoint2=it->second;
          resultmaypointinfo[make_pair(R_M,DFA_identifier)].insert(maypoint2.begin(), maypoint2.end());
        }
        result->setmaypointinfo(resultmaypointinfo);
        return nullptr;
    }
};
class  MayPointToAnalysis : public DataFlowAnalysis<MayPointToInfo, true>
{
private:
    typedef pair<unsigned, unsigned> Edge;
    map<Edge,MayPointToInfo *> EdgeToInfo;
public:
    MayPointToAnalysis (MayPointToInfo & bottom, MayPointToInfo & initialState) :
    DataFlowAnalysis(bottom, initialState) {}
    void flowfunction(Instruction * I, std::vector<unsigned> & IncomingEdges,std::vector<unsigned> & OutgoingEdges,std::vector<MayPointToInfo *> & Infos)
    {
        string opname = I->getOpcodeName();
        map<unsigned, Instruction *> IndexToInstr=getIndexToInstr();
        map<Instruction *, unsigned> InstrToIndex=getInstrToIndex();
        unsigned index=InstrToIndex[I];
        map<Edge,MayPointToInfo *> EdgeToInfo=getEdgeToInfo();
        MayPointToInfo * outputInfo =new MayPointToInfo();
        MayPointToInfo * inputInfo =new MayPointToInfo();
        map<pair<char,unsigned>,set<unsigned>> maypointinfo;
        //join the information of the incoming edges
        for (unsigned i=0;i<IncomingEdges.size();i++)
        {
            Edge incomingEdge = make_pair(IncomingEdges[i], index);
            MayPointToInfo::join(inputInfo, EdgeToInfo[incomingEdge], inputInfo);
            MayPointToInfo::join(outputInfo, EdgeToInfo[incomingEdge], outputInfo);

        }
        // calculate if the operation is allocate
        if (isa<AllocaInst>(I))
        {
            maypointinfo[make_pair('R',index)].insert(index);
            MayPointToInfo::join(outputInfo, new MayPointToInfo(maypointinfo), outputInfo);
        }
        // calculate if the operation is bitcast to
        else if (isa<BitCastInst>(I))
        {
            Instruction *value=(Instruction *)I->getOperand(0);
            unsigned Rv = InstrToIndex[value];
            set<unsigned> Rvtox = inputInfo->getmaypointset(make_pair('R',Rv));
            maypointinfo[make_pair('R',index)].insert(Rvtox.begin(),(Rvtox.end()));
            MayPointToInfo::join(outputInfo, new MayPointToInfo(maypointinfo), outputInfo);
        }
        // calculate if the operation  is getelementptr
        else if (isa<GetElementPtrInst>(I))
        {
            GetElementPtrInst * instr = cast<GetElementPtrInst> (I);
            Instruction *ptrval=(Instruction *)instr->getPointerOperand();
            unsigned Rv = InstrToIndex[ptrval];
            set<unsigned> Rvtox = inputInfo->getmaypointset(make_pair('R',Rv));
            maypointinfo[make_pair('R',index)].insert(Rvtox.begin(),(Rvtox.end()));
            MayPointToInfo::join(outputInfo, new MayPointToInfo(maypointinfo), outputInfo);
        }
       // calculate if the operation is load
        else if (isa<LoadInst>(I))
        {
            if (I->getType()->isPointerTy()) 
            {
                LoadInst * instr = cast<LoadInst> (I);
                Instruction *pointer= (Instruction *)instr->getPointerOperand();
                unsigned Rp = InstrToIndex[pointer];
                set<unsigned> Rptox = inputInfo->getmaypointset(make_pair('R',Rp));
                for (auto it = Rptox.begin(); it != Rptox.end(); ++it) 
                {
                    set<unsigned> xtoy=inputInfo->getmaypointset(make_pair('M',*it));
                    maypointinfo[make_pair('R',index)].insert(xtoy.begin(),(xtoy.end()));
                }
                MayPointToInfo::join(outputInfo, new MayPointToInfo(maypointinfo), outputInfo);
            }
         }
         // calculate if the operation is store
         else if (isa<StoreInst>(I))
         {
            StoreInst * instr = cast<StoreInst> (I);
            Instruction *value=(Instruction *)instr->getValueOperand();
            Instruction *pointer=(Instruction *)instr->getPointerOperand();
            unsigned Rv = InstrToIndex[value];
            unsigned Rp = InstrToIndex[pointer];
            set<unsigned> Rvtox = inputInfo->getmaypointset(make_pair('R',Rv));
            set<unsigned> Rptoy = inputInfo->getmaypointset(make_pair('R',Rp));
            for (auto it = Rptoy.begin(); it != Rptoy.end(); ++it)
            {
                 maypointinfo[make_pair('M',*it)].insert(Rvtox.begin(),(Rvtox.end()));
            }
            MayPointToInfo::join(outputInfo, new MayPointToInfo(maypointinfo), outputInfo);
        }
        // calculate if the operation is select
        else if (isa<SelectInst>(I))
         {
            SelectInst * instr = cast<SelectInst> (I);
            Instruction *val0=(Instruction *)instr->getTrueValue();
            Instruction *valk=(Instruction *)instr->getFalseValue();
            unsigned R0 = InstrToIndex[val0];
            unsigned Rk = InstrToIndex[valk];
            set<unsigned> R0tox = inputInfo->getmaypointset(make_pair('R',R0));
            set<unsigned> Rktox = inputInfo->getmaypointset(make_pair('R',Rk));
            Rktox.insert(R0tox.begin(),R0tox.end());
            maypointinfo[make_pair('R',index)].insert(Rktox.begin(),(Rktox.end()));
            MayPointToInfo::join(outputInfo, new MayPointToInfo(maypointinfo), outputInfo);
        }
        // calculate if the operation is phi
        else if (isa<PHINode>(I)) 
        {
            Instruction * firstNonPhi = I->getParent()->getFirstNonPHI();
            unsigned firstNonPhiIdx = InstrToIndex[firstNonPhi];
            for (unsigned i = index; i < firstNonPhiIdx; ++i) 
            {
                    Instruction *instr = IndexToInstr[i];
                    for (unsigned j = 0; j < instr->getNumOperands(); ++j) 
                    {
                        
                        Instruction *val=(Instruction *)instr->getOperand(j);
                        unsigned Rk = InstrToIndex[val];
                        set<unsigned> X = inputInfo->getmaypointset(make_pair('R',Rk));
                        maypointinfo[make_pair('R',i)].insert(X.begin(),(X.end()));
                    }
            }

            MayPointToInfo::join(outputInfo, new MayPointToInfo(maypointinfo), outputInfo);
        }
        // return the outinfo back
        for (unsigned i = 0; i < OutgoingEdges.size(); ++i)
            {
                Infos.push_back(outputInfo);
            }
    }

};
namespace
{
    struct MayPointToAnalysisPass : public FunctionPass
    {
        static char ID;
        MayPointToAnalysisPass() : FunctionPass(ID) {}
        bool runOnFunction(Function &F) override
        {
            MayPointToInfo bottom;
            MayPointToInfo initialState;
            MayPointToAnalysis  * mypointa = new MayPointToAnalysis(bottom, initialState);
            mypointa->runWorklistAlgorithm(&F);
            mypointa->print();
            return false;
        }
    };
}
char MayPointToAnalysisPass::ID = 0;
static RegisterPass<MayPointToAnalysisPass>X("cse231-maypointto", "maypointto analysis",false ,false);
