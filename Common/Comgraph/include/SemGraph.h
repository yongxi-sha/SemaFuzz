#ifndef _SEMGRAPH_H_
#define _SEMGRAPH_H_
#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include "GenericGraph.h"
#include "DotParser.h"
#include "GraphViz.h"

using namespace std;

class SemgNode;
class SemgEdge : public GenericEdge<SemgNode> 
{
public:

    SemgEdge (SemgNode* s, SemgNode* d):GenericEdge<SemgNode>(s, d)                       
    {
        Attr   = E_ATTR_COLOR_RED;
        HitNum = 0;
    }

    virtual ~SemgEdge() 
    {
    }
    
    void UpdateAttr ()
    {
        if (Attr > E_ATTR_COLOR_BLACK)
        {
            Attr--;
        }
    }

    unsigned GetAttr ()
    {
        return Attr;
    }

protected:
    unsigned Attr;

public:
    unsigned HitNum;
    string Conditions;
};

class SemgNode : public GenericNode<SemgEdge> 
{
public:
    bool Hot;
    unsigned HitNum;
    unsigned FArgNum;
    unsigned FBlkNum;

    string Input;
    string Output;
    string Semantics;

    SemgNode (DWORD Id, string FName): GenericNode<SemgEdge>(Id), FuncName(FName)
    {
        DEBUG ("@@@ new CG node: %u \r\n", Id);
        Hot = false;
        HitNum = 0;

        Input  = "";
        Output = "";
        Semantics = "";
    }

    inline string GetFName ()
    {
        return FuncName;
    }

    void UpdateAttr (unsigned NewAttr=E_ATTR_COLOR_INVALID)
    {
        if (NewAttr >= E_ATTR_COLOR_BLACK && NewAttr <= E_ATTR_COLOR_RED)
        {
            Attr = NewAttr;
            return;
        }

        if (Attr > E_ATTR_COLOR_BLACK)
        {
            Attr--;
        }
    }

    unsigned GetAttr ()
    {
        return Attr;
    }

    void SetSubNodes(unsigned Count)
    {
        SubNodes = Count;
        //printf("@@ SetSubNodes -> %s : %u \r\n", FuncName.c_str(), SubNodes);
    }

    unsigned GetSubNodes() const
    {
        return SubNodes;
    }

    void SetFuncAttr (unsigned ArgNum, unsigned BlkNum)
    {
        FArgNum = ArgNum;
        FBlkNum = BlkNum;
    }

    void UpdateFuncName (string FName)
    {
        FuncName = FName;
    }

protected:
    unsigned Attr;
    unsigned SubNodes;  // New member variable

    string FuncName;
};

class SemGraph : public GenericGraph<SemgNode, SemgEdge> 
{
protected:
    map<unsigned, unsigned> m_Id2Depth;
    map<string, SemgNode*> m_FName2Node;
    SemgNode *m_Entry;
    bool Changed;

protected:
    inline unsigned ComputeSubNodes(SemgNode* Node, set<SemgNode*>& Visited)
    {
        if (Visited.find(Node) != Visited.end())
        {
            return 0;
        }

        Visited.insert(Node);
        unsigned Count = 1; // Count the node itself

        for (auto Itr = Node->OutEdgeBegin(); Itr != Node->OutEdgeEnd(); Itr++)
        {
            SemgEdge* CurEdge = *Itr;

            SemgNode* Neighbor = CurEdge->GetDstNode ();
            Count += ComputeSubNodes(Neighbor, Visited);
        }

        Node->SetSubNodes(Count);

        return Count;
    }

    inline void OutputIslands (vector<vector<SemgNode*>*>& IsLands)
    {
        string AislandFile = "AllIslands.txt";
        FILE *F = fopen (AislandFile.c_str(), "w");
        if (F == NULL)
        {
            fprintf (stderr, "@@ AislandFile created fail!\r\n");
            exit (-1);
        }
        fprintf (F, "====== Total Islands Number: %u ======\r\n\r\n", (unsigned)IsLands.size());

        unsigned IslandNo = 0;
        for (auto Itrs = IsLands.begin(); Itrs != IsLands.end(); Itrs++)
        {
            vector<SemgNode*> *island = *Itrs;
            fprintf (F, "***IsLand [No:%-4u -- Size:%-4u]\r\n", IslandNo, (unsigned)island->size());
            for (auto It = island->begin (); It != island->end (); It++)
            {
                SemgNode* Cn= *It;
                fprintf (F, "\t%s: Sub[%u]\r\n", Cn->GetFName ().c_str(), Cn->GetSubNodes ());
            }
            fprintf (F, "\r\n\r\n");

            IslandNo++;
            if (IslandNo >= 100)
            {
                break;
            }
        }

        fclose (F);
    }

public:
    SemGraph ():m_Entry(NULL)
    {
        Changed = false;
    }
    
    virtual ~SemGraph() 
    {
    }

    inline void UpdateEntry(string entryName="main") 
    {
        SemgNode* Cn = GetNode (entryName);
        if (Cn == NULL)
        {
            cout<<"@@UpdateEntry: "<<entryName<<" does not exist!"<<endl;
            return;
        }

        m_Entry = Cn;
        return;
    }

