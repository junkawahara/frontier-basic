#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef long long int int64;

#define MAX_GRAPH_EDGE_LIST_SIZE 1024

static int Contains(int* vec, int size, int element)
{
	int i;
	for (i = 0; i < size; ++i) {
		if (vec[i] == element) {
			return 1;
		}
	}
	return 0;
}

static void Remove(int* vec, int* size, int element)
{
	int i, j;
	for (i = *size - 1; i >= 0; --i) {
		if (vec[i] == element) {
			for (j = i; j < *size - 1; ++j) {
				vec[j] = vec[j + 1];
			}
			--(*size);
		}
	}
}

/// <summary>
/// グラフの辺を表す構造体
/// </summary>
typedef struct Edge_ {
	int src;
	int dest;
} Edge;

/// <summary>
/// グラフを表すクラス
/// </summary>
typedef struct Graph_ {
	int number_of_vertices;
	Edge edge_list[MAX_GRAPH_EDGE_LIST_SIZE];
	int edge_list_size;
} Graph;

int Graph_GetNumberOfVertices(Graph* graph)
{
	return graph->number_of_vertices;
}

Edge* Graph_GetEdgeList(Graph* graph)
{
	return graph->edge_list;
}

int Graph_GetEdgeListSize(Graph* graph)
{
	return graph->edge_list_size;
}

// ファイルから隣接リスト形式のグラフを読み込む
void Graph_ParseAdjListText(Graph* graph, FILE* fin)
{
	char buff[256];
	int max_vertex = 0;
	char* tp;
	int x, e;

	graph->number_of_vertices = 0;
	graph->edge_list_size = 0;
	
	while (fgets(buff, sizeof(buff), fin) != NULL) {
		++graph->number_of_vertices;

		tp = strtok(buff, " ");

		while (tp != NULL) {
			x = atoi(tp);
			Edge edge;
			edge.src = (graph->number_of_vertices < x ? graph->number_of_vertices : x);
			edge.dest = (graph->number_of_vertices < x ? x : graph->number_of_vertices);

			if (max_vertex < x) {
				max_vertex = x;
			}

			for (e = 0; e < graph->edge_list_size; ++e) {
				if (graph->edge_list[e].src == edge.src && graph->edge_list[e].dest == edge.dest) {
					break;
				}
			}
			if (e >= graph->edge_list_size) {
				graph->edge_list[graph->edge_list_size] = edge;
				++graph->edge_list_size;
				if (graph->edge_list_size >= MAX_GRAPH_EDGE_LIST_SIZE) {
					fprintf(stderr, "Error: The number of edges must be smaller than %d.\n",
						MAX_GRAPH_EDGE_LIST_SIZE);
					exit(1);
				}
			}
			tp = strtok(NULL, " ");
		}
	}
	if (graph->number_of_vertices < max_vertex) {
		graph->number_of_vertices = max_vertex;
	}
}

void Graph_ToString(Graph* graph, FILE* fout)
{
	int i;
	for (i = 0; i < graph->edge_list_size; ++i)
	{
		fprintf(fout, "(%d, %d)", graph->edge_list[i].src, graph->edge_list[i].dest);
		if (i < graph->edge_list_size - 1)
		{
			fprintf(fout, ", ");
		}
	}
}

typedef struct ZDDNode_ {
	int* deg;
	int* comp;
	int64 sol;
	struct ZDDNode_* zero_child;
	struct ZDDNode_* one_child;
	int id;
} ZDDNode;

static ZDDNode* ZeroTerminal; // 0終端
static ZDDNode* OneTerminal;  // 1終端

static int total_id = 2;
static ZDDNode zero_t; // 0終端実体
static ZDDNode one_t;  // 1終端実体

void ZDDNode_Initialize()
{
	ZeroTerminal = &zero_t;
	OneTerminal = &one_t;
	ZeroTerminal->id = 0;
	OneTerminal->id = 1;
	ZeroTerminal->deg = NULL;
	OneTerminal->deg = NULL;
	ZeroTerminal->comp = NULL;
	OneTerminal->comp = NULL;
}

