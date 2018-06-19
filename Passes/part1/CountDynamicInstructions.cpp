//
//  CountDynamicInstructions.cpp
//  LLVM
//
//  Created by hanminghao on 18/1/20.
//  Copyright © 2018年 apple. All rights reserved.
//

//
//  CountDynamicInstructions.cpp
//  LLVM
//
//  Created by hanminghao on 18/1/20.
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
struct CountDynamic: public FunctionPass
    {
        static char ID;
        CountDynamic() : FunctionPass(ID) {}
        virtual bool runOnFunction(Function &F)         {
            //define the updateinst function
            Function *const updateinstfuc=cast<Function>(F.getParent()->getOrInsertFunction(("updateInstrInfo"),
                                                                                              Type::getVoidTy(F.getParent()->getContext()),
                                                                                              Type::getInt32Ty(F.getParent()->getContext()),
                                                                                              Type::getInt32Ty(F.getParent()->getContext())));
           // define the printinst function
            Function *const printinstfuc=cast<Function>(F.getParent()->getOrInsertFunction(("printOutInstrInfo"),
                                                                                             Type::getVoidTy(F.getParent()->getContext())));
            for (BasicBlock &BB : F)
            {
                int returnflag=0;
                map<int,int> instmap;
                for (Instruction &I : BB)
                {
                    instmap[I.getOpcode()]++;
                    if (I.getOpcode()==1)
                    {
                        returnflag=1;
                        break;
                    }
                }
                //set the insert point by the last instruction of each block
                Instruction *lastinst=&(*(--(BB.end())));
                IRBuilder<> builder_update(lastinst);
                for(map<int,int>::iterator it=instmap.begin();it!=instmap.end();++it)
                {
                    //insert update function
                    vector<Value *> args;
                    args.push_back(builder_update.getInt32(it->first));
                    args.push_back(builder_update.getInt32(it->second));
                    builder_update.CreateCall(updateinstfuc,args);

                }
                if (returnflag==1)
                {
                    // insert the printinst function if ret instruction
                    Instruction *lastinst=&(*(--(BB.end())));
                    IRBuilder<> builder_ret (lastinst);
                    builder_ret.CreateCall(printinstfuc);
                    break;
                }
            }
            return true;
        }
};
char  CountDynamic::ID = 0;
static RegisterPass<CountDynamic> X("cse231-cdi","count dynamic pass",false,false);