    inline string GetEntry ()
    {
        return m_Entry->GetFName ();
    }

    inline void ResetNodeMap ()
    {
        m_FName2Node.clear ();
    }

    inline void UpdateNodeMap (string FName, SemgNode* Cn)
    {
        m_FName2Node [FName] = Cn;
        return;
    }

    inline bool IsChanged ()
    {
        return Changed;
    }

    inline void ResetChanged ()
    {
        Changed = false;
    }

    inline void SetChanged ()
    {
        Changed = true;
    }

    inline void UpdateNodeId (string FName, unsigned Id)
    {
        SemgNode* smgNode = GetNode (FName);
        assert (smgNode != NULL);

        if (Id != smgNode->GetId ())
        {
            GenericGraph::UpdateNodeId (smgNode, Id);
        }
        
        return;
    }

    inline void UpdateNodeId (SemgNode* smgNode, unsigned Id)
    {
        if (Id != smgNode->GetId ())
        {
            GenericGraph::UpdateNodeId (smgNode, Id);
        }
        
        return;
    }

    inline SemgNode* AddNode (string FName)
    {
        SemgNode* Cn = GetNode (FName);
        if (Cn == NULL)
        {
            unsigned Id = GetNodeNum()+1;
            Cn = new SemgNode (Id, FName);
            assert (Cn != NULL);

            m_FName2Node[FName] = Cn;
            GenericGraph::AddNode (Id, Cn);
        }

        if (m_Entry == NULL)
        {
            m_Entry = Cn;
        }
        
        return Cn;
    }

    inline SemgNode* GetNode (string FName) const
    {
        auto Itr = m_FName2Node.find (FName);
        if (Itr == m_FName2Node.end ())
        {
            return NULL;
        }
        else
        {
            return Itr->second;
        }
    }

    inline unsigned GetHitNodeNum ()
    {
        unsigned HitNodeNum = 0;
        for (auto Itr = begin (); Itr != end (); Itr++)
        {
            SemgNode* Cn = Itr->second;
            HitNodeNum += (unsigned) (Cn->HitNum != 0);
        }

        return HitNodeNum;
    }

    inline unsigned GetHitEdgeNum ()
    {
        unsigned HitEdgeNum = 0;
        for (auto Itr = begin (); Itr != end (); Itr++)
        {
            SemgNode* Cn = Itr->second;
            if (Cn->HitNum == 0)
            {
                continue;
            }

            for (auto edgeItr = Cn->OutEdgeBegin(); edgeItr != Cn->OutEdgeEnd(); edgeItr++)
            {
                SemgEdge* edge = *edgeItr;
                HitEdgeNum += (unsigned)(edge->HitNum != 0);
            }
        }

        return HitEdgeNum;
    }

    inline bool AddEdge (SemgEdge *Edge)
    {
        bool Ok = GenericGraph::AddEdge (Edge);
        if (Ok)
        {
            Changed = true;
        }

        return Ok;
    }

    inline bool AddEdge (SemgNode* S, SemgNode* N)
    {
        SemgEdge *Edge = new SemgEdge (S, N);
        assert (Edge != NULL);

        bool Ok = GenericGraph::AddEdge (Edge);
        if (Ok)
        {
            Changed = true;
        }
        else 
        {
            delete Edge;
        }

        return Ok;
    }

    void ComputeSubNodesAll()
    {
        set<SemgNode*> Visited;
        SemgNode* Root = m_Entry;

        unsigned Count = ComputeSubNodes(Root, Visited);
        Root->SetSubNodes(Count);
    }

    void ComputeIslands (vector<vector<SemgNode*>*>& IsLands, set<string>* FuncSet, unsigned maxIslands=8196)
    {
        //ComputeSubNodesAll();
        
        set<SemgNode*> visited;
        for (auto Itr = begin(); Itr != end(); Itr++)
        {
            SemgNode* node = Itr->second;

            // If node has HitNum != 0 or is already visited, skip it
            if (node->HitNum != 0 || visited.find(node) != visited.end())
            {
                continue;
            }

            // Perform BFS to find all connected nodes with HitNum == 0
            vector<SemgNode*> *island = new vector<SemgNode*> ();
            queue<SemgNode*> q;
            q.push(node);
            visited.insert(node);

            while (!q.empty())
            {
                SemgNode* current = q.front();
                q.pop();
                island->push_back(current);

                for (auto edgeItr = current->OutEdgeBegin(); edgeItr != current->OutEdgeEnd(); edgeItr++)
                {
                    SemgEdge* edge = *edgeItr;
                    SemgNode* neighbor = edge->GetDstNode();

                    // only handle non-library functions
                    if (FuncSet->find (neighbor->GetFName ()) == FuncSet->end())
                    {
                        //cout<<"Library function detected: "<<neighbor->GetFName ()<<endl;
                        continue;
                    }

                    if (neighbor->HitNum == 0 && visited.find(neighbor) == visited.end())
                    {
                        q.push(neighbor);
                        visited.insert(neighbor);
                    }
                }
            }

            if (island->size () < 3)
            {
                delete island;
                continue;
            }
            IsLands.push_back(island);
            if (IsLands.size () >= maxIslands)
            {
                break;
            }
        }

        std::sort(IsLands.begin(), IsLands.end(), [](const std::vector<SemgNode*>* a, const std::vector<SemgNode*>* b) {
            return a->size() > b->size();
        });

        OutputIslands (IsLands);
        return;
    }

