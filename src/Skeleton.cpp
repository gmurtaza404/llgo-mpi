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
#include "llvm/IR/ValueMap.h"
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
	struct SkeletonPass : public BasicBlockPass {

		static char ID;
		int count;
		const std::string goFuncName;	
		const std::string mainFuncName;
		LLVMContext llContext;

		SkeletonPass() : BasicBlockPass(ID), goFuncName("__go_go"), mainFuncName("main.main") { 
			count = 0;
		}

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

		void writeFile(Module *M, std::string fname){
			// opening file
			std::error_code eCode;
			llvm::sys::fs::OpenFlags flag = llvm::sys::fs::F_RW;				
			raw_fd_ostream file(StringRef(fname), eCode,flag);
			M->print(file, NULL);
			file.close();
			return;
		}

		void getGoFunction(BasicBlock &B, std::unordered_map<Function*, int> &functions) {
			for(auto it = B.begin(), end = B.end(); it != end; it++) {
				Instruction *current = &*it;
				if (CallInst *call = dyn_cast<CallInst>(it)) {
					if (call->getCalledFunction()->getName() != this->goFuncName)
						continue;
					Function *called = call->getCalledFunction();
					Value *temp = call->getArgOperand(1);
					temp->dump();
					if (auto a = dyn_cast<ConstantExpr>(temp)) {
						Value *val = a->getOperand(0);
						if (auto asd =  dyn_cast<Function>(val)) {
							functions[asd] = 0;
						}
					}
				}
			}
		}

		void getFuncDependencies(Function* f, func_map &visitedFunctions) {
			std::queue<Function*> funcQueue;

			funcQueue.push(f);
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
				}else{
					// already visited
					continue;
				}
			}
		}

		void insertFuncCall(Function *caller, Function *callee) {
			if (auto type = dyn_cast<PointerType>(callee->getFunctionType()->getParamType(0))) {
				auto a = ArrayRef<Value*>(ConstantPointerNull::get(type));
				Instruction *I = CallInst::Create(callee, a);
				caller->getEntryBlock().getInstList().push_front(I);
			}
		}

		void cloneAndInsertFunc(Function *old, Module* M) {
			ValueToValueMapTy val_map;

			Function *newF = Function::Create(old->getFunctionType(), GlobalValue::LinkageTypes::ExternalLinkage, old->getName(), M);	
			createFuncValMap(newF, old, val_map);	
			SmallVector<ReturnInst*, 8> returns;
			CloneFunctionInto(newF, old, val_map, true, returns);
		}

		void createFuncValMap(Function* f, Function* old_f, ValueToValueMapTy& val_map) {
			auto f_new = f->arg_begin();
			for (auto t = old_f->arg_begin(), t_end = old_f->arg_end(); t != t_end; t++) {
				val_map[&*t] = &*f_new;
				f_new++;
			}
		}


		Function* makeAndInsertMain(Module *M) {
			FunctionType *funcType = FunctionType::get(Type::getVoidTy(llContext), ArrayRef<Type*>(), false);
			Function *F = Function::Create(funcType, Function::InternalLinkage, this->mainFuncName, M);
			BasicBlock *BB = BasicBlock::Create(llContext);
			F->getBasicBlockList().push_front(BB);
			return F;
		}


		virtual bool runOnBasicBlock(BasicBlock &B) {
			std::unordered_map<Function*, int> functions;

			getGoFunction(B, functions);

			for (auto it = functions.begin(), end = functions.end(); it != end; it++) {
				std::unordered_map<Function*,int> dependencies;
				getFuncDependencies(&*(it->first), dependencies);

				std::string fileName = std::to_string(count);

				Module *M = new Module(fileName + ".ll", llContext);
				Function *entry;
				for (auto i = dependencies.begin(), en = dependencies.end(); i != en; i++) {
					cloneAndInsertFunc(&*(i->first), M);
				}

				for (auto i = M->begin(), en = M->end(); i != en; i++)
					entry = (&*i)->getName() == (&*it->first)->getName() ? &*i : NULL;

				Function *F = makeAndInsertMain(M);
				//get cloned entry
				insertFuncCall(F, entry);
				writeFile(M, fileName + ".ll");
				count++;
			}

			return false;
		}
	};
}

char SkeletonPass::ID = 0;

static RegisterPass<SkeletonPass> X("skeleton", "Hello World Pass",
		false /* Only looks at CFG */,
		false /* Analysis Pass */);
