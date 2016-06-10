# vietcong-unpacker

A collection of tools, used for unpacking .CBF files employed in Vietcong (published by Pterodon).

## Motivation
A .CBF file, which stands for (compressed?) binary file in my opinion, is a part of the game file system, containing all game assets. As the file documentation is not available for public, reverse engineering must be employed and low-level data handling facilities need to be created to process CBF files.

## CBF in general

As for the FS (File system) in Vietcong, it fullfills needs for data compression and obfuscation.

Archived files might be stored in plaintext form, in an "encrypted" form or compressed using LZW compression with dynamic size of code.

Similar to other file types, CBF files are divided into a header, a table of content and the data itself. In addition, the table of content may be encrypted, depending on the status flag, stored in the header. 

