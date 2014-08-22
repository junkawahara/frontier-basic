using System.Collections.Generic;

namespace simpath_basic_csharp
{
    class StateFrontier : State
    {
        protected List<int> previous_frontier_list_ = new List<int>();
        protected List<int> entering_frontier_list_ = new List<int>();
        protected List<int> next_frontier_list_ = new List<int>();
        protected List<int> leaving_frontier_list_ = new List<int>();

        public StateFrontier(Graph graph) : base(graph)
        {
            // nothing to do.
        }

        public override void Update()
        {
            base.Update();

            Edge edge = graph_.GetEdgeList()[current_edge_];
            int src = edge.src;
            int dest = edge.dest;

            previous_frontier_list_ = next_frontier_list_;
            entering_frontier_list_.Clear();

            if (next_frontier_list_.IndexOf(src) < 0)
            {
                next_frontier_list_.Add(src);
                entering_frontier_list_.Add(src);
            }
            if (next_frontier_list_.IndexOf(dest) < 0)
            {
                next_frontier_list_.Add(dest);
                entering_frontier_list_.Add(dest);
            }

            leaving_frontier_list_.Clear();

            if (!Find(current_edge_, src))
            {
                leaving_frontier_list_.Add(src);
                next_frontier_list_.Remove(src);
            }
            if (!Find(current_edge_, dest))
            {
                leaving_frontier_list_.Add(dest);
                next_frontier_list_.Remove(dest);
            }
        }

        public int GetPreviousFrontierSize()
        {
            return previous_frontier_list_.Count;
        }

        public int GetEnteringFrontierSize()
        {
            return entering_frontier_list_.Count;
        }

        public int GetNextFrontierSize()
        {
            return next_frontier_list_.Count;
        }

        public int GetLeavingFrontierSize()
        {
            return leaving_frontier_list_.Count;
        }

        public int GetPreviousFrontierValue(int index)
        {
            return previous_frontier_list_[index];
        }

        public int GetEnteringFrontierValue(int index)
        {
            return entering_frontier_list_[index];
        }

        public int GetNextFrontierValue(int index)
        {
            return next_frontier_list_[index];
        }

        public int GetLeavingFrontierValue(int index)
        {
            return leaving_frontier_list_[index];
        }

        private bool Find(int edge_number, int value)
        {
            List<Edge> edge_list = graph_.GetEdgeList();
            for (int i = edge_number + 1; i < edge_list.Count; ++i)
            {
                if (value == edge_list[i].src || value == edge_list[i].dest)
                {
                    return true;
                }
            }
            return false;
        }
    }
}
