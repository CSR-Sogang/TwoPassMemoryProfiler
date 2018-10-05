__TwoPassMemoryProfiler - Deepmap__
====================================
A holistic memory profiling approach to enable a better understanding on memory access patterns of application. 


Introduction
-------------
Deepmap is the memory profiling tool in object-level. It shows how variables and objects are allocated dynamically are accessed in granularity of instruction by using Pin tool.
* * *
It profiles
 - Temporal locality of variable
 - Spatial locality of variable
 - Read/write ratio of variable
 - Sequentiality of variable
 - Stride of variable
 - Size of variable
 - Lifetime of variable
 - Accessed volume of variable


Components
-----------
Profiling consists of two online processings, which are fast pass and slow pass respectively. Between two passes, programmers should trim the data to profile in offline.
* * *
+ __First fast pass__
-	In fast pass, Deepmap distinguishes variables with call-stack. It hashes the return addresses of allocation function calls.
+ __Offline processing__
-	In offline processing, users should collect the hash values and sizes of variables which users want to profile 
to respective files.
+ __Second slow pass__
-	In slow pass, Deepmap constitutes a hash table with given hash values and sizes. Then it sends the information to custom Pin code via message. Custom Pin code instruments the object-level analysis code between instructions and gives profiling results as a file.


building
---------

Step 1: Clone Deepmap repository

Step 2: Build *‘NVMallocLibrary’*

  (In *‘Deepmap/NVMallocLibrary/’*) make

Step 3: Build *‘calltrack’*

  (In *‘Deepmap/calltrack/’*) make

Step 4: Change path in *'./bin/Config'* to your current Deepmap directory and copy the config file to home directory.

  (In *‘Deepmap/’*) echo  DEEPMAP_REPO=`pwd`  >  ./bin/Config
  (In *‘Deepmap/’*) cp  ./bin/Config  ~/

Step 5: Edit *'Deepmap/pin-tools/Config/makefile.unix.config'*. Find *PIN_ROOT* variable and change its value from *‘/home/matrix/Softwares/pin-2.14-71313-gcc.4.4.7-linux/’* to *‘your_pin_tool_directory’*

  (In *‘Deepmap/pin-tools/Config/makefile.unix.config’*) Change *PIN_ROOT* from *‘/home/matrix/Softwares/pin-2.14-71313-gcc.4.4.7-linux’* to *‘/home/Path_to_IntelPintool’*

Step 6: Build *‘pin-tools’*

  (In *‘Deepmap/pin-tools/’*) make

Step 7: (For example) Compile the *‘example.c’*

  (In *‘Deepmap/calltrack/’*) gcc  -pg  -rdynamic  example.c  -L.  -lcalltrack  -o  example

Step 8: Run the fast pass on your target application (absolute path is highly recommended target application)

  (In *‘Deepmap/’*) ./bin/prefast  /home/Path-to-deepmap-directory/Deepmap/calltrack/example

Step 9: Pick your target variables to profile. Create *‘HashValues’* and *‘VarMinSize’* files to store hash values and sizes of 
only target variables.

  (In *‘Deepmap/’*) python offline_process_ex.py

Step 10: Run the slow pass on your target application (for example, *‘Deepmap/calltrack/example.c’*)

  (In *‘Deepmap/’*) ./bin/preslow  /home/Path_to_IntelPintool/intel64/bin/pinbin  -t  /home/Path-to-deepmap-directory/Deepmap/pin-tools/obj-intel64/pintool.so  --  /home/Path-to-deepmap-directory/Deepmap/calltrack/example

Step 11: After successful run of slow Pass, you can see your profiling result in *‘result-all-major’* file


Publication
------------
#### Ji, Xu, Chao Wang, Nosayba El-Sayed, Xiaosong Ma, Youngjae Kim, Sudharshan S. Vazhkudai, Wei Xue and Daniel Sánchez. “Understanding object-level memory access patterns across the spectrum.” SC (2017).
For details about the design and implementation of Two Pass Memory Profiler, please refer to the [publication].(https://dl.acm.org/citation.cfm?id=3126908.3126917)


Dependencies
-------------

  - Pin tool version 2.14
  - Python 2.7

