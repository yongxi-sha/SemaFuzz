#ifndef __SYMBOLIC_RUN_H__
#define __SYMBOLIC_RUN_H__
#include "llvm/Transforms/Instrumentation/SanitizerCoverage.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#if LLVM_VERSION_MAJOR >= 15
  #if LLVM_VERSION_MAJOR < 17
    #include "llvm/ADT/Triple.h"
  #endif
#endif
#include "llvm/Analysis/PostDominators.h"
#if LLVM_VERSION_MAJOR < 15
  #include "llvm/IR/CFG.h"
#endif
#include "llvm/IR/Constant.h"
#include "llvm/IR/DataLayout.h"
#if LLVM_VERSION_MAJOR < 15
  #include "llvm/IR/DebugInfo.h"
#endif
#include "llvm/IR/Dominators.h"
#if LLVM_VERSION_MAJOR >= 17
  #include "llvm/IR/EHPersonalities.h"
#else
  #include "llvm/Analysis/EHPersonalities.h"
#endif
#include "llvm/IR/Function.h"
#if LLVM_VERSION_MAJOR >= 16
  #include "llvm/IR/GlobalVariable.h"
#endif
#include "llvm/IR/IRBuilder.h"
#if LLVM_VERSION_MAJOR < 15
  #include "llvm/IR/InlineAsm.h"
#endif
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#if LLVM_VERSION_MAJOR < 15
  #include "llvm/IR/MDBuilder.h"
  #include "llvm/IR/Mangler.h"
#endif
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Type.h"
#if LLVM_VERSION_MAJOR < 17
  #include "llvm/InitializePasses.h"
#endif
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SpecialCaseList.h"
#include "llvm/Support/VirtualFileSystem.h"
#if LLVM_VERSION_MAJOR < 15
  #include "llvm/Support/raw_ostream.h"
#endif
#if LLVM_VERSION_MAJOR < 17
  #include "llvm/Transforms/Instrumentation.h"
#else
  #include "llvm/TargetParser/Triple.h"
#endif
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemoryLocation.h"

#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <forward_list>
#include <iterator>
#include "json.hpp"
#include "sym_svfcore.h"
#include "../SemLearn/semgdump/include/function.h"
#include "comgraph/libFunc.h"

using namespace llvm;
using json = nlohmann::json;

const std::string symSumFile = "symbolic_summary.json";

class LLVMTypeUtils 
{
public:
    /**
     * Get the description of an LLVM type.
     *
     * @param type The LLVM type to analyze.
     * @return A string describing the type, including its structure name if applicable.
     */
    static std::string getTypeAsString(const llvm::Type *type) 
    {
        if (type == nullptr) 
        {
            return "InvalidType";
        }
  
        if (type->isArrayTy()) 
        {
            return "Array[" + getTypeAsString(type->getArrayElementType()) + "]";
        } 
        else if (type->isPointerTy()) 
        {
            return getTypeAsString(type->getPointerElementType());
        } 
        else if (type->isStructTy()) 
        {
            return getStructType(type);
        } 
        else if (type->isVectorTy()) 
        {
            return getVectorType (type);
        }
        else
        {
            return type2string (type);
        }

        return "UnknownType";
    }

private:
    static std::string type2string(const llvm::Type *type) 
    {
        std::string typeStr;
        llvm::raw_string_ostream rso(typeStr);
        type->print(rso);
        return rso.str();
    }

    /**
     * Get the name or description of a structure type.
     *
     * @param type The LLVM type to check.
     * @return A string containing the structure name or "AnonymousStruct" for unnamed structs.
     */
    static std::string getStructType(const llvm::Type *type) 
    {
        const llvm::StructType *structType = llvm::cast<llvm::StructType>(type);
        if (structType->hasName()) 
        {
            return structType->getName().str();
        } 
        else 
        {
            return "AnonymousStruct";
        }
    }

    /**
     * Get the description of a vector type.
     *
     * @param type The LLVM type to check.
     * @return A string describing the vector type.
     */
    static std::string getVectorType(const llvm::Type *type) 
    {
        const llvm::VectorType *vectorType = llvm::cast<llvm::VectorType>(type);
        return "Vector(" + getTypeAsString(vectorType->getElementType()) + ")";
    }
};

class SymbolicExpr 
{
public:
    std::string expr;

    SymbolicExpr() : expr("") {}
    SymbolicExpr(const std::string &e) : expr(e)
    {
        if (expr.size () > 256)
        {
            expr = extractKeyWords (expr);
        }
        //print ();
    }

    SymbolicExpr(const SymbolicExpr &other) = default;
    SymbolicExpr &operator=(const SymbolicExpr &other) = default;

    ~SymbolicExpr() = default;

    inline bool hasUnknown ()
    {
        return (expr.find ("unknown") != string::npos);
    }

    inline void print ()
    {
        std::cout<<"#SymbolicExpr: "<<expr<<"\n";
    }

private:
    /* for some too complicated expr, simplify it into keywords list */
    std::string extractKeyWords(const std::string &expr) 
    {
        std::set<std::string> keywords;
        std::regex keyRegex(R"([a-zA-Z_][a-zA-Z_0-9]*)");

        for (std::sregex_iterator it(expr.begin(), expr.end(), keyRegex), end; it != end; ++it) 
        {
            keywords.insert((*it)[0]);
        }

        std::ostringstream result;
        for (auto it = keywords.begin(); it != keywords.end(); ++it) 
        {
            if (it != keywords.begin()) 
            {
                result << " ";
            }
            result << *it;
        }

        return result.str();
    }
};

struct SymbolicState 
{
public:
    SymbolicState() : returnExpr("") {}
    SymbolicState(const SymbolicState &other) = default;  
    SymbolicState &operator=(const SymbolicState &other) = default;  
    SymbolicState(SymbolicState &&other) = default;
    SymbolicState &operator=(SymbolicState &&other) = default;  

    ~SymbolicState() = default;

    std::vector<std::string> pathConditions;
    std::map<Value*, SymbolicExpr> symbolicVars;

    SymbolicExpr returnExpr;
    std::map<llvm::Value*, llvm::Argument*> argumentOutputs;

    bool terminated = false;
    std::map<Value*, std::unordered_set<Value *>> instToOperands;

    inline void defineSymVar (llvm::Value* var, SymbolicExpr synExpr)
    {
        symbolicVars [var] = synExpr;
        //errs()<<"@@@ defineSymVar: var = "<<var<<", expr = "<<synExpr.expr<<"\n";
    }

    inline void addProcedOperand (llvm::Instruction *Inst, llvm::Value* var)
    {
        auto [itr, inserted] = instToOperands.try_emplace(Inst, std::unordered_set<llvm::Value *>());
        if (!inserted && itr->second.find(var) != itr->second.end()) 
        {
            return;
        }

        itr->second.insert(var);

    }

    inline bool isProcedOperand(llvm::Instruction *Inst, llvm::Value *var) 
    {
        auto itr = instToOperands.find(Inst);
        if (itr != instToOperands.end() && itr->second.find(var) != itr->second.end()) 
        {
            return true;
        }

        return false;
    }

