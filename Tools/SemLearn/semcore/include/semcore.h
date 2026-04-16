#ifndef __SEMCORE_H__
#define __SEMCORE_H__
#include <queue>
#include <set>
#include <random>
#include <limits>
#include <dlfcn.h>
#include <filesystem>
#include "semmodule.h"
#include "semdebug.h"
#include "comgraph/SemGraph.h"
#include "json.hpp"

/*
{
    "edges": [
        { "call": "func1 -> func2", "condition": "con1 == true" },
        { "call": "func2 -> func3", "condition": "con2 == true" },
        { "call": "func3 -> func4", "condition": "con3 == true && con4 != false" }
    ]
}
*/
using json = nlohmann::json;
struct SemsPath
{
    string graphID;
    string pathDir;
    vector<SemgNode*> callPath;

    SemsPath (vector<SemgNode*>& cpath, string curSubgraphID)
    {
        callPath = cpath;
        pathDir  = "paths";
        graphID  = curSubgraphID;
        createDirectory (pathDir);
    }

    string computeSems (const ModuleSem& mdSem) const
    {
        json pathJson;
        pathJson["nodes"] = getNodes (mdSem);
        //pathJson["edges"] = getEdges (mdSem);
        pathJson["callpath"] = getCallPath (mdSem);
        
        //cout << "@@computeSems: " << pathJson.dump(4) << endl;

        string outputFile = getFileName ();
        writeJson (outputFile, pathJson);

        return outputFile;
    }

private:
    void createDirectory(const string& path) 
    {
        try 
        {
            filesystem::create_directory(path);
        } 
        catch (const std::filesystem::filesystem_error& e) {}

        return;
    }

    inline string convertAllTypeSyntax(string typeStr) const
    {
        size_t pos = 0;
        string convertStr = typeStr;
        const std::string typePrefix = "@type(";
        
        while ((pos = convertStr.find(typePrefix, pos)) != string::npos) 
        {
            size_t start = pos + typePrefix.length();
            size_t end = convertStr.find(')', start);
            
            if (end == string::npos) {
                break;
            }

            string extractedType = convertStr.substr(start, end - start);
            convertStr.replace(pos, (end - pos) + 1, "@" + extractedType);
            if (convertStr.size () >= 128)
            {
                break;
            }
            
            pos += extractedType.length() + 1;
        }
        
        return convertStr;
    }

    inline json getNodes (const ModuleSem& mdSem) const
    {
        json allNodes = json::array();
        for (size_t i = 0; i < callPath.size(); ++i) 
        {
            json node;

            SemgNode* curNode  = callPath[i];
            FuncSem* fSem = mdSem.getFuncSem (curNode->GetId ());
            if (fSem != NULL)
            {
                node = 
                {
                    { "node", fSem->FName},
                    { "in", convertAllTypeSyntax(fSem->FInputs)},
                    { "out", convertAllTypeSyntax(mdSem.setGlobalVarValue (fSem->FOutputs))}
                };

                if (!fSem->Semantics.empty()) 
                {
                    node["semantic"] = fSem->Semantics;
                }
            }
            else
            {
                node = {{ "node", curNode->GetFName ()}};
            }
  
            allNodes.push_back(node);
        }

        //cout << allNodes.dump(4) << endl;
        return allNodes;
    }

    string extractFunctionCalls (const string& input) const
    {
        set<string> functionNames;
        size_t pos = 0;

        while ((pos = input.find('(', pos)) != string::npos) 
        {
            size_t funcStart = pos;
            while (funcStart > 0 && (isalnum(input[funcStart - 1]) || input[funcStart - 1] == '_')) 
            {
                --funcStart;
            }

            if (funcStart < pos) 
            {
                functionNames.insert(input.substr(funcStart, pos - funcStart));
            }

            pos++;
        }

        string result;
        int i = 0;
        for (auto it = functionNames.begin (); it != functionNames.end (); it++)
        {
            if (i >= 4) break;
            if (i > 0) result += "|"; 
            result += *it;
            i++;
        }

        return result;
    }

    inline string getPathCondition (const ModuleSem& mdSem, unsigned callerId, string callee) const
    {
        FuncSem* callerSem = mdSem.getFuncSem (callerId);
        if (callerSem == NULL)
        {
            return "";
        }

        for (auto& cs: callerSem->FCalls)
        {
            if (cs.isEqualtoCallee (callee))
            {
                string callCon = extractFunctionCalls (cs.callConditions);
                return mdSem.setGlobalVarValue (callCon);
            }
        }

        return "";
    }

    inline json getEdges (const ModuleSem& mdSem) const
    {
        json allEdges = json::array();
        for (size_t i = 0; i < callPath.size() - 1; ++i) 
        {
            SemgNode* curNode  = callPath[i];
            SemgNode* nextNode = callPath[i + 1];

            string callExpr = curNode->GetFName () + " -> " + nextNode->GetFName ();
            json edge = 
            {
                { "call", callExpr }/*,
                { "condition", getPathCondition (mdSem, curNode->GetId (), nextNode->GetFName ())}*/
            };
            allEdges.push_back(edge);
        }

        //cout << allEdges.dump(4) << endl;
        return allEdges;
    }

    inline string getCallPath (const ModuleSem& mdSem) const
    {
        string path = "";
        for (size_t i = 0; i < callPath.size(); ++i) 
        {
            SemgNode* curNode  = callPath[i];
            if (path == "")
            {
                path = curNode->GetFName ();
            }
            else
            {
                path += " -> " + curNode->GetFName ();
            }
        }

        return path;
    }

