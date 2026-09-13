import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';
import { OutputManager } from '../utils/output';
import { RunEvent } from '../nxdev/types';

export class RunCommands {
  private static projectManager = ProjectManager.getInstance();
  private static output = OutputManager.getInstance();
  private static currentRunCts: vscode.CancellationTokenSource | null = null;

  public static isRunning(): boolean {
    return this.currentRunCts !== null;
  }

  public static stopRun(): void {
    if (this.currentRunCts) {
      this.output.warn('Stopping active NXDev run session...');
      this.currentRunCts.cancel();
      this.currentRunCts.dispose();
      this.currentRunCts = null;
    } else {
      vscode.window.showInformationMessage('No active NXDev run session.');
    }
  }

  public static async run(profileOverride?: string, deviceOverride?: string): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to run on Switch');
    if (!project) {return;}

    if (project.operationState !== 'idle') {
      vscode.window.showWarningMessage(`Project is currently ${project.operationState}. Please wait.`);
      return;
    }

    const client = this.projectManager.getClient();
    const profile = profileOverride || project.activeProfile;

    // Resolve target device if not provided
    let targetDevice = deviceOverride;
    if (!targetDevice) {
      const config = vscode.workspace.getConfiguration('nxdev');
      const defaultCfg = config.get<string>('run.defaultDevice');
      if (defaultCfg && defaultCfg.trim().length > 0) {
        targetDevice = defaultCfg.trim();
      }
    }

    this.currentRunCts = new vscode.CancellationTokenSource();
    const token = this.currentRunCts.token;

    this.projectManager.setOperationState(project.rootPath, 'running');
    this.output.show();
    this.output.info(`Starting NXDev Run Session for '${project.info?.name || project.folder.name}' [${profile}]...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: `Running on Switch (${project.info?.name || 'Project'})...`,
          cancellable: true
        },
        async (_progress, progressToken) => {
          progressToken.onCancellationRequested(() => {
            this.stopRun();
          });

          const result = await client.run(project.rootPath, {
            profile,
            device: targetDevice,
            cancellationToken: token,
            onEvent: (evt: RunEvent) => {
              this.handleRunEvent(evt);
            }
          });

          if (result.cancelled) {
            this.output.warn('Run session cancelled by user.');
            vscode.window.showInformationMessage('NXDev run session terminated.');
          } else if (result.exitCode === 0) {
            this.output.info('NXDev run session completed successfully.');
            vscode.window.showInformationMessage(`Application completed on Switch.`);
          } else {
            this.output.error(`Run session exited with code ${result.exitCode}`);
            vscode.window.showWarningMessage(`Run session terminated with code ${result.exitCode}. See output for details.`);
          }
        }
      );
    } catch (err) {
      this.output.error(`Run session error: ${err}`);
      vscode.window.showErrorMessage(`Run failed: ${err}`);
    } finally {
      if (this.currentRunCts) {
        this.currentRunCts.dispose();
        this.currentRunCts = null;
      }
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }

  public static async deploy(profileOverride?: string, deviceOverride?: string): Promise<void> {
    const project = await this.projectManager.pickProject('Select project to deploy NRO');
    if (!project) {return;}

    if (project.operationState !== 'idle') {
      vscode.window.showWarningMessage(`Project is currently ${project.operationState}. Please wait.`);
      return;
    }

    const client = this.projectManager.getClient();
    const profile = profileOverride || project.activeProfile;

    this.projectManager.setOperationState(project.rootPath, 'deploying');
    this.output.show();
    this.output.info(`Deploying NRO for '${project.info?.name || project.folder.name}' [${profile}]...`);

    try {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: `Deploying NRO (${project.info?.name || 'Project'})...`,
          cancellable: true
        },
        async (_progress, token) => {
          const { result, json } = await client.deploy(project.rootPath, {
            profile,
            device: deviceOverride,
            cancellationToken: token
          });

          if (result.cancelled) {
            this.output.warn('Deploy cancelled.');
            return;
          }

          if (result.exitCode === 0 && (!json || json.status === 'success')) {
            const dest = json?.device ? `${json.device.id} (${json.device.host})` : 'Switch';
            this.output.info(`Deployment successful to ${dest}!`);
            vscode.window.showInformationMessage(`Deployed NRO to ${dest} successfully!`);
          } else {
            const err = json?.error?.message || result.stderr || 'Deployment failed';
            this.output.error(`Deployment failed: ${err}`);
            vscode.window.showErrorMessage(`Deployment failed: ${err}`);
          }
        }
      );
    } finally {
      this.projectManager.setOperationState(project.rootPath, 'idle');
    }
  }

  public static async symbolize(): Promise<void> {
    const activeProject = this.projectManager.getActiveProject();
    const projectRoot = activeProject?.rootPath;

    // Check if there's active selection with hex addresses
    const editor = vscode.window.activeTextEditor;
    let initialAddresses = '';
    if (editor && !editor.selection.isEmpty) {
      initialAddresses = editor.document.getText(editor.selection).trim();
    }

    const input = await vscode.window.showInputBox({
      prompt: 'Enter hex crash address(es) to symbolize (separated by space or commas)',
      value: initialAddresses,
      placeHolder: '0x0000007100014230 0x0000007100015500'
    });
    if (!input || input.trim().length === 0) {return;}

    const hexPattern = /0x[0-9a-fA-F]+/g;
    const matches = input.match(hexPattern);
    if (!matches || matches.length === 0) {
      vscode.window.showWarningMessage('No valid hexadecimal addresses (0x...) found.');
      return;
    }

    const client = this.projectManager.getClient();
    this.output.show();
    this.output.info(`Symbolizing ${matches.length} address(es)...`);

    try {
      const res = await client.symbolize(matches, { projectRoot });
      if (!res || !res.symbols || res.symbols.length === 0) {
        this.output.warn('Symbolizer returned no symbols. Ensure ELF binary is built with debug symbols.');
        vscode.window.showWarningMessage('Could not resolve symbols for given addresses.');
        return;
      }

      this.output.info(`Symbolization Results (${res.elf || 'ELF'}):`);
      for (const sym of res.symbols) {
        this.output.info(`  ${sym.address} -> ${sym.function} at ${sym.file}:${sym.line}`);
      }

      vscode.window.showInformationMessage(`Symbolized ${res.symbols.length} frame(s). Check NXDev Output.`);
    } catch (err) {
      this.output.error(`Symbolization failed: ${err}`);
      vscode.window.showErrorMessage(`Symbolization failed: ${err}`);
    }
  }

  private static handleRunEvent(evt: RunEvent): void {
    switch (evt.event) {
      case 'step':
        this.output.info(`[${evt.step}/${evt.totalSteps}] ${evt.name}...`);
        break;
      case 'stdout':
        if (evt.line !== undefined) {
          this.output.appendRaw(`${evt.line}\n`);
        }
        break;
      case 'stderr':
        if (evt.line !== undefined) {
          this.output.appendRaw(`[STDERR] ${evt.line}\n`);
        }
        break;
      case 'crash':
        this.output.error(`CRASH DETECTED on Switch!`);
        if (evt.line) {
          this.output.error(`  Crash log: ${evt.line}`);
        }
        if (evt.addresses && evt.addresses.length > 0) {
          this.output.warn(`  Captured crash frames: ${evt.addresses.join(' ')}`);
        }
        break;
      case 'exit':
        this.output.info(`Remote application exited with code: ${evt.code ?? 0}`);
        break;
    }
  }
}
