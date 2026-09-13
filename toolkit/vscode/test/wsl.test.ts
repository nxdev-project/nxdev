import * as assert from 'assert';
import { WslBridge } from '../src/nxdev/wsl';

suite('WslBridge Tests', () => {
  test('Translates Windows drive path to WSL path', () => {
    assert.strictEqual(
      WslBridge.windowsToWslPath('C:\\Users\\developer\\Projects\\myapp'),
      '/mnt/c/Users/developer/Projects/myapp'
    );
    assert.strictEqual(
      WslBridge.windowsToWslPath('D:/workspace/switch/game'),
      '/mnt/d/workspace/switch/game'
    );
  });

  test('Translates WSL path to Windows path', () => {
    assert.strictEqual(
      WslBridge.wslToWindowsPath('/mnt/c/Users/developer/Projects/myapp'),
      'C:\\Users\\developer\\Projects\\myapp'
    );
    assert.strictEqual(
      WslBridge.wslToWindowsPath('/mnt/d/workspace/switch/game'),
      'D:\\workspace\\switch\\game'
    );
  });

  test('Preserves relative or non-drive paths', () => {
    assert.strictEqual(WslBridge.windowsToWslPath('relative/path'), 'relative/path');
    assert.strictEqual(WslBridge.wslToWindowsPath('relative/path'), 'relative/path');
  });

  test('Wraps arguments for WSL execution', () => {
    const wrappedDefault = WslBridge.wrapWslCommand('nxdev', ['doctor', '--json'], '');
    assert.strictEqual(wrappedDefault.command, 'wsl.exe');
    assert.deepStrictEqual(wrappedDefault.args, ['--', 'nxdev', 'doctor', '--json']);

    const wrappedDistro = WslBridge.wrapWslCommand('nxdev', ['build', '--profile', 'release'], 'Ubuntu-22.04');
    assert.strictEqual(wrappedDistro.command, 'wsl.exe');
    assert.deepStrictEqual(wrappedDistro.args, ['-d', 'Ubuntu-22.04', '--', 'nxdev', 'build', '--profile', 'release']);
  });
});
