# Maneru

Maneru means imitate in Japanese. This is a program to practice Tetris with the strongest PPT AI [ColdClear](https://github.com/MinusKelvin/cold-clear). The bot's plans are displayed on the screen, you can follow the bot's move or play on you own. It's useful to learn from CC's T-spin setups, digging stragegies and recovery methods.

## How to play
Simply connect your Controller/Gamepad to PC and start Maneru.exe. Switch Pro controller and XBOX controller are tested currently. The driver is same to Steam so in theory any controller can be recognized by Steam should work just fine.

Default control setting is same to Tetris 99. Next preview is 7 piece. No gravity, so hard drop is needed to place each piece. If you want to recieve garbage lines, press +(Procon)/Start(XBOX) button. Garbage lines will be placed after the next hard drop.

## ColdClear configuration
By default ColdClear will use only one CPU core and will read up to 100k nodes. This mode consumes relatively small amount of CPU and memory. If your can spare more, please increase cc.max_nodes and cc.threads in maneru.config. The strength of the bot can be improved drastically.

### Default control settings(Switch Pro controller)
AY: Rotate clockwise

BX: Rotate counter-clockwise

Left, Right: Move left & right

Up: Hard drop

Down: Soft drop

L,R: Hold

+: Increase garbage counter

L Stick Button: Return current piece to start position
R Stick Button: Toggle ON/OFF of CC

## maneru.config

game
```
game.next: Number of pieces in next preview at the beginning. After hold and the first piece spawn, it's decreased by 2. If set to >10, only 8 pieces are displayed on the screen, but CC will see every piece.
game.gravity: 0 or 20. 0(default): no drop due to gravity. 20: instant drop.
game.delay_at_beginning: Delay time at the beginning, to give the bot some time for thinking.
game.garbage_min: Minimum value of garbage counter.
game.garbage_max: Maximum value of garbage counter.
game.garbage_autolevel: If highest line is below this level, recieve garbage_min automatically.
game.hole_repeat: Possibility of a repeated garbage hole. 1.0 for always repeat and 0 for never repeat.
game.exact_cc_move: Follow the bots exactly. Prevents other moves from happening. Set to "no" if you want to do yourself.
game.hold_lock: Whether an hold piece can be switch out immediately. By default you must do at least one hard drop before you can hold again.
```
graphic
```
graphic.resolution: 600x400, 900x600, 1200x800, 1800x1200
graphic.fps: frame per seconds
graphic.hint_flash_cycle: in seconds.
graphic.hint_min_opacity: 0~1.0
graphic.plan_opacity: 0~1.0
graphic.max_plan: Maximum number of plans to display, 0 to display all
```
Button assignment, in the format of `button`=`function`

Buttons
```
control.left: Left
control.right: Right
control.up: Up
control.down: Down
control.a: A(XBOX) / B(Procon)
control.b: B(XBOX) / A(Procon)
control.x: X(XBOX) / Y(Procon)
control.y: Y(XBOX) / X(Procon)
control.start: Start(XBOX) / +(Procon)
control.ls Left stick button
control.rs Right stick button
```
Functions
```
left
right
repeat_delay: Autoshift delay
repeat_rate: Autoshift rate
hard_drop
soft_drop
rotate_clockwise
rotate_counter_clockwise
hold
garbage: increase garbage counter
reset_piece: reset current piece to spawn position
toggle_cc: turn on/off CC
```

ColdClear bot settings
```
cc.speculate: CC will speculate on next moves according to 7 bag rule.
cc.pcloop: CC will do perfect clear until garbage is recieved.
cc.min_nodes: Minimal required thinking. Increase this to allow CC to think more after garbage or misdrop.
cc.max_nodes: Maximum thinking. Increase this to allow CC to think more for each move. This will increase memory consumption.
cc.threads: Number of threads CC will use. Increase this for faster thinking. This will increase CPU consumption.
```

## How to build

### Prerequisite

- [Visual Studio](https://aka.ms/vs/16/release/vs_community.exe). Enable `Desktop development with C++` and `Game development with C++`
- [Rust](https://static.rust-lang.org/rustup/dist/x86_64-pc-windows-msvc/rustup-init.exe)
- [git](https://git-scm.com/download/win)

### ColdClear
```
git clone https://github.com/MinusKelvin/cold-clear.git
cd cold-clear
cargo build --release
```
### SDL2
Download and unpack: https://www.libsdl.org/release/SDL2-devel-2.0.12-VC.zip

### Maneru
```
git clone https://github.com/nmnm3/Maneru.git
cd Maneru
mkdir build
cd build
cmake -DCOLD_CLEAR_ROOT=C:/cold-clear -DSDL2_ROOT=C:/SDL2-2.0.12 -G "Visual Studio 16 2019" ..
cmake --build . --config Release --target ALL_BUILD
```