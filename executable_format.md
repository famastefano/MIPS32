# Executable File Format

| Field | Size (bytes) | Offset | Info |
|:-----:|:------------:|:------:|:----:|
| magic        | 4 |         0  | string `fama` in lowercase        |
| version      | 4 |         4  | executable format version         |
| .data_sz     | 4 |         8  | length of .data section in bytes  |
| .text_sz     | 4 |         12 | length of .text section in bytes  |
| .kdata_sz    | 4 |         16 | length of .kdata section in bytes |
| .ktext_sz    | 4 |         20 | length of .ktext section in bytes |
| .data_addr   | 4 |         24 | start address of .data section    |
| .text_addr   | 4 |         28 | start address of .text section    |
| .kdata_addr  | 4 |         32 | start address of .kdata section   |
| .ktext_addr  | 4 |         36 | start address of .ktext section   |
| .data        | .data_sz  | 40 | .data section  |
| .text        | .text_sz  | -- | .text section  |
| .kdata       | .kdata_sz | -- | .kdata section |
| .ktext       | .ktext_sz | -- | .ktext section |

It is important to note that this library loads a file _from memory_,
this means that you can have additional data before the `magic` field and after the `.ktext` field.
