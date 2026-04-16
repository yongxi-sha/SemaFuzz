/***********************************************************
 * Author: Wen Li
 * Date  : 09/30/2024
 * Describe: symbolic execution for function summarization
 * History:
   <1>  09/30/2024, create
************************************************************/
#ifndef __SYM_SVFCORE_H__
#define __SYM_SVFCORE_H__
#include <filesystem>
#include "SVF-LLVM/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "Graphs/IRGraph.h"
#include "WPA/Andersen.h"
#include "WPA/WPAPass.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/Options.h"
#include "comgraph/DotParser.h"
#include "sym_debug.h"

using namespace llvm;
using namespace std;
using namespace SVF;

class LLVMValue
{
public:
    LLVMValue ()
    {

    }

    ~LLVMValue ()
    {
        
    }

private:
    
    inline std::string getConstantData(llvm::ConstantInt *constInt) 
    {
        llvm::APInt intValue = constInt->getValue();
        llvm::SmallVector<char, 16> strValue;
        intValue.toString(strValue, 10, true);
        return std::string(strValue.begin(), strValue.end());
    }


    inline std::string getGlobalData(llvm::GlobalVariable *gv) 
    {
        if (!gv->hasInitializer())
        {
            return "";
        }
        
        llvm::Constant *initializer = gv->getInitializer();
        if (llvm::ConstantDataArray *cda = llvm::dyn_cast<llvm::ConstantDataArray>(initializer))
        {
            if (cda->isString())
            {
                return cda->getAsString().str();
            }
            else
            {
                llvm::outs() << "Not string global.\n";
            }
        }
        else if (llvm::ConstantInt *constInt = llvm::dyn_cast<llvm::ConstantInt>(initializer))
        {
            return getConstantData(constInt);
        }
        else 
        {
            llvm::outs() << "Global variable has an unsupported initializer type.\n";
        }

        return "";
    }

    inline llvm::Value* getLLVMValue(llvm::Value* val)
    {
        if (llvm::Instruction *inst = llvm::dyn_cast<llvm::Instruction>(val)) 
        {
            if (llvm::LoadInst *loadInst = llvm::dyn_cast<llvm::LoadInst>(inst)) 
            {
                return getLLVMValue (loadInst->getPointerOperand());
            } 
            else if (llvm::GetElementPtrInst *gepInst = llvm::dyn_cast<llvm::GetElementPtrInst>(inst)) 
            {
                return getLLVMValue (gepInst->getOperand(0));
            }
            else if (llvm::PHINode *phiNode = llvm::dyn_cast<llvm::PHINode>(inst)) 
            {
                return getLLVMValue (phiNode->getIncomingValue(0));
            } 
            else 
            {
                llvm::outs() <<"\t**** Unsupported instruction: "<<inst->getOpcodeName()<<" --> "<<*inst<<" ****\r\n";
                return NULL;
            }
        }
        else if (llvm::Constant *constVal = llvm::dyn_cast<llvm::Constant>(val)) 
        {
            return constVal;
        } 
        else 
        {
            llvm::outs() <<"\t**** Neither a constant nor an instruction ****\r\n";
            return NULL;
        }
    } 

public:
    string getValue(llvm::Value *operand) 
    {
        llvm::Value* val = getLLVMValue (operand);
        if (val == NULL)
        {
            return "None";
        }

        // global value
        if (llvm::GlobalVariable *gv = llvm::dyn_cast<llvm::GlobalVariable>(val))
        {
            string GvValue = getGlobalData (gv);
            cout<<GvValue<<endl;
            return GvValue;
        }
        else if (llvm::ConstantInt *constVal = llvm::dyn_cast<llvm::ConstantInt>(val))
        {
            string CvValue = getConstantData (constVal);
            cout<<CvValue<<endl;
            return CvValue;
        }
        else
        {
            llvm::outs() << "Unsuported value type: ";
            val->print(llvm::outs());
        }

        return "None";
    }  
};

class SvfCore 
{
public:
    SvfCore (vector<string> &moduleNameVec)
    {
        initializeVFG (moduleNameVec);
    }

    ~SvfCore () 
    {
        delete vfg;
        AndersenWaveDiff::releaseAndersenWaveDiff();
        SVFIR::releaseSVFIR();

        LLVMModuleSet::getLLVMModuleSet()->dumpModulesToFile(".svf.bc");
        SVF::LLVMModuleSet::releaseLLVMModuleSet();

        llvm::llvm_shutdown();

        delete svfBuilder;
    }

public:
    SVFModule* svfModule;
    SVFIRBuilder *svfBuilder;
    SVFIR* svfPag;

