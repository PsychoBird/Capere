#ifndef CAPERE_H
#define CAPERE_H

#include <stdio.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>

typedef uint8_t instruction_t;
typedef int capere_return_t;

#define SAVE_INSTRUCTION_SUCCESS 0
#define SAVE_INSTRUCTION_FAILURE 1
#define PROTECT_HOOK_FAILURE 2
#define WRITE_INSTRUCTION_SUCCESS 3
#define WRITE_INSTRUCTION_FAILURE 4
#define HOOK_SUCCESS 5
#define HOOK_FAILURE 6
#define HOOK_OFFSET_TOO_LARGE 7
#define HOOK_BRANCH_FAILURE 8

#define BRANCH_ARM {0x00, 0x00, 0x00, 0x14}
#define BRANCH_ARM_SIZE 4
#define HOOK_SIZE BRANCH_ARM_SIZE

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
} *Capere;

/*
return an error string for a specific return value
*/
char *capere_error_string(capere_return_t return_value) {
    switch (return_value) {
        case 0:
            return "(capere) save instruction success";
        case 1:
            return "(capere) save instruction failure";
        case 2:
            return "(capere) vm_protect failure";
        case 3:
            return "(capere) write instruction success";
        case 4:
            return "(capere) write instruction failure";
        case 5:
            return "(capere) hook success";
        case 6:
            return "(capere) hook failure";
        case 7:
            return "(capere) hook offset too big";
        case 8:
            return "(capere) hook branch back failure";
    }
    return "(capere) unknown error";
}


/*
Calculate the image slide of the current application
*/
uint64_t real_address(uint64_t address) {
    return address + _dyld_get_image_vmaddr_slide(0);
}

/*
vm_write wrapper to write data to an address
*/
capere_return_t write_region(uint64_t address, vm_size_t size, void *instructions) {
    kern_return_t kret;
    //prep for write instruction
    kret = vm_protect(mach_task_self(), address, size, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
    if (kret != KERN_SUCCESS) {
        return PROTECT_HOOK_FAILURE;
    }
    //write instructions
    kret = vm_write(mach_task_self(), address, (vm_offset_t) instructions, size);
    //set back original instructions
    vm_protect(mach_task_self(), address, size, false, VM_PROT_READ | VM_PROT_EXECUTE);
    if (kret != KERN_SUCCESS) {
        return WRITE_INSTRUCTION_FAILURE;
    }
    return WRITE_INSTRUCTION_SUCCESS;
}

/*
vm_read_overwrite wrapper to save the instructions of a region
*/
capere_return_t save_region(uint64_t address, vm_size_t size, void *save_instructions) {
    kern_return_t kret;
    kret = vm_read_overwrite(mach_task_self(), address, size, (vm_offset_t) save_instructions, &size);
    if (kret != KERN_SUCCESS) {
        return SAVE_INSTRUCTION_FAILURE;
    }
    return SAVE_INSTRUCTION_SUCCESS;
}

/*
write_region and save_region wrapper to store the instructions at a specific address
*/
capere_return_t capere_instruction_patch(uint64_t address, vm_size_t size, void *save_instructions, void *instructions) {
    capere_return_t cret;
    cret = save_region(address, size, save_instructions);
    if (cret != SAVE_INSTRUCTION_SUCCESS) {
        return SAVE_INSTRUCTION_FAILURE;
    }
    cret = write_region(address, size, instructions);
    if (cret != WRITE_INSTRUCTION_SUCCESS) {
        return WRITE_INSTRUCTION_FAILURE;
    }
    return HOOK_SUCCESS;
}

//abstraction for capere_instruction_patch
capere_return_t capere_instruction_patch_abs(Capere capere) {
    return capere_instruction_patch(capere->address, capere->size, capere->save_instructions, capere->instructions);
}

/*
write region wrapper to restore an instruction patch
ultimately this is just used for abstraction
*/
capere_return_t capere_instruction_restore(uint64_t address, vm_size_t size, void *original_instructions) {
    capere_return_t cret;
    cret = write_region(address, size, original_instructions);
    if (cret != WRITE_INSTRUCTION_SUCCESS) {
        return WRITE_INSTRUCTION_FAILURE;
    }
    return HOOK_SUCCESS;
}

//abstraction for capere_instruction_restore
capere_return_t capere_instruction_restore_abs(Capere capere) {
    return capere_instruction_restore(capere->address, capere->size, capere->save_instructions);
}


/*
generate branch instruction
see inside of function for documentation on how generating a branch instruction works
*/
instruction_t* offset_to_branch_instruction(uint64_t address, uint64_t detour_address) {
    static instruction_t convert_array[4];
    uint32_t offset;
    int jump_byte = 0;
    /*
    use this for generating a jump instruction that's backwards.
    if the hook address is before address, we will need to use 0x17 instead of 0x14 to branch back
    relative jump will also count back from 0xF000000
    we divide by 4 to generate the amount of instructions the branch will "skip" over

    08 00 00 14 will branch forward two instructions
    08 00 00 17 will branch back two instructions
    */
    if (detour_address < address) {
        offset = (address - detour_address) / 4;
        offset = 0xF000000 - offset;
        jump_byte = 0x17;
    }
    else {
        offset = (detour_address - address) / 4;
        jump_byte = 0x14;
    }

    memcpy((instruction_t*)convert_array,(instruction_t*)&offset,sizeof(uint32_t));

    convert_array[3] = jump_byte;
    return convert_array;
}

/*
generate a hook at a specific address
*/
capere_return_t capere_hook(Capere capere) {
    capere_return_t cret;
    instruction_t *instructions = offset_to_branch_instruction(capere->address, capere->detour_address);
    vm_size_t size = BRANCH_ARM_SIZE;

    cret = capere_instruction_patch(capere->address, size, capere->save_instructions, instructions);
    if (cret != HOOK_SUCCESS) {
        return cret;
    }
    return HOOK_SUCCESS;
}

/*
restore original instructions and remove hook
*/
capere_return_t capere_hook_restore(Capere capere) {
    capere_return_t cret;
    vm_size_t size = BRANCH_ARM_SIZE;

    cret = capere_instruction_restore(capere->address, size, capere->save_instructions);
    if (cret != HOOK_SUCCESS) {
        return cret;
    }
    return HOOK_SUCCESS;
}

/*
capere_hook_original can be used for a 1 use "quick injection"
*/
capere_return_t capere_hook_original(Capere capere) {
    capere_return_t cret;
    vm_size_t size = BRANCH_ARM_SIZE;

    //place back original instructions
    cret = capere_instruction_restore(capere->address, size, capere->save_instructions);
    if (cret != HOOK_SUCCESS) {
        return cret;
    }

    //jump back to original function
    ((void (*)(void)) capere->address)();

    //we shouldn't be here!
    return HOOK_BRANCH_FAILURE;
}


Capere CapereInit(uint64_t address_init, void* detour_function_init) {
    Capere capere = malloc(sizeof(Capere));
    capere->address = real_address(address_init);
    capere->detour_address = (uint64_t) detour_function_init;
    capere->size = 4;
    capere->Hook = &capere_hook;
    capere->HookRestore = &capere_hook_restore;
    capere->HookOriginal = &capere_hook_original;
    capere->Patch = &capere_instruction_patch_abs;
    capere->Restore = &capere_instruction_restore_abs;
    capere->ErrorString = &capere_error_string;
    return capere;
}

#endif /* CAPERE_H */
