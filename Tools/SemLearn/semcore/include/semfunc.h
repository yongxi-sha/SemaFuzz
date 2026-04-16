#ifndef __SEMFUNC_H__
#define __SEMFUNC_H__
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "seminst.h"

using namespace std;

class SemTest;

struct IntraPath 
{
    unordered_map<string, string> argOutputs; 
    string pathConditions;
    string returnExpr;

    IntraPath(const unordered_map<string, string>& argOutput,
              const string& pathConds,
              const string& retExpr)
        : argOutputs(argOutput), pathConditions(pathConds), returnExpr(retExpr) 
    {
        
    }
};

struct SymCallsite
{
    string callees;
    string calleeExpr;
    string callConditions;

    SymCallsite (string calls, string cexpr, string considitons) 
    {
        callees    = calls;
        calleeExpr = cexpr;
        callConditions = considitons;
    }

    inline vector<string> splitCallees(const string& str, char delimiter='|') const
    {
        vector<string> tokens;
        string token;
        istringstream tokenStream(str);

        while (std::getline(tokenStream, token, delimiter)) 
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    bool isEqualtoCallee (string calleeName) const
    {
        vector<string> allCallees = splitCallees (callees);

        for (auto& callee: allCallees)
        {
            if (callee == calleeName)
            {
                return true;
            }
        }

        return false;
    }

    
};

class FuncSem 
{
public:
    unsigned FId;
    string FName;
    string FInputs;
    string FOutputs;
    string Semantics;
    unsigned Tokens;

    vector<SymCallsite> FCalls;
    vector<IntraPath> Paths;

    // Constructor
    FuncSem(const unsigned id,
            const string& name,
            const string& inputs,
            const string& outputs,
            const string& semantic,
            const unsigned tokens,
            const vector<SymCallsite>& calls,
            const vector<IntraPath>& p)
        : FId(id), FName(name), FInputs(inputs), FOutputs(outputs), Semantics(semantic), Tokens(tokens), FCalls(calls), Paths(p)
    {
        parseCalls ();
    }

    const SymCallsite* getSymCallsite (const string callseeName) const
    {
        for (auto& symCs: FCalls)
        {
            if (symCs.isEqualtoCallee(callseeName))
            {
                return &symCs;
            }
        }

        return NULL;
    }

    void print() const
    {
        cout << "Function ID: " << FId << "\n";
        cout << "Function Name: " << FName << "\n";
        cout << "Function Calls: ";
        for (const auto& call : FCalls) 
        {
            cout << call.callees << ": "<<call.callConditions<<endl;
        }

        cout << "\nPaths:\n";
        for (const auto& path : Paths) 
        {
            cout << "  Argument Outputs: "<<endl;
            for (const auto& [key, value] : path.argOutputs) 
            {
                cout <<"\t"<< key << ": " << value << "\n";
            }
            cout << "  Path Conditions: " << path.pathConditions << "\n";
            cout << "  Return Expression: " << path.returnExpr << "\n";
        }
    }

private:
    inline void parseCalls ()
    {
        for (auto &call: FCalls)
        {
            string res = InstSem::getInstSem (call.calleeExpr);
            //cout<<"@@@parseCalls: "<<call<<" --> "<<res<<endl;
        }
    }
};


class SemTest
{
private:
    static inline void assert (string output, string groundtruth)
    {
        if (output == groundtruth)
        {
            cout<<"Passed: "<<groundtruth<<endl;
        }
        else
        {
            cout<<"Failed: "<<groundtruth<<" ---> "<<output<<endl;
        }
    }

    static inline void callTest ()
    {
        assert (InstSem::getInstSem ("call_xmlHashAddEntry2((call_xmlHashCreate(0)|gep(arg0, 10)),gep(.str.22.4130),null,xmlXPathFalseFunction)"), 
                "xmlHashAddEntry2(xmlHashCreate,arg0,.str.22.4130)");

        assert (InstSem::getInstSem ("call_iconv_close(gep(arg0, 3))"), 
                "iconv_close(arg0)");

        assert (InstSem::getInstSem ("call_fprintf(stdout,gep(.str, 0),arg1)"), 
                "fprintf(stdout,.str,arg1)");

        assert (InstSem::getInstSem ("call_fwrite(gep(.str.1, 0),2,1,stdout)"), 
                "fwrite(.str.1,stdout)");

        assert (InstSem::getInstSem ("call_strcmp(gep(arg1, sext(1), sext(((1+1)|1))),gep(.str.79, 0))"), 
                "strcmp(arg1,.str.79)");

        assert (InstSem::getInstSem ("(call_myMallocFunc|call_malloc)(8)"), 
                "myMallocFunc(),malloc()");

        assert (InstSem::getInstSem ("call_llvm.memset.p0i8.i64(gep((call_myMallocFunc|call_malloc)(56), 40),0,16,0)"), 
                "llvm.memset.p0i8.i64(myMallocFunc(),malloc())");

        assert (InstSem::getInstSem ("call_xmlFreeDoc((call_xmlCopyDoc(call_xmlNewDoc(gep(.str.363, 0)),1)|call_xmlNewDoc(gep(.str.363, 0))))"), 
                "xmlFreeDoc(xmlCopyDoc,xmlNewDoc,.str.363)");

        assert (InstSem::getInstSem ("(call_myFreeFunc|call_free)(gep(xmlCharEncodingAliases, 0, ((0+1)|0)))"), 
                "myFreeFunc(xmlCharEncodingAliases),free(xmlCharEncodingAliases)");

    }

public:
    static inline void semTest ()
    {
        callTest ();
    }
    
};

#endif