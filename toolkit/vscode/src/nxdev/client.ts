import * as vscode from 'vscode';
import { ProcessRunner, ProcessResult } from './process';
import {
  EnvironmentInfo,
  DoctorResult,
  ProjectInfo,
  ManifestValidationResult,
  PackageListResult,
  PackageStatusResult,
  BuildResult,
  PackResult,
  DeviceListResult,
  DeployResult,
  RunEvent,
  SymbolizeResult
} from './types';
export class NxDevClient {
  private runner = new ProcessRunner();

  private getExe(): string {
    return ProcessRunner.getExecutablePath();
  }

  /**
   * Retrieves the raw version string from `nxdev --version`.
   */
  public async getVersion(): Promise<string | null> {
    const res = await this.runner.execute(this.getExe(), ['--version'], {
      suppressOutputLogging: true
    });
    if (res.exitCode === 0 && res.stdout) {
      return res.stdout.trim();
    }
    return null;
  }

  /**
   * Probes environment detection via `nxdev env --json`.
   */
  public async getEnvironment(): Promise<EnvironmentInfo | null> {
    const res = await this.runner.execute(this.getExe(), ['env', '--json'], {
      suppressOutputLogging: true
    });
    if (res.exitCode === 0) {
      const parsed = ProcessRunner.parseJson<EnvironmentInfo>(res.stdout);
      if (parsed.success && parsed.data) {
        return parsed.data;
      }
    }
    return null;
  }

  /**
   * Runs environment diagnostics via `nxdev doctor --json`.
   */
  public async runDoctor(): Promise<DoctorResult | null> {
    const res = await this.runner.execute(this.getExe(), ['doctor', '--json'], {
      suppressOutputLogging: true
    });
    if (res.stdout) {
      const parsed = ProcessRunner.parseJson<DoctorResult>(res.stdout);
      if (parsed.success && parsed.data) {
        return parsed.data;
      }
    }
    return null;
  }

  /**
   * Inspects project metadata via `nxdev --project <path> project info --json`.
   */
  public async getProjectInfo(projectRoot: string): Promise<ProjectInfo | null> {
    const res = await this.runner.execute(this.getExe(), ['--project', projectRoot, 'project', 'info', '--json'], {
      suppressOutputLogging: true,
      cwd: projectRoot
    });
    if (res.exitCode === 0 && res.stdout) {
      const parsed = ProcessRunner.parseJson<ProjectInfo>(res.stdout);
      if (parsed.success && parsed.data) {
        return parsed.data;
      }
    }
    return null;
  }

  /**
   * Performs semantic manifest validation via `nxdev manifest validate <file> --json`.
   */
  public async validateManifest(manifestPath: string): Promise<ManifestValidationResult | null> {
    const res = await this.runner.execute(this.getExe(), ['manifest', 'validate', manifestPath, '--json'], {
      suppressOutputLogging: true
    });
    if (res.stdout) {
      const parsed = ProcessRunner.parseJson<ManifestValidationResult>(res.stdout);
      if (parsed.success && parsed.data) {
        return parsed.data;
      }
    }
    return null;
  }

  /**
   * Lists available SDK modules and portlibs via `nxdev package list --json`.
   */
  public async listPackages(): Promise<PackageListResult | null> {
    const res = await this.runner.execute(this.getExe(), ['package', 'list', '--json'], {
      suppressOutputLogging: true
    });
    if (res.exitCode === 0 && res.stdout) {
      const parsed = ProcessRunner.parseJson<PackageListResult>(res.stdout);
      if (parsed.success && parsed.data) {
        return parsed.data;
      }
    }
    return null;
  }

  /**
   * Checks installed vs missing package dependencies via `nxdev package status --json`.
   */
  public async getPackageStatus(projectRoot?: string): Promise<PackageStatusResult | null> {
    const args = ['package', 'status', '--json'];
    const res = await this.runner.execute(
      this.getExe(),
      projectRoot ? ['--project', projectRoot, ...args] : args,
      {
        suppressOutputLogging: true,
        cwd: projectRoot
      }
    );
    if (res.exitCode === 0 && res.stdout) {
      const parsed = ProcessRunner.parseJson<PackageStatusResult>(res.stdout);
      if (parsed.success && parsed.data) {
        return parsed.data;
      }
    }
    return null;
  }

