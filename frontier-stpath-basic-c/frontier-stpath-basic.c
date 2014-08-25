//
// frontier-stpath-basic.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// main 関数は本コードの末尾にある。
// アルゴリズム本体は Construct 関数である。

typedef long long int int64; // 64ビット整数型

// 入力グラフの辺の数の最大値
#define MAX_GRAPH_EDGE_LIST_SIZE 1024

// ノード配列の初期サイズ（ノード配列の大きさは必要に応じて拡張される）
#define INITIAL_NODE_ARRAY_SIZE 1024

//******************************************************************************
// 補助関数

// 大きさが size の配列 vec に，element が含まれるかどうかを判定。
// 含まれるなら 1 を，含まれないなら 0 を返す。
static int Array_Contains(int* vec, int size, int element)
{
	int i;
	for (i = 0; i < size; ++i) {
		if (vec[i] == element) {
			return 1;
		}
	}
	return 0;
}

// 大きさが size の配列 vec の中から element を全て削除する。
// size はポインタで与える。関数の実行後，size には削除後の配列の大きさが格納される。
static void Array_Remove(int* vec, int* size, int element)
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

//******************************************************************************
// Edge 構造体
// グラフの辺を表す。
typedef struct Edge_ {
	int src;  // 辺の始点
	int dest; // 辺の終点
} Edge;

//******************************************************************************
// Graph 構造体
// （無向）グラフを表す。グラフは辺のリストによって表される。
typedef struct Graph_ {
	int number_of_vertices; // 頂点の数
	Edge edge_list[MAX_GRAPH_EDGE_LIST_SIZE]; // 辺リスト
	int edge_list_size; // 辺リストの要素の数
} Graph;

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

		// 以下では，buff に格納されている "1 4 6 10 15" などの
		// 空白区切りの数値テキストをパースする。strtok 関数を用いる。
		// strtok 関数を呼び出すたびに tp は数値テキストの先頭を指すようになる。
		// atoi 関数によってそれを数値に変換する。
		tp = strtok(buff, " ");
		while (tp != NULL) {
			x = atoi(tp);
			Edge edge;
			// src < dest になるように格納
			edge.src = (graph->number_of_vertices < x ? graph->number_of_vertices : x);
			edge.dest = (graph->number_of_vertices < x ? x : graph->number_of_vertices);

			if (edge.src != edge.dest) { // src == dest のものは無視
				if (max_vertex < x) {
					max_vertex = x;
				}

				for (e = 0; e < graph->edge_list_size; ++e) { // 重複除去
					if (graph->edge_list[e].src == edge.src
						&& graph->edge_list[e].dest == edge.dest) {
						break;
					}
				}
				if (e >= graph->edge_list_size) { // 重複が見つからない
					graph->edge_list[graph->edge_list_size] = edge; // 辺リストに追加
					++graph->edge_list_size;
					// 辺の数が制限を超えた
					if (graph->edge_list_size >= MAX_GRAPH_EDGE_LIST_SIZE) { 
						fprintf(stderr, "Error: The number of edges must be"
							"smaller than %d.\n",
							MAX_GRAPH_EDGE_LIST_SIZE);
						exit(1);
					}
				}
			}
			tp = strtok(NULL, " ");
		}
	}
	if (graph->number_of_vertices < max_vertex) {
		graph->number_of_vertices = max_vertex;
	}
}

void Graph_Print(Graph* graph, FILE* fout)
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

//******************************************************************************
// ZDDNode 構造体
// ZDDノードを表す。
typedef struct ZDDNode_ {
	int* deg;  // deg 配列（フロンティア法アルゴリズムの文献参照）
	int* comp; // comp 配列（フロンティア法アルゴリズムの文献参照）
	int64 sol; // 解の数の計算時に使用する変数
	struct ZDDNode_* zero_child; // 0枝側の子ノード
	struct ZDDNode_* one_child;  // 1枝側の子ノード
	int id; // ノードID。0終端は0，1終端は1，それ以外のノードは2から始まる整数
	int dummy; // コンパイラの警告を避ける
} ZDDNode;

static ZDDNode* ZeroTerminal; // 0終端
static ZDDNode* OneTerminal;  // 1終端

static int total_id = 2; // 次に与えるノードID
static ZDDNode zero_t; // 0終端実体
static ZDDNode one_t;  // 1終端実体

