# 🔗 DLL Merger
A simple tool that embeds DLLs into Windows 32-bit PE executables, along with a loader that manually maps the DLLs into memory instead of using `LoadLibrary`, making the loaded DLLs invisible in the PEB (Process Environment Block).

# Principle of merging
![executable before merging](https://raw.githubusercontent.com/ytk2128/dll-merger/refs/heads/main/doc/before.svg)
![executable after merging](https://raw.githubusercontent.com/ytk2128/dll-merger/refs/heads/main/doc/after.svg)

# How to build
1. ```git clone https://github.com/ytk2128/dll-merger.git --recurse-submodules```
2. Open **src/merger.sln**
3. Build the solution

# Demonstration
1. Run ```merger.exe procexp.exe MyDLL.dll```
2. Execute the created ```procexp.exe_out.exe```
3. Note that ```MyDLL.dll``` is invisibly loaded in the ```procexp.exe_out.exe```
<img width="80%" src="https://user-images.githubusercontent.com/60180255/152683217-81a0c00f-8a66-4659-81c4-91ba8ec4817a.PNG"/>