  /**
   * Installs an SDK package via `nxdev package install <id>`.
   */
  public async installPackage(
    packageId: string,
    options: { dryRun?: boolean; cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    const args = ['package', 'install', packageId];
    if (options.dryRun) {args.push('--dry-run');}
    return this.runner.execute(this.getExe(), args, {
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Installs missing dependencies declared in project manifest via `nxdev package install --missing`.
   */
  public async installMissingPackages(
    projectRoot?: string,
    options: { dryRun?: boolean; cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    const args = ['package', 'install', '--missing'];
    if (options.dryRun) {args.push('--dry-run');}
    const fullArgs = projectRoot ? ['--project', projectRoot, ...args] : args;
    return this.runner.execute(this.getExe(), fullArgs, {
      cwd: projectRoot,
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Removes an SDK package via `nxdev package remove <id>`.
   */
  public async removePackage(
    packageId: string,
    options: { cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    return this.runner.execute(this.getExe(), ['package', 'remove', packageId], {
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Configures CMake workspace via `nxdev configure`.
   */
  public async configure(
    projectRoot: string,
    options: { profile?: string; cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    const args = ['--project', projectRoot, 'configure'];
    if (options.profile) {args.push('--profile', options.profile);}
    return this.runner.execute(this.getExe(), args, {
      cwd: projectRoot,
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Builds the application via `nxdev build`.
   */
  public async build(
    projectRoot: string,
    options: {
      profile?: string;
      clean?: boolean;
      cancellationToken?: vscode.CancellationToken;
    } = {}
  ): Promise<{ result: ProcessResult; json?: BuildResult }> {
    const args = ['--project', projectRoot, 'build', '--json'];
    if (options.profile) {args.push('--profile', options.profile);}
    if (options.clean) {args.push('--clean');}

    const result = await this.runner.execute(this.getExe(), args, {
      cwd: projectRoot,
      cancellationToken: options.cancellationToken
    });

    let json: BuildResult | undefined;
    if (result.stdout) {
      const parsed = ProcessRunner.parseJson<BuildResult>(result.stdout);
      if (parsed.success && parsed.data) {
        json = parsed.data;
      }
    }

    return { result, json };
  }

  /**
   * Cleans build directory via `nxdev clean`.
   */
  public async clean(
    projectRoot: string,
    options: { profile?: string; cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    const args = ['--project', projectRoot, 'clean'];
    if (options.profile) {args.push('--profile', options.profile);}
    return this.runner.execute(this.getExe(), args, {
      cwd: projectRoot,
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Packages project into NRO format via `nxdev pack nro`.
   */
  public async packNro(
    projectRoot: string,
    options: {
      profile?: string;
      noBuild?: boolean;
      outputPath?: string;
      dryRun?: boolean;
      cancellationToken?: vscode.CancellationToken;
    } = {}
  ): Promise<{ result: ProcessResult; json?: PackResult }> {
    const args = ['--project', projectRoot, 'pack', 'nro', '--json'];
    if (options.profile) {args.push('--profile', options.profile);}
    if (options.noBuild) {args.push('--no-build');}
    if (options.outputPath) {args.push('-o', options.outputPath);}
    if (options.dryRun) {args.push('--dry-run');}

    const result = await this.runner.execute(this.getExe(), args, {
      cwd: projectRoot,
      cancellationToken: options.cancellationToken
    });

    let json: PackResult | undefined;
    if (result.stdout) {
      const parsed = ProcessRunner.parseJson<PackResult>(result.stdout);
      if (parsed.success && parsed.data) {
        json = parsed.data;
      }
    }

    return { result, json };
  }

  /**
   * Packages project into NSP format via `nxdev pack nsp`.
   */
  public async packNsp(
    projectRoot: string,
    options: {
      profile?: string;
      keysPath?: string;
      noBuild?: boolean;
      outputPath?: string;
      dryRun?: boolean;
      cancellationToken?: vscode.CancellationToken;
    } = {}
  ): Promise<{ result: ProcessResult; json?: PackResult }> {
    const args = ['--project', projectRoot, 'pack', 'nsp', '--json'];
    if (options.profile) {args.push('--profile', options.profile);}
    if (options.keysPath) {args.push('--keys', options.keysPath);}
    if (options.noBuild) {args.push('--no-build');}
    if (options.outputPath) {args.push('-o', options.outputPath);}
    if (options.dryRun) {args.push('--dry-run');}

    const result = await this.runner.execute(this.getExe(), args, {
      cwd: projectRoot,
      cancellationToken: options.cancellationToken
    });

    let json: PackResult | undefined;
    if (result.stdout) {
      const parsed = ProcessRunner.parseJson<PackResult>(result.stdout);
      if (parsed.success && parsed.data) {
        json = parsed.data;
      }
    }

    return { result, json };
  }

  /**
   * Lists configured devices via `nxdev devices list --json`.
   */
  public async listDevices(projectRoot?: string): Promise<DeviceListResult | null> {
    const args = ['devices', 'list', '--json'];
    const res = await this.runner.execute(
      this.getExe(),
      projectRoot ? ['--project', projectRoot, ...args] : args,
      {
        suppressOutputLogging: true,
        cwd: projectRoot
      }
    );
    if (res.exitCode === 0 && res.stdout) {
      const parsed = ProcessRunner.parseJson<DeviceListResult>(res.stdout);
      if (parsed.success && parsed.data) {
        return parsed.data;
      }
    }
    return null;
  }

  /**
   * Adds a device via `nxdev devices add`.
   */
  public async addDevice(
    device: { id: string; host: string; port?: number; isDefault?: boolean; notes?: string },
    options: { projectRoot?: string; cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    const args = ['devices', 'add', device.id, device.host];
    if (device.port) {args.push('--port', String(device.port));}
    if (device.isDefault) {args.push('--default');}
    if (device.notes) {args.push('--notes', device.notes);}

    const fullArgs = options.projectRoot ? ['--project', options.projectRoot, ...args] : args;
    return this.runner.execute(this.getExe(), fullArgs, {
      cwd: options.projectRoot,
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Removes a device via `nxdev devices remove <id>`.
   */
  public async removeDevice(
    id: string,
    options: { projectRoot?: string; cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    const args = ['devices', 'remove', id];
    const fullArgs = options.projectRoot ? ['--project', options.projectRoot, ...args] : args;
    return this.runner.execute(this.getExe(), fullArgs, {
      cwd: options.projectRoot,
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Sets default device via `nxdev devices set-default <id>`.
   */
  public async setDefaultDevice(
    id: string,
    options: { projectRoot?: string; cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    const args = ['devices', 'set-default', id];
    const fullArgs = options.projectRoot ? ['--project', options.projectRoot, ...args] : args;
    return this.runner.execute(this.getExe(), fullArgs, {
      cwd: options.projectRoot,
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Tests connection to a device via `nxdev devices test <id-or-host>`.
   */
  public async testDevice(
    idOrHost: string,
    options: { projectRoot?: string; port?: number; timeoutMs?: number; cancellationToken?: vscode.CancellationToken } = {}
  ): Promise<ProcessResult> {
    const args = ['devices', 'test', idOrHost];
    if (options.port) {args.push('--port', String(options.port));}
    if (options.timeoutMs) {args.push('--timeout', String(options.timeoutMs));}
    const fullArgs = options.projectRoot ? ['--project', options.projectRoot, ...args] : args;
    return this.runner.execute(this.getExe(), fullArgs, {
      cwd: options.projectRoot,
      cancellationToken: options.cancellationToken
    });
  }

  /**
   * Deploys an NRO to a target device via `nxdev deploy`.
   */
  public async deploy(
    projectRoot: string,
    options: {
      device?: string;
      host?: string;
      profile?: string;
      artifact?: string;
      dryRun?: boolean;
      cancellationToken?: vscode.CancellationToken;
    } = {}
  ): Promise<{ result: ProcessResult; json?: DeployResult }> {
    const args = ['--project', projectRoot, 'deploy', '--json'];
    if (options.device) {args.push('--device', options.device);}
    if (options.host) {args.push('--host', options.host);}
    if (options.profile) {args.push('--profile', options.profile);}
    if (options.artifact) {args.push('--artifact', options.artifact);}
    if (options.dryRun) {args.push('--dry-run');}

    const result = await this.runner.execute(this.getExe(), args, {
      cwd: projectRoot,
      cancellationToken: options.cancellationToken
    });

    let json: DeployResult | undefined;
    if (result.stdout) {
      const parsed = ProcessRunner.parseJson<DeployResult>(result.stdout);
      if (parsed.success && parsed.data) {
        json = parsed.data;
      }
    }

    return { result, json };
  }

  /**
   * Runs project on a target Switch with live output/NDJSON streaming via `nxdev run --json-stream`.
   */
  public async run(
    projectRoot: string,
    options: {
      device?: string;
      host?: string;
      profile?: string;
      noBuild?: boolean;
      noPack?: boolean;
      args?: string;
      dryRun?: boolean;
      cancellationToken?: vscode.CancellationToken;
      onEvent?: (event: RunEvent) => void;
    } = {}
  ): Promise<ProcessResult> {
    const args = ['--project', projectRoot, 'run', '--json-stream'];
    if (options.device) {args.push('--device', options.device);}
    if (options.host) {args.push('--host', options.host);}
    if (options.profile) {args.push('--profile', options.profile);}
    if (options.noBuild) {args.push('--no-build');}
    if (options.noPack) {args.push('--no-pack');}
    if (options.args) {args.push('--args', options.args);}
    if (options.dryRun) {args.push('--dry-run');}

    let lineBuffer = '';

    return this.runner.execute(this.getExe(), args, {
      cwd: projectRoot,
      cancellationToken: options.cancellationToken,
      onStdout: (chunk: string) => {
        lineBuffer += chunk;
        let newlineIdx: number;
        while ((newlineIdx = lineBuffer.indexOf('\n')) !== -1) {
          const line = lineBuffer.slice(0, newlineIdx).trim();
          lineBuffer = lineBuffer.slice(newlineIdx + 1);
          if (line.length > 0 && options.onEvent) {
            try {
              const evt = JSON.parse(line) as RunEvent;
              if (evt && evt.event) {
                options.onEvent(evt);
              }
            } catch {
              // Non-JSON line from raw stdout
            }
          }
        }
      }
    });
  }

  /**
   * Symbolizes crash addresses via `nxdev symbolize <addresses...>`.
   */
  public async symbolize(
    addresses: string[],
    options: {
      elf?: string;
      base?: string;
      projectRoot?: string;
      cancellationToken?: vscode.CancellationToken;
    } = {}
  ): Promise<SymbolizeResult | null> {
    const args = ['symbolize', '--json'];
    if (options.elf) {args.push('--elf', options.elf);}
    if (options.base) {args.push('--base', options.base);}
    args.push(...addresses);

    const fullArgs = options.projectRoot ? ['--project', options.projectRoot, ...args] : args;
    const res = await this.runner.execute(this.getExe(), fullArgs, {
      cwd: options.projectRoot,
      cancellationToken: options.cancellationToken,
      suppressOutputLogging: true
    });

    if (res.exitCode === 0 && res.stdout) {
      const parsed = ProcessRunner.parseJson<SymbolizeResult>(res.stdout);
      if (parsed.success && parsed.data) {
        return parsed.data;
      }
    }
    return null;
  }
}