    string getFileName () const
    {
        ostringstream oss;
        for (size_t i = 0; i < callPath.size(); ++i) 
        {
            SemgNode* curNode  = callPath[i];

            if (i > 0) 
            {
                oss << "_";
            }
            oss << curNode->GetId ();
        }
        return pathDir + "/" + graphID + "_" + oss.str() + ".json";
    }

    void writeJson (const string &outputFile, json& outputJson) const
    {
        std::ofstream file(outputFile, std::ios::out | std::ios::trunc);
        if (!file.is_open())
        {
            std::cout<<"@@writeJson: "<<outputFile<<" open failed"<<endl;
            return;
        }

        file << outputJson.dump(4) << "\n";
        file.close(); 
    }
};


struct SemStat
{
    unsigned numCgNodes;
    unsigned numCgEdges;

    unsigned minPathLength;
    unsigned maxPathLength;
    unsigned meanPathLength;
    double stdDevPathLength;
    unsigned nodeCoverage;
    unsigned totalPaths;

    void computeStat(SemGraph& subCG, vector<SemsPath>& allSemsPaths) 
    {
        numCgNodes = subCG.GetNodeNum();
        numCgEdges = subCG.GetEdgeNum();

        minPathLength = numeric_limits<unsigned>::max();
        maxPathLength = 0;
        meanPathLength = 0;
        stdDevPathLength = 0.0;
        nodeCoverage = 0;
        totalPaths = allSemsPaths.size();

        if (allSemsPaths.empty()) 
        {
            minPathLength = 0;
            meanPathLength = 0;
            stdDevPathLength = 0.0;
            return;
        }

        unsigned totalPathLength = 0;
        unordered_set<SemgNode*> coveredNodes;

        for (const SemsPath& spath : allSemsPaths) 
        {
            unsigned pathLength = spath.callPath.size();

            minPathLength = min(minPathLength, pathLength);
            maxPathLength = max(maxPathLength, pathLength);

            totalPathLength += pathLength;

            for (SemgNode* node : spath.callPath)
            {
                coveredNodes.insert(node);
            }
        }

        meanPathLength = totalPathLength / totalPaths;

        double variance = 0.0;
        for (const SemsPath& spath : allSemsPaths) 
        {
            unsigned pathLength = spath.callPath.size();
            variance += std::pow(pathLength - meanPathLength, 2);
        }
        stdDevPathLength = std::sqrt(variance / totalPaths);

        nodeCoverage = static_cast<unsigned>((static_cast<double>(coveredNodes.size()) / numCgNodes) * 100);
    }

    void showStat() const 
    {
        cout << "==================  SemStat ==================" << endl;
        cout << "Graph Nodes         : " << numCgNodes << endl;
        cout << "Graph Edges         : " << numCgEdges << endl;
        cout << "Sampled Paths       : " << totalPaths << endl;
        cout << "Min Path Length     : " << minPathLength << endl;
        cout << "Max Path Length     : " << maxPathLength << endl;
        cout << "Mean Path Length    : " << meanPathLength << endl;
        cout << "StdDev Path Length  : " << stdDevPathLength << endl;
        cout << "Node Coverage       : " << nodeCoverage << "%" << endl;
        cout << "==============================================" << endl;

        writeLog ();
    }

    void writeLog (string filePath="path_explore_stat.log") const
    {
        ofstream file(filePath, ios::app);
        if (!file.is_open()) 
        {
            cerr << "Error: Unable to open file: " << filePath << endl;
            return;
        }

        string logInfo = "node:" + to_string (numCgNodes) + "," +\
                         "edge:" + to_string (numCgEdges) + "," +\
                         "path:" + to_string (totalPaths) + "," +\
                         "min-pl:" + to_string (minPathLength) + "," +\
                         "max-pl:" + to_string (maxPathLength) + "," +\
                         "mean-pl:" + to_string (meanPathLength) + "," +\
                         "stddev-pl:" + to_string (stdDevPathLength) + "," +\
                         "node-cov:" + to_string (nodeCoverage) + "%";

        file << logInfo << endl;
        file.close();
    }
};


class SemLearner
{
public:
    SemLearner() {}
    SemLearner(const string& symSumPath, const string& cgDotPath)
    {
        init (symSumPath, cgDotPath);
    }

    ~SemLearner () {}

    friend class PathExplorationTest; // Declare the test cl
    friend class PythonAPITest;
    friend class InitSubGraphTest;
private:
    ModuleSem mdSem;

    SemGraph wholeSemg;

    unsigned semThreshold;
    unsigned sampledPathNum;
    unsigned randomGenPath;

    vector<SemStat> semStats;

    string curSubgraphID;

private:
    inline bool initSemGraph (SemGraph& semg, const string& dot)
    {
        SemgDotParser semDtp (dot);
        semDtp.Dot2Graph (semg);
        if (semg.GetNodeNum () == 0)
        {
            return false;
        }

        if (semDtp.GetDotType () == DOT_AIF)
        {
            UpdateSemgNodeName (semg);
        }

        return true;
    }

