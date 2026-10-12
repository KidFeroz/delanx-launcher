import { defaultConfig } from '@tamagui/config/v6'
import { createTamagui } from 'tamagui'

export const config = createTamagui(defaultConfig)

type Conf = typeof config
declare module 'tamagui' {
  interface TamaguiCustomConfig extends Conf {}
}

export default config