// ZDD, ZDDNode を使用するときはこの関数を必ず呼ぶこと。
// 終端ノードの初期化を行う。
void ZDDNode_Initialize(void)
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

// ZDDNode 構造体の使用を終了したときはこの関数を必ず呼ぶこと。
void ZDDNode_Destruct(ZDDNode* node)
{
	free(node->comp);
	free(node->deg);
}

// ノードに次のIDを振る
void ZDDNode_SetNextId(ZDDNode* node)
{
	node->id = total_id;
	++total_id;
}

// 根(root)ノードを作成して返す。
// number_of_vertices: 入力グラフの頂点の数
ZDDNode* ZDDNode_CreateRootNode(int number_of_vertices)
{
	int i;
	ZDDNode* new_node = (ZDDNode*)malloc(sizeof(ZDDNode));
	ZDDNode_SetNextId(new_node); // IDを振る
	// deg配列を動的に作成
	new_node->deg = (int*)malloc(sizeof(int) * (number_of_vertices + 1));
	// comp配列を動的に作成
	new_node->comp = (int*)malloc(sizeof(int) * (number_of_vertices + 1));

	// deg, comp を初期化
	for (i = 1; i <= number_of_vertices; ++i)
	{
		new_node->deg[i] = 0;
		new_node->comp[i] = i;
	}
	return new_node;
}

// ZDDノードをコピーして返す。
// number_of_vertices: 入力グラフの頂点の数
ZDDNode* ZDDNode_MakeCopy(ZDDNode* node, int number_of_vertices)
{
	int i;
	ZDDNode* new_node = (ZDDNode*)malloc(sizeof(ZDDNode));
	// deg配列を動的に作成
	new_node->deg = (int*)malloc(sizeof(int)* (number_of_vertices + 1));
	// comp配列を動的に作成
	new_node->comp = (int*)malloc(sizeof(int)* (number_of_vertices + 1));

	// deg, comp 配列をコピー
	for (i = 1; i <= number_of_vertices; ++i)
	{
		new_node->deg[i] = node->deg[i];
		new_node->comp[i] = node->comp[i];
	}
	return new_node;
}

// ノードに子を設定
// child_num: 0 なら node の0枝側の子として child_node を設定
// 1 なら node の1枝側の子として child_node を設定
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

// ノードの子ノードを取得
// child_num: 0 なら node の0枝側の子を取得
// 1 なら 1枝側の子を取得
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

// ZDDノードを出力する
void ZDDNode_Print(ZDDNode* node, FILE* fout)
{
	fprintf(fout, "%d", node->id);

	if (node->id >= 2)
	{
		fprintf(fout, ":%d,%d", node->zero_child->id, node->one_child->id);
	}
}

//******************************************************************************
// State 構造体

typedef struct State_ {
	Graph* graph; // 入力グラフ
	int s; // s-tパスの始点の頂点番号
	int t; // s-tパスの始点の頂点番号
	int** F; // フロンティアを格納する2次元配列
	int* Fsize; // フロンティアのi番目の大きさを格納する配列
} State;

void State_ComputeFrontier(State* state);

// State 構造体を動的に生成し，初期化して返す
State* State_New(Graph* g, int start, int end)
{
	State* state;

	state = (State*)malloc(sizeof(State));
	state->s = start;
	state->t = end;
	state->graph = g;
	State_ComputeFrontier(state); // フロンティアを計算する
	return state;
}

// State 構造体を破棄する際に呼ばれなければならない
void State_Destruct(State* state)
{
	int i;
	for (i = 0; i < state->graph->edge_list_size + 1; ++i) {
		free(state->F[i]);
	}
	free(state->F);
	state->F = NULL;
	free(state->Fsize);
	state->Fsize = NULL;
}

int State_FindElement(int edge_number, int value, Edge* edge_list, int edge_list_size);

