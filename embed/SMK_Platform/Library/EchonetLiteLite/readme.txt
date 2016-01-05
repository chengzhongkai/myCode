===============================================================================
ECHONET Lite Lite (c) Copyright 2014, SMK Corporation. All rights reserved.
===============================================================================

ECHONET Lite ミドルウェア通信の軽量版

【機能・制約】

他ノードのオブジェクトは保持しない

malloc 使用・不使用を選択できる

malloc 不使用の場合、メモリプールのサイズをあらかじめ指定する
・総オブジェクト数
・総プロパティ数
・総 EDT 用メモリ

プロパティのサイズは初期化時に指定して(固定)、後で動的に変更できない
-> Setup/Getup やコールバックを使うことで対応可能

オブジェクトも後から削除や追加はできない
<- リセットして再構築することで対応可能
<- オブジェクトの free が必要


【機器オブジェクトの定義】

[EPC], [PDC], [EDT_1], ..., [EDT_n], [Access Rule],

※ [EDT] は [PDC] で指定した数だけ必ず含めること

最後は [EPC] に 0x00 を指定することで終端と判断する


【プロパティの値域定義】

プロパティ Set 時の EDT の Validity チェック

プロパティの種類によって、以下の通りのものを定義する

値が決まっているもの (動作状態など)
範囲が決まっているもの (プロパティ値域)
バイナリデータ (メーカコードなど)
ASCII コード (製造番号など) -> バイナリデータで指定
日付 (製造年月日) -> 値域で指定
ビットマップ (有効ビットの箇所を 1 に)
時刻 (タイマ設定) -> 値域で指定

特殊？
プロパティマップ
自ノードインスタンス数 (3バイト)
インスタンスリスト (数 + 3バイト×N)
クラスリスト (数 + 2バイト×N)

アンダーフローコード
オーバーフローコード

・値が決まっているもの
ELL_PROP_LIST, [NUM], [VAL 1], ..., [VAL n]

(EX) 0x30 と 0x31 の 2 種類の値を取り得るプロパティ
ELL_PROP_LIST, 0x02, 0x30, 0x31

・範囲が決まっているもの
ELL_PROP_MIN_MAX, [NUM], [RANGE 1], ..., [RANGE n]

[RANGE] := [TYPE] [MIN] [MAX]

(EX) 1 バイトの符号あり整数のみで、-127 〜 126 の値を取り得るプロパティ
ELL_PROP_MIN_MAX, 0x01, 0x81, 0x81, 0x7E,

ELL_PROP_MIN_MAX, 0x01, 0x82, 0x80, 0x01, 0x7F, 0xFE
ELL_PROP_MIN_MAX, 0x01, 0x84, 0x80, 0x00, 0x00, 0x01, 0x7F, 0xFF, 0xFF, 0xFE,
ELL_PROP_MIN_MAX, 0x01, 0x01, 0x00, 0xFD,
ELL_PROP_MIN_MAX, 0x01, 0x02, 0x00, 0x00, 0xFF, 0xFD,
ELL_PROP_MIN_MAX, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFD,
                 ^^^^  ^^^^ バイト数 (0x0X: 符号なし、0x8X: 符号あり)
                  変数の数

(EX) 日付 (YYYYMMDD で YYYY=1〜9999、MM=1〜12、DD=1〜31)
ELL_PROP_MIN_MAX, 0x03, { 0x02, { 0x00, 0x01, 0x27, 0x0f } },
                       { 0x01, { 0x01, 0x0c } },
                       { 0x01, { 0x01, 0x1f } },

・バイナリデータ/ASCII コード
ELL_PROP_BINARY, [NUM BYTES]

(EX) 12 バイトの ASCII コード
ELL_PROP_BINARY, 0x0c,

・ビットマップ
ELL_PROP_BITMAP [NUM BYTES], [MASK 1], ..., [MASK n]

(EX) 1 バイトのデータのうち bit0 と bit1 が有効な場合
ELL_PROP_BITMAP, 0x01, 0x03

・不定長データ
ELL_PROP_VARIABLE


[Set Rule]
0x80, ELL_PROP_LIST, 0x02, 0x30, 0x31,
// 0xbf, ELL_PROP_BINARY, 0x02,
指定のないものは、基本的にバイナリデータとする


【初期化】

ELL_InitObjects()
ELL_RegisterObject() で各オブジェクトを追加
必要であれば、ELL_AddRangeRule() でプロパティに対する値域の設定
必要であれば、Set/Get のコールバックを登録
固定 PDC でない場合、必要であれば Property の内容を初期化

bool_t ELL_ConstrutObjects(void) を参考に

通信(例えば UDP)を初期化
電文の Send/Multicast のためのコールバックを定義して登録

リソースの Lock/Unlock の登録

ELL_StartUp() で自動的に INF を発行


【受信電文のハンドル】


・要求電文 (ESV = SetI, SetC, Get, INF_REQ, SetGet)
  -> ELL_HandleRequestPacket()

・応答電文 (ESV = SetC_Res, Get_Res, INFC_Res, SetGet_Res, SetI_SNA,
                  SetC_SNA, Get_SNA, INF_SNA, SetGet_SNA)
  -> ELL_HandleResponsePacket()

・通知電文 (ESV = INF, INFC)
  -> ELL_HandleAnnouncePacket()


【サンプル】

受信ループ
main() を参考に、Iterator を作成して各種ハンドラへ処理を渡す


