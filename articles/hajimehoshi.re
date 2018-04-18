= Goのゲーム開発事情

== はじめに

ソフトウェアエンジニアの星一 (ほしはじめ) といいます。
Goは主に趣味でゲームを開発するのに使っています。

本章は、Goにおけるゲーム開発事情について雑多にまとめたものです。
主な内容は、Goで出来たゲームの紹介、Goゲームを作るためのライブラリの紹介です。

== Goでゲーム開発?

Goはよくサーバーサイドアプリケーションには使われるのですが、
クライアントアプリ、ましてやゲーム開発に用いるのは極めて少数派です。
本章を執筆する以前にも、第2回および第3回の技術書典に向けてGoのゲーム開発に関する本を書いたのですが、
「え、Goってサーバーサイド専用の言語じゃないんですか?」という反応が多かった様に思われます。
実際の使われ方の主流としてはそうなのかもしれないですが、
Goがクライアントアプリの開発に向いていないことは決してありません。

Goがゲーム開発に向いていると考えられる理由はいくつかあります。
ひとつめはネイティブコンパイルによって実行が高速であることです。
一般的に、ゲームは実行速度が重要です。
ゲームは毎フレーム (だいたい1/60秒) ごとに状態更新処理を行う必要があります。
またリアルタイムに入力に対して応答する必要があります。
実のところ、ゲーム全体の処理においてコードの実行部分が占める割合というのは必ずしも大きくはなく、
GPUとのやり取り部分がボトルネックだったりします。
それでも大量のオブジェクトを処理する必要がある場合に、コードの速度処理は依然として重要です。
ふたつめは高いポータビリティです。
GoはLinuxやmacOSだけではなくWindowsなどに対してネイティブコンパイルできます。
さらに言うと、GopherJS@<fn>{gopherjs}というGoからJavaScriptへのトランスパイラがあり、
ブラウザで動くゲームすら作ることができます。
AndroidやiOSなども、公式が提供するgomobile@<fn>{gomobile}というツールで対応可能です。

//footnote[gopherjs][@<href>{https://github.com/gopherjs/gopherjs}]
//footnote[gomobile][@<href>{https://golang.org/x/mobile/cmd/gomobile}]

Goのゲーム開発の問題点は、
そもそもやろうとしている人が少なくコミュニティが小さいことや、
ライブラリがあまり充実していないことです。
ライブラリについては
QtやSDLなどの既存ライブラリのバインディングは存在します。
より高レイヤーなゲームライブラリやエンジンは他言語に比べると発展途上と言えます。

Goは他の言語とくらべてシンプルであるとか並行処理が書きやすいといった特徴があります。
筆者ら自らゲームを作った経験から、
Goはゲーム開発でも全く問題ない言語であると言えます。
しかし、Goの仕様のお陰で、他の言語と比べてゲーム開発が著しく
効率的になったりといったことは、特にありません。

なお、FAQとしてGCは問題にならないのかというのがありますが、
筆者はGCが問題になり得るようなシビアなゲームを開発したことがなく、
その質問に対する適切な解答を持ち合わせていません。
よほどシビアなゲームでなければ問題ないのではないでしょうか。

まとめると、Goでゲーム開発すると次のような嬉しいことがあります:

  * ネイティブコンパイルによる高速な実行
  * 高いポータビリティ

また、次のような問題点があります:

  * コミュニティの小ささ
  * ライブラリの少なさ

== Goで出来たゲーム

=== 筆者によるもの

筆者らが開発したゲームとして、
「償いの時計@<fn>{tsugunai}」と「しあわせのあおいとり@<fn>{bluebird}」があります。
Daigo氏@<fn>{daigo}およびZero氏@<fn>{zeroliu}との共同開発です。
おかげさまである程度人気を博しており、
ファミ通に掲載されたり@<fn>{tsugunai_famitsu}@<fn>{bluebird_famitsu}、
有名実況者に実況されたりしました。
ゲームはどちらもAndroid版とiOS版があります。

