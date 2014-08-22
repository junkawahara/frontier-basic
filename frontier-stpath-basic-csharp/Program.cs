using System;
using System.Collections.Generic;
using System.Text;

namespace frontier_stpath_basic_csharp
{
    class Program
    {
        static void Main(string[] args)
        {
            Graph graph = new Graph();

            // グラフ（隣接リスト）を標準入力から読み込む
            string adj_text = "";
            string line = Console.ReadLine();
            while (line != null)
            {
                adj_text += line + "\r\n";
                line = Console.ReadLine();
            }
            graph.ParseAdjListText(adj_text);

            State state = new State(graph, 1, graph.GetNumberOfVertices());

            // 入力グラフの頂点の数と辺の数を出力
            Console.Error.WriteLine("# of vertices = " + graph.GetNumberOfVertices()
                + ", # of edges = " + graph.GetEdgeList().Count);

            ZDD zdd = FrontierAlgorithm.Construct(state); // フロンティア法によるZDD構築

            // 作成されたZDDのノード数と解の数を出力
            Console.Error.WriteLine("# of nodes of ZDD = " + zdd.GetNumberOfNodes());
            Console.Error.WriteLine("# of solutions = " + zdd.GetNumberOfSolutions());

            // ZDDを標準出力に出力
            Console.Write(zdd.GetZDDString());
        }
    }

    /// <summary>
    /// フロンティア法（Simpath）アルゴリズム本体
    /// </summary>
    class FrontierAlgorithm
    {
        public static ZDD Construct(State state)
        {
            List<Edge> edge_list = state.graph.GetEdgeList();
            int[] ZeroOne = new int[] { 0, 1 };
            List<ZDDNode>[] N = new List<ZDDNode>[edge_list.Count + 2];
            N[1] = new List<ZDDNode>();
            N[1].Add(ZDDNode.CreateRootNode(state.graph.GetNumberOfVertices()));

            for (int i = 1; i <= edge_list.Count; ++i)
            {
                N[i + 1] = new List<ZDDNode>();

                foreach (ZDDNode n_hat in N[i])
                {
                    foreach (int x in ZeroOne)
                    {
                        ZDDNode n_prime = CheckTerminal(n_hat, i, x, state);

                        if (n_prime == null)
                        {
                            n_prime = n_hat.MakeCopy();
                            UpdateInfo(n_prime, i, x, state);
                            ZDDNode n_primeprime = Find(n_prime, N[i + 1], i, state);
                            if (n_primeprime != null)
                            {
                                n_prime = n_primeprime;
                            }
                            else
                            {
                                n_prime.SetNextId();
                                N[i + 1].Add(n_prime);
                            }
                        }
                        n_hat.SetChild(n_prime, x);
                    }
                }
            }
            return new ZDD(N);
        }

        private static ZDDNode CheckTerminal(ZDDNode n_hat, int i, int x, State state)
        {
            Edge edge = state.graph.GetEdgeList()[i - 1];
            if (x == 1)
            {
                if (n_hat.comp[edge.src] == n_hat.comp[edge.dest])
                {
                    return ZDDNode.ZeroTerminal;
                }
            }
            ZDDNode n_prime = n_hat.MakeCopy();
            UpdateInfo(n_prime, i, x, state);
            int[] src_dest = new int[] { edge.src, edge.dest };
            foreach (int u in src_dest)
            {
                if ((u == state.s || u == state.t) && n_prime.deg[u] > 1)
                {
                    return ZDDNode.ZeroTerminal;
                }
                else if ((u != state.s && u != state.t) && n_prime.deg[u] > 2)
                {
                    return ZDDNode.ZeroTerminal;
                }
            }
            foreach (int u in src_dest)
            {
                if (!state.F[i].Contains(u))
                {
                    if ((u == state.s || u == state.t) && n_prime.deg[u] != 1)
                    {
                        return ZDDNode.ZeroTerminal;
                    }
                    else if ((u != state.s && u != state.t) && n_prime.deg[u] != 0 && n_prime.deg[u] != 2)
                    {
                        return ZDDNode.ZeroTerminal;
                    }
                }
            }
            if (i == state.graph.GetEdgeList().Count)
            {
                return ZDDNode.OneTerminal;
            }
            return null;
        }

