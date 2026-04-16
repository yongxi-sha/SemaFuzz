/***********************************************************
 * Author: Wen Li
 * Date  : 09/30/2024
 * Describe: symbolic execution for function summarization
 * History:
   <1>  09/30/2024, create
************************************************************/
#include "sym_run.h"

void SymRun::initSymRun (Module &M)
{
    functionCalls.clear ();
    pathSummaries.clear ();
    globalSymbolicVars.clear ();

    activeInst  = NULL;
    activeBlock = NULL;
    activeSymExpr= NULL;

    initGlobalVariables(M);
}

bool SymRun::runOnFunction(Function &F) 
{
    if (istoProcess (F) == false)
    {
        return false;
    }

    //errs() << "\r\n\r\n##### Starting symbolic execution for function: " <<F.getName()<< "\n";
    std::map<BasicBlock*, int> blockExeNum;
    initBlockExeNum(F, blockExeNum);

    // Initialize the worklist and state
    symbolicState = SymbolicState();
    worklist.push(WorklistEntry(&F.getEntryBlock(), symbolicState));

    while (!worklist.empty()) 
    {
        WorklistEntry entry = worklist.front();
        worklist.pop();

        BasicBlock *block = entry.block;
        symbolicState = entry.state;

        auto blockItr = blockExeNum.find (block);
        if (blockItr->second <= 0)
        {
            continue;
        }
        blockItr->second -= 1;

        // entry block, init function arguments
        if (block == &F.getEntryBlock ())
        {
            initializeArguments(F);                
        }

        activeBlock = block;
        analyzeBasicBlock(*block);
        if (isTerminalBlock(block)) 
        {
            savePath(symbolicState);
        }

        // maintain a global vars for PHI
        mergeToGlobalSymbolicVars (symbolicState);
    }

    //errs() << "##### Finished symbolic execution for function: " <<F.getName()<< "\n";

    return true;
}

void SymRun::addSummaryToJSON (unsigned FuncId, Function &F, FuncIDs* funcIDs)
{
    json root;

    //errs() << "\n####### writeSummaryToJSON: " << &symbolicState << "\n";
    root["id"]   = std::to_string(FuncId);
    root["name"] = F.getName ().str();

    std::string inputs = "";
    unsigned argNo = 0;
    for (const auto &Arg : F.args()) 
    {
        if (argNo != 0)
        {
            inputs += ", ";
        }

        std::string argName = Arg.hasName() ? Arg.getName().str() : ("arg" + std::to_string(argNo));
        inputs += argName + "@type(" + LLVMTypeUtils::getTypeAsString (Arg.getType()) + ")";
        argNo++; 
    }
    root["input"] = inputs;

    //paths
    json pathJson = json::array();
    for (const auto &path : pathSummaries) 
    {
        json pathjn;

        pathjn["conditions"] = PathSummary::conditionStr (path.pathConditions);
        pathjn["return"] = path.returnExpr;

        json argOutputsJson;
        for (const auto &argOutput : path.argumentOutputs) 
        {
            argOutputsJson[argOutput.first] = argOutput.second;
        }
        pathjn["arg_outputs"] = argOutputsJson;
        pathJson.push_back(pathjn);
    }
    root["paths"] = pathJson;

    //call sites
    json callsJson = json::array();
    for (auto Itr = functionCalls.begin (); Itr != functionCalls.end (); Itr++) 
    {
        json csJson;

        SymCallsite *cs = &(*Itr);
        if (funcIDs != NULL && funcIDs->getFuncID (cs->callees) == 0)
        {
            // we only keep functions in the list
            continue;
        }
        csJson["expr"] = cs->calleeExpr.expr;
        csJson["callee"]     = cs->callees;
        csJson["conditions"] = PathSummary::conditionStr (cs->callConditions);

        callsJson.push_back(csJson);
    }
    root["calls"] = callsJson;
    allSummaries.push_back(root);

    return;
}

