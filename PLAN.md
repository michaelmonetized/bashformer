# Bashformer Development Plan

## Project Overview

Bashformer is a terminal-based Flappy Bird-style game built with React and Ink, running on Bun. It features smooth 30 FPS gameplay, physics-based flap/gravity mechanics, pipe obstacles, and score tracking. The project also includes experimental C-based terminal games (cflap, ctetris, cdraw, cbreakout, cdaw).

## Current State

- **Version**: Early development
- **Tech Stack**: React 19, Ink 6.5, Bun, TypeScript
- **Features Implemented**:
  - 30 FPS game loop
  - Physics-based flap/gravity movement
  - Pipe obstacle generation and scrolling
  - Collision detection
  - Score tracking
  - Keyboard controls (space/up to flap)
  - Terminal-responsive sizing
- **Status**: Playable prototype

## Phase 1: Core Game Polish (2-3 weeks)

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

## Phase 2: Game Features (3-4 weeks)

### Goals
- [ ] Add day/night visual themes
- [ ] Implement different bird skins/characters
- [ ] Add power-ups (slow-mo, shield, magnet coins)
- [ ] Create coin collectibles between pipes
- [ ] Add achievement system
- [ ] Multiple game modes (classic, timed, zen/no-death)

### Success Criteria
- 3+ game modes
- 2+ power-up types
- Achievement tracking

## Phase 3: Distribution & Community (2-3 weeks)

### Goals
- [ ] Package as npm installable CLI game (`npx bashformer`)
- [ ] Add global leaderboard (simple server or GitHub Gist-based)
- [ ] Write comprehensive README with GIFs
- [ ] Add accessibility options (colorblind modes, reduced motion)
- [ ] Publish C variants as separate packages

### Success Criteria
- Published to npm
- 100+ downloads
- README with gameplay GIFs

## Success Metrics

| Metric | Target |
|--------|--------|
| Game Modes | 3+ |
| npm Downloads | 500+ |
| GitHub Stars | 50+ |
| Frame Rate | Consistent 30 FPS |

## C Experiments

The repo also contains C-based terminal games:
- **cflap** — Flappy Bird in C (ncurses)
- **ctetris** — Tetris clone
- **cdraw** — Terminal drawing tool
- **cbreakout** — Breakout clone
- **cdaw** — Digital audio workstation experiment
