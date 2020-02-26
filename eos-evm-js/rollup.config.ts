import resolve from 'rollup-plugin-node-resolve'
import commonjs from 'rollup-plugin-commonjs'
import sourceMaps from 'rollup-plugin-sourcemaps'
import camelCase from 'lodash.camelcase'
import typescript from 'rollup-plugin-typescript2'
import json from 'rollup-plugin-json'
import copy from 'rollup-plugin-copy'

const pkg = require('./package.json')

const libraryName = 'eos-evm-js'

export default {
  input: `src/${libraryName}.ts`,
  output: [
    { file: pkg.main, name: camelCase(libraryName), format: 'umd', sourcemap: true },
    { file: pkg.module, format: 'es', sourcemap: true },
  ],
  // Indicate here external modules you don't wanna include in your bundle (i.e.: 'lodash')
  external: ['ethereumjs-util', 'ethereumjs-tx', 'ethereumjs-abi', 'ethereumjs-common'],
  watch: {
    include: 'src/**',
  },

  plugins: [
    copy({
      targets: [
        { src: 'src/eos-contracts/**/*', dest: 'dist/eos-contracts' },
        { src: 'src/eth-contracts/**/*', dest: 'dist/eth-contracts' }
      ]
    }),

    // Allow json resolution
    json(),
    // Compile TypeScript files
    typescript({ useTsconfigDeclarationDir: true }),
    // Allow bundling cjs modules (unlike webpack, rollup doesn't understand cjs)
    commonjs({
      namedExports: {
        'ethereumjs-util': ['privateToAddress']
      }
    }),
    // Allow node_modules resolution, so you can use 'external' to control
    // which external modules to include in the bundle
    // https://github.com/rollup/rollup-plugin-node-resolve#usage
    resolve(),

    // Resolve source maps to the original source
    sourceMaps(),
  ],
}
