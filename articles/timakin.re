= Go APIのモバイル認証処理

== はじめに

株式会社Gunosy新規事業開発室エンジニアのtimakin@<fn>{timakin_fn1}です。Gunosyでは主にGoのAPIとSwiftのiOSクライアントサイド開発を担当しております。

//footnote[timakin_fn1][@<href>{https://twitter.com/__timakin__}]

起動直後に特にユーザの認証を必要としないモバイルアプリは多く存在しています。
しかし、裏側ではデバイス認証をしておき、正確にユーザ情報を管理しなくてはならない場合があります。

本章では、モバイルファーストなプロダクトにおいて、このような認証処理をGoで実装する方法について、Gunosyの実例を踏まえて解説します。
なお、本章で対象とするのはOAuth認証などではなく、より単純なデバイスの認証とします。
また、環境はGoogle App Engineで用意し、Goのバージョンは1.8とします。

== 認証処理で必要なこと

認証処理というのは往々にして実装が複雑になりがちです。初回リクエスト時に認証処理として具体的にやるべきことは、たとえば次のような項目です。

 * iOS, Android両方を考慮した値の受け付け
 * 既存ユーザーかどうかをチェック
 * BANされていないかをチェック
 * バージョンを示す文字列やOS種別をレコード時に形式変換
 * 有効なABテスト情報を格納
 * コンテンツ初期表示に必要な情報を格納
 * 取得したユーザー情報をJWTとしてシリアライズ

これらを全て一つのリクエストのうちに済ませようとすると、REST APIを提供していても、このリクエストのみJSON-RPCといっても差し支えないほどに込み入った処理をすることになります。
さらには、そういった処理の中ではトランザクションを適切に張る必要もあり、他のリソース返却のAPIよりも難易度が上がります。
プロダクトが成長するにつれ、チュートリアル処理やキャンペーン情報など、様々な情報をこの認証処理のレスポンスペイロードに入れ始め、混沌としたAPIが出来上がります。

ここではプロダクトのフェーズがその段階に達する前の、シンプルなデバイス認証をどうやってGoで実装すべきかについて書いていきます。

== 起動スクリプトの書き方

起動スクリプトは@<list>{bootscript}のような書き方になります。

//list[bootscript][起動スクリプトの例]{
package main

import (
	"net/http"

	"github.com/fukata/golang-stats-api-handler"
	"github.com/gorilla/mux"
	"github.com/justinas/alice"
	"github.com/timakin/sample_api/services/api/src/domain/auth"
	"github.com/timakin/sample_api/services/api/src/middleware"
	"github.com/timakin/sample_api/services/api/src/repository"
	_ "google.golang.org/appengine/remote_api"
)

func init() {
	h := initHandlers()
	http.Handle("/", h)
}

func initHandlers() http.Handler {
	// ルーティング
	r := mux.NewRouter()

	// リクエスト処理で使うミドルウェア
	chain := alice.New(
		middleware.AccessControl,
		middleware.Authenticator,
	)

	// CPUやメモリ使用量などのstatsを返却
	r.HandleFunc("/api/stats", stats_api.Handler)

	// 認証処理関連の構造体を初期化
	authRepository := repository.NewAuthRepository()
	authService := auth.NewService(authRepository)
	authDependency := &auth.Dependency{
		AuthService: authService,
	}

	r = auth.MakeInitHandler(authDependency, r)

	// ハンドラにミドルウェアをbindする
	h := chain.Then(r)

	return h
}
//}

Google App Engine特有のパッケージが一部読み込まれていますが、基本的な書き方は変わりません。
特筆すべき点としては、

 * @<code>{MakeInitHandler}という関数でルーティング定義
 * @<code>{Repository}, @<code>{Service}などを依存オブジェクトとしてカスタムハンドラに注入している
 * 認証処理を@<code>{auth}というパッケージにまとめている

という点でしょうか。ひとつめはルーティングの定義を外部の関数に切り出しておくというだけなのですが、見栄えが良いので個人的にオススメです。
ふたつめは、Goのリクエストハンドラの実装方法です。この点は次の節で詳しく解説いたします。

== カスタムハンドラとルーティング定義

この節ではハンドラについて述べていきます。カスタムハンドラの定義は@<list>{custom_handler}のように行います。 

//list[custom_handler][カスタムハンドラの定義]{
package auth

import (
	"context"
	"net/http"

	"google.golang.org/appengine"
	"github.com/gorilla/mux"
	"github.com/timakin/sample_api/services/api/src/handler"
)

type Dependency struct {
	AuthService Service
}