    Andersen* ander;
    VFG* vfg;
    SVFG *svfg;
    PTACallGraph* callgraph;

private:
    inline void initializeVFG (vector<string> &moduleNameVec)
    {
        svfModule = LLVMModuleSet::buildSVFModule(moduleNameVec);

        unsigned fSize = filesystem::file_size(moduleNameVec[0]);
        if (fSize > 32*1024*1024)
        {
            ander = NULL;
            svfg  = NULL;
            vfg   = NULL;
            cout<<"@@initializeVFG:"<<moduleNameVec[0]<<" -> file size is " <<fSize/1024/1024<<" MB\r\n";
            return;
        }

        /// Build Program Assignment Graph (SVFIR)
        svfBuilder = new SVFIRBuilder (svfModule);
        svfPag     = svfBuilder->build();

        /// Create Andersen's pointer analysis
        ander = AndersenWaveDiff::createAndersenWaveDiff(svfPag);

        /// Call Graph
        callgraph = ander->getPTACallGraph();

        /// ICFG
        ICFG* icfg = svfPag->getICFG();
        DEBUG_PRINTF("icfg -> %p\r\n", icfg);

        /// Value-Flow Graph (VFG)
        //vfg = new VFG(callgraph);
        vfg = NULL;

        SVFGBuilder *svfgBuilder = new SVFGBuilder ();
        svfg = svfgBuilder->buildFullSVFG(ander);

        //svfg->dump("svfg.dot");
    }

    inline bool isWithinFunction (const llvm::Function *llvmFunc, const llvm::Value *llvmValue)
    {
        if (const llvm::Instruction *nodeInstr = llvm::dyn_cast<llvm::Instruction>(llvmValue)) 
        {
            if (nodeInstr->getFunction() != llvmFunc) return false;
        } 
        else if (const llvm::Argument *nodeArg = llvm::dyn_cast<llvm::Argument>(llvmValue)) 
        {
            if (nodeArg->getParent() != llvmFunc) return false;
        }

        return true;
    }

    inline const llvm::Argument* getReachedArg (const llvm::Function *llvmFunc, const llvm::Value* llvmValue)
    {
        for (const auto &funcArg : llvmFunc->args()) 
        {
            if (!funcArg.getType()->isPointerTy()) 
            {
                continue;
            }

            if (llvmValue == &funcArg) 
            {
                return &funcArg;
            }
        }

        return NULL;
    }

    inline SVFValue* getSVFValue (const llvm::Value *llvmVal)
    {
        return LLVMModuleSet::getLLVMModuleSet()->getSVFValue(llvmVal);
    }

    inline const llvm::Value * getLLVMValue (const SVFValue* svfValue)
    {
        return LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(svfValue);
    }

    inline const SVF::VFGNode* getSVFGNode (const llvm::Value *llvmVal)
    {
        const SVFValue* svfVal = getSVFValue (llvmVal);
        PAGNode* pagNode = svfPag->getGNode(svfPag->getValueNode(svfVal));
        if (pagNode == NULL)
        {
            return NULL;
        }

        return svfg->getDefVFGNode(pagNode);
    }

public:

    inline bool isActive ()
    {
        return (svfg != NULL);
    }

    inline const vector<Value*> getPointsTo(const Value* val) 
    {
        vector<Value*> ptsTo;

        if (ander == NULL)
        {
            return ptsTo;
        }

        PAG *pag = ander->getPAG();
        const SVFValue* svfVal = LLVMModuleSet::getLLVMModuleSet()->getSVFValue(val);
        NodeID nodeID = pag->getValueNode(svfVal);
        if (nodeID == 0) 
        {
            errs() << "Error: Invalid NodeID for value: " << *val << "\n";
            return ptsTo;
        }

        const PointsTo &ptsSet = ander->getPts(nodeID);
        for (SVF::NodeID pt : ptsSet) 
        {
            PAGNode* targetObj = pag->getGNode(pt);
            if(!targetObj->hasValue())
            {
                continue;
            }

            //errs() << "Points-to Node ID: " << pt << "\n";
            svfVal = targetObj->getValue();
            Value* ptValue = (Value*)LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(svfVal);
            if (ptValue) 
            {
                ptsTo.push_back (ptValue);
                //errs() << "Points-to Value: " << *ptValue << "\n";
            }
        }

        return ptsTo;
    }


