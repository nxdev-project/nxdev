import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { ProjectManager } from '../project/manager';
import { OutputManager } from '../utils/output';

export class PackCommands {
  private static projectManager = ProjectManager.getInstance();
  private static output = OutputManager.getInstance();

  public static async packageNro(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to package as NRO');
    if (!project) {return;}

    if (project.operationState !== 'idle') {
      vscode.window.showWarningMessage(`Project is currently ${project.operationState}. Please wait.`);
      return;
    }

    this.projectManager.setOperationState(project.rootPath, 'packaging');
    this.output.show();
    this.output.info(`Packaging NRO for ${project.info?.name || project.folder.name} [${project.activeProfile}]...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: `Packaging ${project.info?.name || 'Project'} (.nro)...`,
          cancellable: true
        },
        async (_progress, token) => {
          const client = this.projectManager.getClient();
          const { result, json } = await client.packNro(project.rootPath, {
            profile: project.activeProfile,
            cancellationToken: token
          });

          if (result.cancelled) {
            this.output.warn('Packaging cancelled by user.');
            return;
          }

          if (result.exitCode === 0 && (!json || json.status === 'success')) {
            const artifactPath = json?.artifact || path.join(project.rootPath, 'dist', project.activeProfile);
            this.output.info(`NRO packaged successfully: ${artifactPath}`);
            const action = await vscode.window.showInformationMessage(
              `NRO packaged successfully!`,
              'Reveal in File Explorer',
              'Copy Path'
            );
            if (action === 'Reveal in File Explorer') {
              if (fs.existsSync(artifactPath)) {
                vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(artifactPath));
              }
            } else if (action === 'Copy Path') {
              await vscode.env.clipboard.writeText(artifactPath);
              vscode.window.showInformationMessage('Artifact path copied to clipboard.');
            }
          } else {
            const errMsg = json?.error?.message || 'NRO packaging failed';
            this.output.error(`NRO packaging failed: ${errMsg}`);
            vscode.window.showErrorMessage(`NRO packaging failed: ${errMsg}`);
          }
        }
      );
    } finally {
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }

  public static async packageNsp(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to package as NSP');
    if (!project) {return;}

    if (project.operationState !== 'idle') {
      vscode.window.showWarningMessage(`Project is currently ${project.operationState}. Please wait.`);
      return;
    }

    // Check if title ID is configured
    if (!project.info?.titleId) {
      const open = await vscode.window.showErrorMessage(
        'NSP packaging requires a 16-hex Title ID configured in application.titleId (nxapp.yaml).',
        'Open Manifest'
      );
      if (open === 'Open Manifest') {
        const doc = await vscode.workspace.openTextDocument(project.manifestPath);
        await vscode.window.showTextDocument(doc);
      }
      return;
    }

    this.projectManager.setOperationState(project.rootPath, 'packaging');
    this.output.show();
    this.output.info(`Packaging NSP for ${project.info?.name || project.folder.name} [${project.activeProfile}]...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: `Packaging ${project.info?.name || 'Project'} (.nsp)...`,
          cancellable: true
        },
        async (_progress, token) => {
          const client = this.projectManager.getClient();
          const { result, json } = await client.packNsp(project.rootPath, {
            profile: project.activeProfile,
            cancellationToken: token
          });

          if (result.cancelled) {
            this.output.warn('Packaging cancelled by user.');
            return;
          }

          if (result.exitCode === 0 && (!json || json.status === 'success')) {
            const artifactPath = json?.artifact || path.join(project.rootPath, 'dist', project.activeProfile);
            this.output.info(`NSP packaged successfully: ${artifactPath}`);
            const action = await vscode.window.showInformationMessage(
              `NSP packaged successfully!`,
              'Reveal in File Explorer',
              'Copy Path'
            );
            if (action === 'Reveal in File Explorer') {
              if (fs.existsSync(artifactPath)) {
                vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(artifactPath));
              }
            } else if (action === 'Copy Path') {
              await vscode.env.clipboard.writeText(artifactPath);
              vscode.window.showInformationMessage('Artifact path copied to clipboard.');
            }
          } else {
            const errCode = json?.error?.code;
            const errMsg = json?.error?.message || 'NSP packaging failed';

            if (errCode === 'KeyFileMissing') {
              this.output.error(`NSP packaging requires Switch key file: ${errMsg}`);
              const pickKey = await vscode.window.showErrorMessage(
                'NSP packaging requires a user-provided Switch key file (prod.keys).',
                'Select Key File',
                'Learn More'
              );
              if (pickKey === 'Select Key File') {
                const picked = await vscode.window.showOpenDialog({
                  canSelectFiles: true,
                  canSelectFolders: false,
                  canSelectMany: false,
                  openLabel: 'Select prod.keys'
                });
                if (picked && picked.length > 0) {
                  // Retry pack with explicit key path
                  await this.packageNspWithKey(project, picked[0].fsPath);
                }
              } else if (pickKey === 'Learn More') {
                vscode.env.openExternal(vscode.Uri.parse('https://github.com/nxdev-project/nxdev/blob/main/docs/pack/keys.md'));
              }
            } else {
              this.output.error(`NSP packaging failed: ${errMsg}`);
              vscode.window.showErrorMessage(`NSP packaging failed: ${errMsg}`);
            }
          }
        }
      );
    } finally {
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }

  private static async packageNspWithKey(project: any, keyPath: string): Promise<void> {
    this.projectManager.setOperationState(project.rootPath, 'packaging');
    try {
      const client = this.projectManager.getClient();
      const { result, json } = await client.packNsp(project.rootPath, {
        profile: project.activeProfile,
        keysPath: keyPath
      });
      if (result.exitCode === 0 && (!json || json.status === 'success')) {
        const artifactPath = json?.artifact || path.join(project.rootPath, 'dist', project.activeProfile);
        this.output.info(`NSP packaged successfully with keys: ${artifactPath}`);
        vscode.window.showInformationMessage(`NSP packaged successfully!`);
      } else {
        const errMsg = json?.error?.message || 'NSP packaging failed';
        this.output.error(`NSP packaging failed: ${errMsg}`);
        vscode.window.showErrorMessage(`NSP packaging failed: ${errMsg}`);
      }
    } finally {
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }

  public static async openOutputFolder(): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to open output folder');
    if (!project) {return;}

    const distDir = path.join(project.rootPath, 'dist', project.activeProfile);
    if (!fs.existsSync(distDir)) {
      fs.mkdirSync(distDir, { recursive: true });
    }
    vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(distDir));
  }
}