type CustomHandler struct {
	Impl func(http.ResponseWriter, *http.Request)
}

func (h CustomHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	ctx := appengine.WithContext(r.Context(), r)
	ctx, cancel := context.WithTimeout(ctx, handler.TimeOutLimit)
	defer cancel()
	ctx = handler.SetReqParams(ctx, vars)
	cr := r.WithContext(ctx)
	h.Impl(w, cr)
}
//}

=== なぜカスタムハンドラが必要か

そもそもなぜカスタムハンドラの定義をする必要があるのでしょうか？これはGo特有の話になってきます。

Goの起動スクリプトで初期化されたミドルウェア(ロガーやデータベースのクライアントなど)は、できることなら各リクエストで使いまわしたいところです。
特に気にせず書いてしまうと、パッケージのグローバル変数として定義したオブジェクトを使ったり、リクエストごとにロガーを初期化、などとなります。

当然ながらグローバル変数の多用やリクエストの度の初期化は、望ましくありません。そこで、Goのインタフェースを使って、ミドルウェアのオブジェクト群を@<code>{Dependency}(依存オブジェクト)として持ったハンドラを作成すると便利です。
@<code>{CustomHandler}がその実装例になります。この構造体は@<code>{ServeHTTP}メソッドを実装しています。これを実装すればGoはHTTPリクエストハンドラとしてみなしてくれます。

ここでは、認証情報へのデータアクセスクライアントなどを持った@<code>{Repository}オブジェクト、をさらに内部に持ったビジネスロジックを実装した@<code>{Service}構造体、を持った@<code>{Dependency}という、ちょっと入れ子構造として多層なオブジェクトを持ったハンドラを認証処理に使います。

=== Goのコンテキストについて

さらに、Goのコンテキストという概念がよく実装で問題になります。これはGoのゴールーチン間でキャンセル処理を統一管理する用途、そしてリクエストに限定的な値(ユーザー情報など)をストアするための用途で使われる機構です。
主にハンドラでどう関係するかといえば、タイムアウト処理で関係してきます。

リクエストがタイムアウトした時に、張ってるトランザクションをキャンセルしつつエラーレスポンスを返さなければなりません。
まさにこのような用途でコンテキストを使います。例えばデータベースのクライアントに実行メソッドとして@<code>{Exec}というのが用意されていれば、標準パッケージならほぼ確実に@<code>{ExecContext}というような、第一引数にcontextを渡せるメソッドが生えています。
これがあれば、タイムアウトと同時にトランザクション処理をキャンセルしたりできます。

なのでタイムアウト設定としてコンテキストをどこかで設定すべきなのですが、ここでは@<code>{ServeHTTP}の中で設定することとします。

=== 認証処理のエンドポイント用ルーティング定義

ここではPOSTリクエストとの@<code>{/api/init}というエンドポイントを、初期の認証処理の入り口として置いています。

//list[routing.go][認証処理用のルーティング定義]{
package auth

import "github.com/gorilla/mux"

// MakeInitHandler ... 認証処理に関するルーティング定義
func MakeInitHandler(d *Dependency, r *mux.Router) *mux.Router {
	initHandler := CustomHandler{Impl: d.InitHandler}
	r.Handle("/api/init", initHandler).Methods("POST")
	return r
}
//}

冒頭でルーティング定義を別メソッドに切り出して置くと見栄えが良いと述べましたが、ここがその実装です。
カスタムハンドラでラップしつつ、実際のリクエストハンドラに引き渡します。その実際のリクエストハンドラが@<list>{request_handler}です。

//list[request_handler][実際にリクエストを処理するハンドラ]{
package auth

import (
	"net/http"

	"github.com/pkg/errors"
	"github.com/timakin/sample_api/services/api/src/handler"
)

// InitHandler ...  アプリ起動時の認証処理を行うハンドラ
// Path: /init
func (d *Dependency) InitHandler(w http.ResponseWriter, r *http.Request) {
	payload, err := decodeInitRequest(r)
	if err != nil {
		msg := "Failed to parse auth params from path components."
		err = errors.Wrap(err, msg)
		res := handler.NewErrorResponse(http.StatusBadRequest, err.Error())
		handler.Redererer.JSON(w, res.Status, res)
		return
	}
	record, isNew, token, err := d.AuthService.Init(r.Context(), &User{
		Device: &Device{
			DUID:       payload.DUID,
			DeviceName: payload.Device,
			OSName:     payload.OS,
			OSVersion:  payload.OSVersion,
			DeviceApp: &DeviceApp{
				AppVersion: payload.AppVersion,
				BundleID:   payload.BundleID,
			},
		},
	})
	if err != nil {
		status := http.StatusInternalServerError
		res := handler.NewErrorResponse(status, err.Error())
		handler.Redererer.JSON(w, res.Status, res)
		return
	}

	resPayload := encodeInitResponse(record.ID, isNew, token)

	handler.Redererer.JSON(w, http.StatusOK, resPayload)
}

