const Web3 = require('web3');
const web3 = new Web3('https://mainnet.infura.io');

async function main () {
  const block = await web3.eth.getBlock(1000000)
  console.dir(block, { depth: null })
}

main()