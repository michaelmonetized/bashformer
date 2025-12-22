#!/usr/bin/env bun
import React, { useEffect, useRef, useState } from 'react';
import { Box, Text, render } from 'ink';
import { useStdin } from 'ink';

// --- Terminal size helpers -------------------------------------------------
const getTerminalSize = () => {
  const cols = process.stdout.columns || parseInt(process.env.COLUMNS || '80', 10);
  const rows = process.stdout.rows || parseInt(process.env.LINES || '24', 10);
  return { width: cols, height: rows };
};

const term = getTerminalSize();

// --- Game configuration -----------------------------------------------------
const CONFIG = {
  FPS: 30,
  GRAVITY: 0.32,       // a bit snappier fall
  FLAP_VY: -1.7,       // slightly stronger flap
  MAX_FALL_VY: 2.2,    // clamp fall speed for smoothness
  MAX_FLAP_VY: -2.2,
  PIPE_SPEED: 3.1,     // a hair faster than 1
  PIPE_SPAWN_RATE: 0.028,
  PIPE_GAP: 8,
  VIEWPORT_WIDTH: term.width,
  VIEWPORT_HEIGHT: Math.max(16, term.height - 1), // keep some minimum height
} as const;

const GROUND_Y = CONFIG.VIEWPORT_HEIGHT - 2; // last row is status line

// --- Types ------------------------------------------------------------------
interface Pipe {
  x: number;         // left edge
  gapY: number;      // center of the vertical gap
}

interface GameState {
  birdY: number;
  birdVy: number;
  pipes: Pipe[];
  score: number;
  gameOver: boolean;
  started: boolean;
}