// フロンティアの計算
void State_ComputeFrontier(State* state)
{
	int i, j, n;
	Edge* edge_list = state->graph->edge_list;
	int edge_list_size = state->graph->edge_list_size;
	n = state->graph->number_of_vertices;

	state->F = (int**)malloc(sizeof(int*) * (edge_list_size + 1));
	state->Fsize = (int*)malloc(sizeof(int) * (edge_list_size + 1));
	state->F[0] = (int*)malloc(sizeof(int) * (n + 1));
	state->Fsize[0] = 0;

	for (i = 0; i < edge_list_size; ++i) {
		state->F[i + 1] = (int*)malloc(sizeof(int) * (n + 1));
		state->Fsize[i + 1] = 0;
		// i 番目のフロンティア配列の要素すべてを，i + 1 番目のフロンティア配列に追加する
		for (j = 0; j < state->Fsize[i]; ++j) {
			state->F[i + 1][state->Fsize[i + 1]] = state->F[i][j];
			++state->Fsize[i + 1];
		}

		Edge edge = edge_list[i];
		int src = edge.src;
		int dest = edge.dest;

		// i + 1 番目のフロンティア配列に src が含まれない（重複チェック）
		if (!Array_Contains(state->F[i + 1], state->Fsize[i + 1], src))
		{
			// i + 1 番目のフロンティア配列に src を追加
			state->F[i + 1][state->Fsize[i + 1]] = src;
			++state->Fsize[i + 1];
		}
		// dest に対しても同様の処理
		if (!Array_Contains(state->F[i + 1], state->Fsize[i + 1], dest))
		{
			// i + 1 番目のフロンティア配列に dest を追加
			state->F[i + 1][state->Fsize[i + 1]] = dest;
			++state->Fsize[i + 1];
		}

		// i + 1 番目以降の辺に，頂点 src が出現しないかどうかチェック。
		// 出現しないなら，i + 1 番目のフロンティアから src が去るので，
		// src を削除する。
		if (!State_FindElement(i, src, edge_list, edge_list_size))
		{
			// src を削除
			Array_Remove(state->F[i + 1], &state->Fsize[i + 1], src);
		}
		// dest に対しても同様の処理
		if (!State_FindElement(i, dest, edge_list, edge_list_size))
		{
			Array_Remove(state->F[i + 1], &state->Fsize[i + 1], dest);
		}
	}
}

// i + 1 番目以降の辺に，頂点 src が出現しないかどうかチェック。
// 出現するなら 1 を，しないなら 0 を返す。
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

//******************************************************************************
// ZDDNodeArray 構造体

typedef struct ZDDNodeArray_ {
	ZDDNode** array; // 配列
	int array_size;  // 配列サイズ
	int capacity;    // 配列のために確保しているメモリ領域の大きさ
} ZDDNodeArray;

// ZDDNodeArray を初期化する
void ZDDNodeArray_Initialize(ZDDNodeArray* node_array)
{
	node_array->capacity = INITIAL_NODE_ARRAY_SIZE;
	node_array->array_size = 0;
	node_array->array = (ZDDNode**)malloc(sizeof(ZDDNode*) * node_array->capacity);
}

// ZDDNodeArray 構造体を破棄する際に呼ばれなければならない
void ZDDNodeArray_Destruct(ZDDNodeArray* node_array)
{
	int i;
	for (i = 0; i < node_array->array_size; ++i) {
		ZDDNode_Destruct(node_array->array[i]);
		free(node_array->array[i]);
	}
	free(node_array->array);
	node_array->array = NULL;
}

// ノード配列の capacity を 2 倍に拡張する
void ZDDNodeArray_Expand(ZDDNodeArray* node_array)
{
	node_array->capacity *= 2;
	node_array->array = (ZDDNode**)realloc(node_array->array, sizeof(ZDDNode*)* node_array->capacity);
}

// node を node_array に加える（必要に応じてメモリ領域を拡張する）
void ZDDNodeArray_Add(ZDDNodeArray* node_array, ZDDNode* node)
{
	node_array->array[node_array->array_size] = node;
	++node_array->array_size;
	if (node_array->array_size >= node_array->capacity) {
		ZDDNodeArray_Expand(node_array);
	}
}

//******************************************************************************
// ZDD 構造体
// ZDD のノードが node_list_array に格納される。
// レベル i のノードは node_list_array[i] に格納される。
// i は1始まり。0は使わない。
// すなわち，レベル i の j 番目のノードは node_list_array[i][j] で参照できる。
// node_list_array[i] のサイズ，すなわちレベル i のノードの個数が
// Nsize[i] に格納される。
// node_list_array_size は node_list_array のサイズ（レベルが何個あるか）
typedef struct ZDD_ {
	ZDDNodeArray* node_array_list;
	int node_array_list_size;
} ZDD;

// ZDD構造体を動的に生成し，初期化して返す。
ZDD* ZDD_New(ZDDNodeArray* N, int size)
{
	ZDD* zdd = (ZDD*)malloc(sizeof(ZDD));
	zdd->node_array_list = N;
	zdd->node_array_list_size = size;
	return zdd;
}

