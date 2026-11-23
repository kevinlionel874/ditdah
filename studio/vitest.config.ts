import { fileURLToPath } from "node:url";
import { defineConfig } from "vitest/config";

export default defineConfig({
  resolve: {
    // Same "@/..." import alias as tsconfig.json, so tests can import "@/lib/...".
    alias: { "@": fileURLToPath(new URL("./", import.meta.url)) },
  },
  test: {
    include: ["test/**/*.test.ts", "test/**/*.test.tsx"],
  },
});
