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
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <system_error>
#include <unordered_map>
#include <set>
#include <string>
#include <fstream>
#include <queue>

typedef std::unordered_map<llvm::Function*, int> func_map;

using namespace llvm;



namespace {
	struct SkeletonPass : public BasicBlockPass{

		static char ID;
		int count;
		const std::string goFuncName;	
		const std::string mainFuncName;
		LLVMContext llContext;

		SkeletonPass() : BasicBlockPass(ID), goFuncName("__go_go"), mainFuncName("main.main") { 
			count = 0;
		}

		

		void writeFile(Module *M, std::string fname){
			// opening file
			std::error_code eCode;
			llvm::sys::fs::OpenFlags flag = llvm::sys::fs::F_RW;				
			raw_fd_ostream file(StringRef(fname), eCode,flag);
			M->print(file, NULL);
			file.close();
			return;
		}

		Function* getGoFunction(BasicBlock &B) {
			for(auto it = B.begin(), end = B.end(); it != end; it++) {
				Instruction *current = &*it;
				if (CallInst *call = dyn_cast<CallInst>(it)) {
					if (call->getCalledFunction() && call->getCalledFunction()->getName() != this->goFuncName)
						continue;
					Function *called = call->getCalledFunction();

					Value *temp = call->getArgOperand(1);
					if (auto a = dyn_cast<ConstantExpr>(temp)) {
						Value *val = a->getOperand(0);
						if (auto asd =  dyn_cast<Function>(val)) {
							return asd;
						}
					}
				}
			}
			return NULL;
		}

		/*
		Function* makeAndInsertMain(Module *M) {
			FunctionType *funcType = FunctionType::get(Type::getVoidTy(llContext), ArrayRef<Type*>(), false);
			Function *F = Function::Create(funcType, Function::InternalLinkage, this->mainFuncName, M);
			BasicBlock *BB = BasicBlock::Create(llContext);
			F->getBasicBlockList().push_front(BB);
			return F;
		}
		*/

		void insertFuncCall(Function *caller, Function *callee) {
			if (auto type = dyn_cast<PointerType>(callee->getFunctionType()->getParamType(0))) {
				auto a = ArrayRef<Value*>(ConstantPointerNull::get(type));
				Instruction *I = CallInst::Create(callee, a);
				caller->getEntryBlock().getInstList().push_front(I);
			}
		}

		void addBasicBlock(Function *F) {
			BasicBlock *BB = BasicBlock::Create(llContext);
			F->getBasicBlockList().push_front(BB);
		}

		void emptyFunction(Function *F) {
			for(auto it = F->begin(), end = F->end(); it != end; it++) {
				BasicBlock *current = &*it;	
				for(auto inst = current->begin(), endInst = current->end(); inst != endInst; inst++) {
					Instruction *temp = &*inst;
				}
			}
		}

		virtual bool runOnBasicBlock(BasicBlock &BB) {
			auto M = BB.getModule();
			GlobalValue *val = M->getNamedValue("main.main");
			ValueToValueMapTy Vmap;
			auto cloned = CloneModule(M, Vmap, [val](const GlobalValue *GV) { 
				return GV->getName() != val->getName();
			});
			Function *new_main = cloned->getFunction("main.main");
			addBasicBlock(new_main);
			
			Function *old_called = getGoFunction(BB);
			if(!old_called || Vmap.find(old_called) == Vmap.end())
				return false;
			auto temp = Vmap[old_called];
			Value *t = temp;
			if(auto new_called = dyn_cast<Function>(t)) {
				insertFuncCall(new_main, new_called);
			}
			auto ret = ReturnInst::Create(llContext);
			new_main->getEntryBlock().getInstList().push_back(ret);
			std::string fileName = std::to_string(count);
			writeFile(cloned.get(), fileName + ".ll");
			return true;
		}
	};
}

char SkeletonPass::ID = 0;

static RegisterPass<SkeletonPass> X("skeleton", "Hello World Pass",
		false /* Only looks at CFG */,
		false /* Analysis Pass */);
