import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { viteSingleFile } from 'vite-plugin-singlefile'

export default defineConfig({
  base: './',
  plugins: [
    react(),
    viteSingleFile(),
  ],
  define: {
    'process.env.TAMAGUI_TARGET': JSON.stringify('web'),
  },
  resolve: {
    dedupe: ['@tamagui/core', '@tamagui/web', 'tamagui', 'react', 'react-dom'],
    alias: {
      'react-native': 'react-native-web',
      'react-native-svg': '@tamagui/react-native-svg',
    },
  },
  build: {
    outDir: '../docs',
    emptyOutDir: false,
  },
})
