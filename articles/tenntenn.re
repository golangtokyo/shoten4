= sync.Onceを味わう

== はじめに

株式会社メルペイのバックエンドエンジニアの@tenntenn@<fn>{fn1}です。
読者の皆様はGoの標準パッケージである@<code>{sync}パッケージにて提供されている@<code>{sync.Once}型を利用した経験はありますでしょうか。
並行処理を書く上で非常に便利な機能を提供する型であるため、利用した事がある人も多いでしょう。
しかし、そのソースコードを読んだことがある人は少ないのではないでしょうか。
実は@<code>{sync.Once}型の実装は短いながらも非常に面白く、実に"味わい深い"コードになっています。
本章では、そんな@<code>{sync.Once}型について様々な側面から味わい尽くします。

//footnote[fn1][@<href>{https://twitter.com/tenntenn}]

== sync.Onceを使って味わう

=== sync.Onceとは

@<code>{sync.Once}は標準パッケージのひとつである@<code>{sync}パッケージで提供されている型です。
関数の実行を1度だけに制限するために用います。
複数のゴルーチンから実行しても必ず1回だけに限定されます。

@<code>{sync.Once}を用いた例を見てみましょう。
@<list>{list1}のように、1回だけ実行したい関数を@<code>{sync.Once}の@<code>{Do}メソッドに渡します。
@<code>{sync.Once}は@<code>{Do}メソッドの呼び出しを1回に限定するように実装されているため、@<code>{Do}メソッドに渡す関数が異なっていても、1回だけしか実行されません。そのため、@<list>{list1}のコードを実行すると@<code>{Hello}と表示されます。

//list[list1][sync.Onceの利用例]{
func main() {
  var once sync.Once
  // 1回目
  once.Do(func() {
    fmt.Println("Hello")
  })
  // 2回目
  once.Do(func() {
    fmt.Println("Gophers")
  })
}
//}

@<code>{sync}パッケージで提供されている型のほとんどはゼロ値のまま使用できるように設計されています。
@<code>{sync.Once}も特別な初期化は不要です。
@<code>{var once sync.Once}のように変数定義しゼロ値のまま使用できます。

=== 複数のゴルーチンからの利用

@<code>{sync}パッケージはその名の通り、複数のゴルーチンから利用されることを想定されいるパッケージです。@<code>{sync.Once}も同様にスレッドセーフに実装されています。

@<list>{list2}は複数のゴルーチンから@<code>{sync.Once}を利用している例です。
@<code>{for}の中で10個のゴルーチンを生成し、それぞれで同じ@<code>{sync.Once}の@<code>{Do}メソッドを呼び出しています。
このように、複数のゴルーチンから呼び出されても必ず1回しか実行されません。

なお、@<list>{list2}では@<code>{sync.WaitGroup}を用いて、10個のゴルーチンの処理が終了するまで待機しています。

//list[list2][複数のゴルーチンから利用する]{
func main() {
  var wg sync.WaitGroup
  var once sync.Once

  // 10個のゴルーチンで呼び出す
  for i := 0; i < 10; i++ {
    wg.Add(1)
    go func() {
      defer wg.Done()
      once.Do(func() {
        fmt.Println("Hello")
      })
    }()
  }
  // すべてのゴルーチンの実行を待つ
  wg.Wait()
}
//}

=== 初期化に利用する

@<code>{sync.Once}の挙動は理解することができました。
実際にどのようなシーンで利用するのでしょうか。
ここでは@<code>{sync.Once}の利用シーンについて考えてみます。

1度しか実行されないという性質を活かし、初期化を行うために@<code>{sync.Once}を用いることがあります。
例えば、構造体型の値をゼロ値のまま使用するために利用できます。

