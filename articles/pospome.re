= 新しく型を定義することによって可能になる実装パターン

== はじめに
DeNAのサーバサイドエンジニア@pospome@<fn>{pospome_fn1}です。
普段は@<kw>{Google App Engine for Go,GAE/Go}を使ってゲームプラットフォームを開発、運用しています。

//footnote[pospome_fn1][@<href>{https://twitter.com/pospome}]

本章では"@<code>{type}を用いて新しい型を定義できる"という
Goの言語仕様によって可能になる実装パターンについて紹介します。

== 新しい型を作る

"新しい型を作る"とはどういうことでしょうか。

具体的には@<code>{int64}型などの既存の型を@<code>{type}を用いることによって
@<list>{pospome_list1}のように、新しい型として定義することを指します。

//list[pospome_list1][int64型を基にしてUserID型を定義した例]{
type UserID int64
//}

もちろん@<code>{int64}型のような基本型だけではなく、@<list>{pospome_list2}のように
関数やスライスなども同様に定義できます。

//list[pospome_list2][関数やスライスを基に型を定義した例]{
type UserIDs []int64

type UserFunc func(id int64) error
//}


== 新しい型を定義するメリット

新しい型を定義することによって、
たとえば、次のようなメリットを享受することができます。

 * 型が@<code>{int64}型ではなく、@<code>{UserID}型になる
 * メソッドを実装できる

=== 型が@<code>{int64}型ではなく、@<code>{UserID}型になる

@<list>{pospome_list3}では、@<code>{UserID}型を@<code>{int64}型を基にして、
新しい型として定義しています。

//list[pospome_list3][UserIDの定義]{
type UserID int64
//}

そのため、@<list>{pospome_list4}のように@<code>{UserID}型の引数に@<code>{int64}型の値を指定することはできません。
指定するとコンパイルエラーになります。

//list[pospome_list4][int64型をUserID型の引数に渡した例]{
type UserID int64

func main() {
    var id int64 = 100

    // cannot use id (type int64) as type UserID
    // in argument to GetByUserID
    GetByUserID(id)
}

func GetByUserID(id UserID) {
    // 省略
}
//}

そのため、@<list>{pospome_list5}のように@<code>{UserID}型の値を指定する必要があります。

//list[pospome_list5][UserID型を用いて引数に渡した例]{
package main

type UserID int64

func main() {
    // id を UserID 型にする
    var id UserID = 100
    GetByUserID(id)
}

func GetByUserID(id UserID) {
    //省略
}
//}

=== メソッドを実装できる

@<list>{pospome_list6}のように、@<code>{UserID}型に@<code>{Hello()}という
メソッドを実装することが可能です。

//list[pospome_list6][UserID型にメソッドを実装した例]{
package main

import (
    "fmt"
)

type UserID int64

func (u UserID) Hello() string {
    return "Hello UserID"
}

func main() {
    var id UserID = 100
    fmt.Println(id.Hello())
}
//}

また、メソッドを定義できるため、インタフェースを実装することも可能です。
@<list>{pospome_list7}は@<code>{UserID}型に@<code>{Stringer}インタフェースを実装した例です。
なお、@<code>{fmt.Println}関数は引数に@<code>{Stringer}インタフェースを実装した値が渡されると、
その@<code>{String}メソッドを呼び出して、標準出力に文字列を表示します。

//list[pospome_list7][Stringerインタフェースを実装した例]{
package main

import (
    "fmt"
)

type UserID int64

func (u UserID) String() string {
    return "this is UserID"
}

func main() {
    var id UserID = 100
    fmt.Println(id) // this is UserID
}
//}

== 実装パターン

ここでは次の実装パターンについて紹介します。

 * 振る舞いの実装
 * インターフェースを実装
 * 基本型に具体性を持たせる
 * スライスに対する振る舞い実装


=== 振る舞いの実装

基本型を基に新しい型を定義する際の一番の動機が"振る舞いの実装"ではないでしょうか。
例えば、string（文字列）は非常に多様な振る舞いを要求されることがあります。
@<list>{pospome_list8}はユーザ名を表示する際に"様"という敬称を付ける場合の実装です。

//list[pospome_list8][振る舞いを実装した例]{
package main

import (
    "fmt"
)

type UserName string

func (u UserName) String() string {
    return string(u) + " 様"
}

func main() {
    var name UserName = "pospome"
    fmt.Println(name) // pospome 様
}
//}

