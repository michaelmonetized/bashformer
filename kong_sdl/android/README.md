# Platformer SDL - Android Version

Android native version of Platformer SDL using C code with JNI wrapper.

## Requirements

- Android Studio Arctic Fox or later
- Android NDK r21 or later
- Minimum SDK: API 21 (Android 5.0)
- Target SDK: API 33 (Android 13)

## Building

1. Open `android/` directory in Android Studio
2. Sync Gradle files
3. Build APK (Build > Build Bundle(s) / APK(s) > Build APK)
4. Install on device or emulator

## Controls

- Touch left side: Move left
- Touch right side: Move right
- Swipe up: Jump
- Tap center: Attack
- Virtual buttons available in UI

## Project Structure

- `app/src/main/java/` - Java/Kotlin code
- `app/src/main/jni/` - Native C code and JNI wrapper
- `app/src/main/assets/` - Game assets (sprites, sounds)


