import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';
import { OutputManager } from '../utils/output';
import { DeviceTreeItem } from '../views/deviceTreeProvider';

export class DeviceCommands {
  private static projectManager = ProjectManager.getInstance();
  private static output = OutputManager.getInstance();

  public static async addDevice(): Promise<void> {
    const id = await vscode.window.showInputBox({
      prompt: 'Enter Device ID (e.g. oled-living-room, switch-desk)',
      placeHolder: 'switch-desk',
      validateInput: (val) => {
        if (!val || val.trim().length === 0) {
          return 'Device ID cannot be empty';
        }
        if (!/^[a-zA-Z0-9_-]+$/.test(val.trim())) {
          return 'Device ID may only contain alphanumeric characters, dashes, and underscores';
        }
        return null;
      }
    });
    if (!id) {return;}

    const host = await vscode.window.showInputBox({
      prompt: 'Enter Device IP or Hostname (e.g. 192.168.1.50, switch.local)',
      placeHolder: '192.168.1.50',
      validateInput: (val) => {
        if (!val || val.trim().length === 0) {
          return 'Host address cannot be empty';
        }
        return null;
      }
    });
    if (!host) {return;}

    const portStr = await vscode.window.showInputBox({
      prompt: 'Enter Port (default: 28280 for nxlink)',
      value: '28280',
      validateInput: (val) => {
        const num = parseInt(val, 10);
        if (isNaN(num) || num < 1 || num > 65535) {
          return 'Port must be between 1 and 65535';
        }
        return null;
      }
    });
    const port = portStr ? parseInt(portStr, 10) : 28280;

    const notes = await vscode.window.showInputBox({
      prompt: 'Optional notes/description',
      placeHolder: 'Nintendo Switch OLED on home Wi-Fi'
    });

    const isDefaultChoice = await vscode.window.showQuickPick(['Yes', 'No'], {
      placeHolder: 'Set as default device for run/deploy?'
    });
    const isDefault = isDefaultChoice === 'Yes';

    const client = this.projectManager.getClient();
    const activeProject = this.projectManager.getActiveProject();
    const projectRoot = activeProject?.rootPath;

    try {
      const res = await client.addDevice(
        {
          id: id.trim(),
          host: host.trim(),
          port,
          isDefault,
          notes: notes?.trim()
        },
        { projectRoot }
      );

      if (res.exitCode === 0) {
        this.output.info(`Added device '${id}' (${host}:${port})`);
        vscode.window.showInformationMessage(`Device '${id}' added successfully.`);
        this.projectManager.refreshAll();
      } else {
        this.output.error(`Failed to add device: ${res.stderr || res.stdout}`);
        vscode.window.showErrorMessage(`Failed to add device: ${res.stderr || res.stdout}`);
      }
    } catch (err) {
      this.output.error(`Failed to add device: ${err}`);
      vscode.window.showErrorMessage(`Failed to add device: ${err}`);
    }
  }

  public static async removeDevice(item?: DeviceTreeItem): Promise<void> {
    const client = this.projectManager.getClient();
    const activeProject = this.projectManager.getActiveProject();
    const projectRoot = activeProject?.rootPath;

    let targetId = item?.device.id;
    if (!targetId) {
      const list = await client.listDevices(projectRoot);
      if (!list || list.devices.length === 0) {
        vscode.window.showInformationMessage('No configured devices found.');
        return;
      }
      const pick = await vscode.window.showQuickPick(
        list.devices.map(d => ({
          label: d.id,
          description: `${d.host}:${d.port}`,
          device: d
        })),
        { placeHolder: 'Select device to remove' }
      );
      if (!pick) {return;}
      targetId = pick.device.id;
    }

    const confirm = await vscode.window.showWarningMessage(
      `Are you sure you want to remove device '${targetId}'?`,
      { modal: true },
      'Remove'
    );
    if (confirm !== 'Remove') {return;}

    try {
      const res = await client.removeDevice(targetId, { projectRoot });
      if (res.exitCode === 0) {
        this.output.info(`Removed device '${targetId}'`);
        vscode.window.showInformationMessage(`Device '${targetId}' removed.`);
        this.projectManager.refreshAll();
      } else {
        this.output.error(`Failed to remove device: ${res.stderr || res.stdout}`);
        vscode.window.showErrorMessage(`Failed to remove device: ${res.stderr || res.stdout}`);
      }
    } catch (err) {
      this.output.error(`Failed to remove device: ${err}`);
      vscode.window.showErrorMessage(`Failed to remove device: ${err}`);
    }
  }

  public static async setDefault(item?: DeviceTreeItem): Promise<void> {
    const client = this.projectManager.getClient();
    const activeProject = this.projectManager.getActiveProject();
    const projectRoot = activeProject?.rootPath;

    let targetId = item?.device.id;
    if (!targetId) {
      const list = await client.listDevices(projectRoot);
      if (!list || list.devices.length === 0) {
        vscode.window.showInformationMessage('No configured devices found.');
        return;
      }
      const pick = await vscode.window.showQuickPick(
        list.devices.map(d => ({
          label: d.id,
          description: `${d.host}:${d.port}`,
          device: d
        })),
        { placeHolder: 'Select device to set as default' }
      );
      if (!pick) {return;}
      targetId = pick.device.id;
    }

    try {
      const res = await client.setDefaultDevice(targetId, { projectRoot });
      if (res.exitCode === 0) {
        this.output.info(`Set default device to '${targetId}'`);
        vscode.window.showInformationMessage(`Default device set to '${targetId}'.`);
        this.projectManager.refreshAll();
      } else {
        this.output.error(`Failed to set default device: ${res.stderr || res.stdout}`);
        vscode.window.showErrorMessage(`Failed to set default device: ${res.stderr || res.stdout}`);
      }
    } catch (err) {
      this.output.error(`Failed to set default device: ${err}`);
      vscode.window.showErrorMessage(`Failed to set default device: ${err}`);
    }
  }

  public static async testDevice(item?: DeviceTreeItem): Promise<void> {
    const client = this.projectManager.getClient();
    const activeProject = this.projectManager.getActiveProject();
    const projectRoot = activeProject?.rootPath;

    let target = item?.device.id;
    if (!target) {
      const list = await client.listDevices(projectRoot);
      if (!list || list.devices.length === 0) {
        const rawHost = await vscode.window.showInputBox({
          prompt: 'Enter Device IP/Hostname to test connectivity',
          placeHolder: '192.168.1.50'
        });
        if (!rawHost) {return;}
        target = rawHost;
      } else {
        const pick = await vscode.window.showQuickPick(
          list.devices.map(d => ({
            label: d.id,
            description: `${d.host}:${d.port}`,
            device: d
          })),
          { placeHolder: 'Select device to test connectivity' }
        );
        if (!pick) {return;}
        target = pick.device.id;
      }
    }

    this.output.show();
    this.output.info(`Testing connectivity to '${target}'...`);

    await vscode.window.withProgress(
      {
        location: vscode.ProgressLocation.Notification,
        title: `Testing connection to ${target}...`,
        cancellable: false
      },
      async () => {
        const res = await client.testDevice(target, { projectRoot });
        if (res.exitCode === 0) {
          this.output.info(`Device test succeeded: ${res.stdout.trim()}`);
          vscode.window.showInformationMessage(`Connection to '${target}' succeeded!`);
        } else {
          this.output.error(`Device test failed: ${res.stderr || res.stdout}`);
          vscode.window.showErrorMessage(`Connection to '${target}' failed: ${res.stderr || res.stdout}`);
        }
      }
    );
  }
}
