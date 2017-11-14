//
// frontier-stpath-basic.cpp
//
// Copyright (c) 2014 Jun Kawahara
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

// main 関数は本コードの末尾にある。
// アルゴリズム本体は FrontierAlgorithm クラスの Construct 関数である。

using namespace std;

//******************************************************************************
// 補助関数

// 配列 vec に，element が含まれるかどうかを判定。
// 含まれるなら true を，含まれないなら false を返す。
static bool Contains(const vector<int>& vec, int element)
{
	return std::find(vec.begin(), vec.end(), element) != vec.end();
}

// 配列 vec の中から element を全て削除する。
static void Remove(vector<int>* vec, int element)
{
	vec->erase(std::remove(vec->begin(), vec->end(), element), vec->end());
}

//******************************************************************************
// Edge 構造体
// グラフの辺を表す。
struct Edge {
public:
	int src;  // 辺の始点
	int dest; // 辺の終点

	Edge(int src_a, int dest_a)
	{
		src = src_a;
		dest = dest_a;
	}
};

//******************************************************************************
// Graph 構造体
// （無向）グラフを表す。グラフは辺のリスト(vector)によって表される。
class Graph {
private:
	int number_of_vertices_; // 頂点の数
	vector<Edge> edge_list_; // 辺リスト

public:
	int GetNumberOfVertices() // 頂点の数を返す
	{
		return number_of_vertices_;
	}

	const vector<Edge>& GetEdgeList() // 辺リストを返す
	{
		return edge_list_;
	}

	// ファイルから隣接リスト形式のグラフを読み込む
	void ParseAdjListText(istream& ist)
	{
		number_of_vertices_ = 0;
		edge_list_.clear();

		string s;
		int max_vertex = 0;
		while (std::getline(ist, s)) {
			++number_of_vertices_;

			// 以下では，s に格納されている "1 4 6 10 15" などの
			// 空白区切りの数値テキストをパースする。
			istringstream iss(s);
			int x;
			while (iss >> x) {
				Edge edge(number_of_vertices_, x);
				if (number_of_vertices_ > x) { // src < dest になるように格納
					std::swap(edge.src, edge.dest);
				}

				if (edge.src != edge.dest) { // src == dest のものは無視
					if (max_vertex < x) {
						max_vertex = x;
					}

					unsigned int e;
					for (e = 0; e < edge_list_.size(); ++e) { // 重複除去
						if (edge_list_[e].src == edge.src && edge_list_[e].dest == edge.dest) {
							break;
						}
					}
					if (e >= edge_list_.size()) { // 重複が見つからない
						edge_list_.push_back(edge); // 辺を追加
					}
				}
			}
		}
		if (number_of_vertices_ < max_vertex) {
			number_of_vertices_ = max_vertex;
		}
	}

	string ToString()
	{
		ostringstream oss;
		for (unsigned int i = 0; i < edge_list_.size(); ++i)
		{
			oss << "(" << edge_list_[i].src << ", " << edge_list_[i].dest
				<< ")";
			if (i < edge_list_.size() - 1)
			{
				oss << ", ";
			}
		}
		return oss.str();
	}
};

//******************************************************************************
// ZDDNode 構造体
// ZDDノードを表す。
class ZDDNode {
public:
	int* deg;  // deg 配列（フロンティア法アルゴリズムの文献参照）
	int* comp; // comp 配列（フロンティア法アルゴリズムの文献参照）
	int64_t sol; // 解の数の計算時に使用する変数
	ZDDNode* zero_child; // 0枝側の子ノード
	ZDDNode* one_child;  // 1枝側の子ノード

	static ZDDNode* ZeroTerminal; // 0終端
	static ZDDNode* OneTerminal;  // 1終端

private:
	int id_; // ノードID。0終端は0，1終端は1，それ以外のノードは2から始まる整数

	static int total_id_; // 次に与えるノードID
	static ZDDNode zero_t_; // 0終端実体
	static ZDDNode one_t_;  // 1終端実体

public:
	// ZDD, ZDDNode を使用するときはこの関数を必ず呼ぶこと。
	// 終端ノードの初期化を行う。
	static void Initialize()
	{
		ZeroTerminal = &zero_t_;
		OneTerminal = &one_t_;
		ZeroTerminal->id_ = 0;
		OneTerminal->id_ = 1;
		ZeroTerminal->deg = NULL;
		OneTerminal->deg = NULL;
		ZeroTerminal->comp = NULL;
		OneTerminal->comp = NULL;
	}

	~ZDDNode()
	{
		delete[] deg;
		delete[] comp;
	}

	void SetNextId() // ノードに次のIDを振る
	{
		id_ = total_id_;
		++total_id_;
	}

	int GetId() // IDを返す
	{
		return id_;
	}

