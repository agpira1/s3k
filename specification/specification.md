# Specification for implementation of stack canaries in S3K. 

## What this project aims to protect against.
Buffer overflow attacks utlilize bugs commonly found in non-memory safe code. Using functions that read or write data from memory that haven't been limited to set bounds enables reading or writing to a larger part of memory than intended. This can give an attacker power over the controlflow of a process as well as remote code execution by writing their own code to the memory before it is executed. Because of it's severity several different ways to protect against this type of attack has been created, usually used together to increase the security of the operative system. The goal is to protect the OS as the process can't be seen as trusted space. 

## What is a stack canary?
Stack canaries are on of many measures that can be used as buffer overflow protection. Canaries are a space filled with a value known to a monitor. 
When the value changes during runtime the monitor knows that a behaviour not inherent to the program has happened and can act to protect the operative system, usually terminating the process. 
There are different types of canaries used to protect the stack.

## Terminator canaries:
Terminator canaries act to protect through the knowledge that most buffer overflow attacks use string operations which end at string terminators. The space allocated to the canary is filled with string terminators such as null terminators, carriage return, page break etc. This will interrupt reading operations such as strcpy() when reading past the strings allocated space from the stack. 

## Random canaries:

Random canaries fill the allocated space on the stack with a randomized value. The value is often gathered from entropy to make knowing the value implausible. The canary is a secure value only known to the function that needs to know it which is the stack canary monitor. To bypass this type of canary the attacker must find a way to read from the stack and then place the correct canary back as the stack is overwritten. 

## Random XOR canaries:

Random XOR canaries are random canaries with extra security implemented to make overwriting the canary more difficult. These canaries use an XOR-algorithm applied to a larger part of the control data together with the canary value when doing the comparison. Effectively forcing the attacker to read a far larger part of memory before being able to interfere with the control-flow. 

## Project idea. 

The project will encompass implementing random canaries into the control-flow of the S3K kernel. 

### Likely steps needed for the canary:

Entropy for a randomized number and creation of number on process start.\
Allocation and writing of the canary to each function on the stack before the stackpointer and returnaddress.\
Canary monitor placed inside of a reasonable handler that already interrupts the process-flow, or creating a new hook at certain actions where the canary monitor can check the value.\
Error handling for when the monitor deems the function tampered with and some label to see why the function exited.


It would be reasonable for us to edit some parts of the processor code that already hooks at good moments and add our own code. Instead of creating an unnecessarily complex workflow. 

The canary function generates one canary string per process and then reuse it in every function to keep the overhead small. The string is created on process start. It would be reasonable to add our code to the process start and declaration function that sets up the process and PCB. 







Sources:
https://en.wikipedia.org/wiki/Buffer_overflow_protection