    const llvm::Argument* getReachableArgument(const llvm::Instruction *Inst, const llvm::Value *Val) 
    {
        if (svfg == NULL)
        {
            return NULL;
        }

        const llvm::Function *curFunction = Inst->getFunction();
        if (!curFunction) 
        {
            return NULL;
        }

        const SVF::VFGNode *curNode = getSVFGNode (Val);
        if (curNode == NULL) 
        {
            return NULL;
        }

        std::set<const SVF::VFGNode *> visited;
        std::queue<const SVF::VFGNode *> worklist;
        worklist.push(curNode);

        while (!worklist.empty()) 
        {
            curNode = worklist.front();
            worklist.pop();

            if (visited.count(curNode)) continue;
            visited.insert(curNode);

            if (curNode->getValue() == NULL)
            {
                continue;
            }

            const llvm::Value *llvmValue = getLLVMValue (curNode->getValue());
            if (auto arg = getReachedArg (curFunction, llvmValue))
            {
                return arg;
            }

            if (!isWithinFunction (curFunction, llvmValue))
            {
                continue;
            }

            for (const auto &predEdge : curNode->getInEdges()) 
            {
                worklist.push(predEdge->getSrcNode());
            }
        }

        return NULL;
    }


    inline const SVFFunction* getSVFFunctionByName(const string& functionName) 
    {
        return svfModule->getSVFFunction(functionName);
    }


    inline void getCFGNodesbyFunctions (vector<string>& funcNames, vector<const ICFGNode*> &cfgNodes)
    {
        for (auto It = funcNames.begin (); It != funcNames.end (); It++)
        {
            string fName = *It;

            const SVFFunction* entryFunction = getSVFFunctionByName(fName);
            if (!entryFunction) 
            {
                cerr << "Entry function not found!" << endl;
                return;
            }

            PTACallGraphEdge::CallInstSet csSet;
            callgraph->getAllCallSitesInvokingCallee(entryFunction, csSet);

            for (const auto& callInst : csSet) 
            {
                const SVFInstruction* svfInst = callInst->getCallSite();
                if (svfInst == NULL)
                {
                    cerr << "Get svfInst from callsite failed!" << endl;
                    return;
                }

                ICFGNode* icfgNode = svfPag->getICFG()->getICFGNode (svfInst);
                if (icfgNode == NULL)
                {
                    cerr << "Get icfgNode from callsite failed!" << endl;
                    return;
                }

                cfgNodes.push_back (icfgNode);
            }
        }

        return;
    }

    inline PTACallGraphNode* getPTACgNodeByName (string funcName)
    {
        const SVFFunction* svfFunc = getSVFFunctionByName(funcName);
        if (svfFunc == NULL) 
        {
            cerr << "svfFunc not found: " << funcName << endl;
            return NULL;
        }

        PTACallGraphNode *cgNode = callgraph->getCallGraphNode (svfFunc);
        if (cgNode == NULL)
        {
            cerr << "PTACallGraphNode not found: " << funcName << endl;
            return NULL;
        }

        return cgNode;
    }

    inline Instruction* getLLVMInst (const ICFGNode* cfgNode)
    {
        const SVFInstruction *svfInst;
        if (IntraICFGNode::classof (cfgNode))
        {
            IntraICFGNode* intraNode = (IntraICFGNode*)cfgNode;
            svfInst = intraNode->getInst ();
        }
        else if (CallICFGNode::classof (cfgNode))
        {
            CallICFGNode* callNode = (CallICFGNode*)cfgNode;
            svfInst = callNode->getCallSite ();
        }
        else
        {
            assert ("Unsupport node type");
            return NULL;
        }

        return (Instruction*)LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(svfInst);
    }

    inline BranchInst* getLLVMBrInst (const ICFGNode* cfgNode)
    {
        Instruction* llvmInst = getLLVMInst (cfgNode);
        if (llvmInst == NULL)
        {
            return NULL;
        }

        if (!llvm::isa<llvm::BranchInst>(llvmInst))
        {
            return NULL;
        }

        // only consider conditional branch
        BranchInst* brInst = llvm::dyn_cast<llvm::BranchInst>(llvmInst);
        if (!brInst->isConditional()) 
        {
            return NULL;
        }

        return brInst;
    }


    inline bool isBrachInst (const ICFGNode* cfgNode)
    {
        BranchInst *brInst = getLLVMBrInst (cfgNode);
        if (brInst == NULL) 
        {
            return false;
            
        }

        //llvm::Value *condition = brInst->getCondition();
        //cout<<"# branch condition: "; condition->print(llvm::outs());

        return true;
    }

    inline void showConditions (llvm::Value *cond)
    {
        LLVMValue llvmVal;
        if (llvm::ICmpInst *icmp = llvm::dyn_cast<llvm::ICmpInst>(cond)) 
        {
            llvmVal.getValue (icmp->getOperand(0));
            llvmVal.getValue (icmp->getOperand(1));
        } else if (llvm::FCmpInst *fcmp = llvm::dyn_cast<llvm::FCmpInst>(cond)) 
        {
            llvmVal.getValue (fcmp->getOperand(0));
            llvmVal.getValue (fcmp->getOperand(1));
        }
    }
};





#endif