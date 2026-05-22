import { PitchDetector } from 'pitchy'

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

export function freqToNote(freq) {
  if (!freq || freq < 50) return null
  const semitones = 12 * Math.log2(freq / 440) + 69
  const midi = Math.round(semitones)
  const noteName = NOTE_NAMES[((midi % 12) + 12) % 12]
  const octave = Math.floor(midi / 12) - 1
  const cents = Math.round((semitones - midi) * 100)
  return { midi, name: noteName, octave, label: `${noteName}${octave}`, cents, freq }
}

export function midiToFreq(midi) {
  return 440 * Math.pow(2, (midi - 69) / 12)
}

export function snapToScale(midi, scale = 'chromatic') {
  if (scale === 'chromatic') return midi
  const scaleIntervals = {
    major: [0, 2, 4, 5, 7, 9, 11],
    minor: [0, 2, 3, 5, 7, 8, 10],
    pentatonic: [0, 2, 4, 7, 9],
  }
  const intervals = scaleIntervals[scale] || scaleIntervals.major
  const root = ((midi % 12) + 12) % 12
  const octave = Math.floor(midi / 12)
  let closest = intervals[0]
  let minDist = Math.abs(root - intervals[0])
  for (const interval of intervals) {
    const dist = Math.abs(root - interval)
    if (dist < minDist) { minDist = dist; closest = interval }
    const distWrap = Math.abs(root - (interval + 12))
    if (distWrap < minDist) { minDist = distWrap; closest = interval + 12 }
  }
  return octave * 12 + (closest % 12)
}

export async function analyzeAudio(audioBuffer) {
  const sampleRate = audioBuffer.sampleRate
  const channelData = audioBuffer.getChannelData(0)
  const frameSize = 2048
  const hopSize = 512
  const detector = PitchDetector.forFloat32Array(frameSize)

  // Compute RMS to scale clarity threshold — quieter input needs looser threshold
  let sumSq = 0
  for (let i = 0; i < channelData.length; i++) sumSq += channelData[i] ** 2
  const rms = Math.sqrt(sumSq / channelData.length)
  // Lower threshold for quiet recordings; 0.85 is generous enough for humming
  const clarityThreshold = rms < 0.02 ? 0.80 : 0.85

  const rawNotes = []
  let lastMidi = null
  let noteStart = null

  for (let i = 0; i + frameSize < channelData.length; i += hopSize) {
    const frame = channelData.slice(i, i + frameSize)
    const [freq, clarity] = detector.findPitch(frame, sampleRate)
    const time = i / sampleRate

    if (clarity > clarityThreshold && freq > 60 && freq < 1400) {
      const note = freqToNote(freq)
      if (!note) continue
      // Treat pitches within 1 semitone as the same note (handles pitch wobble)
      const samePitch = lastMidi !== null && Math.abs(note.midi - lastMidi) <= 1
      const effectiveMidi = samePitch ? lastMidi : note.midi
      if (effectiveMidi !== lastMidi) {
        if (lastMidi !== null && noteStart !== null) {
          rawNotes.push({ midi: lastMidi, start: noteStart, end: time, duration: time - noteStart })
        }
        lastMidi = effectiveMidi
        noteStart = time
      }
    } else {
      if (lastMidi !== null && noteStart !== null) {
        rawNotes.push({ midi: lastMidi, start: noteStart, end: time, duration: time - noteStart })
      }
      lastMidi = null
      noteStart = null
    }
  }

  // Merge consecutive notes that are close in pitch (±1 semitone) and time (<150ms gap)
  const merged = []
  for (const note of rawNotes) {
    if (note.duration < 0.07) continue // skip very short blips
    const prev = merged[merged.length - 1]
    if (prev && Math.abs(prev.midi - note.midi) <= 1 && note.start - prev.end < 0.15) {
      // Extend previous note (keep its pitch)
      prev.end = note.end
      prev.duration = prev.end - prev.start
    } else {
      merged.push({ ...note, label: freqToNote(midiToFreq(note.midi))?.label ?? '?' })
    }
  }
  return merged
}
