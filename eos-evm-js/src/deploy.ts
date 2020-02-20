import { readdirSync } from 'fs'
import { join } from 'path'

export function getDeployableFilesFromDir(dir: string) {
  const dirCont = readdirSync(dir)
  const wasmFileName = dirCont.find(filePath => filePath.match(/.*\.(wasm)$/gi) as any)
  const abiFileName = dirCont.find(filePath => filePath.match(/.*\.(abi)$/gi) as any)
  if (!wasmFileName) throw new Error(`Cannot find a ".wasm file" in ${dir}`)
  if (!abiFileName) throw new Error(`Cannot find an ".abi file" in ${dir}`)
  return {
    wasmPath: join(dir, wasmFileName),
    abiPath: join(dir, abiFileName),
  }
}