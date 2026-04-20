package com.penstream.app

data class ServerInfo(
    val name: String,
    val address: String,
    val port: Int = 9696,
    val width: Int = 1920,
    val height: Int = 1080
)
