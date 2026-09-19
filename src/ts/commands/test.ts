// testing real npm modules
import isNumber from "is-number";

export function test(args: string[]) {
    const text = args.join(" ");
    typeos.print("isNumber(" + text + ") = " + (isNumber(text) ? "true" : "false") + " \n", "cyan");
}
