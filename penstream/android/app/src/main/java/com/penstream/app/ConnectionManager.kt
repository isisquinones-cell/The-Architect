package com.penstream.app

import android.content.Context
import android.net.wifi.WifiManager
import android.os.Build
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import timber.log.Timber
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress

class ConnectionManager(
    private val context: Context,
    private val discoveryListener: DiscoveryListener? = null
) {
    interface DiscoveryListener {
        fun onServerFound(server: ServerInfo)
        fun onDiscoveryComplete()
    }

    private var discoverySocket: DatagramSocket? = null
    private var isDiscovering = false

    fun startDiscovery() {
        if (isDiscovering) return

        isDiscovering = true

        Thread {
            try {
                discoverySocket = DatagramSocket()
                discoverySocket?.broadcast = true
                discoverySocket?.soTimeout = 3000

                // Send broadcast discovery packet
                val broadcastAddress = getBroadcastAddress()
                val discoveryPacket = buildDiscoveryPacket()

                Timber.d("Sending discovery broadcast to ${broadcastAddress?.hostAddress}:9696")

                for (i in 0..2) { // Send 3 times
                    val packet = DatagramPacket(
                        discoveryPacket,
                        discoveryPacket.size,
                        broadcastAddress,
                        9696
                    )
                    discoverySocket?.send(packet)
                    Thread.sleep(100)
                }

                // Listen for responses
                val buffer = ByteArray(1024)
                val startTime = System.currentTimeMillis()
                val timeout = 3000L

                while (System.currentTimeMillis() - startTime < timeout && isDiscovering) {
                    try {
                        val packet = DatagramPacket(buffer, buffer.size)
                        discoverySocket?.receive(packet)

                        val serverInfo = parseServerResponse(packet.data, packet.address, packet.port)
                        serverInfo?.let {
                            discoveryListener?.onServerFound(it)
                            Timber.i("Found server: ${it.name} at ${it.address}")
                        }
                    } catch (e: Exception) {
                        // Timeout, continue listening
                    }
                }

                discoveryListener?.onDiscoveryComplete()

            } catch (e: Exception) {
                Timber.e(e, "Discovery failed")
                discoveryListener?.onDiscoveryComplete()
            } finally {
                stopDiscovery()
            }
        }.start()
    }

    fun stopDiscovery() {
        isDiscovering = false
        discoverySocket?.close()
        discoverySocket = null
    }

    suspend fun connect(server: ServerInfo): Boolean = withContext(Dispatchers.IO) {
        var socket: DatagramSocket? = null
        return@withContext try {
            socket = DatagramSocket()
            socket.soTimeout = 2000

            // Send connect request
            val connectRequest = buildConnectRequest()
            val requestPacket = DatagramPacket(
                connectRequest,
                connectRequest.size,
                InetAddress.getByName(server.address),
                server.port
            )
            socket.send(requestPacket)

            // Wait for response
            val buffer = ByteArray(1024)
            val responsePacket = DatagramPacket(buffer, buffer.size)
            socket.receive(responsePacket)

            val accepted = parseConnectResponse(responsePacket.data)
            accepted
        } catch (e: Exception) {
            Timber.e(e, "Connection to ${server.address} failed")
            false
        } finally {
            socket?.close()
        }
    }

    private fun getBroadcastAddress(): InetAddress? {
        return try {
            val wifiManager = context.applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
            val wifiInfo = wifiManager.connectionInfo
            val ip = wifiInfo.ipAddress
            val netmask = wifiManager.dhcpInfo.netmask

            val broadcast = (ip.toInt() or (netmask.toInt() xor -1))
            InetAddress.getByAddress(
                byteArrayOf(
                    (broadcast and 0xff).toByte(),
                    ((broadcast shr 8) and 0xff).toByte(),
                    ((broadcast shr 16) and 0xff).toByte(),
                    ((broadcast shr 24) and 0xff).toByte()
                )
            )
        } catch (e: Exception) {
            Timber.e(e, "Failed to get broadcast address")
            InetAddress.getByName("255.255.255.255")
        }
    }

    private fun buildDiscoveryPacket(): ByteArray {
        // MAGIC(2) + VERSION(1) + TYPE(1) + SEQ(4) + TIMESTAMP(4) = 12 bytes
        return byteArrayOf(
            0x50, 0x53, // MAGIC "PS"
            0x01,       // VERSION
            0x03,       // TYPE: HEARTBEAT (used for discovery)
            0x00, 0x00, 0x00, 0x00, // SEQ
            0x00, 0x00, 0x00, 0x00  // TIMESTAMP
        )
    }

    private fun buildConnectRequest(): ByteArray {
        val nameBytes = (Build.MODEL + " - " + Build.DEVICE).take(32).toByteArray()
        val namePadded = ByteArray(32) { i -> nameBytes.getOrElse(i) { 0 } }

        return byteArrayOf(
            0x50.toByte(), 0x53.toByte(), // MAGIC
            0x01.toByte(),                // VERSION
            0x10.toByte(),                // TYPE: CONNECT_REQ
            0x00, 0x00, 0x00, 0x01,      // SEQ
            0x00, 0x00, 0x00, 0x00,      // TIMESTAMP
            *namePadded,                  // Client name (32 bytes)
            0x80.toByte(), 0x07,          // Max width (1920)
            0x38.toByte(), 0x04,          // Max height (1080)
            0x01                          // Supported codecs (bitmask: 1=H264)
        )
    }

    private fun parseServerResponse(data: ByteArray, address: java.net.InetAddress, port: Int): ServerInfo? {
        if (data.size < 12) return null
        if (data[0] != 0x50.toByte() || data[1] != 0x53.toByte()) return null

        val type = data[2].toInt() and 0xFF
        if (type != 0x11) return null // CONNECT_RESP

        val accepted = data[12].toInt() != 0
        if (!accepted) return null

        val width = ((data[13].toInt() and 0xFF) shl 8) or (data[14].toInt() and 0xFF)
        val height = ((data[15].toInt() and 0xFF) shl 8) or (data[16].toInt() and 0xFF)

        return ServerInfo(
            name = "PenStream Server",
            address = address.hostAddress ?: "",
            port = port,
            width = width,
            height = height
        )
    }

    private fun parseConnectResponse(data: ByteArray): Boolean {
        if (data.size < 13) return false
        if (data[0] != 0x50.toByte() || data[1] != 0x53.toByte()) return false

        val type = data[2].toInt() and 0xFF
        if (type != 0x11) return false

        return data[12].toInt() != 0
    }
}
