
namespace simpath_basic_csharp
{
    class StateSimpath : StateFrontier
    {
        public StateSimpath(Graph graph) : base(graph)
        {
            // nothing to do.
        }

        public override Mate MakeInitialMate()
        {
            MateSimpath mate = new MateSimpath();
            int[] mate_array = new int[GetNumberOfVertices() + 1];
            for (int i = 1; i <= GetNumberOfVertices(); ++i)
            {
                mate_array[i] = i;
            }
            mate_array[1] = GetNumberOfVertices();
            mate_array[GetNumberOfVertices()] = 1;
            mate.SetMate(mate_array);
            return mate;
        }
    }

    class MateSimpath : Mate
    {
        private static StateFrontier state_ = null;

        private int[] mate_array_ = null;

        public void SetMate(int[] mate_array)
        {
            mate_array_ = mate_array;
        }

        public static void SetState(StateFrontier state)
        {
            state_ = state;
        }

        public override Mate Clone(State state)
        {
            MateSimpath mate = new MateSimpath();
            mate.mate_array_ = new int[state.GetNumberOfVertices() + 1];

            for (int i = 1; i <= state.GetNumberOfVertices(); ++i)
            {
                mate.mate_array_[i] = this.mate_array_[i];
            }
            return mate;
        }

        public override int CheckTerminalPre(State state, int child_num)
        {
            StateFrontier st = (StateFrontier)state;

            if (child_num == 0)
            { // Lo枝のとき
                return -1;
            }
            else
            { // Hi枝のとき
                Edge edge = state.GetCurrentEdge();

                if (mate_array_[edge.src] == 0 || mate_array_[edge.dest] == 0)
                {
                    // 分岐が発生
                    return 0;
                }
                else if (mate_array_[edge.src] == edge.dest)
                {
                    // サイクルが完成

                    // フロンティアに属する残りの頂点についてチェック
                    for (int i = 0; i < st.GetNextFrontierSize(); ++i)
                    {
                        int v = st.GetNextFrontierValue(i);
                        // 張った辺の始点と終点、及びs,tはチェックから除外
                        if (v != edge.src && v != edge.dest)
                        {
                            // パスの途中でなく、孤立した点でもない場合、
                            // 不完全なパスができることになるので、0を返す
                            if (mate_array_[v] != 0 && mate_array_[v] != v)
                            { // v の次数が 1
                                return 0;
                            }
                        }
                    }
                    return 1;
                }
                else
                {
                    return -1;
                }
            }
        }

        public override int CheckTerminalPost(State state)
        {
            StateFrontier st = (StateFrontier)state;

            for (int i = 0; i < st.GetLeavingFrontierSize(); ++i)
            {
                int v = st.GetLeavingFrontierValue(i); // フロンティアから抜ける頂点 v

                if (mate_array_[v] != 0 && mate_array_[v] != v)
                { // v の次数が 1
                    return 0;
                }
            }
            if (state.IsLastEdge())
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }

        public override void Update(State state, int child_num)
        {
            Edge edge = state.GetCurrentEdge();

            if (child_num == 1)
            { // Hi枝のとき
                int sm = mate_array_[edge.src];
                int dm = mate_array_[edge.dest];

                // 辺をつないだときの mate の更新
                // （↓の計算順を変更すると正しく動作しないことに注意）
                mate_array_[edge.src] = 0;
                mate_array_[edge.dest] = 0;
                mate_array_[sm] = dm;
                mate_array_[dm] = sm;
            }
        }

        public override bool Equals(object obj)
        {
            MateSimpath mate2 = (MateSimpath)obj;

            for (int i = 0; i < state_.GetNextFrontierSize(); ++i)
            {
                int v = state_.GetNextFrontierValue(i);
                if (this.mate_array_[v] != mate2.mate_array_[v])
                {
                    return false;
                }
            }
            return true;
        }

        public override int GetHashCode()
        {
            int hash_value = 0;

            for (int i = 0; i < state_.GetNextFrontierSize(); ++i)
            {
                int v = state_.GetNextFrontierValue(i);
                hash_value = hash_value * 84356289 + mate_array_[v];
            }
            return hash_value;
        }
    }
}