    inline void initSemantics ()
    {
        // update function-level-semantics
        for (auto Itr = mdSem.begin (); Itr != mdSem.end (); Itr++)
        {
            FuncSem* FSem = Itr->second;

            // first, update the node ID with function ID
            //cout<<"@@@initSemantics: update function "<<FSem->FName<<" with ID: "<<FSem->FId<<endl;
            wholeSemg.UpdateNodeId (FSem->FName, FSem->FId);

            // second, update the node (function) semantics
            SemgNode *smgNode = wholeSemg.GetGNode (FSem->FId);
            if (smgNode == NULL)
            {
                cout<<"@@initSemantics: smgNode get failed -> "<<FSem->FName<<endl;
                exit (0);
            }
            smgNode->Input  = FSem->FInputs;
            smgNode->Output = FSem->FOutputs;
        }

        //third, update call conditions (edge)
        for (auto Itr = wholeSemg.begin (); Itr != wholeSemg.end (); Itr++)
        {
            SemgNode* smgNode = Itr->second;
            const FuncSem* curFSem = mdSem.getFuncSem (smgNode->GetId());
            if (curFSem == NULL)
            {
                //cout<<"@@initSemantics: curFSem get failed -> "<<smgNode->GetId()<<": "<<smgNode->GetFName ()<<endl;
                continue;
            }

            for (auto it = smgNode->OutEdgeBegin(), eit = smgNode->OutEdgeEnd(); it != eit; ++it)
            {
                SemgEdge* edge = *it;
                SemgNode* nxtNode = edge->GetDstNode();

                const SymCallsite* symCs = curFSem->getSymCallsite (nxtNode->GetFName());
                if (symCs != NULL && symCs->callConditions != "")
                {
                    edge->Conditions = symCs->callConditions;
                    //cout<<"@@computeSems: callee = "<<symCs->callees<<", conditions = "<<symCs->callConditions<<endl;
                }
            }
        } 
    }

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

    inline void UpdateSemgNodeName (SemGraph &semg)
    {
        semg.ResetNodeMap ();
        for (auto Itr = semg.begin(); Itr != semg.end(); Itr++)
        {
            SemgNode* smgNode = Itr->second;

            string FName = smgNode->GetFName ();

            unsigned SepLen  = 0;
            size_t cPos = GetSeqPos (FName, SepLen);
            if (cPos == string::npos)
            {
                fprintf (stderr, "[UpdateSemgNodeName]: %s's format may be incorrect!\r\n", FName.c_str());
                continue;
            }

            string NewFName = FName.substr (cPos+SepLen);
            DEBUG_PRINTF("Update function name: %s -> %s\r\n", FName.c_str(), NewFName.c_str());

            smgNode->UpdateFuncName (NewFName);
            semg.UpdateNodeMap (NewFName, smgNode);
        }
    }

    inline void addToStat (SemGraph &subCG, vector<SemsPath>& allSemsPaths)
    {
        SemStat semSt;
        semSt.computeStat (subCG, allSemsPaths);
        semSt.showStat ();

        semStats.push_back (semSt);
        return;
    }

    inline void getSubSemgEntries (SemGraph& subCG, vector<SemgNode*>& subCgEntries)
    {
        for (auto Itr = subCG.begin (); Itr != subCG.end (); Itr++)
        {
            SemgNode* subNode = Itr->second;
            //cout<<node->GetFName ()<<" -> "<<node->GetIncomingEdgeNum ()<<endl;
            if (subNode->GetIncomingEdgeNum () == 0)
            {
                subCgEntries.push_back (subNode);
            }
        }

        return;
    }

    inline void getNeighbors (SemgNode* smgNode, vector<SemgNode*>& neighbors)
    {
        for (auto it = smgNode->OutEdgeBegin(), eit = smgNode->OutEdgeEnd(); it != eit; ++it) 
        {
            SemgEdge* edge = *it;
            SemgNode* nxtNode = edge->GetDstNode();

            neighbors.push_back(nxtNode);
        }
    }

    inline bool isPathExist (const vector<SemsPath>* allSemsPaths, const vector<SemgNode*>* path) 
    {
        for (const SemsPath& spath : *allSemsPaths) 
        {
            if (spath.callPath.size() >= path->size() &&
                equal(path->begin(), path->end(), spath.callPath.begin()))
            {
                return true;
            }
            
        }
        return false;
    }

    inline bool isLoopPath (vector<SemgNode*> spath, SemgNode* curNode)
    {
        for (SemgNode* node : spath) 
        {
            if (curNode == node) 
            {
                return true;
            }
        }

        return false;
    }

    inline void initNodeWeights (SemGraph& subCG, unordered_map<SemgNode*, unsigned>& weights) 
    {
        for (auto Itr = subCG.begin(); Itr != subCG.end(); Itr++)
        {
            SemgNode* smgNode = Itr->second;
            weights [smgNode] = smgNode->GetSubNodes();      
        }
        return;
    }

    inline void UpdateNodeWeights (vector<SemgNode*>& curPath, 
                                   unordered_set<SemgNode*>& covered,
                                   unordered_map<SemgNode*, unsigned>& weights) 
    {
        for (size_t i = 0; i < curPath.size(); ++i) 
        {
            SemgNode* node = curPath[i];
            unsigned wt = weights[node];
            if (wt == 0) 
            {
                continue;
            }

            // compute reduction
            unsigned reduction = 0;
            for (size_t j = i+1; j < curPath.size(); ++j) 
            {
                SemgNode* n = curPath[j];
                if (covered.count (n))
                {
                    continue;
                }
                reduction++;
            }
            
            weights[node] = (wt > reduction) ? (wt - reduction) : (0);
        }

        // label all the nodes within the path as covered
        for (size_t i = 0; i < curPath.size(); ++i) 
        {
            covered.insert (curPath[i]);
        }

        return;
    }