void SymRun::addGlobalsToJSON(Module &M)
{
    for (const auto &GV : M.globals()) 
    {
        if (!GV.hasInitializer()) 
        {
            continue;
        }
            
        std::string content = getGlobalConstStr(&GV);
        if (content.empty())
        {
            continue;
        }
        
        content = sanitizeInput (content);
        if (content.empty())
        {
            continue;
        }

        content = cleanValue (content);
        if (content.empty())
        {
            continue;
        }

        json globalVar;
        globalVar["name"]  = GV.getName().str();
        globalVar["type"]  = LLVMTypeUtils::getTypeAsString(GV.getType());
        globalVar["value"] = content;

        allGlobalsVars.push_back (globalVar);
    }

    return;
}

void SymRun::writeSummaryToJSON(const std::string &outputFile) 
{
    errs() << "##### writeSummaryToJSON to " <<outputFile<< "\n";
    std::ofstream file(outputFile, std::ios::out | std::ios::trunc);
    if (!file.is_open()) 
    {
        errs() << "Error opening file for JSON output: " << outputFile << "\n";
        return;
    }

    json jsonOutput;

    if (!allGlobalsVars.empty()) 
    {
        jsonOutput["globals"] = allGlobalsVars;
        errs ()<<"##### addGlobalsToJSON -> "<<allGlobalsVars.size()<<"\n";
    }

    jsonOutput["functions"] = allSummaries;
    errs ()<<"##### addFunctionsToJSON -> "<<allSummaries.size()<<"\n";

    file << jsonOutput.dump(4) << "\n";
    file.close();
}


