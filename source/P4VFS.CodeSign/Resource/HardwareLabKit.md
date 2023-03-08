## Virtual Hardware Lab Kit (VHLK) Setup  
1. Enable Hyper-V  
   https://learn.microsoft.com/en-us/windows-hardware/test/hlk/getstarted/getstarted-vhlk
1. Create a new external virtual switch  
   https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/get-started/create-a-virtual-switch-for-hyper-v-virtual-machines
1. Obtain a local copy of the VHLK from the Microsoft Eval Center  
   **Virtual HLK (VHLK) for Windows 10, version 1903**  
   https://www.microsoft.com/en-us/evalcenter/evaluate-virtual-hardware-lab-kit
   
1. Create a new virtual machine in Hyper-V.  
   * 4096MB. 
   * Local HLK VHDX harddisk
   * 2 virtual processors
1. Create an Administrator user named 'HLKAdminUser' with password from VHLK documentation. Change this password and store in KeyVault secret.
1. The VM will then run some HLK setup on first boot. **Do not rename the VM**

## Domain Join VM
1. Settings -> System -> About -> Join a domain

## Add a Domain User to the HLK
1. nce domain-joined, log in as .\HLKAdminUser
1. Open Management Console
1. Expand the datastore with the same name as the VM
1. users -> New User
1. Enter your "domain\username" in the top textbox and select "hlk_DSOwners"
1. Now repeat steps for the WTTIdentity datastore

## HLK studio builds
1. Install the the standalone HLK studio  
   https://learn.microsoft.com/en-us/windows-hardware/test/hlk/user/install-standalone-hlk-studio
   
1. Choose installation for Windows 10, version 1903. Select to install "Controller + Studio"
   https://go.microsoft.com/fwlink/?linkid=2086002