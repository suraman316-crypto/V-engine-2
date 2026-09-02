package com.vengine.app

import android.app.NativeActivity
import android.content.Context
import android.os.Bundle
import android.view.Choreographer
import android.view.SurfaceHolder
import android.view.SurfaceView
import java.io.File

/**
 * V Engine host activity.
 *
 * Drives the native engine from the display Choreographer: one
 * [nativeTick] call per vsync with the frame delta. All rendering and
 * simulation runs in C++; this class only owns the window surface and
 * forwards lifecycle/input events across JNI.
 */
class GameActivity : NativeActivity() {

    private external fun nativeOnCreate(assetMgr: android.content.res.AssetManager, filesDir: String)
    private external fun nativeOnSurfaceCreated(surface: android.view.Surface)
    private external fun nativeOnResume()
    private external fun nativeOnPause()
    private external fun nativeTick(dt: Float)
    private external fun nativeOnDestroy()

    private lateinit var surfaceView: SurfaceView
    private var lastFrameNs: Long = 0L
    private val callback = object : Choreographer.FrameCallback {
        override fun doFrame(frameTimeNanos: Long) {
            val dt = if (lastFrameNs == 0L) 1f / 60f
                     else (frameTimeNanos - lastFrameNs) / 1_000_000_000f
            lastFrameNs = frameTimeNanos
            if (dt in 0f..1f) nativeTick(dt)
            Choreographer.getInstance().postFrameCallback(this)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        System.loadLibrary("vengine")
        nativeOnCreate(assets, filesDir.absolutePath)
        surfaceView = SurfaceView(this)
        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(h: SurfaceHolder) =
                nativeOnSurfaceCreated(h.surface)
            override fun surfaceChanged(h: SurfaceHolder, w: Int, hgt: Int) {}
            override fun surfaceDestroyed(h: SurfaceHolder) {}
        })
        setContentView(surfaceView)
    }

    override fun onResume() {
        super.onResume()
        nativeOnResume()
        lastFrameNs = 0L
        Choreographer.getInstance().postFrameCallback(callback)
    }

    override fun onPause() {
        super.onPause()
        Choreographer.getInstance().removeFrameCallback(callback)
        nativeOnPause()
    }

    override fun onDestroy() {
        nativeOnDestroy()
        super.onDestroy()
    }
}
