#ifndef _GRAPHCOMPARATOR_H_
#define _GRAPHCOMPARATOR_H_

#include "GenericGraph.h"

template<class NodeTy, class EdgeTy>
class GraphComparator 
{
public:
    static bool AreGraphsEqual(const GenericGraph<NodeTy, EdgeTy>& g1, const GenericGraph<NodeTy, EdgeTy>& g2) 
    {
        if (g1.GetNodeNum() != g2.GetNodeNum() || g1.GetEdgeNum() != g2.GetEdgeNum()) 
        {
            return false;
        }

        for (auto it1 = g1.begin(), it2 = g2.begin(); it1 != g1.end() && it2 != g2.end(); ++it1, ++it2) 
        {
            NodeTy* node1 = it1->second;
            NodeTy* node2 = it2->second;

            if (node1->GetId() != node2->GetId()) 
            {
                return false;
            }

            if (!CompareEdges(node1, node2)) 
            {
                return false;
            }
        }

        return true;
    }

private:
    static bool CompareEdges(NodeTy* node1, NodeTy* node2) 
    {
        auto itIn1 = node1->InEdgeBegin(), itIn2 = node2->InEdgeBegin();
        auto itOut1 = node1->OutEdgeBegin(), itOut2 = node2->OutEdgeBegin();

        if (std::distance(node1->InEdgeBegin(), node1->InEdgeEnd()) != std::distance(node2->InEdgeBegin(), node2->InEdgeEnd()) ||
            std::distance(node1->OutEdgeBegin(), node1->OutEdgeEnd()) != std::distance(node2->OutEdgeBegin(), node2->OutEdgeEnd())) 
        {
            return false;
        }

        while (itIn1 != node1->InEdgeEnd() && itIn2 != node2->InEdgeEnd()) 
        {
            if (!AreEdgesEqual(*itIn1, *itIn2)) 
            {
                return false;
            }
            ++itIn1;
            ++itIn2;
        }

        while (itOut1 != node1->OutEdgeEnd() && itOut2 != node2->OutEdgeEnd()) 
        {
            if (!AreEdgesEqual(*itOut1, *itOut2)) 
            {
                return false;
            }

            ++itOut1;
            ++itOut2;
        }

        return true;
    }

    static bool AreEdgesEqual(EdgeTy* edge1, EdgeTy* edge2) 
    {
        return edge1->GetSrcID() == edge2->GetSrcID() &&
               edge1->GetDstID() == edge2->GetDstID() &&
               edge1->GetAttr() == edge2->GetAttr();
    }
};

#endif