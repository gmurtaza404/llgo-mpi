#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"

#include <system_error>
#include <unordered_map>
#include <set>
#include <string>
#include <fstream>

using namespace llvm;

namespace {
	struct SkeletonPass : public BasicBlockPass {
		static char ID;
		int count;
		std::unordered_map<Function*, int> functions;
		SkeletonPass() : BasicBlockPass(ID) { count = 0;}

		virtual bool runOnBasicBlock(BasicBlock &B) {
			//errs() << "I saw a function called " << F.getName() << "!\n";
			for(auto it = B.begin(), end = B.end(); it != end; it++) {
				Instruction *current = &*it;
				//is func call, and __go_go
				if (CallInst *call = dyn_cast<CallInst>(it)) {
					if (call->getCalledFunction()->getName() != "__go_go")
						continue;
					Function *called = call->getCalledFunction();
					Value *temp = call->getArgOperand(1);
					temp->dump();
					errs() << temp->getName();
					if (auto a = dyn_cast<ConstantExpr>(temp)) {
						Value *val = a->getOperand(0);
						if (auto asd =  dyn_cast<Function>(val))
							functions[asd] = 0;
					}
				}
			}
			
			for (auto it = functions.begin(), end = functions.end(); it != end; it++) {
				//std::ofstream file;
 				std::error_code temp;
				llvm::sys::fs::OpenFlags flag = llvm::sys::fs::F_RW;	
 				raw_fd_ostream file(StringRef("temp.ll"), temp,flag);
				//file.open(std::to_string(count) + "temp.ll");
				Function* f = &*(it->first);
				for (auto bb_start = f->begin(), bb_end = f->end(); bb_start != bb_end; bb_start++) {
					BasicBlock *bb = &*bb_start;
					for(auto inst_start = bb->begin(), inst_end = bb->end(); inst_start != inst_end; inst_start++) {
						std::string str;
						Instruction *t = &*inst_start;
						f->print(file);
						//errs() << str;
						//file << str;
					}
				}
				count++;
				file.close();
			}
			return false;
		}
	};
}

char SkeletonPass::ID = 0;

static RegisterPass<SkeletonPass> X("skeleton", "Hello World Pass",
		false /* Only looks at CFG */,
		false /* Analysis Pass */);
