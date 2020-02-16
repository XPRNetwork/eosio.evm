const { Transaction } = require('ethereumjs-tx');

const privateKey = Buffer.from('8dd3ec4846cecac347a830b758bf7e438c4d9b36a396b189610c90b57a70163d', 'hex')
const txData = {
  nonce:    '0x1',
  gasPrice: '0x4a817c800',
  gasLimit: '0x6691b7',
  from:     "0xf79b834a37f3143f4a73fc3934edac67fd3a01cd",
  to: '0x5384C6C53d93b97E0E955175205e8bfCF1b56Ecd',
  value:    '0x0',
  data:    '0xa9059cbb000000000000000000000000c7340a9c53f3b1c2a51445295f199f68abab938b000000000000000000000000000000000000000000000000000000000000000a',
  chainId:  1
}

const tx = new Transaction(txData);
// console.log('RLP-Encoded Tx: 0x' + tx.serialize().toString('hex'))

// Sign transaction
tx.sign(privateKey);

const serializedTx = tx.serialize();
console.log('Signed Raw Transaction: 0x')
console.log(serializedTx.toString('hex'))