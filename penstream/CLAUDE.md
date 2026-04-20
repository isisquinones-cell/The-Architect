# PenStream

Aplicación de streaming de pantalla Windows → Android con input de lápiz, optimizada para baja latencia (<10ms) y alta tasa de FPS (60fps en 1080p).

## Commands

**Server (Windows)**
- `cmake --build . --config Release` — Build server
- `cmake --build . --config Debug` — Build debug
- `./bin/Release/penstream_server.exe` — Ejecutar servidor

**Android**
- `./gradlew assembleDebug` — Build APK debug
- `./gradlew assembleRelease` — Build APK release
- `./gradlew installDebug` — Instalar en dispositivo

## Tech Stack

C++20 (Server Windows) + DirectX 11 + NVENC + Kotlin + Android NDK + OpenGL ES 3.0 + UDP

## Architecture

### Directory Structure
- `server/src/capture/` — DXGI Desktop Duplication
- `server/src/encode/` — NVENC/AMF encoding
- `server/src/network/` — UDP transport + packet builder
- `server/src/input/` — Virtual input handling
- `android/app/src/main/cpp/` — NDK code (decoder, renderer, network, input)
- `android/app/src/main/java/` — Kotlin UI y servicios

### Data Flow
1. DXGI captura framebuffer → zero-copy buffer
2. NVENC encodea H.264 → NAL units
3. UDP envía packets → Client Android
4. MediaCodec decodea → OpenGL render
5. Touch events → UDP → SendInput simula lápiz

### Key Patterns
- Zero-copy frame pooling para evitar allocations
- Bitrate adaptativo según packet loss
- Jitter buffer adaptativo (2-5 frames)
- Heartbeat para detectar desconexión

## Code Organization Rules

1. **RAII para todo.** No raw pointers, usar smart pointers.
2. **Sin excepciones en hot paths.** Usar std::expected o códigos de error.
3. **Logging estructurado.** Usar spdlog con niveles: trace, debug, info, warn, error.
4. **Tests para cada módulo.** Google Test para unit tests.
5. **Documentar packet format.** Cada cambio en protocolo debe actualizar protocol_spec.md.

## Design System

### Colors (Android)
- Primary: #3B82F6
- Background: #0F172A
- Surface: #1E293B
- Text: #F8FAFC
- Success: #22C55E
- Error: #EF4444

### Typography
- Headings: 20sp, 600 weight
- Body: 16sp, 400 weight
- Stats: 14sp monospace

### Style
- Full-screen immersive durante streaming
- Dark mode obligatorio
- Botones mínimo 48dp

## Environment Variables

| Variable | Descripción |
|----------|-------------|
| PENSTREAM_PORT | Puerto UDP (default: 9696) |
| PENSTREAM_BITRATE | Bitrate kbps (default: 10000) |
| PENSTREAM_FPS | FPS target (default: 60) |
| ANDROID_SDK_ROOT | Ruta a Android SDK |
| ANDROID_NDK_ROOT | Ruta a Android NDK |
| NVENC_SDK_PATH | Ruta a NVIDIA Video Codec SDK |

## Reglas No Negociables

1. **Cero GC en hot paths.** Todo en C++ para Android, sin allocations durante streaming.
2. **Latencia <10ms.** Medir en cada commit, si supera 10ms el build falla.
3. **60 fps estables.** Si cae debajo de 55 fps, optimizar antes de continuar.
4. **APK autocontenida.** Sin dependencias externas, debe instalarse con un click.
5. **Conexión automática.** El usuario no debe configurar IPs manualmente.
6. **Soporte para presión de lápiz.** 4096 niveles mínimos, mapeo lineal.
