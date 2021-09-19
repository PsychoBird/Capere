uint64_t original_function_address = 0x10badc0de;

void hook_function(int arg0, int arg1) {
    NSLog(@"Branched to hook function! Run your own code here...");
    //reminder that arguments correspond to the first few registers!

    Capere* HookExample = CapereInit(original_function_address, hook_function);

    /*
    let's return to the original function and remove the hook.
    useful for one time injection
    HookOriginal will redirect execution back to the original instruction without returning
    */
    HookExample->HookOriginal(HookExample);
}

void install_hook() {
    capere_return_t cret = 0;
    Capere* HookExample = CapereInit(original_function_address, hook_function);

    //install hook
    cret = HookExample->Hook(HookExample);
    if (cret != HOOK_SUCCESS)
        NSLog(@"%s", HookExample->ErrorString(cret));

    //remove hook
    HookExample->HookRestore(HookExample);
    if (cret != HOOK_SUCCESS)
        NSLog(@"%s", HookExample->ErrorString(cret));
}


void install_patch() {
    Capere *HookExample = CapereInit(original_function_address, hook_function);

    //ASM: NOP (1F2003Dt)
    instruction_t instructions[HOOK_SIZE] = {0x1F, 0x20, 0x03, 0xD5};
    memcpy(&HookExample->instructions, &instructions, HOOK_SIZE);

    //patch NOP at original_function_address
    capere_return_t cret = HookExample->Patch(HookExample);
    if (cret != HOOK_SUCCESS)
        NSLog(@"%s", HookExample->ErrorString(cret));

    //restore original instructions at that address
    capere_return_t cret = HookExample->Restore(HookExample);
    if (cret != HOOK_SUCCESS)
        NSLog(@"%s", HookExample->ErrorString(cret));
}