type InitRequestPayload struct {
	DUID       string `json:"duid"		validate:"required"`
	AppVersion string `json:"app_version"	validate:"required"`
	BundleID   string `json:"bundle_id"	validate:"required"`
	Device     string `json:"device"	validate:"required"`
	OS         string `json:"os"		validate:"required"`
	OSVersion  string `json:"os_version"	validate:"required"`
}

func decodeInitRequest(r *http.Request) (*InitRequestPayload, error) {
	payload := InitRequestPayload{
		DUID:       r.FormValue("duid"),
		AppVersion: r.FormValue("app_version"),
		BundleID:   r.FormValue("bundle_id"),
		Device:     r.FormValue("device"),
		OS:         r.FormValue("os"),
		OSVersion:  r.FormValue("os_version"),
	}
	err := handler.Validator.Struct(payload)
	if err != nil {
		return nil, err
	}

	return &payload, nil
}

type InitResponsePayload struct {
	UserID      int64   `json:"user_id"`
	IsNewUser   bool    `json:"is_new_user"`
	AccessToken string  `json:"access_token"`
}

func encodeInitResponse(uID int64, nu bool, at string) *InitResponsePayload {
	payload := InitResponsePayload{
		UserID:      uID,
		IsNewUser:   nu,
		AccessToken: at,
	}
	return &payload
}
//}

受け取った@<code>{Init}用のリクエストボディをパースして、ビジネスロジックへの流し込みます。そして結果をレスポンスペイロードに詰めて返却する、というのがおおまかな流れです。
リクエストボディとしてモバイルクライアント側が送信する値は、@<table>{userinfo}の通りです。

//table[userinfo][主に認証に使う値の表]{
-------------------------
DUID		デバイスのユニークID
AppVersion	クライアントにインストールされているアプリバージョン
BundleID	上記と一緒にクライアントに定義されるであろうBundleID
Device 		デバイス名
OS 			OS種別
OSVersion 	OSのバージョン 
//}

などです。広告の配信機構などを実装する場合はもう少し追加の値を追加する必要がありますが、プレーンな実装だとこの程度でできます。
また、返却されるペイロードは、@<table>{respayload}の通りです。

//table[respayload][認証結果のペイロードの表]{
-------------------------
UserID			ユーザーID
IsNewUser		新規ユーザーフラグ
AccessToken		認証後のAPIリクエストでバリデーションに使うJWT
//}

などが要素として入ってきます。IDと新規ユーザーフラグは、ログやチュートリアル実装でよく使うので、そのまま返却してしまいます。ユーザー名などが必要な場合は、別途JWTから引き出すことも可能です。

=== 認証していないユーザーからのリクエストを禁止する

JWTをAccessTokenという値で返却する、と上述しましたが、その値は@<list>{middleware.go}のような場面で利用します。

//list[middleware.go][認証用のミドルウェア]{
package middleware

import (
	"fmt"
	"net/http"
	"regexp"

	"github.com/pkg/errors"
	"github.com/timakin/sample_api/services/api/src/config"
	"github.com/timakin/sample_api/services/api/src/handler"
)

type Endpoint struct {
	Path   string
	Method string
}

var whiteList = []Endpoint{
	Endpoint{Path: `/ping`, Method: "GET"},
	Endpoint{Path: `/ping`, Method: "POST"},
	Endpoint{Path: `/api/stats`, Method: "GET"},
	Endpoint{Path: `/api/categories`, Method: "POST"},
	Endpoint{Path: `/api/topics`, Method: "POST"},
	Endpoint{Path: `/api/videos`, Method: "POST"},
}

func Authenticator(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// POST init endpointのみ認証は不要
		if ok, _ := regexp.MatchString(`^/[^/]+/init`, r.URL.Path); ok {
			if r.Method == "POST" {
				next.ServeHTTP(w, r)
				return
			}
		}

        // 認証処理が不要なホワイトリストを除外
		for i := range whiteList {
			ok, _ := regexp.MatchString(whiteList[i].Path, r.URL.Path)
			if ok && r.Method == whiteList[i].Method {
				next.ServeHTTP(w, r)
				return
			}
		}

		at := handler.NewAuthenticator(config.AccessTokenSecret)
		token, err := at.ValidateFromRequest(r)
		if err != nil {
			err = errors.Wrap(err, fmt.Sprintf("Invalid JWT"))
			status := http.StatusUnauthorized
			res := handler.NewErrorResponse(status, err.Error())
			handler.Redererer.JSON(w, res.Status, res)
			return
		}

		cr := r.WithContext(handler.NewContextWithJWT(r.Context(), token))
		next.ServeHTTP(w, cr)
	})
}
//}

