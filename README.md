
# Capere
An Apple Silicon hooking library written in C

## Installation & Tutorial

Copy the "Capere.h" file and paste it into your project folder. In your main project file `#include "Capere.h"` to begin using Capere. For example usage and documentation, read below and the example "Tweak.x" file.

### The Capere Struct

Below is the Capere struct. The function pointers correspond to their respective and similarly named functions which are assigned in the `CapereInit()` function.

```
typedef struct Capere {
    instruction_t instructions[4];
    instruction_t save_instructions[4];
    uint64_t address;
    uint64_t detour_address;
    size_t size;
    capere_return_t (*Hook)();
    capere_return_t (*HookRestore)();
    capere_return_t (*HookOriginal)();
    capere_return_t (*Patch)();
    capere_return_t (*Restore)();
    char* (*ErrorString)();
} Capere;
```

### Initializing Capere

The `CapereInit()` function is used for declaring a Capere struct. The `CapereInit()` function takes two arguments, the address of the function being hooked and the address of the detour function. The return value is a pointer to the struct. Declaration:

`Capere* CapereInit(uint64_t address_init, void* detour_function_init)`

Example:

`Capere* HookExample = CapereInit(0x10badc0de, hook_function);`

# Documentation

The documentation documents everything in the

## Initialization

### CapereInit()

Official declaration:
```
Capere CapereInit(
uint64_t address_init,
void* detour_function_init)
```

- RETURN TYPE: Capere
- `uint64_t address_init` - address of the function that will be hooked inside of the binary
- `void* detour_function_init` - function inside of the injected library where code will be redirected

#### Notes:
- The image slide will be added to `address_init` automatically inside of `CapereInit()`

## Functions

### Capere->Hook()

Official declaration:

```
capere_return_t Hook(
Capere* capere)
```
- RETURN TYPE: capere_return_t (int)
- Capere* capere - pointer to capere struct

#### Notes:
`Capere->Hook()` will place a hook at `Capere->address` to the function at `Capere->detour_address`. See `CapereInit` for initialization of these variables. The original instruction is stored in `*Capere->save_instructions`.

### Capere->HookRestore()

Official declaration:

```
capere_return_t HookRestore(
Capere* capere)
```
- RETURN TYPE: capere_return_t (int)
- Capere* capere - pointer to capere struct

#### Notes:
`Capere->HookRestore()` will remove the branch instruction and restore the original instructions stored in `*Capere->save_instructions` at `Capere->address`

### Capere->HookOriginal()

Official declaration:

```
capere_return_t HookOriginal(
Capere* capere)
```
- RETURN TYPE: capere_return_t (int)
- Capere* capere - pointer to capere struct

#### Notes:
`Capere->HookOriginal()` will remove the branch instruction and restore the original instructions stored in `*Capere->save_instructions` at `Capere->address` When `Capere->HookOriginal()` is called, execution will be redirected to `Capere->address`. Since execution is redirected to the main program, the function should be called once the hooked function code is executed.

### Capere->Patch()

Official declaration:

```
capere_return_t Patch(
Capere* capere)
```
- RETURN TYPE: capere_return_t (int)
- Capere* capere - pointer to capere struct

#### Notes:
`Capere->Patch()` will write `*Capere->instructions` to `Capere->address`. `*Capere->instructions` should be 4 byte aligned and `Capere->size` should be assigned accordingly. `*Capere->instructions` should be a `uint8_t` array, such as `{0x1D, 0x20, 0x03, 0xD5}` (NOP in ARM assembly). The original instructions are stored in `*Capere->save_instructions` and can be used by `Capere->Restore()` to remove the patch.

### Capere->Restore()

Official declaration:

```
capere_return_t Restore(
Capere* capere)
```
- RETURN TYPE: capere_return_t (int)
- Capere* capere - pointer to capere struct

#### Notes:
`Capere->Restore()` will write `*Capere->save_instructions` to `Capere->address`.  It will restore whatever instructions were originally stored by `Capere->Patch()`


#### Capere->ErrorString()

Official declaration:
```
char* ErrorString(
capere_return_t return_value)
```
- RETURN TYPE: char*
- capere_return_t return_value - return value for Capere->* functions.

#### Notes:
`Capere->ErrorString()` will return a detailed error string corresponding to the return value of Capere->* functions. The declarations of these return values can be found in `Capere.h`