//footnote[tsugunai][@<href>{http://blockbros.net/tsugunai/}]
//footnote[bluebird][@<href>{http://blockbros.net/bluebird/}]
//footnote[daigo][@<href>{https://twitter.com/daigo}]
//footnote[zeroliu][@<href>{https://github.com/zeroliu}]
//footnote[tsugunai_famitsu][@<href>{https://app.famitsu.com/20170514_1042354/}]
//footnote[bluebird_famitsu][@<href>{https://app.famitsu.com/20180222_1243056/}]

//image[bluebird][「しあわせのあおいとり」のキャプチャ][scale=0.5]

ランタイム部分には拙作のゲームライブラリであるEbiten@<fn>{ebiten}が使われています。
ゲームのロジックはすべてGoで書かれています。
広告を出す部分などはReact Nativeを使用しました。

//footnote[ebiten][@<href>{https://hajimehoshi.github.io/ebiten/}]

いずれもビジネス的理由により、オープンソースに出来ていません。
フォント描画など、ゲームに利用したパッケージについては少しずつ公開していっています。

#@# TODO: スクショを入れる

=== 他者によるもの

2017年にGopher Game Jam@<fn>{gophergamejam}が催され、
そこで幾つかゲームが公開されました。

//footnote[gophergamejam][@<href>{https://itch.io/jam/gopher-jam}]

他、オープンソースなゲームならば、
後述のゲームライブラリのパッケージパスでGitHub上で検索すると、
Goで作られたゲームが見つかります。

Goのゲーム開発はあまり活発ではなく、
名が知られているゲームというのは他にないようです。
もっとコミュニティが活発になって、
有名なGoゲームが出ると良いですね。

== ゲームライブラリ

Goにはいくつかゲームライブラリがあります。
ツクールやUnityのようなIDEは筆者の知る限りでは存在しません。
殆どのものが2Dゲーム用のライブラリですが、3Dゲーム用ライブラリもあることはあります。
また殆どのものが個人による開発です。

=== OpenGLラッパー

#@# Š

Goの2Dゲームライブラリで現在もアクティブに開発が行われているものとしては
筆者のEbiten@<fn>{ebiten}、Michal @<embed>{\\u{S\}}trba氏が開発したPixel@<fn>{pixel}、
Noofbiz氏らが開発したEngo@<fn>{engo}、
Patrick Stephen氏らが開発したOak@<fn>{oak}などがあります。
どれも機能は似ていますが、それぞれ方針やポータビリティなどに違いがあります。
方針の違いでいうと、例えばEbitenは単純さを重点に、
EngoはEntity Component Systemの考えに則ったAPIになっているといった特徴があります。
他、Goチームが開発したShiny@<fn>{shiny}というGUI/ゲームライブラリがあるのですが、
現在開発は停滞している模様です。

//footnote[pixel][@<href>{https://github.com/faiface/pixel}]
//footnote[engo][@<href>{https://engo.io/}]
//footnote[oak][@<href>{https://github.com/oakmound/oak}]
//footnote[shiny][@<href>{https://golang.org/x/exp/shiny}]

3Dゲームライブラリとしては
Stephen Gutekanst氏らが開発したAzul3D@<fn>{azul3d}や
Milan Nikolic氏が開発した、raylibのGoバインドであるraylib-go@<fn>{raylib-go}があります。

//footnote[azul3d][@<href>{http://azul3d.org/}]
//footnote[raylib-go][@<href>{https://github.com/gen2brain/raylib-go}]

いずれにせよ全て発展途上であり、決定打は特に無いと言った状況です。

=== バインディング

ゲームライブラリではないですが、OpenGLを直接叩くのであればバンディング@<fn>{gl}が存在します。
またSDLのGoバインディング@<fn>{go-sdl2}やGLFWのGoバインディング@<fn>{glfw}も存在します。
OpenGLの使い方は他の言語と全く同じです。
いずれもPure GoではなくCgoを使っているので、利用にはCコンパイラが必要です。

//footnote[gl][@<href>{https://github.com/go-gl/gl}]
//footnote[go-sdl2][@<href>{https://github.com/veandco/go-sdl2}]
//footnote[glfw][@<href>{https://github.com/go-gl/glfw}]

== フォント

