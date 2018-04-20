= 続・新しく型を定義することによって可能になる実装パターン
== はじめに
メルカリのバックエンドエンジニア@pospome@<fn>{pospome_fn1}です。普段は GCP + Go でサーバサイドの開発をしています。

//footnote[pospome_fn1][@<href>{https://twitter.com/pospome}]

技術書典3で頒布したGopherWalkerでは、"新しく型を定義することによって可能になる実装パターン" というタイトルで、主に@<code>{int},@<code>{string}などの "値を表現する型" に@<code>{type}を用いる実装パターンを紹介しました。

本章はその続編ということで、@<code>{func}に@<code>{type}を用いる実装パターンを紹介します。
GopherWalkerの内容と重複する部分もありますが、@<code>{func}という "処理" を表現するものならではの実装パターンになります。

== func は値である
Goでは、@<code>{func}を値として扱うことができます。次の例では@<code>{func}である@<code>{EchoName()}を値として変数@<code>{e}に代入し、実行しています。
//list[pospome_list1][EchoName() を値として変数eに代入し、実行する例]{
package main

import "fmt"

func main() {
	e := EchoName
	e() //pospome
}

func EchoName() {
	fmt.Println("pospome")
}
//}

@<code>{func}は値なので、引数、戻り値に指定することが可能です。
//list[pospome_list2][引数、戻り値にfuncを指定する例]{
package main

import "fmt"

func main() {
	f := Do(EchoName)
	f() //pospome
}

func EchoName() {
	fmt.Println("pospome")
}

func Do(f func()) func() {
	return f
}
//}

引数に@<code>{func}を指定する実装はとても便利です。次のように特定の@<code>{func}を外部から指定し、任意の処理を実行させることができます。
//list[pospome_list3][特定のfuncを外部から指定し、任意の処理を実行させる例]{
package main

import "fmt"

func main() {
	f1 := EchoName

	//無名関数として定義することもできる。
	f2 := func() {
		fmt.Print("not pospome")
	}

	Do(f1) //logic1,pospome,logic2
	Do(f2) //logic1,not pospome,logic2
}

func EchoName() {
	fmt.Print("pospome")
}

func Do(f func()) {
	fmt.Print("logic1,")
	f()
	fmt.Println(",logic2")
}
//}

== func に型を定義するとは?
@<code>{func}には型を定義できます。具体的には次のように定義することができます。
//list[pospome_list4][funcに対する定義]{
type MyEchoName func()
//}

次のように@<code>{func}に引数と戻り値を指定することも可能です。
//list[pospome_list5][funcに引数と戻り値を指定する]{
type MyEchoName func(id int) (int, error)
//}

== func を type として定義した際にできるようになること
@<code>{func}に@<code>{type}を定義することによって、次のようなことができるようになります。

 * キャスト
 * メソッド定義
 * @<code>{interface}を満たす

=== キャスト
型を満たす@<code>{func}はキャストすることができます。

次の例では@<code>{EchoName()}を@<code>{func()}から@<code>{MyEchoName}へキャストしています。型が@<code>{func()}から@<code>{MyEchoName}へ変わっただけで挙動に変化はありません。
//list[pospome_list6][funcのキャスト]{
package main

import "fmt"

func main() {
	e := EchoName
	m := MyEchoName(e)
	m() //pospome
}

type MyEchoName func()

func EchoName() {
	fmt.Println("pospome")
}
//}

=== メソッド定義
@<code>{func}に@<code>{type}を用いることでメソッドを定義することができます。次の例では@<code>{MyEchoName}に@<code>{DoSomething()}というメソッドを定義してます。
//list[pospome_list7][funcに対するメソッド定義]{
package main

import "fmt"

func main() {
	e := EchoName
	m := MyEchoName(e)
	m.DoSomething() //DoSomething
}

type MyEchoName func()

func (e MyEchoName) DoSomething() {
	fmt.Print("DoSomething")
}

func EchoName() {
	fmt.Print("pospome")
}
//}