// ZDD 構造体を破棄する際に呼ばれなければならない
void ZDD_Destruct(ZDD* zdd)
{
	int i;
	for (i = 0; i < zdd->node_array_list_size; ++i)
	{
		ZDDNodeArray_Destruct(&zdd->node_array_list[i]);
	}
	free(zdd->node_array_list);
	zdd->node_array_list = NULL;
}

// ZDDのノード数を返す
int64 ZDD_GetNumberOfNodes(ZDD* zdd)
{
	int i;
	int64 num = 0;
	for (i = 1; i < zdd->node_array_list_size; ++i)
	{
		num += zdd->node_array_list[i].array_size;
	}
	return num + 2; // + 2 は終端ノードの分
}

// ZDDが表現する集合族の大きさ（解の個数）を返す
int64 ZDD_GetNumberOfSolutions(ZDD* zdd)
{
	int i, j;

	ZeroTerminal->sol = 0;
	OneTerminal->sol = 1;

	// 動的計画法による解の個数の計算。
	// 0枝側のノードの解の個数と，1枝側のノードの解の個数を足したものが，
	// そのノードの解の個数になる。
	// レベルが高いノードから低いノードに向けて計算する
	for (i = zdd->node_array_list_size - 1; i >= 1; --i) // 各レベル i について
	{
		for (j = 0; j < zdd->node_array_list[i].array_size; ++j) // レベル i の各ノードについて
		{
			// 0枝側の子ノード
			ZDDNode* lo_node = ZDDNode_GetChild(zdd->node_array_list[i].array[j], 0);
			// 1枝側の子ノード
			ZDDNode* hi_node = ZDDNode_GetChild(zdd->node_array_list[i].array[j], 1);
			zdd->node_array_list[i].array[j]->sol = lo_node->sol + hi_node->sol;
		}
	}
	return zdd->node_array_list[1].array[0]->sol; // 根ノード
}

// ZDDを出力
void ZDD_PrintZDD(ZDD* zdd, FILE* fout)
{
	int i, j;
	for (i = 1; i < zdd->node_array_list_size - 1; ++i) {
		fprintf(fout, "#%d\r\n", i);
		for (j = 0; j < zdd->node_array_list[i].array_size; ++j) {
			ZDDNode_Print(zdd->node_array_list[i].array[j], fout);
			fprintf(fout, "\r\n");
		}
	}
}

//******************************************************************************
// アルゴリズム本体

ZDDNode* CheckTerminal(ZDDNode* n_hat, int i, int x, State* state);
void UpdateInfo(ZDDNode* n_hat, int i, int x, State* state);
ZDDNode* Find(ZDDNode* n_prime, ZDDNodeArray* node_array, int i, State* state);
int IsEquivalent(ZDDNode* node1, ZDDNode* node2, int i, State* state);

// フロンティア法を実行し，ZDDを作成して返す
// アルゴリズムの中身については文献参照
ZDD* Construct(State* state)
{
	int i, j, x;
	ZDDNode* n_hat, *n_prime, *n_primeprime;
	int m = state->graph->edge_list_size;      // 辺の数
	int n = state->graph->number_of_vertices;  // 頂点の数

	// 生成したノードを格納する配列
	ZDDNodeArray* N = (ZDDNodeArray*)malloc(sizeof(ZDDNodeArray) * (m + 2));
	for (i = 0; i < m + 2; ++i) {
		ZDDNodeArray_Initialize(&N[i]);
	}
	// 根ノードを作成して N[1] に追加
	ZDDNodeArray_Add(&N[1], ZDDNode_CreateRootNode(n));

	for (i = 1; i <= m; ++i) { // 各辺 i についての処理
		for (j = 0; j < N[i].array_size; ++j) // レベル i の各ノードについての処理
		{
			n_hat = N[i].array[j]; // レベル i の j 番目のノード
			for (x = 0; x <= 1; ++x) { // x枝（x = 0, 1）についての処理
				n_prime = CheckTerminal(n_hat, i, x, state);

				if (n_prime == NULL) // x枝の先が0終端でも1終端でもないと判定された
				{
					n_prime = ZDDNode_MakeCopy(n_hat, n);
					UpdateInfo(n_prime, i, x, state);
					n_primeprime = Find(n_prime, &N[i + 1], i, state);
					if (n_primeprime != NULL)
					{
						ZDDNode_Destruct(n_prime); // n_prime を破棄
						free(n_prime);
						n_prime = n_primeprime;
					}
					else
					{
						ZDDNode_SetNextId(n_prime);
						ZDDNodeArray_Add(&N[i + 1], n_prime);
					}
				}
				ZDDNode_SetChild(n_hat, n_prime, x); // node に
			}
		}
	}
	return ZDD_New(N, m + 2); // ZDDを作って返す
}