    inline SymbolicExpr* getSymVar (llvm::Value* var)
    {
        auto Itr = symbolicVars.find (var);
        if (Itr == symbolicVars.end ())
        {
            return NULL;
        }

        return &(Itr->second);
    }
};

struct WorklistEntry 
{
public:
    WorklistEntry(BasicBlock *BB, const SymbolicState &S) : block(BB), state(S) {}
    WorklistEntry(const WorklistEntry &other) = default;
    WorklistEntry &operator=(const WorklistEntry &other) = default;
    WorklistEntry(WorklistEntry &&other) = default;
    WorklistEntry &operator=(WorklistEntry &&other) = default;

    ~WorklistEntry() = default; 

    BasicBlock *block;
    SymbolicState state;
};

struct PathSummary 
{
    std::vector<std::string> pathConditions;
    std::string returnExpr;
    std::map<std::string, std::string> argumentOutputs;

    inline static std::string conditionStr (const std::vector<std::string>& pathCons)
    {   
        std::string pathConStr = "";
        for (size_t i = 0; i < pathCons.size(); ++i) 
        {
            pathConStr += pathCons[i];
            if (i < pathCons.size() - 1) 
            {
                pathConStr += " & ";
            }
        }

        return pathConStr;
    }
};

struct SymCallsite
{
    std::string callees;
    SymbolicExpr calleeExpr;
    std::vector<std::string> callConditions;

    SymCallsite (std::string calls, SymbolicExpr cexpr, std::vector<std::string> considitons) 
    {
        callees    = calls;
        calleeExpr = cexpr;
        callConditions = considitons;
    }

};

class SymRun 
{
public:
    SymRun(SvfCore* svfObj) 
    {
        activeInst   = NULL;
        activeBlock  = NULL;
        activeSymExpr= NULL;

        this->svfObj  = svfObj;

        allSummaries   = json::array();
        allGlobalsVars = json::array();
    }
    ~SymRun() {}

private:
    SvfCore* svfObj;

    SymbolicState symbolicState;
    std::queue<WorklistEntry> worklist;

    std::unordered_map<Value*, SymbolicExpr> globalSymbolicVars;

    std::vector<SymCallsite> functionCalls;

    std::vector<PathSummary> pathSummaries;

    SymbolicExpr* activeSymExpr;
    Instruction *activeInst;
    BasicBlock *activeBlock;

    json allSummaries;
    json allGlobalsVars;

public:
    void initSymRun (Module &M);
    bool runOnFunction(Function &F);
    void addGlobalsToJSON(Module &M);
    void addSummaryToJSON(unsigned FuncId, Function &F, FuncIDs* funcIDs); 
    void writeSummaryToJSON(const std::string &outputFile=symSumFile);

private:
    SymbolicExpr getSymbolicExpr(Value *V);
    void analyzeInstruction(Instruction &I);

    std::string sanitizeInput(const std::string& str) 
    {
        std::string sanitized;
        for (char c : str) 
        {
            if (c >= 0x20) 
            {
                sanitized += c;
            }
        }
        return sanitized;
    }

    inline std::string getGlobalConstStr(const llvm::GlobalVariable *GV) 
    {
        if (!GV->hasInitializer()) 
        {
            return "";
        }
        
        const llvm::Constant *init = GV->getInitializer();
        if (const auto *dataArray = llvm::dyn_cast<llvm::ConstantDataArray>(init)) 
        {
            if (dataArray->isString()) 
            {
                return dataArray->getAsString().str();
            }
        }

        if (const auto *constExpr = llvm::dyn_cast<llvm::ConstantExpr>(init)) 
        {
            if (constExpr->getOpcode() == llvm::Instruction::GetElementPtr) 
            {
                const llvm::Value *basePtr = constExpr->getOperand(0);
                if (const auto *baseGlobal = llvm::dyn_cast<llvm::GlobalVariable>(basePtr)) 
                {
                    return getGlobalConstStr(baseGlobal);
                }
            }
        }

        return "";
    }

    inline bool istoProcess (llvm::Function &llvmFunc)
    {
        if (llvmFunc.empty()) return false;
        if (llvmFunc.isDeclaration()) return false;
        
        if (llvmFunc.getName().find(".module_ctor") != std::string::npos) return false;
        if (llvmFunc.getName().find ("__local_stdio_") != std::string::npos) return false;
        if (llvmFunc.getName().startswith("__sanitizer_")) return false;
        if (llvmFunc.getLinkage() == GlobalValue::AvailableExternallyLinkage) return false;

        if (LibFuncs::isLibFunc (llvmFunc.getName().str())) return false;

        return true;
    }

    void initGlobalVariables(Module &M) 
    {
        for (auto &global : M.globals()) 
        {
            if (!global.hasName()) 
            {
                errs() << "Skipping unnamed global variable.\n";
                continue;
            }

            std::string globalName = global.getName().str();
            std::string typeString = LLVMTypeUtils::getTypeAsString(global.getType());

            SymbolicExpr symbolicGlobal(globalName);
            defineGloalSymVar (&global, symbolicGlobal);
        }
    }

    inline void cacheFunctionCall (SymbolicExpr synExpr, std::string callees)
    {
        for (auto Itr = functionCalls.begin (), endItr = functionCalls.end(); Itr != endItr; Itr++)
        {
            SymCallsite *cs = &(*Itr);
            if (cs->calleeExpr.expr == synExpr.expr)
            {
                return;
            }
        }
        functionCalls.push_back (SymCallsite(callees, synExpr, symbolicState.pathConditions));
    }

    inline void defineGloalSymVar (llvm::Value* var, SymbolicExpr synExpr)
    {
        globalSymbolicVars [var] = synExpr;
    }

    inline SymbolicExpr* getGloalSymVar (llvm::Value* var)
    {
        auto Itr = globalSymbolicVars.find (var);
        if (Itr == globalSymbolicVars.end ())
        {
            return NULL;
        }

        return &(Itr->second);
    }

    inline void mergeToGlobalSymbolicVars(const SymbolicState &state) 
    {
        for (const auto &var : state.symbolicVars) 
        {
            defineGloalSymVar(var.first, var.second);
        }
        return;
    }

    void showPathSummary(const PathSummary& summary) 
    {
        std::cout << "\n#### Path Summary Details ####" << std::endl;

        std::cout << "\tPath Conditions: " << std::endl;
        for (const auto& condition : summary.pathConditions) 
        {
            std::cout << "\t  " << condition << std::endl;
        }

        std::cout << "\tReturn Expression: " << summary.returnExpr << std::endl;

        std::cout << "\tArgument Outputs: " << std::endl;
        for (const auto& argOutputPair : summary.argumentOutputs) 
        {
            const std::string& argName = argOutputPair.first;
            const std::string& argOutput = argOutputPair.second;

            std::cout << "\t  " << argName << ": " << argOutput << std::endl;
        }

        std::cout << "################################\n" << std::endl;
    }

