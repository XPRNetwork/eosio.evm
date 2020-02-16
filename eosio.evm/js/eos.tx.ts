import { transact } from './eos'

async function main () {
  const result = await transact([
    {
      account: 'evm',
      name: 'raw',
      data: {
        tx: 'f866808609184e72a0008303000094b0920c523d582040f2bcb1bd7fb1c7c1ecebdb34808025a0e948a3307231941c23f678a0194089fcd52bd62310ee29b7a589e8c60bfec02aa064154144171470510d686559fe3e168f38a4e02c61e7b1aa2f8a19889836534c',
        sender: '20cff216bd68dbf1a9b9833c9bda81d1e1bd119d'
      },
      authorization: [{ actor: 'evm', permission: 'active' }]
    }
  ])
  console.log(result)
}

main()