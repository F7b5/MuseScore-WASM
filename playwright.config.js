// @ts-check
import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
  testDir: './e2e',
  timeout: 120_000,       // WASM boot can take ~30-60 s
  expect: { timeout: 90_000 },
  reporter: 'list',
  use: {
    // Serve build.artifacts/ as the static root
    baseURL: 'http://localhost:9876',
    headless: true,
    // SharedArrayBuffer is not used (wasm_singlethread build) — no COOP/COEP needed
    browserName: 'chromium',
  },
  projects: [
    {
      name: 'chromium',
      use: { ...devices['Desktop Chrome'] },
    },
  ],
  // Spin up a tiny static server for build.artifacts/
  webServer: {
    command: 'npx serve -l 9876 -s build.artifacts --no-clipboard',
    url: 'http://localhost:9876',
    reuseExistingServer: true,
    timeout: 10_000,
  },
});
