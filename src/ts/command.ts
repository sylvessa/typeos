export interface Command {
    name: string;
    syntax: string;
    description: string;
    run(args: string[]): void;
}
