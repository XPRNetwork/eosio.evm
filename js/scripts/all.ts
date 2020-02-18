import { clearall } from './clearall'
import { create } from './create'
import { deploy } from './deploy'
import { raw } from './raw'
import * as program from 'commander'

program.option('-d, --deploy', 'deploy')
      .option('-x, --clear', 'clear all')
      .option('-c, --create', 'create')
      .option('-r, --raw', 'raw')
      .option('-a, --all', 'all');

program.parse(process.argv);

async function main () {
  if (program.deploy || program.all)
    await deploy()

  if (program.clear || program.all)
    await clearall()

  if (program.create || program.all)
    await create()

  if (program.raw || program.all)
    await raw()
}

main()