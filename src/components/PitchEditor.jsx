import { useState, useRef, useCallback } from 'react'
import { snapToScale, freqToNote, midiToFreq } from '../utils/pitchUtils'
import './PitchEditor.css'

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
const MIN_MIDI = 48
const MAX_MIDI = 84
const SCALES = ['chromatic', 'major', 'minor', 'pentatonic']

function noteColor(midi) {
  const sharp = NOTE_NAMES[midi % 12].includes('#')
  return sharp ? '#6d28d9' : '#8b5cf6'
}

export default function PitchEditor({ audioBuffer, notes, correctedNotes, onNotesChange, onContinue }) {
  const [scale, setScale] = useState('chromatic')
  const [pitchShift, setPitchShift] = useState(0)
  const [autoTuneStrength, setAutoTuneStrength] = useState(0)
  const [dragging, setDragging] = useState(null)
  const svgRef = useRef(null)

  const totalDuration = audioBuffer?.duration || 1

  function getX(time) { return (time / totalDuration) * 100 }
  function getY(midi) { return ((MAX_MIDI - midi) / (MAX_MIDI - MIN_MIDI)) * 100 }
  function yToMidi(pct) { return Math.round(MAX_MIDI - pct * (MAX_MIDI - MIN_MIDI)) }

  function handleAutoTune() {
    const tuned = correctedNotes.map(n => {
      const snapped = snapToScale(n.midi, scale)
      const newMidi = Math.round(n.midi + (snapped - n.midi) * autoTuneStrength / 100)
      return { ...n, midi: newMidi, label: freqToNote(midiToFreq(newMidi))?.label }
    })
    onNotesChange(tuned)
  }

  function handleShiftAll(semitones) {
    const shifted = correctedNotes.map(n => {
      const newMidi = Math.max(MIN_MIDI, Math.min(MAX_MIDI, n.midi + semitones))
      return { ...n, midi: newMidi, label: freqToNote(midiToFreq(newMidi))?.label }
    })
    onNotesChange(shifted)
    setPitchShift(s => s + semitones)
  }

  function handleReset() {
    onNotesChange(notes)
    setPitchShift(0)
  }

  // Unified pointer-down: works for mouse and touch via pointer events
  const handlePointerDown = useCallback((e, idx) => {
    e.preventDefault()
    // Capture pointer on the SVG so moves/ups are delivered even if pointer leaves the note
    svgRef.current?.setPointerCapture(e.pointerId)
    setDragging(idx)
  }, [])

  const handlePointerMove = useCallback((e) => {
    if (dragging === null) return
    const svg = svgRef.current
    if (!svg) return
    const rect = svg.getBoundingClientRect()
    const yPct = (e.clientY - rect.top) / rect.height
    const newMidi = Math.max(MIN_MIDI, Math.min(MAX_MIDI, yToMidi(yPct)))
    const updated = correctedNotes.map((n, i) =>
      i === dragging ? { ...n, midi: newMidi, label: freqToNote(midiToFreq(newMidi))?.label } : n
    )
    onNotesChange(updated)
  }, [dragging, correctedNotes, onNotesChange])

  const handlePointerUp = useCallback(() => setDragging(null), [])

  return (
    <div className="pitch-editor">
      <div className="pitch-header">
        <h2>Pitch Editor</h2>
        <p className="pitch-sub">{correctedNotes.length} notes detected &mdash; drag notes to adjust pitch</p>
      </div>

      <div className="piano-roll-wrapper">
        <div className="piano-keys">
          {Array.from({ length: MAX_MIDI - MIN_MIDI + 1 }, (_, i) => {
            const midi = MAX_MIDI - i
            const name = NOTE_NAMES[midi % 12]
            const isSharp = name.includes('#')
            return (
              <div key={midi} className={`piano-key ${isSharp ? 'sharp' : 'natural'}`}>
                {!isSharp && name === 'C' && <span className="key-label">{name}{Math.floor(midi / 12) - 1}</span>}
              </div>
            )
          })}
        </div>

        <svg
          ref={svgRef}
          className="piano-roll"
          viewBox="0 0 100 100"
          preserveAspectRatio="none"
          style={{ touchAction: 'none' }}
          onPointerMove={handlePointerMove}
          onPointerUp={handlePointerUp}
          onPointerLeave={handlePointerUp}
        >
          {/* grid lines */}
          {Array.from({ length: MAX_MIDI - MIN_MIDI + 1 }, (_, i) => {
            const midi = MAX_MIDI - i
            const y = getY(midi)
            const isSharp = NOTE_NAMES[midi % 12].includes('#')
            return (
              <line
                key={midi}
                x1="0" y1={y} x2="100" y2={y}
                stroke={isSharp ? 'rgba(255,255,255,0.03)' : 'rgba(255,255,255,0.07)'}
                strokeWidth="0.3"
              />
            )
          })}

          {/* original notes (ghost) */}
          {notes.map((n, i) => (
            <rect
              key={`orig-${i}`}
              x={getX(n.start)}
              y={getY(n.midi) - 1}
              width={Math.max(0.5, getX(n.end) - getX(n.start))}
              height={2.2}
              fill="rgba(255,255,255,0.08)"
              rx="0.5"
            />
          ))}

          {/* corrected notes — pointer events for mouse + touch drag */}
          {correctedNotes.map((n, i) => (
            <g key={`note-${i}`}>
              <rect
                x={getX(n.start)}
                y={getY(n.midi) - 1.2}
                width={Math.max(0.5, getX(n.end) - getX(n.start))}
                height={2.5}
                fill={noteColor(n.midi)}
                rx="0.5"
                style={{ cursor: 'ns-resize' }}
                onPointerDown={e => handlePointerDown(e, i)}
              />
              <text
                x={getX(n.start) + 0.3}
                y={getY(n.midi) + 0.5}
                fontSize="1.8"
                fill="white"
                style={{ pointerEvents: 'none', userSelect: 'none' }}
              >
                {n.label}
              </text>
            </g>
          ))}
        </svg>
      </div>

      <div className="pitch-controls">
        <div className="control-group">
          <h3>Auto-Tune</h3>
          <div className="control-row">
            <label>Scale</label>
            <div className="scale-buttons">
              {SCALES.map(s => (
                <button
                  key={s}
                  className={scale === s ? 'scale-btn active' : 'scale-btn'}
                  onClick={() => setScale(s)}
                >
                  {s.charAt(0).toUpperCase() + s.slice(1)}
                </button>
              ))}
            </div>
          </div>
          <div className="control-row">
            <label>Correction Strength <span className="val">{autoTuneStrength}%</span></label>
            <input
              type="range" min="0" max="100" value={autoTuneStrength}
              onChange={e => setAutoTuneStrength(Number(e.target.value))}
            />
          </div>
          <button className="btn-autotune" onClick={handleAutoTune}>
            Apply Auto-Tune
          </button>
        </div>

        <div className="control-group">
          <h3>Pitch Shift</h3>
          <div className="shift-row">
            <button className="shift-btn" onClick={() => handleShiftAll(-12)}>-Oct</button>
            <button className="shift-btn" onClick={() => handleShiftAll(-1)}>-1</button>
            <span className="shift-val">{pitchShift >= 0 ? '+' : ''}{pitchShift} st</span>
            <button className="shift-btn" onClick={() => handleShiftAll(1)}>+1</button>
            <button className="shift-btn" onClick={() => handleShiftAll(12)}>+Oct</button>
          </div>
          <button className="btn-reset" onClick={handleReset}>Reset to Original</button>
        </div>
      </div>

      <div className="pitch-footer">
        <p className="pitch-note">Ghost notes show original pitch. Drag colored notes to fine-tune.</p>
        <button className="btn-continue" onClick={onContinue}>Choose Instruments →</button>
      </div>
    </div>
  )
}
