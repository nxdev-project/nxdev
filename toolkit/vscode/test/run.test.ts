import * as assert from 'assert';
import { RunEvent, SymbolizeResult } from '../src/nxdev/types';

suite('Run Stream and Symbolizer Tests', () => {
  test('Parses RunEvent stream messages', () => {
    const stepEvt: RunEvent = {
      event: 'step',
      step: 1,
      totalSteps: 4,
      name: 'Building ELF binary'
    };
    assert.strictEqual(stepEvt.event, 'step');
    assert.strictEqual(stepEvt.step, 1);
    assert.strictEqual(stepEvt.totalSteps, 4);

    const logEvt: RunEvent = {
      event: 'stdout',
      line: '[INFO] Initializing Horizon subsystems...'
    };
    assert.strictEqual(logEvt.event, 'stdout');
    assert.strictEqual(logEvt.line, '[INFO] Initializing Horizon subsystems...');

    const crashEvt: RunEvent = {
      event: 'crash',
      line: '[NXDEV-CRASH] 0x0000007100014230 0x0000007100015500',
      addresses: ['0x0000007100014230', '0x0000007100015500']
    };
    assert.strictEqual(crashEvt.event, 'crash');
    assert.strictEqual(crashEvt.addresses?.length, 2);
  });

  test('Parses SymbolizeResult structure', () => {
    const mockSym: SymbolizeResult = {
      elf: '/path/to/app.elf',
      symbols: [
        {
          address: '0x0000007100014230',
          function: 'main()',
          file: 'src/main.cpp',
          line: 42
        },
        {
          address: '0x0000007100015500',
          function: 'trigger_panic()',
          file: 'src/panic.cpp',
          line: 15
        }
      ]
    };

    assert.strictEqual(mockSym.symbols.length, 2);
    assert.strictEqual(mockSym.symbols[0].function, 'main()');
    assert.strictEqual(mockSym.symbols[0].line, 42);
    assert.strictEqual(mockSym.symbols[1].file, 'src/panic.cpp');
  });
});