    void dfsExplorePaths(SemgNode* curNode, 
                         SemGraph& subCG,
                         unordered_set<SemgNode*>& covered,
                         vector<SemsPath>& allSemsPaths,
                         unordered_map<SemgNode*, unsigned>& weights,
                         vector<SemgNode*>& curPath) 
    {
        if (allSemsPaths.size() >= sampledPathNum)
        {
            return;
        }

        // connect to the wholegraph with name and ID
        SemgNode* wlNode = wholeSemg.GetNode (curNode->GetFName ());
        if (wlNode == NULL)
        {
            cout<<"@@dfsExplorePaths: wlNode get failed -> "<<curNode->GetFName ()<<endl;
            exit (0);
        }
        subCG.UpdateNodeId(curNode, wlNode->GetId());

        // extend the path
        curPath.push_back(curNode);

        // generate path
        if (curNode->GetOutgoingEdgeNum() == 0) 
        {
            if (!isPathExist(&allSemsPaths, &curPath)) 
            {
                UpdateNodeWeights (curPath, covered, weights);
                //showWeight (subCG, weights);
                allSemsPaths.push_back(SemsPath (curPath, curSubgraphID));
            }
            curPath.pop_back();
            return;
        }

        // Get all neighbors and sort by weights (descending)
        vector<SemgNode*> neighbors;
        getNeighbors (curNode, neighbors);
        sort(neighbors.begin(), neighbors.end(), [&](SemgNode* a, SemgNode* b) 
        {
            return weights[a] > weights[b];
        });

        for (SemgNode* neighbor : neighbors) 
        {
            if (weights[neighbor] == 0)
            {
                continue;
            }

            if (isLoopPath (curPath, neighbor))
            {
                continue;
            }

            dfsExplorePaths(neighbor, subCG, covered, allSemsPaths, weights, curPath);
        }

        curPath.pop_back();
        return;
    }

    inline void showWeight (SemGraph& subCG, unordered_map<SemgNode*, unsigned>& weights)
    {
        cout<<"@@showWeight: "<<endl;
        for (auto Itr = subCG.begin (); Itr != subCG.end (); Itr++)
        {
            SemgNode* sn = Itr->second;
            cout << "<"<<sn->GetFName()<<">"
                 << ", weight: [" << sn->GetSubNodes () <<", " <<weights[sn]<<"]"
                 << " -> [ ";
            for (auto it = sn->OutEdgeBegin(); it != sn->OutEdgeEnd(); ++it) 
            {
                SemgEdge* edge = *it;
                cout << edge->GetDstNode()->GetFName()<<" ";
            }
            cout<<"]"<<endl;   
        }
        cout<<endl;
    }

    inline void explorePathsDFS (SemgNode* entryNode, SemGraph& subCG, std::vector<SemsPath>& allSemsPaths) 
    {
        unordered_set<SemgNode*> covered;
        unordered_map<SemgNode*, unsigned> weights;
        vector<SemgNode*> currentPath;

        initNodeWeights (subCG, weights);
        //showWeight (subCG, weights);
        dfsExplorePaths(entryNode, subCG, covered, allSemsPaths, weights, currentPath);
    }

    inline int genRandomInt(int min, int max) 
    {
        random_device rd;

        mt19937 gen(rd());
        uniform_int_distribution<int> dist(min, max);

        return dist(gen);
    }

    inline int genRandomInt(int max)
    {
        if (max == 0)
        {
            return max;
        }
        srand(static_cast<unsigned>(std::time(nullptr)));
        return (rand() % max);
    }

    inline void explorePathsBFSRandom (SemgNode* entryNode, SemGraph& subCG, vector<SemsPath>& allSemsPaths) 
    {
        unordered_set<SemgNode*> coveredNodes;
        queue<pair<vector<SemgNode*>, SemgNode*>> bfsQueue;

        bfsQueue.push({{entryNode}, entryNode});
        coveredNodes.insert(entryNode);

        while (!bfsQueue.empty() && allSemsPaths.size() < sampledPathNum) 
        {
            auto [curPath, curNode] = bfsQueue.front();
            bfsQueue.pop();

            // connect to the wholegraph with name and ID
            SemgNode* wlNode = wholeSemg.GetNode (curNode->GetFName ());
            if (wlNode == NULL)
            {
                cout<<"@@explorePathsBFS: wlNode get failed -> "<<curNode->GetFName ()<<endl;
                exit (0);
            }
            subCG.UpdateNodeId(curNode, wlNode->GetId());

            if (curNode->GetOutgoingEdgeNum() == 0) 
            {
                if (isPathExist (&allSemsPaths, &curPath) == false)
                {
                    allSemsPaths.push_back(SemsPath (curPath, curSubgraphID));
                }
                continue;
            }

            // get all neighbors
            vector<SemgNode*> neighbors;
            getNeighbors (curNode, neighbors);

            vector<SemgNode*> newPath = curPath;
            unsigned nxtNodeNum = neighbors.size();
            unsigned index = 0;
            while (index < nxtNodeNum)
            {
                auto neighbor = neighbors[index];         
                if (isLoopPath (newPath, neighbor) == false)
                {
                    newPath.push_back(neighbor);
                    bfsQueue.push({newPath, neighbor});
                    newPath.pop_back ();
                }      
                index++;
            }
        }
    }