このように、@<code>{String}メソッドを@<code>{UserName}型に実装したことによって、
文字列を@<code>{UserName}型にすることで"様"という敬称をつけることができます。

たとえば仮に、メソッドとして実装しない場合、
"様"という敬称を付けるコードが複数箇所に書かれてしまう可能性があります。

//list[pospome_list9][複数箇所で"様"をつける場合]{
package main

import (
    "fmt"
)

type UserName string

func main() {
    var name UserName = "pospome"

     // 「様」を付けるコードがあらゆるところに散る可能性がある。
    fmt.Println(name + " 様")
}
//}

ここでは@<code>{string}型を基にした例で説明しました。
しかし、@<code>{string}型に限らず、その値に関連する振る舞いをその値自体が持つことで、
コードの重複を防ぐことができます。

=== インターフェースの実装

前述した@<code>{UserName}型は@<code>{Stringer}インターフェースを実装しています。
単なる@<code>{int}型や@<code>{string}型のような基本型であっても、
それが表現する概念によっては多態が必要になるかもしれません。
そのような場合、それらの基本型を基にして新しい型を定義するとよいでしょう。

=== 基本型に具体性を持たせる

基本型を基に新しい型を定義することにより、基本型に具体性を持たせることができます。
たとえば、洋服を取り扱うECサイトでは洋服のサイズを扱うことになるでしょう。

ここで洋服を発注する@<code>{Order}関数があったとしましょう。
@<code>{Order}関数は引数で指定したサイズの洋服を発注することができます。

//list[pospome_list10][Order関数]{
func Order(size string) {
    //省略
}
//}

@<code>{Order}関数の引数@<code>{size}が@<code>{string}型だった場合、
具体的にどのような値を引数に指定すればいいのかイメージできるでしょうか。
@<code>{string}という情報だけでは、次のように複数の選択肢が生まれてしまいます。

 * S, M, L
 * s, m, l
 * Small, Medium, Large
 * small, medium, large

@<list>{pospome_list11}のようにサイズを@<code>{string}の定数で定義することを考えてみましょう。
そうすると、具体的な値を定義することはできますが、@<code>{Order}関数の引数は依然として@<code>{string}型です。
そのため、具体的にどの値を指定すればいいのかを明示できません。

//list[pospome_list11][定数でサイズを定義した例]{
const (
    Large  = "Large"
    Medium = "Medium"
    Small  = "Small"
)

func Order(size string) {
    //省略
}
//}

そこで、@<list>{pospome_list12}のようにサイズという概念を型として定義します。

//list[pospome_list12][サイズを型として定義した例]{
type Size string

const (
    Large  Size = "Large"
    Medium Size = "Medium"
    Small  Size = "Small"
)

func Order(s Size) {
    //省略
}
//}

@<code>{Order}関数の引数に指定する値は@<code>{Size}型になりました。
こうすることで、具体的に何を指定すればいいのかが分かりやすくなります。

また、@<list>{pospome_list13}のように@<code>{Size}型の値を生成する関数を作成すると便利でしょう。
たとえば、HTTPなどの外部入力から@<code>{Size}型の値を簡単に生成できます。

//list[pospome_list13][HTTPリクエストからSize型の値を作成する関数]{
func SizeFrom(req *http.Request) Size {
    // HTTP をパースし、Size を生成する
}
//}

ECサイトでは"洋服のサイズ"という概念が、発注や配送などの機能を横断して利用されます。
このように機能を横断して利用される概念は@<code>{string}や@<code>{int}のような基本型を利用しない方がよいでしょう。
代わりに@<code>{type}で新しい型を定義することで、次のようなメリットが享受できます。

 * 可読性が上がる可能性が高くなる
 * 同じような定数を複数箇所で定義することがなくなる

仮に"洋服のサイズ"という概念が特定の機能でのみ利用されたり、
ユーザIDや@<code>{bool}型のフラグのように"どのような値を指定すればいいかが自明なもの"であれば、
新しい型として定義する必要はないかもしれません。

新しい型として定義するかどうかはシステム仕様やチームの設計方針によって変わってくるところなので、
その都度検討してみるとよいでしょう。

=== スライスに対する振る舞い実装

最後はスライスに対する振る舞い実装について紹介します。
例として、ユーザ一覧画面を表示する場合を考えてみます。

ここで@<list>{pospome_list14}のように、表示対象のユーザをデータベースからスライスで取得したとします。

//list[pospome_list14][データベースから表示対象ユーザを取得する例]{
type User struct {
    Name string
    Role string
}

