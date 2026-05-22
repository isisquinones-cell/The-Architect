import { useState, useRef, useCallback, useEffect } from 'react'
import * as Tone from 'tone'
import { midiToFreq } from '../utils/pitchUtils'
import './InstrumentPlayer.css'

// Convert hex color + alpha (0-1) to 8-digit hex string, works in all browsers
function withAlpha(hex, alpha) {
  const a = Math.round(alpha * 255).toString(16).padStart(2, '0')
  return hex + a
}

const INSTRUMENTS = [
  {
    id: 'piano',
    name: 'Piano',
    icon: '🎹',
    color: '#8b5cf6',
    // Synth-based piano — no external CDN dependency
    make: () => new Tone.PolySynth(Tone.Synth, {
      oscillator: { type: 'triangle' },
      envelope: { attack: 0.02, decay: 0.5, sustain: 0.3, release: 1.5 },
    }).toDestination(),
  },
  {
    id: 'strings',
    name: 'Strings',
    icon: '🎻',
    color: '#ec4899',
    // MonoSynth supports filterEnvelope; PolySynth(Synth) does not
    make: () => new Tone.PolySynth(Tone.MonoSynth, {
      oscillator: { type: 'sawtooth' },
      envelope: { attack: 0.3, decay: 0.1, sustain: 0.9, release: 1.5 },
      filterEnvelope: { attack: 0.4, decay: 0.2, sustain: 0.5, release: 1.5, baseFrequency: 400, octaves: 2 },
    }).toDestination(),
  },
  {
    id: 'flute',
    name: 'Flute',
    icon: '🪈',
    color: '#06b6d4',
    make: () => new Tone.PolySynth(Tone.Synth, {
      oscillator: { type: 'sine' },
      envelope: { attack: 0.1, decay: 0.05, sustain: 0.9, release: 0.8 },
    }).toDestination(),
  },
  {
    id: 'guitar',
    name: 'Guitar',
    icon: '🎸',
    color: '#f59e0b',
    make: () => new Tone.PluckSynth({
      attackNoise: 1.5,
      dampening: 3500,
      resonance: 0.98,
    }).toDestination(),
  },
  {
    id: 'marimba',
    name: 'Marimba',
    icon: '🎵',
    color: '#10b981',
    make: () => new Tone.PolySynth(Tone.Synth, {
      oscillator: { type: 'sine' },
      envelope: { attack: 0.001, decay: 0.4, sustain: 0, release: 0.6 },
    }).toDestination(),
  },
  {
    id: 'brass',
    name: 'Brass',
    icon: '🎺',
    color: '#f97316',
    // MonoSynth for filter support
    make: () => new Tone.PolySynth(Tone.MonoSynth, {
      oscillator: { type: 'square' },
      envelope: { attack: 0.15, decay: 0.1, sustain: 0.7, release: 0.5 },
      filterEnvelope: { attack: 0.05, decay: 0.2, sustain: 0.5, release: 0.5, baseFrequency: 300, octaves: 3 },
    }).toDestination(),
  },
  {
    id: 'bells',
    name: 'Bells',
    icon: '🔔',
    color: '#a78bfa',
    make: () => new Tone.PolySynth(Tone.FMSynth, {
      harmonicity: 5.1,
      modulationIndex: 12,
      envelope: { attack: 0.001, decay: 1.2, sustain: 0, release: 1 },
      modulation: { type: 'square' },
      modulationEnvelope: { attack: 0.002, decay: 0.5, sustain: 0, release: 0.5 },
    }).toDestination(),
  },
  {
    id: 'choir',
    name: 'Choir',
    icon: '🎤',
    color: '#f43f5e',
    make: () => new Tone.PolySynth(Tone.Synth, {
      oscillator: { type: 'sine' },
      envelope: { attack: 0.4, decay: 0.1, sustain: 1.0, release: 2 },
    }).toDestination(),
  },
]