    inline void explorePathsBFS (SemgNode* entryNode, SemGraph& subCG, vector<SemsPath>& allSemsPaths) 
    {
        unordered_set<SemgNode*> coveredNodes;
        queue<pair<vector<SemgNode*>, SemgNode*>> bfsQueue;

        bfsQueue.push({{entryNode}, entryNode});
        coveredNodes.insert(entryNode);

        while (!bfsQueue.empty() && allSemsPaths.size() < sampledPathNum) 
        {
            auto [curPath, curNode] = bfsQueue.front();
            bfsQueue.pop();

            // connect to the wholegraph with name and ID
            SemgNode* wlNode = wholeSemg.GetNode (curNode->GetFName ());
            if (wlNode == NULL)
            {
                cout<<"@@explorePathsBFS: wlNode get failed -> "<<curNode->GetFName ()<<endl;
                exit (0);
            }
            subCG.UpdateNodeId(curNode, wlNode->GetId());

            if (curNode->GetOutgoingEdgeNum() == 0) 
            {
                if (isPathExist (&allSemsPaths, &curPath) == false)
                {
                    allSemsPaths.push_back(SemsPath (curPath, curSubgraphID));
                }
                continue;
            }

            // get all neighbors
            vector<SemgNode*> neighbors;
            getNeighbors (curNode, neighbors);

            unsigned queueSize = bfsQueue.size();
            vector<SemgNode*> newPath = curPath;
            for (SemgNode* neighbor : neighbors) 
            {
                // prioritize uncovered nodes 
                if (coveredNodes.find(neighbor) != coveredNodes.end()) 
                {
                    continue;
                }
        
                newPath.push_back(neighbor);
                bfsQueue.push({newPath, neighbor});
                coveredNodes.insert(neighbor);
                newPath.pop_back ();
            }

            // none uncovered nodes? randomly select one
            if (queueSize == bfsQueue.size())
            {
                unsigned tryNum = neighbors.size();
                while (tryNum > 0)
                {
                    int index = genRandomInt (neighbors.size()-1);
                    auto neighbor = neighbors[index];
                    
                    if (isLoopPath (newPath, neighbor) == false)
                    {
                        newPath.push_back(neighbor);
                        bfsQueue.push({newPath, neighbor});
                        newPath.pop_back ();
                        break;
                    }         
                    tryNum--;
                }
            }
        }
    }

    inline void getCallPaths (SemGraph &subCG, vector<SemsPath>& allSemsPaths, bool BFS=true, bool randomGen=false)
    {   
        vector<SemgNode*> subSemgEntries;

        getSubSemgEntries (subCG, subSemgEntries);
        cout<<"@@getSubSemgEntries: "<<subSemgEntries.size()<<endl;

        subCG.ComputeSubNodesAll();

        randomGenPath = randomGen;
        for (auto smgNode : subSemgEntries)
        {
            cout<<"@@getCallPaths: smgNode = "<<smgNode->GetFName ()<<", randomGen = "<<randomGenPath<<endl;
            vector<SemgNode*> curPath;

            if (randomGenPath)
            {
                explorePathsBFSRandom (smgNode, subCG, allSemsPaths);
            }
            else
            {
                if (BFS)
                {
                    explorePathsBFS (smgNode, subCG, allSemsPaths);
                }
                else
                {
                    explorePathsDFS (smgNode, subCG, allSemsPaths);
                }
            }      
        }

        return;
    }

    unsigned computeSemanticThreshold(SemGraph* graph, unsigned minThreshold = 10, double percentile = 90.0) 
    {
        std::vector<unsigned> incomingEdgeCounts;

        // Step 1: Collect incoming edge counts for all nodes
        for (auto itr = graph->begin(); itr != graph->end(); itr++) 
        {
            SemgNode* node = itr->second;
            incomingEdgeCounts.push_back(node->GetIncomingEdgeNum());
        }

        if (incomingEdgeCounts.empty()) 
        {
            return minThreshold;
        }

        // Step 2: Sort the incoming edge counts
        std::sort(incomingEdgeCounts.begin(), incomingEdgeCounts.end());
        #if 0
        cout<<"incomingEdgeCounts: ";
        for (auto inNum: incomingEdgeCounts)
        {
            cout<<inNum<<" ";
        }
        cout<<endl;
        #endif

        // Step 3: Calculate statistical metrics
        double mean = 0.0, stddev = 0.0;
        for (unsigned count : incomingEdgeCounts) 
        {
            mean += count;
        }
        mean /= incomingEdgeCounts.size();
        cout<<"@@computeSemanticThreshold: mean = "<<mean<<endl;

        for (unsigned count : incomingEdgeCounts) 
        {
            stddev += (count - mean) * (count - mean);
        }
        stddev = std::sqrt(stddev / incomingEdgeCounts.size());
        cout<<"@@computeSemanticThreshold: stddev = "<<stddev<<endl;

        // Step 4: Compute the percentile value
        size_t index = static_cast<size_t>((percentile / 100.0) * incomingEdgeCounts.size());
        if (index >= incomingEdgeCounts.size()) 
        {
            index = incomingEdgeCounts.size() - 1;
        }
        unsigned percentileValue = incomingEdgeCounts[index];
        cout<<"@@computeSemanticThreshold: percentileValue = "<<percentileValue<<endl;

        // Step 5: Determine the final threshold
        unsigned threshold = std::max({minThreshold, static_cast<unsigned>((mean + stddev)*0.8), percentileValue});
        return threshold;
    }

    inline unsigned GetNodeDepth(SemgNode* node)
    {
        return wholeSemg.GetNodeDepth (node);
    }

