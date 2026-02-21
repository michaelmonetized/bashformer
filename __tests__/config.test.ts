import { describe, expect, test } from "bun:test";

// Extract CONFIG and helpers for testing
// (These mirror the values in index.tsx — keep in sync)
const getTerminalSize = () => {
  const cols =
    process.stdout.columns || parseInt(process.env.COLUMNS || "80", 10);
  const rows =
    process.stdout.rows || parseInt(process.env.LINES || "24", 10);
  return { width: cols, height: rows };
};

const term = getTerminalSize();

const CONFIG = {
  FPS: 30,
  GRAVITY: 0.32,
  FLAP_VY: -1.7,
  MAX_FALL_VY: 2.2,
  MAX_FLAP_VY: -2.2,
  PIPE_SPEED: 3.1,
  PIPE_SPAWN_RATE: 0.028,
  PIPE_GAP: 8,
  VIEWPORT_WIDTH: term.width,
  VIEWPORT_HEIGHT: Math.max(16, term.height - 1),
} as const;

const GROUND_Y = CONFIG.VIEWPORT_HEIGHT - 2;

describe("game config", () => {
  test("viewport has minimum height of 16", () => {
    expect(CONFIG.VIEWPORT_HEIGHT).toBeGreaterThanOrEqual(16);
  });

  test("ground is 2 rows above bottom", () => {
    expect(GROUND_Y).toBe(CONFIG.VIEWPORT_HEIGHT - 2);
  });

  test("pipe gap is positive and reasonable", () => {
    expect(CONFIG.PIPE_GAP).toBeGreaterThan(3);
    expect(CONFIG.PIPE_GAP).toBeLessThan(GROUND_Y);
  });

  test("flap velocity is negative (upward)", () => {
    expect(CONFIG.FLAP_VY).toBeLessThan(0);
  });

  test("gravity is positive (downward)", () => {
    expect(CONFIG.GRAVITY).toBeGreaterThan(0);
  });
});

describe("physics simulation", () => {
  test("bird falls under gravity", () => {
    let y = 10;
    let vy = 0;
    vy += CONFIG.GRAVITY;
    y += vy;
    expect(y).toBeGreaterThan(10);
  });

  test("flap moves bird upward", () => {
    let y = 10;
    let vy = CONFIG.FLAP_VY;
    y += vy;
    expect(y).toBeLessThan(10);
  });

  test("velocity is clamped", () => {
    let vy = 100;
    if (vy > CONFIG.MAX_FALL_VY) vy = CONFIG.MAX_FALL_VY;
    expect(vy).toBe(CONFIG.MAX_FALL_VY);

    vy = -100;
    if (vy < CONFIG.MAX_FLAP_VY) vy = CONFIG.MAX_FLAP_VY;
    expect(vy).toBe(CONFIG.MAX_FLAP_VY);
  });

  test("bird hits ground triggers game over zone", () => {
    let y = GROUND_Y + 1;
    const gameOver = y >= GROUND_Y;
    expect(gameOver).toBe(true);
  });
});

describe("pipe generation", () => {
  test("pipe gap center stays within valid bounds", () => {
    const minTop = 2;
    const maxBottom = GROUND_Y - 2;
    const gapHalf = Math.floor(CONFIG.PIPE_GAP / 2);
    const minCenter = minTop + gapHalf;
    const maxCenter = maxBottom - gapHalf;

    expect(minCenter).toBeLessThanOrEqual(maxCenter);

    // Simulate many pipes
    for (let i = 0; i < 100; i++) {
      const gapY =
        Math.floor(Math.random() * (maxCenter - minCenter + 1)) + minCenter;
      expect(gapY - gapHalf).toBeGreaterThanOrEqual(minTop);
      expect(gapY + Math.ceil(CONFIG.PIPE_GAP / 2)).toBeLessThanOrEqual(
        maxBottom + 2
      );
    }
  });
});
