# Bashformer Development Plan

## Project Overview

Bashformer is a terminal-based platformer game built with React and Ink, running on Bun. It features smooth 60 FPS gameplay, physics-based movement, collectible coins, deadly spikes, and a goal-based win condition. The game represents a significant improvement over its original Bash version with better performance, type safety, and state management.

## Current State

- **Version**: Early development (no version specified)
- **Tech Stack**: React 19, Ink 6.5, Bun, TypeScript
- **Features Implemented**:
  - 60 FPS game loop
  - Physics-based movement with gravity
  - Collision detection
  - Game elements (player, walls, spikes, coins, goal)
  - Death counter tracking
  - Keyboard controls (WASD, arrows, space)
- **Status**: Playable single-level prototype

## Phase 1: Core Game Polish (2-3 weeks)

### Goals
- [ ] Add multiple levels with increasing difficulty
- [ ] Implement level progression system
- [ ] Add persistent high score/best time tracking
- [ ] Create level select menu
- [ ] Add sound effects (terminal beeps/audio)
- [ ] Implement pause functionality
- [ ] Add restart level option without full game restart

### Success Criteria
- 5+ playable levels
- Scores persist between sessions
- Clean menu navigation

## Phase 2: Game Features (3-4 weeks)

### Goals
- [ ] Add moving platforms
- [ ] Implement enemy characters with AI patterns
- [ ] Add power-ups (double jump, invincibility, speed boost)
- [ ] Create checkpoints system
- [ ] Add timer-based challenges
- [ ] Implement level editor mode
- [ ] Add different character skins/themes

### Success Criteria
- 3+ enemy types
- 3+ power-up types
- Functional level editor

## Phase 3: Distribution & Community (2-3 weeks)

### Goals
- [ ] Package as npm installable CLI game
- [ ] Add multiplayer/shared high score leaderboard
- [ ] Create community level sharing system
- [ ] Write comprehensive documentation
- [ ] Add accessibility options (colorblind modes, reduced motion)
- [ ] Implement achievement system
- [ ] Create speedrun mode with global leaderboard

### Success Criteria
- Published to npm
- 100+ downloads
- Community-contributed levels

## Success Metrics

| Metric | Target |
|--------|--------|
| Levels | 15+ |
| npm Downloads | 500+ |
| GitHub Stars | 50+ |
| Community Levels | 10+ |
| Frame Rate | Consistent 60 FPS |

## Timeline Summary

| Phase | Duration | Focus |
|-------|----------|-------|
| Phase 1 | Weeks 1-3 | Core polish & levels |
| Phase 2 | Weeks 4-7 | Advanced features |
| Phase 3 | Weeks 8-10 | Distribution & community |

**Total Estimated Time**: 10 weeks
