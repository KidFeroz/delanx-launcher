import React from 'react'
import ReactDOM from 'react-dom/client'
import { TamaguiProvider, Theme } from 'tamagui'
import config from './tamagui.config'
import App from './App'

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <TamaguiProvider config={config} defaultTheme="dark">
      <Theme name="dark">
        <App />
      </Theme>
    </TamaguiProvider>
  </React.StrictMode>
)
