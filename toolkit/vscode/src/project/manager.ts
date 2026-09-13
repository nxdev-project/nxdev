import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { NxDevClient } from '../nxdev/client';
import { ProjectInfo, EnvironmentInfo, DoctorResult, ProjectOperationState } from '../nxdev/types';
import { OutputManager } from '../utils/output';

export interface NxProjectState {
  folder: vscode.WorkspaceFolder;
  rootPath: string;
  manifestPath: string;
  hasManifest: boolean;
  isValid: boolean;
  info?: ProjectInfo;
  activeProfile: string;
  operationState: ProjectOperationState;
}

export class ProjectManager {
  private static instance: ProjectManager;
  private client = new NxDevClient();
  private output = OutputManager.getInstance();

  private projects: Map<string, NxProjectState> = new Map();
  private activeProjectRoot: string | null = null;
  private environmentCache: EnvironmentInfo | null = null;
  private doctorCache: DoctorResult | null = null;
  private lastEnvFetch = 0;

  private onProjectChangedEmitter = new vscode.EventEmitter<NxProjectState | null>();
  public readonly onProjectChanged = this.onProjectChangedEmitter.event;

  private onProjectsRefreshedEmitter = new vscode.EventEmitter<void>();
  public readonly onProjectsRefreshed = this.onProjectsRefreshedEmitter.event;

  private constructor() {}

  public static getInstance(): ProjectManager {
    if (!ProjectManager.instance) {
      ProjectManager.instance = new ProjectManager();
    }
    return ProjectManager.instance;
  }

  public getClient(): NxDevClient {
    return this.client;
  }

  public getProjects(): NxProjectState[] {
    return Array.from(this.projects.values());
  }

  public getProject(rootPath: string): NxProjectState | undefined {
    return this.projects.get(path.normalize(rootPath));
  }

  public getActiveProject(): NxProjectState | null {
    if (this.activeProjectRoot) {
      const p = this.projects.get(this.activeProjectRoot);
      if (p) {return p;}
    }
    const all = this.getProjects();
    return all.length > 0 ? all[0] : null;
  }

  public setActiveProject(rootPath: string): void {
    const normalized = path.normalize(rootPath);
    if (this.projects.has(normalized)) {
      this.activeProjectRoot = normalized;
      this.onProjectChangedEmitter.fire(this.projects.get(normalized) || null);
    }
  }

  public setOperationState(rootPath: string, state: ProjectOperationState): void {
    const p = this.getProject(rootPath);
    if (p) {
      p.operationState = state;
      this.onProjectsRefreshedEmitter.fire();
    }
  }

  public setActiveProfile(rootPath: string, profile: string): void {
    const p = this.getProject(rootPath);
    if (p) {
      p.activeProfile = profile;
      this.onProjectsRefreshedEmitter.fire();
    }
  }

  public async getEnvironment(forceRefresh = false): Promise<EnvironmentInfo | null> {
    const now = Date.now();
    if (!forceRefresh && this.environmentCache && now - this.lastEnvFetch < 30000) {
      return this.environmentCache;
    }
    try {
      this.environmentCache = await this.client.getEnvironment();
      this.lastEnvFetch = now;
      return this.environmentCache;
    } catch (err) {
      this.output.error('Failed to get environment information', err);
      return null;
    }
  }

  public async getDoctor(forceRefresh = false): Promise<DoctorResult | null> {
    if (!forceRefresh && this.doctorCache) {
      return this.doctorCache;
    }
    try {
      this.doctorCache = await this.client.runDoctor();
      return this.doctorCache;
    } catch (err) {
      this.output.error('Failed to run doctor diagnostics', err);
      return null;
    }
  }

  /**
   * Scans all workspace folders to detect NXDev projects.
   */
  public async refreshAll(): Promise<void> {
    this.projects.clear();
    const folders = vscode.workspace.workspaceFolders || [];

    for (const folder of folders) {
      const rootPath = path.normalize(folder.uri.fsPath);
      const manifestPath = path.join(rootPath, 'nxapp.yaml');
      const hasManifest = fs.existsSync(manifestPath);

      const state: NxProjectState = {
        folder,
        rootPath,
        manifestPath,
        hasManifest,
        isValid: false,
        activeProfile: 'debug',
        operationState: 'idle'
      };

      if (hasManifest) {
        try {
          const info = await this.client.getProjectInfo(rootPath);
          if (info) {
            state.info = info;
            state.isValid = true;
            if (info.defaultProfile) {
              state.activeProfile = info.defaultProfile;
            }
          }
        } catch (err) {
          this.output.warn(`Failed to inspect project at ${rootPath}: ${err}`);
        }
      }

      this.projects.set(rootPath, state);
    }

    if (!this.activeProjectRoot || !this.projects.has(this.activeProjectRoot)) {
      const first = Array.from(this.projects.keys())[0];
      this.activeProjectRoot = first || null;
    }

    this.onProjectsRefreshedEmitter.fire();
  }

  /**
   * Prompts user to pick a project if multiple exist in workspace.
   */
  public async pickProject(prompt = 'Select NXDev Project'): Promise<NxProjectState | null> {
    const all = this.getProjects().filter(p => p.hasManifest);
    if (all.length === 0) {
      vscode.window.showWarningMessage('No NXDev project found in workspace.');
      return null;
    }
    if (all.length === 1) {
      return all[0];
    }

    // If active editor document belongs to one of the projects, prefer it
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor) {
      const docPath = activeEditor.document.uri.fsPath;
      for (const p of all) {
        if (docPath.startsWith(p.rootPath)) {
          return p;
        }
      }
    }

    const items = all.map(p => ({
      label: p.info?.name || p.folder.name,
      description: p.rootPath,
      detail: `Target: ${p.info?.target || 'switch'} • Profile: ${p.activeProfile}`,
      project: p
    }));

    const selected = await vscode.window.showQuickPick(items, {
      placeHolder: prompt
    });

    return selected ? selected.project : null;
  }
}
