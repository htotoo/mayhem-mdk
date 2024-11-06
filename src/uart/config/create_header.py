#
# Copyright (C) 2024 Bernd Herzog
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

### NOTE:       AI GENERATED CODE               ###
### to convert a binary file to a C header file ###

import sys
import os

def create_c_header(binary_file, header_file):
    try:
        with open(binary_file, 'rb') as bf:
            binary_content = bf.read()
        
        variable_name = os.path.splitext(os.path.basename(binary_file))[0]
        header_content = f"unsigned char {variable_name}[] = {{\n"
        for i in range(0, len(binary_content), 16):
            line = ', '.join(f'0x{byte:02x}' for byte in binary_content[i:i+16])
            header_content += f"    {line},\n"

        # Pad with zeroes if the size is not divisible by 128
        remainder = len(binary_content) % 128
        if remainder != 0:
            padding_size = 128 - remainder
            header_content += f"    {', '.join('0x00' for _ in range(padding_size))},\n"
        
        header_content = header_content.rstrip(',\n') + "\n};\n"
        
        with open(header_file, 'w') as hf:
            hf.write(header_content)
        
        print(f"Header file '{header_file}' created successfully.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python create_include.py <binary_file> <header_file>")
    else:
        binary_file = sys.argv[1]
        header_file = sys.argv[2]
        create_c_header(binary_file, header_file)