// --- Game component ---------------------------------------------------------
function Game() {
  const initialBirdY = Math.floor(CONFIG.VIEWPORT_HEIGHT / 2);

  // We keep the real game state in a ref to avoid tons of React state churn.
  const stateRef = useRef<GameState>({
    birdY: initialBirdY,
    birdVy: 0,
    pipes: [],
    score: 0,
    gameOver: false,
    started: false,
  });

  // Single tick counter just to trigger re-renders.
  const [, setTick] = useState(0);

  const keysRef = useRef(new Set<string>());
  const { stdin, setRawMode } = useStdin();

  // --- Input handling -------------------------------------------------------
  useEffect(() => {
    if (!stdin || !setRawMode) return;

    setRawMode(true);

    const handleInput = (chunk: Buffer) => {
      const input = chunk.toString();
      const s = stateRef.current;

      if (input === 'q' || input === 'Q' || input === '\u0003') {
        process.exit(0);
      }

      if (input === ' ' || input === '\u001b[A') {
        if (!s.started) {
          s.started = true;
          s.gameOver = false;
          s.birdVy = 0;
        } else if (s.gameOver) {
          // Restart
          s.birdY = initialBirdY;
          s.birdVy = 0;
          s.pipes = [];
          s.score = 0;
          s.gameOver = false;
          s.started = true;
        } else {
          keysRef.current.add('FLAP');
        }
      }
    };

    stdin.on('data', handleInput);

    return () => {
      stdin.removeListener('data', handleInput);
      setRawMode(false);
    };
  }, [stdin, setRawMode, initialBirdY]);

  // --- Game loop ------------------------------------------------------------
  useEffect(() => {
    const step = () => {
      const s = stateRef.current;
      if (!s.started || s.gameOver) return;

      // Handle flap
      if (keysRef.current.has('FLAP')) {
        keysRef.current.delete('FLAP');
        s.birdVy = CONFIG.FLAP_VY;
      }

      // Gravity
      s.birdVy += CONFIG.GRAVITY;
      if (s.birdVy > CONFIG.MAX_FALL_VY) s.birdVy = CONFIG.MAX_FALL_VY;
      if (s.birdVy < CONFIG.MAX_FLAP_VY) s.birdVy = CONFIG.MAX_FLAP_VY;

      // Integrate position
      s.birdY += s.birdVy;
      if (s.birdY < 0) {
        s.birdY = 0;
        s.birdVy = 0;
      }
      if (s.birdY >= GROUND_Y) {
        s.birdY = GROUND_Y;
        s.gameOver = true;
      }

      // Pipes
      const birdX = Math.floor(CONFIG.VIEWPORT_WIDTH / 5);
      const birdYInt = Math.round(s.birdY);

      const moved: Pipe[] = [];
      for (const p of s.pipes) {
        const x = p.x - CONFIG.PIPE_SPEED;
        if (x > -5) moved.push({ ...p, x });
      }

      // Spawn new pipe
      if (!s.gameOver && Math.random() < CONFIG.PIPE_SPAWN_RATE) {
        const minTop = 2;
        const maxBottom = GROUND_Y - 2;
        const gapHalf = Math.floor(CONFIG.PIPE_GAP / 2);
        const minCenter = minTop + gapHalf;
        const maxCenter = maxBottom - gapHalf;
        const gapY = Math.floor(Math.random() * (maxCenter - minCenter + 1)) + minCenter;
        moved.push({ x: CONFIG.VIEWPORT_WIDTH, gapY });
      }

      // Collisions + scoring
      for (const p of moved) {
        const pipeX = Math.round(p.x);
        const pipeRight = pipeX + 2; // 3-char wide

        if (pipeX <= birdX && pipeRight >= birdX) {
          const topEnd = p.gapY - Math.floor(CONFIG.PIPE_GAP / 2);
          const bottomStart = p.gapY + Math.ceil(CONFIG.PIPE_GAP / 2);
          if (birdYInt < topEnd || birdYInt >= bottomStart) {
            s.gameOver = true;
          }
        }

        // Score when bird just passed the pipe
        if (!s.gameOver && pipeRight === birdX - 1) {
          s.score += 1;
        }
      }

      s.pipes = moved;

      // Trigger re-render
      setTick(t => t + 1);
    };

    const interval = setInterval(step, 1000 / CONFIG.FPS);
    return () => clearInterval(interval);
  }, []);

  // --- Rendering ------------------------------------------------------------
  const renderGame = () => {
    const s = stateRef.current;
    const lines: string[] = [];

    // Status / HUD line (mostly static, only score changes)
    if (!s.started) {
      lines.push(`Press SPACE to start  |  Q to quit`);
    } else if (s.gameOver) {
      lines.push(`GAME OVER  |  Score: ${s.score}  |  SPACE: restart  Q: quit`);
    } else {
      lines.push(`Score: ${s.score}`);
    }

    // Prepare grid
    const grid: string[][] = [];
    for (let y = 0; y < CONFIG.VIEWPORT_HEIGHT; y++) {
      grid[y] = Array(CONFIG.VIEWPORT_WIDTH).fill(' ');
    }

    // Ground
    for (let x = 0; x < CONFIG.VIEWPORT_WIDTH; x++) {
      grid[GROUND_Y][x] = '═';
    }

    // Pipes
    for (const p of s.pipes) {
      const pipeX = Math.round(p.x);
      if (pipeX >= CONFIG.VIEWPORT_WIDTH) continue;
      if (pipeX + 2 < 0) continue;

      const topEnd = p.gapY - Math.floor(CONFIG.PIPE_GAP / 2);
      const bottomStart = p.gapY + Math.ceil(CONFIG.PIPE_GAP / 2);

      for (let x = pipeX; x < pipeX + 3; x++) {
        if (x < 0 || x >= CONFIG.VIEWPORT_WIDTH) continue;
        // Top
        for (let y = 0; y < topEnd && y < CONFIG.VIEWPORT_HEIGHT; y++) {
          grid[y][x] = '█';
        }
        // Bottom
        for (let y = bottomStart; y < GROUND_Y && y < CONFIG.VIEWPORT_HEIGHT; y++) {
          grid[y][x] = '█';
        }
      }
    }

    // Bird
    const birdX = Math.floor(CONFIG.VIEWPORT_WIDTH / 5);
    const birdYInt = Math.round(s.birdY);
    if (birdYInt >= 0 && birdYInt < GROUND_Y && birdX >= 0 && birdX < CONFIG.VIEWPORT_WIDTH) {
      grid[birdYInt][birdX] = '🐦';
    }

    for (let y = 0; y < CONFIG.VIEWPORT_HEIGHT; y++) {
      lines.push(grid[y].join(''));
    }

    return lines.join('\n');
  };

  return (
    <Box flexDirection="column">
      <Text>{renderGame()}</Text>
    </Box>
  );
}

render(<Game />);
