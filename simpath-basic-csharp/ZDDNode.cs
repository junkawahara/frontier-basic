using System.Text;

namespace frontiercs
{
    class ZDDNode
    {
        private static int total_id_ = 2;

        private Mate mate_;
        private int id_;
        private ZDDNode zero_child_;
        private ZDDNode one_child_;
        private long number_of_solutions_;

        public ZDDNode(int n) // n == 0 なら 0終端、n == 1 なら 1終端、n == -1 ならそれ以外のノードを作る
        {
            mate_ = null;
            id_ = n;
        }

        public ZDDNode(Mate mate)
        {
            mate_ = mate;
        }

        public int GetId()
        {
            return id_;
        }

        public void SetNextId()
        {
            id_ = total_id_;
            ++total_id_;
        }

        public static ZDDNode MakeInitialNode(State state)
        {
            ZDDNode node = new ZDDNode(-1);
            node.mate_ = state.MakeInitialMate();
            node.SetNextId();
            return node;
        }

        public Mate GetCopyMate(State state)
        {
            return mate_.Clone(state);
        }

        public override bool Equals(object obj)
        {
            return mate_.Equals(((ZDDNode)obj).mate_);
        }

        public override int GetHashCode()
        {
            return mate_.GetHashCode();
        }

        public void SetChild(ZDDNode node, int child_num)
        {
            if (child_num == 0)
            {
                zero_child_ = node;
            }
            else
            {
                one_child_ = node;
            }
        }

        public ZDDNode GetChild(int child_num)
        {
            if (child_num == 0)
            {
                return zero_child_;
            }
            else
            {
                return one_child_;
            }
        }

        public long GetNumberOfSolutions()
        {
            return number_of_solutions_;
        }

        public void SetNumberOfSolutions(long number_of_solutions)
        {
            number_of_solutions_ = number_of_solutions;
        }

        public override string ToString()
        {
            if (id_ >= 2)
            {
                return id_.ToString() + ":" + zero_child_.id_ + "," + one_child_.id_;
            }
            else
            {
                return id_.ToString();
            }
        }
    }
}
