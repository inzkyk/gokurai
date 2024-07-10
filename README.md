# gokurai

gokurai は自作の手続き型軽量マークアップ言語です。詳しくは[手続き型軽量マークアップ言語 gokurai](https://inzkyk.xyz/software/gokurai/) を見てください。

gokurai は次の書籍の翻訳で利用されました:

- [グラフ理論入門](https://inzkyk.xyz/graph/)
- [オープンソースソフトウェアのパフォーマンス](https://inzkyk.xyz/posa/)
- [コンピューターネットワーク: システム的アプローチ](https://inzkyk.xyz/network/)

## ビルド

### テスト抜きでビルド:

gokurai を使いたい場合は次のコマンドでビルドしてください:

```
$ cmake -B build -S . -G Ninja
$ cmake --build build
$ ./build/gokurai hello.gokurai
```

このとき自動的に Release モードでビルドされます。

### テスト付きでビルド:

gokurai をいじりたい場合は次のコマンドでビルドして、テストの実行をデバッガで追うといいでしょう:

```
$ cmake -B build -S . -G Ninja -Dix_DEV=1
$ cmake --build build
$ ./build/gokurai --test
```

なお、有効になるのは `gokurai` のテストだけで、`ix` のテストは有効になりません。

## ソースコードについて

このレポジトリで公開されるソースコードはプライベートのコードベースから切り出したものです。そのため、使用されていないコードや不自然なコードが含まれます。

## 開発について

gokurai の開発は終了しています。今後アップデートやバグフィックスの予定はありません。

## ライセンス

gokurai は MIT License で公開されます。

gokurai は次の外部ライブラリを使用します:

- [doctest](https://github.com/doctest/doctest) (MIT License)
- [Lua](https://www.lua.org/) (MIT License)
- [sokol_time](https://github.com/floooh/sokol) (Zlib License)
- [Microsoft's STL](https://github.com/microsoft/STL) (Apache License v2.0 with LLVM Exception)

また、[cpprefjp](https://cpprefjp.github.io/index.html) にて [クリエイティブ・コモンズ 表示 3.0 非移植 ライセンス](https://creativecommons.org/licenses/by/3.0/) で公開されている情報を一部利用しています。