export default function InstrumentPlayer({ notes }) {
  const [selectedId, setSelectedId] = useState('piano')
  const [playing, setPlaying] = useState(false)
  const [tempo, setTempo] = useState(100)
  const [volume, setVolume] = useState(-6)
  const [activeNote, setActiveNote] = useState(null)
  const synthRef = useRef(null)
  const partRef = useRef(null)
  const endTimerRef = useRef(null)
  const noteTimersRef = useRef([])

  const selected = INSTRUMENTS.find(i => i.id === selectedId)

  // Clean up on unmount
  useEffect(() => () => {
    clearTimeout(endTimerRef.current)
    noteTimersRef.current.forEach(clearTimeout)
    partRef.current?.stop()
    partRef.current?.dispose()
    synthRef.current?.dispose()
    Tone.getTransport().stop()
    Tone.getTransport().cancel()
  }, [])

  const stopPlayback = useCallback(() => {
    clearTimeout(endTimerRef.current)
    noteTimersRef.current.forEach(clearTimeout)
    noteTimersRef.current = []
    partRef.current?.stop()
    partRef.current?.dispose()
    partRef.current = null
    synthRef.current?.dispose()
    synthRef.current = null
    Tone.getTransport().stop()
    Tone.getTransport().cancel()
    setPlaying(false)
    setActiveNote(null)
  }, [])

  const playMelody = useCallback(async () => {
    if (playing) { stopPlayback(); return }
    if (!notes.length) return

    await Tone.start()
    stopPlayback()

    const instr = INSTRUMENTS.find(i => i.id === selectedId)
    const synth = instr.make()
    synth.volume.value = volume
    synthRef.current = synth

    Tone.getTransport().bpm.value = tempo

    const scaleFactor = 100 / tempo
    const events = notes.map(n => ({
      time: n.start * scaleFactor,
      midi: n.midi,
      duration: Math.max(0.1, n.duration * scaleFactor * 0.9),
    }))

    // Schedule note highlighting using setTimeout — reliable on all platforms
    const timers = []
    const transportStartDelay = 0.05 // small buffer for transport startup (seconds)
    events.forEach(ev => {
      const onMs = (ev.time + transportStartDelay) * 1000
      const offMs = onMs + ev.duration * 1000
      timers.push(
        setTimeout(() => setActiveNote(ev.midi), onMs),
        setTimeout(() => setActiveNote(a => a === ev.midi ? null : a), offMs),
      )
    })
    noteTimersRef.current = timers

    const part = new Tone.Part((time, ev) => {
      const freq = midiToFreq(ev.midi)
      try {
        if (synth instanceof Tone.PluckSynth) {
          synth.triggerAttack(freq, time)
        } else {
          synth.triggerAttackRelease(freq, ev.duration, time)
        }
      } catch (_) {}
    }, events)

    part.start(0)
    partRef.current = part

    const totalMs = (Math.max(...notes.map(n => n.end)) * scaleFactor + 1) * 1000
    endTimerRef.current = setTimeout(() => {
      setPlaying(false)
      setActiveNote(null)
    }, totalMs)

    Tone.getTransport().start()
    setPlaying(true)
  }, [playing, notes, selectedId, tempo, volume, stopPlayback])

  return (
    <div className="instrument-player">
      <div className="player-header">
        <h2>Choose Your Instrument</h2>
        <p className="player-sub">{notes.length} notes ready to play</p>
      </div>

      <div className="instrument-grid">
        {INSTRUMENTS.map(inst => (
          <button
            key={inst.id}
            className={`instrument-card ${selectedId === inst.id ? 'selected' : ''} ${playing && selectedId === inst.id ? 'playing' : ''}`}
            style={{
              '--inst-color': inst.color,
              borderColor: selectedId === inst.id ? inst.color : undefined,
              background: selectedId === inst.id ? withAlpha(inst.color, 0.15) : undefined,
              boxShadow: selectedId === inst.id
                ? `0 4px 20px ${withAlpha(inst.color, 0.3)}`
                : undefined,
            }}
            onClick={() => { stopPlayback(); setSelectedId(inst.id) }}
          >
            <span className="inst-icon">{inst.icon}</span>
            <span className="inst-name">{inst.name}</span>
          </button>
        ))}
      </div>

      <div className="playback-controls">
        <div className="playback-settings">
          <div className="setting">
            <label>Tempo <span className="val">{tempo} BPM</span></label>
            <input type="range" min="40" max="200" value={tempo} onChange={e => setTempo(Number(e.target.value))} />
          </div>
          <div className="setting">
            <label>Volume <span className="val">{volume} dB</span></label>
            <input type="range" min="-30" max="0" value={volume} onChange={e => setVolume(Number(e.target.value))} />
          </div>
        </div>

        <button
          className={`btn-play ${playing ? 'stop' : 'play'}`}
          onClick={playMelody}
          disabled={!notes.length}
        >
          {playing ? (
            <><span className="play-icon">⏹</span> Stop</>
          ) : (
            <><span className="play-icon">▶</span> Play with {selected?.name}</>
          )}
        </button>
      </div>

      {notes.length === 0 && (
        <div className="empty-state">
          <p>No notes detected. Go back and record a melody first.</p>
        </div>
      )}

      <div className="note-display">
        <h3>Melody Notes</h3>
        <div className="note-pills">
          {notes.map((n, i) => (
            <span
              key={i}
              className={`note-pill ${activeNote === n.midi ? 'active' : ''}`}
              style={activeNote === n.midi ? {
                background: withAlpha(selected?.color || '#8b5cf6', 0.3),
                borderColor: selected?.color,
                color: 'white',
              } : undefined}
            >
              {n.label}
            </span>
          ))}
        </div>
      </div>
    </div>
  )
}
