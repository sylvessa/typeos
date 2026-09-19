import type { Command } from "../command";

export function help(args: string[], commands: Command[]) {
    const sorted = [...commands].sort((a, b) => a.name.localeCompare(b.name));

    if (args.length === 0) {
        const nameW = Math.max(...sorted.map((c) => c.name.length));

        typeos.print("\navailable commands:\n\n", "light-cyan");
        for (const c of sorted) {
            typeos.print("  " + c.name.padEnd(nameW + 2) + c.description + "\n", "white");
        }
        typeos.print("\ntype 'help <command>' for details.\n", "dark-gray");
        return;
    }

    const name = args[0];
    const cmd = commands.find((c) => c.name === name);
    if (!cmd) {
        typeos.print(`no such command: '${name}'\n`, "light-red");
        return;
    }

    typeos.print(`\n${cmd.name}\n`, "light-cyan");
    typeos.print(`  usage: ${cmd.syntax}\n`, "white");
    typeos.print(`  ${cmd.description}\n`, "dark-gray");
}
