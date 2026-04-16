#ifndef __SEMG_DUMP_H__
#define __SEMG_DUMP_H__

#include <Queue.h>
#include <malloc.h>
#include <pthread.h>
#include <ctrace/Event.h>
#include <vector>
#include <string>
#include <set>
#include <stack>
#include <queue>
#include <cstdio>
#include <filesystem>
#include "function.h"
#include "comgraph/SemGraph.h"
#include "comgraph/DotParser.h"


#ifdef __MEMORY_STAT__

#define MEMORY_STAT_BEFORE \
    struct mallinfo mi_before = mallinfo()

    #define MEMORY_STAT_AFTER(Msg) \
    do { \
        struct mallinfo mi_after = mallinfo(); \
        int used_memory_before = mi_before.uordblks; \
        int used_memory_after = mi_after.uordblks; \
        int memory_cost = (used_memory_after - used_memory_before)/1024/1024; \
        if (memory_cost > 0) printf("%s -> Memory cost: %d Mbytes\n", Msg, memory_cost); \
    } while(0)

#else

#define MEMORY_STAT_BEFORE 

#define MEMORY_STAT_AFTER 

#endif


#undef DEBUG
#ifdef __DEBUG__
#define DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG(format, ...) 
#endif

using namespace std;

struct CallSeq
{
    CallSeq (unsigned K, unsigned ID): Key(K), FuncID(ID)
    {

    }

    unsigned Key;
    unsigned FuncID;
};

class CSBuf
{
public:
    typedef typename vector<CallSeq>::iterator csq_iterator;

    CSBuf (unsigned maxNum = 1024)
    {
        maxCatchNum = maxNum;
        createTime = time (NULL);
    }

    void AddCSeq (CallSeq CSeq)
    {
        CSeqs.push_back (CSeq);
    }

    csq_iterator begin ()
    {
        return CSeqs.begin ();
    }

    csq_iterator end ()
    {
        return CSeqs.end ();
    }

    inline void Clear ()
    {
        CSeqs.clear ();
        CSeqs.shrink_to_fit();
    }

    inline unsigned Size ()
    {
        return CSeqs.size ();
    }

    inline bool isUpBound (unsigned timeHold=10)
    {
        return  ((time (NULL) - createTime >= timeHold) || 
                 (CSeqs.size () >= maxCatchNum));
    }

private:
    unsigned maxCatchNum;
    unsigned createTime;
    vector<CallSeq> CSeqs;
};


template<class DataTy> class SafeQueue 
{
public:
    SafeQueue ()
    {
        unsigned Ret = pthread_mutex_init(&SfLock, NULL);
        if (Ret != 0)
        {
            fprintf (stderr, "@@ SafeQueue: pthread_mutex_init fail!\r\n");
            exit (-1);
        }
        popNum = 0;
    }

    ~SafeQueue ()
    {
        pthread_mutex_destroy(&SfLock);
    }

    void PushSfQueue(DataTy QData) 
    {
        pthread_mutex_lock (&SfLock);
        SfQueue.push(QData);
        pthread_mutex_unlock (&SfLock);
    }

    unsigned GetSfQueueSize ()
    {
        unsigned QueueSize;
        pthread_mutex_lock (&SfLock);
        QueueSize = SfQueue.size ();
        pthread_mutex_unlock (&SfLock);

        return QueueSize;
    }

    DataTy PopSfQueue() 
    {
        popNum++;
        DataTy QData;

        assert (!SfQueue.empty());
        pthread_mutex_lock (&SfLock);
        QData = SfQueue.front();
        SfQueue.pop();

        if (popNum%8092 == 0 && SfQueue.size () < 8092*3)
        {
            queue<DataTy>().swap(SfQueue);
        }
        pthread_mutex_unlock (&SfLock);

        return QData;
    }

private:
    unsigned popNum;
    queue<DataTy> SfQueue;
    pthread_mutex_t SfLock;
};


class DotQueue : public SafeQueue<string> 
{
public:
    DotQueue () {}
    ~ DotQueue () {}
private:

};


class CSeqQueue : public SafeQueue<CSBuf*> 
{
public:
    CSeqQueue () {}
    ~ CSeqQueue () {}
private:

};


class IsLandQueue : public SafeQueue<string> 
{
public:
    IsLandQueue () {}
    ~ IsLandQueue () {}
private:

};


