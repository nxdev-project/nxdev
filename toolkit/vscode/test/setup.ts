// Setup mock for 'vscode' module when running headless unit tests
export const mockVscode = {
  workspace: {
    isTrusted: true,
    getConfiguration: (_section?: string) => ({
      get: (_key: string, defaultValue?: any) => defaultValue
    }),
    workspaceFolders: [],
    onDidChangeWorkspaceFolders: () => ({ dispose: () => {} }),
    createFileSystemWatcher: () => ({
      onDidCreate: () => ({ dispose: () => {} }),
      onDidChange: () => ({ dispose: () => {} }),
      onDidDelete: () => ({ dispose: () => {} }),
      dispose: () => {}
    }),
    onDidOpenTextDocument: () => ({ dispose: () => {} }),
    onDidChangeTextDocument: () => ({ dispose: () => {} }),
    onDidCloseTextDocument: () => ({ dispose: () => {} }),
    textDocuments: []
  },
  window: {
    createOutputChannel: () => ({
      append: () => {},
      appendLine: () => {},
      clear: () => {},
      show: () => {},
      dispose: () => {}
    }),
    createStatusBarItem: () => ({
      show: () => {},
      hide: () => {},
      dispose: () => {}
    }),
    showInformationMessage: () => Promise.resolve(),
    showWarningMessage: () => Promise.resolve(),
    showErrorMessage: () => Promise.resolve(),
    showQuickPick: () => Promise.resolve(),
    showInputBox: () => Promise.resolve(),
    showOpenDialog: () => Promise.resolve(),
    withProgress: (_options: any, task: any) => task({ report: () => {} }, { isCancellationRequested: false })
  },
  languages: {
    createDiagnosticCollection: () => ({
      set: () => {},
      delete: () => {},
      clear: () => {},
      dispose: () => {}
    }),
    registerCodeActionsProvider: () => ({ dispose: () => {} })
  },
  commands: {
    registerCommand: () => ({ dispose: () => {} }),
    executeCommand: () => Promise.resolve()
  },
  tasks: {
    registerTaskProvider: () => ({ dispose: () => {} })
  },
  StatusBarAlignment: { Left: 1, Right: 2 },
  TreeItemCollapsibleState: { None: 0, Collapsed: 1, Expanded: 2 },
  TreeItem: class {
    constructor(public label: string) {}
  },
  ThemeIcon: class {
    constructor(public id: string, public color?: any) {}
  },
  ThemeColor: class {
    constructor(public id: string) {}
  },
  EventEmitter: class {
    public event = () => {};
    public fire(): void {}
  },
  DiagnosticSeverity: { Error: 0, Warning: 1, Information: 2, Hint: 3 },
  Range: class {
    constructor(
      public startLine: number,
      public startCharacter: number,
      public endLine: number,
      public endCharacter: number
    ) {}
  },
  Position: class {
    constructor(public line: number, public character: number) {}
  },
  Diagnostic: class {
    constructor(public range: any, public message: string, public severity: any) {}
  },
  CodeActionKind: { QuickFix: { value: 'quickfix' } },
  CodeAction: class {
    constructor(public title: string, public kind: any) {}
  },
  WorkspaceEdit: class {
    public insert(): void {}
    public replace(): void {}
  },
  Task: class {
    constructor() {}
  },
  TaskScope: { Workspace: 1 },
  CustomExecution: class {
    constructor() {}
  }
};

const Module = require('module');
const originalRequire = Module.prototype.require;
Module.prototype.require = function (this: any, id: string) {
  if (id === 'vscode') {
    return mockVscode;
  }
  return originalRequire.apply(this, arguments as any);
};
