// Teh actual shell
import type { Command } from "./command";
import { help } from "./commands/help";
import { echo } from "./commands/echo";
import { test } from "./commands/test";

const commands: Command[] = [
    {
        name: "echo",
        syntax: "echo <text...>",
        description: "self explanatory",
        run: (args: string[]) => echo(args),
    },
    {
        name: "help",
        syntax: "help [command]",
        description: "lists available commands",
        run: (args: string[]) => help(args, commands),
    },
    {
        name: "test",
        syntax: "test",
        description: "test command",
        run: (args: string[]) => test(args),
    },
];

function center(text: string, width: number): string {
    const pad = Math.max(0, Math.floor((width - text.length) / 2));
    return " ".repeat(pad) + text;
}

function main(): void {
    typeos.clear();
    typeos.print(center(`typeos v${typeos.version}`, typeos.width()) + "\n", "light-cyan");
    typeos.print("\ntype 'help' for a list of commands.\n \n", "white");

    for (;;) {
        typeos.print("typeos> ", "green");

        const line = typeos.readline().trim();
        if (line === "") continue;

        const parts = line.split(/\s+/);
        const name = parts[0];
        const args = parts.slice(1);

        const cmd = commands.find((c) => c.name === name);
        if (!cmd) {
            typeos.print(`unknown command: '${name}' - type 'help'\n`, "light-red");
            continue;
        }

        try {
            cmd.run(args);
        } catch (err) {
            typeos.print(`error: ${String(err)}\n`, "light-red");
        }
    }
}

main();
