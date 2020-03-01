const yaml = require('js-yaml');
const { readdirSync, readFileSync, writeFileSync, existsSync, mkdirSync } = require('fs');
const { Transaction } = require('ethereumjs-tx');

const getDirectories = source => readdirSync(source)
const testRoot = './BlockchainTests'
const testType = 'GeneralStateTests'
const testDirectory = `${testRoot}/${testType}/`

// https://gist.github.com/drodsou/de2ba6291aea67ffc5bc4b52d8c32abd
function writeFileSyncRecursive(filename, content, charset) {
	// create folder path if not exists
	filename.split('/').slice(0,-1).reduce( (last, folder)=>{
		let folderPath = last ? (last + '/' + folder) : folder
		if (!existsSync(folderPath)) mkdirSync(folderPath)
		return folderPath
	})

	writeFileSync(filename, content, charset)
}

// Get document, or throw exception on error
try {
  const directories = getDirectories(testDirectory);

  for (const directory of directories) {
    const subTestDirectory = `${testDirectory}/${directory}`;
    const tests = getDirectories(subTestDirectory);

    for (const testFileName of tests) {
      let fullTest = require(`${subTestDirectory}/${testFileName}`)

      for (const testName in fullTest) {
        console.log(testName)

        // Only target Istanbul
        if (fullTest[testName].network !== 'Istanbul') {
          delete fullTest[testName];
          continue;
        }

        // Modify blocks to have serialized RLP TXs and a ENV object
        fullTest[testName].blocks = fullTest[testName].blocks.map(block => {
          block.transactionRlps = block.transactions.map(transaction => {
            for (const key in transaction) {
              if (!transaction[key]) delete transaction[key]
            }
            const tx = new Transaction(transaction)
            return tx.serialize().toString('hex')
          })

          block.env = {
            currentCoinbase: block.blockHeader.coinbase,
            currentDifficulty: block.blockHeader.difficulty,
            currentGasLimit: block.blockHeader.gasLimit,
            currentNumber: block.blockHeader.number,
            currentTimestamp: block.blockHeader.timestamp,
            previousHash: block.blockHeader.parentHash
          }
          return block
        })
      }

      // Write to new dir
      writeFileSyncRecursive(`./BlockchainTests/GeneralStateTestsEOS/${directory}/${testFileName}`, JSON.stringify(fullTest, null, 4), 'utf8')
    }
  }

} catch (e) {
  console.log(e);
}