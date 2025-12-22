# Bashformer - React/Ink Platformer Game

A fast, smooth platformer game built with React and Ink for the terminal.

## Features

- Smooth 60 FPS gameplay
- Physics-based movement with gravity
- Collect coins, avoid spikes, reach the goal
- Death counter tracking
- Responsive controls

## Installation

```bash
bun install
```

## Running

```bash
bun run index.tsx
```

Or make it executable and run directly:

```bash
chmod +x index.tsx
./index.tsx
```

## Controls

- **A** or **←** - Move left
- **D** or **→** - Move right  
- **W**, **Space**, or **↑** - Jump
- **Q** - Quit game

## Game Elements

- 🧙 - Player
- █ - Walls (solid blocks)
- ^ - Spikes (deadly!)
- 🪙 - Coins (collect them!)
- 🏁 - Goal (reach to win!)

## Improvements over Bash Version

- **Much faster**: Runs at 60 FPS vs 30 FPS
- **Smoother**: Better frame timing and rendering
- **Less buggy**: Proper state management and collision detection
- **Better input handling**: More responsive controls
- **Type-safe**: Written in TypeScript