@<list>{list3}では@<code>{Counter}構造体を定義しています。
@<code>{Count}メソッドを呼び出すと指定したキーでカウント行います。
キーごとにカウントをしなければならないため、マップ型の@<code>{cnt}フィールドで管理しています。
@<code>{cnt}フィールドは、@<code>{Count}メソッドを呼び出す前に初期化を行う必要があるため、
@<code>{NewCounter}関数が用意されています。

//list[list3][構造体を関数で初期化する]{
type Counter struct {
  cnt map[string]int
}

func NewCounter() *Counter {
  return &Counter{
    cnt: map[string]int{},
  }
}

func (c *Counter) Count(k string) int {
  c.cnt[k]++
  return c.cnt[k]
}

func main() {
  c := NewCounter()
  for i := 0; i < 10; i++ {
    fmt.Println(c.Count("cat"))
  }
}
//}

@<code>{NewCounter}関数を用いた初期化は問題なく動作しますが、@<code>{Counter}の初期化の方法を限定するものではありません。
うっかり@<code>{NewCounter}関数で初期化をせずに@<code>{Count}メソッドを呼び出すと、パニックが発生してしまいます。

@<list>{list4}のように、ゼロ値のまま@<code>{Counter}を利用できれば、正しく初期化されているか気にすること無く利用できます。

//list[list4][ゼロ値のまま利用する]{
func main() {
  var c Counter
  for i := 0; i < 10; i++ {
    fmt.Println(c.Count("cat"))
  }
}
//}

ゼロ値のまま扱うためには、@<code>{Count}メソッドを呼び出す際に@<code>{cnt}フィールドの初期化を行えば良いでしょう。
初期化は1度だけ行えばよいため、@<list>{list5}のように@<code>{sync.Once}を利用することで実現できます。

//list[list5][構造体をsync.Onceで初期化する]{
type Counter struct {
  initOnce sync.Once
  cnt      map[string]int
}

func (c *Counter) init() {
  c.initOnce.Do(func() {
    c.cnt = map[string]int{}
  })
}

func (c *Counter) Count(k string) int {
  c.init()
  c.cnt[k]++
  return c.cnt[k]
}
//}

@<code>{init}メソッドでは、@<code>{sync.Once}を用いて1回だけ@<code>{cnt}フィールドの初期化を行っています。
そして、@<code>{Count}メソッドで@<code>{init}メソッドを呼び出すことで、@<code>{Counter}をゼロ値のまま扱うことができるようになりました。

このように、ゼロ値で扱えるようにすることで、@<code>{Counter}型の利用者に正しく初期化を行う責任を負わすことなく、@<code>{Counter}型を利用できるようになりました。

== sync.Onceの実装を味わう

=== sync.Onceの実装

ここまで@<code>{sync.Once}を利用方法について扱ってきました。
ここからは実装について見ていきましょう。

執筆時点のGoの最新バージョンは、Go1.10です。
Go1.10における@<code>{sync.Once}の実装を@<list>{list6}のようになっています。

//list[list6][Go1.10におけるsync.Onceの実装]{
// Once is an object that will perform exactly one action.
type Once struct {
  m    Mutex
  done uint32
}

// Do calls the function f if and only if Do is being called for the
// first time for this instance of Once. In other words, given
//   var once Once
// if once.Do(f) is called multiple times, only the first call will invoke f,
// even if f has a different value in each invocation. A new instance of
// Once is required for each function to execute.
//
// Do is intended for initialization that must be run exactly once. Since f
// is niladic, it may be necessary to use a function literal to capture the
// arguments to a function to be invoked by Do:
//   config.once.Do(func() { config.init(filename) })
//
// Because no call to Do returns until the one call to f returns, if f causes
// Do to be called, it will deadlock.
//
// If f panics, Do considers it to have returned; future calls of Do return
// without calling f.
//
func (o *Once) Do(f func()) {
  if atomic.LoadUint32(&o.done) == 1 {
    return
  }
  // Slow-path.
  o.m.Lock()
  defer o.m.Unlock()
  if o.done == 0 {
    defer atomic.StoreUint32(&o.done, 1)
    f()
  }
}
//}