/*
1 main

1 functionA
1 functionC
2 functionC
2 functionA

1 functionB
1 functionC
1 functionD
1 functionE
2 functionE
2 functionD
1 functionE
2 functionE
2 functionC
2 functionB

2 main
=====================================
digraph G {
    // Define the nodes
    main [label="main"];
    functionA [label="Function A"];
    functionB [label="Function B"];
    functionC [label="Function C"];
    functionD [label="Function D"];
    functionE [label="Function E"];

    // Define the edges
    main -> functionA;
    main -> functionB;
    functionA -> functionC;
    functionB -> functionC;
    functionC -> functionD;
    functionD -> functionE;
    functionC -> functionE;
}

*/

class SemgDump
{
public:
    DotQueue DtQueue;
    CSeqQueue CsQueue;

    IsLandQueue IsLandsQ;

    bool noSemanticLearn;
    unsigned lastPrintTime;
    unsigned lastLoggedTime;

public:
    SemgDump (string DotName, string FIDFile)
    {
        FIDs = new FuncIDs (FIDFile);
        if (FIDs == NULL)
        {
            fprintf (stderr, "@@SemgDump: FuncIDs init fail!\r\n");
            exit (-1);
        }
        cout<<"@@SemgDump: Load functions -> "<<FIDs->getFuncNum ()<<endl;

        GraphName = GetSemgName (DotName);

        DotFNo      = 0;
        MaxCacheDot = 16;

        noSemanticLearn = (getenv("NON_SEMANTIC_LEARNING") != NULL);
        cout<<"@@SemgDump: NON_SEMANTIC_LEARNING -> "<<noSemanticLearn<<endl;

        lastPrintTime = 0;
        lastLoggedTime = 0;

        cachedCsbufNum = 0;
        thresholdComputeIsland = 4096; // for efficiency consideration, we can not update graph for each execution

        subGraphNo = 0;

        dirSubgraphs = "subGraphs";
        createDirectory (dirSubgraphs);
    }

    ~SemgDump ()
    {
        delete FIDs;
    }

    inline string GetFuncName (unsigned FuncID)
    {
        return FIDs->getFuncName (FuncID);
    }

    inline void InitSemgWithDot (string DotFile)
    {
        SemgDotParser semDtp (DotFile);
        semDtp.Dot2Graph (semg);

        cout<<"@@InitSemgWithDot: Load call graph with nodes: "<<semg.GetNodeNum()<<" and edges: "<<semg.GetEdgeNum ()<<endl;

        if (semDtp.GetDotType () == DOT_SVF)
        {
            UpdateNodeName ();
        }

        cout<<"@@InitSemgWithDot: done!"<<endl;

        return;
    }

    void ConvertDot (string DotFile)
    {
        InitSemgWithDot (DotFile);
        semg.ComputeSubNodesAll ();

        string TempName = GraphName + "_convert";
        SemgViz GV (GraphName, &semg, TempName);

        string ExtLabel = GetGraphLabel ();
        GV.SetNodeLabel (ExtLabel);

        GV.WiteGraph ();

        return;
    }

    void createDirectory(const string& path) 
    {
        try 
        {
            filesystem::create_directory(path);
        } 
        catch (const std::filesystem::filesystem_error& e) {}

        return;
    }

    inline void OutputIslandsDot (vector<vector<SemgNode*>*>& IsLands)
    {
        unsigned IslandNo = 0;
        for (auto Itrs = IsLands.begin(); Itrs != IsLands.end(); Itrs++)
        {
            vector<SemgNode*> *island = *Itrs;

            string DotName = GenDotName (*island);
            if (analyzedSubGraphs.find (DotName) == analyzedSubGraphs.end ())
            {
                analyzedSubGraphs.insert (DotName);
                DotName = dirSubgraphs + "/" + std::to_string(subGraphNo) + "_" + DotName;

                SemgViz GV ("CallGraph", &semg, DotName, island);
                string ExtLabel = "Node Count: " + to_string (island->size());
                GV.SetNodeLabel (ExtLabel);
                GV.WiteGraph ();

                IslandNo++;
                delete island;

                IsLandsQ.PushSfQueue (DotName + ".dot");
                subGraphNo++;
            }    
        }
    }

