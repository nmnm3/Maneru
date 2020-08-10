初期操作設定は99と同じ、十字のみ対応。CCが見えるnextは7個、使うコアは一つで読みは10万通りまで。
重力はなく最後はハードドロップが必要。火力を受けたいときはプラスボタンでライン数を調整して次のハードドロップで下穴が来る。
初期設定のCCは動作確認が目的なので、PCに余裕があるならcc.max_nodesとcc.threadsを上げることがおすすめ。
質問や機能追加の要望などはDiscordで: https://discord.gg/WQXcC3p

AY：時計回り回転
BX：反時計回り回転

左右：ミノの移動
上：ハードドロップ
下：ソフトドロップ
L, R：ホールド

プラス：火力カウンタ加算
火力は次のハードドロップの後で受ける、受けすぎるとゲームオーバー。

Lスティックボタン：ミノを出現位置に戻す
Rスティックボタン：CCをON/OFFに切り替える

詳しい設定はmaneru.configで。
ゲーム
game.next ホールドとメイン画面とnext画面合わせての個数。10以上だとnext画面は8まで、CCからは見えている。
game.gravity 落下重力。0=落下しない、20=瞬間落下。中間値はまだ実装されていない
game.delay_at_beginning 開始画面の待機時間
game.garbage_min 受ける最小火力
game.garbage_max 受ける最大火力
game.garbage_autolevel 自動的に火力を受ける水準、最高点がこのライン数以下なら最小火力を受ける
game.hole_repeat 穴ばら率、0=完全穴ばら、1.0=完全直列
game.exact_cc_move yes: CCが常に最善手を出して、必ず同じ場所にミノを置かなければならない。no: ミノを自由に置ける、CCが先読みを出す。
game.hold_lock ホールドしたばかりのミノはすぐにホールドで出すことができない

グラフィック
graphic.resolution 解像度。600x400、900x600、1200x800、1800x1200のみ対応
graphic.fps 毎秒フレーム数
graphic.hint_flash_cycle 次の置き場所の点滅周期
graphic.hint_min_opacity 次の置き場所の最小透過度
graphic.plan_opacity 先読みの透過度
graphic.max_plan 先読みの最大表示個数、0はすべて表示

ボタン配置
control.left 十字左
control.right 十字右
control.up 十字上
control.down 十字下
control.a XBOX基準でAボタン、プロコンBボタン
control.b XBOX基準でBボタン、プロコンAボタン
control.x XBOX基準でXボタン、プロコンYボタン
control.y XBOX基準でYボタン、プロコンXボタン
control.start スタートボタン / プラスボタン
control.ls 左スティックボタン
control.rs 右スティックボタン

ボタン機能
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
reset_piece ミノを出現位置に戻す
toggle_cc CCをON/OFFに切り替える

ColdClear
cc.speculate nextの先読み
cc.pcloop パフェモード
cc.min_nodes 最小限の読み
cc.max_nodes 最大限の読み。上げると読みの深さは増すがメモリ消費量も上がる
cc.threads 計算に使うコア/スレッド数。上げると読みが早くなるがCPU使用率も上がる