    inline void completeSubSemg (SemGraph& subSemg, bool enableHNDP=true)
    {
        vector<SemgNode*> leafNodes;
        subSemg.GetLeafNodes(leafNodes);

        queue<SemgNode*> worklist;
        set<SemgNode*> visited;

        cout<<"@@completeSubSemg: SemG size = "<<subSemg.GetNodeNum()<<" with seed nodes: "<<leafNodes.size()<<endl;
        for (auto* node : leafNodes)
        {
            worklist.push(node);
            //cout<<"@@completeSubSemg process: "<<node->GetFName ()<<endl;

            while (!worklist.empty()) 
            {
                SemgNode* curNode = worklist.front();
                worklist.pop();
                string curFunc = curNode->GetFName();

                SemgNode* curWLNode = wholeSemg.GetNode (curFunc);
                if (curWLNode == NULL)
                {
                    cout<<"@@completeSubSemg: curWLNode get failed -> "<<curFunc<<endl;
                    exit (0);
                }

                if (visited.find(curWLNode) != visited.end())
                {
                    continue;
                }
                visited.insert(curWLNode);

                for (auto it = curWLNode->OutEdgeBegin(), eit = curWLNode->OutEdgeEnd(); it != eit; ++it) 
                {
                    SemgEdge* wlEdge    = *it;

                    SemgNode* wlNxtNode = wlEdge->GetDstNode();
                    if (enableHNDP == true && wlNxtNode->GetIncomingEdgeNum() > semThreshold)
                    {
                        continue;
                    }     

                    string wlNxtFunc  = wlNxtNode->GetFName();
                    SemgNode* nxtNode = subSemg.AddNode (wlNxtFunc);       
                    //cout<<"\t add node: "<<wlNxtFunc<<" -> outgoing = "<<wlNxtNode->GetOutgoingEdgeNum()<<endl;

                    subSemg.AddEdge (curNode, nxtNode);
                    //cout<<"\t add edge: "<<curFunc<<"["<<GetNodeDepth(curWLNode)<<"] -> "<<wlNxtFunc<<"["<<GetNodeDepth(wlNxtNode)<<"]"<<endl;

                    worklist.push(nxtNode);
                }
            }
        }

        cout<<"@@completeSubSemg: node size = "<<subSemg.GetNodeNum()<<", edge size = "<<subSemg.GetEdgeNum() <<endl;
        return;
    }

    inline void semThresholdTest ()
    {
        for (unsigned percentile = 10; percentile < 100; percentile += 10)
        {
            cout<<"###semThresholdTest, Input percentile = "<<percentile<<endl;
            unsigned threshold = computeSemanticThreshold (&wholeSemg, 10, percentile);
            cout<<"###semThresholdTest, threshold = "<<threshold<<endl;
            cout<<endl;
        }

        exit (0);
    }

    inline void showPath (const vector<SemgNode*>& callPath) const
    {
        cout << " Path: <";
        for (SemgNode* node : callPath) 
        {
            cout << node->GetFName() <<"["<<node->GetId()<<"]";
            if (node != callPath.back ())
            {
                cout<<" - ";
            }
        }
        cout <<">"<<endl;
    }

    inline vector<string> computeSemantics(vector<SemsPath> &allSemsPaths) const 
    {
        vector<string> SemsResults;
        for (const auto& spath : allSemsPaths) 
        {
            //cout<<"@@computePathSemantic for ";
            //showPath (spath.callPath);
            string semsRes = spath.computeSems (mdSem);
            SemsResults.push_back (semsRes);
        }

        return SemsResults;
    }

    void writeLog (SemGraph& subCG, SemGraph& subCGNoHNDP, string filePath="hndp_stat.log") const
    {
        ofstream file(filePath, ios::app);
        if (!file.is_open()) 
        {
            cerr << "Error: Unable to open file: " << filePath << endl;
            return;
        }

        unsigned nodeNum = subCG.GetNodeNum ();
        unsigned nodeNumNoHNDP = subCGNoHNDP.GetNodeNum ();

        string logInfo = "HNDP:" + to_string (semThreshold) + "," +\
                         "NODES:" + to_string (nodeNum) + "," +\
                         "NODES-noHNDP:" + to_string (nodeNumNoHNDP) + "," +\
                         "Reduced:" + to_string (nodeNumNoHNDP - nodeNum);

        file << logInfo << endl;
        file.close();
    }

public:
    inline void init (const string& symSumPath, const string& semgDotPath)
    {
        randomGenPath = false;

        mdSem.initModule (symSumPath);
        if (initSemGraph (wholeSemg, semgDotPath) == false)
        {
            cout<<"@@SemLearner: initSemGraph failed"<<endl;
            return;
        }
        initSemantics ();
        //wholeSemg.UpdateNodeDepths();
        
        //semThresholdTest ();
        semThreshold = computeSemanticThreshold (&wholeSemg);
        cout<<"@@SemLearner: semThreshold = "<<semThreshold<<endl;

        sampledPathNum = 2048; 
        return;
    }

    std::string extractGraphId(const std::string &name) 
    {
        std::size_t pos = 0;
        while (pos < name.size() && std::isdigit(static_cast<unsigned char>(name[pos]))) {
            ++pos;
        }

        if (pos == 0) {
            throw std::runtime_error("No leading graph id in name: " + name);
        }

        return name.substr(0, pos);
    }

