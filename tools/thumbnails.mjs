import { chromium } from 'playwright';
import { PNG } from 'pngjs';
import { createServer } from 'node:http';
import { readFile, mkdir, stat, writeFile } from 'node:fs/promises';
import path from 'node:path';

const root = path.resolve(process.argv[2] || 'dist');
const apps = ['blumme', 'rays', 'wonky_grid', 'space_travel'];
const mount = '/art-studies'; // Exercise GitHub Pages-style subpath hosting.
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.css': 'text/css', '.png': 'image/png' };
const server = createServer(async (request, response) => {
  try {
    const pathname = decodeURIComponent(new URL(request.url, 'http://localhost').pathname);
    if (!pathname.startsWith(mount + '/')) throw new Error('Outside mount');
    const relative = pathname.slice(mount.length);
    const filename = path.resolve(root, '.' + (relative === '/' ? '/index.html' : relative));
    if (!filename.startsWith(root + path.sep)) throw new Error('Outside site');
    response.setHeader('Content-Type', types[path.extname(filename)] || 'application/octet-stream');
    response.end(await readFile(filename));
  } catch {
    response.writeHead(404).end('Not found');
  }
});
await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
const base = `http://127.0.0.1:${server.address().port}${mount}`;
let browser;
try {
  browser = await chromium.launch({ args: [
    '--no-sandbox', '--disable-dev-shm-usage', '--use-gl=angle',
    '--use-angle=swiftshader', '--enable-unsafe-swiftshader'
  ] });
  await mkdir(path.join(root, 'thumbnails'), { recursive: true });
  for (const app of apps) {
    const page = await browser.newPage({ viewport: { width: 720, height: 480 }, deviceScaleFactor: 1 });
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    page.on('response', response => { if (response.status() >= 400) errors.push(`${response.status()} ${response.url()}`); });
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    await page.goto(`${base}/${app}.html?capture=1&seed=42`);
    await page.waitForFunction(() => window.artReady || window.artError, null, { timeout: 60000 });
    if (await page.evaluate(() => !!window.artError)) throw new Error(`${app} failed: ${errors.join('\n')}`);
    // Capture mode advances a fixed 180 simulation steps and then pauses.
    await page.waitForFunction(() => window.artFrames >= 182, null, { timeout: 60000 });
    const filename = path.join(root, 'thumbnails', `${app}.png`);
    const bytes = await page.locator('#canvas').screenshot({ path: filename });
    const png = PNG.sync.read(bytes);
    let low = 255, high = 0;
    for (let i = 0; i < png.data.length; i += 4) {
      low = Math.min(low, png.data[i], png.data[i + 1], png.data[i + 2]);
      high = Math.max(high, png.data[i], png.data[i + 1], png.data[i + 2]);
    }
    if (high - low < 30) throw new Error(`${app}: canvas looks blank`);
    // Verify resizing too: screenshots double as WebGL smoke tests.
    await page.setViewportSize({ width: 640, height: 400 });
    await page.waitForFunction(() => document.querySelector('canvas').width === 640 && document.querySelector('canvas').height === 400);
    // Reveal and exercise the real HTML control rather than synthetic DOM keys.
    await page.evaluate(() => document.body.classList.remove('capture'));
    await page.mouse.move(320, 200);
    const beforeToggle = await page.locator('#special').getAttribute('aria-pressed');
    await page.locator('#special').click();
    if (await page.locator('#special').getAttribute('aria-pressed') === beforeToggle)
      errors.push('App-specific control did not toggle');
    await page.mouse.move(300, 200);
    await page.waitForFunction(() => document.body.classList.contains('ui-idle'));
    await page.mouse.move(301, 200);
    await page.waitForFunction(() => !document.body.classList.contains('ui-idle'));
    if (app === apps[0]) {
      await page.locator('#special').focus();
      await page.waitForTimeout(2800);
      if (await page.evaluate(() => document.body.classList.contains('ui-idle')))
        errors.push('Controls hid while keyboard-focused');
    }
    if (await page.evaluate(() => !!window.artError)) errors.push('Rendering stopped');
    if (errors.length) throw new Error(`${app}: ${errors.join('\n')}`);
    console.log(`${app}: WebGL rendering, resize, auto-hide controls OK; ${(await stat(filename)).size} byte thumbnail`);
    await page.close();
  }
  // Verify gallery images and links below a repository-style path.
  const page = await browser.newPage();
  await page.goto(`${base}/`);
  const links = await page.locator('.card').evaluateAll(cards => cards.map(card => card.getAttribute('href')));
  if (apps.some(app => !links.includes(`./${app}.html`))) throw new Error('Gallery links are incomplete');
  if (!await page.locator('.preview img').evaluateAll(images => images.every(image => image.complete && image.naturalWidth > 0)))
    throw new Error('Gallery thumbnails did not load');
  await page.close();
  await writeFile(path.join(root, '.nojekyll'), '');
} finally {
  if (browser) await browser.close();
  await new Promise(resolve => server.close(resolve));
}
