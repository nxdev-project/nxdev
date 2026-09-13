import { spawn, ChildProcess } from 'child_process';
import * as vscode from 'vscode';
import { OutputManager } from '../utils/output';
import { WslBridge } from './wsl';

export interface ProcessOptions {
  cwd?: string;
  env?: NodeJS.ProcessEnv;
  timeoutMs?: number;
  cancellationToken?: vscode.CancellationToken;
  onStdout?: (data: string) => void;
  onStderr?: (data: string) => void;
  suppressOutputLogging?: boolean;
}

export interface ProcessResult {
  exitCode: number;
  stdout: string;
  stderr: string;
  cancelled: boolean;
  timedOut: boolean;
}

export class ProcessRunner {
  private output = OutputManager.getInstance();

  /**
   * Resolves the configured or default nxdev executable path.
   */
  public static getExecutablePath(): string {
    const config = vscode.workspace.getConfiguration('nxdev');
    const customPath = config.get<string>('cliPath') || config.get<string>('executablePath');
    if (customPath && customPath.trim().length > 0) {
      return customPath.trim();
    }
    return 'nxdev';
  }

  /**
   * Spawns a command directly without shell interpretation.
   */
  public async execute(
    executable: string,
    args: string[],
    options: ProcessOptions = {}
  ): Promise<ProcessResult> {
    const {
      cwd,
      env = process.env,
      timeoutMs = 120000,
      cancellationToken,
      onStdout,
      onStderr,
      suppressOutputLogging = false
    } = options;

    let finalCommand = executable;
    let finalArgs = [...args];

    // Check if Windows host is configured to use WSL bridge
    if (WslBridge.isWindowsHost()) {
      const wslCfg = WslBridge.getWslConfig();
      if (wslCfg.enabled) {
        let wslCwd = cwd ? WslBridge.windowsToWslPath(cwd) : undefined;
        const wrapped = WslBridge.wrapWslCommand(executable, finalArgs, wslCfg.distribution);
        finalCommand = wrapped.command;
        finalArgs = wrapped.args;
        if (wslCwd) {
          finalArgs = ['-e', 'sh', '-c', `cd "${wslCwd}" && ${executable} ${args.map(a => `"${a}"`).join(' ')}`];
        }
      }
    }

    if (!suppressOutputLogging) {
      this.output.debug(`Executing: ${finalCommand} ${finalArgs.join(' ')} (cwd: ${cwd || '.'})`);
    }

    return new Promise<ProcessResult>((resolve) => {
      let stdoutData = '';
      let stderrData = '';
      let cancelled = false;
      let timedOut = false;
      let child: ChildProcess | null = null;
      let timer: NodeJS.Timeout | null = null;

      try {
        child = spawn(finalCommand, finalArgs, {
          cwd,
          env,
          shell: false,
          windowsHide: true
        });
      } catch (err) {
        this.output.error(`Failed to launch process: ${finalCommand}`, err);
        return resolve({
          exitCode: -1,
          stdout: '',
          stderr: String(err),
          cancelled: false,
          timedOut: false
        });
      }

      if (cancellationToken) {
        cancellationToken.onCancellationRequested(() => {
          cancelled = true;
          this.output.warn(`Operation cancelled by user: ${finalCommand}`);
          if (child && !child.killed) {
            child.kill('SIGTERM');
          }
        });
      }

      if (timeoutMs > 0) {
        timer = setTimeout(() => {
          timedOut = true;
          this.output.error(`Process timed out after ${timeoutMs}ms: ${finalCommand}`);
          if (child && !child.killed) {
            child.kill('SIGKILL');
          }
        }, timeoutMs);
      }

      if (child.stdout) {
        child.stdout.on('data', (chunk: Buffer) => {
          const text = chunk.toString('utf-8');
          stdoutData += text;
          if (onStdout) {onStdout(text);}
          if (!suppressOutputLogging) {
            this.output.appendRaw(text);
          }
        });
      }

      if (child.stderr) {
        child.stderr.on('data', (chunk: Buffer) => {
          const text = chunk.toString('utf-8');
          stderrData += text;
          if (onStderr) {onStderr(text);}
          if (!suppressOutputLogging) {
            this.output.appendRaw(text);
          }
        });
      }

      child.on('error', (err) => {
        if (timer) {clearTimeout(timer);}
        this.output.error(`Process error: ${err.message}`);
        resolve({
          exitCode: -1,
          stdout: stdoutData,
          stderr: stderrData ? `${stderrData}\n${err.message}` : err.message,
          cancelled,
          timedOut
        });
      });

      child.on('close', (code) => {
        if (timer) {clearTimeout(timer);}
        resolve({
          exitCode: code !== null ? code : -1,
          stdout: stdoutData,
          stderr: stderrData,
          cancelled,
          timedOut
        });
      });
    });
  }

  /**
   * Helper to parse JSON from process output safely.
   */
  public static parseJson<T>(raw: string): { success: boolean; data?: T; error?: string } {
    try {
      const trimmed = raw.trim();
      if (!trimmed) {
        return { success: false, error: 'Empty output' };
      }

      try {
        const direct = JSON.parse(trimmed) as T;
        return { success: true, data: direct };
      } catch {
        // Direct parse failed, try finding start of json block
      }

      // Try lines that start with { or [
      const lines = trimmed.split('\n');
      for (let i = 0; i < lines.length; i++) {
        const lineTrim = lines[i].trim();
        if (lineTrim.startsWith('{') || lineTrim.startsWith('[')) {
          const candidate = lines.slice(i).join('\n').trim();
          try {
            const data = JSON.parse(candidate) as T;
            return { success: true, data };
          } catch {
            // continue checking
          }
        }
      }

      // Fallback: search for outer { ... } or [ ... ]
      const firstBrace = trimmed.indexOf('{');
      if (firstBrace !== -1) {
        const lastBrace = trimmed.lastIndexOf('}');
        if (lastBrace > firstBrace) {
          try {
            const data = JSON.parse(trimmed.substring(firstBrace, lastBrace + 1)) as T;
            return { success: true, data };
          } catch {}
        }
      }

      return { success: false, error: 'No JSON object found in output' };
    } catch (err) {
      return { success: false, error: `Invalid JSON output: ${err instanceof Error ? err.message : String(err)}` };
    }
  }
}
