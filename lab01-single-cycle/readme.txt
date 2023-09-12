***********************************************************
Lab 1

Group: (Please put your name and netid here)

Student1 (netid1)
Student2 (netid2)

Reference: (Include the reference you use)


***********************************************************

This is the directory for students. 

It should contain these following files:

.
├── MIPS.cpp
├── Makefile
├── dmem.txt  (sample initialized dmem)
├── imem.txt  (sample MIPS program)
├── lab1.pdf
├── readme.txt
└── expected_results
    ├── RFresult_ans.txt
    └── dmemresult_ans.txt

You need to implement the functions in MIPS.cpp 
when you see "TODO: implement". You DO NOT need to 
modify anything else.

You can use the sample we provide to test your result.

Tips: follow these steps when you have no idea how to start.
1. implement the MIPS.cpp and fill out the code
2. compile your code with command such as `make`, and generate 
   executable.
        if you know nothing about Makefile, you can try to  
        read https://earthly.dev/blog/g++-makefile/
3. run executable with command such as `./MIPS`, it should 
   take the imem.txt and dmem.txt and run, then you should 
   check your output result files.


We suggest compiling on a Linux machine.

Compiling on Windows:
Use something like Windows Powershell, but you should be
careful whether your executable is ./MIPS or ./MIPS.exe 

Compiling on Mac:
We recommend using homebrew to install g++. You can run
'brew install gcc' which will include g++. Check out this 
link for more details: https://stackoverflow.com/a/75191179

Before reaching TAs about any compiling issues, I suggest you 
to search stackoverflow such as "how to set up g++ make on mac", 
"How to run g++ compile on Windows", "What is MinGW and how to 
install make". There are many useful answers.

We also encourage to discuss with others about compiling or 
platform issues. If you are still unable to fix your problem, 
please reach us through the office hour.

***********************************************************