    inline void savePath(SymbolicState &state) 
    {
        PathSummary summary;
        summary.pathConditions = state.pathConditions;
        summary.returnExpr = state.returnExpr.expr;

        for (auto Itr = state.argumentOutputs.begin (); Itr != state.argumentOutputs.end (); Itr++) 
        {
            llvm::Argument *arg = Itr->second;
            llvm::Value *val    = Itr->first;
            
            //errs() << "### Argument output " <<*val<< " ----> "<<*arg<<"\n";
            std::string argName = arg->hasName() ? arg->getName().str() : "arg" + std::to_string(arg->getArgNo());
            SymbolicExpr *symExpr = state.getSymVar (val);
            if (symExpr != NULL) 
            {
                summary.argumentOutputs[argName] = symExpr->expr + "@type(" + LLVMTypeUtils::getTypeAsString (val->getType())+ ")";
            } 
            else 
            {
                //errs() << "Warning: Argument " << argName << " does not have a symbolic expression.\n";
                summary.argumentOutputs[argName] = "unknown";
            }
        }
        
        //showPathSummary (summary);
        pathSummaries.push_back(summary);
    }

    void initBlockExeNum(Function &F, std::map<BasicBlock*, int> &blockExeNum) 
    {
        std::queue<BasicBlock*> worklist;

        BasicBlock *entry = &F.getEntryBlock();
        blockExeNum[entry] = 1;
        worklist.push(entry);

        while (!worklist.empty()) 
        {
            BasicBlock *current = worklist.front();
            worklist.pop();

            for (auto succ = succ_begin(current); succ != succ_end(current); ++succ) 
            {
                BasicBlock *succBlock = *succ;
                if (blockExeNum.find (succBlock) != blockExeNum.end ())
                {
                    continue;
                }

                int inEdgeNum = 0;
                for (BasicBlock *pred : predecessors(succBlock)) 
                {
                    auto Itr = blockExeNum.find (pred);
                    if (Itr == blockExeNum.end ())
                    {
                        inEdgeNum += 1;
                    }
                    else
                    {
                        inEdgeNum += Itr->second;
                    }
                }
                blockExeNum[succBlock] = inEdgeNum>16 ? 16 : inEdgeNum;
                worklist.push(succBlock);
            }
        }

        #if 0
        errs()<<"#### Total BlockNum: "<<blockExeNum.size()<<"\n";
        //for (auto &entry : blockExeNum) 
        //{
        //    errs() << "Block " << entry.first << " execution count: " << entry.second << "\n";
        //}
        #endif
    }


    inline bool isTerminalBlock(BasicBlock *block) const 
    {
        for (const Instruction &I : *block) 
        {
            if (isa<ReturnInst>(I) || isa<UnreachableInst>(I)) 
            {
                return true;
            }
        }
        return false;
    }

    inline void ShowSymVars ()
    {
        errs() << "******** ShowSymVars ********" << &symbolicState <<" sym varsize: "<<symbolicState.symbolicVars.size()<< "\n";
        for (auto It = symbolicState.symbolicVars.begin (); It != symbolicState.symbolicVars.end(); It++)
        {
            SymbolicExpr symExpr = It->second;
            errs() <<"********"<< It->first << ": "<< symExpr.expr << "\n";
        }
    }

    std::string getValueWithType(Value *V) 
    {
        std::string typeStr;
        llvm::raw_string_ostream typeStream(typeStr);
        V->getType()->print(typeStream);

        if (isa<PoisonValue>(V)) 
        {
            return typeStream.str() + "_poison";
        }
        else if (isa<UndefValue>(V)) {
            return typeStream.str() + "_undef";
        }
        else
        {
            assert (0);
            return "";
        }

        std::string valueStr;
        llvm::raw_string_ostream valueStream(valueStr);
        V->print(valueStream);
        return valueStream.str();
    }

