import { PitchDetector } from 'pitchy'

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

export function freqToNote(freq) {
  if (!freq || freq < 50) return null
  const semitones = 12 * Math.log2(freq / 440) + 69
  const midi = Math.round(semitones)
  const noteName = NOTE_NAMES[midi % 12]
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
  const root = midi % 12
  const octave = Math.floor(midi / 12)
  let closest = intervals[0]
  let minDist = Math.abs(root - intervals[0])
  for (const interval of intervals) {
    const dist = Math.abs(root - interval)
    if (dist < minDist) { minDist = dist; closest = interval }
    // also check wrap-around
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
  const notes = []
  let lastMidi = null
  let noteStart = null

  for (let i = 0; i + frameSize < channelData.length; i += hopSize) {
    const frame = channelData.slice(i, i + frameSize)
    const [freq, clarity] = detector.findPitch(frame, sampleRate)
    const time = i / sampleRate

    if (clarity > 0.9 && freq > 60 && freq < 1200) {
      const note = freqToNote(freq)
      if (!note) continue
      if (note.midi !== lastMidi) {
        if (lastMidi !== null && noteStart !== null) {
          notes.push({ midi: lastMidi, start: noteStart, end: time, duration: time - noteStart, label: freqToNote(midiToFreq(lastMidi))?.label })
        }
        lastMidi = note.midi
        noteStart = time
      }
    } else {
      if (lastMidi !== null && noteStart !== null && time - noteStart > 0.05) {
        notes.push({ midi: lastMidi, start: noteStart, end: time, duration: time - noteStart, label: freqToNote(midiToFreq(lastMidi))?.label })
      }
      lastMidi = null
      noteStart = null
    }
  }

  // merge very short consecutive same-note fragments
  const merged = []
  for (const note of notes) {
    const prev = merged[merged.length - 1]
    if (prev && prev.midi === note.midi && note.start - prev.end < 0.1) {
      prev.end = note.end
      prev.duration = prev.end - prev.start
    } else if (note.duration > 0.08) {
      merged.push({ ...note })
    }
  }
  return merged
}