【パケットの作成】

ELL_PrepareEDATA でワークを作成
ELL_AddEPC でワークへ EPC を追加
(SetGet の場合、Set の EPC 追加が終れば ELL_SetNextOPC で Get へ移行)
ELL_SetHeaderToEDATA でワークへヘッダを設定して完了
(パケットサイズが返る)


【コールバック】

SetProperty のコールバックでは、自前で値域のチェックをしなければならな
い
ルールが登録されていれば、ELL_CheckProperty() を呼び出してもよい
変更があれば、自動的にアナウンスのフラグが立てられる

【状変プロパティのチェック】

ELL_CheckAnnounce() でフラグをチェックすると同時に、INF が送信される。
(この関数を呼び出すことで、以下の処理が実行される。)

----
ELL_SetProperty() でプロパティに値をセットする際、以前の値と比較して異
なっていれば、そのプロパティに対して NEED_ANNOUNCE のフラグが立つ。

ELL_GetAnnounceEPC() で NEED_ANNOUNCE のフラグが立っている EPC のリス
トを取得する。
ELL_MakeINFPacket で INF パケットを作成して、マルチキャストで送信する。

なお、NEED_ANNOUNCE のフラグは、ELL_GetAnnounceEPC() で自動的に倒され
る。

【ミドルウェアアダプタの実装】

状態遷移がある

・イベント
  フレーム受信
  受信タイムアウト


【ToDo】

[X] オブジェクト内の EPC のソート

[X] EDATA というか、OPC 以降を構造体にして、最後にパケット生成させるか
-> 送信バッファと共通にしたいので、これは不採用

[X] SetGet 対応

[X] Set -> Set_Res の対応

[X] SetI -> SetI_SNA の対応

[X] コールバックの登録

[X] インスタンスコード 0x00 への対応

[X] ハンドラの共通化

DEOJ_1 Set EPC_1
DEOJ_2 Get EPC_2
:          :
DEOJ_n     EPC_n

[X] 応答電文のコールバック
-> ELL_RecvPropertyCB で header、addr、property を渡すようにした

[X] INF 電文のコールバック
-> ELL_RecvPropertyCB で header、addr、property を渡すようにした

[X] プロパティ構造体の見直し

[X] gSendBuffer が HandleRecvPacket と CheckAnnounce で競合してしまっている
-> Lock/Unlock の関数をセットできるよう変更

[X] ルールで定義できないパラメータに対するコールバック登録
-> ルールを定義せず、Setup で実装可能

[X] MAX 属性の追加
<- MAX 以下であれば OK なプロパティ
<- 数が大きいときの定義が難しい
<- 8bit なので、高々 255 バイト
<- EPCDefine で紐づけられる EDT の先頭 1 バイトを領域の長さに

[X] 可変長属性の導入 (ELL_PROP_VARIABLE)
<- MAX より小さければどのようなサイズでもセット可能

[ ] SetGet の実装をスッキリさせたい

[ ] malloc 未使用バージョン

[ ] 一度に INF できる EPC の数 (現状 16 で固定)

[ ] MQX でのライブラリ化(独立したプロジェクト化)

[X] ノードプロファイルのクラス情報の自動計算 (0xD3〜0xD7)
=> ミドルウェアアダプタの NodeProfile の定義作成に含める方向で

[X] 機器オブジェクトのプロパティマップ上書き (0x9D、0x9E、0x9F)
=> まぎらわしいので不採用

[X] プロパティの検索をバイナリサーチで実装

[X] プロパティ定義時(ELL_RegisterObject)、値を後で設定できるように PDC=0 を許容する
=> ELL_InitProperty で確定する

[X] PDC=0 のプロパティ(Setup/Getup のもの)の Anno をどうするか？
=> ELL_SetProperty した後、ELL_NeedAnnounce でフラグを立てる

[ ] スタックのリセット

[ ] MWA ステートマシンからの ELL_ 関数のアクセスを排他処理
<= 別タスクでの動作を前提とするため、リソースが競合する可能性あり

[X] プロパティ初期化で 0x82、0x8A〜0x8E、0x9B〜0x9E は無視する

[ ] 通常動作開始時に、レディ機器からプロパティ通知がなければアクセス要求で取得

[ ] ENL ⇒ レディ機器の IASetup で状変プロパティが変更されたとき、アダ
    プタが INF を発行すべきかどうか (レディ機器がやるべきか？)

[ ] 存在しない EOJ に対して要求電文に対して無視すること


【ミドルウェアアダプタ】

■ プラットフォーム依存

  以下はプラットフォームに依存して実装されるパートである

  * UART 送受信の実装
  * フレーム送受信のキューの実装
  * タイムアウト用タイマー


■ フレーム受信

  1 バイトごとに ELL_RecvData() を呼び出す

  1 バイト受信後からある程度の時間経過があれば、ELL_ConfirmRecvFrame()
  を呼び出して受信フレームを確定する


次のプロパティをセット

0x82 Version 情報
0x8A メーカコード
0x8B 事業所コード
0x8C 商品コード
0x8D 製造番号
0x8E 製造年月日

StateMachine で NormalOperation C1 のタイムアウトを


・東芝エアコン(レディ機器)での評価
  ← プロパティマップに載っていないプロパティが通知される (0xFE)
  ← 0x0003, 0x11 に対して 0xFFFF の result を返しているが OK か？