@<code>{sync.Once}の実装は、非常にシンプルでコードのほとんどがコメントになっています。
@<list>{list6}を見ると、構造体型として定義されており、フィールドとして@<code>{m}と@<code>{done}を持っていることが分かります。
また、メソッドは@<code>{Do}メソッドだけしか定義されていません。

シンプルですが非常に考えられた実装になっています。
実装を読んでいくうちに、その"味わい深さ"を感じることができます。
それでは、実装の"味わい深さ"を解説していきましょう。

=== Doメソッドが複数回呼ばれた時の挙動

これまで述べたように、@<code>{sync.Once}は初期化を行う際に用いられことがあります。
そのため、@<code>{Do}メソッドが複数回呼ばれた時の挙動が重要となってきます。

@<list>{list7}のように、@<code>{Do}メソッドで呼び出される関数の実行に時間がかかる場合を考えてみます。
1度目の@<code>{Do}メソッドが実行されている最中に2度目の呼び出される可能性があります。
2度目の@<code>{Do}メソッドの呼び出しでは、1度目の呼び出しが終わるまで内部でブロックが発生します。
つまり、2度目の実行以降では1度目の@<code>{Do}メソッドの呼び出しが必ず終了しています。
この挙動のおかげで、@<code>{Do}メソッド内で初期化を行っている場合、@<code>{Do}メソッドの呼び出しを行えば初期化が終了していることが保証されます。

//list[list7][Doメソッドを複数回呼んだ場合の挙動]{
func main() {
  var once sync.Once
  var wg sync.WaitGroup

  for i := 0; i < 10; i++ {
    i := i
    wg.Add(1)
    go func() {
      defer wg.Done()
      fmt.Println("START", i)
      once.Do(func() {
        time.Sleep(5 * time.Second)
      })
      fmt.Println("END", i)
    }()
  }
  wg.Wait()
}
//}

ここで@<code>{sync.Once}の実装を見てみましょう。
@<code>{sync.Once}型は、@<list>{list8}のように定義されています。
フィールド@<code>{m}は、@<code>{sync.Mutex}型で定義されていおり、複数のゴルーチンで@<code>{Do}メソッドが呼ばれた場合にロックをとるために用いられます。
フィールド@<code>{done}は、@<code>{Do}メソッドが呼び出されたかどうかを表す値です。
@<code>{0}の場合は、まだ呼び出されていないことを表します。

//list[list8][sync.Once型の定義]{
type Once struct {
  m    Mutex
  done uint32
}
//}

@<code>{Do}メソッドが複数回呼び出された際にブロックされる挙動は、フィールド@<code>{m}を用いて実現されています。
@<list>{list9}のように、@<code>{defer}でアンロックを行っているため、@<code>{Do}メソッドで渡した関数が終了してからロックが解除されます。

//list[list9][ロックを行っている部分]{
func (o *Once) Do(f func()) {
  // 略
  o.m.Lock()
  defer o.m.Unlock()
  // 略
}
//}

=== パニックが起きた場合の挙動

@<code>{Do}メソッドの呼び出しでパニックが発生した場合、どのような挙動になるのでしょうか？
@<list>{list10}のように、HTTPハンドラで使用する変数の初期化の際にパニックが発生した場合を考えてみましょう。

//list[list10][パニックが発生した場合の挙動]{
package main

import (
  "fmt"
  "log"
  "net/http"
  "sync"
)

var (
  n    int
  once sync.Once
)

func index(w http.ResponseWriter, r *http.Request) {
  once.Do(func() {
    panic("hoge") // うっかりパニックが発生してしまった
    n = 100
  })
  fmt.Fprintln(w, n)
  log.Println(n)
}

func main() {
  http.HandleFunc("/", index)
  http.ListenAndServe(":8080", nil)
}
//}

