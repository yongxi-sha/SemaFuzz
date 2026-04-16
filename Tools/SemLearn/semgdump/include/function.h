#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#undef DEBUG
#ifdef __DEBUG__
#define DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG(format, ...) 
#endif

using namespace std;

class TrcFunction
{
public:
  string FuncName;
  unsigned funcId;

  TrcFunction (string FName, unsigned Id)
  {  
    funcId = Id;
    FuncName = FName;
  }

  ~TrcFunction () {}
};

class FuncIDs
{
public:
    FuncIDs (string FIDFile="functions_list"): FuncIDFile(FIDFile)
    {
        LoadFunctions ();
        GetPuredFuncName ();
    }

    ~FuncIDs ()
    {
        for (auto Itr = FID2FName.begin (); Itr != FID2FName.end (); Itr++)
        {
            TrcFunction* TF = Itr->second;
            delete TF;
        }
    }

    inline set<string>* getFuncSet ()
    {
        return &FuncSet;
    }

    inline string getFuncName (unsigned FID)
    {
        auto Itr = FID2FName.find (FID);
        if (Itr == FID2FName.end ())
        {
            return "";
        }
        else
        {
            TrcFunction* TF = Itr->second;
            return TF->FuncName;
        }
    }

    inline unsigned getFuncID (string puredName)
    {
        auto it = purName2Tfunc.find(puredName);
        if (it != purName2Tfunc.end()) 
        {
            TrcFunction* tf = it->second;
            return tf->funcId;
        }

        return 0;
    }

    inline string getFuncName (string puredName)
    {
        auto it = purName2Tfunc.find(puredName);
        if (it != purName2Tfunc.end()) 
        {
            TrcFunction* tf = it->second;
            return tf->FuncName;
        }

        return "";
    }

    inline unsigned getFuncNum ()
    {
        return (unsigned) FID2FName.size ();
    }

    inline bool isMainEntry (unsigned FuncId)
    {
        return (MainFunc.find (FuncId) != MainFunc.end ());
    }

private:
    string FuncIDFile;
    map<unsigned, TrcFunction*> FID2FName;
    map<string, TrcFunction*> purName2Tfunc;
    map<unsigned, string> MainFunc;
    set<string> FuncSet;

private:
    inline size_t GetSeqPos (const string& str, unsigned& SepLen)
    {
        vector<string> suffix = {"_c_", "_cpp_", "_cxx_", "_cc_"};

        for (auto&suf: suffix)
        {
            size_t Pos = str.find(suf);
            if (Pos != string::npos) 
            {
                SepLen = suf.size();
                return Pos;
            }
        }

        SepLen = 0;
        return string::npos;
    }   


    inline void GetPuredFuncName ()
    {
        for (auto Itr = FID2FName.begin (); Itr != FID2FName.end (); Itr++)
        {
            TrcFunction* TF = Itr->second;

            unsigned SepLen  = 0;
            string PuredName = "";
            string WholeName = TF->FuncName;

            size_t SepPos = GetSeqPos (WholeName, SepLen);
            if (SepPos == string::npos)
            {
                fprintf (stderr, "[GetPuredFuncName]: %s's format is incorrect!\r\n", WholeName.c_str());
                continue;
            }
            PuredName = WholeName.substr(SepPos + SepLen);

            purName2Tfunc.insert ({PuredName, TF});
            //cout<<"@@ --> PuredName function: " + TF->PuredName + " ---> Whole: "<<WholeName<<endl;
        }
    }


    bool IsMain (const string& str)
    {
        string FName = "";
        unsigned SepLen = 0;

        size_t SepPos = GetSeqPos (str, SepLen);
        if (SepPos == string::npos)
        {
            return false;
        }

        FName = str.substr(SepPos + SepLen);
        return (FName == "main");
    }

    vector<string> SplitString(const string& str, char delimiter) 
    {
        vector<string> tokens;
        string token;
        istringstream tokenStream(str);

        while (getline(tokenStream, token, delimiter)) 
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    void LoadFunctions ()
    {
        ifstream inputFile(FuncIDFile);
        if (!inputFile) {
            fprintf (stderr, "@@ FuncIDs file (%s) open fail!\r\n", FuncIDFile.c_str());
            exit (-1);
        }

        string FuncName;
        unsigned FuncID;

        while (getline(inputFile, FuncName)) {
            /* FuncName:ArgNum:BlkNum:ID*/
            vector<string> tokens = SplitString (FuncName, ':');
            if (tokens.size() < 4)
            {
                fprintf (stderr, "Incorrect format while parsing function list: %s\r\n", FuncName.c_str());
                continue;
            } 

            FuncName = tokens[0];
            FuncID   = (unsigned)stoi (tokens[3]);
            if (FID2FName.find (FuncID) != FID2FName.end())
                continue;
            FID2FName[FuncID] = new TrcFunction (FuncName, FuncID);
            FuncSet.insert (FuncName);
            //cout<<"@@ --> load function: " + FuncName + " with ID: "<<FuncID<<endl;

            if (IsMain (FuncName))
            {
                //cout<<"@@ --> Add main function: " + FuncName + " with ID: "<<FuncID<<endl;
                MainFunc[FuncID] = FuncName;
            }
        }

        inputFile.close();
    }
};

#endif