//
//  GameState.swift
//  PlatformerSDL
//
//  Game state management - ported from C code
//

import Foundation

struct RectF {
    var x: Float
    var y: Float
    var width: Float
    var height: Float
}

struct Platform {
    var rect: RectF
}

struct Ladder {
    var rect: RectF
    var height: Float { rect.height }
    var x: Float { rect.x }
}

struct Player {
    var rect: RectF
    var x: Float { rect.x }
    var y: Float { rect.y }
    var vx: Float = 0
    var vy: Float = 0
    var onGround: Bool = false
    var onLadder: Bool = false
    var facing: Int = 1 // -1 = left, 1 = right
    var animTime: Float = 0
}

class GameState {
    var platforms: [Platform] = []
    var ladders: [Ladder] = []
    var player: Player
    var level: Int = 1
    var score: Int = 0
    var lives: Int = 3
    var running: Bool = true
    var win: Bool = false
    var gameOver: Bool = false
    
    let gravity: Float = 800.0
    let moveSpeed: Float = 200.0
    let jumpSpeed: Float = 400.0
    
    init() {
        player = Player(rect: RectF(x: 400, y: 100, width: 32, height: 48))
    }
    
    func initLevel() {
        // Initialize level platforms and ladders
        // This is a simplified version - you'd port the full init_level function
        platforms.removeAll()
        ladders.removeAll()
        
        // Ground platform
        platforms.append(Platform(rect: RectF(x: 0, y: 0, width: 800, height: 32)))
        
        // Example platforms (you'd port the full level generation)
        platforms.append(Platform(rect: RectF(x: 100, y: 200, width: 200, height: 32)))
        platforms.append(Platform(rect: RectF(x: 500, y: 350, width: 200, height: 32)))
        
        // Example ladder
        ladders.append(Ladder(rect: RectF(x: 300, y: 0, width: 32, height: 200)))
        
        // Reset player
        player.rect.x = 400
        player.rect.y = 100
        player.vx = 0
        player.vy = 0
    }
    
    func update(deltaTime: Float, moveLeft: Bool, moveRight: Bool, jump: Bool, climbUp: Bool, climbDown: Bool, attack: Bool) {
        // Update player movement
        if moveLeft {
            player.vx = -moveSpeed
            player.facing = -1
        } else if moveRight {
            player.vx = moveSpeed
            player.facing = 1
        } else {
            player.vx = 0
        }
        
        // Jump
        if jump && player.onGround {
            player.vy = jumpSpeed
            player.onGround = false
        }
        
        // Climbing
        if player.onLadder {
            if climbUp {
                player.vy = moveSpeed
            } else if climbDown {
                player.vy = -moveSpeed
            }
        } else {
            // Apply gravity
            player.vy -= gravity * deltaTime
        }
        
        // Update position
        player.rect.x += player.vx * deltaTime
        player.rect.y += player.vy * deltaTime
        
        // Collision with platforms
        checkPlatformCollisions()
        
        // Check ladder collisions
        checkLadderCollisions(climbUp: climbUp, climbDown: climbDown)
        
        // Boundary checks
        if player.rect.x < 0 { player.rect.x = 0 }
        if player.rect.x > 800 - player.rect.width { player.rect.x = 800 - player.rect.width }
        if player.rect.y < 0 {
            player.rect.y = 0
            player.vy = 0
        }
    }
    
    func checkPlatformCollisions() {
        player.onGround = false
        
        for platform in platforms {
            if player.rect.x < platform.rect.x + platform.rect.width &&
               player.rect.x + player.rect.width > platform.rect.x &&
               player.rect.y < platform.rect.y + platform.rect.height &&
               player.rect.y + player.rect.height > platform.rect.y {
                
                // Collision detected
                if player.vy <= 0 && player.rect.y > platform.rect.y {
                    // Landing on top of platform
                    player.rect.y = platform.rect.y + platform.rect.height
                    player.vy = 0
                    player.onGround = true
                }
            }
        }
    }
    
    func checkLadderCollisions(climbUp: Bool, climbDown: Bool) {
        player.onLadder = false
        
        for ladder in ladders {
            if player.rect.x < ladder.rect.x + ladder.rect.width &&
               player.rect.x + player.rect.width > ladder.rect.x &&
               player.rect.y < ladder.rect.y + ladder.rect.height &&
               player.rect.y + player.rect.height > ladder.rect.y {
                player.onLadder = true
                break
            }
        }
    }
}