@<code>{func}にメソッドを定義できるのって何か不思議だなーと思うのは、私だけでしょうか・・・?

=== interfaceを満たす
メソッドを定義できるのであれば、当然@<code>{interface}を満たすことができます。次は@<code>{MyEchoName}が@<code>{XxxInterface}を満たしている実装です。
//list[pospome_list8][funcのメソッドがinterfaceを満たす]{
package main

import "fmt"

func main() {
	e := EchoName
	m := MyEchoName(e)
	Do(m)
}

type XxxInterface interface {
	Xxx()
}

type MyEchoName func()

func (e MyEchoName) Xxx() {
	fmt.Print("Xxx")
}

func EchoName() {
	fmt.Print("pospome")
}

func Do(x XxxInterface) {
	x.Xxx()
}
//}

== 実装パターン
=== 引数を型で明示する
仕様が複雑なWebサービスのアプリケーションコードにおいて、"一部だけ処理の流れが違う" というケースはよくあるのではないでしょうか?
//list[pospome_list9][一部だけ処理の流れが違う例]{
func Do() {
	//前半の処理は常に同じ

	//途中の処理だけ常に同じとは限らない

	//後半の処理も常に同じ
}
//}

この場合、次のように@<code>{if}を利用して解決することができます。
//list[pospome_list10][一部だけ処理の流れが違うケースに対してifで解決する]{
func Do() {
	//前半の処理は常に同じ

	//途中の処理だけ常に同じとは限らない
	if xxx {
		//省略
	} else {
		//省略
	}

	//後半の処理も常に同じ
}
//}

しかし、次のケースでは引数に@<code>{func}を指定して解決することが多いでしょう。

 * @<code>{if}の分岐パターンが予想できない場合 
 * @<code>{Do()}の内部で分岐処理を書くと見通しが悪くなる場合
 * @<code>{interface}で表現すると不自然になる場合

//list[pospome_list11][引数にfuncを指定して解決する]{
func Do(f func()) {
	//前半の処理は常に同じ

	//途中の処理だけ常に同じとは限らない
	f()

	//後半の処理も常に同じ
}
//}

@<code>{func}の定義方法は複数存在します。次のように無名関数として@<code>{func}を定義する方法です。
//list[pospome_list12][無名関数としてfuncを定義する]{
package main

import "fmt"

func main() {
	//無名関数として定義する
	f := func() {
		fmt.Println("pospome")
	}
	Do(f)
}

func Do(f func()) {
	//前半の処理はいつも同じ

	//途中の処理だけ常に同じとは限らない
	f()

	//後半の処理もいつも同じ
}
//}

このコードには"具体的に何の処理なのかが分かりづらい"という問題があります。無名関数は文字通り "無名" なので、具体的に何の処理なのかを明示することができません。あくまで処理内容がコードとして記載されているだけです。

これは無名関数を通常の関数として定義することで解決することができます。次の例は無名関数の実装を@<code>{EchoName()}として定義しています。
//list[pospome_list13][無名関数を通常の関数として定義する]{
package main

import "fmt"

func main() {
	e := EchoName
	Do(e)
}

func Do(f func()) {
	//前半の処理はいつも同じ

	//途中の処理だけ同じとは限らないので引数で渡される f を利用する
	f()

	//後半の処理もいつも同じ
}

//関数名で具体的な処理を表現できる。
//他の箇所で使い回すことができる。
func EchoName() {
	fmt.Println("pospome")
}
//}

ちなみに、無名関数内の処理が単純で自明なものであれば、無名関数として定義しても問題ありません。単純で自明なものであるかどうかの基準はしっかりと考える必要があります。

これで解決したように見えますが、まだ1つだけ問題が残っています。それは"引数に何を指定するか分かりづらい" という点です。次の例では@<code>{Do()}の引数は@<code>{func()}です。
//list[pospome_list14][引数がfuncに何を指定するか分かりづらい]{
func Do(f func()) {
	//前半の処理はいつも同じ

	//途中の処理だけ同じとは限らないので引数で渡される f を利用する
	f()

	//後半の処理もいつも同じ
}
//}

