//
//  AppDelegate.swift
//  PlatformerSDL
//
//  Main app entry point
//

import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    
    var window: UIWindow?
    
    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        window = UIWindow(frame: UIScreen.main.bounds)
        
        let gameViewController = GameViewController()
        window?.rootViewController = gameViewController
        window?.makeKeyAndVisible()
        
        return true
    }
}