	// 根(root)ノードを作成して返す。
	// number_of_vertices: 入力グラフの頂点の数
	static ZDDNode* CreateRootNode(int number_of_vertices)
	{
		ZDDNode* node = new ZDDNode();
		node->SetNextId();
		node->deg = new int[number_of_vertices + 1];
		node->comp = new int[number_of_vertices + 1];

		// deg, comp を初期化
		for (int i = 1; i <= number_of_vertices; ++i)
		{
			node->deg[i] = 0;
			node->comp[i] = i;
		}
		return node;
	}

	// ZDDノードをコピーして返す。
	// number_of_vertices: 入力グラフの頂点の数
	ZDDNode* MakeCopy(int number_of_vertices)
	{
		ZDDNode* node = new ZDDNode();
		node->deg = new int[number_of_vertices + 1];
		node->comp = new int[number_of_vertices + 1];

		// deg, comp 配列をコピー
		for (int i = 1; i <= number_of_vertices; ++i)
		{
			node->deg[i] = deg[i];
			node->comp[i] = comp[i];
		}
		return node;
	}

	// ノードに子を設定
	// child_num: 0 なら0枝側の子として node を設定
	// 1 なら1枝側の子として node を設定
	void SetChild(ZDDNode* node, int child_num)
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

	// ノードの子ノードを取得
	// child_num: 0 なら0枝側の子を取得
	// 1 なら1枝側の子を取得
	ZDDNode* GetChild(int child_num)
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

	// ZDDノードを文字列にして返す
	string ToString()
	{
		ostringstream oss;
		oss << id_;

		if (id_ >= 2)
		{
			oss << ":" << zero_child->id_ << "," << one_child->id_;
		}
		return oss.str();
	}
};

int ZDDNode::total_id_ = 2;
ZDDNode* ZDDNode::ZeroTerminal = NULL;
ZDDNode* ZDDNode::OneTerminal = NULL;
ZDDNode ZDDNode::zero_t_;
ZDDNode ZDDNode::one_t_;

//******************************************************************************
// State 構造体

class State
{
public:
	Graph* graph; // 入力グラフ
	int s; // s-tパスの始点の頂点番号
	int t; // s-tパスの始点の頂点番号
	vector<int>** F; // フロンティアを格納する2次元配列

public:
	State(Graph* g, int start, int end)
	{
		s = start;
		t = end;
		graph = g;
		ComputeFrontier();
	}

	~State()
	{
		for (unsigned int i = 0; i < graph->GetEdgeList().size() + 1; ++i) {
			delete F[i];
		}
		delete[] F;
	}

private:
	void ComputeFrontier() // フロンティアの計算
	{
		const vector<Edge>& edge_list = graph->GetEdgeList();

		F = new vector<int>*[edge_list.size() + 1];
		F[0] = new vector<int>;

		for (unsigned int i = 0; i < edge_list.size(); ++i)
		{
			F[i + 1] = new vector<int>;
			// i 番目のフロンティア配列の要素すべてを，i + 1 番目のフロンティア配列に追加する
			for (unsigned int j = 0; j < F[i]->size(); ++j) {
				F[i + 1]->push_back((*F[i])[j]);
			}

			Edge edge = edge_list[i];
			int src = edge.src;
			int dest = edge.dest;

			// i + 1 番目のフロンティア配列に src が含まれない（重複チェック）
			if (!Contains(*F[i + 1], src))
			{
				// i + 1 番目のフロンティア配列に src を追加
				F[i + 1]->push_back(src);
			}
			// dest に対しても同様の処理
			if (!Contains(*F[i + 1], dest))
			{
				F[i + 1]->push_back(dest);
			}

			// i + 1 番目以降の辺に，頂点 src が出現しないかどうかチェック。
			// 出現しないなら，i + 1 番目のフロンティアから src が去るので，
			// src を削除する。
			if (!FindElement(i, src))
			{
				// src を削除
				Remove(F[i + 1], src);
			}
			// dest に対しても同様の処理
			if (!FindElement(i, dest))
			{
				Remove(F[i + 1], dest);
			}
		}
	}

	// i + 1 番目以降の辺に，頂点 src が出現しないかどうかチェック。
	// 出現するなら true を，しないなら false を返す。
	bool FindElement(int edge_number, int value)
	{
		const vector<Edge>& edge_list = graph->GetEdgeList();
		for (unsigned int i = edge_number + 1; i < edge_list.size(); ++i)
		{
			if (value == edge_list[i].src || value == edge_list[i].dest)
			{
				return true;
			}
		}
		return false;
	}
};

