frontier
========

An implementation for the frontier-based search.

書籍「超高速グラフ列挙アルゴリズム」に掲載されているフロンティア法（s-tパスの場合）の実装例です。
C、C++、C# 言語で書かれています。分かりやすさを重視して書いていますので、
速度、メモリ効率は良くないです。

s-tパス以外のフロンティア法については、[frontier](https://github.com/junkawahara/frontier) をご覧ください。

* frontier-stpath-basic-c: フロンティア法（s-tパスの場合）のC言語での実装
* frontier-stpath-basic-cpp: フロンティア法（s-tパスの場合）のC++言語での実装
* frontier-stpath-basic-csharp: フロンティア法（s-tパスの場合）のC#言語での実装
* simpath-basic-csharp: Knuth 氏による [simpath](http://www-cs-faculty.stanford.edu/~knuth/programs/simpath.w) を C# に移植して読みやすくしたもの

# 実行方法

C++ 版を gcc でコンパイルして実行する方法

```
cd frontier-stpath-basic-cpp    # ディレクトリの移動
g++ frontier-stpath-basic.cpp   # コンパイル
./a.out <grid2x2.txt            # 実行（入力グラフは grid2x2.txt）
```

# ファイルの入力形式

入力グラフは隣接リスト形式です。i行目には、頂点 i が隣接する頂点番号を
空白区切りで並べます。例えば、以下のグラフを考えます。

```
1---2
|   |
|   |
3---4
```

隣接リストは以下の通りになります。

```
2 3
1 4
1 4
2 3
```

例えば、1行目は頂点 1 が頂点 2 と 3 に隣接していることを表します。

