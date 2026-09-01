# Specification for implementation of stack canaries in S3K. 
## What is a stack canary?
Stack canaries are used as buffer overflow protection of the stack by operative systems. Canaries are a space filled with a value known to a monitor. 
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

### Steps needed for the canary:

Entropy for a randomized number. 
Allocation and writing of the canary to each function on the stack before the stackpointer and returnaddress. 
Canary monitor placed inside of a reasonable handler that already interrupts the process-flow, or creating a new hook at certain actions where the canary monitor can check the value. 
Error handling for when the monitor deems the function tampered with. 
















Sources:
https://en.wikipedia.org/wiki/Buffer_overflow_protection