//******************************************************************************
// ZDD 構造体
// ZDD のノードが node_list_array_ に格納される。
// レベル i のノードは (*node_list_array_)[i] に格納される。
// i は1始まり。0は使わない。i = m + 1 はダミー。
// レベル i の j 番目のノードは (*node_list_array)[i][j] で参照できる。
class ZDD
{
private:
	vector<vector<ZDDNode*> >* node_list_array_;

public:
	ZDD(vector<vector<ZDDNode*> >* node_list_array)
	{
		node_list_array_ = node_list_array;
	}

	~ZDD()
	{
		for (unsigned int i = 0; i < node_list_array_->size(); ++i) {
			for (unsigned int j = 0; j < (*node_list_array_)[i].size(); ++j) {
				delete (*node_list_array_)[i][j];
			}
		}
		delete node_list_array_;
	}

	// ZDDのノード数を返す
	int64_t GetNumberOfNodes()
	{
		int64_t num = 0;
		for (unsigned int i = 1; i < node_list_array_->size(); ++i)
		{
			num += static_cast<int64_t>((*node_list_array_)[i].size());
		}
		return num + 2; // + 2 は終端ノードの分
	}

	// ZDDが表現する集合族の大きさ（解の個数）を返す
	int64_t GetNumberOfSolutions()
	{
		ZDDNode::ZeroTerminal->sol = 0;
		ZDDNode::OneTerminal->sol = 1;

		// 動的計画法による解の個数の計算。
		// 0枝側のノードの解の個数と，1枝側のノードの解の個数を足したものが，
		// そのノードの解の個数になる。
		// レベルが高いノードから低いノードに向けて計算する
		for (int i = static_cast<int>(node_list_array_->size()) - 1; i >= 1; --i)
		{
			for (unsigned int j = 0; j < (*node_list_array_)[i].size(); ++j)
			{
				// 0枝側の子ノード
				ZDDNode* lo_node = (*node_list_array_)[i][j]->GetChild(0);
				// 1枝側の子ノード
				ZDDNode* hi_node = (*node_list_array_)[i][j]->GetChild(1);
				(*node_list_array_)[i][j]->sol = lo_node->sol + hi_node->sol;
			}
		}
		return (*node_list_array_)[1][0]->sol;
	}

	// ZDDを文字列にして返す
	string GetZDDString()
	{
		ostringstream oss;

		for (unsigned int i = 1; i < node_list_array_->size() - 1; ++i) {
			oss << "#" << i << "\r\n";
			for (unsigned int j = 0; j < (*node_list_array_)[i].size(); ++j) {
				oss << (*node_list_array_)[i][j]->ToString() << "\r\n";
			}
		}
		return oss.str();
	}
};

//******************************************************************************
// アルゴリズム本体

class FrontierAlgorithm {

public:
	// フロンティア法を実行し，ZDDを作成して返す
	// アルゴリズムの中身については文献参照
	static ZDD* Construct(State* state)
	{
		const vector<Edge>& edge_list = state->graph->GetEdgeList();
		// 生成したノードを格納する配列
		vector<vector <ZDDNode*> >* N = new vector<vector <ZDDNode*> >(edge_list.size() + 2);
		// 根ノードを作成して N[1] に追加
		(*N)[1].push_back(ZDDNode::CreateRootNode(state->graph->GetNumberOfVertices()));

		for (unsigned int i = 1; i <= edge_list.size(); ++i) { // 各辺 i についての処理
			for (unsigned int j = 0; j < (*N)[i].size(); ++j) { // レベル i の各ノードについての処理
				ZDDNode* n_hat = (*N)[i][j]; // レベル i の j 番目のノード
				for (int x = 0; x <= 1; ++x) { // x枝（x = 0, 1）についての処理
					ZDDNode* n_prime = CheckTerminal(n_hat, i, x, state);

					if (n_prime == NULL) { // x枝の先が0終端でも1終端でもないと判定された
						n_prime = n_hat->MakeCopy(state->graph->GetNumberOfVertices());
						UpdateInfo(n_prime, i, x, state);
						ZDDNode* n_primeprime = Find(n_prime, (*N)[i + 1], i, state);
						if (n_primeprime != NULL)
						{
							delete n_prime; // n_prime を破棄
							n_prime = n_primeprime;
						}
						else
						{
							n_prime->SetNextId();
							(*N)[i + 1].push_back(n_prime);
						}
					}
					n_hat->SetChild(n_prime, x);
				}
			}
		}
		return new ZDD(N);
	}

private:
	// アルゴリズムの中身については文献参照
	static ZDDNode* CheckTerminal(ZDDNode* n_hat, int i, int x, State* state)
	{
		Edge edge = state->graph->GetEdgeList()[i - 1];
		if (x == 1)
		{
			if (n_hat->comp[edge.src] == n_hat->comp[edge.dest])
			{
				return ZDDNode::ZeroTerminal;
			}
		}
		ZDDNode* n_prime = n_hat->MakeCopy(state->graph->GetNumberOfVertices());
		UpdateInfo(n_prime, i, x, state);

		for (int y = 0; y <= 1; ++y)
		{
			int u = (y == 0 ? edge.src : edge.dest);
			if ((u == state->s || u == state->t) && n_prime->deg[u] > 1)
			{
				delete n_prime;
				return ZDDNode::ZeroTerminal;
			}
			else if ((u != state->s && u != state->t) && n_prime->deg[u] > 2)
			{
				delete n_prime;
				return ZDDNode::ZeroTerminal;
			}
		}
		for (int y = 0; y <= 1; ++y)
		{
			int u = (y == 0 ? edge.src : edge.dest);
			if (!Contains(*state->F[i], u))
			{
				if ((u == state->s || u == state->t) && n_prime->deg[u] != 1)
				{
					delete n_prime;
					return ZDDNode::ZeroTerminal;
				}
				else if ((u != state->s && u != state->t) && n_prime->deg[u] != 0 && n_prime->deg[u] != 2)
				{
					delete n_prime;
					return ZDDNode::ZeroTerminal;
				}
			}
		}
		if (i == static_cast<int>(state->graph->GetEdgeList().size()))
		{
			delete n_prime;
			return ZDDNode::OneTerminal;
		}
		delete n_prime;
		return NULL;
	}

