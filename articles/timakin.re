= Goはいかにしてビルドを高速化したのか

== はじめに

株式会社Gunosy新規事業開発室エンジニアのtimakin@<fn>{timakin_fn1}です。普段は主にGoのAPIとSwiftのiOSクライアントサイド開発を担当しております。

//footnote[timakin_fn1][@<href>{https://twitter.com/__timakin__}]

本章では、Go1.10で追加されたGoのビルド、およびテストのキャッシュ機構について、Goの内部のコードを見ながら解説していきます。

== 自動キャッシュ機構の概要

この節では、キャッシュ機構の概要を見ていきましょう。Go1.10から、Goは@<code>{go build}コマンド、あるいは@<code>{go test}コマンド時に、自動で実行時キャッシュを作成してくれるようになりました。
コマンドを実行すると、<code>{GOCACHE}という環境変数に指定されたディレクトリに、<code>{sha256}で暗号化されたビルド情報を@<list>{cached_list}のように記録しています。

//list[cached_list][キャッシュ成果物のリストの例]{
$ tree $(go env GOCACHE)

/Users/takahashiseiji/Library/Caches/go-build
├── 00
├── 01
├── 02
├── 03
│   └── 03045cb46e286224612c2292a102928dcb89cf7d2a6c72d4c0df57c7b1454534-d
├── 04
├── 05
│   └── 0529cfd6227927cad78eeed52c1846a6b5e54cf9a9262e09f51b9758934cb988-a

//省略

├── fd
│   └── fd5a15c43f38a51172bc204df73908e4dcb871c39022023e9656845d8638188c-a
├── fe
│   └── fe408866e021d770eb99ed3af628a45d6317b014cc061bba71abbdbf2a3db454-a
├── ff
├── log.txt
└── trim.txt
//}

なお、これは単純にビルド成果物だけをキャッシュしているわけではなく、

* 何をビルドしたのか
* 何をテストしたのか
* どんなオプションをコマンドに付与したのか

という情報をキャッシュしています。実際に@<list>{artifact}のような、ビルド時に作成されたキャッシュを出力することもできます。


//list[artifact][キャッシュ成果物]{
$ more $(go env GOCACHE)/03/03045cb46e286224612c2292a102928dcb89cf7d2a6c72d4c0df57c7b1454534-d

