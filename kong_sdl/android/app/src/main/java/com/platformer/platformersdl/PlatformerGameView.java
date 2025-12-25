package com.platformer.platformersdl;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class PlatformerGameView extends SurfaceView implements SurfaceHolder.Callback, Runnable {
    
    private PlatformerGameJNI gameJNI;
    private Thread gameThread;
    private boolean running = false;
    private SurfaceHolder holder;
    
    // Touch control areas
    private float screenWidth;
    private float screenHeight;
    private float leftTouchArea;
    private float rightTouchArea;
    private float centerTouchArea;
    
    // Input state
    private boolean moveLeft = false;
    private boolean moveRight = false;
    private boolean jump = false;
    private boolean climbUp = false;
    private boolean climbDown = false;
    private boolean attack = false;
    
    // Touch tracking
    private float touchStartY = 0;
    private boolean touchActive = false;
    
    public PlatformerGameView(Context context, PlatformerGameJNI gameJNI) {
        super(context);
        this.gameJNI = gameJNI;
        holder = getHolder();
        holder.addCallback(this);
        setFocusable(true);
    }
    
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        screenWidth = getWidth();
        screenHeight = getHeight();
        leftTouchArea = screenWidth / 3.0f;
        rightTouchArea = screenWidth * 2.0f / 3.0f;
        centerTouchArea = screenWidth;
        
        gameJNI.onSurfaceCreated(getWidth(), getHeight());
        running = true;
        gameThread = new Thread(this);
        gameThread.start();
    }
    
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        screenWidth = width;
        screenHeight = height;
        leftTouchArea = screenWidth / 3.0f;
        rightTouchArea = screenWidth * 2.0f / 3.0f;
        centerTouchArea = screenWidth;
        
        gameJNI.onSurfaceChanged(width, height);
    }
    
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        running = false;
        try {
            gameThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    
    @Override
    public void run() {
        long lastTime = System.nanoTime();
        double nsPerFrame = 1000000000.0 / 60.0; // 60 FPS
        
        while (running) {
            long now = System.nanoTime();
            double deltaTime = (now - lastTime) / nsPerFrame;
            lastTime = now;
            
            // Update game
            gameJNI.update(deltaTime, moveLeft, moveRight, jump, climbUp, climbDown, attack);
            
            // Reset jump flag (one-time action)
            jump = false;
            
            // Render
            Canvas canvas = null;
            try {
                canvas = holder.lockCanvas();
                if (canvas != null) {
                    gameJNI.render(canvas);
                }
            } finally {
                if (canvas != null) {
                    holder.unlockCanvasAndPost(canvas);
                }
            }
        }
    }
    
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();
        
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                touchActive = true;
                touchStartY = y;
                
                if (x < leftTouchArea) {
                    moveLeft = true;
                    moveRight = false;
                } else if (x < rightTouchArea) {
                    moveLeft = false;
                    moveRight = true;
                } else {
                    attack = true;
                }
                break;
                
            case MotionEvent.ACTION_MOVE:
                if (touchActive) {
                    float deltaY = touchStartY - y;
                    
                    // Swipe up for jump
                    if (deltaY > 100) {
                        jump = true;
                    }
                    
                    // Vertical movement for climbing
                    if (Math.abs(deltaY) > 20) {
                        if (deltaY > 0) {
                            climbUp = true;
                            climbDown = false;
                        } else {
                            climbDown = true;
                            climbUp = false;
                        }
                    }
                }
                break;
                
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                touchActive = false;
                moveLeft = false;
                moveRight = false;
                climbUp = false;
                climbDown = false;
                attack = false;
                break;
        }
        
        return true;
    }
    
    public void onResume() {
        // Resume game if needed
    }
    
    public void onPause() {
        // Pause game if needed
    }
}


