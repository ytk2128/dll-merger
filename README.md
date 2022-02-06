# 🔗 dll-merger
Merging DLLs with a PE32 EXE without LoadLibrary.

# Building the project
* ```git clone https://github.com/ytk2128/dll-merger.git --recurse-submodules```
* Open **src/merger.sln**
* Build Solution

# Principle of merging
dll-merger merges DLLs with one PE32 EXE and injects the loader code into the PE32 EXE and the injected loader loads DLLs manually without LoadLibrary and thus the loaded DLLs are invisible in the PEB.

![executable before merging](https://user-images.githubusercontent.com/60180255/152682145-3c217853-daf0-4174-a6cd-17fbf1662e20.svg)
![executable after merging](https://user-images.githubusercontent.com/60180255/152682142-6a587520-7208-4b91-ae22-4dc32558d8c7.svg)

# Demonstration
```MyDLL.dll``` is loaded in the Process Explorer but it's invisible.
<img width="80%" src="https://user-images.githubusercontent.com/60180255/152683217-81a0c00f-8a66-4659-81c4-91ba8ec4817a.PNG"/>
