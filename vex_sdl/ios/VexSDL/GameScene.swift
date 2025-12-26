//
//  GameScene.swift
//  VexSDL
//
//  iOS version of Vex game using SpriteKit
//

import SpriteKit
import GameplayKit

class GameScene: SKScene {
    
    // Game state
    var gameState: GameState!
    var cameraNode: SKCameraNode!
    var playerNode: SKSpriteNode!
    var platforms: [SKSpriteNode] = []
    var ladders: [SKSpriteNode] = []
    var barrels: [SKSpriteNode] = []
    var baddies: [SKSpriteNode] = []
    var coins: [SKSpriteNode] = []
    var powerUps: [SKSpriteNode] = []
    var goalNode: SKSpriteNode!
    
    // Control state
    var moveLeft = false
    var moveRight = false
    var jump = false
    var climbUp = false
    var climbDown = false
    var attack = false
    
    // Touch handling
    var touchStartLocation: CGPoint?
    var leftTouchArea: CGRect?
    var rightTouchArea: CGRect?
    var centerTouchArea: CGRect?
    
    // Sprite atlases
    var heroAtlas: SKTextureAtlas?
    var spritesAtlas: SKTextureAtlas?
    var powerAtlas: SKTextureAtlas?
    var baddiesAtlas: SKTextureAtlas?
    
    override func didMove(to view: SKView) {
        setupCamera()
        setupTouchAreas()
        loadSprites()
        gameState = GameState()
        gameState.initLevel()
        setupScene()
    }
    
    func setupCamera() {
        cameraNode = SKCameraNode()
        addChild(cameraNode)
        camera = cameraNode
    }
    
    func setupTouchAreas() {
        let screenWidth = size.width
        let screenHeight = size.height
        
        // Divide screen into three touch areas
        leftTouchArea = CGRect(x: 0, y: 0, width: screenWidth / 3, height: screenHeight)
        rightTouchArea = CGRect(x: screenWidth / 3, y: 0, width: screenWidth / 3, height: screenHeight)
        centerTouchArea = CGRect(x: screenWidth * 2 / 3, y: 0, width: screenWidth / 3, height: screenHeight)
    }
    
    func loadSprites() {
        // Load texture atlases (you'll need to create these from the PNGs)
        heroAtlas = SKTextureAtlas(named: "Hero")
        spritesAtlas = SKTextureAtlas(named: "Sprites")
        powerAtlas = SKTextureAtlas(named: "Power")
        baddiesAtlas = SKTextureAtlas(named: "Baddies")
    }
    
    func setupScene() {
        // Setup platforms, ladders, player, etc.
        // This will be populated from gameState
        backgroundColor = SKColor(red: 0.1, green: 0.1, blue: 0.2, alpha: 1.0)
        
        // Create player
        if let heroAtlas = heroAtlas {
            let heroTexture = heroAtlas.textureNamed("hero_idle")
            playerNode = SKSpriteNode(texture: heroTexture)
            playerNode.position = CGPoint(x: size.width / 2, y: 100)
            playerNode.zPosition = 10
            addChild(playerNode)
        }
        
        // Setup level from gameState
        setupLevel()
    }
    
    func setupLevel() {
        // Clear existing nodes
        platforms.forEach { $0.removeFromParent() }
        ladders.forEach { $0.removeFromParent() }
        platforms.removeAll()
        ladders.removeAll()
        
        // Create platforms from gameState
        for platform in gameState.platforms {
            let platformNode = SKSpriteNode(color: .brown, size: CGSize(width: CGFloat(platform.width), height: 32))
            platformNode.position = CGPoint(x: CGFloat(platform.x), y: CGFloat(platform.y))
            platformNode.physicsBody = SKPhysicsBody(rectangleOf: platformNode.size)
            platformNode.physicsBody?.isDynamic = false
            platformNode.physicsBody?.categoryBitMask = PhysicsCategory.platform
            addChild(platformNode)
            platforms.append(platformNode)
        }
        
        // Create ladders
        for ladder in gameState.ladders {
            let ladderNode = SKSpriteNode(color: .gray, size: CGSize(width: 32, height: CGFloat(ladder.height)))
            ladderNode.position = CGPoint(x: CGFloat(ladder.x), y: CGFloat(ladder.y))
            ladderNode.zPosition = 1
            addChild(ladderNode)
            ladders.append(ladderNode)
        }
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        let location = touch.location(in: self)
        touchStartLocation = location
        
        // Determine which area was touched
        if let leftArea = leftTouchArea, leftArea.contains(location) {
            moveLeft = true
        } else if let rightArea = rightTouchArea, rightArea.contains(location) {
            moveRight = true
        } else if let centerArea = centerTouchArea, centerArea.contains(location) {
            attack = true
        }
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first, let start = touchStartLocation else { return }
        let location = touch.location(in: self)
        let delta = location.y - start.y
        
        // Swipe up for jump
        if delta < -50 {
            jump = true
        }
        
        // Vertical movement for climbing
        if abs(delta) > 20 {
            if delta > 0 {
                climbUp = true
            } else {
                climbDown = true
            }
        }
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        moveLeft = false
        moveRight = false
        jump = false
        climbUp = false
        climbDown = false
        attack = false
        touchStartLocation = nil
    }
    
    override func update(_ currentTime: TimeInterval) {
        let deltaTime: Float = 1.0 / 60.0
        
        // Update game state
        gameState.update(deltaTime: deltaTime, 
                        moveLeft: moveLeft, 
                        moveRight: moveRight, 
                        jump: jump, 
                        climbUp: climbUp, 
                        climbDown: climbDown, 
                        attack: attack)
        
        // Update sprites based on game state
        updateSprites()
        
        // Update camera to follow player
        if let cameraNode = cameraNode, let playerNode = playerNode {
            cameraNode.position = playerNode.position
        }
    }
    
    func updateSprites() {
        // Update player position and sprite
        if let playerNode = playerNode {
            playerNode.position = CGPoint(x: CGFloat(gameState.player.x), y: CGFloat(gameState.player.y))
            
            // Update animation based on state
            if moveLeft || moveRight {
                // Play walk animation
            } else {
                // Play idle animation
            }
        }
        
        // Update barrels, baddies, coins, etc.
        // This would sync the sprite nodes with gameState entities
    }
}

// Physics categories for collision detection
struct PhysicsCategory {
    static let none: UInt32 = 0
    static let player: UInt32 = 0b1
    static let platform: UInt32 = 0b10
    static let ladder: UInt32 = 0b100
    static let barrel: UInt32 = 0b1000
    static let baddie: UInt32 = 0b10000
    static let coin: UInt32 = 0b100000
    static let powerUp: UInt32 = 0b1000000
}


