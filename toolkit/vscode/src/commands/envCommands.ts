import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';
import { OutputManager } from '../utils/output';

export class EnvCommands {
  private static projectManager = ProjectManager.getInstance();
  private static output = OutputManager.getInstance();

  public static async runDoctor(): Promise<void> {
    this.output.show();
    this.output.info('Running NXDev Doctor diagnostics...');

    const doctor = await this.projectManager.getDoctor(true);
    if (!doctor) {
      this.output.error('Failed to run doctor diagnostics.');
      vscode.window.showErrorMessage('NXDev Doctor diagnostics failed.');
      return;
    }

    this.output.appendLine('--------------------------------------------------');
    this.output.appendLine('NXDev Doctor Diagnostics');
    this.output.appendLine('--------------------------------------------------');

    for (const check of doctor.checks) {
      const icon = check.status === 'passed' ? '✔' : check.status === 'warning' ? '!' : '✖';
      this.output.appendLine(`${icon} [${check.status.toUpperCase()}] ${check.name}: ${check.message}`);
      if (check.details) {
        this.output.appendLine(`    Details: ${check.details}`);
      }
      if (check.remediation) {
        this.output.appendLine(`    Fix: ${check.remediation}`);
      }
    }

    this.output.appendLine('--------------------------------------------------');
    const s = doctor.summary;
    this.output.appendLine(`Summary: ${s.passed} passed, ${s.failed} failed, ${s.warnings} warnings (${s.total} total)`);

    if (s.failed === 0 && s.warnings === 0) {
      vscode.window.showInformationMessage(`NXDev Doctor: All ${s.passed} checks passed.`);
    } else if (s.failed > 0) {
      vscode.window.showErrorMessage(`NXDev Doctor: ${s.failed} check(s) failed. See Output channel for remediation.`);
    } else {
      vscode.window.showWarningMessage(`NXDev Doctor: ${s.warnings} warning(s) detected.`);
    }
  }

  public static async showEnvironment(): Promise<void> {
    this.output.show();
    const env = await this.projectManager.getEnvironment(true);
    if (!env) {
      this.output.error('Failed to retrieve environment information.');
      vscode.window.showErrorMessage('Failed to retrieve environment.');
      return;
    }

    this.output.appendLine('==================================================');
    this.output.appendLine('NXDev Environment Summary');
    this.output.appendLine('==================================================');
    this.output.appendLine(`Host OS:       ${env.host.os} (${env.host.arch})${env.host.is_wsl ? ' [WSL]' : ''}`);
    this.output.appendLine(`devkitPro:     ${env.devkitpro.is_valid ? env.devkitpro.path : 'Not configured'}`);
    this.output.appendLine(`devkitA64:     ${env.devkita64.is_valid ? env.devkita64.path : 'Not configured'}`);
    this.output.appendLine(`libnx:         ${env.libnx.is_valid ? env.libnx.path : 'Not configured'}`);
    this.output.appendLine(`switch-tools:  ${env.switch_tools.is_valid ? env.switch_tools.path : 'Not configured'}`);
    this.output.appendLine('--------------------------------------------------');
    this.output.appendLine('Detected Tools:');
    for (const [name, tool] of Object.entries(env.tools)) {
      this.output.appendLine(`  • ${name}: ${tool.usable ? `${tool.path} (${tool.version})` : 'Not found'}`);
    }
    this.output.appendLine('==================================================');
  }

  public static async refresh(): Promise<void> {
    await this.projectManager.refreshAll();
    vscode.window.showInformationMessage('NXDev workspace refreshed.');
  }

  public static async showMenu(): Promise<void> {
    const project = this.projectManager.getActiveProject();

    const items: Array<{ label: string; description?: string; detail?: string; command: string }> = [
      { label: '$(gear) Configure', description: 'Configure CMake workspace', command: 'nxdev.configure' },
      { label: '$(play) Build', description: `Build with profile '${project?.activeProfile || 'debug'}'`, command: 'nxdev.build' },
      { label: '$(debug-alt) Build Debug', description: 'Compile Switch AArch64 debug binary', command: 'nxdev.build.debug' },
      { label: '$(rocket) Build Release', description: 'Compile Switch AArch64 release binary', command: 'nxdev.build.release' },
      { label: '$(package) Package NRO', description: 'Package as Homebrew Menu executable (.nro)', command: 'nxdev.pack.nro' },
      { label: '$(package) Package NSP', description: 'Package as installable Home Menu title (.nsp)', command: 'nxdev.pack.nsp' },
      { label: '$(folder) Open Output Folder', description: 'Reveal dist directory in OS file explorer', command: 'nxdev.pack.openOutput' },
      { label: '$(cloud-download) Install Missing Dependencies', description: 'Run pacman to install missing portlibs', command: 'nxdev.package.installMissing' },
      { label: '$(symbol-namespace) Add SDK Module', description: 'Declare an optional SDK module in nxapp.yaml', command: 'nxdev.package.addDependency' },
      { label: '$(tools) Switch Build Profile', description: `Current: ${project?.activeProfile || 'debug'}`, command: 'nxdev.project.selectProfile' },
      { label: '$(file-code) Open Manifest', description: 'Open project nxapp.yaml', command: 'nxdev.manifest.open' },
      { label: '$(check) Validate Manifest', description: 'Run semantic validation on manifest', command: 'nxdev.manifest.validate' },
      { label: '$(pulse) Run Doctor', description: 'Check toolchain, libnx, and compiler health', command: 'nxdev.doctor' },
      { label: '$(info) Show Environment', description: 'View detected tools and paths', command: 'nxdev.showEnvironment' },
      { label: '$(refresh) Refresh NXDev', description: 'Re-scan workspace and toolchains', command: 'nxdev.refresh' }
    ];

    const selected = await vscode.window.showQuickPick(items, {
      placeHolder: `NXDev Menu — ${project?.info?.name || 'Nintendo Switch'}`
    });

    if (selected) {
      vscode.commands.executeCommand(selected.command);
    }
  }
}