    inline bool isConstVector (Value *V)
    {
        if (dyn_cast<ConstantVector>(V) || 
            dyn_cast<ConstantDataVector>(V) || dyn_cast<ConstantAggregateZero>(V))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    inline void initializeArguments(Function &F) 
    {
        unsigned argIndex = 0;
        for (auto &Arg : F.args()) 
        {
            std::string argName = "arg" + std::to_string(argIndex++);

            SymbolicExpr symbolicValue(argName);
            symbolicState.defineSymVar (&Arg, symbolicValue);

            //errs() << "Initialized symbolic value for argument: " << Arg << "("<<&Arg<<")"
            //      << " = " << symbolicValue.expr <<" sym varsize: "<<symbolicState.symbolicVars.size()<< "\n";
        }
    }

    inline Value* resolveBasePointer(Value *V) 
    {
        while (auto *bitcast = dyn_cast<BitCastInst>(V)) 
        {
            V = bitcast->getOperand(0);
        }
        return V;
    }

    inline void analyzeBasicBlock(BasicBlock &BB) 
    {
        //errs() << "\r\n\r\n### Processing Basic Block: " << BB << "\n";
        for (Instruction &I : BB) 
        {
            //errs() << "\n## Processing instruction: " << &I <<" -> "<<I <<"\n";
            activeInst = &I;
            analyzeInstruction(I);
        }
    }

    inline void handleUnreachable(UnreachableInst &I) 
    {
        //errs() << "Encountered unreachable instruction: No further execution possible.\n";
        savePath(symbolicState);
        symbolicState.terminated = true;
    }

    inline void handleInsertElement(InsertElementInst &I) 
    {
        SymbolicExpr vectorExpr = getSymbolicExpr(I.getOperand(0)); // The base vector
        SymbolicExpr elementExpr = getSymbolicExpr(I.getOperand(1)); // The element to insert
        SymbolicExpr indexExpr = getSymbolicExpr(I.getOperand(2)); // The index

        SymbolicExpr result("insertelement(" + vectorExpr.expr + "," + elementExpr.expr + "," + indexExpr.expr + ")");
        symbolicState.defineSymVar (&I, result);
        //errs() << "InsertElement: " << result.expr << "\n";
    }

    inline std::string formatShuffleMask(const ArrayRef<int> &mask) 
    {
        std::string result = "{";
        for (size_t i = 0; i < mask.size(); ++i) 
        {
            if (i > 0) result += ",";
            if (mask[i] == -1) 
            {
                result += "undef";
            } 
            else 
            {
                result += std::to_string(mask[i]);
            }
        }
        result += "}";

        return result;
    }

    inline void handleShuffleVector(ShuffleVectorInst &I) 
    {
        Value *v1 = I.getOperand(0);
        Value *v2 = I.getOperand(1);
        ArrayRef<int> mask = I.getShuffleMask();

        SymbolicExpr exprV1 = getSymbolicExpr(v1);
        SymbolicExpr exprV2 = getSymbolicExpr(v2);
        std::string maskStr = formatShuffleMask(mask);
        std::string resultExpr = "shufflevector(" + exprV1.expr + "," + exprV2.expr + "," + maskStr + ")";

        symbolicState.defineSymVar (&I, SymbolicExpr(resultExpr));
        //errs() << "ShuffleVector: " << resultExpr << "\n";
    }

    inline void handleBitCast(BitCastInst &I)
    {
        Value *operand = I.getOperand(0);

        SymbolicExpr operandExpr = getSymbolicExpr(operand);
        SymbolicExpr result("bitcast(" + operandExpr.expr + ")");
        symbolicState.defineSymVar (&I, result);

        //errs() << "BitCast Instruction: " << &I << " -> " << result.expr << "\n";
    }

    inline void handleBranch(BranchInst &I) 
    {
        if (I.isConditional()) 
        {
            SymbolicExpr cond = getSymbolicExpr(I.getCondition());
            if (cond.hasUnknown()) 
            {
                //errs() << "Warning: Condition is unknown. Adding generic branches.\n";
            }

            //errs()<<"###handleBranch: getCondition() --> "<<*(I.getCondition())<<"\n";

            // True branch state
            SymbolicState trueState = symbolicState; // Clone the current state
            trueState.pathConditions.push_back(cond.expr); // Use raw condition for true branch
            worklist.push(WorklistEntry(I.getSuccessor(0), trueState));

            // False branch state
            SymbolicState falseState = symbolicState; // Clone the current state
            falseState.pathConditions.push_back("!" + cond.expr); // Negate condition for false branch
            worklist.push(WorklistEntry(I.getSuccessor(1), falseState));

            //errs() << "Conditional branch on: " << cond.expr << "\n";
            //errs() << "  True branch: " << cond.expr << "\n";
            //errs() << "  False branch: " << "!" << cond.expr << "\n";
        } 
        else 
        {
            worklist.push(WorklistEntry(I.getSuccessor(0), symbolicState));
            //errs() << "Unconditional branch.\n";
        }
    }


    inline void handleSwitch(SwitchInst &I) 
    {
        SymbolicExpr condition = getSymbolicExpr(I.getCondition());
        if (condition.hasUnknown()) 
        {
            //errs() << "Warning: Switch condition is unknown: "<<condition.expr<<"\n";
        }

        // Handle each case block
        for (auto &caseBlock : I.cases()) 
        {
            Value *caseValue = caseBlock.getCaseValue();

            SymbolicExpr caseExpr = getSymbolicExpr(caseValue);
            if (caseExpr.hasUnknown()) 
            {
                //errs() << "Warning: Case value is unknown.\n";
            }

            SymbolicState caseState = symbolicState;
            caseState.pathConditions.push_back(condition.expr + "==" + caseExpr.expr);

            worklist.push(WorklistEntry(caseBlock.getCaseSuccessor(), caseState));

            //errs() << "Case: " << caseExpr.expr << " -> Block: " 
            //      << caseBlock.getCaseSuccessor()->getName() << "\n";
        }

        // Handle the default block
        BasicBlock *defaultBlock = I.getDefaultDest();
        SymbolicState defaultState = symbolicState;

        std::string defaultCondition = "!(";
        bool firstCase = true;
        for (auto &caseBlock : I.cases()) 
        {
            if (!firstCase) 
            {
                defaultCondition += "|";
            }
            Value *caseValue = caseBlock.getCaseValue();
            SymbolicExpr caseExpr = getSymbolicExpr(caseValue);
            defaultCondition += "(" + condition.expr + "==" + caseExpr.expr + ")";
            firstCase = false;
        }
        defaultCondition += ")";
        defaultState.pathConditions.push_back(defaultCondition);

        worklist.push(WorklistEntry(defaultBlock, defaultState));
        //errs() << "Default: " << defaultBlock->getName() << " with condition: " 
        //      << defaultCondition << "\n";
    }

    inline void handleReturn(ReturnInst &I) 
    {
        if (I.getReturnValue()) 
        {
            Value *ret = I.getReturnValue();
            symbolicState.returnExpr = getSymbolicExpr(ret);
            symbolicState.returnExpr.expr += "@type(" + LLVMTypeUtils::getTypeAsString (ret->getType()) + ")";
            //errs() << "Return value: " << &symbolicState << ": " << symbolicState.returnExpr.expr << "\n";
        } 
        else 
        {
            symbolicState.returnExpr = SymbolicExpr("");
            //errs() << "Return value: " << &symbolicState << ": " << "Return without value (void).\n";
        }
    }

    inline void initFuncPointerArg(Value *arg, int argNo) 
    {
        if (arg->getType()->isPointerTy() && arg->getType()->getPointerElementType()->isFunctionTy()) 
        {
            if (arg->hasName()) 
            {
                defineGloalSymVar (arg, SymbolicExpr(arg->getName().str()));
            } 
            else 
            {
                defineGloalSymVar (arg, SymbolicExpr("unknow_arg_" + std::to_string(argNo)));
            }
        }
        return;
    }

    inline std::string getArgsExpr (CallInst &I)
    {
        std::string allArgExpr = "";
        for (unsigned i = 0; i < I.getNumOperands ()-1; ++i) 
        {
            Value *arg = I.getArgOperand(i);
            initFuncPointerArg (arg, i);

            SymbolicExpr argExpr = getSymbolicExpr(arg);

            if (i > 0) allArgExpr += ",";
            allArgExpr += argExpr.expr;

            //errs() << "Argument " << i << ": " << *arg << ", expr = " << argExpr.expr << "\n";
        }

        return allArgExpr;
    }

    inline llvm::Argument* getCalleeReachedArgs (CallInst &I)
    {
        if (svfObj->isActive () == false)
        {
            return NULL;
        }

        for (unsigned i = 0; i < I.getNumOperands ()-1; ++i) 
        {
            Value *operand = I.getArgOperand(i);
            if (!operand->getType()->isPointerTy())
            {
                continue;
            }

            llvm::Argument* arg = (llvm::Argument*)svfObj->getReachableArgument (&I, operand);
            if (arg != NULL)
            {
                return arg;
            }   
        }

        return NULL;
    }

    inline void handleCall(CallInst &I) 
    {
        if (auto arg = getCalleeReachedArgs (I))
        {
            symbolicState.argumentOutputs[&I] = arg;
        }

        Function *callee = I.getCalledFunction();
        if (callee) 
        {
            std::string calleeName = callee->getName().str();
            if (calleeName.find("llvm.dbg") == 0 || 
                calleeName.find("llvm.lifetime") == 0 ||
                calleeName.find("llvm.experimental") == 0)
            {
                return;
            }

            std::string resultExpr;
            std::string arguments = getArgsExpr (I);
            //errs() << "Direct function call: arguments -> " << arguments << "\n";
            if (arguments.find (calleeName) == string::npos)
            {
                resultExpr = calleeName + "(" + arguments + ")";
            }
            else
            {
                // represent recursive
                resultExpr = calleeName + "(" + calleeName + ")";
            }

            SymbolicExpr calledExpr = SymbolicExpr(resultExpr);
            symbolicState.defineSymVar (&I, calledExpr);
            cacheFunctionCall (calledExpr, calleeName);
            //errs() << "Direct function call: " << calleeName << " with expression: " << resultExpr << "\n";
        } 
        else 
        {
            Value *calledValue = I.getCalledOperand();
            vector<Value*> ptsFuncs = svfObj->getPointsTo(calledValue);
            if (ptsFuncs.size () != 0)
            {
                std::string calleeNames = "";
                std::string resultExpr = "";
                //errs() << "\t### PTS function num: " << ptsFuncs.size () << "\n";
                resultExpr += "(";
                unsigned maxNum = 8;
                for (auto Itr = ptsFuncs.begin (); Itr != ptsFuncs.end (); Itr++)
                {
                    Value* callee = *Itr;
                    SymbolicExpr calledExpr;
                    if (getGloalSymVar (callee) == NULL)
                    {
                        calledExpr = SymbolicExpr(callee->getName().str());
                        defineGloalSymVar (callee, calledExpr);
                    }
                    else
                    {
                        calledExpr = getSymbolicExpr(callee);
                    }

                    if (resultExpr.find (calledExpr.expr) != string::npos)
                    {
                        continue;
                    }

                    if (resultExpr != "(") resultExpr += "|";
                    if (calleeNames != "") calleeNames += "|";

                    calleeNames += calledExpr.expr;
                    resultExpr += calledExpr.expr;

                    maxNum--;
                    if (maxNum == 0)
                    {
                        break;
                    }
                }
                resultExpr += ")(" + getArgsExpr (I) + ")";

                SymbolicExpr calledExpr = SymbolicExpr(resultExpr);
                symbolicState.defineSymVar (&I, calledExpr);
                cacheFunctionCall (calledExpr, calleeNames);
                //errs() << "PTS: Indirect function call with expression: " << resultExpr << "\n";
            }
            else
            {
                SymbolicExpr calledExpr = getSymbolicExpr(calledValue);
                std::string resultExpr = calledExpr.expr + "(" + getArgsExpr (I) + ")";
                symbolicState.defineSymVar (&I, SymbolicExpr(resultExpr));
                //errs() << "NON-PTS: Indirect function call with expression: " << resultExpr << "\n";
            }   
        }
    }

    inline void handleInvoke(llvm::InvokeInst &I) 
    {
        Value *calledFunction = I.getCalledOperand();
        vector<Value*> ptsFuncs = svfObj->getPointsTo(calledFunction);
        if (ptsFuncs.size() != 0)
        {
            calledFunction = ptsFuncs[0];
        }
        SymbolicExpr funcExpr = getSymbolicExpr(calledFunction);

        std::vector<SymbolicExpr> argumentExprs;
        for (unsigned i = 0; i < I.getNumOperands() - 3; ++i) 
        {
            Value *arg = I.getOperand(i);
            SymbolicExpr argExpr = getSymbolicExpr(arg);
            argumentExprs.push_back(argExpr);
        }

        #ifdef SIMPLIFY_EXPR
        SymbolicExpr resultExpr(funcExpr.expr + "()");
        #else
        SymbolicExpr resultExpr("invoke(" + funcExpr.expr + ")");
        #endif
        symbolicState.defineSymVar(&I, resultExpr);

        // Debug output for function and arguments
        //errs() << "Invoke instruction: " << I << "\n";
        //errs() << "Function being invoked: " << funcExpr.expr << "\n";
        //errs() << "Arguments:\n";
        //for (unsigned i = 0; i < argumentExprs.size(); ++i) 
        //{
        //    errs() << "  Arg " << i << ": " << argumentExprs[i].expr << "\n";
        //}
    }


    inline void handleLoad(LoadInst &I) 
    {
        Value *pointer = I.getPointerOperand();

        SymbolicExpr addressExpr = getSymbolicExpr(pointer);
        SymbolicExpr resultExpr(addressExpr.expr);
        symbolicState.defineSymVar (&I, resultExpr);

        if (addressExpr.hasUnknown()) 
        {
            //errs() << "Warning: Loading from an unknown address: "<<addressExpr.expr<<"\n";
        }

        //errs() << "Load instruction: " << I << "\n";
        //errs() << "Address: " << addressExpr.expr << "\n";
        //errs() << "Resulting Symbolic Expression: " << resultExpr.expr << "\n";
    }

    inline void handlArgOutput (StoreInst &I)
    {
        Value *ptr = I.getPointerOperand();
        llvm::Argument* arg = (llvm::Argument*)svfObj->getReachableArgument (&I, ptr);
        if (arg == NULL)
        {
            return;
        }

        symbolicState.argumentOutputs[ptr] = arg;
        //errs ()<<"### ArgumentOutputs: "<<*ptr <<"@type ("<<*(ptr->getType ()) <<") ---> "
        //      <<*arg<<"@type ("<<*(ptr->getType ())<<")\n";

        return;
    }

    inline void handleStore(StoreInst &I) 
    {
        Value *ptr = I.getPointerOperand();
        Value *val = I.getValueOperand();

        handlArgOutput (I);

        SymbolicExpr valueExpr = getSymbolicExpr(val);
        
        ptr = resolveBasePointer(ptr);
        SymbolicExpr pointerExpr = getSymbolicExpr(ptr);

        if (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) 
        {
            SymbolicExpr baseExpr = getSymbolicExpr(gep->getPointerOperand());

            #ifdef SIMPLIFY_EXPR
            symbolicState.defineSymVar (gep, SymbolicExpr(baseExpr.expr));
            #else
            std::string offsetExpr;
            for (unsigned i = 1; i < gep->getNumOperands(); ++i) 
            {
                SymbolicExpr indexExpr = getSymbolicExpr(gep->getOperand(i));
                if (!offsetExpr.empty()) offsetExpr += " + ";
                offsetExpr += indexExpr.expr;
            }
            symbolicState.defineSymVar (gep, SymbolicExpr("gep(" + baseExpr.expr + "," + offsetExpr + ")"));
            #endif
        }
        else 
        {
            symbolicState.defineSymVar (ptr, valueExpr);
        }
    }

    inline void handleBinaryOp(BinaryOperator &I) 
    {
        if (I.getOpcode() == Instruction::LShr) 
        {
            handleLshr (I);
            return;
        }
        else if (I.getOpcode() == Instruction::Or) 
        {
            handleOr (I);
            return;
        }
        else if (I.getOpcode() == Instruction::FDiv)
        {
            handleFpDiv (I);
            return;
        }

        Value *lhs = I.getOperand(0);
        Value *rhs = I.getOperand(1);

        std::string resExpr = "";
        SymbolicExpr lhsExpr = getSymbolicExpr(lhs);
        SymbolicExpr rhsExpr = getSymbolicExpr(rhs);

        std::string opSymbol;
        switch (I.getOpcode()) 
        {
        case Instruction::Add:  opSymbol = "+"; break;
        case Instruction::Sub:  opSymbol = "-"; break;
        case Instruction::Mul:  opSymbol = "*"; break;
        case Instruction::UDiv: opSymbol = "/"; break;
        case Instruction::SDiv: opSymbol = "/"; break;
        case Instruction::URem: opSymbol = "%"; break;
        case Instruction::SRem: opSymbol = "%"; break;
        case Instruction::And:  opSymbol = "&"; break;
        case Instruction::Or:   opSymbol = "|"; break;
        case Instruction::Xor:  opSymbol = "^"; break;
        case Instruction::Shl:  opSymbol = "<<"; break;
        case Instruction::LShr: opSymbol = ">>"; break;
        case Instruction::AShr: opSymbol = ">>"; break;
        default:
            opSymbol = I.getOpcodeName();
            break;
        }

        if (lhsExpr.hasUnknown() || rhsExpr.hasUnknown()) 
        {
            //errs() << "Warning: Binary operation has unknown operand(s).\n";
        }

        resExpr = "(" + lhsExpr.expr + opSymbol + rhsExpr.expr + ")";
        symbolicState.defineSymVar (&I, SymbolicExpr(resExpr));

        //errs() << "Binary Operation: " << lhsExpr.expr << " " << opSymbol << " " << rhsExpr.expr << "\n";
        //errs() << "Resulting Symbolic Expression: " << result.expr << "\n";
    }

    std::string predicateToSign(const std::string& operation) 
    {
        static const std::unordered_map<std::string, std::string> operationMap = 
        {
            {"sgt", ">"},   // Signed Greater Than
            {"slt", "<"},   // Signed Less Than
            {"sle", "<="},  // Signed Less Than or Equal
            {"sge", ">="},  // Signed Greater Than or Equal
            {"eq", "=="},   // Equality
            {"ne", "!="},   // Not Equal
            {"ugt", ">"},   // Unsigned Greater Than
            {"ult", "<"},   // Unsigned Less Than
            {"ule", "<="},  // Unsigned Less Than or Equal
            {"uge", ">="},  // Unsigned Greater Than or Equal
            {"oeq",	"=="},
            {"one",	"!="},
            {"olt",	"<"},
            {"ole",	"<="},
            {"ogt",	">"},
            {"oge",	">="},
            {"ord",	"!NaN"},       
            {"ueq",	"=="},
            {"une",	"!="},
            {"ult",	"<"},
            {"ule",	"<="},
            {"ugt",	">"},
            {"uge",	">="},
            {"uno",	"!NaN"},
        };

        auto it = operationMap.find(operation);
        if (it != operationMap.end()) 
        {
            return it->second;
        } 
        else 
        {
            errs()<<"###predicateToSign: "<<operation<<"\n";
            return "unknown";
        }
    }

    inline void handleICmp(ICmpInst &I) 
    {
        Value *lhs = I.getOperand(0);
        Value *rhs = I.getOperand(1);

        SymbolicExpr lhsExpr = getSymbolicExpr(lhs);
        SymbolicExpr rhsExpr = getSymbolicExpr(rhs); 
        if (lhsExpr.hasUnknown() || rhsExpr.hasUnknown()) 
        {
            //errs() << "Warning: ICmp operation has unknown operand(s): "<<lhs<<" --- "<<rhs<<"\n";
        }

        std::string predicate = predicateToSign(ICmpInst::getPredicateName(I.getPredicate()).str());
        SymbolicExpr result("(" + lhsExpr.expr + predicate + rhsExpr.expr + ")");
        symbolicState.defineSymVar (&I, result);

        //errs() << "ICmp Operation: " << lhsExpr.expr <<predicate<< rhsExpr.expr << "\n";
        //errs() << "Resulting Symbolic Expression: " << result.expr << "\n";
    }

    inline void handlePHI(PHINode &I) 
    {
        std::string resultExpr;

        if (activeSymExpr == NULL)
        {
            resultExpr = "";
        }
        else
        {
            resultExpr = activeSymExpr->expr;
            if (resultExpr[0] == '(') resultExpr = resultExpr.substr(1, resultExpr.size() - 2);
        }
    
        for (unsigned i = 0; i < I.getNumIncomingValues(); ++i) 
        {
            Value *incomingVal = I.getIncomingValue(i);
            if (symbolicState.isProcedOperand (&I, incomingVal))
            {
                continue;
            }

            SymbolicExpr incomingExpr = getSymbolicExpr(incomingVal);
            if (!incomingExpr.hasUnknown()) 
            {
                if (resultExpr != "")
                {
                    resultExpr += "|";
                }

                resultExpr += incomingExpr.expr;
                symbolicState.addProcedOperand (&I, incomingVal);
            }
            else
            {
                // it is fine, we will update it later
                //errs() << "[PHI]Warning: unknown incoming value -> " << *incomingVal << "\n";
            }
        }

        if (resultExpr.find ("|") != string::npos)
        {
            resultExpr = "(" + resultExpr + ")";
        }

        resultExpr = mergeExpressions (resultExpr);
        symbolicState.defineSymVar (&I, SymbolicExpr(resultExpr));
        //errs() << "PHI Node Symbolic Expression: " << resultExpr << "\n";
    }

    inline void handleSExt(SExtInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);
        if (operandExpr.hasUnknown()) 
        {
            //errs() << "Warning: SExt operation has an unknown operand.\n";
        }

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr result(operandExpr.expr);
        #else
            SymbolicExpr result("sext(" + operandExpr.expr + ")");
        #endif
        symbolicState.defineSymVar (&I, result);
        //errs() << "SExt Symbolic Expression: " << result.expr << "\n";
    }