@<code>{func()}には具体的には何を指定すればいいのでしょうか?どこかに定義されている関数を指定するのでしょうか?それとも無名関数で実装するのでしょうか?もしどこかに定義されている関数を指定する場合、それはどの関数なのでしょうか?この問題は@<code>{func}に@<code>{type}を用いることで解決することができます。

次の例は@<code>{Do()}の引数を@<code>{func()}から@<code>{EchoName}に変更しています。
//list[pospome_list15][特定のfuncを外部から指定し、任意の処理を実行させる例]{
package main

import "fmt"

func main() {
	p := NewEchoPospome()
	Do(p)
}

func Do(e EchoName) {
	//前半の処理はいつも同じ

	//途中の処理だけ同じとは限らないので引数で渡される f を利用する
	e()

	//後半の処理もいつも同じ
}

type EchoName func()

//pospome用の実装
func NewEchoPospome() EchoName {
	f := func() {
		fmt.Println("pospome")
	}
	return EchoName(f)
}

//他の人用の実装
func NewEchoSomebody() EchoName {
	f := func() {
		fmt.Println("somebody")
	}
	return EchoName(f)
}
//}

@<code>{EchoName}を定義し、それを生成する@<code>{NewEchoPospome()},@<code>{NewEchoSomebody()}を実装することで次の項目が明示的に表現できます。

 * @<code>{Do()}に指定する引数が@<code>{EchoName}であること
 * @<code>{EchoName}には Pospome実装とSomebody実装の2つのみ提供されていること
 * 提供されている実装以外の実装が必要な場合、新規実装しなければいけないこと

例として挙げたコードは@<code>{Do()},@<code>{EchoName}のように比較的抽象的だったので、有効性がイメージできないかもしれませんが、次のように具体的なロジックを表現する場合、単なる@<code>{func()}という定義よりも分かりやすくなるでしょう。
//list[pospome_list16][ユーザーのスコア算出ロジックを切り替える例]{
type UserScoreLogic func(userScore int) (totalScore int)

func NewNormalUserLogic() UserScoreLogic {
	//省略
}

func NewSpecialUserLogic() UserScoreLogic {
	//省略
}

//ユーザーの score 算出ロジックを切り替えられることが分かる。
func Do(f UserScoreLogic) {

}
//}

=== ロジックの差し替え
本節で紹介する "ロジックの差し替え" は、@<code>{func}そのものの性質を利用した実装パターンです。
@<code>{func}に@<code>{type}を用いることによる実装パターンではないのですが、この機会に紹介させていただきます。

前節でも言及したように、@<code>{func}はそれ自体がGoにおけるプリミティブな定義なので、@<code>{func}を引数に指定することによって任意の処理を実行させることができます。

例えば、次の実装の場合、@<code>{Do()}の引数である@<code>{DoFunc}を満たす値を指定することで、任意の処理を実行させることができます。
//list[pospome_list17][funcを引数に指定することによって任意の処理を実行させる例]{
package main

import "fmt"

func main() {
	Do(NewHelloFunc())
}

func Do(f DoFunc) {
	f()
}

type DoFunc func()

func NewHelloFunc() DoFunc {
	f := func(){
		fmt.Println("hello")
	}
	return DoFunc(f)
}
//}

こういった "処理の差し替え" は、@<code>{interface}を利用することでも実現できます。次の例は@<code>{struct}+@<code>{interface}を組み合わせたものです。
//list[pospome_list18][struct+interfaceの例]{
package main

import "fmt"

func main() {
	h := &Hello{}
	Do(h)
}

func Do(f DoFunction) {
	f.Exec()
}

type DoFunction interface {
	Exec()
}

//フィールドを持たない struct になる
type Hello struct {
}

func (h *Hello) Exec() {
	fmt.Println("hello")
}
//}

