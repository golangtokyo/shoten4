= Protocol Buffers

== はじめに
ピックアップ株式会社で新規事業のサーバイサイドエンジニアをしている@sonatard@<fn>{sonatard_fn1}です。

//footnote[sonatard_fn1][@<href>{https://twitter.com/sonatard}]

新規事業を始める際には様々な技術スタックの選定が必要になります。
本章ではその中でもクライアントとサーバ間でやりとりする構造化データ仕様のProtocol Buffersと周辺技術について紹介します。

特に4つの点をお伝えします。

 * GoでProtocol Buffersの採用は容易である
 * JSONからの移行は容易である
 * gRPCのメリットとして紹介される内容の一部はProtocol Buffersによるものである
 * gRPCを使わなくともProtocol Buffersだけを採用することが可能である

== Protocol Buffersの概要

=== Protocol Buffersとは

Protocol Buffersは、言語とプラットフォームに依存しない構造化されたデータをシリアライズするためのものです。
テキストフォーマットのXMLやJSONより小さく、速く、そしてシンプルです。protoファイルにデータを構造化する方法を定義してから、
特別に生成されたソースコードを使用して、様々な言語で構造化データを簡単に読み書きすることができます。

=== メリット

==== 開発効率
最大のメリットはクライアントとサーバの間で言語に依存せずコンパイルが可能になることです。
JSONを利用する場合はサーバの提供するAPI仕様をドキュメントから理解して、クライアントエンジニアが正しく実装しなければなりません。
もし間違った実装をしても実際にリクエストを送信して結果を確認しないことには検証することができません。
しかしProtocol Buffersではprotoファイルという共通仕様からクライアントとサーバのコードを生成し、それを利用することでコンパイル時にバグを発見することができます。

またソースコードが生成されることで、各言語のIDEやエディタにより補完が効くようになるため開発効率が向上します。

==== 実行速度
バイナリフォーマットであるため通信経路上を流れるデータサイズが小さくなり、スループットが向上します。
またJSONなどのテキストのフォーマットと比較して、バイナリフォーマットであるProtocol Buffersはエンコード、デコードの処理が高速です。


=== デメリット
==== 環境構築
言語ごとに環境構築が必要です。
公式の@<code>{protoc}コマンドだけではすべての言語に対応していないためサードパーティ製のツールをインストールする必要があります。
しかし難しいことではないためデメリットというほどのことでもありません。

==== デバッグ
バイナリフォーマットであるためJSONのようにそのままHTTPリクエストのボディに乗せて送信することができません。
サーバでHTTPヘッダの@<code>{Content-Type}からJSONかProtocol Buffersかを判断してデコードするようにしておくと開発時のストレスがなくなります。
gRPCを利用していれば@<code>{grpc-gateway}を使う解決策もあります。


=== Protocol Buffersを用いた開発

実際にProtocol Buffersを利用したGoの開発の例を紹介します。

==== protoファイルから生成したコードをGoから利用する

//list[person.proto][protoファイルの例]{
syntax = "proto3";

message Person {
  required string name = 1;
  required int32 id = 2;
  optional string email = 3;
}
//}

//list[protoc_person.proto][protoファイルからのソースコード生成するコマンド]{
$ protoc --go_out=. person.proto
$ ls
person.pb.go  person.proto
//}

生成された@<code>{person.pb.go}と@<list>{proto_main.go}の@<code>{main.go}を@<list>{proto_directory}のように配置してビルドをします。

//list[proto_directory][ディレクトリ構成]{
${GOPATH}/github.com/sonatard/proto
├── Makefile
├── main
│   └── main.go
├── person.pb.go
└── person.proto
//}


//list[proto_main.go][main.go 生成されたperson.Person構造体の利用]{
package main

import (
	"fmt"
	"github.com/sonatard/proto"
)

func main() {
	p := person.Person{
		Name:  "sonatard",
		Id:    12345,
		Email: "sonatard@example.com",
	}
	fmt.Printf("%#v", p)
}
//}

//list[person.proto_stdout][生成されたperson.Personの構造体利用 実行結果]{
person.Person{Name:"sonatard", Id:12345, Email:"sonatard@example.com"}
//}


==== protoファイルで定義した構造化データをエンコードしてHTTPで送信、受信してデコード

//list[proto_main2.go][生成されたperson.Personのエンコードとデコード]{
package main

import (
    "bytes"
    "fmt"
    "io"
    "io/ioutil"
    "net/http"
    "os"

    "github.com/golang/protobuf/proto"
    "github.com/sonatard/proto"
)

