package com.penstream.app

import android.content.SharedPreferences
import android.os.Bundle
import android.widget.ArrayAdapter
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.PreferenceManager
import com.penstream.app.databinding.ActivitySettingsBinding
import timber.log.Timber

class SettingsActivity : AppCompatActivity(), SharedPreferences.OnSharedPreferenceChangeListener {

    private lateinit var binding: ActivitySettingsBinding
    private lateinit var prefs: SharedPreferences

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivitySettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefs = PreferenceManager.getDefaultSharedPreferences(this)

        setupUI()
        loadSettings()
    }

    private fun setupUI() {
        // Resolution options
        val resolutionOptions = arrayOf("1920x1080", "1280x720", "854x480")
        binding.resolutionSpinner.adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_item,
            resolutionOptions
        )

        // FPS options
        val fpsOptions = arrayOf("60 FPS", "30 FPS")
        binding.fpsSpinner.adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_item,
            fpsOptions
        )

        // Quality options
        val qualityOptions = arrayOf("High", "Medium", "Low")
        binding.qualitySpinner.adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_item,
            qualityOptions
        )

        binding.saveButton.setOnClickListener {
            saveSettings()
            finish()
        }

        binding.cancelButton.setOnClickListener {
            finish()
        }
    }

    private fun loadSettings() {
        val resolution = prefs.getString("resolution", "1920x1080")
        val fps = prefs.getString("fps", "60")
        val quality = prefs.getString("quality", "High")

        binding.resolutionSpinner.setSelection(
            (binding.resolutionSpinner.adapter as ArrayAdapter<String>).getPosition(resolution ?: "1920x1080")
        )
        binding.fpsSpinner.setSelection(
            (binding.fpsSpinner.adapter as ArrayAdapter<String>).getPosition(fps ?: "60")
        )
        binding.qualitySpinner.setSelection(
            (binding.qualitySpinner.adapter as ArrayAdapter<String>).getPosition(quality ?: "High")
        )
    }

    private fun saveSettings() {
        val resolution = binding.resolutionSpinner.selectedItem.toString()
        val fps = binding.fpsSpinner.selectedItem.toString()
        val quality = binding.qualitySpinner.selectedItem.toString()

        prefs.edit().apply {
            putString("resolution", resolution)
            putString("fps", fps.replace(" FPS", ""))
            putString("quality", quality)
            apply()
        }

        Timber.d("Settings saved: resolution=$resolution, fps=$fps, quality=$quality")
    }

    override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences?, key: String?) {
        // Handle preference changes
    }
}