@<list>{pospome_list18}の@<code>{Hello struct}は、@<code>{int}に変更することも可能です。
//list[pospome_list][int+interfaceの例]{
//int でも問題ない
type Hello int
//}

@<code>{func}を利用しても、@<code>{interface}を利用しても "処理の差し替え" は可能です。では、@<code>{func}と@<code>{interface}はどのように使い分ければいいのでしょうか?個人的には次の基準で判断しています。

 * @<code>{interface}を実装する対象のtypeが機能しないのであれば、@<code>{func}を利用する。
 * @<code>{interface}を実装する対象のtypeが機能するのであれば、@<code>{interface}を利用する。

@list{pospome_list18}では、@<code>{Hello struct}がフィールドを持ちません。フィールドを持たない@<code>{struct}だからこそ、@<code>{struct}を@<code>{int}に変更しても特に問題が発生することはありません。@<code>{interface}を実装する対象の@<code>{type}は何でもいいのです。つまり、@<code>{struct}は機能していないことになります。@<code>{DoFunction interface}の@<code>{Exec()}は引数の値によって戻り値が決まる振る舞いなので、@<code>{interface}である必要がありません。こういった場合は@<code>{func}を利用して処理を差し替える方が適しているでしょう。

しかし、実際はそう簡単に判断できる問題ではありません。将来的な要件によって@<code>{interface}の方が適している場合もあるでしょうし、チームの方針、個人の好みもあるでしょう。使い分けの判断は難しいですが、 "なんとなくfuncにする" という個人の感覚による実装は避けましょう。感覚によって実装する癖が付いてしまうと、本来あるべき正しさを考えずにコードを書く癖が付いてしまいます。それを避けるためにも  "なぜそういった判断をしたのか?" だけは明確に説明できるようにしておきましょう。空の@<code>{struct}に@<code>{interface}を実装するコードを書こうとした時は一度立ち止まって考えてみることをおすすめします。

ちなみに、@<code>{interface}には "複数の振る舞いを定義できる" という仕様がありますが、@<code>{func}は値なので複数定義することはできません。
//list[pospome_list19][interfaceに複数の振る舞いを定義する]{
type DBFunc interface {
	Begin()
	Commit()
	Rollback()
}
//}

これは次のように@<code>{func}を束ねる@<code>{struct}を用意することで解決できます。
//list[pospome_list20][funcを束ねるstructを用意する]{
type DBFunc struct {
	Begin Begin
	Commit Commit
	Rollback Rollback
}

type Begin func()
type Commit func()
type Rollback func()
//}

複数の@<code>{func}を@<code>{struct}で定義する場合は、上記の@<code>{Begin},@<code>{Commit},@<code>{Rollback}のように明示的に@<code>{type}を定義すると、
@<code>{struct}との関連性が分かりやすくなります。

=== interfaceを提供することによる実装選択肢の確保
前節では "処理の差し替え" を@<code>{func}によって実現するという例を紹介しました。しかし、@<code>{func}によって "処理の差し替え" を実現し、さらに@<code>{interface}も定義するケースが存在します。

次の例は前節で紹介したものです。
//list[pospome_list21][funcを引数に指定することによって任意の処理を実行させる例]{
package main

import "fmt"

func main() {
	Do(NewHelloFunc())
}

func Do(f DoFunc) {
	f()
}

type DoFunc func()

func NewHelloFunc() DoFunc {
	return func(){
		fmt.Println("hello")
	}
}
//}

一見問題ないように見えますが、コードは時と共に要件が変化していくものです。1年後、@<code>{Do()}の引数に@<code>{struct}を指定したくなる可能性もあります。その際、既存の@<code>{DoFunc}を利用しているコードに影響を与えることは避けたいでしょう。この問題は@<code>{interface}を定義することで解決できます。

具体的には、@<code>{Do()}の引数に@<code>{func}ではなく、@<code>{DoInterface}を指定するように変更します。そして、@<code>{DoFunc}には@<code>{DoInterface.Call()}を実装します。@<code>{DoFunc.Call()}は@<code>{DoFunc}をそのまま実行するだけです。
//list[pospome_list22][引数をfuncからinterfaceに変更]{
package main

