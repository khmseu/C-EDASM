#!/usr/bin/env python3
"""
Helper script to run C-EDASM test_asm and save binary output
Parses hex dump from test_asm output and writes binary file
"""

import sys
import subprocess
import re

def assemble_and_save(src_file, output_file):
    """Run test_asm and save binary output"""
    
    # Run test_asm
    try:
        result = subprocess.run(
            ['./build/test_asm', src_file],
            capture_output=True,
            text=True,
            check=False
        )
    except FileNotFoundError:
        print("Error: ./build/test_asm not found. Run ./scripts/build.sh first.")
        return False
    
    # Check if assembly succeeded
    if 'FAILED' in result.stdout:
        print("Assembly FAILED")
        print(result.stdout)
        return False
    
    if 'SUCCEEDED' not in result.stdout:
        print("Unexpected output from test_asm")
        print(result.stdout)
        return False
    
    # Parse hex dump
    # Format: "800: a9 00 8d 00 04 ..."
    hex_pattern = re.compile(r'^[0-9a-f]+:\s+((?:[0-9a-f]{2}\s*)+)', re.MULTILINE | re.IGNORECASE)
    matches = hex_pattern.findall(result.stdout)
    
    if not matches:
        print("Error: No hex dump found in output")
        print(result.stdout)
        return False
    
    # Collect all bytes
    code_bytes = []
    for match in matches:
        hex_bytes = match.strip().split()
        for hex_byte in hex_bytes:
            code_bytes.append(int(hex_byte, 16))
    
    # Write binary file
    try:
        with open(output_file, 'wb') as f:
            f.write(bytes(code_bytes))
        
        print(f"✓ Assembly successful: {len(code_bytes)} bytes written to {output_file}")
        return True
    
    except IOError as e:
        print(f"Error writing output file: {e}")
        return False

def main():
    if len(sys.argv) != 3:
        print("Usage: assemble_helper.py <source.src> <output.bin>")
        sys.exit(1)
    
    src_file = sys.argv[1]
    output_file = sys.argv[2]
    
    success = assemble_and_save(src_file, output_file)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
