import * as vscode from 'vscode';
import * as path from 'path';
import { ProjectManager } from '../project/manager';

export class ProjectTreeItem extends vscode.TreeItem {
  constructor(
    public readonly label: string,
    public readonly description?: string,
    public readonly collapsibleState: vscode.TreeItemCollapsibleState = vscode.TreeItemCollapsibleState.None,
    public readonly contextValue?: string,
    public readonly iconName?: string,
    public readonly command?: vscode.Command
  ) {
    super(label, collapsibleState);
    this.description = description;
    this.contextValue = contextValue;
    if (iconName) {
      this.iconPath = new vscode.ThemeIcon(iconName);
    }
  }
}

export class ProjectTreeProvider implements vscode.TreeDataProvider<ProjectTreeItem> {
  private _onDidChangeTreeData: vscode.EventEmitter<ProjectTreeItem | undefined | void> =
    new vscode.EventEmitter<ProjectTreeItem | undefined | void>();
  readonly onDidChangeTreeData: vscode.Event<ProjectTreeItem | undefined | void> =
    this._onDidChangeTreeData.event;

  private projectManager = ProjectManager.getInstance();

  constructor() {
    this.projectManager.onProjectsRefreshed(() => this.refresh());
    this.projectManager.onProjectChanged(() => this.refresh());
  }

  public refresh(): void {
    this._onDidChangeTreeData.fire();
  }

  public getTreeItem(element: ProjectTreeItem): vscode.TreeItem {
    return element;
  }

  public async getChildren(element?: ProjectTreeItem): Promise<ProjectTreeItem[]> {
    if (element) {
      return [];
    }

    const project = this.projectManager.getActiveProject();
    if (!project) {
      return [
        new ProjectTreeItem(
          'No NXDev project detected',
          undefined,
          vscode.TreeItemCollapsibleState.None,
          'welcome',
          'info'
        )
      ];
    }

    if (!project.hasManifest) {
      return [
        new ProjectTreeItem(
          'Missing nxapp.yaml',
          'No manifest found in folder',
          vscode.TreeItemCollapsibleState.None,
          'no-manifest',
          'warning'
        )
      ];
    }

    const items: ProjectTreeItem[] = [];
    const info = project.info;

    // 1. Application Name & ID
    items.push(
      new ProjectTreeItem(
        info?.name || path.basename(project.rootPath),
        info?.version ? `v${info.version}` : undefined,
        vscode.TreeItemCollapsibleState.None,
        'app-info',
        'gamepad'
      )
    );

    // 2. Author
    if (info?.author) {
      items.push(
        new ProjectTreeItem(
          'Author',
          info.author,
          vscode.TreeItemCollapsibleState.None,
          'author',
          'person'
        )
      );
    }

    // 3. Target Platform
    items.push(
      new ProjectTreeItem(
        'Target',
        info?.target || 'switch',
        vscode.TreeItemCollapsibleState.None,
        'target',
        'device-mobile'
      )
    );

    // 4. Active Build Profile
    items.push(
      new ProjectTreeItem(
        'Build Profile',
        project.activeProfile,
        vscode.TreeItemCollapsibleState.None,
        'profile',
        'gear',
        {
          command: 'nxdev.project.selectProfile',
          title: 'Switch Active Profile'
        }
      )
    );

    // 5. Title ID (if present)
    if (info?.titleId) {
      items.push(
        new ProjectTreeItem(
          'Title ID',
          info.titleId,
          vscode.TreeItemCollapsibleState.None,
          'titleId',
          'key'
        )
      );
    }

    // 6. Manifest Shortcut
    items.push(
      new ProjectTreeItem(
        'Manifest',
        'nxapp.yaml',
        vscode.TreeItemCollapsibleState.None,
        'manifest',
        'file-code',
        {
          command: 'nxdev.manifest.open',
          title: 'Open nxapp.yaml'
        }
      )
    );

    return items;
  }
}
