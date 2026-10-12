import React, { useState, useEffect } from 'react'
import {
  YStack,
  XStack,
  Text,
  Paragraph,
  H1,
  H2,
  H3,
  H4,
  Button,
  Card,
  Separator,
  ScrollView,
} from 'tamagui'
import {
  Download,
  ExternalLink,
  Copy,
  Check,
  Globe,
  Terminal,
  ShieldCheck,
  Cpu,
} from 'lucide-react'

type Language = 'en' | 'es'

export default function App() {
  const [lang, setLang] = useState<Language>('en')
  const [copied, setCopied] = useState(false)
  const [currentTab, setCurrentTab] = useState('decomps')

  useEffect(() => {
    const stored = localStorage.getItem('delanx_lang') as Language | null
    if (stored === 'es' || stored === 'en') {
      setLang(stored)
      return
    }
    const browserLang = (navigator.language || (navigator.languages && navigator.languages[0]) || '').toLowerCase()
    if (browserLang.startsWith('es')) {
      setLang('es')
    } else {
      setLang('en')
    }
  }, [])

  const switchLang = (newLang: Language) => {
    setLang(newLang)
    localStorage.setItem('delanx_lang', newLang)
  }

  const copyPath = () => {
    navigator.clipboard.writeText('sdmc:/switch/Delanx/Delanx.nro')
    setCopied(true)
    setTimeout(() => setCopied(false), 2500)
  }

  const t = {
    en: {
      subheadingMeta: 'Horizon OS Homebrew / devkitA64 & libnx / GPLv3',
      heroTitle: 'Clean port management and direct execution on Nintendo Switch',
      heroDesc: 'A native C++20 launcher built to eliminate common homebrew friction: validates asset integrity (.otr, .mpq, .wad), audits available Horizon OS memory heap, and transfers execution control seamlessly.',
      downloadBtn: 'Download Binary (.nro)',
      copiedText: 'Path copied!',
      copyPathBtn: 'sdmc:/switch/Delanx/Delanx.nro',
      specs: [
        { label: 'C++20 Native', sub: 'Zero JS or webview overhead' },
        { label: 'Hardware Blit', sub: 'SDL2 locked at 60 FPS' },
        { label: 'Pre-Flight Check', sub: 'SHA-256 crash prevention' },
        { label: 'Clean Hand-off', sub: 'libnx envSetNextLoad()' },
      ],
      problemHeaderTag: 'Console Architecture Diagnostics',
      problemTitle: 'Why do so many ports crash on launch?',
      problemBadge: 'Atmosphere Kernel Panic Screen',
      p1Title: '1. Missing proprietary assets',
      p1Desc: 'Ports cannot legally distribute copyrighted game data. If a user forgets to place the expected data file in the exact path, the NRO terminates with an immediate panic screen.',
      p2Title: '2. Heap exhaustion in Applet Mode',
      p2Desc: 'Launching hbmenu via the Album limits heap to ~450 MB. Modern decompilations and ports requiring texture cache crash instantly without Title Override enabled.',
      p3Title: 'The Delanx Solution',
      p3Desc: 'Delanx validates file presence and cryptographic hash before launching, verifies current Horizon memory mode, and warns you directly on screen before dispatching.',
      explorerTag: 'Interactive MicroSD Inspector',
      explorerTitle: 'Architecture and Data Format Explorer',
      explorerDesc: 'Select an architecture below to inspect its exact filesystem layout and pre-flight validation strategy.',
      tabs: {
        decomps: 'Decomps (.otr / .o2r)',
        pcclassic: 'PC Ports (.mpq / .bin)',
        engines: 'Retro Engines (.wad / .pk3)',
        schema: 'apps.json Schema',
      },
      memModeLabel: 'Memory Mode:',
      integrityLabel: 'Integrity Check:',
      moddingTip: 'Modding tip: You can tweak launch arguments or directory paths without recompiling simply by editing apps.json.',
      lifecycleTag: 'Under the Bonnet',
      lifecycleTitle: 'Execution Lifecycle with libnx',
      lifecycleDesc: 'How Delanx coordinates an atomic hand-off without remaining resident in background memory.',
      steps: [
        { num: '01', tag: 'NACP Parse', title: 'Title Detection', desc: 'Reads ROM headers, extracts NACP data, and binds local cached artwork.' },
        { num: '02', tag: 'Pre-Flight', title: 'Integrity Audit', desc: 'Validates required data files and verifies SHA-256 checksums.' },
        { num: '03', tag: 'Teardown', title: 'Total Memory Cleanup', desc: 'Destroys SDL2 texture context, closes file descriptors, and zeroes heap.' },
        { num: '04', tag: 'Hand-off', title: 'envSetNextLoad()', desc: 'Hands off execution atomically to Horizon OS with target path and arguments.' },
      ],
      installTag: 'Step by Step',
      installTitle: 'Installing on your MicroSD',
      i1Title: '1. Download the executable',
      i1Desc: 'Grab the latest Delanx.nro release from GitHub.',
      i2Title: '2. Copy to sdmc:/switch/',
      i2Desc: 'Place the binary at sdmc:/switch/Delanx/Delanx.nro alongside your cache folder.',
      i3Title: '3. Launch via Title Override',
      i3Desc: 'Hold R while launching any official installed game to give Delanx full 3.2 GB RAM access.',
      aboutTag: 'Open Development',
      aboutTitle: 'Vibecoding, Homebrew & Community',
      aboutDesc: 'Delanx is an open-source tool developed with AI assistance (vibecoding) by KidFeroz, compiled using official devkitA64 toolchain and verified on actual Nintendo Switch hardware. Released under GNU GPLv3.',
      badges: ['100% Free Software (GPLv3)', 'Tested on Real Hardware', 'Community PRs Welcome'],
      footerCopyright: 'Delanx / Created by KidFeroz / GNU General Public License v3.0',
    },
    es: {
      subheadingMeta: 'Horizon OS Homebrew / devkitA64 y libnx / GPLv3',
      heroTitle: 'Gestión y ejecución limpia de ports en Nintendo Switch',
      heroDesc: 'Un launcher nativo en C++20 diseñado para resolver la fricción común del homebrew: valida la integridad de assets (.otr, .mpq, .wad), audita el heap disponible de memoria de Horizon OS y transfiere el control sin intermediarios.',
      downloadBtn: 'Descargar Binario (.nro)',
      copiedText: '¡Ruta copiada!',
      copyPathBtn: 'sdmc:/switch/Delanx/Delanx.nro',
      specs: [
        { label: 'C++20 Nativo', sub: 'Cero overhead de JS o webview' },
        { label: 'Hardware Blit', sub: 'SDL2 a 60 FPS estables' },
        { label: 'Pre-Flight Check', sub: 'SHA-256 previene crashes' },
        { label: 'Hand-off Limpio', sub: 'libnx envSetNextLoad()' },
      ],
      problemHeaderTag: 'Diagnóstico de Arquitectura en Consola',
      problemTitle: '¿Por qué crashean tantos ports al ejecutarse?',
      problemBadge: 'Pantalla Panic / Atmosphere',
      p1Title: '1. Assets propietarios ausentes',
      p1Desc: 'Los ports no distribuyen datos con copyright. Si el usuario olvida colocar el archivo en la ruta exacta, el NRO aborta inmediatamente con un panic screen.',
      p2Title: '2. Agotamiento de heap en Modo Applet',
      p2Desc: 'Iniciar el hbmenu desde el Álbum limita la memoria a ~450 MB. Las decompilaciones y ports modernos crashean al instante si no se usa Title Override.',
      p3Title: 'La Solución de Delanx',
      p3Desc: 'Delanx comprueba la presencia y el hash criptográfico antes de lanzar, audita el modo de memoria activo de Horizon y te avisa en pantalla antes de ejecutar.',
      explorerTag: 'Inspector Interactivo en MicroSD',
      explorerTitle: 'Explorador de Arquitecturas y Formatos',
      explorerDesc: 'Selecciona una arquitectura para ver la estructura exacta de carpetas en tu tarjeta y su estrategia de validación.',
      tabs: {
        decomps: 'Decompilaciones (.otr / .o2r)',
        pcclassic: 'Puertos PC (.mpq / .bin)',
        engines: 'Motores Retro (.wad / .pk3)',
        schema: 'Esquema apps.json',
      },
      memModeLabel: 'Modo de Memoria:',
      integrityLabel: 'Chequeo de Integridad:',
      moddingTip: 'Consejo de modding: Puedes cambiar argumentos de inicio o rutas sin recompilar simplemente editando apps.json.',
      lifecycleTag: 'Bajo el Capó',
      lifecycleTitle: 'Ciclo de Vida con libnx',
      lifecycleDesc: 'Cómo Delanx coordina el traspaso limpio de memoria sin quedar residente en segundo plano.',
      steps: [
        { num: '01', tag: 'Parseo NACP', title: 'Detección de Título', desc: 'Lee cabeceras NRO, extrae datos NACP y enlaza miniaturas locales.' },
        { num: '02', tag: 'Pre-Flight', title: 'Auditoría de Integridad', desc: 'Comprueba archivos requeridos y valida sumas criptográficas SHA-256.' },
        { num: '03', tag: 'Limpieza', title: 'Liberación de Memoria', desc: 'Destruye contexto SDL2, cierra descriptores de archivo y deja el heap en cero.' },
        { num: '04', tag: 'Traspaso', title: 'envSetNextLoad()', desc: 'Transfiere la ejecución atómicamente a Horizon OS con ruta y argumentos.' },
      ],
      installTag: 'Paso a Paso',
      installTitle: 'Instalación en la MicroSD',
      i1Title: '1. Descarga el ejecutable',
      i1Desc: 'Obtén la compilación oficial de Delanx.nro desde Releases en GitHub.',
      i2Title: '2. Copia a sdmc:/switch/',
      i2Desc: 'Ubica el archivo en sdmc:/switch/Delanx/Delanx.nro junto con tu carpeta de caché.',
      i3Title: '3. Ejecuta con Title Override',
      i3Desc: 'Mantén presionado R al abrir cualquier juego instalado para tener los 3.2 GB completos de RAM.',
      aboutTag: 'Desarrollo Abierto',
      aboutTitle: 'Vibecoding, Homebrew y Comunidad',
      aboutDesc: 'Delanx es una herramienta de código abierto desarrollada con asistencia de IA (vibecoding) por KidFeroz, compilada con la toolchain oficial devkitA64 y probada en hardware real de Nintendo Switch. Licencia GNU GPLv3.',
      badges: ['100% Software Libre (GPLv3)', 'Probado en Hardware Real', 'PRs y Auditorías Bienvenidos'],
      footerCopyright: 'Delanx / Creado por KidFeroz / GNU General Public License v3.0',
    },
  }[lang]

  const archDetails: Record<string, {
    title: string
    badge: string
    desc: string
    mem: string
    check: string
    tree: string
  }> = {
    decomps: {
      title: lang === 'en' ? 'Modern Decompilations' : 'Decompilaciones Modernas',
      badge: lang === 'en' ? 'Asset Packs (.otr / .o2r)' : 'Paquetes de Assets (.otr / .o2r)',
      desc: lang === 'en'
        ? 'Modern decompilation projects generate custom archive containers (.otr / .o2r) extracted from legitimate ROMs. Delanx inspects this container before launching.'
        : 'Proyectos modernos de decompilación generan contenedores propios (.otr / .o2r) a partir de una ROM legal. Delanx comprueba la presencia de este paquete antes de invocar el ejecutable.',
      mem: lang === 'en' ? 'Title Override (Required)' : 'Title Override (Requerido)',
      check: 'SHA-256 pre-flight',
      tree: `sdmc:/
└── switch/
    ├── Delanx/
    │   ├── Delanx.nro          # Launcher binary
    │   ├── apps.json           # Registry & paths
    │   └── cache/              # Artwork (.png)
    └── sm64/
        ├── sm64.nro            # Port binary
        └── assets.otr          # Verified assets (SHA-256 OK)`
    },
    pcclassic: {
      title: lang === 'en' ? 'Classic PC Ports' : 'Puertos Clásicos de PC',
      badge: lang === 'en' ? 'Installation Archives (.mpq / .bin)' : 'Archivos de Instalación (.mpq / .bin)',
      desc: lang === 'en'
        ? 'Ports such as DevilutionX (Diablo 1) or OpenLara require original game data packages (such as DIABDAT.MPQ). Delanx verifies file paths before calling.'
        : 'Puertos nativos como DevilutionX (Diablo 1) u OpenLara requieren datos de instalación original (como DIABDAT.MPQ). Delanx verifica las rutas antes de la llamada.',
      mem: lang === 'en' ? 'Applet or Title Override' : 'Applet o Title Override',
      check: 'File presence + SHA-256',
      tree: `sdmc:/
└── switch/
    ├── Delanx/
    │   └── Delanx.nro
    └── devilutionx/
        ├── devilutionx.nro     # Executable
        └── diabdat.mpq         # Verified data archive`
    },
    engines: {
      title: lang === 'en' ? 'Retro Engines (Build / id Tech)' : 'Motores Retro (Build / id Tech)',
      badge: lang === 'en' ? 'Game Packages (.wad / .pk3 / .grp)' : 'Paquetes de Juego (.wad / .pk3 / .grp)',
      desc: lang === 'en'
        ? 'Engines requiring IWADs or PK3 packages check for game files in their subfolder. Delanx validates that the files are present and builds argv.'
        : 'Motores que requieren IWADs o paquetes PK3 comprueban que los archivos existan en su carpeta. Delanx valida su presencia y prepara argv.',
      mem: lang === 'en' ? 'Title Override' : 'Title Override',
      check: 'Recognised IWAD hash',
      tree: `sdmc:/
└── switch/
    ├── Delanx/
    │   └── Delanx.nro
    └── nxquake/
        ├── quake.nro           # Executable
        └── id1/
            └── pak0.pak        # Verified game data`
    },
    schema: {
      title: lang === 'en' ? 'Decoupled apps.json Schema' : 'Esquema Desacoplado apps.json',
      badge: lang === 'en' ? 'Open Extensible JSON' : 'JSON Abierto y Extensible',
      desc: lang === 'en'
        ? 'Register custom ports, declare relative paths on the SD card, and define mandatory files to audit before launching without touching C++ code.'
        : 'Registra puertos personalizados, declara rutas relativas en la MicroSD y define archivos obligatorios para auditar antes de lanzar sin tocar C++.',
      mem: lang === 'en' ? 'Configurable' : 'Configurable',
      check: 'Schema validated',
      tree: `{
  "name": "Custom Port",
  "folderName": "custom_port",
  "switch": {
    "targetDir": "/switch/custom_port",
    "nro": "port.nro",
    "requiredUserFiles": [
      { "path": "sdmc:/switch/custom_port/assets.otr", "present": true }
    ]
  }
}`
    }
  }

  const activeArch = archDetails[currentTab] || archDetails.decomps

  return (
    <YStack backgroundColor="#07090E" minHeight="100vh" color="#E2E8F0">
      
      {/* NAVBAR */}
      <XStack
        tag="header"
        height={64}
        paddingHorizontal="$4"
        alignItems="center"
        justifyContent="space-between"
        borderBottomWidth={1}
        borderBottomColor="rgba(255,255,255,0.08)"
        backgroundColor="rgba(7,9,14,0.92)"
        style={{ backdropFilter: 'blur(16px)', position: 'sticky', top: 0, zIndex: 50 }}
      >
        <XStack alignItems="center" gap="$3">
          <img
            src="./assets/delanx-iso-dark.svg"
            alt="Delanx"
            style={{ width: 30, height: 30, objectFit: 'contain' }}
          />
          <Text
            fontFamily="'Cal Sans', sans-serif"
            fontSize="$6"
            fontWeight="800"
            color="white"
            letterSpacing={1}
          >
            Delanx
          </Text>
          <XStack
            display="none"
            $gtSm={{ display: 'flex' }}
            backgroundColor="rgba(0,245,255,0.06)"
            borderColor="rgba(0,245,255,0.2)"
            borderWidth={1}
            borderRadius="$2"
            paddingHorizontal="$2"
            paddingVertical="$1"
          >
            <Text fontFamily="'DM Mono', monospace" fontSize="$1" color="#00F5FF">
              Horizon OS
            </Text>
          </XStack>
        </XStack>

        <XStack alignItems="center" gap="$3">
          {/* Language Switcher: Clean Segmented Controls without pills or dots */}
          <XStack
            backgroundColor="#0E131F"
            borderColor="rgba(255,255,255,0.12)"
            borderWidth={1}
            borderRadius="$2"
            padding="$1"
            gap="$1"
          >
            <Button
              size="$2"
              height={26}
              paddingHorizontal="$2"
              backgroundColor={lang === 'en' ? 'rgba(0,245,255,0.15)' : 'transparent'}
              borderColor={lang === 'en' ? 'rgba(0,245,255,0.4)' : 'transparent'}
              borderWidth={1}
              borderRadius="$1"
              onPress={() => switchLang('en')}
            >
              <Text
                fontFamily="'DM Mono', monospace"
                fontSize="$1"
                fontWeight="700"
                color={lang === 'en' ? '#00F5FF' : '#94A3B8'}
              >
                EN-GB
              </Text>
            </Button>

            <Button
              size="$2"
              height={26}
              paddingHorizontal="$2"
              backgroundColor={lang === 'es' ? 'rgba(0,245,255,0.15)' : 'transparent'}
              borderColor={lang === 'es' ? 'rgba(0,245,255,0.4)' : 'transparent'}
              borderWidth={1}
              borderRadius="$1"
              onPress={() => switchLang('es')}
            >
              <Text
                fontFamily="'DM Mono', monospace"
                fontSize="$1"
                fontWeight="700"
                color={lang === 'es' ? '#00F5FF' : '#94A3B8'}
              >
                ES
              </Text>
            </Button>
          </XStack>

          <Button
            tag="a"
            href="https://github.com/KidFeroz/Delanx"
            target="_blank"
            size="$3"
            height={34}
            backgroundColor="rgba(255,255,255,0.05)"
            borderColor="rgba(255,255,255,0.1)"
            borderWidth={1}
            borderRadius="$2"
            icon={<ExternalLink size={14} color="#E2E8F0" />}
          >
            <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#E2E8F0">
              GitHub
            </Text>
          </Button>

          <Button
            tag="a"
            href="https://github.com/KidFeroz/Delanx/releases"
            target="_blank"
            size="$3"
            height={34}
            backgroundColor="#00F5FF"
            color="#07090E"
            borderRadius="$2"
            fontWeight="700"
            icon={<Download size={14} color="#07090E" />}
          >
            <Text fontFamily="'Cal Sans', sans-serif" fontSize="$3" color="#07090E" fontWeight="700">
              v0.1.0 .NRO
            </Text>
          </Button>
        </XStack>
      </XStack>

      {/* HERO SECTION */}
      <YStack
        alignItems="center"
        justifyContent="center"
        paddingVertical="$10"
        paddingHorizontal="$4"
        maxWidth={960}
        alignSelf="center"
        width="100%"
        textAlign="center"
      >
        {/* Geometric Technical Label (NO pill badges, NO dot separators) */}
        <XStack
          alignItems="center"
          backgroundColor="#0D121F"
          borderColor="rgba(255,255,255,0.12)"
          borderWidth={1}
          borderRadius="$2"
          paddingHorizontal="$3"
          paddingVertical="$1.5"
          marginBottom="$6"
        >
          <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#00F5FF" fontWeight="600">
            {t.subheadingMeta}
          </Text>
        </XStack>

        <H1
          fontFamily="'Cal Sans', sans-serif"
          fontSize="$10"
          $gtSm={{ fontSize: "$11" }}
          color="white"
          lineHeight="$10"
          marginBottom="$4"
          letterSpacing={-1}
        >
          {t.heroTitle}
        </H1>

        <Paragraph
          fontFamily="'Kulim Park', sans-serif"
          fontSize="$5"
          $gtSm={{ fontSize: "$6" }}
          color="#94A3B8"
          maxWidth={700}
          marginBottom="$8"
          lineHeight="$6"
        >
          {t.heroDesc}
        </Paragraph>

        {/* Action Buttons: Clean geometric rounded borders */}
        <XStack flexWrap="wrap" justifyContent="center" gap="$3" marginBottom="$8">
          <Button
            tag="a"
            href="https://github.com/KidFeroz/Delanx/releases"
            target="_blank"
            size="$5"
            height={46}
            backgroundColor="white"
            borderRadius="$2"
            icon={<Download size={18} color="#07090E" />}
          >
            <Text fontFamily="'Cal Sans', sans-serif" fontSize="$4" color="#07090E" fontWeight="700">
              {t.downloadBtn}
            </Text>
          </Button>

          <Button
            size="$5"
            height={46}
            backgroundColor="#0E131F"
            borderColor="rgba(255,255,255,0.15)"
            borderWidth={1}
            borderRadius="$2"
            onPress={copyPath}
            icon={copied ? <Check size={18} color="#10B981" /> : <Copy size={18} color="#00F5FF" />}
          >
            <Text fontFamily="'DM Mono', monospace" fontSize="$3" color={copied ? '#10B981' : '#E2E8F0'}>
              {copied ? t.copiedText : t.copyPathBtn}
            </Text>
          </Button>
        </XStack>

        {/* Quick Specs Cards (NO bullet dots) */}
        <XStack
          flexWrap="wrap"
          justifyContent="center"
          gap="$3"
          width="100%"
          maxWidth={780}
          borderTopWidth={1}
          borderTopColor="rgba(255,255,255,0.08)"
          paddingTop="$6"
        >
          {t.specs.map((spec, i) => (
            <Card
              key={i}
              flex={1}
              minWidth={160}
              padding="$3"
              backgroundColor="#0D121F"
              borderColor="rgba(255,255,255,0.08)"
              borderWidth={1}
              borderRadius="$2"
            >
              <Text fontFamily="'Cal Sans', sans-serif" fontSize="$4" color={i === 1 ? '#00F5FF' : i === 2 ? '#10B981' : i === 3 ? '#A855F7' : 'white'} fontWeight="700">
                {spec.label}
              </Text>
              <Text fontFamily="'DM Mono', monospace" fontSize="$1" color="#94A3B8" marginTop="$1">
                {spec.sub}
              </Text>
            </Card>
          ))}
        </XStack>
      </YStack>

      {/* EDUCATIONAL SECTION: PROBLEM DIAGNOSTICS */}
      <YStack maxWidth={960} width="100%" alignSelf="center" paddingHorizontal="$4" marginBottom="$10">
        <Card
          backgroundColor="#0C101B"
          borderColor="rgba(255,255,255,0.09)"
          borderWidth={1}
          borderRadius="$3"
          padding="$6"
          $gtSm={{ padding: "$8" }}
        >
          <XStack justifyContent="space-between" alignItems="center" marginBottom="$4" flexWrap="wrap" gap="$2">
            <YStack>
              <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#00F5FF" textTransform="uppercase">
                {t.problemHeaderTag}
              </Text>
              <H2 fontFamily="'Cal Sans', sans-serif" fontSize="$7" color="white">
                {t.problemTitle}
              </H2>
            </YStack>
            <XStack
              backgroundColor="rgba(255,42,85,0.12)"
              borderColor="rgba(255,42,85,0.3)"
              borderWidth={1}
              paddingHorizontal="$2.5"
              paddingVertical="$1"
              borderRadius="$2"
            >
              <Text fontFamily="'DM Mono', monospace" fontSize="$1" color="#FF2A55" fontWeight="600">
                {t.problemBadge}
              </Text>
            </XStack>
          </XStack>

          <Separator marginVertical="$3" borderColor="rgba(255,255,255,0.06)" />

          <XStack flexWrap="wrap" gap="$5" marginTop="$4">
            <YStack flex={1} minWidth={240} gap="$2">
              <Text fontFamily="'Cal Sans', sans-serif" fontSize="$4" color="white" fontWeight="700">
                {t.p1Title}
              </Text>
              <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8" lineHeight="$4">
                {t.p1Desc}
              </Paragraph>
            </YStack>

            <YStack flex={1} minWidth={240} gap="$2">
              <Text fontFamily="'Cal Sans', sans-serif" fontSize="$4" color="white" fontWeight="700">
                {t.p2Title}
              </Text>
              <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8" lineHeight="$4">
                {t.p2Desc}
              </Paragraph>
            </YStack>

            <YStack flex={1} minWidth={240} gap="$2">
              <Text fontFamily="'Cal Sans', sans-serif" fontSize="$4" color="#10B981" fontWeight="700">
                {t.p3Title}
              </Text>
              <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8" lineHeight="$4">
                {t.p3Desc}
              </Paragraph>
            </YStack>
          </XStack>
        </Card>
      </YStack>

      {/* INTERACTIVE EXPLORER (TABBED ARCHITECTURE) */}
      <YStack maxWidth={960} width="100%" alignSelf="center" paddingHorizontal="$4" marginBottom="$12">
        <YStack alignItems="center" textAlign="center" marginBottom="$6">
          <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#00F5FF" textTransform="uppercase">
            {t.explorerTag}
          </Text>
          <H2 fontFamily="'Cal Sans', sans-serif" fontSize="$8" color="white">
            {t.explorerTitle}
          </H2>
          <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8">
            {t.explorerDesc}
          </Paragraph>
        </YStack>

        {/* Tab Buttons (NO pills) */}
        <XStack flexWrap="wrap" justifyContent="center" gap="$2" marginBottom="$4">
          {(['decomps', 'pcclassic', 'engines', 'schema'] as const).map(tabKey => (
            <Button
              key={tabKey}
              size="$3"
              height={32}
              backgroundColor={currentTab === tabKey ? 'rgba(0,245,255,0.15)' : '#0C101B'}
              borderColor={currentTab === tabKey ? '#00F5FF' : 'rgba(255,255,255,0.1)'}
              borderWidth={1}
              borderRadius="$2"
              onPress={() => setCurrentTab(tabKey)}
            >
              <Text
                fontFamily="'DM Mono', monospace"
                fontSize="$2"
                fontWeight="700"
                color={currentTab === tabKey ? '#00F5FF' : '#94A3B8'}
              >
                {t.tabs[tabKey]}
              </Text>
            </Button>
          ))}
        </XStack>

        {/* Tab Content Card */}
        <Card
          backgroundColor="#0C101B"
          borderColor="rgba(255,255,255,0.09)"
          borderWidth={1}
          borderRadius="$3"
          padding="$6"
        >
          <XStack flexWrap="wrap" gap="$6" alignItems="flex-start">
            {/* Tree View */}
            <YStack
              flex={1}
              minWidth={280}
              backgroundColor="#06090E"
              padding="$4"
              borderRadius="$2"
              borderWidth={1}
              borderColor="rgba(255,255,255,0.06)"
            >
              <XStack justifyContent="space-between" marginBottom="$3" paddingBottom="$2" borderBottomWidth={1} borderBottomColor="rgba(255,255,255,0.08)">
                <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#94A3B8" fontWeight="700">
                  sdmc:/
                </Text>
                <Text fontFamily="'DM Mono', monospace" fontSize="$1" color="#00F5FF">
                  FAT32 / exFAT
                </Text>
              </XStack>
              <Text
                tag="pre"
                fontFamily="'DM Mono', monospace"
                fontSize="$2"
                color="#E2E8F0"
                style={{ margin: 0, overflowX: 'auto', lineHeight: '1.6' }}
              >
                {activeArch.tree}
              </Text>
            </YStack>

            {/* Info Panel */}
            <YStack flex={1} minWidth={260} gap="$3">
              <XStack
                alignSelf="flex-start"
                backgroundColor="rgba(0,245,255,0.08)"
                paddingHorizontal="$2"
                paddingVertical="$1"
                borderRadius="$1"
                borderWidth={1}
                borderColor="rgba(0,245,255,0.2)"
              >
                <Text fontFamily="'DM Mono', monospace" fontSize="$1" color="#00F5FF" fontWeight="700">
                  {activeArch.badge}
                </Text>
              </XStack>

              <H3 fontFamily="'Cal Sans', sans-serif" fontSize="$6" color="white">
                {activeArch.title}
              </H3>

              <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8" lineHeight="$4">
                {activeArch.desc}
              </Paragraph>

              <Separator borderColor="rgba(255,255,255,0.06)" marginVertical="$2" />

              <YStack gap="$1.5">
                <XStack justifyContent="space-between">
                  <Text fontFamily="'Kulim Park', sans-serif" fontSize="$2" color="#94A3B8">
                    {t.memModeLabel}
                  </Text>
                  <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="white" fontWeight="700">
                    {activeArch.mem}
                  </Text>
                </XStack>
                <XStack justifyContent="space-between">
                  <Text fontFamily="'Kulim Park', sans-serif" fontSize="$2" color="#94A3B8">
                    {t.integrityLabel}
                  </Text>
                  <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#10B981" fontWeight="700">
                    {activeArch.check}
                  </Text>
                </XStack>
              </YStack>

              <Text fontFamily="'Kulim Park', sans-serif" fontSize="$2" color="#64748B" fontStyle="italic" marginTop="$2">
                {t.moddingTip}
              </Text>
            </YStack>
          </XStack>
        </Card>
      </YStack>

      {/* TECHNICAL LIFECYCLE (ATOMIC HAND-OFF) */}
      <YStack maxWidth={960} width="100%" alignSelf="center" paddingHorizontal="$4" marginBottom="$12">
        <YStack alignItems="center" textAlign="center" marginBottom="$6">
          <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#00F5FF" textTransform="uppercase">
            {t.lifecycleTag}
          </Text>
          <H2 fontFamily="'Cal Sans', sans-serif" fontSize="$8" color="white">
            {t.lifecycleTitle}
          </H2>
          <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8">
            {t.lifecycleDesc}
          </Paragraph>
        </YStack>

        <XStack flexWrap="wrap" gap="$4">
          {t.steps.map((st, i) => (
            <Card
              key={i}
              flex={1}
              minWidth={200}
              padding="$4"
              backgroundColor="#0C101B"
              borderColor="rgba(255,255,255,0.08)"
              borderWidth={1}
              borderRadius="$2"
            >
              <Text fontFamily="'DM Mono', monospace" fontSize="$1" color={i === 0 ? '#00F5FF' : i === 1 ? '#10B981' : i === 2 ? '#A855F7' : '#FF2A55'} fontWeight="700" marginBottom="$2">
                {st.num} / {st.tag}
              </Text>
              <Text fontFamily="'Cal Sans', sans-serif" fontSize="$4" color="white" fontWeight="700" marginBottom="$1">
                {st.title}
              </Text>
              <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$2" color="#94A3B8" lineHeight="$3">
                {st.desc}
              </Paragraph>
            </Card>
          ))}
        </XStack>
      </YStack>

      {/* INSTALLATION SECTION */}
      <YStack maxWidth={960} width="100%" alignSelf="center" paddingHorizontal="$4" marginBottom="$12">
        <YStack alignItems="center" textAlign="center" marginBottom="$6">
          <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#00F5FF" textTransform="uppercase">
            {t.installTag}
          </Text>
          <H2 fontFamily="'Cal Sans', sans-serif" fontSize="$8" color="white">
            {t.installTitle}
          </H2>
        </YStack>

        <XStack flexWrap="wrap" gap="$4">
          <Card flex={1} minWidth={240} padding="$5" backgroundColor="#0C101B" borderColor="rgba(255,255,255,0.08)" borderWidth={1} borderRadius="$2">
            <Text fontFamily="'Cal Sans', sans-serif" fontSize="$5" color="white" fontWeight="700" marginBottom="$2">
              {t.i1Title}
            </Text>
            <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8" lineHeight="$4">
              {t.i1Desc}
            </Paragraph>
          </Card>

          <Card flex={1} minWidth={240} padding="$5" backgroundColor="#0C101B" borderColor="rgba(255,255,255,0.08)" borderWidth={1} borderRadius="$2">
            <Text fontFamily="'Cal Sans', sans-serif" fontSize="$5" color="white" fontWeight="700" marginBottom="$2">
              {t.i2Title}
            </Text>
            <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8" lineHeight="$4">
              {t.i2Desc}
            </Paragraph>
          </Card>

          <Card flex={1} minWidth={240} padding="$5" backgroundColor="#0C101B" borderColor="rgba(255,255,255,0.08)" borderWidth={1} borderRadius="$2">
            <Text fontFamily="'Cal Sans', sans-serif" fontSize="$5" color="white" fontWeight="700" marginBottom="$2">
              {t.i3Title}
            </Text>
            <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8" lineHeight="$4">
              {t.i3Desc}
            </Paragraph>
          </Card>
        </XStack>
      </YStack>

      {/* ABOUT & TRANSPARENCY SECTION (NO PILLS, NO DOTS) */}
      <YStack maxWidth={960} width="100%" alignSelf="center" paddingHorizontal="$4" marginBottom="$12" textAlign="center">
        <Card
          backgroundColor="#080C14"
          borderColor="rgba(255,255,255,0.08)"
          borderWidth={1}
          borderRadius="$3"
          padding="$6"
          alignItems="center"
        >
          <H2 fontFamily="'Cal Sans', sans-serif" fontSize="$7" color="white" marginBottom="$2">
            {t.aboutTitle}
          </H2>
          <Paragraph fontFamily="'Kulim Park', sans-serif" fontSize="$3" color="#94A3B8" maxWidth={680} lineHeight="$4" marginBottom="$6">
            {t.aboutDesc}
          </Paragraph>

          <XStack flexWrap="wrap" justifyContent="center" gap="$3">
            {t.badges.map((b, idx) => (
              <XStack
                key={idx}
                backgroundColor="#0F1422"
                borderColor="rgba(255,255,255,0.12)"
                borderWidth={1}
                borderRadius="$1"
                paddingHorizontal="$3"
                paddingVertical="$1.5"
              >
                <Text fontFamily="'DM Mono', monospace" fontSize="$1" color={idx === 0 ? '#10B981' : idx === 1 ? '#00F5FF' : '#A855F7'} fontWeight="600">
                  {b}
                </Text>
              </XStack>
            ))}
          </XStack>
        </Card>
      </YStack>

      {/* FOOTER */}
      <YStack
        tag="footer"
        borderTopWidth={1}
        borderTopColor="rgba(255,255,255,0.08)"
        paddingVertical="$6"
        paddingHorizontal="$4"
        backgroundColor="#07090E"
      >
        <XStack
          maxWidth={960}
          width="100%"
          alignSelf="center"
          justifyContent="space-between"
          alignItems="center"
          flexWrap="wrap"
          gap="$4"
        >
          <XStack alignItems="center" gap="$3">
            <img src="./assets/delanx-iso-dark.svg" alt="Delanx" style={{ width: 22, height: 22, objectFit: 'contain' }} />
            <Text fontFamily="'Kulim Park', sans-serif" fontSize="$2" color="#64748B">
              {t.footerCopyright}
            </Text>
          </XStack>

          <XStack alignItems="center" gap="$3">
            <Button
              tag="a"
              href="https://github.com/KidFeroz/Delanx"
              target="_blank"
              size="$2"
              backgroundColor="transparent"
            >
              <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#94A3B8">Repo</Text>
            </Button>
            <Button
              tag="a"
              href="https://github.com/KidFeroz/Delanx/releases"
              target="_blank"
              size="$2"
              backgroundColor="transparent"
            >
              <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#94A3B8">Releases</Text>
            </Button>
            <Button
              tag="a"
              href="https://github.com/KidFeroz/Delanx/blob/main/LICENSE"
              target="_blank"
              size="$2"
              backgroundColor="transparent"
            >
              <Text fontFamily="'DM Mono', monospace" fontSize="$2" color="#94A3B8">GPLv3</Text>
            </Button>
          </XStack>
        </XStack>
      </YStack>

    </YStack>
  )
}
