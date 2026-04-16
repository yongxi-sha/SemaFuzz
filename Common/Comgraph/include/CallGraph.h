
#ifndef _CALLGRAPH_H_
#define _CALLGRAPH_H_
#include "GenericGraph.h"
#include "GraphViz.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>


using namespace std;
class CGNode;
class CGViz;

class CGEdge : public GenericEdge<CGNode> 
{
public:
    CGEdge(CGNode* s, CGNode* d):GenericEdge<CGNode>(s, d)                       
    {
    }

    virtual ~CGEdge() 
    {
    }
};


class CGNode : public GenericNode<CGEdge> 
{  
public:
    CGNode(DWORD Id, string FName): GenericNode<CGEdge>(Id), FuncName(FName)
    {
    }

    inline string GetFName ()
    {
        return FuncName;
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

    void UpdateFuncName (string FName)
    {
        FuncName = FName;
    }

protected:
    unsigned SubNodes;
    string FuncName;
};


class CGGraph : public GenericGraph<CGNode, CGEdge> 
{
protected:
    map<string, CGNode*> m_FName2Node;
    CGNode *m_Entry;

protected:
    inline unsigned ComputeSubNodes(CGNode* Node, set<CGNode*>& Visited)
    {
        if (Visited.find(Node) != Visited.end())
        {
            return 0;
        }

        Visited.insert(Node);
        unsigned Count = 1; // Count the node itself

        for (auto Itr = Node->OutEdgeBegin(); Itr != Node->OutEdgeEnd(); Itr++)
        {
            CGEdge* CurEdge = *Itr;

            CGNode* Neighbor = CurEdge->GetDstNode ();
            Count += ComputeSubNodes(Neighbor, Visited);
        }

        Node->SetSubNodes(Count);

        return Count;
    }

public:
    CGGraph() { m_Entry = NULL; }
    
    virtual ~CGGraph() { }

    inline void UpdateEntry(string entryName="main") 
    {
        CGNode* Cn = GetNode (entryName);
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

    inline void UpdateNodeMap (string FName, CGNode* Cn)
    {
        m_FName2Node [FName] = Cn;
        return;
    }

    inline CGNode* AddNode (string FName)
    {
        CGNode* Cn = GetNode (FName);
        if (Cn == NULL)
        {
            unsigned Id = GetNodeNum() + 1;
            Cn = new CGNode (Id, FName);
            assert (Cn != NULL);

            m_FName2Node[FName] = Cn;
            GenericGraph::AddNode (Id, Cn);
        }
        
        return Cn;
    }

    inline CGNode* GetNode (string FName) const
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

    inline bool AddEdge (CGEdge *Edge)
    {
        return GenericGraph::AddEdge (Edge);
    }

    inline bool AddEdge (CGNode* S, CGNode* N)
    {
        CGEdge *Edge = new CGEdge (S, N);
        assert (Edge != NULL);

        return GenericGraph::AddEdge (Edge);
    }

    void ComputeSubNodesAll()
    {
        set<CGNode*> Visited;
        CGNode* Root = m_Entry;

        unsigned Count = ComputeSubNodes(Root, Visited);
        Root->SetSubNodes(Count);
    }
};


class CGViz: public GraphViz <CGNode, CGEdge, CGGraph>
{

public:
    CGViz(string GraphName, CGGraph *Graph, string DotFile)
        :GraphViz<CGNode, CGEdge, CGGraph>(GraphName, Graph, DotFile)
    {
        Attr2Color[E_ATTR_COLOR_BLACK] = "color=black";
        Attr2Color[E_ATTR_COLOR_BLUE]  = "color=blue";
        Attr2Color[E_ATTR_COLOR_RED]   = "color=red";
    }

    ~CGViz ()
    {
    }

    inline string GetNodeLabel(CGNode *Node) 
    {
        return Node->GetFName ();
    }

    inline string GetNodeName(CGNode *Node) 
    {
        return Node->GetFName ();
    }

    inline string GetNodeAttributes(CGNode *Node) 
    {
        string str = "color=black";
        return str;
    }

    inline string GetEdgeAttributes(CGNode *Edge) 
    {
        string str = "color=black";
        return str;
    }

private:
    map<unsigned, string> Attr2Color;
};


#endif 
