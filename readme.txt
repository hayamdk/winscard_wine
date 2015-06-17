□□□　概要

Linux(Wine)上でwinscard.dllの代用になるlibpcscliteのラッパーdllです。
Wineに含まれるwinscard.dllはほとんどのAPIがまだ実装されていないため、Windows向けにコンパイルされたカードリーダーを使用する録画・視聴用ソフトウェア資産をそのまま動かせません。このDLLを用いることでそれらをLinux上で動かすことができます。


□□□　使い方

wine、wine-dev、libpcsclite、libpcsclite-devなどのパッケージが必要です（32ビット版を用いる場合はi386のパッケージ）。
makeを行うとwinscard.dllが生成されるので、これを動かしたいアプリケーションと同じディレクトリに置きます。
winecfg→「ライブラリ」タブ→「ライブラリの新規オーバーライド」にwinscard.dllを設定すると、Wine内製のwinscard.dllよりも自分で設置したwinscard.dllを優先して使用するようになります。
64ビットのwinscard.dllをビルドする場合はMakefileの「-m32」の記述を削除すること（Linuxカーネルのビット数ではなくアプリケーションの.exeファイルのビット数）。


□□□　注意

一般的な録画・視聴ソフトで使われている関数のみ実装しており純正のwinscard.dllのすべての関数を実装しているわけではありません。実装が足りず動かない録画・視聴ソフトもあるかもしれません。TVTest.exeの動作を確認しています。64ビットでの動作は一切確認していません。