void ZDDNode_Destruct(ZDDNode* node)
{
	free(node->comp);
	free(node->deg);
}

void ZDDNode_SetNextId(ZDDNode* node)
{
	node->id = total_id;
	++total_id;
}

int ZDDNode_GetId(ZDDNode* node)
{
	return node->id;
}

ZDDNode* ZDDNode_CreateRootNode(int number_of_vertices)
{
	int i;
	ZDDNode* new_node = (ZDDNode*)malloc(sizeof(ZDDNode));
	ZDDNode_SetNextId(new_node);
	new_node->deg = (int*)malloc(sizeof(int) * (number_of_vertices + 1));
	new_node->comp = (int*)malloc(sizeof(int) * (number_of_vertices + 1));

	for (i = 1; i <= number_of_vertices; ++i)
	{
		new_node->deg[i] = 0;
		new_node->comp[i] = i;
	}
	return new_node;
}

ZDDNode* ZDDNode_MakeCopy(ZDDNode* node, int number_of_vertices)
{
	int i;
	ZDDNode* new_node = (ZDDNode*)malloc(sizeof(ZDDNode));
	new_node->deg = (int*)malloc(sizeof(int)* (number_of_vertices + 1));
	new_node->comp = (int*)malloc(sizeof(int)* (number_of_vertices + 1));
	// 配列のコピーを生成
	for (i = 1; i <= number_of_vertices; ++i)
	{
		new_node->deg[i] = node->deg[i];
		new_node->comp[i] = node->comp[i];
	}
	return new_node;
}

void ZDDNode_SetChild(ZDDNode* node, ZDDNode* child_node, int child_num)
{
	if (child_num == 0)
	{
		node->zero_child = child_node;
	}
	else
	{
		node->one_child = child_node;
	}
}

ZDDNode* ZDDNode_GetChild(ZDDNode* node, int child_num)
{
	if (child_num == 0)
	{
		return node->zero_child;
	}
	else
	{
		return node->one_child;
	}
}

void ZDDNode_ToString(ZDDNode* node, FILE* fout)
{
	fprintf(fout, "%d", node->id);

	if (node->id >= 2)
	{
		fprintf(fout, ":%d,%d", node->zero_child->id, node->one_child->id);
	}
}

typedef struct State_ {
	Graph* graph;
	int s;
	int t;
	int** F;
	int* Fsize;
} State;

void State_ComputeFrontier(State* state);

State* State_New(Graph* g, int start, int end)
{
	State* state;

	state = (State*)malloc(sizeof(State));
	state->s = start;
	state->t = end;
	state->graph = g;
	State_ComputeFrontier(state);
	return state;
}

void State_Destruct(State* state)
{
	int i;
	for (i = 0; i < Graph_GetEdgeListSize(state->graph) + 1; ++i) {
		free(state->F[i]);
	}
	free(state->F);
	state->F = NULL;
	free(state->Fsize);
	state->Fsize = NULL;
}

int State_FindElement(int edge_number, int value, Edge* edge_list, int edge_list_size);

