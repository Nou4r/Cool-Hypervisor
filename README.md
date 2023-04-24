# Cool-Hypervisor
A intel hypervisor, implementing many virtualization techniques

# Features
This hypervisor implements quite some things:
-> Basic virtualization of its environment

-> Ept are implemented

-> Ept hooks are implemented for both Um and Km

-> Syscall hooks are implemented (ty Hypervisor from Scratch)

-> Vmcall handler with read/write using physical memory

-> GetModuleBase without attaching to a process using only Processname and Modulename

-> Handling of all important vmexits 

-> Own Idt for Hypervisor (ty Mr Jono also for ldasm.cpp)

# Things to add for people wanting to extend this

-> Own gdt for host

-> Implement the hell that is apic virtualization

-> Add support to make this manually mappable
  . At the moment this driver must be loaded either through test signing or dse flip
  
-> Extend the usermode part of the hypevisor, also if you want to utilize the usermodepagehook you have to be in the same address space as the function to be hooked
  
# Lets call them unsolved problems

->The GetModuleBaseFunction:
I know, I know, this is UGLY. For anyone wondering, I have implemented a pending operation system in my vmcall handler for the GetModuleBase function. Because of this there is a sleep statement in the usermode part of the hypervisor when trying out the getmodulebase part
Why?: When trying to get the Module base in my Vmcallhandler the copying of the module name didn't work, feel free to dm me so that I can fix this issue.
I would be glad to do so, and I would also like to know why this could be the case. 

# Final notes

This project has been started in late August last year, and I have continued to develop it further until this point. I kinda want to try new things and maybe reverse some acs. 
This is btw not what this hypervisor has been designed for; It is far to vulnerable for that... 
I know it may sound weird, but in some way this project has helped me through some mentally hard times. Didn't think programming could do that

Dm me in case you have any question or remarks about my project. Constructive feedback is also welcome and wanted (;.
Cooler TYp#0995