inline SymbolicExpr SymRun::getSymbolicExpr(llvm::Value *V) 
{
    //errs() << ">>>> getSymbolicExpr: " << V << "--> " <<*V<< "\n";
    //ShowSymVars ();
    V = resolveBasePointer(V);

    // local state search
    SymbolicExpr *symExpr = symbolicState.getSymVar (V);
    if (symExpr != NULL) 
    {
        return *symExpr;
    }

    // global state search
    symExpr = getGloalSymVar (V);
    if (symExpr != NULL) 
    {
        return *symExpr;
    }

    // create new symbolic variable
    if (auto *constInt = dyn_cast<ConstantInt>(V)) 
    {
        const llvm::APInt &apIntValue = constInt->getValue();
        if (apIntValue.getSignificantBits() <= 64) 
        {
            return SymbolicExpr(std::to_string(apIntValue.getSExtValue()));
        }
        else
        {
            return SymbolicExpr("INT_LARGE");
        }
    }
    else if (auto *constFP = dyn_cast<ConstantFP>(V)) 
    {
        std::ostringstream stream;
        stream << constFP->getValueAPF().convertToDouble();
        return SymbolicExpr(stream.str());
    }
    else if (auto *arg = dyn_cast<Argument>(V)) 
    {
        return SymbolicExpr("arg_" + std::to_string(arg->getArgNo()));
    } 
    else if (auto *gep = dyn_cast<GetElementPtrInst>(V)) 
    {
        SymbolicExpr baseExpr = getSymbolicExpr(gep->getPointerOperand());
        std::string resultExpr = "gep(" + baseExpr.expr;

        for (unsigned i = 1; i < gep->getNumOperands(); ++i) 
        {
            SymbolicExpr indexExpr = getSymbolicExpr(gep->getOperand(i));
            resultExpr += "," + indexExpr.expr;
        }
        resultExpr += ")";
        return SymbolicExpr(resultExpr);
    }
    else if (auto *constExpr = dyn_cast<ConstantExpr>(V)) 
    {
        if (constExpr->getOpcode() == Instruction::GetElementPtr) 
        {
            SymbolicExpr baseExpr = getSymbolicExpr(constExpr->getOperand(0));
            std::string resultExpr = "gep(" + baseExpr.expr;

            for (unsigned i = 1; i < constExpr->getNumOperands(); ++i) 
            {
                SymbolicExpr indexExpr = getSymbolicExpr(constExpr->getOperand(i));
                resultExpr += "," + indexExpr.expr;
            }
            resultExpr += ")";
            return SymbolicExpr(resultExpr);
        }
        else if (constExpr->getOpcode() == Instruction::IntToPtr) 
        {
            SymbolicExpr intExpr = getSymbolicExpr(constExpr->getOperand(0));
            std::string resultExpr = "inttoptr(" + intExpr.expr + ")";
            return SymbolicExpr(resultExpr);
        }
        else if (constExpr->getOpcode() == Instruction::PtrToInt) 
        {
            SymbolicExpr ptrExpr = getSymbolicExpr(constExpr->getOperand(0));
            std::string resultExpr = "ptrtoint(" + ptrExpr.expr + ")";
            return SymbolicExpr(resultExpr);
        }
        else if (constExpr->getOpcode() == Instruction::ICmp) 
        {
            auto *cmpInst = dyn_cast<ICmpInst>(constExpr->getAsInstruction());
            if (cmpInst)
            {
                handleICmp(*cmpInst);
                return getSymbolicExpr(cmpInst);
            }
        }
        else if (constExpr->getOpcode() == Instruction::Select) 
        {
            auto *selectInst = dyn_cast<SelectInst>(constExpr->getAsInstruction());
            if (selectInst)
            {
                handleSelect(*selectInst);
                return getSymbolicExpr(selectInst);
            }
        }
        else if (constExpr->getOpcode() == Instruction::BitCast) 
        {
            return getSymbolicExpr(constExpr->getOperand(0));
        }
    } 
    else if (isa<UndefValue>(V) || V->getName().str() == "poison") 
    {
        return SymbolicExpr(getValueWithType (V));
    }
    else if (isa<ConstantPointerNull>(V)) 
    {
        return SymbolicExpr("null");
    }
    else if (auto *intToPtr = dyn_cast<IntToPtrInst>(V)) 
    {
        SymbolicExpr operandExpr = getSymbolicExpr(intToPtr->getOperand(0));
        std::string resultExpr = "inttoptr(" + operandExpr.expr + "->" + getTypeString(intToPtr->getType()) + ")";
        return SymbolicExpr(resultExpr);
    }
    else if (auto *func = dyn_cast<Function>(V)) 
    {
        return SymbolicExpr(func->getName().str());
    }
    else if (isConstVector (V))
    {
        return SymbolicExpr("ConstVector");
    }
    else if (dyn_cast<PHINode>(V))
    {
        return SymbolicExpr("unknown");
    }
    else if (V->getType()->isAggregateType()) 
    {
        if (auto *inst = dyn_cast<Constant>(V))
        {
            std::string resultExpr = "{";
            for (unsigned i = 0; i < inst->getNumOperands(); ++i) 
            {
                Value *element = inst->getOperand(i);   
                if (!element) 
                {
                    continue;
                }

                SymbolicExpr elemExpr = getSymbolicExpr(element);
                resultExpr += elemExpr.expr;
                if (i < inst->getNumOperands() - 1) 
                {
                    resultExpr += ", ";
                }
            }

            resultExpr += "}";
            return SymbolicExpr(resultExpr);
        }
    }

    if (!dyn_cast<PHINode>(activeInst))
    {
        // only warning for non-PHI insts
        //errs() << "\nActive Inst: "<<*activeInst<<"\n";
        //errs() << "Warning: Unknown symbolic expression for Value: "<<*V<<" with Type = "<<*(V->getType())<<"\n";
    }
        
    return SymbolicExpr("unknown");
}


