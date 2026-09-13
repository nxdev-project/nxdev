import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { ProjectManager } from '../project/manager';
import { OutputManager } from '../utils/output';

export class ProjectCommands {
  private static projectManager = ProjectManager.getInstance();
  private static output = OutputManager.getInstance();

  public static async openManifest(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to open manifest');
    if (!project) {return;}

    if (!fs.existsSync(project.manifestPath)) {
      vscode.window.showErrorMessage(`Manifest not found at ${project.manifestPath}`);
      return;
    }

    const doc = await vscode.workspace.openTextDocument(project.manifestPath);
    await vscode.window.showTextDocument(doc);
  }

  public static async validateManifest(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to validate manifest');
    if (!project) {return;}

    const client = this.projectManager.getClient();
    const res = await client.validateManifest(project.manifestPath);

    if (res?.valid) {
      vscode.window.showInformationMessage(`nxapp.yaml is valid (${project.info?.name || 'Project'})`);
    } else {
      const errCount = res?.diagnostics.filter(d => d.severity === 'error').length || 0;
      const warnCount = res?.diagnostics.filter(d => d.severity === 'warning').length || 0;
      vscode.window.showErrorMessage(`Manifest validation failed: ${errCount} error(s), ${warnCount} warning(s).`);
    }
  }

  public static async inspectManifest(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to inspect');
    if (!project) {return;}

    const client = this.projectManager.getClient();
    const info = await client.getProjectInfo(project.rootPath);
    if (!info) {
      vscode.window.showErrorMessage('Failed to inspect project manifest.');
      return;
    }

    const content = JSON.stringify(info, null, 2);
    const doc = await vscode.workspace.openTextDocument({
      content,
      language: 'json'
    });
    await vscode.window.showTextDocument(doc, { preview: true });
  }

  public static async selectProfile(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to change build profile');
    if (!project) {return;}

    const availableProfiles = project.info?.profiles && project.info.profiles.length > 0
      ? project.info.profiles
      : ['debug', 'release'];

    const items = availableProfiles.map(p => ({
      label: p,
      description: p === project.activeProfile ? '(Active)' : undefined
    }));

    const selected = await vscode.window.showQuickPick(items, {
      placeHolder: `Select active build profile for ${project.info?.name || project.folder.name}`
    });

    if (selected) {
      this.projectManager.setActiveProfile(project.rootPath, selected.label);
      this.output.info(`Active profile switched to '${selected.label}' for ${project.info?.name || project.folder.name}`);
      vscode.window.showInformationMessage(`Active profile set to '${selected.label}'`);
    }
  }

  public static async createProject(): Promise<void> {
    const folders = vscode.workspace.workspaceFolders;
    let targetDir: string | undefined;

    if (folders && folders.length === 1) {
      targetDir = folders[0].uri.fsPath;
    } else {
      const picked = await vscode.window.showOpenDialog({
        canSelectFiles: false,
        canSelectFolders: true,
        canSelectMany: false,
        openLabel: 'Select Project Directory'
      });
      if (picked && picked.length > 0) {
        targetDir = picked[0].fsPath;
      }
    }

    if (!targetDir) {return;}

    const appName = await vscode.window.showInputBox({
      prompt: 'Enter application display name',
      value: path.basename(targetDir) || 'MySwitchApp',
      validateInput: (v) => (!v || v.trim().length === 0 ? 'Name cannot be empty' : null)
    });

    if (!appName) {return;}

    const author = await vscode.window.showInputBox({
      prompt: 'Enter author / team name',
      value: 'Homebrew Developer'
    });

    if (!author) {return;}

    const manifestPath = path.join(targetDir, 'nxapp.yaml');
    if (fs.existsSync(manifestPath)) {
      const overwrite = await vscode.window.showWarningMessage(
        `nxapp.yaml already exists in ${targetDir}. Overwrite?`,
        'Overwrite',
        'Cancel'
      );
      if (overwrite !== 'Overwrite') {return;}
    }

    const defaultManifest = `schemaVersion: 1
target: switch

application:
  name: "${appName}"
  author: "${author}"
  version: "0.1.0"
  titleId: "0100000000001000"

build:
  defaultProfile: debug
  profiles:
    debug:
      type: debug
    release:
      type: release

assets:
  romfs: romfs/
`;

    fs.writeFileSync(manifestPath, defaultManifest, 'utf-8');
    this.output.info(`Created new NXDev manifest at ${manifestPath}`);

    await this.projectManager.refreshAll();
    const doc = await vscode.workspace.openTextDocument(manifestPath);
    await vscode.window.showTextDocument(doc);
    vscode.window.showInformationMessage(`Created NXDev project: ${appName}`);
  }
}
