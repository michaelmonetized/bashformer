#!/usr/bin/env bash
# bash-platformer-nerdfont-fast.sh
# Controls: A/D or ←/→ to move, W/Space/↑ to jump, Q to quit

set -u

### ------------------------------ config ---------------------------------- ###
FPS=30
SLEEP_SEC="0.033"   # ~30fps; change to 0.016 for 60fps (might still be ok now)

GRAVITY=1
JUMP_VY=-6
MOVE_VX=2
MAX_FALL=7

VW=64
VH=20

# Nerd Font sprites (edit to taste)
SPR_PLAYER=""
SPR_WALL="󰟢"
SPR_SPIKE="󰛦"
SPR_GOAL="󰄬"
SPR_COIN="󰆦"
SPR_EMPTY=" "

### ------------------------------ level ----------------------------------- ###
read -r -d '' LEVEL <<'EOF'
####################################################################################################
#..................................................................................................#
#..................................................................................................#
#..................................................................................................#
#.........................o......................o............................o.....................#
#..................................................................................................#
#.............#####............................................................#####..............#
#..................................#####.........................................................G#
#....................o.................................................o....................#####..#
#..........#####.........................#####....................#####............................#
#..................................................................................................#
#.............................................................o....................................#
#.....................#####.............#####......................................................#
#..................................................................................................#
#.....o............................................................................................#
#..................#####...................................................#####...................#
#...............................................#####..............................................#
#..............................o...............................o...................................#
#...............^^^^^^^....................^^^^^^^....................^^^^^^^.......................#
####################################################################################################
EOF

### ------------------------------ setup ----------------------------------- ###
cleanup() {
  tput cnorm 2>/dev/null || true
  stty echo 2>/dev/null || true
  printf '\e[?25h\e[0m\e[2J\e[H' 2>/dev/null || true
}
trap cleanup EXIT INT TERM

stty -echo -icanon time 0 min 0 2>/dev/null || true
tput civis 2>/dev/null || true
printf '\e[?25l\e[2J'

term_resize() {
  local cols rows
  cols=$(tput cols 2>/dev/null || echo 80)
  rows=$(tput lines 2>/dev/null || echo 24)
  (( VW > cols )) && VW=$cols
  (( VH > rows-1 )) && VH=$((rows-1))
  (( VH < 10 )) && VH=10
  (( VW < 30 )) && VW=30
}

mapfile -t LINES <<<"$LEVEL"
MAP_H=${#LINES[@]}
MAP_W=${#LINES[0]}

tile_at() { # x y
  local x=$1 y=$2
  if (( x < 0 || y < 0 || y >= MAP_H || x >= MAP_W )); then
    printf '#'
    return
  fi
  local line=${LINES[y]}
  printf '%s' "${line:x:1}"
}

is_solid() { [[ "$1" == "#" ]]; }
is_spike() { [[ "$1" == "^" ]]; }
is_goal()  { [[ "$1" == "G" ]]; }
is_coin()  { [[ "$1" == "o" ]]; }

remove_coin() {
  local x=$1 y=$2
  local line=${LINES[y]}
  LINES[y]="${line:0:x}.${line:x+1}"
}

### ---- fast tile translation table: map raw chars -> sprite chars -------- ###
# We’ll translate viewport slices using bash string replacement.
# (Much faster than per-cell case.)
to_sprites() {
  local s="$1"
  s=${s//\#/$SPR_WALL}
  s=${s//\^/$SPR_SPIKE}
  s=${s//G/$SPR_GOAL}
  s=${s//o/$SPR_COIN}
  s=${s//./$SPR_EMPTY}
  s=${s// /$SPR_EMPTY}
  printf '%s' "$s"
}

### ------------------------------ input ----------------------------------- ###
KEY=""
read_key() {
  KEY=""
  local k
  IFS= read -rsn1 k 2>/dev/null || true
  [[ -z "$k" ]] && return

  if [[ "$k" == $'\e' ]]; then
    local k2 k3
    IFS= read -rsn1 k2 2>/dev/null || true
    IFS= read -rsn1 k3 2>/dev/null || true
    if [[ "$k2" == "[" ]]; then
      case "$k3" in
        A) KEY="JUMP" ;;
        C) KEY="RIGHT" ;;
        D) KEY="LEFT" ;;
      esac
    fi
    return
  fi

  case "$k" in
    [aA]) KEY="LEFT" ;;
    [dD]) KEY="RIGHT" ;;
    [wW]) KEY="JUMP" ;;
    " ")  KEY="JUMP" ;;
    [qQ]) KEY="QUIT" ;;
  esac
}

### ------------------------------ game state ------------------------------ ###
px=2
py=$((MAP_H-3))
vx=0
vy=0
on_ground=0
camx=0

