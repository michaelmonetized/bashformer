# Platformer SDL Mobile Ports - Structure Overview

## Directory Structure

```
platformer_sdl/
├── shared/              # Shared C code
│   └── platformer_sdl.c      # Original game code
├── ios/                 # iOS Swift version
│   ├── PlatformerSDL/        # Swift source files
│   │   ├── GameScene.swift
│   │   ├── GameState.swift
│   │   ├── GameViewController.swift
│   │   └── AppDelegate.swift
│   └── README.md
└── android/             # Android native version
    └── app/
        └── src/
            └── main/
                ├── java/com/platformer/platformersdl/
                │   ├── MainActivity.java
                │   ├── PlatformerGameView.java
                │   └── PlatformerGameJNI.java
                ├── jni/
                │   ├── platformer_game_jni.c
                │   ├── Android.mk
                │   └── CMakeLists.txt
                └── AndroidManifest.xml
```

## iOS Version

The iOS version uses **SpriteKit** for rendering and **Swift** for game logic.

### Key Components:
- **GameScene.swift**: Main game scene handling rendering and touch input
- **GameState.swift**: Game logic ported from C (simplified version)
- **GameViewController.swift**: View controller hosting the game
- **AppDelegate.swift**: App entry point

### Touch Controls:
- Left third of screen: Move left
- Right third of screen: Move right  
- Center third: Attack
- Swipe up: Jump
- Vertical swipe: Climb up/down

### Next Steps for iOS:
1. Create Xcode project file (.xcodeproj)
2. Create sprite atlases from PNG files
3. Port full game logic from C (update_game, render_game functions)
4. Add audio support using AVAudioPlayer
5. Create Info.plist with proper settings

## Android Version

The Android version uses **JNI** to call C code and **SurfaceView** for rendering.

### Key Components:
- **MainActivity.java**: Main activity entry point
- **PlatformerGameView.java**: Custom SurfaceView for game rendering and touch handling
- **PlatformerGameJNI.java**: JNI interface definitions
- **platformer_game_jni.c**: JNI wrapper implementation bridging Java and C

### Touch Controls:
- Left third of screen: Move left
- Right third of screen: Move right
- Center third: Attack
- Swipe up: Jump
- Vertical swipe: Climb up/down

### Next Steps for Android:
1. Create full Android Studio project structure
2. Add Gradle build files (build.gradle, settings.gradle)
3. Adapt C code to work with Android Asset Manager (for loading sprites/sounds)
4. Implement OpenGL ES rendering (or Canvas API) to replace SDL rendering
5. Export update_game and render_game functions (remove static or create wrappers)
6. Add audio support using Android AudioTrack or OpenSL ES
7. Create resources (strings.xml, styles.xml, etc.)

## Important Notes

### Code Adaptations Needed:

1. **Remove static functions**: The C code uses `static` functions which aren't accessible from JNI. You'll need to:
   - Remove `static` keyword from `update_game` and `render_game`
   - Or create wrapper functions that JNI can call

2. **Asset Loading**: 
   - iOS: Use Bundle resources
   - Android: Use AAssetManager (shown in JNI code)

3. **Rendering**:
   - iOS: Use SpriteKit (current implementation uses this)
   - Android: Need to implement OpenGL ES or Canvas rendering (not shown in detail)

4. **Audio**:
   - iOS: Use AVAudioPlayer/AVAudioEngine
   - Android: Use AudioTrack or OpenSL ES

## Making it a Git Submodule

To convert this to a git submodule:

```bash
cd platformer_sdl
git init
git add .
git commit -m "Initial mobile ports"
cd ..
git submodule add ./platformer_sdl platformer_sdl
```

Or if you want it as a separate repository:

```bash
# In platformer_sdl directory
git init
git remote add origin <your-repo-url>
git add .
git commit -m "Initial mobile ports"
git push -u origin main

# In parent directory
git submodule add <your-repo-url> platformer_sdl
```


