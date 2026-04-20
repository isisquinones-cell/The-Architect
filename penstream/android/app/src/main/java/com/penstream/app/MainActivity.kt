package com.penstream.app

import android.content.Intent
import android.net.wifi.WifiManager
import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.penstream.app.databinding.ActivityMainBinding
import kotlinx.coroutines.launch
import timber.log.Timber

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private var connectionManager: ConnectionManager? = null
    private val discoveredServers = mutableListOf<ServerInfo>()
    private var adapter: ArrayAdapter<String>? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        Timber.plant(Timber.DebugTree())
        Timber.d("PenStream starting")

        setupUI()
        startServerDiscovery()
    }

    private fun setupUI() {
        adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, mutableListOf())
        binding.serverListView.adapter = adapter
        binding.serverListView.onItemClickListener = AdapterView.OnItemClickListener { _, _, position, _ ->
            val server = discoveredServers[position]
            connectToServer(server)
        }

        binding.settingsButton.setOnClickListener {
            startActivity(Intent(this, SettingsActivity::class.java))
        }

        binding.refreshButton.setOnClickListener {
            startServerDiscovery()
        }
    }

    private fun startServerDiscovery() {
        binding.statusView.text = "Scanning for servers..."
        discoveredServers.clear()
        adapter?.clear()

        connectionManager?.stopDiscovery()

        connectionManager = ConnectionManager(this, object : ConnectionManager.DiscoveryListener {
            override fun onServerFound(server: ServerInfo) {
                runOnUiThread {
                    if (!discoveredServers.any { it.address == server.address }) {
                        discoveredServers.add(server)
                        adapter?.add("${server.name}\n${server.address}")
                        adapter?.notifyDataSetChanged()
                        binding.statusView.text = "Servers found: ${discoveredServers.size}"
                    }
                }
            }

            override fun onDiscoveryComplete() {
                runOnUiThread {
                    if (discoveredServers.isEmpty()) {
                        binding.statusView.text = "No servers found. Make sure PenStream Server is running on your PC."
                    }
                }
            }
        })

        connectionManager?.startDiscovery()
    }

    private fun connectToServer(server: ServerInfo) {
        binding.statusView.text = "Connecting to ${server.address}..."

        lifecycleScope.launch {
            try {
                val connected = connectionManager?.connect(server)
                if (connected == true) {
                    binding.statusView.text = "Connected!"
                    binding.serverListView.isEnabled = false
                    binding.connectButton.visibility = View.VISIBLE
                    binding.connectButton.setOnClickListener {
                        // Start streaming session
                        startStreaming(server)
                    }
                } else {
                    binding.statusView.text = "Connection failed"
                    Toast.makeText(this@MainActivity, "Failed to connect", Toast.LENGTH_SHORT).show()
                }
            } catch (e: Exception) {
                Timber.e(e, "Connection failed")
                binding.statusView.text = "Connection error: ${e.message}"
            }
        }
    }

    private fun startStreaming(server: ServerInfo) {
        val intent = Intent(this, PenStreamService::class.java)
        intent.putExtra("server_address", server.address)
        intent.putExtra("server_port", server.port)
        startForegroundService(intent)

        // Transition to streaming view
        val streamingIntent = Intent(this, StreamingActivity::class.java)
        streamingIntent.putExtra("server_address", server.address)
        startActivity(streamingIntent)
    }

    override fun onDestroy() {
        connectionManager?.stopDiscovery()
        super.onDestroy()
    }

    override fun onResume() {
        super.onResume()
        binding.serverListView.isEnabled = true
        binding.connectButton.visibility = View.GONE
    }
}
