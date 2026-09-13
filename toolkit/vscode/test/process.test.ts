import * as assert from 'assert';
import { ProcessRunner } from '../src/nxdev/process';

suite('ProcessRunner Tests', () => {
  test('Parses clean JSON output', () => {
    const jsonText = JSON.stringify({
      schemaVersion: 1,
      target: 'switch',
      application: {
        name: 'SuperDemo',
        titleId: '0100000000001000',
        version: '1.0.0',
        author: 'Developer'
      }
    });

    const parsed = ProcessRunner.parseJson<any>(jsonText);
    assert.strictEqual(parsed.success, true);
    assert.strictEqual(parsed.data?.application?.name, 'SuperDemo');
  });

  test('Extracts JSON from mixed stdout with prefixes', () => {
    const rawOutput = `
[info] Connecting to daemon...
[info] Running manifest validation...
{
  "valid": true,
  "errors": [],
  "warnings": ["Deprecated field"]
}
`;
    const parsed = ProcessRunner.parseJson<any>(rawOutput);
    assert.strictEqual(parsed.success, true);
    assert.strictEqual(parsed.data?.valid, true);
    assert.strictEqual(parsed.data?.warnings?.length, 1);
  });

  test('Gracefully handles invalid JSON string', () => {
    const badOutput = 'Error: could not find devkitPro installation';
    const parsed = ProcessRunner.parseJson<any>(badOutput);
    assert.strictEqual(parsed.success, false);
    assert.ok(parsed.error?.includes('No JSON object found'));
  });

  test('Gracefully handles truncated JSON', () => {
    const malformed = '{"valid": true, "errors": [';
    const parsed = ProcessRunner.parseJson<any>(malformed);
    assert.strictEqual(parsed.success, false);
    assert.ok(parsed.error && parsed.error.length > 0);
  });
});
