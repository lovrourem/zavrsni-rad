import React from 'react'
import { BrowserRouter as Router, Route, Link, Routes } from 'react-router-dom';
import './App.css'
import Homepage from './Homepage.jsx'
import Settings from './Settings.jsx'

function App() {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<Homepage/>} />
        <Route path="/Settings" element={<Settings/>} />
      </Routes>
    </Router>
  )
}

export default App
