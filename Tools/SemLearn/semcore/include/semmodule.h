#ifndef __SEMMODULE_H__
#define __SEMMODULE_H__
#include <fstream>
#include "semfunc.h"
#include "json.hpp"

using json = nlohmann::json;

struct GlobalVariable 
{
    string name;
    string type;
    string value;
};

class ModuleSem 
{
public:
    ModuleSem () 
    {
        defSemJsonPath = "symbolic_summary.json";
    }

    ~ModuleSem () 
    {
        for (auto Itr = funcId2FuncSem.begin (); Itr != funcId2FuncSem.end (); Itr++)
        {
            delete Itr->second;
        }
    }   

private:
    string defSemJsonPath;
    unordered_map<unsigned, FuncSem*> funcId2FuncSem;
    typedef typename unordered_map<unsigned, FuncSem*>::iterator funcsem_iterator;

    unordered_map<string, GlobalVariable> globalsMap;
    typedef typename unordered_map<string, GlobalVariable>::iterator gv_iterator;
private:
    
    inline void addFunction(FuncSem* FSem) 
    {
        assert (FSem != NULL);
        funcId2FuncSem[FSem->FId] = FSem;

        return;
    }

    inline string extractArgOutput (const string& key, const string& value)
    {
        /* gep(arg0,0 + 17)@type(struct._xmlNode) -> @type(struct._xmlNode) */
        size_t startPos = value.find("@type(");
        if (startPos == std::string::npos) 
        {
            return key;
        }

        return key + value.substr(startPos);
    }


    inline void addToOutput (string& fOutput, string expr)
    {
        if (expr == "" || fOutput.find (expr) != string::npos)
        {
            return;
        }
            
        if (fOutput == "")
        {
            fOutput = expr;
        }
        else
        {
            fOutput += "|" + expr;
        }

        return;
    }

    inline void processFunction(const json& func) 
    {
        unsigned fid    = stoul(func["id"].get<string>());
        string fname    = func["name"];
        string finputs  = func["input"];
        string foutputs = "";
        string semantic = "";
        unsigned tokens = 0;

        if (func.find("semantic") != func.end()) 
        {
            semantic = func ["semantic"];
        }

        if (func.find("tokens") != func.end()) 
        {
            tokens = func ["tokens"];
        }
      
        vector<SymCallsite> fcalls;
        for (const auto& call : func["calls"]) 
        {
            string callee = call["callee"].get<string>();
            string expr   = call["expr"].get<string>();
            string conditions = call["conditions"].get<string>();

            fcalls.push_back (SymCallsite (callee, expr, conditions));
        }

        vector<IntraPath> paths;
        if (func.find("paths") != func.end())
        {
            for (const auto& path : func["paths"]) 
            {
                unordered_map<string, string> argument_outputs;
                if (!path["arg_outputs"].is_null()) 
                {
                    for (auto it = path["arg_outputs"].begin(); it != path["arg_outputs"].end(); ++it) 
                    {
                        argument_outputs[it.key()] = it.value();
                        string strOut = extractArgOutput (it.key(), it.value());
                        addToOutput (foutputs, strOut);
                    }
                }

                string retrunExpr = path["return"].get<string>();
                addToOutput (foutputs, retrunExpr);

                IntraPath p 
                {
                    argument_outputs,
                    path["conditions"].get<string>(),
                    retrunExpr
                };

                paths.push_back(p);
            }
        }

        addFunction(new FuncSem(fid, fname, finputs, foutputs, semantic, tokens, fcalls, paths));
        //printf ("@@[processFunction]: load function summary: %-60s\n", fname.c_str());
    }

    inline void processGlobal(const json& global) 
    {
        string name  = global["name"];
        string type  = global["type"];
        string value = global["value"];

        GlobalVariable globalVar {name, type, value};
        ModuleSem::globalsMap[name] = globalVar;

        //printf("@@[processGlobal]: Loaded global: Name: %-30s | Type: %-20s | Value: %-50s\n",
        //       name.c_str(), type.c_str(), value.c_str());
    }

    inline void getFilePathName (const string& fPath, string& dir, string& name)
    {
        size_t lastSlashPos = fPath.find_last_of('/');

        dir = (lastSlashPos != string::npos) ? fPath.substr(0, lastSlashPos) : "";
        name = (lastSlashPos != std::string::npos) ? fPath.substr(lastSlashPos + 1) : fPath;

        cout<<"@@getFilePathName: "<<fPath<<" -> dir = "<<dir<<", name = "<<name<<endl;
        return;
    }

    inline void test (const string input)
    {
        string newStr = setGlobalVarValue (input);
        cout<<"IN: "<<input<<", OUT: "<<newStr<<endl;
    }
    void unitTest ();

public:

    inline funcsem_iterator begin ()
    {
        return funcId2FuncSem.begin ();
    }

    inline funcsem_iterator end ()
    {
        return funcId2FuncSem.end ();
    }

    inline gv_iterator gv_begin ()
    {
        return globalsMap.begin ();
    }

    inline gv_iterator gv_end ()
    {
        return globalsMap.end ();
    }

    FuncSem* getFuncSem (const unsigned Id) const
    {
        auto Itr = funcId2FuncSem.find (Id);
        if (Itr == funcId2FuncSem.end ())
        {
            return NULL;
        }
        return Itr->second;
    }

    void print() 
    {
        for (auto Itr = this->begin (); Itr != this->end (); Itr++)
        {
            FuncSem* funcSum = Itr->second;
            funcSum->print ();
            cout << "---------------------------\n";
        }
    }

    inline string getGlobalValue (const string& gvName) const
    {
        auto It = globalsMap.find (gvName);
        if (It == globalsMap.end ())
        {
            return "";
        }

        GlobalVariable* gv = (GlobalVariable*) (&It->second);
        return gv->value;
    }

    void exportSemSummary (string outputFilePath="");
    void initModule(const string& semJsonPath);
    void mergeGlobalVar (const string& gvName, GlobalVariable globalVar);
    string setGlobalVarValue (const string& strExpr) const;
};

#endif