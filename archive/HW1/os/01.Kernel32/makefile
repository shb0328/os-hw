all: VirtualOS.bin KernelLoader.bin 



VirtualOS.bin: VirtualOS.asm

	nasm -o VirtualOS.bin VirtualOS.asm


KernelLoader.bin: KernelLoader.asm

	nasm -o KernelLoader.bin KernelLoader.asm



clean:

	rm -f VirtualOS.bin
	rm -f KernelLoader.bin


