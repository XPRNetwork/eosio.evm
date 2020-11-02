const fs = require('fs')
const path = require('path')
const solc = require('solc')

const contractInput = {
  sources: {
    ERC20: {
      content: fs.readFileSync(path.join(__dirname, 'ERC20.sol'), 'utf8')
    },
    ERC721: {
      content: fs.readFileSync(path.join(__dirname, 'ERC721.sol'), 'utf8')
    }
  },
  language: 'Solidity',
  settings: {
    optimizer: {
      enabled: true,
      runs: 300
    },
    evmVersion: 'istanbul',
    outputSelection: {
      '*': {
        '*': ['abi', 'evm.bytecode']
      }
    }
  }
}

const findImports = (_path: string) => ({
  contents: fs.readFileSync(path.join(__dirname, '../../', _path), 'utf8')
})

const compiledContract = solc.compile(JSON.stringify(contractInput), { import: findImports })
fs.writeFileSync(
  path.join(__dirname, 'compiled.json'),
  JSON.stringify(JSON.parse(compiledContract), null, 4)
)
