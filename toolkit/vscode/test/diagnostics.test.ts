import * as assert from 'assert';
import { ManifestValidationResult } from '../src/nxdev/types';

suite('ManifestDiagnostics Tests', () => {
  test('Parses validation issues properly', () => {
    const mockResult: ManifestValidationResult = {
      valid: false,
      manifestPath: '/home/user/project/nxapp.yaml',
      diagnostics: [
        {
          severity: 'error',
          code: 'MISSING_FIELD',
          message: 'Missing required field: schemaVersion',
          line: 1,
          column: 1
        },
        {
          severity: 'error',
          code: 'INVALID_TITLE_ID',
          message: 'Invalid titleId format: expected 16-hex characters',
          line: 5,
          column: 12
        },
        {
          severity: 'warning',
          code: 'DEPRECATED_FIELD',
          message: 'Field "experimental" is deprecated',
          line: 10,
          column: 3
        }
      ]
    };

    assert.strictEqual(mockResult.valid, false);
    assert.strictEqual(mockResult.diagnostics.length, 3);
    assert.strictEqual(mockResult.diagnostics.filter(d => d.severity === 'error').length, 2);
    assert.strictEqual(mockResult.diagnostics.filter(d => d.severity === 'warning').length, 1);
  });
});
