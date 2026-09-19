export {};

declare global {
    type Color =
        | "black"
        | "blue"
        | "green"
        | "cyan"
        | "red"
        | "magenta"
        | "brown"
        | "gray"
        | "dark-gray"
        | "light-blue"
        | "light-green"
        | "light-cyan"
        | "light-red"
        | "light-magenta"
        | "yellow"
        | "white";

    interface TypeOS {
        readonly version: string;

        print(text: string, color?: Color): void;
        clear(): void;
        setCursor(col: number, row: number): void;
        getCursor(): [number, number];
        readline(): string;
        width(): number;
        height(): number;
    }

    const typeos: TypeOS;
}
