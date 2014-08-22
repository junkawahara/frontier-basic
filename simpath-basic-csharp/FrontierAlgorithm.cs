using System;
using System.IO;
using System.Text;

namespace frontiercs
{
    /// <summary>
    /// フロンティア法（Simpath）アルゴリズム本体
    /// </summary>
    class FrontierAlgorithm
    {
        public static PseudoZDD Construct(State state)
        {
            PseudoZDD zdd = new PseudoZDD();
            zdd.CreateRootNode(state);

            for (int edge = 1; edge <= state.GetNumberOfEdges(); ++edge) {
                state.Update();

                zdd.SetLevelStart();

                for (int i = 0; i < zdd.GetCurrentLevelSize(); ++i) {
                    ZDDNode node = zdd.GetCurrentLevelNode(i);

                    for (int child_num = 0; child_num < 2; ++child_num) {
                        ZDDNode child_node = MakeChildNode(node, state, child_num, zdd);

                        if (child_node != PseudoZDD.ZeroTerminal && child_node != PseudoZDD.OneTerminal) {
                            ZDDNode cand_node = zdd.FindNodeFromNextLevel(child_node, state);
                            if (cand_node != null) {
                                child_node = cand_node;
                            } else {
                                child_node.SetNextId();
                                zdd.AddNodeToNextLevel(child_node, state);
                            }
                        }
                        zdd.SetChildNode(node, child_node, child_num);
                    }
                }
            }
            return zdd;
        }

        private static ZDDNode MakeChildNode(ZDDNode node, State state, int child_num, PseudoZDD zdd)
        {
            Mate mate = node.GetCopyMate(state);

            int c = mate.CheckTerminalPre(state, child_num);
            if (c == 0)
            {
                return PseudoZDD.ZeroTerminal;
            }
            else if (c == 1)
            {
                return PseudoZDD.OneTerminal;
            }

            mate.Update(state, child_num);

            c = mate.CheckTerminalPost(state);
            if (c == 0)
            {
                return PseudoZDD.ZeroTerminal;
            }
            else if (c == 1)
            {
                return PseudoZDD.OneTerminal;
            }
            else
            {
                return new ZDDNode(mate);
            }
        }
    }    
}
