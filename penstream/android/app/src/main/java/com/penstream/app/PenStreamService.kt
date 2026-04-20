package com.penstream.app

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.graphics.SurfaceTexture
import android.os.Binder
import android.os.Build
import android.os.IBinder
import android.view.Surface
import androidx.core.app.NotificationCompat
import timber.log.Timber

class PenStreamService : Service() {

    companion object {
        private const val CHANNEL_ID = "penstream_channel"
        private const val NOTIFICATION_ID = 1001
        private var instance: PenStreamService? = null

        fun getInstance(): PenStreamService? = instance
    }

    private val binder = LocalBinder()
    private var serverAddress: String = ""
    private var serverPort: Int = 9696
    private var surface: Surface? = null

    // Native methods (NDK)
    external fun nativeInit()
    external fun nativeStartStreaming(address: String, port: Int)
    external fun nativeStopStreaming()
    external fun nativeSendInput(x: Float, y: Float, pressure: Float, tiltX: Int, tiltY: Int, buttons: Int)
    external fun nativeSetSurface(surface: Surface?)

    inner class LocalBinder : Binder() {
        fun getService(): PenStreamService = this@PenStreamService
    }

    override fun onCreate() {
        super.onCreate()
        instance = this
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        serverAddress = intent?.getStringExtra("server_address") ?: ""
        serverPort = intent?.getIntExtra("server_port", 9696) ?: 9696

        val notification = createNotification()
        startForeground(NOTIFICATION_ID, notification)

        Timber.i("Starting streaming to $serverAddress:$serverPort")

        // Initialize native code
        try {
            System.loadLibrary("penstream")
            nativeInit()
            nativeStartStreaming(serverAddress, serverPort)
        } catch (e: UnsatisfiedLinkError) {
            Timber.e(e, "Failed to load native library")
        }

        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder {
        return binder
    }

    override fun onDestroy() {
        nativeStopStreaming()
        instance = null
        super.onDestroy()
    }

    fun setSurface(surface: Surface?) {
        this.surface = surface
        nativeSetSurface(surface)
    }

    fun sendInput(x: Float, y: Float, pressure: Float, tiltX: Int, tiltY: Int, buttons: Int) {
        nativeSendInput(x, y, pressure, tiltX, tiltY, buttons)
    }

    fun stopStreaming() {
        nativeStopStreaming()
        stopForeground(STOP_FOREGROUND_REMOVE)
        stopSelf()
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "PenStream Streaming",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Active PenStream streaming session"
            }

            val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(channel)
        }
    }

    private fun createNotification(): Notification {
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("PenStream")
            .setContentText("Streaming to $serverAddress")
            .setSmallIcon(android.R.drawable.ic_menu_send)
            .setOngoing(true)
            .build()
    }
}
