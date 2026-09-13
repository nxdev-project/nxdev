import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';

export class EnvironmentTreeItem extends vscode.TreeItem {
  constructor(
    public readonly label: string,
    public readonly description?: string,
    public readonly collapsibleState: vscode.TreeItemCollapsibleState = vscode.TreeItemCollapsibleState.None,
    public readonly contextValue?: string,
    public readonly iconName?: string,
    public readonly tooltipText?: string
  ) {
    super(label, collapsibleState);
    this.description = description;
    this.contextValue = contextValue;
    if (iconName) {
      this.iconPath = new vscode.ThemeIcon(iconName);
    }
    if (tooltipText) {
      this.tooltip = tooltipText;
    }
  }
}

export class EnvironmentTreeProvider implements vscode.TreeDataProvider<EnvironmentTreeItem> {
  private _onDidChangeTreeData: vscode.EventEmitter<EnvironmentTreeItem | undefined | void> =
    new vscode.EventEmitter<EnvironmentTreeItem | undefined | void>();
  readonly onDidChangeTreeData: vscode.Event<EnvironmentTreeItem | undefined | void> =
    this._onDidChangeTreeData.event;

  private projectManager = ProjectManager.getInstance();

  constructor() {
    this.projectManager.onProjectsRefreshed(() => this.refresh());
  }

  public refresh(): void {
    this._onDidChangeTreeData.fire();
  }

  public getTreeItem(element: EnvironmentTreeItem): vscode.TreeItem {
    return element;
  }

  public async getChildren(element?: EnvironmentTreeItem): Promise<EnvironmentTreeItem[]> {
    if (element) {
      return [];
    }

    const env = await this.projectManager.getEnvironment();
    const doctor = await this.projectManager.getDoctor();

    const items: EnvironmentTreeItem[] = [];

    // 1. Host Platform
    if (env?.host) {
      const hostDesc = `${env.host.os} (${env.host.arch})${env.host.is_wsl ? ' [WSL]' : ''}`;
      items.push(
        new EnvironmentTreeItem(
          'Host Platform',
          hostDesc,
          vscode.TreeItemCollapsibleState.None,
          'host',
          'server'
        )
      );
    }

    // 2. devkitPro
    if (env?.devkitpro) {
      const isOk = env.devkitpro.is_valid;
      items.push(
        new EnvironmentTreeItem(
          'devkitPro',
          isOk ? env.devkitpro.path : 'Not configured / missing',
          vscode.TreeItemCollapsibleState.None,
          'devkitpro',
          isOk ? 'check' : 'error',
          `Source: ${env.devkitpro.source}`
        )
      );
    }

    // 3. devkitA64 (AArch64 Toolchain)
    if (env?.devkita64) {
      const isOk = env.devkita64.is_valid;
      items.push(
        new EnvironmentTreeItem(
          'devkitA64',
          isOk ? env.devkita64.path : 'Not configured / missing',
          vscode.TreeItemCollapsibleState.None,
          'devkita64',
          isOk ? 'check' : 'error',
          `Source: ${env.devkita64.source}`
        )
      );
    }

    // 4. libnx
    if (env?.libnx) {
      const isOk = env.libnx.is_valid;
      items.push(
        new EnvironmentTreeItem(
          'libnx',
          isOk ? env.libnx.path : 'Not configured / missing',
          vscode.TreeItemCollapsibleState.None,
          'libnx',
          isOk ? 'check' : 'error',
          `Source: ${env.libnx.source}`
        )
      );
    }

    // 5. switch-tools
    if (env?.switch_tools) {
      const isOk = env.switch_tools.is_valid;
      items.push(
        new EnvironmentTreeItem(
          'switch-tools',
          isOk ? env.switch_tools.path : 'Not configured / missing',
          vscode.TreeItemCollapsibleState.None,
          'switch-tools',
          isOk ? 'check' : 'error',
          `Source: ${env.switch_tools.source}`
        )
      );
    }

    // 6. Doctor Checks Summary
    if (doctor) {
      const failed = doctor.summary.failed;
      const warnings = doctor.summary.warnings;
      const passed = doctor.summary.passed;
      const docLabel = `Doctor: ${passed} passed, ${failed} failed, ${warnings} warnings`;
      const icon = failed > 0 ? 'error' : warnings > 0 ? 'warning' : 'pass';
      items.push(
        new EnvironmentTreeItem(
          'Diagnostics',
          docLabel,
          vscode.TreeItemCollapsibleState.None,
          'doctor',
          icon
        )
      );
    }

    return items;
  }
}
