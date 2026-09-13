import * as vscode from 'vscode';
import { ProjectManager } from './project/manager';
import { ManifestDiagnosticsProvider } from './diagnostics/manifestDiagnostics';
import { ManifestCodeActionProvider } from './diagnostics/codeActions';
import { NxStatusBar } from './statusBar/statusBar';
import { ProjectTreeProvider } from './views/projectTreeProvider';
import { EnvironmentTreeProvider } from './views/environmentTreeProvider';
import { SdkModulesTreeProvider } from './views/sdkModulesTreeProvider';
import { DeviceTreeProvider } from './views/deviceTreeProvider';
import { NxDevTaskProvider } from './tasks/taskProvider';
import { ProjectCommands } from './commands/projectCommands';
import { BuildCommands } from './commands/buildCommands';
import { PackCommands } from './commands/packCommands';
import { PackageCommands } from './commands/packageCommands';
import { EnvCommands } from './commands/envCommands';
import { DeviceCommands } from './commands/deviceCommands';
import { RunCommands } from './commands/runCommands';
import { OutputManager } from './utils/output';

export async function activate(context: vscode.ExtensionContext): Promise<void> {
  const output = OutputManager.getInstance();
  output.info('Activating NXDevToolkit extension...');

  const projectManager = ProjectManager.getInstance();
  const client = projectManager.getClient();

  // 1. Check CLI availability on startup
  try {
    const version = await client.getVersion();
    if (version) {
      output.info(`Detected NXDev CLI: ${version}`);
    } else {
      output.warn('NXDev CLI was not found in system PATH. Build and pack commands require nxdev.');
    }
  } catch (err) {
    output.warn(`Could not verify NXDev CLI version: ${err}`);
  }

  // 2. Register Diagnostics and Code Actions for nxapp.yaml
  const diagnosticsProvider = new ManifestDiagnosticsProvider(client);
  diagnosticsProvider.register(context);

  context.subscriptions.push(
    vscode.languages.registerCodeActionsProvider(
      { pattern: '**/nxapp.{yaml,yml}' },
      new ManifestCodeActionProvider(),
      { providedCodeActionKinds: ManifestCodeActionProvider.providedCodeActionKinds }
    )
  );

  // 3. Register Status Bar
  const statusBar = new NxStatusBar();
  statusBar.register(context);

  // 4. Register Tree View Providers
  const projectTreeProvider = new ProjectTreeProvider();
  const environmentTreeProvider = new EnvironmentTreeProvider();
  const sdkModulesTreeProvider = new SdkModulesTreeProvider();
  const deviceTreeProvider = new DeviceTreeProvider();

  context.subscriptions.push(
    vscode.window.registerTreeDataProvider('nxdev-project', projectTreeProvider),
    vscode.window.registerTreeDataProvider('nxdev-environment', environmentTreeProvider),
    vscode.window.registerTreeDataProvider('nxdev-packages', sdkModulesTreeProvider),
    vscode.window.registerTreeDataProvider('nxdev-devices', deviceTreeProvider)
  );

  // 5. Register Task Provider
  context.subscriptions.push(
    vscode.tasks.registerTaskProvider(NxDevTaskProvider.NXDevType, new NxDevTaskProvider())
  );

  // 6. Register Commands
  context.subscriptions.push(
    // Project & Manifest
    vscode.commands.registerCommand('nxdev.openManifest', () => ProjectCommands.openManifest()),
    vscode.commands.registerCommand('nxdev.manifest.open', () => ProjectCommands.openManifest()),
    vscode.commands.registerCommand('nxdev.validateManifest', () => ProjectCommands.validateManifest()),
    vscode.commands.registerCommand('nxdev.manifest.validate', () => ProjectCommands.validateManifest()),
    vscode.commands.registerCommand('nxdev.inspectManifest', () => ProjectCommands.inspectManifest()),
    vscode.commands.registerCommand('nxdev.manifest.inspect', () => ProjectCommands.inspectManifest()),
    vscode.commands.registerCommand('nxdev.selectProfile', () => ProjectCommands.selectProfile()),
    vscode.commands.registerCommand('nxdev.project.selectProfile', () => ProjectCommands.selectProfile()),
    vscode.commands.registerCommand('nxdev.createProject', () => ProjectCommands.createProject()),
    vscode.commands.registerCommand('nxdev.project.create', () => ProjectCommands.createProject()),

    // Build & Configuration
    vscode.commands.registerCommand('nxdev.configure', () => checkTrust(() => BuildCommands.configure())),
    vscode.commands.registerCommand('nxdev.build', () => checkTrust(() => BuildCommands.build())),
    vscode.commands.registerCommand('nxdev.buildDebug', () => checkTrust(() => BuildCommands.buildDebug())),
    vscode.commands.registerCommand('nxdev.build.debug', () => checkTrust(() => BuildCommands.buildDebug())),
    vscode.commands.registerCommand('nxdev.buildRelease', () => checkTrust(() => BuildCommands.buildRelease())),
    vscode.commands.registerCommand('nxdev.build.release', () => checkTrust(() => BuildCommands.buildRelease())),
    vscode.commands.registerCommand('nxdev.clean', () => checkTrust(() => BuildCommands.clean())),

    // Packaging
    vscode.commands.registerCommand('nxdev.packNro', () => checkTrust(() => PackCommands.packageNro())),
    vscode.commands.registerCommand('nxdev.pack.nro', () => checkTrust(() => PackCommands.packageNro())),
    vscode.commands.registerCommand('nxdev.packNsp', () => checkTrust(() => PackCommands.packageNsp())),
    vscode.commands.registerCommand('nxdev.pack.nsp', () => checkTrust(() => PackCommands.packageNsp())),
    vscode.commands.registerCommand('nxdev.openOutputFolder', () => PackCommands.openOutputFolder()),
    vscode.commands.registerCommand('nxdev.pack.openOutput', () => PackCommands.openOutputFolder()),

    // Deploy & Run & Symbolize
    vscode.commands.registerCommand('nxdev.run', () => checkTrust(() => RunCommands.run())),
    vscode.commands.registerCommand('nxdev.deploy', () => checkTrust(() => RunCommands.deploy())),
    vscode.commands.registerCommand('nxdev.stopRun', () => RunCommands.stopRun()),
    vscode.commands.registerCommand('nxdev.symbolize', () => RunCommands.symbolize()),

    // Device Management
    vscode.commands.registerCommand('nxdev.devices.add', () => DeviceCommands.addDevice()),
    vscode.commands.registerCommand('nxdev.devices.remove', (item) => DeviceCommands.removeDevice(item)),
    vscode.commands.registerCommand('nxdev.devices.setDefault', (item) => DeviceCommands.setDefault(item)),
    vscode.commands.registerCommand('nxdev.devices.test', (item) => DeviceCommands.testDevice(item)),
    vscode.commands.registerCommand('nxdev.devices.refresh', () => deviceTreeProvider.refresh()),

    // SDK & Package Management
    vscode.commands.registerCommand('nxdev.installMissingPackages', () => checkTrust(() => PackageCommands.installMissing())),
    vscode.commands.registerCommand('nxdev.package.installMissing', () => checkTrust(() => PackageCommands.installMissing())),
    vscode.commands.registerCommand('nxdev.installPackage', (item) => checkTrust(() => PackageCommands.installPackage(item))),
    vscode.commands.registerCommand('nxdev.package.install', (item) => checkTrust(() => PackageCommands.installPackage(item))),
    vscode.commands.registerCommand('nxdev.removePackage', (item) => checkTrust(() => PackageCommands.removePackage(item))),
    vscode.commands.registerCommand('nxdev.package.remove', (item) => checkTrust(() => PackageCommands.removePackage(item))),
    vscode.commands.registerCommand('nxdev.addDependency', (item) => PackageCommands.addDependency(item)),
    vscode.commands.registerCommand('nxdev.package.addDependency', (item) => PackageCommands.addDependency(item)),

    // Diagnostics & Environment
    vscode.commands.registerCommand('nxdev.doctor', () => EnvCommands.runDoctor()),
    vscode.commands.registerCommand('nxdev.showEnvironment', () => EnvCommands.showEnvironment()),
    vscode.commands.registerCommand('nxdev.refreshEnvironment', () => EnvCommands.refresh()),
    vscode.commands.registerCommand('nxdev.refreshProjects', () => projectManager.refreshAll()),
    vscode.commands.registerCommand('nxdev.refreshPackages', () => sdkModulesTreeProvider.refresh()),
    vscode.commands.registerCommand('nxdev.refresh', () => EnvCommands.refresh()),
    vscode.commands.registerCommand('nxdev.showStatusBarMenu', () => EnvCommands.showMenu()),
    vscode.commands.registerCommand('nxdev.showMenu', () => EnvCommands.showMenu()),
    vscode.commands.registerCommand('nxdev.showVersion', async () => {
      const v = await client.getVersion();
      vscode.window.showInformationMessage(`NXDev CLI: ${v || 'Not detected in PATH'}`);
    })
  );

  // 7. Watch for workspace folder changes and manifest file creation/deletion
  const watcher = vscode.workspace.createFileSystemWatcher('**/nxapp.{yaml,yml}');
  context.subscriptions.push(
    watcher.onDidCreate(() => projectManager.refreshAll()),
    watcher.onDidDelete(() => projectManager.refreshAll()),
    watcher.onDidChange(() => {
      projectManager.refreshAll();
    })
  );

  context.subscriptions.push(
    vscode.workspace.onDidChangeWorkspaceFolders(() => projectManager.refreshAll())
  );

  // Initial workspace scan
  await projectManager.refreshAll();
  output.info('NXDevToolkit initialized.');
}

/**
 * Workspace Trust gate for executing child processes.
 */
function checkTrust(action: () => Promise<void>): void {
  if (!vscode.workspace.isTrusted) {
    vscode.window.showWarningMessage(
      'NXDev execution is restricted because this workspace is not trusted. Please trust the workspace to run builds and packaging.'
    );
    return;
  }
  action();
}

export function deactivate(): void {
  OutputManager.getInstance().dispose();
}