        private static void UpdateInfo(ZDDNode n_hat, int i, int x, State state)
        {
            Edge edge = state.graph.GetEdgeList()[i - 1];
            int[] src_dest = new int[] { edge.src, edge.dest };
            foreach (int u in src_dest)
            {
                if (!state.F[i - 1].Contains(u))
                {
                    n_hat.deg[u] = 0;
                    n_hat.comp[u] = u;
                }
            }
            if (x == 1)
            {
                ++n_hat.deg[edge.src];
                ++n_hat.deg[edge.dest];
                int c_min = Math.Min(n_hat.comp[edge.src], n_hat.comp[edge.dest]);
                int c_max = Math.Max(n_hat.comp[edge.src], n_hat.comp[edge.dest]);
                foreach (int u in state.F[i])
                {
                    if (n_hat.comp[u] == c_max)
                    {
                        n_hat.comp[u] = c_min;
                    }
                }
            }
        }

        private static ZDDNode Find(ZDDNode n_prime, List<ZDDNode> N_i, int i, State state)
        {
            foreach (ZDDNode n_primeprime in N_i)
            {
                if (IsEquivalent(n_prime, n_primeprime, i, state))
                {
                    return n_primeprime;
                }
            }
            return null;
        }

        private static bool IsEquivalent(ZDDNode node1, ZDDNode node2, int i, State state)
        {
            foreach (int v in state.F[i]) // フロンティア上の頂点についてのみ比較
            {
                if (node1.deg[v] != node2.deg[v])
                {
                    return false;
                }
                if (node1.comp[v] != node2.comp[v])
                {
                    return false;
                }
            }
            return true;
        }
    }

    /// <summary>
    /// グラフの辺を表すクラス
    /// </summary>
    public class Edge
    {
        public int src;
        public int dest;

        public Edge(int src, int dest)
        {
            this.src = src;
            this.dest = dest;
        }
    }

    /// <summary>
    /// グラフを表すクラス
    /// </summary>
    public class Graph
    {
        private int number_of_vertices;
        private List<Edge> edge_list = new List<Edge>();

        public int GetNumberOfVertices()
        {
            return number_of_vertices;
        }

        public List<Edge> GetEdgeList()
        {
            return edge_list;
        }

