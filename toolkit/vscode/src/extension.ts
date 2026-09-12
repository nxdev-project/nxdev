import * as vscode from 'vscode';
import { exec } from 'child_process';
import { promisify } from 'util';

const execAsync = promisify(exec);

/**
 * Executes an nxdev CLI command or falls back gracefully if nxdev is not found in PATH.
 */
async function runCliCommand(subcommand: string): Promise<string> {
  try {
    const { stdout } = await execAsync(`nxdev ${subcommand}`);
    return stdout;
  } catch {
    return `NXDev CLI (nxdev) not found in system PATH. Subcommand '${subcommand}' could not be executed directly.`;
  }
}

export function activate(context: vscode.ExtensionContext) {
  const versionCmd = vscode.commands.registerCommand('nxdev.showVersion', async () => {
    const output = await runCliCommand('version');
    vscode.window.showInformationMessage(`NXDev Toolkit: ${output.trim()}`);
  });

  const doctorCmd = vscode.commands.registerCommand('nxdev.doctor', async () => {
    const output = await runCliCommand('doctor');
    const channel = vscode.window.createOutputChannel('NXDev Doctor');
    channel.show();
    channel.appendLine(output);
  });

  context.subscriptions.push(versionCmd, doctorCmd);
}

export function deactivate() {}
