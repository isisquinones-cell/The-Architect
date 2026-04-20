# PenStream ProGuard Rules

# Keep native methods
-keepclassmembers class com.penstream.app.PenStreamService {
    public native void *();
}

# Keep data classes
-keep class com.penstream.app.ServerInfo { *; }
-keep class com.penstream.app.ConnectionManager { *; }

# Timber
-dontwarn timber.log.Timber
-keep class timber.log.Timber { *; }
