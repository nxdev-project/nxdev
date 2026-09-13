import * as vscode from 'vscode';

export class ManifestCodeActionProvider implements vscode.CodeActionProvider {
  public static readonly providedCodeActionKinds = [
    vscode.CodeActionKind.QuickFix
  ];

  public provideCodeActions(
    document: vscode.TextDocument,
    _range: vscode.Range | vscode.Selection,
    context: vscode.CodeActionContext,
    _token: vscode.CancellationToken
  ): vscode.CodeAction[] {
    const actions: vscode.CodeAction[] = [];

    for (const diagnostic of context.diagnostics) {
      if (diagnostic.source !== 'NXDevAppManifest') {continue;}

      const codeStr = String(diagnostic.code || '');

      // 1. Missing schemaVersion
      if (codeStr.includes('SCHEMA_VERSION_MISSING') || diagnostic.message.includes('schemaVersion')) {
        const action = new vscode.CodeAction('Add schemaVersion: 1', vscode.CodeActionKind.QuickFix);
        action.edit = new vscode.WorkspaceEdit();
        action.edit.insert(document.uri, new vscode.Position(0, 0), 'schemaVersion: 1\ntarget: switch\n\n');
        action.diagnostics = [diagnostic];
        action.isPreferred = true;
        actions.push(action);
      }

      // 2. Missing target
      if (codeStr.includes('TARGET_MISSING') || diagnostic.message.includes('target')) {
        const action = new vscode.CodeAction('Add target: switch', vscode.CodeActionKind.QuickFix);
        action.edit = new vscode.WorkspaceEdit();
        action.edit.insert(document.uri, new vscode.Position(1, 0), 'target: switch\n');
        action.diagnostics = [diagnostic];
        action.isPreferred = true;
        actions.push(action);
      }

      // 3. Missing application field
      if (codeStr.includes('APPLICATION_MISSING') || diagnostic.message.includes('application')) {
        const action = new vscode.CodeAction('Add basic application metadata block', vscode.CodeActionKind.QuickFix);
        action.edit = new vscode.WorkspaceEdit();
        const block = `application:\n  name: "My Switch App"\n  author: "Developer"\n  version: "1.0.0"\n\n`;
        action.edit.insert(document.uri, new vscode.Position(document.lineCount, 0), block);
        action.diagnostics = [diagnostic];
        actions.push(action);
      }

      // 4. Missing missing dependency code action
      if (diagnostic.message.includes('Missing dependency') || diagnostic.message.includes('package')) {
        const match = diagnostic.message.match(/'(nxdev\.[a-z0-9_-]+)'/);
        if (match) {
          const pkgId = match[1];
          const action = new vscode.CodeAction(`Install missing package '${pkgId}'`, vscode.CodeActionKind.QuickFix);
          action.command = {
            command: 'nxdev.package.install',
            title: `Install ${pkgId}`,
            arguments: [pkgId]
          };
          action.diagnostics = [diagnostic];
          actions.push(action);
        }
      }
    }

    return actions;
  }
}
