import { readdirSync, readFileSync } from 'fs'
import { join, resolve } from 'path'
import { Serialize } from '@jafri/eosjs2'
import { transact, eos, publicKeys } from '../eos'

const CONTRACT = process.env.CONTRACT || 'evm'
const CONTRACT_DIR = resolve(__dirname + '../../..')

function getDeployableFilesFromDir(dir) {
  const dirCont = readdirSync(dir)
  const wasmFileName = dirCont.find(filePath => filePath.match(/.*\.(wasm)$/gi))
  const abiFileName = dirCont.find(filePath => filePath.match(/.*\.(abi)$/gi))
  if (!wasmFileName) throw new Error(`Cannot find a ".wasm file" in ${dir}`)
  if (!abiFileName) throw new Error(`Cannot find an ".abi file" in ${dir}`)
  return {
    wasmPath: join(dir, wasmFileName),
    abiPath: join(dir, abiFileName),
  }
}

const deployContract = async function deployContract(account, contractDir) {
  const { wasmPath, abiPath } = getDeployableFilesFromDir(contractDir)

  // 1. Prepare SETCODE
  // read the file and make a hex string out of it
  const wasm = readFileSync(wasmPath).toString(`hex`)

  // 2. Prepare SETABI
  const buffer = new Serialize.SerialBuffer({
    textEncoder: eos.textEncoder,
    textDecoder: eos.textDecoder,
  })

  let abi = JSON.parse(readFileSync(abiPath, `utf8`))
  const abiDefinition = eos.abiTypes.get(`abi_def`)
  // need to make sure abi has every field in abiDefinition.fields
  // otherwise serialize throws
  abi = abiDefinition.fields.reduce(
    (acc, { name: fieldName }) =>
      Object.assign(acc, { [fieldName]: acc[fieldName] || [] }),
    abi
  )
  abiDefinition.serialize(buffer, abi)

  // 3. Set code
  try {
    await transact([
      {
        account: 'eosio',
        name: 'setcode',
        authorization: [
          {
            actor: account,
            permission: 'active',
          },
        ],
        data: {
          account: account,
          vmtype: 0,
          vmversion: 0,
          code: wasm,
        },
      }
    ])
  } catch (e) {
    console.log('Set code failed')
  }

  // Set ABI
  try {
    await transact([
      {
        account: 'eosio',
        name: 'setabi',
        authorization: [
          {
            actor: account,
            permission: 'active',
          },
        ],
        data: {
          account: account,
          abi: Buffer.from(buffer.asUint8Array()).toString(`hex`),
        },
      }
    ])
  } catch (e) {
    console.log('Set abi failed')
  }
}

export const deploy = async () => {
  try {
    const account = await eos.rpc.get_account(CONTRACT);
  } catch (e) {
    await transact([
      {
        account: 'eosio',
        name: 'newaccount',
        data: {
          owner: {
            accounts: [],
            keys: [
              { key: publicKeys[0], weight: 1 }
            ],
            threshold: 1,
            waits: []
          },
          active: {
            accounts: [
              {
                permission: {
                  actor: CONTRACT,
                  permission: 'eosio.code'
                },
                weight: 1
              }
            ],
            keys: [
              { key: publicKeys[0], weight: 1 }
            ],
            threshold: 1,
            waits: []
          },
          creator: "eosio",
          name: CONTRACT
        },
        authorization: [{ actor: 'eosio', permission: 'active' }]
      }
    ])
  }

  console.log('deploying')
  await deployContract(CONTRACT, CONTRACT_DIR)
}