    inline vector<string> getSemantics (const string inDot, unsigned randomGen=false)
    {
        SemGraph subCG;
        if (initSemGraph (subCG, inDot) == false)
        {
            cout<<"@@getSemantics: initSemGraph failed --> "<<inDot<<endl;
            return vector<string>();
        }
        curSubgraphID = extractGraphId(inDot);

        completeSubSemg (subCG);
        #if 1
        SemGraph subCGNoHNDP;
        initSemGraph (subCGNoHNDP, inDot);
        completeSubSemg (subCGNoHNDP, false);
        writeLog (subCG, subCGNoHNDP);
        #endif

        vector<SemsPath> allSemsPaths;
        getCallPaths (subCG, allSemsPaths, true, randomGen);
        cout<<"@@[Graph-"<<inDot<<"]getSemantics: total paths = "<<allSemsPaths.size()<<endl;

        addToStat (subCG, allSemsPaths);

        return computeSemantics (allSemsPaths);
    }

    inline vector<string> getFuncList ()
    {
        vector<string> funcList;
        for (auto it = wholeSemg.begin (); it != wholeSemg.end (); it++) 
        {
            SemgNode* node = it->second;

            if (mdSem.getFuncSem (node->GetId()) == NULL)
            {
                continue;
            }
            funcList.push_back (node->GetFName ());
        }

        return funcList;
    }

    inline string getFuncSummary (const string funcName) const
    {
        SemgNode* node = wholeSemg.GetNode (funcName);
        if (node == NULL)
        {
            cout<<"@@getFuncSummary: fail to get SemgNode by name -> "<<funcName<<endl;
            return "";
        }

        string summary = "{";
        summary += "name:["   + funcName     + "],";
        summary += "input:["  + node->Input  + "],";
        summary += "output:[" + mdSem.setGlobalVarValue(node->Output);
        summary += "]}";

        return summary;
    }

    inline void setFuncSemantic (const string funcName, const string Semantics)
    {
        SemgNode* node = wholeSemg.GetNode (funcName);
        if (node == NULL)
        {
            return;
        }

        FuncSem* fSem = mdSem.getFuncSem (node->GetId ());
        if (fSem == NULL)
        {
            cout<<"@@setFuncSemantic: fail to get FuncSem by name -> "<<funcName<<endl;
            return;
        }

        fSem->Semantics = Semantics;
        return;
    }

    inline unsigned getFuncTokenNum (const string funcName) const
    {
        SemgNode* node = wholeSemg.GetNode (funcName);
        if (node == NULL)
        {
            cout<<"@@getFuncTokenNum: fail to get SemgNode by name -> "<<funcName<<endl;
            return 0;
        }

        FuncSem* fSem = mdSem.getFuncSem (node->GetId ());
        if (fSem == NULL)
        {
            cout<<"@@getFuncTokenNum: fail to get FuncSem by name -> "<<funcName<<endl;
            return 0;
        }

        return fSem->Tokens;
    }

    inline void setFuncTokenNum (const string funcName, unsigned TokenNum) const
    {
        SemgNode* node = wholeSemg.GetNode (funcName);
        if (node == NULL)
        {
            cout<<"@@setFuncTokenNum: fail to get SemgNode by name -> "<<funcName<<endl;
            return;
        }

        FuncSem* fSem = mdSem.getFuncSem (node->GetId ());
        if (fSem == NULL)
        {
            cout<<"@@setFuncTokenNum: fail to get FuncSem by name -> "<<funcName<<endl;
            return;
        }
        fSem->Tokens = TokenNum;

        return;
    }

    inline void saveSummary (string symSumPath="")
    {
        mdSem.exportSemSummary (symSumPath);
    }   
};


class Test 
{
public:
    virtual void run(SemLearner* semln=NULL, 
                     const string& symSumPath="", 
                     const string& cgDotPath="") = 0;
    virtual ~Test() = default;
};

class SemInExportTest : public Test 
{
public:
    ModuleSem mdSem;
    void run(SemLearner* semln=NULL, 
             const string& symSumPath="", 
             const string& cgDotPath="") override 
    {
        mdSem.initModule (symSumPath);
        mdSem.exportSemSummary ();
    } 
};

class InitSubGraphTest : public Test 
{
public:
    void run(SemLearner* semln=NULL, 
             const string& symSumPath="", 
             const string& cgDotPath="") override 
    {
        if (semln == NULL)
        {
            return;
        }
        SemGraph Semg;
        semln->initSemGraph (Semg, cgDotPath);
    } 
};

class PythonAPITest : public Test 
{
public:
    void run(SemLearner* semln=NULL, 
             const string& symSumPath="", 
             const string& cgDotPath="") override 
    {
        if (semln == NULL || cgDotPath == "" || symSumPath == "")
        {
            return;
        }

        semln->init (symSumPath, cgDotPath);
        cout << "Running test: PythonAPITest...\n";

        string funcName = "xmlShell";
        string funcSummary = semln->getFuncSummary (funcName);
        cout<<funcName<<" -> "<<funcSummary<<endl;
        assert (funcSummary == "{name:[xmlShell],input:[doc@type(struct._xmlDoc), filename@type(i8), input@type(i8* (i8*)), output@type(struct._IO_FILE)],output:[filename@type(i8)]}");

        vector<string> funcList = semln->getFuncList ();
        cout<<"funcList -> "<<funcList.size()<<endl;
        assert (funcList.size() != 0);
        cout<<"funcList start with"<<funcList[0]<<endl;
    }
};