	// アルゴリズムの中身については文献参照
	static void UpdateInfo(ZDDNode* n_hat, int i, int x, State* state)
	{
		Edge edge = state->graph->GetEdgeList()[i - 1];
		for (int y = 0; y <= 1; ++y)
		{
			int u = (y == 0 ? edge.src : edge.dest);
			if (!Contains(*state->F[i - 1], u))
			{
				n_hat->deg[u] = 0;
				n_hat->comp[u] = u;
			}
		}
		if (x == 1)
		{
			++n_hat->deg[edge.src];
			++n_hat->deg[edge.dest];
			int c_min = std::min(n_hat->comp[edge.src], n_hat->comp[edge.dest]);
			int c_max = std::max(n_hat->comp[edge.src], n_hat->comp[edge.dest]);


			for (unsigned int j = 0; j < state->F[i]->size(); ++j)
			{
				int u = (*state->F[i])[j];
				if (n_hat->comp[u] == c_max)
				{
					n_hat->comp[u] = c_min;
				}
			}
		}
	}

	// ノード配列 N_i の中に，n_prime と等価なノードが存在するか調べる。
	// 等価なノードが存在すればそれを返す。存在しなければ NULL を返す。
	// i: レベル
	static ZDDNode* Find(ZDDNode* n_prime, const vector<ZDDNode*>& N_i, int i, State* state)
	{
		for (unsigned int j = 0; j < N_i.size(); ++j) {
			ZDDNode* n_primeprime = N_i[j];
		
			// n_prime と n_primeprime が等価かどうか判定
			if (IsEquivalent(n_prime, n_primeprime, i, state))
			{
				return n_primeprime;
			}
		}
		return NULL;
	}

	// node1 と node2 が等価か調べる。
	// 等価なら true を，そうでなければ false を返す。
	// i: レベル
	static bool IsEquivalent(ZDDNode* node1, ZDDNode* node2, int i, State* state)
	{
		vector<int>& frontier = (*state->F[i]);

		for (unsigned int j = 0; j < frontier.size(); ++j) { // フロンティア上の頂点についてのみ比較
			int v = frontier[j];
			if (node1->deg[v] != node2->deg[v]) {
				return false;
			}
			if (node1->comp[v] != node2->comp[v]) {
				return false;
			}
		}
		return true;
	}
};

int main()
{
	// ZDDNode の初期化（プログラム実行時に1度呼ぶ）
	ZDDNode::Initialize();

	Graph graph;

	// グラフ（隣接リスト）を標準入力から読み込む
	graph.ParseAdjListText(cin);

	// State の作成
	State state(&graph, 1, graph.GetNumberOfVertices());

	// 入力グラフの頂点の数と辺の数を出力
	cerr << "# of vertices = " << graph.GetNumberOfVertices()
         << ", # of edges = " << graph.GetEdgeList().size() << endl;

	// フロンティア法によるZDD構築
	ZDD* zdd = FrontierAlgorithm::Construct(&state);

	// 作成されたZDDのノード数と解の数を出力
	cerr << "# of nodes of ZDD = " << zdd->GetNumberOfNodes();
	cerr << ", # of solutions = " << zdd->GetNumberOfSolutions() << endl;

	// ZDDを標準出力に出力
	cout << zdd->GetZDDString();

	// 後処理
	delete zdd;
	return 0;
}
