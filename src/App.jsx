import { useState } from 'react'
import Recorder from './components/Recorder'
import PitchEditor from './components/PitchEditor'
import InstrumentPlayer from './components/InstrumentPlayer'
import './App.css'

export default function App() {
  const [audioBuffer, setAudioBuffer] = useState(null)
  const [pitchNotes, setPitchNotes] = useState([])
  const [correctedNotes, setCorrectedNotes] = useState([])
  const [activeTab, setActiveTab] = useState('record')

  function handleRecordingDone(buffer, notes) {
    setAudioBuffer(buffer)
    setPitchNotes(notes)
    setCorrectedNotes(notes)
    setActiveTab('pitch')
  }

  return (
    <div className="app">
      <header className="app-header">
        <div className="logo">
          <span className="logo-icon">🎵</span>
          <h1>MelodyForge</h1>
        </div>
        <p className="tagline">Hum it. Shape it. Play it.</p>
      </header>

      <nav className="tabs">
        <button
          className={activeTab === 'record' ? 'tab active' : 'tab'}
          onClick={() => setActiveTab('record')}
        >
          <span>🎤</span> Record
        </button>
        <button
          className={activeTab === 'pitch' ? 'tab active' : 'tab'}
          onClick={() => setActiveTab('pitch')}
          disabled={!audioBuffer}
        >
          <span>🎼</span> Pitch Editor
        </button>
        <button
          className={activeTab === 'instrument' ? 'tab active' : 'tab'}
          onClick={() => setActiveTab('instrument')}
          disabled={!audioBuffer}
        >
          <span>🎹</span> Instruments
        </button>
      </nav>

      <main className="app-main">
        {activeTab === 'record' && (
          <Recorder onRecordingDone={handleRecordingDone} />
        )}
        {activeTab === 'pitch' && audioBuffer && (
          <PitchEditor
            audioBuffer={audioBuffer}
            notes={pitchNotes}
            correctedNotes={correctedNotes}
            onNotesChange={setCorrectedNotes}
            onContinue={() => setActiveTab('instrument')}
          />
        )}
        {activeTab === 'instrument' && audioBuffer && (
          <InstrumentPlayer
            notes={correctedNotes.length ? correctedNotes : pitchNotes}
          />
        )}
      </main>
    </div>
  )
}