func main() {
	// サーバ
	handler := func(w http.ResponseWriter, r *http.Request) {
		body, err := ioutil.ReadAll(r.Body)
		if err != nil {
			http.Error(w, "Failed to read body", http.StatusBadRequest)
			return
		}

		var p person.Person
		err = proto.Unmarshal(body, &p)
		if err != nil {
			http.Error(w, "Failed to unmarshal", http.StatusBadRequest)
		}
		fmt.Fprintf(w, "#%v", p)
	}

	http.HandleFunc("/", handler)
	go http.ListenAndServe(":8080", nil)

	// クライアント
	p := person.Person{
		Name:  "sonatard",
		Id:    12345,
		Email: "sonatard@example.com",
	}
	pbytes, err := proto.Marshal(&p)
	if err != nil {
		fmt.Errorf("%#v", err)
	}
	resp, err := http.Post("http://localhost:8080/", "application/x-protobuf",
	                      bytes.NewBuffer(pbytes))
	if err != nil {
		fmt.Errorf("%#v", err)
	}

	io.Copy(os.Stdout, resp.Body)
}
//}


このようにGoを書く上ではJSONとProtocol Buffersの違いは少なく、
構造体にjsonタグを指定するか、protoファイルから構造体を生成するかという違いしかありません。

=== GoとProtocol Buffersの型の対応

GoとProtocol Buffersの型は名称が異なります。

//table[identifier][GoとProtocol Buffersの型の対応]{
Go	Protocl Buffers
--------------------------------------------------------------------------
float64	double
float32	float
int32	int32
int64	int64
bool	bool
string	string
time.Time	google.protobuf.Timestamp
slice	型の前にrepeatedを追加
//}


@<code>{time.Time}はProtocol Buffersの標準で規定されていないためGoogleが作成した@<code>{google/protobuf/timestamp.proto}のインポートが必要になります。
また@<code>{github.com/golang/protobuf/ptypes}にタイムスタンプに関連する関数が用意されています。
現在時刻の取得をする@<code>{func TimestampNow() *tspb.Timestamp}、Goの@<code>{time.Time}から@<code>{Timestamp}に変換する@<code>{func TimestampProto(t time.Time) (*tspb.Timestamp, error)}などがあります。


== 技術選定

本章では、Protocol Buffersと比較対象となる技術スタックを紹介します。
周辺技術としてHTTP、gRPC、REST、XML、JSON、JSON-RPCなどがありますが、これらは単純比較できるものではありません。
正しく比較検討するためには、同じレイヤーの技術間で比較しなければなりません。
比較検討の対象を誤ると「Protocol Buffersを利用するためにはgRPCを利用しなければならない」というような誤解をしてしまいます。
そこで周辺技術がどのような要素を持ち、何を規定しているのかを整理します。

=== 構成要素と規定している仕様

	* ◎はそのプロトコルが規定している対象。
	* ○はそのプロトコルが使うことを定めている他のプロトコル。
	* ×はそのプロトコルが規定せず、自由に変更可能なプロトコル。

//table[identifier][構成要素と規定している仕様]{
.					通信プロトコル		API実行ルール	構造化データ仕様
------------------------------------------------------------------------------------
HTTP				◎					×				×
gRPC				○(HTTP2)			◎				×(標準がProtocol Buffers)
JSON-RPC			×(HTTPをよく使う)	◎				○(JSON)
REST				○(HTTP)				◎				×(JSONをよく使う)
XML					×					×				◎
JSON				×					×				◎
Protocol Buffers	×					×				◎
//}


=== 選定対象となる技術の整理

==== 通信プロトコル
HTTP/HTTP2, その他プロトコルから選定することができます。現時点ではWebサービスを作る上ではほとんどがHTTP/HTTP2となります。
将来的にはTCP+HTTP2からUDP+QUIC+HTTP2への移行も考えられますが、クラウドを利用している限りあまり意識することはないでしょう。

選定基準は、インフラが対応しているか、性能、機能面に不足がないかなどになります。

==== API実行ルール
REST、JSON-RPC、gRPCなどから選定します。

選定基準は、インフラが対応しているか、URL設計に合うか、性能、機能面に不足がないか、開発言語のライブラリが揃っているかなどになります。

==== 構造化データ仕様
XML、JSON、Protocol Buffersなどから選定します。
Protocol BuffersはgRPCの標準として採用されていますが、API実行ルールがgRPCである必要はありません。
そのためgRPCが利用できないGoogle App EngineでHTTP+Protocol Buffersを採用することも可能です。
またHTTP+JSON-RPCという環境からHTTPはそのままにProtocol Buffersを利用することが可能です。
その場合にはJSON-RPCのJSON部分をProtocol Buffersに置き換えますが、RPCのAPI実行ルール部分はそのまま利用することになります。つまりHTTP+JSON-RPCのRPC部+Protocol Buffersという構成になります。