void State_ComputeFrontier(State* state)
{
	int i, j, n;
	Edge* edge_list = Graph_GetEdgeList(state->graph);
	int edge_list_size = Graph_GetEdgeListSize(state->graph);
	n = Graph_GetNumberOfVertices(state->graph);

	state->F = (int**)malloc(sizeof(int*) * (edge_list_size + 1));
	state->Fsize = (int*)malloc(sizeof(int) * (edge_list_size + 1));
	state->F[0] = (int*)malloc(sizeof(int) * (n + 1));
	state->Fsize[0] = 0;

	for (i = 0; i < edge_list_size; ++i) {
		state->F[i + 1] = (int*)malloc(sizeof(int) * (n + 1));
		state->Fsize[i + 1] = 0;
		for (j = 0; j < state->Fsize[i]; ++j) {
			state->F[i + 1][state->Fsize[i + 1]] = state->F[i][j];
			++state->Fsize[i + 1];
		}

		Edge edge = edge_list[i];
		int src = edge.src;
		int dest = edge.dest;

		if (!Contains(state->F[i + 1], state->Fsize[i + 1], src))
		{
			state->F[i + 1][state->Fsize[i + 1]] = src;
			++state->Fsize[i + 1];
		}
		if (!Contains(state->F[i + 1], state->Fsize[i + 1], dest))
		{
			state->F[i + 1][state->Fsize[i + 1]] = dest;
			++state->Fsize[i + 1];
		}

		if (!State_FindElement(i, src, edge_list, edge_list_size))
		{
			Remove(state->F[i + 1], &state->Fsize[i + 1], src);
		}
		if (!State_FindElement(i, dest, edge_list, edge_list_size))
		{
			Remove(state->F[i + 1], &state->Fsize[i + 1], dest);
		}
	}
}

int State_FindElement(int edge_number, int value, Edge* edge_list, int edge_list_size)
{
	int i;
	for (i = edge_number + 1; i < edge_list_size; ++i)
	{
		if (value == edge_list[i].src || value == edge_list[i].dest)
		{
			return 1;
		}
	}
	return 0;
}

typedef struct ZDD_ {
	ZDDNode*** node_list_array;
	int node_list_array_size;
	int* Nsize;
} ZDD;

ZDD* ZDD_New(ZDDNode*** nlistarray, int nlistarray_size, int* Ns)
{
	ZDD* zdd = (ZDD*)malloc(sizeof(ZDD));
	zdd->node_list_array = nlistarray;
	zdd->node_list_array_size = nlistarray_size;
	zdd->Nsize = Ns;
	return zdd;
}

void ZDD_Destruct(ZDD* zdd)
{
	int i, j;
	for (i = zdd->node_list_array_size - 1; i >= 1; --i)
	{
		for (j = 0; j < zdd->Nsize[i]; ++j)
		{
			ZDDNode_Destruct(zdd->node_list_array[i][j]);
			free(zdd->node_list_array[i][j]);
		}
		free(zdd->node_list_array[i]);
	}
	free(zdd->node_list_array);
	zdd->node_list_array = NULL;
	free(zdd->Nsize);
	zdd->Nsize = NULL;
}

int64 ZDD_GetNumberOfNodes(ZDD* zdd)
{
	int i;
	int64 num = 0;
	for (i = 1; i < zdd->node_list_array_size; ++i)
	{
		num += zdd->Nsize[i];
	}
	return num + 2;
}

int64 ZDD_GetNumberOfSolutions(ZDD* zdd)
{
	int i, j;

	ZeroTerminal->sol = 0;
	OneTerminal->sol = 1;

	for (i = zdd->node_list_array_size - 1; i >= 1; --i)
	{
		for (j = 0; j < zdd->Nsize[i]; ++j)
		{
			ZDDNode* lo_node = ZDDNode_GetChild(zdd->node_list_array[i][j], 0);
			ZDDNode* hi_node = ZDDNode_GetChild(zdd->node_list_array[i][j], 1);
			zdd->node_list_array[i][j]->sol = lo_node->sol + hi_node->sol;
		}
	}
	return zdd->node_list_array[1][0]->sol;
}

void ZDD_PrintZDD(ZDD* zdd, FILE* fout)
{
	int i, j;
	for (i = 1; i < zdd->node_list_array_size - 1; ++i) {
		fprintf(fout, "#%d\r\n", i);
		for (j = 0; j < zdd->Nsize[i]; ++j) {
			ZDDNode_ToString(zdd->node_list_array[i][j], fout);
			fprintf(fout, "\r\n");
		}
	}
}

