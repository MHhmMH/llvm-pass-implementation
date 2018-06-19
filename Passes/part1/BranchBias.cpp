//
//  BranchBias.cpp
//  LLVM
//
//  Created by hanminghao on 18/1/22.
//  Copyright © 2018年 apple. All rights reserved.
//
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include <string>
#include <map>
#include <iostream>
using namespace std;
using namespace llvm;
struct Countbranch: public FunctionPass
    {
        static char ID;
        Countbranch() : FunctionPass(ID) {}
        virtual bool runOnFunction(Function &F)
        {
           // define the updatebranch function
            Function *const updatebranchfuc=cast<Function>(F.getParent()->getOrInsertFunction(("updateBranchInfo"),
                                                                                        Type::getVoidTy(F.getParent()->getContext()),
                                                                                        Type::getInt1Ty(F.getParent()->getContext())));
           // define the printbranch function
            Function *const printbranchfuc=cast<Function>(F.getParent()->getOrInsertFunction(("printOutBranchInfo"),Type::getVoidTy(F.getParent()->getContext())));
            int returnflag=0;
            for (BasicBlock &BB : F)
            {
                for (Instruction &I : BB)
                {
                    // insert the updatebranch function
                    if (I.getOpcode()==2&&I.getNumOperands() > 1)
                    {
                    IRBuilder<> builder1(&(*(--BB.end())));
                    vector<Value *> args;
                    args.push_back(I.getOperand(0));
                    builder1.CreateCall(updatebranchfuc,args);
                    }
                    if (I.getOpcode()==1)
                    {
                        returnflag=1;
                        break;
                    }
                }
                // insert the printbranch function if ret function
                if (returnflag==1)
                {
                    IRBuilder<> builder2(&(*(--BB.end())));
                    builder2.CreateCall(printbranchfuc);
                }

            }
            return true;
        }
    };
char  Countbranch::ID = 0;
static RegisterPass<Countbranch> X("cse231-bb","count branch pass",false,false);
