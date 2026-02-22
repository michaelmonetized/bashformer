# Bashformer - Terminal Flappy Bird

A smooth Flappy Bird clone built with React and [Ink](https://github.com/vadimdemedes/ink) for the terminal.

## Features

- 30 FPS gameplay with smooth physics
- Pipe obstacles with randomized gaps
- Score tracking
- Responsive terminal-size adaptation
- Clean Unicode rendering (█ pipes, 🐦 bird, ═ ground)

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

- **Space** - Flap (start game / fly upward)
- **Q** - Quit game

## How It Works

Navigate the bird through gaps between pipes. Each pipe passed scores a point. Hit a pipe or the ground and it's game over. Press Space to restart.

## Tech

- **Runtime:** Bun
- **UI:** React + Ink (terminal rendering)
- **Language:** TypeScript