ZDDNode* CheckTerminal(ZDDNode* n_hat, int i, int x, State* state);
void UpdateInfo(ZDDNode* n_hat, int i, int x, State* state);
ZDDNode* Find(ZDDNode* n_prime, ZDDNode** N_i, int N_i_size, int i, State* state);
int IsEquivalent(ZDDNode* node1, ZDDNode* node2, int i, State* state);

ZDD* Construct(State* state)
{
	int i, j, x;
	Edge* edge_list = Graph_GetEdgeList(state->graph);
	int edge_list_size = Graph_GetEdgeListSize(state->graph);
	ZDDNode*** N = (ZDDNode***)malloc(sizeof(ZDDNode**) * (edge_list_size + 2));
	int* Nsize = (int*)malloc(sizeof(int) * (edge_list_size + 2));
	int* Ncapacity = (int*)malloc(sizeof(int)* (edge_list_size + 2));
	for (i = 0; i < edge_list_size + 2; ++i) {
		Nsize[i] = 0;
		Ncapacity[i] = 1024;
	}
	N[1] = (ZDDNode**)malloc(sizeof(ZDDNode*) * Ncapacity[1]);
	N[1][0] = ZDDNode_CreateRootNode(Graph_GetNumberOfVertices(state->graph));
	++Nsize[1];

	for (i = 1; i <= edge_list_size; ++i) {
		N[i + 1] = (ZDDNode**)malloc(sizeof(ZDDNode*) * Ncapacity[i + 1]);

		for (j = 0; j < Nsize[i]; ++j)
		{
			ZDDNode* n_hat = N[i][j];
			for (x = 0; x <= 1; ++x)
			{
				ZDDNode* n_prime = CheckTerminal(n_hat, i, x, state);

				if (n_prime == NULL)
				{
					n_prime = ZDDNode_MakeCopy(n_hat, Graph_GetNumberOfVertices(state->graph));
					UpdateInfo(n_prime, i, x, state);
					ZDDNode* n_primeprime = Find(n_prime, N[i + 1], Nsize[i + 1], i, state);
					if (n_primeprime != NULL)
					{
						ZDDNode_Destruct(n_prime);
						free(n_prime);
						n_prime = n_primeprime;
					}
					else
					{
						ZDDNode_SetNextId(n_prime);
						N[i + 1][Nsize[i + 1]] = n_prime;
						++Nsize[i + 1];
						if (Nsize[i + 1] >= Ncapacity[i + 1]) {
							Ncapacity[i + 1] *= 2;
							N[i + 1] = realloc(N[i + 1], sizeof(ZDDNode*) * Ncapacity[i + 1]);
						}
					}
				}
				ZDDNode_SetChild(n_hat, n_prime, x);
			}
		}
	}
	free(Ncapacity);
	return ZDD_New(N, edge_list_size + 2, Nsize);
}

ZDDNode* CheckTerminal(ZDDNode* n_hat, int i, int x, State* state)
{
	int y, u;
	Edge edge = Graph_GetEdgeList(state->graph)[i - 1];
	if (x == 1)
	{
		if (n_hat->comp[edge.src] == n_hat->comp[edge.dest])
		{
			return ZeroTerminal;
		}
	}
	ZDDNode* n_prime = ZDDNode_MakeCopy(n_hat, Graph_GetNumberOfVertices(state->graph));
	UpdateInfo(n_prime, i, x, state);

	for (y = 0; y <= 1; ++y)
	{
		u = (y == 0 ? edge.src : edge.dest);
		if ((u == state->s || u == state->t) && n_prime->deg[u] > 1)
		{
			ZDDNode_Destruct(n_prime);
			free(n_prime);
			return ZeroTerminal;
		}
		else if ((u != state->s && u != state->t) && n_prime->deg[u] > 2)
		{
			ZDDNode_Destruct(n_prime);
			free(n_prime);
			return ZeroTerminal;
		}
	}
	for (y = 0; y <= 1; ++y)
	{
		u = (y == 0 ? edge.src : edge.dest);
		if (!Contains(state->F[i], state->Fsize[i], u))
		{
			if ((u == state->s || u == state->t) && n_prime->deg[u] != 1)
			{
				ZDDNode_Destruct(n_prime);
				free(n_prime);
				return ZeroTerminal;
			}
			else if ((u != state->s && u != state->t) && n_prime->deg[u] != 0 && n_prime->deg[u] != 2)
			{
				ZDDNode_Destruct(n_prime);
				free(n_prime);
				return ZeroTerminal;
			}
		}
	}
	if (i == Graph_GetEdgeListSize(state->graph))
	{
		ZDDNode_Destruct(n_prime);
		free(n_prime);
		return OneTerminal;
	}
	ZDDNode_Destruct(n_prime);
	free(n_prime);
	return NULL;
}

