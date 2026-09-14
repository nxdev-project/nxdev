/**
 * Strongly typed interfaces for NXDev CLI JSON responses and configuration.
 */

export interface CliHostInfo {
  os: string;
  arch: string;
  is_wsl: boolean;
  wsl_distro?: string;
}

export interface CliToolchainInfo {
  is_configured: boolean;
  is_valid: boolean;
  path: string;
  source: string;
}

export interface CliToolInfo {
  name: string;
  executable_name: string;
  category: string;
  source: string;
  path: string;
  version: string;
  exists: boolean;
  is_executable: boolean;
  usable: boolean;
}

export interface EnvironmentInfo {
  host: CliHostInfo;
  devkitpro: CliToolchainInfo;
  devkita64: CliToolchainInfo;
  libnx: CliToolchainInfo;
  switch_tools: CliToolchainInfo;
  tools: Record<string, CliToolInfo>;
  nxdev_version?: string;
}

export type DoctorStatus = 'passed' | 'failed' | 'warning' | 'skipped';

export interface DoctorCheck {
  name: string;
  status: DoctorStatus;
  message: string;
  details?: string;
  remediation?: string;
}

export interface DoctorResult {
  checks: DoctorCheck[];
  summary: {
    total: number;
    passed: number;
    failed: number;
    warnings: number;
  };
}

export interface ProjectInfo {
  name: string;
  author: string;
  version: string;
  titleId?: string;
  target: string;
  root: string;
  manifest: string;
  defaultProfile: string;
  profiles: string[];
  dependencies: string[];
}

export type DiagnosticSeverity = 'error' | 'warning' | 'info';

export interface ManifestDiagnostic {
  severity: DiagnosticSeverity;
  code: string;
  message: string;
  line: number;
  column: number;
  endLine?: number;
  endColumn?: number;
  field?: string;
}

export interface ManifestValidationResult {
  valid: boolean;
  manifestPath: string;
  diagnostics: ManifestDiagnostic[];
}

export type PackageType = 'builtin' | 'portlib';

export interface PackageInfo {
  id: string;
  name: string;
  description: string;
  category: string;
  type: PackageType;
  installed: boolean;
  installedVersion?: string;
  pacmanPackage?: string;
  cmakeTarget?: string;
  dependencies: string[];
}

export interface PackageListResult {
  packages: PackageInfo[];
}

export interface PackageStatusResult {
  declared: string[];
  installed: PackageInfo[];
  missing: PackageInfo[];
}

export interface BuildResult {
  status: 'success' | 'error';
  profile: string;
  artifact: string;
  fileSizeBytes: number;
  logs?: string[];
  error?: {
    code: string;
    message: string;
    exitCode: number;
  };
}

export interface BackendExecutionDetails {
  name?: string;
  revision?: string;
  executable?: string;
  stage?: string;
  workingDir?: string;
  logPath?: string;
  stagingDir?: string;
  exitCode?: number;
  stdout?: string;
  stderr?: string;
}

export interface PackResult {
  status: 'success' | 'error';
  format: 'nro' | 'nsp';
  profile: string;
  artifact: string;
  fileSizeBytes: number;
  metadata?: {
    titleId?: string;
    nacp?: string;
    icon?: string;
    iconSource?: string;
    romfs?: string;
    npdm?: string;
    nso?: string;
  };
  backend?: {
    name: string;
    revision: string;
  };
  error?: {
    code: string;
    stage?: string;
    message: string;
    exitCode: number;
    backend?: BackendExecutionDetails;
  };
  logs?: string[];
}

export interface Device {
  id: string;
  name?: string;
  host: string;
  port: number;
  is_default: boolean;
  notes?: string;
}

export interface DeviceListResult {
  devices: Device[];
}

export interface DeployResult {
  status: 'success' | 'error';
  device?: Device;
  artifact?: string;
  fileSizeBytes?: number;
  error?: {
    code: string;
    message: string;
  };
}

export interface RunEvent {
  event: 'step' | 'stdout' | 'stderr' | 'crash' | 'exit';
  step?: number;
  totalSteps?: number;
  name?: string;
  line?: string;
  addresses?: string[];
  code?: number;
}

export interface SymbolInfo {
  address: string;
  function: string;
  file: string;
  line: number;
}

export interface SymbolizeResult {
  elf?: string;
  symbols: SymbolInfo[];
}

export type ProjectOperationState =
  | 'idle'
  | 'building'
  | 'packaging'
  | 'installing'
  | 'configuring'
  | 'cleaning'
  | 'deploying'
  | 'running';