関数@<code>{index}はHTTPハンドラです。
このハンドラ内で使用している変数@<code>{n}は、@<code>{sync.Once}を用いて1度だけ@<code>{100}に初期化しようとしています。
しかし、初期化が行われる前にパニックが発生してしまったとします。
@<code>{Do}メソッドがパニックを起こすことになるため、リカバーしない限りコールスタックをたどりながら、上位の関数呼び出しにパニックの発生が伝えられていきます。
どの関数もリカバーしない場合は、プログラムが終了してしまいます。
しかし、HTTPハンドラがパニックを発生した場合、@<code>{net/http}パッケージによってハンドラ単位でリカバーされるため、プログラムが終了することはありません。

2度目のHTTPハンドラの呼び出しでは、@<code>{sync.Once}の@<code>{Do}メソッドの呼び出しはどのように処理がされるのでしょうか。
@<list>{list11}に示した@<code>{Do}メソッドの実装を見てみましょう。

//list[list11][Doメソッドの実装]{
func (o *Once) Do(f func()) {
  if atomic.LoadUint32(&o.done) == 1 {
    return
  }
  // Slow-path.
  o.m.Lock()
  defer o.m.Unlock()
  if o.done == 0 {
    defer atomic.StoreUint32(&o.done, 1)
    f()
  }
}
//}

@<code>{sync.Once}のフィールド@<code>{done}は、@<code>{Do}メソッドが呼び出されたかどうかを表すために用いられます。
@<code>{1}の場合は、すでに@<code>{Do}メソッドが呼び出されていることを表します。
フィールド@<code>{done}は、@<code>{sync/atomic}パッケージの@<code>{StoreUint32}関数を用いてセットされます。
実装を見ると@<code>{defer atomic.StoreUint32(&o.done, 1)}のように、@<code>{defer}で呼び出していることが分かります。
@<code>{defer}で呼び出しを遅延させた関数は、パニックが発生しても呼び出されます。
そのため、@<code>{Do}メソッドに渡した関数内でパニックが発生した場合でもフィールド@<code>{done}には、@<code>{1}がセットされます。
つまり、パニックが発生しようとも@<code>{sync.Once}は1度しか関数を実行しません。

このように、パニックが発生する可能性のある初期化を@<code>{sync.Once}で行ってしまうと、意図しない挙動になってしまう可能性があります。
実際に@<list>{list10}の例では、変数@<code>{n}は初期化されず、ずっと@<code>{0}のままになってしまいます。

メルカリ カウルでも、Google App Engineのインスタンスごとの初期化に@<code>{sync.Once}を使う予定でしたが結局は使用せず、@<list>{list12}のような自前実装を用意しました。
@<list>{list12}の実装では、@<code>{Do}メソッドはエラーを返す関数を引数にとることができるようになります。
そのため、パニックのままにしておきたくない場合は、リカバーしてエラーにすることで適切にハンドリングすることができるようになっています。
また、フィールド@<code>{done}には@<code>{defer}でセットしていないため、パニックやエラーが起きた場合は実行されたことになりません。

//list[list12][Doメソッドの実装]{
type Once struct {
  m    sync.Mutex
  done uint32
}

func (o *Once) Do(f func() error) error {
  if atomic.LoadUint32(&o.done) == 1 {
    return nil
  }
  // Slow-path.
  o.m.Lock()
  defer o.m.Unlock()
  if o.done == 0 {
    if err := f(); err != nil {
      return err
    }
    atomic.StoreUint32(&o.done, 1)
  }
  return nil
}
//}

== おわりに

本章では、@<code>{sync.Once}の基本的な利用法から内部の実装までを解説していきました。
@<code>{sync.Once}の実装はシンプルながらもよく考えられています。
本章を通して読者の皆様にも@<code>{sync.Once}の"味わい深さ"を感じ、今後の開発に活かして頂けたら幸いです。