class PathExplorationTest : public Test 
{
public:
    void run(SemLearner* semln=NULL, 
             const string& symSumPath="", 
             const string& cgDotPath="") override 
    {
        cout << "Running test: PathExplorationTest...\n";

        SemGraph subCG;
        createGraph(subCG);

        SemLearner learner;
        createGraph(learner.wholeSemg);
        learner.semThreshold = 16;
        learner.sampledPathNum = 3;

        // Get paths
        vector<SemsPath> allSemsPaths;
        learner.getCallPaths(subCG, allSemsPaths, false);

        //printGraph (subCG);
        learner.addToStat (subCG, allSemsPaths);

        cout << "Total paths explored: " << allSemsPaths.size() << "\n";
        for (const SemsPath& path : allSemsPaths) 
        {
            cout << "Path: ";
            for (SemgNode* node : path.callPath) 
            {
                cout << node->GetFName() << " -> ";
            }
            cout << "END\n";
        }

        compareResults(allSemsPaths, 
        {
            {"Node0", "Node1", "Node2", "Node3", "Node4", "Node5", "Node6"},
            {"Node0", "Node1", "Node2", "Node9", "Node10", "Node11", "Node6"},
            {"Node0", "Node12", "Node13", "Node14", "Node15"}
        });
    }

private:
    void createGraph(SemGraph& graph) 
    {
        // Create nodes
        SemgNode* nodes[16];
        for (int i = 0; i < 16; ++i) 
        {
            nodes[i] = graph.AddNode("Node" + to_string(i));
        }

        // Create edges to ensure maximum path depth is 6
        graph.AddEdge(nodes[0], nodes[1]);
        graph.AddEdge(nodes[1], nodes[2]);
        graph.AddEdge(nodes[2], nodes[3]);
        graph.AddEdge(nodes[3], nodes[4]);
        graph.AddEdge(nodes[4], nodes[5]);
        graph.AddEdge(nodes[5], nodes[6]);

        // Additional connections to create multiple paths
        graph.AddEdge(nodes[0], nodes[7]);
        graph.AddEdge(nodes[7], nodes[8]);
        graph.AddEdge(nodes[8], nodes[5]);
        graph.AddEdge(nodes[2], nodes[9]);
        graph.AddEdge(nodes[9], nodes[10]);
        graph.AddEdge(nodes[10], nodes[11]);
        graph.AddEdge(nodes[11], nodes[6]);

        graph.AddEdge(nodes[0], nodes[12]);
        graph.AddEdge(nodes[12], nodes[13]);
        graph.AddEdge(nodes[13], nodes[14]);
        graph.AddEdge(nodes[14], nodes[15]);

        cout << "Manual graph with 16 nodes and max path depth of 6 created.\n";
    }

    void printGraph(SemGraph& graph) const
    {
        cout << "\n@@@Graph Nodes and Edges:\n";
        for (auto it = graph.begin (); it != graph.end (); it++) 
        {
            SemgNode* node = it->second;
            cout << "<"<<node->GetFName()<<">"
                 << ", weight: " << node->GetSubNodes ()
                 << " -> [ ";
            for (auto it = node->OutEdgeBegin(); it != node->OutEdgeEnd(); ++it) 
            {
                SemgEdge* edge = *it;
                cout << edge->GetDstNode()->GetFName()<<" ";
            }
            cout<<"]"<<endl;
        }
        cout<<endl;
    }

    void compareResults(const vector<SemsPath>& actualPaths, const vector<vector<string>>& expectedPaths) 
    {
        bool passed = (actualPaths.size() == expectedPaths.size());

        if (passed) 
        {
            for (size_t i = 0; i < actualPaths.size(); ++i) 
            {
                const auto& actualPath = actualPaths[i].callPath;
                if (actualPath.size() != expectedPaths[i].size()) 
                {
                    passed = false;
                    break;
                }

                for (size_t j = 0; j < actualPath.size(); ++j) 
                {
                    if (actualPath[j]->GetFName() != expectedPaths[i][j]) 
                    {
                        passed = false;
                        break;
                    }
                }

                if (!passed) break;
            }
        }

        if (passed) 
        {
            cout << "PathExplorationTest passed.\n\n";
        } 
        else 
        {
            cerr << "PathExplorationTest failed.\nExpected paths:\n";
            for (const auto& expectedPath : expectedPaths) 
            {
                cerr << "Path: ";
                for (const auto& nodeName : expectedPath) 
                {
                    cerr << nodeName << " -> ";
                }
                cerr << "END\n";
            }

            cerr << "Actual paths:\n";
            for (const SemsPath& path : actualPaths) 
            {
                cerr << "Path: ";
                for (SemgNode* node : path.callPath) 
                {
                    cerr << node->GetFName() << " -> ";
                }
                cerr << "END\n";
            }
            cerr << "\n";
        }
    }
};

class SemLearnerTest 
{
public:
    static void runTests(string testUnits,
                         SemLearner* semln=NULL, 
                         const string& symSumPath="", 
                         const string& cgDotPath="") 
    {
        if (testUnits == "all")
        {
            addTest(new PathExplorationTest());
            addTest(new PythonAPITest());
        }
        
        addTest(new SemInExportTest ());

        addTest (new InitSubGraphTest());

        for (Test* test : tests) 
        {
            test->run(semln, symSumPath, cgDotPath);
        }

        cleanup();
    }

private:
    static vector<Test*> tests;

    static void addTest(Test* test) 
    {
        tests.push_back(test);
    }

    static void cleanup() 
    {
        for (Test* test : tests) 
        {
            delete test;
        }
        tests.clear();
    }
};

#endif