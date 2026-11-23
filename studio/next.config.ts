import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  // The shared decoder (../decoder) ships TypeScript source, so Next compiles it too.
  transpilePackages: ["@ditdah/decoder"],
  cacheComponents: true,
  partialPrefetching: true,
  turbopack: {
    rules: {
      "*.css": {
        loaders: ["@tailwindcss/turbopack"],
        as: "*.css",
      },
    },
  },
};

export default nextConfig;
