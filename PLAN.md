# Bashformer Development Plan

## Project Overview

Bashformer is a terminal-based Flappy Bird-style game built with React and Ink, running on Bun. Smooth 30 FPS gameplay, physics-based flap/gravity, pipe obstacles, and score tracking.

**HEAD note (Feb 2026 cleanup):** Experimental C/SDL trees (`vex_sdl`, cflap, ctetris, cdraw, cbreakout, cdaw) were **removed** from this repo. Do not plan distribution of C variants from this tree.

## Current State

- **Tech Stack**: React 19, Ink, Bun, TypeScript
- **Status**: Playable Ink prototype (unreleased)
- **Features Implemented**:
  - 30 FPS game loop
  - Physics-based flap/gravity movement
  - Pipe obstacle generation and scrolling
  - Collision detection
  - Score tracking
  - Keyboard controls (space/up to flap)
  - Terminal-responsive sizing

## Phase 1: Core Game Polish (open)

### Goals
- [ ] Add difficulty scaling (pipes get tighter/faster over time)
- [ ] Implement persistent high score tracking (file-based)
- [ ] Add start screen / game over screen with stats
- [ ] Add sound effects (terminal beeps)
- [ ] Improve visual polish (colors, pipe sprites, ground animation)
- [ ] Add pause functionality

### Success Criteria
- High scores persist between sessions
- Clean start/game-over flow
- Increasing difficulty curve

## Phase 2: Game Features

### Goals
- [ ] Add day/night visual themes
- [ ] Implement different bird skins/characters
- [ ] Add power-ups (slow-mo, shield, magnet coins)
- [ ] Create coin collectibles between pipes
- [ ] Add achievement system
- [ ] Multiple game modes (classic, timed, zen/no-death)

## Phase 3: Distribution & Community

### Goals
- [ ] Package as npm/bun installable CLI game (`npx bashformer` / `bunx bashformer`)
- [ ] Add global leaderboard (simple server or GitHub Gist-based)
- [ ] Write comprehensive README with GIFs
- [ ] Add accessibility options (colorblind modes, reduced motion)

### Success Criteria
- Published to a package registry
- README with gameplay GIFs

## Success Metrics

| Metric | Target |
|--------|--------|
| Game Modes | 3+ |
| Package Downloads | 500+ |
| GitHub Stars | 50+ |
| Frame Rate | Consistent 30 FPS |

*PLAN parity sync: 2026-09-08 — removed deleted C/SDL experiment claims.*