    string GenDot (CSBuf *CSB)
    {
        semg.ResetChanged ();

        MEMORY_STAT_BEFORE;
        UpdateSemGraph (CSB);
        MEMORY_STAT_AFTER (("@@ CallGraph Edge process with CSB size: " + to_string (CSB->Size())).c_str());

        UpdateGraphAttr ();

        string ExtLabel = GetGraphLabel ();
        logFunctionCov (ExtLabel);

        if (noSemanticLearn == true)
        {
            return "None";
        }

        cachedCsbufNum++;
        string TempName = "None";
        if (cachedCsbufNum >= thresholdComputeIsland /*semg.IsChanged ()*/)
        {
            cachedCsbufNum = 0;

            vector<vector<SemgNode*>*> IsLands;
            semg.ComputeIslands (IsLands, FIDs->getFuncSet());
            OutputIslandsDot (IsLands);
            IsLands.clear();
            IsLands.shrink_to_fit();

            #if 0
            TempName = GraphName + to_string (DotFNo);
            SemgViz GV (GraphName, &semg, TempName);
            GV.SetNodeLabel (ExtLabel);
            GV.WiteGraph ();

            if (DtQueue.GetSfQueueSize () > MaxCacheDot)
            {
                string OldDot = DtQueue.PopSfQueue ();
                remove (OldDot.c_str());
            }
            DtQueue.PushSfQueue (TempName+".dot");
            #endif

            if (IsLandsQ.GetSfQueueSize () == 0)
            {
                //analyzedSubGraphs.clear ();
            }
        }
        DotFNo++;

        return TempName;
    }

    inline bool IsProgramEntry (unsigned FId)
    {
        return FIDs->isMainEntry (FId);
    }

    inline void ShowCSeqs (CSBuf *CSB, string DotName)
    {
        if (DotName == "")
        {
            return;
        }

        string SeqFName = DotName + "_" + "callseqs.txt";
        FILE *F = fopen (SeqFName.c_str(), "w");
        for (auto Itr = CSB->begin (); Itr != CSB->end (); Itr++) 
        {
            CallSeq CSq = *Itr;
            if (CSq.Key == TARGET_BEGIN_KEY)
                fprintf (F, "%s start\r\n", GetFuncName (CSq.FuncID).c_str());

            if (CSq.Key == TARGET_END_KEY)
                fprintf (F, "%s end\r\n", GetFuncName (CSq.FuncID).c_str());
        }
        fclose (F);
    }

private:
    string GraphName;
    string dirSubgraphs;

    FuncIDs *FIDs;
    SemGraph semg;

    unsigned DotFNo;
    unsigned MaxCacheDot;

    unsigned cachedCsbufNum;
    unsigned thresholdComputeIsland;

    unsigned subGraphNo;

    set<string> analyzedSubGraphs;

private:
    inline string computeHash(const vector<SemgNode*>& island)
    {
        unsigned long hash = 5381;

        for (auto* node : island)
        {
            const string name = node->GetFName();  
            for (char ch : name)
            {
                hash = ((hash << 5) + hash) + static_cast<unsigned char>(ch);
            }
        }
        
        ostringstream oss;
        oss << hex << hash;
    
        return oss.str();
    }

    string GenDotName (const vector<SemgNode*>& island) 
    {
        SemgNode* firstNode = island[0];
        string label = firstNode->GetFName ().substr (0, 64);

        string baseName = "IsLand_" + to_string (island.size()) + "_SemG_" + computeHash (island) + "_" + label;
        return baseName;
    }

    std::string GetTimeString ()
    {
        std::time_t now = std::time(nullptr);
        char timeBuffer[32];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d_%H:%M:%S", std::localtime(&now));

        return std::string(timeBuffer);
    }

    inline void logFunctionCov (const std::string& content, const std::string& filename="func_cov.txt")
    {
        std::time_t now = std::time(nullptr);
        string logInfo = "[" + GetTimeString() + "] " + content;
        
        // to terminal
        if (now - lastPrintTime > 10)
        {
            cout<<logInfo<<endl;
            cout<<"\t[SgmtDump]cachedCsbufNum: "<<cachedCsbufNum<<", CsQueue: "<<CsQueue.GetSfQueueSize ()<<", IsLandsQ: "<<IsLandsQ.GetSfQueueSize ()<<"\n";
            lastPrintTime = now;
        }
        
        // to log
        if (now - lastLoggedTime > 60)
        {
            std::ofstream file(filename, std::ios::app);
            if (!file.is_open()) {
                std::cerr << "Error: Unable to open file: " << filename << std::endl;
                return;
            }

            file << logInfo << std::endl;
            file.close();
            lastLoggedTime = now;
        }  
    }

    inline string GetGraphLabel ()
    {
        unsigned TotalFuncNum = semg.GetNodeNum ();
        unsigned HitedFuncNum = semg.GetHitNodeNum ();

        string ExtLabel = "Total Functions: " + to_string (TotalFuncNum) + ", Hited Functions: " + to_string (HitedFuncNum) +\
                          " [ " + to_string (HitedFuncNum*100.0/TotalFuncNum) + "% ]" +\
                          ", Hited Edges: " + to_string (semg.GetHitEdgeNum ()) + ", Total Edges: " + to_string (semg.GetEdgeNum ());
        //cout<<ExtLabel<<endl;
        return ExtLabel;
    }

