import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';

export class NxStatusBar {
  private statusBarItem: vscode.StatusBarItem;
  private projectManager = ProjectManager.getInstance();

  constructor() {
    this.statusBarItem = vscode.window.createStatusBarItem(
      vscode.StatusBarAlignment.Left,
      100
    );
    this.statusBarItem.command = 'nxdev.showMenu';
  }

  public register(context: vscode.ExtensionContext): void {
    context.subscriptions.push(this.statusBarItem);

    // Subscribe to project changes
    context.subscriptions.push(
      this.projectManager.onProjectChanged(() => this.update()),
      this.projectManager.onProjectsRefreshed(() => this.update())
    );

    this.update();
  }

  public update(): void {
    const config = vscode.workspace.getConfiguration('nxdev');
    if (!config.get<boolean>('showStatusBar', true)) {
      this.statusBarItem.hide();
      return;
    }

    const project = this.projectManager.getActiveProject();

    if (!project) {
      this.statusBarItem.text = '$(circle-slash) NXDev • No Project';
      this.statusBarItem.tooltip = 'NXDev: No active Nintendo Switch project detected in workspace.';
      this.statusBarItem.show();
      return;
    }

    if (!project.hasManifest) {
      this.statusBarItem.text = '$(circle-slash) NXDev • No Project';
      this.statusBarItem.tooltip = `Workspace folder '${project.folder.name}' does not contain an nxapp.yaml manifest.`;
      this.statusBarItem.show();
      return;
    }

    if (project.operationState === 'building') {
      this.statusBarItem.text = `$(sync~spin) NXDev • Building (${project.activeProfile})`;
      this.statusBarItem.tooltip = `Building ${project.info?.name || 'Project'} [${project.activeProfile}]... Click for actions.`;
      this.statusBarItem.show();
      return;
    }

    if (project.operationState === 'packaging') {
      this.statusBarItem.text = `$(package) NXDev • Packaging (${project.activeProfile})`;
      this.statusBarItem.tooltip = `Packaging ${project.info?.name || 'Project'}... Click for actions.`;
      this.statusBarItem.show();
      return;
    }

    if (project.operationState === 'installing') {
      this.statusBarItem.text = `$(cloud-download) NXDev • Installing SDK`;
      this.statusBarItem.tooltip = `Installing dependencies... Click for actions.`;
      this.statusBarItem.show();
      return;
    }

    if (project.operationState === 'deploying') {
      this.statusBarItem.text = `$(cloud-upload) NXDev • Deploying NRO`;
      this.statusBarItem.tooltip = `Deploying to Switch... Click for actions.`;
      this.statusBarItem.show();
      return;
    }

    if (project.operationState === 'running') {
      this.statusBarItem.text = `$(play) NXDev • Running on Switch`;
      this.statusBarItem.tooltip = `Streaming runtime logs from Switch... Click to open NXDev menu or stop.`;
      this.statusBarItem.show();
      return;
    }

    if (!project.isValid) {
      this.statusBarItem.text = `$(warning) NXDev • Manifest Error`;
      this.statusBarItem.tooltip = `Project manifest nxapp.yaml has validation errors. Click to inspect.`;
      this.statusBarItem.show();
      return;
    }

    // Ready state
    const profileTitle = project.activeProfile.charAt(0).toUpperCase() + project.activeProfile.slice(1);
    this.statusBarItem.text = `$(gamepad) NXDev • ${profileTitle}`;
    this.statusBarItem.tooltip = `Application: ${project.info?.name || 'Switch Project'}\nTarget: ${project.info?.target || 'switch'}\nProfile: ${project.activeProfile}\nClick to open NXDev menu.`;
    this.statusBarItem.show();
  }

  public dispose(): void {
    this.statusBarItem.dispose();
  }
}
