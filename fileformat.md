# File formats
All the information below was resolved by myself using a hex editor and debugger. Therefore, the correctness is not guaranteed.
## CBF File format

### header with size 52 bytes (0x34)

| Size   |      Name      |  Note |
|----------|:-------------:|------:|
| 4 bytes |  signature 	 | 0x46474942 - "BIGF" 	Big File format |
| 4 bytes |  signature2	 |  0x4C425A01 -  LSB = isEncrypted/compressed |
| 4 bytes | fileSize |  the size of CBF file, in bytes |
| 4 bytes | unk |  - |
| 4 bytes |  CountOfFiles |  the count of files, packed in CBF |
| 4 bytes |  TableOfFilesOffset | the position of ToF, in bytes, from the start of file |
| 4 bytes | unk |  - |
| 4 bytes | sizeOf(TableOfFiles) |  - |
	
### The table of files structure
The size of a row is flexible and depends on the length of filename. Use the size of row members to move through the table.
#### Each row in table

| Size   |      Name      |  Note |
|----------|:-------------:|------:|
| 2 bytes	  | size of a row   |    Note: each row contains a file name|
|4 bytes 	| start offset | - |
| 0x10	|	  isCompressedWithLZW	|  - | 
| 0x14		|  size of file (first byte = key) | - |
| 0x20	|	  state (1 - LZW compressed, 0 - uncompressed) | - | 
| 0x2A   |   offset	a NULL-terminated filename | - |
	
## LZW file
### header with size 12 bytes (0xC)
| Size   |      Name      |  Note |
|----------|:-------------:|------:|
| 4 bytes | 	magic number | 0x5D2E2E5B |
| 4 bytes	 | size - input | - |
| 4 bytes	 |	size - ouput | - |
	