    inline void UpdateNodeName ()
    {
        unsigned nodeNum = semg.GetNodeNum();
        unsigned index = 0;

        semg.ResetNodeMap ();
        for (auto Itr = semg.begin(); Itr != semg.end(); Itr++)
        {
            SemgNode* sgNode = Itr->second;
            index++;

            string WFName = FIDs->getFuncName (sgNode->GetFName ());
            if (WFName == "")
            {
                //fprintf (stderr, "[UpdateNodeName] we can not get the whole function name: %s\r\n", Cn->GetFName ().c_str());
                continue;
            }

            sgNode->UpdateFuncName (WFName);
            semg.UpdateNodeMap (WFName, sgNode);
            printf ("@@UpdateNodeName --> %-8u/%-8u\r", index, nodeNum);
        }
        printf ("\n");
    }


    inline string GetSemgName (string DotFile) 
    {
        size_t pos = DotFile.find_last_of(".");
        if (pos != string::npos) 
        {
            return DotFile.substr(0, pos);
        }
        else
        {
            return DotFile;
        }
    }

    inline void UpdateGraphAttr ()
    {
        for (auto Itr = semg.begin(); Itr != semg.end(); Itr++)
        {
            SemgNode* sgNode = Itr->second;
            if (sgNode->Hot == true)
            {
                sgNode->Hot = false;
                sgNode->UpdateAttr (E_ATTR_COLOR_RED);
            }
            else
            {
                sgNode->UpdateAttr ();
            }
        }
    }

    inline void UpdateSemGraph (CSBuf *CSB)
    {
        for (auto Itr = CSB->begin (); Itr != CSB->end (); Itr++) 
        {
            CallSeq CSq = *Itr;

            string FuncName = GetFuncName (CSq.FuncID);
            if (FuncName == "")
            {
                continue;
            }
            
            SemgNode* sgNode = semg.GetNode (FuncName);
            //printf ("(CSq.FuncName = %s, sgNode = %p\r\n", CSq.FuncName.c_str(), sgNode);
            if (sgNode == NULL)
            {  
                sgNode = semg.AddNode (FuncName);
                if (sgNode == NULL)
                {
                    fprintf (stderr, "[UpdateCallGraph] we can not generate new Call-graph node: %s\r\n", FuncName.c_str());
                    exit (0);
                }
            }

            if (sgNode->HitNum == 0)
            {
                semg.SetChanged ();
            }
            
            sgNode->Hot = true;
            sgNode->HitNum += (unsigned)(CSq.Key == TARGET_BEGIN_KEY);
        }

#if 0
        stack<unsigned> CallStack;

        // Parse the call sequence
        for (auto Itr = CSB->begin (); Itr != CSB->end (); Itr++) 
        {
            CallSeq CSq = *Itr;
            string function = GetFuncName (CSq.FuncID);

            if (CSq.Key == TARGET_BEGIN_KEY) 
            {   // Function call
                if (!CallStack.empty()) 
                {
                    string caller = GetFuncName(CallStack.top());

                    SemgNode *S = semg.GetNode (caller);
                    SemgNode *E = semg.GetNode (function);

                    SemgEdge *Edge = new SemgEdge (S, E);
                    assert (Edge != NULL);

                    bool SuccEdge = semg.AddEdge (Edge);
                    if (SuccEdge == true) 
                    {
                        cout<<"\t@@UpdateSemGraph: add new edge from "<<S->GetFName ()<<" to "<<E->GetFName ()<<"\n";
                        Edge->HitNum++;
                    }
                    else
                    {
                        SemgEdge *ExEdge = S->GetOutgoingEdge (Edge);
                        if (ExEdge == NULL)
                        {
                            fprintf (stderr, "[UpdateCallGraph] ExEdge should not be NULL: %s -> %s\r\n", caller.c_str(), function.c_str());
                            exit (0);

                        }
                        ExEdge->HitNum++;

                        delete Edge;
                    }
                }
                CallStack.push(CSq.FuncID);
            } 
            else if (CSq.Key == TARGET_END_KEY) 
            {   // Function return
                if (!CallStack.empty() && CallStack.top() == CSq.FuncID) 
                {
                    CallStack.pop();
                }
            }
            else
            {
                assert (0);
            }
        }
#endif
        return;
    }
};

#endif