このコードはミドルウェアの関数として定義されたもので、リクエストのたびに呼ばれますが、その役割は「@<code>{init}以外のリクエストではJWTの値を検証して、妥当なユーザーかどうか検証する」ということです。
認証処理というのは、デバイスの登録・更新に加えて、認証後に発行されたトークンを用いてAPIを許可されたユーザーだけが呼べるようにする、という一連の流れを指します。
Goではミドルウェアの関数を使ってこのように認証済みユーザー以外を弾くのが望ましいです。

== 必要なデータ定義

認証に必要になる構造体の定義としては、@<list>{user.go}のようなものがあります。

//list[user.go][構造体定義]{
// User ... エンドユーザー
type User struct {
	ID        int64   `json:"id" 		  datastore:"-" goon:"id"`
	CreatedAt int64   `json:"created_at"`
	UpdatedAt int64   `json:"updated_at"`
	Enabled   bool    `json:"enabled"`
	IsNew     bool    `json:"is_new" 	  datastore:"-"`
	Device    *Device `json:"device" 	  datastore:"-"`
}

// Device ... ユーザーの利用端末
type Device struct {
	ID           int64      `json:"id" datastore:"-" goon:"id"`
	UserID       int64      `json:"user_id"`
	DUID         string     `json:"duid"           validate:"duid"`
	OSName       string     `json:"os_name"        validate:"os_name"`
	OSTypeID     int        `json:"os_type_id"     validate:"os_type_id"`
	OSVersion    string     `json:"os_version"     validate:"os_version"`
	OSVersionNum int        `json:"os_version_num" validate:"os_version_num"`
	DeviceName   string     `json:"device_name"    validate:"device_name"`
	CreatedAt    int64      `json:"created_at"     validate:"created_at"`
	UpdatedAt    int64      `json:"updated_at"     validate:"updated_at"`
	DeviceApp    *DeviceApp `json:"device_app"     validate:"device_app"`
}

// DeviceApp ... アプリ
type DeviceApp struct {
	ID            int64  `json:"id"                datastore:"-" goon:"id"`
	DeviceID      int64  `json:"device_id"         validate:"device_id"`
	BundleID      string `json:"bundle_id"         validate:"bundle_id"`
	AppVersion    string `json:"app_version"       validate:"app_version"`
	AppVersionNum int    `json:"app_version_num"   validate:"app_version_num"`
	CreatedAt     int64  `json:"created_at"        validate:"created_at"`
	UpdatedAt     int64  `json:"updated_at"        validate:"updated_at"`
}
//}

基本的なユーザー情報を表す@<code>{User}、利用端末の情報を表した@<code>{Device}、さらにそれにインストールされた@<code>{DeviceApp}という情報で構成します。
@<code>{DeviceApp}というのは、多くの場合Deviceに含めてしまっていい情報もありますが、たまにこのように一つの端末に複数の同一アプリがインストールされることがあります。
現にGunosyではプリインストールユーザーなどでユーザー情報の上書きを防ぐために@<code>{DeviceApp}というところにアプリ情報を正規化して切り出すことで、そのバグを防いでいました。

== 認証のビジネスロジックの実装

実際のビジネスロジックは@<code>{Service}という構造体が持っているというのを前述しましたが、@<list>{service.go}がその実装です。

//list[service.go][認証ロジックの実装]{
type Service interface {
	Init(ctx context.Context, u *User) (*User, bool, string, error)
}

type service struct {
	repo Repository
}

// NewService creates a handling event service with necessary dependencies.
func NewService(repo Repository) Service {
	return &service{
		repo: repo,
	}
}

