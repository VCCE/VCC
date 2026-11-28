# Usage

### Launch.json Options

| Key | Value |
|----------|--------|
| type | Which debugger extension to use, which is `vcc` for our purposes. |
| request | The kind of launch request. `attach` means it will attempt to attach to a running VCC process. Other debuggers support `launch`, but this one does not. |
| name | It's just the label shown in the Run & Debug dropdown; can be anything you like. |
| port | The socket port to attach to, which for VCC is 6666. |
| paste | Optional. A string to 'type' (paste) into VCC prior to debugging to start. Some projects may launch their program via a basic loader, so you can specify `run\"loader\"\n` or similar. Double quotes must be escaped with a backslash. The `ENTER` key is represented by `\n`. |
| entryPoint | Optional. The debugger will stop on entry at this address. Specify it in hexadecimal, starting with a `$` sign, e.g. `$2000`. When used with `paste`, this option will automatically run VCC at its fastest speed until it reaches the entry point address. |
| reset | Optional. Reset VCC on attach. If `paste` has a value, it will be pasted after 30 frames to allow time for DOS/ECB to initialize. VCC will be reset when Visual Studio Code disconnects from the debugging session, ready for the next one. |
| program | Optional. If specified, the binary will be loaded directly into memory. Version 2.1.10 supports only DECB binaries. |
| listings | Required in version 2.1.10. Should point to a listing file produced by lwasm for your binary. This corresponds to the `--list=x` option. |
| preLaunchTask | Optional. Specifies a task label from `tasks.json` to run before the debugging session starts. This is useful for compiling the project in preparation for debugging before you press F5 to start. |

### Keyboard Shortcuts

These are the standard Visual Studio debugging keys.

<div align="center">

| Shortcut | Action |
|----------|--------|
| `F5` | Start debugging / Continue (resume after pause) |
| `Shift+F5` | Stop debugging |
| `F9` | Toggle breakpoint on current line |
| `F10` | Step over (next line, don't enter functions) |
| `F11` | Step into (enter the function call) |
| `Shift+F11` | Step out (finish current function, return to caller) |
| `Ctrl+Shift+Y` | Open Debug Console |
| `Ctrl+Shift+D` | Open Run and Debug sidebar |

</div>   



<hr>
<p align="center">
  <sub><sup>Written by Craig Allsop · Updated 2026-11-27 · <a href="#">Back to top</a></sup></sub>
</p>
