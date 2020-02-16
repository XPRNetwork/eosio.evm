import { clearall } from './clearall'
import { create } from './create'
import { deploy } from './deploy'
import { raw } from './raw'

async function main () {
  await deploy()
  await clearall()
  await create()
  await raw()
}

main()