    void GetLeafNodes(vector<string>& leafNodes) 
    {
        for (auto Itr = begin(); Itr != end(); Itr++) 
        {
            SemgNode* node = Itr->second;

            if (node->OutEdgeBegin() == node->OutEdgeEnd()) 
            {
                leafNodes.push_back(node->GetFName ());
            }
        }
    }

    void GetLeafNodes(vector<SemgNode*>& leafNodes) 
    {
        for (auto Itr = begin(); Itr != end(); Itr++) 
        {
            SemgNode* node = Itr->second;

            if (node->OutEdgeBegin() == node->OutEdgeEnd())
            {
                leafNodes.push_back(node);
            }
        }
    }

    void ComputeNodeDepths() 
    {
        UpdateEntry();
        m_Id2Depth.clear();

        queue<pair<SemgNode*, unsigned>> q;
        q.push({m_Entry, 0});
        cout<<"@@@UpdateNodeDepths: m_Entry = "<<m_Entry->GetFName()<<endl;

        while (!q.empty()) 
        {
            SemgNode* curNode = q.front().first;
            unsigned curDepth = q.front().second;
            q.pop();

            unsigned nodeId = curNode->GetId();
            if (m_Id2Depth.find(nodeId) == m_Id2Depth.end()) 
            {
                m_Id2Depth[nodeId] = curDepth;
                //cout<<"@@@UpdateNodeDepths: "<<curNode->GetFName()<<" depth -> "<<curDepth<<endl;
            }
            else
            {
                // keep the min depth for each node
                continue;
            }

            for (auto edgeItr = curNode->OutEdgeBegin(); edgeItr != curNode->OutEdgeEnd(); edgeItr++) 
            {
                SemgEdge* edge = *edgeItr;
                SemgNode* neighbor = edge->GetDstNode();
                q.push({neighbor, curDepth + 1});
            }
        }
    }

    unsigned GetNodeDepth (SemgNode* node)
    {
        assert (node != NULL);
        unsigned id = node->GetId ();
        return m_Id2Depth[id];
    }
};


class SemgDotParser: public DotParser <SemgNode, SemgEdge, SemGraph>
{
public:
    SemgDotParser (string DotF): DotParser (DotF) {}
    ~SemgDotParser () {}

};

class SemgViz: public GraphViz <SemgNode, SemgEdge, SemGraph>
{

public:
    SemgViz (string GraphName, SemGraph *Graph, string DotFile, vector<SemgNode*>* VizNodes=NULL)
        :GraphViz<SemgNode, SemgEdge, SemGraph>(GraphName, Graph, DotFile)
    {
        Attr2Color[E_ATTR_COLOR_BLACK] = "color=black";
        Attr2Color[E_ATTR_COLOR_BLUE]  = "color=blue";
        Attr2Color[E_ATTR_COLOR_RED]   = "color=red";

        SubCGNodes = VizNodes;
    }

    ~ SemgViz ()
    {
    }

    inline string GetNodeLabel(SemgNode *Node) 
    {
        string NdLabel;
        if (Node->HitNum > 1024)
        {
            NdLabel = Node->GetFName () + "(" + to_string (Node->GetSubNodes()) + ", " + to_string (Node->HitNum/1024) + "K)";
        }
        else
        {
            NdLabel = Node->GetFName () + "(" + to_string (Node->GetSubNodes()) + ", " + to_string (Node->HitNum) + ")";
        }
        
        return NdLabel;
    }

    inline string GetNodeName(SemgNode *Node) 
    {
        return Node->GetFName ();
    }

    inline string GetNodeAttributes(SemgNode *Node) 
    {
        string str = "color=black";  

        auto Itr = Attr2Color.find (Node->GetAttr ());
        if (Itr != Attr2Color.end ())
        {
            str = Itr->second;
        }
         
        return str;
    }

    inline string GetEdgeAttributes(SemgNode *Edge) 
    {
        string str = "color=black";  

        auto Itr = Attr2Color.find (Edge->GetAttr ());
        if (Itr != Attr2Color.end ())
        {
            str = Itr->second;
        }
         
        return str;
    }

    inline BOOL IsVizNode (SemgNode *Node)
    {
        if(SubCGNodes == NULL)
        {
            return TRUE;
        }
        
        for (auto Itr = SubCGNodes->begin (); Itr != SubCGNodes->end(); Itr++)
        {
            SemgNode* N = *Itr;
            if(Node == N)
            {
                return TRUE;
            }
        }

        return FALSE;
    }

private:
    map<unsigned, string> Attr2Color;
    vector<SemgNode*>* SubCGNodes;
};


#endif
