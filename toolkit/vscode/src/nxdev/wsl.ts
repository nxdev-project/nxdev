import * as os from 'os';
import * as vscode from 'vscode';

export interface WslConfig {
  enabled: boolean;
  distribution?: string;
}

export class WslBridge {
  /**
   * Checks whether the current extension is running directly inside a Linux/WSL host.
   */
  public static isRunningInLinux(): boolean {
    return os.platform() === 'linux';
  }

  /**
   * Checks whether the extension is running in Windows-hosted VS Code with WSL support.
   */
  public static isWindowsHost(): boolean {
    return os.platform() === 'win32';
  }

  /**
   * Converts a Windows path (e.g. C:\Users\Dev\Project) to a WSL path (/mnt/c/Users/Dev/Project).
   */
  public static windowsToWslPath(winPath: string): string {
    if (!winPath) {return '';}
    const normalized = winPath.replace(/\\/g, '/');
    const driveMatch = normalized.match(/^([a-zA-Z]):\/(.*)$/);
    if (driveMatch) {
      const driveLetter = driveMatch[1].toLowerCase();
      const rest = driveMatch[2];
      return `/mnt/${driveLetter}/${rest}`;
    }
    // UNC or already POSIX
    if (normalized.startsWith('//wsl$/') || normalized.startsWith('//wsl.localhost/')) {
      const parts = normalized.split('/');
      // Remove //wsl$/<distro>/
      if (parts.length >= 4) {
        return '/' + parts.slice(4).join('/');
      }
    }
    return normalized;
  }

  /**
   * Converts a WSL path (/mnt/c/Users/Dev/Project) to a Windows path (C:\Users\Dev\Project).
   */
  public static wslToWindowsPath(wslPath: string): string {
    if (!wslPath) {return '';}
    const mntMatch = wslPath.match(/^\/mnt\/([a-zA-Z])\/(.*)$/);
    if (mntMatch) {
      const driveLetter = mntMatch[1].toUpperCase();
      const rest = mntMatch[2].replace(/\//g, '\\');
      return `${driveLetter}:\\${rest}`;
    }
    return wslPath;
  }

  /**
   * Resolves execution configuration for WSL bridge.
   */
  public static getWslConfig(): WslConfig {
    const config = vscode.workspace.getConfiguration('nxdev.wsl');
    return {
      enabled: config.get<boolean>('enabled', false),
      distribution: config.get<string>('distribution') || undefined
    };
  }

  /**
   * Transforms executable and arguments when running through Windows-hosted WSL bridge.
   */
  public static wrapWslCommand(
    executable: string,
    args: string[],
    wslDistro?: string
  ): { command: string; args: string[] } {
    const wslArgs: string[] = [];
    if (wslDistro && wslDistro.trim().length > 0) {
      wslArgs.push('-d', wslDistro.trim());
    }
    wslArgs.push('--', executable, ...args);
    return {
      command: 'wsl.exe',
      args: wslArgs
    };
  }
}
