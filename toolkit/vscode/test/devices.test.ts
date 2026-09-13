import * as assert from 'assert';
import { DeviceListResult, DeployResult } from '../src/nxdev/types';

suite('Devices and Deploy Tests', () => {
  test('Parses device list structure properly', () => {
    const mockList: DeviceListResult = {
      devices: [
        {
          id: 'switch-living-room',
          name: 'Living Room OLED',
          host: '192.168.1.150',
          port: 28280,
          is_default: true,
          notes: 'Connected via 5GHz Wi-Fi'
        },
        {
          id: 'switch-desk',
          name: 'Desk Switch Lite',
          host: 'switch-lite.local',
          port: 28280,
          is_default: false
        }
      ]
    };

    assert.strictEqual(mockList.devices.length, 2);
    const defaultDev = mockList.devices.find(d => d.is_default);
    assert.ok(defaultDev);
    assert.strictEqual(defaultDev?.id, 'switch-living-room');
    assert.strictEqual(defaultDev?.port, 28280);
  });

  test('Validates deploy result format', () => {
    const mockDeploy: DeployResult = {
      status: 'success',
      artifact: '/path/to/demo.nro',
      fileSizeBytes: 1048576,
      device: {
        id: 'switch-desk',
        host: '192.168.1.50',
        port: 28280,
        is_default: true
      }
    };

    assert.strictEqual(mockDeploy.status, 'success');
    assert.strictEqual(mockDeploy.device?.host, '192.168.1.50');
    assert.strictEqual(mockDeploy.fileSizeBytes, 1048576);
  });
});
