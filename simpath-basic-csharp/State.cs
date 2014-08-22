using System;
using System.Collections.Generic;

namespace simpath_basic_csharp
{
    class State
    {
        protected Graph graph_;
        protected int number_of_vertices_;
        protected int number_of_edges_;
        protected int current_edge_;

        public State(Graph graph)
        {
            graph_ = graph;
            current_edge_ = -1;

            if (graph != null)
            {
                number_of_vertices_ = graph.GetNumberOfVertices();
                number_of_edges_ = graph.GetEdgeList().Count;
            }
        }

        public int GetNumberOfVertices()
        {
            return number_of_vertices_;
        }

        public int GetNumberOfEdges()
        {
            return graph_.GetEdgeList().Count;
        }

        public int GetCurrentEdgeNumber()
        {
            return current_edge_;
        }

        public Edge GetCurrentEdge()
        {
            return graph_.GetEdgeList()[current_edge_];
        }

        public List<Edge> GetEdgeList()
        {
            return graph_.GetEdgeList();
        }

        public bool IsLastEdge()
        {
            return current_edge_ >= number_of_edges_ - 1;
        }

        public virtual void Update()
        {
            ++current_edge_;
        }

        public virtual Mate MakeInitialMate()
        {
            throw new NotImplementedException();
        }
    }

    abstract class Mate
    {
        public abstract Mate Clone(State state);
        public abstract int CheckTerminalPre(State state, int child_num);
        public abstract int CheckTerminalPost(State state);
        public abstract void Update(State state, int child_num);
    }
}