!<arch>
__.PKGDEF       0           0     0     644     6225      `
go object darwin amd64 go1.10.1 X:framepointer
build id "B9jegu8hj7CxM9O-Kgnh/PNi0GAC_RLtwZahEaU3j"
----

build id "B9jegu8hj7CxM9O-Kgnh/PNi0GAC_RLtwZahEaU3j"

$$B
version 5

^@^B^A^Gspew^@^C^?>#^@  UsersESCtakahashiseiji^Cgo^Esrc^Sgithub.com^Mtimakin^Mgonvert^Kvendor^L^Ostretchr^Mtestify^R^L^Mdavecgh^Mgo-spew^B
^Qbypass.goESCUnsafeDisabled^@(!^E^M^?J#^@^D^F^H
^L^N^P^R^L^T^V^R^L^X^Z^B^Qconfig.go^UConfigState^@^U^N

//省略
//}

ビルド時には固有のIDが振られ、キャッシュ取得時にはそのIDをベースに最終ビルド時点でキャッシュされているかを判別します。
また、実行時のオプションを変えたり、依存パッケージが増えたりした場合はキャッシュが更新されるようになっています。
依存パッケージが増えれば増えるほど、その効力は大きくなり、高速化されることとなります。

また、テスト時にも同様にキャッシュが行われます。こちらは有効期限がファイルに保存され、コマンドオプションによってはキャッシュが無効になってしまします。
キャッシュが有効となる@<code>{go test}コマンドのオプションは、以下の通りです。

* -cpu
* -list
* -parallel
* -run
* -short
* -v

これらのオプションの共通点は、実行のたびに結果が変わらない、ということです。
逆に、実行のたびに結果が変わるようなオプションは、@<code>{-bench}、@<code>{-cpuprofile}などの計測する類の実行オプションを指します。

== キャッシュ機構はどのように実装されているのか

さて、前節までで概要を紹介した自動キャッシュ機構ですが、どのように実装されているのでしょうか？
この節では、 コマンドの実行からキャッシュを読み取り、更新または作成するところまでを書いていきます。

=== コマンドを実行する

キャッシュが関係するコマンドに限りませんが、実行可能なGoのコマンドは、@<list>{cmd_init}のように登録されます。
Goのコマンドとして登録されているのは、利用方法やフラグ情報を持った@<code>{Command}という構造体の実装です。
実行時には、この構造体が持つ@<code>{Run}メソッドを呼びます。

//list[cmd_init][コマンドの初期化]{
func init() {
	base.Commands = []*base.Command{
		//省略
		work.CmdBuild,
		//省略
		test.CmdTest,
		//省略
	}
}
//}

=== 実行オプションを取得する

概要でも書いた通り、テスト実行時にはオプションに応じてキャッシュを使うか判別します。
Goのテストに付随する内部パッケージで@<code>{TryCache}というメソッドがあります。
この中で、@<list>{option_list}のような処理で、引数に指定されたコマンドオプションがキャッシュに利用可能なものかどうか判別しています。

//list[option_list][オプションのフィルタリング]{
func (c *runCache) tryCacheWithID(b *work.Builder, a *work.Action, id string) bool {
	//省略

	var cacheArgs []string
	for _, arg := range testArgs {
		//省略
		switch arg[:i] {
		case "-test.cpu",
			"-test.list",
			"-test.parallel",
			"-test.run",
			"-test.short",
			"-test.v":
			// These are cacheable.
			// Note that this list is documented above,
			// so if you add to this list, update the docs too.
			cacheArgs = append(cacheArgs, arg)

		case "-test.timeout":
			// Special case: this is cacheable but ignored during the hash.
			// Do not add to cacheArgs.

		default:
			// nothing else is cacheable
			if cache.DebugTest {
				fmt.Fprintf(os.Stderr, "testcache: caching disabled for test argument: %s\n", arg)
			}
			c.disableCache = true
			return false
		}
	}
	//省略
}
//}

=== キャッシュの存在確認をする

通常ビルドもテスト時のビルドも、@<code>{useCache}というメソッドで キャッシュを利用するかどうかを決定します。
この時、@<code>{GOCACHE}という環境変数で指定されたパスにキャッシュが存在するかをチェックします。
なお、このパスですが、@<list>{default_dir}にある通り、OSによって利用されるパスが違います。

//list[default_dir][OS別キャッシュ保存先]{
func DefaultDir() string {
	// 省略
	switch runtime.GOOS {
	case "windows":
		dir = os.Getenv("LocalAppData")
		//省略

	case "darwin":
		dir = os.Getenv("HOME")
		//省略

	case "plan9":
		dir = os.Getenv("home")
		//省略

	default: // Unix
		dir = os.Getenv("XDG_CACHE_HOME")
		//省略
	}
	return filepath.Join(dir, "go-build")
}
//}

=== キャッシュを読み取る

キャッシュはテストされる関数やビルド時のパッケージ探索の度に@<code>{cache.Default}というメソッドが呼ばれます。
こちらは@<code>{sync.Once}で実行時に一度だけ、@<list>{default_once}で、ファイルの読み出しを行います。

//list[default_once][キャッシュの読み取り処理]{
// Default returns the default cache to use, or nil if no cache should be used.
func Default() *Cache {
	defaultOnce.Do(initDefaultCache)
	return defaultCache
}

var (
	defaultOnce  sync.Once
	defaultCache *Cache
)

// initDefaultCache does the work of finding the default cache
// the first time Default is called.
func initDefaultCache() {
	dir := DefaultDir()
	// 省略
	// 取得したキャッシュをdefaultCacheに一度だけ代入する
	defaultCache = c
}
//}

ここで呼び出されたキャッシュのファイルの中身を、テスト実行時の出力結果として用いたり、ビルド時の成果物としてそのまま利用します。
例えば以下の@<list>{read_build_cache}では、キャッシュが存在していれば過去のビルド成果物のヘッダー情報を読み取り、ファイル内容をビルド結果として利用します。

//list[read_build_cache][ビルドキャッシュの読み込みの例]{
if c := cache.Default(); c != nil {
  if !cfg.BuildA {
    entry, err := c.Get(actionHash)
    if err == nil {
      file := c.OutputFile(entry.OutputID)
      info, err1 := os.Stat(file)
      buildID, err2 := buildid.ReadFile(file)
      if err1 == nil && err2 == nil && info.Size() == entry.Size {
        stdout, stdoutEntry, err := c.GetBytes(cache.Subkey(a.actionID, "stdout"))
        if err == nil {
          if len(stdout) > 0 {
            if cfg.BuildX || cfg.BuildN {
              b.Showcmd("", "%s  # internal", joinUnambiguously(str.StringList("cat", c.OutputFile(stdoutEntry.OutputID))))
            }
            if !cfg.BuildN {
              b.Print(string(stdout))
            }
          }
          a.built = file
          a.Target = "DO NOT USE - using cache"
          a.buildID = buildID
          return true
        }
      }
    }
  }
  // 省略
//}

=== キャッシュを作成する

ビルド成功時には、@<list>{build_cache}のように、ビルド成果の標準出力をそのままファイルに保存します。

//list[build_cache][ビルドキャッシュの作成]{
//省略
if c := cache.Default(); c != nil && a.Mode == "build" {
  r, err := os.Open(target)
  if err == nil {
    if a.output == nil {
      panic("internal error: a.output not set")
    }
    outputID, _, err := c.Put(a.actionID, r)
    if err == nil && cfg.BuildX {
      b.Showcmd("", "%s # internal", joinUnambiguously(str.StringList("cp", target, c.OutputFile(outputID))))
    }
    c.PutBytes(cache.Subkey(a.actionID, "stdout"), a.output)
    r.Close()
  }
}
//省略
//}

一方、テストの成功時はテストのログを@<list>{test_cache}のように記録しています。
キャッシュ機構自体はビルド時もテスト時も同じものを利用しているのですが、ビルド時は保存時に前回のキャッシュとの比較を行うかどうかという違いがあります。
また、テストキャッシュには有効期限が@<code>{testexpire.txt}というテキストファイルに設定されています。

//list[test_cache][テストキャッシュの作成]{
func (c *runCache) saveOutput(a *work.Action) {
	//省略
	if c.id1 != (cache.ActionID{}) {
		if cache.DebugTest {
			fmt.Fprintf(os.Stderr, "testcache: %s: save test ID %x => input ID %x => %x\n", a.Package.ImportPath, c.id1, testInputsID, testAndInputKey(c.id1, testInputsID))
		}
		cache.Default().PutNoVerify(c.id1, bytes.NewReader(testlog))
		cache.Default().PutNoVerify(testAndInputKey(c.id1, testInputsID), bytes.NewReader(a.TestOutput.Bytes()))
	}
	if c.id2 != (cache.ActionID{}) {
		if cache.DebugTest {
			fmt.Fprintf(os.Stderr, "testcache: %s: save test ID %x => input ID %x => %x\n", a.Package.ImportPath, c.id2, testInputsID, testAndInputKey(c.id2, testInputsID))
		}
		cache.Default().PutNoVerify(c.id2, bytes.NewReader(testlog))
		cache.Default().PutNoVerify(testAndInputKey(c.id2, testInputsID), bytes.NewReader(a.TestOutput.Bytes()))
	}
}
//}

== おわりに

本章では、Goの自動キャッシュ機構の実装について書きました。
ビルド、テスト共に、同じ機構を用いて、暗号化されたキャッシュが生成されます。
実行のたびに結果が変化しないオプションであれば、このキャッシュはデフォルトで有効なので、CIや手元で繰り返しビルドするプロジェクトでは、今すぐ1.10にアップグレードすることをお勧めします。
