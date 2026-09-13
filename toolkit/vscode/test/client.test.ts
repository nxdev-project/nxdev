import * as assert from 'assert';
import { NxDevClient } from '../src/nxdev/client';

suite('NxDevClient Tests', () => {
  test('NxDevClient instantiates properly', () => {
    const client = new NxDevClient();
    assert.ok(client);
  });
});
