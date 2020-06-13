# まねるちゃん

最強AIである[ColdClear](https://github.com/MinusKelvin/cold-clear)のプレイをまねて火力と掘りの練習する。CCの積み方は精密な計算の上に成り立っているーー十万、百万手先読みして最善を取る。人間では同じ計算は出来ないが、AIの最善パターンから学べるものは多いはず。

## 使い方
コントローラーをPCに接続してManeru.exeを起動する。SwitchプロコンとXBOXコントローラーはテスト済み、Steamと同じドライバーを使っているから別のコントローラーにも対応できる。

初期操作設定は99と同じ、十字のみ対応。CCが見えるnextは7個、使うコアは一つで読みは10万通りまで。重力はなく最後はハードドロップが必要。火力を受けたいときはプラスボタンでライン数を調整して次のハードドロップで下穴が来る。

初期設定のCCは動作確認が目的なので、PCに余裕があるならcc.max_nodesとcc.threadsを上げることがおすすめ。

### 初期設定(プロコン基準)
AY：時計回り回転

BX：反時計回り回転

左右：ミノの移動

上：ハードドロップ

下：ソフトドロップ

L, R：ホールド

プラス：火力カウンタ加算
火力は次のハードドロップの後で受ける、受けすぎるとゲームオーバー。

## 詳しい設定
maneru.config

ゲーム
```
game.next ホールドとメイン画面とnext画面合わせての個数。10以上だとnext画面は8まで、CCからは見えている。
game.delay_at_beginning 開始画面の待機時間
game.garbage_min 受ける最小火力
game.garbage_max 受ける最大火力
game.hole_repeat 穴ばら率、0=完全穴ばら、1.0=完全直列
game.exact_cc_move CCの動き縛り。yes=CCの読みと違ったら操作は無効化される、no=縛りなし
game.hold_lock ホールドしたばかりのミノはすぐにホールドで出すことができない
```
グラフィック
```
graphic.resolution 解像度。600x400、900x600、1200x800のみ対応
graphic.fps 毎秒フレーム数
graphic.hint_flash_cycle 次の置き場所の点滅周期
graphic.hint_min_opacity 次の置き場所の最小透過度
graphic.plan_opacity 先読みの透過度

```
ボタン配置
```
control.left 十字左
control.right 十字右
control.up 十字上
control.down 十字下
control.a XBOX基準でAボタン、プロコンBボタン
control.b XBOX基準でBボタン、プロコンAボタン
control.x XBOX基準でXボタン、プロコンYボタン
control.y XBOX基準でYボタン、プロコンXボタン
control.start スタートボタン / プラスボタン
```

ボタン機能
```
left 左移動
right 右移動
repeat_delay 溜め操作開始時間
repeat_rate 溜め操作間隔
hard_drop ハードドロップ
soft_drop ソフトドロップ
rotate_clockwise 時計回り
rotate_counter_clockwise 反時計回り
garbage 火力カウンタ
hold ホールド
```

ColdClear
```
cc.speculate nextの先読み
cc.pcloop パフェモード
cc.min_nodes 最小限の読み
cc.max_nodes 最大限の読み。上げると読みの深さは増すがメモリ消費量も上がる
cc.threads 計算に使うコア/スレッド数。上げると読みが早くなるがCPU使用率も上がる
```

## ビルド

### 必要なツール

- [Visual Studio](https://aka.ms/vs/16/release/vs_community.exe)、Desktop development with C++とGame development with C++を有効
- [Rust](https://static.rust-lang.org/rustup/dist/x86_64-pc-windows-msvc/rustup-init.exe)
- [git](https://git-scm.com/download/win)

### ColdClear
```
git clone https://github.com/MinusKelvin/cold-clear.git
cd cold-clear
cargo build --release
```
### SDL2
ダウンロード&解凍のみ: https://www.libsdl.org/release/SDL2-devel-2.0.12-VC.zip

### まねるちゃん
```
git clone https://github.com/nmnm3/Maneru.git
cd Maneru
mkdir build
cd build
cmake -DCOLD_CLEAR_ROOT=C:/cold-clear -DSDL2_ROOT=C:/SDL2-2.0.12 -G "Visual Studio 16 2019" ..
cmake --build . --config Release --target ALL_BUILD
```