inline void SymRun::analyzeInstruction(Instruction &I) 
{
    activeSymExpr = symbolicState.getSymVar (&I);

    if (auto *phi = dyn_cast<PHINode>(&I)) 
    {
        handlePHI(*phi);
    } 
    else if (auto *icmp = dyn_cast<ICmpInst>(&I)) 
    {
        handleICmp(*icmp);
    } 
    else if (auto *switchInst = dyn_cast<SwitchInst>(&I)) 
    {
        handleSwitch(*switchInst);
    } 
    else if (auto *sext = dyn_cast<SExtInst>(&I)) 
    {
        handleSExt(*sext);
    }
    else if (auto *zext = dyn_cast<ZExtInst>(&I)) 
    {
        handleZExt(*zext);
    }
    else if (auto *branch = dyn_cast<BranchInst>(&I)) 
    {
        handleBranch(*branch);
    } 
    else if (auto *ret = dyn_cast<ReturnInst>(&I)) 
    {
        handleReturn(*ret);
    } 
    else if (auto *call = dyn_cast<CallInst>(&I)) 
    {
        handleCall(*call);
    }
    else if (auto *invoke = dyn_cast<InvokeInst>(&I)) 
    {
        handleInvoke(*invoke);
    }
    else if (auto *trunc = dyn_cast<TruncInst>(&I)) 
    {
        handleTrunc(*trunc);
    } 
    else if (auto *binaryOp = dyn_cast<BinaryOperator>(&I)) 
    {
        handleBinaryOp(*binaryOp);
    }
    else if (auto *alloca = dyn_cast<AllocaInst>(&I)) 
    {
        handleAlloca(*alloca);
    } 
    else if (auto *gep = dyn_cast<GetElementPtrInst>(&I)) 
    {
        handleGEP(*gep);
    } 
    else if (auto *load = dyn_cast<LoadInst>(&I)) 
    {
        handleLoad(*load);
    }
    else if (auto *storeInst = dyn_cast<StoreInst>(&I)) 
    {
        handleStore(*storeInst);
    } 
    else if (auto *selectInst = dyn_cast<SelectInst>(&I)) 
    {
        handleSelect(*selectInst);
    }
    else if (auto *bitcast = dyn_cast<BitCastInst>(&I)) 
    {
        handleBitCast(*bitcast);
    }
    else if (auto *insertElem = dyn_cast<InsertElementInst>(&I)) 
    {
        handleInsertElement(*insertElem);
    } 
    else if (auto *shuffleVec = dyn_cast<ShuffleVectorInst>(&I)) 
    {
        handleShuffleVector(*shuffleVec);
    }
    else if (auto *unreachable = dyn_cast<UnreachableInst>(&I)) 
    {
        handleUnreachable(*unreachable);
    }
    else if (auto *ptrtoint = dyn_cast<PtrToIntInst>(&I)) 
    {
        handlePtrToInt(*ptrtoint);
    }
    else if (auto *intToPtr = dyn_cast<IntToPtrInst>(&I)) 
    {
        handleIntToPtr(*intToPtr);
    }
    else if (auto *freezeInst = dyn_cast<FreezeInst>(&I)) 
    {
        handleFreeze(*freezeInst);
    }
    else if (auto *extractValue = dyn_cast<ExtractValueInst>(&I)) 
    {
        handleExtractValue(*extractValue);
    }
    else if (auto *insertValue = dyn_cast<InsertValueInst>(&I)) 
    {
        handleInsertValue(*insertValue);
    }
    else if (auto *fcmp = dyn_cast<FCmpInst>(&I)) 
    {
        handleFCmp(*fcmp);
    }
    else if (auto *fpTrunk = dyn_cast<FPTruncInst>(&I)) 
    {
        handleFPTrunc(*fpTrunk);
    }
    else if (auto *extractElement = dyn_cast<ExtractElementInst>(&I)) 
    {
        handleExtractElement(*extractElement);
    }
    else if (auto *uiToFpInst = dyn_cast<UIToFPInst>(&I)) 
    {
        handleUIToFP(*uiToFpInst);
    }
    else if (auto *siToFpInst = dyn_cast<SIToFPInst>(&I)) 
    {
        handleSIToFP(*siToFpInst);
    }
    else if (auto *fnegInst = dyn_cast<UnaryOperator>(&I)) 
    {
        if (fnegInst->getOpcode() == Instruction::FNeg) 
        {
            handleFNeg(*fnegInst);
        }
        else
        {
            //errs() << "Unhandled UnaryOperator instruction: " << I.getOpcodeName() << "\n";
        }
    }
    else if (auto *fpExtInst = dyn_cast<FPExtInst>(&I)) 
    {
        handleFPExt(*fpExtInst);
    }
    else if (auto *fpToSiInst = dyn_cast<FPToSIInst>(&I)) 
    {
        handleFPToSI(*fpToSiInst);
    }
    else if (auto *fpToUiInst = dyn_cast<FPToUIInst>(&I)) 
    {
        handleFPToUI(*fpToUiInst);
    }
    else 
    {
        //errs() << "Unhandled instruction: " << I.getOpcodeName() << "\n";
    }
}