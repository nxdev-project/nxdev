import * as vscode from 'vscode';
import * as path from 'path';
import { NxDevClient } from '../nxdev/client';
import { ManifestDiagnostic } from '../nxdev/types';
import { OutputManager } from '../utils/output';

export class ManifestDiagnosticsProvider {
  private diagnosticCollection: vscode.DiagnosticCollection;
  private client: NxDevClient;
  private output = OutputManager.getInstance();
  private debounceTimers: Map<string, NodeJS.Timeout> = new Map();

  constructor(client: NxDevClient) {
    this.client = client;
    this.diagnosticCollection = vscode.languages.createDiagnosticCollection('nxdev-manifest');
  }

  public register(context: vscode.ExtensionContext): void {
    context.subscriptions.push(this.diagnosticCollection);

    // Validate on open
    context.subscriptions.push(
      vscode.workspace.onDidOpenTextDocument((doc) => {
        if (this.isManifest(doc)) {
          this.validateDocument(doc);
        }
      })
    );

    // Validate on save
    context.subscriptions.push(
      vscode.workspace.onDidSaveTextDocument((doc) => {
        if (this.isManifest(doc)) {
          this.validateDocument(doc);
        }
      })
    );

    // Debounced validation on change while typing
    context.subscriptions.push(
      vscode.workspace.onDidChangeTextDocument((event) => {
        if (this.isManifest(event.document)) {
          const key = event.document.uri.toString();
          const existing = this.debounceTimers.get(key);
          if (existing) {clearTimeout(existing);}

          const timer = setTimeout(() => {
            this.validateDocument(event.document);
            this.debounceTimers.delete(key);
          }, 600);

          this.debounceTimers.set(key, timer);
        }
      })
    );

    // Clean up when closed
    context.subscriptions.push(
      vscode.workspace.onDidCloseTextDocument((doc) => {
        if (this.isManifest(doc)) {
          this.diagnosticCollection.delete(doc.uri);
        }
      })
    );

    // Initial scan of visible editors
    for (const editor of vscode.window.visibleTextEditors) {
      if (this.isManifest(editor.document)) {
        this.validateDocument(editor.document);
      }
    }
  }

  private isManifest(doc: vscode.TextDocument): boolean {
    const filename = path.basename(doc.uri.fsPath);
    return filename === 'nxapp.yaml' || filename === 'nxapp.yml';
  }

  public async validateDocument(doc: vscode.TextDocument): Promise<void> {
    const config = vscode.workspace.getConfiguration('nxdev');
    if (!config.get<boolean>('autoValidateManifest', true)) {
      return;
    }

    const filePath = doc.uri.fsPath;
    try {
      const res = await this.client.validateManifest(filePath);
      if (!res) {
        this.diagnosticCollection.delete(doc.uri);
        return;
      }

      const diagnostics: vscode.Diagnostic[] = [];
      for (const d of res.diagnostics) {
        const vDiag = this.convertToVsCodeDiagnostic(doc, d);
        if (vDiag) {
          diagnostics.push(vDiag);
        }
      }

      this.diagnosticCollection.set(doc.uri, diagnostics);
    } catch (err) {
      this.output.debug(`Manifest validation error for ${filePath}: ${err}`);
    }
  }

  public convertToVsCodeDiagnostic(
    doc: vscode.TextDocument,
    d: ManifestDiagnostic
  ): vscode.Diagnostic | null {
    // Convert 1-based line/col from CLI to 0-based VS Code Range
    const startLine = Math.max(0, (d.line || 1) - 1);
    const startCol = Math.max(0, (d.column || 1) - 1);
    const endLine = d.endLine ? Math.max(0, d.endLine - 1) : startLine;
    let endCol = d.endColumn ? Math.max(0, d.endColumn - 1) : startCol + 1;

    // Safety checks against document line count
    if (startLine >= doc.lineCount) {
      const lastLine = doc.lineCount - 1;
      const range = new vscode.Range(lastLine, 0, lastLine, doc.lineAt(lastLine).text.length);
      return new vscode.Diagnostic(range, d.message, this.getSeverity(d.severity));
    }

    const lineText = doc.lineAt(startLine).text;
    if (endCol <= startCol || endCol > lineText.length) {
      endCol = lineText.length;
    }

    const range = new vscode.Range(startLine, startCol, endLine, endCol);
    const diag = new vscode.Diagnostic(range, d.message, this.getSeverity(d.severity));
    diag.source = 'NXDevAppManifest';
    diag.code = d.code;
    return diag;
  }

  private getSeverity(sev: string): vscode.DiagnosticSeverity {
    switch (sev) {
      case 'error':
        return vscode.DiagnosticSeverity.Error;
      case 'warning':
        return vscode.DiagnosticSeverity.Warning;
      case 'info':
      default:
        return vscode.DiagnosticSeverity.Information;
    }
  }

  public clearAll(): void {
    this.diagnosticCollection.clear();
  }
}
