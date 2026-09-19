# Type(script)OS

a little operating system where typescript is the user interface. QuickJS runs on top of a thin freestanding C kernel.

weird idea i got in a dream. this is so shit.

this is way more simpler but also vastly weirder than shiggy. shiggy will always remain my main project where i do genuine lowl evel system works. if you havent seen shiggy [CHECK IT OUT NOW](https://github.com/sylvessa/shiggy)

## what it looks like

run `make run` ok

<img width="796" height="439" alt="image" src="https://github.com/user-attachments/assets/12ca2240-e7ae-417c-b0d6-ad1d0a33c9ef" />

## how tho

same kinda thing as shiggy, bootloader is hand written

| Layer             | Location              | Purpose                                       |
| ----------------- | --------------------- | --------------------------------------------- |
| TypeScript bundle | `src/ts/*.ts`         | the interface                                 |
| QuickJS           | `third_party/quickjs` | the friggin javascript thing                  |
| QuickJS <- kernel | `src/kernel/js.c`     | lets js talk to the kernel                    |
| C kernel          | `src/kernel/*.c`      | necessary kernel shit                         |
| Bootloader        | `src/boot/*.asm`      | real -> protected mode and loading the kernel |

### boot path

1. `src/boot/boot.asm` is the 512 byte boot sector. starts in real mode, sets up a stack, then reads the kernel from the floppy using BIOS `int 0x13` CHS reads
2. kernel is around 800 KB and real mode cant directly get to where it needs to go, so it reads it in 32KiB chunks into a bounce buffer at `0x80000`. `pm_copy.asm` switches to protected mode and `rep movsd`s each chunk up to `0x100000`
3. `src/kernel/entry.asm` (`_start`) sets up the stack and calls `kmain`
4. `kmain` in `src/kernel/kernel.c` brings up serial, VGA, PIC/IDT, and the heap, then calls `js_run()`
5. `js_run` in `src/kernel/js.c` makes the QuickJS runtime/context, adds the `typeos` thing, and runs the embedded typescript bundle

## ok but wat about typescript

bun builds the bundle and `tools/embed.ts` turns it into a C array that gets linked into the kernel

`src/ts/main.ts` does the banner and readline loop. commands are in `src/ts/commands/`

the `typeos` host object is declared in `src/ts/host.d.ts` and lets the ts code write to VGA, clear the screen, and read a line

@orisrightpaw Am i torturing you enough

## memory layout

|    Address | Region                |
| ---------: | --------------------- |
|  `0x00000` | real mode IVT/BIOS    |
| `0x100000` | kernel image          |
|            | around 1.1 MB of code |
| `0x400000` | `HEAP_END`            |
| `0x800000` | `STACK_TOP`           |
|            | 32 MB RAM (`-m 32`)   |

## BUILDING

you need:

- `make`
- `nasm`
- `bun`
- `qemu-system-i386`
- `i386-elf` gcc + binutils

same as shiggy, you can just do `make toolchain` if you dont have the cross compiler stuff

## RUNNING

```sh
make run
```

ok thats it