文字描画はゲームに欠かせない機能の一つです。
Goは標準ライブラリにはフォント関係のライブラリはありません。
その代わり準標準ライブラリである@<code>{golang.org/x}配下にライブラリがあります。

@<code>{golang.org/x/image/font}パッケージに@<code>{Face}インターフェイスが定義されています。
これがフォントフェイスを表現するデファクトスタンダードになっています。

当然インターフェイスだけ定義されていても仕方がなく、
フォントファイルのパーサが必要です。
フォントのファイルの形式にはOpenTypeやTrueTypeなどがあります。
GoではTrueTypeパーサはありますが、OpenTypeパーサで2018年4月現在利用可能なものはありません。

TrueTypeパーサーとしてはfreetype@<fn>{freetype}パッケージがあります。
これはライセンスの関係上@<code>{golang.org/x}にはありません。
これのパース結果が@<code>{Face}インターフェイスを実装するオブジェクトを
出力します。

//footnote[freetype][@<href>{https://github.com/golang/freetype}]

OpenTypeについては@<code>{golang.org/x/image/font/opentype}パッケージがあるのですが、
2018年4月現在まだ実装が完了していないようです。
残念ながら現時点ではTrueTypeパーサーしか存在せず、
OpenTypeフォントを使いたい場合にはTrueTypeにわざわざ変換する必要があります。

筆者はいくつかフォント用パッケージを開発しました。
M+ Bitmapフォント@<fn>{mplusbitmap}を@<code>{Face}インターフェイスのオブジェクトにした
go-mplusbitmap@<fn>{go-mplusbitmap}パッケージなどです。
いずれも我々のゲームのために作成したものですが、
他のゲームでも問題なく使用できます。
共通のインターフェイスを介して
場所を限定することなく活用できるというのがGoのいいところですね。

//emlist[go-mplusbitmapの使用例][go]{
const text = `Hello, World!

こんにちは世界!`

func run() error {
    const (
        ox = 16
        oy = 16
    )

    dst := image.NewRGBA(image.Rect(0, 0, 320, 240))

    // 描画先画像を白で塗りつぶして初期化する。
    draw.Draw(dst, dst.Bounds(), image.NewUniform(color.White),
        image.ZP, draw.Src)

    f := mplusbitmap.Gothic12r

    // golang.org/x/image/fontのDrawerは文字列を指定した画像に描画する。
    d := font.Drawer{
        Dst:  dst,
        Src:  image.NewUniform(color.Black),
        Face: f,
        Dot:  fixed.P(ox, oy),
    }

    for _, l := range strings.Split(text, "\n") {
        d.DrawString(l)
        d.Dot.X = fixed.I(ox)
        d.Dot.Y += f.Metrics().Height
    }

    // 画像dstに文字列が描画された。

    return nil
}
//}

//footnote[mplusbitmap][@<href>{http://mplus-fonts.osdn.jp/mplus-bitmap-fonts/}]
//footnote[go-mplusbitmap][@<href>{https://github.com/hajimehoshi/go-mplusbitmap}]

== サウンド

サウンドについて、標準ライブラリおよび準標準ライブラリには
関連ライブラリは一切ありません。

筆者らはOto@<fn>{oto}という低レイヤーなサウンドライブラリを開発しました。
これは@<code>{io.Writer}を実装するプレイヤーを提供するもので、
バイト列を書き込むとそのまま音がなるというものです。
マルチプラットフォームであり、デスクトップ、モバイル、ブラウザなどの環境で動きます。
Ebitenはこれを使っているのですが、他のPixelやEngoなどのゲームライブラリでも活用されています。
ありがたいことです。

//footnote[oto][@<href>{https://github.com/hajimehoshi/oto}]

またMP3やOgg/Vorbisなどのデコーダもあります。
MP3は筆者がPublic Domainのものを移植したgo-mp3@<fn>{go-mp3}が、
Ogg/Vorbisはoggvorbis@<fn>{oggvorbis}という実装があります。
いずれもPure Goなので環境を問わず利用できます。

//footnote[go-mp3][@<href>{https://github.com/hajimehoshi/go-mp3}]
//footnote[oggvorbis][@<href>{https://github.com/jfreymuth/oggvorbis}]

