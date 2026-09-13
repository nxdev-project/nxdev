import * as vscode from 'vscode';
import * as fs from 'fs';
import { ProjectManager } from '../project/manager';
import { OutputManager } from '../utils/output';
import { SdkModuleTreeItem } from '../views/sdkModulesTreeProvider';

export class PackageCommands {
  private static projectManager = ProjectManager.getInstance();
  private static output = OutputManager.getInstance();

  public static async installMissing(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to install dependencies for');
    if (!project) {return;}

    if (project.operationState !== 'idle') {
      vscode.window.showWarningMessage(`Project is currently ${project.operationState}. Please wait.`);
      return;
    }

    const client = this.projectManager.getClient();
    const status = await client.getPackageStatus(project.rootPath);
    if (!status || status.missing.length === 0) {
      vscode.window.showInformationMessage('All project dependencies are already installed.');
      return;
    }

    const missingNames = status.missing.map(m => m.name || m.id).join(', ');
    const confirm = await vscode.window.showInformationMessage(
      `Install missing packages (${missingNames})?`,
      'Install',
      'Cancel'
    );
    if (confirm !== 'Install') {return;}

    this.projectManager.setOperationState(project.rootPath, 'installing');
    this.output.show();
    this.output.info(`Installing missing dependencies for ${project.info?.name || project.folder.name}...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: 'Installing missing SDK packages...',
          cancellable: false
        },
        async () => {
          const res = await client.installMissingPackages(project.rootPath);
          if (res.exitCode === 0) {
            this.output.info('Packages installed successfully.');
            vscode.window.showInformationMessage('Dependencies installed successfully.');
            await this.projectManager.refreshAll();
          } else {
            this.output.error(`Package installation failed with exit code ${res.exitCode}`);
            vscode.window.showErrorMessage('Package installation failed. See Output channel for details.');
          }
        }
      );
    } finally {
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }

  public static async installPackage(item?: SdkModuleTreeItem | string): Promise<void> {
    let pkgId: string | undefined;

    if (typeof item === 'string') {
      pkgId = item;
    } else if (item?.pkg) {
      pkgId = item.pkg.id;
    } else {
      const client = this.projectManager.getClient();
      const list = await client.listPackages();
      if (!list || list.packages.length === 0) {
        vscode.window.showErrorMessage('No SDK packages available in registry.');
        return;
      }
      const picked = await vscode.window.showQuickPick(
        list.packages.filter(p => !p.installed).map(p => ({
          label: p.name,
          description: p.id,
          detail: p.description,
          pkgId: p.id
        })),
        { placeHolder: 'Select SDK package to install' }
      );
      if (picked) {
        pkgId = picked.pkgId;
      }
    }

    if (!pkgId) {return;}

    const confirm = await vscode.window.showInformationMessage(
      `Install package '${pkgId}'?`,
      'Install',
      'Cancel'
    );
    if (confirm !== 'Install') {return;}

    this.output.show();
    this.output.info(`Installing package '${pkgId}'...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: `Installing ${pkgId}...`,
          cancellable: false
        },
        async () => {
          const client = this.projectManager.getClient();
          const res = await client.installPackage(pkgId);
          if (res.exitCode === 0) {
            this.output.info(`Installed ${pkgId} successfully.`);
            vscode.window.showInformationMessage(`Package '${pkgId}' installed successfully.`);
            await this.projectManager.refreshAll();
          } else {
            this.output.error(`Failed to install ${pkgId}: exit code ${res.exitCode}`);
            vscode.window.showErrorMessage(`Failed to install '${pkgId}'.`);
          }
        }
      );
    } catch (err) {
      this.output.error(`Install error: ${err}`);
    }
  }

  public static async removePackage(item?: SdkModuleTreeItem | string): Promise<void> {
    let pkgId: string | undefined;

    if (typeof item === 'string') {
      pkgId = item;
    } else if (item?.pkg) {
      if (item.pkg.type === 'builtin') {
        vscode.window.showWarningMessage(`Built-in SDK module '${item.pkg.name}' cannot be uninstalled.`);
        return;
      }
      pkgId = item.pkg.id;
    }

    if (!pkgId) {return;}

    const confirm = await vscode.window.showWarningMessage(
      `Uninstall devkitPro portlib package '${pkgId}'?`,
      'Uninstall',
      'Cancel'
    );
    if (confirm !== 'Uninstall') {return;}

    this.output.show();
    this.output.info(`Removing package '${pkgId}'...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: `Removing ${pkgId}...`,
          cancellable: false
        },
        async () => {
          const client = this.projectManager.getClient();
          const res = await client.removePackage(pkgId);
          if (res.exitCode === 0) {
            this.output.info(`Removed ${pkgId} successfully.`);
            vscode.window.showInformationMessage(`Package '${pkgId}' removed.`);
            await this.projectManager.refreshAll();
          } else {
            this.output.error(`Failed to remove ${pkgId}: exit code ${res.exitCode}`);
            vscode.window.showErrorMessage(`Failed to remove '${pkgId}'.`);
          }
        }
      );
    } catch (err) {
      this.output.error(`Remove error: ${err}`);
    }
  }

  public static async addDependency(item?: SdkModuleTreeItem): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to add dependency to');
    if (!project) {return;}

    let pkgId: string | undefined;
    if (item?.pkg) {
      pkgId = item.pkg.id;
    } else {
      const client = this.projectManager.getClient();
      const list = await client.listPackages();
      if (!list || list.packages.length === 0) {return;}

      const currentDeps = new Set(project.info?.dependencies || []);
      const available = list.packages.filter(p => !currentDeps.has(p.id));

      const picked = await vscode.window.showQuickPick(
        available.map(p => ({
          label: p.name,
          description: p.id,
          detail: p.description,
          pkgId: p.id
        })),
        { placeHolder: 'Select SDK module / portlib to add to nxapp.yaml' }
      );
      if (picked) {pkgId = picked.pkgId;}
    }

    if (!pkgId) {return;}

    const manifestContent = fs.readFileSync(project.manifestPath, 'utf-8');
    if (manifestContent.includes(pkgId)) {
      vscode.window.showInformationMessage(`'${pkgId}' is already declared in nxapp.yaml.`);
      return;
    }

    // Safely append dependency to manifest
    let updatedContent = manifestContent;
    if (updatedContent.includes('dependencies:')) {
      updatedContent = updatedContent.replace(
        /dependencies:(\r?\n)/,
        `dependencies:$1  - ${pkgId}$1`
      );
    } else {
      updatedContent += `\ndependencies:\n  - ${pkgId}\n`;
    }

    fs.writeFileSync(project.manifestPath, updatedContent, 'utf-8');
    this.output.info(`Added dependency '${pkgId}' to ${project.manifestPath}`);
    vscode.window.showInformationMessage(`Added '${pkgId}' to nxapp.yaml`);

    await this.projectManager.refreshAll();
  }
}