void UpdateInfo(ZDDNode* n_hat, int i, int x, State* state)
{
	int y, j, u, c_min, c_max;
	Edge edge = Graph_GetEdgeList(state->graph)[i - 1];
	for (y = 0; y <= 1; ++y)
	{
		u = (y == 0 ? edge.src : edge.dest);
		if (!Contains(state->F[i - 1], state->Fsize[i - 1], u))
		{
			n_hat->deg[u] = 0;
			n_hat->comp[u] = u;
		}
	}
	if (x == 1)
	{
		++n_hat->deg[edge.src];
		++n_hat->deg[edge.dest];
		c_min = (n_hat->comp[edge.src] < n_hat->comp[edge.dest] ? n_hat->comp[edge.src] : n_hat->comp[edge.dest]);
		c_max = (n_hat->comp[edge.src] < n_hat->comp[edge.dest] ? n_hat->comp[edge.dest] : n_hat->comp[edge.src]);

		for (j = 0; j < state->Fsize[i]; ++j) {
			u = state->F[i][j];
			if (n_hat->comp[u] == c_max)
			{
				n_hat->comp[u] = c_min;
			}
		}
	}
}

ZDDNode* Find(ZDDNode* n_prime, ZDDNode** N_i, int N_i_size, int i, State* state)
{
	int j;
	for (j = 0; j < N_i_size; ++j) {
		ZDDNode* n_primeprime = N_i[j];

		if (IsEquivalent(n_prime, n_primeprime, i, state)) {
			return n_primeprime;
		}
	}
	return NULL;
}

int IsEquivalent(ZDDNode* node1, ZDDNode* node2, int i, State* state)
{
	int j, v;
	for (j = 0; j < state->Fsize[i]; ++j) { // フロンティア上の頂点についてのみ比較
		v = state->F[i][j];
		if (node1->deg[v] != node2->deg[v]) {
			return 0;
		}
		if (node1->comp[v] != node2->comp[v]) {
			return 0;
		}
	}
	return 1;
}

int main()
{
	Graph graph;
	State* state;
	ZDD* zdd;

	ZDDNode_Initialize();

	// グラフ（隣接リスト）を標準入力から読み込む
	Graph_ParseAdjListText(&graph, stdin);

	state = State_New(&graph, 1, Graph_GetNumberOfVertices(&graph));

	// 入力グラフの頂点の数と辺の数を出力
	fprintf(stderr, "# of vertices = %d, # of edges = %d\n",
		Graph_GetNumberOfVertices(&graph), Graph_GetEdgeListSize(&graph));

	zdd = Construct(state); // フロンティア法によるZDD構築

	// 作成されたZDDのノード数と解の数を出力
	fprintf(stderr, "# of nodes of ZDD = %lld, # of solutions = %lld\n", 
		ZDD_GetNumberOfNodes(zdd), ZDD_GetNumberOfSolutions(zdd));

	// ZDDを標準出力に出力
	ZDD_PrintZDD(zdd, stdout);

	// 後処理
	ZDD_Destruct(zdd);
	free(zdd);
	State_Destruct(state);
	free(state);
	return 0;
}
