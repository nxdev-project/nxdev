import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';
import { PackageInfo, PackageListResult, PackageStatusResult } from '../nxdev/types';

export class SdkModuleTreeItem extends vscode.TreeItem {
  constructor(
    public readonly label: string,
    public readonly description?: string,
    public readonly collapsibleState: vscode.TreeItemCollapsibleState = vscode.TreeItemCollapsibleState.None,
    public readonly pkg?: PackageInfo,
    public readonly isCategory = false,
    public readonly contextValue?: string,
    public readonly iconName?: string
  ) {
    super(label, collapsibleState);
    this.description = description;
    this.contextValue = contextValue;
    if (iconName) {
      this.iconPath = new vscode.ThemeIcon(iconName);
    }
  }
}

export class SdkModulesTreeProvider implements vscode.TreeDataProvider<SdkModuleTreeItem> {
  private _onDidChangeTreeData: vscode.EventEmitter<SdkModuleTreeItem | undefined | void> =
    new vscode.EventEmitter<SdkModuleTreeItem | undefined | void>();
  readonly onDidChangeTreeData: vscode.Event<SdkModuleTreeItem | undefined | void> =
    this._onDidChangeTreeData.event;

  private projectManager = ProjectManager.getInstance();

  constructor() {
    this.projectManager.onProjectsRefreshed(() => this.refresh());
    this.projectManager.onProjectChanged(() => this.refresh());
  }

  public refresh(): void {
    this._onDidChangeTreeData.fire();
  }

  public getTreeItem(element: SdkModuleTreeItem): vscode.TreeItem {
    return element;
  }

  public async getChildren(element?: SdkModuleTreeItem): Promise<SdkModuleTreeItem[]> {
    const project = this.projectManager.getActiveProject();
    const client = this.projectManager.getClient();

    let listRes: PackageListResult | null = null;
    let statusRes: PackageStatusResult | null = null;

    try {
      listRes = await client.listPackages();
      if (project?.hasManifest) {
        statusRes = await client.getPackageStatus(project.rootPath);
      }
    } catch {
      // Fallback
    }

    if (!listRes || !listRes.packages || listRes.packages.length === 0) {
      return [
        new SdkModuleTreeItem(
          'No SDK packages available',
          undefined,
          vscode.TreeItemCollapsibleState.None,
          undefined,
          false,
          'none',
          'info'
        )
      ];
    }

    const declaredSet = new Set<string>(statusRes?.declared || project?.info?.dependencies || []);

    // Root level: return categories
    if (!element) {
      const categoriesMap = new Map<string, number>();
      for (const p of listRes.packages) {
        const cat = p.category || (p.type === 'builtin' ? 'Built-in SDK' : 'General');
        categoriesMap.set(cat, (categoriesMap.get(cat) || 0) + 1);
      }

      const categoryItems: SdkModuleTreeItem[] = [];
      for (const [cat, count] of categoriesMap.entries()) {
        categoryItems.push(
          new SdkModuleTreeItem(
            cat,
            `(${count})`,
            vscode.TreeItemCollapsibleState.Expanded,
            undefined,
            true,
            'category',
            'symbol-namespace'
          )
        );
      }
      return categoryItems;
    }

    // Category children: return modules belonging to category
    if (element.isCategory) {
      const categoryName = element.label;
      const matched = listRes.packages.filter(
        p => (p.category || (p.type === 'builtin' ? 'Built-in SDK' : 'General')) === categoryName
      );

      return matched.map(p => {
        const isDeclared = declaredSet.has(p.id);
        const isBuiltin = p.type === 'builtin';
        const isInstalled = p.installed;

        let icon = 'circle-outline';
        let desc = isBuiltin ? 'Built-in' : isInstalled ? 'Installed' : 'Available';
        let contextVal = isBuiltin ? 'module-builtin' : isInstalled ? 'module-installed' : 'module-missing';

        if (isDeclared) {
          desc += ' • In Project';
          icon = isInstalled ? 'check' : 'warning';
        } else if (isInstalled) {
          icon = 'pass';
        }

        const item = new SdkModuleTreeItem(
          p.name || p.id,
          desc,
          vscode.TreeItemCollapsibleState.None,
          p,
          false,
          contextVal,
          icon
        );
        item.tooltip = `${p.name} (${p.id})\n${p.description || ''}\nType: ${p.type}\nInstalled: ${isInstalled ? 'Yes' : 'No'}`;
        return item;
      });
    }

    return [];
  }
}
