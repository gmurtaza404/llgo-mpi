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
#include <queue>

using namespace llvm;



namespace {
	struct SkeletonPass : public BasicBlockPass {
		
		static char ID;
		int count = 0;
		
		std::unordered_map<Function*, int> functions;
		SkeletonPass() : BasicBlockPass(ID) { count = 0;}

		void getAllFunctions(Function* func, std::queue<Function*> &nestedCalls){
			if(func->begin() == func->end()){
				errs()<< "empty block";
				return;
			}
			for (auto bb_start=func->begin(),bb_end = func->end(); bb_start != bb_end; bb_start++){
				BasicBlock *bb = &*bb_start;
				for (auto ins_start=bb->begin(),ins_end=bb->end();ins_end != ins_start; ins_start++){
					Instruction *temp = &*ins_start;
					if(CallInst *call_ins = dyn_cast<CallInst>(ins_start)){
						//errs() << call_ins->getCalledFunction()->getName();
						nestedCalls.push(call_ins->getCalledFunction());
					}	
				}
			}
		}

		void writeFunctionOnaFile(Function* func,std::string fname){
				// opening file
				std::error_code eCode;
				llvm::sys::fs::OpenFlags flag = llvm::sys::fs::F_RW;				
				raw_fd_ostream file(StringRef(fname), eCode,flag);
				func->print(file);
				file.close();
				return;
		}
		
		virtual bool runOnBasicBlock(BasicBlock &B) {
			for(auto it = B.begin(), end = B.end(); it != end; it++) {
				Instruction *current = &*it;
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
				
				std::unordered_map<Function*,int> visitedFunctions;
				std::queue<Function*> funcQueue;

				Function* f = &*(it->first);
				getAllFunctions(f,funcQueue);
				
				// assuming no go calls in the go call.
				while(!funcQueue.empty()){
					Function* currentFunction = funcQueue.front();
					funcQueue.pop();
					auto tempIt = visitedFunctions.find(currentFunction);
					if(tempIt == visitedFunctions.end()){
						errs()<< "found a function \n";
						// mark the function visited
						visitedFunctions[currentFunction] = 0;
						// get all the function calls of that functio
						getAllFunctions(currentFunction,funcQueue);
						
						// write function to the file.
						std::string fileName = std::to_string(count);
						writeFunctionOnaFile(currentFunction,fileName);
					}else{
						// already visited
						continue;
					}
				}
				errs() << visitedFunctions.size() << "\n";
			}
			return false;
		}
	};
}

char SkeletonPass::ID = 0;

static RegisterPass<SkeletonPass> X("skeleton", "Hello World Pass",
		false /* Only looks at CFG */,
		false /* Analysis Pass */);