選定基準は、性能、機能面に不足がないか、開発言語のライブラリが揃っているかなどになります。

=== インフラによる制限

Goを採用する企業では、Google Container Engine、Google App Engine、Amazon EC2の利用が多いですが、
Protocol Buffersはインフラに制限されることはなくすべての環境で利用できます。


== おわりに
Protocol Buffersは、構造化データ仕様を定めているものであり様々な通信プロトコルやAPI実行ルールと組み合わせて利用することができます。
JSONの課題を解決しており、Protocol Buffersの導入障壁も低いため新規プロジェクトを始める際は是非候補にしてください。
インフラの制限がなく、より技術的なチャレンジが可能であればgRPCにも挑戦してみてください。
みなさんが技術選定する際に少しでもお役に立てれば幸いです。


== TIPS

私たちが採用しているAPI(Go)、iOS(Swift)、Andoird(Kotlin)、Web(TypeScript)のProtocol Buffersの環境構築手順と開発している中で知ったprotoファイルのTIPSを紹介します。

=== 開発環境構築

//table[identifier][環境]{
環境			開発言語		protoファイルからのソースコード生成ツール
--------------------------------------------------------------------------
サーバサイド	Go				https://github.com/golang/protobuf/protoc-gen-go
iOS				Swift			https://github.com/apple/swift-protobuf
Andoird			Kotlin			https://github.com/square/wire
Web				TypeScript		https://github.com/dcodeIO/ProtoBuf.js
//}


=== Mac環境構築、実行

==== 共通

//list[protobuf_install][Protocol Buffersのインストール]{
brew install protobuf
//}

==== Go

//list[go_install][Goのインストール]{
go get -u github.com/golang/protobuf/protoc-gen-go
//}

//list[protoc_go][.protoから.goを生成]{
protoc --go_out=. *.proto
//}

==== Swift

XCodeをインストールします。

//list[swift_protoc_install][Swift関連ツールのインストール]{
git clone https://github.com/apple/swift-protobuf
cd swift-protobuf
swift build -c release -Xswiftc -static-stdlib
# 生成されたprotoc-gen-swiftをパスが通る場所に移動
//}

//list[protoc_swift][.protoから.goを生成]{
protoc --swift_out=. *.proto
//}

==== Kotlin(Java)公式

生成されるのはJavaコードです。
protocコマンドが標準で対応しています。

//list[protoc_java][.protoから.javaを生成]{
protoc --java_out=. *.proto
//}


==== Kotlin(Java) square/wireの利用

生成されるのはJavaコードです。
公式のprotocコマンドが生成するJavaのソースコードサイズがとても大きいという問題があるためsquare/wireを使うことで解決します。

Javaをインストールします。

wireをインストール。
https://search.maven.org/remote_content?g=com.squareup.wire&a=wire-compiler&c=jar-with-dependencies&v=LATEST

//list[protoc_java_by_wire][.protoから.javaを生成]{
java -jar wire-compiler-2.3.0-RC1-jar-with-dependencies.jar \
     --proto_path=. --java_out=. *.proto
//}

==== TypeScript

//list[ts_protoc_install][TypeScript関連ツールのインストール]{
npm install protobufjs
export PATH=${PATH}:${HOME}/node_modules/.bin
//}

//list[protoc_typescript][.protoから.tsを生成]{
pbjs -t static-module *.proto | pbts -o proto.d.ts -
//}


=== protoファイルのTIPS

==== protoファイルにパッケージを設定する

パッケージを設定すると生成されるGoのソースコードも同様のパッケージになります。
パッケージを設定しない場合にはファイル名がパッケージ名になります。

//list[person_proto1][パッケージの指定]{
syntax = "proto3";

package person;

message Person {
  required string name = 1;
  required int32 id = 2;
  optional string email = 3;
}
//}

==== protoファイルにパッケージを指定しつつ、生成するコードは別パッケージ名にする
生成されるソースコードのパッケージ名をprotoファイルとは別に指定する場合には@<list>{person_proto2}のようにprotoファイルに設定します。

//list[person_proto2][パッケージの制御]{
syntax = "proto3";
option go_package = "pb";
option java_package = "pb";

package person;

message Person {
  required string name = 1;
  required int32 id = 2;
  optional string email = 3;
}
//}

==== protoファイルにパッケージを指定した場合に、Swiftのソースコードに追加される余計なプレフィックスを削除する。

//list[person_proto3][swiftのプレフィックスを削除]{
syntax = "proto3";
option swift_prefix="";

package person;

message Person {
  required string name = 1;
  required int32 id = 2;
  optional string email = 3;
}
//}