func (s service) Init(ctx context.Context, u *User) (*User, bool, string, error) {
	user, err := s.repo.GetUser(ctx, &User{
		Device: &Device{
			DUID: u.Device.DUID,
		},
	})
	if err != nil {
		if err == ErrResourceNotFound {
			err = nil
			err := datastore.RunInTransaction(ctx, 
					func(tc context.Context) error {
				now := time.Now().Unix()
				u.Enabled = true
				u.CreatedAt = now
				u.UpdatedAt = now
				u.IsNew = true
				user, err = s.repo.UpsertUser(tc, u)
				if err != nil {
					return err
				}
				osVerNum, err := VerStrToInt(u.Device.OSVersion)
				if err != nil {
					return err
				}
				u.Device.UserID = u.ID
				u.Device.CreatedAt = now
				u.Device.UpdatedAt = now
				u.Device.OSTypeID = OSNameToTypeID[u.Device.OSName]
				u.Device.OSVersionNum = osVerNum
				d, err := s.repo.UpsertDevice(tc, u.Device)
				if err != nil {
					return err
				}
				user.Device = d

				appVer := u.Device.DeviceApp.AppVersion
				appVerNum, err := VerStrToInt(appVer)
				if err != nil {
					return err
				}
				u.Device.DeviceApp.DeviceID = d.ID
				u.Device.DeviceApp.CreatedAt = now
				u.Device.DeviceApp.UpdatedAt = now
				u.Device.DeviceApp.AppVersionNum = appVerNum
				dapp := u.Device.DeviceApp
				da, err := s.repo.UpsertDeviceApp(tc, dapp)
				if err != nil {
					return err
				}
				user.Device.DeviceApp = da

				return nil
			}, &datastore.TransactionOptions{
				XG: true,
			})
			if err != nil {
				return nil, false, "", err
			}
		} else {
			return nil, false, "", err
		}
	} else {
		user.IsNew = false
	}

	appVerNum, err := VerStrToInt(u.Device.DeviceApp.AppVersion)
	if err != nil {
		return nil, false, "", err
	}

	if user.Device.DeviceApp.AppVersionNum < appVerNum {
		user.Device.DeviceApp.AppVersion = u.Device.DeviceApp.AppVersion
		user.Device.DeviceApp.AppVersionNum = appVerNum
		user.Device.DeviceApp.UpdatedAt = time.Now().Unix()
		_, err := s.repo.UpsertDeviceApp(ctx, u.Device.DeviceApp)
		if err != nil {
			return nil, false, "", err
		}
	}

	// JWT生成
	token := user.NewAccessToken()
	at := handler.NewAuthenticator(config.AccessTokenSecret)
	sJWT, err := at.SignToken(token)
	if err != nil {
		return nil, false, "", err
	}

	return user, u.IsNew, sJWT, nil
}
//}

@<code>{Init}という関数の中で@<code>{User}、@<code>{Device}、@<code>{DeviceApp}を登録していきます。ここではGoogle Cloud PlatformのDatastoreをデータベースとして利用しているので、@<code>{RunInTransaction}という関数でトランザクションを張っています。
不恰好かもしれませんが、認証処理のように複数のテーブルを一挙に参照するような場面では、必ずトランザクションを張るべきです。

また、ここでは省略していますが、バージョン番号が古すぎる場合は強制アップデートフラグをつけるなどの処理もここで必要になってくるでしょう。
ユーザー情報を抜き取ったあとは、@<code>{User}構造体に用意したトークン化のメソッドなどでデータをシリアライズします。

== リポジトリ経由でユーザーを登録する

いわゆるデータアクセスするレイヤーの書き方はシンプルで、@<list>{repository.go}のようになります。当該処理はサービスの処理の中から呼ばれます。 

//list[repository.go][データアクセスを担うリポジトリ層の実装]{
func (repo authRepository) UpsertUser(ctx context.Context, u *auth.User) 
		(*auth.User, error) {
	g := goon.FromContext(ctx)
	if _, err := g.Put(u); err != nil {
		return nil, err
	}
	return u, nil
}
//}

Datastoreの場合、MySQLなどとは違い値に@<code>{Key}という識別子が必要になってきますが、その発行処理をいちいち書いていたら面倒です。
ここではGoogle App Engineユーザー向けに最適化された@<code>{goon}というパッケージを使いますが、他にも自前でラップするなどして、使いやすいクライアントを模索すべきです。

また、Google App Engine向けと書きましたが、App Engine以外からのDatastore利用の場合都合のいいパッケージがなかったので、筆者は@<code>{timakin/gosto}というgoonと同様の処理をApp Engine以外からのCloud API呼び出しでできるようにしたものを作成しました。

== おわりに

上記のようにつらつらとコードとその補足を書いてきましたが、認証処理は通常のロジックとは違い、入り組んだデータ構造とトランザクション処理が必要になります。
書いたようなGoのコンテキストのような機構を使えばだいぶ楽にキャンセル処理をハンドリングできます。
JWTなどを容易に扱えるパッケージなども揃っているので、モバイル向けAPIとしてGoを採用し、認証処理を書く場面があれば、ぜひ一部だけでもご参考いただければと思います。


