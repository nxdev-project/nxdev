import * as vscode from 'vscode';

export class OutputManager {
  private static instance: OutputManager;
  private channel: vscode.OutputChannel;
  private isVerbose = false;

  private constructor() {
    this.channel = vscode.window.createOutputChannel('NXDev');
  }

  public static getInstance(): OutputManager {
    if (!OutputManager.instance) {
      OutputManager.instance = new OutputManager();
    }
    return OutputManager.instance;
  }

  public setVerbose(verbose: boolean): void {
    this.isVerbose = verbose;
  }

  public appendLine(message: string): void {
    const timestamp = new Date().toLocaleTimeString();
    this.channel.appendLine(`[${timestamp}] ${message}`);
  }

  public appendRaw(text: string): void {
    this.channel.append(text);
  }

  public info(message: string): void {
    this.appendLine(`[INFO] ${message}`);
  }

  public warn(message: string): void {
    this.appendLine(`[WARN] ${message}`);
  }

  public error(message: string, error?: unknown): void {
    this.appendLine(`[ERROR] ${message}`);
    if (error && this.isVerbose) {
      if (error instanceof Error) {
        this.appendLine(`  ${error.stack || error.message}`);
      } else {
        this.appendLine(`  ${String(error)}`);
      }
    }
  }

  public debug(message: string): void {
    if (this.isVerbose) {
      this.appendLine(`[DEBUG] ${message}`);
    }
  }

  public show(preserveFocus = true): void {
    this.channel.show(preserveFocus);
  }

  public clear(): void {
    this.channel.clear();
  }

  public dispose(): void {
    this.channel.dispose();
  }
}
