using System;
using System.Diagnostics;
using System.IO;

namespace frontiercs
{
    class Program
    {
        static void Main(string[] args)
        {
            Graph graph = new Graph();

            // グラフ（隣接リスト）を標準入力から読み込む
            string adj_text = "";
            while (true) {
                string line = Console.ReadLine();
                if (line == null)
                {
                    break;
                }
                adj_text += line + "\r\n";
            }
            graph.ParseAdjListText(adj_text);

            // state オブジェクトの作成（フロンティア法実行に必要）
            StateSimpath state = new StateSimpath(graph);
            MateSimpath.SetState(state);

            // 入力グラフの頂点の数と辺の数を出力
            Console.Error.WriteLine("# of vertices = " + graph.GetNumberOfVertices()
                + ", # of edges = " + graph.GetEdgeList().Count);

            PseudoZDD zdd = FrontierAlgorithm.Construct(state); // フロンティア法によるZDD構築

            // 作成されたZDDのノード数と解の数を出力
            Console.Error.WriteLine("# of nodes of ZDD = " + zdd.GetNumberOfNodes());
            Console.Error.WriteLine("# of solutions = " + zdd.GetNumberOfSolutions());

            // ZDDを標準出力に出力
            Console.Write(zdd.ToString());
        }
    }
}
