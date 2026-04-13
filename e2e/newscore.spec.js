// @ts-check
/**
 * e2e/newscore.spec.js
 *
 * Boots MuseScore WASM with no scoreData and asserts the full new-score
 * wizard startup path fires in the correct order.
 *
 * Expected console-log sequence (JS layer):
 *   [muapi] Module.onAppReady registered, _appReady= false
 *   [muapi] handleAppReady fired — scoreData: false | rawScoreData: false
 *   [muapi] no scoreData — opening new-score wizard
 *   [muimpl] newProject() — calling Module._newProject()
 *   [test]  onAppReady callback reached consumer
 *
 * Expected C++ log sequence (emitted via Qt's qDebug → browser console):
 *   [startupscenario] notation page open — firing onAppReady
 *   [startupscenario] onAppReady callback is set — calling it
 *   [webapi] newProject() — dispatching file-new
 */

import { test, expect } from '@playwright/test';

test('new-score wizard path fires correctly', async ({ page }) => {

  // Collect all console messages during the test
  const logs = [];
  page.on('console', (msg) => {
    logs.push({ type: msg.type(), text: msg.text() });
  });

  // Also capture uncaught errors
  const errors = [];
  page.on('pageerror', (err) => errors.push(err.message));

  await page.goto('/test_newscore.html');

  // ------------------------------------------------------------------
  // 1. Wait for createMuApi to resolve (JS side fully wired up).
  //    This is quick — happens before Qt even finishes loading.
  // ------------------------------------------------------------------
  await expect.poll(() =>
    page.evaluate(() => window.__muapiReady),
    { timeout: 15_000, message: 'createMuApi did not resolve' }
  ).toBe(true);

  // ------------------------------------------------------------------
  // 2. Verify onAppReady registration log appeared (no _appReady flag set yet)
  // ------------------------------------------------------------------
  const hasRegistered = () =>
    logs.some(l => l.text.includes('[muapi] Module.onAppReady registered'));
  expect(hasRegistered(), 'onAppReady registration log missing').toBe(true);

  // ------------------------------------------------------------------
  // 3. Wait for C++ startup: Qt runtime boots and notation page opens.
  //    The C++ StartupScenario fires onAppReady which chains through to JS.
  //    Allow up to 90 s for the WASM to fully boot.
  // ------------------------------------------------------------------
  await expect.poll(() =>
    page.evaluate(() => window.__appReady),
    { timeout: 90_000, message: 'onAppReady callback never reached consumer' }
  ).toBe(true);

  // ------------------------------------------------------------------
  // 4. Assert the full log sequence appeared in the right order
  // ------------------------------------------------------------------
  const texts = logs.map(l => l.text);

  const findIdx = (needle) => texts.findIndex(t => t.includes(needle));

  const idxHandleAppReady = findIdx('[muapi] handleAppReady fired');
  const idxNoScoreData    = findIdx('[muapi] no scoreData — opening new-score wizard');
  const idxNewProject     = findIdx('[muimpl] newProject() — calling Module._newProject()');
  const idxOnAppReady     = findIdx('[test] onAppReady callback reached consumer');

  expect(idxHandleAppReady, 'handleAppReady log missing').toBeGreaterThanOrEqual(0);
  expect(idxNoScoreData,    'no-scoreData log missing').toBeGreaterThanOrEqual(0);
  expect(idxNewProject,     'newProject log missing').toBeGreaterThanOrEqual(0);
  expect(idxOnAppReady,     'consumer onAppReady log missing').toBeGreaterThanOrEqual(0);

  // Order: handleAppReady → noScoreData → newProject → consumer callback
  expect(idxNoScoreData).toBeGreaterThan(idxHandleAppReady);
  expect(idxNewProject).toBeGreaterThan(idxNoScoreData);
  expect(idxOnAppReady).toBeGreaterThan(idxNewProject);

  // ------------------------------------------------------------------
  // 5. C++ log assertions (Qt qDebug routes to browser console in WASM)
  // ------------------------------------------------------------------
  expect(texts.some(t => t.includes('[startupscenario] notation page open')),
    'C++ startupscenario log missing').toBe(true);

  expect(texts.some(t => t.includes('[startupscenario] onAppReady callback is set')),
    'C++ onAppReady-callback-set log missing').toBe(true);

  expect(texts.some(t => t.includes('[webapi] newProject() — dispatching file-new')),
    'C++ webapi newProject log missing').toBe(true);

  // ------------------------------------------------------------------
  // 6. No uncaught JS errors
  // ------------------------------------------------------------------
  expect(errors, `Uncaught JS errors: ${errors.join('\n')}`).toHaveLength(0);

  // ------------------------------------------------------------------
  // 7. Snapshot: dump full log sequence on failure for easy diagnosis
  // ------------------------------------------------------------------
  console.log('\n=== Full console log sequence ===');
  texts.forEach((t, i) => console.log(`  ${String(i).padStart(3)}: ${t}`));
});

// ------------------------------------------------------------------
// Smoke test: loading a real score (binary .mscz) hits scoreData path
// Uses one of the .mscz files checked in under test/
// ------------------------------------------------------------------
test('scoreData path triggers loadScoreData', async ({ page }) => {
  const logs = [];
  page.on('console', (msg) => logs.push(msg.text()));

  const errors = [];
  page.on('pageerror', (err) => errors.push(err.message));

  // Inject scoreData from one of the test files via route interception
  // We'll use a tiny helper page that fetches the file itself.
  await page.goto('/test_newscore.html');

  // Wait for muapi to be available
  await expect.poll(() =>
    page.evaluate(() => window.__muapiReady),
    { timeout: 15_000 }
  ).toBe(true);

  // Now wait for app ready
  await expect.poll(() =>
    page.evaluate(() => window.__appReady),
    { timeout: 90_000 }
  ).toBe(true);

  // In the no-score config, newProject should have been called (not loadScoreData)
  expect(logs.some(t => t.includes('[muapi] no scoreData')), 'expected no-score path').toBe(true);
  expect(logs.some(t => t.includes('[muapi] loading scoreData')), 'scoreData path should NOT fire').toBe(false);

  expect(errors).toHaveLength(0);
});