        // ファイルから隣接リスト形式のグラフを読み込む
        public void ParseAdjListText(string adj_list_text)
        {
            edge_list.Clear();

            // 行ごとに区切る
            string[] line = adj_list_text.Split(new string[] { "\r\n", "\n", "\r" }, StringSplitOptions.None);
            // 各行について
            for (int i = 0; i < line.Length; ++i)
            {
                if (line[i] != "")
                {
                    // 空白ごとに区切る
                    string[] dest_list = line[i].Split(new string[] { " ", "\t" }, StringSplitOptions.RemoveEmptyEntries);
                    foreach (string dest in dest_list)
                    {
                        int d = int.Parse(dest);
                        if (i + 1 < d)
                        {
                            // i + 1 が始点、d が終点となる枝を加える
                            edge_list.Add(new Edge(i + 1, d));
                        }
                    }
                }
            }

            // 節点の最大番号を求める
            int max_num = int.MinValue;

            foreach (Edge e in edge_list)
            {
                if (e.src > max_num)
                {
                    max_num = e.src;
                }
                if (e.dest > max_num)
                {
                    max_num = e.dest;
                }
            }
            number_of_vertices = max_num; // 節点の最大番号を節点数とする。
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < edge_list.Count; ++i)
            {
                sb.Append("(").Append(edge_list[i].src).Append(", ").Append(edge_list[i].dest).Append(")");
                if (i < edge_list.Count - 1)
                {
                    sb.Append(", ");
                }
            }
            return sb.ToString();
        }
    }

    class ZDDNode
    {
        public int[] deg;
        public int[] comp;
        public long sol;
        public ZDDNode zero_child;
        public ZDDNode one_child;

        public static ZDDNode ZeroTerminal; // 0終端
        public static ZDDNode OneTerminal;  // 1終端

        private int id_;

        private static int total_id_ = 2;

        static ZDDNode() // static コンストラクタ。ZDDNode 使用開始直前に呼び出される
        {
            ZeroTerminal = new ZDDNode();
            OneTerminal = new ZDDNode();
            ZeroTerminal.id_ = 0;
            OneTerminal.id_ = 1;
        }

        public void SetNextId()
        {
            id_ = total_id_;
            ++total_id_;
        }

        public int GetId()
        {
            return id_;
        }

        public static ZDDNode CreateRootNode(int number_of_vertices)
        {
            ZDDNode node = new ZDDNode();
            node.SetNextId();
            node.deg = new int[number_of_vertices + 1];
            node.comp = new int[number_of_vertices + 1];

            for (int i = 1; i <= number_of_vertices; ++i)
            {
                node.deg[i] = 0;
                node.comp[i] = i;
            }
            return node;
        }

        public ZDDNode MakeCopy()
        {
            ZDDNode node = new ZDDNode();
            node.deg = (int[])deg.Clone();   // 配列のコピーを生成
            node.comp = (int[])comp.Clone();
            return node;
        }

        public void SetChild(ZDDNode node, int child_num)
        {
            if (child_num == 0)
            {
                zero_child = node;
            }
            else
            {
                one_child = node;
            }
        }

        public ZDDNode GetChild(int child_num)
        {
            if (child_num == 0)
            {
                return zero_child;
            }
            else
            {
                return one_child;
            }
        }

        public override string ToString()
        {
            if (id_ >= 2)
            {
                return id_.ToString() + ":" + zero_child.id_ + "," + one_child.id_;
            }
            else
            {
                return id_.ToString();
            }
        }
    }

    class State
    {
        public Graph graph;
        public int s;
        public int t;
        public List<int>[] F;

        public State(Graph g, int start, int end)
        {
            s = start;
            t = end;
            graph = g;
            ComputeFrontier();
        }

        private void ComputeFrontier()
        {
            List<Edge> edge_list = graph.GetEdgeList();

            F = new List<int>[edge_list.Count + 1];
            F[0] = new List<int>();

            for (int i = 0; i < edge_list.Count; ++i)
            {
                F[i + 1] = new List<int>();
                F[i + 1].AddRange(F[i]);

                Edge edge = edge_list[i];
                int src = edge.src;
                int dest = edge.dest;

                if (!F[i + 1].Contains(src))
                {
                    F[i + 1].Add(src);
                }
                if (!F[i + 1].Contains(dest))
                {
                    F[i + 1].Add(dest);
                }

                if (!FindElement(i, src))
                {
                    F[i + 1].Remove(src);
                }
                if (!FindElement(i, dest))
                {
                    F[i + 1].Remove(dest);
                }
            }
        }

        private bool FindElement(int edge_number, int value)
        {
            List<Edge> edge_list = graph.GetEdgeList();
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

    class ZDD
    {
        List<ZDDNode>[] node_list_array_;

        public ZDD(List<ZDDNode>[] node_list_array)
        {
            node_list_array_ = node_list_array;
        }

        public long GetNumberOfNodes()
        {
            long num = 0;
            for (int i = 1; i < node_list_array_.Length; ++i)
            {
                num += node_list_array_[i].Count;
            }
            return num + 2;
        }

        public long GetNumberOfSolutions()
        {
            ZDDNode.ZeroTerminal.sol = 0;
            ZDDNode.OneTerminal.sol = 1;

            for (int i = node_list_array_.Length - 1; i >= 1; --i)
            {
                for (int j = 0; j < node_list_array_[i].Count; ++j)
                {
                    ZDDNode lo_node = node_list_array_[i][j].GetChild(0);
                    ZDDNode hi_node = node_list_array_[i][j].GetChild(1);
                    node_list_array_[i][j].sol = lo_node.sol + hi_node.sol;
                }
            }
            return node_list_array_[1][0].sol;
        }

        public string GetZDDString()
        {
            StringBuilder sb = new StringBuilder();

            for (int i = 1; i < node_list_array_.Length - 1; ++i)
            {
                sb.Append("#").Append(i).Append("\r\n");
                for (int j = 0; j < node_list_array_[i].Count; ++j)
                {
                    sb.Append(node_list_array_[i][j].ToString()).Append("\r\n");
                }
            }
            return sb.ToString();
        }
    }
}
