//
//  platformer_game_jni.c
//  JNI wrapper for Platformer SDL game
//

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the shared game code
#include "../../shared/vex_old.c"

#define LOG_TAG "PlatformerGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global game state
static Game *g_game = NULL;
static Sprites *g_sprites = NULL;
static AAssetManager *g_assetManager = NULL;
static int g_width = 800;
static int g_height = 600;

// Android-specific rendering context (simplified - you'd use OpenGL ES in real implementation)
static void *g_renderContext = NULL;

// Helper function to load asset file
static char* load_asset_file(const char* filename) {
    if (!g_assetManager) {
        LOGE("Asset manager not initialized");
        return NULL;
    }
    
    AAsset* asset = AAssetManager_open(g_assetManager, filename, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open asset: %s", filename);
        return NULL;
    }
    
    size_t length = AAsset_getLength(asset);
    char* buffer = (char*)malloc(length + 1);
    AAsset_read(asset, buffer, length);
    AAsset_close(asset);
    buffer[length] = '\0';
    
    return buffer;
}

JNIEXPORT void JNICALL
Java_com_kong_kongsdl_KongGameJNI_init(JNIEnv *env, jobject thiz, jobject assetManager) {
    LOGI("Initializing Platformer Game JNI");
    
    // Get asset manager
    g_assetManager = AAssetManager_fromJava(env, assetManager);
    if (!g_assetManager) {
        LOGE("Failed to get asset manager");
        return;
    }
    
    // Allocate game state
    g_game = (Game*)calloc(1, sizeof(Game));
    if (!g_game) {
        LOGE("Failed to allocate game state");
        return;
    }
    
    // Allocate sprites
    g_sprites = (Sprites*)calloc(1, sizeof(Sprites));
    if (!g_sprites) {
        LOGE("Failed to allocate sprites");
        free(g_game);
        g_game = NULL;
        return;
    }
    
    // Initialize game (you'd need to adapt load_sprites and init_level for Android)
    g_game->level = 1;
    g_game->score = 0;
    g_game->lives = 3;
    g_game->nextLifeScore = 5000;
    g_game->running = 1;
    
    LOGI("Platformer Game JNI initialized");
}

JNIEXPORT void JNICALL
Java_com_kong_kongsdl_KongGameJNI_cleanup(JNIEnv *env, jobject thiz) {
    LOGI("Cleaning up Platformer Game JNI");
    
    if (g_sprites) {
        // Cleanup sprites (you'd need to adapt cleanup functions)
        free(g_sprites);
        g_sprites = NULL;
    }
    
    if (g_game) {
        free(g_game);
        g_game = NULL;
    }
    
    g_assetManager = NULL;
}

JNIEXPORT void JNICALL
Java_com_kong_kongsdl_KongGameJNI_onSurfaceCreated(JNIEnv *env, jobject thiz, jint width, jint height) {
    LOGI("Surface created: %dx%d", width, height);
    g_width = width;
    g_height = height;
    
    // Initialize rendering context (OpenGL ES, etc.)
    // This is simplified - you'd set up OpenGL ES context here
}

JNIEXPORT void JNICALL
Java_com_kong_kongsdl_KongGameJNI_onSurfaceChanged(JNIEnv *env, jobject thiz, jint width, jint height) {
    LOGI("Surface changed: %dx%d", width, height);
    g_width = width;
    g_height = height;
    
    // Update viewport, etc.
}

JNIEXPORT void JNICALL
Java_com_kong_kongsdl_KongGameJNI_update(JNIEnv *env, jobject thiz, 
                                         jdouble deltaTime,
                                         jboolean moveLeft,
                                         jboolean moveRight,
                                         jboolean jump,
                                         jboolean climbUp,
                                         jboolean climbDown,
                                         jboolean attack) {
    if (!g_game) return;
    
    // Update game state
    // Note: You'd need to make update_game function accessible (remove static or create wrapper)
    // For now, this is a placeholder structure
    float dt = (float)deltaTime;
    
    // Update player input
    int moveLeft_int = moveLeft ? 1 : 0;
    int moveRight_int = moveRight ? 1 : 0;
    int jump_int = jump ? 1 : 0;
    int climbUp_int = climbUp ? 1 : 0;
    int climbDown_int = climbDown ? 1 : 0;
    int attack_int = attack ? 1 : 0;
    
    // Call update_game (you'd need to export this or create wrapper)
    // update_game(g_game, dt, moveLeft_int, moveRight_int, jump_int, climbUp_int, climbDown_int, attack_int);
}

JNIEXPORT void JNICALL
Java_com_kong_kongsdl_KongGameJNI_render(JNIEnv *env, jobject thiz, jobject canvas) {
    if (!g_game || !g_sprites) return;
    
    // Render game
    // Note: Canvas rendering would need to be adapted from SDL rendering
    // For a real implementation, you'd use OpenGL ES or Android's Canvas API
    // This is a placeholder structure
    
    // render_game(renderer, g_game, g_sprites); // Would need Android renderer
}