import "fmt"

func main() {
	//ここのコードは修正不要
	Do(NewHelloFunc())
}

//引数を DoInterface に変更
func Do(d DoInterface) {
	d.Call()
}

type DoInterface interface {
	Call()
}

type DoFunc func()

// DoFunc を実行するだけのメソッド
func (d DoFunc) Call() {
	d()
}

//ここの戻り値は DoFunc のままでOK
func NewHelloFunc() DoFunc {
	f := func(){
		fmt.Println("hello")
	}
	return DoFunc(f)
}
//}

@<code>{Do()}の引数を@<code>{interface}にすることで、次のように@<code>{struct}を引数に指定することが可能になります。
//list[pospome_list23][interfaceによってstructを引数に指定する]{
type DoStruct {
	X, Y int
}

func (d DoStruct) Call() {
	fmt.Println(d.X)
	fmt.Println(d.Y)
}
//}

この例を見る限り、@<code>{interface}は便利なので、全て@<code>{interface}にした方がいいのでは? と思うかもしれませんが、そうとも限りません。

@<code>{interface}は "複数のtypeに共通する振る舞い" を定義するものです。将来必要になるかも分からない段階で@<code>{interface}を実装してしまうと、実装対象の仕様とコードにミスマッチが生まれてしまい、コードから伝わる意図が間違ったものになってしまいます。意味もなく@<code>{interface}を適用させるのは避けましょう。

反面、不特定多数が利用するライブラリ、フレームワークなどは、利用者のニーズを絞り込むことが難しいことが多いです。利用者によっては、@<code>{struct}に振る舞いを実装したいかもしれませんし、@<code>{int}に振る舞いを実装したいかもしれません。こういった利用者のニーズに幅広く対応するのを目的とした場合、@<code>{interface}を提供する場合があります。

=== メソッドを利用した特定処理の差し込み
@<code>{func}に@<code>{type}を用いることによって、メソッドを定義することができます。
//list[pospome_list24][funcにメソッドを定義]{
type Xxx func()

func (x Xxx) Do() {
}
//}

レシーバは@<code>{func}なので、@<code>{func}を実行する前後に特定の処理を差し込むことが可能です。次の例では引数@<code>{userScore}に対するバリデーションをメソッドによって実装しています。
//list[pospome_list25][引数 userScore に対するバリデーションをメソッドによって実装]{
package main

import "fmt"

func main() {
	userScore := 100

	c := NewTotalCalcScore()

	fmt.Println(c(userScore)) //100
	fmt.Println(c.WithValidation(userScore))//100

	userScore = 0
	fmt.Println(c.WithValidation(userScore))//panic: score = 0
}

type CalcTotalScore func(userScore int) (totalScore int)

func NewTotalCalcScore() CalcTotalScore {
	f := func(userScore int) (totalScore int) {
		//計算ロジックが実装されている想定
		return userScore
	}
	return CalcTotalScore(f)
}

func (c CalcTotalScore) WithValidation(userScore int) (totalScore int) {
	if userScore == 0 {
		panic("score = 0")
	}
	return c(userScore)
}
//}

@list{pospome_list25}では、@<code>{CalcTotalScore} = "スコア計算" がメインの目的であり、それに対するオプションとして@<code>{WithValidation()}が提供されています。実際にこのようなバリデーションが適切な実装かどうかは判断しかねますが、メソッドによって処理を差し込むイメージは理解できたはずです。

アプリケーションコードでは特定の処理をベースに複数の挙動が想定されているケースが存在します。そのようなケースでは、こういった実装を上手く活用できる可能性があります。

== おわりに
本章では次の実装パターンを紹介しました。

 * 引数を型で明示する
 * ロジックの差し替え
 * @<code>{interface}を提供することによる実装選択肢の確保
 * メソッドを利用した特定処理の差し込み

頭の片隅にしまっておくと、いつか役に立つかも知れませんね。
