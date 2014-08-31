using System;
using System.Collections.Generic;
using System.Text;

namespace simpath_basic_csharp
{
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

            // 頂点の最大番号を求める
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
            number_of_vertices = max_num; // 頂点の最大番号を頂点数とする。
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
}
