#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

typedef long long int int64;

static bool Contains(const vector<int>& vec, int element) {
	return std::find(vec.begin(), vec.end(), element) != vec.end();
}

static void Remove(vector<int>* vec, int element) {
	vec->erase(std::remove(vec->begin(), vec->end(), element), vec->end());
}


/// <summary>
/// グラフの辺を表すクラス
/// </summary>
struct Edge {
public:
	int src;
	int dest;

	Edge(int src_a, int dest_a)
	{
		src = src_a;
		dest = dest_a;
	}
};

/// <summary>
/// グラフを表すクラス
/// </summary>
class Graph {
private:
	int number_of_vertices_;
	vector<Edge> edge_list_;

public:
	int GetNumberOfVertices()
	{
		return number_of_vertices_;
	}

	vector<Edge>* GetEdgeList()
	{
		return &edge_list_;
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
			istringstream iss(s);
			int x;
			while (iss >> x) {
				Edge edge(number_of_vertices_, x);
				if (number_of_vertices_ > x) {
					std::swap(edge.src, edge.dest);
				}

				if (max_vertex < x) {
					max_vertex = x;
				}

				unsigned int e;
				for (e = 0; e < edge_list_.size(); ++e) {
					if (edge_list_[e].src == edge.src && edge_list_[e].dest == edge.dest) {
						break;
					}
				}
				if (e >= edge_list_.size()) {
					edge_list_.push_back(edge);
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

class ZDDNode {
public:
	int* deg;
	int* comp;
	int64 sol;
	ZDDNode* zero_child;
	ZDDNode* one_child;

	static ZDDNode* ZeroTerminal; // 0終端
	static ZDDNode* OneTerminal;  // 1終端

private:
	int id_;

	static int total_id_;
	static ZDDNode zero_t_; // 0終端実体
	static ZDDNode one_t_;  // 1終端実体

public:
	static void Initialize() // static コンストラクタ。ZDDNode 使用開始直前に呼び出される
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

	void SetNextId()
	{
		id_ = total_id_;
		++total_id_;
	}

	int GetId()
	{
		return id_;
	}

	static ZDDNode* CreateRootNode(int number_of_vertices)
	{
		ZDDNode* node = new ZDDNode();
		node->SetNextId();
		node->deg = new int[number_of_vertices + 1];
		node->comp = new int[number_of_vertices + 1];

		for (int i = 1; i <= number_of_vertices; ++i)
		{
			node->deg[i] = 0;
			node->comp[i] = i;
		}
		return node;
	}

	ZDDNode* MakeCopy(int number_of_vertices)
	{
		ZDDNode* node = new ZDDNode();
		node->deg = new int[number_of_vertices + 1];
		node->comp = new int[number_of_vertices + 1];
		// 配列のコピーを生成
		for (int i = 1; i <= number_of_vertices; ++i)
		{
			node->deg[i] = deg[i];
			node->comp[i] = comp[i];
		}
		return node;
	}

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

class State
{
public:
	Graph* graph;
	int s;
	int t;
	vector<int>** F;

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
		for (unsigned int i = 0; i < graph->GetEdgeList()->size() + 1; ++i) {
			delete F[i];
		}
		delete[] F;
	}

private:
	void ComputeFrontier()
	{
		vector<Edge>* edge_list = graph->GetEdgeList();

		F = new vector<int>*[edge_list->size() + 1];
		F[0] = new vector<int>;

		for (unsigned int i = 0; i < edge_list->size(); ++i)
		{
			F[i + 1] = new vector<int>;
			for (unsigned int j = 0; j < F[i]->size(); ++j) {
				F[i + 1]->push_back((*F[i])[j]);
			}

			Edge edge = (*edge_list)[i];
			int src = edge.src;
			int dest = edge.dest;

			if (!Contains(*F[i + 1], src))
			{
				F[i + 1]->push_back(src);
			}
			if (!Contains(*F[i + 1], dest))
			{
				F[i + 1]->push_back(dest);
			}

			if (!FindElement(i, src))
			{
				Remove(F[i + 1], src);
			}
			if (!FindElement(i, dest))
			{
				Remove(F[i + 1], dest);
			}
		}
	}

	bool FindElement(int edge_number, int value)
	{
		vector<Edge>* edge_list = graph->GetEdgeList();
		for (unsigned int i = edge_number + 1; i < edge_list->size(); ++i)
		{
			if (value == (*edge_list)[i].src || value == (*edge_list)[i].dest)
			{
				return true;
			}
		}
		return false;
	}
};

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

	int64 GetNumberOfNodes()
	{
		int64 num = 0;
		for (unsigned int i = 1; i < node_list_array_->size(); ++i)
		{
			num += static_cast<int64>((*node_list_array_)[i].size());
		}
		return num + 2;
	}

	int64 GetNumberOfSolutions()
	{
		ZDDNode::ZeroTerminal->sol = 0;
		ZDDNode::OneTerminal->sol = 1;

		for (int i = static_cast<int>(node_list_array_->size()) - 1; i >= 1; --i)
		{
			for (unsigned int j = 0; j < (*node_list_array_)[i].size(); ++j)
			{
				ZDDNode* lo_node = (*node_list_array_)[i][j]->GetChild(0);
				ZDDNode* hi_node = (*node_list_array_)[i][j]->GetChild(1);
				(*node_list_array_)[i][j]->sol = lo_node->sol + hi_node->sol;
			}
		}
		return (*node_list_array_)[1][0]->sol;
	}

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

class FrontierAlgorithm {

public:
	static ZDD* Construct(State* state)
	{
		vector<Edge>* edge_list = state->graph->GetEdgeList();
		vector<vector <ZDDNode*> >* N = new vector<vector <ZDDNode*> >(edge_list->size() + 2);
		(*N)[1].push_back(ZDDNode::CreateRootNode(state->graph->GetNumberOfVertices()));

		for (unsigned int i = 1; i <= edge_list->size(); ++i)
		{
			for (unsigned int j = 0; j < (*N)[i].size(); ++j)
			{
				ZDDNode* n_hat = (*N)[i][j];
				for (int x = 0; x <= 1; ++x)
				{
					ZDDNode* n_prime = CheckTerminal(n_hat, i, x, state);

					if (n_prime == NULL)
					{
						n_prime = n_hat->MakeCopy(state->graph->GetNumberOfVertices());
						UpdateInfo(n_prime, i, x, state);
						ZDDNode* n_primeprime = Find(n_prime, (*N)[i + 1], i, state);
						if (n_primeprime != NULL)
						{
							delete n_prime;
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
	static ZDDNode* CheckTerminal(ZDDNode* n_hat, int i, int x, State* state)
	{
		Edge edge = (*state->graph->GetEdgeList())[i - 1];
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
		if (i == state->graph->GetEdgeList()->size())
		{
			delete n_prime;
			return ZDDNode::OneTerminal;
		}
		delete n_prime;
		return NULL;
	}

	static void UpdateInfo(ZDDNode* n_hat, int i, int x, State* state)
	{
		Edge edge = (*state->graph->GetEdgeList())[i - 1];
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

	static ZDDNode* Find(ZDDNode* n_prime, const vector<ZDDNode*>& N_i, int i, State* state)
	{
		for (unsigned int j = 0; j < N_i.size(); ++j) {
			ZDDNode* n_primeprime = N_i[j];
		
			if (IsEquivalent(n_prime, n_primeprime, i, state))
			{
				return n_primeprime;
			}
		}
		return NULL;
	}

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
	ZDDNode::Initialize();

	Graph graph;

	// グラフ（隣接リスト）を標準入力から読み込む
	graph.ParseAdjListText(cin);

	State state(&graph, 1, graph.GetNumberOfVertices());

	// 入力グラフの頂点の数と辺の数を出力
	cerr << "# of vertices = " << graph.GetNumberOfVertices()
		<< ", # of edges = " << graph.GetEdgeList()->size() << endl;

	ZDD* zdd = FrontierAlgorithm::Construct(&state); // フロンティア法によるZDD構築

	// 作成されたZDDのノード数と解の数を出力
	cerr << "# of nodes of ZDD = " << zdd->GetNumberOfNodes();
	cerr << ", # of solutions = " << zdd->GetNumberOfSolutions() << endl;

	// ZDDを標準出力に出力
	cout << zdd->GetZDDString();

	// 後処理
	delete zdd;
	return 0;
}