go-mp3はデコード結果を@<code>{io.Reader}形式で返しますが、一方Otoは@<code>{io.Writer}形式の
プレイヤーを提供します。
ということは、@<code>{io.Copy}を使うと音を鳴らすということができるということです。

//emlist[Otoとgo-mp3の使用例][go]{
func run() error {
    f, err := os.Open("classic.mp3")
    if err != nil {
        return err
    }
    defer f.Close()

    // MP3ストリームをデコードする。結果はio.Readerになる。
    d, err := mp3.NewDecoder(f)
    if err != nil {
        return err
    }
    defer d.Close()

    // Otoのプレイヤーを作成する。結果はio.Writerになる。
    p, err := oto.NewPlayer(d.SampleRate(), 2, 2, 8192)
    if err != nil {
        return err
    }
    defer p.Close()

    // io.Copyを使うと再生できる!
    if _, err := io.Copy(p, d); err != nil {
        return err
    }
    return nil
}
//}

== 物理エンジン

Box2DのPure Go実装@<fn>{box2d}やChipmunkのPure Go実装@<fn>{chipmunk}@<fn>{chipmunk2}があるようです。
なお、筆者はいずれも本格的に使用したことがありません。

//footnote[box2d][@<href>{https://github.com/ByteArena/box2d}]
//footnote[chipmunk][@<href>{https://github.com/jakecoffman/cp}]
//footnote[chipmunk2][@<href>{https://github.com/vova616/chipmunk}]

#@# == モバイル
#@# モバイルは公式が提供しているgomobile@<fn>{gomobile}というツールを使用します。
#@# AndroidまたはiOS向けのバイナリを出力できます。
#@# //footnote[gomobile][@<href>{https://golang.org/x/mobile/cmd/gomobile}]
#@# gomobileではgomobile build

== ブラウザ

GoはGopherJS@<fn>{gopherjs}というトランスパイラを用いて、ブラウザ向けに出力することが出来ます。
GopherJSの概要については以前筆者がQiitaで「GopherJSの基礎@<fn>{gopherjs-basic}」という記事を書きましたので、
ご参考にしていただけたらと思います。
なお筆者はGopherJSのCollaboratorとして、気が向いたときにGopherJSのメンテナンスをしています。

//footnote[gopherjs-basic][@<href>{https://qiita.com/hajimehoshi/items/bf16816e058f312386f0}]

2018年4月現在、WebAssembly向け出力がGo1.11でリリースされる予定です。
GopherJSでは変換結果としてJavaScriptを吐くので、
ファイルサイズが大きなり、また実行速度にも問題がありました。
WebAssemblyならばバイナリ形式なのでファイルサイズも小さく、実行速度も早くなることが期待されます。
これによりGoでのクライアントサイドのWebアプリケーションがより開発しやすくなるでしょう。

== コミュニティ

Goでゲーム開発しようとする人は世界規模で見ても少ないので、
必然的にコミュニティも少ないです。
また日本語だけのコミュニティというのはまずなく、
大体が英語です。

2018年4月現時点では、
Gophers Slack@<fn>{gophers-slack}の#gamedevチャンネルが
最も盛んなコミュニティだと思われます。
Gophers Slackはゲーム開発に限らずGoに関する様々な話題を取り扱っているので、
是非入っておくと良いでしょう。

//footnote[gophers-slack][@<href>{https://invite.slack.golangbridge.org/}]

またRedditにgogamedevというGoゲーム開発専門コミュニティ@<fn>{reddit-gogamedev}もあるのですが、
あまり活発ではないようです。
Go一般のコミュニティ@<fn>{reddit-golang}のほうが
得られるフィードバックは遥かに大きいです。

//footnote[reddit-gogamedev][@<href>{https://www.reddit.com/r/gogamedev/}]
//footnote[reddit-golang][@<href>{https://www.reddit.com/r/golang/}]

== おわりに

Goのゲーム開発事情についてざっくりと説明しました。
Goはゲーム開発に決して向いていないわけではないのですが、
コミュニティが小さくまだまだ発展途上といった具合です。
Goのゲーム開発界隈がより活発になることを願っています。