coins=0
deaths=0
won=0

# spawn finder
for ((y=1; y<MAP_H-1; y++)); do
  for ((x=1; x<MAP_W/4; x++)); do
    t=$(tile_at "$x" "$y")
    b=$(tile_at "$x" $((y+1)))
    if [[ "$t" != "#" && "$t" != "^" ]] && is_solid "$b"; then
      px=$x; py=$y
      y=$MAP_H; break
    fi
  done
done

die() {
  ((deaths++))
  vx=0; vy=0
  px=2; py=$((MAP_H-3))
  for ((y=1; y<MAP_H-1; y++)); do
    for ((x=1; x<MAP_W/4; x++)); do
      t=$(tile_at "$x" "$y")
      b=$(tile_at "$x" $((y+1)))
      if [[ "$t" != "#" && "$t" != "^" ]] && is_solid "$b"; then
        px=$x; py=$y
        y=$MAP_H; break
      fi
    done
  done
}

check_tile_events() {
  local t
  t=$(tile_at "$px" "$py")
  if is_spike "$t"; then die; return; fi
  if is_goal  "$t"; then won=1; fi
  if is_coin  "$t"; then remove_coin "$px" "$py"; ((coins++)); fi
}

try_move_x() {
  local dx=$1 nx=$((px+dx))
  local t; t=$(tile_at "$nx" "$py")
  is_solid "$t" && return
  px=$nx
}

try_move_y() {
  local dy=$1 ny=$((py+dy))
  local t; t=$(tile_at "$px" "$ny")
  if is_solid "$t"; then
    (( dy > 0 )) && on_ground=1
    vy=0
    return 1
  fi
  py=$ny
  return 0
}

### ------------------------------ rendering -------------------------------- ###
draw() {
  term_resize

  camx=$((px - VW/2))
  (( camx < 0 )) && camx=0
  (( camx > MAP_W - VW )) && camx=$((MAP_W - VW))
  (( camx < 0 )) && camx=0

  local FRAME=""
  FRAME+=$'\e[H'
  FRAME+="Coins:${coins}  Deaths:${deaths}  (A/D or ←/→) Jump:(W/Space/↑) Quit:(Q)"
  FRAME+=$'\n'

  local y gy raw slice line_s
  for ((y=0; y<VH; y++)); do
    gy=$y
    # Grab raw map slice quickly (string slice). OOB rows treated as all wall.
    if (( gy < 0 || gy >= MAP_H )); then
      raw=$(printf '%*s' "$VW" "" | tr ' ' '#')
    else
      raw=${LINES[gy]}
      slice=${raw:camx:VW}
      # If we're near edges and slice shorter, pad with walls
      if (( ${#slice} < VW )); then
        slice+=$(printf '%*s' $((VW-${#slice})) "" | tr ' ' '#')
      fi
      raw="$slice"
    fi

    line_s=$(to_sprites "$raw")

    # Overlay player if within this row and viewport
    if (( gy == py )); then
      local pxv=$((px - camx))
      if (( pxv >= 0 && pxv < VW )); then
        line_s="${line_s:0:pxv}${SPR_PLAYER}${line_s:pxv+1}"
      fi
    fi

    FRAME+="$line_s"$'\n'
  done

  if (( won )); then
    FRAME+=$'\n'"YOU WIN! Press Q to quit."
  fi

  printf '%s' "$FRAME"
}

### ------------------------------ main loop -------------------------------- ###
while :; do
  read_key
  [[ "$KEY" == "QUIT" ]] && break

  if (( ! won )); then
    vx=0
    [[ "$KEY" == "LEFT"  ]] && vx=$((-MOVE_VX))
    [[ "$KEY" == "RIGHT" ]] && vx=$(( MOVE_VX))

    if [[ "$KEY" == "JUMP" ]]; then
      if (( on_ground )); then
        vy=$JUMP_VY
        on_ground=0
      fi
    fi
  else
    vx=0
  fi

  if (( ! on_ground )); then
    vy=$((vy + GRAVITY))
    (( vy > MAX_FALL )) && vy=$MAX_FALL
  fi

  if (( vx != 0 )); then
    step=$(( vx>0 ? 1 : -1 ))
    for ((i=0; i<${vx#-}; i++)); do
      try_move_x "$step"
      check_tile_events
    done
  fi

  if (( vy != 0 )); then
    step=$(( vy>0 ? 1 : -1 ))
    on_ground=0
    for ((i=0; i<${vy#-}; i++)); do
      if ! try_move_y "$step"; then break; fi
      check_tile_events
    done
  else
    below=$(tile_at "$px" $((py+1)))
    on_ground=0
    is_solid "$below" && on_ground=1
  fi

  check_tile_events
  draw
  sleep "$SLEEP_SEC"
done