// アルゴリズムの中身については文献参照
ZDDNode* CheckTerminal(ZDDNode* n_hat, int i, int x, State* state)
{
	int y, u;
	Edge edge = state->graph->edge_list[i - 1];
	if (x == 1)
	{
		if (n_hat->comp[edge.src] == n_hat->comp[edge.dest])
		{
			return ZeroTerminal;
		}
	}
	ZDDNode* n_prime = ZDDNode_MakeCopy(n_hat, state->graph->number_of_vertices);
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
		if (!Array_Contains(state->F[i], state->Fsize[i], u))
		{
			if ((u == state->s || u == state->t) && n_prime->deg[u] != 1)
			{
				ZDDNode_Destruct(n_prime);
				free(n_prime);
				return ZeroTerminal;
			}
			else if ((u != state->s && u != state->t) &&
				n_prime->deg[u] != 0 && n_prime->deg[u] != 2)
			{
				ZDDNode_Destruct(n_prime);
				free(n_prime);
				return ZeroTerminal;
			}
		}
	}
	if (i == state->graph->edge_list_size)
	{
		ZDDNode_Destruct(n_prime);
		free(n_prime);
		return OneTerminal;
	}
	ZDDNode_Destruct(n_prime);
	free(n_prime);
	return NULL;
}

// アルゴリズムの中身については文献参照
void UpdateInfo(ZDDNode* n_hat, int i, int x, State* state)
{
	int y, j, u, c_min, c_max;
	Edge edge = state->graph->edge_list[i - 1];
	for (y = 0; y <= 1; ++y)
	{
		u = (y == 0 ? edge.src : edge.dest);
		if (!Array_Contains(state->F[i - 1], state->Fsize[i - 1], u))
		{
			n_hat->deg[u] = 0;
			n_hat->comp[u] = u;
		}
	}
	if (x == 1)
	{
		++n_hat->deg[edge.src];
		++n_hat->deg[edge.dest];
		c_min = (n_hat->comp[edge.src] < n_hat->comp[edge.dest] ?
			n_hat->comp[edge.src] : n_hat->comp[edge.dest]);
		c_max = (n_hat->comp[edge.src] < n_hat->comp[edge.dest] ?
			n_hat->comp[edge.dest] : n_hat->comp[edge.src]);

		for (j = 0; j < state->Fsize[i]; ++j) {
			u = state->F[i][j];
			if (n_hat->comp[u] == c_max)
			{
				n_hat->comp[u] = c_min;
			}
		}
	}
}

// ノード配列 N_i の中に，n_prime と等価なノードが存在するか調べる
// 等価なノードが存在すればそれを返す。存在しなければ NULL を返す。
// N_i_size: N_i の大きさ（要素数）
// i: レベル
ZDDNode* Find(ZDDNode* n_prime, ZDDNodeArray* node_array, int i, State* state)
{
	int j;
	for (j = 0; j < node_array->array_size; ++j) {
		ZDDNode* n_primeprime = node_array->array[j];

		// n_prime と n_primeprime が等価かどうか判定
		if (IsEquivalent(n_prime, n_primeprime, i, state)) {
			return n_primeprime;
		}
	}
	return NULL;
}

// node1 と node2 が等価か調べる
// i: レベル
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

int main(void)
{
	Graph graph;
	State* state;
	ZDD* zdd;

	// ZDDNode の初期化（プログラム実行時に1度呼ぶ）
	ZDDNode_Initialize();

	// グラフ（隣接リスト）を標準入力から読み込む
	Graph_ParseAdjListText(&graph, stdin);

	// State の作成
	state = State_New(&graph, 1, graph.number_of_vertices);

	// 入力グラフの頂点の数と辺の数を出力
	fprintf(stderr, "# of vertices = %d, # of edges = %d\n",
		graph.number_of_vertices, graph.edge_list_size);

	// フロンティア法によるZDD構築
	zdd = Construct(state);

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