    inline void handleZExt(ZExtInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);
        if (operandExpr.hasUnknown()) 
        {
            //errs() << "Warning: ZExt operation has an unknown operand.\n";
        }

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr result(operandExpr.expr);
        #else
            std::string bitWidth = "";
            if (I.getType()->isIntegerTy())
            {
                bitWidth = std::to_string(I.getType()->getIntegerBitWidth());
            }
            else
            {
                bitWidth = getTypeString (I.getType());
            }
            SymbolicExpr result("zext(" + operandExpr.expr + "->i" + bitWidth + ")");
        #endif

        symbolicState.defineSymVar (&I, result);
        //errs() << "ZExt Symbolic Expression: " << result.expr << "\n";
    }


    inline void handleTrunc(TruncInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);
        if (operandExpr.hasUnknown()) 
        {
            //errs() << "Warning: Trunc operation has an unknown operand.\n";
        }

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr result( operandExpr.expr);
        #else
            std::string bitWidth = "";
            if (I.getType()->isIntegerTy())
            {
                bitWidth = std::to_string(I.getType()->getIntegerBitWidth());
            }
            else
            {
                bitWidth = getTypeString (I.getType());
            }
            SymbolicExpr result("trunc(" + operandExpr.expr + "->i" + bitWidth + ")");
        #endif

        symbolicState.defineSymVar (&I, result);
        //errs() << "Trunc Symbolic Expression: " << result.expr << "\n";
    }

    inline void handleLshr(BinaryOperator &I) 
    {
        Value *lhs = I.getOperand(0);
        Value *rhs = I.getOperand(1);

        SymbolicExpr lhsExpr = getSymbolicExpr(lhs);
        SymbolicExpr rhsExpr = getSymbolicExpr(rhs);

        if (lhsExpr.hasUnknown() || rhsExpr.hasUnknown()) 
        {
            //errs() << "Warning: Lshr operation has unknown operand(s).\n";
        }

        SymbolicExpr result("(" + lhsExpr.expr + ">>" + rhsExpr.expr + ")");
        symbolicState.defineSymVar (&I, result);
        //errs() << "Lshr Symbolic Expression: " << result.expr << "\n";
    }

    inline void handleAlloca(AllocaInst &I) 
    {
        std::string varName = I.hasName() ? I.getName().str() : "unnamed";

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr result(varName);
        #else
            SymbolicExpr result("alloca(" + varName + ")");
        #endif

        symbolicState.defineSymVar (&I, result);
        //errs() << "Alloca Symbolic Expression: " << result.expr << "\n";
    }

    inline void handleGEP(GetElementPtrInst &I) 
    {
        Value *basePointer = I.getPointerOperand();
        SymbolicExpr baseExpr = getSymbolicExpr(basePointer);
        if (baseExpr.hasUnknown()) 
        {
            //errs() << "Warning: GEP operation has an unknown base pointer: "<<baseExpr.expr<<"\n";
        }

        #ifdef SIMPLIFY_EXPR
            std::string resultExpr = "";
            if (activeSymExpr != NULL)
            {
                resultExpr = activeSymExpr->expr;
            }
            else
            {
                resultExpr = baseExpr.expr;
            }
        #else
            std::string resultExpr = "";
            if (activeSymExpr != NULL)
            {
                resultExpr = activeSymExpr->expr;
                resultExpr.pop_back ();
            }
            else
            {
                resultExpr = "gep(" + baseExpr.expr;
            }

            for (unsigned i = 1; i < I.getNumOperands(); ++i) 
            {
                SymbolicExpr indexExpr = getSymbolicExpr(I.getOperand(i));
                if (resultExpr.find (indexExpr.expr) == string::npos)
                {
                    resultExpr += ", " + indexExpr.expr;
                }
            }
            resultExpr += ")";
        #endif

        

        SymbolicExpr result(resultExpr);
        symbolicState.defineSymVar (&I, result);
        //errs() << "GEP Symbolic Expression: " << &I << " --> " <<result.expr << "\n";
    }

    inline void handleSelect(SelectInst &I) 
    {
        Value *cond = I.getCondition();
        Value *trueVal = I.getTrueValue();
        Value *falseVal = I.getFalseValue();

        SymbolicExpr condExpr = getSymbolicExpr(cond);
        SymbolicExpr trueExpr = getSymbolicExpr(trueVal);
        SymbolicExpr falseExpr = getSymbolicExpr(falseVal);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr result("(" + condExpr.expr + "|" + trueExpr.expr + "|" + falseExpr.expr + ")");
        #else
            SymbolicExpr result("select(" + condExpr.expr + "," + trueExpr.expr + "," + falseExpr.expr + ")");
        #endif
        
        symbolicState.defineSymVar (&I, result);
        //errs() << "Select Instruction: " << result.expr << "\n";
    }

    inline std::string getTypeString(llvm::Type *type) 
    {
        std::string typeStr;
        llvm::raw_string_ostream rso(typeStr);
        type->print(rso);
        return rso.str();
    }

    inline void handlePtrToInt(PtrToIntInst &I) 
    {
        Value *ptr = I.getOperand(0);

        SymbolicExpr ptrExpr = getSymbolicExpr(ptr);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr result(ptrExpr.expr);
        #else
            std::string intType = getTypeString(I.getType());
            SymbolicExpr result("ptrtoint(" + ptrExpr.expr + "->" + intType + ")");
        #endif    

        symbolicState.defineSymVar (&I, result);
        //errs() << "PtrToInt Instruction: " << result.expr << "\n";
    }

    inline void handleIntToPtr(IntToPtrInst &I) 
    {
        Value *operand = I.getOperand(0);

        SymbolicExpr operandExpr = getSymbolicExpr(operand);
        #ifdef SIMPLIFY_EXPR
            std::string resultExpr = operandExpr.expr;
        #else
            std::string resultExpr = "inttoptr(" + operandExpr.expr + "->" + getTypeString(I.getType()) + ")";
        #endif  
        
        SymbolicExpr result(resultExpr);

        symbolicState.defineSymVar (&I, result);
        //errs() << "IntToPtr Instruction: " << resultExpr << "\n";
    }

    inline void handleFreeze(FreezeInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);
        symbolicState.defineSymVar (&I, operandExpr);
    }

    inline void handleOr(BinaryOperator &I) 
    {
        Value *lhs = I.getOperand(0);
        Value *rhs = I.getOperand(1);

        //errs ()<<"\tleft-type = "<<*(lhs->getType ())<<", right-type = "<<*(rhs->getType ())<<"\n";

        std::string resExpr = "";
        SymbolicExpr lhsExpr = getSymbolicExpr(lhs);
        SymbolicExpr rhsExpr = getSymbolicExpr(rhs);
        if (activeSymExpr == NULL || activeSymExpr->expr.size () <= 1)
        {  
            resExpr = "(" + lhsExpr.expr + "|" + rhsExpr.expr + ")";
        }
        else
        {
            resExpr = activeSymExpr->expr;
            resExpr.pop_back ();

            if (resExpr.find (lhsExpr.expr) == string::npos)
            {
                resExpr += "|" + lhsExpr.expr;
            }

            if (resExpr.find (rhsExpr.expr) == string::npos)
            {
                resExpr += "|" + lhsExpr.expr;
            }
            resExpr += ")";
        }

        resExpr = mergeExpressions (resExpr);
        symbolicState.defineSymVar (&I, SymbolicExpr(resExpr));
    }

    inline void handleFpDiv (BinaryOperator &I)
    {
        Value *lhs = I.getOperand(0);
        Value *rhs = I.getOperand(1);

        SymbolicExpr lhsExpr = getSymbolicExpr(lhs);
        SymbolicExpr rhsExpr = getSymbolicExpr(rhs);

        if (lhsExpr.hasUnknown() || rhsExpr.hasUnknown()) 
        {
            //errs() << "Warning: Binary operation has unknown operand(s).\n";
        }

        SymbolicExpr result("(" + lhsExpr.expr + "/" + rhsExpr.expr + ")");
        symbolicState.symbolicVars[&I] = result;

        //errs() << "FDiv Operation: " << lhsExpr.expr << "/" << rhsExpr.expr << "\n";
        //errs() << "Resulting Symbolic Expression: " << result.expr << "\n";
    }

    inline void handleExtractValue(ExtractValueInst &I) 
    {
        Value *aggregate = I.getAggregateOperand();
        SymbolicExpr aggregateExpr = getSymbolicExpr(aggregate);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr result(aggregateExpr.expr);
        #else
            std::string indicesExpr;
            for (unsigned idx : I.getIndices()) 
            {
                if (!indicesExpr.empty()) indicesExpr += ",";
                indicesExpr += std::to_string(idx);
            }
            SymbolicExpr result("extractvalue(" + aggregateExpr.expr + "," + indicesExpr + ")");
        #endif  
        
        symbolicState.defineSymVar (&I, result);
    }

    inline void handleInsertValue(InsertValueInst &I) 
    {
        Value *aggregate = I.getAggregateOperand();
        Value *value = I.getInsertedValueOperand();

        SymbolicExpr aggregateExpr = getSymbolicExpr(aggregate);
        SymbolicExpr valueExpr = getSymbolicExpr(value);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr result(aggregateExpr.expr + "insert(" + valueExpr.expr + ")");
        #else
            std::string indicesExpr;
            for (unsigned idx : I.getIndices()) 
            {
                if (!indicesExpr.empty()) indicesExpr += ",";
                indicesExpr += std::to_string(idx);
            }
            SymbolicExpr result("insertvalue(" + aggregateExpr.expr + "," + valueExpr.expr + "," + indicesExpr + ")");
        #endif 
        
        symbolicState.defineSymVar (&I, result);
        //errs() << "InsertValue Symbolic Expression: " << result.expr << "\n";
    }

    inline void handleFCmp(FCmpInst &I) 
    {
        Value *lhs = I.getOperand(0);
        Value *rhs = I.getOperand(1);

        SymbolicExpr lhsExpr = getSymbolicExpr(lhs);
        SymbolicExpr rhsExpr = getSymbolicExpr(rhs);

        std::string predicate = predicateToSign(FCmpInst::getPredicateName(I.getPredicate()).str());
        SymbolicExpr result("(" + lhsExpr.expr + predicate + rhsExpr.expr + ")");
        symbolicState.defineSymVar (&I, result);

        //errs() << "FCmp Operation: " << lhsExpr.expr << " " << predicate << " " << rhsExpr.expr << "\n";
        //errs() << "Resulting Symbolic Expression: " << result.expr << "\n";
    }

    inline void handleFPTrunc(FPTruncInst &I) 
    {
        Value *operand = I.getOperand(0);

        SymbolicExpr operandExpr = getSymbolicExpr(operand);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr resultExpr(operandExpr.expr);
        #else
            std::string targetType = getTypeString(I.getType());
            SymbolicExpr resultExpr("fptrunc(" + operandExpr.expr + "->" + targetType + ")");
        #endif 
        
        symbolicState.defineSymVar (&I, resultExpr);
        //errs() << "FPTrunc Symbolic Expression: " << resultExpr.expr << "\n";
    }

    inline void handleUIToFP(UIToFPInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr resultExpr(operandExpr.expr);
        #else
            std::string targetType = getTypeString(I.getType());
            SymbolicExpr resultExpr("uitofp(" + operandExpr.expr + "->" + targetType + ")");
        #endif  

        symbolicState.defineSymVar (&I, resultExpr);
        //errs() << "UIToFP Symbolic Expression: " << resultExpr.expr << "\n";
    }

    inline void handleExtractElement(ExtractElementInst &I) 
    {
        Value *vector = I.getVectorOperand();
        SymbolicExpr vectorExpr = getSymbolicExpr(vector);

        Value *index = I.getIndexOperand();
        SymbolicExpr indexExpr = getSymbolicExpr(index);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr resultExpr(vectorExpr.expr + "[" + indexExpr.expr + "]");
        #else
            SymbolicExpr resultExpr("extractelement(" + vectorExpr.expr + "," + indexExpr.expr + ")");
        #endif   
        

        symbolicState.defineSymVar (&I, resultExpr);
        //errs() << "ExtractElement Symbolic Expression: " << resultExpr.expr << "\n";
    }

    inline void handleSIToFP(SIToFPInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr resultExpr(operandExpr.expr);
        #else
            std::string targetType = I.getType()->isFloatTy() ? "float" : "double";
            SymbolicExpr resultExpr("sitofp(" + operandExpr.expr + "->" + targetType + ")");
        #endif

        symbolicState.defineSymVar (&I, resultExpr);
        //errs() << "SIToFP Instruction: " << result.expr << "\n";
    }

    inline void handleFNeg(UnaryOperator &I) 
    {
        if (I.getOpcode() != Instruction::FNeg) 
        {
            errs() << "Error: Non-fneg instruction passed to handleFNeg.\n";
            return;
        }

        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr resultExpr(operandExpr.expr);
        #else
            SymbolicExpr resultExpr("fneg(" + operandExpr.expr + ")");
        #endif
        
        symbolicState.defineSymVar (&I, resultExpr);
        //errs() << "FNeg Instruction: " << resultExpr.expr << "\n";
    }

    inline void handleFPExt(FPExtInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr resultExpr(operandExpr.expr);
        #else
            std::string targetType = getTypeString(I.getType());
            SymbolicExpr resultExpr("fpext(" + operandExpr.expr + "->" + targetType + ")");
        #endif

        symbolicState.defineSymVar (&I, resultExpr);
        //errs() << "FPExt Instruction: " << resultExpr.expr << "\n";
    }

    inline void handleFPToSI(FPToSIInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr resultExpr(operandExpr.expr);
        #else
            std::string targetType = getTypeString(I.getType());
            SymbolicExpr resultExpr("fptosi(" + operandExpr.expr + "->" + targetType + ")");
        #endif

        symbolicState.defineSymVar (&I, resultExpr);
        //errs() << "FPToSI Instruction: " << resultExpr.expr << "\n";
    }

    inline void handleFPToUI(FPToUIInst &I) 
    {
        Value *operand = I.getOperand(0);
        SymbolicExpr operandExpr = getSymbolicExpr(operand);

        #ifdef SIMPLIFY_EXPR
            SymbolicExpr resultExpr(operandExpr.expr);
        #else
            std::string targetType = getTypeString(I.getType());
            SymbolicExpr resultExpr("fptoui(" + operandExpr.expr + "->" + targetType + ")");
        #endif

        symbolicState.symbolicVars[&I] = resultExpr;
        //errs() << "FPToUI Instruction: " << resultExpr.expr << "\n";
    }
        

    inline void testMergeExprs ()
    {
        std::string expr = "(0|1|0|(0 + 2))";
        std::string merged = mergeExpressions(expr);
        std::cout <<expr<< "---> Merged Expression: " << merged << std::endl;
    }

    inline std::string mergeExpressions(const std::string &expr) 
    {
        std::set <string> uniqueExprs;

        if (expr.empty() || expr.find ("|") == string::npos) 
        {
            return expr;
        }

        std::string exprNew = expr.substr(1, expr.size() - 2);
        std::stringstream ss(exprNew);
        std::string token;
        while (std::getline(ss, token, '|')) 
        {
            uniqueExprs.insert (token);
        }

        std::string simplifiedExpr = "";
        for (const auto& term : uniqueExprs) {
            if (simplifiedExpr != "") 
            {
                simplifiedExpr += "|";
            }
            simplifiedExpr += term;
        }

        if (simplifiedExpr.find ("|") != string::npos)
        {
            simplifiedExpr = "(" + simplifiedExpr + ")";
        }

        return simplifiedExpr;
    }

    inline std::string removeFormatStrings(const std::string& value) 
    {
        std::regex formatRegex(R"(%[0-9.]*[a-zA-Z])");
        return std::regex_replace(value, formatRegex, "");
    }

    inline std::string cleanValue(const std::string& value) 
    {
        string cleanVal = removeFormatStrings (value);

        std::istringstream stream(cleanVal);
        std::ostringstream cleanedStream;
        std::string word;

        bool firstWord = true;
        while (stream >> word) 
        {
            word.erase(std::remove_if(
                word.begin(), word.end(), [](char c) 
                {
                    return !std::isalnum(c) && c != '_' && c != '<' && c != '!' && c != '/' && c != ':' && c != '.' && c != '-'; 
                }
            ), word.end());
            
            if (word.empty()) continue;

            if (!firstWord) 
            {
                cleanedStream << " ";
            }
            cleanedStream << word;
            firstWord = false;
        }

        return cleanedStream.str();
    }
};

#endif

