//
//  CountStaticInstructions.cpp
//  LLVM
//
//  Created by hanminghao on 18/1/19.
//  Copyright © 2018年 apple. All rights reserved.
//

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include <string>
#include <map>
#include <iostream>
using namespace std;
using namespace llvm;
struct hmhllvm : public FunctionPass
{
    static char ID;
    hmhllvm() : FunctionPass(ID) {}
    map<string,int> instmap;
    virtual bool runOnFunction(Function &F)
    {
        for (BasicBlock &BB : F)

        {
            for (Instruction &I : BB)
                {
                string inst=I.getOpcodeName();
                instmap[inst]++;
                }
        }
        //use a map to store each instruction and the count
        map<string,int>::iterator it;
        for(it=instmap.begin();it!=instmap.end();++it)
        {
          //print out the result
            errs()<<it->first<<'\t'<<it->second<<'\n';
        }
        return false;
    }
};
char hmhllvm::ID = 0;
static RegisterPass<hmhllvm> X("cse231-csi","count static instruction",false,false);
