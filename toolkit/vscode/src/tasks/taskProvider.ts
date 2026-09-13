import * as vscode from 'vscode';
import { ProjectManager } from '../project/manager';
import { ProcessRunner } from '../nxdev/process';

export class NxDevTaskProvider implements vscode.TaskProvider {
  public static readonly NXDevType = 'nxdev';
  private projectManager = ProjectManager.getInstance();

  public provideTasks(_token?: vscode.CancellationToken): vscode.Task[] {
    return this.getTasks();
  }

  public resolveTask(task: vscode.Task, _token?: vscode.CancellationToken): vscode.Task | undefined {
    return task;
  }

  private getTasks(): vscode.Task[] {
    const tasks: vscode.Task[] = [];
    const projects = this.projectManager.getProjects().filter(p => p.hasManifest);

    for (const project of projects) {
      const exe = ProcessRunner.getExecutablePath();
      const folder = project.folder;

      // 1. Build Task
      const buildTask = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'build', profile: project.activeProfile },
        folder,
        `Build (${project.activeProfile})`,
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'build', '--profile', project.activeProfile]),
        ['$gcc']
      );
      buildTask.group = vscode.TaskGroup.Build;
      tasks.push(buildTask);

      // 2. Build Debug
      const buildDebug = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'build', profile: 'debug' },
        folder,
        'Build Debug',
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'build', '--profile', 'debug']),
        ['$gcc']
      );
      buildDebug.group = vscode.TaskGroup.Build;
      tasks.push(buildDebug);

      // 3. Build Release
      const buildRelease = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'build', profile: 'release' },
        folder,
        'Build Release',
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'build', '--profile', 'release']),
        ['$gcc']
      );
      buildRelease.group = vscode.TaskGroup.Build;
      tasks.push(buildRelease);

      // 4. Configure
      const configTask = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'configure' },
        folder,
        'Configure',
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'configure'])
      );
      tasks.push(configTask);

      // 5. Clean
      const cleanTask = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'clean' },
        folder,
        'Clean',
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'clean']),
        ['$gcc']
      );
      cleanTask.group = vscode.TaskGroup.Clean;
      tasks.push(cleanTask);

      // 6. Pack NRO
      const packNroTask = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'pack', format: 'nro' },
        folder,
        'Pack NRO',
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'pack', 'nro'])
      );
      tasks.push(packNroTask);

      // 7. Pack NSP
      const packNspTask = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'pack', format: 'nsp' },
        folder,
        'Pack NSP',
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'pack', 'nsp'])
      );
      tasks.push(packNspTask);

      // 8. Deploy NRO
      const deployTask = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'deploy', profile: project.activeProfile },
        folder,
        `Deploy NRO (${project.activeProfile})`,
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'deploy', '--profile', project.activeProfile])
      );
      tasks.push(deployTask);

      // 9. Run on Switch
      const runTask = new vscode.Task(
        { type: NxDevTaskProvider.NXDevType, command: 'run', profile: project.activeProfile },
        folder,
        `Run on Switch (${project.activeProfile})`,
        'NXDev',
        new vscode.ProcessExecution(exe, ['--project', project.rootPath, 'run', '--profile', project.activeProfile])
      );
      tasks.push(runTask);
    }

    return tasks;
  }
}
