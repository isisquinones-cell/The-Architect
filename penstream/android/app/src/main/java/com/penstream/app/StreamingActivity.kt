package com.penstream.app

import android.annotation.SuppressLint
import android.os.Bundle
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.penstream.app.databinding.ActivityStreamingBinding
import timber.log.Timber

class StreamingActivity : AppCompatActivity(), SurfaceHolder.Callback {

    private lateinit var binding: ActivityStreamingBinding
    private var surfaceHolder: SurfaceHolder? = null
    private var streamingService: PenStreamService? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityStreamingBinding.inflate(layoutInflater)
        setContentView(binding.root)

        Timber.d("StreamingActivity created")

        setupSurface()
        setupTouchOverlay()
    }

    private fun setupSurface() {
        surfaceHolder = binding.surfaceView.holder
        surfaceHolder?.addCallback(this)
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun setupTouchOverlay() {
        binding.touchOverlay.setOnTouchListener { view, event ->
            handleTouchEvent(event)
            true
        }
    }

    private fun handleTouchEvent(event: MotionEvent): Boolean {
        val pointerCount = event.pointerCount
        val action = event.actionMasked

        for (i in 0 until pointerCount) {
            val x = event.getX(i) / binding.touchOverlay.width
            val y = event.getY(i) / binding.touchOverlay.height
            val pressure = event.getPressure(i)

            when (action) {
                MotionEvent.ACTION_DOWN,
                MotionEvent.ACTION_POINTER_DOWN -> {
                    Timber.d("Touch down: x=$x, y=$y, pressure=$pressure")
                    streamingService?.sendInput(x, y, pressure, 0, 0, 1)
                }
                MotionEvent.ACTION_MOVE -> {
                    Timber.d("Touch move: x=$x, y=$y, pressure=$pressure")
                    streamingService?.sendInput(x, y, pressure, 0, 0, if (pressure > 0.01f) 1 else 0)
                }
                MotionEvent.ACTION_UP,
                MotionEvent.ACTION_POINTER_UP -> {
                    Timber.d("Touch up")
                    streamingService?.sendInput(x, y, 0f, 0, 0, 0)
                }
            }
        }

        return true
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        Timber.d("Surface created")
        streamingService = PenStreamService.getInstance()
        streamingService?.setSurface(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Timber.d("Surface changed: ${width}x${height}")
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Timber.d("Surface destroyed")
        streamingService?.setSurface(null)
    }

    override fun onBackPressed() {
        super.onBackPressed()
        streamingService?.stopStreaming()
        finish()
    }
}
