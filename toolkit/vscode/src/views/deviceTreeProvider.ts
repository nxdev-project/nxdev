import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';
import { Device } from '../nxdev/types';

export class DeviceTreeItem extends vscode.TreeItem {
  constructor(
    public readonly device: Device,
    public readonly collapsibleState: vscode.TreeItemCollapsibleState = vscode.TreeItemCollapsibleState.None
  ) {
    const label = device.is_default ? `${device.id} [default]` : device.id;
    super(label, collapsibleState);

    const hostDesc = `${device.host}:${device.port}${device.name ? ` • ${device.name}` : ''}`;
    this.description = hostDesc;
    this.contextValue = device.is_default ? 'deviceItem-default' : 'deviceItem';
    this.iconPath = new vscode.ThemeIcon(device.is_default ? 'star-full' : 'device-mobile');
    this.tooltip = `ID: ${device.id}\nHost: ${device.host}:${device.port}\nDefault: ${device.is_default ? 'Yes' : 'No'}${device.notes ? `\nNotes: ${device.notes}` : ''}`;
  }
}

export class DeviceTreeProvider implements vscode.TreeDataProvider<DeviceTreeItem> {
  private _onDidChangeTreeData: vscode.EventEmitter<DeviceTreeItem | undefined | void> =
    new vscode.EventEmitter<DeviceTreeItem | undefined | void>();
  readonly onDidChangeTreeData: vscode.Event<DeviceTreeItem | undefined | void> =
    this._onDidChangeTreeData.event;

  private projectManager = ProjectManager.getInstance();

  constructor() {
    this.projectManager.onProjectsRefreshed(() => this.refresh());
  }

  public refresh(): void {
    this._onDidChangeTreeData.fire();
  }

  public getTreeItem(element: DeviceTreeItem): vscode.TreeItem {
    return element;
  }

  public async getChildren(element?: DeviceTreeItem): Promise<DeviceTreeItem[]> {
    if (element) {
      return [];
    }

    const client = this.projectManager.getClient();
    const activeProject = this.projectManager.getActiveProject();
    const projectRoot = activeProject?.rootPath;

    try {
      const res = await client.listDevices(projectRoot);
      if (!res || !res.devices || res.devices.length === 0) {
        return [];
      }

      return res.devices.map(d => new DeviceTreeItem(d));
    } catch {
      return [];
    }
  }
}