//表示対象のユーザをDBからスライスで取得する
func GetUsersFromDB() []*User {
    //省略
}
//}

単に一覧を表示するだけであれば、このままスライスを扱っても良いでしょう。
しかし、仕様によってはフィルタリング処理が必要になることがあります。

たとえば、次のような仕様があった場合、対象のユーザをどのように取得すればよいでしょうか。

 * ユーザには"管理者権限"を持ったユーザが存在する
 * "管理者権限を持ったユーザ"は一覧の上の方に表示する

たとえば@<list>{pospome_list15}のように@<code>{GetAdminUsersFromDB}という関数を別に用意したとします。

//list[pospome_list15][GetAdminUsersFromDB関数を別に用意した場合]{
func GetUsersFromDB() []*User {
    //省略
}

func GetAdminUsersFromDB() []*User {
    //省略
}
//}

そして、@<list>{pospome_list16}のように、@<code>{GetUsersFromDB}関数と@<code>{GetAdminUsersFromDB}}関数をそれぞれ実行します。
こうすることで、"一覧表示に必要なユーザ"と"管理者権限を持ったユーザ"をそれぞれ取得することができます。

//list[pospome_list16][管理者権限を持ったユーザを別に取得する例]{
users := GetUsersFromDB()
adminUsers := GetAdminUsersFromDB()
//}

このような実装でも問題なく管理者権限を持ったユーザは取得できます。
しかし、@<code>{GetUsersFromDB}関数で取得したユーザの中に管理者権限を持ったユーザは含まれます。
そのため、わざわざ再度データベースにアクセスして、管理者権限を持ったユーザを取得する必要はありません。
最初に取得したユーザから管理者権限を持ったユーザを抽出する方が効率的な実装になります。

スライスをそのまま扱うのであれば、@<list>{pospome_list17}のように、
フィルタリング用の関数を実装することで抽出することになるでしょう。

//list[pospome_list17][フィルタリング用の関数]{
func FilterAdminUsers(u []*User) []*User{
    //省略
}
//}

前述のように関数として実装する方法の他に、@<list>{pospome_list18}のように型を新しく定義し、
そこにメソッドとして実装する方法もあります。

//list[pospome_list18][抽出する処理をメソッドとして定義する方法]{
type Users []*User

func (u Users) FilterAdmin() Users {
    //省略
}
//}

さらに、前述した@<code>{GetUsersFromDB}から
@<list>{pospome_list19}のように、@<code>{[]*User}型ではなく、
@<code>{Users}型で値を返すようにします。

//list[pospome_list19][GetUsersFromDB関数の戻り値をUsers型にした例]{
func GetUsersFromDB() Users {
    //省略
}
//}

こうすることで、 @<list>{pospome_list20}のように
@<code>{GetUsersFromDB}関数で取得したユーザのうち
管理者権限を持つユーザだけを抽出できます。

//list[pospome_list20][FilterAdminメソッドを使った例]{
users := GetUsersFromDB()
adminUsers := users.FilterAdmin()
//}

このようにメソッドとして抽出処理を実装することで、
スライスと抽出処理を一緒に管理することができます。
こうすることで、同じような処理が重複して実装されてしまうことは少なくなるでしょう。

また、型として具体性を持たせることができるため、"何のスライスなのか?"も明示することが可能です。
たとえば、@<list>{pospome_list21}のように、通常のユーザと特殊なユーザのスライスをそれぞれ型として定義することができます。

//list[pospome_list21][型として何のスライスなのか明示した例]{
//通常ユーザのスライス
type UserList []*User

//特殊ユーザのスライス
type SpecialUserList []*User
//}

そして、前述したように、メソッドとして実装することでインタフェースを実装することもできます。
スライスに対する特定の値の加算処理や抽出処理などを抽象化し、
異なる型に持たせることができるのも利点の1つでしょう。

このパターンは気軽に使える@<kw>{First Class Collection}といったところでしょうか。

== おわりに

本章では次の実装パターンを紹介しました。

 * 振る舞いの実装
 * インターフェースを実装
 * 基本型に具体性を持たせる
 * スライスに対する振る舞い実装

普段気にせず利用している@<code>{int}型や@<code>{string}型などの基本型ですが、
@<code>{type}を用いて新しい型として定義することで、コード上で何かしらの意図を伝えることができます。

今まで基本型で定義していた概念を新しい型として定義してみてはいかがでしょうか?
