const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const assetsDir = path.join(__dirname, '..', 'docs', 'assets');

const svgFiles = [
  'delanx-iso-dark.svg',
  'delanx-iso-light.svg',
  'delanx-horizontal-dark.svg',
  'delanx-horizontal-light.svg',
  'delanx-stacked-dark.svg',
  'delanx-stacked-light.svg',
  'delanx-ultra-horizontal.svg',
  'delanx-ultra-icon.svg',
  'delanx-ultra-banner-16-9.svg'
];

console.log('--- Iniciando rasterización de SVGs a PNG y WebP con sharp-cli ---');

for (const file of svgFiles) {
  const baseName = path.basename(file, '.svg');
  const inputSvg = path.join(assetsDir, file);
  const outPng = path.join(assetsDir, `${baseName}.png`);
  const outWebp = path.join(assetsDir, `${baseName}.webp`);

  if (!fs.existsSync(inputSvg)) {
    console.warn(`[OMITIDO] No encontrado: ${inputSvg}`);
    continue;
  }

  console.log(`Convirtiendo: ${file}...`);
  try {
    // Generar PNG
    execSync(`npx sharp-cli -i "${inputSvg}" -o "${outPng}"`, { stdio: 'pipe', shell: true });
    // Generar WebP
    execSync(`npx sharp-cli -i "${inputSvg}" -o "${outWebp}"`, { stdio: 'pipe', shell: true });
    console.log(`  -> ${baseName}.png y ${baseName}.webp generados.`);
  } catch (err) {
    console.error(`Error convirtiendo ${file}:`, err.message);
  }
}

// También actualizar icon.jpg (Switch NRO icon en 256x256) desde delanx-ultra-icon.svg
try {
  const iconJpg = path.join(__dirname, '..', 'icon.jpg');
  execSync(`npx sharp-cli -i "${path.join(assetsDir, 'delanx-iso-dark.svg')}" -o "${iconJpg}" resize 256 256`, { stdio: 'pipe', shell: true });
  console.log('  -> icon.jpg (Switch NACP 256x256) actualizado.');
} catch (e) {
  console.log('Aviso actualizando icon.jpg:', e.message);
}

console.log('--- Rasterización completada con éxito ---');
