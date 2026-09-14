import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';
import { OutputManager } from '../utils/output';

export class BuildCommands {
  private static projectManager = ProjectManager.getInstance();
  private static output = OutputManager.getInstance();

  public static async configure(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to configure');
    if (!project) {return;}

    if (project.operationState !== 'idle') {
      vscode.window.showWarningMessage(`Project is currently ${project.operationState}. Please wait.`);
      return;
    }

    this.projectManager.setOperationState(project.rootPath, 'configuring');
    this.output.show();
    this.output.info(`Configuring project: ${project.info?.name || project.folder.name}...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: `Configuring ${project.info?.name || 'Project'}...`,
          cancellable: true
        },
        async (_progress, token) => {
          const client = this.projectManager.getClient();
          const res = await client.configure(project.rootPath, {
            profile: project.activeProfile,
            cancellationToken: token
          });

          if (res.exitCode === 0) {
            this.output.info('Configuration completed successfully.');
            vscode.window.showInformationMessage(`Configured ${project.info?.name || 'Project'} (${project.activeProfile})`);
          } else {
            this.output.error(`Configuration failed with exit code ${res.exitCode}`);
            vscode.window.showErrorMessage(`Configuration failed. See NXDev Output channel for details.`);
          }
        }
      );
    } finally {
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }

  public static async build(profileOverride?: string): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to build');
    if (!project) {return;}

    if (project.operationState !== 'idle') {
      vscode.window.showWarningMessage(`Project is currently ${project.operationState}. Please wait.`);
      return;
    }

    const profile = profileOverride || project.activeProfile;
    this.projectManager.setOperationState(project.rootPath, 'building');
    this.output.show();
    this.output.info(`Building ${project.info?.name || project.folder.name} [${profile}]...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: `Building ${project.info?.name || 'Project'} (${profile})...`,
          cancellable: true
        },
        async (_progress, token) => {
          const client = this.projectManager.getClient();
          const { result, json } = await client.build(project.rootPath, {
            profile,
            cancellationToken: token
          });

          if (result.cancelled) {
            this.output.warn('Build cancelled by user.');
            vscode.window.showWarningMessage('Build cancelled.');
            return;
          }

          if (result.exitCode === 0 && (!json || json.status === 'success')) {
            const artifact = json?.artifact || 'Switch ELF binary';
            this.output.info(`Build successful! Artifact: ${artifact}`);
            vscode.window.showInformationMessage(`Build succeeded for ${project.info?.name || 'Project'} [${profile}]`);
          } else {
            if (json?.resourceLimited) {
              const peakMb = json.peakMemoryBytes ? `${(json.peakMemoryBytes / (1024 * 1024)).toFixed(1)} MiB` : 'unknown';
              this.output.error(`Build stopped to protect WSL from memory exhaustion (Peak memory: ${peakMb}).`);
              vscode.window.showErrorMessage(
                `Build stopped to protect WSL from memory exhaustion. Peak memory: ${peakMb}`,
                'Open Build Log',
                'Open Resource Settings'
              ).then((action) => {
                if (action === 'Open Build Log') {
                  this.output.show();
                } else if (action === 'Open Resource Settings') {
                  vscode.commands.executeCommand('workbench.action.openSettings', 'nxdev');
                }
              });
            } else {
              const errMsg = json?.error?.message || (json?.diagnostics && json.diagnostics.length > 0 ? json.diagnostics[0] : 'Compilation failed');
              this.output.error(`Build failed: ${errMsg}`);
              vscode.window.showErrorMessage(`Build failed. See NXDev Output for details.`);
            }
          }
        }
      );
    } finally {
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }

  public static async buildDebug(): Promise<void> {
    return this.build('debug');
  }

  public static async buildRelease(): Promise<void> {
    return this.build('release');
  }

  public static async clean(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to clean');
    if (!project) {return;}

    if (project.operationState !== 'idle') {
      vscode.window.showWarningMessage(`Project is currently ${project.operationState}. Please wait.`);
      return;
    }

    this.projectManager.setOperationState(project.rootPath, 'cleaning');
    this.output.show();
    this.output.info(`Cleaning build directory for ${project.info?.name || project.folder.name}...`);

    try {
      const client = this.projectManager.getClient();
      const res = await client.clean(project.rootPath, { profile: project.activeProfile });
      if (res.exitCode === 0) {
        this.output.info('Clean completed successfully.');
        vscode.window.showInformationMessage(`Cleaned build artifacts for ${project.info?.name || 'Project'}`);
      } else {
        this.output.error(`Clean failed with exit code ${res.exitCode}`);
        vscode.window.showErrorMessage(`Clean failed.`);
      }
    } finally {
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }
}
