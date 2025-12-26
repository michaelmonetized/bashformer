package com.vex.vexsdl;

import android.content.res.AssetManager;
import android.graphics.Canvas;

public class VexGameJNI {
    
    static {
        System.loadLibrary("vexgame");
    }
    
    // Native methods
    public native void init(AssetManager assetManager);
    public native void cleanup();
    public native void onSurfaceCreated(int width, int height);
    public native void onSurfaceChanged(int width, int height);
    public native void update(double deltaTime, boolean moveLeft, boolean moveRight, 
                             boolean jump, boolean climbUp, boolean climbDown, boolean attack);
    public native void render(Canvas canvas);
}


