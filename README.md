# TwoPassMemoryProfiler - Deepmap (Underdevelopment)
A holistic memory access profiling approach to enable a better understanding of program-system memory interactions. 
We have developed a two-pass tool adopting fast online and slow offline profiling, with which we have profiled at 
the variable/object level.
For details about the design and implementation of Two Pass Memory Profiler, please refer to the [publication].(https://dl.acm.org/citation.cfm?id=3126908.3126917)

Deepmap is the memory profiling tool in object-level. It shows how variables allocated dynamically are accessed in 
granularity of instruction by using Pin tool.
It profiles
- Temporal locality of variable
 - Spatial locality of variable
 - Read/write ratio of variable
 - Sequentiality of variable
 - Stride of variable
 - Size of variable
 - Lifetime of variable
 - Accessed volume of variable

It consists of three steps; fast pass, offline processing and slow pass.
(First fast pass)
-	In fast pass, Deepmap distinguishes variables with call-stack. It hashes the return addresses of allocation 
function calls.
(Offline processing)
-	In offline processing, users should collect the hash values and sizes of variables which users want to profile 
to respective files.
(Second slow pass)
-	In slow pass, Deepmap constitutes a hash table with given hash values and sizes. Then it sends the information 
to custom Pin code via message. Custom Pin code instruments the object-level analysis code between instructions and
gives profiling results as a file.

(Steps for running Deepmap)

Step 1: Clone Deepmap

Step 2: Downloaded pin-tool version 2.14

Step 3: Go to ‘NVMallocLibrary’ directory and build (this will make necessary object file of libnvm.a)

-	(In ‘Deepmap/NVMallocLibrary/’) make

Step 4: Go to ‘calltrack’ directory and build

-	(In ‘Deepmap/calltrack/’) make

Step 5: Change path in ./bin/Config to your current Deepmap directory and copy the config file to home directory.

-	(In ‘Deepmap/’) echo  DEEPMAP_REPO=`pwd`  >  ./bin/Config

-	(In ‘Deepmap/bin/’) cp  Config  ~/

Step 6: Edit deepmap/pin-tools/Config/makefile.unix.config. 
Find PIN_ROOT variable and change its value from 
‘/home/matrix/Softwares/pin-2.14-71313-gcc.4.4.7-linux/source/tools/Utils/testGccVersion:’ to ‘your pin tool directory’

-	(In ‘Deepmap/pin-tools/Config/makefile.unix.config’) 
Change PIN_ROOT from ‘/home/matrix/Softwares/pin-2.14-71313-gcc.4.4.7-linux’ 
to ‘/home/Path_to_IntelPintool’

Step 7: Go to ‘pin-tools’ directory and build

-	(In ‘Deepmap/pin-tools/’) make

Step 8: (For example) Compile the ‘example.c’

-	(In ‘Deepmap/calltrack/’) gcc  -pg  -rdynamic  example.c  -L.  -lcalltrack  -o  example

Step 9: Run the fast pass on your target application (absolute path is recommended target application)

-	(In ‘Deepmap/’) ./bin/prefast  /home/Path-to-deepmap-directory/Deepmap/calltrack/example

Step 10: Pick your target variables to profile. Create ‘HashValues’ and ‘VarMinSize’ files to store hash values and sizes of 
only target variables.

-	(In ‘Deepmap/’) python extract.py

-	(In ‘Deepmap/HashValues and Deepmap/VarMinSize) select the target variables which you want to profile and remove the others 
in both files.

Step 11: Run the slow pass on your target application (for example, ‘Deepmap/calltrack/example.c’)

-	(In ‘Deepmap/’) ./bin/preslow  /home/Path_to_IntelPintool/linux/intel64/bin/pinbin  -t  /home/Path-to-deepmap-directory/Deepmap/pin-tools/obj-intel64/pintool.so  --  /home/Path-to-deepmap-directory/Deepmap/calltrack/example

Step 12: After successful run of slow Pass, you can see your profiling result in ‘result-all-major’ file
