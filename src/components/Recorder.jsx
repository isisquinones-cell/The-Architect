import { useState, useRef, useEffect } from 'react'
import { analyzeAudio } from '../utils/pitchUtils'
import './Recorder.css'

function getSupportedMimeType() {
  if (typeof MediaRecorder === 'undefined') return ''
  const types = [
    'audio/webm;codecs=opus',
    'audio/webm',
    'audio/mp4;codecs=mp4a.40.2',
    'audio/mp4',
    'audio/ogg;codecs=opus',
  ]
  return types.find(t => MediaRecorder.isTypeSupported(t)) || ''
}

export default function Recorder({ onRecordingDone }) {
  const [phase, setPhase] = useState('idle')
  const [duration, setDuration] = useState(0)
  const [error, setError] = useState(null)
  const [wavePoints, setWavePoints] = useState([])
  const mediaRecorderRef = useRef(null)
  const chunksRef = useRef([])
  const timerRef = useRef(null)
  const animFrameRef = useRef(null)
  const streamRef = useRef(null)
  const mimeTypeRef = useRef('')

  useEffect(() => () => {
    clearInterval(timerRef.current)
    cancelAnimationFrame(animFrameRef.current)
    streamRef.current?.getTracks().forEach(t => t.stop())
  }, [])

  function drawWave(analyser) {
    const buf = new Uint8Array(analyser.frequencyBinCount)
    function loop() {
      animFrameRef.current = requestAnimationFrame(loop)
      analyser.getByteTimeDomainData(buf)
      const pts = []
      for (let i = 0; i < buf.length; i += 16) {
        pts.push((buf[i] - 128) / 128)
      }
      setWavePoints(pts)
    }
    loop()
  }

  async function startRecording() {
    setError(null)
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true })
      streamRef.current = stream

      const ctx = new AudioContext()
      const source = ctx.createMediaStreamSource(stream)
      const analyser = ctx.createAnalyser()
      analyser.fftSize = 256
      source.connect(analyser)

      const mimeType = getSupportedMimeType()
      mimeTypeRef.current = mimeType
      const mr = mimeType
        ? new MediaRecorder(stream, { mimeType })
        : new MediaRecorder(stream)
      mediaRecorderRef.current = mr
      chunksRef.current = []
      mr.ondataavailable = e => { if (e.data.size > 0) chunksRef.current.push(e.data) }
      mr.start(100) // collect data every 100ms for reliability on iOS
      setPhase('recording')
      setDuration(0)
      timerRef.current = setInterval(() => setDuration(d => d + 1), 1000)
      drawWave(analyser)
    } catch (e) {
      setError('Microphone access denied. Please allow microphone access and try again.')
    }
  }

  async function stopRecording() {
    clearInterval(timerRef.current)
    cancelAnimationFrame(animFrameRef.current)
    setWavePoints([])

    const mr = mediaRecorderRef.current
    if (!mr) return

    // Create AudioContext HERE — inside the button-click user gesture.
    // On iOS, AudioContext created outside a user gesture stays suspended forever.
    const ctx = new AudioContext()
    if (ctx.state === 'suspended') await ctx.resume()

    setPhase('analyzing')
    mr.onstop = async () => {
      // Stop tracks AFTER onstop fires so we don't lose the last audio chunk
      streamRef.current?.getTracks().forEach(t => t.stop())
      try {
        const usedMime = mr.mimeType || mimeTypeRef.current || 'audio/webm'
        const blob = new Blob(chunksRef.current, { type: usedMime })
        const arrayBuffer = await blob.arrayBuffer()
        const audioBuffer = await ctx.decodeAudioData(arrayBuffer)
        const notes = await analyzeAudio(audioBuffer)
        setPhase(notes.length > 0 ? 'done' : 'empty')
        if (notes.length > 0) onRecordingDone(audioBuffer, notes)
      } catch (e) {
        setError('Could not analyze audio. Please try again in a quieter environment.')
        setPhase('idle')
      }
    }
    mr.stop()
  }

  const fmtTime = s => `${Math.floor(s / 60).toString().padStart(2, '0')}:${(s % 60).toString().padStart(2, '0')}`

  return (
    <div className="recorder">
      <div className="recorder-card">
        <div className="mic-visual">
          <div className={`mic-ring ${phase === 'recording' ? 'pulsing' : ''}`}>
            <div className="mic-icon">🎤</div>
          </div>
        </div>

        {phase === 'idle' && (
          <div className="recorder-instructions">
            <h2>Record Your Melody</h2>
            <p>Press Record and hum or sing your melody. We'll detect the notes automatically.</p>
            <button className="btn-record" onClick={startRecording}>Start Recording</button>
          </div>
        )}

        {phase === 'recording' && (
          <div className="recorder-active">
            <div className="timer">{fmtTime(duration)}</div>
            <div className="waveform">
              {wavePoints.map((v, i) => (
                <div
                  key={i}
                  className="wave-bar"
                  style={{ height: `${Math.max(4, Math.abs(v) * 80)}px` }}
                />
              ))}
            </div>
            <p className="recording-hint">Hum your melody clearly...</p>
            <button className="btn-stop" onClick={stopRecording}>Stop &amp; Analyze</button>
          </div>
        )}

        {phase === 'analyzing' && (
          <div className="analyzing">
            <div className="spinner" />
            <p>Analyzing your melody...</p>
          </div>
        )}

        {phase === 'done' && (
          <div className="done-state">
            <div className="done-icon">✅</div>
            <p>Melody captured! Head to the Pitch Editor tab.</p>
            <button className="btn-record" onClick={() => { setPhase('idle'); setDuration(0) }}>Record Again</button>
          </div>
        )}

        {phase === 'empty' && (
          <div className="done-state">
            <div className="done-icon">🔇</div>
            <p>No notes detected. Try humming louder and closer to your mic, in a quiet room.</p>
            <button className="btn-record" onClick={() => { setPhase('idle'); setDuration(0) }}>Try Again</button>
          </div>
        )}

        {error && <p className="error">{error}</p>}
      </div>

      <div className="tips">
        <h3>Tips for best results</h3>
        <ul>
          <li>Use a quiet room with minimal background noise</li>
          <li>Hum a clear, single-note melody</li>
          <li>Keep each note for at least 0.2 seconds</li>
          <li>Stay close to the microphone</li>
        </ul>
      </div>
    </div>
  )
}
