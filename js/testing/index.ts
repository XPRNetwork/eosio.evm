import { getTestsFromArgs } from 'ethereumjs-testing'
import { transact } from '../eos'
import { deploy } from '../scripts/deploy'
import { decodeTx } from 'ethereum-tx-decoder'

const execute = async (rlp) => {
  return await transact([
    {
      "account": process.env.CONTRACT,
      "name": "raw",
      "data": {
        tx: rlp,
        sender: '424a26f6de36eb738762cead721bac23c62a724e'
      },
      "authorization": [
        {
          "actor": process.env.CONTRACT,
          "permission": "active"
        }
      ]
    }
  ])
}

async function executeTest (testName, test) {
  try {
    const rm0x = test.rlp.substring(2)
    await execute(rm0x)
    console.log('\x1b[32m%s\x1b[0m', testName);
  } catch (e) {
    console.log('\x1b[31m%s\x1b[0m', testName);
    console.log(decodeTx(test.rlp))
    console.log(e)
  }
}

async function startTest () {
  getTestsFromArgs('TransactionTests', (fileName, testName, test) => {
    return executeTest(testName, test)
  }, { forkConfig: 'Istanbul' })
}

async function main () {
  await deploy()
  await startTest()
}

main()