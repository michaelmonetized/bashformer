package com.vex.vexsdl;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;

public class MainActivity extends Activity {
    
    private VexGameView gameView;
    private VexGameJNI gameJNI;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Initialize JNI
        gameJNI = new VexGameJNI();
        gameJNI.init(getAssets());
        
        // Create game view
        gameView = new VexGameView(this, gameJNI);
        gameView.setLayoutParams(new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ));
        
        setContentView(gameView);
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        if (gameView != null) {
            gameView.onResume();
        }
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        if (gameView != null) {
            gameView.onPause();
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (gameJNI != null) {
            gameJNI.cleanup();
        }
    }
}


