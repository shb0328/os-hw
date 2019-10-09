all: BootLoader.bin KernelLoader.bin


BootLoader.bin: BootLoader.asm

	nasm -o BootLoader.bin BootLoader.asm

KernelLoader.bin: KernelLoader.asm

	nasm -o KernelLoader.bin KernelLoader.asm

clean:

	rm -f BootLoader.bin
	rm -f KernelLoader.bin

