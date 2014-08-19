using System.Collections.Generic;
using System.Text;

namespace frontiercs
{
    /// <summary>
    /// （フロンティア法で構築する）ZDDを表すクラス
    /// </summary>
    class PseudoZDD
    {
        public static ZDDNode ZeroNode = new ZDDNode(0); // 0終端
        public static ZDDNode OneNode = new ZDDNode(1);  // 1終端

        private List<List<ZDDNode>> node_list_list_ = new List<List<ZDDNode>>();
        private int current_level_ = -1;

        private Dictionary<ZDDNode, ZDDNode> node_hash_set_ = new Dictionary<ZDDNode, ZDDNode>();

        public void CreateRootNode(State state)
        {
            node_list_list_.Add(new List<ZDDNode>());
            node_list_list_[0].Add(ZDDNode.MakeInitialNode(state));
        }

        public void SetLevelStart()
        {
            ++current_level_;
            node_hash_set_.Clear();
            node_list_list_.Add(new List<ZDDNode>());
        }

        public int GetCurrentLevelSize()
        {
            return node_list_list_[current_level_].Count;
        }

        public ZDDNode GetCurrentLevelNode(int index)
        {
            return node_list_list_[current_level_][index];
        }

        public ZDDNode FindNodeFromNextLevel(ZDDNode child_node, State state)
        {
            ZDDNode value;
            if (node_hash_set_.TryGetValue(child_node, out value))
            {
                return value;
            }
            else
            {
                return null;
            }
        }

        public void AddNodeToNextLevel(ZDDNode child_node, State state)
        {
            node_list_list_[current_level_ + 1].Add(child_node);
            node_hash_set_.Add(child_node, child_node);
        }

        public void SetChildNode(ZDDNode node, ZDDNode child_node, int child_num)
        {
            node.SetChild(child_node, child_num);
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < node_list_list_.Count - 1; ++i)
            {
                sb.Append("#").Append(i + 1).Append("\r\n");
                for (int j = 0; j < node_list_list_[i].Count; ++j)
                {
                    sb.Append(node_list_list_[i][j].ToString()).Append("\r\n");
                }
            }
            return sb.ToString();
        }

        public long GetNumberOfNodes()
        {
            long num = 0;
            for (int i = 0; i < node_list_list_.Count; ++i)
            {
                num += node_list_list_[i].Count;
            }
            return num + 2;
        }

        public long GetNumberOfSolutions()
        {
            PseudoZDD.ZeroNode.SetNumberOfSolutions(0);
            PseudoZDD.OneNode.SetNumberOfSolutions(1);

            for (int i = node_list_list_.Count - 1; i >= 0; --i)
            {
                for (int j = 0; j < node_list_list_[i].Count; ++j)
                {
                    ZDDNode lo_node = node_list_list_[i][j].GetChild(0);
                    ZDDNode hi_node = node_list_list_[i][j].GetChild(1);
                    node_list_list_[i][j].SetNumberOfSolutions(
                        lo_node.GetNumberOfSolutions() + hi_node.GetNumberOfSolutions());
                }
            }
            return node_list_list_[0][0].GetNumberOfSolutions();
        }
    }
}
